
enable_sse:
; 开启 sse 
    mov eax, cr0
    and ax, 0xFFFB		;clear coprocessor emulation CR0.EM
    or ax, 0x2			;set coprocessor monitoring  CR0.MP
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9		;set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
    mov cr4, eax
; 测试是否开启 sse 
    mov eax, 0x1
    cpuid
    test edx, 1<<25
    jz .noSSE

    ret


.noSSE
    mov byte [gs:320],'n'
    mov byte [gs:322],'o'
    mov byte [gs:324],'S'
    mov byte [gs:326],'S'
    mov byte [gs:328],'E'
    jmp $