/*
（1）选择通道，
（2）sector count 寄存器中写入待操作的扇区数。
（3）三个 LBA 寄存器写入扇区起始地址的低 24 位。
（4）device 寄存器中写入 LBA 地址的 24～27 位，并置第 6 位为 1，使其为 LBA 模式，设置第 4位，选择操作的硬盘（master 硬盘或 slave 硬盘）。
（5）command 寄存器写入操作命令。
（6）读取该通道上的 status 寄存器，判断硬盘工作是否完成。
（7）如果以上步骤是读硬盘，进入下一个步骤。否则，结束。
（8）将硬盘数据读出。
 */
#include "ide.h"
#include "global.h"
#include "debug.h"
#include "stdio.h"
#include "kstdio.h"
#include "io.h"
#include "timer.h"
#include "string.h"
#include "interrupt.h"

#define DISK_CNT_PTR 0x475
#define SECTOER_SIZE 512

/* 定义硬盘各寄存器的端口号 */
#define reg_data(channel) (channel->port_base + 0)
#define reg_error(channel) (channel->port_base + 1)
#define reg_sect_cnt(channel) (channel->port_base + 2)
#define reg_lba_l(channel) (channel->port_base + 3)
#define reg_lba_m(channel) (channel->port_base + 4)
#define reg_lba_h(channel) (channel->port_base + 5)
#define reg_dev(channel) (channel->port_base + 6)
#define reg_status(channel) (channel->port_base + 7)
/* 状态和指令复用同一端口 */
#define reg_cmd(channel) (reg_status(channel))
// reg_alt_status 值总是与 reg_status 相同，但不影响中断
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel) reg_alt_status(channel)

/* reg_status寄存器的一些关键位 */
#define BIT_STAT_BSY 0x80 // 第 7 位 BSY 位为 1 表示硬盘正忙
/* 第 6 位 DRDY，表示硬盘就绪
此位是在对硬盘诊断时用的，表示硬盘检测正常，可以继续执行命令*/
#define BIT_STAT_DRDY 0x40
#define BIT_STAT_DRQ 0x8 // 第 3 位为 1 表示硬盘数据已就绪,可读取

// device 寄存器第 0-3 位用来存储 LBA 地址的第 24～27 位
// device 寄存器第 7 位和第 5 位固定为 1
#define BIT_DEV_MBS 0xa0
// device 寄存器第 6 位为 1 代表启用 LBA 模式，0 代表启用 CHS 模式。
#define BIT_DEV_LBA 0x40
// device 寄存器第 4 位为 0 代表主盘，1 代表从盘
#define BIT_DEV_DEV 0x10

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY 0xec     // identify指令，获取硬盘的信息
#define CMD_READ_SECTOR 0x20  // 读扇区指令
#define CMD_WRITE_SECTOR 0x30 // 写扇区指令

ide_channel channels[CHANNEL_CNT];
uint8_t channel_cnt = 0;
// 总拓展分区起始 lba 地址
uint32_t ext_lba_base = 0;
uint8_t primary_partition_nr = 0, logic_partition_nr = 0;

list partition_list;

void interrupt_ide_handler(uint8_t irq_no);
static void set_disk(ide_channel *ch, ide_device device);
static void set_sector(ide_channel *ch, ide_device device, uint32_t lba, uint8_t sector_cnt);
static void set_cmd(ide_channel *ch, uint8_t cmd);
static bool busy_wait(ide_channel *ch);
static void channel_read(ide_channel *ch, void *buf, uint8_t sector_cnt);
static void channel_write(ide_channel *ch, void *buf, uint8_t sector_cnt);

