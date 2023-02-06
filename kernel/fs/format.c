
#include "fs.h"
#include "memory.h"
#include "bitmap.h"
#include "string.h"
#include "kstdio.h"
static bool __format_all(list_elem *e, int arg __attribute__((unused)));


void format_all_partition(){
    list_traversal(&partition_list, __format_all, (int)NULL);

}
static bool __format_all(list_elem *e, int arg __attribute__((unused)))
{
    // 整块读入
    superblock *sb = sys_malloc(BLOCK_SIZE);
    partition *ppart = container_of(partition, partition_tag, e);
    disk_read(ppart->belong_to_disk, sb, ppart->start_lba + 1, 1);
    // 已经存在则不重新格式化
    if (sb->magic == SUPER_BLOCK_MAGIC)
    {
        printk("%s has been formatted,skip\n", ppart->name);
        return false;
    }
    format_partition(ppart);
    // 此处 false 只是为了让主调函数 list_traversal 继续向下遍历元素
    return false;
}

void format_partition(partition *part)
{
    superblock *sb = sys_malloc(sizeof(superblock));
    init_superblock_for_raw_partition(sb, part);
    disk *hd = part->belong_to_disk;
    disk_write(hd, sb, sb->part_lba_base + 1, 1);

    /* 找出数据量最大的元信息，用其尺寸做存储缓冲区*/
    // inode_table = sizeof(inode)*inode 数量，因此 inode table 总比 inode bitmap 大
    // 因此只要比较 inode_table 和 block_bitmap
    uint32_t buf_size = BLOCK_SIZE * (sb->inode_table_block_cnt > sb->block_bitmap_block_cnt ? sb->inode_table_block_cnt : sb->block_bitmap_block_cnt);
    uint8_t *buf = sys_malloc(buf_size);

    bitmap btmp;
    btmp.bits = buf;

    // block bitmap
    btmp.byte_length = sb->block_bitmap_block_cnt * BLOCK_SIZE;
    bitmap_init(&btmp);
    // 第一个 block 分配给根目录
    bitmap_alloc_from(&btmp, 0, 1);
    disk_write(hd, btmp.bits, sb->block_bitmap_lba, sb->block_bitmap_block_cnt);

    // inode bitmap
    btmp.byte_length = sb->inode_bitmap_block_cnt * BLOCK_SIZE;
    bitmap_init(&btmp);
    // 第一个 inode 分配给根目录
    bitmap_alloc_from(&btmp, 0, 1);
    disk_write(hd, btmp.bits, sb->inode_bitmap_lba, sb->inode_bitmap_block_cnt);

    // inode table
    memset(buf, 0, buf_size);
    inode *root_inode = (inode *)buf;
    // 根目录 inode 编号为 0
    root_inode->i_nr = 0;
    // . 和 .. 两个目录
    root_inode->i_size = sb->dir_entry_size * 2;
    // 由于上面的 memset， i_blocks 数组的其他元素都初始化为 0
    root_inode->i_blocks[0] = sb->data_start_lba;
    disk_write(hd, buf, sb->inode_table_lba, sb->inode_table_block_cnt);
    // 将根目录写入 sb.data_start_lba
    memset(buf, 0, buf_size);
    dir_entry *p_dir = (dir_entry *)buf;
    // 根目录本身
    memcpy(p_dir->filename, ".", 1);
    p_dir->f_type = FS_DIRECTORY;
    p_dir->i_nr = 0;

    // 根目录的父目录
    p_dir++;
    memcpy(p_dir->filename, "..", 2);
    p_dir->f_type = FS_DIRECTORY;
    p_dir->i_nr = 0;

    disk_write(hd, buf, sb->data_start_lba, 1);
    printk("%s format done\n", part->name);
    sys_free(buf);
    sys_free(sb);
}

