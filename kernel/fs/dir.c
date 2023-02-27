#include "fs.h"
#include "memory.h"
#include "kstdio.h"
#include "string.h"
#include "debug.h"

dir root_dir;

// 加载 root 所在的 inode 到内存
void open_root_dir(partition *part)
{
    root_dir.inode = inode_open(part, part->sb->root_inode_nr);
    root_dir.dir_pos = 0;
}

// 加载 dir 所在的 inode 到内存
dir *dir_open(partition *part, uint32_t inode_nr)
{
    dir *d = (dir *)sys_malloc(sizeof(dir));
    d->inode = inode_open(part, inode_nr);
    d->dir_pos = 0;
    return d;
}

void dir_close(dir *dir)
{
    /************* 根目录不能关闭 ***************
1 根目录自打开后就不应该关闭，否则还需要再次 open_root_dir();
2 root_dir 所在的内存是低端 1MB 之内，并非在堆中， free 会出问题 */

    if (dir->inode->i_nr == root_dir.inode->i_nr)
        return;
    inode_close(dir->inode);
    sys_free(dir);
}

void create_dir_entry(char *filename, uint32_t inode_nr, FS_TYPE f_type, dir_entry *pde)
{
    ASSERT(strlen(filename) <= MAX_FILE_NAME_LEN);
    memcpy(pde->filename, filename, strlen(filename));
    pde->i_nr = inode_nr;
    pde->f_type = f_type;
}

// 根据文件名称查找对应的 dir entry
bool search_dir_entry(partition *part, dir *pdir, const char *name, dir_entry *dir_e)
{
    /*
    1. 先读取 dir
    2. 查找 dir_entry
     */
    // dir->inode->i_blocks[13] 本身有 13个元素，第 13 个元素不直接存放数据，因此没有为其申请空间
    // dir inode 中总共记录了 12 + BLOCK_SIZE /sizeof(uint32_t) 个数据 block 的 lba
    uint32_t max_block_cnt = 12 + BLOCK_SIZE / sizeof(uint32_t);
    // dir inode 中所有 blocks 的编号，相当于 uint32_t all_block[max_block_cnt]
    // uint32_t *all_blocks = (uint32_t *)sys_malloc(max_block_cnt * sizeof(uint32_t));
    uint32_t *all_data_blocks = (uint32_t *)sys_malloc(48 + BLOCK_SIZE);
    if (all_data_blocks == NULL)
    {
        printk("search_dir_entry: sys_malloc for all_blocks failed");
        return false;
    }

    // 已获取 dir 的 inode 中记录的所有扇区编号，开始逐扇区查找 dir_entry
    uint8_t *buf = (uint8_t *)sys_malloc(BLOCK_SIZE);
    if (buf == NULL)
    {
        printk("search_dir_entry: sys_malloc for io buf failed");
        sys_free(all_data_blocks);
        return false;
    }

    memset(all_data_blocks, 0, 48 + BLOCK_SIZE);
    collect_inode_datablock_lba_table(part, pdir->inode, all_data_blocks);

    dir_entry *p_de;
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    uint32_t dir_entry_idx;
    uint32_t block_idx;

    for (block_idx = 0; block_idx < max_block_cnt; block_idx++)
    {
        // 为空则跳过
        if (all_data_blocks[block_idx] == 0)
            continue;
        memset(buf, 0, BLOCK_SIZE);
        // 读取 all_blocks[block_idx] 指向的 block
        disk_read(part->belong_to_disk, buf, all_data_blocks[block_idx], 1);
        // block 内查找 dir entry
        for (dir_entry_idx = 0, p_de = (dir_entry *)buf; dir_entry_idx < dir_entry_cnt; dir_entry_idx++, p_de++)
        {
            if (!strcmp(p_de->filename, name))
            {
                memcpy(dir_e, p_de, dir_entry_size);
                sys_free(buf);
                sys_free(all_data_blocks);
                return true;
            }
        }
    }
    sys_free(buf);
    sys_free(all_data_blocks);
    return false;
}

/*
同步 dir entry 到硬盘
同步父目录 inode 时 inode 可能跨两个 block，buf 至少要两倍 block_size 大小
 */
