#include "ngawi_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct RuntimeAllocs {
  void **items;
  size_t count;
  size_t cap;
  int initialized;
} RuntimeAllocs;

static RuntimeAllocs g_runtime_allocs = {0};

static void ng_runtime_cleanup(void) {
  for (size_t i = 0; i < g_runtime_allocs.count; i++) {
    free(g_runtime_allocs.items[i]);
  }
  free(g_runtime_allocs.items);
  g_runtime_allocs.items = NULL;
  g_runtime_allocs.count = 0;
  g_runtime_allocs.cap = 0;
  g_runtime_allocs.initialized = 0;
}

static int ng_runtime_init(void) {
  if (g_runtime_allocs.initialized) return 1;
  if (atexit(ng_runtime_cleanup) != 0) return 0;
  g_runtime_allocs.initialized = 1;
  return 1;
}

static void *ng_runtime_alloc(size_t size) {
  if (!ng_runtime_init()) return NULL;

  void *p = malloc(size);
  if (!p) return NULL;

  if (g_runtime_allocs.count == g_runtime_allocs.cap) {
    size_t next_cap = g_runtime_allocs.cap == 0 ? 64 : g_runtime_allocs.cap * 2;
    void **next = (void **)realloc(g_runtime_allocs.items, next_cap * sizeof(void *));
    if (!next) {
      free(p);
      return NULL;
    }
    g_runtime_allocs.items = next;
    g_runtime_allocs.cap = next_cap;
  }

  g_runtime_allocs.items[g_runtime_allocs.count++] = p;
  return p;
}

static char *ng_runtime_string_alloc(size_t size) { return (char *)ng_runtime_alloc(size); }

void ng_print_int(int64_t v) { printf("%lld", (long long)v); }

void ng_print_float(double v) { printf("%.17g", v); }

void ng_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }

void ng_print_string(const char *s) { fputs(s ? s : "", stdout); }

int64_t ng_array_checked_index(int64_t index, int64_t len) {
  if (index < 0 || index >= len) {
    fprintf(stderr, "runtime error: array index out of bounds (index=%lld, len=%lld)\n",
            (long long)index, (long long)len);
    exit(1);
  }
  return index;
}

int64_t ng_int_array_get(ng_int_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

ng_int_array_t ng_int2_array_get(ng_int2_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

double ng_float_array_get(ng_float_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

ng_float_array_t ng_float2_array_get(ng_float2_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

bool ng_bool_array_get(ng_bool_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

ng_bool_array_t ng_bool2_array_get(ng_bool2_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

const char *ng_string_array_get(ng_string_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

ng_string_array_t ng_string2_array_get(ng_string2_array_t arr, int64_t index) {
  return arr.data[ng_array_checked_index(index, arr.len)];
}

void ng_int_array_set(ng_int_array_t *arr, int64_t index, int64_t value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_int2_array_set(ng_int2_array_t *arr, int64_t index, ng_int_array_t value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_float_array_set(ng_float_array_t *arr, int64_t index, double value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_float2_array_set(ng_float2_array_t *arr, int64_t index, ng_float_array_t value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_bool_array_set(ng_bool_array_t *arr, int64_t index, bool value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_bool2_array_set(ng_bool2_array_t *arr, int64_t index, ng_bool_array_t value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_string_array_set(ng_string_array_t *arr, int64_t index, const char *value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

void ng_string2_array_set(ng_string2_array_t *arr, int64_t index, ng_string_array_t value) {
  arr->data[ng_array_checked_index(index, arr->len)] = value;
}

ng_int_array_t ng_int_array_push(ng_int_array_t arr, int64_t value) {
  int64_t new_len = arr.len + 1;
  int64_t *out = (int64_t *)ng_runtime_alloc((size_t)new_len * sizeof(int64_t));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(int64_t));
  out[new_len - 1] = value;
  ng_int_array_t r = {out, new_len};
  return r;
}

ng_int_array_t ng_int_array_pop(ng_int_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_int_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  int64_t *out = (int64_t *)ng_runtime_alloc((size_t)new_len * sizeof(int64_t));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(int64_t));
  ng_int_array_t r = {out, new_len};
  return r;
}

ng_int2_array_t ng_int2_array_push(ng_int2_array_t arr, ng_int_array_t value) {
  int64_t new_len = arr.len + 1;
  ng_int_array_t *out = (ng_int_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_int_array_t));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(ng_int_array_t));
  out[new_len - 1] = value;
  ng_int2_array_t r = {out, new_len};
  return r;
}

ng_int2_array_t ng_int2_array_pop(ng_int2_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_int2_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  ng_int_array_t *out = (ng_int_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_int_array_t));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(ng_int_array_t));
  ng_int2_array_t r = {out, new_len};
  return r;
}

ng_float_array_t ng_float_array_push(ng_float_array_t arr, double value) {
  int64_t new_len = arr.len + 1;
  double *out = (double *)ng_runtime_alloc((size_t)new_len * sizeof(double));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(double));
  out[new_len - 1] = value;
  ng_float_array_t r = {out, new_len};
  return r;
}

ng_float_array_t ng_float_array_pop(ng_float_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_float_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  double *out = (double *)ng_runtime_alloc((size_t)new_len * sizeof(double));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(double));
  ng_float_array_t r = {out, new_len};
  return r;
}

ng_float2_array_t ng_float2_array_push(ng_float2_array_t arr, ng_float_array_t value) {
  int64_t new_len = arr.len + 1;
  ng_float_array_t *out = (ng_float_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_float_array_t));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(ng_float_array_t));
  out[new_len - 1] = value;
  ng_float2_array_t r = {out, new_len};
  return r;
}

ng_float2_array_t ng_float2_array_pop(ng_float2_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_float2_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  ng_float_array_t *out = (ng_float_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_float_array_t));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(ng_float_array_t));
  ng_float2_array_t r = {out, new_len};
  return r;
}

ng_bool_array_t ng_bool_array_push(ng_bool_array_t arr, bool value) {
  int64_t new_len = arr.len + 1;
  bool *out = (bool *)ng_runtime_alloc((size_t)new_len * sizeof(bool));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(bool));
  out[new_len - 1] = value;
  ng_bool_array_t r = {out, new_len};
  return r;
}

ng_bool_array_t ng_bool_array_pop(ng_bool_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_bool_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  bool *out = (bool *)ng_runtime_alloc((size_t)new_len * sizeof(bool));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(bool));
  ng_bool_array_t r = {out, new_len};
  return r;
}

