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

void A();
void B();

void main(void)
{
  init_all();

  int *pa = sys_malloc(sizeof(int));
  printk("addr: %x\n", (uint32_t)pa);
  sys_free(pa);
  int *pb = sys_malloc(sizeof(char));

  printk("addr: %x\n", (uint32_t)pb);

  sys_free(pb);

  task_struct *taska = process_create("a", A);

  task_struct *taskb = process_create("b", B);

  process_start(taska);
  process_start(taskb);

  interrupt_enable();

  while (1)
    ;
}

void A()
{
  while (1)
  {
    printf("a\n");

    int *p = malloc(sizeof(int));
    printf("addr: %x\n", (uint32_t)p);
    free(p);
  }
}

void B()
{
  while (1)
  {
    printf("b\n");
    int *p1 = malloc(sizeof(char));
    int *p2 = malloc(sizeof(char));
    printf("addr: %x\n", (uint32_t)p1);
    printf("addr: %x\n", (uint32_t)p2);
    free(p1);
    free(p2);
  }
}