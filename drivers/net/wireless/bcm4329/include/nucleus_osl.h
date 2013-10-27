/*
 * Nucleus OS Support Layer
 *
 * $ Copyright Broadcom Corporation 2008 $
 * $Id: nucleus_osl.h,v 1.4 2009-04-17 22:54:58 nvalji Exp $
 */


#ifndef nucleus_osl_h
#define nucleus_osl_h

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Include Files ---------------------------------------------------- */

/* Map bcopy to memcpy. */
#define BWL_MAP_BCOPY_TO_MEMCPY
#include "generic_osl.h"

#include "nucleus.h"


/* ---- Constants and Types ---------------------------------------------- */

/* This is really platform specific and not OS specific. */
#ifndef BWL_NU_TICKS_PER_SECOND
#define BWL_NU_TICKS_PER_SECOND 1024
#endif

#define OSL_MSEC_TO_TICKS(msec)  ((BWL_NU_TICKS_PER_SECOND * (msec)) / 1000)

#define OSL_TICKS_TO_MSEC(ticks) ((1000 * (ticks)) / BWL_NU_TICKS_PER_SECOND)


/* Get processor cycle count */
#define OSL_GETCYCLES(x)	((x) = NU_Retrieve_Clock())


/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */


#ifdef __cplusplus
	}
#endif

#endif  /* nucleus_osl_h  */
