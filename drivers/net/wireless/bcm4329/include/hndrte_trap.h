/*
 * HNDRTE Trap handling.
 *
 * $Copyright (C) 2003 Broadcom Corporation$
 *
 * $Id: hndrte_trap.h,v 13.10 2009-07-15 20:45:42 csm Exp $
 */

#ifndef	_HNDRTE_TRAP_H
#define	_HNDRTE_TRAP_H


/* Trap handling */


#if defined(mips)
#include <hndrte_mipstrap.h>
#elif defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
#include <hndrte_armtrap.h>
#endif


#ifndef	_LANGUAGE_ASSEMBLY

#include <typedefs.h>

extern uint32 hndrte_set_trap(uint32 hook);
extern void hndrte_die(uint32 line);
extern void hndrte_unimpl(void);

#endif	/* !_LANGUAGE_ASSEMBLY */
#endif	/* _HNDRTE_TRAP_H */
