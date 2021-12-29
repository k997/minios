#include "sync.h"
#include "debug.h"
void sema_init(semaphore *psema, uint8_t value)
{
    psema->value = value;
    list_init(&psema->waiters);
}

void sema_down(semaphore *psema)
{
    interrupt_status old_status = interrupt_disable();
    while (psema->value == 0)
    {
        task_struct *current_thread = thread_running();
        if (!list_find_elem(&psema->waiters, &current_thread->thread_status_list_tag))
        {
            list_append(&psema->waiters, &current_thread->thread_status_list_tag);
            thread_block(TASK_BLOCKED);
        }
    }
    psema->value--;
    interrupt_set_status(old_status);
}

void sema_up(semaphore *psema)
{
    interrupt_status old_status = interrupt_disable();
    if (!list_empty(&psema->waiters))
    {
        task_struct *thread_blocked = container_of(task_struct, thread_status_list_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;

    interrupt_set_status(old_status);
}

void lock_init(lock *plock)
{
    plock->lock_holder = NULL;
    plock->lock_holder_repeat_num = 0;
    sema_init(&plock->semaphore, 1);
}

void lock_acquire(lock *plock)
{
    task_struct *current_thread = thread_running();
    if (plock->lock_holder != current_thread)
    {
        sema_down(&plock->semaphore);
        plock->lock_holder = current_thread;
        ASSERT(plock->lock_holder_repeat_num == 0);
        plock->lock_holder_repeat_num = 1;
    }
    else
    {
        plock->lock_holder_repeat_num++;
    }
}

void lock_release(lock *plock)
{
    ASSERT(plock->lock_holder == thread_running());
    if (plock->lock_holder_repeat_num > 1)
    {
        plock->lock_holder_repeat_num--;
        return;
    }
    ASSERT(plock->lock_holder_repeat_num == 1);
    plock->lock_holder = NULL;
    plock->lock_holder_repeat_num = 0;
    sema_up(&plock->semaphore);
}