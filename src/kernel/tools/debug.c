#include "debug.h"
#include "kstdio.h"
#include "interrupt.h"

// 出现错误则整个程序关中断打印错误信息，os 停止运行
void panic_spin(char *filename, int line, const char *func, const char *condition)
{
    interrupt_disable();
    printk("\n\n\n===============fatal error===============\n"
           "filename: %s\n"
           "line: %d\n"
           "function: %s\n"
           "condition: %s\n",
           filename, line, func, condition);
    while (1)
        ;
}
