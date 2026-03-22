#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/break_continue.ngawi */

int main(void);

int main(void)
{
  int64_t i = 0;
  int64_t sum = 0;
  while ((i < 6))
  {
    i = (i + 1);
    if ((i == 2))
    {
      continue;
    }
    if ((i == 5))
    {
      break;
    }
    sum = (sum + i);
  }
  ng_print_int(sum);
  ng_print_string("\n");
  return 0;
}

