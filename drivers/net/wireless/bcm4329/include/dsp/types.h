/*******************************************************************************
 * $Id: types.h,v 1.14 1998-06-01 15:40:57 stafford Exp $
 * typedefs.h - dsp specific typedefs
 ******************************************************************************/

#ifndef _DSP_TYPES_H_
#define _DSP_TYPES_H_

/* start with top level typedefs */
#include "typedefs.h"

#if defined(_C6X_)
/* int type particular to C6X */
typedef signed long	int40;
typedef unsigned long	uint40;
#endif

/* base neutral memory offset */
typedef uint32 mem_offset_t;

/* DSP pointer */
typedef uint32 dsp_ptr_t;

/* complex types */
typedef struct {
    int8 i;
    int8 q;
} cint8;

typedef struct {
    int16 i;
    int16 q;
} cint16;

typedef struct {
    int32 i;
    int32 q;
} cint32;

#if defined(_C6X_)
typedef struct {
    int40 i;
    int40 q;
} cint40;
#endif

#if (defined(_MSC_VER) && !defined(__MWERKS__)) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))
typedef struct {
    int64 i;
    int64 q;
} cint64;
#endif

typedef struct {
    uint8 i;
    uint8 q;
} cuint8;

typedef struct {
    uint16 i;
    uint16 q;
} cuint16;

typedef struct {
    uint32 i;
    uint32 q;
} cuint32;

#if defined(_C6X_)
typedef struct {
    uint40 i;
    uint40 q;
} cuint40;
#endif

#if (defined(_MSC_VER) && !defined(__MWERKS__)) || (defined(__GNUC__) && !defined(__STRICT_ANSI__)) || (defined(__ICL) && !(defined(__STDC__)))
typedef struct {
    uint64 i;
    uint64 q;
} cuint64;
#endif

#if !defined(_C6X_)
typedef struct {
    float_t i;
    float_t q;
} cfloat_t;

typedef struct {
    float32 i;
    float32 q;
} cfloat32;

typedef struct {
    float64 i;
    float64 q;
} cfloat64;
#endif

#endif /* _DSP_TYPES_H_ */
