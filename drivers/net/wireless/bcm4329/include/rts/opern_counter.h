/*************************************************************
 * $Id: opern_counter.h,v 5.1 1998-06-04 18:18:54 harip Exp $
 * opern_counter.h - operation count data structure
 ************************************************************/

#ifndef _OPERN_COUNTER_H_
#define _OPERN_COUNTER_H_

#include "typedefs.h"

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
	uint32 c_mpy;
	uint32 c_mpya;
	uint32 rc_mpy;
	uint32 rc_mpya;
	uint32 c_conj;
} opcounts_t;

#endif /* _OPERN_COUNTER_H_ */
