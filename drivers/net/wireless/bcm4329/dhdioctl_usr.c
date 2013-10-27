/*
 * DHD ioctl interface from user to kernel space.
 * User space application link against this to interface to
 * MacOS DHD driver.
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhdioctl_usr.c,v 1.2.486.1 2010-12-22 23:47:24 hharte Exp $
 *
 */
#ifdef KERNEL
#error Error: This file is used in user-space apps only.
#endif

#include <IOKit/IOKitLib.h>
#include <ApplicationServices/ApplicationServices.h>

#undef MACOSX
#define TYPEDEF_BOOL
#define TYPEDEF_FLOAT_T
#include <wlioctl.h>
#include <dhdioctl.h>
#include "dhdioctl_usr.h"

static io_connect_t
dhdioctl_usr_open()
{
	kern_return_t       kret;
	io_service_t        svcObj;
	io_connect_t        dataPort;
	io_iterator_t       iterator;
	CFDictionaryRef     classToMatch;

	classToMatch = IOServiceMatching(kBrcmDhdDriverClassName);
	if (classToMatch == NULL) {
		printf("IOServiceMatching failed for class \"%s\"\n", kBrcmDhdDriverClassName);
		goto err;
	}

	kret = IOServiceGetMatchingServices(kIOMasterPortDefault, classToMatch, &iterator);
	if (kret != KERN_SUCCESS) {
		printf("IOServiceGetMatchingServices returned %d\n\n", kret);
		goto err;
	}

	svcObj = IOIteratorNext(iterator);
	IOObjectRelease(iterator);
	if (!svcObj) {
		printf("No matching kext driver class \"%s\"\n", kBrcmDhdDriverClassName);
		goto err;
	}

	kret = IOServiceOpen(svcObj, mach_task_self(), 0, &dataPort);
	IOObjectRelease(svcObj);
	if (kret != KERN_SUCCESS) {
		printf("IOServiceOpen failed=%d\n", kret);
		goto err;
	}

	kret = IOConnectMethodScalarIScalarO(dataPort, kFuncIndex_open, 0, 0);
	if (kret != KERN_SUCCESS) {
		IOServiceClose(dataPort);
		goto err;
	}

	return dataPort;

err:
	return 0;
}

static int
dhdioctl_usr_close(io_connect_t dataPort)
{
	kern_return_t	kret;

	if (dataPort == 0)
		goto err;

	kret = IOConnectMethodScalarIScalarO(dataPort, kFuncIndex_close, 0, 0);
	if (kret != KERN_SUCCESS) {
		printf("close() failed=%d\n", kret);
		goto err;
	}

	kret = IOServiceClose(dataPort);
	if (kret != KERN_SUCCESS) {
		printf("IOServiceClose failed=%d\n", kret);
		goto err;
	}

	return 0;
err:
	return -1;
}

int
dhdioctl_usr_dongle(void *dhd, int cmd, void *buf, int len, bool set, bool wl)
{
	kern_return_t	kret;
	io_connect_t	dataPort;
	dhd_ioctl_t	dhdioc;
	IOByteCount	dhdioc_size = sizeof(dhd_ioctl_t);
	wl_ioctl_t	wlioc;
	IOByteCount	wlioc_size = sizeof(wl_ioctl_t);
	IOByteCount	ioc_size;
	int		funcIndex;
	void		*ioc;

	dataPort = dhdioctl_usr_open();
	if (dataPort == 0) {
		printf("dhdusr_ioctl_open failed!\n");
		goto err;
	}

	if (wl == true) {
		wlioc.cmd = cmd;
		wlioc.buf = buf;
		wlioc.len = len;
#if defined(BCMINTERNAL) && defined(DONGLEOVERLAYS)
		wlioc.action = set ? WL_IOCTL_ACTION_SET : WL_IOCTL_ACTION_GET;
#else
		wlioc.set = set;
#endif /* defined(BCMINTERNAL) && defined(DONGLEOVERLAYS) */
		ioc_size = wlioc_size;
		ioc = &wlioc;
		funcIndex = kFuncIndex_wlioctl_hook;
	} else {
		dhdioc.cmd = cmd;
		dhdioc.buf = buf;
		dhdioc.len = len;
		dhdioc.set = set;
		dhdioc.driver = DHD_IOCTL_MAGIC;
		ioc_size = dhdioc_size;
		ioc = &dhdioc;
		funcIndex = kFuncIndex_dhdioctl_hook;
	}

	kret = IOConnectMethodScalarIStructureI(dataPort,
		funcIndex,
		0,
		ioc_size,
		ioc);

	if (kret != kIOReturnSuccess) {
		printf("wl_ioctl returned %d\n\n", kret);
		goto err;
	}

	dhdioctl_usr_close(dataPort);
	return 0;
err:
	return -1;
}
