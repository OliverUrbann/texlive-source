/* Minimal stand-in: ixemul.library has no wide character support.
   Only the types that libstdc++'s headers mention.  See amiga/build.sh.  */
#ifndef TL_AMIGA_WCHAR_H
#define TL_AMIGA_WCHAR_H
#include <stddef.h>
typedef int wint_t;
typedef struct { int __count; int __value; } mbstate_t;
#ifndef WEOF
#define WEOF ((wint_t) -1)
#endif
#endif
