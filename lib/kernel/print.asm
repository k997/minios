; TI, Table Indicator
TI_GDT equ 0 ; TI, Table Indicator——GDT
RPL0 equ 0
SELECTOR_VIDEO	equ (0x0003<<3) + TI_GDT + RPL0

section .data 
put_int_buffer	dq	 0 ; 定义 8 字节缓冲区用于数字到字符的转换
					   ; 以16进制输出32位整数,16进制每个字符表示4位数据
					   ; 32位整数16进制共需8个字符，占8字节

[bits 32]
section .text

; 以16进制输出数字
global put_int
put_int:
	pushad
	mov	 ebp,esp
	mov	 eax,[ebp+36] ; call 的返回地址占 4 字节+pushad 的 8 个寄存器 × 4 字节
	mov	 edx,eax
	mov	 edi,7 		  ; 从后往前在缓冲区中输入字符，缓冲区共8字节
					  ; 指向缓冲区最后一个字节
	mov	 ecx,8		  ; 用于loop循环，需处理的字节个数，每次处理4位，32/4=8次 
	mov	 ebx,put_int_buffer	; ebx作为缓冲区的基址
;将 32 位数字按照十六进制的形式从低位到高位逐个处理
.16base_4bits:		  ; 每 4 位二进制是十六进制数字的 1 位
	and	 edx,0x0000000F	; 解析4位二进制数据
	cmp	 edx,9			
	jg	 .is_A2F		; 数据大于9则转换为A-F
	add  edx,'0'		; 数据小于9即以0-9显示
	jmp	 .store
.is_A2F:
	sub	 edx,10
	add	 edx,'A'
.store:
	mov  [ebx+edi],dl 	; dl为4位数据对应字符的ascii码，存入put_int_buffer
	dec  edi			; 从后往前在缓冲区中输入字符，指向缓冲区的前一个位置
	shr	 eax,4
	mov	 edx,eax
	loop .16base_4bits

.ready_to_print:
	inc	 edi	; store中当edi在存储结束时edi多减了1
				; edi 退减为-1(0xffffffff)，加 1 使其为 0

; 查找第一个非 '0' 的字符
.skip_prefix_0:
	cmp	 edi,8	; 若已经比较第 9 个字符，说明数字全为0, 则直接输出0
	je	 .full0

;找出连续的 0 字符, edi 作为非 0 的最高位字符的偏移
.go_on_skip:
	mov	 cl,[put_int_buffer+edi]	; 缓冲区中取一个字符
	inc	 edi						; 使edi指向下一个字符
	cmp	 cl,'0'						; 判断是否为'0'字符，是则跳过
	je	 .skip_prefix_0
	dec  edi						; edi 在上面的 inc 操作中指向了下一个字符
									; 若当前字符不为'0',要使 edi 减 1 恢复指向当前字符
	jmp	 .put_each_num

.full0:
	mov	 cl,'0'		; 输入的数字为全 0 时，则只打印 0

.put_each_num:
	push ecx		; 此时 cl 中为可打印的字符
	call put_char	; 打印单个字符
	add	 esp,4		; 回收栈空间
	inc	 edi		; 使 edi 指向缓冲区下一个字符
	mov	 cl,[put_int_buffer+edi]
	cmp	 edi,8		; 判断字符是否输出完毕
	jl	 .put_each_num
	popad
	ret


	

global put_str
put_str:
	push ebx			; 备份ebx，ecx
	push ecx
	xor	 ecx,ecx
	mov	 ebx,[esp + 12] ; 从栈中获取待打印的字符串的地址
						; 备份的ebx，ecx及返回地址，共12字节
.go_on
	mov  cl, [ebx]	; 从ebx指向的内存处获取一个字符存入cl
	cmp	 cl,0		; 判断字符串是否结束，'\0' ascii码为0，表示字符结束
	jz	 .str_over	
	push ecx		; put_char参数
	call put_char
	add  esp,4		; 回收参数所占的栈空间
	inc  ebx		; ebx指向下一个字符
	jmp  .go_on
.str_over			; 字符串结束
	pop  ecx
	pop	 ebx
	ret
	
global put_char
put_char:
;1 备份寄存器现场。
	pushad ;push all double，压入所有(8个)双字长的寄存器
		   ;入栈先后顺序是：EAX->ECX->EDX->EBX-> ESP-> EBP->ESI->EDI

; 用户进程特权级为3，当用户程序调用DPL为0的段时，对应的描述符会被CPU指向GDT第0个段描述符
; 此处需重新设置段描述符
	mov ax,SELECTOR_VIDEO
	mov gs,ax
