#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/cast.ngawi */

int main(void);

int main(void)
{
  double a = 7.9000000000000004;
  int64_t b = ((int64_t)(a));
  double c = ((double)(3));
  ng_print_int(b);
  ng_print_string(" ");
  ng_print_float(c);
  ng_print_string("\n");
  return 0;
}

