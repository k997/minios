#include "fs.h"
#include "debug.h"
#include "string.h"
#include "memory.h"
#include "kstdio.h"

static bool __mount_partition(list_elem *e, int arg);
partition *cur_part;

void mount_partition(char *partname)
{
    list_traversal(&partition_list, __mount_partition, (int)partname);
}

// 挂载分区即把所有分区的 superblock 读取到内存
static bool __mount_partition(list_elem *e, int arg)
{
    char *default_part_name = (char *)arg;
    partition *ppart = container_of(partition, partition_tag, e);

    if (strcmp(ppart->name, default_part_name))
        return false;
    cur_part = ppart;
    disk *hd = ppart->belong_to_disk;
    // 读取超级块
    cur_part->sb = (struct superblock *)sys_malloc(BLOCK_SIZE);
    if (cur_part->sb == NULL)
        PANIC("malloc failed when mount partition\n");
    memset(cur_part->sb, 0, BLOCK_SIZE);
    disk_read(hd, cur_part->sb, ppart->start_lba + 1, 1);

    superblock *sb = cur_part->sb;
    // 读取 blockbitmap
    ppart->block_bitmap.byte_length = sb->block_bitmap_block_cnt * BLOCK_SIZE;
    ppart->block_bitmap.bits = (uint8_t *)sys_malloc(ppart->block_bitmap.byte_length);
    disk_read(hd, ppart->block_bitmap.bits, sb->block_bitmap_lba, sb->block_bitmap_block_cnt);

    // 读取 inodebitmap
    ppart->inode_bitmap.byte_length = sb->inode_bitmap_block_cnt * BLOCK_SIZE;
    ppart->inode_bitmap.bits = (uint8_t *)sys_malloc(ppart->inode_bitmap.byte_length);
    disk_read(hd, ppart->inode_bitmap.bits, sb->inode_bitmap_lba, sb->inode_bitmap_block_cnt);

    list_init(&ppart->open_inodes);
    printk("mount %s done!\n", cur_part->name);

    // 此处 false 只是为了让主调函数 list_traversal 继续向下遍历元素
    return true;
}
