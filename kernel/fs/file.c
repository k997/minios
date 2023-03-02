#include "fs.h"
#include "thread.h"
#include "kstdio.h"
#include "memory.h"
#include "string.h"
#include "debug.h"

// 整个系统的文件打开表
file global_open_file_table[MAX_FILE_OPEN];

void global_open_file_table_init()
{
    uint32_t idx;
    for (idx = 0; idx < MAX_FILE_OPEN; idx++)
    {
        global_open_file_table[idx].fd_inode = NULL;
    }
}
int32_t get_free_slot_in_global_open_file_table()
{
    int32_t fd_idx = 3;
    for (fd_idx = 3; fd_idx < MAX_FILE_OPEN; fd_idx++)
    {
        if (global_open_file_table[fd_idx].fd_inode == NULL)
            return fd_idx;
    }
    return -1;
}

// 将 global_open_file_table 的描述符安装到进程自己的 pcb 中
int32_t pcb_fd_intall(int32_t global_idx)
{
    task_struct *cur_thread = thread_running();
    uint32_t local_idx; // 跨过 stdin,stdout,stderr
    for (local_idx = 3; local_idx < MAX_FILES_OPEN_PER_PROC; local_idx++)
    {
        if (cur_thread->fd_table[local_idx] != -1)
        {
            cur_thread->fd_table[local_idx] = global_idx;
            return local_idx;
        }
    }
    printk("exceed max open files_per_proc\n");
    return -1;
}

// pcb 中 fd 转换为对应的文件
file *fd_to_file(uint32_t fd)
{
    task_struct *cur_task = thread_running();
    uint32_t global_idx = cur_task->fd_table[fd];
    ASSERT(global_idx > 0 && global_idx < MAX_FILE_OPEN);
    return &global_open_file_table[global_idx];
}

int32_t file_open(uint32_t inode_nr, oflags flag)
{
    int fd_idx = get_free_slot_in_global_open_file_table();
    if (fd_idx == -1)
    {
        printk("exceed max open files\n");
        return -1;
    }

    global_open_file_table[fd_idx].fd_flag = flag;
    global_open_file_table[fd_idx].fd_inode = inode_open(cur_part, inode_nr);
    global_open_file_table[fd_idx].fd_pos = 0;

    // 如果以写文件的方式打开
    if (flag & O_W_ONLY || flag & O_RW)
    {
        bool *write_deny = &global_open_file_table[fd_idx].fd_inode->write_deny;

        interrupt_status old = interrupt_disable();
        // 如果现在还没有其他进程在写该文件则将 write_deny 设为 true,避免多个进程同时写此文件
        if (*write_deny == false)
        {
            *write_deny = true;
            interrupt_set_status(old);
        }
        else
        { // 失败直接返回
            interrupt_set_status(old);
            printk("file can’t be write now, try again later\n");
            return -1;
        }
    }
    // 若是读文件或创建文件，不用理会 write_deny，保持默认
    return pcb_fd_intall(fd_idx);
}

int32_t file_close(file *f)
{
    if (f == NULL)
        return -1;
    f->fd_inode->write_deny = false;
    inode_close(f->fd_inode);
    f->fd_inode = NULL; // 从 global_open_file_table 释放
    return 0;
}

/*
1. 申请新文件 inode 并初始化
2. 从全局文件打开表中获取空闲位置
3. 创建目录项
4. 同步所有数据到硬盘
5. 将新文件加入到分区的 inode 打开表
6. 将文件描述符插入进程 pcb
 */
