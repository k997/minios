SECTION MBR vstart=0x7c00
    ;初始化
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    ;设置栈
    mov sp,0x7c00
    ;清屏
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0x18f4
    int 0x10

    ; 获取光标位置
    mov ah,3 ;功能号
    mov bh,0 ;bh 第0页
    int 0x10

    mov ax,message
    mov bp,ax
    ; cx 为message字符串长度
    mov cx,12



    ;在光标位置处打印
    mov ax,0x1301
    mov bx,0x0002
    int 0x10
    
    jmp $

    message db "hello,loader"
    times 510-($-$$) db 0
    db 0x55,0xaa

