/* tlunpack: check and unpack one TeX Live package container.

   Part of the TeX Live installer for AmigaOS (amiga/tlinst).  The
   installer itself is a texlua script; this small program does the work
   that is too slow or needs too much memory in Lua on a 68k: the SHA-512
   check of the downloaded .tar.xz, the xz decompression and unpacking the
   tar archive.  Everything is streamed, so the memory needed does not
   depend on the size of the package (only the xz dictionary, at most a
   few MB for TeX Live's containers).

   usage: tlunpack [-c sha512] [-l listfile] [-d destdir] [-x prefix]
                   file.tar.xz
          tlunpack -s file          print the SHA-512 of a file

     -c sha512   expected SHA-512 (hex, as in texlive.tlpdb); unpack only
                 if it matches
     -l listfile write the names of the unpacked files to listfile, one per
                 line, relative to destdir (for uninstalling later)
     -d destdir  where to unpack (default: current directory); an Amiga
                 path like TeX: works
     -x prefix   skip the files whose names start with prefix (e.g. tlpkg/:
                 tlmgr's own files)
     -p          show the progress (a line every 1000 files), for big
                 archives

   Exit code: 0 ok, 5 (WARN) checksum mismatch, 10 (ERROR) other errors.
   Plain C with stdio; no Amiga-specific code apart from path joining, so
   the same source builds for Linux (to build the package on the host),
   AmigaOS 3 (ixemul), and later AmigaOS 4 and MorphOS.

   Uses XZ Embedded (amiga/tlinst/xz, public domain / 0BSD) and the
   SHA-512 code of pplib (libs/pplib).  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include "xz.h"
#include "util/utilsha.h"

#define RC_OK    0
#define RC_WARN  5
#define RC_ERROR 10

#define BUFSIZE  65536
#define DICT_MAX (64UL << 20)   /* largest xz dictionary we accept */

static const char *prog = "tlunpack";

static void
fail (const char *fmt, const char *arg)
{
  fprintf (stderr, "%s: ", prog);
  fprintf (stderr, fmt, arg);
  fputc ('\n', stderr);
  exit (RC_ERROR);
}

/* ---- SHA-512 -------------------------------------------------------- */

static void
sha512_file (const char *name, char hex[129])
{
  static unsigned char buf[BUFSIZE];
  sha512_state st;
  uint8_t dig[64];
  size_t n;
  int i;
  FILE *f = fopen (name, "rb");

  if (!f)
    fail ("cannot open %s", name);
  sha512_digest_init (&st);
  while ((n = fread (buf, 1, sizeof buf, f)) > 0)
    sha512_digest_add (&st, buf, n);
  if (ferror (f))
    fail ("read error on %s", name);
  fclose (f);
  sha512_digest_get (&st, dig, 0);
  for (i = 0; i < 64; i++)
    sprintf (hex + 2 * i, "%02x", dig[i]);
  hex[128] = '\0';
}

/* ---- xz: a pull reader --------------------------------------------- */

static FILE *xz_in;
static struct xz_dec *xz;
static struct xz_buf xb;
static unsigned char xz_inbuf[BUFSIZE];
static int xz_done;

/* read exactly n bytes of decompressed data; returns the number read
   (less than n only at the end of the stream) */
static size_t
xz_read (unsigned char *out, size_t n)
{
  size_t got = 0;

  while (got < n && !xz_done) {
    enum xz_ret r;
    if (xb.in_pos == xb.in_size) {
      xb.in_size = fread (xz_inbuf, 1, sizeof xz_inbuf, xz_in);
      xb.in_pos = 0;
      if (ferror (xz_in))
        fail ("read error on %s", "input");
    }
    xb.out = out + got;
    xb.out_pos = 0;
    xb.out_size = n - got;
    r = xz_dec_run (xz, &xb);
    got += xb.out_pos;
    if (r == XZ_STREAM_END)
      xz_done = 1;
    else if (r == XZ_OK) {
      if (xb.in_size == 0 && xb.out_pos == 0)
        fail ("%s: unexpected end of the xz data", "input");
    } else if (r == XZ_UNSUPPORTED_CHECK) {
      /* data is fine, the check cannot be verified: go on */
    } else {
      static char msg[40];
      sprintf (msg, "xz error %d", (int) r);
      fail ("%s (damaged or not an xz file)", msg);
    }
  }
  return got;
}

/* ---- tar ------------------------------------------------------------ */

static unsigned long
octal (const char *p, int len)
{
  unsigned long v = 0;
  while (len > 0 && (*p == ' ' || *p == '\0'))
    p++, len--;
  while (len > 0 && *p >= '0' && *p <= '7')
    v = v * 8 + (unsigned long) (*p++ - '0'), len--;
  return v;
}

static char destdir[1024] = "";
static const char *skip_prefix = NULL;
static int progress = 0;

