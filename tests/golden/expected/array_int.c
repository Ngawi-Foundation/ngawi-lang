#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/array_int.ngawi */

int main(void);

int main(void)
{
  ng_int_array_t a = (ng_int_array_t){.data=(int64_t[]){10, 20, 30}, .len=3};
  (a).data[1] = 99;
  int64_t x = ((a).data[1]);
  int64_t n = ((a).len);
  ng_print_int(x);
  ng_print_string(" ");
  ng_print_int(n);
  ng_print_string("\n");
  return 0;
}