int32_t file_create(dir *parent_dir, char *filename, oflags flag)
{

    void *buf = sys_malloc(2 * BLOCK_SIZE); // 某些数据结构可能跨扇区
    if (buf == NULL)
    {
        printk("in file_create: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint8_t rollback_setp = 0; // 用于某阶段失败的话回滚，回收已分配的资源

    int32_t inode_nr = bitmap_alloc(&cur_part->inode_bitmap, 1);
    if (inode_nr < 0)
    {
        printk("in file_create: allocate inode failed\n");
        return -1;
    }

    /* 此 inode 要从堆中申请内存，不可生成局部变量（函数退出时会释放），
     * 因为系统的文件打开表 global_open_file_table 中文件描述符的 inode 指针要指向它 */

    inode *new_inode = (inode *)sys_malloc(sizeof(inode));
    if (new_inode == NULL)
    {
        printk("file_create: sys_malloc for inode failded\n");
        rollback_setp = 1;
        goto rollback;
    }
    inode_init(inode_nr, new_inode);
    int32_t fd_idx = get_free_slot_in_global_open_file_table();
    if (fd_idx == -1)
    {
        printk("exceed max open files\n");
        rollback_setp = 2;
        goto rollback;
    }

    global_open_file_table[fd_idx].fd_inode = new_inode;
    global_open_file_table[fd_idx].fd_flag = flag;
    global_open_file_table[fd_idx].fd_pos = 0;
    global_open_file_table[fd_idx].fd_inode->write_deny = false;

    dir_entry d_entry;
    memset(&d_entry, 0, sizeof(dir_entry));                  // 按理说不需要 memset
    create_dir_entry(filename, inode_nr, FS_FILE, &d_entry); // create_dir_entry 只是内存操作不出意外，不会返回失败
    /* 同步内存数据到硬盘 */
    if (!sync_dir_entry(parent_dir, &d_entry, buf)) // 同步 dir entry
    {
        printk("sync dir_entry to disk failed\n");
        rollback_setp = 3;
        goto rollback;
    }

    memset(buf, 0, 2 * BLOCK_SIZE);
    inode_sync(cur_part, parent_dir->inode, buf); // 同步父目录 i 结点

    memset(buf, 0, 2 * BLOCK_SIZE);
    inode_sync(cur_part, new_inode, buf); // 新创建文件的 i 结点

    memset(buf, 0, 2 * BLOCK_SIZE);
    bitmap_sync(cur_part, inode_nr, INODE_BITMAP); // 同步 inode_bitmap 位图

    /* 将创建的文件 i 结点添加到 open_inodes 链表 */
    list_push(&cur_part->open_inodes, &new_inode->inode_tag);
    new_inode->i_open_cnts = 1;

    sys_free(buf);
    return pcb_fd_intall(fd_idx);

rollback:
    switch (rollback_setp)
    {
        // 注意没有 break
    case 3:
        /* 失败时，将 global file_table 中的相应位清空 */
        memset(&global_open_file_table[fd_idx], 0, sizeof(struct file));

    case 2:
        sys_free(new_inode);
    case 1:
        bitmap_free(&cur_part->inode_bitmap, inode_nr, 1);
    }
    sys_free(buf);
    return -1;
}

/* 把 buf 中的 count 个字节写入 file，成功则返回写入的字节数，失败则返回-1
新写入的数据总是在原来数据之后
*/
int32_t file_write(file *f, const void *buf, uint32_t nbytes)
{
    uint32_t file_max_block_cnt = 12 + BLOCK_SIZE / sizeof(uint32_t);
    if (f->fd_inode->i_size + nbytes > file_max_block_cnt * BLOCK_SIZE)
    {
        printk("exceed max file_size, write file failed\n");
        return -1;
    }

    // 文件使用的 block 的所有 lba
    uint32_t *file_blocks_lba = (uint32_t *)sys_malloc(sizeof(uint32_t) * file_max_block_cnt);
    if (file_blocks_lba == NULL)
        return -1;
    memset(file_blocks_lba, 0, sizeof(uint32_t) * file_max_block_cnt);
    void *io_buf = sys_malloc(BLOCK_SIZE);
    if (io_buf == NULL)
    {
        sys_free(file_blocks_lba);
        return -1;
    }

    // 1 分配写入数据所需要的 block
    uint32_t will_used_block = (f->fd_inode->i_size + nbytes) % BLOCK_SIZE == 0 ? (f->fd_inode->i_size + nbytes) / BLOCK_SIZE : (f->fd_inode->i_size + nbytes) / BLOCK_SIZE + 1;
    // 1.1开始收集所有扇区信息到 file_blocks_lba
    uint32_t blk_idx;
    for (blk_idx = 0; blk_idx < 12; blk_idx++) // 直接块
    {
        file_blocks_lba[blk_idx] = f->fd_inode->i_blocks[blk_idx];
    }

    if (will_used_block > 12) // 间接块
    {
        if (f->fd_inode->i_blocks[12] == 0)
        { // 间接索引表未分配
            int32_t block_bitmap_idx = bitmap_alloc(&cur_part->block_bitmap, 1);
            if (block_bitmap_idx < 0)
            {
                printk("alloc block bitmap for file_write failed!\n");
                sys_free(file_blocks_lba);
                sys_free(io_buf);
                return -1;
            }
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP); // 每分配一个块就同步一次 block_bitmap
            f->fd_inode->i_blocks[12] = data_block_lba(cur_part, block_bitmap_idx);
            memset(io_buf, 0, BLOCK_SIZE);
            disk_write(cur_part->belong_to_disk, io_buf, f->fd_inode->i_blocks[12], 1);
        }
        else
        {
            disk_read(cur_part->belong_to_disk, file_blocks_lba + 12, f->fd_inode->i_blocks[12], 1);
        }
    }

    // 1.2 所有扇区信息收集完毕，开始分配 block
    for (blk_idx = 0; blk_idx < will_used_block; blk_idx++)
    {
        // 存在则跳过，不存在则分配
        if (file_blocks_lba[blk_idx] != 0)
            continue;
        int32_t block_bitmap_idx = bitmap_alloc(&cur_part->block_bitmap, 1);
        if (block_bitmap_idx < 0)
        {
            printk("alloc block bitmap for file_write failed!\n");
            sys_free(file_blocks_lba);
            sys_free(io_buf);
            return -1;
        }
        bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP); // 每分配一个块就同步一次 block_bitmap
        file_blocks_lba[blk_idx] = data_block_lba(cur_part, block_bitmap_idx);

        if (blk_idx < 12) // 对于直接块 inode 的 block 也要直接更新
            f->fd_inode->i_blocks[blk_idx] = file_blocks_lba[blk_idx];
    }
    // block 分配完毕，有间接块则写回间接索引表
    if (f->fd_inode->i_blocks[12] != 0)
        disk_write(cur_part->belong_to_disk, file_blocks_lba + 12, f->fd_inode->i_blocks[12], 1);
    // 2 开始写人数据
    uint32_t blk_offset, blk_left_bytes, blk_lba;
    uint32_t this_time_write_bytes;
    uint32_t bytes_left = nbytes;
    const uint8_t *write_ptr = buf;
    while (bytes_left > 0)
    {
        memset(io_buf, 0, BLOCK_SIZE);
        blk_idx = f->fd_inode->i_size / BLOCK_SIZE;
        blk_lba = file_blocks_lba[blk_idx];
        blk_offset = f->fd_inode->i_size % BLOCK_SIZE;
        blk_left_bytes = BLOCK_SIZE - blk_offset;

        this_time_write_bytes = bytes_left < blk_left_bytes ? bytes_left : blk_left_bytes;

        if (blk_offset != 0) // 如果当前块内有数据则要先读出
            disk_read(cur_part->belong_to_disk, io_buf, blk_lba, 1);
        memcpy(io_buf + blk_offset, write_ptr, this_time_write_bytes);
        disk_write(cur_part->belong_to_disk, io_buf, blk_lba, 1);
        bytes_left -= this_time_write_bytes; /* this_time_write_bytes 永远是 bytes_left 和  blk_left_bytes
                                            较小的一个，因此不用担心相减溢出*/
        write_ptr += this_time_write_bytes;
        f->fd_pos += this_time_write_bytes - 1; // 文件打开位置实时更新
        f->fd_inode->i_size += this_time_write_bytes;
    }
    inode_sync(cur_part, f->fd_inode, io_buf);
    sys_free(file_blocks_lba);
    sys_free(io_buf);
    return nbytes - bytes_left;
}

