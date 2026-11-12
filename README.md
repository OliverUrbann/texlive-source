# TeX Live for AmigaOS 3

This fork of [texlive-source](https://github.com/TeX-Live/texlive-source)
ports TeX Live to classic AmigaOS 3.x (68020 or better, ixemul.library):
pdfTeX, LuaTeX with HarfBuzz, TeX, BibTeX, MakeIndex. AmigaOS 4 and MorphOS
are planned.

**Download:** the
[releases page](https://github.com/OliverUrbann/texlive-source/releases)
has a ready package with the programs, a texmf tree from TeX Live 2023
(LaTeX, KOMA-Script, TikZ, beamer, biblatex, latex-extra, ...) and the
formats. Unpack it anywhere, double-click `TeX-Shell`, and run
`pdflatex`, `lualatex`, `bibtex`, `makeindex`.

Status: tested in an emulator (FS-UAE, Kickstart 3.1, 68030) against the
output of TeX Live on Linux; not yet tested on real hardware.

- [amiga/README.amiga.md](amiga/README.amiga.md): what works, what is
  missing, how it was done, how to build
- [amiga/ReadMe](amiga/ReadMe): the user documentation in the package
  (German and English)
- [README](README): the original TeX Live source README; the original
  README.md of TeX-Live/texlive-source follows below

Everything specific to the Amiga is in `amiga/` and inside
`#if defined (__ixemul__)` in the sources; nothing of TeX Live was
removed.

---

*Original README.md of TeX-Live/texlive-source:*

$Id$

This README.md file exists for people browsing on github.

The TeX-Live project on github (https://github.com/TeX-Live) is a mirror
of the upstream Subversion repository (https://tug.org/texlive/svn). We
use it primarily for automatic building of the sources on several platforms.

The releases built here (https://github.com/TeX-Live/texlive-source/releases)
are not usable TeX systems on their own, but bare binary directories.
They can be dropped into a TeX Live installation as some bin/newdir and
they should work, including resolving the dangling symlinks that are in
the bare tarballs.

For reports and other feedback, we prefer using mailing lists:
tex-live@tug.org for general bugs and discussion, tex-k@tug.org for bugs
and patches for the compiled programs, and so on. All the TL mailing
lists: https://tug.org/texlive/lists.html.

However, if you prefer to open issues or submit PRs on github, that's
ok. We will see them and will reply as soon as we can.

One common problem: Package bug reports should go to the package
maintainer, however they wish to receive reports, not to any general
TeX Live list. TL redistributes what is uploaded to CTAN without changes.

For technical information about the source tree here, see the ./README
file here, not this README.md.
