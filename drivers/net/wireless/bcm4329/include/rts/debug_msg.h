/******************************************************************************
 * $Id: debug_msg.h,v 2.11 1999-08-19 05:00:43 cast Exp $
 * debug_msg.h - prototypes for message handling code
 *****************************************************************************/

#ifndef _DEBUG_MSG_H_
#define _DEBUG_MSG_H_

#include <stdarg.h>

/*
 * Running as daemon:	yes				no
 * fatal, error,...	issue a syslog			write to stderr
 * message		put into circular buffer	write to stderr
 */

extern void fatal(const char *fmtmsg, ...);
extern void error(const char *fmtmsg, ...);
extern void exception(const char *fmtmsg, ...);
#if !defined(TARGETENV_freebsd)
extern void warn(const char *fmtmsg, ...);
#endif /* !(TARGETENV_freebsd) */
extern void info(const char *fmtmsg, ...);

extern void message(const char *fmtmsg, ...);
extern void vmessage(const char *fmtmsg, va_list arqs);

/* 
 * message macros: 3 levels of messaging (0,1,2), with level 0 being the
 * highest priority
 */

/* generic MESSAGE macro */
#define MESSAGE(level, args)	( MESSAGE##level (args) )

#if defined(MSG0)
/* highest-priority messages only */
#define MESSAGE0(args)	( message args )
#define MESSAGE1(args)	((void)0)
#define MESSAGE2(args)	((void)0)
#else
#if defined(MSG1)
/* high and medium priority messages only */
#define MESSAGE0(args)	( message args )
#define MESSAGE1(args)	( message args )
#define MESSAGE2(args)	((void)0)
#else
#if defined(MSG2)
/* all messages */
#define MESSAGE0(args)	( message args )
#define MESSAGE1(args)	( message args )
#define MESSAGE2(args)	( message args )
#else
/* no messages */
#define MESSAGE0(args)	((void)0)
#define MESSAGE1(args)	((void)0)
#define MESSAGE2(args)	((void)0)
#endif /* MSG2 */
#endif /* MSG1 */
#endif /* MSG0 */


/* fixup for win32 */
#ifdef _WIN32

#if !defined(RING0)
#include <string.h>
#define _STRERROR() strerror(0)
#endif /* !defined(RING0) */

#else /* !_WIN32 */

extern char *strerror(int);
#if !defined(errno)
extern int errno;
#endif /* !defined(errno) */
#define _STRERROR() strerror(errno)

#endif /* _WIN32 */

#define STRERROR _STRERROR()

#endif /* _DEBUG_MSG_H_ */
