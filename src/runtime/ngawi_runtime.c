#include "ngawi_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ng_print_int(int64_t v) { printf("%lld", (long long)v); }

void ng_print_float(double v) { printf("%.17g", v); }

void ng_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }

void ng_print_string(const char *s) { fputs(s ? s : "", stdout); }

int ng_string_eq(const char *a, const char *b) {
  if (a == b) return 1;
  if (!a || !b) return 0;
  return strcmp(a, b) == 0;
}

int64_t ng_string_len(const char *s) {
  if (!s) return 0;
  return (int64_t)strlen(s);
}

const char *ng_string_concat(const char *a, const char *b) {
  const char *lhs = a ? a : "";
  const char *rhs = b ? b : "";

  size_t la = strlen(lhs);
  size_t lb = strlen(rhs);

  char *out = (char *)malloc(la + lb + 1);
  if (!out) return "";

  memcpy(out, lhs, la);
  memcpy(out + la, rhs, lb);
  out[la + lb] = '\0';
  return out;
}
