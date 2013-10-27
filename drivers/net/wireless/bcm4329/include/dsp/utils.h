/*******************************************************************************
 * $Id: utils.h,v 1.55 2008-12-08 23:40:02 akira Exp $
 * utils.h - dsp related macros and inline function utilities
 ******************************************************************************/

#ifndef _DSP_UTILS_H_
#define _DSP_UTILS_H_

#include <math.h> /* this should be the only dependency on libm */
#include "dsp/types.h"

/* prototypes for functions in utils.c */
uint16 dB_ui16(uint16);
uint16 db_power(uint32);
uint32 db_power_inv(uint16);
uint16 mag_approx(int16, int16);
void mag_approx_2(cint16 *, uint32 *, uint16 *, uint32 *, uint8);

#if COUNT_OPS

typedef struct {
    uint32 add;
    uint32 sub;
    uint32 mul;
    uint32 muladd;
    uint32 mulsub;
    uint32 div;
    uint32 add2;
    uint32 sub2;
    uint32 cfloat;
    uint32 c_mag2;
    uint32 c_neg;
    uint32 c_add;
    uint32 c_mpy;
    uint32 c_mpya;
    uint32 rc_mpy;
    uint32 rc_mpya;
    uint32 c_conj;
} opcounts_t;

extern opcounts_t opcounts;

#endif /* COUNT_OPS */

#if defined(_X86_)
#include "x86/proc_utils.h"
#endif /* _X86_ */

/* basic mux operation - can be optimized on several architectures */
#define MUX(pred, true, false) ((pred) ? (true) : (false))

#ifndef MAX
#define MAX(a, b) MUX((a) > (b), (a), (b))
#endif
#ifndef MIN
#define MIN(a, b) MUX((a) < (b), (a), (b))
#endif

/* modulo inc/dec - assumes x E [0, bound - 1] */
#define MODDEC(x, bound) MUX((x) == 0, (bound) - 1, (x) - 1)
#define MODINC(x, bound) MUX((x) == (bound) - 1, 0, (x) + 1)

/* modulo inc/dec, bound = 2^k */
#define MODDEC_POW2(x, bound) (((x) - 1) & ((bound) - 1))
#define MODINC_POW2(x, bound) (((x) + 1) & ((bound) - 1))

/* modulo add/sub - assumes x, y E [0, bound - 1] */
#define MODADD(x, y, bound) \
    MUX((x) + (y) >= (bound), (x) + (y) - (bound), (x) + (y))
#define MODSUB(x, y, bound) \
    MUX(((int)(x)) - ((int)(y)) < 0, (x) - (y) + (bound), (x) - (y))

/* module add/sub, bound = 2^k */
#define MODADD_POW2(x, y, bound) (((x) + (y)) & ((bound) - 1))
#define MODSUB_POW2(x, y, bound) (((x) - (y)) & ((bound) - 1))

/* round towards nearest integer */
//#define ROUND(x, s) (((x) + (1 << ((s) - 1))) >> (s))
#define HWROUND(x, s) (((x) >> (s)) + (((x) >> ((s) - 1)) & 1))
#define HWROUND0(x, s) (((x) >> (s)) + (((x) >> ((s) - 1)) & (s!=0)))

/* convert a fixedpoint number to float */
/* i is the fixedpoint integer, p is the position of the fractional point */
#define FIXED2FLOAT(i, p) ((float)(i)/(1<<(p)))
#define FIXED2DOUBLE(i, p) ((double)(i)/(1<<(p)))

/* absolute value */
#define ABS_I8(x)  ((((x) >>  7) ^ (x)) - ((x) >>  7))
#define ABS_I16(x) ((((x) >> 15) ^ (x)) - ((x) >> 15))
#define ABS_I32(x) ((((x) >> 31) ^ (x)) - ((x) >> 31))

/* limit to [min, max] */
#define LIMIT(x, min, max) \
    ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/* limit to  max */
#define LIMIT_TO_MAX(x, max) \
    (((x) > (max) ? (max) : (x)))

/* limit to min */
#define LIMIT_TO_MIN(x, min) \
    (((x) < (min) ? (min) : (x)))

