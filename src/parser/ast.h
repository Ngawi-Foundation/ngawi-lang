#ifndef NGAWI_AST_H
#define NGAWI_AST_H

#include <stddef.h>
#include <stdint.h>

typedef enum TypeKind {
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_BOOL,
  TYPE_STRING,
  TYPE_VOID,
} TypeKind;

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum ExprKind {
  EXPR_INT,
  EXPR_FLOAT,
  EXPR_STRING,
  EXPR_BOOL,
  EXPR_IDENT,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_CALL,
} ExprKind;

struct Expr {
  ExprKind kind;
  int line;
  int col;
  TypeKind inferred_type;
  union {
    int64_t int_val;
    double float_val;
    int bool_val;
    char *string_val;
    char *ident_name;
    struct {
      int op;
      Expr *expr;
    } unary;
    struct {
      int op;
      Expr *left;
      Expr *right;
    } binary;
    struct {
      char *name;
      Expr **args;
      size_t arg_count;
    } call;
  } as;
};

typedef enum StmtKind {
  STMT_BLOCK,
  STMT_VAR_DECL,
  STMT_CONST_DECL,
  STMT_ASSIGN,
  STMT_EXPR,
  STMT_RETURN,
  STMT_IF,
  STMT_WHILE,
  STMT_FOR,
} StmtKind;

struct Stmt {
  StmtKind kind;
  int line;
  int col;
  union {
    struct {
      Stmt **items;
      size_t count;
    } block;
    struct {
      char *name;
      int has_type;
      TypeKind type;
      Expr *init;
    } var_decl;
    struct {
      char *name;
      Expr *value;
    } assign;
    struct {
      Expr *expr;
    } expr_stmt;
    struct {
      int has_expr;
      Expr *expr;
    } ret;
    struct {
      Expr *cond;
      Stmt *then_branch;
      Stmt *else_branch;
    } if_stmt;
    struct {
      Expr *cond;
      Stmt *body;
    } while_stmt;
    struct {
      Stmt *init;
      Expr *cond;
      Stmt *update;
      Stmt *body;
    } for_stmt;
  } as;
};

typedef struct Param {
  char *name;
  TypeKind type;
} Param;

typedef struct FunctionDecl {
  char *name;
  Param *params;
  size_t param_count;
  TypeKind return_type;
  Stmt *body;
} FunctionDecl;

typedef struct Program {
  FunctionDecl *funcs;
  size_t func_count;
} Program;

void program_free(Program *p);

#endif
