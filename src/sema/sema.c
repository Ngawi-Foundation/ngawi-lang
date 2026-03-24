#include "sema.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/diag.h"
#include "../lexer/token.h"

typedef struct VarSymbol {
  char *name;
  TypeKind type;
  int is_const;
  struct VarSymbol *next;
} VarSymbol;

typedef struct Scope {
  VarSymbol *vars;
  struct Scope *parent;
} Scope;

typedef struct FuncSymbol {
  const char *name;
  const FunctionDecl *decl;
} FuncSymbol;

typedef struct Sema {
  const char *file;
  const char *source;
  int had_error;
  int error_count;
  int max_errors;
  int stop_analysis;
  int loop_depth;
  Scope *scope;
  FuncSymbol *funcs;
  size_t func_count;
  const FunctionDecl *current_fn;
} Sema;

const char *type_kind_name(TypeKind t) {
  switch (t) {
    case TYPE_INT: return "int";
    case TYPE_FLOAT: return "float";
    case TYPE_BOOL: return "bool";
    case TYPE_STRING: return "string";
    case TYPE_INT_ARRAY: return "int[]";
    case TYPE_INT2_ARRAY: return "int[][]";
    case TYPE_FLOAT_ARRAY: return "float[]";
    case TYPE_FLOAT2_ARRAY: return "float[][]";
    case TYPE_BOOL_ARRAY: return "bool[]";
    case TYPE_BOOL2_ARRAY: return "bool[][]";
    case TYPE_STRING_ARRAY: return "string[]";
    case TYPE_STRING2_ARRAY: return "string[][]";
    case TYPE_VOID: return "void";
    default: return "<unknown>";
  }
}

int type_is_numeric(TypeKind t) { return t == TYPE_INT || t == TYPE_FLOAT; }

static int type_eq(TypeKind a, TypeKind b) { return a == b; }

static int type_is_array(TypeKind t) {
  return t == TYPE_INT_ARRAY || t == TYPE_INT2_ARRAY || t == TYPE_FLOAT_ARRAY ||
         t == TYPE_FLOAT2_ARRAY || t == TYPE_BOOL_ARRAY || t == TYPE_BOOL2_ARRAY ||
         t == TYPE_STRING_ARRAY || t == TYPE_STRING2_ARRAY;
}

static TypeKind array_of_scalar(TypeKind t) {
  switch (t) {
    case TYPE_INT: return TYPE_INT_ARRAY;
    case TYPE_FLOAT: return TYPE_FLOAT_ARRAY;
    case TYPE_BOOL: return TYPE_BOOL_ARRAY;
    case TYPE_STRING: return TYPE_STRING_ARRAY;
    default: return TYPE_VOID;
  }
}

static TypeKind array_elem_type(TypeKind t) {
  switch (t) {
    case TYPE_INT_ARRAY: return TYPE_INT;
    case TYPE_INT2_ARRAY: return TYPE_INT_ARRAY;
    case TYPE_FLOAT_ARRAY: return TYPE_FLOAT;
    case TYPE_FLOAT2_ARRAY: return TYPE_FLOAT_ARRAY;
    case TYPE_BOOL_ARRAY: return TYPE_BOOL;
    case TYPE_BOOL2_ARRAY: return TYPE_BOOL_ARRAY;
    case TYPE_STRING_ARRAY: return TYPE_STRING;
    case TYPE_STRING2_ARRAY: return TYPE_STRING_ARRAY;
    default: return TYPE_VOID;
  }
}

static TypeKind array_of_elem(TypeKind t) {
  if (t == TYPE_INT_ARRAY) return TYPE_INT2_ARRAY;
  if (t == TYPE_FLOAT_ARRAY) return TYPE_FLOAT2_ARRAY;
  if (t == TYPE_BOOL_ARRAY) return TYPE_BOOL2_ARRAY;
  if (t == TYPE_STRING_ARRAY) return TYPE_STRING2_ARRAY;
  return array_of_scalar(t);
}

static int expr_is_empty_array_literal(const Expr *e) {
  return e && e->kind == EXPR_ARRAY_LITERAL && e->as.array_lit.count == 0;
}

static int min3(int a, int b, int c) {
  int m = a < b ? a : b;
  return m < c ? m : c;
}

