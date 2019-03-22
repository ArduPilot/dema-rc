/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#pragma once

#include <sys/epoll.h>

int event_loop_init(void);
void event_loop_shutdown(void);

typedef void (*EventCallback)(int fd, void *data, int ev_mask);
struct EventSource;

int event_loop_add_source(int fd, void *data, int ev_mask, EventCallback cb);
int event_loop_remove_source(int fd);
struct EventSource *event_loop_add_timeout(unsigned long timeout_msec, void *data,
                                           EventCallback cb);
int event_loop_remove_timeout(struct EventSource *source);

void event_loop_stop(void);
void event_loop_run(void);
