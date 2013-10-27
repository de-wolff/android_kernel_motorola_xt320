/*
 * Windows implementations of Broadcom secure string functions
 *
 * Copyright(c) 2010 Broadcom Corp.
 * $Id: msft_str.h,v 1.1.2.2 2010-09-01 01:28:11 linm Exp $
 */

#ifndef __MSFT_STR_H__
#define __MSFT_STR_H__

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static INLINE int bcm_strcpy_s(
	char *dst,
	size_t noOfElements,
	const char *src)
{
	int ret;

	ret = strcpy_s(dst, noOfElements, src);
	return (int)ret;
}

static INLINE int bcm_strncpy_s(
	char *dst,
	size_t noOfElements,
	const char *src,
	size_t count)
{
	return (int)strncpy_s(dst, noOfElements, src, count);
}

static INLINE int bcm_strcat_s(
	char *dst,
	size_t noOfElements,
	const char *src)
{
	int ret;

	ret = strcat_s(dst, noOfElements, src);
	return (int)ret;
}

extern int vsprintf_s(
		      char *buffer,
		      size_t numberOfElements,
		      const char *format,
		      va_list argptr
);

static INLINE int bcm_sprintf_s(
	char *buffer,
	size_t noOfElements,
	const char *format,
	...)
{
	va_list argptr;
	int ret;

	va_start(argptr, format);
	ret = vsprintf_s(buffer, noOfElements, format, argptr);
	va_end(argptr);
	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* __MSFT_STR_H__ */