bool sync_dir_entry(dir *parent_dir, dir_entry *d_e, void *buf)
{
    inode *parent_dir_inode = parent_dir->inode;
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    ASSERT(parent_dir_inode->i_size % dir_entry_size == 0);
    uint32_t dir_entrys_per_block = BLOCK_SIZE / dir_entry_size;

    // dir->inode->i_blocks[13] 本身有 13个元素，第 13 个元素为间接索引表，不直接存放数据，因此没有为其申请空间
    // dir inode 中总共记录了 12 + BLOCK_SIZE /sizeof(uint32_t) 个数据 block 的 lba
    uint32_t block_cnt = 12 + BLOCK_SIZE / sizeof(uint32_t);
    // 12 个直接块 和 BLOCK_SIZE / sizeof(uint32_t) 个间接块
    uint32_t *all_data_blocks = (uint32_t *)sys_malloc(4 * block_cnt);
    if (all_data_blocks == NULL)
    {
        printk("sync_dir_entry: sys_malloc for all_data_blocks failed");
        return false;
    }
    memset(all_data_blocks, 0, 4 * block_cnt);
    // 初始化 all block
    collect_inode_datablock_lba_table(cur_part, parent_dir_inode, all_data_blocks);
    uint32_t block_idx;
    for (block_idx = 0; block_idx < block_cnt; block_idx++)
    {
        // 间接块未分配则分配间接块
        if (block_idx == 12 && parent_dir_inode->i_blocks[12] == 0)
        {
            int32_t indirect_block_lba = indirect_block_alloc(cur_part, parent_dir_inode, buf);
            if (indirect_block_lba == -1)
            {
                printk("sync_dir_entry: indirect_block_alloc failed");
                sys_free(all_data_blocks);
                return false;
            }
        }
        // 若第block_idx块已存在且是有效的数据块，将其读进内存,然后在该块中查找空目录项
        if (all_data_blocks[block_idx] != 0)
        {
            disk_read(cur_part->belong_to_disk, buf, all_data_blocks[block_idx], 1);
            uint32_t dir_entry_idx;
            dir_entry *cur_dir_entry;
            for (dir_entry_idx = 0, cur_dir_entry = (dir_entry *)buf; dir_entry_idx < dir_entrys_per_block; dir_entry_idx++, cur_dir_entry++)
            {
                // 未分配的dir entry
                if (cur_dir_entry->f_type == FS_UNKNOWN)
                {
                    memcpy(cur_dir_entry, d_e, dir_entry_size);
                    disk_write(cur_part->belong_to_disk, buf, all_data_blocks[block_idx], 1);
                    // 更新父目录 dir size
                    parent_dir_inode->i_size += dir_entry_size;
                    // 同步父目录 inode
                    memset(buf, 0, BLOCK_SIZE * 2);
                    inode_sync(cur_part, parent_dir_inode, buf);
                    sys_free(all_data_blocks);
                    return true;
                }
            }
        }
        // block 未分配
        else
        {
            // 若第block_idx块不存在，则先分配 block ，然后从新 block 中为其分配 dir_entry
            int32_t block_bitmap_idx = bitmap_alloc(&cur_part->block_bitmap, 1);
            if (block_bitmap_idx < 0)
            {
                printk("alloc block bitmap for sync_dir_entry failed!\n");
                sys_free(all_data_blocks);
                return false;
            }
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
            all_data_blocks[block_idx] = data_block_lba(cur_part, block_bitmap_idx);

            // 先分配block，且分配的块是直接块
            if (block_idx < 12)
            {
                // 同步父目录的 inode 信息
                // parent_dir_inode->i_blocks 本身修改随后续 isize 修改后一起写入磁盘
                parent_dir_inode->i_blocks[block_idx] = all_data_blocks[block_idx];
            }
            else
            {
                // 父目录的 inode 与间接块无关，直接同步间接块分配情况到磁盘
                disk_write(cur_part->belong_to_disk, all_data_blocks + 12, parent_dir_inode->i_blocks[12], 1);
            }
            // 新块数据初始化为 0
            memset(buf, 0, BLOCK_SIZE);
            // 新 block 中写入 dir entry
            memcpy(buf, d_e, dir_entry_size);
            disk_write(cur_part->belong_to_disk, buf, all_data_blocks[block_idx], 1);
            parent_dir_inode->i_size += dir_entry_size;
            // 同步父目录 inode
            inode_sync(cur_part, parent_dir_inode, buf);
            sys_free(all_data_blocks);
            return true;
        }
    }
    sys_free(all_data_blocks);
    printk("directory is full!\n");
    return false;
}

/*
把分区part目录pdir中编号为inode_nr的目录项删除
同步 inode 时 inode 可能跨两个 block，buf 至少要两倍 block_size 大小
 */
