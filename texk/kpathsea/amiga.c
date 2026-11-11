/* amiga.c: support code for Amiga-like systems.

   Copyright 2026 Oliver Urbann.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   Everything here is for AmigaOS 3 with ixemul.library (__ixemul__).
   On other systems this file compiles to nothing.  AmigaOS 4 and
   MorphOS are not ported yet; see amiga.h for how they would fit in.

   This file is a member of libkpathsea.a.  Every kpathsea program calls
   kpathsea_amiga_progdir (from kpathsea_selfdir), so the linker always
   takes this whole object, and with it the definitions below that are
   not referenced by name until late in the link (__stack, which ixemul's
   startup code reads, and the newlib symbols libstdc++.a needs).  */

#include <kpathsea/config.h>

#if defined (__ixemul__)

#include <errno.h>
#include <math.h>
#include <complex.h>
#include <signal.h>

/* The NDK's devices/timer.h defines struct timeval again; ixemul's
   <sys/time.h> already did.  Rename the NDK one, we do not use it.  */
#define timeval amiga_ndk_timeval
#include <exec/execbase.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#undef timeval

/* ---- Stack ---------------------------------------------------------

   ixemul's default stack is 4 KB; TeX's expansion and pdfTeX's PDF
   parser recurse deeply.  ixemul's startup code reads __stack.  */
unsigned long __stack = 512 * 1024;

/* ---- Program directory ---------------------------------------------

   argv[0] is whatever the user typed in the Shell: "SYS:TeX/bin/tex",
   or just "tex" when found via the Shell's Path (there is no PATH
   variable).  Neither works with kpathsea's Unix logic, so ask AmigaDOS
   for the program's directory and turn "Volume:dir/sub" into ixemul's
   Unix form "/Volume/dir/sub".  */

char *
kpathsea_amiga_progdir (void)
{
  struct ExecBase *SysBase = *(struct ExecBase **) 4L;
  struct DosLibrary *DOSBase;
  char buf[1024];
  char *ret = NULL;
  sigset_t all, old;

  /* No signals (Ctrl-C) in the middle of a DOS packet; learned in the
     amiga_rsync port (Alert 87000004 on exit otherwise).  */
  sigfillset (&all);
  sigprocmask (SIG_BLOCK, &all, &old);
  DOSBase = (struct DosLibrary *) OpenLibrary ((CONST_STRPTR) "dos.library", 36);
  if (DOSBase) {
    BPTR lock = GetProgramDir ();
    if (lock && NameFromLock (lock, (STRPTR) buf, sizeof (buf))) {
      char *colon = strchr (buf, ':');
      if (colon) {
        size_t vol = colon - buf;
        const char *rest = colon + 1;
        ret = (char *) xmalloc (strlen (buf) + 3);
        ret[0] = '/';
        memcpy (ret + 1, buf, vol);
        ret[vol + 1] = '\0';
        if (*rest) {
          strcat (ret, "/");
          strcat (ret, rest);
        }
      }
    }
    CloseLibrary ((struct Library *) DOSBase);
  }
  sigprocmask (SIG_SETMASK, &old, NULL);
  return ret;
}

/* ---- Math ------------------------------------------------------------

   ixemul's libm.a is empty; the math lives in libc.a/ixemul.library and
   stops at C89.  xpdf (pdfTeX) needs these.  */

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

/* ---- newlib symbols for libstdc++ -------------------------------------

   The toolchain's libstdc++ was built against newlib/libnix headers, so
   it references _impure_ptr (for stdin, stdout, stderr), __errno and
   __xpg_strerror_r.  They are only used on error paths (std::terminate,
   assertions, std::system_error).  */

/* First fields of newlib's struct _reent; libstdc++ only reads
   _stdin/_stdout/_stderr (offsets 4, 8, 12).  */
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
