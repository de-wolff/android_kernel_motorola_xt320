/*
 * $Id: profile_timer.h,v 2.6 1998-06-05 17:39:45 harip Exp $
 * profile_timer.h - code timing utilities
 */

#ifndef _PROFILE_TIMER_H_
#define _PROFILE_TIMER_H_

#include "typedefs.h"

/* timer ticks per millisecond */
uint32
ticks_per_ms(void);

/* Define TIMING = 1 to turn on timers */

#if TIMING == 0
/* Timing disabled with null macros */
#define profile_timer_reset(x)
#define profile_timer_start(x)
#define profile_timer_stop(x)
#define profile_timer_val(x) 0
#else /* TIMING != 0 */

#include <assert.h>

#if defined(CPU) && CPU==R4650
#include "nic.h"
#elif !defined(_X86_)
#error "Profile_Timers only implemented for x86 or R4650"
#endif


/****************************************************************************** 
 *  Constants
 ******************************************************************************/

#define MAX_PROFILE_TIMERS 16

/****************************************************************************** 
 *  Types
 ******************************************************************************/

typedef struct {
	uint32 start_time;
	uint32 total_time;
} profile_timer;

/****************************************************************************** 
 *  Globals
 ******************************************************************************/

extern profile_timer profile_timers[];

/****************************************************************************** 
 *  Macros
 ******************************************************************************/

#if defined(CPU) && CPU==R4650
#define GET_TIME() r4k_timer()
#else 
#define GET_TIME() tsc()
#endif

#define valid_profile_timer(x) assert(x < MAX_PROFILE_TIMERS)

/****************************************************************************** 
 *  Inlines
 ******************************************************************************/

#if defined(_X86_)

#if defined(_MSC_VER)
static __inline uint32
tsc()
{
    volatile uint32 tsc_lo;
	__asm {
		PUSH eax
		PUSH edx
		_emit 0x0f
		_emit 0x31
		MOV tsc_lo, eax
		POP edx
		POP eax
	}
    return tsc_lo;
}
static __inline uint64
tsc_64()
{
    volatile uint32 tsc_lo, tsc_hi;
    volatile uint64 tsc;
	__asm {
		PUSH eax
		PUSH edx
		_emit 0x0f
		_emit 0x31
		MOV tsc_lo, eax
		MOV tsc_hi, edx
		POP edx
		POP eax
	}
    tsc=tsc_lo+(((uint64)tsc_hi)<<32);
    return tsc;
}
#elif defined(__GNUC__)
static INLINE uint32
tsc()
{
	uint32 time_lo;
	uint32 time_hi;
	/* volatile on asm so gcc will not do sub-expression elimination
	 * on the asm line */
	__asm__ volatile (".byte 0x0f, 0x31" : "=a" (time_lo), "=d" (time_hi));
	return time_lo;
}
#endif /* _MSC_VER */

#endif /* _X86_ */

static INLINE void
profile_timer_reset(int x)
{
	valid_profile_timer(x);
	
	profile_timers[x].total_time = 0;
}

static INLINE void
profile_timer_start(int x)
{
	valid_profile_timer(x);
	
	profile_timers[x].start_time = GET_TIME();
}

static INLINE uint32
profile_timer_stop(int x)
{
	uint32 increment = GET_TIME() - profile_timers[x].start_time;

	valid_profile_timer(x);

	profile_timers[x].total_time += increment;

	return increment;
}

static INLINE uint32
profile_timer_val(int x)
{
	valid_profile_timer(x);
	return profile_timers[x].total_time;
}

#endif /* TIMING */

#endif /* _TIMER_H_ */
