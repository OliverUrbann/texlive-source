# TeX Live on AmigaOS 3

A port of TeX Live's `tex`, `pdftex` and `kpsewhich` to classic AmigaOS
3.x: 68020 or better, `ixemul.library`. AmigaOS 4 and MorphOS are planned
but not started; the code is laid out so they can be added (see
"Other Amiga systems").

## Status

Tested in FS-UAE (A1200, 68030/68882, 64 MB, Kickstart 3.1, ixemul 80.1).
Not yet tested on real hardware.

| What | Result |
|---|---|
| `kpsewhich` finds files, with and without `ls-R` | works |
| `tex -ini` builds the plain format | works |
| `tex` typesets a plain TeX file | works, DVI identical to Linux |
| `pdftex -ini` builds the pdfTeX and pdfLaTeX formats | works |
| `pdftex` with the LaTeX format typesets an article | works, text identical to Linux, Type 1 fonts embedded |
| `pdftex` includes PNG and PDF images | works |
| JPEG and JBIG2 images, `\write18`, `\pdffilemoddate` etc. | not tested |
| Paths in Amiga form on the command line (`Work:doc/a.tex`) | not supported, use `/Work/doc/a.tex` or `CD` there first |
| `mktexfmt`, `mktexpk`, `mktextfm` (automatic generation) | switched off, see below |

Everything else in TeX Live is switched off in `amiga/build.sh` with
`--disable-...` flags: e-TeX/pTeX variants, Metafont, MetaPost, the web
utilities, dvips, bibtex and the other programs. That includes the Unicode
engines XeTeX and LuaTeX, which need ICU and HarfBuzz. pdfTeX itself is an
8-bit engine; UTF-8 input goes through LaTeX's own `inputenc` code.

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

    TeX/bin/m68k-amigaos/tex, pdftex, kpsewhich
    TeX/texmf.cnf                     (from amiga/texmf.cnf)
    TeX/texmf-dist/web2c/texmf.cnf    (TeX Live's own)
    TeX/texmf-dist/...                (from a TeX Live installation)
    TeX/texmf-dist/ls-R

The programs find the tree from their own location, so it can be anywhere.
`texmf-dist` is not part of this repository; take it from any TeX Live
installation of a similar year. Create `ls-R` on Linux with `mktexlsr`, or
`ls -R` with the usual first line. Without `ls-R` searching works but is
slow.

ixemul needs `Assign AmiTCP:` to exist, otherwise each program asks for
the volume.

Then:

    Path Work:TeX/bin/m68k-amigaos ADD
    CD Work:mydoc
    pdftex -ini -jobname=pdflatex -progname=pdflatex -translate-file=cp227.tcx -etex pdflatex.ini
    pdftex -fmt=pdflatex -progname=pdflatex hello.tex

Formats are found in the current directory or in `texmf-var/web2c/pdftex/`.
Building the LaTeX format took about 20 minutes in the emulator. A format
built on Linux by the same pdfTeX version with the same `texmf.cnf`
values should also load (format files are byte-order independent); this
has not been tested.

## Memory

TeX Live's default array sizes allocate well over 100 MB. `amiga/texmf.cnf`
reduces them to about 32 MB. The largest item is `main_memory`: the LaTeX
kernel's Unicode tables (`expl3`, "codepoint" data) need about 2 million
words while the format is built. Plain TeX needs far less; lower the
values there if RAM is short.

The stack is set to 512 KB in the programs themselves (`__stack` in
`kpathsea/amiga.c`).

## What was changed

All changes are inside `#if defined (__ixemul__)`, nothing was removed.

- `texk/kpathsea/amiga.h`, `amiga.c` (new): `fseeko`/`ftello`; the stack
  size; the program's directory from AmigaDOS (`GetProgramDir`), because
  `argv[0]` and a `PATH` variable do not help on the Amiga; `atan2`,
  `trunc`, `cexp`, missing in ixemul; three newlib symbols that the
  toolchain's libstdc++ expects.
- `texk/kpathsea/line.c`: no `flockfile`/`getc_unlocked`.
- `texk/kpathsea/progname.c`: uses the program directory;
  `KPATHSEA_DEBUG_OUTPUT` works as on Windows (the 3.1 Shell cannot
  redirect stderr).
- `texk/kpathsea/tex-make.c`: does not try to run the `mktex...` shell
  scripts (no `fork()`, no Unix shell).
- `libs/xpdf/.../GlobalParams.cc`: no `localtime_r`.
- `amiga/ixemul-fixes.h` (new), included into every file by the compiler:
  ixemul's `va_start`/`va_arg` macros read wrong values with a current
  gcc. This showed as kpathsea not finding any file.

kpathsea's old `#ifdef AMIGA` code (SAS/C, native Amiga paths) is still
there but not used: the build passes `-UAMIGA`.

## Other Amiga systems

`kpathsea/amiga.h` is included for `__amigaos__`, `__amigaos4__` and
`__MORPHOS__`, and its contents are keyed by the reason (`__ixemul__`
today). For a new system:

- add its section to `amiga.h`/`amiga.c`; most of the ixemul work-arounds
  (math, newlib symbols, `va_arg`) should not be needed with a current C
  library, the program directory lookup probably is;
- copy `amiga/build.sh` and change the compiler and flags;
- remove `--disable-...` flags to try more programs. On MorphOS, with
  Unicode support in the system, XeTeX or LuaTeX could be candidates.

## Debugging

`SetEnv KPATHSEA_DEBUG_OUTPUT /Work/kdebug.txt` and `SetEnv KPATHSEA_DEBUG -1`
(or `kpsewhich -debug=-1 file`) write kpathsea's search trace to a file.
TeX's own `.log` is written in large blocks, so it stays empty for a long
time during a long run.