int32_t file_read(file *f, void *buf, uint32_t nbytes)
{
    uint32_t _nbytes = nbytes;
    if (f->fd_pos + nbytes > f->fd_inode->i_size)
    {
        _nbytes = f->fd_inode->i_size - f->fd_pos;
        if (_nbytes == 0)
            return -1;
    }

    uint8_t *io_buf = sys_malloc(BLOCK_SIZE);
    if (io_buf == NULL)
    {
        printk("file_read: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint32_t file_max_block_cnt = 12 + BLOCK_SIZE / sizeof(uint32_t);
    uint32_t *file_blocks_lba = (uint32_t *)sys_malloc(sizeof(uint32_t) * file_max_block_cnt);
    if (file_blocks_lba == NULL)
    {
        sys_free(io_buf);
        printk("file_read: sys_malloc for file_blocks_lba failed\n");
        return -1;
    }

    uint32_t blk_idx;
    for (blk_idx = 0; blk_idx < 12; blk_idx++)
    { // 读取直接块信息
        file_blocks_lba[blk_idx] = f->fd_inode->i_blocks[blk_idx];
    }
    uint32_t end_blk_idx = (f->fd_pos + _nbytes) / BLOCK_SIZE;
    if (end_blk_idx >= 12)
    { // 需要读取间接块
        disk_read(cur_part->belong_to_disk, file_blocks_lba + 12, f->fd_inode->i_blocks[12], 1);
    }
    uint32_t blk_offset, blk_left_bytes, blk_lba;
    uint32_t this_time_read_bytes;
    uint32_t bytes_left = _nbytes;
    uint8_t *read_ptr = buf;
    while (bytes_left > 0)
    {
        memset(io_buf, 0, BLOCK_SIZE);
        blk_idx = f->fd_pos / BLOCK_SIZE;
        blk_offset = f->fd_pos % BLOCK_SIZE;

        blk_lba = file_blocks_lba[blk_idx];
        blk_left_bytes = BLOCK_SIZE - blk_offset;

        this_time_read_bytes = bytes_left < blk_left_bytes ? bytes_left : blk_left_bytes;

        disk_read(cur_part->belong_to_disk, io_buf, blk_lba, 1);
        memcpy(read_ptr, io_buf + blk_offset, this_time_read_bytes);

        bytes_left -= this_time_read_bytes; /* this_time_read_bytes 永远是 bytes_left 和  blk_left_bytes
                                            较小的一个，因此不用担心相减溢出*/
        read_ptr += this_time_read_bytes;
        f->fd_pos += this_time_read_bytes; // 文件打开位置实时更新
    }
    sys_free(file_blocks_lba);
    sys_free(io_buf);
    return _nbytes - bytes_left;
}