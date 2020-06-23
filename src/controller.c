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

#include <c-ini.h>
#include <c-stdaux.h>

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

enum SC2Btn {
    SC2BTN_SETTINGS,
    SC2BTN_HOME,
    SC2BTN_TAKEOFF,
    SC2BTN_B,
    SC2BTN_A,
    SC2BTN_LEFT,
    SC2BTN_RIGHT,
    SC2BTN_RIGHT_WHEEL_LEFT,
    SC2BTN_RIGHT_WHEEL_RIGHT,
    SC2BTN_STICK_LEFT_PRESS,
    SC2BTN_STICK_RIGHT_PRESS,
    _SC2BTN_COUNT,
};

struct Controller {
    int fd;
    bool grab_device;

    /* Axis + buttons: 16 channels */
    int val[_AXIS_COUNT + _SC2BTN_COUNT];

    struct {
        int range[_AXIS_COUNT][_INFO_ABS_COUNT];
    } info;

    struct EventSource *remote_update_timeout;
};

static struct Controller controller;

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

/* Currently we have a static map for SkyController 2 */
static int get_axis_from_evdev(unsigned long code)
{
    switch (code) {
    case ABS_Z:
        return AXIS_ROLL;
    case ABS_RX:
        return AXIS_PITCH;
    case ABS_Y:
        return AXIS_THROTTLE;
    case ABS_X:
        return AXIS_YAW;
    case ABS_RY:
        return AXIS_AUX_LEFT;
    }

    return -1;
}

static int get_btn_from_evdev(unsigned long code)
{
    switch (code) {
    case BTN_TRIGGER:
        return SC2BTN_SETTINGS;
    case BTN_THUMB:
        return SC2BTN_HOME;
    case BTN_THUMB2:
        return SC2BTN_TAKEOFF;
    case BTN_TOP:
        return SC2BTN_B;
    case BTN_TOP2:
        return SC2BTN_A;
    case BTN_PINKIE:
        return SC2BTN_RIGHT;
    case BTN_BASE:
        return SC2BTN_LEFT;
    case BTN_BASE2:
        /* ??? in theory we have, but I couldn't find */
        return -1;
    case BTN_BASE3:
        return SC2BTN_STICK_LEFT_PRESS;
    case BTN_BASE4:
        return SC2BTN_STICK_RIGHT_PRESS;
    case BTN_BASE5:
        return SC2BTN_RIGHT_WHEEL_LEFT;
    case BTN_BASE6:
        return SC2BTN_RIGHT_WHEEL_RIGHT;
    };

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

    /* We need at least EV_ABS events in @fd */
    memset(mask, 0, sizeof(mask));
    ioctl(fd, EVIOCGBIT(0, EV_MAX), mask);
    if (!test_bit(EV_ABS, mask)) {
        log_error("EV_ABS event is not supported\n");
        return -EINVAL;
    }

    memset(mask, 0, sizeof(mask));
    ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), mask);
    for (code = 0; code < EV_MAX && naxis < _AXIS_COUNT; code++) {
        struct input_absinfo abs;
        int axis;

        if (!test_bit(code, mask))
            continue;

        axis = get_axis_from_evdev(code);
        if (axis < 0)
            continue;

        memset(&abs, 0, sizeof(abs));
        ioctl(fd, EVIOCGABS(code), &abs);

        /* fill struct */
        c->info.range[axis][INFO_ABS_MIN] = abs.minimum;
        c->info.range[axis][INFO_ABS_MAX] = abs.maximum;
        c->val[axis] = controller_abs_scale(c, axis, abs.value);

        log_debug("axis: %d min: %d max: %d\n", axis, abs.minimum, abs.maximum);

        naxis++;
    }

    if (naxis < _AXIS_COUNT) {
        log_error("Not all required axis supported by this input\n");
        return -EINVAL;
    }

    /* Buttons are assumed to be low at start */
    for (naxis = _AXIS_COUNT; naxis < _AXIS_COUNT + _SC2BTN_COUNT; naxis++)
        c->val[naxis] = 1000;

    log_debug("controller ok\n");

    return 0;
}

static void evdev_handle_abs(struct Controller *c, struct input_event *e)
{
    int axis = get_axis_from_evdev(e->code);

    if (axis < 0) {
        log_debug("ignoring axis %u\n", e->code);
        return;
    }

    c->val[axis] = controller_abs_scale(c, axis, e->value);

    log_debug("received event axis=%d val=%u\n", axis, c->val[axis]);
}

static void evdev_handle_key(struct Controller *c, struct input_event *e)
{
    int btn = get_btn_from_evdev(e->code);

    if (btn < 0) {
        log_debug("ignoring btn %u\n", e->code);
        return;
    }

    /* Ignore button release */
    if (!e->value)
        return;

    /* Toggle button according with the last value */
    c->val[_AXIS_COUNT + btn] = c->val[_AXIS_COUNT + btn] == 1000 ? 2000 : 1000;

    log_debug("received event btn=%d val=%u\n", btn, e->value);
}

static void evdev_handler(int fd, void *data, int ev_mask)
{
    struct Controller *c = data;
    struct input_event events[64], *e;
    int r;

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
        switch (e->type) {
        case EV_ABS:
            evdev_handle_abs(c, e);
            break;
        case EV_KEY:
            evdev_handle_key(c, e);
            break;
        }
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

    remote_send_pkt(c->val, _AXIS_COUNT + _SC2BTN_COUNT);
}

static void parse_config(CIniDomain *config)
{
    CIniGroup *group;
    CIniEntry *entry;
    const char *value;
    int b;

    group = c_ini_domain_find(config, "General", -1);
    if (!group)
        return;

    entry = c_ini_group_find(group, "GrabDevice", -1);
    if (!entry)
        return;

    value = c_ini_entry_get_value(entry, NULL);
    b = parse_boolean(value);
    if (b < 0) {
        log_warning("Invalid value General.GrabDevice=%s\n", value);
        return;
    }

    controller.grab_device = b;
}

int controller_init(const char *device, CIniDomain *config)
{
    int fd, r;

    controller.fd = -1;

    parse_config(config);

    fd = open(device, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        log_error("can't open %s: %m", device);
        return -errno;
    }

    r = controller.grab_device && evdev_grab_device(fd);
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
