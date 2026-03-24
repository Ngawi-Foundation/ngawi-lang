#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/array_int.ngawi */

int main(void);

int main(void)
{
  ng_int_array_t a = (ng_int_array_t){.data=(int64_t[]){10, 20, 30}, .len=3};
  ng_float_array_t f = (ng_float_array_t){.data=(double[]){1.5, 2.5}, .len=2};
  ng_bool_array_t b = (ng_bool_array_t){.data=(bool[]){true, false}, .len=2};
  ng_string_array_t s = (ng_string_array_t){.data=(const char *[]){"aa", "bb"}, .len=2};
  a = ng_int_array_push(a, 40);
  a = ng_int_array_pop(a);
  s = ng_string_array_push(s, "cc");
  ng_int_array_set(&a, (int64_t)(1), 99);
  int64_t x = ng_int_array_get(a, (int64_t)(1));
  double y = ng_float_array_get(f, (int64_t)(0));
  bool z = ng_bool_array_get(b, (int64_t)(1));
  const char * t = ng_string_array_get(s, (int64_t)(2));
  int64_t n = ((a).len);
  ng_print_int(x);
  ng_print_string(" ");
  ng_print_float(y);
  ng_print_string(" ");
  ng_print_bool(z);
  ng_print_string(" ");
  ng_print_string(t);
  ng_print_string(" ");
  ng_print_int(n);
  ng_print_string("\n");
  return 0;
}

