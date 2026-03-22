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

static void test_alias_type_keywords(void) {
  const char *src = "let a: amba = 1; let b: rusdi = 2.0; let c: fuad = true; let d: imut = \"x\";";
  Lexer lx;
  lexer_init(&lx, "alias.ngawi", src);

  Token t;
  int seen_amba = 0, seen_rusdi = 0, seen_fuad = 0, seen_imut = 0;
  do {
    t = lexer_next(&lx);
    if (t.kind == TOK_KW_AMBA) seen_amba = 1;
    if (t.kind == TOK_KW_RUSDI) seen_rusdi = 1;
    if (t.kind == TOK_KW_FUAD) seen_fuad = 1;
    if (t.kind == TOK_KW_IMUT) seen_imut = 1;
  } while (t.kind != TOK_EOF);

  if (!seen_amba) {
    fprintf(stderr, "FAIL alias keyword: amba not recognized\n");
    failures++;
  }
  if (!seen_rusdi) {
    fprintf(stderr, "FAIL alias keyword: rusdi not recognized\n");
    failures++;
  }
  if (!seen_fuad) {
    fprintf(stderr, "FAIL alias keyword: fuad not recognized\n");
    failures++;
  }
  if (!seen_imut) {
    fprintf(stderr, "FAIL alias keyword: imut not recognized\n");
    failures++;
  }
}

static void test_alias_decl_keywords(void) {
  const char *src = "muwani a: amba = 1; crot b: amba = 2;";
  Lexer lx;
  lexer_init(&lx, "alias_decl.ngawi", src);

  Token t;
  int let_count = 0;
  int const_count = 0;
  do {
    t = lexer_next(&lx);
    if (t.kind == TOK_KW_LET) let_count++;
    if (t.kind == TOK_KW_CONST) const_count++;
  } while (t.kind != TOK_EOF);

  if (let_count == 0) {
    fprintf(stderr, "FAIL alias keyword: muwani not recognized as let\n");
    failures++;
  }
  if (const_count == 0) {
    fprintf(stderr, "FAIL alias keyword: crot not recognized as const\n");
    failures++;
  }
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
  test_alias_type_keywords();
  test_alias_decl_keywords();
  test_invalid_token();

  if (failures > 0) {
    fprintf(stderr, "\n%d test(s) failed\n", failures);
    return 1;
  }

  printf("All lexer tests passed\n");
  return 0;
}
