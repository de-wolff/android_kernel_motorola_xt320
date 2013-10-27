/*
 * Safe string functions, only windows version is fully implemented
 *
 * $Copyright Open Broadcom Corporation$
 * $Id: bcmsafestr.h,v 1.1.2.3 2010-09-01 01:39:11 linm Exp $
 */

#if defined(NDIS) || defined(WINDOWS)
#include <msft_str.h>
#else
#include <nonms_str.h>
#endif
