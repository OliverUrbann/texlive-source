/* tl-amiga-cxx.h: force-included into C++ compilations for AmigaOS 3
   with ixemul.library (see amiga/build.sh, "C++").  */
#ifndef TL_AMIGA_CXX_H
#define TL_AMIGA_CXX_H

/* ixemul's <stddef.h> hides gcc's, which would define this.  */
#if defined (__cplusplus) && !defined (_GCC_MAX_ALIGN_T)
#define _GCC_MAX_ALIGN_T
typedef struct {
  long long __max_align_ll __attribute__ ((__aligned__ (__alignof__ (long long))));
  long double __max_align_ld __attribute__ ((__aligned__ (__alignof__ (long double))));
} max_align_t;
#endif

/* C99 float math.  ixemul-fixes.h maps these names to gcc builtins with
   macros, but <cmath> removes such macros.  For C++, real inline
   functions instead.  */
#ifdef __cplusplus
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
#define TL_AMIGA_F1(n) static inline float n##f (float x) { return (float) __builtin_##n (x); }
#define TL_AMIGA_F2(n) static inline float n##f (float x, float y) { return (float) __builtin_##n (x, y); }
TL_AMIGA_F1 (sin) TL_AMIGA_F1 (cos) TL_AMIGA_F1 (tan) TL_AMIGA_F1 (asin)
TL_AMIGA_F1 (acos) TL_AMIGA_F1 (atan) TL_AMIGA_F1 (exp) TL_AMIGA_F1 (log)
TL_AMIGA_F1 (log10) TL_AMIGA_F1 (sqrt) TL_AMIGA_F1 (fabs) TL_AMIGA_F1 (floor)
TL_AMIGA_F1 (ceil) TL_AMIGA_F1 (round) TL_AMIGA_F1 (trunc)
TL_AMIGA_F2 (pow) TL_AMIGA_F2 (atan2) TL_AMIGA_F2 (fmod) TL_AMIGA_F2 (hypot)
#undef TL_AMIGA_F1
#undef TL_AMIGA_F2
#ifndef NAN
#define NAN (__builtin_nanf (""))
#endif
#endif

/* errno values that libstdc++'s <system_error> names and ixemul lacks */
#define ENOMSG 91
#define EILSEQ 92

#endif /* not TL_AMIGA_CXX_H */
