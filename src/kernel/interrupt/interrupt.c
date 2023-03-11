#include "stdint.h"
#include "interrupt.h"
//IF 位于 eflags 中的第 9 位
#define EFLAGS_IF 0x00000200
// 无法直接获取 eflags 寄存器，push，pop获取 eflags ，g内存约束
// GET_EFLAGS()括号必须紧跟 GET_EFLAGS, 不能有空格
#define GET_EFLAGS(EFALGS_VAR) asm volatile("pushfl;popl %0":"=g"(EFALGS_VAR))

// 获取中断状态
interrupt_status interrupt_get_status(){
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    // 判断 IF 位是否开启
    return (EFLAGS_IF & eflags ) ? INTR_ON : INTR_OFF;
}

// 开中断并返回开中断前的状态
interrupt_status interrupt_enable(){
    interrupt_status old_status = interrupt_get_status();
    // sti 将 eflags 中的 IF 位置 1
    if ( old_status == INTR_OFF  )asm volatile("sti");
    return old_status;
}

// 关中断并返回关中断前的状态
interrupt_status interrupt_disable(){
    interrupt_status old_status = interrupt_get_status();
    // cli 将 eflags 中的 IF 位置 0
    if (old_status == INTR_ON  )asm volatile("cli":::"memory");
    return old_status;
}

// 设置中断状态并返回设置后的状态
interrupt_status interrupt_set_status(interrupt_status status){
    return status & INTR_ON ? interrupt_enable() : interrupt_disable(); 
}