#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/diag.h"
#include "../lexer/lexer.h"

typedef struct Parser {
  const char *file;
  const char *source;
  Lexer lx;
  Token cur;
  Token next;
  int had_error;
  int error_count;
  int max_errors;
  int panic_mode;
  int stop_parsing;
} Parser;

static void *xcalloc(size_t n, size_t sz) {
  void *p = calloc(n, sz);
  if (!p) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(1);
  }
  return p;
}

static void *xrealloc(void *ptr, size_t n, size_t sz) {
  void *p = realloc(ptr, n * sz);
  if (!p) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(1);
  }
  return p;
}

static char *dup_lexeme(const Token *t) {
  char *s = (char *)xcalloc(t->length + 1, 1);
  memcpy(s, t->start, t->length);
  s[t->length] = '\0';
  return s;
}

static void parser_init(Parser *p, const char *file, const char *source) {
  p->file = file;
  p->source = source;
  p->had_error = 0;
  p->error_count = 0;
  p->max_errors = 20;
  p->panic_mode = 0;
  p->stop_parsing = 0;
  lexer_init(&p->lx, file, source);
  p->cur = lexer_next(&p->lx);
  p->next = lexer_next(&p->lx);
}

static void advance(Parser *p) {
  p->cur = p->next;
  p->next = lexer_next(&p->lx);
}

static int check(Parser *p, TokenKind k) { return p->cur.kind == k; }

static int match(Parser *p, TokenKind k) {
  if (!check(p, k)) return 0;
  advance(p);
  return 1;
}

static void synchronize(Parser *p) {
  p->panic_mode = 0;

  while (!check(p, TOK_EOF)) {
    if (check(p, TOK_SEMI)) {
      advance(p);
      return;
    }

    if (check(p, TOK_RBRACE) || check(p, TOK_KW_FN) || check(p, TOK_KW_LET) ||
        check(p, TOK_KW_CONST) || check(p, TOK_KW_IF) || check(p, TOK_KW_WHILE) ||
        check(p, TOK_KW_FOR) || check(p, TOK_KW_BREAK) || check(p, TOK_KW_CONTINUE) ||
        check(p, TOK_KW_RETURN)) {
      return;
    }

    advance(p);
  }
}

static void parse_error(Parser *p, const char *msg) {
  if (p->panic_mode) return;

  p->panic_mode = 1;
  diag_error_source(p->file, p->source, p->cur.line, p->cur.col, "%s, found %s", msg,
                    token_kind_name(p->cur.kind));

  p->had_error = 1;
  p->error_count++;

  if (p->error_count >= p->max_errors) {
    diag_error(p->file, p->cur.line, p->cur.col, "too many parser errors (max %d)",
               p->max_errors);
    p->stop_parsing = 1;
  }
}

static Token consume(Parser *p, TokenKind k, const char *msg) {
  Token t = p->cur;
  if (check(p, k)) {
    advance(p);
    return t;
  }
  parse_error(p, msg);
  return t;
}

static TypeKind parse_type(Parser *p) {
  if (match(p, TOK_KW_INT) || match(p, TOK_KW_AMBA)) return TYPE_INT;
  if (match(p, TOK_KW_FLOAT) || match(p, TOK_KW_RUSDI)) return TYPE_FLOAT;
  if (match(p, TOK_KW_BOOL) || match(p, TOK_KW_FUAD)) return TYPE_BOOL;
  if (match(p, TOK_KW_STRING) || match(p, TOK_KW_IMUT)) return TYPE_STRING;
  if (match(p, TOK_KW_VOID)) return TYPE_VOID;
  parse_error(p, "expected type");
  return TYPE_VOID;
}

static Expr *parse_expression(Parser *p);
static Stmt *parse_statement(Parser *p);

static Expr *new_expr(ExprKind kind, int line, int col) {
  Expr *e = (Expr *)xcalloc(1, sizeof(Expr));
  e->kind = kind;
  e->line = line;
  e->col = col;
  return e;
}

static Stmt *new_stmt(StmtKind kind, int line, int col) {
  Stmt *s = (Stmt *)xcalloc(1, sizeof(Stmt));
  s->kind = kind;
  s->line = line;
  s->col = col;
  return s;
}

