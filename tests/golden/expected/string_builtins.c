#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/string_builtins.ngawi */

int main(void);

int main(void)
{
  const char * s = "  NgawiLang  ";
  const char * t = ng_string_trim(s);
  bool c = ng_string_contains(t, "awi");
  bool p = ng_string_starts_with(t, "Nga");
  bool e = ng_string_ends_with(t, "Lang");
  const char * low = ng_string_to_lower(t);
  const char * up = ng_string_to_upper(t);
  ng_print_bool(c);
  ng_print_string(" ");
  ng_print_bool(p);
  ng_print_string(" ");
  ng_print_bool(e);
  ng_print_string(" ");
  ng_print_string(low);
  ng_print_string(" ");
  ng_print_string(up);
  ng_print_string("\n");
  return 0;
}

