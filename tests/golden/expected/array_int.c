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
  (a).data[ng_array_checked_index((int64_t)(1), ((a).len))] = 99;
  int64_t x = ((a).data[ng_array_checked_index((int64_t)(1), ((a).len))]);
  double y = ((f).data[ng_array_checked_index((int64_t)(0), ((f).len))]);
  bool z = ((b).data[ng_array_checked_index((int64_t)(1), ((b).len))]);
  const char * t = ((s).data[ng_array_checked_index((int64_t)(2), ((s).len))]);
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

