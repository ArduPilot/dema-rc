/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <c-ini.h>
#include <c-stdaux.h>

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

static CIniDomain *config_domain;

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
            " [input_device]        Controller's input device\n"
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

    /* positional arguments: input device, destination */
    positional = argc - optind;

    if (positional >= 1)
        device = argv[optind++];

    if (positional >= 2)
        remote_dest = argv[optind];

    return ARGS_RESULT_SUCCESS;
}

static void config_file_parse_general_group(CIniDomain *domain)
{
    CIniGroup *group = c_ini_domain_find(domain, "General", -1);
    if (!group)
        return;

    for (CIniEntry *entry = c_ini_group_iterate(group); entry; entry = c_ini_entry_next(entry)) {
        const char *key, *value;
        size_t keylen;

        key = c_ini_entry_get_key(entry, &keylen);
        value = c_ini_entry_get_value(entry, NULL);

        log_debug("conf: General.%s = %s\n", key, value);

        if (!device && strncasecmp(key, "InputDevice", keylen))
            device = value;
        else if (!remote_dest && strncasecmp(key, "Destination", keylen))
            remote_dest = value;
    }
}

static int config_file_init(CIniDomain **config_domainp)
{
    _c_cleanup_(c_closep) int fd = -1;
    _c_cleanup_(c_ini_reader_freep) CIniReader *reader = NULL;
    _c_cleanup_(c_ini_domain_unrefp) CIniDomain *domain = NULL;
    const char *path = PKGSYSCONFDIR "/dema-rc.conf";
    int r;

    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        if (errno != ENOENT) {
            log_error("Could not open %s (%m)\n", path);
            return -errno;
        }
        return 0;
    }

    r = c_ini_reader_new(&reader);
    if (r < 0)
        return r;

    /* clang-format off */
    c_ini_reader_set_mode(reader,
                          C_INI_MODE_EXTENDED_WHITESPACE |
                          C_INI_MODE_MERGE_GROUPS |
                          C_INI_MODE_OVERRIDE_ENTRIES);
    /* clang-format on */

    for (;;) {
        uint8_t buf[1024];
        ssize_t len;

        len = read(fd, buf, sizeof(buf));
        if (len < 0)
            return -errno;
        else if (len == 0)
            break;

        r = c_ini_reader_feed(reader, buf, len);
        if (r < 0)
            return r;
    }

    r = c_ini_reader_seal(reader, &domain);
    if (r < 0)
        return r;

    /* keep a ref around: this ensures all entries with their values are still valid */
    *config_domainp = c_ini_domain_ref(domain);

    config_file_parse_general_group(domain);

    return 0;
}

static void config_file_shutdown(void)
{
    if (config_domain)
        c_ini_domain_unref(config_domain);
}

int main(int argc, char *argv[])
{
    int r;

    log_init();

    r = parse_args(argc, argv);
    if (r == ARGS_RESULT_EXIT)
        return 0;
    if (r == ARGS_RESULT_FAILURE)
        goto fail;

    if (verbose)
        log_set_max_level(LOG_DEBUG);

    r = config_file_init(&config_domain);
    if (r < 0)
        goto fail;

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

    /*
     * We don't make any more use of configuration after initializing everything, so just release
     * the memory allocated
     */
    config_file_shutdown();

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
    config_file_shutdown();
    log_shutdown();
    return EXIT_FAILURE;
}
