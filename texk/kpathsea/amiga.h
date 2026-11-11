/* amiga.h: compatibility definitions for Amiga-like systems.

   Copyright 2026 Oliver Urbann.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   Included from <kpathsea/config.h>.  Sections are keyed by the reason,
   not by the machine, so that AmigaOS 4 (newlib/clib4) and MorphOS can
   later pick only what they need:

     __ixemul__     AmigaOS 3.x with ixemul.library (the current port):
                    a Unix view of the file system (/Work/dir), but an
                    old BSD 4.3-style C library.
     __amigaos4__   AmigaOS 4, not yet ported.
     __MORPHOS__    MorphOS, not yet ported.  */

#ifndef KPATHSEA_AMIGA_H
#define KPATHSEA_AMIGA_H

#if defined (__ixemul__)
/* No large-file API in ixemul; off_t is 32 bits anyway.  */
#ifndef fseeko
#define fseeko fseek
#endif
#ifndef ftello
#define ftello ftell
#endif
#endif /* __ixemul__ */

#endif /* not KPATHSEA_AMIGA_H */
