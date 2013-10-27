/*
 * Global ASSERT Logging Public Interface
 *
 * $Copyright (C) 2006-2007 Broadcom Corporation$
 *
 * $Id: bcm_assert_log.h,v 13.2 2009-07-28 02:12:28 mthawani Exp $
 */
#ifndef _WLC_ASSERT_LOG_H_
#define _WLC_ASSERT_LOG_H_

#include "wlioctl.h"

typedef struct bcm_assert_info bcm_assert_info_t;

extern void bcm_assertlog_init(void);
extern void bcm_assertlog_deinit(void);
extern int bcm_assertlog_get(void *outbuf, int iobuf_len);

extern void bcm_assert_log(char *str);

#endif /* _WLC_ASSERT_LOG_H_ */
