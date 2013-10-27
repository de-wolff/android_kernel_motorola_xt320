/*
 * HND SiliconBackplane ARM core software interface.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: hndarm.h,v 13.12.236.1 2010-10-21 00:25:17 csm Exp $
 */

#ifndef _hndarm_h_
#define _hndarm_h_

#include <sbhndarm.h>

extern void *hndarm_armr;
extern uint32 hndarm_rev;


extern void si_arm_init(si_t *sih);

extern void enable_arm_ints(uint32 which);
extern void disable_arm_ints(uint32 which);

extern uint32 get_arm_cyclecount(void);
extern void set_arm_cyclecount(uint32 ticks);

extern uint32 get_arm_inttimer(void);
extern void set_arm_inttimer(uint32 ticks);

extern uint32 get_arm_intmask(void);
extern void set_arm_intmask(uint32 ticks);

extern uint32 get_arm_intstatus(void);
extern void set_arm_intstatus(uint32 ticks);

extern void arm_wfi(si_t *sih);
extern void arm_jumpto(void *addr);

extern void traptest(void);

#endif /* _hndarm_h_ */
