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
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

/* ---- AmigaDOS path to Unix path ---------------------------------- */

char *
kpathsea_amiga_unix_path (const char *name)
{
  const char *colon;
  char *ret;
  size_t vol;

  if (!name || !*name || *name == '-' || *name == '&' || *name == '\\'
      || *name == '*' || *name == '/' || *name == ':')
    return NULL;
  colon = strchr (name, ':');
  if (!colon || memchr (name, '/', colon - name)
      || memchr (name, ' ', colon - name))
    return NULL;
  vol = colon - name;
  ret = (char *) xmalloc (strlen (name) + 3);
  ret[0] = '/';
  memcpy (ret + 1, name, vol);
  ret[vol + 1] = '\0';
  if (colon[1]) {
    strcat (ret, "/");
    strcat (ret, colon + 1);
  }
  return ret;
}

/* ---- localtime_r, gmtime_r ------------------------------------------ */

struct tm *
tl_amiga_localtime_r (const void *t, struct tm *r)
{
  struct tm *p = localtime ((const time_t *) t);
  if (!p)
    return NULL;
  *r = *p;
  return r;
}

struct tm *
tl_amiga_gmtime_r (const void *t, struct tm *r)
{
  struct tm *p = gmtime ((const time_t *) t);
  if (!p)
    return NULL;
  *r = *p;
  return r;
}

/* ---- nanosleep ------------------------------------------------------ */

int
nanosleep (const struct timespec *req, struct timespec *rem)
{
  if (req->tv_sec > 0)
    sleep (req->tv_sec);
  if (req->tv_nsec >= 1000)
    usleep (req->tv_nsec / 1000);
  if (rem) {
    rem->tv_sec = 0;
    rem->tv_nsec = 0;
  }
  return 0;
}

/* ---- getaddrinfo and friends ----------------------------------------

   ixemul.library declares them in <netdb.h> but does not have them.
   LuaTeX's socket library (luasocket) needs them.  IPv4 only, on top of
   gethostbyname and getservbyname, which the TCP stack provides.  */

static struct addrinfo *
amiga_ai_new (struct in_addr addr, int port, int socktype, int protocol)
{
  struct addrinfo *ai;
  struct sockaddr_in *sin;

  ai = calloc (1, sizeof (*ai) + sizeof (*sin));
  if (!ai)
    return NULL;
  sin = (struct sockaddr_in *) (ai + 1);
  sin->sin_len = sizeof (*sin);
  sin->sin_family = AF_INET;
  sin->sin_port = htons (port);
  sin->sin_addr = addr;
  ai->ai_family = AF_INET;
  ai->ai_socktype = socktype;
  ai->ai_protocol = protocol;
  ai->ai_addrlen = sizeof (*sin);
  ai->ai_addr = (struct sockaddr *) sin;
  return ai;
}

int
getaddrinfo (const char *node, const char *service,
             const struct addrinfo *hints, struct addrinfo **res)
{
  int flags = hints ? hints->ai_flags : 0;
  int family = hints ? hints->ai_family : AF_UNSPEC;
  int socktype = hints ? hints->ai_socktype : 0;
  int protocol = hints ? hints->ai_protocol : 0;
  struct in_addr one, *addrs[2], **list;
  struct hostent *hp = NULL;
  struct addrinfo *first = NULL, **next = &first;
  int port = 0;

  if (!res)
    return EAI_FAIL;
  *res = NULL;
  if (!node && !service)
    return EAI_NONAME;
  if (family != AF_UNSPEC && family != AF_INET)
    return EAI_FAMILY;
  if (socktype == 0)
    socktype = SOCK_STREAM;
  if (socktype != SOCK_STREAM && socktype != SOCK_DGRAM)
    return EAI_SOCKTYPE;

  if (service && *service) {
    char *end;
    long n = strtol (service, &end, 10);
    if (*end == '\0' && n >= 0 && n <= 65535)
      port = (int) n;
    else if (flags & AI_NUMERICSERV)
      return EAI_NONAME;
    else {
      struct servent *se = getservbyname (service,
                                          socktype == SOCK_DGRAM ? "udp" : "tcp");
      if (!se)
        return EAI_SERVICE;
      port = ntohs (se->s_port);
    }
  }

  addrs[1] = NULL;
  list = addrs;
  if (!node) {
    one.s_addr = htonl ((flags & AI_PASSIVE) ? INADDR_ANY : INADDR_LOOPBACK);
    addrs[0] = &one;
  } else if (inet_aton (node, &one)) {
    addrs[0] = &one;
  } else if (flags & AI_NUMERICHOST) {
    return EAI_NONAME;
  } else {
    hp = gethostbyname (node);
    if (!hp || hp->h_addrtype != AF_INET || !hp->h_addr_list[0])
      return EAI_NONAME;
    list = (struct in_addr **) hp->h_addr_list;
  }

  for (; *list; list++) {
    struct addrinfo *ai = amiga_ai_new (**list, port, socktype, protocol);
    if (!ai) {
      freeaddrinfo (first);
      return EAI_MEMORY;
    }
    *next = ai;
    next = &ai->ai_next;
  }
  if ((flags & AI_CANONNAME) && first) {
    first->ai_canonname = strdup (hp ? hp->h_name : node ? node : "");
    if (!first->ai_canonname) {
      freeaddrinfo (first);
      return EAI_MEMORY;
    }
  }
  *res = first;
  return 0;
}

void
freeaddrinfo (struct addrinfo *res)
{
  while (res) {
    struct addrinfo *next = res->ai_next;
    free (res->ai_canonname);
    free (res);
    res = next;
  }
}

