[bits 32]
; 有的异常 CPU 会自动往栈中压入错误码，有的不会
; 若在相关的异常中 CPU 已经自动压入了错误码，为保持栈中格式统一，不做操作
%define ERROR_CODE nop  
; 若在相关的异常中 CPU 没有压入错误码， 为了统一栈中格式，手工压入一个 0
%define ZERO push 0     
                        

extern interrupt_program_table

section .data
global interrupt_entry_table ; 定义中断处理程序入口数组
interrupt_entry_table:

; 宏名称为 VECTOR 参数个数为2
; 第一个参数为中断向量号，第二个参数为指令操作，压入空指令或中断错误码
%macro VECTOR 2 
section .text
int_%1_entry:   ; 此处 %1 被替换为为宏的第一个参数，即中断向量号

    %2          ; 空指令或压入中断错误码
    
    ; 保存上下文环境
    push ds
    push es
    push fs
    push gs
    pushad

    mov  al,0x20; 8259A 设置为手动结束中断，所以此处发送中断结束命令 EOI
                ; OCW2 中第 5 位是 EOI 位，此位为 1，其余位全为 0，所以是 0x20

    out  0xa0,al; 向从片发送 EOI
    out  0x20,al; 向主片发送 EOI

    push %1     ;不管 interrupt_program_table 中的目标程序是否需要参数
                ;都一律压入中断向量号，方便调试
    call [interrupt_program_table + %1 * 8 ]    ; 调用 interrupt_program_table 中的 C 版本中断处理函数
                                                ; 32 位模式下所有指针均占 4 字节
                                                ; 中断处理函数void* 4 字节，处理函数名称char* 占 4 字节
    jmp interrupt_exit

section .data           
    dd  int_%1_entry    ; 存储各个中断入口程序的地址
                        ; 形成 intr_entry_table 数组
                        ; 编译器会将属性相同的 section 合并到同一个大的 segment 中
%endmacro

section .text
global interrupt_exit
interrupt_exit:
    add esp, 4 ; 跳过中断向量号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp,4 ; 跳过 error_code
    iretd


; python print(f"VECTOR { hex(i) },ZERO")
VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO	;时钟中断对应的入口