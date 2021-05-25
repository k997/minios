;m_16 16位模式下读硬盘
read_disk_m_16:


;第 0 步:备份寄存器
    ; 利用寄存器ax、bx、cx传递参数
    ; eax=LBA 扇区号
    ; bx=将数据写入的内存地址
    ; cx=读入的扇区数
    mov esi,eax ; 备份eax，al被用于 out 指令
    mov di,cx   ; 备份cx，cx的值会在读取数据时用到


;第 1 步:设置要读取的扇区数
    ; 指定操作的端口号，out可用dx或立即数为端口
    ; 0x1f2为Sector count寄存器
    mov dx,0x1f2
    ; cl的值由主调函数传入，硬盘Sector count寄存器为8位，因此用cl
    mov al,cl
    ; 将al的值入dx指向的端口
    out dx,al       



;第 2 步:将指定LBA地址以及device寄存器
    
    ; LBA地址 7～0 位写入端口 0x1f3
    mov dx,0x1f3
    ; 恢复eax，即待读入的扇区起始地址
    mov eax,esi 
    out dx,al

    ; LBA 地址 15～8 位写入端口 0x1f4
    mov dx,0x1f4
    ; eax右移8位即 LBA 地址的 15～8 位
    mov cl,8
    shr eax,cl
    out dx,al

    ; LBA 地址 23～16 位写入端口 0x1f5
    mov dx,0x1f5
    shr eax,cl
    out dx,al

    ; device寄存器 端口为 0x1f6
    mov dx,0x1f6
    ; 拼凑 device 寄存器的值
    ; device 低4位，LBA 第 24～27 位
    shr eax,cl
    and al,0x0f ; 0000 1111b
    ; device 高4位， 1110表示 lba 模式
    or al,0xe0  ; 1110 0000b

    out dx,al



;第 3 步:向command寄存器写入读命令,0x20
;       0x1f7 端口此时为command寄存器
    mov dx,0x1f7
    mov al,0x20
    out dx,al



;第 4 步:检测硬盘状态
;       0x1f7 端口此时为status寄存器
.not_ready:

    nop ;相当于sleep，减少对硬盘的干扰
    in al,dx
    ; 第 4 位为 1 表示硬盘控制器已准备好数据传输
    and al,0x08 ; 0000 1000b
    ; cmp 指令相当于做减法运算并设置zf的值
    cmp al,0x08 ; 0000 1000b
    jnz .not_ready


;第 5 步:从 0x1f0 端口读数据
;data寄存器为16位，计算读取512字节需要循环的读取data寄存器的次数
; mov dx,0x1f0 ;此处不能提前写入端口号，因为后面还要用到dx
    mov ax,di
    ; data寄存器为16位，每次 in 操作只读入 2 字节
    ; 每扇区512字节，读入的数据总量为 扇区数*512 字节
    ; 共执行 扇区数*512/2 次 in 指令
    ; 即 扇区数*256 次 in 指令
    mov dx,256
    ; mul 乘法操作，mul 16位模式下有两种情况
    ; mul 操作数是 8 位，被乘数为是 al 寄存器的值，乘积为 16 位，位于 ax 寄存器
    ; mul 操作数是 16 位，被乘数为是 ax 寄存器的值，乘积的低 16 位存入 ax，高 16 位存入 dx
    mul dx
    ; 我们知道这两个乘数 ax 的值和 dx 的值都不大
    ; 因此只将积的低 16 位移入 cx 作为循环读取的次数
    mov cx,ax


;0x1f0为数据寄存器
    mov dx,0x1f0
    
    
; 在实模式下偏移地址为 16 位，所以用 bx 只会访问到 0～FFFFh 的偏移
; 待写入的地址超过 bx 的范围时，从硬盘上读出的数据会把 0x0000～0xffff 的覆盖
; 所以此处加载的程序不能超过 64KB
.go_on_read:
    in ax,dx
    mov [bx],ax
    ; bx =将数据写入的内存地址
    ; 写入 2 字节，因此 bx + 2
    add bx,2
    loop .go_on_read
    ret