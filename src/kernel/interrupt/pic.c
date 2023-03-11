#include "io.h"

#define PIC_M_CTRL 0x20   //主片控制端口
#define PIC_M_DATA 0x21   //主片数据端口
#define PIC_S_CTRL 0xa0   //从片控制端口
#define PIC_S_DATA 0xa1   //从片数据端口

//初始化8259A
void pic_init(void)
{
  // ICW1
  outb(PIC_M_CTRL, 0x11); // 第 1 位 SNGL 置为 0，表示级联 8259
                          // 第 3 位的 LTIM 为 0，表示边沿触发
                          // 第 4 位是固定为 1,需要 ICW4
  // ICW2
  outb(PIC_M_DATA, 0x20); // 起始中断向量号为 0x20 (即 32)
                          // 也就是 IR[0-7] 为 0x20 ～ 0x27
                          // 中断向量号 0-31 CPU保留
  // ICW3
  outb(PIC_M_DATA, 0x04); // IR2 接从片，0x4 即 0000 0100b
  // ICW4
  outb(PIC_M_DATA, 0x01); // 8086 CPU 第 0 位固定为 1
                          // 手动 EOI 为 0

  //初始化从片
  // ICW1
  outb(PIC_S_CTRL, 0x11); // 同主片
  // ICW2
  outb(PIC_S_DATA, 0x28); // 起始中断向量号为 0x28
                          // 也就是 IR[8-15]为 0x28 ～ 0x2F
                          // 序号接主片
  // ICW3
  outb(PIC_S_DATA, 0x02); // 设置从片连接到主片的 IR2 ( 0x2 )引脚

  // ICW4
  outb(PIC_S_DATA, 0x01); // 同主片


  /*   
  OCW1 设置 IMR 寄存器
  0 表示不屏蔽, 1 表示屏蔽。
  打开主片上 IR0 时钟中断
  打开主片上 IR2 响应从片中断
   */
  outb(PIC_M_DATA, 0xfa); // 1111 1010b
  // 从片打开 IR14 IR15接受硬盘中断
  outb(PIC_S_DATA, 0x3f); // 0011 1111b
}