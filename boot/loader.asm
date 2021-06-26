%include "boot_conf.asm"
SECTION LOADER  vstart=LOADER_BASE_ADDR
; 用于初始化栈，LOADER_BASE_ADDR以上为loader，以下为栈
LOADER_STACK_TOP equ LOADER_BASE_ADDR


jmp loader_start
%include "memory.asm"
%include "gdt.asm"
loader_start:


; -----------------打印字符串 ----------------
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

;  -----------------获取内存容量----------------

%include "get_memory.asm"


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
; 只能运行在32位环境下
%include "paging.asm"
p_mode_start:
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax

    mov byte [gs:0x120],'P'
    
; 分页
    ; 设置页表
    call setup_page 

    ; 将gdtr内容存储到原来 gdt 的位置，随后存放到内核地址空间
    sgdt [GDT_PTR]  

    ; 修改显存段描述符地址到内核空间
    mov ebx,[GDT_PTR+2] ;ebx为gdt基址， gdtr 前2字节为段界限，其后4字节为gdt基址

    ; 段基址在段描述符中格式较为复杂，不能直接加0xc0000000
    ; 段基址加0xc0000000只影响段基址高4位，故采用or修改段基址24-31位
    or  dword [ebx + 0x18 + 4],0xc0000000   ; + 0x18=24 ：显存段为第3个段描述符，每个段描述符8字节
                                            ; + 4       ：段基址24-31位 位于段描述符高4字节

    ;将GDT映射到内核地址
    add dword [GDT_PTR+2], 0xc0000000

    ; 将栈指针映射到内核地址
    add esp,0xc0000000  

    ; 将页目录表地址赋值给cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开 cr0 的 pg 位（第 31 位）
    mov eax,cr0
    or  eax,0x80000000
    mov cr0,eax

    ; 分页后重新加载gdt 
    lgdt [GDT_PTR]

    mov byte [gs:160],'V'
    jmp $





