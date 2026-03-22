#include "ngawi_runtime.h"

#include <stdio.h>

void ng_print_int(int64_t v) { printf("%lld", (long long)v); }

void ng_print_float(double v) { printf("%.17g", v); }

void ng_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }

void ng_print_string(const char *s) { fputs(s ? s : "", stdout); }
