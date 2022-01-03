#ifndef __LIB_THREAD_H
#define __LIB_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"

#define THREAD_STACK_MAGIC_NUM 0x19870916
// 自定义通用函数类型,在多线程函数中作为形参

typedef void thread_func(void *);

typedef enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status;

// 中断栈
// 发生中断时用于保存进程上下文环境
// 实现用户进程时, 保存用户进程的初始信息
// 地址固定在页的最顶端
// 参考 kernel/interrupt/interrupt.asm 中入栈与出栈顺序
typedef struct interrupt_stack
{
    uint32_t ver_nr; // 中断向量号
    // pushad EAX,ECX,EDX,EBX,ESP(初始值)，EBP,ESI,EDI.
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; // esp 是不断变化的
                        // 虽然 pushad 压入 esp, 但 popad 忽略 esp
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    // 以下由 CPU 从低特权级进入高特权级时压入
    /*参考中断部分的中断与栈
    https://www.kongwg.top/archives/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E7%9C%9F%E8%B1%A1%E8%BF%98%E5%8E%9F10%E4%B8%AD%E6%96%AD
    若涉及特权级转移,SS_old、ESP_old 压入新栈,EFLAGS 、CS、EIP 依次入栈,若有中断错误码（ERROR_CODE），中断错误码入栈
    不涉及特权级转移EFLAGS 、CS、EIP 依次入栈,若有中断错误码（ERROR_CODE），中断错误码入栈
    */
    uint32_t err_code; // 中断错误码
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
} interrupt_stack;

// 线程栈
// 用于存储线程中待执行的函数
// 此结构在线程自己的内核栈中位置不固定
typedef struct thread_stack
{
    // 386 CPU 的所有寄存器具有全局性，对主调函数和被调函数均可见
    // ebp、ebx、edi、esi、和 esp 归主调函数所用，其余的寄存器归被调函数所用
    // 主调函数保存 ebp、ebx、edi、esi
    // 由调用约定来保证 esp
    // 其余寄存由被调函数保证
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    // eip ,函数指针，指向待运行的函数
    // 函数返回值类型 (* 指针变量名) (函数参数列表);
    // 返回类型 void，指针名 eip，参数列表thread_func *，void *
    void (*eip)(thread_func *func, void *func_args);
    // 以下代码仅供线程第一次被调度到 CPU 时使用

    void(*unused_retaddr); // 线程第一运行时通过 ret 进行调度
                           // 此处仅占位，充当返回地址，用于跳过内存空间，实际用不到
    thread_func *function; // 第一次被调度时的函数
    void *func_args;       // 第一次被调度时函数的参数
} thread_stack;

// PCB
typedef struct task_struct
{
    uint32_t *self_kstack; // PCB的栈顶为各内核线程自己的内核栈，包括线程栈和中断栈
    char name[16];
    task_status status;
    uint8_t priority;       // 线程优先级，实际是线程每次被调度后可运行的总时间片
    uint8_t ticks;          // 线程被调度后可运行的剩余时间片，线程重新加入就绪队列时重置该值为 priority 的值
    uint32_t elapsed_ticks; // 线程被创建后已运行的总的时间片
    // 由于 PCB 较大, 进程管理系统通过 tag 对线程进行管理
    // 配合 offsetof 和 container_of， 可实现 PCB 与 TAG 之间的相互转换
    list_elem thread_status_list_tag; // 线程状态队列的节点
    list_elem thread_all_list_tag;    // 全部线程队列的节点

    uint32_t *pgdir;      // 进程页表地址, 若是线程则为 NULL
    pool vaddr;
    uint32_t stack_magic; // PCB 的边界标记,用于检测栈的溢出, 防止栈内容覆盖 PCB 其他信息
                          // 该值为自定义的 magic number, 若 PCB 边界等于该值, 则说明 PCB 数据没有被栈覆盖
} task_struct;

// 线程模块初始化
void thread_init(void);
// 创建线程
task_struct *thread_create(char *name, int priority, thread_func func, void *func_args);
// 调度新的线程
void thread_schedule(void);
// 获取正在运行的线程
task_struct *thread_running(void);
// 阻塞当前线程
void thread_block(task_status stat);
// 解除指定线程阻塞状态
void thread_unblock(task_struct *pthread);
#endif