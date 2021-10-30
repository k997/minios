

ASFLAGS =  -I include

all: $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin

$(BUILD_DIR)/mbr.bin: mbr.asm  include/boot_conf.asm include/read_disk.asm
	$(AS) $(ASFLAGS) -o $@ $<


$(BUILD_DIR)/loader.bin: loader.asm $(wildcard include/*.asm )
	
	$(AS) $(ASFLAGS) -o $@ $<



