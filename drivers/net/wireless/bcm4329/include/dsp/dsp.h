/*******************************************************************************
 * $Id: dsp.h,v 1.14 1999-01-12 00:47:27 khp Exp $
 * dsp.h - top level dsp include
 ******************************************************************************/

#ifndef _DSP_H_
#define _DSP_H_

#ifndef RESOURCE_MANAGER
#define RESOURCE_MANAGER 0
#endif

#include "dsp/types.h"
#include "dsp/utils.h"
#include "dsp/module.h"
#if !RESOURCE_MANAGER
#include "dsp/test.h"
#endif /* RESOURCE_MANAGER */

/* common prototypes */
#if RESOURCE_MANAGER
void *claim_dmem(uint nbytes); /* THROWS(EX_DMEM_CLAIM_FAIL) */
void declare_pointer(void** ptr);
#else /* RESOURCE_MANAGER */
#define claim_dmem(n) test_claim_dmem(n, __FILE__, __LINE__)
#endif /* RESOURCE_MANAGER */

/* constants for dmem */
#define DMEM_ALIGNMENT 8

/* common constants */
#define PI    3.141592654
#define TWOPI 6.283185307

#endif /* _DSP_H_ */
