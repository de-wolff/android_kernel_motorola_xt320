/*
 * SiliconBackplane Sharedcommon core hardware definitions.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: sbsharedc.h,v 13.1 2007-02-02 19:08:40 jqliu Exp $
 */

#ifndef	_SBSHAREDC_H
#define	_SBSHAREDC_H

#ifndef _LANGUAGE_ASSEMBLY

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

typedef volatile struct {
	uint32	capabilities;		/* 0x0 */
	uint32	PAD[2];
	uint32	bist;			/* 0xc */
	uint32	bankstbyctl;		/* 0x10 */
	uint32	PAD[3];
	uint32	wlintstatus;		/* 0x20 */
	uint32	wlintmask;		/* 0x24 */
	uint32	btintstatus;		/* 0x28 */
	uint32	btintmask;		/* 0x2c */
	uint32	wlsem;			/* 0x30 */
	uint32	PAD[3];
	uint32	btsem;			/* 0x40 */
} sharedcregs_t;

#endif /* _LANGUAGE_ASSEMBLY */

/* capabilities */
#define SC_CAP_SMEMSIZE_MASK	0x0000000f
#define SC_CAP_NUMSEMS_MASK	0x000000f0
#define SC_CAP_NUMSEMS_SHIFT	4

/* wlan/bt intstatus and intmask */
#define SC_I_SEM(s)		(1 << (s))

/* wlan/bt semaphore */
#define SC_SEM_REQ(s)	(1 << ((s) << 1))	/* Request */
#define SC_SEM_GIVE(s)	(2 << ((s) << 1))	/* Release */
#define SC_SEM_NAV(s)	(3 << ((s) << 1))	/* Unavailable */

#endif	/* _SBSHAREDC_H */
