#include "syscall.h"
#include "interrupt.h"
#include "debug.h"
#include "print.h"
#include "string.h"
#include "idt.h"
#include "memory.h"
#include "fs.h"


extern uint32_t syscall_handler(void);
syscall syscall_table[SYSCALL_TOTAL_NR];

// 系统调用初始化
void syscall_init()
{

    // 单独处理系统调用，系统调用对应的中断门 dpl 为 3
    // 系统调用的中断处理程序自己保存中断上下文，因此不用 interrupt_program_register
    idt_desc_register(SYSCALL_INTR_NR, IDT_DESC_ATTR_DPL3, syscall_handler);
    // 注册 write 系统调用
    syscall_register(SYS_WRITE, sys_write);
    syscall_register(SYS_MALLOC, sys_malloc);
    syscall_register(SYS_FREE, sys_free);
}

// 系统调用注册函数
void syscall_register(SYSCALL_NR _syscall_nr, syscall _syscall_func)
{
    ASSERT(_syscall_nr < SYSCALL_TOTAL_NR);
    syscall_table[_syscall_nr] = _syscall_func;
}

// write 系统调用封装，等价于 _syscall_3(SYS_WRITE, fd, buf, nbytes)
uint32_t write(int32_t fd, void *buf, uint32_t nbytes)
{
    return _syscall_3(SYS_WRITE, fd, buf, nbytes);
}

void free(void *ptr)
{
    _syscall_1(SYS_FREE, ptr);
}

void *malloc(uint32_t size)
{
    return (void *)_syscall_1(SYS_MALLOC, size);
}
