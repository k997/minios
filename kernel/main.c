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
   printk("/dir1 create %s!\n", sys_mkdir("/dir1") == 0 ? "done" : "fail");
   printk("/dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");

   char path[] = "/dir1/subdir1";
   stat st;
   sys_stat(path, &st);
   printk("%s stat: inode nr %d, file size %d", path, st.st_i_nr, st.st_size);
   if (st.st_ftype == FS_DIRECTORY)
      printk(",file type: dir");
   else
      printk(",file type: file");

   while (1)
      ;
   return 0;
}
