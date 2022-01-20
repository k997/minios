#ifndef __LIB_DEVICE_IOQUEUE_H
#define __LIB_DEVICE_IOQUEUE_H

#include "stdint.h"
#include "sync.h"
#include "thread.h"

#define BUF_SIZE 64

/*
    环形缓冲区队列
 */
typedef struct ioqueue
{
    lock lock;
    // 生产消费者问题
    task_struct *producer; // 缓冲区满时 wait 状态的生产者
    task_struct *consumer; // 缓冲区空时 wait 状态的消费者
    // 由 buf 数组及 head, tail 组成的双向队列
    char buf[BUF_SIZE];
    int32_t head; // head 处存入数据
    int32_t tail; // tail 处取出数据
} ioqueue;

bool ioqueue_full(ioqueue *ioq);
bool ioqueue_empty(ioqueue *ioq);
char ioqueue_getchar(ioqueue *ioq);
void ioqueue_putchar(ioqueue *ioq, char ch);
void ioqueue_init(ioqueue *ioq);
#endif