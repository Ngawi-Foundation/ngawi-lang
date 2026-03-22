#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/diag.h"
#include "codegen/cgen.h"
#include "parser/parser.h"
#include "sema/sema.h"

typedef struct StrList {
  char **items;
  size_t count;
  size_t cap;
} StrList;

typedef struct StrBuf {
  char *data;
  size_t len;
  size_t cap;
} StrBuf;

static void print_usage(void) {
  puts("Usage: ngawic build <input.ngawi> [-o output] [-S]");
}

static bool has_ngawi_ext(const char *path) {
  const char *dot = strrchr(path, '.');
  return dot && strcmp(dot, ".ngawi") == 0;
}

static int str_list_contains(const StrList *list, const char *value) {
  for (size_t i = 0; i < list->count; i++) {
    if (strcmp(list->items[i], value) == 0) return 1;
  }
  return 0;
}

static int str_list_push_owned(StrList *list, char *value) {
  if (list->count == list->cap) {
    size_t next_cap = list->cap == 0 ? 8 : list->cap * 2;
    char **next = (char **)realloc(list->items, next_cap * sizeof(char *));
    if (!next) return 0;
    list->items = next;
    list->cap = next_cap;
  }
  list->items[list->count++] = value;
  return 1;
}

static char *str_list_pop(StrList *list) {
  if (list->count == 0) return NULL;
  return list->items[--list->count];
}

static void str_list_free(StrList *list) {
  for (size_t i = 0; i < list->count; i++) free(list->items[i]);
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->cap = 0;
}