static Expr *parse_primary(Parser *p) {
  Token t = p->cur;

  if (match(p, TOK_INT_LIT)) {
    Expr *e = new_expr(EXPR_INT, t.line, t.col);
    e->as.int_val = strtoll(t.start, NULL, 10);
    return e;
  }

  if (match(p, TOK_FLOAT_LIT)) {
    char *tmp = dup_lexeme(&t);
    Expr *e = new_expr(EXPR_FLOAT, t.line, t.col);
    e->as.float_val = strtod(tmp, NULL);
    free(tmp);
    return e;
  }

  if (match(p, TOK_STRING_LIT)) {
    Expr *e = new_expr(EXPR_STRING, t.line, t.col);
    // strip quotes only
    size_t inner = t.length >= 2 ? t.length - 2 : 0;
    e->as.string_val = (char *)xcalloc(inner + 1, 1);
    if (inner > 0) memcpy(e->as.string_val, t.start + 1, inner);
    return e;
  }

  if (match(p, TOK_KW_TRUE)) {
    Expr *e = new_expr(EXPR_BOOL, t.line, t.col);
    e->as.bool_val = 1;
    return e;
  }

  if (match(p, TOK_KW_FALSE)) {
    Expr *e = new_expr(EXPR_BOOL, t.line, t.col);
    e->as.bool_val = 0;
    return e;
  }

  if (match(p, TOK_IDENT)) {
    char *name = dup_lexeme(&t);
    if (match(p, TOK_LPAREN)) {
      Expr *e = new_expr(EXPR_CALL, t.line, t.col);
      e->as.call.name = name;
      e->as.call.args = NULL;
      e->as.call.arg_count = 0;
      if (!check(p, TOK_RPAREN)) {
        for (;;) {
          Expr *arg = parse_expression(p);
          e->as.call.args = (Expr **)xrealloc(e->as.call.args, e->as.call.arg_count + 1,
                                              sizeof(Expr *));
          e->as.call.args[e->as.call.arg_count++] = arg;
          if (!match(p, TOK_COMMA)) break;
        }
      }
      consume(p, TOK_RPAREN, "expected ')' after arguments");
      return e;
    }

    Expr *e = new_expr(EXPR_IDENT, t.line, t.col);
    e->as.ident_name = name;
    return e;
  }

  if (match(p, TOK_LPAREN)) {
    Expr *inner = parse_expression(p);
    consume(p, TOK_RPAREN, "expected ')' after expression");
    return inner;
  }

  parse_error(p, "expected expression");
  advance(p);
  return new_expr(EXPR_INT, t.line, t.col);
}

static Expr *parse_unary(Parser *p) {
  if (check(p, TOK_BANG) || check(p, TOK_MINUS)) {
    Token op = p->cur;
    advance(p);
    Expr *rhs = parse_unary(p);
    Expr *e = new_expr(EXPR_UNARY, op.line, op.col);
    e->as.unary.op = op.kind;
    e->as.unary.expr = rhs;
    return e;
  }
  return parse_primary(p);
}

static Expr *parse_factor(Parser *p) {
  Expr *left = parse_unary(p);
  while (check(p, TOK_STAR) || check(p, TOK_SLASH)) {
    Token op = p->cur;
    advance(p);
    Expr *right = parse_unary(p);
    Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
    bin->as.binary.op = op.kind;
    bin->as.binary.left = left;
    bin->as.binary.right = right;
    left = bin;
  }
  return left;
}

static Expr *parse_term(Parser *p) {
  Expr *left = parse_factor(p);
  while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
    Token op = p->cur;
    advance(p);
    Expr *right = parse_factor(p);
    Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
    bin->as.binary.op = op.kind;
    bin->as.binary.left = left;
    bin->as.binary.right = right;
    left = bin;
  }
  return left;
}

static Expr *parse_comparison(Parser *p) {
  Expr *left = parse_term(p);
  while (check(p, TOK_LT) || check(p, TOK_LE) || check(p, TOK_GT) ||
         check(p, TOK_GE)) {
    Token op = p->cur;
    advance(p);
    Expr *right = parse_term(p);
    Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
    bin->as.binary.op = op.kind;
    bin->as.binary.left = left;
    bin->as.binary.right = right;
    left = bin;
  }
  return left;
}

static Expr *parse_equality(Parser *p) {
  Expr *left = parse_comparison(p);
  while (check(p, TOK_EQ) || check(p, TOK_NE)) {
    Token op = p->cur;
    advance(p);
    Expr *right = parse_comparison(p);
    Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
    bin->as.binary.op = op.kind;
    bin->as.binary.left = left;
    bin->as.binary.right = right;
    left = bin;
  }
  return left;
}

