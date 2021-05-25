%include "boot_conf.asm"
SECTION LOADER  vstart=LOADER_BASE_ADDR
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

    jmp $