#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "bitmap.h"
#include "memory.h"
#include "thread.h"

void thread_run_func(void *args);
void main(void)
{
  init_all();
  thread_create("taskA", 10, thread_run_func, "taskA ");
  thread_create("taskB", 20, thread_run_func, "taskB ");
  thread_create("taskC", 30, thread_run_func, "taskC ");
  void* page = kernel_page_alloc_from(0xc2000000,1);
  put_int(page);
  put_char('\n');

  put_int(vaddr2paddr((uint32_t)page));
  put_char('\n');
  interrupt_enable();
  while (1)

    ;
}

// 若将 thread_run_func 的实现放在 main 函数上面则错误
void thread_run_func(void *args)
{
  char *_args = (char *)args;

  while (1)
    put_str(_args);
  ;
}
