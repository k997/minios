#include "memory.h"
#include "thread.h"
#include "global.h"
#include "debug.h"

mem_bin kernel_mem_bin[BIN_CNT];
static void *small_mchunk_alloc(uint32_t size);
static mem_chunk *arena2mchunk(arena *a, uint32_t idx);
static arena *mchunk2arena(mem_chunk *mc);

/* malloc 系统调用，申请 size 字节内存 */
void *sys_malloc(uint32_t size)
{
    arena *a = NULL;
    /* 超过最大内存块 1024，就分配页框 */
    if (size > 1024)
    {
        uint32_t pg_cnt = DIV_ROUND_UP(size + sizeof(arena), PG_SIZE);
        a = page_alloc(pg_cnt);
        if (!a)
            return NULL;
        a->large = true;
        a->free_chunk_or_page_cnt = pg_cnt;
        a->mb = NULL;
        return (void *)(a + 1); // 跨过 arena 大小，把剩下的内存返回
    }
    else
        return small_mchunk_alloc(size);
}

/* free 系统调用，回收 ptr 处内存 */
void sys_free(void *ptr)
{
    ASSERT(ptr != NULL);
    if (!ptr)
        return;
    mem_chunk *mchunk_ptr = (mem_chunk *)ptr;
    arena *a = mchunk2arena(mchunk_ptr);
    ASSERT(a->large == 0 || a->large == 1);

    if (a->large && a->mb == NULL)
    {
        page_free(a, a->free_chunk_or_page_cnt);
        return;
    }

    // list 本身互斥
    list_append(&a->mb->free_list, &mchunk_ptr->free_elem);

    ASSERT(list_find_elem(&a->mb->free_list, &mchunk_ptr->free_elem));

    if (++a->free_chunk_or_page_cnt == a->mb->chunks_per_arena)
    {

        uint32_t chk_idx;


        for (chk_idx = 0; chk_idx < a->mb->chunks_per_arena; chk_idx++)
        {
            mchunk_ptr = arena2mchunk(a, chk_idx);
            ASSERT(list_find_elem(&a->mb->free_list, &mchunk_ptr->free_elem));
            list_remove(&mchunk_ptr->free_elem);
        }
        page_free(a, 1);
    }
}

static void *small_mchunk_alloc(uint32_t size)
{
    mem_chunk *mchunk_ptr;
    arena *a;
    struct task_struct *cur_thread = thread_running();

    mem_bin *mb;
    if (cur_thread->pgdir == NULL)
        mb = kernel_mem_bin;
    else
        mb = cur_thread->mb;

    /* 匹配合适的内存块规格 */
    while (size > mb->chunk_size)
        mb++;

    /* mem bin 中没有空闲的 chunk */
    if (list_empty(&mb->free_list))
    {
        a = page_alloc(1);
        if (!a)
            return NULL;
        a->mb = mb;
        a->free_chunk_or_page_cnt = mb->chunks_per_arena;
        a->large = false;

        // 将 arena 分割成多个 mem chunk
        uint32_t chk_idx;
        for (chk_idx = 0; chk_idx < mb->chunks_per_arena; chk_idx++)
        {
            mchunk_ptr = arena2mchunk(a, chk_idx);
            ASSERT(!list_find_elem(&a->mb->free_list, &mchunk_ptr->free_elem));
            list_append(&a->mb->free_list, &mchunk_ptr->free_elem);
        }
    }

    /* 分配 mem chunk */
    mchunk_ptr = container_of(mem_chunk, free_elem, list_pop(&mb->free_list));
    a = mchunk2arena(mchunk_ptr);
    a->free_chunk_or_page_cnt--;

    return (void *)mchunk_ptr;
}

static mem_chunk *arena2mchunk(arena *a, uint32_t idx)
{
    return (mem_chunk *)((uint32_t)a + sizeof(arena) + idx * a->mb->chunk_size);
}

static arena *mchunk2arena(mem_chunk *mc)
{
    return (arena *)((uint32_t)mc & 0xfffff000);
}

void mem_bin_init(mem_bin *mb)
{
    uint32_t bin_idx, chunk_size = 16;
    for (bin_idx = 0; bin_idx < BIN_CNT; bin_idx++)
    {
        mb[bin_idx].chunk_size = chunk_size;
        mb[bin_idx].chunks_per_arena = (PG_SIZE - sizeof(arena)) / chunk_size;
        list_init(&mb[bin_idx].free_list);
        chunk_size *= 2;
    }
}

void kernel_mem_bin_init()
{
    mem_bin_init(kernel_mem_bin);
}