/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>

#include "macro.h"

void log_init(void);
void log_shutdown(void);

int log_get_max_level(void) _pure_;
void log_set_max_level(int level);
bool log_get_show_colors(void) _pure_;
void log_set_show_colors(bool show);

void log_printf(int level, const char *fmt, ...) _printf_format_(2, 3);

#define _log(level, ...)                            \
    do {                                            \
        int _level = (level);                       \
        if (log_get_max_level() >= LOG_PRI(_level)) \
            log_printf(_level, __VA_ARGS__);        \
    } while (0)

/* Normal logging */
#define log_debug(...) _log(LOG_DEBUG, __VA_ARGS__)
#define log_info(...) _log(LOG_INFO, __VA_ARGS__)
#define log_notice(...) _log(LOG_NOTICE, __VA_ARGS__)
#define log_warning(...) _log(LOG_WARNING, __VA_ARGS__)
#define log_error(...) _log(LOG_ERR, __VA_ARGS__)