static int str_buf_append_n(StrBuf *buf, const char *s, size_t n) {
  if (buf->len + n + 1 > buf->cap) {
    size_t need = buf->len + n + 1;
    size_t next_cap = buf->cap == 0 ? 1024 : buf->cap;
    while (next_cap < need) next_cap *= 2;
    char *next = (char *)realloc(buf->data, next_cap);
    if (!next) return 0;
    buf->data = next;
    buf->cap = next_cap;
  }

  memcpy(buf->data + buf->len, s, n);
  buf->len += n;
  buf->data[buf->len] = '\0';
  return 1;
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

static char *dup_n(const char *s, size_t n) {
  char *out = (char *)malloc(n + 1);
  if (!out) return NULL;
  memcpy(out, s, n);
  out[n] = '\0';
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

static char *path_dirname_dup(const char *path) {
  const char *slash = strrchr(path, '/');
  if (!slash) return dup_n(".", 1);
  if (slash == path) return dup_n("/", 1);
  return dup_n(path, (size_t)(slash - path));
}

static char *path_join(const char *dir, const char *rel) {
  if (rel[0] == '/') return dup_n(rel, strlen(rel));

  size_t nd = strlen(dir);
  size_t nr = strlen(rel);
  char *out = (char *)malloc(nd + 1 + nr + 1);
  if (!out) return NULL;

  memcpy(out, dir, nd);
  out[nd] = '/';
  memcpy(out + nd + 1, rel, nr);
  out[nd + 1 + nr] = '\0';
  return out;
}

static char *path_to_abs(const char *path) {
  if (path[0] == '/') return dup_n(path, strlen(path));

  char cwd[4096];
  if (!getcwd(cwd, sizeof(cwd))) return NULL;
  return path_join(cwd, path);
}

static int str_list_index_of(const StrList *list, const char *value) {
  for (size_t i = 0; i < list->count; i++) {
    if (strcmp(list->items[i], value) == 0) return (int)i;
  }
  return -1;
}

static void diag_import_context(const char *importer_file,
                                int importer_line,
                                const char *target_text) {
  if (!importer_file || !target_text) return;
  diag_note(importer_file, importer_line, 1, "while importing \"%s\"", target_text);
}

static void diag_import_cycle(const StrList *stack, size_t start_idx, const char *repeat) {
  StrBuf chain = {0};
  for (size_t i = start_idx; i < stack->count; i++) {
    if (i > start_idx && !str_buf_append_n(&chain, " -> ", 4)) break;
    if (!str_buf_append_n(&chain, stack->items[i], strlen(stack->items[i]))) break;
  }
  if (chain.len > 0) str_buf_append_n(&chain, " -> ", 4);
  str_buf_append_n(&chain, repeat, strlen(repeat));

  diag_error(repeat, 1, 1, "import cycle detected: %s", chain.data ? chain.data : repeat);
  free(chain.data);
}

static int load_module_recursive(const char *path,
                                 StrList *stack,
                                 StrList *loaded,
                                 StrBuf *out_source,
                                 const char *importer_file,
                                 int importer_line,
                                 const char *target_text) {
  char *canon = path_to_abs(path);
  if (!canon) {
    diag_error(path, 1, 1, "cannot resolve module path: %s", strerror(errno));
    diag_import_context(importer_file, importer_line, target_text);
    return 1;
  }

  if (str_list_contains(loaded, canon)) {
    free(canon);
    return 0;
  }

  int cycle_idx = str_list_index_of(stack, canon);
  if (cycle_idx >= 0) {
    diag_import_cycle(stack, (size_t)cycle_idx, canon);
    diag_import_context(importer_file, importer_line, target_text);
    free(canon);
    return 1;
  }

  if (!str_list_push_owned(stack, canon)) {
    diag_error(path, 1, 1, "out of memory");
    free(canon);
    return 1;
  }

  char *source = read_file_all(canon);
  if (!source) {
    diag_error(canon, 1, 1, "cannot read module: %s", strerror(errno));
    diag_import_context(importer_file, importer_line, target_text);
    char *p = str_list_pop(stack);
    free(p);
    return 1;
  }

  int had_parse_error = 0;
  Program *module_prog = parse_program(canon, source, &had_parse_error);
  if (had_parse_error) {
    program_free(module_prog);
    free(source);
    char *p = str_list_pop(stack);
    free(p);
    return 1;
  }

  char *dir = path_dirname_dup(canon);
  if (!dir) {
    diag_error(canon, 1, 1, "out of memory");
    program_free(module_prog);
    free(source);
    char *p = str_list_pop(stack);
    free(p);
    return 1;
  }

  for (size_t i = 0; i < module_prog->import_count; i++) {
    ImportDecl *imp = &module_prog->imports[i];

    char *dep_path = path_join(dir, imp->path);
    if (!dep_path) {
      diag_error(canon, imp->line, imp->col, "out of memory");
      free(dir);
      program_free(module_prog);
      free(source);
      char *p = str_list_pop(stack);
      free(p);
      return 1;
    }

    if (!has_ngawi_ext(dep_path)) {
      diag_error_source(canon, source, imp->line, imp->col,
                        "import path must end with .ngawi: %s", imp->path);
      free(dep_path);
      free(dir);
      program_free(module_prog);
      free(source);
      char *p = str_list_pop(stack);
      free(p);
      return 1;
    }

    int rc = load_module_recursive(dep_path, stack, loaded, out_source, canon, imp->line,
                                   imp->path);
    free(dep_path);
    if (rc != 0) {
      free(dir);
      program_free(module_prog);
      free(source);
      char *p = str_list_pop(stack);
      free(p);
      return 1;
    }
  }

  if (!str_buf_append_n(out_source, source, strlen(source))) {
    diag_error(canon, 1, 1, "out of memory");
    free(dir);
    program_free(module_prog);
    free(source);
    char *p = str_list_pop(stack);
    free(p);
    return 1;
  }
  if (out_source->len > 0 && out_source->data[out_source->len - 1] != '\n') {
    if (!str_buf_append_n(out_source, "\n", 1)) {
      diag_error(canon, 1, 1, "out of memory");
      free(dir);
      program_free(module_prog);
      free(source);
      char *p = str_list_pop(stack);
      free(p);
      return 1;
    }
  }

  free(dir);
  program_free(module_prog);
  free(source);

  char *done = str_list_pop(stack);
  if (!str_list_push_owned(loaded, done)) {
    diag_error(canon, 1, 1, "out of memory");
    free(done);
    return 1;
  }

  return 0;
}

static char *load_source_with_imports(const char *input) {
  StrList stack = {0};
  StrList loaded = {0};
  StrBuf out = {0};

  int rc = load_module_recursive(input, &stack, &loaded, &out, NULL, 0, NULL);

  while (stack.count > 0) {
    char *s = str_list_pop(&stack);
    free(s);
  }
  free(stack.items);

  str_list_free(&loaded);

  if (rc != 0) {
    free(out.data);
    return NULL;
  }

  if (!out.data) out.data = dup_n("", 0);
  return out.data;
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

  char *source = load_source_with_imports(input);
  if (!source) return 1;

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
