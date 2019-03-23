/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include "remote.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "event_loop.h"
#include "log.h"
#include "macro.h"
#include "util.h"

#define DEFAULT_DEST "127.0.0.1"
#define DEFAULT_PORT 777UL

/* -- start AP RCINPUT_UDP protocol -- */

#define RCINPUT_UDP_NUM_CHANNELS 8
#define RCINPUT_UDP_VERSION 2

/* All fields are Little Endian */
struct _packed rc_udp_packet {
    uint32_t version;
    uint64_t timestamp_us;
    uint16_t seq;
    uint16_t ch[RCINPUT_UDP_NUM_CHANNELS];
};

/* ----------------------------------- */

static struct {
    int sfd;
    struct sockaddr_in sockaddr;
    struct rc_udp_packet pkt;
    usec_t last_error_ts;
} remote_ctx = {
    .sfd = -1,
};

void remote_send_pkt(int val[], int count)
{
    struct rc_udp_packet *pkt = &remote_ctx.pkt;
    ssize_t r;
    int i;

    for (i = 0; i < count; i++)
        pkt->ch[i] = val[i];

    pkt->seq++;
    pkt->timestamp_us = now_usec();

    r = sendto(remote_ctx.sfd, pkt, sizeof(*pkt), 0, (struct sockaddr *)&remote_ctx.sockaddr,
               sizeof(struct sockaddr));
    if (r == -1 && errno != EAGAIN) {
        if (errno != ECONNREFUSED && errno != ENETUNREACH)
            log_error("could not send packet: %m\n");
        else if (pkt->timestamp_us - remote_ctx.last_error_ts > 5 * USEC_PER_SEC)
            log_debug("5s without sending update\n");
    }
}
int remote_init(const char *remote_dest)
{
    const char *addr;
    _cleanup_free_ char *buf = NULL;
    char *p;
    unsigned long port;

    if (!remote_dest) {
        addr = DEFAULT_DEST;
        port = DEFAULT_PORT;
    } else {
        buf = strdup(remote_dest);
        p = strchr(buf, ':');
        if (!p) {
            port = DEFAULT_PORT;
        } else if (safe_atoul(p, &port) < 0) {
            log_error("could not parse address %s\n", remote_dest);
            return -EINVAL;
        }
        *p = '\0';
    }

    remote_ctx.sockaddr.sin_family = AF_INET;
    remote_ctx.sockaddr.sin_addr.s_addr = inet_addr(addr);
    remote_ctx.sockaddr.sin_port = htons(port);

    remote_ctx.sfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (remote_ctx.sfd == -1) {
        log_error("could not create socket: %m\n");
        return -errno;
    }

    remote_ctx.pkt.version = RCINPUT_UDP_VERSION;

    return 0;
}

void remote_shutdown(void)
{
    if (remote_ctx.sfd < 0)
        return;

    close(remote_ctx.sfd);
}
