#ifndef NGAWI_RUNTIME_H
#define NGAWI_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct ng_int_array_t {
  int64_t *data;
  int64_t len;
} ng_int_array_t;

typedef struct ng_float_array_t {
  double *data;
  int64_t len;
} ng_float_array_t;

typedef struct ng_bool_array_t {
  bool *data;
  int64_t len;
} ng_bool_array_t;

typedef struct ng_string_array_t {
  const char **data;
  int64_t len;
} ng_string_array_t;

void ng_print_int(int64_t v);
void ng_print_float(double v);
void ng_print_bool(bool v);
void ng_print_string(const char *s);
int ng_string_eq(const char *a, const char *b);
int64_t ng_string_len(const char *s);
const char *ng_string_concat(const char *a, const char *b);
int ng_string_contains(const char *s, const char *sub);
int ng_string_starts_with(const char *s, const char *prefix);
int ng_string_ends_with(const char *s, const char *suffix);
const char *ng_string_to_lower(const char *s);
const char *ng_string_to_upper(const char *s);
const char *ng_string_trim(const char *s);

#endif
