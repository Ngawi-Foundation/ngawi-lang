#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/array_nested.ngawi */

int main(void);

int main(void)
{
  ng_int_array_t a = (ng_int_array_t){.data=(int64_t[]){1, 2}, .len=2};
  ng_int_array_t b = (ng_int_array_t){.data=(int64_t[]){3, 4, 5}, .len=3};
  ng_int2_array_t m = (ng_int2_array_t){.data=(ng_int_array_t[]){a, b}, .len=2};
  ng_float2_array_t ff = (ng_float2_array_t){.data=(ng_float_array_t[]){(ng_float_array_t){.data=(double[]){1.5}, .len=1}, (ng_float_array_t){.data=(double[]){2.5, 3.5}, .len=2}}, .len=2};
  ng_bool2_array_t bb = (ng_bool2_array_t){.data=(ng_bool_array_t[]){(ng_bool_array_t){.data=(bool[]){true}, .len=1}, (ng_bool_array_t){.data=(bool[]){false, true}, .len=2}}, .len=2};
  ng_string2_array_t ss = (ng_string2_array_t){.data=(ng_string_array_t[]){(ng_string_array_t){.data=(const char *[]){"x"}, .len=1}, (ng_string_array_t){.data=(const char *[]){"y", "z"}, .len=2}}, .len=2};
  m = ng_int2_array_push(m, (ng_int_array_t){.data=(int64_t[]){7, 8}, .len=2});
  ng_int2_array_set(&m, (int64_t)(0), (ng_int_array_t){.data=(int64_t[]){9, 10}, .len=2});
  ff = ng_float2_array_push(ff, (ng_float_array_t){.data=(double[]){4.5}, .len=1});
  bb = ng_bool2_array_pop(bb);
  ss = ng_string2_array_push(ss, (ng_string_array_t){.data=(const char *[]){"k"}, .len=1});
  ng_int_array_t row = ng_int2_array_get(m, (int64_t)(0));
  int64_t x = ng_int_array_get(row, (int64_t)(1));
  int64_t y = ng_int_array_get(ng_int2_array_get(m, (int64_t)(1)), (int64_t)(2));
  double f = ng_float_array_get(ng_float2_array_get(ff, (int64_t)(1)), (int64_t)(1));
  bool q = ng_bool_array_get(ng_bool2_array_get(bb, (int64_t)(0)), (int64_t)(0));
  const char * t = ng_string_array_get(ng_string2_array_get(ss, (int64_t)(2)), (int64_t)(0));
  ng_print_int(x);
  ng_print_string(" ");
  ng_print_int(y);
  ng_print_string(" ");
  ng_print_float(f);
  ng_print_string(" ");
  ng_print_bool(q);
  ng_print_string(" ");
  ng_print_string(t);
  ng_print_string(" ");
  ng_print_int(((m).len));
  ng_print_string("\n");
  return 0;
}

