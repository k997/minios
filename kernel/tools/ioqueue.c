#include "ioqueue.h"
#include "interrupt.h"
#include "debug.h"

static int32_t next_pos(int32_t pos);
static void wait(task_struct **waiter);
static void wakeup(task_struct **waiter);

/*
    环形缓冲队列初始化
 */
void ioqueue_init(ioqueue *ioq)
{
    lock_init(&ioq->lock);
    ioq->producer = ioq->consumer = NULL;
    ioq->head = ioq->tail = 0;
}
/*
    环形缓冲队列放入字符
*/
void ioqueue_putchar(ioqueue *ioq, char ch)
{
    ASSERT(interrupt_get_status() == INTR_OFF);
    /*
    缓冲区满，将 producer 指向当前运行线程，并使当前线程休眠
    将来由消费者唤醒
    由于可能有多个生产者，因此采用 while 判断
    */
    while (ioqueue_full(ioq))
    {
        lock_acquire(&ioq->lock);
        wait(&ioq->producer);
        lock_release(&ioq->lock);
    }

    ioq->buf[ioq->head] = ch;
    ioq->head = next_pos(ioq->head);
    // 缓冲区已放入数据，唤醒消费者线程
    if (ioq->consumer != NULL)
    {
        wakeup(&ioq->consumer);
    }
}

/*
    环形缓冲队列取出字符
*/
char ioqueue_getchar(ioqueue *ioq)
{
    ASSERT(interrupt_get_status() == INTR_OFF);
    /*
    缓冲区空，将 consumer 指向当前运行线程，并使当前线程休眠
    将来由生产者唤醒
    由于可能有多个消费者，因此采用 while 判断
     */
    while (ioqueue_empty(ioq))
    {
        lock_acquire(&ioq->lock);
        wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char ch = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);
    // 缓冲区有了空位置，唤醒生产者线程
    if (ioq->producer != NULL)
    {
        wakeup(&ioq->producer);
    }
    return ch;
}
/*
    队列空
*/
bool ioqueue_empty(ioqueue *ioq)
{
    ASSERT(interrupt_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}
/*
    队列满
*/
bool ioqueue_full(ioqueue *ioq)
{
    ASSERT(interrupt_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}
/*
    使生产者或消费者等待
*/
static void wait(task_struct **waiter)
{
    ASSERT(*waiter == NULL && waiter != NULL);
    *waiter = thread_running();
    thread_block(TASK_BLOCKED);
}

/*
    唤醒生产者或消费者等待
*/
static void wakeup(task_struct **waiter)
{
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}
/*
    返回缓冲区的下一个位置
*/
static int32_t next_pos(int32_t pos)
{
    return (pos + 1) % BUF_SIZE;
}