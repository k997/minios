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

void thread_run_func(void *args);

void u_func();
void main(void)
{
  init_all();

  task_struct *taskA = process_create("taskA", u_func);
  task_struct *taskB = thread_create("taskB", 20, thread_run_func, "taskB ");
  task_struct *taskC = thread_create("taskC", 30, thread_run_func, "taskC ");
  put_str("task has been created \n");
  process_start(taskA);
  process_start(taskB);
  process_start(taskC);
  put_str("task has been ready \n");

  interrupt_enable();

  while (1)

    ;
}

void u_func()
{

  int i = 0;
  while (1)
  {
    // 用户模式下不能运行 put_str(); 否则访问 gs 寄存器时导致 gp 异常
    // put_str("A");
    i++;
  };
}

// 若将 thread_run_func 的实现放在 main 函数上面则错误
void thread_run_func(void *args)
{
  char *_args = (char *)args;

  while (1)
    put_str(_args);
  ;
}
