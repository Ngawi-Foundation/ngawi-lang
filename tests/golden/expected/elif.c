#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/elif.ngawi */

int main(void);

int main(void)
{
  int64_t x = 2;
  if ((x == 0))
  {
    ng_print_string("zero");
    ng_print_string("\n");
  }
  else
  if ((x == 1))
  {
    ng_print_string("one");
    ng_print_string("\n");
  }
  else
  if ((x == 2))
  {
    ng_print_string("two");
    ng_print_string("\n");
  }
  else
  {
    ng_print_string("other");
    ng_print_string("\n");
  }
  return 0;
}

