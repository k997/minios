#include "fs.h"
#include "debug.h"
#include "ide.h"
#include "string.h"

typedef struct inode_position {
    bool cross_block;        // inode 是否跨块
    uint32_t lba;            // inode 所在块号
    uint32_t offset_inblock; // inode 在 block 中偏移
} inode_position;

static void inode_locate(partition *part, uint32_t inode_nr, inode_position *inode_p);

static void inode_locate(partition *part, uint32_t inode_nr, inode_position *inode_p) {
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
void inode_sync(partition *part, inode *node, void *io_buf) {
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

inode *inode_open(partition *part, uint32_t inode_nr) {
    list_elem *elem;
    inode *inode_found;
    for (elem = part->open_inodes.head.next; elem != &part->open_inodes.tail; elem = elem->next) {
        inode_found = container_of(inode, inode_tag, elem);
        if (inode_found->i_nr == inode_nr) {
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

void inode_close(inode *_inode) {
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
void inode_init(uint32_t inode_nr, struct inode *new_inode) {
    new_inode->i_nr = inode_nr;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;
    uint8_t idx;
    for (idx = 0; idx < 13; idx++) {
        /* i_blocks[12]为一级间接块地址 */
        new_inode->i_blocks[idx] = 0;
    }
}