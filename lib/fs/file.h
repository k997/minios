#ifndef __LIB_FILE_H
#define __LIB_FILE_H
#include "stdint.h"
#include "fs.h"

#define MAX_FILE_OPEN 32

typedef struct file
{
    uint32_t fd_pos;        // 当前文件操作的偏移地址
    uint32_t fd_flag;       // 文件读写权限标识
    struct inode *fd_inode; // 指向 part-> open_inodes 中的 inode
} file;
typedef enum std_fd
{
    STD_IN,
    STD_OUT,
    STD_ERR
} std_fd;

/* 打开文件的选项 */
typedef enum oflags
{
    O_R_ONLY,   // 只读
    O_W_ONLY,   // 只写
    O_RW,       // 读写
    O_CREAT = 4 // 创建
} oflags;



void global_open_file_table_init();
file* fd_to_file(uint32_t fd);
int32_t file_open(uint32_t inode_nr, oflags flag);
int32_t file_close(file *f);
int32_t file_create(dir *parent_dir, char *filename, oflags flag);
int32_t file_write(file *f, const void *buf, uint32_t nbytes);
int32_t file_read(file *f, void *buf, uint32_t nbytes);

#endif
