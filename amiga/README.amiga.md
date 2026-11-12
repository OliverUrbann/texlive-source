# TeX Live on AmigaOS 3

A port of parts of TeX Live to classic AmigaOS 3.x: 68020 or better,
`ixemul.library`. AmigaOS 4 and MorphOS are planned but not started; the
code is laid out so they can be added (see "Other Amiga systems").

## Programs

| Program | What it is for |
|---|---|
| `pdftex`, `pdflatex` | LaTeX to PDF, the usual 8-bit engine |
| `luatex`, `lualatex` | Unicode input, OpenType/TrueType fonts, Lua |
| `tex` | Knuth's TeX, DVI output |
| `bibtex` | bibliographies |
| `makeindex` | indexes and glossaries |
| `kpsewhich` | finds files the way TeX does |
| `mktexlsr` | writes the `ls-R` file name databases (new, in C; the original is a shell script) |

`pdflatex` and `lualatex` are the same files as `pdftex` and `luatex`
under another name; the `Install` script creates them.

## Status

Tested in FS-UAE (A1200, 68030/68882, Kickstart 3.1, ixemul 80.1), with
64 MB for pdfTeX and 128 MB for LuaTeX. Not yet tested on real hardware.
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
| 139-page book: 150 references, 25 plots, tables, longtable, index | same; a few pixels differ on some pages, cause not found (the Linux pdfTeX is an older version) |
| LuaLaTeX with fontspec: Latin Modern OpenType; DejaVu Serif TrueType with Greek and Cyrillic | same |

Not tested: JBIG2 images, `\write18`/shell escape, network access from
Lua, MetaPost inside LuaTeX, and the small web2c tools (`dvicopy`,
`patgen`, `tftopl`, ...), which the build compiles but does not copy to
`dist`.

## What is not there

Switched off in `amiga/build.sh` with `--disable-...` flags, nothing was
removed from the source:

- `biber`: a Perl program. Use biblatex with `backend=bibtex`.
- `bibtex8`, `bibtexu`: tied to the ICU library in TeX Live's build.
- `xetex`: needs ICU, HarfBuzz and Fontconfig. `lualatex` covers the same
  ground.
- `luahbtex` (HarfBuzz shaping, needed for Arabic, Indic scripts etc.) and
  LuaJIT: not tried yet. TeX Live's own `lualatex` is `luahbtex`; here it
  is `luatex`, where luaotfload does the shaping in Lua.
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

## Installing

Copy `dist/TeX` to the Amiga and add the texmf tree, so that it looks like
this:

    TeX/bin/m68k-amigaos/...
    TeX/texmf.cnf                     (from amiga/texmf.cnf)
    TeX/Install, TeX/MakeFormats
    TeX/texmf-dist/web2c/texmf.cnf    (TeX Live's own)
    TeX/texmf-dist/...                (from a TeX Live installation)

The programs find the tree from their own location, so it can be anywhere.
`texmf-dist` is not part of this repository; take it from a TeX Live
installation. The tests used the tree of TeX Live 2023.

ixemul needs `Assign AmiTCP:` to exist, otherwise each program asks for
the volume.

Then, in a Shell:

    CD Work:TeX
    Execute Install        ; pdflatex, lualatex, ls-R
    Execute MakeFormats    ; the formats, in texmf-var/web2c/
    Path Work:TeX/bin/m68k-amigaos ADD

and

    pdflatex paper.tex
    bibtex paper
    makeindex paper
    lualatex Work:doc/unicode.tex

File names can be given in Amiga form (`Work:doc/a.tex`) or in ixemul's
Unix form (`/Work/doc/a.tex`). Search paths in `texmf.cnf` and in
environment variables must use the Unix form, because `:` separates
entries there.

Run `mktexlsr` after adding files to the tree.

## Time and memory

Building the LaTeX format took about 20 minutes on the emulated 68030
without JIT, a one-page LaTeX document about 15 seconds.

TeX Live's default array sizes allocate well over 100 MB. `amiga/texmf.cnf`
reduces them to about 40 MB for pdfTeX. The largest item is `main_memory`:
the LaTeX kernel's Unicode tables need about 2 million words while the
format is built, and that high-water mark stays in the format. Plain TeX
needs far less; lower the values there if RAM is short.

LuaTeX allocates as it goes. Building the LuaLaTeX format failed with
64 MB and worked with 128 MB. The first LuaLaTeX run builds a font name
database and caches each font it loads in `texmf-var/luatex-cache`.

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
  and friends, which always fail; three newlib symbols that the
  toolchain's libstdc++ expects.
- `texk/kpathsea/progname.c`: uses the program directory; sets `PATH` if
  there is none (Lua libraries expect it); `KPATHSEA_DEBUG_OUTPUT` works
  as on Windows (the 3.1 Shell cannot redirect stderr).
- `texk/kpathsea/tex-make.c`: does not try to run the `mktex...` scripts.
- `texk/kpathsea/line.c`, `texk/web2c/lib/texmfmp.c`,
  `luatexdir/lua/luainit.c`: locking; Amiga paths on the command line.
- `luatexdir/luasocket/src/amiga-ipv6.h` (new), `usocket.h`: IPv6
  definitions so that luasocket compiles. The socket library is off by
  default in LuaTeX; name lookup does not work.
- `luatexdir/luaffi/ffi.h`: no `dlfcn.h`/`sys/mman.h`; ffi is a stub on
  m68k as on other unsupported processors.
- `luatexdir/luafilesystem/src/lfs.c`: `lfs.lock` succeeds without
  locking.
- `amiga/mktexlsr.c`, `amiga/Install`, `amiga/MakeFormats`,
  `amiga/texmf.cnf`, `amiga/build.sh` (new).

kpathsea's old `#ifdef AMIGA` code (SAS/C, native Amiga paths) is still
there but not used: the build passes `-UAMIGA`.

## Other Amiga systems

`kpathsea/amiga.h` is included for `__amigaos__`, `__amigaos4__` and
`__MORPHOS__`, and its contents are keyed by the reason (`__ixemul__`
today). For a new system:

- add its section to `amiga.h`/`amiga.c`; most of the ixemul work-arounds
  (math, newlib symbols, `va_arg`, IPv6) should not be needed with a
  current C library, the program directory and path conversion probably
  are;
- copy `amiga/build.sh` and change the compiler and flags;
- remove `--disable-...` flags to try more programs.

## Debugging

`SetEnv KPATHSEA_DEBUG_OUTPUT /Work/kdebug.txt` and
`SetEnv KPATHSEA_DEBUG -1` (or `kpsewhich -debug=-1 file`) write
kpathsea's search trace to a file. TeX's own `.log` is written in large
blocks, so it stays empty for a long time during a long run.
