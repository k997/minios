total_mem_bytes	dd	0   ; 物理内存容量，单位为byte ,地址为0x0903

ards_buf	times 244 db 0 	;ards缓冲区
ards_nr		dw 0 			;ards结构体数量
