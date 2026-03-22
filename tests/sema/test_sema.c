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
  test_missing_main();

  if (failures) {
    fprintf(stderr, "\n%d sema test(s) failed\n", failures);
    return 1;
  }

  printf("All sema tests passed\n");
  return 0;
}
