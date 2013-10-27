/*
 * SDIO host client driver interface of Broadcom HNBU
 *     export functions to client drivers
 *     abstract OS and BUS specific details of SDIO
 *
 * $Copyright(c) 2004 Broadcom Corporation$
 * $Id: bcmsdh_generic.h,v 1.2 2008-12-01 22:56:58 nvalji Exp $
 */

#ifndef	_bcmsdh_generic_h_
#define	_bcmsdh_generic_h_

/* ---- Include Files ---------------------------------------------------- */

#include "bcmsdh.h"


/* ---- Constants and Types ---------------------------------------------- */
/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */

extern void* bcmsdh_probe(osl_t *osh);
extern void bcmsdh_remove(osl_t *osh, void *instance);

#endif	/* _bcmsdh_generic_h_ */
