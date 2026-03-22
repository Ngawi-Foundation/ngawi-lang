#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/forward_call.ngawi */

int main(void);
int64_t twice(int64_t x);

int main(void)
{
  int64_t v = twice(21);
  ng_print_int(v);
  ng_print_string("\n");
  return 0;
}

int64_t twice(int64_t x)
{
  return (x * 2);
}

