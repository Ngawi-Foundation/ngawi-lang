#include "lexer.h"

#include <ctype.h>
#include <string.h>

static char peek(const Lexer *lx) { return *lx->cur; }

static char peek_next(const Lexer *lx) {
  if (*lx->cur == '\0') return '\0';
  return lx->cur[1];
}

static char advance(Lexer *lx) {
  char c = *lx->cur;
  if (c == '\0') return c;
  lx->cur++;
  if (c == '\n') {
    lx->line++;
    lx->col = 1;
  } else {
    lx->col++;
  }
  return c;
}

static int match(Lexer *lx, char expected) {
  if (*lx->cur != expected) return 0;
  advance(lx);
  return 1;
}

static Token make_token(const Lexer *lx,
                        TokenKind kind,
                        const char *start,
                        int line,
                        int col) {
  Token t;
  t.kind = kind;
  t.start = start;
  t.length = (size_t)(lx->cur - start);
  t.line = line;
  t.col = col;
  return t;
}

static Token make_single(Lexer *lx, TokenKind kind, const char *start, int line, int col) {
  (void)advance(lx);
  return make_token(lx, kind, start, line, col);
}

static void skip_ws_and_comments(Lexer *lx) {
  for (;;) {
    char c = peek(lx);
    if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
      advance(lx);
      continue;
    }

    if (c == '/' && peek_next(lx) == '/') {
      while (peek(lx) != '\0' && peek(lx) != '\n') advance(lx);
      continue;
    }

    break;
  }
}

static TokenKind keyword_kind(const char *s, size_t n) {
  if (n == 2 && strncmp(s, "fn", 2) == 0) return TOK_KW_FN;
  if (n == 2 && strncmp(s, "if", 2) == 0) return TOK_KW_IF;
  if (n == 3 && strncmp(s, "let", 3) == 0) return TOK_KW_LET;
  if (n == 6 && strncmp(s, "muwani", 6) == 0) return TOK_KW_LET;
  if (n == 4 && strncmp(s, "else", 4) == 0) return TOK_KW_ELSE;
  if (n == 4 && strncmp(s, "true", 4) == 0) return TOK_KW_TRUE;
  if (n == 4 && strncmp(s, "void", 4) == 0) return TOK_KW_VOID;
  if (n == 3 && strncmp(s, "int", 3) == 0) return TOK_KW_INT;
  if (n == 4 && strncmp(s, "amba", 4) == 0) return TOK_KW_AMBA;
  if (n == 5 && strncmp(s, "const", 5) == 0) return TOK_KW_CONST;
  if (n == 4 && strncmp(s, "crot", 4) == 0) return TOK_KW_CONST;
  if (n == 5 && strncmp(s, "while", 5) == 0) return TOK_KW_WHILE;
  if (n == 3 && strncmp(s, "for", 3) == 0) return TOK_KW_FOR;
  if (n == 5 && strncmp(s, "break", 5) == 0) return TOK_KW_BREAK;
  if (n == 8 && strncmp(s, "continue", 8) == 0) return TOK_KW_CONTINUE;
  if (n == 5 && strncmp(s, "false", 5) == 0) return TOK_KW_FALSE;
  if (n == 5 && strncmp(s, "float", 5) == 0) return TOK_KW_FLOAT;
  if (n == 5 && strncmp(s, "rusdi", 5) == 0) return TOK_KW_RUSDI;
  if (n == 4 && strncmp(s, "fuad", 4) == 0) return TOK_KW_FUAD;
  if (n == 4 && strncmp(s, "imut", 4) == 0) return TOK_KW_IMUT;
  if (n == 6 && strncmp(s, "return", 6) == 0) return TOK_KW_RETURN;
  if (n == 6 && strncmp(s, "string", 6) == 0) return TOK_KW_STRING;
  if (n == 4 && strncmp(s, "bool", 4) == 0) return TOK_KW_BOOL;
  return TOK_IDENT;
}

static Token lex_ident_or_kw(Lexer *lx, const char *start, int line, int col) {
  while (isalnum((unsigned char)peek(lx)) || peek(lx) == '_') advance(lx);
  Token t = make_token(lx, TOK_IDENT, start, line, col);
  t.kind = keyword_kind(t.start, t.length);
  return t;
}

static Token lex_number(Lexer *lx, const char *start, int line, int col) {
  TokenKind kind = TOK_INT_LIT;
  while (isdigit((unsigned char)peek(lx))) advance(lx);
  if (peek(lx) == '.' && isdigit((unsigned char)peek_next(lx))) {
    kind = TOK_FLOAT_LIT;
    advance(lx);
    while (isdigit((unsigned char)peek(lx))) advance(lx);
  }
  return make_token(lx, kind, start, line, col);
}

static Token lex_string(Lexer *lx, const char *start, int line, int col) {
  advance(lx);  // opening quote
  while (peek(lx) != '\0' && peek(lx) != '"') {
    if (peek(lx) == '\\') {
      advance(lx);
      if (peek(lx) == '\0') break;
      char esc = peek(lx);
      if (esc == '"' || esc == '\\' || esc == 'n' || esc == 't') {
        advance(lx);
        continue;
      }
      // unsupported escape, still consume for recovery
      advance(lx);
      continue;
    }
    advance(lx);
  }

  if (peek(lx) != '"') return make_token(lx, TOK_INVALID, start, line, col);
  advance(lx);  // closing quote
  return make_token(lx, TOK_STRING_LIT, start, line, col);
}

