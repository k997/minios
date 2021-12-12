#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "bitmap.h"
#include "memory.h"

void main(void)
{
  init_all();

  void *addr = kernel_page_alloc(3);
  put_str("\nkernel_page start vaddr is ");
  put_int((uint32_t)addr);
  put_str("\n");
  while (1)
    ;
}