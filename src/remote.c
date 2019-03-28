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
    uint64_t timestamp_usec;
    uint16_t seq;
    uint16_t ch[RCINPUT_UDP_NUM_CHANNELS];
};

/* ----------------------------------- */

/* -- sitl -- */

#define SITL_NUM_CHANNELS 8

/* All fields are Little Endian */
struct _packed rc_udp_sitl_packet {
    uint16_t ch[SITL_NUM_CHANNELS];
};
/**/

static struct {
    int sfd;
    struct sockaddr_in sockaddr;
    union {
        struct rc_udp_packet pkt;
        struct rc_udp_sitl_packet sitl_pkt;
    };
    enum RemoteOutputFormat format;
    usec_t last_error_ts;
} remote_ctx = {
    .sfd = -1,
};

static int _send(const void *buf, size_t len)
{
    int r = sendto(remote_ctx.sfd, buf, len, 0, (struct sockaddr *)&remote_ctx.sockaddr,
                   sizeof(struct sockaddr));
    if (r == -1 && errno != EAGAIN) {
        if (errno != ECONNREFUSED && errno != ENETUNREACH)
            log_error("could not send packet: %m\n");
    }

    return r;
}

static void sitl_send_pkt(const int val[], int count)
{
    struct rc_udp_sitl_packet *pkt = &remote_ctx.sitl_pkt;
    int i;

    assert(count <= RCINPUT_UDP_NUM_CHANNELS);

    for (i = 0; i < count; i++)
        pkt->ch[i] = val[i];

    _send(pkt, sizeof(*pkt));
}

static void simple_send_pkt(const int val[], int count)
{
    struct rc_udp_packet *pkt = &remote_ctx.pkt;
    ssize_t r;
    int i;

    assert(count <= SITL_NUM_CHANNELS);

    for (i = 0; i < count; i++)
        pkt->ch[i] = val[i];

    pkt->seq++;
    pkt->timestamp_usec = now_usec();

    r = _send(pkt, sizeof(*pkt));
    if (r == -1 && pkt->timestamp_usec - remote_ctx.last_error_ts > 5 * USEC_PER_SEC) {
        log_debug("5s without sending update\n");
        remote_ctx.last_error_ts = pkt->timestamp_usec;
    }
}

void remote_send_pkt(const int val[], int count)
{
    switch (remote_ctx.format) {
    case REMOTE_OUTPUT_AP_UDP_SIMPLE:
        return simple_send_pkt(val, count);
    case REMOTE_OUTPUT_AP_SITL:
        return sitl_send_pkt(val, count);
    default:
        break;
    }
}

int remote_init(const char *remote_dest, enum RemoteOutputFormat format)
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
        } else if (safe_atoul(p + 1, &port) < 0) {
            log_error("could not parse address %s\n", remote_dest);
            return -EINVAL;
        }
        *p = '\0';
        addr = buf;
    }

    remote_ctx.sockaddr.sin_family = AF_INET;
    remote_ctx.sockaddr.sin_addr.s_addr = inet_addr(addr);
    remote_ctx.sockaddr.sin_port = htons(port);

    remote_ctx.sfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (remote_ctx.sfd == -1) {
        log_error("could not create socket: %m\n");
        return -errno;
    }

    if (format == REMOTE_OUTPUT_AP_UDP_SIMPLE)
        remote_ctx.pkt.version = RCINPUT_UDP_VERSION;

    remote_ctx.format = format;

    return 0;
}

void remote_shutdown(void)
{
    if (remote_ctx.sfd < 0)
        return;

    close(remote_ctx.sfd);
}
