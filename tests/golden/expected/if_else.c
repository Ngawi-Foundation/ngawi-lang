#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/if_else.ngawi */

int main(void);

int main(void)
{
  int64_t n = 7;
  if ((n > 5))
  {
    ng_print_string("big");
    ng_print_string("\n");
  }
  else
  {
    ng_print_string("small");
    ng_print_string("\n");
  }
  return 0;
}

