#!/bin/sh
# Make the file list for mkpackage.sh from the installed Debian/Ubuntu
# packages of TeX Live: all packages that texlive-full depends on, without
# the -doc packages and without the programs (see "pkgs" below), plus the
# font packages that texlive-fonts-extra-links points to.  Install them
# first (apt-get install), so that updmap, update-language etc. have run.
#
# usage: amiga/mktexmflist.sh > texmf-list
#
# Left out: documentation and sources; files of programs that are not
# ported (XeTeX, ConTeXt, dvips, dvipdfmx, xdvi, Metafont, upTeX/pTeX,
# tex4ht, LaTeX-dev formats); scripts except Lua scripts (luaotfload-tool
# needs those); caches and formats of the Linux installation.
set -e
pkgs=$(apt-cache depends texlive-full | awk '/Depends/{print $2}' | tr -d '<>' |
	grep -v -- '-doc$' |
	grep -v -E '^(texlive-binaries|context|context-modules|texlive-full|lcdf-typetools|biber|xindy|xindy-rules|prerex|vprerex|chktex|lacheck|latexmk|latexdiff|purifyeps|dvidvi|dvipng|psutils|t1utils|tipa|feynmf|fragmaster|texlive-extra-utils|texlive-xetex|texinfo|latex-cjk-all|asymptote|info)$')
fonts=$(apt-cache depends texlive-fonts-extra-links | awk '/Depends/{print $2}' | grep '^fonts-')
all=$(for p in $pkgs $fonts fonts-lmodern fonts-texgyre tex-common; do dpkg -L "$p" 2>/dev/null; done; find /var/lib/texmf \( -type f -o -type l \))
{
echo "$all" | grep -E '/texmf-dist/|/usr/share/texmf/|/var/lib/texmf/' |
	grep -v -E '/doc/|/source/|/tex/xelatex/|/tex/context/|/tex/latex-dev/|/tex4ht/|/texmf-dist/scripts/|/(texmf-dist|usr/share/texmf|var/lib/texmf)/(dvips|dvipdfmx|xdvi|metafont|web2c/.*\.(fmt|log|base)|luatex-cache|context|xelatex)/?|/fonts/misc/xetex/|ls-R$|/fonts/(tfm|vf|ofm|type1|opentype|truetype)/(uptex-fonts|ptex-fonts|japanese-otf)/|/var/lib/texmf/web2c/'
echo "$all" | grep -E '/texmf-dist/scripts/.*\.(lua|tlu|texlua)$'
} | sort -u | while read -r f; do [ -f "$f" ] && echo "$f"; done
