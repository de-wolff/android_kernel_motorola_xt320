/*
 * $Id: c6x.h,v 1.6 1997-10-07 04:38:08 khp Exp $
 * c6x.h - inlines for compatibility with c6x intrinsics
 */

#ifndef _C6X_H_
#define _C6X_H_

#include "dsp/defs.h"
#include "dsp/types.h"

static INLINE int32
_add2(int32 src1, int32 src2)
{
    
}

static INLINE uint32
_clr(uint32 src2, uint32 csta, uint32 cstb)
{
    return (uint32)0;
}

static INLINE int32
_ext(uint32 src2, uint32 csta, int32 cstb)
{
    return (int32)0;
}

static INLINE uint32
_extu(uint32 src2, uint32 csta, uint32 cstb)
{
    return (uint32)0;
}

static INLINE uint32
_lmbd(uint32 src1, uint32 src2)
{
    return (uint32)0;
}

static INLINE int32
_mpy(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpyus(uint32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpysu(int32 src1, uint32 src2)
{
    return (int32)0;
}

static INLINE uint32
_mpyu(uint32 src1, uint32 src2)
{
    return (uint32)0;
}

static INLINE int32
_mpyh(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpyhus(uint32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpyhsu(int32 src1, uint32 src2)
{
    return (int32)0;
}

static INLINE uint32
_mpyhu(uint32 src1, uint32 src2)
{
    return (uint32)0;
}

static INLINE int32
_mpyhl(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpyhuls(uint32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpyhslu(int32 src1, uint32 src2)
{
    return (int32)0;
}

static INLINE uint32
_mpyhlu(uint32 src1, uint32 src2)
{
    return (uint32)0;
}

static INLINE int32
_mpylh(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_mpyluhs(uint32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32 
_mpylshu(int32 src1, uint32 src2)
{
    return (int32)0;
}

static INLINE uint32
_mpylhu(uint32 src1, uint32 src2)
{
    return (uint32)0;
}

static INLINE uint32
_norm(int32 src2)
{
    return (uint32)0;
}

static INLINE uint32
_lnorm(int40 src2)
{
    return (uint32)0;
}

static INLINE int32
_sadd(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int40
_lsadd(int32 src1, int40 src2)
{
    return (int40)0;
}

static INLINE int40
_sat(int32 src2)
{
    return (int40)0;
}

static INLINE uint32
_set(uint32 src2, uint32 csta, uint32 cstb)
{
    return (uint32)0;
}

static INLINE int32
_smpy(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_smpyh(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_smpyhl(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int32
_smpyhlh(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE uint32
_sshl(uint32 src2, uint32 src2)
{
    return (uint32)0;
}

static INLINE int32
_ssub(int32 src1, int32 src2)
{
    return (int32)0;
}

static INLINE int40
_lssub(int32 src1, int40 src2)
{
    return (int40)0;
}

static INLINE uint32
_subc(uint32 src1, uint32 src2)
{
    return (uint32)0;
}

static INLINE int32
_sub2(int32 src1, int32 src2)
{
    return (int32)0;
}

#endif /* _C6X_H_ */
