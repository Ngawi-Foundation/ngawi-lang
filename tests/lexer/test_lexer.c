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

static void test_elif_keyword(void) {
  const char *src = "if (true) { } elif (false) { } else { }";
  Lexer lx;
  lexer_init(&lx, "elif.ngawi", src);

  Token t;
  int seen_elif = 0;
  do {
    t = lexer_next(&lx);
    if (t.kind == TOK_KW_ELIF) seen_elif = 1;
  } while (t.kind != TOK_EOF);

  if (!seen_elif) {
    fprintf(stderr, "FAIL keyword token: elif not recognized\n");
    failures++;
  }
}

static void test_import_keyword(void) {
  const char *src = "import \"x.ngawi\";";
  Lexer lx;
  lexer_init(&lx, "import.ngawi", src);

  Token t1 = lexer_next(&lx);
  Token t2 = lexer_next(&lx);
  Token t3 = lexer_next(&lx);
  Token t4 = lexer_next(&lx);

  expect_kind(t1, TOK_KW_IMPORT, "kw_import");
  expect_kind(t2, TOK_STRING_LIT, "kw_import_string");
  expect_kind(t3, TOK_SEMI, "kw_import_semi");
  expect_kind(t4, TOK_EOF, "kw_import_eof");
}

static void test_match_tokens(void) {
  const char *src = "match x { 0 => { } _ => { } }";
  Lexer lx;
  lexer_init(&lx, "match.ngawi", src);

  Token t;
  int seen_match = 0, seen_fat_arrow = 0;
  do {
    t = lexer_next(&lx);
    if (t.kind == TOK_KW_MATCH) seen_match = 1;
    if (t.kind == TOK_FAT_ARROW) seen_fat_arrow = 1;
  } while (t.kind != TOK_EOF);

  if (!seen_match) {
    fprintf(stderr, "FAIL keyword token: match not recognized\n");
    failures++;
  }
  if (!seen_fat_arrow) {
    fprintf(stderr, "FAIL operator token: => not recognized\n");
    failures++;
  }
}

static void test_loop_control_keywords(void) {
  const char *src = "break; continue;";
  Lexer lx;
  lexer_init(&lx, "loop_ctrl.ngawi", src);

  Token t1 = lexer_next(&lx);
  Token t2 = lexer_next(&lx);
  Token t3 = lexer_next(&lx);
  Token t4 = lexer_next(&lx);
  Token t5 = lexer_next(&lx);

  expect_kind(t1, TOK_KW_BREAK, "kw_break");
  expect_kind(t2, TOK_SEMI, "kw_break_semi");
  expect_kind(t3, TOK_KW_CONTINUE, "kw_continue");
  expect_kind(t4, TOK_SEMI, "kw_continue_semi");
  expect_kind(t5, TOK_EOF, "kw_continue_eof");
}

static void test_percent_token(void) {
  const char *src = "let x: int = 5 % 2;";
  Lexer lx;
  lexer_init(&lx, "percent.ngawi", src);

  Token t;
  int seen_percent = 0;
  do {
    t = lexer_next(&lx);
    if (t.kind == TOK_PERCENT) seen_percent = 1;
  } while (t.kind != TOK_EOF);

  if (!seen_percent) {
    fprintf(stderr, "FAIL operator token: percent not recognized\n");
    failures++;
  }
}

static void test_compound_assign_tokens(void) {
  const char *src = "x += 1; y -= 2; z *= 3; a /= 4; b %= 5;";
  Lexer lx;
  lexer_init(&lx, "compound.ngawi", src);

  int seen_plus = 0, seen_minus = 0, seen_star = 0, seen_slash = 0, seen_percent = 0;
  Token t;
  do {
    t = lexer_next(&lx);
    if (t.kind == TOK_PLUS_ASSIGN) seen_plus = 1;
    if (t.kind == TOK_MINUS_ASSIGN) seen_minus = 1;
    if (t.kind == TOK_STAR_ASSIGN) seen_star = 1;
    if (t.kind == TOK_SLASH_ASSIGN) seen_slash = 1;
    if (t.kind == TOK_PERCENT_ASSIGN) seen_percent = 1;
  } while (t.kind != TOK_EOF);

  if (!seen_plus) {
    fprintf(stderr, "FAIL operator token: += not recognized\n");
    failures++;
  }
  if (!seen_minus) {
    fprintf(stderr, "FAIL operator token: -= not recognized\n");
    failures++;
  }
  if (!seen_star) {
    fprintf(stderr, "FAIL operator token: *= not recognized\n");
    failures++;
  }
  if (!seen_slash) {
    fprintf(stderr, "FAIL operator token: /= not recognized\n");
    failures++;
  }
  if (!seen_percent) {
    fprintf(stderr, "FAIL operator token: %%= not recognized\n");
    failures++;
  }
}

static void test_incdec_tokens(void) {
  const char *src = "i++; j--;";
  Lexer lx;
  lexer_init(&lx, "incdec.ngawi", src);

  Token t1 = lexer_next(&lx);
  Token t2 = lexer_next(&lx);
  Token t3 = lexer_next(&lx);
  Token t4 = lexer_next(&lx);
  Token t5 = lexer_next(&lx);
  Token t6 = lexer_next(&lx);

  expect_kind(t1, TOK_IDENT, "incdec_ident_i");
  expect_kind(t2, TOK_PLUS_PLUS, "incdec_plus_plus");
  expect_kind(t3, TOK_SEMI, "incdec_semi_1");
  expect_kind(t4, TOK_IDENT, "incdec_ident_j");
  expect_kind(t5, TOK_MINUS_MINUS, "incdec_minus_minus");
  expect_kind(t6, TOK_SEMI, "incdec_semi_2");
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
  test_elif_keyword();
  test_import_keyword();
  test_match_tokens();
  test_loop_control_keywords();
  test_percent_token();
  test_compound_assign_tokens();
  test_incdec_tokens();
  test_invalid_token();

  if (failures > 0) {
    fprintf(stderr, "\n%d test(s) failed\n", failures);
    return 1;
  }

  printf("All lexer tests passed\n");
  return 0;
}
