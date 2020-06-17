#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1+

set -eu

"$@" '-' -o/dev/null </dev/null