void ide_init()
{
    uint8_t hd_cnt = *((uint8_t *)DISK_CNT_PTR); // 获取硬盘数量

    // BUG! bios初始化时将硬盘数量写入地址 0x475 处，加载内核时该处内容被内核覆盖
    // ASSERT(hd_cnt > 0);
    channel_cnt = DIV_ROUND_UP(hd_cnt, DISK_CNT_PER_CHANNEL); // 根据硬盘数量获取通道数量
    // printk("%d disks found!\n", hd_cnt);

    list_init(&partition_list);
    ide_channel *ch_ptr;
    uint8_t hd_idx, ch_idx;
    for (ch_idx = 0; ch_idx < CHANNEL_CNT; ch_idx++)
    {
        // 初始化通道
        ch_ptr = &channels[ch_idx];
        sprintf(ch_ptr->name, "ide%d", ch_idx);
        switch (ch_idx)
        {
        case 0:
            ch_ptr->port_base = 0x1f0;
            // 0x20 8259A起始中断向量号
            ch_ptr->irq_no = 0x20 + 14;
            break;
        case 1:
            ch_ptr->port_base = 0x170;
            ch_ptr->irq_no = 0x20 + 15;
            break;
        }
        ch_ptr->expecting_intr = false;
        lock_init(&ch_ptr->lock);         // 初始通道锁
        sema_init(&ch_ptr->disk_done, 0); //
        // 注册中断处理函数
        interrupt_program_register(ch_ptr->irq_no, ch_ptr->name, interrupt_ide_handler);

        // 初始化通道内的硬盘
        for (hd_idx = 0; hd_idx < DISK_CNT_PER_CHANNEL; hd_idx++)
        {
            primary_partition_nr = logic_partition_nr = 0;

            disk *hd = &ch_ptr->device[hd_idx];
            hd->belong_to_channel = ch_ptr;
            hd->device = hd_idx;
            sprintf(hd->name, "sd%c", 'a' + ch_idx * 2 + hd_idx);
            // 状态寄存器为 0 说明硬盘不存在
            if (inb(reg_alt_status(hd->belong_to_channel)) != 0)
            {
                identify_disk(hd);
                partition_scan(hd, 0);
            }
        }
    }
    printk("\nall partition info\n");
    list_traversal(&partition_list, partition_info, (int)NULL);
}

void disk_read(disk *hd, void *buf, uint32_t lba, uint8_t sector_cnt)
{
    ASSERT(sector_cnt > 0);
    ide_channel *ch = hd->belong_to_channel;
    lock_acquire(&ch->lock);
    set_disk(ch, hd->device);

    uint32_t sector_op;       // 每次操作的扇区数
    uint32_t sector_done = 0; // 已完成的扇区数
    while (sector_done < sector_cnt)
    {
        // 每次硬盘操作最多支持 256 个扇区，读写的扇区数超过 256 则分多次读写
        if ((sector_done + 256) <= sector_done)
            sector_op = 256;
        else
            sector_op = sector_cnt - sector_done;

        set_sector(ch, hd->device, lba + sector_done, sector_op);
        set_cmd(ch, CMD_READ_SECTOR);

        // 写入命令后硬盘开始工作，阻塞线程，由硬盘的中断处理程序唤醒
        /* 注意写入和读取阻塞自己的时机
        disk_read 时 set_cmd 硬盘即开始运行
        disk_write 时 set_cmd 后硬盘等待线程的数据才开始运行 */

        sema_down(&ch->disk_done);

        // // 检查硬盘是否可读
        if (!busy_wait(ch))
        {
            char error[64];
            sprintf(error, "%s read sectors %d faild!\n", hd->name, lba + sector_done);
            PANIC(error);
        }

        channel_read(ch, (void *)((uint32_t)buf + sector_done * SECTOER_SIZE), sector_op);
        sector_done += sector_op;
    }

    lock_release(&ch->lock);
}

void disk_write(disk *hd, void *buf, uint32_t lba, uint8_t sector_cnt)
{
    ASSERT(sector_cnt > 0);
    ide_channel *ch = hd->belong_to_channel;
    lock_acquire(&ch->lock);
    set_disk(ch, hd->device);

    uint32_t sector_op;       // 每次操作的扇区数
    uint32_t sector_done = 0; // 已完成的扇区数
    while (sector_done < sector_cnt)
    {
        // 每次硬盘操作最多支持 256 个扇区，读写的扇区数超过 256 则分多次读写

        if ((sector_done + 256) <= sector_done)
            sector_op = 256;
        else
            sector_op = sector_cnt - sector_done;

        set_sector(ch, hd->device, lba + sector_done, sector_op);
        set_cmd(ch, CMD_WRITE_SECTOR);
        // 检查硬盘是否可读
        if (!busy_wait(ch))
        {
            char error[64];
            sprintf(error, "%s write sectors %d faild!\n", hd->name, lba + sector_done);
            PANIC(error);
        }
        channel_write(ch, (void *)((uint32_t)buf + sector_done * SECTOER_SIZE), sector_op);
        // 硬盘响应期间阻塞线程，由硬盘的中断处理程序唤醒
        sema_down(&ch->disk_done);
        sector_done += sector_op;
    }
    lock_release(&ch->lock);
}

