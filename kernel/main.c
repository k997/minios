#include "init.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "thread.h"
#include "process.h"
#include "ioqueue.h"
#include "timer.h"
#include "stdio.h"
#include "kstdio.h"
#include "syscall.h"
#include "ide.h"

void main(void)
{
  init_all();
  boot_sector* bs = sys_malloc(sizeof(boot_sector));
  disk_read(&channels[0].device[0],bs,0,1);
  printk("boot sector singnature: %x",bs->singnature);

  interrupt_enable();

  while (1)
    ;
}
