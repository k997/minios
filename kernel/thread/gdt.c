#include "gdt.h"
#include "memory.h"
#include "string.h"
#include "global.h"

struct tss
{
    uint32_t backlink; // 指向上衣任务的 TSS
    uint32_t *esp0;    // 0 特权级栈指针
    uint32_t ss0;      // 0 特权级栈寄存器
    uint32_t *esp1;
    uint32_t ss1;
    uint32_t *esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;   // 保留未用
    uint32_t io_base; // io 位图在 TSS 的偏移地址
};

static struct tss tss;
static gdt_desc gdt_desc_make(void *desc_addr, uint32_t desc_limit,
                              uint8_t desc_attr_low,
                              uint8_t desc_attr_high);

// tss 0 特权级栈更新为 pthread 的 0 特权级栈
void tss_update_esp(task_struct *pthread)
{
    // PCB 全部位于内核空间，esp0 的地址在页表最顶端
    tss.esp0 = (uint32_t *)((uint32_t)pthread + PG_SIZE);
}

// 初始化 tss 段描述符，以及特权级 3 下的数据段描述符、代码段描述符
void gdt_init()
{
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_K_STACK;
    // IO 位图的偏移地址 < sizeof(tss). 表示没有位图
    tss.io_base = sizeof(tss);

    // 设置第 4 个段描述符为 tss
    *((gdt_desc *)(GDT_BASE_ADDR + 8 * 4)) = gdt_desc_make(&tss, sizeof(tss) - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);

    // 设置第 5 个段描述符为 dpl 为 3 的代码段描述符
    *((gdt_desc *)(GDT_BASE_ADDR + 8 * 5)) = gdt_desc_make((void *)0, 0xfffff, DESC_CODE_ATTR_LOW_DPL3, DESC_ATTR_HIGH);

    // 设置第 6 个段描述符为 dpl 为 3 的数据段描述符
    *((gdt_desc *)(GDT_BASE_ADDR + 8 * 6)) = gdt_desc_make((void *)0, 0xfffff, DESC_DATA_ATTR_LOW_DPL3, DESC_ATTR_HIGH);

    // 32 位地址经过左移操作后，高位将被丢弃
    // 若原地址高 16 位不是 0 会造成数据错误
    // 故将地址转换成 64 位整型后再进行左移操作

    // lgdt 操作数低 16 位为段界限，高 32 位为段基址
    // 共有 7 个段描述符，段界限单位为字节
    uint64_t gdt_operand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)GDT_BASE_ADDR) << 16);
    asm volatile("lgdt %0" ::"m"(gdt_operand));  // m：表示操作数可以使用任意一种内存形式。
    asm volatile("ltr %w0" ::"r"(SELECTOR_TSS)); // r: eax/ebx/ecx/edx/esi/edi任意一个
}

static gdt_desc gdt_desc_make(void *desc_addr, uint32_t desc_limit,
                              uint8_t desc_attr_low,
                              uint8_t desc_attr_high)
{
    gdt_desc desc;
    uint32_t desc_base = (uint32_t)desc_addr;
    desc.limit_low = desc_limit & 0x0000ffff;
    desc.limit_high_and_attr_high = (uint8_t)(((desc_limit & 0x000f0000) >> 16) +
                                              desc_attr_high);
    desc.attr_low = (uint8_t)desc_attr_low;
    desc.base_low = desc_base & 0x0000ffff;
    desc.base_mid = (desc_base & 0x00ff0000) >> 16;
    desc.base_high = desc_base >> 24;
    return desc;
}