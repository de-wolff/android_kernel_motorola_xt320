/*******************************************************************************
 * $Id: probe.h,v 2.7 2000-12-07 01:52:58 abagchi Exp $
 * probe.h - defs for signal probing utility
 *
 * Overview
 * 
 * This is a simple utility for probing data (which could be anything
 * but it was intended mainly for signals in DSP code).  The user just
 * calls the routine probe() with three arguments, a probe name, a
 * pointer to the buffer containing the data, and the number of bytes
 * in the buffer.  For now, probe() writes to files using fwrite() but
 * in the future, data could be stored anywhere.  More importantly,
 * probe() manages all of the i/o setup and state so the user need not
 * be concerned with opening files and storing pointers to file state.
 * Probe names must be unique since probe() uses the name to find a
 * pointer to file state if it already exists, and otherwise stores
 * the name and along with a new pointer to file state.  The name of
 * the file is a concatenation of argv[0] (the program name), the
 * probe name, and the suffix "_probe.dat".  Storage for probe names
 * is static but the user can minimize storage by setting 
 * PROBE_MAX_SIGNAME_SIZE and PROBE_NSIGNALS at compile time and otherwise,
 * defaults values (set below) will be used.
 ******************************************************************************/

#ifndef _PROBE_H_
#define _PROBE_H_

/* user definable max signal name length */
#ifndef PROBE_MAX_SIGNAME_SIZE
#define PROBE_MAX_SIGNAME_SIZE 64
#endif

/* user definable number of signals to probe */
#ifndef PROBE_NSIGNALS
#define PROBE_NSIGNALS 128
#endif

void probe_init(const char *);
void probe(const char *, void *, uint32);
int  probe_save(const char *, char *);

#endif /* _PROBE_H_ */
