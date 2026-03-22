#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/for_loop.ngawi */

int main(void);

int main(void)
{
  int64_t sum = 0;
  for (int64_t i = 1; (i <= 3); i = (i + 1))
  {
    sum = (sum + i);
  }
  ng_print_int(sum);
  ng_print_string("\n");
  return 0;
}

