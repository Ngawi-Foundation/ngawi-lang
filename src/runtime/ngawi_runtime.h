#ifndef NGAWI_RUNTIME_H
#define NGAWI_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

void ng_print_int(int64_t v);
void ng_print_float(double v);
void ng_print_bool(bool v);
void ng_print_string(const char *s);
int ng_string_eq(const char *a, const char *b);
int64_t ng_string_len(const char *s);
const char *ng_string_concat(const char *a, const char *b);

#endif
