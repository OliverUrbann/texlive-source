# TeX Live on AmigaOS 3

A port of parts of TeX Live to classic AmigaOS 3.x: 68020 or better,
`ixemul.library`. AmigaOS 4 and MorphOS are planned but not started; the
code is laid out so they can be added (see "Other Amiga systems").

A ready package is on the
[releases page](https://github.com/OliverUrbann/texlive-source/releases):
the programs and a package installer (about 6 MB). Unpack it anywhere,
double-click `Install-Packages` to load the TeX Live packages you want,
then `TeX-Shell`. Its `ReadMe` (German and English) explains the use;
this file is about the port itself.

## Programs

| Program | What it is for |
|---|---|
| `pdftex`, `pdflatex` | LaTeX to PDF, the usual 8-bit engine |
| `luahbtex`, `lualatex` | Unicode input, OpenType/TrueType fonts, Lua; HarfBuzz shaping (the default here, see below) |
| `luatex` | LuaTeX without HarfBuzz |
| `tex` | Knuth's TeX, DVI output |
| `bibtex` | bibliographies |
| `makeindex` | indexes and glossaries |
| `kpsewhich` | finds files the way TeX does |
| `mktexlsr` | writes the `ls-R` file name databases (new, in C; the original is a shell script) |
| `tlunpack` | checks and unpacks TeX Live package containers (new, in C) |
| `tlinst` | the package installer, a texlua script (`tlpkg/tlinst.lua`, new) |

`pdflatex` and `lualatex` are the same files as `pdftex` and `luahbtex`
under another name, as in TeX Live; `tlinst` makes these copies on the
Amiga, so that the archive stays small. `luatex` is built but not in the
package: `luahbtex` does all it does.

## Status

Tested in FS-UAE (A1200, 68030/68882, Kickstart 3.1, ixemul 80.1), with
64 MB for pdfTeX and 128 MB for LuaTeX. Not yet tested on real hardware.
The final test run was made without the emulator's JIT compiler: with JIT,
FS-UAE now and then loses the FPU control register, and programs then
print wrong numbers or hang. That is the emulator, not the port.
Each result was compared with the output of TeX Live on Linux for the
same input (text, page count, fonts, rendered pages).

| Test | Result |
|---|---|
| plain TeX format and document | DVI identical |
| article: babel (ngerman), UTF-8 input, T1 + Latin Modern, amsmath, graphicx, booktabs, siunitx, listings, hyperref, natbib with `bibtex`, index with `makeindex` | same |
| TikZ with pgfplots | same |
| KOMA-Script report with biblatex (`backend=bibtex`), csquotes | same |
| beamer slides | same |
| PNG, JPEG and PDF images | same |
| 139-page book: 150 references, 25 plots, tables, longtable, index | same; a few pixels differ on some pages, cause not found (the Linux pdfTeX is an older version). Tested with JIT only, before the last changes |
| LuaLaTeX with fontspec: Latin Modern OpenType; DejaVu Serif TrueType with Greek and Cyrillic | same |
| LuaLaTeX with `Renderer=HarfBuzz`: Latin Modern; DejaVu Sans with Arabic, right to left | same |
| the math functions added for ixemul (`amiga/tests/mathtest.c`) | same values, except the sign of zero, the last digit of two float results, and `pow(0, negative)`, which is ixemul's own |

In the article test 23 pixels differ on page 2; cause not found.

Not tested: JBIG2 images, `\write18`/shell escape, network access from
Lua, MetaPost inside LuaTeX, and the small web2c tools (`dvicopy`,
`patgen`, `tftopl`, ...), which the build compiles but does not copy to
`dist`.

## What is not there

Switched off in `amiga/build.sh` with `--disable-...` flags, nothing was
removed from the source:

- `biber`: a Perl program. Use biblatex with `backend=bibtex`.
- `bibtex8`, `bibtexu`: tied to the ICU library in TeX Live's build.
- `xetex`: needs ICU and Fontconfig. `lualatex` covers the same ground.
  ICU is the obstacle: it needs C++ threads, which the toolchain does not
  have (thread model "single").
- LuaJIT (`luajittex`): no m68k back end.
- Metafont, MetaPost, dvips, dvipdfmx, pTeX/upTeX and the remaining
  programs: not tried yet.
- `mktexfmt`, `mktexpk`, `mktextfm` (automatic generation): shell scripts.
  Formats are built by `MakeFormats`; bitmap fonts are not needed for PDF
  output with Type 1 or OpenType fonts.

## Building

On Linux, with the m68k-amigaos toolchain from
<https://github.com/AmigaPorts/amiga-gcc> (`make min ndk`) in `PATH`, and
`gcc-multilib` installed:

    amiga/build.sh [build-dir]

The result is in `build-dir/dist/TeX`. The script explains each compiler
flag.

## Installing packages (tlinst)

The programs of this port belong to one TeX Live year (the source is
trunk of July 2025, so TeX Live 2025), and so must the packages. The
installer therefore takes them from the *frozen* release of that year
in TUG's historic archive, which never changes:
`http://ftp.math.utah.edu/pub/tex/historic/systems/texlive/2025/tlnet-final/`.
Plain HTTP, since texlua has no TLS on AmigaOS 3; the SHA-512 sums of
`texlive.tlpdb` protect the downloads. (Note: that server refuses
HTTP/1.0 requests; luasocket uses HTTP/1.1.)

- `amiga/tlinst/mkindex.py` turns `texlive.tlpdb` (20 MB) into
  `tlpkg/tlinst.idx` (1 MB): names, categories, dependencies, container
  sizes and checksums, `execute` lines, short descriptions. Left out:
  platform packages, and the engines that are not ported (`EXCLUDE`,
  including pTeX/upTeX and what only they use). Dependencies of single
  packages on whole collections are dropped (`yb-book` would otherwise
  pull in all of `collection-fontsextra`).
- `amiga/tlinst/tlinst.lua` (texlua; `tlnet.lua` does the HTTP part)
  resolves dependencies, downloads `archive/<name>.tar.xz` (or takes it
  from a local copy of `tlnet-final`), unpacks it with `tlunpack`, and
  keeps a list of the files of each package (`tlpkg/lists`) for `remove`.
  Then it does what tlmgr and updmap do on Unix: `language.dat`,
  `language.def`, `language.dat.lua` from the `AddHyphen` lines and
  `pdftex.map` from the `addMap`/`addMixedMap` lines (as updmap with its
  default settings), all in `texmf-var`; checked to be identical with the
  files `install-tl` makes for the same packages. Then ls-R, the formats
  (`MakeFormats`) and luaotfload's font list, if needed.
- `amiga/tlinst/tlunpack.c`: SHA-512 (pplib's code), xz (XZ Embedded,
  in `amiga/tlinst/xz`) and tar, all streamed, so memory does not depend
  on the size of a package. Also builds for the host
  (`build-tlunpack.sh`).
- `amiga/Install-Packages`: the Workbench front end, an Installer script.
- `amiga/mkcore.sh` puts the drawer together, optionally with packages
  already in it (installed with `tlinst` on the host; the formats are
  then built at the first start of the TeX-Shell).

The same `tlinst.lua` runs on the build host (with TeX Live's Linux
texlua and a host `tlunpack`), with `--root`, `--no-formats`,
`--no-fontlist` and `--mktexlsr lua`.

In FS-UAE (68030 with JIT) scheme-basic, 114 packages, installs over the
network in about three minutes including formats; per package the
download and unpacking take well under a second, a font package of
some MB a few seconds.

## Making the package (complete tree, the old way)

`amiga/mkpackage.sh` puts the drawer together: the programs from
`build.sh`, a texmf tree taken from a list of files (a TeX Live
installation or the files of Debian/Ubuntu texlive packages), ixemul,
`TeX-Shell` with its icon (drawn by `amiga/mkicon.py`) and the `ReadMe`.
The released package has the complete texmf tree of TeX Live 2023 as
packaged by Ubuntu 24.04: all packages that `texlive-full` depends on,
without the `-doc` packages and the programs, plus the font packages that
`texlive-fonts-extra-links` points to. Install them on the build host,
then `amiga/mktexmflist.sh` writes the list for `mkpackage.sh`. It leaves
out documentation and sources and the files of programs that are not
ported (XeTeX, ConTeXt, dvips, dvipdfmx, xdvi, Metafont, upTeX/pTeX,
tex4ht). That is about 3.1 GB in 152000 files.

`mkpackage.sh` also builds LuaLaTeX's font list (luaotfload's names
database) with the host's `luaotfload-tool`, on the package's tree only.
On the Amiga that takes over an hour in the emulator, because luaotfload
reads all of the 3500 or so OpenType/TrueType fonts. The list stores
host paths, but luaotfload finds fonts of a texmf tree again by their
file name through kpathsea, so it works wherever the package is
unpacked. Two details: only the text form (`.lua.gz`) is shipped, since
the `.luc` is bytecode of the host's Lua; and the host's Lua writes some
numbers as hexadecimal floats (`0x0p+0`), which LuaTeX on the Amiga does
not read (its number conversion uses ixemul's `strtod`), so
`amiga/fixhexfloat.py` rewrites them as decimals. When files are added
to `texmf-local`, the TeX-Shell updates the list on the Amiga; since the
stored paths differ, luaotfload then reads all fonts again.

`amiga/mkfontaddons.sh` built font add-ons for the first package, which
had a smaller tree; the complete tree has fonts for all scripts.

## Languages and fonts

All hyphenation patterns of TeX Live are in the base package, but only
LuaLaTeX uses all of them:

- `tex/luatex/config/language.dat` (and `.def`) list all 84 languages.
  LuaLaTeX's search path has `tex/luatex` before `tex/generic`, so it
  finds this file; babel assigns the languages when a document starts,
  and LuaTeX loads the patterns of a language when it is first used.
  `tex/generic/config/language.dat.lua` is the complete list as well.
- `tex/generic/config/language.dat` lists English, German and Greek. It
  is what pdfTeX reads when its formats are built, and pdfTeX puts the
  patterns of every listed language into the format. With all languages
  the pdfLaTeX format needs `trie_size = 1100000` and more memory.

babel's language descriptions (`.ini` files, more than 380 languages)
come with babel itself; the classic `babel-<language>` packages are
included too.

Fonts are the other half: the tree has fonts for all common scripts
(CMU, FreeSerif, Amiri, Culmus, Shobhika, TLWG, Fandol, Harano Aji,
Baekmuk, UnFonts and many more). Further OpenType/TrueType fonts go into
`texmf-local`, which is in kpathsea's search path before `texmf-dist`.
The TeX-Shell runs `mktexlsr --if-changed TeX:texmf-local` at every
start, which rewrites that `ls-R` only when files were added or removed
(it then returns WARN). In that case, and when there is no font list,
the TeX-Shell runs `luaotfload-tool --update`. luaotfload would try that
by itself during a LuaLaTeX run, but on the Amiga that attempt never
saves the list, so every font looked up by name then starts another
search through all fonts; why it does not save is not known. Run from
the TeX-Shell, the tool works.

The formats must be built by the Amiga programs: `Execute MakeFormats` in
the drawer, on an Amiga or in an emulator. Then pack the drawer with LhA.

    TeX/TeX-Shell(.info)             opens a Shell set up for TeX
    TeX/S/TeX-Startup, TeX-Assigns   what the TeX-Shell runs
    TeX/MakeFormats                  rebuilds the formats
    TeX/bin/m68k-amigaos/...
    TeX/Libs/ixemul.library, ixnet.library
    TeX/texmf.cnf                    from amiga/texmf.cnf
    TeX/texmf-dist/...               TeX Live's tree, web2c/texmf.cnf from kpathsea
    TeX/texmf-local/                 for the user's own packages
    TeX/texmf-var/                   formats, LuaTeX's font cache

The programs find the tree from their own location, so it can be anywhere.
Nothing is copied into the system: the TeX-Shell (`S/TeX-Assigns`) adds
`TeX:Libs` to `LIBS:`, assigns `AmiTCP:` if no TCP stack did (ixemul asks
for that volume otherwise) and adds the programs to the Shell's path.

File names can be given in Amiga form (`Work:doc/a.tex`) or in ixemul's
Unix form (`/Work/doc/a.tex`). Search paths in `texmf.cnf` and in
environment variables must use the Unix form, because `:` separates
entries there.

Run `mktexlsr` after adding files to the tree.

## Time and memory

Building the LaTeX format took about 20 minutes on the emulated 68030
without JIT, a one-page LaTeX document about 15 seconds.

TeX Live's default array sizes allocate well over 100 MB. `amiga/texmf.cnf`
reduces them to about 56 MB for pdfTeX: `main_memory` 3 million words
(fixed in the format) plus `extra_mem_top` 2 million words, which is
added at run time for one-word nodes, i.e. tokens (`extra_mem_bot`, the
other part, holds boxes and glue and did not help). A 100-page thesis with TikZ and pgfplots needed 3.5
million. The largest item is `main_memory`:
the LaTeX kernel's Unicode tables need about 2 million words while the
format is built, and that high-water mark stays in the format. Plain TeX
needs far less; lower the values there if RAM is short.

LuaTeX allocates as it goes. Building the LuaLaTeX format failed with
64 MB and worked with 128 MB. The first LuaLaTeX run caches each font it
loads in `texmf-var/luatex-cache`.

Fonts decide LuaLaTeX's memory use. With luaotfload's own shaper
(`mode=node`) the first load of a big font turns all its tables into Lua
tables: measured natively, FreeSerif took 709 MB, Amiri 599 MB. On the
Amiga, Lua allocations start to fail at about 100 MB of Lua heap even
with more RAM free, and luaotfload then reports the font as not found.
With HarfBuzz (`mode=harf`) the font stays in HarfBuzz's compact form and
needs a few MB. fontspec passes `mode=node` itself, so a luaotfload
setting does not help; the package therefore installs
`tex/lualatex/config/fontspec.cfg` (from `amiga/texmf/`) with
`\defaultfontfeatures{Renderer=HarfBuzz}`. `Renderer=Node` in a document
still works for small fonts.

babel creates one font instance per language (different script and
features), so a document with five languages loads its font five times:
FreeSerif needed 394 MB natively for `examples/languages.tex`, CMU Serif
163 MB. The example therefore uses CMU Serif. In FS-UAE with 128 MB
Zorro III RAM, single-script documents work (FreeSerif Cyrillic, Amiri,
Noto Sans Devanagari); `chinese.tex` (Droid Sans Fallback) and
`languages.tex` need the 256 MB configuration.

The stack is set to 512 KB in the programs themselves (`__stack` in
`kpathsea/amiga.c`).

## What was changed

Everything is inside `#if defined (__ixemul__)`.

- `amiga/ixemul-fixes.h` (new), included into every file by the compiler
  (`-include`): ixemul's `va_start`/`va_arg` macros read wrong values with
  a current gcc (this showed as kpathsea not finding any file); names of
  C99 and POSIX functions that ixemul lacks.
- `texk/kpathsea/amiga.h`, `amiga.c` (new): the stack size; the program's
  directory from AmigaDOS (`GetProgramDir`), because `argv[0]` and a `PATH`
  variable do not help on the Amiga; Amiga path to Unix path; the missing
  math functions, `nanosleep`, `localtime_r`, `gmtime_r`; `getaddrinfo`
  and friends (IPv4, on top of `gethostbyname`, for luasocket);
  `system()` through the AmigaDOS shell (`SystemTags`), because ixemul's
  runs `/bin/sh` (this makes Lua's `os.execute` and `\write18` work);
  three newlib symbols that the toolchain's libstdc++ expects.
- `texk/kpathsea/progname.c`: uses the program directory; sets `PATH` if
  there is none (Lua libraries expect it); `KPATHSEA_DEBUG_OUTPUT` works
  as on Windows (the 3.1 Shell cannot redirect stderr).
- `texk/kpathsea/tex-make.c`: does not try to run the `mktex...` scripts.
- `texk/kpathsea/line.c`, `texk/web2c/lib/texmfmp.c`,
  `luatexdir/lua/luainit.c`: locking; Amiga paths on the command line.
- `luatexdir/luasocket/src/amiga-ipv6.h` (new), `usocket.h`: IPv6
  definitions so that luasocket compiles. The socket library is off by
  default in LuaTeX (but there in texlua, which `tlinst` uses).
- `luatexdir/luaffi/ffi.h`: no `dlfcn.h`/`sys/mman.h`; ffi is a stub on
  m68k as on other unsupported processors.
- `luatexdir/luafilesystem/src/lfs.c`: `lfs.lock` succeeds without
  locking.
- `amiga/cxx/` (new) and a generated `cxx-compat` directory: what the C++
  compiler needs to use libstdc++'s headers over ixemul's C library
  (HarfBuzz and Graphite are C++). `amiga/build.sh` explains it.
- ICU is left out of the build although TeX Live lists it as a dependency
  of HarfBuzz; HarfBuzz does not use it.
- `amiga/tlinst/`, `amiga/Install-Packages`, `amiga/mkcore.sh` (new):
  the package installer, see above.
- `amiga/mktexlsr.c`, `amiga/MakeFormats`, `amiga/TeX-Shell`, `amiga/S/`,
  `amiga/mkpackage.sh`, `amiga/mkicon.py`, `amiga/ReadMe`,
  `amiga/texmf.cnf`, `amiga/build.sh`, `amiga/tests/mathtest.c` (new).

kpathsea's old `#ifdef AMIGA` code (SAS/C, native Amiga paths) is still
there but not used: the build passes `-UAMIGA`.

## Other Amiga systems

`kpathsea/amiga.h` is included for `__amigaos__`, `__amigaos4__` and
`__MORPHOS__`, and its contents are keyed by the reason (`__ixemul__`
today). For a new system:

- add its section to `amiga.h`/`amiga.c`; most of the ixemul work-arounds
  (math, newlib symbols, `va_arg`, IPv6, the C++ layer) should not be needed with a
  current C library, the program directory and path conversion probably
  are;
- copy `amiga/build.sh` and change the compiler and flags;
- remove `--disable-...` flags to try more programs.

## Debugging

`SetEnv KPATHSEA_DEBUG_OUTPUT /Work/kdebug.txt` and
`SetEnv KPATHSEA_DEBUG -1` (or `kpsewhich -debug=-1 file`) write
kpathsea's search trace to a file. TeX's own `.log` is written in large
blocks, so it stays empty for a long time during a long run.
