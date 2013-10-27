/*
 * EFI stdarg file that maps EFI defines
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: stdarg.h,v 1.1 2007-04-27 21:24:19 abhorkar Exp $
 */

#ifndef _STDARG_H_
#define _STDARG_H_

#include "Tiano.h"
typedef VA_LIST va_list;
#define va_start VA_START
#define va_end VA_END
#define va_arg VA_ARG
#endif
