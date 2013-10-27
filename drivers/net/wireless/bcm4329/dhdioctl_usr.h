/*
 * DHD ioctl interface from user to kernel space.
 * User space application link against this to interface to
 * MacOS DHD driver.
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhdioctl_usr.h,v 1.2 2007-07-10 22:39:53 lut Exp $
 *
 */

#ifndef __DHDIOCTL_USR_H_
#define __DHDIOCTL_USR_H_

/* MacOS DHD driver ioctl interface
 * This uses MacOS's IOUserClient object to enable user apps to
 * interface to kernel drivers
 */
#define kBrcmDhdDriverClassName         "DhdEther"
enum
{
	kFuncIndex_open,
	kFuncIndex_close,
	kFuncIndex_wlioctl_hook,
	kFuncIndex_dhdioctl_hook,
	kNumberOfMethods
};

#ifndef MACOS_DHD_DONGLE
#define dhdioctl_usr_dongle(a, b, c, d, e, f) -1
#else
extern int dhdioctl_usr_dongle(void *dhd, int cmd, void *buf, int len, bool set, bool wl);
#endif

#endif /* __DHDIOCTL_USR_H_ */
