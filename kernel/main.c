#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "bitmap.h"

void main(void)
{
  bitmap btmp;
  btmp.bits=(void*)(0xc009a000);
  btmp.byte_length = 1;
  bitmap_init(&btmp);
  put_int(btmp.bits[0]);
  put_str("\naddr bits:");
  put_int((uint32_t)btmp.bits);
  put_str("\n");
  bitmap_alloc(&btmp,3);
  put_int(btmp.bits[0]);
  put_str("\n");
  bitmap_alloc(&btmp,2);
  put_int(btmp.bits[0]);
  put_str("\n");
  bitmap_free(&btmp,3,2);
  put_int(btmp.bits[0]);
  while (1)
    ;
}