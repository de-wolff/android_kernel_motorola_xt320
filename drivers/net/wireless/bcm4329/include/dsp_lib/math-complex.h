// Compatibility file for C99 and C++ complex.  This header
// can be included by either C99 or ANSI C++ programs to
// allow complex arithmetic to be written in a common subset.
// Note that overloads for both the real and complex math
// functions are available after this header has been
// included.

#ifndef MATH_COMPLEX_H_INCLUDED
#define MATH_COMPLEX_H_INCLUDED

/* Refer to 64-bit integers with typedefs "int64_t" and "uint64_t" */
/* Get defs for int16_t, etc */
#ifndef _WIN32
#include <inttypes.h>
#endif

#ifdef __cplusplus

#include <dsp_lib/complex.h>
//#include <cmath>
//#include <complex>

//using namespace std;
using namespace SPUC;

typedef complex<float> float_complex;
typedef complex<double> double_complex;
typedef complex<long double> long_double_complex;
typedef complex<int> int_complex;
typedef complex<long> long_complex;
typedef complex<int16_t> int16_complex;
typedef complex<int32_t> int32_complex;

#else

// Note that <tgmath.h> includes <math.h> and <complex.h>
#include <tgmath.h>

typedef float complex float_complex;
typedef double complex double_complex;
typedef long double complex long_double_complex;
typedef int complex int_complex;
typedef long complex long_complex;
typedef int32_t complex int32_complex;
typedef int16_t complex int16_complex;

#define float_complex(r,i) ((float)(r) + ((float)(i))*I)
#define double_complex(r,i) ((double)(r) + ((double)(i))*I)
#define long_double_complex(r,i) ((long double)(r) + ((long double)(i))*I)
#define int_complex(r,i) ((int)(r) + ((int)(i))*I)
#define long_complex(r,i) ((long)(r) + ((long)(i))*I)
#define int32_complex(r,i) ((int32_t)(r) + ((int32_t)(i))*I)
#define int16_complex(r,i) ((int16_t)(r) + ((int16_t)(i))*I)

#define real(x) creal(x)
#define imag(x) cimag(x)
//#define abs(x) fabs(x)
#define arg(x) carg(x)

#endif  // #ifdef __cplusplus

#endif  // #ifndef MATH_COMPLEX_H_INCLUDED
