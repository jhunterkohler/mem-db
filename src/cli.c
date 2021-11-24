/*
 * Copyright (C) 2021 Hunter Kohler <jhunterkohler@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "malloc.h"
#include "common.h"

const char version[] = "1.0.0";
const char usage[] =
    "Usage: mem-db-cli [options]\n"
    "\n"
    " --host <hostname>         Server hostname. (Default: 127.0.0.1)\n"
    " --port <port>             Server port. (Default: 11111).\n"
    " --help                    Display this help message.\n"
    " --version                 Display versioning information.";

struct cli_cfg {
    char *hostname;
    uint16_t port;
};

struct cli_cfg *getcfg(int argc, char **argv)
{
    struct cli_cfg *cfg = xzalloc(sizeof(*cfg));

    int i;
    for (i = 1; i < argc; i++) {
        bool last = i + 1 == argc;
        const char *arg = argv[i];

        if (!strcmp(arg, "--host")) {
            if (last)
                goto err_optarg;
            cfg->hostname = xstrdup(argv[++i]);
        } else if (!strcmp(arg, "--port")) {
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

    if (!cfg->hostname)
        cfg->hostname = xstrdup("127.0.0.1");
    if (!cfg->port)
        cfg->port = 11111;

    return cfg;

err_optarg:
    fatal("Expected argument for option '%s'\n", argv[i]);
}

int repl_loop(struct cli_cfg *cfg)
{
    printf("mem-db %s\n", version);
    printf(">> ");

    // printf("cfg->port = %s", cfg->hostname);
    return 0;
}

int main(int argc, char **argv)
{
    struct cli_cfg *cfg = getcfg(argc, argv);
    repl_loop(cfg);
}
