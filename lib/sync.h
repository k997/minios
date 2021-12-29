/*
    进程同步
 */
#ifndef __LIB_LOCK_H
#define __LIB_LOCK_H

#include "list.h"
#include "stdint.h"
#include "thread.h"
#include "interrupt.h"
/* 信号量结构 */
typedef struct semaphore
{
    uint8_t value;
    list waiters;
} semaphore;
/* 锁 */
typedef struct lock
{
    task_struct *lock_holder;
    semaphore semaphore;
    uint32_t lock_holder_repeat_num; // 锁持有者重复申请锁的次数
} lock;

/* 初始化信号量 */
void sema_init(semaphore *psema, uint8_t value);
/* 信号量 P */
void sema_down(semaphore *psema);
/* 信号量 V */
void sema_up(semaphore *psema);
/* 初始化锁 */
void lock_init(lock *plock);
/* 获取锁 */
void lock_acquire(lock *plock);
/* 释放锁 */
void lock_release(lock *plock);

#endif