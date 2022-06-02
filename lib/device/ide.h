#ifndef __LIB_DEVICE_IDE_H
#define __LIB_DEVICE_IDE_H

#include "stdint.h"
#include "sync.h"
#include "list.h"

#define CHANNEL_CNT 2
#define DISK_CNT_PER_CHANNEL 2
#define IDE_NAME_LEN 16

// 每个分区表中 4 个表项
#define PARTITION_TABLE_ENTRY_CNT 4
// 主分区数量
#define PRIMARY_PARTS_CNT 4
// 逻辑分区数量
#define LOGIC_PARTS_CNT 8


/* 分区表中的每个表项, 大小为 16 字节 */
struct partition_table_entry
{
    uint8_t bootable;     // 是否可引导
    uint8_t start_head;   // 起始磁头
    uint8_t start_sector; // 起始扇区
    uint8_t start_chs;    // 起始柱面
    uint8_t fs_type;      // 分区类型
    uint8_t end_head;     // 结束磁头
    uint8_t end_sector;   // 结束扇区
    uint8_t end_chs;      // 结束柱面
    uint32_t start_lba;   // 起始扇区 LBA 号
    uint32_t sector_cnt;  // 总扇区数

} __attribute__((packed)); /*  packed 禁止 GCC 为对齐而中填充空隙，
                            保证结构体大小为 16 字节 */
/* 分区表中的每个表项, 大小为 16 字节 */
typedef struct partition_table_entry partition_table_entry;

/* 512 字节引导扇区,可以是 mbr，ebr，obr 等 */
struct boot_sector
{
    uint8_t other[446];                       // 引导代码, 此处用于占位
    partition_table_entry partition_table[PARTITION_TABLE_ENTRY_CNT]; // 64 字节分区表（16字节*4分区表项）
    uint16_t singnature;                      // 0x55，0xaa
                                              // x86 小端字节序，实际为 0xaa55
} __attribute__((packed));                    // 保证结构体大小为 512 字节
/* 512 字节引导扇区,可以是 mbr，ebr，obr 等 */
typedef struct boot_sector boot_sector;

/* 分区 */
typedef struct partition
{
    char name[IDE_NAME_LEN];
    uint32_t start_lba;          // 起始扇区
    uint32_t sector_cnt;         // 扇区数
    struct disk *belong_to_disk; // 指针都是 32 位，所以可以编译通过
    list_elem partition_tag;     // 队列中管理分区的标记
} partition;


/* 通道中主盘还是从盘 */
typedef enum ide_device
{
    DISK_PRIMARY = 0,
    DISK_SECONDARY = 1
} ide_device;


/* 硬盘 */
typedef struct disk
{
    char name[IDE_NAME_LEN];
    struct ide_channel *belong_to_channel;
    enum ide_device device; // 通道的主盘还是从盘，0 为主盘，1 为从盘
    struct partition primary_parts[PRIMARY_PARTS_CNT];
    struct partition logic_parts[LOGIC_PARTS_CNT];
} disk;

/* ide 通道 */
typedef struct ide_channel
{
    char name[IDE_NAME_LEN];
    uint16_t port_base;                       // 通道起始 IO 端口基址
    uint8_t irq_no;                           // 通道使用的中断号
    struct lock lock;                         // 通道锁
    bool expecting_intr;                      // 判断区分硬盘发送的中断是否是用户操作引起的
    struct semaphore disk_done;               // 用于阻塞或唤醒驱动程序
    struct disk device[DISK_CNT_PER_CHANNEL]; // 每个通道可连接主从 2 个硬盘
} ide_channel;


extern list partition_list;
extern uint8_t channel_cnt;
extern ide_channel channels[];

void ide_init();
void disk_read(disk *hd, void *buf, uint32_t lba, uint8_t sector_cnt);
void disk_write(disk *hd, void *buf, uint32_t lba, uint8_t sector_cnt);
void identify_disk(disk *hd);
void partition_scan(struct disk *hd, uint32_t ext_lba);
bool partition_info(list_elem *pelem, int arg __attribute__((unused)));

#endif