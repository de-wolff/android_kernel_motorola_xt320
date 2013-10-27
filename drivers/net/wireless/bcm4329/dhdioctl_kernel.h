/*
 * DHD Ioctl Interface via IOUserClient
 * This is kernel component for MacOS
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhdioctl_kernel.h,v 1.2 2007-07-10 22:39:53 lut Exp $
 *
 */
#ifndef __DHDIOCTL_KERNEL_H_
#define __DHDIOCTL_KERNEL_H_

#include <IOKit/IOUserClient.h>

class dhdioctl_kernel : public IOUserClient
{
	OSDeclareDefaultStructors(dhdioctl_kernel)

protected:
	IOService	*m_Provider;

public:

	/*
	 * IOUserClient methods
	 */
	virtual IOReturn  open(void);
	virtual IOReturn  close(void);
	virtual void stop(IOService * provider);
	virtual bool start(IOService * provider);

	virtual IOReturn message(UInt32 type, IOService * provider,  void * argument);
	virtual bool initWithTask(task_t owningTask, void * security_id, UInt32 type);
	virtual bool finalize(IOOptionBits options);
	virtual IOExternalMethod * getTargetAndMethodForIndex(IOService ** target, UInt32 index);
	virtual IOReturn clientClose(void);
	virtual IOReturn clientDied(void);
	virtual bool terminate(IOOptionBits options = 0);
};
#endif /* __DHDIOCTL_KERNEL_H_ */
