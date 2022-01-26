#ifndef __LIB_KERNEL_INTERRUPT_H
#define __LIB_KERNEL_INTERRUPT_H
#include "stdint.h"

typedef void *interrupt_handler;
typedef enum
{
    INTR_OFF = 0, // 关中断
    INTR_ON = 1   // 开中断
} interrupt_status;


#define MAX_INTR_NR 0x21 // 目前总共支持的中断数


/* 中段处理程序自己保存中断上下文使用 idt_desc_register */
void idt_desc_register(uint8_t intr_ver_nr, uint8_t attr, interrupt_handler handler);
void interrupt_program_init(void);
/* 中段处理程序自己不保存中断上下文使用 interrupt_program_register */
void interrupt_program_register(uint8_t ver_nr, char *name, interrupt_handler handler);
void pic_init(void);
void idt_init();
interrupt_status interrupt_get_status(void);
/* 开中断并返回开中断前的状态*/
interrupt_status interrupt_enable(void);

/* 关中断并返回关中断前的状态*/
interrupt_status interrupt_disable(void);

interrupt_status interrupt_set_status(interrupt_status status);
#endif
