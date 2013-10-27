/*
 * DHD SDIO Ethernet for MacOS
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhd_sdio_macos.cpp,v 1.2 2007-07-10 22:44:35 lut Exp $
 *
 */
#include <IOKit/pci/IOPCIDevice.h>
#include "dhd_sdio_macos.h"

extern "C"
{
	#include "epivers.h"
}

/*
 * MacOS specific
 */
#define super DhdEther
OSDefineMetaClassAndStructors(DhdSdioEther, DhdEther)

/*
 * Static class members
 */
IOPCIDevice* DhdSdioEther::m_pciNub = NULL;

/*
 * Class Members
 */
bool
DhdSdioEther::init(OSDictionary *properties)
{
	if (super::init(properties, sTxQLen) == false)
		goto err;

	m_pciNub = NULL;
	return true;

err:
	/* if init() fails, free() is called automatically */
	return false;
}

void
DhdSdioEther::free()
{
	super::free();
}

bool
DhdSdioEther::start(IOService *provider)
{
	if (!super::start(provider, sSdpcmReserve))
		goto err;

	m_pciNub = OSDynamicCast(IOPCIDevice, provider);
	if (!m_pciNub)
	{
		IOLog("%s(%p) - Provider not PCI device!\n", getName(), this);
		goto err;
	}
	m_pciNub->retain();

	if (netifAttach() == false)
		goto err;

	return true;
err:
	/* if start() fails, detach() is called automatically */
	return false;
}

void
DhdSdioEther::stop(IOService *provider)
{
	if (m_pciNub)
	{
		m_pciNub->release();
		m_pciNub = NULL;
	}

	netifDetach();
	super::stop(provider);
}

bool
DhdSdioEther::terminate(IOOptionBits options)
{
	/* Need terminate to properly cleanup if cable
	 * is disconnected suddenly
	 */
	if (m_pciNub)
	{
		m_pciNub->release();
		m_pciNub = NULL;
	}
	return super::terminate(options);
}

IOReturn
DhdSdioEther::setPromiscuousMode(IOEnetPromiscuousMode mode)
{
	return kIOReturnSuccess;
}

const OSString*
DhdSdioEther::newVendorString() const
{
	return OSString::withCString((const char *)"DHD SDIO Ethernet");
}

const OSString*
DhdSdioEther::newModelString() const
{
	return OSString::withCString("DHD SDIO");
}

const OSString*
DhdSdioEther::newRevisionString() const
{
	return OSString::withCString(EPI_VERSION_STR);
}

IOReturn
DhdSdioEther::getHardwareAddress(IOEthernetAddress *addr)
{
	char buff[6] = { 0x00, 0x90, 0x4c, 0x11, 0x22, 0x33 };

	/* FIX: Implement */
	bcopy(buff, addr, sizeof(*addr));
	return kIOReturnSuccess;
}

IOReturn
DhdSdioEther::enable(IONetworkInterface *netif)
{
	IOReturn ret = kIOReturnSuccess;

	/* FIX: Implement */
	ret = super::enable(netif);
	return kIOReturnSuccess;
}

IOReturn
DhdSdioEther::disable(IONetworkInterface *netif)
{
	/* FIX: Implement */
	return super::disable(netif);
}


UInt32
DhdSdioEther::outputPacket(mbuf_t m, void *param)
{
	/* FIX: Implement */
	PKTFREE(m_osh, m, false);
	return kIOReturnOutputDropped;
}

extern "C" {

int
dhd_bus_txctl(struct dhd_bus *bus, uchar *msg, uint msglen)
{
	/* FIX: Implement */
	return -1;
}

int dhd_bus_rxctl(struct dhd_bus *bus, uchar *msg, uint msglen)
{
	/* FIX: Implement */
	return -1;
}

} /* extern C */
