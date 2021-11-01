/*
C 语言编写中断处理程序，汇编语言负责处理中断上下文
*/

#include "stdint.h"
#include "print.h"
#include "interrupt.h"

struct interrupt_program
{
    interrupt_handler handler;
    char *name; // 下面的编译后地址高
};

// interrupt.asm中使用
struct interrupt_program interrupt_program_table[IDT_DESC_CNT];

static void general_interrupt_handler(uint8_t ver_nr);

// 设置中断处理程序
void interrupt_program_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        interrupt_program_table[i].name = "unknow";
        interrupt_program_table[i].handler = general_interrupt_handler;
    }
    interrupt_program_table[0].name = "#DE Divide Error";
    interrupt_program_table[1].name = "#DB Debug Exception";
    interrupt_program_table[2].name = "NMI Interrupt";
    interrupt_program_table[3].name = "#BP Breakpoint Exception";
    interrupt_program_table[4].name = "#OF Overflow Exception";
    interrupt_program_table[5].name = "#BR BOUND Range Exceeded Exception";
    interrupt_program_table[6].name = "#UD Invalid Opcode Exception";
    interrupt_program_table[7].name = "#NM Device Not Available Exception";
    interrupt_program_table[8].name = "#DF Double Fault Exception";
    interrupt_program_table[9].name = "Coprocessor Segment Overrun";
    interrupt_program_table[10].name = "#TS Invalid TSS Exception";
    interrupt_program_table[11].name = "#NP Segment Not Present";
    interrupt_program_table[12].name = "#SS Stack Fault Exception";
    interrupt_program_table[13].name = "#GP General Protection Exception";
    interrupt_program_table[14].name = "#PF Page-Fault Exception";
    // interrupt_program_table[15] 第15项是intel保留项，未使用
    interrupt_program_table[16].name = "#MF x87 FPU Floating-Point Error";
    interrupt_program_table[17].name = "#AC Alignment Check Exception";
    interrupt_program_table[18].name = "#MC Machine-Check Exception";
    interrupt_program_table[19].name = "#XF SIMD Floating-Point Exception";
}

static void general_interrupt_handler(uint8_t ver_nr)
{
    //IRQ7 和 IRQ15 会产生伪中断，无需处理
    // 0x27 = 0010 0111 b
    //0x2f 是从片 8259A 上的最后一个 IRQ 引脚，保留项
    if (ver_nr == 0x27 || ver_nr == 0x2f)
    {
        return;
    }
    put_str("int vector: 0x");
    put_int(ver_nr);
    put_char('\n');
}