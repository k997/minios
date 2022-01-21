#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "bitmap.h"
#include "memory.h"
#include "thread.h"
#include "process.h"
#include "ioqueue.h"
#include "timer.h"

ioqueue ioq;

void consumerA();
void consumerB();
void producer();
void print();
void main(void)
{
  init_all();

  ioqueue_init(&ioq);
  // task_struct *taskP = thread_create("producer", 100, producer, NULL);
  // task_struct *taskC1 = thread_create("consumer1", 10, consumerA, NULL);
  // task_struct *taskC2 = thread_create("consumer2", 10, consumerB, NULL);
  task_struct *taskC = thread_create("print", 20, print, NULL);

  put_str("task has been created \n");
  // process_start(taskP);
  // process_start(taskC2);
  // process_start(taskC1);
  process_start(taskC);
  put_str("task has been ready \n");

  interrupt_enable();

  while (1)
    ;
}
void print()
{
  while (1)
  {
    put_str("hellp");
    sleep(5);
  }
}
void consumerA()
{

  while (1)
  {
    interrupt_status old_status = interrupt_disable();
    if (!ioqueue_empty(&ioq))
    {
      put_str(" A:");
      put_char(ioqueue_getchar(&ioq));
      thread_yield();// 若 buf size 太小，不加 yield 的话在前的消费者会直接消费完所有字符，导致在后的消费者一直无法获取字符串
    }
    interrupt_set_status(old_status);
  }
}
void consumerB()
{

  while (1)
  {
    interrupt_status old_status = interrupt_disable();
    if (!ioqueue_empty(&ioq))
    {
      put_str(" B:");
      put_char(ioqueue_getchar(&ioq));
      thread_yield();
    }
    interrupt_set_status(old_status);
  }
}
void producer()
{
  while (1)
  {
    interrupt_status old_status = interrupt_disable();
    if (!ioqueue_full(&ioq))
      ioqueue_putchar(&ioq, 'P');
    interrupt_set_status(old_status);
  }
}