#include "cgen.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../lexer/token.h"

typedef struct CGen {
  FILE *out;
  int indent;
} CGen;

static void emit_indent(CGen *g) {
  for (int i = 0; i < g->indent; i++) fputs("  ", g->out);
}

static void emit(CGen *g, const char *s) { fputs(s, g->out); }

static void emitf(CGen *g, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(g->out, fmt, args);
  va_end(args);
}

static const char *c_type(TypeKind t) {
  switch (t) {
    case TYPE_INT: return "int64_t";
    case TYPE_FLOAT: return "double";
    case TYPE_BOOL: return "bool";
    case TYPE_STRING: return "const char *";
    case TYPE_VOID: return "void";
    default: return "void";
  }
}

static const char *op_text(int op) {
  switch (op) {
    case TOK_PLUS: return "+";
    case TOK_MINUS: return "-";
    case TOK_STAR: return "*";
    case TOK_SLASH: return "/";
    case TOK_PERCENT: return "%";
    case TOK_EQ: return "==";
    case TOK_NE: return "!=";
    case TOK_LT: return "<";
    case TOK_LE: return "<=";
    case TOK_GT: return ">";
    case TOK_GE: return ">=";
    case TOK_AND_AND: return "&&";
    case TOK_OR_OR: return "||";
    case TOK_BANG: return "!";
    default: return "?";
  }
}

static void emit_string_escaped(CGen *g, const char *s) {
  fputc('"', g->out);
  for (const char *p = s; *p; p++) {
    if (*p == '"' || *p == '\\') {
      fputc('\\', g->out);
      fputc(*p, g->out);
    } else if (*p == '\n') {
      fputs("\\n", g->out);
    } else if (*p == '\t') {
      fputs("\\t", g->out);
    } else {
      fputc(*p, g->out);
    }
  }
  fputc('"', g->out);
}

static void emit_expr(CGen *g, Expr *e);
static void emit_stmt(CGen *g, Stmt *st);

static int is_print_call(Expr *e) {
  return e && e->kind == EXPR_CALL && strcmp(e->as.call.name, "print") == 0;
}

static void emit_print_stmt(CGen *g, Expr *call) {
  for (size_t i = 0; i < call->as.call.arg_count; i++) {
    Expr *arg = call->as.call.args[i];
    emit_indent(g);
    switch (arg->inferred_type) {
      case TYPE_INT:
        emit(g, "ng_print_int(");
        emit_expr(g, arg);
        emit(g, ");\n");
        break;
      case TYPE_FLOAT:
        emit(g, "ng_print_float(");
        emit_expr(g, arg);
        emit(g, ");\n");
        break;
      case TYPE_BOOL:
        emit(g, "ng_print_bool(");
        emit_expr(g, arg);
        emit(g, ");\n");
        break;
      case TYPE_STRING:
        emit(g, "ng_print_string(");
        emit_expr(g, arg);
        emit(g, ");\n");
        break;
      case TYPE_VOID:
      default:
        emit(g, "/* invalid print arg */\n");
        break;
    }

    if (i + 1 < call->as.call.arg_count) {
      emit_indent(g);
      emit(g, "ng_print_string(\" \");\n");
    }
  }
  emit_indent(g);
  emit(g, "ng_print_string(\"\\n\");\n");
}

