#include <stdio.h>

#include "../../src/parser/parser.h"

static int failures = 0;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    failures++;
  }
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
  Program *p = parse_program("bad.ngawi", src, &had_error);
  expect(had_error != 0, "parse should fail on missing semicolon");
  program_free(p);
}

int main(void) {
  test_parse_main();
  test_parse_error();

  if (failures) {
    fprintf(stderr, "\n%d parser test(s) failed\n", failures);
    return 1;
  }

  printf("All parser tests passed\n");
  return 0;
}
