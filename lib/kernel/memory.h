#ifndef __KERNEL_MEMEORY_H
#define __KERNEL_MEMORY_H

#define PG_P_1 1
#define PG_P_0 0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

// 页尺寸为 4KB
#define PG_SIZE 4096
#define MEM_MAX_SIZE 0xFFFFFFFF

#include "bitmap.h"
#include "stdint.h"

typedef struct
{
    bitmap btmp;         // 位图用于管理内存
    uint32_t addr_start; // 内存起始地址
    uint32_t size;       // 内存大小
                         // 若为虚拟内存池，则设置为 MEM_MAX_SIZE
} pool;

// 区分内核内存池及用户内存池
typedef enum
{
    PF_KERNEL = 1,
    PF_USER = 2
} pool_flag;

// 外部引用声明
extern pool kernel_phy_pool, user_phy_pool, kernel_virtual_pool;

void mem_init(void);
void *page_alloc(pool_flag pf, uint32_t cnt);
void *kernel_page_alloc(uint32_t cnt);
void *user_page_alloc(uint32_t cnt);
#endif