#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1+
#
# We must be connected through adb already
# If connecting through usb-ethernet, make sure your ethernet
# iface is set to dhcp. Then it's usually the following command
#
# 	adb connect 192.168.53.1:9050
#
# If connecting via Disco as a hop, you will need the following
# binaries: netstat and adb, built for Disco (you can find
# statically built binaries for armv7, those should work).
#
# Find SkyController 2's IP with:
#
# 	netstat -nu | grep 9988
#
# This shows the packets with communication between the SkyController's mpp
# software and Disco
#
# We will do some sanity checks, but be careful.
#
# Requirements:
#
#   - adb (already connected to SkyController2)
#   - tar
#   - readelf (from binutils)
#   - patchelf (to be removed in future)

set -e

PREFIX="/data/ftp/internal_000/dema-rc/usr"

fatal() {
	echo "$@" > /dev/stderr
	exit 1
}

# check if we are really connected to SkyController 2
assert_sk2() {
	uid=$(adb shell grep ro.parrot.build.uid /etc/build.prop)
	version=$(adb shell grep ro.parrot.build.version /etc/build.prop)

	[[ "$uid" == "ro.parrot.build.uid=mpp-linux"* ]]  ||
		fatal "adb not connected to SkyController 2"
	[ "$version" == "ro.parrot.build.version=1.0.9" ] ||
		fatal "SkyController 2 not running latest firmware (1.0.9)"

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

relocate_bundle() {
	DESTDIR=$(mktemp -d --tmpdir demarc-install.XXXXXXXX)
	LIBDIR="$PREFIX/lib"
	BINDIR="$PREFIX/bin"

	tar -C "$DESTDIR" -xf "$tarball" \
		--transform="s@./usr@.$PREFIX@" \
		--transform="s@./lib@.$LIBDIR@"

	# Change ld interpreter to be inside LIBDIR, if present
	BINARY=${DESTDIR}${PREFIX}/bin/dema-rc
	ld=$(readelf --program-headers $BINARY | sed -n 's/ *\[Requesting program interpreter: \(.*\)]/\1/p')
	if [ -x ${DESTDIR}${PREFIX}${ld} ]; then
		patchelf --set-interpreter ${PREFIX}${ld} "$BINARY"
	fi
}

calculate_space() {
	read req_data xx <<<$(du -sk ${DESTDIR}${PREFIX})
	read req_system xx <<<$(du -sk ${DESTDIR})
	req_system=$((req_system - req_data))

	echo "Space requirements on SkyController 2:"
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
	echo "Installing these files on SkyController 2:"
	(
	cd $DESTDIR
	find . -type f -o -type l
	)
}

parse_args "$@"

assert_sk2

register_cleanup
relocate_bundle

calculate_space
assert_space

summary_install

# push everything
adb shell <<EOF
echo
echo "################"
echo "SkyController 2"
echo "################"
echo

echo "Mounting / as rw..."
mount -o remount,rw /

echo "Removing previous installation under $PREFIX"
rm -rf $PREFIX
EOF

adb push $DESTDIR/. /

# fixup permissions - we should try to fix the tarball generation so
# we don't need this
adb shell <<EOF
echo
echo "Fixing up file permissions..."
chmod 0640 /etc/boxinit.d/99-demarc.rc
chmod 0755 /data/ftp/internal_000/dema-rc/usr/bin/dema-rc
chmod 0755 /data/ftp/internal_000/dema-rc/usr/bin/dema-rc-cm
chmod 0755 /data/ftp/internal_000/dema-rc/usr/bin/dema-rc-btn

mount -o remount,ro /
sync
EOF

echo -e "\nInstallation finished successfully, please reboot SkyController 2\n"