static int edit_distance(const char *a, const char *b) {
  size_t n = strlen(a);
  size_t m = strlen(b);

  int *prev = (int *)malloc((m + 1) * sizeof(int));
  int *curr = (int *)malloc((m + 1) * sizeof(int));
  if (!prev || !curr) {
    free(prev);
    free(curr);
    return 9999;
  }

  for (size_t j = 0; j <= m; j++) prev[j] = (int)j;

  for (size_t i = 1; i <= n; i++) {
    curr[0] = (int)i;
    for (size_t j = 1; j <= m; j++) {
      int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      curr[j] = min3(prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost);
    }
    int *tmp = prev;
    prev = curr;
    curr = tmp;
  }

  int d = prev[m];
  free(prev);
  free(curr);
  return d;
}

static int should_stop(Sema *s) { return s->stop_analysis; }

static void sema_error(Sema *s, int line, int col, const char *fmt, ...) {
  char msg[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  diag_error_source(s->file, s->source, line, col, "%s", msg);
  s->had_error = 1;
  s->error_count++;

  if (s->error_count >= s->max_errors && !s->stop_analysis) {
    diag_error(s->file, line, col, "too many semantic errors (max %d)", s->max_errors);
    s->stop_analysis = 1;
  }
}

static void sema_note(Sema *s, int line, int col, const char *fmt, ...) {
  if (should_stop(s)) return;

  char msg[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  diag_note_source(s->file, s->source, line, col, "%s", msg);
}

static const char *suggest_var_name(Sema *s, const char *name) {
  const char *best = NULL;
  int best_dist = 9999;

  for (Scope *sc = s->scope; sc; sc = sc->parent) {
    for (VarSymbol *v = sc->vars; v; v = v->next) {
      int d = edit_distance(name, v->name);
      if (d < best_dist) {
        best_dist = d;
        best = v->name;
      }
    }
  }

  if (best && best_dist <= 2) return best;
  return NULL;
}

static const char *suggest_fn_name(Sema *s, const char *name) {
  const char *best = NULL;
  int best_dist = 9999;

  for (size_t i = 0; i < s->func_count; i++) {
    int d = edit_distance(name, s->funcs[i].name);
    if (d < best_dist) {
      best_dist = d;
      best = s->funcs[i].name;
    }
  }

  if (best && best_dist <= 2) return best;
  return NULL;
}

static void push_scope(Sema *s) {
  Scope *sc = (Scope *)calloc(1, sizeof(Scope));
  if (!sc) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(1);
  }
  sc->parent = s->scope;
  s->scope = sc;
}

static void pop_scope(Sema *s) {
  Scope *sc = s->scope;
  if (!sc) return;
  VarSymbol *v = sc->vars;
  while (v) {
    VarSymbol *next = v->next;
    free(v->name);
    free(v);
    v = next;
  }
  s->scope = sc->parent;
  free(sc);
}

static char *dup_string(const char *s) {
  size_t n = strlen(s);
  char *out = (char *)malloc(n + 1);
  if (!out) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(1);
  }
  memcpy(out, s, n + 1);
  return out;
}

static VarSymbol *find_in_current_scope(Sema *s, const char *name) {
  for (VarSymbol *v = s->scope ? s->scope->vars : NULL; v; v = v->next) {
    if (strcmp(v->name, name) == 0) return v;
  }
  return NULL;
}

static VarSymbol *lookup_var(Sema *s, const char *name) {
  for (Scope *sc = s->scope; sc; sc = sc->parent) {
    for (VarSymbol *v = sc->vars; v; v = v->next) {
      if (strcmp(v->name, name) == 0) return v;
    }
  }
  return NULL;
}

static void declare_var(Sema *s, int line, int col, const char *name, TypeKind type, int is_const) {
  if (!s->scope) push_scope(s);
  if (find_in_current_scope(s, name)) {
    sema_error(s, line, col, "redeclaration of '%s'", name);
    return;
  }

  VarSymbol *v = (VarSymbol *)calloc(1, sizeof(VarSymbol));
  if (!v) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(1);
  }
  v->name = dup_string(name);
  v->type = type;
  v->is_const = is_const;
  v->next = s->scope->vars;
  s->scope->vars = v;
}

static const FuncSymbol *lookup_fn(Sema *s, const char *name) {
  for (size_t i = 0; i < s->func_count; i++) {
    if (strcmp(s->funcs[i].name, name) == 0) return &s->funcs[i];
  }
  return NULL;
}

static TypeKind check_expr(Sema *s, Expr *e);
static void check_stmt(Sema *s, Stmt *st);

static TypeKind set_expr_type(Expr *e, TypeKind t) {
  e->inferred_type = t;
  return t;
}

