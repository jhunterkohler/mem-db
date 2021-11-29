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

#define SERVER_BACKLOG 128
#define SERVER_DEFAULT_PORT 11111

/*
 * Socket wrapper for server. TCP, supporting IPv6 and IPv4.
 *
 * Note: Database protocol itself need-not be TCP specific, though depends on
 * an equivalent streaming protocol, such as unix sockets.
 */
struct server_socket {
    /*
     * Open socket's file descriptor.
     */
    int fd;

    /*
     * Bound port in host byte order. Opposed to `addr.sin6_port`
     * in network byte order.
     */
    uint16_t port;

    struct sockaddr_in6 addr;
};

/*
 * Initialize a IPv4 socket address. `port` must be in host byte order. If
 * `sin_addr` is `NULL`, it defaults to assigning IPv4 any.
 */
void sockaddr_in_init(struct sockaddr_in *addr, in_port_t port,
                      struct in_addr *sin_addr)
{
    addr->sin_len = sizeof(*addr);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = sin_addr ? sin_addr->s_addr : htonl(INADDR_ANY);
    memset(&addr->sin_zero, 0, sizeof(addr->sin_zero));
}

/*
 * Initialize a IPv6 socket address. `port` must be in host byte order. If
 * `sin6_addr` is `NULL`, it defaults to assigning IPv6 any.
 */
void sockaddr_in6_init(struct sockaddr_in6 *addr, in_port_t port,
                       struct in6_addr *sin6_addr)
{
    addr->sin6_len = sizeof(*addr);
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    addr->sin6_flowinfo = 0;
    addr->sin6_addr = sin6_addr ? *sin6_addr : in6addr_any;
    addr->sin6_scope_id = 0;
}

/*
 * Argument `port` must be in host byte order. Uses IPv6 with socket option
 * `IPV6_V6ONLY` off, thus using IPv4-mapped address, and making it impossible
 * to bind with an IPv4 socket.
 */
int server_socket_init(struct server_socket *sock, in_port_t port,
                       size_t backlog)
{
    sock->port = port;
    sockaddr_in6_init(&sock->addr, sock->port, NULL);

    if ((sock->fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return -1;

    /*
     * Set `IPV6_V6ONLY` explicitly for cross-platform compatibility.
     */
    if (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){ 1 },
                   sizeof(int)) ||
        bind(sock->fd, (struct sockaddr *)&sock->addr, sizeof(sock->addr)) ||
        listen(sock->fd, backlog)) {
        close(sock->fd);
        return -1;
    }

    return 0;
}

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

void server_config_init(struct server_config *config, int argc,
                        char *const *argv)
{
    int opt;
    int longindex;

    opterr = false;
    optind = 1;
    while ((opt = server_getopt(argc, argv, &longindex)) != -1) {
        switch (opt) {
        case OPTION_PORT:
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
        config->port = SERVER_DEFAULT_PORT;
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
     * Fields `changelist_cap` and `eventlist_cap` are struct internals for
     * tracking array allocations.
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
 * TODO: Look into de-registering events on `kqueue_table` destruction.
 */
void kqueue_table_destroy(struct kqueue_table *kt)
{
    free(kt->changelist);
    free(kt->eventlist);
}

int server_loop(struct server_config *cfg)
{
    int err;
    struct kqueue_table *ktable;
    struct thread_pool *threads;

    void *const mem = malloc(sizeof(*ktable) + sizeof(*threads));

    if (!mem)
        return -1;

    ktable = mem;
    threads = mem + sizeof(*ktable);

    if ((err = thread_pool_init(threads, 0)) ||
        (err = kqueue_table_init(ktable)))
        goto error;

    free(mem);
    return 0;
error:
    free(mem);
    return -1;
}

int main(int argc, char **argv)
{
    struct server_config config;
    struct server_socket sock;
    struct kqueue_table ktable;
    struct thread_pool tpool;

    if (!thread_pool_init(&tpool, core_count()))
        fatal("thread_pool_init");

    if (!kqueue_table_init(&ktable))
        fatal("kqueue_table_init");

    if (!server_socket_init(&sock, config->port, SERVER_BACKLOG))
        fatal("server_socket_init");

    while (true) {
    }

    // void *const mem = malloc(sizeof(*ktable) + sizeof(*threads));

    // struct server_config *const config = xmalloc(sizeof(*config));
    // server_config_init(config, argc, argv);

    // if (server_loop(config))
    //     fatal_errno();

    // int err;
    // struct kqueue_table *ktable;
    // struct thread_pool *threads;

    // void *const mem = malloc(sizeof(*ktable) + sizeof(*threads));

    // if (!mem)
    //     return -1;

    // ktable = mem;
    // threads = mem + sizeof(*ktable);

    // if ((err = thread_pool_init(threads, 0)) ||
    //     (err = kqueue_table_init(ktable)))
    //     goto error;
}
