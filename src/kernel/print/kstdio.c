#include "kstdio.h"
#include "print.h"

/* 内核打印函数 */
void printk(const char *format, ...)
{
    va_list args;
    va_start(args, format); // args = (void*)&format;
    char buf[STDIO_BUF_SIZE] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    put_str(buf);
}