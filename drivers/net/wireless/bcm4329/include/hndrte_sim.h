/*
 * HND Run Time Environment Simulation specific.
 *
 * $Copyright (C) 2004 Broadcom Corporation$
 *
 * $Id: hndrte_sim.h,v 13.12 2008-11-18 21:17:06 gmo Exp $
 */

#ifndef _hndrte_sim_h_
#define _hndrte_sim_h_

#include <stddef.h>

/* uncached/cached virtual address */
#define	hndrte_uncached(va)		((void *)(va))
#define	hndrte_cached(va)		((void *)(va))

/* map/unmap physical to virtual I/O */
#define	hndrte_reg_map(pa, size)	((void *)(pa))
#define	hndrte_reg_unmap(va)		do {} while (0)

/* map/unmap shared (dma-able) memory */
#define	hndrte_dma_map(va, size)	virt_to_phys(va)
#define	hndrte_dma_unmap(pa, size)	do {} while (0)

extern uint32 rreg32(volatile uint32 *r);
extern uint16 rreg16(volatile uint16 *r);
extern uint8 rreg8(volatile uint8 *r);
extern void wreg32(volatile uint32 *r, uint32 v);
extern void wreg16(volatile uint16 *r, uint16 v);
extern void wreg8(volatile uint8 *r, uint8 v);

extern uintptr virt_to_phys(void *p);
extern void *phys_to_virt(uintptr pa);

#ifdef ARM7TIMESTAMPS

#define TIMER_BASE			0x0a800000

#define CURRENT_TIME()		(*((volatile uint32*)(TIMER_BASE + 0x04)) & 0xFFFF)
#define ENABLE_TIMER()		do { (*((volatile uint32*)(TIMER_BASE + 0x08))) = 0x80;} while (0)
#define DISABLE_TIMER()		do { (*((volatile uint32*)(TIMER_BASE + 0x08))) = 0x00;}.while (0)

#define PROF_START()		do { __gcycles = 0; __gtimer = CURRENT_TIME();} while (0)
#define PROF_SUSPEND()		(__gcycles += ((__gtimer - CURRENT_TIME()) & 0xFFFF) - __gcaladj)
#define PROF_RESUME()		(__gtimer = CURRENT_TIME())
#define PROF_INTERIM(tag)					\
		do {						\
			PROF_SUSPEND(); 			\
			printf("Interim profile %s, %lu cycles\n", \
				tag, __gcycles);		\
			PROF_RESUME();				\
		} while (0)
#define PROF_FINISH(tag)					\
		do {						\
			PROF_SUSPEND();				\
			printf("Final profile %s, %lu cycles\n", \
				tag, __gcycles);		\
		} while (0)

#endif /* ARM7TIMESTAMPS */

#ifdef CORTEXM3TIMESTAMPS

#define TIMER_BASE			0xe000e010

#define CURRENT_TIME()		curr_time()

extern uint32 curr_time();

#define ENABLE_TIMER()						\
		do {						\
			unsigned int volatile			\
			*p = (unsigned int volatile *)0xe000e010; \
			p[0] = 0x00000000;			\
			p[1] = 0x00FFFFFF;			\
			p[2] = 0x00000000;			\
			p[0] = 0x00000005;			\
		} while (0)

#define DISABLE_TIMER()						\
		do {						\
			unsigned int volatile			\
			*p = (unsigned int volatile *)0xe000e010; \
			unsigned int a = p[0];			\
			if (a & 0x00010000)			\
				printf("ERROR: CortexM3 time-stamp overflowed\n"); \
			else					\
				p[0] = 0x00000000;		\
		} while (0)

/* Enable the timer everytime you start profiling because its only
 * 24bits counter and not 32 bits, will get over in < 1sec for 80MHz clock
 */
#define PROF_START()						\
		do {						\
			ENABLE_TIMER();				\
			__gcycles = 0;				\
			__gtimer = CURRENT_TIME();		\
		} while (0)
#define PROF_SUSPEND()						\
		(__gcycles +=					\
			((__gtimer - CURRENT_TIME()) &		\
			0xFFFFFF) - __gcaladj)
#define PROF_RESUME()			(__gtimer = CURRENT_TIME())
#define PROF_INTERIM(tag)					\
		do {						\
			PROF_SUSPEND();				\
			printf("Interim profile %s, %lu cycles\n", \
				tag, __gcycles);		\
			PROF_RESUME();				\
		} while (0)
#define PROF_FINISH(tag)					\
		do {						\
			PROF_SUSPEND();				\
			printf("Final profile %s, %lu cycles\n", \
				tag, __gcycles);		\
		} while (0)

#endif /* CORTEXM3TIMESTAMPS */

extern uint32 __gcycles, __gtimer, __gcaladj;

/* Cache support */
static inline void caches_on(void) { return; };
static inline void blast_dcache(void) { return; };
static inline void blast_icache(void) { return; };
static inline void flush_dcache(uint32 base, uint size) { return; };
static inline void flush_icache(uint32 base, uint size) { return; };

#endif	/* _hndrte_sim_h_ */
