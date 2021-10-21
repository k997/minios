#ifndef __LIB_IO_H
#define __LIB_IO_H

/*
    为了提高运行速度，直接将与io端口相关的操作写入头文件
    inline 将函数在调用处展开，编译后的代码不含call指令
*/
#include "stdint.h"

// b 1字节
// w 2字节

//向端口写入一个字节
static inline void outb(uint16_t port, uint8_t data)
{
    //%b0 b 代表 1 字节，0 代表内联汇编代码中出现的第 0 个寄存器，此处对应 al
    //%w1 w 代表 2 字节，1 代表内联汇编代码中出现的第 1 个寄存器，此处对应 dx
    //N 立即数约束，表示 0～255 的立即数
    //d 寄存器约束，表示 dx，存储端口号,
    asm volatile("outb %b0, %w1" ::"a"(data), "Nd"(port));
}
// 从端口 port 读取一个字节
static inline uint8_t inb(uint16_t port)
{
    // 寄存器参数同上
    uint8_t data;
    asm volatile("inb %w1, %b0"
                 : "=a"(data)
                 : "Nd"(port));
    return data;
}

/*
数据复制的三剑客。
（1）字符串搬运指令族 movs[dw] 
（2）重复执行指令 rep 
（3）方向指令 cld 和 std
*/

// addr 处的 word_cnt 字节写入 port
static inline void outsw(uint16_t port, const void *addr, uint32_t word_cnt)
{
    //cld ，清除方向标志（DF），制定地址增长方向
    // + 代表可读可写
    // S 寄存器约束，代表 esi
    // 将 ds:esi 处的 16 位的内容写入 port 端口
    // loader中设置段描述符时已经设置好 ds 为 0
    // 平坦模型，段寄存器 ds、es、ss 都为 0, 偏移地址即实际地址
    asm volatile(
        "cld;rep outsw"
        : "+S"(addr), "+c"(word_cnt)
        : "d"(port));
}

// addr 处的 word_cnt 字节写入 port
static inline void insw(uint16_t port, const void *addr, uint32_t word_cnt)
{
    //cld ，清除方向标志（DF），制定地址增长方向
    // + 代表可读可写
    // D 寄存器约束，代表 edi
    // 将 port 处的 16 位的内容写入 ds:edi
    // loader中设置段描述符时已经设置好 ds 为 0
    // 平坦模型，段寄存器 ds、es、ss 都为 0, 偏移地址即实际地址
    asm volatile(
        "cld;rep insw"
        : "+D"(addr), "+c"(word_cnt)
        : "d"(port));
}

#endif