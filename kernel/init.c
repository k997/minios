#include "init.h"
#include "interrupt.h"
#include "print.h"
#include "memory.h"
#include "timer.h"
#include "thread.h"
#include "gdt.h"
void init_all()
{
  put_str("init all\n");

  put_str("interrupt init\n");
  pic_init();               //初始化可编程中断控制器
  interrupt_program_init(); //设置中断处理例程
  idt_init();               // 初始化中断描述符表
  put_str("interrupt init done\n");

  put_str("memory init\n");
  mem_init();
  put_str("memory init done\n");

  put_str("timer init\n");
  timer_init();
  put_str("timer init done\n");

  put_str("thread module init\n");
  thread_init();
  put_str("thread module init done\n");

  put_str("tss init\n");
  gdt_init();
  put_str("tss init done\n");
}