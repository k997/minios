#/bin/sh
BOOT="../boot"
OUTPUT="./build"
BOCHS_DIR="."
BOCHS_DISK="$BOCHS_DIR/os.img"


yasm $BOOT/mbr.asm -I $BOOT/include/ -o $OUTPUT/mbr.bin
yasm $BOOT/loader.asm -I $BOOT/include/ -o $OUTPUT/loader.bin

dd if=$OUTPUT/mbr.bin of=$BOCHS_DISK bs=512 count=1 conv=notrunc
dd if=$OUTPUT/loader.bin of=$BOCHS_DISK bs=512 count=1 seek=2 conv=notrunc

bochs -f $BOCHS_DIR/bochs.bxrc
