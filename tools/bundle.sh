#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1+

set -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
BUILD_DIR=${BUILD_DIR:-$(realpath $SCRIPT_DIR/../build)}
TOOLCHAIN_DIR=${TOOLCHAIN_DIR:-/opt/arm-buildroot-linux-gnueabihf_sdk-buildroot}
TOOLCHAIN=${TOOLCHAIN:-arm-buildroot-linux-gnueabihf}
OUTPUT=${1:-dema-rc-bundle.tar.gz}

instdir=

trap '
    ret=$?;
    set +e;
    if [[ -n "$instdir" ]]; then
        rm -rf $instdir
    fi
    if [[ $ret -ne 0 ]]; then
        echo Error generating bundle >&2
    fi
    exit $ret;
    ' EXIT

relocate_bundle() {
	local PREFIX=data/ftp/internal_000/dema-rc/usr
	local LIBDIR="$PREFIX/lib"
	local BINDIR="$PREFIX/bin"

	pushd $instdir
	mkdir -p $PREFIX
	if [ -d usr/lib ]; then
		mv usr/lib $PREFIX/
	fi
	if [ -d usr/bin ]; then
		mv usr/bin $PREFIX/
	fi

	rmdir usr 2>/dev/null || true

	# Change ld interpreter to be inside LIBDIR, if present
	BINARY=${BINDIR}/dema-rc
	ld=$(readelf --program-headers $BINARY | sed -n 's/ *\[Requesting program interpreter: \(.*\)]/\1/p')
	if [ -x ${PREFIX}${ld} ]; then
		patchelf --set-interpreter /${PREFIX}${ld} "$BINARY"
	fi
	popd
}

# clean up after ourselves no matter how we die.
trap 'exit 1;' SIGINT

instdir=$(mktemp -d --tmpdir dema-rc-bundle.XXXXXXXX)
DESTDIR="$instdir" ninja -C $SCRIPT_DIR/../build install

$SCRIPT_DIR/bundler.sh \
	-x 'ld-*.so' -x libc.so \
	-q qemu-arm-static \
	-d -o $instdir \
	"$TOOLCHAIN_DIR" \
	"$TOOLCHAIN" \
	"$instdir/usr/bin/dema-rc"

# overlay distro/sc2/
cp -a $SCRIPT_DIR/../distro/sc2/. "$instdir"

# relocate binaries and libs to side load
relocate_bundle

rm -f dema-rc-bundle.tar.gz
tar --owner=0 --group=0 -C "$instdir" -czf ${OUTPUT}  .
