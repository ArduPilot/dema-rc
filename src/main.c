/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller.h"
#include "demarc_signal.h"
#include "event_loop.h"
#include "log.h"
#include "remote.h"

enum ArgsResult {
    /* fail to parse options */
    ARGS_RESULT_FAILURE = -1,
    /* options parsed, continue loading */
    ARGS_RESULT_SUCCESS,
    /* options parsed, nothing to do upon return */
    ARGS_RESULT_EXIT,
};

static const char *device;
static const char *remote_dest;
static enum RemoteOutputFormat remote_output_format = REMOTE_OUTPUT_AP_UDP_SIMPLE;
static bool verbose;

static enum RemoteOutputFormat output_format_from_str(const char *s)
{
    if (strcasecmp(s, "ardupilot-udp-simple") == 0)
        return REMOTE_OUTPUT_AP_UDP_SIMPLE;
    if (strcasecmp(s, "ardupilot-sitl") == 0)
        return REMOTE_OUTPUT_AP_SITL;

    return _REMOTE_OUTPUT_UNKNOWN;
}

static void help(FILE *fp)
{
    fprintf(fp,
            "%s [OPTIONS...] <input_device> [dest]\n\n"
            "optional arguments:\n"
            " --version             Show version\n"
            " -h --help             Print this message\n"
            " -v --verbose          Print debug messages\n"
            " -o --output-format    Output format. One of: ardupilot-udp-simple, ardupilot-sitl\n"
            "                       (default: ardupilot-udp-simple)\n"
            "\n"
            "positional arguments:\n"
            " <input_device>        Controller's input device\n"
            " [dest]                Optional destination - default 127.0.0.1:777\n",
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
        {"output-format", required_argument, NULL, 'o'},
        {},
    };
    static const char *short_options = "vho:";
    int c, positional;

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
        case 'o':
            remote_output_format = output_format_from_str(optarg);
            if (remote_output_format == _REMOTE_OUTPUT_UNKNOWN) {
                fprintf(stderr, "unknown format '%s'\n", optarg);
                return ARGS_RESULT_FAILURE;
            }
            break;
        case '?':
            return ARGS_RESULT_FAILURE;
        default:
            help(stderr);
            return ARGS_RESULT_FAILURE;
        }
    }

    positional = argc - optind;

    /* mandatory arg: input device*/
    if (positional < 1) {
        fprintf(stderr, "error: input device is required\n");
        help(stderr);
        return ARGS_RESULT_FAILURE;
    }

    device = argv[optind++];

    if (positional == 2)
        remote_dest = argv[optind];

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

    r = signal_init();
    if (r < 0)
        goto fail_signal;

    r = controller_init(device);
    if (r < 0)
        goto fail_controller;

    r = remote_init(remote_dest, remote_output_format);
    if (r < 0)
        goto fail_remote;

    event_loop_run();

    remote_shutdown();
    controller_shutdown();
    signal_shutdown();
    event_loop_shutdown();
    log_shutdown();

    return 0;

fail_remote:
    remote_shutdown();
fail_controller:
    signal_shutdown();
fail_signal:
    event_loop_shutdown();
fail:
    log_shutdown();
    return EXIT_FAILURE;
}