static Expr *parse_logical_and(Parser *p) {
  Expr *left = parse_equality(p);
  while (check(p, TOK_AND_AND)) {
    Token op = p->cur;
    advance(p);
    Expr *right = parse_equality(p);
    Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
    bin->as.binary.op = op.kind;
    bin->as.binary.left = left;
    bin->as.binary.right = right;
    left = bin;
  }
  return left;
}

static Expr *parse_expression(Parser *p) {
  Expr *left = parse_logical_and(p);
  while (check(p, TOK_OR_OR)) {
    Token op = p->cur;
    advance(p);
    Expr *right = parse_logical_and(p);
    Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
    bin->as.binary.op = op.kind;
    bin->as.binary.left = left;
    bin->as.binary.right = right;
    left = bin;
  }
  return left;
}

static Stmt *parse_block(Parser *p) {
  Token lb = consume(p, TOK_LBRACE, "expected '{'");
  Stmt *blk = new_stmt(STMT_BLOCK, lb.line, lb.col);
  blk->as.block.items = NULL;
  blk->as.block.count = 0;

  while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->stop_parsing) {
    Stmt *it = parse_statement(p);
    blk->as.block.items = (Stmt **)xrealloc(blk->as.block.items, blk->as.block.count + 1,
                                            sizeof(Stmt *));
    blk->as.block.items[blk->as.block.count++] = it;

    if (p->panic_mode) synchronize(p);
  }

  consume(p, TOK_RBRACE, "expected '}'");
  return blk;
}

static Stmt *parse_if(Parser *p) {
  Token kw = consume(p, TOK_KW_IF, "expected 'if'");
  consume(p, TOK_LPAREN, "expected '(' after if");
  Expr *cond = parse_expression(p);
  consume(p, TOK_RPAREN, "expected ')' after if condition");
  Stmt *then_branch = parse_statement(p);
  Stmt *else_branch = NULL;
  if (match(p, TOK_KW_ELSE)) else_branch = parse_statement(p);

  Stmt *s = new_stmt(STMT_IF, kw.line, kw.col);
  s->as.if_stmt.cond = cond;
  s->as.if_stmt.then_branch = then_branch;
  s->as.if_stmt.else_branch = else_branch;
  return s;
}

static Stmt *parse_while(Parser *p) {
  Token kw = consume(p, TOK_KW_WHILE, "expected 'while'");
  consume(p, TOK_LPAREN, "expected '(' after while");
  Expr *cond = parse_expression(p);
  consume(p, TOK_RPAREN, "expected ')' after while condition");
  Stmt *body = parse_statement(p);

  Stmt *s = new_stmt(STMT_WHILE, kw.line, kw.col);
  s->as.while_stmt.cond = cond;
  s->as.while_stmt.body = body;
  return s;
}

static Stmt *parse_for_clause_stmt(Parser *p, int with_semicolon) {
  if (check(p, TOK_KW_LET) || check(p, TOK_KW_CONST)) {
    int is_const = check(p, TOK_KW_CONST);
    Token kw = p->cur;
    advance(p);
    Token name = consume(p, TOK_IDENT, "expected identifier");

    int has_type = 0;
    TypeKind type = TYPE_VOID;
    if (match(p, TOK_COLON)) {
      has_type = 1;
      type = parse_type(p);
    }

    consume(p, TOK_ASSIGN, "expected '=' in declaration");
    Expr *init = parse_expression(p);
    if (with_semicolon) consume(p, TOK_SEMI, "expected ';' after declaration");

    Stmt *s = new_stmt(is_const ? STMT_CONST_DECL : STMT_VAR_DECL, kw.line, kw.col);
    s->as.var_decl.name = dup_lexeme(&name);
    s->as.var_decl.has_type = has_type;
    s->as.var_decl.type = type;
    s->as.var_decl.init = init;
    return s;
  }

  if (check(p, TOK_IDENT) && p->next.kind == TOK_ASSIGN) {
    Token name = p->cur;
    advance(p);
    consume(p, TOK_ASSIGN, "expected '='");
    Expr *rhs = parse_expression(p);
    if (with_semicolon) consume(p, TOK_SEMI, "expected ';' after assignment");

    Stmt *s = new_stmt(STMT_ASSIGN, name.line, name.col);
    s->as.assign.name = dup_lexeme(&name);
    s->as.assign.value = rhs;
    return s;
  }

  Token t = p->cur;
  Expr *e = parse_expression(p);
  if (with_semicolon) consume(p, TOK_SEMI, "expected ';' after expression");
  Stmt *s = new_stmt(STMT_EXPR, t.line, t.col);
  s->as.expr_stmt.expr = e;
  return s;
}

