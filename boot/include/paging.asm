; 页目录项地址
PAGE_DIR_TABLE_POS equ 0x100000
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b




setup_page:
    mov ecx,4096
    mov esi,0
; 页目录表4KB逐字节清零
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir
    
; 创建页目录项(PDE) 
.create_pde:
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000 ; 设置第一个页表位置（第一个二级页表）
    				; 0x1000=4096,跳过页目录表（一级页表）
    mov ebx, eax    ; 备份第一个页表位置
    									
    or  eax, PG_US_U | PG_RW_W | PG_P   ; 设置第一个页表属性
										; init进程是内核启动的第一个用户级程序，其位于内核地址空间，未来会在特权级 3 的情况下执行 init
										
; 分页前loader和内核都位于低端1MB内存，为了保证开启分页后的loader和内核物理地址和虚拟地址一致，以便loader和内核正常运行，需将低端1MB内存同时加载到第1个页表（用户地址空间0～0x3fffff（4MB），loader位于此处）以及第 768 个页表（内核地址空间0xc0000000～0xc03fffff（4MB），内核位于此处。0xc0000000的高10位是 0x300，即十进制的 768)

; 为了保证开启分页前后的loader物理地址和虚拟地址一致
    mov [PAGE_DIR_TABLE_POS + 0x0 ], eax ; 在页目录表中的第 1 个目录项写入第一个页表的位置及属性
; 为了保证开启分页前后的内核物理地址和虚拟地址一致    
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax  ; 在页目录表中的第 768 个目录项写入第一个页表的位置及属性
                                            
    sub eax ,0x1000
    mov [PAGE_DIR_TABLE_POS+4092],eax   ; 使最后一个目录项指向页目录表自己的地址，便于动态操作页表
    									; 4096-4=4092

; 创建页表项PTE
; PDE中第0个页目录项对应的页表
; 一个页表表示的内存容量是 4MB，目前只用到1MB空间，只将低端1MB内存写入第一个页表
	mov ecx,256	; 1M 低端内存 / 每页大小 4k = 256个页表项，逐项设置属性
	mov esi,0
	mov edx, PG_US_U | PG_RW_W | PG_P	; edx存储页表项，即页的地址及属性
.create_pte:
	mov [ebx+esi*4],edx	; 将页表项填入页表
	add edx,4096	;edx内容更新为下一个页表项（下一个页的地址及属性）
	inc esi
	loop .create_pte

;创建内核页表PDE
;除了内核第0个页表外的其他页表
	mov eax,PAGE_DIR_TABLE_POS
	add eax,0x2000			; 创建内核第二个页表
	or	eax, PG_US_U | PG_RW_W | PG_P 
	mov	ebx,PAGE_DIR_TABLE_POS	
	mov	ecx,254				; 范围为第 769～1022 的所有目录项数， 1023指向页目录表本身
	mov	esi,769
.create_kernel_pde:
	mov [ebx+esi*4],eax
	inc esi
	add eax,0x1000
	loop .create_kernel_pde
	ret
