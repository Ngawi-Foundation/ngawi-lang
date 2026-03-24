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

typedef struct ng_int2_array_t {
  ng_int_array_t *data;
  int64_t len;
} ng_int2_array_t;

typedef struct ng_float2_array_t {
  ng_float_array_t *data;
  int64_t len;
} ng_float2_array_t;

typedef struct ng_bool_array_t {
  bool *data;
  int64_t len;
} ng_bool_array_t;

typedef struct ng_string_array_t {
  const char **data;
  int64_t len;
} ng_string_array_t;

typedef struct ng_bool2_array_t {
  ng_bool_array_t *data;
  int64_t len;
} ng_bool2_array_t;

typedef struct ng_string2_array_t {
  ng_string_array_t *data;
  int64_t len;
} ng_string2_array_t;

int64_t ng_array_checked_index(int64_t index, int64_t len);

int64_t ng_int_array_get(ng_int_array_t arr, int64_t index);
ng_int_array_t ng_int2_array_get(ng_int2_array_t arr, int64_t index);
double ng_float_array_get(ng_float_array_t arr, int64_t index);
ng_float_array_t ng_float2_array_get(ng_float2_array_t arr, int64_t index);
bool ng_bool_array_get(ng_bool_array_t arr, int64_t index);
ng_bool_array_t ng_bool2_array_get(ng_bool2_array_t arr, int64_t index);
const char *ng_string_array_get(ng_string_array_t arr, int64_t index);
ng_string_array_t ng_string2_array_get(ng_string2_array_t arr, int64_t index);

void ng_int_array_set(ng_int_array_t *arr, int64_t index, int64_t value);
void ng_int2_array_set(ng_int2_array_t *arr, int64_t index, ng_int_array_t value);
void ng_float_array_set(ng_float_array_t *arr, int64_t index, double value);
void ng_float2_array_set(ng_float2_array_t *arr, int64_t index, ng_float_array_t value);
void ng_bool_array_set(ng_bool_array_t *arr, int64_t index, bool value);
void ng_bool2_array_set(ng_bool2_array_t *arr, int64_t index, ng_bool_array_t value);
void ng_string_array_set(ng_string_array_t *arr, int64_t index, const char *value);
void ng_string2_array_set(ng_string2_array_t *arr, int64_t index, ng_string_array_t value);

ng_int_array_t ng_int_array_push(ng_int_array_t arr, int64_t value);
ng_int_array_t ng_int_array_pop(ng_int_array_t arr);
ng_int2_array_t ng_int2_array_push(ng_int2_array_t arr, ng_int_array_t value);
ng_int2_array_t ng_int2_array_pop(ng_int2_array_t arr);
ng_float_array_t ng_float_array_push(ng_float_array_t arr, double value);
ng_float_array_t ng_float_array_pop(ng_float_array_t arr);
ng_float2_array_t ng_float2_array_push(ng_float2_array_t arr, ng_float_array_t value);
ng_float2_array_t ng_float2_array_pop(ng_float2_array_t arr);
ng_bool_array_t ng_bool_array_push(ng_bool_array_t arr, bool value);
ng_bool_array_t ng_bool_array_pop(ng_bool_array_t arr);
ng_bool2_array_t ng_bool2_array_push(ng_bool2_array_t arr, ng_bool_array_t value);
ng_bool2_array_t ng_bool2_array_pop(ng_bool2_array_t arr);
ng_string_array_t ng_string_array_push(ng_string_array_t arr, const char *value);
ng_string_array_t ng_string_array_pop(ng_string_array_t arr);
ng_string2_array_t ng_string2_array_push(ng_string2_array_t arr, ng_string_array_t value);
ng_string2_array_t ng_string2_array_pop(ng_string2_array_t arr);

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
