#!/bin/bash

set -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))

cd $SCRIPT_DIR
adb shell mkdir -p /data/ftp/internal_000/ardupilot/bin/
adb push ./data/ftp/internal_000/ardupilot/bin/demarc-btn /data/ftp/internal_000/ardupilot/bin/
adb push ./data/ftp/internal_000/ardupilot/bin/demarc-cm /data/ftp/internal_000/ardupilot/bin/
adb shell chmod +x /data/ftp/internal_000/ardupilot/bin/demarc-btn
adb shell chmod +x /data/ftp/internal_000/ardupilot/bin/demarc-cm

adb shell mount -o remount,rw /

adb shell mkdir -p /etc/dema-rc/
adb push ./etc/dema-rc/dema-rc.conf /etc/dema-rc/

adb push ./etc/boxinit.d/99-demarc.rc /etc/boxinit.d/
adb shell chmod 0400 /etc/boxinit.d/99-demarc.rc