void lexer_init(Lexer *lx, const char *file, const char *source) {
  lx->file = file;
  lx->src = source;
  lx->cur = source;
  lx->line = 1;
  lx->col = 1;
}

Token lexer_next(Lexer *lx) {
  skip_ws_and_comments(lx);

  const char *start = lx->cur;
  int line = lx->line;
  int col = lx->col;

  char c = peek(lx);
  if (c == '\0') return make_token(lx, TOK_EOF, start, line, col);

  if (isalpha((unsigned char)c) || c == '_') return lex_ident_or_kw(lx, start, line, col);
  if (isdigit((unsigned char)c)) return lex_number(lx, start, line, col);

  switch (c) {
    case '"': return lex_string(lx, start, line, col);
    case '+': return make_single(lx, TOK_PLUS, start, line, col);
    case '-':
      advance(lx);
      if (match(lx, '>')) return make_token(lx, TOK_ARROW, start, line, col);
      return make_token(lx, TOK_MINUS, start, line, col);
    case '*': return make_single(lx, TOK_STAR, start, line, col);
    case '/': return make_single(lx, TOK_SLASH, start, line, col);
    case '!':
      advance(lx);
      if (match(lx, '=')) return make_token(lx, TOK_NE, start, line, col);
      return make_token(lx, TOK_BANG, start, line, col);
    case '=':
      advance(lx);
      if (match(lx, '=')) return make_token(lx, TOK_EQ, start, line, col);
      return make_token(lx, TOK_ASSIGN, start, line, col);
    case '<':
      advance(lx);
      if (match(lx, '=')) return make_token(lx, TOK_LE, start, line, col);
      return make_token(lx, TOK_LT, start, line, col);
    case '>':
      advance(lx);
      if (match(lx, '=')) return make_token(lx, TOK_GE, start, line, col);
      return make_token(lx, TOK_GT, start, line, col);
    case '&':
      advance(lx);
      if (match(lx, '&')) return make_token(lx, TOK_AND_AND, start, line, col);
      return make_token(lx, TOK_INVALID, start, line, col);
    case '|':
      advance(lx);
      if (match(lx, '|')) return make_token(lx, TOK_OR_OR, start, line, col);
      return make_token(lx, TOK_INVALID, start, line, col);
    case '(': return make_single(lx, TOK_LPAREN, start, line, col);
    case ')': return make_single(lx, TOK_RPAREN, start, line, col);
    case '{': return make_single(lx, TOK_LBRACE, start, line, col);
    case '}': return make_single(lx, TOK_RBRACE, start, line, col);
    case ',': return make_single(lx, TOK_COMMA, start, line, col);
    case ':': return make_single(lx, TOK_COLON, start, line, col);
    case ';': return make_single(lx, TOK_SEMI, start, line, col);
    default:
      advance(lx);
      return make_token(lx, TOK_INVALID, start, line, col);
  }
}

const char *token_kind_name(TokenKind kind) {
  switch (kind) {
    case TOK_EOF: return "EOF";
    case TOK_INVALID: return "INVALID";
    case TOK_IDENT: return "IDENT";
    case TOK_INT_LIT: return "INT_LIT";
    case TOK_FLOAT_LIT: return "FLOAT_LIT";
    case TOK_STRING_LIT: return "STRING_LIT";
    case TOK_KW_FN: return "KW_FN";
    case TOK_KW_RETURN: return "KW_RETURN";
    case TOK_KW_LET: return "KW_LET";
    case TOK_KW_CONST: return "KW_CONST";
    case TOK_KW_IF: return "KW_IF";
    case TOK_KW_ELSE: return "KW_ELSE";
    case TOK_KW_WHILE: return "KW_WHILE";
    case TOK_KW_FOR: return "KW_FOR";
    case TOK_KW_BREAK: return "KW_BREAK";
    case TOK_KW_CONTINUE: return "KW_CONTINUE";
    case TOK_KW_TRUE: return "KW_TRUE";
    case TOK_KW_FALSE: return "KW_FALSE";
    case TOK_KW_VOID: return "KW_VOID";
    case TOK_KW_INT: return "KW_INT";
    case TOK_KW_FLOAT: return "KW_FLOAT";
    case TOK_KW_BOOL: return "KW_BOOL";
    case TOK_KW_STRING: return "KW_STRING";
    case TOK_KW_AMBA: return "KW_AMBA";
    case TOK_KW_RUSDI: return "KW_RUSDI";
    case TOK_KW_FUAD: return "KW_FUAD";
    case TOK_KW_IMUT: return "KW_IMUT";
    case TOK_PLUS: return "PLUS";
    case TOK_MINUS: return "MINUS";
    case TOK_STAR: return "STAR";
    case TOK_SLASH: return "SLASH";
    case TOK_BANG: return "BANG";
    case TOK_ASSIGN: return "ASSIGN";
    case TOK_EQ: return "EQ";
    case TOK_NE: return "NE";
    case TOK_LT: return "LT";
    case TOK_LE: return "LE";
    case TOK_GT: return "GT";
    case TOK_GE: return "GE";
    case TOK_AND_AND: return "AND_AND";
    case TOK_OR_OR: return "OR_OR";
    case TOK_ARROW: return "ARROW";
    case TOK_LPAREN: return "LPAREN";
    case TOK_RPAREN: return "RPAREN";
    case TOK_LBRACE: return "LBRACE";
    case TOK_RBRACE: return "RBRACE";
    case TOK_COMMA: return "COMMA";
    case TOK_COLON: return "COLON";
    case TOK_SEMI: return "SEMI";
    default: return "<unknown>";
  }
}
