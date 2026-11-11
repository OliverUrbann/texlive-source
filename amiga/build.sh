#!/bin/sh
# Cross-build TeX Live's kpathsea, tex and pdftex for AmigaOS 3.x
# (68020+, ixemul.library) on Linux.
#
# usage: amiga/build.sh [build-dir]        (default: ./build-amiga)
#
# Needs in PATH: m68k-amigaos-gcc from https://github.com/AmigaPorts/amiga-gcc
# ("make min ndk"), and a native gcc.  TeX Live's configure also probes a
# native 32-bit compiler for LuaJIT, even though LuaJIT is not built:
# install gcc-multilib (Debian/Ubuntu).
#
# Result: <build-dir>/dist/TeX/bin/m68k-amigaos/{tex,pdftex,kpsewhich}
#         <build-dir>/dist/TeX/texmf.cnf, <build-dir>/dist/TeX/texmf-dist/web2c/texmf.cnf
# See amiga/README.amiga.md for what to add to make it a working TeX.
set -e

src=$(cd "$(dirname "$0")/.." && pwd)
build=${1:-$PWD/build-amiga}
jobs=${JOBS:-$(nproc 2>/dev/null || echo 2)}
cpu=${AMIGA_CPU:-68020}

command -v m68k-amigaos-gcc >/dev/null || {
	echo "m68k-amigaos-gcc not found in PATH" >&2
	exit 1
}
mkdir -p "$build"
build=$(cd "$build" && pwd)

# ---- 1. native tangle, ctangle, tie ---------------------------------
# A cross build cannot run the tangle it compiles; TeX Live expects the
# tools in PATH.  Build them from this source tree.
if [ ! -x "$build/native-bin/tangle" ]; then
	echo "== native tangle/ctangle/tie"
	mkdir -p "$build/native" "$build/native-bin"
	(
		cd "$build/native"
		"$src/configure" --disable-all-pkgs --enable-web2c --enable-tex \
			--disable-synctex --without-x --disable-shared >configure.log 2>&1
		# the top-level make configures the subdirectories
		make -j"$jobs" >make.log 2>&1
		make -C texk/kpathsea -j"$jobs" >>make.log 2>&1
		make -C texk/web2c -j"$jobs" tangle ctangle tie >>make.log 2>&1
		cp texk/web2c/tangle texk/web2c/ctangle texk/web2c/tie "$build/native-bin/"
	) || { echo "native build failed, see $build/native/*.log" >&2; exit 1; }
fi
PATH=$build/native-bin:$PATH
export PATH

# ---- 2. cross configure ------------------------------------------------
# Compiler flags:
#   -mcrt=ixemul           link against ixemul.library
#   -UAMIGA -Uamiga        kpathsea has old SAS/C-era "#ifdef AMIGA" code
#                          for native Amiga paths; with ixemul we use the
#                          Unix code paths.  The port's own code tests
#                          __ixemul__ (and, later, __amigaos4__/__MORPHOS__).
#   -include ixemul-fixes.h  see that file (broken va_arg in ixemul headers)
#   -std=gnu17 -fgnu89-inline
#                          gcc 16 defaults to C23, where "int f()" means
#                          "no arguments"; and ixemul's headers rely on
#                          GNU89 "extern inline".
flags="-mcrt=ixemul -m$cpu -UAMIGA -Uamiga -include $src/amiga/ixemul-fixes.h"

# Everything that is not tex/pdftex is switched off here.  To try another
# program later, remove its --disable flag and see what breaks.
mkdir -p "$build/cross"
cd "$build/cross"
if [ ! -f Makefile ]; then
	echo "== configure for m68k-amigaos"
	"$src/configure" \
		--host=m68k-amigaos --build="$("$src/build-aux/config.guess")" \
		--disable-all-pkgs --enable-web2c --enable-tex --enable-pdftex \
		--disable-etex --disable-ptex --disable-eptex --disable-uptex \
		--disable-euptex --disable-aleph --disable-hitex --disable-texprof \
		--disable-xetex --disable-mf --disable-mf-nowin --disable-mflua \
		--disable-mfluajit --disable-luatex --disable-luajittex \
		--disable-luahbtex --disable-luajithbtex --disable-mp --disable-pmp \
		--disable-upmp --disable-web-progs \
		--disable-mktexfmt-default --disable-mktexmf-default \
		--disable-mktexpk-default --disable-mktextfm-default \
		--disable-synctex --disable-tex-synctex --disable-pdftex-synctex \
		--without-x --disable-shared --enable-static --disable-largefile \
		CC="m68k-amigaos-gcc $flags" CXX="m68k-amigaos-g++ $flags" \
		CFLAGS="-O2 -std=gnu17 -fgnu89-inline" CXXFLAGS="-O2" \
		>configure.log 2>&1 || { echo "configure failed, see $build/cross/configure.log" >&2; exit 1; }
fi

# ---- 3. build ----------------------------------------------------------
# Not the plain top-level "make": it would also configure utils/, whose
# configure scripts fail for this host although nothing there is enabled.
# "make recurse" at the top configures and builds kpathsea.
echo "== build"
{
	make recurse -j"$jobs" &&
	make -C libs -j"$jobs" &&
	make -C texk -j"$jobs"
} >make.log 2>&1 || { echo "build failed, see $build/cross/make.log" >&2; exit 1; }

# ---- 4. collect --------------------------------------------------------
dist=$build/dist/TeX
mkdir -p "$dist/bin/m68k-amigaos" "$dist/texmf-dist/web2c"
for p in texk/web2c/tex texk/web2c/pdftex texk/kpathsea/kpsewhich; do
	m68k-amigaos-strip -o "$dist/bin/m68k-amigaos/$(basename $p)" $p
done
cp "$src/texk/kpathsea/texmf.cnf" "$dist/texmf-dist/web2c/texmf.cnf"
cp "$src/amiga/texmf.cnf" "$dist/texmf.cnf"
cp "$src/amiga/README.amiga.md" "$dist/README.amiga.md" 2>/dev/null || true
echo "== done:"
ls -l "$dist/bin/m68k-amigaos"
