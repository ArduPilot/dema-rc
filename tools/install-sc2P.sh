#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1+
#
# We must be connected through adb already
# If connecting through usb-ethernet, make sure your ethernet
# iface is set to dhcp. Then it's usually the following command
#
#   adb connect 192.168.53.1:9050
#
# If connecting via Disco as a hop, you will need the following
# binaries: netstat and adb, built for Disco (you can find
# statically built binaries for armv7, those should work).
#
# Find SkyController 2P's IP with:
#
#   netstat -nu | grep 9988
#
# This shows the packets with communication between the SkyController's mpp
# software and Disco
#
# We will do some sanity checks, but be careful.
#
# Requirements:
#
#   - adb (already connected to SkyController2P)
#   - tar
#   - readelf (from binutils)
#   - patchelf (to be removed in future)

set -e

DEMA_RC_OVERLAY="/data/lib/ftp/internal_000/dema-rc"
PREFIX="$DEMA_RC_OVERLAY/usr"
DESTDIR="" ## used for patching

fatal() {
    echo "$@" > /dev/stderr
    exit 1
}

# check if we are really connected to SkyController 2P
assert_sc2p() {
    uid=$(adb shell grep ro.parrot.build.uid /etc/build.prop)
    version=$(adb shell grep ro.parrot.build.version /etc/build.prop)

    if [[ "$uid" == "ro.parrot.build.uid=mpp2-linux"* ]] && [ "$version" == "ro.parrot.build.version=1.0.5" ]; then
        echo "We're on the SkyController2P!"
    else
        fatal "SkyController 2P not running firmware 1.0.5"
    fi

    df_system=$(adb shell 'df -P -k | grep -w / | awk "{ print \$4 }"')
    df_data=$(adb shell  'df -P -k | grep -w /data | awk "{ print \$4 }"')
}

register_cleanup() {
    # cleanup extracted rootfs and exit with the latest returncode
    trap '
        ret=$?;
        set +e;
        if [ ! -z "$DESTDIR" ]; then
            rm -rf $DESTDIR
        fi
        if [ $ret -ne 0 ]; then
            echo Installation failed! Please check execution log above >&2
        fi
        exit $ret;
    ' EXIT

}

parse_args() {
    # argument: tarball from CI
    if [ $# -lt 1 ]; then
        fatal "Missing argument: bundle tarball from CI"
    fi

    tarball=$1
}

makepatch_demarc_cm() {
    patchfile=$(mktemp)
    cat << '_EOF_' > $patchfile
--- distro/sc2/usr/bin/dema-rc-cm   2023-07-17 21:21:18.814231887 +0200
+++ dema-rc-cm  2023-07-18 23:33:21.303025978 +0200
@@ -5,19 +5,77 @@
 monitor_pid=0
 gcs_ip_monitor_pid=0

+
+#! -- copied from: https://raw.githubusercontent.com/uavpal/beboptwo4g/master/skycontroller2/uavpal/bin/uavpal_sc2.sh
+change_led_color()
+{
+   case "$1" in
+   off)
+       echo -n -e "\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x00\x00\x00\x00\x00\x00\x00\x11\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x11\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /dev/input/event1
+   ;;
+   red)
+       echo -n -e "\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x01\x00\x00\x00\x00"\
+"\x00\x00\x00\x00\x00\x00\x00\x11\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x11\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /dev/input/event1
+   ;;
+   green)
+       echo -n -e "\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x00\x00\x00\x00\x00\x00\x00\x11\x00\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x11\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /dev/input/event1
+   ;;
+   blue)
+       echo -n -e "\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x00\x00\x00\x00\x00\x00\x00\x11\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x11\x00\x02\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /dev/input/event1
+   ;;
+   magenta)
+       echo -n -e "\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x01\x00\x00\x00\x00"\
+"\x00\x00\x00\x00\x00\x00\x00\x11\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x11\x00\x02\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /dev/input/event1
+   ;;
+   white)
+       echo -n -e "\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x01\x00\x00\x00\x00"\
+"\x00\x00\x00\x00\x00\x00\x00\x11\x00\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"\
+"\x11\x00\x02\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /dev/input/event1
+   ;;
+   *)
+   esac
+}
+
+led_flash()
+{
+   if [ "$led_flash_pid" -gt "0" ]; then kill -9 $led_flash_pid; fi
+   (while true; do
+       i=2
+       while [ "$i" -le "$#" ]; do
+           eval "arg=\${$i}"
+           change_led_color $arg
+           usleep $(($1 * 100000))
+           i=$((i + 1))
+       done
+   done) &
+   led_flash_pid=$!
+}
+
+
 notify_searching()
 {
-    mpp_bb_cli blink 3 0 1000 50
+    ##mpp_bb_cli blink 3 0 1000 50
+    led_flash 2 blue white
 }

 notify_connecting()
 {
-    mpp_bb_cli blink 7 3  200 20
+    ##mpp_bb_cli blink 7 3  200 20
+    led_flash 5 magenta off
 }

 notify_connected()
 {
-    mpp_bb_cli on 3
+    ##mpp_bb_cli on 3
+    led_flash 20 blue off
 }

 forward_ports()
