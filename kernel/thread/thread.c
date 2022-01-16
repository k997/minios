#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include "process.h"

task_struct *main_thread; // 主线程 PCB
task_struct *idle_thread; // 系统无任务时时运行的线程
list thread_ready_list;
list thread_all_list;
static void make_main_thread(char *name, int priority);
static void kernel_thread(thread_func *func, void *func_args);
static void thread_kstack(task_struct *thread, thread_func func, void *func_args);
static void idle(void *args);

// 线程模块初始化
void thread_init(void)
{
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    // main 线程的 PCB 空间已在 0xc009e000 提前分配好，无需再次分配空间, 且此时其已经是运行状态，因此单独为 main 线程创建 PCB
    make_main_thread("main", 31);
    idle_thread = thread_create("idle", 10, idle, NULL);
    thread_start(idle_thread);
}

task_struct *thread_create(char *name, int priority, thread_func func, void *func_args)
{
    // 为 PCB 申请空间并清 0
    task_struct *thread = kernel_page_alloc(1);
    memset(thread, 0, sizeof(*thread));

    strcpy(thread->name, name);
    thread->status = TASK_NEW;
    thread->priority = priority;
    thread->self_kstack = (uint32_t *)((uint32_t)thread + PG_SIZE); // 设置 PCB 栈顶为 PCB 顶端地址
    thread->ticks = priority;
    thread->elapsed_ticks = 0;
    thread->pgdir = NULL;
    thread->stack_magic = THREAD_STACK_MAGIC_NUM;

    // thread_kstack 预留中断栈及内核栈的栈空间
    // 将 thread->self_kstack 指向线程的线程栈，使栈中 eip 指向要运行的函数 func
    thread_kstack(thread, func, func_args);

    return thread;
}

void thread_start(task_struct *pthread)
{
    interrupt_status old_status = interrupt_disable();
    pthread->status = TASK_READY;

    // 线程加入就绪队列
    ASSERT(!list_find_elem(&thread_ready_list, &pthread->thread_status_list_tag))
    list_append(&thread_ready_list, &pthread->thread_status_list_tag);
    // 线程加入全部线程队列
    ASSERT(!list_find_elem(&thread_all_list, &pthread->thread_all_list_tag))
    list_append(&thread_all_list, &pthread->thread_all_list_tag);
    interrupt_set_status(old_status);
}

task_struct *thread_running(void)
{
    // 栈位于 PCB 顶端，通过栈获取 PCB 地址
    // 获取当前栈的地址
    uint32_t esp;
    asm("mov %%esp,%0" // %0 即变量 esp，g 表示内存或寄存器约束
        : "=g"(esp));
    return (task_struct *)(esp & 0xfffff000);
}

void thread_schedule(void)
{
    // 触发时钟中断后 CPU 自动关闭中断，此时 CPU 应处于关中断状态
    ASSERT(interrupt_get_status() == INTR_OFF);
    // 正在运行的线程不在 thread_ready_list 中，只在 thread_all_list 中
    task_struct *current_thread = thread_running();
    if (current_thread->status == TASK_RUNNING) //时间片到期,当前线程重新加入就绪队列，并重置剩余可运行的时间片
    {
        ASSERT(!list_find_elem(&thread_ready_list, &current_thread->thread_status_list_tag));
        list_append(&thread_ready_list, &current_thread->thread_status_list_tag);
        current_thread->status = TASK_READY;
        current_thread->ticks = current_thread->priority;
    }

    // 若任务就绪队列为空，则运行 idle 线程
    if (list_empty(&thread_ready_list))
    {
        thread_unblock(idle_thread);
    }

    list_elem *thread_tag = list_pop(&thread_ready_list);
    task_struct *next_thread = container_of(task_struct, thread_status_list_tag, thread_tag);
    next_thread->status = TASK_RUNNING;
    // 激活任务页表
    process_activate(next_thread);
    switch_to(current_thread, next_thread);
}

/*
    thread_kstack 预留中断栈及内核栈的栈空间
    将 thread->self_kstack 指向线程的线程栈，使栈中 eip 指向要运行的函数 func
*/
static void thread_kstack(task_struct *thread, thread_func func, void *func_args)
{
    // 预留中断栈空间
    thread->self_kstack -= sizeof(interrupt_stack);
    // 预留线程栈空间
    thread->self_kstack -= sizeof(thread_stack);
    // 初始化线程栈,将待执行的函数和参数放到 thread_stack 中相应的位置
    thread_stack *kthread_stack = (thread_stack *)thread->self_kstack;
    kthread_stack->ebp = 0;
    kthread_stack->ebx = 0;
    kthread_stack->esi = 0;
    kthread_stack->edi = 0;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = func;
    kthread_stack->func_args = func_args;
}

void thread_block(task_status stat)
{
    // 只有以下三种状态可以使线程不被调度
    ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING));
    interrupt_status old_status = interrupt_disable();
    task_struct *current_thread = thread_running();
    current_thread->status = stat;
    // 将当前线程换下处理器
    thread_schedule();
    interrupt_set_status(old_status);
}

void thread_unblock(task_struct *pthread)
{
    interrupt_status old_status = interrupt_disable();
    // 断言仅开启调试模式启用
    ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING));
    if (pthread->status != TASK_READY)
    {
        ASSERT(!list_find_elem(&thread_ready_list, &pthread->thread_status_list_tag));
        // 放入就绪队列最前，使其尽快被调度
        list_push(&thread_ready_list, &pthread->thread_status_list_tag);
        pthread->status = TASK_READY;
    }
    interrupt_set_status(old_status);
}

// 线程主动让出处理器
void thread_yield(void)
{
    task_struct *current_thread = thread_running();
    interrupt_status old_status = interrupt_disable();
    list_append(&thread_ready_list, &current_thread->thread_status_list_tag);
    current_thread->status = TASK_READY;
    thread_schedule();
    interrupt_set_status(old_status);
}
static void kernel_thread(thread_func *func, void *func_args)
{
    // 任务调度基于时钟中断
    // 开中断防止时钟中断屏蔽其他中断，导致无法调度其他线程
    interrupt_enable();
    func(func_args);
}
static void make_main_thread(char *name, int priority)
{
    main_thread = thread_running();
    memset(main_thread, 0, sizeof(*main_thread));
    strcpy(main_thread->name, name);
    // main_thread 已经在运行
    main_thread->status = TASK_RUNNING;
    main_thread->priority = priority;
    main_thread->self_kstack = (uint32_t *)((uint32_t)main_thread + PG_SIZE);
    main_thread->ticks = priority;
    main_thread->elapsed_ticks = 0;
    main_thread->pgdir = NULL;
    main_thread->stack_magic = THREAD_STACK_MAGIC_NUM;

    ASSERT(!list_find_elem(&thread_all_list, &main_thread->thread_all_list_tag));
    list_append(&thread_all_list, &main_thread->thread_all_list_tag);
}

// 就绪队列无任务时运行的任务
static void idle(void *args)
{
    while (1)
    {
        // idle 线程阻塞自己
        thread_block(TASK_BLOCKED);
        // 在 thread_schedule 中 unblock，执行 hlt 指令
        asm volatile("sti;hlt" ::/* 运行 hlt 必须在开中断 sti 情况下运行 */
                     : "memory");
    }
}