static Stmt *parse_for(Parser *p) {
  Token kw = consume(p, TOK_KW_FOR, "expected 'for'");
  consume(p, TOK_LPAREN, "expected '(' after for");

  Stmt *init = NULL;
  if (!check(p, TOK_SEMI)) {
    init = parse_for_clause_stmt(p, 1);
  } else {
    consume(p, TOK_SEMI, "expected ';' after for initializer");
  }

  Expr *cond = NULL;
  if (!check(p, TOK_SEMI)) {
    cond = parse_expression(p);
  }
  consume(p, TOK_SEMI, "expected ';' after for condition");

  Stmt *update = NULL;
  if (!check(p, TOK_RPAREN)) {
    update = parse_for_clause_stmt(p, 0);
  }

  consume(p, TOK_RPAREN, "expected ')' after for clauses");
  Stmt *body = parse_statement(p);

  Stmt *s = new_stmt(STMT_FOR, kw.line, kw.col);
  s->as.for_stmt.init = init;
  s->as.for_stmt.cond = cond;
  s->as.for_stmt.update = update;
  s->as.for_stmt.body = body;
  return s;
}

static Stmt *parse_statement(Parser *p) {
  if (check(p, TOK_LBRACE)) return parse_block(p);
  if (check(p, TOK_KW_IF)) return parse_if(p);
  if (check(p, TOK_KW_WHILE)) return parse_while(p);
  if (check(p, TOK_KW_FOR)) return parse_for(p);

  if (check(p, TOK_KW_LET) || check(p, TOK_KW_CONST)) {
    int is_const = check(p, TOK_KW_CONST);
    Token kw = p->cur;
    advance(p);
    Token name = consume(p, TOK_IDENT, "expected identifier");

    int has_type = 0;
    TypeKind type = TYPE_VOID;
    if (match(p, TOK_COLON)) {
      has_type = 1;
      type = parse_type(p);
    }

    consume(p, TOK_ASSIGN, "expected '=' in declaration");
    Expr *init = parse_expression(p);
    consume(p, TOK_SEMI, "expected ';' after declaration");

    Stmt *s = new_stmt(is_const ? STMT_CONST_DECL : STMT_VAR_DECL, kw.line, kw.col);
    s->as.var_decl.name = dup_lexeme(&name);
    s->as.var_decl.has_type = has_type;
    s->as.var_decl.type = type;
    s->as.var_decl.init = init;
    return s;
  }

  if (check(p, TOK_KW_RETURN)) {
    Token kw = p->cur;
    advance(p);
    Stmt *s = new_stmt(STMT_RETURN, kw.line, kw.col);
    s->as.ret.has_expr = !check(p, TOK_SEMI);
    s->as.ret.expr = s->as.ret.has_expr ? parse_expression(p) : NULL;
    consume(p, TOK_SEMI, "expected ';' after return");
    return s;
  }

  if (check(p, TOK_KW_BREAK)) {
    Token kw = p->cur;
    advance(p);
    consume(p, TOK_SEMI, "expected ';' after break");
    return new_stmt(STMT_BREAK, kw.line, kw.col);
  }

  if (check(p, TOK_KW_CONTINUE)) {
    Token kw = p->cur;
    advance(p);
    consume(p, TOK_SEMI, "expected ';' after continue");
    return new_stmt(STMT_CONTINUE, kw.line, kw.col);
  }

  if (check(p, TOK_IDENT) && p->next.kind == TOK_ASSIGN) {
    Token name = p->cur;
    advance(p);
    consume(p, TOK_ASSIGN, "expected '='");
    Expr *rhs = parse_expression(p);
    consume(p, TOK_SEMI, "expected ';' after assignment");

    Stmt *s = new_stmt(STMT_ASSIGN, name.line, name.col);
    s->as.assign.name = dup_lexeme(&name);
    s->as.assign.value = rhs;
    return s;
  }

  Token t = p->cur;
  Expr *e = parse_expression(p);
  consume(p, TOK_SEMI, "expected ';' after expression");
  Stmt *s = new_stmt(STMT_EXPR, t.line, t.col);
  s->as.expr_stmt.expr = e;
  return s;
}

