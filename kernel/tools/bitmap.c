#include "bitmap.h"
#include "string.h"
#include "debug.h"
static uint8_t bitmap_bit_status(bitmap *btmp, uint32_t index);
static void bitmap_set(bitmap *btmp, uint32_t index, uint8_t value);
static int bitmap_scan(bitmap *btmp, uint32_t count);

void bitmap_init(bitmap *btmp)
{
    memset(btmp->bits, 0, btmp->byte_length);
}

// 分配连续 count 个 bit， 并返回起始位置, 失败则返回 -1
int bitmap_alloc(bitmap *btmp, uint32_t count)
{
    int start_index = bitmap_scan(btmp, count);

    uint32_t offset = 0;
    if (start_index != -1)
    {
        for (offset = 0; offset < count; offset++)
            bitmap_set(btmp, start_index + offset, 1);
    }
    return start_index;
}
// 释放从 start_index 开始的连续 count 个 bit
void bitmap_free(bitmap *btmp, uint32_t index, uint32_t count)
{
    uint32_t offset;
    for (offset = 0; offset < count; offset++)
        bitmap_set(btmp, index + offset, 0);
}

// 获取 index 处的 bit 状态
static uint8_t bitmap_bit_status(bitmap *btmp, uint32_t index)
{
    uint32_t byte_index = index / 8;
    uint32_t bit_index = index % 8;

    return (btmp->bits[byte_index]) & (BITMAP_MASK << bit_index);
}

// 查找连续 count 个 bit
static int bitmap_scan(bitmap *btmp, uint32_t count)
{
    int start_bit_index = -1;
    uint32_t byte_index = 0;
    uint32_t bit_index = 0;
    uint32_t __count = 0;
    uint32_t next_bit_index = 0;
    for (byte_index = 0; byte_index < btmp->byte_length; byte_index++)
    {
        // btmp->bits[byte_index] 为 0xff，表示没有为 0 的 bit 可以分配
        if (0xff == (btmp->bits[byte_index]))
            continue;
        // 该字节内有可分配的 bit，查找字节内的为 0 的 bit (可分配的 bit) 的位置
        while (((uint8_t)(BITMAP_MASK << bit_index)) &
               (btmp->bits[byte_index]))
            bit_index++;
        // 可分配 bit 相对于 bitmap 起始位置的偏移
        start_bit_index = byte_index * 8 + bit_index;
        next_bit_index = start_bit_index + 1;
        for (__count = 1; __count < count && !bitmap_bit_status(btmp, next_bit_index); __count++, next_bit_index++)
            ;
        // 已查找到连续 count 个为 0 的 bit，退出循环
        if (__count == count)
            break;
        // 未查找到，从查找失败的位置之后继续查找
        next_bit_index++;
        byte_index = next_bit_index / 8;
        bit_index = next_bit_index % 8;
        start_bit_index = -1;
    }

    return start_bit_index;
}

static void bitmap_set(bitmap *btmp, uint32_t index, uint8_t value)
{
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_index = index / 8;
    uint32_t bit_index = index % 8;
    if (value)
        btmp->bits[byte_index] = (btmp->bits[byte_index]) | (BITMAP_MASK << bit_index);
    else
        btmp->bits[byte_index] = (btmp->bits[byte_index]) & ~(BITMAP_MASK << bit_index);
}
