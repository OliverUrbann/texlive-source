/* mathtest.c: prints values of the math functions that kpathsea/amiga.c
   adds for ixemul.  Build it natively and for the Amiga, run both,
   compare the output:
     gcc -o mathtest mathtest.c -lm
     m68k-amigaos-gcc -mcrt=ixemul -m68020 -include ../ixemul-fixes.h \
        -o mathtest mathtest.c <build>/cross/texk/kpathsea/.libs/libkpathsea.a  */
#include <stdio.h>
#include <math.h>
#include <complex.h>

static volatile double v[] = { 0.0, 0.3, 0.5, 1.0, 1.5, 2.5, 3.7, -0.3, -0.5, -1.5, -2.5, -3.7, 1e6 + 0.5, 123456.789 };
#define N (sizeof v / sizeof v[0])

int
main (void)
{
  unsigned i, j;
  for (i = 0; i < N; i++) {
    double x = v[i];
    float f = (float) x;
    printf ("%g: round %g trunc %g rint %g lround %ld llround %ld log2 %.9g\n",
            x, round (x), trunc (x), rint (x), lround (x), (long) llround (x),
            x > 0 ? log2 (x) : 0.0);
    printf ("%g: floorf %g ceilf %g roundf %g truncf %g sqrtf %.6g fabsf %g sinf %.6g cosf %.6g\n",
            x, (double) floorf (f), (double) ceilf (f), (double) roundf (f),
            (double) truncf (f), (double) sqrtf (fabsf (f)), (double) fabsf (f),
            (double) sinf (f), (double) cosf (f));
    for (j = 0; j < N; j += 3) {
      double y = v[j];
      printf ("  %g,%g: atan2 %.9g fmod %.9g hypot %.9g fmin %g fmax %g powf %.5g\n",
              x, y, atan2 (x, y), y != 0 ? fmod (x, y) : 0.0, hypot (x, y),
              fmin (x, y), fmax (x, y),
              (double) powf (fabsf (f), (float) (y > 3 || y < -3 ? 1.0 : y)));
    }
  }
  {
    /* sin and cos of one argument: gcc may turn this into cexp */
    volatile double a = 0.75;
    double s = sin (a), c = cos (a);
    double complex z = cexp (0.5 + 0.75 * I);
    printf ("sincos %.9g %.9g cexp %.9g %.9g isfinite %d %d\n", s, c,
            creal (z), cimag (z), isfinite (a) != 0, isfinite (a / 0.0) != 0);
  }
  return 0;
}
