#include "fs.h"
#include "string.h"
#include "kstdio.h"
#include "debug.h"

void fs_init()
{

    format_all_partition();
    char default_part[IDE_NAME_LEN] = "sdb0";
    mount_partition(default_part);
    open_root_dir(cur_part);
    global_open_file_table_init();
}

/* 打开或创建文件成功后，返回 pcb 中文件描述符，否则返回-1 */
int32_t sys_open(const char *path, uint8_t flag)
{
    if (path[strlen(path) - 1] == '/')
    {
        printk("can`t open a directory %s\n", path);
        return -1;
    }
    int32_t fd = -1; // 默认找不到, 此 fd 是指任务 pcb->fd_table 数组中的元素下标
    path_search_record record;
    memset(&record, 0, sizeof(record));
    int32_t inode_nr = search_file(path, &record);

    uint32_t path_depth_cnt = path_depth((char *)path);
    uint32_t searched_path_depth_cnt = path_depth(record.searched_path);
    ASSERT(path_depth_cnt == searched_path_depth_cnt);

    // 无论最后一级目录中文件是否存在，searched_path 和文件路径应该一致,除非中途查找失败
    if (path_depth_cnt != searched_path_depth_cnt)
    {
        printk("cannot access %s: %s is’t exist\n", path, record.searched_path);
        dir_close(record.parent_dir);
        return -1;
    }

    bool found = inode_nr != -1 ? true : false;
    bool is_create = flag & O_CREAT;

    if (found)
    {
        if (record.f_type == FS_DIRECTORY)
        {
            printk("can`t open a direcotry with open(), use opendir() to instead\n");
            dir_close(record.parent_dir);
            return -1;
        }
        if (is_create)
        {
            printk("%s has already exist!\n", path);
            dir_close(record.parent_dir);
            return -1;
        }

        fd = file_open(inode_nr, flag);
        dir_close(record.parent_dir);
        return fd;
    }
    else
    {
        char *_filename = strrchr(path, '/') + 1;

        if (is_create)
        {
            fd = file_create(record.parent_dir, _filename, flag);
            dir_close(record.parent_dir);
            return fd;
        }

        printk("in path %s, file %s is`t exist\n", record.searched_path, _filename);
        dir_close(record.parent_dir);
        return -1;
    }
}

/* 关闭 pcb 中文件描述符 fd 指向的文件，成功返回 0，否则返回-1 */
int32_t sys_close(int32_t fd)
{
    int32_t ret = -1;
    if (fd > 2)
    {
        file *f = fd_to_file(fd);
        ret = file_close(f);
        thread_running()->fd_table[fd] = -1;
    }
    return ret;
}

/* 将 buf 中连续 count 个字节写入文件描述符 fd，成功则返回写入的字节数，失败返回-1 */
int32_t sys_write(int32_t fd, const void *buf, uint32_t nbytes)
{
    if (fd < 0)
    {
        printk("sys_write: fd error\n");
        return -1;
    }
    if (fd == STD_OUT)
    {
        char tmp_buf[STDIO_BUF_SIZE] = {0};
        memcpy(tmp_buf, buf, nbytes);
        printk(tmp_buf);
        return strlen(tmp_buf);
    }

    file *f = fd_to_file(fd);
    if (f->fd_flag & O_W_ONLY || f->fd_flag & O_RW)
    {
        uint32_t _nbytes = file_write(f, buf, nbytes);
        return _nbytes;
    }
    else
    {
        // 没有写权限
        printk("sys_write: not allowed to write file\n");
        return -1;
    }
}

int32_t sys_read(int32_t fd, void *buf, uint32_t nbytes)
{
    if (fd < 0)
    {
        printk("sys_read: fd error\n");
        return -1;
    }
    ASSERT(buf != NULL);
    file *f = fd_to_file(fd);
    return file_read(f, buf, nbytes);
}

/* 删除文件（非目录），成功返回 0，失败返回-1 */
int32_t sys_unlink(const char *pathname)
{
    ASSERT(strlen(pathname) < MAX_PATH_LEN);
    path_search_record record;
    memset(&record, 0, sizeof(record));
    int32_t inode_nr = search_file(pathname, &record);
    if (inode_nr == -1)
    {
        printk("file %s not found!\n", pathname);
        dir_close(record.parent_dir);
        return -1;
    }
    if (record.f_type == FS_DIRECTORY)
    {
        printk("can`t delete a direcotry with unlink(),use rmdir() to instead\n");
        dir_close(record.parent_dir);
        return -1;
    }

    //  文件在使用则禁止删除
    uint32_t file_idx;
    for (file_idx = 0; file_idx < MAX_FILE_OPEN; file_idx++)
    {
        if (global_open_file_table[file_idx].fd_inode != NULL &&
            (uint32_t)inode_nr == global_open_file_table[file_idx].fd_inode->i_nr)
        {
            printk("file %s is in use, not allow to delete!\n", pathname);
            dir_close(record.parent_dir);
            return -1;
        }
    }

    void *buf = sys_malloc(2 * BLOCK_SIZE);
    if (buf == NULL)
    {
        printk("sys_unlink: malloc for buf failed\n");
        dir_close(record.parent_dir);
        return -1;
    }

    delete_dir_entry(cur_part, record.parent_dir, inode_nr, buf);
    inode_release(cur_part, inode_nr);
    dir_close(record.parent_dir);
    sys_free(buf);
    return 0;
}

