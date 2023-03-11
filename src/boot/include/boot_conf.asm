; boot配置文件

; loader加载的内存地址
LOADER_BASE_ADDR equ 0x900
; loader所在的逻辑扇区地址，即 LBA 地址
LOADER_START_SECTOR equ 0x2
; loader占用的扇区数
LOADER_SECTOR equ 0x4