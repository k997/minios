#ifndef __LIB_INTERRUPT_H
#define __LIB_INTERRUPT_H
typedef void *interrupt_handler;
#define IDT_DESC_CNT 0x21 // 目前总共支持的中断数
void interrupt_program_init(void);
void pic_init(void);
void idt_init();
#endif