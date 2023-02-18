#include "fs.h"
#include "file.h"
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
            printk("%x\n", &root_dir);

            printk("parent %d,file %s flag: %d\n", record.parent_dir->inode, _filename, flag);

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