#define R_SAT(x,num_bits) \
    LIMIT(x, -(1<<(num_bits-1)), ((1<<(num_bits-1))-1) )

#define R_SAT64(x,num_bits) \
    LIMIT(x, -(int64(1)<<(num_bits-1)), ((int64(1)<<(num_bits-1))-int64(1)) )

/* abstracted libm functions */
#define FLOOR(x)    floor(x)
#define CEIL(x)     ceil(x)
#define ATAN2(x, y) atan2(x, y)
#define LOG10(x)    log10(x)
#define LOG(x)      log(x)
#define FMOD(x, y) \
    MUX((x) > 0.0, fmod((x), (y)), fmod((x), (y)) + (y))
#define SQRT(x)     sqrt(x)

/* bit level operators */
#define EXTRACT_UI32(x, start, nbits) \
     (((uint32)(x) << (31 - (start))) >> (32 - (nbits)))

#if COUNT_OPS

/* basic arithmetic ops */
#define MUL(x, y)       ( opcounts.mul++, ((x) * (y)) )
#define MULADD(a, x, y) ( opcounts.muladd++, ((a) + (x) * (y)) )
#define MULSUB(a, x, y) ( opcounts.mulsub++, ((a) - (x) * (y)) )
#define DIV(x, y)       ( opcounts.div++, ((x) / (y)) )
#define ADD(x, y)       ( opcounts.add++, ((x) + (y)) )
#define SUB(x, y)       ( opcounts.sub++, ((x) - (y)) )
#define ADD2(x, y)      ( opcounts.add2++, ((x) * (x) + (y) * (y)) )
#define SUB2(x, y)      ( opcounts.sub2++, ((x) * (x) - (y) * (y)) )

#else /* !COUNT_OPS */

/* basic arithmetic ops */
#define MUL(x, y)       ((x) * (y))
#define MULADD(a, x, y) ((a) + (x) * (y))
#define MULSUB(a, x, y) ((a) - (x) * (y))
#define DIV(x, y)       ((x) / (y))
#define ADD(x, y)       ((x) + (y))
#define SUB(x, y)       ((x) - (y))
#define ADD2(x, y)      ((x) * (x) + (y) * (y))
#define SUB2(x, y)      ((x) * (x) - (y) * (y))

#endif /* COUNT_OPS */

static INLINE cfloat_t
CFLOAT(
    float_t i,
    float_t q
)
{
    cfloat_t z;
    z.i = i;
    z.q = q;
    return(z);
}

static INLINE float_t
C_MAG2(
    cfloat_t x
)
{
#if COUNT_OPS
    opcounts.c_mag2++;
#endif
    return(x.i*x.i + x.q*x.q);
}

static INLINE cfloat_t
C_NEG(
    cfloat_t x
)
{
    cfloat_t z;

    z.i = -x.i;
    z.q = -x.q;
    return(z);
}

static INLINE cfloat_t
C_MPY(
    cfloat_t x,
    cfloat_t y
)
{
    cfloat_t z;

    z.i = x.i * y.i - x.q * y.q;
    z.q = x.i * y.q + x.q * y.i;
#if COUNT_OPS
    opcounts.c_mpy++;
#endif
    return(z);
}

static INLINE void
C_MPYA(
    cfloat_t x,
    cfloat_t y,
    cfloat_t *acc
)
{
    acc->i += x.i * y.i - x.q * y.q;
    acc->q += x.i * y.q + x.q * y.i;
#if COUNT_OPS
    opcounts.c_mpya++;
#endif
}

static INLINE void
C_MPYS(
    cfloat_t x,
    cfloat_t y,
    cfloat_t *acc
)
{
    acc->i -= x.i * y.i - x.q * y.q;
    acc->q -= x.i * y.q + x.q * y.i;
#if COUNT_OPS
    opcounts.c_mpya++;
#endif
}

static INLINE cfloat_t
RC_MPY(
    float_t x,
    cfloat_t y
)
{
    cfloat_t z;
    z.i = x * y.i;
    z.q = x * y.q;
#if COUNT_OPS
    opcounts.rc_mpy++;
#endif
    return(z);
}

