/*
 * Generic Broadcom Home Networking Division (HND) PIO engine HW interface
 * This supports the following chips: BCM42xx, 44xx, 47xx .
 *
 * $Copyright Open Broadcom Corporation$
 * $Id: sbhndpio.h,v 13.2 2005-06-21 18:01:47 abhorkar Exp $
 */

#ifndef	_sbhndpio_h_
#define	_sbhndpio_h_

/* PIO structure,
 *  support two PIO format: 2 bytes access and 4 bytes access
 *  basic FIFO register set is per channel(transmit or receive)
 *  a pair of channels is defined for convenience
 */

/* 2byte-wide pio register set per channel(xmt or rcv) */
typedef volatile struct {
	uint16	fifocontrol;
	uint16	fifodata;
	uint16	fifofree;	/* only valid in xmt channel, not in rcv channel */
	uint16	PAD;
} pio2regs_t;

/* a pair of pio channels(tx and rx) */
typedef volatile struct {
	pio2regs_t	tx;
	pio2regs_t	rx;
} pio2regp_t;

/* 4byte-wide pio register set per channel(xmt or rcv) */
typedef volatile struct {
	uint32	fifocontrol;
	uint32	fifodata;
} pio4regs_t;

/* a pair of pio channels(tx and rx) */
typedef volatile struct {
	pio4regs_t	tx;
	pio4regs_t	rx;
} pio4regp_t;

#endif /* _sbhndpio_h_ */
