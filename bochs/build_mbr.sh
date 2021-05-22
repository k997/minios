#/bin/sh
yasm ../boot/mbr.asm -o ./mbr.bin
dd if=./mbr.bin of=os.img bs=512 count=1 conv=notrunc
bochs -f bochs.bxrc
