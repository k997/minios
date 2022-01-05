#include "memory.h"
#include "bitmap.h"
#include "string.h"
#include "debug.h"
#include "sync.h"
#include "thread.h"
#include "stdint.h"
/*
    页面分配流程
    virtual_page_alloc 分配虚拟内存,返回虚拟内存地址
    physical_page_alloc 分配物理页面,返回物理内存地址
    map_vaddr2phyaddr 将负责填充页表,虚拟内存映射到物理内存

    pool_page_alloc 为从不同内存池申请页面的辅助函数
    pde_ptr pte_ptr 用于定位页目录项及页表项

*/
// 页目录项索引，虚拟地址高 10 位
#define PDE_INDEX(addr) ((addr & 0xffc00000) >> 22)
// 页表项索引，虚拟地址中间 10 位
#define PTE_INDEX(addr) ((addr & 0x003ff000) >> 12)

// 位图也位于低端 1MB 内存之中，低端 1MB 物理内存不在内存管理单元的管理范围之内
#define MEM_BITMAP_BASE 0xc009a000

/*
    内核堆起始地址
    0xc0000000～0xc00fffff 映射到了物理地址 0x00000000～0x000fffff（低端 1MB 的内存）
    为了让虚拟地址连续，将堆的起始虚拟地址设为 0xc0100000
    实际上可以不连续
 */
#define K_HEAP_START 0xc0100000

// 该地址由 loader 中地址计算而来
#define TOTAL_MEM_ADDR 0x903

#define RANDOM_PAGE_ADDR MEM_MAX_SIZE
// 区分内核内存池及用户内存池
typedef enum
{
    PF_KERNEL = 1,
    PF_USER = 2
} pool_flag;

// 内核虚拟内存池
pool kernel_virtual_pool;
// 内核物理内存池，用户物理内存池
static pool kernel_phy_pool, user_phy_pool;
// 内存池互斥访问
static lock kernel_pool_lock, user_pool_lock;

static void *physical_page_alloc(pool_flag pf, uint32_t cnt);
static void *virtual_page_alloc_from(pool_flag pf, uint32_t addr, uint32_t cnt);
static void map_vaddr2phyaddr(void *_vaddr, void *_phyaddr);
static void *page_alloc_from(pool_flag pf, uint32_t addr, uint32_t cnt);
static void *pool_page_alloc_from(pool *current_pool, uint32_t addr, uint32_t cnt);
static uint32_t *pde_ptr(uint32_t vaddr);
static uint32_t *pte_ptr(uint32_t vaddr);
static void physical_mem_pool_init();
static void kernel_virtual_mem_pool_init();
static void unmap_vaddr2phyaddr(uint32_t vaddr);
static void pool_page_free(pool *current_pool, uint32_t addr, uint32_t cnt);
static void physical_page_free(uint32_t addr, uint32_t cnt);
static void virtual_page_free(uint32_t addr, uint32_t cnt);

void *kernel_page_alloc(uint32_t cnt)
{
    lock_acquire(&kernel_pool_lock);
    void *vaddr = page_alloc_from(PF_KERNEL, RANDOM_PAGE_ADDR, cnt);
    lock_release(&kernel_pool_lock);
    return vaddr;
}

void *kernel_page_alloc_from(uint32_t addr, uint32_t cnt)
{
    lock_acquire(&kernel_pool_lock);
    void *vaddr = page_alloc_from(PF_KERNEL, addr, cnt);
    lock_release(&kernel_pool_lock);
    return vaddr;
}

void *user_page_alloc(uint32_t cnt)
{
    lock_acquire(&user_pool_lock);
    void *vaddr = page_alloc_from(PF_USER, RANDOM_PAGE_ADDR, cnt);
    lock_release(&user_pool_lock);
    return vaddr;
}

