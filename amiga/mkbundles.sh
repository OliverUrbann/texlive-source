#!/bin/sh
# Make the package bundles TeXLive2025-<name>.tar.xz (amiga/tlinst/bundles.cfg).
#
# usage: amiga/mkbundles.sh <tlinst.idx> <repository> <out-dir> [name...]
#
#   repository  a local copy of tlnet-final (with archive/), or its URL
#   name...     only these bundles (default: all)
#
# Each bundle is installed with tlinst on this host into a staging
# directory and packed with its file lists (tlpkg/lists) and its package
# list (tlpkg/bundles/<name>.dat), without configuration (ls-R, maps,
# formats: tlinst makes them on the Amiga).  xz -6: its 8 MB dictionary
# is what the Amiga needs to unpack.  Needs texlua (TeX Live's) in PATH
# and a host tlunpack (TLUNPACK, default tlunpack).
set -e
src=$(cd "$(dirname "$0")/.." && pwd)
idx=$1 repo=$2 out=$3
[ -n "$out" ] || { sed -n '2,15p' "$0"; exit 1; }
shift 3
mkdir -p "$out"
out=$(cd "$out" && pwd)
work=$out/.staging
cfg=$src/amiga/tlinst/bundles.cfg

grep -v '^#' "$cfg" | grep . | while read -r name base pkgs; do
	if [ $# -gt 0 ]; then
		case " $* " in *" $name "*) ;; *) continue ;; esac
	fi
	echo "== $name"
	s=$work/$name
	rm -rf "$s"
	mkdir -p "$s/tlpkg/bundles"
	cp "$idx" "$s/tlpkg/tlinst.idx"
	if [ "$base" != "-" ]; then
		# what the base has counts as installed: left out of this bundle
		[ -f "$work/$base.installed" ] || { echo "make $base first"; exit 1; }
		cp "$work/$base.installed" "$s/tlpkg/installed.dat"
	fi
	texlua "$src/amiga/tlinst/tlinst.lua" --root "$s" --repository "$repo" \
		--tlunpack "${TLUNPACK:-tlunpack}" --no-config install $pkgs | grep -v '^\[' || true
	sort "$s/tlpkg/installed.dat" > "$work/$name.installed"
	if [ "$base" != "-" ]; then
		comm -23 "$work/$name.installed" "$work/$base.installed" > "$s/tlpkg/bundles/$name.dat"
	else
		cp "$work/$name.installed" "$s/tlpkg/bundles/$name.dat"
	fi
	rm -f "$s/tlpkg/installed.dat" "$s/tlpkg/tlinst.idx"
	rm -rf "$s/tlpkg/download"
	echo "   $(wc -l < "$s/tlpkg/bundles/$name.dat") packages, $(find "$s" -type f | wc -l) files"
	(cd "$s" && LC_ALL=C find . -mindepth 1 -maxdepth 1 | sed 's#^\./##' | sort |
		tar --owner=0 --group=0 --numeric-owner -cf - -T - | xz -6 -T0) \
		> "$out/TeXLive2025-$name.tar.xz"
	rm -rf "$s"
	ls -la "$out/TeXLive2025-$name.tar.xz"
done
rm -rf "$work"/*/
