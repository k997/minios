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


#endif
