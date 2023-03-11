#include "interrupt.h"
#include "thread.h"
#include "debug.h"
#include "global.h"

/*
    8253 计数器初始条件下中断发生频率为 18.206 次每秒，
    即每秒的滴答数为 18.206，此处记为 19
 */
#define ticks_per_second 19
/* 每次中断花费的毫秒数 */
#define mil_seconds_per_tick (1000 / ticks_per_second)

// ticks 是内核自中断开启以来总共的嘀嗒数
uint32_t ticks;
static void interrupt_timer_handler(void);
static void sleep_ticks(uint32_t sleep_ticks);

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

// 以秒为单位休眠
void sleep(uint32_t seconds)
{
    uint32_t _ticks = seconds * ticks_per_second;
    sleep_ticks(_ticks);
}

// 以毫秒为单位休眠
void sleep_mtime(uint32_t m_seconds)
{
    uint32_t _ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_tick);
    ASSERT(_ticks > 0);
    sleep_ticks(_ticks);
}

// 以滴答数为单位休眠
static void sleep_ticks(uint32_t sleep_ticks)
{
    uint32_t end_sleep_ticks = ticks + sleep_ticks;
    while (ticks < end_sleep_ticks)
        thread_yield();
}