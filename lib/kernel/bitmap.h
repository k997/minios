#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H

#include "global.h"
#include "stdint.h"

#define BITMAP_MASK 1

typedef struct
{
    uint32_t byte_length; // 位示图位长度,单位为字节
    uint8_t *bits;        // 位示图起始地址,字节型指针

} bitmap;

void bitmap_init(bitmap *btmp);
int bitmap_alloc(bitmap *btmp, uint32_t count);
void bitmap_free(bitmap *btmp, uint32_t index, uint32_t count);
#endif