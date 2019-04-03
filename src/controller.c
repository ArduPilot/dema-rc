/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#include "controller.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "event_loop.h"
#include "log.h"
#include "remote.h"
#include "util.h"

#define REMOTE_UPDATE_INTERVAL 10

enum InfoAbs {
    INFO_ABS_MIN,
    INFO_ABS_MAX,
    _INFO_ABS_COUNT,
};

enum Axis {
    AXIS_ROLL,
    AXIS_PITCH,
    AXIS_THROTTLE,
    AXIS_YAW,
    AXIS_AUX_LEFT,
    _AXIS_COUNT,
};

struct evdev_abs_axis {
    int value;
    int min;
    int max;
    int fuzz;
    int flat;
    int resolution;
};

struct Controller {
    int fd;

    int val[_AXIS_COUNT];
    struct {
        int range[_AXIS_COUNT][_INFO_ABS_COUNT];
    } info;

    struct EventSource *remote_update_timeout;
};

static struct Controller controller;

/* Currently we have a static map for SkyController 2 */
static const int evdev_axis_mapping[] = {
    [AXIS_ROLL] = ABS_Z, [AXIS_PITCH] = ABS_RX,    [AXIS_THROTTLE] = ABS_Y,
    [AXIS_YAW] = ABS_X,  [AXIS_AUX_LEFT] = ABS_RY,
};

static uint16_t controller_abs_scale(struct Controller *c, enum Axis axis, int val)
{
    int rmin = c->info.range[axis][INFO_ABS_MIN];
    int rmax = c->info.range[axis][INFO_ABS_MAX];

    /* constrain values to range - linux input doesn't do this for us */
    val = constrain(val, rmin, rmax);

    /* transpose to [1000, 2000], or close to it */
    val -= rmin;
    val = val * (1000 / (rmax - rmin)) + 1000;

    return (uint16_t)val;
}

static int get_axis_from_evdev(unsigned long code)
{
    int e;

    for (e = 0; e < _AXIS_COUNT; e++)
        if (evdev_axis_mapping[e] == (int)code)
            return e;

    return -1;
}

static int evdev_grab_device(int fd)
{
    return ioctl(fd, EVIOCGRAB, 1UL);
}

static int evdev_fill_info(int fd, struct Controller *c)
{
    /* query events and codes supported */
    unsigned long mask[BITMASK_NLONGS(KEY_MAX)];
    unsigned long code;
    unsigned int naxis = 0;

    memset(mask, 0, sizeof(mask));
    ioctl(fd, EVIOCGBIT(0, EV_MAX), mask);
    if (!test_bit(EV_ABS, mask)) {
        log_error("EV_ABS event is not supported\n");
        return -EINVAL;
    }

    memset(mask, 0, sizeof(mask));
    ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), mask);
    for (code = 0; code < EV_MAX && naxis < _AXIS_COUNT; code++) {
        struct evdev_abs_axis abs;
        int axis;

        if (!test_bit(code, mask))
            continue;

        axis = get_axis_from_evdev(code);
        if (axis < 0)
            continue;

        memset(&abs, 0, sizeof(abs));
        ioctl(fd, EVIOCGABS(code), &abs);

        /* fill struct */
        c->info.range[axis][INFO_ABS_MIN] = abs.min;
        c->info.range[axis][INFO_ABS_MAX] = abs.max;
        c->val[axis] = abs.value;

        log_debug("axis: %d min: %d max: %d\n", axis, abs.min, abs.max);

        naxis++;
    }

    if (naxis < _AXIS_COUNT) {
        log_error("Not all required axis supported by this input\n");
        return -EINVAL;
    }

    log_debug("controller ok\n");

    return 0;
}

static void evdev_handler(int fd, void *data, int ev_mask)
{
    struct Controller *c = data;
    struct input_event events[64], *e;
    int r, axis;

    if (!(ev_mask & EPOLLIN))
        return;

    r = read(fd, events, sizeof(events));
    if (r < 0) {
        log_error("read: %m\n");
        return;
    }

    if ((size_t)r < sizeof(*events)) {
        log_warning("expected at least %zu bytes\n", sizeof(*events));
        return;
    }

    for (e = events; e < events + r / sizeof(*events); e++) {
        if (e->type != EV_ABS)
            continue;

        axis = get_axis_from_evdev(e->code);
        if (axis < 0) {
            log_debug("ignoring axis %u\n", e->code);
            continue;
        }

        c->val[axis] = controller_abs_scale(c, axis, e->value);

        log_debug("received event axis=%d val=%u\n", axis, c->val[axis]);
    }
}

static void remote_update_handler(int fd, void *data, int ev_mask)
{
    struct Controller *c = data;
    uint64_t count = 0;
    int r;

    r = read(fd, &count, sizeof(count));
    if (r < 1 || count == 0)
        return;

    remote_send_pkt(c->val, _AXIS_COUNT);
}

int controller_init(const char *device)
{
    int fd, r;

    controller.fd = -1;

    fd = open(device, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        log_error("can't open %s: %m", device);
        return -errno;
    }

    r = evdev_grab_device(fd);
    if (r != 0)
        log_warning("Could not grab device %s: no exclusive access\n", device);

    /* TODO: use EVIOCGID and check device id to support other controllers */
    r = evdev_fill_info(fd, &controller);
    if (r < 0)
        goto fail_fill;

    controller.fd = fd;
    if (event_loop_add_source(fd, &controller, EPOLLIN, evdev_handler) < 0)
        goto fail_loop;

    controller.remote_update_timeout
        = event_loop_add_timeout(REMOTE_UPDATE_INTERVAL, &controller, remote_update_handler);
    if (!controller.remote_update_timeout)
        goto fail_timeout;

    return 0;

fail_timeout:
    event_loop_remove_source(fd);
fail_loop:
    controller.fd = -1;
fail_fill:
    close(fd);
    return r;
}

void controller_shutdown(void)
{
    if (controller.fd < 0)
        return;

    event_loop_remove_source(controller.fd);
    event_loop_remove_timeout(controller.remote_update_timeout);

    close(controller.fd);
}
