/*
 * NetBSD  2.0 OS Common defines
 * Private between the OSL and BSD per-port code
 *
 * $Copyright (C) 2001-2005 Broadcom Corporation$
 *
 * $Id: bcmbsd.h,v 13.1 2005-07-15 01:20:04 tongchia Exp $
 */

#ifndef _bcmbsd_h_
#define _bcmbsd_h_

/* This macro checks to see if the data area is within the same page */
#define IN_PAGE(p, len) ((((PAGE_SIZE-1) & (uint)(p)) + ((len) - 1)) < PAGE_SIZE)

#endif /* _bcmbsd_h_ */
