#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "stdint.h"

#define IDT_DESC_CNT 0x21 // 目前总共支持的中断数
#define PIC_M_CTRL 0x20   //主片控制端口
#define PIC_M_DATA 0x21   //主片数据端口
#define PIC_S_CTRL 0xa0   //从片控制端口
#define PIC_S_DATA 0xa1   //从片数据端口

typedef void *interrupt_handler;
// 中断门描述符
struct gate_desc
{
  // 结构体中位置越偏下的成员，其地址越高
  uint16_t func_off_low_word;  // 中断处理程序0-15位
  uint16_t selector;           // 中断处理程序目标代码段选择子
  uint8_t dcount;              // 未使用，为固定值
  uint8_t attr;                // 属性位，type 4位、S 1位、DPL 2位、P 1位
  uint16_t func_off_high_word; // 中断处理程序16-31位
};

static struct gate_desc idt[IDT_DESC_CNT];
// interrupt_entry_table ,interrupt.asm中的中断处理程序数组
extern interrupt_handler interrupt_entry_table[IDT_DESC_CNT];

static void make_idt_desc(struct gate_desc *gd, uint8_t attr,
                          interrupt_handler handler);
static void idt_desc_init(void);
static void pic_init(void);

void idt_init()
{
  idt_desc_init(); //初始化中断描述符
  pic_init();      //初始化可编程中断控制器

  // lidt 操作数前 16 位是界限 limit，后 32 位是基址
  uint16_t idt_limit = sizeof(idt) - 1;
  // 32 位地址经过左移操作后，高位将被丢弃
  // 若原地址高 16 位不是 0 会造成数据错误
  // 故将 idt 地址转换成 64 位整型后再进行左移操作
  uint64_t idt_addr = ((uint64_t)(uint32_t)idt) << 16;
  uint64_t idt_operand = idt_limit | idt_addr;

  // 加载中断描述符表
  // lidt 48 位内存数据, 此处用64位操作数代替
  // %0 其实是 idt_operand 的地址&idt_operand, lidt 会自动截取前48位做操作数
  asm volatile("lidt %0" ::"m"(idt_operand));
}

// 创建中断门描述符
static void make_idt_desc(struct gate_desc *gd, uint8_t attr,
                          interrupt_handler handler)
{
  gd->func_off_low_word = (uint32_t)handler & 0x0000FFFF;
  gd->selector = SELECTOR_K_CODE;
  gd->attr = attr;
  gd->func_off_high_word = ((uint32_t)handler & 0xFFFF0000) >> 16;
}

//初始化8259A
static void pic_init(void)
{
  // ICW1
  outb(PIC_M_CTRL, 0x11); // 第 1 位 SNGL 置为 0，表示级联级联 8259
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

  // OCW1 设置 IMR 寄存器
  // 0 表示不屏蔽, 1 表示屏蔽。
  // 打开主片上 IR0,也就是目前只接受时钟产生的中断
  outb(PIC_M_DATA, 0xfe); // 0xfe,即 1111 1110b
  // 从片全部屏蔽
  outb(PIC_S_DATA, 0xff);
}

// 初始化中断描述符表
static void idt_desc_init(void)
{
  int i;
  for (i = 0; i < IDT_DESC_CNT; i++)
  {
    make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, interrupt_entry_table[i]);
  }
}