/* destdir + "/" + rel; an Amiga device name ("TeX:") needs no slash */
static void
join (char *out, size_t size, const char *rel)
{
  size_t d = strlen (destdir);
  if (d == 0)
    snprintf (out, size, "%s", rel);
  else if (destdir[d - 1] == ':' || destdir[d - 1] == '/')
    snprintf (out, size, "%s%s", destdir, rel);
  else
    snprintf (out, size, "%s/%s", destdir, rel);
}

/* create all parent directories of path (not path itself) */
static void
mkparents (char *path)
{
  char *p;
  /* skip an Amiga device part ("TeX:") */
  char *start = strchr (path, ':');
  start = start ? start + 1 : path;
  for (p = start + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir (path, 0755) != 0 && errno != EEXIST) {
        struct stat st;
        if (stat (path, &st) != 0 || !S_ISDIR (st.st_mode))
          fail ("cannot create directory %s", path);
      }
      *p = '/';
    }
  }
}

static int
safe_name (const char *n)
{
  const char *p;
  if (!*n || n[0] == '/' || strchr (n, ':'))
    return 0;
  for (p = n; *p; ) {
    if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0'))
      return 0;
    p = strchr (p, '/');
    if (!p)
      break;
    p++;
  }
  return 1;
}

/* read the data of a pax extended header and return its "path" value */
static void
pax_path (unsigned long size, char *path, size_t pathsize)
{
  char *data = malloc (size + 1), *p;
  if (!data)
    fail ("%s", "out of memory");
  if (xz_read ((unsigned char *) data, size) != size)
    fail ("%s: truncated tar archive", "input");
  data[size] = '\0';
  /* records: "<len> key=value\n" */
  for (p = data; p < data + size; ) {
    char *sp = strchr (p, ' ');
    long len = strtol (p, NULL, 10);
    if (!sp || len <= 0)
      break;
    if (strncmp (sp + 1, "path=", 5) == 0) {
      char *v = sp + 6, *e = p + len - 1;  /* e: the newline */
      size_t l = (size_t) (e - v);
      if (l >= pathsize)
        fail ("%s: name too long", "input");
      memcpy (path, v, l);
      path[l] = '\0';
    }
    p += len;
  }
  free (data);
  /* skip the padding to the next 512-byte block */
  {
    unsigned char pad[512];
    size_t r = (512 - size % 512) % 512;
    if (r && xz_read (pad, r) != r)
      fail ("%s: truncated tar archive", "input");
  }
}

static int
untar (FILE *list)
{
  static unsigned char hdr[512];
  static unsigned char buf[BUFSIZE];
  char longname[1024] = "", name[1024], path[2048];
  int files = 0;

  for (;;) {
    unsigned long size, left;
    char type;

    if (xz_read (hdr, 512) != 512)
      break;                            /* end of data */
    if (hdr[0] == '\0')
      break;                            /* end-of-archive block */
    size = octal ((char *) hdr + 124, 12);
    type = (char) hdr[156];

    if (type == 'x' || type == 'L') {   /* pax header / GNU long name */
      if (type == 'x')
        pax_path (size, longname, sizeof longname);
      else {
        if (size >= sizeof longname)
          fail ("%s: name too long", "input");
        if (xz_read ((unsigned char *) longname, size) != size)
          fail ("%s: truncated tar archive", "input");
        longname[size] = '\0';
        {
          unsigned char pad[512];
          size_t r = (512 - size % 512) % 512;
          if (r && xz_read (pad, r) != r)
            fail ("%s: truncated tar archive", "input");
        }
      }
      continue;
    }
    if (type == 'g') {                  /* global pax header: skip */
      left = (size + 511) / 512 * 512;
      while (left > 0) {
        size_t n = left > sizeof buf ? sizeof buf : left;
        if (xz_read (buf, n) != n)
          fail ("%s: truncated tar archive", "input");
        left -= n;
      }
      continue;
    }

    if (longname[0]) {
      snprintf (name, sizeof name, "%s", longname);
      longname[0] = '\0';
    } else {
      char n1[101], pre[156];
      memcpy (n1, hdr, 100); n1[100] = '\0';
      memcpy (pre, hdr + 345, 155); pre[155] = '\0';
      if (memcmp (hdr + 257, "ustar", 5) == 0 && pre[0])
        snprintf (name, sizeof name, "%s/%s", pre, n1);
      else
        snprintf (name, sizeof name, "%s", n1);
    }
    while (strncmp (name, "./", 2) == 0)
      memmove (name, name + 2, strlen (name) - 1);

    if (skip_prefix && strncmp (name, skip_prefix, strlen (skip_prefix)) == 0) {
      if (type == '0' || type == '\0' || type == '7') {
        left = (size + 511) / 512 * 512;
        while (left > 0) {
          size_t n = left > sizeof buf ? sizeof buf : left;
          if (xz_read (buf, n) != n)
            fail ("%s: truncated tar archive", "input");
          left -= n;
        }
      }
      continue;
    }
    if (!safe_name (name))
      fail ("refusing to unpack %s", name);
    join (path, sizeof path, name);

    if (type == '5') {                  /* directory */
      size_t l = strlen (path);
      if (l && path[l - 1] == '/')
        path[l - 1] = '\0';
      strcat (path, "/x");
      mkparents (path);
      continue;
    }
    if (type == '2' || type == '1') {
      /* links: TeX Live containers hardly have any; AmigaOS 3 has no
         usable ones.  Copying the target would be possible, but the
         target may come later in the archive: report and go on. */
      fprintf (stderr, "%s: skipping link %s\n", prog, name);
      continue;
    }
    if (type != '0' && type != '\0' && type != '7') {
      fprintf (stderr, "%s: skipping %s (type %c)\n", prog, name, type);
      left = (size + 511) / 512 * 512;
      while (left > 0) {
        size_t n = left > sizeof buf ? sizeof buf : left;
        if (xz_read (buf, n) != n)
          fail ("%s: truncated tar archive", "input");
        left -= n;
      }
      continue;
    }

    /* a regular file */
    {
      FILE *out;
      unsigned long mtime = octal ((char *) hdr + 136, 12);
      mkparents (path);
      remove (path);              /* also clears a read-only old copy */
      out = fopen (path, "wb");
      if (!out)
        fail ("cannot write %s", path);
      left = size;
      while (left > 0) {
        size_t n = left > sizeof buf ? sizeof buf : left;
        if (xz_read (buf, n) != n)
          fail ("%s: truncated tar archive", "input");
        if (fwrite (buf, 1, n, out) != n)
          fail ("write error on %s (disk full?)", path);
        left -= n;
      }
      if (fclose (out) != 0)
        fail ("write error on %s (disk full?)", path);
      if (mtime) {
        struct utimbuf ut;
        ut.actime = ut.modtime = (time_t) mtime;
        utime (path, &ut);
      }
      /* padding */
      if (size % 512) {
        size_t r = 512 - size % 512;
        if (xz_read (buf, r) != r)
          fail ("%s: truncated tar archive", "input");
      }
      if (list)
        fprintf (list, "%s\n", name);
      files++;
      if (progress && files % 1000 == 0) {
        printf ("  %d files\n", files);
        fflush (stdout);
      }
    }
  }
  return files;
}

