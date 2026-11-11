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
/* amiga.c: the program's directory in Unix form ("/Work/TeX/bin"),
   from AmigaDOS, or NULL.  Used by kpathsea_selfdir.  */
extern char *kpathsea_amiga_progdir (void);

/* amiga.c: if NAME is an AmigaDOS path with a volume ("Work:doc/a.tex",
   "RAM:a.tex"), return a new string with the Unix form ("/Work/doc/a.tex").
   Return NULL if NAME is not such a path: options, TeX commands
   ("\\input a", "&fmt") and names without a colon in the first
   component.  */
extern char *kpathsea_amiga_unix_path (const char *name);
#endif /* __ixemul__ */

#endif /* not KPATHSEA_AMIGA_H */