static INLINE void
RC_MPYA(
    float_t x,
    cfloat_t y,
    cfloat_t *acc
)
{
    acc->i += x * y.i;
    acc->q += x * y.q;
#if COUNT_OPS
    opcounts.rc_mpya++;
#endif
}

static INLINE cfloat_t
C_CONJ(
    cfloat_t x
)
{
    cfloat_t z;

    z.i = x.i;
    z.q = -x.q;
    return(z);
}

static INLINE cfloat_t
CLOAD(
    float_t i,
    float_t q
)
{
    cfloat_t z;
    z.i = i;
    z.q = q;
    return(z);
}

static INLINE cint8
CLOAD8(
    int8 i,
    int8 q
)
{
    cint8 z;
    z.i = i;
    z.q = q;
    return(z);
}

static INLINE cint16
CLOAD16(
    int16 i,
    int16 q
)
{
    cint16 z;
    z.i = i;
    z.q = q;
    return(z);
}

static INLINE cint32
CLOAD32(
    int32 i,
    int32 q
)
{
    cint32 z;
    z.i = i;
    z.q = q;
    return(z);
}

static INLINE cint64
CLOAD64(
    int64 i,
    int64 q
)
{
    cint64 z;
    z.i = i;
    z.q = q;
    return(z);
}

#if defined(FIXED_POINT)
static INLINE uint16
C_MAG2_FXPT(
    cint16 x,
    uint16 nfracbits
)
{
#if COUNT_OPS
    opcounts.c_mag2++;
#endif
    return(HWROUND((x.i * x.i + x.q * x.q), nfracbits));
}
#endif



#if defined(FIXED_POINT)
static INLINE cint16
C_NEG16(
    cint16 x
)
{
    cint16 z;

    z.i = -x.i;
    z.q = -x.q;
    return(z);
}
#endif

/*******************************************************************
 * unsigned 16 x unsigned 16 bit multiply, returns rounded 16 bits
 ********************************************************************/

#if defined(FIXED_POINT)
static INLINE uint16
MULT16(
    uint16 x,
    uint16 y,
    uint nbits
)
{
    uint16 z;

    z = HWROUND(x * y, nbits);
#if COUNT_OPS
    opcounts.mul++;
#endif
    return (z);
}
#endif


#if defined(FIXED_POINT)
static INLINE cint16
C16C16_MPY(
    cint16 x,
    cint16 y,
    uint16 nfracbits
)
{
    cint16 z;

    z.i = HWROUND( (x.i*y.i - x.q*y.q),nfracbits);
    z.q = HWROUND( (x.i*y.q + x.q*y.i),nfracbits);
#if COUNT_OPS
    opcounts.c_mpy++;
#endif
    return(z);
}
#endif

/* compute log2(x) where x = 2^k */
static INLINE int
LOG2(
    uint32 x
)
{
    int log2_x = 0;
    while ((x >>= 1) != 0)
        log2_x++;
    return log2_x;
}

static INLINE int
INT64_LOG2(
    uint64 x
)
{
    int log2_x = 0;
    while ((x >>= 1) != 0)
        log2_x++;
    return log2_x;
}


#if defined(_MMX_) && defined(_MSC_VER)

/* Vector Dot-product
 *
 * First and second parameters are pointers to fixedpoint vectors,
 * stored in an array of int16 values.
 *
 * The third parameter, vecsize, is the number of elements
 * in each vector, and must be a multiple of 4.
 *
 * Returns the scalar dot product result as an int32.
 */
