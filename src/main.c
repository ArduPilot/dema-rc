/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

enum ArgsResult {
    /* fail to parse options */
    ARGS_RESULT_FAILURE = -1,
    /* options parsed, continue loading */
    ARGS_RESULT_SUCCESS,
    /* options parsed, nothing to do upon return */
    ARGS_RESULT_EXIT,
};

static const char *device;

static void help(FILE *fp)
{
    fprintf(fp,
            "%s [OPTIONS...] <input_device>\n\n"
            "optional arguments:\n"
            " --version             Show version\n"
            " -h --help             Print this message\n"
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
        {},
    };
    static const char *short_options = "h";
    int c;

    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) >= 0) {
        switch (c) {
        case 'h':
            help(stdout);
            return ARGS_RESULT_EXIT;
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

    return 0;

fail:
    return EXIT_FAILURE;
}
