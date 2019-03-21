/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#pragma once

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define BITS_PER_BYTE 8
#define BITS_PER_LONG (BITS_PER_BYTE * sizeof(unsigned long))

/* how many longs needed to represent nr bits */
#define BITMASK_NLONGS(n) DIV_ROUND_UP(n, BITS_PER_LONG)

static inline int test_bit(int n, unsigned long *bitmask)
{
    return 1UL & (bitmask[n / BITS_PER_LONG] >> (n & (BITS_PER_LONG - 1)));
}

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
/* clang-format on */
