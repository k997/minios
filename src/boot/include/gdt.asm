;-------------- gdt 描述符属性 -------------
;G段界限的粒度
;G=0，段界限单位为字节，G=1，段界限单位为4KB。
DESC_G_4K equ 1_00000000000000000000000b
;D/B 字段，用来指示有效地址（段内偏移地址）及操作数的大小。
;若D=0为16位，D=1为32位
DESC_D_32 equ 1_0000000000000000000000b
; L 字段，用来设置是否是 64 位代码段。
; L=1 表示64位代码段，L=0表示 32 位代码段。
DESC_L equ 0_000000000000000000000b
; AVL对硬件没有专门的用途
DESC_AVL equ 0_00000000000000000000b
; 段界限，用 20 个二进制位表示（低32位0-15，高32位16-19)
; 代码段高32位中第16-19位段界限
DESC_LIMIT_CODE2 equ 1111_0000000000000000b
; 数据段段高32位中第16-19位段界限
DESC_LIMIT_DATA2 equ DESC_LIMIT_CODE2 
; 显存段高32位中第16-19位段界限
DESC_LIMIT_VIDEO2 equ 0000_000000000000000b
; P，Present，指示段是否存在于内存中
DESC_P equ 1_000000000000000b
; DPL，即描述符特权级0-3
DESC_DPL_0 equ 00_0000000000000b
DESC_DPL_1 equ 01_0000000000000b
DESC_DPL_2 equ 10_0000000000000b
DESC_DPL_3 equ 11_0000000000000b
; S为 0 时表示系统段，为 1 时表示数据段，注意此处数据段的含义
DESC_S_CODE equ 1_000000000000b
DESC_S_DATA equ 1_000000000000b
DESC_S_sys 	equ 0_000000000000b 
; TYPE和S结合起来确定描述符的类型
; 代码段x=1,c=0,r=0,a=0 可执行，非一致性，不可读，已访问位 a 清 0
DESC_TYPE_CODE equ 1000_00000000b
;数据段 x=0,e=0,w=1,a=0 不可执行，向上扩展，可写，已访问位 a 清 0
DESC_TYPE_DATA equ 0010_00000000b 
; 段描述符高32位，其中(0x00 << 24)为段基址24-31位，最后的0x00为段基址16-23位
DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00 
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b 




;-------------- 选择子属性 ---------------
; RPL 请求特权级0-1
RPL0 equ 00b
RPL1 equ 01b
RPL2 equ 10b
RPL3 equ 11b
; TI, Table Indicator
TI_GDT equ 000b
TI_LDT equ 100b 





;----------------- gdt ----------------

; dw,dd, dq 是伪指令
; 一个字是 2 字节 
; dw define word ,2 bytes
; dd define double-word，4 bytes
; dq define quadword, 8 bytes
; GDT第0个段不可用
GDT_BASE:
	dd 0x00000000
	dd 0x00000000
;代码段
; 段描述符低32位,0-15位为段界限的低16位,16-31位为段基址的低16位
CODE_DESC:
	dd 0x0000FFFF
	dd DESC_CODE_HIGH4
; 数据段和栈段共同使用一个段描述符
; CPU 不知道此段是用来干什么的，只有用此段的人才知道。
; 栈段向下扩展，是 push 使指栈指针 esp 指向的地址逐渐减小，和段描述符的扩展方向无关，此扩展方向是用来配合段界限的
DATA_STACK_DESC:
	dd 0x0000FFFF
	dd DESC_DATA_HIGH4
; 显存段不采用平坦模型,显存段文本模式起始地址b8000
; b位于显存段描述符高4字节
;limit=(0xbffff-0xb8000)/4k=0x7
VIDEO_DESC: 
	dd 0x80000007
	dd DESC_VIDEO_HIGH4
; gdt 界限
GDT_SIZE 	equ $ - GDT_BASE 
GDT_LIMIT 	equ GDT_SIZE - 1

; 预留60个描述符空位
times 60 dq 0

;以下是 gdt 的指针，前 2 字节是 gdt 界限，后 4 字节是 gdt 起始地址
GDT_PTR:
	dw	GDT_LIMIT
	dd 	GDT_BASE

; 段选择子
; GDT第0个段不可用,所以从1开始
; 相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0 

SELECTOR_CODE 	equ (0x0001<<3) + TI_GDT + RPL0 ; 0x0x0008
SELECTOR_DATA 	equ (0x0002<<3) + TI_GDT + RPL0 ; 0x0x0010
SELECTOR_VIDEO 	equ (0x0003<<3) + TI_GDT + RPL0 ; 0x0x0018


