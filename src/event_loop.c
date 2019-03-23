/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include "event_loop.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "array.h"
#include "log.h"
#include "util.h"

/* clang-format off */

#define DEFAULT_SOURCE_CAPACITY 16

enum EventType {
    EVENT_GENERIC,
    EVENT_TIMEOUT,
};

struct EventSource {
    void *user_data;
    EventCallback cb;
    int fd;
    enum EventType type;
};

struct TimeoutSource {
    struct EventSource event;
    struct itimerspec ts;
};

static struct {
    int fd;
    bool should_exit;

    struct array sources;
} ev_ctx = {
    .fd = -1,
};

/* clang-format off */

int event_loop_init(void)
{
    assert(ev_ctx.fd < 0);

    ev_ctx.fd = epoll_create1(EPOLL_CLOEXEC);
    if (ev_ctx.fd == -1) {
        log_error("%m\n");
        return -errno;
    }

    array_init(&ev_ctx.sources, DEFAULT_SOURCE_CAPACITY);

    return 0;
}

void event_loop_shutdown(void)
{
    if (ev_ctx.fd >= 0) {
        close(ev_ctx.fd);
        ev_ctx.fd = -1;
    }

    array_free_array(&ev_ctx.sources);
}

static int _event_loop_add_source(struct EventSource *source, int fd, void *data,
                                  int ev_mask, EventCallback cb)
{
    struct epoll_event ev = { };
    int r;

    r = array_append(&ev_ctx.sources, source);
    if (r < 0) {
        log_error("Could not add source (%s)\n", strerror(-r));
        return r;
    }

    source->user_data = data;
    source->cb = cb;
    source->fd = fd;

    ev.events = ev_mask;
    ev.data.ptr = source;

    r = epoll_ctl(ev_ctx.fd, EPOLL_CTL_ADD, fd, &ev);
    if (r < 0) {
        r = -errno;
        log_error("Could not add fd: %d (%m)\n", fd);
        goto fail_epoll;
    }

    log_debug("source %d added\n", fd);

    return 0;

fail_epoll:
    array_pop(&ev_ctx.sources);
    return r;
}

int event_loop_add_source(int fd, void *data, int ev_mask, EventCallback cb)
{
    struct EventSource *source;
    int r;

    source = calloc(1, sizeof(*source));
    if (!source) {
        log_error("Could not add source (%m)\n");
        return -errno;
    }

    r = _event_loop_add_source(source, fd, data, ev_mask, cb);
    if (r < 0) {
        free(source);
        return r;
    }

    return 0;
}

int event_loop_remove_source(int fd)
{
    struct EventSource *source;
    int i;

    for (i = 0; i < (int) ev_ctx.sources.count; i++) {
        source = ev_ctx.sources.array[i];

        if (source->fd == fd)
            break;
    }

    if (i >= (int) ev_ctx.sources.count) {
        log_error("fd not found: %d\n", fd);
        return -EINVAL;
    }

    if (epoll_ctl(ev_ctx.fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        log_error("Could not remove fd: %d (%m)", fd);
        return -errno;
    }

    free(source);
    array_remove_at(&ev_ctx.sources, i);

    log_debug("source %d removed\n", fd);

    return 0;
}

struct EventSource *event_loop_add_timeout(unsigned long timeout_msec, void *data, EventCallback cb)
{
    struct TimeoutSource *source;
    int r, fd;

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0) {
        log_error("unable to create timerfd: %m\n");
        return NULL;
    }

    source = calloc(1, sizeof(*source));
    if (!source) {
        log_error("Could not add source (%m)\n");
        goto fail_alloc;
    }

    r = _event_loop_add_source(&source->event, fd, data, EPOLLIN, cb);
    if (r < 0)
        goto fail_add_source;

    source->ts.it_interval.tv_sec = timeout_msec / MSEC_PER_SEC;
    source->ts.it_interval.tv_nsec = (timeout_msec % MSEC_PER_SEC) * NSEC_PER_MSEC;
    source->ts.it_value.tv_sec = source->ts.it_interval.tv_sec;
    source->ts.it_value.tv_nsec = source->ts.it_interval.tv_nsec;
    source->event.type = EVENT_TIMEOUT;

    /* start timeout */
    timerfd_settime(fd, 0, &source->ts, NULL);

    log_debug("timeout added: fd=%d\n", fd);

    return &source->event;

fail_add_source:
    free(source);
fail_alloc:
    close(fd);
    return NULL;
}

int event_loop_remove_timeout(struct EventSource *source)
{
    int r, fd;

    assert(source->type == EVENT_TIMEOUT);

    fd = source->fd;
    r = event_loop_remove_source(fd);
    close(fd);

    return r;
}

void event_loop_stop(void)
{
    ev_ctx.should_exit = true;
}

void event_loop_run(void)
{
    const int max_events = 16;
    struct epoll_event events[max_events];

    if (ev_ctx.fd < 0)
        return;

    while (!ev_ctx.should_exit) {
        int r, i;

        r = epoll_wait(ev_ctx.fd, events, max_events, -1);
        if (r < 0 && errno == EINTR) {
            log_debug("Interrupted epoll (%m)\n");
            continue;
        }

        for (i = 0; i < r; i++) {
            struct EventSource *source = events[i].data.ptr;
            source->cb(source->fd, source->user_data, events[i].events);
        }
    }
}
