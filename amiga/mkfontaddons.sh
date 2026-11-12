#!/bin/sh
# Build the font add-on archives TeX-Fonts-*.lha.  Each archive holds
# TeX/texmf-local/fonts/... and TeX/texmf-local/doc/fonts/ (licences), so
# that it is unpacked where the base package was unpacked.
#
# usage: amiga/mkfontaddons.sh <out-dir>
#
# Takes the fonts from these Debian/Ubuntu packages, which must be
# installed: fonts-freefont-otf fonts-cmu fonts-hosny-amiri
# fonts-sil-scheherazade fonts-noto-core fonts-droid-fallback
set -e
out=${1:?usage: mkfontaddons.sh out-dir}
mkdir -p "$out"
out=$(cd "$out" && pwd)
N=/usr/share/fonts/truetype/noto

# put <archive> <fonts subdir> <files...>
put () {
	a=$1 sub=$2
	shift 2
	mkdir -p "$out/$a/TeX/texmf-local/fonts/$sub"
	cp -L "$@" "$out/$a/TeX/texmf-local/fonts/$sub/"
}
# licence <archive> <debian packages...>
licence () {
	a=$1
	shift
	mkdir -p "$out/$a/TeX/texmf-local/doc/fonts"
	for p in "$@"; do
		cp "/usr/share/doc/$p/copyright" "$out/$a/TeX/texmf-local/doc/fonts/$p.copyright"
	done
}

a=TeX-Fonts-Europe
put $a opentype/public/freefont /usr/share/fonts/opentype/freefont/*.otf
put $a truetype/public/cmu /usr/share/fonts/truetype/cmu/*.ttf
licence $a fonts-freefont-otf fonts-cmu

a=TeX-Fonts-Arabic-Hebrew
put $a truetype/public/amiri /usr/share/fonts/opentype/fonts-hosny-amiri/Amiri-Regular.ttf \
	/usr/share/fonts/opentype/fonts-hosny-amiri/Amiri-Bold.ttf \
	/usr/share/fonts/opentype/fonts-hosny-amiri/Amiri-Italic.ttf \
	/usr/share/fonts/opentype/fonts-hosny-amiri/Amiri-BoldItalic.ttf
put $a truetype/public/scheherazade /usr/share/fonts/truetype/scheherazade/*.ttf
put $a truetype/public/noto $N/NotoSansHebrew-*.ttf $N/NotoSerifHebrew-*.ttf \
	$N/NotoNaskhArabic-*.ttf $N/NotoSansArabic-Regular.ttf $N/NotoSansArabic-Bold.ttf
licence $a fonts-hosny-amiri fonts-sil-scheherazade fonts-noto-core

a=TeX-Fonts-Asia-Africa
for s in Devanagari Bengali Gujarati Gurmukhi Kannada Malayalam Oriya Tamil \
	Telugu Sinhala Thai Lao Khmer Myanmar Georgian Armenian Ethiopic Tibetan; do
	for f in $N/NotoSans$s-Regular.ttf $N/NotoSans$s-Bold.ttf \
		$N/NotoSerif$s-Regular.ttf $N/NotoSerif$s-Bold.ttf; do
		[ -f "$f" ] && put $a truetype/public/noto "$f"
	done
done
licence $a fonts-noto-core

a=TeX-Fonts-CJK
put $a truetype/public/droid /usr/share/fonts/truetype/droid/DroidSansFallback*.ttf
licence $a fonts-droid-fallback

cd "$out"
for a in TeX-Fonts-*/; do
	a=${a%/}
	rm -f "$a.lha"
	(cd "$a" && lha aq "../$a.lha" TeX)
	ls -l "$a.lha"
done
