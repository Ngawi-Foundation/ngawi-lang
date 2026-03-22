#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/while.ngawi */

int main(void);

int main(void)
{
  int64_t i = 1;
  while ((i <= 3))
  {
    ng_print_int(i);
    ng_print_string("\n");
    i = (i + 1);
  }
  return 0;
}

