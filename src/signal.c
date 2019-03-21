/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include <assert.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "demarc_signal.h"
#include "event_loop.h"
#include "log.h"

static int sfd = -1;

static void signal_handler(int fd, void *data, int ev_mask)
{
    struct signalfd_siginfo info;
    int r;

    r = read(fd, &info, sizeof(info));
    if (r != sizeof(struct signalfd_siginfo))
        return;

    event_loop_stop();
}

int signal_init(void)
{
    sigset_t mask;
    int fd;

    assert(sfd == -1);

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        log_error("Failed to setup signals: %m\n");
        return -1;
    }

    fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0 || event_loop_add_source(fd, NULL, EPOLLIN, signal_handler) < 0) {
        log_error("Failed to setup signalfd: %m\n");
        return -1;
    }

    sfd = fd;

    return 0;
}

void signal_shutdown(void)
{
    if (sfd < 0)
        return;

    event_loop_remove_source(sfd);
    close(sfd);
}