void *user_page_alloc_from(uint32_t addr, uint32_t cnt)
{
    lock_acquire(&user_pool_lock);
    void *vaddr = page_alloc_from(PF_USER, addr, cnt);
    lock_release(&user_pool_lock);
    return vaddr;
}

/*
    从 addr 起申请连续 cnt 页内存
    申请成功返回内存页起始地址，失败返回 NULL
 */
static void *page_alloc_from(pool_flag pf, uint32_t addr, uint32_t cnt)
{
    void *vaddr_start = virtual_page_alloc_from(pf, addr, cnt);
    if (vaddr_start == NULL)
        return NULL;

    /* 虚拟地址连续，但物理地址可以不连续，所以逐个做映射*/
    uint32_t __cnt = cnt;
    // 若随机申请内存，addr 可能是 RANDOM_PAGE_ADDR, 此处必须重新赋值
    addr = (uint32_t)vaddr_start;
    while (__cnt-- > 0)
    {
        void *paddr = physical_page_alloc(pf, 1);
        if (paddr == NULL)
        {
            // 分配失败应全部释放已分配的内存,此处暂未实现
            return NULL;
        }
        map_vaddr2phyaddr((void *)addr, paddr);
        addr += PG_SIZE;
    }
    // 分配后所有页框清零
    memset(vaddr_start, 0, cnt * PG_SIZE);
    return vaddr_start;
}
/*
    从虚拟地址池 addr 起申请连续 cnt 页内存
    申请成功返回内存页起始地址，失败返回 NULL
 */
static void *virtual_page_alloc_from(pool_flag pf, uint32_t addr, uint32_t cnt)
{
    pool *current_pool = (pf == PF_KERNEL) ? &kernel_virtual_pool : &(thread_running())->vaddr;

    return pool_page_alloc_from(current_pool, addr, cnt);
}
/*
    从指定内存池地址池随机申请连续 cnt 页内存
    申请成功返回内存页起始地址，失败返回 NULL
 */
static void *pool_page_alloc_from(pool *current_pool, uint32_t addr, uint32_t cnt)
{
    int bit_index_start;
    // 随机分配地址
    if (addr == RANDOM_PAGE_ADDR)
    {
        bit_index_start = bitmap_alloc(&(current_pool->btmp), cnt);
        addr = current_pool->addr_start + bit_index_start * PG_SIZE;
    }
    else
    {
        bit_index_start = (addr - current_pool->addr_start) / PG_SIZE;
        ASSERT(bit_index_start > 0);
        bit_index_start = bitmap_alloc_from(&(current_pool->btmp), bit_index_start, cnt);
    }
    if (bit_index_start == -1)
        return NULL;
    return (void *)addr;
}

// 物理内存池申请连续 cnt 页内存
static void *physical_page_alloc(pool_flag pf, uint32_t cnt)
{
    pool *current_pool = (pf == PF_KERNEL) ? &kernel_phy_pool : &user_phy_pool;
    // 随机申请内存页
    return pool_page_alloc_from(current_pool, RANDOM_PAGE_ADDR, cnt);
}

static void map_vaddr2phyaddr(void *_vaddr, void *_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t phyaddr = (uint32_t)_phyaddr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);
    // 若 pde 不存在，则创建 pde，否则导致 page fault
    if (!(*pde & 0x00000001))
    {
        // 页表的页框一律从内核内存池分配
        uint32_t pde_phyaddr = (uint32_t)physical_page_alloc(PF_KERNEL, 1);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        // 页表物理页起始地址(int)pte & 0xfffff000
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE);
    }

    // 页表存在则说明重复分配页面
    if (*pte & 0x00000001)
        PANIC("pte repeat");
    // 填充页表
    *pte = (phyaddr | PG_US_U | PG_RW_W | PG_P_1);
}

