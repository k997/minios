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

void k_thread_a(void *);
void k_thread_b(void *);
void k_thread_test_large(void *arg);
int main(void)
{
   printk("I am kernel\n");
   init_all();
   interrupt_enable();


   while (1)
      ;
   return 0;
}
