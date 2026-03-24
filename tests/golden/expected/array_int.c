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
  (a).data[1] = 99;
  int64_t x = ((a).data[1]);
  double y = ((f).data[0]);
  bool z = ((b).data[1]);
  const char * t = ((s).data[1]);
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

