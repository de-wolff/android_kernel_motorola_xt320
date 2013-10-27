/*
 * MIPS 74k definitions
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: mips74k_core.h,v 13.2 2009-08-04 06:26:22 musterc Exp $
 */

#ifndef	_mips74k_core_h_
#define	_mips74k_core_h_

#include <mipsinc.h>

#ifndef _LANGUAGE_ASSEMBLY

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

typedef volatile struct {
	uint32	corecontrol;
	uint32	exceptionbase;
	uint32	PAD[1];
	uint32	biststatus;
	uint32	intstatus;
	uint32	intmask[6];
	uint32	nmimask;
	uint32	PAD[4];
	uint32	gpioselect;
	uint32	gpiooutput;
	uint32	gpioenable;
	uint32	PAD[101];
	uint32	clkcontrolstatus;
} mips74kregs_t;

/* Core specific status flags */
#define SISF_CHG_CLK_OTF_PRESENT	0x0001

#endif	/* _LANGUAGE_ASSEMBLY */

#endif	/* _mips74k_core_h_ */
