#include "debug.h"
#include "print.h"
#include "interrupt.h"


// 出现错误则整个程序关中断打印错误信息，os 停止运行
void panic_spin(char* filename, int line , const char* func,const char* condition){
    interrupt_disable(); 
    put_str("\n\n\n===============fatal error===============\n");
    put_str("filename:");put_str((char*)filename);put_str("\n");
    put_str("line:0x");put_int(line);put_str("\n");
    put_str("function:");put_str((char*)func);put_str("\n");
    put_str("condition:");put_str((char*)condition);put_str("\n");
    while(1);
}
