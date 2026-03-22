#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/diag.h"
#include "codegen/cgen.h"
#include "parser/parser.h"
#include "sema/sema.h"

static void print_usage(void) {
  puts("Usage: ngawic build <input.ngawi> [-o output] [-S]");
}

static bool has_ngawi_ext(const char *path) {
  const char *dot = strrchr(path, '.');
  return dot && strcmp(dot, ".ngawi") == 0;
}

static char *dup_with_suffix(const char *base, const char *suffix) {
  size_t n = strlen(base) + strlen(suffix) + 1;
  char *out = (char *)malloc(n);
  if (!out) return NULL;
  snprintf(out, n, "%s%s", base, suffix);
  return out;
}

static char *default_output_from_input(const char *input) {
  const char *dot = strrchr(input, '.');
  size_t stem_len = dot ? (size_t)(dot - input) : strlen(input);
  char *out = (char *)malloc(stem_len + 1);
  if (!out) return NULL;
  memcpy(out, input, stem_len);
  out[stem_len] = '\0';
  return out;
}

static char *read_file_all(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }

  long n = ftell(f);
  if (n < 0) {
    fclose(f);
    return NULL;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }

  char *buf = (char *)malloc((size_t)n + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t got = fread(buf, 1, (size_t)n, f);
  fclose(f);
  if (got != (size_t)n) {
    free(buf);
    return NULL;
  }

  buf[n] = '\0';
  return buf;
}

int main(int argc, char **argv) {
  if (argc < 3 || strcmp(argv[1], "build") != 0) {
    print_usage();
    return 1;
  }

  const char *input = argv[2];
  const char *output = NULL;
  bool emit_c_only = false;

  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 >= argc) {
        diag_error("<cli>", 1, 1, "missing value for -o");
        return 1;
      }
      output = argv[++i];
    } else if (strcmp(argv[i], "-S") == 0) {
      emit_c_only = true;
    } else {
      diag_error("<cli>", 1, 1, "unknown flag: %s", argv[i]);
      return 1;
    }
  }

  if (!has_ngawi_ext(input)) {
    diag_error(input, 1, 1, "input file must have .ngawi extension");
    return 1;
  }

  char *source = read_file_all(input);
  if (!source) {
    diag_error(input, 1, 1, "cannot read input: %s", strerror(errno));
    return 1;
  }

  int had_parse_error = 0;
  Program *program = parse_program(input, source, &had_parse_error);
  if (had_parse_error) {
    program_free(program);
    free(source);
    return 1;
  }

  if (sema_check_program(input, source, program) != 0) {
    program_free(program);
    free(source);
    return 1;
  }

  int exit_code = 0;
  char *owned_output = NULL;
  char *c_path = NULL;

  if (!output) {
    owned_output = default_output_from_input(input);
    if (!owned_output) {
      diag_error(input, 1, 1, "out of memory");
      exit_code = 1;
      goto cleanup;
    }
    output = owned_output;
  }

  c_path = dup_with_suffix(output, ".c");
  if (!c_path) {
    diag_error(input, 1, 1, "out of memory");
    exit_code = 1;
    goto cleanup;
  }

  if (codegen_emit_c(input, program, c_path) != 0) {
    exit_code = 1;
    goto cleanup;
  }

  if (emit_c_only) {
    printf("C11 output: %s\n", c_path);
    exit_code = 0;
    goto cleanup;
  }

  char cmd[2048];
  snprintf(cmd, sizeof(cmd),
           "gcc -std=c11 -O2 -Isrc/runtime %s src/runtime/ngawi_runtime.c -o %s", c_path,
           output);

  int rc = system(cmd);
  if (rc != 0) {
    diag_error(input, 1, 1, "gcc failed (exit code %d)", rc);
    exit_code = 1;
    goto cleanup;
  }

  printf("Built native binary: %s\n", output);

cleanup:
  free(c_path);
  free(owned_output);
  program_free(program);
  free(source);
  return exit_code;
}
