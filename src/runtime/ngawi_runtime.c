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

typedef struct ArrayCapEntry {
  void *data;
  int64_t cap;
} ArrayCapEntry;

typedef struct ArrayCaps {
  ArrayCapEntry *items;
  size_t count;
  size_t cap;
} ArrayCaps;

static RuntimeAllocs g_runtime_allocs = {0};
static ArrayCaps g_array_caps = {0};

static void ng_runtime_cleanup(void) {
  for (size_t i = 0; i < g_runtime_allocs.count; i++) {
    free(g_runtime_allocs.items[i]);
  }
  free(g_runtime_allocs.items);
  g_runtime_allocs.items = NULL;
  g_runtime_allocs.count = 0;
  g_runtime_allocs.cap = 0;

  free(g_array_caps.items);
  g_array_caps.items = NULL;
  g_array_caps.count = 0;
  g_array_caps.cap = 0;

  g_runtime_allocs.initialized = 0;
}

static int ng_runtime_init(void) {
  if (g_runtime_allocs.initialized) return 1;
  if (atexit(ng_runtime_cleanup) != 0) return 0;
  g_runtime_allocs.initialized = 1;
  return 1;
}

static int ng_runtime_alloc_index_of(void *ptr) {
  if (!ptr) return -1;
  for (size_t i = 0; i < g_runtime_allocs.count; i++) {
    if (g_runtime_allocs.items[i] == ptr) return (int)i;
  }
  return -1;
}

static int ng_array_cap_index_of(void *data) {
  if (!data) return -1;
  for (size_t i = 0; i < g_array_caps.count; i++) {
    if (g_array_caps.items[i].data == data) return (int)i;
  }
  return -1;
}

static int ng_runtime_ptr_is_tracked(void *ptr) { return ng_runtime_alloc_index_of(ptr) >= 0; }

static int ng_array_cap_set(void *data, int64_t cap) {
  if (!data) return 1;

  int idx = ng_array_cap_index_of(data);
  if (idx >= 0) {
    g_array_caps.items[idx].cap = cap;
    return 1;
  }

  if (g_array_caps.count == g_array_caps.cap) {
    size_t next_cap = g_array_caps.cap == 0 ? 32 : g_array_caps.cap * 2;
    ArrayCapEntry *next =
        (ArrayCapEntry *)realloc(g_array_caps.items, next_cap * sizeof(ArrayCapEntry));
    if (!next) return 0;
    g_array_caps.items = next;
    g_array_caps.cap = next_cap;
  }

  g_array_caps.items[g_array_caps.count].data = data;
  g_array_caps.items[g_array_caps.count].cap = cap;
  g_array_caps.count++;
  return 1;
}

static int64_t ng_array_cap_get_or_len(void *data, int64_t len) {
  int idx = ng_array_cap_index_of(data);
  if (idx >= 0) return g_array_caps.items[idx].cap;
  return len;
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

static void *ng_runtime_realloc(size_t elem_size, void *old_ptr, int64_t new_cap) {
  if (!old_ptr || new_cap <= 0) return NULL;
  int alloc_idx = ng_runtime_alloc_index_of(old_ptr);
  if (alloc_idx < 0) return NULL;

  void *p = realloc(old_ptr, (size_t)new_cap * elem_size);
  if (!p) return NULL;

  g_runtime_allocs.items[alloc_idx] = p;

  int cap_idx = ng_array_cap_index_of(old_ptr);
  if (cap_idx >= 0) {
    g_array_caps.items[cap_idx].data = p;
    g_array_caps.items[cap_idx].cap = new_cap;
  } else if (!ng_array_cap_set(p, new_cap)) {
    return NULL;
  }

  return p;
}

static int64_t ng_next_capacity(int64_t current_cap, int64_t need_len) {
  int64_t cap = current_cap > 0 ? current_cap : 4;
  while (cap < need_len) {
    if (cap > (INT64_MAX / 2)) return need_len;
    cap *= 2;
  }
  return cap;
}

static char *ng_runtime_string_alloc(size_t size) { return (char *)ng_runtime_alloc(size); }

void ng_print_int(int64_t v) { printf("%lld", (long long)v); }

void ng_print_float(double v) { printf("%.17g", v); }

void ng_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }

void ng_print_string(const char *s) { fputs(s ? s : "", stdout); }

static void ng_runtime_error(const char *msg) {
  fprintf(stderr, "runtime error: %s\n", msg);
  exit(1);
}

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
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  int64_t *out = arr.data;
  if (!out) {
    out = (int64_t *)ng_runtime_alloc((size_t)next_cap * sizeof(int64_t));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (int64_t *)ng_runtime_realloc(sizeof(int64_t), out, next_cap);
      if (!out) return arr;
    } else {
      int64_t *next = (int64_t *)ng_runtime_alloc((size_t)next_cap * sizeof(int64_t));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(int64_t));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_int_array_t r = {out, new_len};
  return r;
}

ng_int_array_t ng_int_array_pop(ng_int_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty int[]");
  ng_int_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_int2_array_t ng_int2_array_push(ng_int2_array_t arr, ng_int_array_t value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  ng_int_array_t *out = arr.data;
  if (!out) {
    out = (ng_int_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_int_array_t));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (ng_int_array_t *)ng_runtime_realloc(sizeof(ng_int_array_t), out, next_cap);
      if (!out) return arr;
    } else {
      ng_int_array_t *next =
          (ng_int_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_int_array_t));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(ng_int_array_t));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_int2_array_t r = {out, new_len};
  return r;
}

