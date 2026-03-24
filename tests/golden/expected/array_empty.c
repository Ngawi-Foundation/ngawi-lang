#include <stdbool.h>
#include <stdint.h>
#include "ngawi_runtime.h"

/* Generated from examples/array_empty.ngawi */

int main(void);

int main(void)
{
  ng_int_array_t a = (ng_int_array_t){.data=(int64_t[]){}, .len=0};
  ng_print_int(((a).len));
  ng_print_string("\n");
  return 0;
}

