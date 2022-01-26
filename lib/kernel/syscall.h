#ifndef __LIB_KERNEL_SYSCALL_H
#define __LIB_KERNEL_SYSCALL_H
#include "stdint.h"

// 系统调用总共支持 32 个子功能
#define SYSCALL_TOTAL_NR 32

typedef enum SYSCALL_NR
{
    SYS_WRITE
} SYSCALL_NR;
typedef void *syscall;


// 0 个参数系统调用
#define _syscall_0(SYSCALL_NR) ({  \
    int retval;                    \
    asm volatile("int $0x80"       \
                 : "=a"(retval)    \
                 : "a"(SYSCALL_NR) \
                 : "memory");      \
    retval;                        \
})

// 1 个参数系统调用，通过寄存器 ebx 传递参数
#define _syscall_1(SYSCALL_NR, ARG) ({       \
    int retval;                              \
    asm volatile("int $0x80"                 \
                 : "=a"(retval)              \
                 : "a"(SYSCALL_NR), "b"(ARG) \
                 : "memory");                \
    retval;                                  \
})

// 2 个参数系统调用，通过寄存器 ebx, ecx 传递参数
#define _syscall_2(SYSCALL_NR, ARG1, ARG2) ({            \
    int retval;                                          \
    asm volatile("int $0x80"                             \
                 : "=a"(retval)                          \
                 : "a"(SYSCALL_NR), "b"(ARG1), "c"(ARG2) \
                 : "memory");                            \
    retval;                                              \
})

// 3 个参数系统调用，通过寄存器 ebx, ecx, edx 传递参数
#define _syscall_3(SYSCALL_NR, ARG1, ARG2, ARG3) ({                 \
    int retval;                                                     \
    asm volatile("int $0x80"                                        \
                 : "=a"(retval)                                     \
                 : "a"(SYSCALL_NR), "b"(ARG1), "c"(ARG2), "d"(ARG3) \
                 : "memory");                                       \
    retval;                                                         \
})

uint32_t write(char *str);
void syscall_init();
void syscall_register(SYSCALL_NR _syscall_nr, syscall _syscall_func);

#endif