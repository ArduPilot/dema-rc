/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include "util.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

int safe_atoul(const char *s, unsigned long *ret)
{
    char *x = NULL;
    unsigned long l;

    assert(s);
    assert(ret);

    errno = 0;
    l = strtoul(s, &x, 0);

    if (!x || x == s || *x || errno)
        return errno ? -errno : -EINVAL;

    *ret = l;

    return 0;
}

static usec_t ts_usec(const struct timespec *ts)
{
    if (ts->tv_sec == (time_t)-1 && ts->tv_nsec == (long)-1)
        return USEC_INFINITY;

    if ((usec_t)ts->tv_sec > (UINT64_MAX - (ts->tv_nsec / NSEC_PER_USEC)) / USEC_PER_SEC)
        return USEC_INFINITY;

    return (usec_t)ts->tv_sec * USEC_PER_SEC + (usec_t)ts->tv_nsec / NSEC_PER_USEC;
}

usec_t now_usec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts_usec(&ts);
}
