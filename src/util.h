/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#pragma once

#include <inttypes.h>
#include <stdlib.h>

#include "string.h"

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define BITS_PER_BYTE 8
#define BITS_PER_LONG (BITS_PER_BYTE * sizeof(unsigned long))

/* how many longs needed to represent nr bits */
#define BITMASK_NLONGS(n) DIV_ROUND_UP(n, BITS_PER_LONG)

static inline void freep(void *p)
{
    free(*(void **)p);
}

#define _cleanup_free_ _cleanup_(freep)

static inline int test_bit(int n, unsigned long *bitmask)
{
    return 1UL & (bitmask[n / BITS_PER_LONG] >> (n & (BITS_PER_LONG - 1)));
}

int safe_atoul(const char *s, unsigned long *ret);

#define max(x, y)              \
    ({                         \
        __auto_type __x = x;   \
        __auto_type __y = y;   \
        __x > __y ? __x : __y; \
    })

#define min(x, y)              \
    ({                         \
        __auto_type __x = x;   \
        __auto_type __y = y;   \
        __x < __y ? __x : __y; \
    })

/* clang-format off */
#define constrain(val, rmin, rmax)  \
    ({                              \
        __auto_type __val = (val);  \
        __val <= rmin ? rmin :      \
        __val >= rmax ? rmax :      \
        __val;                      \
    })

typedef uint64_t usec_t;
typedef uint64_t nsec_t;
#define USEC_INFINITY ((usec_t) -1)

#define MSEC_PER_SEC  1000ULL
#define USEC_PER_SEC  ((usec_t) 1000000ULL)
#define USEC_PER_MSEC ((usec_t) 1000ULL)
#define NSEC_PER_SEC  ((nsec_t) 1000000000ULL)
#define NSEC_PER_MSEC ((nsec_t) 1000000ULL)
#define NSEC_PER_USEC ((nsec_t) 1000ULL)

usec_t now_usec(void);

#define streq(a,b) (strcmp((a),(b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)
#define strcaseeq(a,b) (strcasecmp((a),(b)) == 0)
#define strncaseeq(a, b, n) (strncasecmp((a), (b), (n)) == 0)

/* clang-format on */
