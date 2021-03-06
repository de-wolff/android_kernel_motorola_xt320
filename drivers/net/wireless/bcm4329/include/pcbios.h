/*
 *  PCBIOS OS Environment Layer
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: pcbios.h,v 13.2 2005-06-21 18:01:47 abhorkar Exp $
 */

#ifndef _pcbios_h_
#define _pcbios_h_


/* ACPI system timer related  */

#define ACPITIMER_TICKS_PER_MS	3580	/* 3579.7 ticks per ms */
#define ACPITIMER_TICKS_PER_US	4	/* 3.579 ticks pwr us */
#define MAX_ACPITIMER_TICKCOUNT 0x00FFFFFF /* 24 bit */


#endif	/* _pcbios_h_ */
