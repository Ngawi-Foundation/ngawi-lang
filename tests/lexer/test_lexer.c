#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/lexer/lexer.h"

static int failures = 0;

static void expect_kind(Token got, TokenKind expected, const char *label) {
  if (got.kind != expected) {
    fprintf(stderr,
            "FAIL %s: expected %s, got %s at %d:%d\n",
            label,
            token_kind_name(expected),
            token_kind_name(got.kind),
            got.line,
            got.col);
    failures++;
  }
}

static void test_keywords_and_symbols(void) {
  const char *src = "fn main() -> int { return 0; }";
  Lexer lx;
  lexer_init(&lx, "test.ngawi", src);

  TokenKind expected[] = {
      TOK_KW_FN,   TOK_IDENT,  TOK_LPAREN, TOK_RPAREN, TOK_ARROW, TOK_KW_INT,
      TOK_LBRACE,  TOK_KW_RETURN, TOK_INT_LIT, TOK_SEMI, TOK_RBRACE, TOK_EOF,
  };

  for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
    Token t = lexer_next(&lx);
    expect_kind(t, expected[i], "keywords_and_symbols");
  }
}

static void test_numbers_strings_comments(void) {
  const char *src =
      "let x: int = 42; // comment\n"
      "let y = 3.14;\n"
      "let s = \"hi\\n\";\n";
  Lexer lx;
  lexer_init(&lx, "test.ngawi", src);

  Token t;
  do {
    t = lexer_next(&lx);
  } while (t.kind != TOK_FLOAT_LIT);
  expect_kind(t, TOK_FLOAT_LIT, "float_lit");

  do {
    t = lexer_next(&lx);
  } while (t.kind != TOK_STRING_LIT && t.kind != TOK_EOF);
  expect_kind(t, TOK_STRING_LIT, "string_lit");
}

static void test_invalid_token(void) {
  const char *src = "let x = @;";
  Lexer lx;
  lexer_init(&lx, "test.ngawi", src);

  Token t;
  do {
    t = lexer_next(&lx);
  } while (t.kind != TOK_INVALID && t.kind != TOK_EOF);

  expect_kind(t, TOK_INVALID, "invalid_char");
}

int main(void) {
  test_keywords_and_symbols();
  test_numbers_strings_comments();
  test_invalid_token();

  if (failures > 0) {
    fprintf(stderr, "\n%d test(s) failed\n", failures);
    return 1;
  }

  printf("All lexer tests passed\n");
  return 0;
}
