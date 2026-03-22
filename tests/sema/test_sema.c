#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

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

static int run_program(const char *name, const char *src, int quiet_stderr) {
  int saved_fd = -1;
  int silenced = 0;
  if (quiet_stderr && !test_show_errors_enabled()) silenced = silence_stderr_begin(&saved_fd);

  int parse_err = 0;
  Program *p = parse_program(name, src, &parse_err);
  if (parse_err) {
    if (silenced) silence_stderr_end(saved_fd);
    program_free(p);
    return 1;
  }

  int sema_err = sema_check_program(name, src, p);
  program_free(p);

  if (silenced) silence_stderr_end(saved_fd);
  return sema_err;
}

static void test_valid_program(void) {
  const char *src =
      "fn add(a: int, b: int) -> int {\n"
      "  return a + b;\n"
      "}\n"
      "fn main() -> int {\n"
      "  let x: int = add(1, 2);\n"
      "  print(x);\n"
      "  return 0;\n"
      "}\n";
  expect(run_program("ok.ngawi", src, 0) == 0, "valid program should pass sema");
}

static void test_type_mismatch(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let x: int = 1;\n"
      "  x = \"oops\";\n"
      "  return 0;\n"
      "}\n";
  expect(run_program("type_mismatch.ngawi", src, 1) != 0,
         "assignment type mismatch should fail sema");
}

static void test_break_continue_scope(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let i: int = 0;\n"
      "  while (i < 5) {\n"
      "    i = i + 1;\n"
      "    if (i == 2) { continue; }\n"
      "    if (i == 4) { break; }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  break;\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("loop_ctrl_ok.ngawi", ok_src, 0) == 0,
         "break/continue inside loop should pass");
  expect(run_program("loop_ctrl_bad.ngawi", bad_src, 1) != 0,
         "break outside loop should fail");
}

static void test_modulo_type_rules(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let r: int = 9 % 4;\n"
      "  return r;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let a: float = 9.0;\n"
      "  let b: float = 4.0;\n"
      "  let r = a % b;\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("mod_ok.ngawi", ok_src, 0) == 0, "int modulo should pass");
  expect(run_program("mod_bad.ngawi", bad_src, 1) != 0, "float modulo should fail");
}

static void test_builtin_casts(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let a: float = 3.8;\n"
      "  let b: int = to_int(a);\n"
      "  let c: float = to_float(7);\n"
      "  let d: int = to_amba(a);\n"
      "  let e: float = to_rusdi(9);\n"
      "  print(b, c, d, e);\n"
      "  return 0;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let s: string = \"x\";\n"
      "  let v = to_amba(s);\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("cast_ok.ngawi", ok_src, 0) == 0, "valid casts should pass");
  expect(run_program("cast_bad.ngawi", bad_src, 1) != 0, "invalid cast arg should fail");
}

static void test_compound_assign_rules(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let x: int = 10;\n"
      "  x += 2;\n"
      "  x *= 3;\n"
      "  x -= 4;\n"
      "  x /= 2;\n"
      "  x %= 5;\n"
      "  return x;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let s: string = \"a\";\n"
      "  s += 1;\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("compound_ok.ngawi", ok_src, 0) == 0,
         "valid compound assignments should pass");
  expect(run_program("compound_bad.ngawi", bad_src, 1) != 0,
         "invalid compound assignment should fail");
}

static void test_incdec_rules(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let i: int = 0;\n"
      "  i++;\n"
      "  i--;\n"
      "  for (let j: int = 0; j < 3; j++) {\n"
      "    i += j;\n"
      "  }\n"
      "  return i;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let s: string = \"x\";\n"
      "  s++;\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("incdec_ok.ngawi", ok_src, 0) == 0, "inc/dec on int should pass");
  expect(run_program("incdec_bad.ngawi", bad_src, 1) != 0,
         "inc/dec on non-numeric should fail");
}

static void test_string_equality(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let a: string = \"x\";\n"
      "  let b: string = \"x\";\n"
      "  let same: bool = a == b;\n"
      "  print(same);\n"
      "  return 0;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let a: string = \"x\";\n"
      "  let b: int = 1;\n"
      "  let same: bool = a == b;\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("str_eq_ok.ngawi", ok_src, 0) == 0, "string equality should pass");
  expect(run_program("str_eq_bad.ngawi", bad_src, 1) != 0, "mixed equality should fail");
}