int32_t sys_mkdir(const char *pathname)
{
    uint8_t rollback_step = 0;
    void *io_buf = sys_malloc(BLOCK_SIZE * 2);
    if (io_buf == NULL)
    {
        printk("sys_mkdir: sys_malloc for io_buf failed\n");
        return -1;
    }
    path_search_record record;
    memset(&record, 0, sizeof(record));
    int32_t inode_nr = -1;
    inode_nr = search_file(pathname, &record);
    // 文件已存在
    if (inode_nr != -1)
    {
        printk("sys_mkdir: file or directory %s exist!\n", pathname);
        rollback_step = 1;
        goto rollback;
    }
    else
    {
        uint32_t pathname_depth = path_depth((char *)pathname);
        uint32_t record_path = path_depth(record.searched_path);
        // 中间目录不存在
        if (pathname_depth != record_path)
        {
            printk("sys_mkdir: create %s fail, can not access subpath %s\n",
                   pathname, record.searched_path);

            rollback_step = 1;
            goto rollback;
        }
    }
    // 文件夹不存在且父目录存在
    // 初始化目录 inode
    inode_nr = bitmap_alloc(&cur_part->inode_bitmap, 1);
    if (inode_nr == -1)
    {
        printk("sys_mkdir: allocate inode failed\n");
        rollback_step = 1;
        goto rollback;
    }
    // 为目录申请块
    int32_t block_bitmap_idx;
    block_bitmap_idx = bitmap_alloc(&cur_part->block_bitmap, 1);
    if (block_bitmap_idx == -1)
    {
        printk("sys_mkdir: block_bitmap_alloc for create directory failed\n");
        rollback_step = 2;
    }
    bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP); // 每分配一个块就同步一次 block_bitmap

    inode new_inode;
    inode_init(inode_nr, &new_inode);
    new_inode.i_blocks[0] = data_block_lba(cur_part, block_bitmap_idx);

    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    dir_entry *dentry = (dir_entry *)io_buf;

    memset(io_buf, 0, BLOCK_SIZE * 2);
    memcpy(dentry->filename, ".", 1);
    dentry->f_type = FS_DIRECTORY;
    dentry->i_nr = inode_nr;
    new_inode.i_size += dir_entry_size;

    dir *parent_dir = record.parent_dir;
    dentry++;
    memcpy(dentry->filename, "..", 2);
    dentry->f_type = FS_DIRECTORY;
    dentry->i_nr = parent_dir->inode->i_nr;
    new_inode.i_size += dir_entry_size;

    disk_write(cur_part->belong_to_disk, io_buf, new_inode.i_blocks[0], 1);
    // 在父目录中添加目录项
    dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(dir_entry));
    char *dirname = strrchr(record.searched_path, '/') + 1; // 目录名称后可能会有字符'/',record.searched_path，无'/'
    create_dir_entry(dirname, inode_nr, FS_DIRECTORY, &new_dir_entry);

    memset(io_buf, 0, BLOCK_SIZE * 2);
    if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf))
    {
        printk("sys_mkdir: sync_dir_entry to disk failed!\n");
        rollback_step = 2;
        goto rollback;
    }

    // 同步父目录 inode
    inode_sync(cur_part, parent_dir->inode, io_buf);

    // 同步新目录 inode
    inode_sync(cur_part, &new_inode, io_buf);

    // 同步 inode 位图
    bitmap_sync(cur_part, inode_nr, INODE_BITMAP);
    sys_free(io_buf);
    dir_close(parent_dir);
    return 0;
rollback:
    switch (rollback_step)
    {
    case 2:
        bitmap_free(&cur_part->inode_bitmap, inode_nr, 1);
    case 1:
        dir_close(record.parent_dir);
        sys_free(io_buf);
    }
    return -1;
}

// 打开 dir ，成功返回 dir* ，失败返回 NULL
dir *sys_opendir(const char *name)
{
    ASSERT(strlen(name) < MAX_FILE_NAME_LEN);
    path_search_record record;
    memset(&record, 0, sizeof(path_search_record));
    int32_t inode_nr = search_file(name, &record);
    dir *ret = NULL;
    if (inode_nr == -1)
    {
        printk("In %s, sub path %s not exist\n", name, record.searched_path);
    }
    else if (record.f_type != FS_DIRECTORY)
    {
        printk("%s is not directory!\n", name);
    }
    else
    {
        ret = dir_open(cur_part, inode_nr);
    }
    dir_close(record.parent_dir);
    return ret;
}

