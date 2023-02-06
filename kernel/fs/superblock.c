#include "fs.h"
#include "kstdio.h"
#include "ide.h"

#define BITS_PER_BLOCK (8 * BLOCK_SIZE)

// 为未格式化分区中创建 superblock
void init_superblock_for_raw_partition(superblock *sb, const partition *part)
{
    uint32_t partition_blks = part->sector_cnt / SECTORS_PER_BLOCK;

    uint32_t boot_sector_blks = 1;
    uint32_t super_block_blks = 1;

    uint32_t inode_bitmap_block_cnt =
        DIV_ROUND_UP(MAX_FILE_NAME_LEN, BITS_PER_BLOCK);

    uint32_t inode_table_size = sizeof(struct inode) * MAX_FILES_PER_PART;
    uint32_t inode_table_block_cnt = DIV_ROUND_UP(inode_table_size,
                                                  BLOCK_SIZE);

    uint32_t used_blks = boot_sector_blks + super_block_blks +
                         +inode_bitmap_block_cnt + inode_table_block_cnt;

    uint32_t free_blks = partition_blks - used_blks;
    uint32_t block_bitmap_blks = DIV_ROUND_UP(free_blks, BITS_PER_BLOCK);
    free_blks = free_blks - block_bitmap_blks;

    /*********************************************************/

    /* 超级块初始化 */
    sb->magic = SUPER_BLOCK_MAGIC;
    sb->sec_cnt = part->sector_cnt;
    sb->inode_cnt = MAX_FILES_PER_PART;
    sb->part_lba_base = part->start_lba;
    // 第 0 块是引导块，第 1 块是超级块
    sb->block_bitmap_lba = sb->part_lba_base + 2 * SECTORS_PER_BLOCK;
    sb->block_bitmap_block_cnt = block_bitmap_blks;

    sb->inode_bitmap_lba = sb->block_bitmap_lba + sb->block_bitmap_block_cnt * SECTORS_PER_BLOCK;
    sb->inode_bitmap_block_cnt = inode_bitmap_block_cnt;

    sb->inode_table_lba = sb->inode_bitmap_lba + sb->inode_bitmap_block_cnt * SECTORS_PER_BLOCK;
    sb->inode_table_block_cnt = inode_table_block_cnt;

    sb->data_start_lba = sb->inode_table_lba + sb->inode_table_block_cnt * SECTORS_PER_BLOCK;
    sb->root_inode_nr = 0;
    sb->dir_entry_size = sizeof(struct dir_entry);

    printk("%s info:\n", part->name);
    printk(" magic:0x%x\n"
           "part_lba_base:0x%x\n"
           "all_sectors : 0x%x\n"
           "inode_cnt : 0x%x\n"
           "block_bitmap_lba : 0x%x\n"
           "block_bitmap_sectors : 0x%x\n"
           "inode_bitmap_lba : 0x%x\n"
           "inode_bitmap_sectors : 0x%x\n"
           "inode_table_lba : 0x %x\n"
           "inode_table_sectors : 0x%x\n"
           "data_start_lba : 0x%x\n",
           sb->magic,
           sb->part_lba_base, sb->sec_cnt, sb->inode_cnt,
           sb->block_bitmap_lba, sb->block_bitmap_block_cnt, sb->inode_bitmap_lba,
           sb->inode_bitmap_block_cnt, sb->inode_table_lba,
           sb->inode_table_block_cnt, sb->data_start_lba);
}