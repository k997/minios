#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "stdint.h"

#define STDIO_BUF_SIZE 1024

typedef void *va_list;
/* va_list 类型为空指针 */
#define va_start(ap, v) ap = (va_list)&v
/*
    经 va_start 初始化后 ap 已经指向了栈中可变参数中的第 1 个参数
    32 位栈的存储单元是 4 字节，(ap+=4)将指向下一个参数在栈中的地址
    而后将其强制转换成 type 型指针(type*)，最后再用*号取值
    即*((t*)(ap += 4))是下一个参数的值。
*/
#define va_arg(ap, type) *((type *)(ap += 4))
/* 回收指针 ap */
#define va_end(ap) ap = NULL


char *itoa(int value, char *str, uint8_t base);
uint32_t sprintf(char *buf, const char *format, ...);
uint32_t vsprintf(char *str, const char *format, va_list ap);

#endif