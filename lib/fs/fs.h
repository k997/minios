#ifndef __LIB_FS_H
#define __LIB_FS_H

#include "stdint.h"
#include "list.h"
#include "ide.h"

#define MAX_FILE_NAME_LEN 16
#define MAX_FILES_PER_PART 4096
#define SUPER_BLOCK_MAGIC 0x19590318
#define MAX_PATH_LEN 512
#define MAX_FILE_OPEN 32

typedef enum FS_TYPE
{
    FS_UNKNOWN,
    FS_FILE,
    FS_DIRECTORY
} FS_TYPE;

/* 目录结构 */
typedef struct dir
{
    struct inode *inode;
    uint32_t dir_pos;            // 记录在目录 inode 内的偏移
    uint8_t dir_buf[BLOCK_SIZE]; // 目录的数据缓存
} dir;

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
    /*
    superblock 在磁盘上占用 BLOCKSIZE 字节（ 1 个块），在内存中占用 sizeof(superblock)
    为了避免 superblock 二者空间不一致导致的读写问题，将 superblock 设置为 blocksize
    disk_read 每次读取 N 块，可能将内存中 superblock 之后的其他数据覆盖
    */
    uint8_t pad[BLOCK_SIZE - 13 * 4]; // 13个字段*4字节
    // 必须为block_size ，主要是为了读写方便

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
    文件最大容量是 (直接块数量 + 块大小 / block编号空间大小) * 块尺寸
    (12 + BLOCK_SIZE /sizeof(uint32_t)) * BLOCK_SIZE
    数组中存放的是 block 的 lba
     */
    uint32_t i_blocks[13];
    struct list_elem inode_tag;
} inode;

// 记录查找过程中的路径
typedef struct path_search_record
{
    char searched_path[MAX_PATH_LEN];
    dir *parent_dir;
    FS_TYPE f_type;
} path_search_record;

typedef enum bitmap_type
{
    INODE_BITMAP, // inode 位图
    BLOCK_BITMAP  // 块位图
} bitmap_type;

/* 文件属性结构体 */
typedef struct stat
{
    uint32_t st_i_nr;      // inode 编号
    uint32_t st_size;      // 尺寸
    enum FS_TYPE st_ftype; // 文件类型
} stat;
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

extern partition *cur_part;
extern dir root_dir;
extern file global_open_file_table[MAX_FILE_OPEN];

void fs_init();

void format_partition(partition *part);
void format_all_partition();
void mount_partition(char *partname);

void inode_sync(partition *part, inode *node, void *io_buf);
inode *inode_open(partition *part, uint32_t inode_nr);
void inode_close(inode *_inode);
void inode_init(uint32_t inode_nr, struct inode *new_inode);
void inode_release(partition *part, uint32_t inode_nr);
void collect_inode_datablock_lba_table(partition *part, inode *node, uint32_t *block_lba_table);
int32_t indirect_block_alloc(partition *part, inode *node, void *io_buf);

uint32_t data_block_lba(partition *part, uint32_t block_bitmap_idx);
uint32_t data_block_bitmap_idx(partition *part, uint32_t lba);
void bitmap_sync(partition *part, uint32_t idx, bitmap_type btmp_type);

void open_root_dir(partition *part);
dir *dir_open(partition *part, uint32_t inode_nr);
void dir_close(dir *dir);
dir_entry *dir_read(dir *dir);
int32_t dir_remove(dir *parent_dir, dir *child_dir);
bool dir_is_empty(dir *pdir);

void global_open_file_table_init();
file *fd_to_file(uint32_t fd);
int32_t file_open(uint32_t inode_nr, oflags flag);
int32_t file_close(file *f);
int32_t file_create(dir *parent_dir, char *filename, oflags flag);
int32_t file_write(file *f, const void *buf, uint32_t nbytes);
int32_t file_read(file *f, void *buf, uint32_t nbytes);

void create_dir_entry(char *filename, uint32_t inode_nr, FS_TYPE f_type, dir_entry *pde);
bool search_dir_entry(partition *part, dir *pdir, const char *name, dir_entry *dir_e);
bool search_dir_entry_by_inode(partition *part, dir *pdir, int inode_nr, dir_entry *dir_e);
bool sync_dir_entry(dir *parent_dir, dir_entry *d_e, void *buf);
bool delete_dir_entry(partition *part, dir *pdir, uint32_t inode_nr, void *buf);

uint32_t path_depth(char *path);
int search_file(const char *path, path_search_record *record);

int32_t sys_open(const char *path, uint8_t flag);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void *buf, uint32_t nbytes);
int32_t sys_read(int32_t fd, void *buf, uint32_t nbytes);
int32_t sys_unlink(const char *pathname);
int32_t sys_mkdir(const char *pathname);
dir *sys_opendir(const char *name);
int32_t sys_closedir(dir *d);
dir_entry *sys_readdir(dir *dir);
void sys_rewinddir(dir *dir);
int32_t sys_rmdir(const char *pathname);
int32_t sys_chdir(const char *path);
char *sys_getcwd(char *buf, uint32_t size);
int32_t sys_stat(const char *path, stat *st);

#endif
