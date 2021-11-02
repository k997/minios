#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"



void main(void) {
  put_char('K');
  put_char('E');
  put_char('R');
  put_char('N');
  put_char('E');
  put_char('L');
  put_char('!');
  put_char('\b');
  put_char('\n');
  put_str("kernel string edition");
  put_char('\n');

  init_all();
  put_str("get interrupt status\n");
  put_int(interrupt_get_status());
  put_char('\n');

  put_str("enable interrupt\n");
  interrupt_set_status(INTR_ON);
  // 等待一段时间，
  uint32_t i = 0xFFFFF;
  while (i--)
    ;

  put_str("disable interrupt\n");
  interrupt_set_status(INTR_OFF);
  put_int(interrupt_get_status());
  put_char('\n');

  ASSERT(1==2);
  while (1)
    ;
}