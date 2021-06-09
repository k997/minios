%include "boot_conf.asm"
SECTION LOADER  vstart=LOADER_BASE_ADDR
; 用于初始化栈，LOADER_BASE_ADDR以上为loader，以下为栈
LOADER_STACK_TOP equ LOADER_BASE_ADDR

jmp loader_start

%include "gdt.asm"

loader_start:
    mov ax,0xb800
    mov gs,ax
    
    ;清屏
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0x18f4
    int 0x10

    ; 输出loader
    mov byte [gs:0x00],'l' ; byte代表写入一字节
    mov byte [gs:0x02],'o' ; 注意地址，每个字符占2字节
    mov byte [gs:0x04],'a'
    mov byte [gs:0x06],'d'
    mov byte [gs:0x08],'e'
    mov byte [gs:0x10],'r'

;----------------- 打开 A20 ----------------
    in al,0x92
    or al,0000_0010B
    out 0x92,al 
;----------------- 加载 GDT ----------------

    lgdt	[GDT_PTR]

;----------------- cr0 第 0 位置 1 ---------------- 
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
; 刷新流水线,因为在p_mode_start之前的指令为16位，之后的指令为32位
; p_mode_start后的32位代码已经被以16位指令的形式译码存入流水线
; 必须使用jmp清空指令流水线以保证程序正确执行
    jmp dword SELECTOR_CODE:p_mode_start 

[bits 32]
p_mode_start:
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax

    mov byte [gs:0x120],'P'
    jmp $