_EOF_
    echo "$patchfile"
}


patch_files_2P() {
    [ ! -d "$DESTDIR" ] && fatal "extracted tarball not found"
    # Patch SkyController 2P
    echo "Patching paths and sensors for the 2P"
    pushd .
    cd $DESTDIR
    grep "/data/ftp/internal_000/" . -Rl | xargs -I '%%' sed -i 's|/data/ftp/|/data/lib/ftp/|g' '%%'
    grep "/dev/input/event0" . -Rl | xargs -I '%%' sed -i 's|/dev/input/event0|/dev/input/event1|g' '%%'

    ## Patch LED file
    echo "Patching LEDs for the 2P"
    patchfile=$(makepatch_demarc_cm)
    patch "${DESTDIR}/data/ftp/internal_000/dema-rc/usr/bin/dema-rc-cm" $patchfile

    echo "Migrating paths for the 2P"
    cd data
    mkdir lib
    mv ftp lib
    popd
}

extract_tarball() {
    DESTDIR=$(mktemp -d --tmpdir demarc-install.XXXXXXXX)

    LIBDIR="$PREFIX/lib"
    BINDIR="$PREFIX/bin"

    tar -C "$DESTDIR" -xf "$tarball"
}

insert_ssid(){
    [ ! -d "$DESTDIR" ] && fatal "extracted tarball not found"
    while [[ "$ssid" == "" ]]; do
        read -p "Name of Parrot SSID: " ssid
    done
    echo $ssid > "$DESTDIR"/data/ftp/internal_000/ssid.txt
}


calculate_space() {
    read req_data xx <<<$(du -sk ${DESTDIR}${PREFIX})
    read req_system xx <<<$(du -sk ${DESTDIR})
    req_system=$((req_system - req_data))

    echo "Space requirements on SkyController 2P:"
    echo -e "/data:\tFree:\t\t${df_data} KB\n\tRequired:\t${req_data} KB"
    echo -e "/:\tFree:\t\t${df_system} KB\n\tRequired:\t${req_system} KB"
}

assert_space() {
    [ $req_data -lt $df_data ] ||
        fatal "Not enough space on /data partition"
    [ $req_system -lt $df_system ] ||
        fatal "Not enough space on / partition"
}

summary_install() {
    echo
    echo
    echo "Installing these files on SkyController 2P:"
    (
        cd $DESTDIR
        find . -type f -o -type l
    )
}

parse_args "$@"

assert_sc2p

register_cleanup
extract_tarball

insert_ssid
patch_files_2P

calculate_space
assert_space

summary_install

# push everything
adb shell <<EOF
echo
echo "################"
echo "SkyController 2P"
echo "################"
echo

echo "Mounting / as rw..."
mount -o remount,rw /

echo "Removing previous installation under $DEMA_RC_OVERLAY"
rm -rf $DEMA_RC_OVERLAY
EOF

adb push $DESTDIR/. /

# fixup permissions - we should try to fix the tarball generation so
# we don't need this
adb shell <<EOF
echo
echo "Fixing up file permissions..."
chmod 0640 /etc/boxinit.d/99-demarc.rc
chmod 0755 /data/lib/ftp/internal_000/dema-rc/usr/bin/dema-rc
chmod 0755 /data/lib/ftp/internal_000/dema-rc/usr/bin/dema-rc-cm
chmod 0755 /data/lib/ftp/internal_000/dema-rc/usr/bin/dema-rc-btn

mount -o remount,ro /
sync
EOF

echo -e "\nInstallation finished successfully, please reboot SkyController 2P\n"