ng_int2_array_t ng_int2_array_pop(ng_int2_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty int[][]");
  ng_int2_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_float_array_t ng_float_array_push(ng_float_array_t arr, double value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  double *out = arr.data;
  if (!out) {
    out = (double *)ng_runtime_alloc((size_t)next_cap * sizeof(double));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (double *)ng_runtime_realloc(sizeof(double), out, next_cap);
      if (!out) return arr;
    } else {
      double *next = (double *)ng_runtime_alloc((size_t)next_cap * sizeof(double));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(double));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_float_array_t r = {out, new_len};
  return r;
}

ng_float_array_t ng_float_array_pop(ng_float_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty float[]");
  ng_float_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_float2_array_t ng_float2_array_push(ng_float2_array_t arr, ng_float_array_t value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  ng_float_array_t *out = arr.data;
  if (!out) {
    out = (ng_float_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_float_array_t));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (ng_float_array_t *)ng_runtime_realloc(sizeof(ng_float_array_t), out, next_cap);
      if (!out) return arr;
    } else {
      ng_float_array_t *next =
          (ng_float_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_float_array_t));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(ng_float_array_t));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_float2_array_t r = {out, new_len};
  return r;
}

ng_float2_array_t ng_float2_array_pop(ng_float2_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty float[][]");
  ng_float2_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_bool_array_t ng_bool_array_push(ng_bool_array_t arr, bool value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  bool *out = arr.data;
  if (!out) {
    out = (bool *)ng_runtime_alloc((size_t)next_cap * sizeof(bool));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (bool *)ng_runtime_realloc(sizeof(bool), out, next_cap);
      if (!out) return arr;
    } else {
      bool *next = (bool *)ng_runtime_alloc((size_t)next_cap * sizeof(bool));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(bool));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_bool_array_t r = {out, new_len};
  return r;
}

ng_bool_array_t ng_bool_array_pop(ng_bool_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty bool[]");
  ng_bool_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_bool2_array_t ng_bool2_array_push(ng_bool2_array_t arr, ng_bool_array_t value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  ng_bool_array_t *out = arr.data;
  if (!out) {
    out = (ng_bool_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_bool_array_t));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (ng_bool_array_t *)ng_runtime_realloc(sizeof(ng_bool_array_t), out, next_cap);
      if (!out) return arr;
    } else {
      ng_bool_array_t *next =
          (ng_bool_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_bool_array_t));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(ng_bool_array_t));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_bool2_array_t r = {out, new_len};
  return r;
}

ng_bool2_array_t ng_bool2_array_pop(ng_bool2_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty bool[][]");
  ng_bool2_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_string_array_t ng_string_array_push(ng_string_array_t arr, const char *value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  const char **out = arr.data;
  if (!out) {
    out = (const char **)ng_runtime_alloc((size_t)next_cap * sizeof(const char *));
    if (!out) return arr;
    if (!ng_array_cap_set((void *)out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked((void *)out)) {
      out = (const char **)ng_runtime_realloc(sizeof(const char *), (void *)out, next_cap);
      if (!out) return arr;
    } else {
      const char **next =
          (const char **)ng_runtime_alloc((size_t)next_cap * sizeof(const char *));
      if (!next) return arr;
      memcpy((void *)next, (const void *)out, (size_t)arr.len * sizeof(const char *));
      out = next;
      if (!ng_array_cap_set((void *)out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_string_array_t r = {out, new_len};
  return r;
}

ng_string_array_t ng_string_array_pop(ng_string_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty string[]");
  ng_string_array_t r = {arr.data, arr.len - 1};
  return r;
}

ng_string2_array_t ng_string2_array_push(ng_string2_array_t arr, ng_string_array_t value) {
  int64_t new_len = arr.len + 1;
  int64_t cap = ng_array_cap_get_or_len(arr.data, arr.len);
  int64_t next_cap = ng_next_capacity(cap, new_len);

  ng_string_array_t *out = arr.data;
  if (!out) {
    out = (ng_string_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_string_array_t));
    if (!out) return arr;
    if (!ng_array_cap_set(out, next_cap)) return arr;
  } else if (cap < new_len) {
    if (ng_runtime_ptr_is_tracked(out)) {
      out = (ng_string_array_t *)ng_runtime_realloc(sizeof(ng_string_array_t), out, next_cap);
      if (!out) return arr;
    } else {
      ng_string_array_t *next =
          (ng_string_array_t *)ng_runtime_alloc((size_t)next_cap * sizeof(ng_string_array_t));
      if (!next) return arr;
      memcpy(next, out, (size_t)arr.len * sizeof(ng_string_array_t));
      out = next;
      if (!ng_array_cap_set(out, next_cap)) return arr;
    }
  }

  out[new_len - 1] = value;
  ng_string2_array_t r = {out, new_len};
  return r;
}

ng_string2_array_t ng_string2_array_pop(ng_string2_array_t arr) {
  if (arr.len <= 0) ng_runtime_error("pop() on empty string[][]");
  ng_string2_array_t r = {arr.data, arr.len - 1};
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
