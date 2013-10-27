/*
 * HND SiliconBackplane MIPS/ARM cores software interface.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: hndcpu.h,v 13.10.112.1 2011-02-07 19:16:03 willfeng Exp $
 */

#ifndef _hndcpu_h_
#define _hndcpu_h_

#if defined(mips)
#include <hndmips.h>
#elif defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
#include <hndarm.h>
#endif

extern uint si_irq(si_t *sih);
extern uint32 si_cpu_clock(si_t *sih);
extern uint32 si_mem_clock(si_t *sih);
extern void hnd_cpu_wait(si_t *sih);
extern void hnd_cpu_jumpto(void *addr);
extern void hnd_cpu_reset(si_t *sih);
extern void hnd_cpu_deadman_timer(si_t *sih, uint32 val);
extern void si_dmc_phyctl(si_t *sih, uint32 phyctl_val);

#endif /* _hndcpu_h_ */
