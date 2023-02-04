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
   task_struct *ta = thread_create("k_thread_a", 31, k_thread_a, "test_small_a");
   thread_start(ta);
   thread_start(thread_create("k_thread_test_large", 31, k_thread_test_large, "k_thread_test_large"));
   thread_start(thread_create("k_thread_b", 31, k_thread_b, "test_small_b"));

   while (1)
      ;
   return 0;
}

void k_thread_test_large(void *arg)
{
   printk("%s start\n", (char *)arg);
   void *addr1;
   void *addr2;
   addr1 = sys_malloc(2048);
   addr2 = sys_malloc(4096);
   printk("addr1: %x ", addr1);
   printk("addr2: %x ", addr2);

   sys_free(addr1);
   sys_free(addr2);
   while (1)
      ;
}
/* 在线程中运行的函数 */
void k_thread_a(void *arg)
{
   printk("%s start\n",(char*)arg);

   void *addr1;
   void *addr2;
   void *addr3;
   void *addr4;
   void *addr5;
   void *addr6;
   void *addr7;
   int max = 10;
   while (max-- > 0)
   {
      printk("thread a loop %d  ", max);
      int size = 128;
      addr1 = sys_malloc(size);
      printk("addr1: %x ", addr1);
      size *= 2;
      addr2 = sys_malloc(size);
      printk("addr2: %x ", addr2);

      size *= 2;
      addr3 = sys_malloc(size);
      printk("addr3: %x ", addr3);

      sys_free(addr2);
      addr4 = sys_malloc(size);
      printk("addr4: %x ", addr4);
      size *= 2;

      addr5 = sys_malloc(size);
      // printk("sise %d ",size);
      printk("addr5: %x ", addr5);

      addr6 = sys_malloc(size);
      printk("addr6: %x ", addr6);

      sys_free(addr5);
      // printk("free addr5 ");
      size *= 2;
      addr7 = sys_malloc(size);
      printk("addr7: %x ", addr7);

      sys_free(addr6);
      sys_free(addr7);
      sys_free(addr3);
      sys_free(addr4);
   }
   printk(" thread_a end\n");
   while (1)
      ;
}

/* 在线程中运行的函数 */
void k_thread_b(void *arg)
{
   printk("%s start\n",(char*)arg);
   void *addr1;
   void *addr2;
   void *addr3;
   void *addr4;
   void *addr5;
   void *addr6;
   void *addr7;
   void *addr8;
   void *addr9;
   int max = 10;
   while (max-- > 0)
   {
      int size = 9;
      addr1 = sys_malloc(size);
      size *= 2;
      addr2 = sys_malloc(size);
      size *= 2;
      sys_free(addr2);
      addr3 = sys_malloc(size);
      sys_free(addr1);
      addr4 = sys_malloc(size);
      addr5 = sys_malloc(size);
      addr6 = sys_malloc(size);
      sys_free(addr5);
      size *= 2;
      addr7 = sys_malloc(size);
      sys_free(addr6);
      sys_free(addr7);
      sys_free(addr3);
      sys_free(addr4);

      size *= 2;
      size *= 2;
      size *= 2;
      addr1 = sys_malloc(size);
      addr2 = sys_malloc(size);
      addr3 = sys_malloc(size);
      addr4 = sys_malloc(size);
      addr5 = sys_malloc(size);
      addr6 = sys_malloc(size);
      addr7 = sys_malloc(size);
      addr8 = sys_malloc(size);
      addr9 = sys_malloc(size);
      sys_free(addr1);
      sys_free(addr2);
      sys_free(addr3);
      sys_free(addr4);
      sys_free(addr5);
      sys_free(addr6);
      sys_free(addr7);
      sys_free(addr8);
      sys_free(addr9);
   }
   printk(" thread_b end\n");
   while (1)
      ;
}

// void main(void)
// {
//   init_all();

//   interrupt_enable();

//   void *addr1, *addr2, *addr3;

//   addr1 = sys_malloc(33);
//   printf("addr1 %x\n", addr1);

//   addr2 = sys_malloc(62);
//   printf("addr2 %x\n", addr2);
//   sys_free(addr2);
//   sys_free(addr1);

//   addr1 = sys_malloc(3);
//   printf("%x\n", addr1);

//   sys_free(addr1);

//   addr2 = sys_malloc(512);
//   printf("%x\n", addr2);
//   sys_free(addr2);

//   addr3 = sys_malloc(512);
//   printf("%x\n", addr3);
//   sys_free(addr3);

//   addr2 = sys_malloc(128);
//   printf("%x\n", addr2);
//   addr3 = sys_malloc(512);
//   printf("%x\n", addr3);
//     addr1 = sys_malloc(512);
//   printf("%x\n", addr1);

//   while (1)
//     ;
// }
