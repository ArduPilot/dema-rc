/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include "util.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

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
