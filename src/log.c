/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include "log.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/* clang-format off */
#define COLOR_RED          "\033[31m"
#define COLOR_LIGHTBLUE    "\033[34;1m"
#define COLOR_YELLOW       "\033[33;1m"
#define COLOR_ORANGE       "\033[0;33m"
#define COLOR_WHITE        "\033[37;1m"
#define COLOR_RESET        "\033[0m"

static struct {
    FILE *target_stream;
    int max_level;
    bool show_colors;
} log_ctx = {
    .max_level = LOG_INFO,
    .show_colors = false,
};

/* clang-format on */

static const char *level_colors[] = {
    [LOG_ERR] = COLOR_RED,    [LOG_WARNING] = COLOR_ORANGE,  [LOG_NOTICE] = COLOR_YELLOW,
    [LOG_INFO] = COLOR_WHITE, [LOG_DEBUG] = COLOR_LIGHTBLUE,
};

void log_init(void)
{
    log_ctx.target_stream = stderr;

    if (isatty(fileno(log_ctx.target_stream)))
        log_ctx.show_colors = true;
}

void log_shutdown(void) {}

int log_get_max_level(void)
{
    return log_ctx.max_level;
}

void log_set_max_level(int level)
{
    log_ctx.max_level = level;
}

bool log_get_show_colors(void)
{
    return log_ctx.show_colors;
}

void log_set_show_colors(bool show)
{
    log_ctx.show_colors = true;
}

static const char *get_color(int level)
{
    if (!log_ctx.show_colors)
        return NULL;
    return level_colors[level];
}

void log_printf(int level, const char *fmt, ...)
{
    va_list ap;
    const char *color;
    int save_errno;

    /* so %m works as expected */
    save_errno = errno;

    assert((level & LOG_PRIMASK) == level);
    color = get_color(level);

    if (color)
        fputs(color, log_ctx.target_stream);

    va_start(ap, fmt);
    errno = save_errno;
    vfprintf(log_ctx.target_stream, fmt, ap);
    va_end(ap);

    if (color)
        fputs(COLOR_RESET, log_ctx.target_stream);
}
