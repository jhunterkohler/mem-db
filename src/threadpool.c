#include "threadpool.h"

/* TODO: pthread related error handling */

int thread_pool_init(struct thread_pool *tp, size_t thread_count)
{
    tp->threads = malloc(thread_count * sizeof(*tp->threads));

    if (!tp->threads)
        return ENOMEM;

    tp->state = THREAD_POOL_ACTIVE;
    tp->thread_count = thread_count;
    tp->active_count = 0;
    tp->work_count = 0;
    tp->work_head = NULL;
    tp->work_tail = NULL;

    pthread_mutex_init(&tp->mutex, NULL);
    pthread_cond_init(&tp->stopped_cond, NULL);
    pthread_cond_init(&tp->work_cond, NULL);

    return 0;
}

int thread_pool_destroy(struct thread_pool *tp)
{
    free(tp->threads);

    pthread_mutex_destroy(&tp->mutex);
    pthread_cond_destroy(&tp->stopped_cond);
    pthread_cond_destroy(&tp->work_cond);

    return 0;
}

int thread_pool_stop(struct thread_pool *tp)
{
    pthread_mutex_lock(&tp->mutex);

    switch (tp->state) {
    case THREAD_POOL_ACTIVE:
        tp->state = THREAD_POOL_STOPPING;

    case THREAD_POOL_STOPPING:
        while (tp->active_count > 0)
            pthread_cond_wait(&tp->stopped_cond, &tp->mutex);
        tp->state = THREAD_POOL_STOPPED;
    }

    pthread_mutex_unlock(&tp->mutex);
    return 0;
}

int thread_pool_start(struct thread_pool *tp)
{
    pthread_mutex_lock(&tp->mutex);

    switch (tp->state) {
    case THREAD_POOL_STOPPING:
        while (tp->state != THREAD_POOL_STOPPED)
            pthread_cond_wait(&tp->stopped_cond, &tp->mutex);

    case THREAD_POOL_STOPPED:
        tp->state = THREAD_POOL_ACTIVE;
        pthread_cond_broadcast(&tp->work_cond);
    }

    pthread_mutex_unlock(&tp->mutex);
    return 0;
}

int thread_pool_add_work(struct thread_pool *tp, void *(routine)(void *),
                         void *arg)
{
    struct thread_pool_work *work = malloc(sizeof(*work));

    if (!work)
        return ENOMEM;

    work->next = NULL;
    work->routine = routine;
    work->arg = arg;

    pthread_mutex_lock(&tp->mutex);

    if (tp->work_tail) {
        tp->work_tail->next = work;
    } else {
        tp->work_tail = work;
        tp->work_head = work;
    }

    pthread_cond_signal(&tp->work_cond);
    pthread_mutex_unlock(&tp->mutex);
    return 0;
}