static void test_string_concat(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let a: string = \"na\";\n"
      "  let b: string = \"gawi\";\n"
      "  let s: string = a + b;\n"
      "  print(s);\n"
      "  return 0;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let a: string = \"x\";\n"
      "  let n: int = 1;\n"
      "  let s = a + n;\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("str_concat_ok.ngawi", ok_src, 0) == 0, "string concat should pass");
  expect(run_program("str_concat_bad.ngawi", bad_src, 1) != 0,
         "string plus non-string should fail");
}

static void test_len_builtin(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let s: string = \"ngawi\";\n"
      "  let n: int = len(s);\n"
      "  print(n);\n"
      "  return 0;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let x: int = 123;\n"
      "  let n = len(x);\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("len_ok.ngawi", ok_src, 0) == 0, "len on string should pass");
  expect(run_program("len_bad.ngawi", bad_src, 1) != 0, "len on non-string should fail");
}

static void test_string_builtins(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let s: string = \"NgawiLang\";\n"
      "  let c: bool = contains(s, \"Lang\");\n"
      "  let p: bool = starts_with(s, \"Nga\");\n"
      "  let low: string = to_lower(s);\n"
      "  print(c, p, low);\n"
      "  return 0;\n"
      "}\n";

  const char *bad_src =
      "fn main() -> int {\n"
      "  let x: int = 42;\n"
      "  let c = contains(x, \"a\");\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("str_builtin_ok.ngawi", ok_src, 0) == 0, "string builtins should pass");
  expect(run_program("str_builtin_bad.ngawi", bad_src, 1) != 0,
         "invalid string builtins args should fail");
}

static void test_match_rules(void) {
  const char *ok_src =
      "fn main() -> int {\n"
      "  let x: int = 2;\n"
      "  match x {\n"
      "    0 => { print(\"zero\"); }\n"
      "    2 => { print(\"two\"); }\n"
      "    _ => { print(\"other\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  const char *ok_bool_src =
      "fn main() -> int {\n"
      "  let ok: bool = true;\n"
      "  match ok {\n"
      "    true => { print(\"t\"); }\n"
      "    false => { print(\"f\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  const char *dup_src =
      "fn main() -> int {\n"
      "  let x: int = 1;\n"
      "  match x {\n"
      "    1 => { print(\"a\"); }\n"
      "    1 => { print(\"b\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  const char *dup_bool_src =
      "fn main() -> int {\n"
      "  let ok: bool = true;\n"
      "  match ok {\n"
      "    true => { print(\"a\"); }\n"
      "    true => { print(\"b\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  const char *bad_type_src =
      "fn main() -> int {\n"
      "  let s: string = \"x\";\n"
      "  match s {\n"
      "    _ => { print(\"other\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  const char *bad_arm_src =
      "fn main() -> int {\n"
      "  let ok: bool = true;\n"
      "  match ok {\n"
      "    1 => { print(\"one\"); }\n"
      "    _ => { print(\"other\"); }\n"
      "  }\n"
      "  return 0;\n"
      "}\n";

  expect(run_program("match_ok.ngawi", ok_src, 0) == 0, "valid int match should pass");
  expect(run_program("match_ok_bool.ngawi", ok_bool_src, 0) == 0,
         "valid bool match should pass");
  expect(run_program("match_dup.ngawi", dup_src, 1) != 0, "duplicate int match arm should fail");
  expect(run_program("match_dup_bool.ngawi", dup_bool_src, 1) != 0,
         "duplicate bool match arm should fail");
  expect(run_program("match_bad_type.ngawi", bad_type_src, 1) != 0,
         "non-int/bool match subject should fail");
  expect(run_program("match_bad_arm.ngawi", bad_arm_src, 1) != 0,
         "bool match with int arm should fail");
}

static void test_missing_main(void) {
  const char *src =
      "fn nope() -> int {\n"
      "  return 0;\n"
      "}\n";
  expect(run_program("missing_main.ngawi", src, 1) != 0,
         "missing main should fail sema");
}

int main(void) {
  test_valid_program();
  test_type_mismatch();
  test_modulo_type_rules();
  test_builtin_casts();
  test_compound_assign_rules();
  test_incdec_rules();
  test_string_equality();
  test_string_concat();
  test_len_builtin();
  test_string_builtins();
  test_match_rules();
  test_break_continue_scope();
  test_missing_main();

  if (failures) {
    fprintf(stderr, "\n%d sema test(s) failed\n", failures);
    return 1;
  }

  printf("All sema tests passed\n");
  return 0;
}
