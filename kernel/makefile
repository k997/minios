

all: $(BUILD_DIR)/init.o $(BUILD_DIR)/main.o

$(BUILD_DIR)/init.o:init.c  $(C_LIB)/kernel/print.h \
			$(C_LIB)/kernel/init.h $(C_LIB)/kernel/interrupt.h 

	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<

$(BUILD_DIR)/main.o:main.c $(C_LIB)/kernel/print.h \
			$(C_LIB)/kernel/init.h
	$(CC) $(CFLAGS) $(C_INCLUDE) -o $@  $<