// 虚拟地址转换为对应的物理地址
uint32_t vaddr2paddr(uint32_t vaddr)
{
    uint32_t *pte = pte_ptr(vaddr);
    /*
    (*pte)的值是页表所在的物理页框地址，
    虚拟地址对应的物理地址 = *pde 去掉其低 12 位的页表项属性 + 虚拟地址 vaddr 的低 12 位
     */
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

// pde 页目录项，页目录表的条目
// 构造能访问 vaddr 所在的 pde 的虚拟地址
static uint32_t *pde_ptr(uint32_t vaddr)
{
    // 最后一个页目录项(0xfffff000)指向页目录表物理地址
    // 虚拟地址高 10 位是页目录项相对于页目录表的偏移
    // 页目录表物理地址 + 页目录项偏移 * 4，即虚拟地址对应的 pde
    uint32_t *pde = (uint32_t *)(0xfffff000 + PDE_INDEX(vaddr) * 4);
    return pde;
}

// pte 页表项，页表的条目
// 构造能访问 vaddr 所在的 pte 的虚拟地址
static uint32_t *pte_ptr(uint32_t vaddr)
{
    // 0xffc00000 获取 页目录表 本身，并将页目录表视为"页表"访问
    // 此处"页表"是 页目录表，"页表项"是 页目录项，"页表项"对应的数据即页表
    // 虚拟地址高 10 位是页目录项相对于 页目录表 的偏移，此处用于计算"页表项"相对于"页表"的偏移
    // 虚拟地址中间 10 位是 页表项 相对于 页表 的偏移，此处是"页表项"对应页的"页内偏移地址"
    // 但因为其只有 10 位，其乘 4 后即"页表项"对应页的"页内偏移地址"
    // 页目录表 + 页目录项偏移 + 页表偏移 * 4
    // 页目录表 0xffc00000

    // (PDE_INDEX(addr) << 12) 可写成  ((vaddr & 0xffc00000) >> 10)
    uint32_t *pte = (uint32_t *)(0xffc00000 +
                                 (PDE_INDEX(vaddr) << 12) +
                                 PTE_INDEX(vaddr) * 4);

    return pte;
}

// 初始化内核物理内存池、用户物理内存池、内核虚拟地址池
void mem_pool_init()
{
    lock_init(&kernel_pool_lock);
    lock_init(&user_pool_lock);
    physical_mem_pool_init();
    kernel_virtual_mem_pool_init();
}

// 内核虚拟地址池初始化
static void kernel_virtual_mem_pool_init()
{
    // 内核虚拟内存池的位图长度同内核物理内存池的位图长度相同
    kernel_virtual_pool.btmp.byte_length = kernel_phy_pool.btmp.byte_length;
    kernel_virtual_pool.btmp.bits = (void *)(MEM_BITMAP_BASE + kernel_phy_pool.btmp.byte_length +
                                             user_phy_pool.btmp.byte_length);
    kernel_virtual_pool.addr_start = K_HEAP_START;
    kernel_virtual_pool.size = MEM_MAX_SIZE;
    bitmap_init(&kernel_virtual_pool.btmp);
}

/*
    内存管理单元只管理空闲内存
    固定占用的内存（即页表与低端 1 MB 内存）不在内存管理单元的管理范围之内
    低端 1 MB 物理地址： 0x00000～0xfffff，内核虚拟地址 0xc0000000～0xc00fffff
    页表地址物理地址：0x100000～0x101fff，内核虚拟地址 0xc0100000～0xc0101fff 不能映射到该物理地址处
 */
// 物理内存池初始化
static void physical_mem_pool_init()
{
    uint32_t total_mem = (*(uint32_t *)(TOTAL_MEM_ADDR));

    // 在loader中属于内核的页表已经全部创建完成
    // pde 占 1 页， 第 0 和第 768 个 页目录项相同，占 1 页
    // 第 769～1022 个页目录项共指向 254 个页表
    // 256 = 1 + 1 + 254
    uint32_t page_table_size = PG_SIZE * 256; // 页表占用内存
    // 0x100000 为低端 1MB 内存
    uint32_t used_mem = page_table_size + 0x100000;
    uint32_t free_mem = total_mem - used_mem;
    uint16_t total_free_pages = free_mem / PG_SIZE;
    // 内核物理内存与用户物理内存各占一半，分别管理
    uint16_t kernel_free_pages = total_free_pages / 2;
    uint16_t user_free_pages = total_free_pages - kernel_free_pages;

    // 初始化内核物理内存池
    kernel_phy_pool.addr_start = used_mem;
    kernel_phy_pool.size = kernel_free_pages * PG_SIZE;
    kernel_phy_pool.btmp.byte_length = kernel_free_pages / 8;
    kernel_phy_pool.btmp.bits = (void *)MEM_BITMAP_BASE;
    bitmap_init(&kernel_phy_pool.btmp);

    // 初始化用户物理内存池
    user_phy_pool.addr_start = kernel_phy_pool.addr_start + kernel_phy_pool.size;
    user_phy_pool.size = user_free_pages * PG_SIZE;
    user_phy_pool.btmp.byte_length = user_free_pages / 8;
    user_phy_pool.btmp.bits = (void *)(MEM_BITMAP_BASE + kernel_phy_pool.btmp.byte_length);
    bitmap_init(&user_phy_pool.btmp);
}

// 释放 addr 起的 cnt 页内存
void page_free(void *addr, uint32_t cnt)
{
    uint32_t __paddr, __vaddr = (uint32_t)addr;
    ASSERT(__vaddr % PG_SIZE == 0);
    // addr 地址不是自然页起始地址
    if (__vaddr % PG_SIZE != 0)
        return;
    // 释放虚拟地址
    virtual_page_free(__vaddr, cnt);
    // 物理内存不连续，逐页释放物理内存
    uint32_t i;
    for (i = 0; i < cnt; i++)
    {
        __paddr = vaddr2paddr(__vaddr + PG_SIZE * i);

        physical_page_free(__paddr, 1);
        // 删除页表
        unmap_vaddr2phyaddr(__vaddr + PG_SIZE * i);
    }
}


// 从虚拟地址池释放 addr 起连续 cnt 页内存
static void virtual_page_free(uint32_t addr, uint32_t cnt)
{
    // 释放内存可以通过判断是否有页表判断是用户内存还是内核地址
    // 但分配内存不可以，因为内存分配可能发生进程未初始化时
    task_struct *current_thread = thread_running();
    // 无单独页表为内核线程
    pool *current_pool = (current_thread->pgdir == NULL) ? &kernel_virtual_pool : &current_thread->vaddr;
    pool_page_free(current_pool, addr, cnt);
}


// 物理内存池释放 addr 起连续 cnt 页内存
static void physical_page_free(uint32_t addr, uint32_t cnt)
{
    // 跳过保留内存，低端 1 MB 和内核页表
    if (addr < 0x102000)
        return;
    // 在内核物理内存地址低于用户物理内存
    pool *current_pool = (addr < user_phy_pool.addr_start) ? &kernel_phy_pool : &user_phy_pool;
    pool_page_free(current_pool, addr, cnt);
}


static void pool_page_free(pool *current_pool, uint32_t addr, uint32_t cnt)
{
    int idx = (addr - current_pool->addr_start) / PG_SIZE;
    bitmap_free(&current_pool->btmp, idx, cnt);
}

// 删除 addr 对应的页表项，即对应页表项 P 位置 0
static void unmap_vaddr2phyaddr(uint32_t vaddr)
{
    uint32_t *pte = pte_ptr(vaddr);
    *pte = *pte & PG_P_0; // 页表项页表 P 置 0
    // invlpg [addr];  注意方括号，invlpg 参数必须是 addr 处内存
    asm volatile("invlpg %0" ::"m"(vaddr)
                 : "memory"); // 刷新 tlb, 使 vaddr 对应的 tlb 条目失效
}