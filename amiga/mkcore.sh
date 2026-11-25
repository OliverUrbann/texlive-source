#!/bin/sh
# Put together the core archive: programs, ixemul.library, the TeX-Shell
# and the package installer (tlpkg/), without TeX Live packages or with
# only a few.  Everything else is installed on the Amiga with
# Install-Packages or tlinst, from the frozen TeX Live year the index
# was made for.
#
# usage: amiga/mkcore.sh <build-dir> <tlinst.idx> <ixemul-dir> <out-dir> [package...]
#
#   build-dir   the directory given to amiga/build.sh (uses dist/TeX)
#   tlinst.idx  made by amiga/tlinst/mkindex.py from texlive.tlpdb
#   ixemul-dir  a directory with ixemul.library and ixnet.library
#               (68020 versions from ixemul 80.1)
#   out-dir     the drawer is created as out-dir/TeX
#   package...  TeX Live packages to put in already (e.g. scheme-basic),
#               installed with tlinst on this host: needs texlua in PATH
#               and a host tlunpack (amiga/tlinst/build-tlunpack.sh).
#               The formats are built on the Amiga at the first start of
#               the TeX-Shell.
#
# Environment: TLUNPACK (host tlunpack, default: tlunpack in PATH),
#              REPOSITORY (default: from the index).
set -e
src=$(cd "$(dirname "$0")/.." && pwd)
build=$1 idx=$2 ixemul=$3 out=$4
[ -n "$out" ] || { sed -n '2,24p' "$0"; exit 1; }
shift 4

pkg=$out/TeX
bin=$pkg/bin/m68k-amigaos
mkdir -p "$bin" "$pkg/Libs" "$pkg/S" "$pkg/tlpkg" "$pkg/texmf-local" \
	"$pkg/texmf-var" "$pkg/examples" "$pkg/texmf-dist/web2c"

echo "== programs"
# luatex is left out (luahbtex does all it does); pdflatex and lualatex
# are copies that tlinst makes on the Amiga
for p in tex pdftex luahbtex bibtex makeindex kpsewhich mktexlsr; do
	cp "$build/dist/TeX/bin/m68k-amigaos/$p" "$bin/"
done
m68k-amigaos-strip "$bin"/* 2>/dev/null || true
PATH=/opt/amiga/bin:$PATH "$src/amiga/tlinst/build-tlunpack.sh" "$bin/tlunpack" \
	"m68k-amigaos-gcc -mcrt=ixemul -m68020 -s"
cp "$src/amiga/texmf.cnf" "$pkg/texmf.cnf"
cp "$src/texk/kpathsea/texmf.cnf" "$pkg/texmf-dist/web2c/texmf.cnf"
cp "$ixemul/ixemul.library" "$ixemul/ixnet.library" "$pkg/Libs/"

echo "== installer"
cp "$src/amiga/tlinst/tlinst.lua" "$src/amiga/tlinst/tlnet.lua" "$idx" "$pkg/tlpkg/"
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/Install-Packages" >"$pkg/Install-Packages"
python3 "$src/amiga/mkicon.py" "$pkg/Install-Packages.info" Installer \
	"SCRIPT=Install-Packages" "APPNAME=TeX Live" "MINUSER=AVERAGE"

echo "== TeX-Shell, scripts, documentation"
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/TeX-Shell" >"$pkg/TeX-Shell"
python3 "$src/amiga/mkicon.py" "$pkg/TeX-Shell.info"
for f in TeX-Startup TeX-Assigns; do
	iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/S/$f" >"$pkg/S/$f"
done
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/MakeFormats" >"$pkg/MakeFormats"
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/ReadMe" >"$pkg/ReadMe"
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/texmf-local-ReadMe" >"$pkg/texmf-local/ReadMe"
cp "$src"/amiga/examples/*.tex "$src"/amiga/examples/*.bib "$src"/amiga/examples/*.png "$pkg/examples/"
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/examples/ReadMe" >"$pkg/examples/ReadMe"

if [ $# -gt 0 ]; then
	echo "== packages: $*"
	set -- --root "$pkg" --tlunpack "${TLUNPACK:-tlunpack}" --mktexlsr lua \
		--no-formats --no-fontlist ${REPOSITORY:+--repository "$REPOSITORY"} install "$@"
	texlua "$src/amiga/tlinst/tlinst.lua" "$@" | grep -v '^\[' || true
	rm -rf "$pkg/tlpkg/download"
fi

echo "== ls-R"
for d in texmf-dist texmf-local texmf-var; do
	(cd "$pkg/$d" && { echo '% ls-R -- filename database for kpathsea; do not change this line.'; echo; LC_ALL=C ls -R ./ ; } > ls-R)
done
du -sh "$pkg"
