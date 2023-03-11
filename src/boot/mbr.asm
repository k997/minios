%include "boot_conf.asm"
SECTION MBR vstart=0x7c00
    ;初始化
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    ;设置栈
    mov sp,0x7c00

    ; 通过gs寄存器设置显存起始地址
    ; 但无法直接将立即数写入gs，因此通过ax操作
    ; 显存地址为0xb8000
    ; 实模式下地址访问的方式为 段基址:段内偏移，
    ; 0xb800*16=0xb8000，因此将0xb800写入gs
    mov ax,0xb800
    mov gs,ax
    
    ; ;清屏
    ;INT 0x10 功能号:0x06 功能描述:上卷窗口
    ;AH 功能号= 0x06 
    ;AL = 上卷的行数(如果为 0,表示全部)
    ;BH = 上卷行属性
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0 		; 左上角: (0, 0)
    mov dx, 0x18f4	; 右下角: (80,25)
                    ; VGA 文本模式中,一行只能容纳 80 个字符,共 25 行
                    ; 下标从 0 开始,所以 0x18=24,0x4f=79 
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
    mov cx,LOADER_SECTOR ; 待读入的扇区数
    call read_disk_m_16 ; 读取程序的起始部分(一个扇区)

    jmp LOADER_BASE_ADDR

; include "read_disk.asm" 写在section 上面导致0和0x55aa没有正确被填充
; 且read_disk.asm写在开头会直接被执行，此时eax\cx等寄存器还未准备好 ，read_disk陷入死循环
; boot_conf.asm 中都是伪指令，所以没问题
%include "read_disk.asm" 
	; $和$$表示当前行和本section 的地址
	; 用0填充第一个扇区

    times 510-($-$$) db 0
    ; 魔数0x55aa，表示该扇区存在引导程序（MBR）
    db 0x55,0xaa

