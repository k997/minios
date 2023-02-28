#include "fs.h"
#include "debug.h"
#include "ide.h"
#include "string.h"
#include "kstdio.h"

typedef struct inode_position
{
    bool cross_block;        // inode 是否跨块
    uint32_t lba;            // inode 所在块号
    uint32_t offset_inblock; // inode 在 block 中偏移
} inode_position;

static void inode_locate(partition *part, uint32_t inode_nr, inode_position *inode_p);

static void inode_locate(partition *part, uint32_t inode_nr, inode_position *inode_p)
{
    // inode 编号最大不会超过 MAX_FILES_PER_PART
    ASSERT(inode_nr < MAX_FILES_PER_PART);
    // inode 所在的 block
    uint32_t blocks_idx = inode_nr * sizeof(inode) / BLOCK_SIZE;
    uint32_t offset_inblocks = inode_nr * sizeof(inode) % BLOCK_SIZE;

    /* 判断此 i 结点是否跨越 2 个block */
    // block 剩余空间是否可以以容下一个 inode
    inode_p->cross_block = (BLOCK_SIZE - offset_inblocks) < sizeof(inode) ? true : false;
    inode_p->lba = part->sb->inode_table_lba + blocks_idx * SECTORS_PER_BLOCK;
    inode_p->offset_inblock = offset_inblocks;
}

/*

一般情况下把内存中的数据同步到硬盘都是最后的操作，其前已经做了大量工作
若到这最后一步（也就是执行到此函数）时才申请内存失败，前面的所有操作都白费了不说，还要回
滚到之前的旧状态，代价太大，因此 io_buf 必须由主调函数提前申请好
 */
void inode_sync(partition *part, inode *node, void *io_buf)
{
    inode_position pos;
    inode_locate(part, node->i_nr, &pos);
    // 去除 inode 中无需同步到磁盘的信息
    inode pure_inode;
    memcpy(&pure_inode, node, sizeof(struct inode));
    pure_inode.i_open_cnts = 0;
    // 置为 false，以保证在硬盘中读出时为可写
    pure_inode.write_deny = false;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

    uint32_t block_cnt = pos.cross_block ? 2 : 1;
    disk_read(part->belong_to_disk, io_buf, pos.lba, block_cnt);
    memcpy(io_buf + pos.offset_inblock, &pure_inode, sizeof(inode));
    disk_write(part->belong_to_disk, io_buf, pos.lba, block_cnt);
}

// 释放 inode 数据及其本身
void inode_release(partition *part, uint32_t inode_nr)
{
    ASSERT(inode_nr < MAX_FILES_PER_PART);
    // 回收数据块
    inode *del_inode = inode_open(part, inode_nr);
    uint32_t max_block_cnt_per_file = 12 + BLOCK_SIZE / sizeof(uint32_t);
    uint32_t size = sizeof(uint32_t) * max_block_cnt_per_file;
    uint32_t file_blocks_lba[size];
    memset(file_blocks_lba, 0, size);

    collect_inode_datablock_lba_table(part, del_inode, file_blocks_lba);

    int32_t bitmap_idx;
    if (del_inode->i_blocks[12] != 0)
    {
        bitmap_idx = data_block_bitmap_idx(part, del_inode->i_blocks[12]);
        bitmap_free(&part->block_bitmap, bitmap_idx, 1);
        bitmap_sync(part, bitmap_idx, BLOCK_BITMAP);
    }
    uint32_t blk_idx;
    for (blk_idx = 0; blk_idx < max_block_cnt_per_file; blk_idx++)
    {
        if (file_blocks_lba[blk_idx] == 0)
            continue;
        bitmap_idx = data_block_bitmap_idx(part, file_blocks_lba[blk_idx]);
        bitmap_free(&part->block_bitmap, bitmap_idx, 1);
        bitmap_sync(part, bitmap_idx, BLOCK_BITMAP);
    }
    // 回收 inode
    bitmap_free(&part->inode_bitmap, inode_nr, 1);
    bitmap_sync(part, bitmap_idx, INODE_BITMAP);
    inode_close(del_inode);
}

void collect_inode_datablock_lba_table(partition *part, inode *node, uint32_t *block_lba_table)
{
    uint32_t blk_idx;
    for (blk_idx = 0; blk_idx < 12; blk_idx++) // 直接块
    {
        block_lba_table[blk_idx] = node->i_blocks[blk_idx];
    }
    if (node->i_blocks[12] != 0)
    {
        disk_read(part->belong_to_disk, block_lba_table + 12, node->i_blocks[12], 1);
    }
}

// 为 inode 分配间接块，分配成功则返回间接块的 lba，失败则返回 -1
int32_t indirect_block_alloc(partition *part, inode *node, void *io_buf)
{
    if (node->i_blocks[12] != 0) // 间接索引表已分配
        return node->i_blocks[12];

    int32_t block_bitmap_idx = bitmap_alloc(&part->block_bitmap, 1);
    if (block_bitmap_idx < 0)
    {
        printk("alloc block bitmap for indirect_block_alloc failed!\n");
        return -1;
    }
    bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP); // 每分配一个块就同步一次 block_bitmap
    node->i_blocks[12] = data_block_lba(part, block_bitmap_idx);
    inode_sync(part,node,io_buf);
    return node->i_blocks[12];
}

inode *inode_open(partition *part, uint32_t inode_nr)
{
    list_elem *elem;
    inode *inode_found;
    for (elem = part->open_inodes.head.next; elem != &part->open_inodes.tail; elem = elem->next)
    {
        inode_found = container_of(inode, inode_tag, elem);
        if (inode_found->i_nr == inode_nr)
        {
            inode_found->i_open_cnts++;
            return inode_found;
        }
    }

    // 已打开 inode 未找到对应的 inode，到硬盘中寻找
    inode_position inode_pos;
    inode_locate(part, inode_nr, &inode_pos);

    // 申请内核空间，使得 inode 可以被所有进程共享
    task_struct *current_task = thread_running();
    uint32_t *pg_dir_backup = current_task->pgdir;
    current_task->pgdir = NULL;
    inode_found = sys_malloc(sizeof(inode));
    current_task->pgdir = pg_dir_backup;

    // 找到后读取
    uint32_t block_cnt = inode_pos.cross_block ? 2 : 1;
    void *inode_buf = sys_malloc(BLOCK_SIZE * block_cnt);
    disk_read(part->belong_to_disk, inode_buf, inode_pos.lba, block_cnt);
    memcpy(inode_found, inode_buf + inode_pos.offset_inblock, sizeof(inode));
    sys_free(inode_buf);

    //  一会很可能要用到此 inode，故将其插入到队首便于提前检索到
    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;

    return inode_found;
}

void inode_close(inode *_inode)
{
    if (--_inode->i_open_cnts == 0)
        list_remove(&_inode->inode_tag);
    // 从内核空间释放 inode
    task_struct *current_task = thread_running();
    uint32_t *pg_dir_backup = current_task->pgdir;
    current_task->pgdir = NULL;
    sys_free(_inode);
    current_task->pgdir = pg_dir_backup;
}

// inode 编号及待初始化的 inode 指针
void inode_init(uint32_t inode_nr, struct inode *new_inode)
{
    new_inode->i_nr = inode_nr;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;
    uint8_t idx;
    for (idx = 0; idx < 13; idx++)
    {
        /* i_blocks[12]为一级间接块地址 */
        new_inode->i_blocks[idx] = 0;
    }
}
