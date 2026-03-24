#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../src/parser/parser.h"

static int failures = 0;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    failures++;
  }
}

static int silence_stderr_begin(int *saved_fd) {
  fflush(stderr);
  *saved_fd = dup(STDERR_FILENO);
  if (*saved_fd < 0) return 0;

  int devnull = open("/dev/null", O_WRONLY);
  if (devnull < 0) {
    close(*saved_fd);
    return 0;
  }

  if (dup2(devnull, STDERR_FILENO) < 0) {
    close(devnull);
    close(*saved_fd);
    return 0;
  }

  close(devnull);
  return 1;
}

static void silence_stderr_end(int saved_fd) {
  fflush(stderr);
  (void)dup2(saved_fd, STDERR_FILENO);
  close(saved_fd);
}

static int test_show_errors_enabled(void) {
  const char *v = getenv("NGAWI_TEST_SHOW_ERRORS");
  return v && (strcmp(v, "1") == 0 || strcmp(v, "true") == 0 || strcmp(v, "yes") == 0);
}

static Program *parse_program_maybe_quiet(const char *name,
                                          const char *src,
                                          int *had_error,
                                          int quiet_stderr) {
  int saved_fd = -1;
  int silenced = 0;
  if (quiet_stderr && !test_show_errors_enabled()) silenced = silence_stderr_begin(&saved_fd);

  Program *p = parse_program(name, src, had_error);

  if (silenced) silence_stderr_end(saved_fd);
  return p;
}

static void test_parse_main(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let x: int = 1 + 2 * 3;\n"
      "  print(\"ok\", x);\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("test.ngawi", src, &had_error);
  expect(had_error == 0, "parse should succeed");
  expect(p != NULL, "program not null");
  expect(p->func_count == 1, "one function expected");
  if (p && p->func_count == 1) {
    expect(p->funcs[0].body != NULL, "main body exists");
  }
  program_free(p);
}

static void test_parse_error(void) {
  const char *src = "fn main() -> int { let x = 1 return 0; }";
  int had_error = 0;
  Program *p = parse_program_maybe_quiet("bad.ngawi", src, &had_error, 1);
  expect(had_error != 0, "parse should fail on missing semicolon");
  program_free(p);
}

static void test_parse_modulo_expr(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let r: int = 5 % 2;\n"
      "  return r;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("modulo.ngawi", src, &had_error);
  expect(had_error == 0, "modulo parse should succeed");
  expect(p != NULL, "modulo program not null");
  expect(p->func_count == 1, "modulo one function expected");
  program_free(p);
}

static void test_parse_compound_assign(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let x: int = 1;\n"
      "  x += 2;\n"
      "  x *= 3;\n"
      "  return x;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("compound_assign.ngawi", src, &had_error);
  expect(had_error == 0, "compound assignment parse should succeed");
  expect(p != NULL, "compound assignment program not null");
  expect(p->func_count == 1, "compound assignment one function expected");
  program_free(p);
}

static void test_parse_incdec_stmt(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let i: int = 0;\n"
      "  i++;\n"
      "  i--;\n"
      "  return i;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("incdec.ngawi", src, &had_error);
  expect(had_error == 0, "inc/dec parse should succeed");
  expect(p != NULL, "inc/dec program not null");
  expect(p->func_count == 1, "inc/dec one function expected");
  program_free(p);
}

static void test_parse_elif_chain(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let x: int = 2;\n"
      "  if (x == 0) {\n"
      "    print(\"zero\");\n"
      "  } elif (x == 1) {\n"
      "    print(\"one\");\n"
      "  } elif (x == 2) {\n"
      "    print(\"two\");\n"
      "  } else {\n"
      "    print(\"other\");\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("elif.ngawi", src, &had_error);
  expect(had_error == 0, "elif parse should succeed");
  expect(p != NULL, "elif program not null");
  expect(p->func_count == 1, "elif one function expected");
  program_free(p);
}

static void test_parse_match_stmt(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let x: int = 2;\n"
      "  match x {\n"
      "    0 => { print(\"zero\"); }\n"
      "    2 => { print(\"two\"); }\n"
      "    _ => { print(\"other\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("match.ngawi", src, &had_error);
  expect(had_error == 0, "match parse should succeed");
  expect(p != NULL, "match program not null");
  expect(p->func_count == 1, "match one function expected");
  program_free(p);
}

static void test_parse_match_bool_stmt(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let ok: bool = true;\n"
      "  match ok {\n"
      "    true => { print(\"t\"); }\n"
      "    false => { print(\"f\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("match_bool.ngawi", src, &had_error);
  expect(had_error == 0, "bool match parse should succeed");
  expect(p != NULL, "bool match program not null");
  expect(p->func_count == 1, "bool match one function expected");
  program_free(p);
}

