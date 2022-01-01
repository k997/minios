#ifndef __LIB_KERNEL_GDT_H
#define __LIB_KERNEL_GDT_H
#include "stdint.h"
#include "thread.h"

//-------------- gdt 描述符属性 -------------
// G段界限的粒度
// G=0，段界限单位为字节，G=1，段界限单位为4KB。
#define DESC_G_4K 1
// D/B 字段，用来指示有效地址（段内偏移地址）及操作数的大小。
// 若D=0为16位，D=1为32位
#define DESC_D_32 1
#define DESC_B_16 0 // TSS D/B 字段为 0

#define DESC_L 0   // L=1 表示64位代码段，L=0表示 32 位代码段。
#define DESC_AVL 0 // AVL对硬件没有专门的用途
#define DESC_P 1   // P，Present，指示段是否存在于内存中
// DPL，即描述符特权级0-3
#define DESC_DPL_0 0
#define DESC_DPL_1 1
#define DESC_DPL_2 2
#define DESC_DPL_3 3
// S 为 0 时表示系统段，为 1 时表示数据段，注意此处数据段的含义
#define DESC_S_CODE 1
#define DESC_S_DATA DESC_S_CODE
#define DESC_S_SYS 0
// TYPE 和 S 结合起来确定描述符的类型
// 代码段x=1,c=0,r=0,a=0 可执行，非一致性，不可读，已访问位 a 清 0
#define DESC_TYPE_CODE 8 // 1000b
// 数据段 x=0,e=0,w=1,a=0 不可执行，向上扩展，可写，已访问位 a 清 0
#define DESC_TYPE_DATA 2 // 0010b

// 描述符高 32 位中第 20-23 属性位
#define DESC_ATTR_HIGH ((DESC_G_4K << 7) + \
                        (DESC_D_32 << 6) + \
                        (DESC_L << 5) + (DESC_AVL << 4))

// 用户特权级下代码段描述符高 32 位中第 8-15 属性位
#define DESC_CODE_ATTR_LOW_DPL3 ((DESC_P << 7) +     \
                                 (DESC_DPL_3 << 5) + \
                                 (DESC_S_CODE << 4) + DESC_TYPE_CODE)
// 用户特权级下数据段描述符高 32 位中第 8-15 属性位
#define DESC_DATA_ATTR_LOW_DPL3 ((DESC_P << 7) +     \
                                 (DESC_DPL_3 << 5) + \
                                 (DESC_S_DATA << 4) + DESC_TYPE_DATA)

// -------------- TSS ----------------------
// 80386 TSS 的 TYPE 字段为 10B1，B 位为 0 时，表示任务不繁忙，B 位为 1 时，表示任务繁忙
#define DESC_TYPE_TSS 9 // 1001b，不繁忙
// TSS 描述符高 32 位中第 20-23 属性位
// TSS D/B 字段为 0
#define TSS_ATTR_HIGH ((DESC_G_4K << 7) + \
                       (DESC_B_16 << 6) + \
                       (DESC_L << 5) + (DESC_AVL << 4))

// TSS 描述符高 32 位中第 8-15 属性位
#define TSS_ATTR_LOW ((DESC_P << 7) +     \
                      (DESC_DPL_3 << 5) + \
                      (DESC_S_SYS << 4) + DESC_TYPE_TSS)

#define GDT_BASE_ADDR 0xc00009fd

typedef struct gdt_desc
{
    uint16_t limit_low;               // 低 32 位第 0-15 位，段界限低 16 位
    uint16_t base_low;                // 低 32 位第 16-31 位，段基址低 16 位
    uint8_t base_mid;                 // 高 32 位第 9-7 位，段基址中间 8 位
    uint8_t attr_low;                 // 高 32 位中第 8-15 属性位, TYPE,S,DPL,P
    uint8_t limit_high_and_attr_high; // 高 32 位 16-23 位，其中 16-19 为段界限高 4 位
                                      // 20-23 为属性位 AVL，L，D/B，G
    uint8_t base_high;                // 高 32 位 24-31 位，段基址高 8 位
} gdt_desc;

// tss 0 特权级栈更新为 pthread 的 0 特权级栈
void tss_update_esp(task_struct *pthread);
// 初始化 tss 段描述符，以及特权级 3 下的数据段描述符、代码段描述符
void gdt_init();
#endif