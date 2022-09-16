#include "global.h"
#include "memory.h"
#include "gdt.h"
#include "string.h"
#include "interrupt.h"
#include "debug.h"
#include "process.h"
#include "print.h"
static void run_program(void *program);
static void process_vaddr_init(pool *vaddr);
static uint32_t *process_page_dir_create(void);
static void process_page_dir_activate(task_struct *pthread);
// 创建进程
task_struct *process_create(char *name,void *program)
{
    task_struct *pthread = thread_create(name, default_prio, run_program, program);
    pthread->pgdir = process_page_dir_create();
    process_vaddr_init(&pthread->vaddr);
    mem_bin_init(pthread->mb);
    return pthread;
}

// 将进程加入线程就绪队列
void process_start(task_struct *pthread)
{
    thread_start(pthread);
}

// 激活线程或进程页表，更新 tss 中 esp0 为进程特权级 0 栈
void process_activate(task_struct *pthread)
{
    // 激活内核页表或进程页表
    process_page_dir_activate(pthread);
    // 若没有页表，则是内核线程，已经在 0 特权级，无需更新 tss
    if (pthread->pgdir)
        tss_update_esp(pthread);
}

// 激活内核页表或进程页表
static void process_page_dir_activate(task_struct *pthread)
{
    uint32_t pgdir_phy_addr = 0x100000; // 内核页表地址
    if (pthread->pgdir != NULL)
        // 若为进程，则替换为进程自身页表
        pgdir_phy_addr = vaddr2paddr((uint32_t)pthread->pgdir);
    // 更新 cr3 寄存器
    asm volatile("movl %0,%%cr3" ::"r"(pgdir_phy_addr)
                 : "memory");
}

// 创建进程页表
static uint32_t *process_page_dir_create(void)
{
    // 页目录项和 uint32_t 都是四字节, 方便操作页表
    // 页表都位于内核
    uint32_t *page_dir_vaddr = kernel_page_alloc(1);
    // 页面分配失败
    if (page_dir_vaddr == NULL)
        return NULL;
    // 复制内核页目录项到进程页表
    // 0x300 第 768 个页目录项
    // 每个页目录项占 4 字节
    // 1024 - 768 = 256 个页目录项,256 * 4 = 1024 字节
    // 0xfffff000 指向页目录表本身
    memcpy((void *)((uint32_t)page_dir_vaddr + 0x300 * 4),
           (void *)(0xfffff000 + 0x300 * 4), 1024);

    uint32_t page_dir_phyaddr = vaddr2paddr((uint32_t)page_dir_vaddr);
    // 修改页目录表最后一项为页目录表本身
    page_dir_vaddr[1023] = page_dir_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

// 初始化进程地址空间
static void process_vaddr_init(pool *vaddr)
{
    vaddr->addr_start = USER_VADDR_START;
    // 8 是 byte 到 bit 换算
    uint32_t btmp_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    vaddr->btmp.bits = kernel_page_alloc(btmp_pg_cnt);
    vaddr->btmp.byte_length = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&vaddr->btmp);
}

// 初始化中断栈，从内核特权级返回用户特权级，启动用户进程
static void run_program(void *program)
{
    task_struct *current_thread = thread_running();
    // 选择中断栈
    interrupt_stack *istack = (interrupt_stack *)(current_thread->self_kstack + sizeof(thread_stack));
    istack->edi = istack->esi = istack->ebp = istack->esp_dummy = 0;
    istack->ebx = istack->edx = istack->ecx = istack->eax = 0;
    istack->gs = 0;
    istack->ds = istack->es = istack->fs = SELECTOR_U_DATA;
    istack->eip = program;
    // CS 请求特权级为 3
    istack->cs = SELECTOR_U_CODE;
    // EFLAGS_IOPL_0 禁止用户 IO 操作
    // EFLAGS_IF_1 使进程能继续响应其他中断
    istack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    // 申请用户地址空间最高处的一页，并设置内存最高处地址为栈顶
    istack->esp = (void *)((uint32_t)user_page_alloc_from(USER_STACK3_VADDR, 1) + PG_SIZE);
    istack->ss = SELECTOR_U_DATA;
    asm volatile("movl %0,%%esp;jmp interrupt_exit" ::"g"(istack)
                 : "memory");
}