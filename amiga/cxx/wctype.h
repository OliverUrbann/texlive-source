/* Minimal stand-in: ixemul.library has no wide character support.
   Only the types that libstdc++'s headers mention.  See amiga/build.sh.  */
#ifndef TL_AMIGA_WCTYPE_H
#define TL_AMIGA_WCTYPE_H
#include <wchar.h>
typedef int wctype_t;
typedef int wctrans_t;
#endif
