#ifndef __LIB_KERNEL_MEMORY_H
#define __LIB_KERNEL_MEMORY_H

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



// 外部引用声明
extern pool kernel_virtual_pool;
void mem_init(void);
void mem_pool_init(void);
void *kernel_page_alloc(uint32_t cnt);
void *kernel_page_alloc_from(uint32_t addr, uint32_t cnt);
void *user_page_alloc(uint32_t cnt);
void *user_page_alloc_from(uint32_t addr, uint32_t cnt);
uint32_t vaddr2paddr(uint32_t vaddr);
void page_free(void *addr, uint32_t cnt);
#endif