static TypeKind check_call(Sema *s, Expr *e) {
  if (should_stop(s)) return set_expr_type(e, TYPE_VOID);

  if (strcmp(e->as.call.name, "print") == 0) {
    for (size_t i = 0; i < e->as.call.arg_count; i++) {
      TypeKind at = check_expr(s, e->as.call.args[i]);
      if (should_stop(s)) return set_expr_type(e, TYPE_VOID);
      if (at == TYPE_VOID) {
        // Skip cascade error. Root error is reported in child expression.
        continue;
      }
    }
    return set_expr_type(e, TYPE_VOID);
  }

  if (strcmp(e->as.call.name, "to_int") == 0 || strcmp(e->as.call.name, "to_amba") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "to_int/to_amba expects 1 argument, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(at, TYPE_INT) && !type_eq(at, TYPE_FLOAT)) {
      sema_error(s, e->line, e->col, "to_int/to_amba expects int or float, got '%s'",
                 type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_INT);
  }

  if (strcmp(e->as.call.name, "to_float") == 0 || strcmp(e->as.call.name, "to_rusdi") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "to_float/to_rusdi expects 1 argument, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(at, TYPE_INT) && !type_eq(at, TYPE_FLOAT)) {
      sema_error(s, e->line, e->col, "to_float/to_rusdi expects int or float, got '%s'",
                 type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_FLOAT);
  }

  if (strcmp(e->as.call.name, "len") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "len expects 1 argument, got %zu", e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(at, TYPE_STRING) && !type_is_array(at)) {
      sema_error(s, e->line, e->col, "len expects string or array, got '%s'", type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_INT);
  }

  if (strcmp(e->as.call.name, "push") == 0) {
    if (e->as.call.arg_count != 2) {
      sema_error(s, e->line, e->col, "push expects 2 arguments, got %zu", e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    TypeKind vt = check_expr(s, e->as.call.args[1]);
    if (at == TYPE_VOID || vt == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_is_array(at)) {
      sema_error(s, e->line, e->col, "push expects array as first argument, got '%s'",
                 type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind elem = array_elem_type(at);
    if (!type_eq(elem, vt)) {
      sema_error(s, e->line, e->col, "push on '%s' expects value '%s', got '%s'",
                 type_kind_name(at), type_kind_name(elem), type_kind_name(vt));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, at);
  }

  if (strcmp(e->as.call.name, "pop") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "pop expects 1 argument, got %zu", e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_is_array(at)) {
      sema_error(s, e->line, e->col, "pop expects array, got '%s'", type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, at);
  }

  if (strcmp(e->as.call.name, "contains") == 0) {
    if (e->as.call.arg_count != 2) {
      sema_error(s, e->line, e->col, "contains expects 2 arguments, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind a0 = check_expr(s, e->as.call.args[0]);
    TypeKind a1 = check_expr(s, e->as.call.args[1]);
    if (a0 == TYPE_VOID || a1 == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(a0, TYPE_STRING) || !type_eq(a1, TYPE_STRING)) {
      sema_error(s, e->line, e->col, "contains expects (string, string), got ('%s', '%s')",
                 type_kind_name(a0), type_kind_name(a1));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_BOOL);
  }

  if (strcmp(e->as.call.name, "starts_with") == 0) {
    if (e->as.call.arg_count != 2) {
      sema_error(s, e->line, e->col, "starts_with expects 2 arguments, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind a0 = check_expr(s, e->as.call.args[0]);
    TypeKind a1 = check_expr(s, e->as.call.args[1]);
    if (a0 == TYPE_VOID || a1 == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(a0, TYPE_STRING) || !type_eq(a1, TYPE_STRING)) {
      sema_error(s, e->line, e->col,
                 "starts_with expects (string, string), got ('%s', '%s')",
                 type_kind_name(a0), type_kind_name(a1));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_BOOL);
  }

  if (strcmp(e->as.call.name, "ends_with") == 0) {
    if (e->as.call.arg_count != 2) {
      sema_error(s, e->line, e->col, "ends_with expects 2 arguments, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind a0 = check_expr(s, e->as.call.args[0]);
    TypeKind a1 = check_expr(s, e->as.call.args[1]);
    if (a0 == TYPE_VOID || a1 == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(a0, TYPE_STRING) || !type_eq(a1, TYPE_STRING)) {
      sema_error(s, e->line, e->col,
                 "ends_with expects (string, string), got ('%s', '%s')",
                 type_kind_name(a0), type_kind_name(a1));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_BOOL);
  }

  if (strcmp(e->as.call.name, "to_lower") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "to_lower expects 1 argument, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(at, TYPE_STRING)) {
      sema_error(s, e->line, e->col, "to_lower expects string, got '%s'", type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_STRING);
  }

  if (strcmp(e->as.call.name, "to_upper") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "to_upper expects 1 argument, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(at, TYPE_STRING)) {
      sema_error(s, e->line, e->col, "to_upper expects string, got '%s'", type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_STRING);
  }

  if (strcmp(e->as.call.name, "trim") == 0) {
    if (e->as.call.arg_count != 1) {
      sema_error(s, e->line, e->col, "trim expects 1 argument, got %zu",
                 e->as.call.arg_count);
      return set_expr_type(e, TYPE_VOID);
    }
    TypeKind at = check_expr(s, e->as.call.args[0]);
    if (at == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
    if (!type_eq(at, TYPE_STRING)) {
      sema_error(s, e->line, e->col, "trim expects string, got '%s'", type_kind_name(at));
      return set_expr_type(e, TYPE_VOID);
    }
    return set_expr_type(e, TYPE_STRING);
  }

  const FuncSymbol *fn = lookup_fn(s, e->as.call.name);
  if (!fn) {
    sema_error(s, e->line, e->col, "undefined function '%s'", e->as.call.name);
    const char *suggest = suggest_fn_name(s, e->as.call.name);
    if (suggest) sema_note(s, e->line, e->col, "did you mean '%s'?", suggest);
    return set_expr_type(e, TYPE_VOID);
  }

  if (e->as.call.arg_count != fn->decl->param_count) {
    sema_error(s, e->line, e->col, "function '%s' expects %zu argument(s), got %zu",
               e->as.call.name, fn->decl->param_count, e->as.call.arg_count);
    return set_expr_type(e, fn->decl->return_type);
  }

  for (size_t i = 0; i < e->as.call.arg_count; i++) {
    TypeKind got = check_expr(s, e->as.call.args[i]);
    TypeKind exp = fn->decl->params[i].type;
    if (should_stop(s)) return set_expr_type(e, fn->decl->return_type);
    if (got == TYPE_VOID) continue;
    if (!type_eq(got, exp)) {
      sema_error(s, e->as.call.args[i]->line, e->as.call.args[i]->col,
                 "argument %zu of '%s' expects '%s', got '%s'", i + 1,
                 e->as.call.name, type_kind_name(exp), type_kind_name(got));
    }
  }

  return set_expr_type(e, fn->decl->return_type);
}

static TypeKind check_expr(Sema *s, Expr *e) {
  if (should_stop(s)) return set_expr_type(e, TYPE_VOID);

  switch (e->kind) {
    case EXPR_INT:
      return set_expr_type(e, TYPE_INT);
    case EXPR_FLOAT:
      return set_expr_type(e, TYPE_FLOAT);
    case EXPR_STRING:
      return set_expr_type(e, TYPE_STRING);
    case EXPR_BOOL:
      return set_expr_type(e, TYPE_BOOL);
    case EXPR_IDENT: {
      VarSymbol *v = lookup_var(s, e->as.ident_name);
      if (!v) {
        sema_error(s, e->line, e->col, "use of undeclared identifier '%s'", e->as.ident_name);
        const char *suggest = suggest_var_name(s, e->as.ident_name);
        if (suggest) sema_note(s, e->line, e->col, "did you mean '%s'?", suggest);
        return set_expr_type(e, TYPE_VOID);
      }
      return set_expr_type(e, v->type);
    }
    case EXPR_ARRAY_LITERAL: {
      if (e->as.array_lit.count == 0) {
        sema_error(s, e->line, e->col,
                   "empty array literal requires explicit array type context");
        return set_expr_type(e, TYPE_VOID);
      }

      TypeKind elem_t = TYPE_VOID;
      for (size_t i = 0; i < e->as.array_lit.count; i++) {
        TypeKind it = check_expr(s, e->as.array_lit.items[i]);
        if (it == TYPE_VOID) continue;

        if (elem_t == TYPE_VOID) {
          elem_t = it;
          continue;
        }

        if (!type_eq(it, elem_t)) {
          sema_error(s, e->as.array_lit.items[i]->line, e->as.array_lit.items[i]->col,
                     "array literal expects '%s' elements, got '%s'", type_kind_name(elem_t),
                     type_kind_name(it));
        }
      }

      TypeKind arr_t = array_of_elem(elem_t);
      if (arr_t == TYPE_VOID) {
        sema_error(s, e->line, e->col, "array literal element type is not supported");
        return set_expr_type(e, TYPE_VOID);
      }
      return set_expr_type(e, arr_t);
    }
    case EXPR_INDEX: {
      TypeKind tt = check_expr(s, e->as.index.target);
      TypeKind it = check_expr(s, e->as.index.index);
      if (tt == TYPE_VOID || it == TYPE_VOID) return set_expr_type(e, TYPE_VOID);
      if (!type_is_array(tt)) {
        sema_error(s, e->line, e->col, "indexing expects array target, got '%s'",
                   type_kind_name(tt));
        return set_expr_type(e, TYPE_VOID);
      }
      if (!type_eq(it, TYPE_INT)) {
        sema_error(s, e->line, e->col, "array index must be int, got '%s'", type_kind_name(it));
        return set_expr_type(e, TYPE_VOID);
      }
      return set_expr_type(e, array_elem_type(tt));
    }
    case EXPR_CALL:
      return check_call(s, e);
    case EXPR_UNARY: {
      TypeKind rhs = check_expr(s, e->as.unary.expr);
      if (rhs == TYPE_VOID) return set_expr_type(e, TYPE_VOID);

      if (e->as.unary.op == TOK_BANG) {
        if (!type_eq(rhs, TYPE_BOOL)) {
          sema_error(s, e->line, e->col, "operator '!' expects bool, got '%s'",
                     type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, TYPE_BOOL);
      }
      if (e->as.unary.op == TOK_MINUS) {
        if (!type_is_numeric(rhs)) {
          sema_error(s, e->line, e->col, "unary '-' expects numeric type, got '%s'",
                     type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, rhs);
      }
      return set_expr_type(e, TYPE_VOID);
    }
    case EXPR_BINARY: {
      TypeKind lhs = check_expr(s, e->as.binary.left);
      TypeKind rhs = check_expr(s, e->as.binary.right);
      int op = e->as.binary.op;

      if (lhs == TYPE_VOID || rhs == TYPE_VOID) return set_expr_type(e, TYPE_VOID);

      if (op == TOK_PLUS) {
        if (type_eq(lhs, TYPE_STRING) && type_eq(rhs, TYPE_STRING)) {
          return set_expr_type(e, TYPE_STRING);
        }

        if (!type_is_numeric(lhs) || !type_is_numeric(rhs) || !type_eq(lhs, rhs)) {
          sema_error(s, e->line, e->col,
                     "'+' expects same numeric types or two strings, got '%s' and '%s'",
                     type_kind_name(lhs), type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, lhs);
      }

      if (op == TOK_MINUS || op == TOK_STAR || op == TOK_SLASH) {
        if (!type_is_numeric(lhs) || !type_is_numeric(rhs) || !type_eq(lhs, rhs)) {
          sema_error(s, e->line, e->col,
                     "arithmetic operator expects same numeric types, got '%s' and '%s'",
                     type_kind_name(lhs), type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, lhs);
      }

      if (op == TOK_PERCENT) {
        if (!type_eq(lhs, TYPE_INT) || !type_eq(rhs, TYPE_INT)) {
          sema_error(s, e->line, e->col,
                     "'%%' operator expects int operands, got '%s' and '%s'",
                     type_kind_name(lhs), type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, TYPE_INT);
      }

      if (op == TOK_LT || op == TOK_LE || op == TOK_GT || op == TOK_GE) {
        if (!type_is_numeric(lhs) || !type_is_numeric(rhs) || !type_eq(lhs, rhs)) {
          sema_error(s, e->line, e->col,
                     "comparison operator expects same numeric types, got '%s' and '%s'",
                     type_kind_name(lhs), type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, TYPE_BOOL);
      }

      if (op == TOK_EQ || op == TOK_NE) {
        if (!type_eq(lhs, rhs)) {
          sema_error(s, e->line, e->col,
                     "equality operator expects same types, got '%s' and '%s'",
                     type_kind_name(lhs), type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, TYPE_BOOL);
      }

      if (op == TOK_AND_AND || op == TOK_OR_OR) {
        if (!type_eq(lhs, TYPE_BOOL) || !type_eq(rhs, TYPE_BOOL)) {
          sema_error(s, e->line, e->col,
                     "logical operator expects bool operands, got '%s' and '%s'",
                     type_kind_name(lhs), type_kind_name(rhs));
          return set_expr_type(e, TYPE_VOID);
        }
        return set_expr_type(e, TYPE_BOOL);
      }

      return set_expr_type(e, TYPE_VOID);
    }
  }

  return set_expr_type(e, TYPE_VOID);
}

static int stmt_guarantees_return(Stmt *st) {
  if (!st) return 0;
  switch (st->kind) {
    case STMT_RETURN:
      return 1;
    case STMT_BLOCK:
      for (size_t i = 0; i < st->as.block.count; i++) {
        if (stmt_guarantees_return(st->as.block.items[i])) return 1;
      }
      return 0;
    case STMT_IF:
      if (!st->as.if_stmt.else_branch) return 0;
      return stmt_guarantees_return(st->as.if_stmt.then_branch) &&
             stmt_guarantees_return(st->as.if_stmt.else_branch);
    default:
      return 0;
  }
}

static void check_stmt(Sema *s, Stmt *st) {
  if (should_stop(s)) return;

  switch (st->kind) {
    case STMT_BLOCK:
      push_scope(s);
      for (size_t i = 0; i < st->as.block.count; i++) {
        check_stmt(s, st->as.block.items[i]);
        if (should_stop(s)) break;
      }
      pop_scope(s);
      break;

    case STMT_VAR_DECL:
    case STMT_CONST_DECL: {
      TypeKind init_t = TYPE_VOID;
      if (st->as.var_decl.has_type && expr_is_empty_array_literal(st->as.var_decl.init)) {
        if (!type_is_array(st->as.var_decl.type)) {
          sema_error(s, st->line, st->col,
                     "empty array literal can only initialize an array-typed variable");
        } else {
          init_t = st->as.var_decl.type;
          set_expr_type(st->as.var_decl.init, init_t);
        }
      } else {
        init_t = check_expr(s, st->as.var_decl.init);
      }

      TypeKind final_t = init_t;
      if (st->as.var_decl.has_type) {
        final_t = st->as.var_decl.type;
        if (init_t != TYPE_VOID && !type_eq(final_t, init_t)) {
          sema_error(s, st->line, st->col,
                     "cannot assign '%s' to variable '%s' of type '%s'",
                     type_kind_name(init_t), st->as.var_decl.name, type_kind_name(final_t));
        }
      }
      if (final_t == TYPE_VOID) {
        sema_error(s, st->line, st->col, "variable '%s' cannot have type 'void'",
                   st->as.var_decl.name);
      }
      st->as.var_decl.has_type = 1;
      st->as.var_decl.type = final_t;
      declare_var(s, st->line, st->col, st->as.var_decl.name, final_t,
                  st->kind == STMT_CONST_DECL);
      break;
    }

    case STMT_ASSIGN: {
      VarSymbol *v = lookup_var(s, st->as.assign.name);
      if (!v) {
        sema_error(s, st->line, st->col, "assignment to undeclared variable '%s'",
                   st->as.assign.name);
        const char *suggest = suggest_var_name(s, st->as.assign.name);
        if (suggest) sema_note(s, st->line, st->col, "did you mean '%s'?", suggest);
        check_expr(s, st->as.assign.value);
        break;
      }
      if (v->is_const) {
        sema_error(s, st->line, st->col, "cannot assign to const variable '%s'",
                   st->as.assign.name);
      }
      TypeKind rhs = TYPE_VOID;
      if (expr_is_empty_array_literal(st->as.assign.value) && type_is_array(v->type)) {
        rhs = v->type;
        set_expr_type(st->as.assign.value, rhs);
      } else {
        rhs = check_expr(s, st->as.assign.value);
      }
      if (rhs != TYPE_VOID && !type_eq(v->type, rhs)) {
        sema_error(s, st->line, st->col,
                   "cannot assign '%s' to variable '%s' of type '%s'", type_kind_name(rhs),
                   st->as.assign.name, type_kind_name(v->type));
      }
      break;
    }

    case STMT_INDEX_ASSIGN: {
      TypeKind tt = check_expr(s, st->as.index_assign.target);
      TypeKind it = check_expr(s, st->as.index_assign.index);
      TypeKind vt = check_expr(s, st->as.index_assign.value);

      if (st->as.index_assign.target->kind != EXPR_IDENT) {
        sema_error(s, st->line, st->col,
                   "indexed assignment target must be an array variable identifier");
      } else {
        VarSymbol *v = lookup_var(s, st->as.index_assign.target->as.ident_name);
        if (v && v->is_const) {
          sema_error(s, st->line, st->col, "cannot assign through const variable '%s'",
                     st->as.index_assign.target->as.ident_name);
        }
      }

      if (tt != TYPE_VOID && !type_is_array(tt)) {
        sema_error(s, st->line, st->col, "indexed assignment expects array target, got '%s'",
                   type_kind_name(tt));
      }
      if (it != TYPE_VOID && !type_eq(it, TYPE_INT)) {
        sema_error(s, st->line, st->col, "array index must be int, got '%s'", type_kind_name(it));
      }

      TypeKind elem_t = array_elem_type(tt);
      if (elem_t != TYPE_VOID && vt != TYPE_VOID && !type_eq(elem_t, vt)) {
        sema_error(s, st->line, st->col, "%s assignment expects %s value, got '%s'",
                   type_kind_name(tt), type_kind_name(elem_t), type_kind_name(vt));
      }
      break;
    }

    case STMT_EXPR:
      (void)check_expr(s, st->as.expr_stmt.expr);
      break;

    case STMT_RETURN: {
      TypeKind fn_ret = s->current_fn ? s->current_fn->return_type : TYPE_VOID;
      if (!st->as.ret.has_expr) {
        if (!type_eq(fn_ret, TYPE_VOID)) {
          sema_error(s, st->line, st->col,
                     "return without value in function returning '%s'", type_kind_name(fn_ret));
        }
      } else {
        TypeKind got = check_expr(s, st->as.ret.expr);
        if (got != TYPE_VOID && !type_eq(got, fn_ret)) {
          sema_error(s, st->line, st->col, "return type mismatch: expected '%s', got '%s'",
                     type_kind_name(fn_ret), type_kind_name(got));
        }
      }
      break;
    }

    case STMT_IF: {
      TypeKind ct = check_expr(s, st->as.if_stmt.cond);
      if (ct != TYPE_VOID && !type_eq(ct, TYPE_BOOL)) {
        sema_error(s, st->line, st->col, "if condition must be bool, got '%s'",
                   type_kind_name(ct));
      }
      check_stmt(s, st->as.if_stmt.then_branch);
      if (st->as.if_stmt.else_branch) check_stmt(s, st->as.if_stmt.else_branch);
      break;
    }

    case STMT_WHILE: {
      TypeKind ct = check_expr(s, st->as.while_stmt.cond);
      if (ct != TYPE_VOID && !type_eq(ct, TYPE_BOOL)) {
        sema_error(s, st->line, st->col, "while condition must be bool, got '%s'",
                   type_kind_name(ct));
      }
      s->loop_depth++;
      check_stmt(s, st->as.while_stmt.body);
      s->loop_depth--;
      break;
    }

    case STMT_FOR: {
      push_scope(s);

      if (st->as.for_stmt.init) check_stmt(s, st->as.for_stmt.init);

      if (st->as.for_stmt.cond) {
        TypeKind ct = check_expr(s, st->as.for_stmt.cond);
        if (ct != TYPE_VOID && !type_eq(ct, TYPE_BOOL)) {
          sema_error(s, st->line, st->col, "for condition must be bool, got '%s'",
                     type_kind_name(ct));
        }
      }

      if (st->as.for_stmt.update) check_stmt(s, st->as.for_stmt.update);

      s->loop_depth++;
      check_stmt(s, st->as.for_stmt.body);
      s->loop_depth--;

      pop_scope(s);
      break;
    }

    case STMT_MATCH: {
      TypeKind stype = check_expr(s, st->as.match_stmt.subject);
      if (stype != TYPE_VOID && !type_eq(stype, TYPE_INT) && !type_eq(stype, TYPE_BOOL) &&
          !type_eq(stype, TYPE_STRING)) {
        sema_error(s, st->line, st->col, "match subject must be int, bool, or string, got '%s'",
                   type_kind_name(stype));
      }

      int wildcard_count = 0;
      int wildcard_index = -1;
      int seen_true = 0;
      int seen_false = 0;

      for (size_t i = 0; i < st->as.match_stmt.arm_count; i++) {
        MatchArm *arm = &st->as.match_stmt.arms[i];

        if (arm->pattern_kind == MATCH_PATTERN_WILDCARD) {
          wildcard_count++;
          if (wildcard_count > 1) {
            sema_error(s, arm->line, arm->col, "match allows only one wildcard '_' arm");
          }
          if (wildcard_index < 0) wildcard_index = (int)i;
        } else {
          if (wildcard_index >= 0) {
            sema_error(s, arm->line, arm->col, "unreachable match arm after wildcard '_'");
          }

          if (stype == TYPE_INT && arm->pattern_kind != MATCH_PATTERN_INT) {
            sema_error(s, arm->line, arm->col, "int match arm must be int literal or '_'");
          } else if (stype == TYPE_BOOL && arm->pattern_kind != MATCH_PATTERN_BOOL) {
            sema_error(s, arm->line, arm->col, "bool match arm must be true/false or '_'");
          } else if (stype == TYPE_STRING && arm->pattern_kind != MATCH_PATTERN_STRING) {
            sema_error(s, arm->line, arm->col,
                       "string match arm must be string literal or '_'");
          }

          if (stype == TYPE_BOOL && arm->pattern_kind == MATCH_PATTERN_BOOL) {
            if (arm->bool_value) {
              seen_true = 1;
            } else {
              seen_false = 1;
            }
          }

          for (size_t j = 0; j < i; j++) {
            MatchArm *prev = &st->as.match_stmt.arms[j];
            if (prev->pattern_kind != arm->pattern_kind) continue;

            if (arm->pattern_kind == MATCH_PATTERN_INT && prev->int_value == arm->int_value) {
              sema_error(s, arm->line, arm->col, "duplicate match arm value '%lld'",
                         (long long)arm->int_value);
              break;
            }
            if (arm->pattern_kind == MATCH_PATTERN_BOOL && prev->bool_value == arm->bool_value) {
              sema_error(s, arm->line, arm->col, "duplicate match arm value '%s'",
                         arm->bool_value ? "true" : "false");
              break;
            }
            if (arm->pattern_kind == MATCH_PATTERN_STRING && prev->string_value &&
                arm->string_value && strcmp(prev->string_value, arm->string_value) == 0) {
              sema_error(s, arm->line, arm->col, "duplicate match arm value '%s'",
                         arm->string_value);
              break;
            }
          }
        }

        push_scope(s);
        check_stmt(s, arm->body);
        pop_scope(s);
      }

      if (stype == TYPE_BOOL && wildcard_count == 0 && (!seen_true || !seen_false)) {
        const char *missing = !seen_true ? "true" : "false";
        sema_error(s, st->line, st->col,
                   "non-exhaustive bool match: missing '%s' arm or wildcard '_'", missing);
      }
      break;
    }

    case STMT_BREAK:
      if (s->loop_depth <= 0) {
        sema_error(s, st->line, st->col, "'break' can only be used inside a loop");
      }
      break;

    case STMT_CONTINUE:
      if (s->loop_depth <= 0) {
        sema_error(s, st->line, st->col, "'continue' can only be used inside a loop");
      }
      break;
  }
}

static void collect_functions(Sema *s, Program *prog) {
  s->funcs = (FuncSymbol *)calloc(prog->func_count, sizeof(FuncSymbol));
  if (!s->funcs && prog->func_count > 0) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(1);
  }

  s->func_count = 0;
  for (size_t i = 0; i < prog->func_count; i++) {
    FunctionDecl *fn = &prog->funcs[i];
    for (size_t j = 0; j < s->func_count; j++) {
      if (strcmp(s->funcs[j].name, fn->name) == 0) {
        sema_error(s, fn->body ? fn->body->line : 1, fn->body ? fn->body->col : 1,
                   "duplicate function '%s'", fn->name);
      }
    }
    s->funcs[s->func_count].name = fn->name;
    s->funcs[s->func_count].decl = fn;
    s->func_count++;
  }
}

static void check_main_signature(Sema *s) {
  const FuncSymbol *main_fn = lookup_fn(s, "main");
  if (!main_fn) {
    sema_error(s, 1, 1, "program must define 'fn main() -> int'");
    const char *suggest = suggest_fn_name(s, "main");
    if (suggest) sema_note(s, 1, 1, "did you mean function '%s'?", suggest);
    return;
  }

  if (main_fn->decl->param_count != 0 || main_fn->decl->return_type != TYPE_INT) {
    sema_error(s, main_fn->decl->body ? main_fn->decl->body->line : 1,
               main_fn->decl->body ? main_fn->decl->body->col : 1,
               "invalid main signature, expected 'fn main() -> int'");
  }
}

int sema_check_program(const char *file, const char *source, Program *prog) {
  Sema s;
  memset(&s, 0, sizeof(s));
  s.file = file;
  s.source = source;
  s.error_count = 0;
  s.max_errors = 20;
  s.stop_analysis = 0;

  collect_functions(&s, prog);
  check_main_signature(&s);

  for (size_t i = 0; i < prog->func_count; i++) {
    if (should_stop(&s)) break;

    FunctionDecl *fn = &prog->funcs[i];
    s.current_fn = fn;

    push_scope(&s);
    for (size_t p = 0; p < fn->param_count; p++) {
      declare_var(&s, fn->body ? fn->body->line : 1, fn->body ? fn->body->col : 1,
                  fn->params[p].name, fn->params[p].type, 0);
      if (should_stop(&s)) break;
    }

    check_stmt(&s, fn->body);

    if (!should_stop(&s) && fn->return_type != TYPE_VOID && !stmt_guarantees_return(fn->body)) {
      sema_error(&s, fn->body ? fn->body->line : 1, fn->body ? fn->body->col : 1,
                 "non-void function '%s' may not return on all paths", fn->name);
    }

    pop_scope(&s);
  }

  free(s.funcs);
  return s.had_error;
}
