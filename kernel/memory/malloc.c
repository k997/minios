#include "memory.h"
#include "thread.h"
#include "global.h"
#include "debug.h"


mem_bin kernel_mem_bin[BIN_CNT];


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