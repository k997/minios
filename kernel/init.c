#include "init.h"
#include "interrupt.h"
#include "print.h"

void init_all() {
  put_str("init all\n");

  //初始化中断
  put_str("idt init\n");
  idt_init();
  
}