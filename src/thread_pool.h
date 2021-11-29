#ifndef MEMDB_THREAD_POOL_H_
#define MEMDB_THREAD_POOL_H_

#include <pthread.h>
#include "common.h"

#define WORK_QUEUE_WAIT 0x1
#define WORK_QUEUE_DESTROY 0x2

struct work_msg {
    void (*routine)(void *);
    void *data;
    struct work_msg *next;
};

struct worker {
    pthread_t thread;
    struct worker *next;
    struct work_msg *task;
};

struct work_queue {
    unsigned int flags;

    pthread_mutex_t mutex;

    pthread_cond_t msg_cond;
    pthread_cond_t wait_cond;
    pthread_cond_t destroy_cond;

    pthread_attr_t thread_attr;

    struct timespec linger;

    size_t idle;
    size_t workers;
    size_t max_workers;
    size_t min_workers;

    struct worker *active;

    struct work_msg *work_head;
    struct work_msg *work_tail;
};

void work_queue_destroy(struct work_queue *wq)
{
    pthread_mutex_lock(&wq->mutex);

    wq->flags |= WORK_QUEUE_DESTROY;

    pthread_cond_broadcast(&wq->msg_cond);
    while (wq->workers)
        pthread_cond_wait(&wq->destroy_cond, &wq->mutex);

    for (struct work_msg *it = wq->work_tail; it; it = it->next)
        free(it);

    pthread_mutex_unlock(&wq->mutex);

    pthread_mutex_destroy(&wq->mutex);
    pthread_cond_destroy(&wq->msg_cond);
    pthread_cond_destroy(&wq->wait_cond);
    pthread_cond_destroy(&wq->destroy_cond);
    pthread_attr_destroy(&wq->thread_attr);
}

/*
 * Arguments `dest` and `src` must be initialized.
 */
int copy_thread_attr(pthread_attr_t *restrict dest,
                     const pthread_attr_t *restrict src)
{
    int error;
    int value;
    size_t size;

    if ((error = pthread_attr_getscope(src, &value)) ||
        (error = pthread_attr_setscope(dest, value)) ||

        /*
         * TODO: Check 'stack' attribute usage: Likely unsafe to inherit stack
         * address.
         */
        (error = pthread_attr_getstack(src, &(void *){ 0 }, &size)) ||
        (error = pthread_attr_setstack(dest, NULL, size)) ||
        (error = pthread_attr_getinheritsched(src, &value)) ||
        (error = pthread_attr_setinheritsched(dest, value)) ||
        (error = pthread_attr_getschedpolicy(src, &value)) ||
        (error = pthread_attr_setschedpolicy(dest, value)) ||
        (error = pthread_attr_getguardsize(src, &size)) ||
        (error = pthread_attr_setguardsize(dest, size)) ||

        /*
         * If not detached, program will likely enter deadlock for some
         * non-zero `min_threads` if program does not implement special
         * (non-trivial) cleanup. Even when the queue does not have work.
         *
         * This makes `work_queue::destroy_cond` neccessary, because, trivially,
         * `pthread_join()` cannot be used.
         */
        (error = pthread_attr_setdetachstate(dest, PTHREAD_CREATE_DETACHED))) {
        return error;
    }

    return 0;
}

int work_queue_init(struct work_queue *wq, size_t min_workers,
                    size_t max_workers, struct timespec *linger,
                    pthread_attr_t *thread_attr)
{
    int error = 0;

    wq->linger = *linger;
    wq->idle = 0;
    wq->threads = 0;
    wq->min_threads = min_threads;
    wq->max_threads = max_threads;
    wq->active = NULL;
    wq->mq_head = NULL;
    wq->mq_tail = NULL;

    /* clang-format off */
    if ((error = pthread_mutex_init(&wq->mutex, NULL))) goto error_mutex;
    if ((error = pthread_cond_init(&wq->msg_cond, NULL))) goto error_msg_cond;
    if ((error = pthread_cond_init(&wq->wait_cond, NULL))) goto error_wait_cond;
    if ((error = pthread_cond_init(&wq->destroy_cond, NULL))) goto error_destroy_cond;
    if ((error = pthread_attr_init(&wq->thread_attr))) goto error_thread_attr;
    if ((error = copy_thread_attr(&wq->thread_attr, thread_attr))) goto error_copy_attr;
    /* clang-format on */

    pthread_mutex_lock(&wq->mutex);

    for (int i = 0; i < min_threads, i++) {
        if ((error = launch_worker_thread(wq))) {
            pthread_mutex_unlock(&wq->mutex);
            work_queue_destroy(wq);
            return error;
        }
    }

    pthread_mutex_unlock(&wq->mutex);
    return 0;

error_copy_attr:
    pthread_attr_destroy(wq->thread_attr);
error_thread_attr:
    pthread_cond_destroy(wq->destroy_cond);
error_destroy_cond:
    pthread_cond_destroy(wq->wait_cond);
error_wait_cond:
    pthread_cond_destroy(wq->msg_cond);
error_msg_cond:
    pthread_mutex_destroy(wq->mutex);
error_mutex:
    return error;
}

// struct list_node {
//     struct list_node *next;
//     struct list_node *prev;
// };

struct thread_pool {
    bool stop;
    pthread_cond_t event;
    pthread_mutex_t mutex;
    pthread_t *threads;
    size_t thread_count;
    struct thread_pool_job *head;
    struct thread_pool_job *tail;
};

struct thread_pool_job {
    struct thread_pool_job *next;
    void *(*routine)(void *);
    void *arg;
};

int thread_pool_init(struct thread_pool *tp, size_t thread_count);
int thread_pool_destroy(struct thread_pool *tp);
int thread_pool_run(struct thread_pool *tp, void *(*routine)(), void *arg);

#endif
