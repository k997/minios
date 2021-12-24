[bits 32]
section .text
global switch_to
switch_to:
; 备份当前线程环境
; ABI 要求备份 esi、edi、ebx、ebp 
    push esi
    push edi
    push ebx
    push ebp
; PCB 起始地址即 self_kstack
    mov eax,[esp+20]  ; 获取 current_thread
    mov [eax],esp     ; 等价于 current_thread->self_kstack = esp

; 恢复下一个线程环境
    mov eax,[esp+24]  ; 获取 next_thread
    mov esp,[eax]     ; 等价于 esp = next_thread->self_kstack
    pop ebp
    pop ebx
    pop edi
    pop esi

    ret               ; 未由中断进入，第一次执行时会返回到 kernel_thread
