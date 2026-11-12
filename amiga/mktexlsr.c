/* mktexlsr.c: create ls-R file name databases, for systems without a
   Unix shell (TeX Live's mktexlsr is a shell script).

   Copyright 2026 Oliver Urbann.
   You may freely use, modify and/or distribute this file.

   usage: mktexlsr [--if-changed] [DIR...]
   Without DIR, the directories of kpathsea's TEXMFDBS are used, as with
   the original.  Directories that do not exist are skipped.
   --if-changed (or -c): leave an ls-R alone if it is already up to
   date, and say nothing then.  The exit status is 5 (WARN on AmigaOS)
   if an ls-R was written, 0 if all were up to date.  The TeX-Shell runs
   "mktexlsr --if-changed TeX:texmf-local" at every start, so that files
   unpacked there are found without further ado.

   Plain C and kpathsea only; nothing here is specific to AmigaOS.  */

#include <kpathsea/config.h>
#include <kpathsea/c-dir.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/expand.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/progname.h>
#include <kpathsea/variable.h>

static int if_changed = 0;
static int updated = 0;   /* an ls-R was written */

static const char magic[] =
  "% ls-R -- filename database for kpathsea; do not change this line.";

struct names { char **v; unsigned n, max; };

static void
add (struct names *l, const char *s)
{
  if (l->n == l->max) {
    l->max = l->max ? 2 * l->max : 64;
    l->v = (char **) xrealloc (l->v, l->max * sizeof (char *));
  }
  l->v[l->n++] = xstrdup (s);
}

static int
cmp (const void *a, const void *b)
{
  return strcmp (*(char *const *) a, *(char *const *) b);
}

/* Nonzero if the files A and B have the same contents.  */
static int
same_file (const char *a, const char *b)
{
  FILE *fa = fopen (a, "rb"), *fb = fopen (b, "rb");
  int same = fa && fb;
  while (same) {
    int ca = getc (fa), cb = getc (fb);
    if (ca != cb)
      same = 0;
    else if (ca == EOF)
      break;
  }
  if (fa)
    fclose (fa);
  if (fb)
    fclose (fb);
  return same;
}

/* Write the listing of ROOT/REL (REL is "." or "./a/b") and recurse.  */
static void
list_dir (FILE *out, const char *root, const char *rel, unsigned long *count)
{
  struct names files = { NULL, 0, 0 }, dirs = { NULL, 0, 0 };
  char *path = concat3 (root, "/", rel);
  DIR *d = opendir (path);
  struct dirent *e;
  unsigned i;

  if (!d) {
    free (path);
    return;
  }
  while ((e = readdir (d)) != NULL) {
    char *full;
    struct stat st;
    if (strcmp (e->d_name, ".") == 0 || strcmp (e->d_name, "..") == 0)
      continue;
    /* like the original: skip version control directories */
    if (strcmp (e->d_name, ".git") == 0 || strcmp (e->d_name, ".svn") == 0)
      continue;
    /* our own files: the temporary one is left out, ls-R itself is
       added below whether it exists yet or not, so that the listing
       does not change between the first and the second run */
    if (strcmp (rel, ".") == 0 && (strcmp (e->d_name, "ls-R.tmp") == 0
                                   || strcmp (e->d_name, "ls-R") == 0))
      continue;
    full = concat3 (path, "/", e->d_name);
    add (&files, e->d_name);
    if (stat (full, &st) == 0 && S_ISDIR (st.st_mode))
      add (&dirs, e->d_name);
    free (full);
  }
  closedir (d);
  free (path);
  if (strcmp (rel, ".") == 0)
    add (&files, "ls-R");

  qsort (files.v, files.n, sizeof (char *), cmp);
  qsort (dirs.v, dirs.n, sizeof (char *), cmp);

  fprintf (out, "\n%s:\n", rel);
  for (i = 0; i < files.n; i++) {
    fprintf (out, "%s\n", files.v[i]);
    free (files.v[i]);
  }
  *count += files.n;
  free (files.v);

  for (i = 0; i < dirs.n; i++) {
    char *sub = concat3 (rel, "/", dirs.v[i]);
    list_dir (out, root, sub, count);
    free (sub);
    free (dirs.v[i]);
  }
  free (dirs.v);
}

static int
do_tree (const char *root)
{
  struct stat st;
  char *name, *tmp;
  FILE *out;
  unsigned long count = 0;

  if (stat (root, &st) != 0 || !S_ISDIR (st.st_mode))
    return 0;                   /* not there: fine, as in the original */
  name = concat (root, "/ls-R");
  tmp = concat (root, "/ls-R.tmp");
  out = fopen (tmp, "w");
  if (!out) {
    perror (tmp);
    return 1;
  }
  if (!if_changed) {
    printf ("mktexlsr: Updating %s...\n", name);
    fflush (stdout);
  }
  fputs (magic, out);
  fputs ("\n", out);
  list_dir (out, root, ".", &count);
  if (fclose (out) != 0) {
    perror (tmp);
    return 1;
  }
  if (if_changed && same_file (tmp, name)) {
    remove (tmp);
    free (name);
    free (tmp);
    return 0;
  }
  remove (name);
  if (rename (tmp, name) != 0) {
    perror (name);
    return 1;
  }
  updated = 1;
  if (if_changed)
    printf ("mktexlsr: updated %s (%lu entries).\n", name, count);
  else
    printf ("mktexlsr: %lu entries.\n", count);
  free (name);
  free (tmp);
  return 0;
}

int
main (int argc, char **argv)
{
  int status = 0, i, first = 1;

  kpse_set_program_name (argv[0], "mktexlsr");

  if (argc > 1 && (strcmp (argv[1], "--if-changed") == 0
                   || strcmp (argv[1], "-c") == 0)) {
    if_changed = 1;
    first = 2;
  }

  if (argc > first) {
    for (i = first; i < argc; i++) {
      char *dir = argv[i];
#if defined (__ixemul__)
      char *u = kpathsea_amiga_unix_path (dir);   /* Work:TeX/texmf-dist */
      if (u)
        dir = u;
#endif
      status |= do_tree (dir);
    }
  } else {
    char *dbs = kpse_var_value ("TEXMFDBS");
    char *expanded, *elt;
    if (!dbs) {
      fprintf (stderr, "mktexlsr: TEXMFDBS is not set in texmf.cnf.\n");
      return 1;
    }
    expanded = kpse_brace_expand (dbs);
    for (elt = strtok (expanded, ENV_SEP_STRING); elt;
         elt = strtok (NULL, ENV_SEP_STRING)) {
      char *dir = xstrdup (elt);
      size_t len;
      char *p = dir;
      while (*p == '!')
        p++;
      len = strlen (p);
      while (len > 1 && p[len - 1] == '/')
        p[--len] = '\0';
      if (*p)
        status |= do_tree (p);
      free (dir);
    }
  }
  if (status == 0 && !if_changed)
    printf ("mktexlsr: Done.\n");
  if (status == 0 && if_changed && updated)
    return 5;
  return status;
}
