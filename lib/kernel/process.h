#ifndef __LIB_KERNEL_RPOCESS_H
#define __LIB_KERNEL_PROCESS_H
#include "thread.h"

// 特权级 3 下用户栈，位于用户地址空间最高处的一页，也是内核地址空间之下一页 (0x1000 = 4 KB)
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
//  Linux 用户程序入口地址
#define USER_VADDR_START 0x8048000
#define default_prio 31
// interrupt.asm 中断退出程序
extern void interrupt_exit(void);

task_struct *process_create(char *name,void *program);
void process_start(task_struct *pthread);
void process_activate(task_struct *pthread);

#endif