// 关闭 dir ，成功返回 0 ，失败返回 -1
int32_t sys_closedir(dir *d)
{
    int32_t ret = -1;
    if (d)
    {
        dir_close(d);
        ret = 0;
    }
    return ret;
}

// 根据 dir_pos 读取一个 dir_entry, 成功返回  dir_entry*，失败返回 NULL
dir_entry *sys_readdir(dir *dir)
{
    ASSERT(dir != NULL);
    return dir_read(dir);
}

/* 把目录 dir 的指针 dir_pos 置 0 */
void sys_rewinddir(dir *dir)
{
    dir->dir_pos = 0;
}

int32_t sys_rmdir(const char *pathname)
{

    path_search_record record;
    memset(&record, 0, sizeof(path_search_record));
    int32_t retval = -1;
    int32_t inode_nr = search_file(pathname, &record);
    if (inode_nr == -1)
        retval = -1;
    else
    {
        if (record.f_type == FS_FILE)
            printk("%s is regular file!\n", pathname);
        else
        {
            dir *pdir = dir_open(cur_part, inode_nr);
            ASSERT(pdir != NULL);
            if (dir_is_empty(pdir))
            {

                if (dir_remove(record.parent_dir, pdir) == 0)
                    retval = 0;
                else
                    printk("dir %s is not empty, it is not allowed to delete a nonempty directory!\n", pathname);
            }

            dir_close(pdir);
        }
    }

    dir_close(record.parent_dir);

    return retval;
}

// 获取当前工作目录
char *sys_getcwd(char *buf, uint32_t size)
{
    ASSERT(buf != NULL);

    char full_path_reverse[MAX_PATH_LEN] = {0};

    uint32_t cur_inode_nr = thread_running()->cwd_inode_nr;
    if (cur_inode_nr == 0)
    {
        strcpy(buf, "/");
        return buf;
    }
    uint32_t parent_inode_nr;
    dir_entry parent_dir_entry, cur_dir_entry;

    while (cur_inode_nr)
    {
        dir *cur_dir = dir_open(cur_part, cur_inode_nr);
        if (cur_dir == NULL)
            return NULL;
        // 找到当前目录父目录的inode_nr
        if (!search_dir_entry(cur_part, cur_dir, "..", &parent_dir_entry))
            return NULL;
        dir_close(cur_dir);
        parent_inode_nr = parent_dir_entry.i_nr;
        // 在父目录中查找当前目录的目录名称
        dir *parent_dir = dir_open(cur_part, parent_inode_nr);
        if (parent_dir == NULL)
            return NULL;
        if (!search_dir_entry_by_inode(cur_part, parent_dir, cur_inode_nr, &cur_dir_entry))
            return NULL;
        dir_close(parent_dir);
        // 将当前目录名称写入 path
        strcat(full_path_reverse, "/");
        strcat(full_path_reverse, cur_dir_entry.filename);

        cur_inode_nr = parent_inode_nr;
    }
    ASSERT(strlen(full_path_reverse) <= size);
    memset(buf, 0, size);
    char *last_slash;
    while (last_slash = strrchr(full_path_reverse, '/'))
    {
        uint32_t len = strlen(buf);
        strcpy(buf + len, last_slash);
        *last_slash = 0;
    }
    return buf;
}

/* 更改当前工作目录为绝对路径 path，成功则返回 0，失败返回-1 */
int32_t sys_chdir(const char *path)
{
    int32_t retval = -1;
    path_search_record record;
    memset(&record, 0, sizeof(path_search_record));
    int32_t i_nr = search_file(path, &record);
    if (i_nr != -1)
    {
        thread_running()->cwd_inode_nr = i_nr;
        retval = 0;
    }
    else
        printk("sys_chdir: %s is regular file or other!\n", path);
    dir_close(record.parent_dir);
    return retval;
}

 /*查看文件结构相关信息，成功时返回 0，失败返回-1 */ 
int32_t sys_stat(const char *path, stat *st)
{
    if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/.."))
    {
        st->st_i_nr = root_dir.inode->i_nr;
        st->st_size = root_dir.inode->i_size;
        st->st_ftype = FS_DIRECTORY;
        return 0;
    }

    int32_t retval = -1;
    path_search_record record;
    memset(&record, 0, sizeof(path_search_record));

    int32_t inode_nr = search_file(path, &record);
    if (inode_nr != -1)
    {
        inode *_node = inode_open(cur_part, inode_nr);
        st->st_size = _node->i_size;
        inode_close(_node);
        st->st_ftype = record.f_type;
        st->st_i_nr = inode_nr;
        retval = 0;
    }
    else
        printk("sys_stat: %s not found\n", path);
    dir_close(record.parent_dir);
    return retval;
}