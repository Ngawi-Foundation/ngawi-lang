#ifndef NGAWI_RUNTIME_H
#define NGAWI_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

void ng_print_int(int64_t v);
void ng_print_float(double v);
void ng_print_bool(bool v);
void ng_print_string(const char *s);

#endif
