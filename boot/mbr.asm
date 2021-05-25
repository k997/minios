%include "boot_conf.asm"
; include "read_disk.asm" 写在section 上面导致0和0x55aa没有正确被填充
; 且read_disk.asm写在开头会直接被执行，此时eax\cx等寄存器还未准备好 ，read_disk陷入死循环
; boot_conf.asm 中都是伪指令，所以没问题
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
    ; 显存地址为0xb8000
    ; 实模式下访问的方式为 段基址:段内偏移，0xb800*16为0xb8000
    mov ax,0xb800
    mov gs,ax
    
    ; ;清屏
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0x18f4
    int 0x10

    ; 显存位置写入字符串
    ; 每个字符占2字节，低字节为ASCII码，高字节为属性
    
    mov byte [gs:0x00],'m' ; byte代表写入一字节
    mov byte [gs:0x01],0xA4 ; A 表示绿色背景闪烁，4 表示前景色为红色
    mov byte [gs:0x02],'b'
    mov byte [gs:0x03],0xA4 
    mov byte [gs:0x04],'r'
    mov byte [gs:0x05],0xA4 

    mov eax,LOADER_START_SECTOR ; 起始扇区 lba 地址
    mov bx,LOADER_BASE_ADDR ; 写入的地址
    mov cx,1 ; 待读入的扇区数
    call read_disk_m_16 ; 读取程序的起始部分(一个扇区)

    jmp LOADER_BASE_ADDR

; include "read_disk.asm" 写在section 上面导致后面的0和0x55aa没有正确被填充
; 且read_disk.asm写在开头会直接被执行，此时eax\cx等寄存器还未准备好
; boot_conf.asm 中都是伪指令，所以没问题
%include "read_disk.asm" 
    times 510-($-$$) db 0
    db 0x55,0xaa

