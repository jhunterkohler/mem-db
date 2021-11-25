#include "thread_pool.h"

void thread_pool_loop(struct thread_pool *tp)
{
    while (true) {
        pthread_mutex_lock(&tp->mutex);

        while (!tp->head && !tp->stop)
            pthread_cond_wait(&tp->event, &tp->mutex);

        if (tp->stop) {
            pthread_mutex_unlock(&tp->mutex);
            pthread_exit(NULL);
        }

        struct thread_pool_job *job = tp->head;

        if (tp->tail == job) {
            tp->tail = NULL;
            tp->head = NULL;
        } else {
            tp->head = job->next;
        }

        pthread_mutex_unlock(&tp->mutex);

        job->routine(job->arg);
        free(job);
    }
}

int thread_pool_init(struct thread_pool *tp, size_t thread_count)
{
    tp->threads = malloc(thread_count * sizeof(*tp->threads));

    if (!tp->threads)
        return 1;

    tp->thread_count = thread_count;
    tp->head = NULL;
    tp->tail = NULL;
    tp->stopping = false;
    tp->stopped_count = 0;

    pthread_mutex_init(&tp->mutex);
    pthread_cond_init(&tp->event);
    pthread_cond_init(&tp->all_stopped);

    return 0;
}

int thread_pool_destroy(struct thread_pool *tp)
{
    pthread_mutex_lock(&tp->mutex);
    tp->stop = true;
    pthread_cond_broadcast(&tp->event);
    pthread_mutex_unlock(&tp->mutex);

    for (int i = 0; i < tp->thread_count; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    struct thread_pool_job *job = tp->head;
    struct thread_pool_job *next;

    while (job) {
        next = job->next;
        free(job);
        job = next;
    }

    free(tp->threads);

    pthread_mutex_destroy(&tp->mutex);
    pthread_cond_destroy(&tp->event);

    return 0;
}

int thread_pool_run(struct thread_pool *tp, void *(*routine)(), void *arg)
{
    struct thread_pool_job *job = malloc(sizeof(*job));

    if (!job)
        return 1;

    job->routine = routine;
    job->arg = arg;

    pthread_mutex_lock(&tp->mutex);

    if (tp->tail) {
        tp->tail->next = job;
        tp->tail = job;
    } else {
        tp->head = job;
        tp->tail = job;
    }

    pthread_cond_signal(&tp->event);
    pthread_mutex_unlock(&tp->mutex);

    return 0;
}
