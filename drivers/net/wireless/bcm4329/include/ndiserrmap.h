/*
 * NDIS Error mappings
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: ndiserrmap.h,v 13.3 2005-09-13 23:15:41 gmo Exp $
 */

#ifndef _ndiserrmap_h_
#define _ndiserrmap_h_

extern int ndisstatus2bcmerror(NDIS_STATUS status);
extern NDIS_STATUS bcmerror2ndisstatus(int bcmerror);

#endif	/* _ndiserrmap_h_ */
