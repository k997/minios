
#include "fs.h"

// bitmap 第 bit_idx 位所在的 BLOCKSIZE 字节同步到硬盘
void bitmap_sync(partition *part, uint32_t idx, bitmap_type btmp_type)
{
    uint32_t block_offset = idx / BLOCK_SIZE * 8;
    uint32_t byte_offset = block_offset * BLOCK_SIZE;

    uint32_t lba;
    uint8_t *buf;
    switch (btmp_type)
    {
    case INODE_BITMAP:
        lba = part->sb->inode_bitmap_lba;
        buf = part->inode_bitmap.bits + byte_offset;
        break;

    case BLOCK_BITMAP:
        lba = part->sb->block_bitmap_lba;
        buf = part->block_bitmap.bits + byte_offset;
        break;
    }
    disk_write(part->belong_to_disk, buf, lba, 1);
}

// 根据 bitmap idx 返回 block 的 lba
uint32_t data_block_lba(partition *part, uint32_t block_bitmap_idx)
{
    return part->sb->data_start_lba + block_bitmap_idx * SECTORS_PER_BLOCK;
}

// 根据 lba 返回 block bitmap idx
uint32_t data_block_bitmap_idx(partition *part, uint32_t lba)
{
    return lba - part->sb->data_start_lba;
}