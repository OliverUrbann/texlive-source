/* ixemul-fixes.h: force-included (gcc -include) into every compilation
   for AmigaOS 3 with ixemul.library.  See amiga/README.amiga.md.

   ixemul's headers date from BSD 4.4 (1991-1996).  Some definitions
   there do not work with a modern optimizing GCC; they are replaced here
   before any source file sees them.  AmigaOS 4 and MorphOS do not use
   this file.  */

#ifndef TL_AMIGA_IXEMUL_FIXES_H
#define TL_AMIGA_IXEMUL_FIXES_H

#if defined (__ixemul__)

/* This header must not include system headers: configure's link tests
   declare the functions they probe as "char f ();", which clashes with
   a real prototype and makes existing functions look missing.  So:
   macros, gcc builtins and forward declarations only.  */

/* <machine/stdarg.h> implements va_start as "address of the last named
   argument plus its size".  GCC may keep arguments in registers or copy
   them, so va_arg then reads garbage (seen in kpathsea_set_suffixes:
   corrupted suffix lists).  Define the include guards of ixemul's
   <stdarg.h> and <machine/stdarg.h> and provide gcc's own version.  */
#define _STDARG_H
#define _M68K_STDARG_H_
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start (ap, last)
#define va_arg(ap, type)   __builtin_va_arg (ap, type)
#define va_end(ap)         __builtin_va_end (ap)
#define va_copy(d, s)      __builtin_va_copy (d, s)
#define __va_copy(d, s)    __builtin_va_copy (d, s)

/* Functions that ixemul.library does not have and that several
   libraries (kpathsea, Lua, ...) use without asking configure.  off_t is
   32 bits, and nothing here is multi-threaded.  */
#define fseeko fseek
#define ftello ftell
#define flockfile(f)     ((void) 0)
#define funlockfile(f)   ((void) 0)
#define getc_unlocked(f) getc (f)
#define putc_unlocked(c, f) putc (c, f)

/* Implemented in texk/kpathsea/amiga.c.  The time_t pointers are
   declared as "const void *" because time_t is not known here.
   amiga/build.sh tells configure that nanosleep exists (LuaTeX's
   configure requires it).  */
#ifdef __cplusplus
extern "C" {
#endif
struct tm;
struct timespec;
struct tm *tl_amiga_localtime_r (const void *t, struct tm *r);
struct tm *tl_amiga_gmtime_r (const void *t, struct tm *r);
int nanosleep (const struct timespec *req, struct timespec *rem);
#ifdef __cplusplus
}
#endif
#define localtime_r tl_amiga_localtime_r
#define gmtime_r tl_amiga_gmtime_r

/* ixemul's <math.h> is C89: no float variants, no round, trunc, log2.
   Map them to gcc's builtins, which call the double functions; those
   that ixemul.library lacks (atan2, fmod, round, trunc, log2, hypot)
   are in texk/kpathsea/amiga.c.  Macros and builtins, not prototypes:
   <math.h> must not be included here, configure's link tests declare
   the functions they probe in their own way.  */
#define round(x)    __builtin_round (x)
#define trunc(x)    __builtin_trunc (x)
#define log2(x)     __builtin_log2 (x)
#define isfinite(x) __builtin_isfinite (x)
#define lround(x)   __builtin_lround (x)
#define llround(x)  __builtin_llround (x)
#define fmin(x, y)  __builtin_fmin (x, y)
#define fmax(x, y)  __builtin_fmax (x, y)
#define sinf(x)     ((float) __builtin_sin (x))
#define cosf(x)     ((float) __builtin_cos (x))
#define tanf(x)     ((float) __builtin_tan (x))
#define asinf(x)    ((float) __builtin_asin (x))
#define acosf(x)    ((float) __builtin_acos (x))
#define atanf(x)    ((float) __builtin_atan (x))
#define expf(x)     ((float) __builtin_exp (x))
#define logf(x)     ((float) __builtin_log (x))
#define log10f(x)   ((float) __builtin_log10 (x))
#define sqrtf(x)    ((float) __builtin_sqrt (x))
#define fabsf(x)    ((float) __builtin_fabs (x))
#define floorf(x)   ((float) __builtin_floor (x))
#define ceilf(x)    ((float) __builtin_ceil (x))
#define roundf(x)   ((float) __builtin_round (x))
#define truncf(x)   ((float) __builtin_trunc (x))
#define powf(x, y)  ((float) __builtin_pow (x, y))
#define atan2f(x, y) ((float) __builtin_atan2 (x, y))
#define fmodf(x, y) ((float) __builtin_fmod (x, y))

#endif /* __ixemul__ */

#endif /* not TL_AMIGA_IXEMUL_FIXES_H */