int
getnameinfo (const struct sockaddr *sa, socklen_t salen, char *host,
             socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
  const struct sockaddr_in *sin = (const struct sockaddr_in *) sa;

  if (!sa || sa->sa_family != AF_INET || salen < (socklen_t) sizeof (*sin))
    return EAI_FAMILY;
  if (host && hostlen) {
    const char *name = NULL;
    if (!(flags & NI_NUMERICHOST)) {
      struct hostent *hp = gethostbyaddr ((const char *) &sin->sin_addr,
                                          sizeof (sin->sin_addr), AF_INET);
      if (hp)
        name = hp->h_name;
    }
    if (!name) {
      if (flags & NI_NAMEREQD)
        return EAI_NONAME;
      name = inet_ntoa (sin->sin_addr);
    }
    if (strlen (name) >= (size_t) hostlen)
      return EAI_OVERFLOW;
    strcpy (host, name);
  }
  if (serv && servlen) {
    char buf[16];
    const char *name = NULL;
    if (!(flags & NI_NUMERICSERV)) {
      struct servent *se = getservbyport (sin->sin_port,
                                          (flags & NI_DGRAM) ? "udp" : "tcp");
      if (se)
        name = se->s_name;
    }
    if (!name) {
      sprintf (buf, "%u", (unsigned) ntohs (sin->sin_port));
      name = buf;
    }
    if (strlen (name) >= (size_t) servlen)
      return EAI_OVERFLOW;
    strcpy (serv, name);
  }
  return 0;
}

char *
gai_strerror (int code)
{
  switch (code) {
  case EAI_AGAIN:    return (char *) "temporary failure in name resolution";
  case EAI_FAIL:     return (char *) "non-recoverable failure in name resolution";
  case EAI_FAMILY:   return (char *) "address family not supported";
  case EAI_MEMORY:   return (char *) "memory allocation failure";
  case EAI_NONAME:   return (char *) "host or service not known";
  case EAI_SERVICE:  return (char *) "service not supported for socket type";
  case EAI_SOCKTYPE: return (char *) "socket type not supported";
  case EAI_OVERFLOW: return (char *) "argument buffer overflow";
  default:           return (char *) "name resolution error";
  }
}

/* ---- Math ------------------------------------------------------------

   ixemul's libm.a is empty; the math lives in libc.a/ixemul.library and
   stops at C89.  xpdf (pdfTeX) needs these.  */

#undef round
#undef trunc
#undef log2
#undef lround
#undef llround
#undef fmin
#undef fmax

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

double
round (double x)
{
  return x < 0.0 ? ceil (x - 0.5) : floor (x + 0.5);
}

long
lround (double x)
{
  volatile double r = round (x);   /* see AMIGA_F1 about volatile */
  return (long) r;
}

long long
llround (double x)
{
  volatile double r = round (x);
  return (long long) r;
}

double
fmin (double x, double y)
{
  return x < y ? x : y;
}

double
fmax (double x, double y)
{
  return x > y ? x : y;
}

double
rint (double x)
{
  /* round half to even, as the default rounding mode does */
  double r = floor (x + 0.5);
  if (r - x == 0.5 && fmod (r, 2.0) != 0.0)
    r -= 1.0;
  return r;
}

/* The float variants as real functions too: gcc may emit calls to them
   (from __builtin_ceilf and the like) whatever the headers say.  */
#undef sinf
#undef cosf
#undef tanf
#undef asinf
#undef acosf
#undef atanf
#undef expf
#undef logf
#undef log10f
#undef sqrtf
#undef fabsf
#undef floorf
#undef ceilf
#undef roundf
#undef truncf
#undef powf
#undef atan2f
#undef fmodf
/* The volatile temporaries matter: gcc turns "(float) floor ((double) x)"
   back into a call of floorf, which would be this very function.  */
#define AMIGA_F1(n) float n##f (float x) \
  { volatile double d = x; d = n (d); return (float) d; }
#define AMIGA_F2(n) float n##f (float x, float y) \
  { volatile double d = x, e = y; d = n (d, e); return (float) d; }
double round (double);
double trunc (double);
double fmod (double, double);
double atan2 (double, double);
double hypot (double, double);
AMIGA_F1 (sin) AMIGA_F1 (cos) AMIGA_F1 (tan) AMIGA_F1 (asin) AMIGA_F1 (acos)
AMIGA_F1 (atan) AMIGA_F1 (exp) AMIGA_F1 (log) AMIGA_F1 (log10) AMIGA_F1 (sqrt)
AMIGA_F1 (fabs) AMIGA_F1 (floor) AMIGA_F1 (ceil) AMIGA_F1 (round)
AMIGA_F1 (trunc)
AMIGA_F2 (pow) AMIGA_F2 (atan2) AMIGA_F2 (fmod) AMIGA_F2 (hypot)

double
fmod (double x, double y)
{
  double q;
  if (y == 0.0)
    return x;
  modf (x / y, &q);
  return x - q * y;
}

double
log2 (double x)
{
  return log (x) / M_LN2;
}

double
hypot (double x, double y)
{
  return sqrt (x * x + y * y);
}

double complex
cexp (double complex z)
{
  /* gcc itself calls cexp when it sees sin and cos of one argument, so
     this function must not look like that: two volatile copies.  */
  volatile double ic = cimag (z), is = cimag (z);
  double r = exp (creal (z));
  double c = cos (ic);
  double s = sin (is);
  return __builtin_complex (r * c, r * s);
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
