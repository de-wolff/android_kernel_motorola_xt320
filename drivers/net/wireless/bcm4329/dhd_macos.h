/*
 * Abstract DHD Ethernet Controller Class for MacOS
 * This is common to both USB and SDIO
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhd_macos.h,v 1.2 2007-07-10 22:39:52 lut Exp $
 *
 */
#ifndef __DHD_MACOS_H_
#define __DHD_MACOS_H_

#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOGatedOutputQueue.h>

extern "C" {
	#include "osl.h"
	#include "dhdioctl.h"
	#include "wlioctl.h"
	#include "dngl_stats.h"
	#include "dhd.h"
	#include "dhd_proto.h"
}

typedef struct linkitem {
	void *data;
	struct linkitem *next;
} linkitem_t;

typedef struct linklist {
	linkitem_t *head;
	linkitem_t *tail;
} linklist_t;

class DhdEther : public IOEthernetController
{
	OSDeclareAbstractStructors(DhdEther)

protected:
	osl_t			*m_osh;
	dhd_pub_t		*m_dhd;
	IOLock			*m_ioctlLock;
	IOLock			*m_listLock;
	UInt8 			*m_ioctlBuf;

	IOEthernetInterface	*m_etherIf;
	bool			m_etherIfEnab;

	IOGatedOutputQueue	*m_outpQ;
	UInt8			m_outpQSize;
	IONetworkStats		*m_netStats;
	IOEthernetStats		*m_etherStats;

	bool publishEtherMedium(void);
	bool netifAttach();
	bool netifDetach();

	void ll_enq(linklist_t *list, linkitem_t *item);
	linkitem_t* ll_deq(linklist_t *list);
	int ll_cnt(linklist_t *list);
public:

	virtual bool init(OSDictionary *properties, UInt8 outQSize);
	virtual void free();
	virtual bool start(IOService *provider, UInt8 hdrLen = 0);
	virtual void stop(IOService *provider);
	virtual bool terminate(IOOptionBits options);
	virtual void detach(IOService *provider);

	IOReturn wlioctl_hook(wl_ioctl_t *ioc, IOByteCount iocSize);
	IOReturn dhdioctl_hook(dhd_ioctl_t *ioc, IOByteCount iocSize);

	/*
	 * IOEthernetController methods - must implement these
	 */
	virtual IOReturn enable(IONetworkInterface *netif);
	virtual IOReturn disable(IONetworkInterface *netif);
	virtual bool configureInterface(IONetworkInterface *netif);
	virtual IOOutputQueue *createOutputQueue(void);
	virtual IOReturn getPacketFilters(const OSSymbol *group, UInt32 *filters) const;
	virtual IOReturn getHardwareAddress(IOEthernetAddress *addr);

	/*
	 * IOEthernetController methods - helper functions
	 */
	virtual IOReturn setMulticastMode(IOEnetMulticastMode mode);
	virtual IOReturn setMulticastList(IOEthernetAddress *addrs, UInt32 count);
	virtual IOReturn setWakeOnMagicPacket(bool active);
	virtual IOReturn selectMedium(const IONetworkMedium *medium);

};
#endif /* __DHD_MACOS_H_ */
