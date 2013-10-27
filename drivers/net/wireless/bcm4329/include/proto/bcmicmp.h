/*
 * Fundamental constants relating to ICMP Protocol
 *
 * $Copyright (C) 2004 Broadcom Corporation$
 *
 * $Id: bcmicmp.h,v 1.2 2008-12-01 22:55:15 nvalji Exp $
 */

#ifndef _bcmicmp_h_
#define _bcmicmp_h_

#ifndef _TYPEDEFS_H_
#include <typedefs.h>
#endif

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>


#define ICMP_CHKSUM_OFFSET	2	/* ICMP body checksum offset */

/* These fields are stored in network order */
BWL_PRE_PACKED_STRUCT struct bcmicmp_hdr
{
	uint8	type;		/* Echo or Echo-reply */
	uint8	code;		/* Always 0 */
	uint16	chksum;		/* Icmp packet checksum */
} BWL_POST_PACKED_STRUCT;

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

#endif	/* #ifndef _bcmicmp_h_ */
