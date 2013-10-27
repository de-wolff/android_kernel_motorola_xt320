/*
 * HND SiliconBackplane chipcommon support.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: hndchipc.h,v 13.7 2008-11-13 23:38:15 gmo Exp $
 */

#ifndef _hndchipc_h_
#define _hndchipc_h_

typedef void (*si_serial_init_fn)(void *regs, uint irq, uint baud_base, uint reg_shift);

extern void si_serial_init(si_t *sih, si_serial_init_fn add);

extern void *hnd_jtagm_init(si_t *sih, uint clkd, bool exttap);
extern void hnd_jtagm_disable(si_t *sih, void *h);
extern uint32 jtag_scan(si_t *sih, void *h, uint irsz, uint32 ir0, uint32 ir1,
                        uint drsz, uint32 dr0, uint32 *dr1, bool rti);

typedef	void (*cc_isr_fn)(void* cbdata, uint32 ccintst);

extern bool si_cc_register_isr(si_t *sih, cc_isr_fn isr, uint32 ccintmask, void *cbdata);
extern void si_cc_isr(si_t *sih, chipcregs_t *regs);

#endif /* _hndchipc_h_ */
