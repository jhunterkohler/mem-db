#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "common.h"

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

struct server_cfg *cfg getcfg(int argc, char **argv)
{
    struct server_cfg *cfg = malloc(sizeof(*cfg));

    if (!cfg)
        goto err_alloc;

    int i;
    for (i = 1; i < argc; i++) {
        bool last = i + 1 == argc;
        const char *arg = argv[i];

        if (!strcmp(arg, "--port")) {
            if (last)
                goto err_optarg;
            cfg->port = atoi(argv[++i]);
        } else if (!strcmp(arg, "--version")) {
            printf("%s\n", version);
            exit(0);
        } else if (!strcmp(arg, "--help")) {
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
    fatal("Expected argument for option '%s'\n", arg);
err_alloc:
    fatal("Could not allocate memory\n");
}

int server_loop(struct server_cfg *cfg)
{
    int fd_sock;
    struct sockaddr_in *addr;

    if(!(addr = malloc(sizeof(*addr)))
        goto alloc_err;
}

int main(int argc, char **argv)
{
    struct server_cfg *cfg = getcfg(argc, argv);
    server_loop(cfg);
}
