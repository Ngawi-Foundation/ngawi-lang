#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/string_eq.ngawi */

int main(void);

int main(void)
{
  const char * a = "ngawi";
  const char * b = "ngawi";
  const char * c = "lang";
  ng_print_bool(ng_string_eq(a, b));
  ng_print_string(" ");
  ng_print_bool((!ng_string_eq(a, c)));
  ng_print_string("\n");
  return 0;
}

