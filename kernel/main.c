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
   char filename[] = "/file1/hellow/da/a";
   char buf[MAX_FILE_NAME_LEN];
   char *rest = filename;
   while (rest != NULL)
   {
      rest = path_parse(rest, buf);

      if (rest)
         printk("rest: %s, buf: %s\n", rest, buf);
      else
         printk("rest:  , buf: %s\n", buf);
   }


   while (1)
      ;
   return 0;
}
