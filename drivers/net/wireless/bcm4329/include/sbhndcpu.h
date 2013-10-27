/*
 * HND SiliconBackplane MIPS/ARM hardware description.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: sbhndcpu.h,v 13.3 2008-03-28 19:09:19 gmo Exp $
 */

#ifndef _sbhndcpu_h_
#define _sbhndcpu_h_

#if defined(mips)
#include <mips33_core.h>
#include <mips74k_core.h>
#elif defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
#include <sbhndarm.h>
#endif

#endif /* _sbhndcpu_h_ */
