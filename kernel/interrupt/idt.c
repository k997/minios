#include "interrupt.h"
#include "global.h"
#include "stdint.h"
#include "idt.h"

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

// 中断描述符表
static struct gate_desc idt[MAX_INTR_NR];
// interrupt_entry_table ,interrupt.asm中的中断处理程序数组
extern interrupt_handler interrupt_entry_table[MAX_INTR_NR];

static void make_idt_desc(struct gate_desc *gd, uint8_t attr,
                          interrupt_handler handler);
static void idt_desc_init(void);

void idt_init()
{
  idt_desc_init(); //初始化中断描述符

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

// 注册中断描述符
void idt_desc_register(uint8_t intr_ver_nr, uint8_t attr, interrupt_handler handler)
{
  make_idt_desc(&idt[intr_ver_nr], attr, handler);
}
// 创建中断门描述符辅助函数
static void make_idt_desc(struct gate_desc *gd, uint8_t attr,
                          interrupt_handler handler)
{
  gd->func_off_low_word = (uint32_t)handler & 0x0000FFFF;
  gd->selector = SELECTOR_K_CODE;
  gd->dcount = 0;
  gd->attr = attr;
  gd->func_off_high_word = ((uint32_t)handler & 0xFFFF0000) >> 16;
}

// 初始化中断描述符表
static void idt_desc_init(void)
{
  int i;
  for (i = 0; i < MAX_INTR_NR; i++)
  {
    idt_desc_register(i, IDT_DESC_ATTR_DPL0, interrupt_entry_table[i]);
  }
}
