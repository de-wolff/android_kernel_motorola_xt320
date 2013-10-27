/*
 * rts.h: Common definitions for the Run-time System.
 *
 * Copyright 1998 Epigram, Inc.
 *
 * $Id: rts.h,v 5.2 1998-12-11 02:57:26 gmo Exp $
 */

#ifndef _RTS_H_
#define _RTS_H_

#include "typedefs.h"

/* Where the program name is recorded */

extern char argv0[]; /* Set from the string passed to rts_init */

/* Typedef for the main and exit cleanup routines */

typedef void (*hdlr_t) (void);

/* Typedef for the initcheck routine */

typedef bool (*initrtn_t) (void);


/*
 * Initialize the RTS environment,
 * make a copy of the program name to a global variable, 
 * and set an exit handler.
 */

extern void rts_init (char *pgmpath, hdlr_t handler);

/*
 * Do the work to run as a deamon.
 *
 * Returns 0 if unsuccessful.
 */
extern int rts_do_daemon(initrtn_t init_routine, hdlr_t main_routine, char *dname, void *xevent);

#endif /* _RTS_H_ */
