#include "interrupt.h"
#include "thread.h"
#include "debug.h"

// ticks 是内核自中断开启以来总共的嘀嗒数
uint32_t ticks;
static void interrupt_timer_handler(void);
void timer_init(void)
{
    interrupt_program_register(0x20, "interrupt timer", interrupt_timer_handler);
}
static void interrupt_timer_handler(void)
{
    ticks++;

    task_struct *current_thread = thread_running();
    // 判断栈是否溢出
    ASSERT(current_thread->stack_magic == THREAD_STACK_MAGIC_NUM);
    current_thread->elapsed_ticks++;
    if (current_thread->ticks == 0) // 时间片用完则调度新线程
    {
        thread_schedule();
        return;
    }
    current_thread->ticks--;
}