#!/bin/bash

set -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
PREFIX=/data/ftp/internal_000
BINDIR=${PREFIX}/usr/bin
SYSCONFDIR=/etc

cd $SCRIPT_DIR
adb shell mkdir -p $BINDIR
adb push ./usr/bin/demarc-btn $BINDIR
adb push ./usr/bin/demarc-cm $BINDIR
adb shell chmod +x $BINDIR/demarc-btn
adb shell chmod +x $bindir/demarc-cm

adb shell mount -o remount,rw /

adb shell mkdir -p /etc/dema-rc/
adb push ./etc/dema-rc/dema-rc.conf /etc/dema-rc/

adb push ./etc/boxinit.d/99-demarc.rc /etc/boxinit.d/
adb shell chmod 0400 /etc/boxinit.d/99-demarc.rc
