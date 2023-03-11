#include "init.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "thread.h"
#include "process.h"
#include "ioqueue.h"
#include "timer.h"
#include "stdio.h"
#include "kstdio.h"
#include "syscall.h"
#include "ide.h"
#include "fs.h"
void print_pid();

int main(void)
{
   printk("I am kernel\n");
   init_all();

   interrupt_enable();
   printk("main thread pid is %d\n", sys_getpid());
   task_struct *task = process_create("pid", print_pid);
   process_start(task);
   while (1)
      ;
   return 0;
}
void print_pid()
{
   printf("user prog pid is %d\n", getpid());
   
   while (1)
      ;
}