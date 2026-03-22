#ifndef NGAWI_SEMA_TYPES_H
#define NGAWI_SEMA_TYPES_H

#include "../parser/ast.h"

const char *type_kind_name(TypeKind t);
int type_is_numeric(TypeKind t);

#endif
