all: $(BUILD_DIR)/interrupt_asm.o $(BUILD_DIR)/pic.o $(BUILD_DIR)/interrupt_program.o $(BUILD_DIR)/idt.o

$(BUILD_DIR)/interrupt_asm.o: interrupt.asm
	$(AS) $(ASFLAGS) -o $@ $<


$(BUILD_DIR)/pic.o:pic.c $(C_LIB)/kernel/io.h
	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<

$(BUILD_DIR)/interrupt_program.o:interrupt_program.c   \
			$(C_LIB)/stdint.h $(C_LIB)/kernel/print.h $(C_LIB)/kernel/interrupt.h 
	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<

$(BUILD_DIR)/idt.o:idt.c   \
			 $(C_LIB)/stdint.h $(C_LIB)/kernel/interrupt.h $(C_LIB)/kernel/global.h 
	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<
