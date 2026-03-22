#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/string_concat.ngawi */

int main(void);

int main(void)
{
  const char * a = "ng";
  const char * b = "awi";
  const char * s = ng_string_concat(a, b);
  ng_print_string(s);
  ng_print_string("\n");
  return 0;
}

