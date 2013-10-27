/*
 * Nucleus OS Support Extension Layer
 *
 * $ Copyright Broadcom Corporation 2008 $
 * $Id: nucleus_osl_ext.h,v 1.4 2009-11-27 18:32:41 nvalji Exp $
 */


#ifndef _nucleus_osl_ext_h_
#define _nucleus_osl_ext_h_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Include Files ---------------------------------------------------- */

#include <nucleus.h>
#include <typedefs.h>


/* ---- Constants and Types ---------------------------------------------- */

/* Semaphore. */
typedef NU_SEMAPHORE osl_ext_sem_t;

/* Mutex. */
typedef NU_SEMAPHORE osl_ext_mutex_t;

/* Timer. */
typedef NU_TIMER osl_ext_timer_t;

/* Task. */
typedef struct osl_ext_task_t
{
	NU_TASK		nu_task;
	void		*func;
	void		*arg;
	uint8		*stack;
	unsigned int	stack_size;
} osl_ext_task_t;

/* Queue. */
typedef struct osl_ext_queue_t
{
	NU_QUEUE	nu_queue;
	char		*nu_buffer;
	unsigned int nu_buffer_size;
} osl_ext_queue_t;

/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */


#ifdef __cplusplus
	}
#endif

#endif  /* _nucleus_osl_ext_h_  */
