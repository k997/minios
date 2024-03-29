/*
C 语言编写中断处理程序，汇编语言负责处理中断上下文
*/

#include "print.h"
#include "interrupt.h"
#include "string.h"
struct interrupt_program
{
    interrupt_handler handler;
    char *name; // 下面的编译后地址高
};

// interrupt.asm中使用
struct interrupt_program interrupt_program_table[MAX_INTR_NR];

static void general_interrupt_handler(uint8_t ver_nr);
static void page_fault_handler();

// 设置中断处理程序
void interrupt_program_init(void)
{
    int i;
    for (i = 0; i < MAX_INTR_NR; i++)
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
    // interrupt_program_table[14].name = "#PF Page-Fault Exception";
    interrupt_program_register(14, "#PF Page-Fault Exception", page_fault_handler);
    // interrupt_program_table[15] 第15项是intel保留项，未使用
    interrupt_program_table[16].name = "#MF x87 FPU Floating-Point Error";
    interrupt_program_table[17].name = "#AC Alignment Check Exception";
    interrupt_program_table[18].name = "#MC Machine-Check Exception";
    interrupt_program_table[19].name = "#XF SIMD Floating-Point Exception";
}

// 注册中断处理程序
void interrupt_program_register(uint8_t ver_nr, char *name, interrupt_handler handler)
{
    interrupt_program_table[ver_nr].name = name;
    interrupt_program_table[ver_nr].handler = handler;
}
static void general_interrupt_handler(uint8_t ver_nr)
{
    // IRQ7 和 IRQ15 会产生伪中断，无需处理
    //  0x27 = 0010 0111 b
    // 0x2f 是从片 8259A 上的最后一个 IRQ 引脚，保留项
    if (ver_nr == 0x27 || ver_nr == 0x2f)
    {
        return;
    }
    put_str("\nint vector: 0x");
    put_int(ver_nr);
    put_str("\ninterrupt name: ");
    put_str(interrupt_program_table[ver_nr].name);
    put_str("\n");

    // 能进入中断处理程序就表示已经处在关中断情况下
    // 不会出现调度进程的情况。故下面的循环不会再被中断
    while (1)
        ;
}

static void page_fault_handler()
{
    put_str("\nint vector: 0xe");
    put_str("\ninterrupt name: #PF Page-Fault Exception\n");
    // 若为 Pagefault，打印缺失的地址
    // Pagefault 时缺失地址暂存在 cr2 寄存器

    int32_t page_fault_vaddr = 0;
    asm("movl %%cr2,%0"
        : "=r"(page_fault_vaddr));
    put_str("page fault addr is: ");
    put_int(page_fault_vaddr);
    put_str("\n");

    // 能进入中断处理程序就表示已经处在关中断情况下
    // 不会出现调度进程的情况。故下面的循环不会再被中断
    while (1)
        ;
}
