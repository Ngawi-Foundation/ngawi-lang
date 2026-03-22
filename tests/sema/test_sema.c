#include <stdio.h>

#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

static int failures = 0;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    failures++;
  }
}

static int run_program(const char *name, const char *src) {
  int parse_err = 0;
  Program *p = parse_program(name, src, &parse_err);
  if (parse_err) {
    program_free(p);
    return 1;
  }
  int sema_err = sema_check_program(name, src, p);
  program_free(p);
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
  expect(run_program("ok.ngawi", src) == 0, "valid program should pass sema");
}

static void test_type_mismatch(void) {
  const char *src =
      "fn main() -> int {\n"
      "  let x: int = 1;\n"
      "  x = \"oops\";\n"
      "  return 0;\n"
      "}\n";
  expect(run_program("type_mismatch.ngawi", src) != 0,
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

  expect(run_program("loop_ctrl_ok.ngawi", ok_src) == 0,
         "break/continue inside loop should pass");
  expect(run_program("loop_ctrl_bad.ngawi", bad_src) != 0,
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

  expect(run_program("mod_ok.ngawi", ok_src) == 0, "int modulo should pass");
  expect(run_program("mod_bad.ngawi", bad_src) != 0, "float modulo should fail");
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

  expect(run_program("cast_ok.ngawi", ok_src) == 0, "valid casts should pass");
  expect(run_program("cast_bad.ngawi", bad_src) != 0, "invalid cast arg should fail");
}

static void test_missing_main(void) {
  const char *src =
      "fn nope() -> int {\n"
      "  return 0;\n"
      "}\n";
  expect(run_program("missing_main.ngawi", src) != 0,
         "missing main should fail sema");
}

int main(void) {
  test_valid_program();
  test_type_mismatch();
  test_modulo_type_rules();
  test_builtin_casts();
  test_break_continue_scope();
  test_missing_main();

  if (failures) {
    fprintf(stderr, "\n%d sema test(s) failed\n", failures);
    return 1;
  }

  printf("All sema tests passed\n");
  return 0;
}
