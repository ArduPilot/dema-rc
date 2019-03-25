/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#pragma once

enum RemoteOutputFormat {
    REMOTE_OUTPUT_AP_UDP_SIMPLE,
    REMOTE_OUTPUT_AP_SITL,
    _REMOTE_OUTPUT_UNKNOWN,
};

int remote_init(const char *remote_dest, enum RemoteOutputFormat format);
void remote_shutdown(void);

void remote_send_pkt(const int val[], int count);