;2 获取光标坐标值
	mov dx,0x03d4	; Address Register
	mov al,0x0e		; 用于提供光标位置的高 8 位
	out dx,al
	mov dx,0x03d5	; Data Register
	in al,dx		; 读写数据端口
	mov ah,al
	
	mov dx, 0x03d4	; Address Register
	mov al,0x0f		; 用于提供光标位置的低 8 位
	out dx,al
	mov dx,0x03d5	; Data Register
	in 	al,dx		; 读写数据端口
	
	mov bx,ax		; 将光标存入 bx
;3 获取待打印的字符
	mov ecx,[esp+36]; pushad 压入 4×8＝32 字节
					; 主调函数 4 字节的返回地址
;4 判断字符是否为控制字符
	cmp cl,0xd		; 回车 CR（carriage_return）
	jz .is_carriage_return
	cmp cl,0xa		; 换行 LF（line_feed）
	jz	.is_line_feed
	cmp cl,0x8		; 退格 backspace
	jz .is_backspace

;5 打印字符
;  打印完成后未超出屏幕范围则设置光标，超出屏幕范围则换行后滚屏
	shl bx,1	; 根据光标计算字符显存中的偏移字节,光标*2
	or cx,0x0700 ; 设置属性，黑底白字0x07
	mov word [gs:bx],cx
	shr bx,1	; 恢复光标
	inc bx		; 下一个光标值
	cmp bx,2000	; 若光标值小于 2000，表示未写到显存的最后，则去设置新的光标值
	jl .set_cursor 
	jmp .is_line_feed ; 若超出屏幕字符数大小（2000）则换行后滚屏


.is_backspace: ; 退格
    dec bx      ; 光标减一
    shl bx,1    ; 根据光标计算字符显存中的偏移字节
                ; 光标左移 1 位等于乘 2，光标左移 1 位等于乘 2
    mov word [gs:bx], 0x0720  	; 对应地址的字符设置为空格 0x20，黑底白字0x07
								; 小端字节序
    jmp .set_cursor
	
.is_line_feed:	; 换行
.is_carriage_return: ; 回车

	; 光标坐标值 bx 对 80 求模，再用坐标值 bx 减去余数就是行首字符的位置
	; 16位无符号除法，被除数高16位位于dx，低16位位于ax，商位于ax，余数位于dx
	xor dx,dx	; 被除数高16位，清零
	mov ax,bx	; 被除数低16位
	mov si,80	; 除数80，每行80个字符
	div si		
	sub bx,dx	; bx减去余数，即当前行行首坐标
	add bx,80	; 指向下一行

	cmp bx,2000 ; 若光标值小于 2000，表示未写到显存的最后，则去设置新的光标值
	jl .set_cursor 
	; 若超出屏幕字符数大小（2000）则滚屏
;	jmp .roll_screen
				
;6 滚屏
; 屏幕每行 80 个字符，共 25 行(0~24)
;（1）将第 1～24 行的内容整块搬到第 0～23 行，也就是把第 0 行的数据覆盖。
;（2）再将第 24 行，也就是最后一行的字符用空格覆盖，这样它看上去是一个新的空行。
;（3）把光标移到第 24 行也就是最后一行行首
.roll_screen:
	cld
	mov ecx,960	; 2000-80=1920 个字符要搬运，共 1920*2=3840 字节
				; movsd 一次搬 4 字节，共 3840/4=960 次
	mov esi,0xc00b80a0 ; 搬运源地址，第 1 行行首
	mov edi,0xc00b8000 ; 搬运目的地址，第 0 行行首
	rep movsd

;将最后一行填充为空白
	mov ebx,3840 ; 最后一行首字符的第一个字节偏移= 1920 * 2
	mov ecx,80	;  共需移动80次，每次2字节
.cls:
	mov word [gs:ebx], 0x0720 ; 空格 0x20，黑底白字0x07
	add ebx,2
	loop .cls
	mov ebx,1920 ;将光标值重置为 1920，最后一行的首字符

;7 设置光标
.set_cursor
	; 高8位
	mov dx,0x03d4 ; 索引寄存器
	mov al,0x0e
	out dx,al
	mov dx,0x03d5 ; 高8位写入数据寄存器
	mov al,bh
	out dx,al
	; 低8位
	mov dx,0x03d4 ; 索引寄存器
	mov al,0x0f
	out dx,al
	mov dx,0x03d5 ; 高8位写入数据寄存器
	mov al,bl
	out dx,al
;8 打印结束
	popad
	ret 






	
