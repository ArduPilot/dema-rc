/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#pragma once

#define _printf_format_(a, b) __attribute__((format(printf, a, b)))
#define _pure_ __attribute__((pure))
#define _cleanup_(x) __attribute__((cleanup(x)))
