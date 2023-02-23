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
#include "file.h"

int main(void)
{
   printk("I am kernel\n");
   init_all();
   interrupt_enable();
   printk("/dir3/subdir3 create %s!\n", sys_mkdir("/dir3/subdir3") == 0 ? "done" : "fail");
   printk("/dir1 create %s!\n", sys_mkdir("/dir1") == 0 ? "done" : "fail");
   printk("/dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
   while (1)
      ;
   return 0;
}
