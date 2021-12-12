#include "memory.h"
#include "bitmap.h"
#include "string.h"
#include "debug.h"
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

static void *physical_page_alloc(pool_flag pf);
static void *virtual_page_alloc(pool_flag pf, uint32_t cnt);
static void map_vaddr2phyaddr(void *_vaddr, void *_phyaddr);
static void *pool_page_alloc(pool *current_pool, uint32_t cnt);
static uint32_t *pde_ptr(uint32_t vaddr);
static uint32_t *pte_ptr(uint32_t vaddr);

void *kernel_page_alloc(uint32_t cnt)
{
    return page_alloc(PF_KERNEL, cnt);
}

void *user_page_alloc(uint32_t cnt)
{
    return page_alloc(PF_USER, cnt);
}

void *page_alloc(pool_flag pf, uint32_t cnt)
{
    void *vaddr_start = virtual_page_alloc(pf, cnt);
    if (vaddr_start == NULL)
        return NULL;

    /* 虚拟地址连续，但物理地址可以不连续，所以逐个做映射*/
    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t __cnt = cnt;
    while (__cnt-- > 0)
    {
        void *paddr = physical_page_alloc(pf);
        if (paddr == NULL)
        {
            // 分配失败应全部释放已分配的内存,此处暂未实现
            return NULL; 
        }
        map_vaddr2phyaddr((void *)vaddr, paddr);
        vaddr += PG_SIZE;
    }
    // 分配后所有页框清零
    memset(vaddr_start, 0, cnt * PG_SIZE);
    return vaddr_start;
}

// 物理内存地址可以不连续，所以每次只申请 1 页，失败则返回 NULL
static void *physical_page_alloc(pool_flag pf)
{
    pool *current_pool;
    switch (pf)
    {
    case PF_KERNEL:
        current_pool = &kernel_phy_pool;
        break;
    case PF_USER:
        current_pool = &user_phy_pool;
        break;
    }

    return pool_page_alloc(current_pool, 1);
}
// 虚拟内存池中申请连续 cnt 页，并返回第一块的起始地址，失败则返回 NULL
static void *virtual_page_alloc(pool_flag pf, uint32_t cnt)
{
    pool *current_pool;
    switch (pf)
    {
    case PF_KERNEL:
        current_pool = &kernel_virtual_pool;
        break;
    case PF_USER:
        // 用户程序的内存分配依赖进程的实现
        break;
    }
    return pool_page_alloc(current_pool, cnt);
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
        uint32_t pde_phyaddr = (uint32_t)physical_page_alloc(PF_KERNEL);
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

// 从指定内存池中申请连续 cnt 页，并返回第一页的起始地址，失败则返回 NULL
static void *pool_page_alloc(pool *current_pool, uint32_t cnt)
{
    int addr_start, bit_index_start = -1;

    bit_index_start = bitmap_alloc(&(current_pool->btmp), cnt);

    if (bit_index_start == -1)
        return NULL;
    addr_start = current_pool->addr_start + bit_index_start * PG_SIZE;
    return (void *)addr_start;
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