#!/bin/bash

set -e

QEMU=
OUTPUT=deps.tar

args=0
while getopts "q:o:" o; do
    case "${o}" in
        q) QEMU=${OPTARG}; args=$[$args + 2] ;;
        o) OUTPUT=${OPTARG}; args=$[$args + 2] ;;
        \?) exit 1;;
    esac
done
shift $args

TOOLCHAIN_PATH=$1
TRIPLET=$2
BINARY=$3

# Get linker from ELF header
ld=$(readelf --program-headers $BINARY | sed -n 's/ *\[Requesting program interpreter: \(.*\)]/\1/p')

# Find same linker in the toolchain dir; any of the copies there should work
ld_toolchain=$(find $TOOLCHAIN_PATH/$TRIPLET -name "$(basename $ld)" -print -quit)

# Run linker with --list to get a list of the libraries
out=$($QEMU $ld_toolchain \
	--library-path $TOOLCHAIN_PATH/$TRIPLET/sysroot/usr/lib:$TOOLCHAIN_PATH/$TRIPLET/sysroot/lib \
	--inhibit-cache \
	--list $BINARY)

bundler=$(mktemp -d --tmpdir bundler.XXXXXX)

while read x xx d addr; do
	s=${d#$TOOLCHAIN_PATH/$TRIPLET/sysroot/}
	install -D -T $d $bundler/$s
done <<<$out

if [ ! -f "$OUTPUT" ]; then
	tar -cf "$OUTPUT" --files-from=/dev/null
fi

tar -C $bundler -rf "$OUTPUT" .

rm -rf $bundler
