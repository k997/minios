TOP_DIR := $(shell pwd)
BOCHS_DIR = $(TOP_DIR)
BUILD_DIR = $(TOP_DIR)/build
BOOTLOADER_DIR = $(TOP_DIR)/boot
KERNEL_DIR = $(TOP_DIR)/kernel

SUB_DIR = $(BOOTLOADER_DIR) $(KERNEL_DIR) 


BOCHS_DISK = $(BOCHS_DIR)/os.img

AS = yasm
CC = gcc
LD = ld


export CC AS TOP_DIR BUILD_DIR KERNEL_DIR

.PHONY : mk_dir hd clean build all run

mk_dir:
# - 允许失败
	-mkdir -p $(BOCHS_DIR)
	-mkdir -p $(BUILD_DIR)

build:
# 遍历所有makefile
# 关闭显示
	@for i in $(SUB_DIR); do $(MAKE) -C $$i; done;

hd:
# 写入硬盘
# mbr
	dd if=$(BUILD_DIR)/mbr.bin of=$(BOCHS_DISK) bs=512 count=1 conv=notrunc
# loader
	dd if=$(BUILD_DIR)/loader.bin of=$(BOCHS_DISK) bs=512 count=4 seek=2 conv=notrunc
# kernel
	dd if=$(BUILD_DIR)/kernel.bin of=$(BOCHS_DISK) bs=512 count=200 seek=9 conv=notrunc 
clean:
	cd $(BUILD_DIR) && rm -rf ./*

bochs_run:
	bochs -f bochs.bxrc
run: all bochs_run

all: mk_dir build hd

rebuild: clean all



   
