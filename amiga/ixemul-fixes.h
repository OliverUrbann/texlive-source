/* ixemul-fixes.h: force-included (gcc -include) into every compilation
   for AmigaOS 3 with ixemul.library.  See amiga/README.amiga.md.

   ixemul's headers date from BSD 4.4 (1991-1996).  Some definitions
   there do not work with a modern optimizing GCC; they are replaced here
   before any source file sees them.  AmigaOS 4 and MorphOS do not use
   this file.  */

#ifndef TL_AMIGA_IXEMUL_FIXES_H
#define TL_AMIGA_IXEMUL_FIXES_H

#if defined (__ixemul__)

/* <machine/stdarg.h> implements va_start as "address of the last named
   argument plus its size".  GCC may keep arguments in registers or copy
   them, so va_arg then reads garbage (seen in kpathsea_set_suffixes:
   corrupted suffix lists).  va_list itself is already __builtin_va_list
   (machine/ansi.h), so only the macros change.  The include guard keeps
   ixemul's version from coming back.  */
#include <stdarg.h>
#undef va_start
#undef va_arg
#undef va_end
#undef va_copy
#undef __va_copy
#define va_start(ap, last) __builtin_va_start (ap, last)
#define va_arg(ap, type)   __builtin_va_arg (ap, type)
#define va_end(ap)         __builtin_va_end (ap)
#define va_copy(d, s)      __builtin_va_copy (d, s)
#define __va_copy(d, s)    __builtin_va_copy (d, s)

#endif /* __ixemul__ */

#endif /* not TL_AMIGA_IXEMUL_FIXES_H */