/* ---- main ----------------------------------------------------------- */

static void
usage (void)
{
  fprintf (stderr,
           "usage: %s [-c sha512] [-l listfile] [-d destdir] [-x prefix] file.tar.xz\n"
           "       %s -s file\n", prog, prog);
  exit (RC_ERROR);
}

int
main (int argc, char **argv)
{
  const char *check = NULL, *listname = NULL, *file = NULL;
  FILE *list = NULL;
  int i, n;

  for (i = 1; i < argc; i++) {
    if (strcmp (argv[i], "-s") == 0 && i + 1 < argc) {
      char hex[129];
      sha512_file (argv[i + 1], hex);
      printf ("%s\n", hex);
      return RC_OK;
    } else if (strcmp (argv[i], "-c") == 0 && i + 1 < argc)
      check = argv[++i];
    else if (strcmp (argv[i], "-l") == 0 && i + 1 < argc)
      listname = argv[++i];
    else if (strcmp (argv[i], "-d") == 0 && i + 1 < argc)
      snprintf (destdir, sizeof destdir, "%s", argv[++i]);
    else if (strcmp (argv[i], "-p") == 0)
      progress = 1;
    else if (strcmp (argv[i], "-x") == 0 && i + 1 < argc)
      skip_prefix = argv[++i];
    else if (argv[i][0] == '-' || file)
      usage ();
    else
      file = argv[i];
  }
  if (!file)
    usage ();

  if (check) {
    char hex[129];
    sha512_file (file, hex);
    if (strcmp (hex, check) != 0) {
      fprintf (stderr, "%s: checksum mismatch for %s\n", prog, file);
      return RC_WARN;
    }
  }

  xz_in = fopen (file, "rb");
  if (!xz_in)
    fail ("cannot open %s", file);
  xz_crc32_init ();
#ifdef XZ_USE_CRC64
  xz_crc64_init ();
#endif
  xz = xz_dec_init (XZ_DYNALLOC, DICT_MAX);
  if (!xz)
    fail ("%s", "out of memory");
  xb.in = xz_inbuf;
  xb.in_pos = xb.in_size = 0;

  if (listname) {
    list = fopen (listname, "w");
    if (!list)
      fail ("cannot write %s", listname);
  }
  n = untar (list);
  if (list && fclose (list) != 0)
    fail ("write error on %s", listname);
  xz_dec_end (xz);
  fclose (xz_in);
  printf ("%d files\n", n);
  return RC_OK;
}
