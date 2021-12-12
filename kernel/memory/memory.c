#include "memory.h"
#include "bitmap.h"
#include "print.h"

// 位图也位于低端 1MB 内存之中，低端 1MB 物理内存不在内存管理单元的管理范围之内
#define MEM_BITMAP_BASE 0xc009a000

/*
    内核堆起始地址
    0xc0000000～0xc00fffff 映射到了物理地址 0x00000000～0x000fffff（低端 1MB 的内存）
    为了让虚拟地址连续，将堆的起始虚拟地址设为 0xc0100000
    实际上可以不连续
 */
#define K_HEAP_START 0xc0100000

// 内核物理内存池，用户物理内存池，内核虚拟内存池
pool kernel_phy_pool, user_phy_pool, kernel_virtual_pool;

static void physical_mem_pool_init(uint32_t total_mem);
static void kernel_virtual_mem_pool_init();

void mem_init()
{
    // 该地址由 loader 中地址计算而来
    uint32_t total_mem = (*(uint32_t *)(0x903));
    physical_mem_pool_init(total_mem);
    kernel_virtual_mem_pool_init();
}

// 初始化内核虚拟内存池
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
static void physical_mem_pool_init(uint32_t total_mem)
{
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