#!/bin/sh
# Put together the package: a drawer "TeX" with the programs, a texmf
# tree, ixemul.library and the TeX-Shell.  Nothing has to be installed
# into the system; the user unpacks it anywhere.
#
# usage: amiga/mkpackage.sh <build-dir> <texmf-list> <ixemul-dir> <out-dir>
#
#   build-dir      the directory given to amiga/build.sh (uses dist/TeX)
#   texmf-list     a file with one absolute path per line: the files of
#                  the texmf tree.  Paths below .../texmf-dist/ or
#                  /usr/share/texmf/ are taken relative to that point, so
#                  a list of a TeX Live installation or of the files of
#                  Debian/Ubuntu texlive packages (dpkg -L) both work.
#                  Documentation and sources should be left out.
#                  Debian/Ubuntu generate pdftex.map below
#                  /var/lib/texmf/; add it to the list.
#
# Environment: LANG_FULL, LANG_PDF (see "Languages" below).
#   ixemul-dir     a directory with ixemul.library and ixnet.library
#                  (68020 versions, e.g. ixemul020/ixnet020 from ixemul 80.1)
#   out-dir        the package is created as out-dir/TeX
#
# Formats are not built here (they must be built by the Amiga programs):
# run "Execute MakeFormats" in the TeX drawer on the Amiga or in an
# emulator, then pack the drawer with LhA.
set -e

src=$(cd "$(dirname "$0")/.." && pwd)
build=$1 list=$2 ixemul=$3 out=$4
[ -n "$out" ] || { sed -n '2,20p' "$0"; exit 1; }

pkg=$out/TeX
mkdir -p "$pkg"
cp -r "$build/dist/TeX/." "$pkg/"
bin=$pkg/bin/m68k-amigaos
# as in TeX Live: the format is chosen by the program name
cp "$bin/pdftex" "$bin/pdflatex"
cp "$bin/luahbtex" "$bin/lualatex"

echo "== texmf tree"
python3 - "$list" "$pkg/texmf-dist" <<'EOF'
import os, shutil, sys
lst, dst = sys.argv[1], sys.argv[2]
n = 0
for line in open(lst):
    p = line.strip()
    if not os.path.isfile(p):
        continue
    for marker in ("/texmf-dist/", "/usr/share/texmf/", "/var/lib/texmf/"):
        i = p.find(marker)
        if i >= 0:
            rel = p[i + len(marker):]
            break
    else:
        continue
    if rel.startswith("web2c/texmf.cnf"):
        continue
    t = os.path.join(dst, rel)
    if os.path.exists(t):
        continue
    os.makedirs(os.path.dirname(t), exist_ok=True)
    shutil.copy(os.path.realpath(p), t)
    n += 1
print(n, "files")
EOF
cp "$src/texk/kpathsea/texmf.cnf" "$pkg/texmf-dist/web2c/texmf.cnf"

# Languages.  language.dat in tex/generic/config is read by pdfTeX when
# its formats are built; keep it short (English, German, Greek), so that
# the pdfLaTeX format stays small.  LuaLaTeX looks in tex/luatex/config
# first: give it the complete list (language.dat, language.def) and
# language.dat.lua.  The pattern files for all of them come from
# hyph-utf8 (a list of a full TeX Live installation contains it).
#   LANG_FULL: directory with the complete language.dat, .def, .dat.lua
#   LANG_PDF:  directory with the short language.dat and language.def
if [ -n "$LANG_FULL" ]; then
	mkdir -p "$pkg/texmf-dist/tex/luatex/config" "$pkg/texmf-dist/tex/generic/config"
	cp "$LANG_FULL/language.dat" "$LANG_FULL/language.def" "$pkg/texmf-dist/tex/luatex/config/"
	cp "$LANG_FULL/language.dat.lua" "$pkg/texmf-dist/tex/generic/config/"
fi
# LuaLaTeX: shape with HarfBuzz by default (much less memory); found
# before fontspec's own fontspec.cfg, see the comment in the file
mkdir -p "$pkg/texmf-dist/tex/lualatex/config"
cp "$src/amiga/texmf/fontspec.cfg" "$pkg/texmf-dist/tex/lualatex/config/"
if [ -n "$LANG_PDF" ]; then
	cp "$LANG_PDF/language.dat" "$LANG_PDF/language.def" "$pkg/texmf-dist/tex/generic/config/"
fi

echo "== ixemul"
mkdir -p "$pkg/Libs"
cp "$ixemul/ixemul.library" "$ixemul/ixnet.library" "$pkg/Libs/"

echo "== TeX-Shell"
# AmigaDOS files in ISO-8859-1
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/TeX-Shell" >"$pkg/TeX-Shell"
python3 "$src/amiga/mkicon.py" "$pkg/TeX-Shell.info"
mkdir -p "$pkg/S" "$pkg/texmf-local"
# a file, so that its ls-R is not empty (kpathsea warns about an empty one)
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/texmf-local-ReadMe" >"$pkg/texmf-local/ReadMe"
for f in TeX-Startup TeX-Assigns; do
	iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/S/$f" >"$pkg/S/$f"
done
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/ReadMe" >"$pkg/ReadMe"
mkdir -p "$pkg/examples"
cp "$src"/amiga/examples/*.tex "$src"/amiga/examples/*.bib "$src"/amiga/examples/*.png "$pkg/examples/"
iconv -f UTF-8 -t ISO-8859-1 "$src/amiga/examples/ReadMe" >"$pkg/examples/ReadMe"

echo "== ls-R"
for d in texmf-dist texmf-local; do
	(cd "$pkg/$d" && { echo '% ls-R -- filename database for kpathsea; do not change this line.'; ls -R . ; } > ls-R)
done
du -sh "$pkg"
