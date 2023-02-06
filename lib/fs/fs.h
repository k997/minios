#ifndef __LIB_FS_H
#define __LIB_FS_H

#include "stdint.h"
#include "list.h"
#include "ide.h"

#define MAX_FILE_NAME_LEN 16
#define MAX_FILES_PER_PART 4096
#define SUPER_BLOCK_MAGIC 0x19590318

typedef enum FS_TYPE
{
    FS_UNKNOW,
    FS_FILE,
    FS_DIRECTORY
} FS_TYPE;

/* 目录结构 */
struct dir
{
    struct inode *inode;
    uint32_t dir_pos;     // 记录在目录 inode 内的偏移
    uint8_t dir_buf[512]; // 目录的数据缓存
};

/* 目录项结构 */
typedef struct dir_entry
{
    char filename[MAX_FILE_NAME_LEN]; // 普通文件或目录名称
    uint32_t i_nr;                    // 普通文件或目录对应的 inode 编号
    enum FS_TYPE f_type;              // 文件类型
} dir_entry;

struct superblock
{
    uint32_t magic;
    uint32_t sec_cnt;       // 本分区总共的扇区数
    uint32_t inode_cnt;     // 本分区中 inode 数量
    uint32_t part_lba_base; // 本分区的起始 lba 地址

    uint32_t block_bitmap_lba;       // 块位图本身起始扇区地址
    uint32_t block_bitmap_block_cnt; // 扇区位图本身占用的 bit 长度

    uint32_t inode_bitmap_lba;       // i 结点位图起始扇区 lba 地址
    uint32_t inode_bitmap_block_cnt; // i 结点位图占用的 bit 长度

    uint32_t inode_table_lba;       // i 结点表起始扇区 lba 地址
    uint32_t inode_table_block_cnt; // i 结点表占用的 字节 长度

    uint32_t data_start_lba; // 数据区开始的第一个扇区号
    uint32_t root_inode_nr;  // 根目录所在的 i 结点号
    uint32_t dir_entry_size; // 目录项大小

    uint8_t pad[BLOCK_SIZE - 13 * 4]; // 13个字段*4字节
                                      // 必须为block_size ，主要是为了写回硬盘方便
} __attribute__((packed));
typedef struct superblock superblock;

typedef struct inode
{
    uint32_t i_nr; // inode 编号

    /* 当此 inode 是文件时， i_size 是指文件大小,
    若此 inode 是目录， i_size 是指该目录下所有目录项大小之和*/
    uint32_t i_size;

    uint32_t i_open_cnts; // 记录此文件被打开的次数
    bool write_deny;      // 写文件不能并行，进程写文件前检查此标识

    /* i_sectors[0-11]是直接块， i_sectors[12]用来存储一级间接块指针
    数组中存放的是 block 的编号
     */
    uint32_t i_blocks[13];
    struct list_elem inode_tag;
} inode;

void init_superblock_for_raw_partition(superblock *sb, const partition *part);
void fs_init();
void format_partition(partition *part);
void format_all_partition();
void mount_partition(char *partname);
#endif
