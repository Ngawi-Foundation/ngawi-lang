#ifndef NGAWI_LEXER_H
#define NGAWI_LEXER_H

#include "token.h"

typedef struct Lexer {
  const char *file;
  const char *src;
  const char *cur;
  int line;
  int col;
} Lexer;

void lexer_init(Lexer *lx, const char *file, const char *source);
Token lexer_next(Lexer *lx);

#endif