static void emit_expr(CGen *g, Expr *e) {
  switch (e->kind) {
    case EXPR_INT:
      emitf(g, "%lld", (long long)e->as.int_val);
      break;
    case EXPR_FLOAT:
      emitf(g, "%.17g", e->as.float_val);
      break;
    case EXPR_STRING:
      emit_string_escaped(g, e->as.string_val ? e->as.string_val : "");
      break;
    case EXPR_BOOL:
      emit(g, e->as.bool_val ? "true" : "false");
      break;
    case EXPR_IDENT:
      emit(g, e->as.ident_name);
      break;
    case EXPR_UNARY:
      emit(g, "(");
      emit(g, op_text(e->as.unary.op));
      emit_expr(g, e->as.unary.expr);
      emit(g, ")");
      break;
    case EXPR_BINARY:
      if ((e->as.binary.op == TOK_EQ || e->as.binary.op == TOK_NE) &&
          e->as.binary.left && e->as.binary.right &&
          e->as.binary.left->inferred_type == TYPE_STRING &&
          e->as.binary.right->inferred_type == TYPE_STRING) {
        if (e->as.binary.op == TOK_NE) emit(g, "(!");
        emit(g, "ng_string_eq(");
        emit_expr(g, e->as.binary.left);
        emit(g, ", ");
        emit_expr(g, e->as.binary.right);
        emit(g, ")");
        if (e->as.binary.op == TOK_NE) emit(g, ")");
        break;
      }

      if (e->as.binary.op == TOK_PLUS && e->as.binary.left && e->as.binary.right &&
          e->as.binary.left->inferred_type == TYPE_STRING &&
          e->as.binary.right->inferred_type == TYPE_STRING) {
        emit(g, "ng_string_concat(");
        emit_expr(g, e->as.binary.left);
        emit(g, ", ");
        emit_expr(g, e->as.binary.right);
        emit(g, ")");
        break;
      }

      emit(g, "(");
      emit_expr(g, e->as.binary.left);
      emit(g, " ");
      emit(g, op_text(e->as.binary.op));
      emit(g, " ");
      emit_expr(g, e->as.binary.right);
      emit(g, ")");
      break;
    case EXPR_CALL:
      if ((strcmp(e->as.call.name, "to_int") == 0 || strcmp(e->as.call.name, "to_amba") == 0) &&
          e->as.call.arg_count == 1) {
        emit(g, "((int64_t)(");
        emit_expr(g, e->as.call.args[0]);
        emit(g, "))");
        break;
      }
      if ((strcmp(e->as.call.name, "to_float") == 0 ||
           strcmp(e->as.call.name, "to_rusdi") == 0) &&
          e->as.call.arg_count == 1) {
        emit(g, "((double)(");
        emit_expr(g, e->as.call.args[0]);
        emit(g, "))");
        break;
      }
      if (strcmp(e->as.call.name, "len") == 0 && e->as.call.arg_count == 1) {
        emit(g, "ng_string_len(");
        emit_expr(g, e->as.call.args[0]);
        emit(g, ")");
        break;
      }
      if (strcmp(e->as.call.name, "contains") == 0 && e->as.call.arg_count == 2) {
        emit(g, "ng_string_contains(");
        emit_expr(g, e->as.call.args[0]);
        emit(g, ", ");
        emit_expr(g, e->as.call.args[1]);
        emit(g, ")");
        break;
      }
      if (strcmp(e->as.call.name, "starts_with") == 0 && e->as.call.arg_count == 2) {
        emit(g, "ng_string_starts_with(");
        emit_expr(g, e->as.call.args[0]);
        emit(g, ", ");
        emit_expr(g, e->as.call.args[1]);
        emit(g, ")");
        break;
      }
      if (strcmp(e->as.call.name, "to_lower") == 0 && e->as.call.arg_count == 1) {
        emit(g, "ng_string_to_lower(");
        emit_expr(g, e->as.call.args[0]);
        emit(g, ")");
        break;
      }

      emit(g, e->as.call.name);
      emit(g, "(");
      for (size_t i = 0; i < e->as.call.arg_count; i++) {
        if (i > 0) emit(g, ", ");
        emit_expr(g, e->as.call.args[i]);
      }
      emit(g, ")");
      break;
  }
}

static void emit_block(CGen *g, Stmt *blk) {
  emit_indent(g);
  emit(g, "{\n");
  g->indent++;
  for (size_t i = 0; i < blk->as.block.count; i++) {
    emit_stmt(g, blk->as.block.items[i]);
  }
  g->indent--;
  emit_indent(g);
  emit(g, "}\n");
}

static void emit_for_clause_stmt(CGen *g, Stmt *st) {
  if (!st) return;

  switch (st->kind) {
    case STMT_VAR_DECL:
    case STMT_CONST_DECL:
      if (st->kind == STMT_CONST_DECL && st->as.var_decl.type != TYPE_STRING) emit(g, "const ");
      emit(g, c_type(st->as.var_decl.type));
      emit(g, " ");
      emit(g, st->as.var_decl.name);
      emit(g, " = ");
      emit_expr(g, st->as.var_decl.init);
      break;

    case STMT_ASSIGN:
      emit(g, st->as.assign.name);
      emit(g, " = ");
      emit_expr(g, st->as.assign.value);
      break;

    case STMT_EXPR:
      emit_expr(g, st->as.expr_stmt.expr);
      break;

    default:
      break;
  }
}