ng_bool2_array_t ng_bool2_array_push(ng_bool2_array_t arr, ng_bool_array_t value) {
  int64_t new_len = arr.len + 1;
  ng_bool_array_t *out = (ng_bool_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_bool_array_t));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(ng_bool_array_t));
  out[new_len - 1] = value;
  ng_bool2_array_t r = {out, new_len};
  return r;
}

ng_bool2_array_t ng_bool2_array_pop(ng_bool2_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_bool2_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  ng_bool_array_t *out = (ng_bool_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_bool_array_t));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(ng_bool_array_t));
  ng_bool2_array_t r = {out, new_len};
  return r;
}

ng_string_array_t ng_string_array_push(ng_string_array_t arr, const char *value) {
  int64_t new_len = arr.len + 1;
  const char **out = (const char **)ng_runtime_alloc((size_t)new_len * sizeof(const char *));
  if (!out) return arr;
  if (arr.len > 0 && arr.data) memcpy(out, arr.data, (size_t)arr.len * sizeof(const char *));
  out[new_len - 1] = value;
  ng_string_array_t r = {out, new_len};
  return r;
}

ng_string_array_t ng_string_array_pop(ng_string_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_string_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  const char **out = (const char **)ng_runtime_alloc((size_t)new_len * sizeof(const char *));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(const char *));
  ng_string_array_t r = {out, new_len};
  return r;
}

ng_string2_array_t ng_string2_array_push(ng_string2_array_t arr, ng_string_array_t value) {
  int64_t new_len = arr.len + 1;
  ng_string_array_t *out =
      (ng_string_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_string_array_t));
  if (!out) return arr;
  if (arr.len > 0 && arr.data)
    memcpy(out, arr.data, (size_t)arr.len * sizeof(ng_string_array_t));
  out[new_len - 1] = value;
  ng_string2_array_t r = {out, new_len};
  return r;
}

ng_string2_array_t ng_string2_array_pop(ng_string2_array_t arr) {
  if (arr.len <= 1 || !arr.data) {
    ng_string2_array_t r = {NULL, 0};
    return r;
  }
  int64_t new_len = arr.len - 1;
  ng_string_array_t *out =
      (ng_string_array_t *)ng_runtime_alloc((size_t)new_len * sizeof(ng_string_array_t));
  if (!out) return arr;
  memcpy(out, arr.data, (size_t)new_len * sizeof(ng_string_array_t));
  ng_string2_array_t r = {out, new_len};
  return r;
}

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

  char *out = ng_runtime_string_alloc(la + lb + 1);
  if (!out) return "";

  memcpy(out, lhs, la);
  memcpy(out + la, rhs, lb);
  out[la + lb] = '\0';
  return out;
}

int ng_string_contains(const char *s, const char *sub) {
  const char *haystack = s ? s : "";
  const char *needle = sub ? sub : "";
  return strstr(haystack, needle) != NULL;
}

int ng_string_starts_with(const char *s, const char *prefix) {
  const char *str = s ? s : "";
  const char *pre = prefix ? prefix : "";
  size_t n = strlen(pre);
  return strncmp(str, pre, n) == 0;
}

int ng_string_ends_with(const char *s, const char *suffix) {
  const char *str = s ? s : "";
  const char *suf = suffix ? suffix : "";
  size_t ls = strlen(str);
  size_t le = strlen(suf);
  if (le > ls) return 0;
  return strcmp(str + (ls - le), suf) == 0;
}

const char *ng_string_to_lower(const char *s) {
  const char *src = s ? s : "";
  size_t n = strlen(src);
  char *out = ng_runtime_string_alloc(n + 1);
  if (!out) return "";

  for (size_t i = 0; i < n; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c >= 'A' && c <= 'Z') {
      out[i] = (char)(c - 'A' + 'a');
    } else {
      out[i] = (char)c;
    }
  }
  out[n] = '\0';
  return out;
}

const char *ng_string_to_upper(const char *s) {
  const char *src = s ? s : "";
  size_t n = strlen(src);
  char *out = ng_runtime_string_alloc(n + 1);
  if (!out) return "";

  for (size_t i = 0; i < n; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c >= 'a' && c <= 'z') {
      out[i] = (char)(c - 'a' + 'A');
    } else {
      out[i] = (char)c;
    }
  }
  out[n] = '\0';
  return out;
}

static int ng_is_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

const char *ng_string_trim(const char *s) {
  const char *src = s ? s : "";
  size_t n = strlen(src);
  size_t lo = 0;
  while (lo < n && ng_is_space((unsigned char)src[lo])) lo++;

  size_t hi = n;
  while (hi > lo && ng_is_space((unsigned char)src[hi - 1])) hi--;

  size_t out_n = hi - lo;
  char *out = ng_runtime_string_alloc(out_n + 1);
  if (!out) return "";
  if (out_n > 0) memcpy(out, src + lo, out_n);
  out[out_n] = '\0';
  return out;
}
