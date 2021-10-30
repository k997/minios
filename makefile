
TOP_DIR := $(shell pwd)
BUILD_DIR = $(TOP_DIR)/bochs/build

BOCHS_DISK = $(TOP_DIR)/bochs/os.img
BOOTLOADER_DIR = $(TOP_DIR)/boot
KERNEL_DIR = $(TOP_DIR)/kernel
C_LIB = $(TOP_DIR)/lib
C_INCLUDE = -I $(TOP_DIR)/lib -I $(TOP_DIR)/lib/kernel

SUB_DIR = $(BOOTLOADER_DIR) $(KERNEL_DIR) $(KERNEL_DIR)/print $(KERNEL_DIR)/interrupt

AS = yasm
CC = gcc
LD = ld

ENTRY_POINT = 0xc0001500
# -c 只编译不链接，否则报错，-fno-builtin不适用c语言内置函数
CFLAGS =  -m32 -fno-builtin -fno-stack-protector  -c
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main
ASFLAGS = -f elf32


export CC  CFLAGS AS ASFLAGS C_LIB C_INCLUDE TOP_DIR BUILD_DIR KERNEL_DIR


# := 变量必须依次出现，防止递归调用
# 此处main 必须在最前面
OBJS := main.o idt.o interrupt_asm.o interrupt_program.o  pic.o print.o init.o  
OBJS := $(foreach OBJ,$(OBJS),$(BUILD_DIR)/$(OBJ))
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@


	

.PHONY : mk_dir hd clean build all sub_dir

mk_dir:
# - 允许失败
	-mkdir -p $(BUILD_DIR)


# 遍历所有makefile
sub_dir:
# 关闭显示
	@for i in $(SUB_DIR); do $(MAKE) -C $$i; done;
# 写入硬盘
hd:
# mbr
	dd if=$(BUILD_DIR)/mbr.bin of=$(BOCHS_DISK) bs=512 count=1 conv=notrunc
# loader
	dd if=$(BUILD_DIR)/loader.bin of=$(BOCHS_DISK) bs=512 count=4 seek=2 conv=notrunc
# kernel
	dd if=$(BUILD_DIR)/kernel.bin of=$(BOCHS_DISK) bs=512 count=200 seek=9 conv=notrunc 
clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin


all: mk_dir sub_dir build hd

	



   
