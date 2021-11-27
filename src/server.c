#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>

#include "common.h"
#include "sys.h"
#include "thread_pool.h"
#include "malloc.h"

#define SERVER_BACKLOG 10
#define DEFAULT_PORT 11111

/*
 * Returns negative value on invalid port. Ignores leading and trailing
 * whitespace. `str` must be null temrinated.
 */
int parse_port(const char *str)
{
    while (isspace(*str))
        str++;

    int port = 0;
    int i = 0;

    if (*str == '0')
        return -1;

    while (isdigit(*str) && i++ < 5)
        port = 10 * port + *str++ - '0';

    while (isspace(*str))
        str++;

    return *str || port >= 1 << 16 || port <= 0 ? -1 : port;
}

struct server_config {
    uint16_t port;
};

const char version[] = "1.0.0";
const char usage[] =
    "Usage: mem-db-server [options]\n"
    "\n"
    " -p, --port <port>            Server port. (Default: 11111).\n"
    " -h, --help                   Display this help message.\n"
    " -v, --version                Display versioning information.";

enum option_id {
    OPTION_PORT = 'p',
    OPTION_HELP = 'h',
    OPTION_VERSION = 'v',
};

const struct option longopts[] = {
    {   "port", required_argument, NULL,    OPTION_PORT},
    {      "p", required_argument, NULL,    OPTION_PORT},
    {"version",       no_argument, NULL, OPTION_VERSION},
    {      "v",       no_argument, NULL, OPTION_VERSION},
    {   "help",       no_argument, NULL,    OPTION_HELP},
    {      "h",       no_argument, NULL,    OPTION_HELP},
    {        0,                 0,    0,              0}
};

int server_getopt(int argc, char *const *argv, int *longindex)
{
    return getopt_long_only(argc, argv, ":", longopts, longindex);
}

int server_config_init(struct server_config *config, int argc,
                       char *const *argv)
{
    int opt;
    int longindex;

    opterr = false;
    optind = 1;
    while ((opt = server_getopt(argc, argv, &longindex)) != -1) {
        printf("opt = %c\n", opt);

        switch (opt) {
        case OPTION_PORT:
            printf("optind = %d\n", optind);
            int port = parse_port(optarg);
            if (port < 0)
                fatal("Invalid port: '%s'\n", optarg);
            config->port = port;
            break;
        case OPTION_VERSION:
            printf("%s\n", version);
            exit(0);
        case OPTION_HELP:
            printf("%s\n", usage);
            exit(0);
        case ':':
            fatal("Expected argument for option: '%s'\n", argv[optind - 1]);
        case '?':
            fatal("Unknown option: '%s'\n", argv[optind - 1]);
        }
    }

    if (optind != argc)
        fatal("Extraneous arguments.\n");

    if (!config->port)
        config->port = DEFAULT_PORT;

    return 0;
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
    struct server_config *config = xmalloc(sizeof(*config));
    server_config_init(config, argc, argv);
}
