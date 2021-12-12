; 页目录项地址
PAGE_DIR_TABLE_POS equ 0x100000
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b

; 创建页表过程
; 1 页目录表逐字节清零
; 2 创建 0 、 768 页目录项
; 3 创建 1023 页目录项
; 4 创建 768 页目录项对应的页表
; 5 创建 768-1022 页目录项（内核页表项）

setup_page:
; 页目录表逐字节清零（4KB）
    mov ecx,4096
    mov esi,0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir
    
; 创建页目录项(PDE) 
.create_pde:
; 设置第 0 、第 768 个页目录项
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000 ; 第 0 个页表地址为 0x101000 = PAGE_DIR_TABLE_POS + 0x1000
    mov ebx, eax    ; 备份第 0 页表物理地址
    									
    or  eax, PG_US_U | PG_RW_W | PG_P   ; 设置第 0 页表属性，PG_US_U 允许所有特权级访问该内存
										; init进程是内核启动的第一个用户级程序，其位于内核地址空间，未来会在特权级 3 的情况下执行 init
										

; 第 0 个页表映射到低端 1MB 物理内存，第 0 个、第 0xc00 个页目录项均指向第 0 个页表

	; 分页前 loader 和内核都位于低端 1MB 内存，为了使开启分页后 loader 和内核正常运行，必须保证分页后 loader 及内核的虚拟地址仍映射到低端 1MB 内存(0～0xfffff)
	; 分页后 loader 虚拟地址为 0～0xfffff（1MB），其起始地址高 10 位为 0，属于第 0 个页表，第 0 个页表映射到物理地址 0～0xfffff
	; 分页后内核虚拟地址为 0xc0000000～0xc00fffff（1MB），0xc0000000 的高 10 位是 0x300，即十进制的 768，第 768 个页表映射到物理地址 0～0xfffff

	; 为了保证开启分页前后的loader虚拟地址和物理地址指向的数据一致
    mov [PAGE_DIR_TABLE_POS + 0x0 ], eax ; 在页目录表中的第 0 个目录项写入第一个页表的位置及属性
	; 为了保证开启分页前后的内核虚拟地址和物理地址指向的数据一致
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax  ; 在页目录表中的第 768 个目录项写入第一个页表的位置及属性

; 创建第 1023 目录项（最后一个页目录项）
	; 使最后一个目录项指向页目录表自己的地址，便于动态操作页表                                      
    sub eax ,0x1000	; 此时页目录项属性为 PG_US_U | PG_RW_W | PG_P
    mov [PAGE_DIR_TABLE_POS+4092],eax   ; 4096-4=4092 字节


; 创建第 768 内核页表的页表项（PTE）
	; 每个页表可映射的地址空间为 4MB
	; 第 768 页表指向物理地址 0～0xfffff，只用到了 1MB 的地址空间，，其余 3MB 并未分配
	mov ecx,256	; 1MB / 页大小 4KB = 256个页表项，逐项设置属性
	mov esi,0
	mov edx, PG_US_U | PG_RW_W | PG_P	; edx存储页表项，即页的地址及属性
.create_pte:
	mov [ebx+esi*4],edx	; 将页表项填入页表
						; ebx 此时指向第 0 个页表的物理地址
	add edx,4096	;edx内容更新为下一个页表项（下一个页的地址及属性）
	inc esi
	loop .create_pte


; 创建内核其他页表的页目录项（PDE）（第 769～1022 页目录项）
	mov eax,PAGE_DIR_TABLE_POS
	add eax,0x2000			; 第 2 个内核页表
	or	eax, PG_US_U | PG_RW_W | PG_P 
	mov	ebx,PAGE_DIR_TABLE_POS	
	mov	ecx,254				; 范围为第 769～1022 的所有目录项， 1023指向页目录表本身
	mov	esi,769
.create_kernel_pde:
	mov [ebx+esi*4],eax		; ebx 此时指向页目录表物理地址
	inc esi
	add eax,0x1000
	loop .create_kernel_pde
	ret