bool delete_dir_entry(partition *part, dir *pdir, uint32_t inode_nr, void *buf)
{
    inode *dir_inode = pdir->inode;
    uint32_t dir_entrys_per_block = BLOCK_SIZE / part->sb->dir_entry_size;
    // dir->inode->i_blocks[13] 本身有 13个元素，第 13 个元素不直接存放数据，因此没有为其申请空间
    // dir inode 中总共记录了 12 + BLOCK_SIZE /sizeof(uint32_t) 个数据 block 的 lba
    uint32_t all_data_block_cnt = 12 + BLOCK_SIZE / sizeof(uint32_t);
    uint32_t *all_data_blocks = (uint32_t *)sys_malloc(4 * all_data_block_cnt);
    memset(all_data_blocks, 0, 4 * all_data_block_cnt);

    // 复制所有数据 block 的 lba
    uint32_t blk_idx;
    for (blk_idx = 0; blk_idx < 12; blk_idx++) // 直接块
        all_data_blocks[blk_idx] = dir_inode->i_blocks[blk_idx];

    if (dir_inode->i_blocks[12]) // 若有间接块
        disk_read(part->belong_to_disk, all_data_blocks + 12, dir_inode->i_blocks[12], 1);

    bool is_dir_first_block; // 目录的第一个块
    uint32_t dir_entry_idx, dir_entry_cnt;
    dir_entry *cur_dir_entry;
    dir_entry *dir_entry_found = NULL;

    for (blk_idx = 0; blk_idx < all_data_block_cnt; blk_idx++)
    {
        if (all_data_blocks[blk_idx] == 0)
            continue;

        memset(buf, 0, BLOCK_SIZE);                                        // 清空缓冲区
        disk_read(part->belong_to_disk, buf, all_data_blocks[blk_idx], 1); // 读取 block

        dir_entry_cnt = 0; // 用于统计该 block 下目录项的数量，以方便回收 block
        is_dir_first_block = false;
        // 遍历 block 下所有目录项
        for (dir_entry_idx = 0, cur_dir_entry = (dir_entry *)buf; dir_entry_idx < dir_entrys_per_block; dir_entry_idx++, cur_dir_entry++)
        {
            if (cur_dir_entry->f_type == FS_UNKNOWN) // 无效的目录项
                continue;

            // 每个 dir 的第一个目录项必定是 ., 因此当前读取的块必定是该目录的第一个块
            if (!strcmp(cur_dir_entry->filename, "."))
            {
                is_dir_first_block = true;
                continue;
            }
            // 跳过 '.' 和 '..'，只统计子文件
            if (strcmp(cur_dir_entry->filename, ".") && strcmp(cur_dir_entry->filename, ".."))
            {
                dir_entry_cnt++;
                if (cur_dir_entry->i_nr == inode_nr)
                {
                    ASSERT(dir_entry_found == NULL); // 确保目录中只有一个编号为inode_no的inode,找到一次后dir_entry_found就不再是NULL
                    dir_entry_found = cur_dir_entry; // 找到后也继续遍历,统计总共的目录项数
                }
            }
        }

        /* 若此扇区未找到该目录项,继续在下个扇区中找 */
        if (dir_entry_found == NULL)
            continue;

        // 找到目录项必定 >= 1
        ASSERT(dir_entry_cnt >= 1);
        // 当前 block 还有其他目录项或者当前 block 是该文件的第一个 block (要保存 . 和 ..)，因此只删除要删除的目录项
        if (dir_entry_cnt > 1 || is_dir_first_block)
        {
            memset(dir_entry_found, 0, part->sb->dir_entry_size);
            disk_write(part->belong_to_disk, buf, all_data_blocks[blk_idx], 1); // 擦除目录项写回当前 block
        }
        else // 除目录第1个扇区外,若该扇区上只有该目录项自己,则将整个扇区回收
        {
            // 1. 块位图中回收该块
            uint32_t bitmap_idx = data_block_bitmap_idx(part, all_data_blocks[blk_idx]);
            bitmap_free(&part->block_bitmap, bitmap_idx, 1);
            bitmap_sync(part, bitmap_idx, BLOCK_BITMAP);
            // 2. 块地址从数组i_blocks或索引表中去掉
            if (blk_idx < 12) // 2.1 直接快直接从内存回收
            {
                dir_inode->i_blocks[blk_idx] = 0;
            }
            else
            {
                // 2.2 即将回收的块是间接块，统计间接索引表中还有多少有效的数据块
                uint32_t indirect_blocks = 0;
                uint32_t indirect_block_idx;
                for (indirect_block_idx = 12; indirect_block_idx < all_data_block_cnt; indirect_block_idx++)
                {
                    if (all_data_blocks[indirect_block_idx] != 0)
                        indirect_blocks++;
                }
                // 2.3 除了即将回收的块，间接索引表中还包括其它间接块,仅在索引表中擦除当前的间接块地址, 并写入磁盘
                if (indirect_blocks > 1)
                {
                    all_data_blocks[blk_idx] = 0;
                    disk_write(part->belong_to_disk, all_data_blocks + 12, dir_inode->i_blocks[12], 1);
                }
                else // 2.4 即将回收的块是间接索引表中唯一块,直接把间接索引表所在的块回收,然后擦除间接索引表块地址
                {
                    bitmap_idx = data_block_bitmap_idx(part, dir_inode->i_blocks[12]);
                    bitmap_free(&part->block_bitmap, bitmap_idx, 1);
                    bitmap_sync(part, bitmap_idx, BLOCK_BITMAP);

                    dir_inode->i_blocks[12] = 0; // 将间接索引表地址清0
                }
            }
        }
        /* 更新 i 结点信息并同步到硬盘 */
        dir_inode->i_size -= part->sb->dir_entry_size;
        memset(buf, 0, BLOCK_SIZE * 2);
        inode_sync(part, dir_inode, buf);
        sys_free(all_data_blocks);
        return true;
    }
    sys_free(all_data_blocks);
    // 所有块中未找到则返回 false，出现这种情况应该是 serarch_file 出错了
    return false;
}