#ifndef MEMDB_THREAD_POOL_H_
#define MEMDB_THREAD_POOL_H_

#include <pthread.h>
#include "common.h"

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

/*
 * Loop run by the worker threads.
 */
void thread_pool_loop(struct thread_pool *tp);

#endif
