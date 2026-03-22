#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/factorial.ngawi */

int64_t fact(int64_t n);
int main(void);

int64_t fact(int64_t n)
{
  if ((n <= 1))
  {
    return 1;
  }
  return (n * fact((n - 1)));
}

int main(void)
{
  int64_t x = 5;
  int64_t y = fact(x);
  ng_print_string("fact");
  ng_print_string(" ");
  ng_print_int(x);
  ng_print_string(" ");
  ng_print_string("=");
  ng_print_string(" ");
  ng_print_int(y);
  ng_print_string("\n");
  return 0;
}

