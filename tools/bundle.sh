#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1+

set -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
BUILD_DIR=${BUILD_DIR:-$(realpath $SCRIPT_DIR/../build)}
TOOLCHAIN_DIR=${TOOLCHAIN_DIR:-/opt/arm-buildroot-linux-gnueabihf_sdk-buildroot}
TOOLCHAIN=${TOOLCHAIN:-arm-buildroot-linux-gnueabihf}

BOARD=
BOARD_OPTIONS=
OUTPUT=

args=0
while getopts "o:b:" o; do
    case "${o}" in
        o) OUTPUT=${OPTARG}; args=$[$args + 2] ;;
	b) BOARD=${OPTARG}; args=$[$args + 2] ;;
        \?) exit 1;;
    esac
done
shift $args

OUTPUT=${OUTPUT:-dema-rc-bundle.tar.gz}

instdir=

if [ "$BOARD" == "sc2" ]; then
	BOARD_OPTIONS="-x ld-*.so -x libc.so -q qemu-arm-static"
else
	echo "Unknown distro option $BOARD" >/dev/stderr
	exit 1
fi

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

	pushd $instdir > /dev/null
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
	popd > /dev/null
}

# clean up after ourselves no matter how we die.
trap 'exit 1;' SIGINT

instdir=$(mktemp -d --tmpdir dema-rc-bundle.XXXXXXXX)
DESTDIR="$instdir" ninja -C $SCRIPT_DIR/../build install > /dev/null

$SCRIPT_DIR/bundler.sh \
	$BOARD_OPTIONS \
	-d -o $instdir \
	"$TOOLCHAIN_DIR" \
	"$TOOLCHAIN" \
	"$instdir/usr/bin/dema-rc"

# overlay distro/$BOARD/
cp -a $SCRIPT_DIR/../distro/$BOARD/. "$instdir"

# relocate binaries and libs to side load
relocate_bundle

rm -f $OUTPUT
tar --owner=0 --group=0 -C "$instdir" -czf ${OUTPUT}  .

echo "Bundle created: $(realpath $OUTPUT)"
