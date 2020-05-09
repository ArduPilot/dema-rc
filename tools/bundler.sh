#!/bin/bash

set -e

QEMU=
DIR=0
OUTPUT=
EXCLUDE=()

args=0
while getopts "x:q:o:d" o; do
    case "${o}" in
        q) QEMU=${OPTARG}; args=$[$args + 2] ;;
        o) OUTPUT=${OPTARG}; args=$[$args + 2] ;;
	d) DIR=1; args=$[$args + 1] ;;
	x) EXCLUDE+=( ${OPTARG} ); args=$[$args + 2] ;;
        \?) exit 1;;
    esac
done
shift $args

if [ -z "$OUTPUT" ]; then
	if [ $DIR -eq 1 ]; then
		OUTPUT=deps
	else
		OUTPUT=deps.tar
	fi

fi
TOOLCHAIN_PATH=$1
TRIPLET=$2
BINARY=$3

function is_excluded() {
	local f

	for f in "${EXCLUDE[@]}"; do
		[[ $1 == *$f* ]] && return 0
	done

	return 1
}

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
	is_excluded $s && continue
	install -D -T $d $bundler/$s
done <<<$out

if [ $DIR -eq 1 ]; then
	mkdir -p $OUTPUT
	cp -a "$bundler/." "$OUTPUT/"
else
	if [ ! -f "$OUTPUT" ]; then
		tar -cf "$OUTPUT" --files-from=/dev/null
	fi
	tar -C $bundler -rf "$OUTPUT" .
fi

rm -rf $bundler