static INLINE int32
VEC_DOT(int16* va, int16* vb, uint vecsize)
{
	int32 result;

__asm {
    MOV eax, va
    MOV ebx, vb
    MOV ecx, vecsize

    PXOR	mm2, mm2	; mm2 <- 0 for software piplining
	PXOR	mm7, mm7	; mm7 <- 0 accumulator, even and odd parts

loop1:
    MOVQ    mm0, 0[eax]	; mm0 <- {a0, a1, a2, a3}
    PADDD   mm7, mm2 	; mm7 <- {acc.e, acc.o} sofware piplining

    MOVQ    mm1, 0[ebx]	; mm1 <- {b0, b1, b2, b3}
    MOVQ    mm2, mm0 	; mm2 <- {a0, a1, a2, a3}

    PMADDWD mm2, mm1 	; mm2 <- {a0*b0 + a1*b1, a2*b2 + a3*b3}

    ADD	    eax, 8		; src1 index++
    ADD	    ebx, 8		; src2 index++

    SUB	    ecx, 4		; vecsize - 4

    JA loop1

    PADDD   mm7, mm2 	; mm7 <- {acc.e, acc.o} sofware piplining

    MOVQ    mm0, mm7 	; copy acc_i
    PSRLQ   mm7, 32 	; shift acc.e down, mm0 <- {0, acc.e}

    PADDD   mm7, mm0 	; sum acc into low word, mm7 <- {???, acc}

    MOVD    result, mm7	; move {acc_q16, acc_i16} to result
	}

	return result;
}

/* Complex Vector Dot-product
 *
 * First and second parameters are pointers to fixedpoint complex
 * vectors, stored in an array of int16 values in the form
 * vec = { i1, q1, i2, q2, i3, q3, ... }
 *
 * The third parameter, vecsize, is the number of complex elements
 * in each vector, and must be even.
 *
 * Returns the scalar dot product result as a cint32.
 */
static INLINE cint32
CVEC_DOT(cint16* va, cint16* vb, uint vecsize)
{
	cint32 result;

__asm {
    MOV eax, va
    MOV ebx, vb
    MOV ecx, vecsize

    PXOR	mm2, mm2	; mm2 <- 0
    PXOR	mm3, mm3	; mm3 <- 0
	PXOR	mm4, mm4	; mm4 <- 0 acc_i accumulator, odd and even parts
    PXOR	mm5, mm5	; mm5 <- 0 acc_q accumulator, odd and even parts

    PCMPEQD mm7, mm7	; mm7 <- -1
    PSRLD   mm7, 31		; mm7 <- [0,  1,  0, 1]
	PCMPEQW mm4, mm7	; mm4 <- [-1, 0, -1, 0]
	POR		mm7, mm4	; mm7 <- [-1, 1, -1, 1]
	PXOR	mm4, mm4	; mm4 <- 0

loop1:
    MOVQ    mm0, 0[eax]	; mm0 <- { a1.q, a1.i,  a0.q,  a0.i}
    PADDD   mm4, mm3 	; mm4 <- {acc_i.o, acc_i.e} sofware piplining

    PMULLW  mm0, mm7	; mm0 <- {-a1.q, a1.i, -a0.q,  a0.i}
    PADDD   mm5, mm2 	; mm5 <- {acc_q.o, acc_q.e} sofware piplining

    MOVQ    mm1, 0[ebx]	; mm1 <- {b1.q, b1.i, b0.q, b0.i}

    ;; generate q coeffs
    MOVQ    mm2, mm0	; copy a0, a1
    MOVQ    mm6, mm0 	; copy a0, a1

    PSLLD   mm2, 16 	; shift i up,    mm3 <- {a1.i,     0, a0.i,     0}
    PSRLD   mm6, 16 	; shift -q down, mm2 <- {   0, -a1.q,    0, -a0.q}

	MOVQ	mm3, mm0	; mm3 <- {-a1.q, a1.i, -a0.q,  a0.i}

    PSUBW   mm2, mm6 	; subtract q,    mm2 <- {a1.i,  a1.q, a0.i,  a0.q}

    PMADDWD mm3, mm1 	; mm0 <- -b1.q*a1.q + b1.i*a1.i, -b0.q*a0.q + b0.i*a0.i
    PMADDWD mm2, mm1 	; mm2 <-  b1.q*a1.i + b1.i*a1.q,  b0.q*a0.i + b0.i*a0.q

    ADD	    eax, 8		; src1 index++
    ADD	    ebx, 8		; src2 index++

    SUB	    ecx, 2		; vecsize - 2

    JA loop1

    PADDD   mm4, mm3 	; mm4 <- {acc_i.o, acc_i.e} sofware piplining
    PADDD   mm5, mm2 	; mm5 <- {acc_q.o, acc_q.e} sofware piplining

    MOVQ    mm0, mm4 	; copy acc_i
    MOVQ    mm1, mm5 	; copy acc_q

    PSRLQ   mm0, 32 	; shift i down, mm0 <- {0, acc_i.o}
    PADDD   mm0, mm4 	; sum acc_i into low word, mm0 <- {???, acc_i}

    PSRLQ   mm1, 32 	; shift q down, mm1 <- {0, acc_q.o}
    PADDD   mm1, mm5 	; sum acc_q into low word, mm1 <- {???, acc_q}

    PUNPCKLDQ mm0, mm1	; mm0 <- {acc_q, acc_i}

    MOVQ    result, mm0	; move {acc_q16, acc_i16} to result
	}

	return result;
}

