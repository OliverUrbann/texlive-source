/* ixemul-compat.c: fill gaps in ixemul.library for TeX Live on AmigaOS 3.

   Copyright 2026 Oliver Urbann.
   You may freely use, modify and/or distribute this file.

   This file is linked as a plain object into every program (see
   amiga/build.sh, LIBS=...), not as an archive member, because some of
   the symbols are needed by libstdc++.a, which g++ puts at the very end
   of the link line.

   Only for AmigaOS 3 with ixemul.library (__ixemul__).  AmigaOS 4 and
   MorphOS have complete C libraries; they would not use this file.

   0. A larger default stack.
   1. Math functions that ixemul.library does not export (its libm.a is
      empty, the math lives in libc.a/ixemul.library and stops at C89).
   2. newlib symbols: the toolchain's libstdc++ was built against
      newlib/libnix headers, so it references _impure_ptr (for stdin,
      stdout, stderr), __errno and __xpg_strerror_r.  They are only used
      on error paths (std::terminate, assertions, std::system_error).  */

#if defined (__ixemul__)

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <complex.h>

/* ---- 0. stack ---------------------------------------------------- */

/* ixemul's default stack is 4 KB; TeX's expansion and pdfTeX's PDF
   parser recurse deeply.  ixemul's startup code reads __stack.  */
unsigned long __stack = 512 * 1024;

/* ---- 1. math ------------------------------------------------------ */

double
trunc (double x)
{
  double ip;
  modf (x, &ip);
  return ip;
}

double
atan2 (double y, double x)
{
  if (x > 0.0)
    return atan (y / x);
  if (x < 0.0)
    return y >= 0.0 ? atan (y / x) + M_PI : atan (y / x) - M_PI;
  /* x == 0 */
  if (y > 0.0)
    return M_PI_2;
  if (y < 0.0)
    return -M_PI_2;
  return 0.0;
}

double complex
cexp (double complex z)
{
  double r = exp (creal (z));
  double i = cimag (z);
  return __builtin_complex (r * cos (i), r * sin (i));
}

/* ---- 2. newlib symbols for libstdc++ ------------------------------ */

/* First fields of newlib's struct _reent; libstdc++ only reads
   _stdin/_stdout/_stderr (offsets 4, 8, 12).  The rest is padding in
   case a function looks further.  */
struct ixemul_compat_reent
{
  int _errno;
  FILE *_stdin, *_stdout, *_stderr;
  char _pad[256];
};

static struct ixemul_compat_reent compat_reent;
struct ixemul_compat_reent *_impure_ptr = &compat_reent;

static void __attribute__ ((constructor))
ixemul_compat_init (void)
{
  compat_reent._stdin = stdin;
  compat_reent._stdout = stdout;
  compat_reent._stderr = stderr;
}

int *
__errno (void)
{
  return &errno;
}

int
__xpg_strerror_r (int errnum, char *buf, size_t len)
{
  const char *s = strerror (errnum);
  if (len == 0)
    return ERANGE;
  strncpy (buf, s ? s : "Unknown error", len - 1);
  buf[len - 1] = '\0';
  return 0;
}

#endif /* __ixemul__ */
