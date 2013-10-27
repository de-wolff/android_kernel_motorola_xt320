/*******************************************************************************
 * $Id: test.h,v 1.10 1999-01-11 22:17:59 khp Exp $
 * test.h - defs for dsp test support
 ******************************************************************************/

#ifndef _DSP_TEST_H_
#define _DSP_TEST_H_

#include "dsp/dsp.h"
#include "dsp/module.h"

/* prototypes */
void* test_alloc_dmem(reqs_t *preqs);
void test_free_dmem(void* dmem_ptr);
void test_check_dmem(void);

/* macro expansion of claim_dmem() */
void* test_claim_dmem(uint nbytes, char* file, int line);

#endif /* _DSP_TEST_H_ */