/* Complex Vector Dot-product, with expanded terms
 *
 * The first parameter is a pointer to the first fixedpoint complex
 * vector in an array of int16 values in the form
 * vec = { i1, q1, i2, q2, i3, q3, ... }
 *
 * The second parameter is a pointer to the values of the second
 * vector, expanded to match the terms of the complex mutiplication, in
 * an array of int16 values in the form
 * vec_exp = { i1, -q1, q1, i1, i2, -q2, q2, i2, i3, -q3, q3, i3, ... }
 *
 * The third parameter, vecsize, is the number of complex elements
 * in each vector, and must be even.
 *
 * Returns the scalar dot product result as a cint32.
 */
static INLINE cint32
CVEC_DOT_EXP(cint16* va, cint16* vb_exp, uint vecsize)
{
	cint32 result;
__asm {
    MOV eax, va
    MOV ebx, vb_exp
    MOV ecx, vecsize
    PXOR	mm2, mm2	; mm2 <- 0
    PXOR	mm3, mm3	; mm3 <- 0
	PXOR	mm4, mm4	; mm4 <- 0

loop1:
	; fetch two input values
    MOVQ    mm0, 0[eax]	; mm0 <- {a1.q, a1.i, a0.q,  a0.i}
	; accumulate first result
    PADDD   mm4, mm2 	; mm4 <- {acc_q, acc_i}

	; fetch first filter coeffs
    MOVQ    mm1, 0[ebx]	; mm1 <- {b0.i, b0.q, -b0.q, b0.i}
	; copy the input
    MOVQ    mm2, mm0	; mm2 <- {a1.q, a1.i, a0.q,  a0.i}

	; duplicate the first value
    PUNPCKLDQ mm2, mm2	; mm2 <- {a0.q, a0.i, a0.q,  a0.i}
	; accumulate second result
    PADDD   mm4, mm3 	; mm4 <- {acc_q, acc_i}

	; do first multipy
    PMADDWD mm2, mm1 	; mm2 <- a0.q*b0.i + a0.i*b0.q, -a0.q*b0.q + a0.i*b0.i
	; copy the input
    MOVQ    mm3, mm0	; mm3 <- { a1.q, a1.i,  a0.q,  a0.i}

	; fetch second filter coeffs
    MOVQ    mm1, 8[ebx]	; mm1 <- {b1.i, b1.q, -b1.q, b1.i}
	; duplicate the second value
    PUNPCKHDQ mm3, mm3	; mm3 <- { a1.q, a1.i,  a1.q,  a1.i}

	; do second multipy
    PMADDWD mm3, mm1 	; mm3 <- a1.q*b1.i + a1.i*b1.q, -a1.q*b1.q + a1.i*b1.i

    ADD	    eax, 8		; src1 index++
    ADD	    ebx, 16		; src2 index++

    SUB	    ecx, 2		; vecsize - 2

    JA loop1

	; accumulate results
    PADDD   mm4, mm2 	; mm4 <- {acc_q, acc_i}
    PADDD   mm4, mm3 	; mm4 <- {acc_q, acc_i}

    MOVQ    result, mm4	; move {acc_q, acc_i} to result
    }

	return result;
}

#endif /* _MMX && _MSC_VER */

#endif /* _DSP_UTILS_H_ */
