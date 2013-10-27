/*
 * $Copyright (C) Broadcom Corporation$
 *
 * $Id: osl_vx.h,v 1.2 2008-08-12 17:50:33 araja Exp $
 */

#ifndef _OSL_VX_H_
#define _OSL_VX_H_

#include <vxWorks.h>
#include <semLib.h>
#include <wdLib.h>

struct osl_lock
{
	SEM_ID mutex_sem;       /* Mutual exclusion semaphore */
	uint8  name[16];        /* Name of the lock */
};

typedef struct osl_lock *osl_lock_t;

static inline osl_lock_t
OSL_LOCK_CREATE(uint8 *name)
{
	osl_lock_t lock;

	lock = MALLOC(NULL, sizeof(osl_lock_t));

	if (lock == NULL)
	{
		printf("Memory alloc for lock object failed\n");
		return (NULL);
	}

	lock->mutex_sem = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE |
	                             SEM_INVERSION_SAFE);

	if (lock->mutex_sem == NULL)
	{
		printf("semMCreate failed\n");
		MFREE(NULL, lock, sizeof(osl_lock_t));
		return (NULL);
	}

	strncpy(lock->name, name, 16);

	return (lock);
}

static inline void
OSL_LOCK_DESTROY(osl_lock_t lock)
{
	semDelete(lock->mutex_sem);
	MFREE(NULL, lock, sizeof(osl_lock_t));
	return;
}

#define OSL_LOCK(lock)          semTake((lock)->mutex_sem, WAIT_FOREVER)
#define OSL_UNLOCK(lock)        semGive((lock)->mutex_sem)

static inline char *
DEV_IFNAME(void *if_ent)
{
	static char ifname[16];

	/* Extract interface name and index from ifent (emf_iflist_t) */
	strncpy(ifname, ((char *)(if_ent)) + sizeof(int), 14);
	ifname[strlen(ifname)] = *((char *)if_ent) + '0';

	return (ifname);
}

typedef struct osl_timer {
	WDOG_ID timer;
	void    (*fn)(void *);
	void    *arg;
	uint32  ms;
	bool    periodic;
	int32   interval;
	bool    set;
#ifdef BCMDBG
	char    *name;          /* Desription of the timer */
#endif
} osl_timer_t;

extern osl_timer_t *osl_timer_init(const char *name, void (*fn)(void *arg), void *arg);
extern void osl_timer_add(osl_timer_t *t, uint32 ms, bool periodic);
extern void osl_timer_update(osl_timer_t *t, uint32 ms, bool periodic);
extern bool osl_timer_del(osl_timer_t *t);
extern osl_lock_t OSL_LOCK_CREATE(uint8 *name);
extern void OSL_LOCK_DESTROY(osl_lock_t lock);

#endif /* _OSL_VX_H_ */
