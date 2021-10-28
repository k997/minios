#include "init.h"
#include "interrupt.h"
#include "print.h"

void init_all() {
  put_str("init all\n");

  
  put_str("interrupt init\n");
  pic_init();      //初始化可编程中断控制器
  interrupt_program_init();//设置中断处理例程
  idt_init();      // 初始化中断描述符表
  put_str("interrupt init done\n");
  
}