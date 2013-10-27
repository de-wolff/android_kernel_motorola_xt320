/*
 * DHD Ioctl Interface via IOUserClient
 * This is kernel component for MacOS
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhdioctl_kernel.cpp,v 1.2 2007-07-10 22:39:52 lut Exp $
 *
 */
#include "dhd_macos.h"
#include "dhdioctl_usr.h"
#include "dhdioctl_kernel.h"

#define super IOUserClient
OSDefineMetaClassAndStructors(dhdioctl_kernel, IOUserClient)

IOExternalMethod *
dhdioctl_kernel::getTargetAndMethodForIndex(IOService **target, UInt32 index)
{
	static const IOExternalMethod UserFuncTable[kNumberOfMethods] = {
		{   /* kFuncIndex_open */
			NULL,
			(IOMethod) &dhdioctl_kernel::open,
			kIOUCScalarIScalarO, /* Scalar Input, Scalar Output. */
			0, /* No scalar input values. */
			0 /* No scalar output values. */
		},
		{   /* kFuncIndex_close */
			NULL,
			(IOMethod) &dhdioctl_kernel::close,
			kIOUCScalarIScalarO,
			0,
			0
		},
		{   /* kFuncIndex_wlioctl_hook */
			NULL,
			(IOMethod) &DhdEther::wlioctl_hook,
			kIOUCScalarIStructI,
			0,
			sizeof(wl_ioctl_t)
		},
		{   /* kFuncIndex_dhdioctl_hook */
			NULL,
			(IOMethod) &DhdEther::dhdioctl_hook,
			kIOUCScalarIStructI,
			0,
			sizeof(dhd_ioctl_t)
		}
	};

	if (index < (UInt32)kNumberOfMethods)
	{
		if (index == kFuncIndex_open || index == kFuncIndex_close)
			*target = this;
		else
			*target = m_Provider;

		return (IOExternalMethod *) &UserFuncTable[index];
	}

	return NULL;
}

bool
dhdioctl_kernel::initWithTask(task_t owningTask, void *security_id, UInt32 type)
{
	if (!super::initWithTask(owningTask, security_id, type))
		return false;

	if (!owningTask)
		return false;

	m_Provider = NULL;

	return true;
}

bool
dhdioctl_kernel::start(IOService *provider)
{
	if (!super::start(provider))
		return false;

	m_Provider = OSDynamicCast(DhdEther, provider);

	if (!m_Provider)
		return false;

	return true;
}

void
dhdioctl_kernel::stop(IOService *provider)
{
	super::stop(provider);
}

IOReturn
dhdioctl_kernel::open(void)
{
	if (!m_Provider)
		return kIOReturnNotAttached;

	if (!m_Provider->open(this))
		return kIOReturnExclusiveAccess;

	return kIOReturnSuccess;
}

IOReturn
dhdioctl_kernel::close(void)
{
	if (!m_Provider)
		return kIOReturnNotAttached;

	if (m_Provider->isOpen(this))
		m_Provider->close(this);

	return kIOReturnSuccess;
}

IOReturn
dhdioctl_kernel::clientClose(void)
{
	close();

	m_Provider = NULL;
	terminate();

	return kIOReturnSuccess;
}

IOReturn
dhdioctl_kernel::clientDied(void)
{
	return super::clientDied();
}

IOReturn
dhdioctl_kernel::message(UInt32 type, IOService *provider,  void *argument)
{
	return super::message(type, provider, argument);
}

bool
dhdioctl_kernel::finalize(IOOptionBits options)
{
	return super::finalize(options);
}

bool
dhdioctl_kernel::terminate(IOOptionBits options)
{
	return super::terminate(options);
}
