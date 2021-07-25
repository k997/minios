PT_NULL equ 0
KERNEL_ENTRY_POINT equ  0xc0001500
KERNEL_BIN_BASE_ADDR equ 0x70000
kernel_init:
	xor eax,eax
	xor ebx,ebx
	xor ecx,ecx
	xor edx,edx
	
	mov dx, [ KERNEL_BIN_BASE_ADDR + 42 ]     ;  e_phentsize，表示每个 program header 大小
	mov ebx,[ KERNEL_BIN_BASE_ADDR + 28 ]    ;  e_phoff， program header 在文件中的偏移
    add ebx,KERNEL_BIN_BASE_ADDR            ;  program header 地址 = KERNEL_BIN_BASE_ADDR + e_phoff
    mov cx, [ KERNEL_BIN_BASE_ADDR + 44 ]   ;  e_phnum，表示有几个 program header

.each_segment:
    cmp byte [ ebx + 0 ], PT_NULL
    je .PTNULL
; mem_cpy通过栈传递参数 mem_cpy（dst，src，size）
; 从右向左开始入栈
; mem_cpy -> size 复制大小
    push dword  [ ebx + 16 ]      ; p_filesz, 本段在文件中的大小, 占4字节
    mov  eax,   [ ebx + 4 ]       ; p_offset, 本段在文件内的起始偏移字节
    add  eax,   KERNEL_BIN_BASE_ADDR ; 本段在文件内起始地址= BASE_ADDR + p_offset
; mem_cpy -> src 源地址
    push eax
; mem_cpy -> dst 目的地址
    push dword  [ ebx + 8 ]         ; p_vaddr

    call mem_cpy
; 清除参数
    add esp,12


.PTNULL
    add ebx, edx    ; ebx指向下一个段（program header）
                    ; edx 为 program header 大小
    loop .each_segment

; kernel_init 执行完成，返回
    ret

mem_cpy:
    cld ; clean direction，将 eflags 寄存器中的方向标志位 DF 置为 0
        ; rep 配合[e]si 和[e]di搬运字符时，源地址和目的地址逐渐增大

        ; std. set direction作用相反
; 保存环境
    push ebp    
    mov  ebp,esp
    push ecx    ;  loop .each_segment使用cx
; 自左向右取出参数
    mov edi,[ ebp + 8  ]    ; dst
    mov esi,[ ebp + 16 ]    ; src
    mov ecx,[ ebp + 24 ]    ; size
    rep movsb               ; ecx为0则rep停止
                            ; movsb: [e]si和[e]di 自动加 1, 即逐字节复制

; 恢复环境
    pop ecx
    pop ebp
    ret


