#ifndef __LIB_INTERRUPT_H
#define __LIB_INTERRUPT_H
#include "stdint.h"

typedef void *interrupt_handler;
typedef enum
{
    INTR_OFF = 0, // 关中断
    INTR_ON = 1   // 开中断
} interrupt_status;
#define IDT_DESC_CNT 0x21 // 目前总共支持的中断数
void interrupt_program_init(void);
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
