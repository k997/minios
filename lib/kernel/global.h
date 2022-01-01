#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#include "stdint.h"
//-------------- 选择子属性 ---------------
// RPL 请求特权级0-1
#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3
// TI, Table Indicator
#define TI_GDT 0
#define TI_LDT 1

//-------------- 段选择子属性 -------------

// 内核段 特权级0
#define SELECTOR_K_CODE ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_GS ((3 << 3) + (TI_GDT << 2) + RPL0)

// 第 4 个描述符为 TSS
#define SELECTOR_TSS ((4 << 3) + (TI_GDT << 2) + RPL0)

// 用户段 特权级3
#define SELECTOR_U_CODE ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK SELECTOR_U_DATA

#endif