void interrupt_ide_handler(uint8_t irq_no)
{
    // 只支持 2 个通道
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    // 根据中断判断是哪个通道
    // 中断号减去中断基址
    uint8_t ch_no = irq_no - 0x2e;
    ide_channel *ch = &channels[ch_no];
    ASSERT(ch->irq_no == irq_no);
    // 读写硬盘前以加锁，无需判断硬盘（事实上也无法判断中断来自通道的哪个硬盘）
    if (ch->expecting_intr)
    {
        ch->expecting_intr = false;
        sema_up(&ch->disk_done);
        // 必须显式通知硬盘控制器此次中断已经处理完成
        inb(reg_status(ch));
    }
}

static void set_disk(ide_channel *ch, ide_device device)
{
    // 第 7 位和第 5 位固定为 1, 启用 LBA 模式
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    // 若是从盘，device 寄存器第 4 位置 1
    if (device == DISK_SECONDARY)
    {
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(ch), reg_device);
}

// 设置 ch 通道中从 lba 起的 sector_cnt 个扇区
static void set_sector(ide_channel *ch, ide_device device, uint32_t lba, uint8_t sector_cnt)
{
    outb(reg_sect_cnt(ch), sector_cnt);
    outb(reg_lba_l(ch), lba);
    outb(reg_lba_m(ch), lba >> 8);
    outb(reg_lba_h(ch), lba >> 16);
    uint8_t device_reg = BIT_DEV_MBS | BIT_DEV_LBA |
                         (device == DISK_SECONDARY ? BIT_DEV_DEV : 0) | lba >> 24;
    outb(reg_dev(ch), device_reg);
}

static void set_cmd(ide_channel *ch, uint8_t cmd)
{
    // 说明中断是有用户命令发起的
    ch->expecting_intr = true;
    outb(reg_cmd(ch), cmd);
}

static void channel_read(ide_channel *ch, void *buf, uint8_t sector_cnt)
{

    /*
        data 寄存器是 8 位寄存器
        每次读写最多是 255 个扇区
        当写入端口值为 0 时，则写入 256 个扇区）
     */
    uint32_t size_in_byte;
    if (sector_cnt == 0)
        size_in_byte = 256 * SECTOER_SIZE;
    else
        size_in_byte = sector_cnt * SECTOER_SIZE;
    // insw 单位为字 16 字节
    insw(reg_data(ch), buf, size_in_byte / 2);
}

static void channel_write(ide_channel *ch, void *buf, uint8_t sector_cnt)
{
    /*
        data 寄存器是 8 位寄存器
        每次读写最多是 255 个扇区
        当写入端口值为 0 时，则写入 256 个扇区
     */
    uint32_t size_in_byte;
    if (sector_cnt == 0)
        size_in_byte = 256 * SECTOER_SIZE;
    else
        size_in_byte = sector_cnt * SECTOER_SIZE;
    // insw 单位为字 16 字节
    outsw(reg_data(ch), buf, size_in_byte / 2);
}

static bool busy_wait(ide_channel *ch)
{
    /*
         ata 手册规定所有的操作都应该在 31 秒内完成
     */
    int time_limit;
    for (time_limit = 30 * 1000; time_limit > 0; time_limit -= 10) /* 每10 ms 检查一次 */
    {
        // busy 位为 1 则其他位无效
        if (!(inb(reg_status(ch)) & BIT_STAT_BSY))
            return (inb(reg_status(ch)) & BIT_STAT_DRQ); // data request 位判断硬盘是否就绪
        else
            sleep_mtime(10);
    }
    return false;
}

