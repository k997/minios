#/bin/sh
BOOT="../boot"
KERNEL="../kernel"
LIB="../lib"
OUTPUT="./build"
BOCHS_DIR="."
BOCHS_DISK="$BOCHS_DIR/os.img"

# mbr
yasm $BOOT/mbr.asm -I $BOOT/include/ -o $OUTPUT/mbr.bin && \
dd if=$OUTPUT/mbr.bin of=$BOCHS_DISK bs=512 count=1 conv=notrunc
# loader
yasm $BOOT/loader.asm -I $BOOT/include/ -o $OUTPUT/loader.bin &&\
dd if=$OUTPUT/loader.bin of=$BOCHS_DISK bs=512 count=4 seek=2 conv=notrunc

# kernel

yasm -f elf32 -o $OUTPUT/print.o $LIB/kernel/print.asm && \
gcc -m32 -I $LIB -I $LIB/kernel -c -o $OUTPUT/main.o $KERNEL/main.c && \
ld -m elf_i386 -Ttext 0xc0001500 -e main -o $OUTPUT/kernel.bin \
   $OUTPUT/main.o $OUTPUT/print.o       && \
dd if=$OUTPUT/kernel.bin of=os.img bs=512 count=200 seek=9 conv=notrunc 

# run bochs
bochs -f $BOCHS_DIR/bochs.bxrc
