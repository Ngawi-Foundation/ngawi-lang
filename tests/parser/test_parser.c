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
  Program *p = parse_program("recover.ngawi", src, &had_error);
  expect(had_error != 0, "recovery test should still report parse error");
  expect(p != NULL, "program not null in recovery test");
  expect(p->func_count == 2, "parser should continue and parse following function");
  program_free(p);
}

int main(void) {
  test_parse_main();
  test_parse_error();
  test_parse_for_loop();
  test_parse_break_continue();
  test_parse_recovery_keeps_following_functions();

  if (failures) {
    fprintf(stderr, "\n%d parser test(s) failed\n", failures);
    return 1;
  }

  printf("All parser tests passed\n");
  return 0;
}
