
; 0xE820 
    xor ebx,ebx
    mov edx,0x534d4150
    mov di, ards_buf
.e820_mem_get_loop:
    mov eax,0xE820 		;执行 int 0x15 后，eax 值变为 0x534d4150
                    	;所以每次执行 int 前都要更新为子功能号
    mov ecx, 20			;执行 int 0x15 后 ecx 为写入ARDS缓冲区的字节数
    int 0x15
    jc .e820_failed_try_e801 ; cf 为1， 0xe280获取内存容量失败，尝试0xe801
    add di,cx			; 使 di 增加 20 字节指向缓冲区中新的 ARDS 结构位置
    inc word [ards_nr]	;ards结构体数量+1
    cmp ebx,0			; ebx 为 0表示 ards 全部返回
    jnz .e820_mem_get_loop
    
    ; 查找ards中最大的块
    mov ecx, [ards_nr] 	; 配合loop，遍历所有内存块
    mov ebx, ards_buf	
    xor edx,edx 		; edx记录最大的内存块
.find_max_mem_area:
	mov eax,[ebx]       ; 内存基址低32位
	add eax,[ebx+8]     ; 内存长度低32位
	add ebx,20          ; 指向缓冲区中下一个 ARDS 结构
	cmp edx,eax         ;冒泡排序,找出最大,edx 寄存器始终是最大的内存容量
	jge .next_ards 	    ; edx >= eax ，比较下一个ards
	mov edx,eax		    ; edx < eax, 更新edx为最大的ards
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok
	

; 0xE801
.e820_failed_try_e801
	mov ax, 0xe801
	int 0x15
	jc .e801_failed_try88 ; cf为1， 0xe801获取内存容量失败，尝试0x88
	
	; 低15MB内存转换为以 byte 为单位
	; ax 和 cx 以 KB 为单位
	mov cx,0x400	; cx 和 ax 值一样,均为15MB 以下内存容量
					; ax保留原值， cx 用作乘数，0x400=1024,1KB
	mul cx			; ax*1024
	shl edx,16		; mul结果为 32 位，高 16 在 dx 寄存器，低 16 位在 ax 寄存器
	and eax,0x0000FFFF
	or	edx,eax		; 拼凑完整乘积
	add	edx,0x10000	; ax 只是 15MB，故要加 1MB 
	mov esi,edx		; 先把低 15MB 的内存容量存入 esi 寄存器备份
	
	; 16MB 以上的内存转换为 byte 为单位
	; bx 和 dx 以 64KB 为单位
	xor eax,eax
	mov ax,bx
	mov ecx, 0x10000; 0x10000=65536, 64KB
	mul ecx			; ax*65536
					; mul结果为 64 位，高 32 在 edx 寄存器，低 32 位在 eax 寄存器
					; 但e801最多检测到4G内存，因此一定在eax
	add esi,eax		; 两部分容量相加
	mov edx,esi		; 总容量存入edx
	jmp	.mem_get_ok

; 0x88
.e801_failed_try88:
	mov ah,0x88	;功能号
	int 0x15
	jc .error_hlt	;cf为1， 获取内存容量失败，跳转到错误处理
	; ax 以 KB 为单位
	and eax,0x0000FFFF	; ax有效，eax高位无效，清零
	mov cx,0x400		; 0x400=1024,1KB
	mul cx				; mul 低16位 ax ，mul 高16位 dx
	shl	edx,16			; 高16位左移
    or 	edx,eax			; 拼接高16位和低16位
    add edx,0x100000	; 0x88只返回1MB以上内存容量，须加 1MB




.error_hlt:
    hlt

.mem_get_ok:
	mov [total_mem_bytes],edx