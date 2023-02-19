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
   char filename[] = "/file1";
   char content[] = "hello world";
   uint32_t len = strlen(content);
   int32_t fd;
   // 创建文件
   fd = sys_open(filename, O_CREAT);
   printk("create fd %d\n", fd);
   sys_close(fd);

   // 读取文件
   fd = sys_open(filename, O_W_ONLY);
   int nbytes = sys_write(fd, content, strlen(content));
   sys_close(fd);
   printk("write fd: %d, content's len is %d, write %d bytes\n", fd, len, nbytes);


   // 读取文件
   fd = sys_open(filename, O_R_ONLY);
   char buf[512] = {0};
   int32_t read_bytes = sys_read(fd, buf, len);
   printk("read fd %d,read_bytes %d,str: %s\n", fd, read_bytes, buf);
   sys_close(fd);

   sys_unlink(filename);
   fd = sys_open(filename, O_R_ONLY); // 此时应提示文件不存在

   while (1)
      ;
   return 0;
}