static void test_parse_match_string_stmt(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let s: string = \"hi\";\n"
      "  match s {\n"
      "    \"hi\" => { print(\"h\"); }\n"
      "    _ => { print(\"o\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("match_string.ngawi", src, &had_error);
  expect(had_error == 0, "string match parse should succeed");
  expect(p != NULL, "string match program not null");
  expect(p->func_count == 1, "string match one function expected");
  program_free(p);
}

static void test_parse_for_loop(void) {
  const char *src =
      "fn main() -> int {\n"
      "  for (let i: int = 1; i <= 3; i = i + 1) {\n"
      "    print(i);\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("for.ngawi", src, &had_error);
  expect(had_error == 0, "for loop parse should succeed");
  expect(p != NULL, "for loop program not null");
  expect(p->func_count == 1, "for loop one function expected");
  program_free(p);
}

static void test_parse_break_continue(void) {
  const char *src =
      "fn main() -> int {\n"
      "  for (let i: int = 0; i < 10; i = i + 1) {\n"
      "    if (i == 3) { continue; }\n"
      "    if (i == 7) { break; }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("break_continue.ngawi", src, &had_error);
  expect(had_error == 0, "break/continue parse should succeed");
  expect(p != NULL, "break/continue program not null");
  expect(p->func_count == 1, "break/continue one function expected");
  program_free(p);
}

static void test_parse_import_toplevel(void) {
  const char *src =
      "import \"lib.ngawi\";\n"
      "fn main() -> int {\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("import_top.ngawi", src, &had_error);
  expect(had_error == 0, "top-level import parse should succeed");
  expect(p != NULL, "program not null for import parse");
  expect(p->import_count == 1, "one import expected");
  expect(p->func_count == 1, "one function expected with import");
  if (p && p->import_count == 1) {
    expect(strcmp(p->imports[0].path, "lib.ngawi") == 0, "import path should be captured");
  }
  program_free(p);
}

static void test_parse_int_array(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let a: int[] = [1, 2, 3];\n"
      "  let f: float[] = [1.0, 2.5];\n"
      "  let b: bool[] = [true, false];\n"
      "  let s: string[] = [\"a\", \"b\"];\n"
      "  a[1] = 9;\n"
      "  let x: int = a[1];\n"
      "  return x;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program("arr.ngawi", src, &had_error);
  expect(had_error == 0, "array parse should succeed");
  expect(p != NULL, "array program not null");
  expect(p->func_count == 1, "array one function expected");
  program_free(p);
}

static void test_parse_import_not_allowed_in_block(void) {
  const char *src =
      "fn main() -> int {\n"
      "  import \"x.ngawi\";\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program_maybe_quiet("import_inner.ngawi", src, &had_error, 1);
  expect(had_error != 0, "import inside function should fail parse");
  program_free(p);
}

static void test_parse_invalid_import_syntax(void) {
  const char *src =
      "import lib.ngawi;\n"
      "fn main() -> int {\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program_maybe_quiet("import_bad.ngawi", src, &had_error, 1);
  expect(had_error != 0, "invalid import syntax should fail parse");
  program_free(p);
}

static void test_parse_recovery_keeps_following_functions(void) {
  const char *src =
      "fn broken() -> int {\n"
      "  let x = 1\n"
      "  return x;\n"
      "}\n"
      "fn main() -> int {\n"
      "  return 0;\n"
      "}\n";

  int had_error = 0;
  Program *p = parse_program_maybe_quiet("recover.ngawi", src, &had_error, 1);
  expect(had_error != 0, "recovery test should still report parse error");
  expect(p != NULL, "program not null in recovery test");
  expect(p->func_count == 2, "parser should continue and parse following function");
  program_free(p);
}

int main(void) {
  test_parse_main();
  test_parse_error();
  test_parse_modulo_expr();
  test_parse_compound_assign();
  test_parse_incdec_stmt();
  test_parse_elif_chain();
  test_parse_match_stmt();
  test_parse_match_bool_stmt();
  test_parse_match_string_stmt();
  test_parse_for_loop();
  test_parse_break_continue();
  test_parse_import_toplevel();
  test_parse_int_array();
  test_parse_import_not_allowed_in_block();
  test_parse_invalid_import_syntax();
  test_parse_recovery_keeps_following_functions();

  if (failures) {
    fprintf(stderr, "\n%d parser test(s) failed\n", failures);
    return 1;
  }

  printf("All parser tests passed\n");
  return 0;
}
