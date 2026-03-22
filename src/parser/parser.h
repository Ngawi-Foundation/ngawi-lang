#ifndef NGAWI_PARSER_H
#define NGAWI_PARSER_H

#include "ast.h"

Program *parse_program(const char *file, const char *source, int *had_error);

#endif