// 交换 dst 相邻两个字节，存入 buf
static void swap_pairs_bytes(const char *dst, char *buf, uint32_t len)
{
    uint8_t idx;
    for (idx = 0; idx < len; idx += 2)
    {
        buf[idx + 1] = *dst++;
        buf[idx] = *dst++;
    }
    buf[idx] = '\0';
}
void identify_disk(disk *hd)
{
    char hd_info[512];
    ide_channel *ch = hd->belong_to_channel;

    set_disk(ch, hd->device);
    set_cmd(ch, CMD_IDENTIFY);

    sema_down(&ch->disk_done);

    // 检测硬盘是否可读
    if (!busy_wait(ch))
    {
        char error[64];
        sprintf(error, "%s identify faild!\n", hd->name);
        PANIC(error);
    }
    channel_read(ch, hd_info, 1);
    char buf[64];
    uint8_t sn_start = 10 * 2, sn_len = 20,
            model_start = 27 * 2, model_len = 40;

    printk("  disk %s info: \n", hd->name);
    // 获取硬盘 sn 码
    // identify 按字读取，相邻字符的位置互换，需转换为正常顺序
    swap_pairs_bytes(&hd_info[sn_start], buf, sn_len);
    printk("    SN: %s\n", buf);

    // 清空 buffer
    memset(buf, 0, sizeof(buf));
    // 获取硬盘型号
    swap_pairs_bytes(&hd_info[model_start], buf, model_len);
    printk("    MODULE: %s\n", buf);

    uint32_t sectors = *(uint32_t *)&hd_info[60 * 2];
    // 硬盘扇区
    printk("    SECTORS: %d\n", sectors);
    // 硬盘容量
    printk("    CAPACITYS: %dMB\n", sectors * 512 / 1024 / 1024);
}

/* 获取硬盘 hd 中 ext_lba 扇区中的所有分区 */
void partition_scan(struct disk *hd, uint32_t ext_lba)
{
    // 此处必须从堆中申请内存，否则递归查找扇区时可能导致栈溢出
    boot_sector *bs = sys_malloc(sizeof(boot_sector));
    disk_read(hd, bs, ext_lba, 1);

    partition_table_entry *p_entry;
    uint8_t part_idx = 0;
    for (part_idx = 0, p_entry = bs->partition_table; part_idx++ < PARTITION_TABLE_ENTRY_CNT; part_idx++, p_entry++) /* 每个分区表共 4 个表项 */
    {
        switch (p_entry->fs_type)
        {
        case 0: /* 无效的分区类型 */
            break;
        case 0x5: /* 扩展分区 */
                  /* ext_lba_base 为 0 说明是第一次访问扩展分区，则该分区为总扩展分区，更新总扩展分区起始 lba 地址
                     ext_lba_base 不为 0 说明是普通扩展分区
                  */
            if (ext_lba_base != 0)
            {
                partition_scan(hd, ext_lba_base + p_entry->start_lba);
            }
            else
            {
                ext_lba_base = p_entry->start_lba;
                partition_scan(hd, ext_lba_base);
            }

            break;
        default: /* 有效的分区类型 */
        {
            partition *ppart;
            // 分区名 = 硬盘名+分区序号，如 sda1
            // 0-4 为 主分区，逻辑分区从 5 开始
            if (ext_lba == 0) /* 此时全部都是主分区 */
            {
                ppart = &hd->primary_parts[primary_partition_nr];
                sprintf(ppart->name, "%s%d", hd->name, primary_partition_nr);
                primary_partition_nr++;
                ASSERT(primary_partition_nr < PRIMARY_PARTS_CNT);
            }
            else
            {
                ppart = &hd->logic_parts[logic_partition_nr];
                sprintf(ppart->name, "%s%d", hd->name, logic_partition_nr + PRIMARY_PARTS_CNT + 1);
                logic_partition_nr++;
                ASSERT(logic_partition_nr < LOGIC_PARTS_CNT);
            }
            ppart->start_lba = ext_lba + p_entry->start_lba;
            ppart->sector_cnt = p_entry->sector_cnt;
            ppart->belong_to_disk = hd;
            list_append(&partition_list, &ppart->partition_tag);

            break;
        }
        }
    }
    sys_free(bs);
}

bool partition_info(list_elem *pelem, int arg __attribute__((unused)))
{
    partition *ppart = container_of(partition, partition_tag, pelem);
    printk("    %s start_lba:0x%x, sector_cnt:0x%x\n",
           ppart->name, ppart->start_lba, ppart->sector_cnt);
    // 此处 false 只是为了让主调函数 list_traversal 继续向下遍历元素
    return false;
}