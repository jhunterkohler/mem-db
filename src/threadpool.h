#ifndef MEMDB_THREADPOOL_H_
#define MEMDB_THREADPOOL_H_

#include <pthread.h>
#include "common.h"

enum thread_pool_state {
    THREAD_POOL_STOPPED,
    THREAD_POOL_STOPPING,
    THREAD_POOL_ACTIVE
};

struct thread_pool {
    enum thread_pool_state state;
    pthread_t *threads;
    size_t active_count;
    size_t thread_count;
    pthread_mutex_t mutex;
    pthread_cond_t stopped_cond;

    size_t work_count;
    pthread_cond_t work_cond;
    struct thread_pool_work *work_head;
    struct thread_pool_work *work_tail;
};

struct thread_pool_work {
    struct thread_pool_work *next;
    void *(*routine)(void *);
    void *arg;
};

int thread_pool_init(struct thread_pool *tp, size_t thread_count);
int thread_pool_destroy(struct thread_pool *tp);
int thread_pool_stop(struct thread_pool *tp);
int thread_pool_start(struct thread_pool *tp);
int thread_pool_add_work(struct thread_pool *tp, void *(routine)(void *),
                         void *arg);

#endif
