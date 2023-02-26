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

   dir *p_dir = sys_opendir("/dir1/subdir1");
   if (p_dir)
   {
      printk("/dir1/subdir1 open done!\n");
      printk("/dir1/subdir1 inode_nr is %d\n",p_dir->inode->i_nr);
      if (sys_closedir(p_dir) == 0)
         printk("/dir1/subdir1 close done!\n");
      else
         printk("/dir1/subdir1 close fail!\n");
   }
   else
      printk("/dir1/subdir1 open fail!\n");
   while (1)
      ;
   return 0;
}
