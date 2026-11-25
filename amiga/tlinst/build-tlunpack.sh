#!/bin/sh
# Build tlunpack for the host (to build packages) or for the Amiga.
#
# usage: amiga/tlinst/build-tlunpack.sh <output> [cc]
#   cc  default: cc (host); for AmigaOS 3:
#       "m68k-amigaos-gcc -mcrt=ixemul -m68020"
set -e
here=$(cd "$(dirname "$0")" && pwd)
src=$(cd "$here/../.." && pwd)
out=${1:?usage: build-tlunpack.sh <output> [cc]}
cc=${2:-cc}
pp=$src/libs/pplib/pplib-src/src
$cc -O2 -std=gnu99 -DXZ_USE_CRC64 -DXZ_USE_SHA256 \
	-I"$here/xz" -I"$pp" -I"$pp/util" \
	-o "$out" "$here/tlunpack.c" \
	"$here/xz/xz_crc32.c" "$here/xz/xz_crc64.c" "$here/xz/xz_sha256.c" \
	"$here/xz/xz_dec_lzma2.c" "$here/xz/xz_dec_stream.c" \
	"$pp/util/utilsha.c"
