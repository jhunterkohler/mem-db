#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <netinet/in.h>

#include "common.h"
#include "sys.h"
#include "thread_pool.h"
#include "malloc.h"

struct server_cfg {
    uint16_t port;
};

const char version[] = "1.0.0";
const char usage[] =
    "Usage: mem-db-server [options]\n"
    "\n"
    " --port <port>            Server port. (Default: 11111).\n"
    " --help                   Display this help message.\n"
    " --version                Display versioning information.";

struct server_cfg *getcfg(int argc, char **argv)
{
    struct server_cfg *cfg = xmalloc(sizeof(*cfg));

    int i;
    for (i = 1; i < argc; i++) {
        bool last = i + 1 == argc;

        if (!strcmp(argv[i], "--port")) {
            if (last)
                goto err_optarg;
            cfg->port = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--version")) {
            printf("%s\n", version);
            exit(0);
        } else if (!strcmp(argv[i], "--help")) {
            printf("%s\n", usage);
            exit(0);
        } else if (argv[i][0] == '-') {
            fatal("Unknown option: '%s'\n%s\n", argv[i], usage);
        } else {
            fatal("Unexpected argument: '%s'\n%s\n", argv[i], usage);
        }
    }

    if (!cfg->port)
        cfg->port = 11111;

    return cfg;

err_optarg:
    fatal("Expected argument for option '%s'\n", argv[i]);
}

/*
 * kqueue (2) documentation:
 *
 * FreeBSD: https://www.freebsd.org/cgi/man.cgi?query=kevent&apropos=0&sektion=
 * 0&manpath=FreeBSD+9.0-RELEASE&arch=default&format=html
 *
 * MacOS: https://developer.apple.com/library/archive/documentation/System/Conc
 * eptual/ManPages_iPhoneOS/man2/kqueue.2.html
 */

/*
 * `EV_SET` wrapper.
 */
void kevent_set(struct kevent *event, uintptr_t ident, int16_t filter,
                uint16_t flags, uint32_t fflags, intptr_t data, void *udata)
{
    EV_SET(event, ident, filter, flags, fflags, data, udata);
}

/*
 * Tracks and describes events and changes for `kqueue`, including arbitrary
 * 'list' allocations.
 */
struct kqueue_table {
    /*
     * `kqueue` psuedo file descriptor.
     */
    int kq;

    /*
     * Process ID associated with the `kqueue`. This will be the one that
     * spawned the descriptor. Use this field when using shared mem mapping.
     */
    pid_t pid;

    int nchanges;
    int nevents;
    struct kevent *changelist;
    struct kevent *eventlist;

    /*
     * `changelist_cap` and `eventlist_cap` are struct internal for tracking
     * the trivially associated array allocations.
     */
    size_t changelist_cap;
    size_t eventlist_cap;
};

int kqueue_table_init(struct kqueue_table *kt)
{
    int kq = kqueue();
    if (kq < 0)
        return kq;

    kt->kq = kq;
    kt->pid = getpid();
    kt->nchanges = 0;
    kt->nevents = 0;
    kt->changelist = NULL;
    kt->eventlist = NULL;
    kt->changelist_cap = 0;
    kt->changelist_cap = 0;

    return 0;
}

/*
 * Adds to change list. Returns negative on failure. May reallocate changelist.
 */
int kqueue_table_change(struct kqueue_table *kt, uintptr_t ident,
                        int16_t filter, uint16_t flags, uint32_t fflags,
                        intptr_t data, void *udata)
{
    if (kt->changelist_cap == kt->nchanges) {
        kt->changelist_cap = max(kt->changelist_cap * 3 / 2, 2);
        kt->changelist = realloc(kt->changelist,
                                 kt->changelist_cap * sizeof(*kt->changelist));
        if (!kt->changelist)
            return ENOMEM;
    }

    kevent_set(kt->changelist + kt->nchanges++, ident, filter, flags, fflags,
               data, udata);
    return 0;
}

/*
 * Passes data to `kevent()` system call. Specify `nevents` for the max number
 * of events to return at once. The list will be alloated inside of the
 * `kqueue_table` struct. Specify timeout, or `NULL` for infinite wait. Put
 * zero'd `timespec` to exact a poll.
 */
int kqueue_table_commit(struct kqueue_table *kt, int nevents,
                        struct timespec *timeout)
{
    kt->nevents = kevent(kt->kq, kt->changelist, kt->nchanges, kt->eventlist,
                         nevents, timeout);
    return kt->nevents;
}

/*
 * Note: Database protocol need-not be TCP specific, though depends on
 * an equivalent streaming protocol, such as unix sockets.
 */
int server_loop(struct server_config *cfg)
{
    int fd_sock;
    int kq;
    struct kevent event;
    struct sockaddr_in addr;
    struct thread_pool tp;

    thread_pool_init(&tp, core_count());

    // Networking

    if ((fd_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        fatal("socket: %s\n", strerror(errno));

    if (bind(fd_sock, (struct sockaddr *)&addr, sizeof(addr)))
        fatal("bind: %s\n", strerror(errno));

    if (listen(fd_sock, SERVER_BACKLOG))
        fatal("listen: %s\n", strerror(errno));

    // Kqueue

    memset(&event, 0, sizeof(event));
    event.ident = fd_sock;
    event.filter = EVFILT_READ;
    event.flags = EV_ADD;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg->port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((kq = kqueue()) < 0)
        fatal("kqueue: %s\n", strerror(errno));

    return 0;
}

int main(int argc, char **argv)
{
    struct server_cfg *cfg = getcfg(argc, argv);
    server_loop(cfg);
}
