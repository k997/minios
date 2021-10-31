

SRC_DIRS := .
SRCS := $(shell find $(SRC_DIRS)  -name '*.c' -or -name '*.asm')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)


ENTRY_POINT = 0xc0001500
# -c 只编译不链接，否则报错，-fno-builtin不适用c语言内置函数
CFLAGS =  -m32 -fno-builtin -fno-stack-protector  -c -MMD -MP
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main
ASFLAGS = -f elf32

C_INCLUDE = -I $(TOP_DIR)/lib -I $(TOP_DIR)/lib/kernel

all: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<

$(BUILD_DIR)/%.asm.o: %.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

-include $(DEPS)