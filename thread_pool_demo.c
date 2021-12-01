#include <unistd.h>
#include "./src/thread_pool.c"

#define THREAD_COUNT 8
#define HIST_COUNT 10
#define ID_COUNT 10

struct work_history {
    int id;
    int index;
    pthread_t ids[ID_COUNT];
};

struct thread_pool tp;
struct work_history hists[HIST_COUNT];
pthread_mutex_t stdout_mutex;

void *log_history(void *arg)
{
    struct work_history *hist = arg;
    bool last = hist->index == ID_COUNT;

    if (hist->index < ID_COUNT) {
        hist->ids[hist->index++] = pthread_self();
        thread_pool_run(&tp, log_history, hist);
    } else {
        flockfile(stdout);

        printf("History %d:\n", hist->id);
        for (int i = 0; i < hist->index; i++) {
            printf("\t%p\n", hist->ids[i]);
        }

        funlockfile(stdout);
    }
}

int main()
{
    thread_pool_init(&tp, THREAD_COUNT);

    for (int i = 0; i < HIST_COUNT; i++) {
        hists[i].id = i + 1;
        hists[i].index = 0;
    }

    for (int i = 0; i < HIST_COUNT; i++) {
        thread_pool_run(&tp, log_history, &hists[i]);
    }

    thread_pool_destroy(&tp);
    printf("done\n");
}
