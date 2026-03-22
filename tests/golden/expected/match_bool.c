#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/match_bool.ngawi */

int main(void);

int main(void)
{
  bool ok = true;
  switch (ok)
  {
    case 1:
    {
      {
        ng_print_string("yes");
        ng_print_string("\n");
      }
      break;
    }
    case 0:
    {
      {
        ng_print_string("no");
        ng_print_string("\n");
      }
      break;
    }
  }
  return 0;
}

