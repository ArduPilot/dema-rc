/* SPDX-License-Identifier: LGPL-2.1+ */
/* Copyright (c) 2019 Lucas De Marchi <lucas.de.marchi@gmail.com> */

#pragma once

typedef struct CIniDomain CIniDomain;

int controller_init(const char *device, CIniDomain *config);
void controller_shutdown(void);
