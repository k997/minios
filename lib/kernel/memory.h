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

#define BIN_CNT 7

#include "bitmap.h"
#include "stdint.h"
#include "list.h"

/*
pool 用于记录内存分配情况
类似于 linux 的 mm_struct( memory map strcut)
每个进程只支持一个 arena（即从 addr_start 开始到 0xFFFFFFFF 整个虚拟地址空间）
arena 的 vm_start 即 addr_start
arena 的 vm_end 即  0xFFFFFFFF
arena 的 brk 在此处不做限制
 */
typedef struct
{
    bitmap btmp;         // 位图用于管理内存
    uint32_t addr_start; // 内存起始地址
    uint32_t size;       // 内存大小
                         // 若为虚拟内存池，则设置为 MEM_MAX_SIZE
} pool;

// 不同内存块大小的空闲内存链表
typedef struct mem_bin
{
    uint32_t chunk_size;       // 链表中内存块的大小
    uint32_t chunks_per_arena; // 每个 arena 的内存块数量
    list free_list;            // 空闲内存块链表
} mem_bin;

typedef struct mem_chunk
{
    list_elem free_elem;
} mem_chunk;

/*
malloc 通过 arena 向内存池申请一页或多页内存，释放时也已 arena 为单位释放
arena 在申请的内存区域最开始的地方存放 arena 的元数据
 */
typedef struct arena
{
    mem_bin *mb;                     // 此 arena 关联的 mem_bin
    uint32_t free_chunk_or_page_cnt; // large 为 ture 时，表示的是页框数。否则表示空闲 mem_chunk 数量
    bool large;                      // 是否占据整个页框
} arena;

// 外部引用声明
extern pool kernel_virtual_pool;
void mem_init(void);
void mem_pool_init(void);
void *page_alloc(uint32_t pg_cnt);
void *page_alloc_from(uint32_t addr, uint32_t pg_cnt);
void *kernel_page_alloc(uint32_t cnt);
void *kernel_page_alloc_from(uint32_t addr, uint32_t cnt);
void *user_page_alloc(uint32_t cnt);
void *user_page_alloc_from(uint32_t addr, uint32_t cnt);
uint32_t vaddr2paddr(uint32_t vaddr);
void page_free(void *addr, uint32_t cnt);
void *sys_malloc(uint32_t size);
void sys_free(void *ptr);

void mem_bin_init(mem_bin mb[]);
void kernel_mem_bin_init();
#endif