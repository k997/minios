

SRC_DIRS := .
SRCS := $(shell find $(SRC_DIRS)  -name '*.c' -or -name '*.asm')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

MAIN_OBJ := $(filter %main.c.o,$(OBJS))
LD_OBJS := $(filter-out $(MAIN_OBJ),$(OBJS))

# -c 只编译不链接，否则报错，-fno-builtin不适用c语言内置函数  -mno-sse -mno-sse2  禁用浮点运算
CFLAGS =  -m32 -fno-builtin -fno-stack-protector -mno-sse -mno-sse2 -c -MMD -MP
LDFLAGS = -m32 -fno-builtin -nostdlib -T kernel.lds  -Wl,-Map=$(BUILD_DIR)/kernel.map -mgeneral-regs-only -mno-sse
ASFLAGS = -f elf32

LIB_DIR = $(SOURCE_DIR)/lib
C_INCLUDE = -I $(LIB_DIR) -I $(LIB_DIR)/kernel -I $(LIB_DIR)/device -I $(LIB_DIR)/fs


all: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<

$(BUILD_DIR)/%.asm.o: %.asm
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(CC) $(LDFLAGS) $(MAIN_OBJ) $(LD_OBJS) -o $@

-include $(DEPS)