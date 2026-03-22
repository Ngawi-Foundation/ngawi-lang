#ifndef NGAWI_SEMA_H
#define NGAWI_SEMA_H

#include "../parser/ast.h"

int sema_check_program(const char *file, const char *source, Program *prog);

#endif
