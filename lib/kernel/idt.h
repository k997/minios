#ifndef __KERNEL_IDT_H
#define __KERNEL_IDT_H
// idt 描述符属性
#define IDT_DESC_P 1
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3
#define IDT_DESC_32_TYPE 0XE //32 位的门
#define IDT_DESC_16_TYPE 0X6 //16 位的门，不会用到

#define IDT_DESC_ATTR_DPL0  \
    ((IDT_DESC_P << 7) +    \
     (IDT_DESC_DPL0 << 5) + \
     IDT_DESC_32_TYPE)

#define IDT_DESC_ATTR_DPL3  \
    ((IDT_DESC_P << 7) +    \
     (IDT_DESC_DPL3 << 5) + \
     IDT_DESC_32_TYPE)

#endif