static FunctionDecl parse_function(Parser *p) {
  consume(p, TOK_KW_FN, "expected 'fn'");
  Token name_tok = consume(p, TOK_IDENT, "expected function name");
  consume(p, TOK_LPAREN, "expected '('");

  FunctionDecl fn;
  memset(&fn, 0, sizeof(fn));
  fn.name = dup_lexeme(&name_tok);
  fn.params = NULL;
  fn.param_count = 0;

  if (!check(p, TOK_RPAREN)) {
    for (;;) {
      Token pname = consume(p, TOK_IDENT, "expected parameter name");
      consume(p, TOK_COLON, "expected ':' after parameter name");
      TypeKind ptype = parse_type(p);

      fn.params = (Param *)xrealloc(fn.params, fn.param_count + 1, sizeof(Param));
      fn.params[fn.param_count].name = dup_lexeme(&pname);
      fn.params[fn.param_count].type = ptype;
      fn.param_count++;

      if (!match(p, TOK_COMMA)) break;
    }
  }

  consume(p, TOK_RPAREN, "expected ')' after parameters");
  consume(p, TOK_ARROW, "expected '->' after parameters");
  fn.return_type = parse_type(p);
  fn.body = parse_block(p);
  return fn;
}

Program *parse_program(const char *file, const char *source, int *had_error) {
  Parser p;
  parser_init(&p, file, source);

  Program *prog = (Program *)xcalloc(1, sizeof(Program));
  while (!check(&p, TOK_EOF) && !p.stop_parsing) {
    if (!check(&p, TOK_KW_FN)) {
      parse_error(&p, "expected top-level 'fn'");
      synchronize(&p);
      continue;
    }

    FunctionDecl fn = parse_function(&p);
    prog->funcs = (FunctionDecl *)xrealloc(prog->funcs, prog->func_count + 1,
                                           sizeof(FunctionDecl));
    prog->funcs[prog->func_count++] = fn;

    if (p.panic_mode) synchronize(&p);
  }

  if (had_error) *had_error = p.had_error;
  return prog;
}

static void expr_free(Expr *e) {
  if (!e) return;
  switch (e->kind) {
    case EXPR_STRING: free(e->as.string_val); break;
    case EXPR_IDENT: free(e->as.ident_name); break;
    case EXPR_UNARY: expr_free(e->as.unary.expr); break;
    case EXPR_BINARY:
      expr_free(e->as.binary.left);
      expr_free(e->as.binary.right);
      break;
    case EXPR_CALL:
      free(e->as.call.name);
      for (size_t i = 0; i < e->as.call.arg_count; i++) expr_free(e->as.call.args[i]);
      free(e->as.call.args);
      break;
    default: break;
  }
  free(e);
}

static void stmt_free(Stmt *s) {
  if (!s) return;
  switch (s->kind) {
    case STMT_BLOCK:
      for (size_t i = 0; i < s->as.block.count; i++) stmt_free(s->as.block.items[i]);
      free(s->as.block.items);
      break;
    case STMT_VAR_DECL:
    case STMT_CONST_DECL:
      free(s->as.var_decl.name);
      expr_free(s->as.var_decl.init);
      break;
    case STMT_ASSIGN:
      free(s->as.assign.name);
      expr_free(s->as.assign.value);
      break;
    case STMT_EXPR:
      expr_free(s->as.expr_stmt.expr);
      break;
    case STMT_RETURN:
      if (s->as.ret.has_expr) expr_free(s->as.ret.expr);
      break;
    case STMT_IF:
      expr_free(s->as.if_stmt.cond);
      stmt_free(s->as.if_stmt.then_branch);
      stmt_free(s->as.if_stmt.else_branch);
      break;
    case STMT_WHILE:
      expr_free(s->as.while_stmt.cond);
      stmt_free(s->as.while_stmt.body);
      break;
    case STMT_FOR:
      stmt_free(s->as.for_stmt.init);
      expr_free(s->as.for_stmt.cond);
      stmt_free(s->as.for_stmt.update);
      stmt_free(s->as.for_stmt.body);
      break;
    case STMT_BREAK:
    case STMT_CONTINUE:
      break;
  }
  free(s);
}

void program_free(Program *p) {
  if (!p) return;
  for (size_t i = 0; i < p->func_count; i++) {
    FunctionDecl *fn = &p->funcs[i];
    free(fn->name);
    for (size_t j = 0; j < fn->param_count; j++) free(fn->params[j].name);
    free(fn->params);
    stmt_free(fn->body);
  }
  free(p->funcs);
  free(p);
}