static void emit_stmt(CGen *g, Stmt *st) {
  switch (st->kind) {
    case STMT_BLOCK:
      emit_block(g, st);
      break;

    case STMT_VAR_DECL:
    case STMT_CONST_DECL:
      emit_indent(g);
      if (st->kind == STMT_CONST_DECL && st->as.var_decl.type != TYPE_STRING) emit(g, "const ");
      emit(g, c_type(st->as.var_decl.type));
      emit(g, " ");
      emit(g, st->as.var_decl.name);
      emit(g, " = ");
      emit_expr(g, st->as.var_decl.init);
      emit(g, ";\n");
      break;

    case STMT_ASSIGN:
      emit_indent(g);
      emit(g, st->as.assign.name);
      emit(g, " = ");
      emit_expr(g, st->as.assign.value);
      emit(g, ";\n");
      break;

    case STMT_EXPR:
      if (is_print_call(st->as.expr_stmt.expr)) {
        emit_print_stmt(g, st->as.expr_stmt.expr);
      } else {
        emit_indent(g);
        emit_expr(g, st->as.expr_stmt.expr);
        emit(g, ";\n");
      }
      break;

    case STMT_RETURN:
      emit_indent(g);
      emit(g, "return");
      if (st->as.ret.has_expr) {
        emit(g, " ");
        emit_expr(g, st->as.ret.expr);
      }
      emit(g, ";\n");
      break;

    case STMT_IF:
      emit_indent(g);
      emit(g, "if (");
      emit_expr(g, st->as.if_stmt.cond);
      emit(g, ")\n");
      emit_stmt(g, st->as.if_stmt.then_branch);
      if (st->as.if_stmt.else_branch) {
        emit_indent(g);
        emit(g, "else\n");
        emit_stmt(g, st->as.if_stmt.else_branch);
      }
      break;

    case STMT_WHILE:
      emit_indent(g);
      emit(g, "while (");
      emit_expr(g, st->as.while_stmt.cond);
      emit(g, ")\n");
      emit_stmt(g, st->as.while_stmt.body);
      break;

    case STMT_FOR:
      emit_indent(g);
      emit(g, "for (");
      emit_for_clause_stmt(g, st->as.for_stmt.init);
      emit(g, "; ");
      if (st->as.for_stmt.cond) emit_expr(g, st->as.for_stmt.cond);
      emit(g, "; ");
      emit_for_clause_stmt(g, st->as.for_stmt.update);
      emit(g, ")\n");
      emit_stmt(g, st->as.for_stmt.body);
      break;

    case STMT_MATCH:
      emit_indent(g);
      emit(g, "switch (");
      emit_expr(g, st->as.match_stmt.subject);
      emit(g, ")\n");
      emit_indent(g);
      emit(g, "{\n");
      g->indent++;
      for (size_t i = 0; i < st->as.match_stmt.arm_count; i++) {
        MatchArm *arm = &st->as.match_stmt.arms[i];
        emit_indent(g);
        if (arm->pattern_kind == MATCH_PATTERN_WILDCARD) {
          emit(g, "default:\n");
        } else if (arm->pattern_kind == MATCH_PATTERN_INT) {
          emitf(g, "case %lld:\n", (long long)arm->int_value);
        } else {
          emitf(g, "case %d:\n", arm->bool_value ? 1 : 0);
        }

        emit_indent(g);
        emit(g, "{\n");
        g->indent++;
        emit_stmt(g, arm->body);
        if (arm->body->kind != STMT_RETURN && arm->body->kind != STMT_BREAK &&
            arm->body->kind != STMT_CONTINUE) {
          emit_indent(g);
          emit(g, "break;\n");
        }
        g->indent--;
        emit_indent(g);
        emit(g, "}\n");
      }
      g->indent--;
      emit_indent(g);
      emit(g, "}\n");
      break;

    case STMT_BREAK:
      emit_indent(g);
      emit(g, "break;\n");
      break;

    case STMT_CONTINUE:
      emit_indent(g);
      emit(g, "continue;\n");
      break;
  }
}

static void emit_prelude(CGen *g, const char *input_file) {
  emit(g, "#include <stdbool.h>\n");
  emit(g, "#include <stdint.h>\n");
  emit(g, "#include \"ngawi_runtime.h\"\n\n");

  emitf(g, "/* Generated from %s */\n\n", input_file);
}

int codegen_emit_c(const char *input_file, Program *prog, const char *out_c_path) {
  FILE *f = fopen(out_c_path, "w");
  if (!f) {
    fprintf(stderr, "%s:1:1: error: cannot write C output '%s': %s\n", input_file,
            out_c_path, strerror(errno));
    return 1;
  }

  CGen g;
  g.out = f;
  g.indent = 0;

  emit_prelude(&g, input_file);

  /* Prototypes pass (allows forward calls). */
  for (size_t i = 0; i < prog->func_count; i++) {
    FunctionDecl *fn = &prog->funcs[i];
    if (strcmp(fn->name, "main") == 0) {
      emit(&g, "int main(void);\n");
      continue;
    }

    emit(&g, c_type(fn->return_type));
    emit(&g, " ");
    emit(&g, fn->name);
    emit(&g, "(");
    for (size_t p = 0; p < fn->param_count; p++) {
      if (p > 0) emit(&g, ", ");
      emit(&g, c_type(fn->params[p].type));
      emit(&g, " ");
      emit(&g, fn->params[p].name);
    }
    emit(&g, ");\n");
  }
  emit(&g, "\n");

  for (size_t i = 0; i < prog->func_count; i++) {
    FunctionDecl *fn = &prog->funcs[i];
    if (strcmp(fn->name, "main") == 0) {
      emit(&g, "int main(void)\n");
    } else {
      emit(&g, c_type(fn->return_type));
      emit(&g, " ");
      emit(&g, fn->name);
      emit(&g, "(");
      for (size_t p = 0; p < fn->param_count; p++) {
        if (p > 0) emit(&g, ", ");
        emit(&g, c_type(fn->params[p].type));
        emit(&g, " ");
        emit(&g, fn->params[p].name);
      }
      emit(&g, ")\n");
    }

    emit_stmt(&g, fn->body);
    emit(&g, "\n");
  }

  fclose(f);
  return 0;
}
