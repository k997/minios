#include "memory.h"
#include "bitmap.h"
#include "thread.h"
#include "global.h"
#include "debug.h"

void mem_init()
{
    mem_pool_init();
}
/* malloc 系统调用，申请 size 字节内存 */
void *sys_malloc(uint32_t size)
{
    void *addr = NULL;
    task_struct *current_thread = thread_running();

    // 只支持按页分配内存, 且单次申请的最大内存为 1 页
    // 即不管申请多少内存，都返回 1 页
    uint32_t pg_cnt = 1;
    // uint32_t pg_cnt = DIV_ROUND_UP(size, PG_SIZE);
    if (current_thread->pgdir == NULL)
        addr = kernel_page_alloc(pg_cnt);
    else
        addr = user_page_alloc(pg_cnt);
    return addr;
}
/* free 系统调用，回收 ptr 处内存 */
void sys_free(void *ptr)
{
    ASSERT(ptr != NULL);
    // 即不管 ptr 所指向内存大小，都释放 1 页
    page_free(ptr, 1);
}