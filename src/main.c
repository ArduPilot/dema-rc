/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "controller.h"
#include "event_loop.h"
#include "log.h"

enum ArgsResult {
    /* fail to parse options */
    ARGS_RESULT_FAILURE = -1,
    /* options parsed, continue loading */
    ARGS_RESULT_SUCCESS,
    /* options parsed, nothing to do upon return */
    ARGS_RESULT_EXIT,
};

static const char *device;
static bool verbose;

static void help(FILE *fp)
{
    fprintf(fp,
            "%s [OPTIONS...] <input_device>\n\n"
            "optional arguments:\n"
            " --version             Show version\n"
            " -h --help             Print this message\n"
            " -v --verbose          Print debug messages\n"
            "\n"
            "mandatory argument:\n"
            " <input_device>        Controller's input device\n",
            program_invocation_short_name);
}

static enum ArgsResult parse_args(int argc, char *argv[])
{
    enum {
        ARG_VERSION = 0x100,
    };
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, ARG_VERSION},
        {"verbose", no_argument, NULL, 'v'},
        {},
    };
    static const char *short_options = "vh";
    int c;

    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) >= 0) {
        switch (c) {
        case 'h':
            help(stdout);
            return ARGS_RESULT_EXIT;
        case 'v':
            verbose = true;
            break;
        case ARG_VERSION:
            puts(PACKAGE " version " PACKAGE_VERSION);
            return ARGS_RESULT_EXIT;
        case '?':
            return ARGS_RESULT_FAILURE;
        default:
            help(stderr);
            return ARGS_RESULT_FAILURE;
        }
    }

    /* mandatory arg: input device*/
    if (optind + 1 != argc) {
        fprintf(stderr, "error: input device is required\n");
        help(stderr);
        return ARGS_RESULT_FAILURE;
    }

    device = argv[optind];

    return ARGS_RESULT_SUCCESS;
}

int main(int argc, char *argv[])
{
    int r;

    r = parse_args(argc, argv);
    if (r == ARGS_RESULT_EXIT)
        return 0;
    if (r == ARGS_RESULT_FAILURE)
        goto fail;

    log_init();
    if (verbose)
        log_set_max_level(LOG_DEBUG);

    r = event_loop_init();
    if (r < 0)
        goto fail;

    r = controller_init(device);
    if (r < 0)
        goto fail_controller;

    event_loop_run();

    controller_shutdown();
    event_loop_shutdown();
    log_shutdown();

    return 0;

fail_controller:
    event_loop_shutdown();
fail:
    log_shutdown();
    return EXIT_FAILURE;
}
