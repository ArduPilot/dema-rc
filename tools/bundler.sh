#!/bin/bash

TOOLCHAIN_PATH=$1
TRIPLET=$2
BINARY=$3

# Get linker from ELF header
ld=$(readelf --program-headers $BINARY | sed -n 's/ *\[Requesting program interpreter: \(.*\)]/\1/p')

# Find same linker in the toolchain dir; any of the copies there should work
ld_toolchain=$(find $TOOLCHAIN_PATH/$TRIPLET -name "$(basename $ld)" -print -quit)

# Run linker with --list to get a list of the libraries
out=$($ld_toolchain \
	--library-path $TOOLCHAIN_PATH/$TRIPLET/sysroot/usr/lib:$TOOLCHAIN_PATH/$TRIPLET/sysroot/lib \
	--inhibit-cache \
	--list $BINARY)

bundler=$(mktemp -d --tmpdir bundler.XXXXXX)

while read x xx d addr; do
	s=${d#$TOOLCHAIN_PATH/$TRIPLET/sysroot/}
	install -D -T $d $bundler/$s
done <<<$out

rm -f deps.tar
tar -C $bundler -cf deps.tar .

rm -rf $bundler
