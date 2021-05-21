SECTION MBR vstart=0x7c00
    ;初始化
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    ;设置栈
    mov sp,0x7c00

    ; 设置显存起始地址
    ; 不能直接写入gs，通过ax操作
    mov ax,0xb800
    mov gs,ax
    
    ;清屏
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0x18f4
    int 0x10

    ; 显存位置写入字符串
    ; 每个字符占2字节，低字节为ASCII码，高字节为属性
    
    mov byte [gs:0x00],'m'
    mov byte [gs:0x01],0xA4 ; A 表示绿色背景闪烁，4 表示前景色为红色
    mov byte [gs:0x02],'b'
    mov byte [gs:0x04],'r'
    jmp $

    message db "hello,loader"
    times 510-($-$$) db 0
    db 0x55,0xaa

