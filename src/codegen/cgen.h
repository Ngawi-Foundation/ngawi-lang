#ifndef NGAWI_CGEN_H
#define NGAWI_CGEN_H

#include "../parser/ast.h"

int codegen_emit_c(const char *input_file, Program *prog, const char *out_c_path);

#endif
