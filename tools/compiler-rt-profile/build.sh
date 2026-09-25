#!/bin/sh

set -eu

if [ "$#" -lt 7 ]; then
	echo "usage: $0 cc ar compiler-rt-source srcroot objroot output target-flags..." >&2
	exit 2
fi

cc=$1
ar=$2
compiler_rt_source=$3
srcroot=$4
objroot=$5
output=$6
shift 6

profile_source="$compiler_rt_source/lib/profile"
object_dir="${output}.objects"
mkdir -p "$object_dir"

for source in \
	InstrProfiling.c \
	InstrProfilingBuffer.c \
	InstrProfilingInternal.c \
	InstrProfilingNameVar.c \
	InstrProfilingPlatformDarwin.c \
	InstrProfilingVersionVar.c \
	InstrProfilingWriter.c
do
	object="$object_dir/${source%.c}.o"
	"$cc" "$@" \
		-ffreestanding \
		-fno-builtin \
		-fno-stack-protector \
		-nostdlibinc \
		-DKERNEL=1 \
		-I "$srcroot/tools/compiler-rt-profile/include" \
		-I "$srcroot/EXTERNAL_HEADERS" \
		-I "$objroot/EXPORT_HDRS/osfmk" \
		-I "$objroot/EXPORT_HDRS/bsd" \
		-I "$objroot/EXPORT_HDRS/libkern" \
		-I "$compiler_rt_source/include" \
		-I "$profile_source" \
		-c "$profile_source/$source" \
		-o "$object"
done

"$ar" rcs "$output" "$object_dir"/*.o
