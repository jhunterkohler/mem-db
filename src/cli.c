/*
 * Copyright (C) 2021 Hunter Kohler <jhunterkohler@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

// clang-format off
const char version[] = "1.0.0";
const char usage[] =
"Usage: mem-db [options]\n"
"\n"
" --host <hostname>     Server hostname. (Default: 127.0.0.1)\n"
" --port <port>         Server port. (Default: 11111).\n"
" --user <aut>"
" --verbose                 Produce verbose output.\n"
" --help                    Display this help message.\n"
" --version                 Display versioning information.";
// clang-format on

struct config {
    bool verbose;
    char *hostname;
    uint16_t port;
};

noreturn void fatal_no_optarg(const char *arg)
{
    fatal("Expected argument for option '%s'\n", arg);
}

struct config *parse_args(int argc, const char **argv)
{
    struct config *cfg = xzalloc(sizeof(*cfg));

    int i;
    for (i = 0; i < argc; i++) {
        bool last = i + 1 == argc;
        const char *arg = argv[i];

        if (!strcmp(arg, "--host")) {
            if (last)
                fatal_no_optarg(arg);
            cfg->hostname = xstrdup(argv[++i]);
        } else if (!strcmp(arg, "--port")) {
            if (last)
                fatal_no_optarg(arg);
            cfg->port = atoi(argv[++i]);
        } else if (!strcmp(arg, "--verbose")) {
            cfg->verbose = true;
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
        cfg->hostname = strdup("127.0.0.1");
    if (!cfg->port)
        cfg->port = 11111;

    return cfg;
}

int main() {}
