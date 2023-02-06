#include "file.h"
#include "thread.h"
#include "kstdio.h"
// 整个系统的文件打开表
file global_file_table[MAX_FILE_OPEN];

int32_t get_free_slot_in_global()
{
    uint32_t fd_idx = 3;
    for (fd_idx = 3; fd_idx < MAX_FILE_OPEN; fd_idx++)
    {
        if (global_file_table[fd_idx].fd_inode == NULL)
            return fd_idx;
    }
    return -1;
}

// 将 global_file_table 的描述符安装到进程自己的 pcb 中
int32_t pcb_fd_intall(int32_t global_idx)
{
    task_struct *cur_thread = thread_running();
    uint32_t local_idx;// 跨过 stdin,stdout,stderr
    for(local_idx=3;local_idx<MAX_FILES_OPEN_PER_PROC;local_idx++)
    {
        if(cur_thread->fd_table[local_idx]!=-1){
            cur_thread->fd_table[local_idx]=global_idx;
            return local_idx;
        }
    }
    printk("exceed max open files_per_proc\n");
    return -1;
}

