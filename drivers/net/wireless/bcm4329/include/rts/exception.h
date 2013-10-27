/******************************************************************************
 * $Id: exception.h,v 1.22 2002-08-13 21:04:29 davidm Exp $
 * exception.h - defs for exception handling
 *****************************************************************************/

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "typedefs.h"
#ifdef vxworks
#include "vxWorks.h"
#endif
#include <setjmp.h>

/****************************************************************************** 
 *  Types
 ******************************************************************************/

typedef int exception_code_t;

typedef struct exception_t exception_t;
struct exception_t
{
    exception_code_t	code;
    void*		pdata;
};

typedef struct _exception_frame _exception_frame;
struct _exception_frame {
    exception_t		exception;
    jmp_buf		jumpBuf;
};

/****************************************************************************** 
 *  Constants
 ******************************************************************************/

#define EX_NO_EXCEPTION	0
#define EX_OUT_OF_MEM	1

/****************************************************************************** 
 *  Globals
 ******************************************************************************/

#if defined (PTHREADS_EXCEPTIONS)
extern _exception_frame **thread_exception_frame (void);
#define _current_frame (*thread_exception_frame ())
#else
extern _exception_frame* _current_frame;
#endif

/****************************************************************************** 
 *  Prototypes
 ******************************************************************************/

void throw(exception_code_t exception, void* pdata);

/****************************************************************************** 
 *  Macros
 ******************************************************************************/

/* for prototype documentation */
#define THROWS(exception)

#define rethrow(e)  throw(e.code, e.pdata)

#define current_exception() _try_frame.exception

#define _try_push_frame(pframe)	\
	_try_previous_frame	= _current_frame;	\
	_current_frame	= pframe

#define _try_pop_frame()	\
	_current_frame	= _try_previous_frame

#define try \
	{ \
	_exception_frame	_try_frame; \
	_exception_frame*	_try_previous_frame;\
	int _try_caught; \
	_try_frame.exception.code	= EX_NO_EXCEPTION; \
	_try_frame.exception.pdata	= NULL; \
	_try_push_frame(&_try_frame); \
	if (setjmp(_try_frame.jumpBuf) == 0) { \
		_try_caught = 0;

#define catch(_catch_exception) \
	} else { \
		exception_t _catch_exception  = _current_frame->exception;\
		_try_pop_frame(); \
		_try_caught = 1;

#define end_try \
	} \
		if (_try_caught == 0)	\
			_try_pop_frame();	\
	}

#define unwind_protect \
	{ \
	exception_code_t _unwind_code;	\
	void* _unwind_data;				\
	int _unwind_rethrow;			\
	try { \
		_unwind_rethrow = 0;		\
		_unwind_code = EX_NO_EXCEPTION;\
		_unwind_data = NULL;

#define on_unwind \
	} catch(_unwind_exception) {	\
		_unwind_rethrow = 1;		\
		_unwind_code	= _unwind_exception.code;	\
		_unwind_data	= _unwind_exception.pdata;	\
		} \
	end_try

#define end_unwind \
	if (_unwind_rethrow == 1) \
		throw(_unwind_code, _unwind_data); \
	}

#endif /* _EXCEPTION_H_ */
