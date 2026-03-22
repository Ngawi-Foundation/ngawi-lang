#include "ngawi_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct RuntimeStringAllocs {
  char **items;
  size_t count;
  size_t cap;
  int initialized;
} RuntimeStringAllocs;

static RuntimeStringAllocs g_runtime_string_allocs = {0};

static void ng_runtime_string_cleanup(void) {
  for (size_t i = 0; i < g_runtime_string_allocs.count; i++) {
    free(g_runtime_string_allocs.items[i]);
  }
  free(g_runtime_string_allocs.items);
  g_runtime_string_allocs.items = NULL;
  g_runtime_string_allocs.count = 0;
  g_runtime_string_allocs.cap = 0;
  g_runtime_string_allocs.initialized = 0;
}

static int ng_runtime_string_init(void) {
  if (g_runtime_string_allocs.initialized) return 1;
  if (atexit(ng_runtime_string_cleanup) != 0) return 0;
  g_runtime_string_allocs.initialized = 1;
  return 1;
}

static char *ng_runtime_string_alloc(size_t size) {
  if (!ng_runtime_string_init()) return NULL;

  char *p = (char *)malloc(size);
  if (!p) return NULL;

  if (g_runtime_string_allocs.count == g_runtime_string_allocs.cap) {
    size_t next_cap = g_runtime_string_allocs.cap == 0 ? 64 : g_runtime_string_allocs.cap * 2;
    char **next = (char **)realloc(g_runtime_string_allocs.items, next_cap * sizeof(char *));
    if (!next) {
      free(p);
      return NULL;
    }
    g_runtime_string_allocs.items = next;
    g_runtime_string_allocs.cap = next_cap;
  }

  g_runtime_string_allocs.items[g_runtime_string_allocs.count++] = p;
  return p;
}

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
