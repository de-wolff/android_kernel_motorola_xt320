/*
 * Abstract DHD Ethernet Controller Class for MacOS
 * This is common to both USB and SDIO
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhd_macos.cpp,v 1.4 2009-04-09 19:02:26 csm Exp $
 *
 */
#include "dhd_macos.h"

/*
 * MacOS specific
 */
#define super IOEthernetController
OSDefineMetaClassAndAbstractStructors(DhdEther, IOEthernetController)

/*
 * Class Members
 */
bool
DhdEther::init(OSDictionary *properties, UInt8 outQSize)
{
	if (super::init(properties) == false)
		return false;

	m_outpQSize = outQSize;
	m_outpQ = NULL;
	m_etherIf = NULL;
	m_osh = NULL;
	m_dhd = NULL;
	m_ioctlBuf = NULL;
	m_ioctlLock = NULL;
	m_listLock = NULL;

	m_ioctlLock = IOLockAlloc();
	if (!m_ioctlLock)
		goto err;

	m_listLock = IOLockAlloc();
	if (!m_listLock)
		goto err;

	return true;

err:
	return false;
}

void
DhdEther::free()
{
	if (m_ioctlLock) {
		IOLockFree(m_ioctlLock);
		m_ioctlLock = NULL;
	}

	if (m_listLock) {
		IOLockFree(m_listLock);
		m_listLock = NULL;
	}

	super::free();
}

bool
DhdEther::start(IOService *provider, UInt8 hdrLen)
{
	if (!super::start(provider))
		goto err;

	m_outpQ = (IOGatedOutputQueue *)getOutputQueue();
	if (!m_outpQ)
		goto err;
	m_outpQ->retain();

	m_osh = dhd_osl_attach(provider, (uint)-1);
	if (m_osh == NULL)
		goto err;

	m_dhd = dhd_attach(m_osh, (struct dhd_bus *)this, hdrLen);
	if (m_dhd == NULL)
		goto err;

	m_ioctlBuf = (UInt8 *) MALLOC(m_osh, WLC_IOCTL_MAXLEN);
	if (m_ioctlBuf == NULL)
		goto err;

	return true;
err:
	/* if start() fails, detach() is called automatically */
	return false;

}

void
DhdEther::stop(IOService *provider)
{
	if (m_outpQ) {
		m_outpQ->release();
		m_outpQ = NULL;
	}

	if (m_ioctlBuf) {
		MFREE(m_osh, m_ioctlBuf, WLC_IOCTL_MAXLEN);
		m_ioctlBuf = NULL;
	}

	if (m_dhd) {
		dhd_detach(m_dhd);
		m_dhd = NULL;
	}

	if (m_osh) {
		dhd_osl_detach(m_osh);
		m_osh = NULL;
	}

	super::stop(provider);
}

bool
DhdEther::terminate(IOOptionBits options)
{
	netifDetach();
	return super::terminate(options);
}

void
DhdEther::detach(IOService *provider)
{
	/*
	 * Need detach() if start() fails to properly cleanup
	 * and be able to unload driver
	 * Be careful because detach() may be called multiple times
	 */
	stop(provider);
	super::detach(provider);
}

bool
DhdEther::netifAttach()
{
	if (m_etherIf)
		return false;

	if (!attachInterface((IONetworkInterface **)&m_etherIf, false))
		return false;

	m_etherIf->registerService();

	/* publish for ioctl user clients */
	registerService();

	return true;
}

bool
DhdEther::netifDetach()
{
	if (m_etherIf) {
		detachInterface((IONetworkInterface *)m_etherIf);
		m_etherIf->release();
		m_etherIf = NULL;
	}

	return true;
}

IOOutputQueue*
DhdEther::createOutputQueue()
{
	return IOBasicOutputQueue::withTarget(this, m_outpQSize);
}

IOReturn
DhdEther::setMulticastMode(IOEnetMulticastMode mode)
{
	return kIOReturnSuccess;
}

IOReturn
DhdEther::setMulticastList(IOEthernetAddress *addrs, UInt32 count)
{
	return kIOReturnSuccess;
}

IOReturn
DhdEther::setWakeOnMagicPacket(bool active)
{
	return kIOReturnSuccess;
}

IOReturn
DhdEther::selectMedium(const IONetworkMedium *medium)
{
	return kIOReturnSuccess;
}

IOReturn
DhdEther::getPacketFilters(const OSSymbol *group, UInt32 *filters) const
{
	IOReturn rtn = kIOReturnSuccess;

	if (group == gIONetworkFilterGroup) {
		*filters = kIOPacketFilterUnicast | kIOPacketFilterBroadcast |
			kIOPacketFilterMulticast | kIOPacketFilterMulticastAll |
			kIOPacketFilterPromiscuous;
	} else {
		rtn = super::getPacketFilters(group, filters);
	}

	return rtn;
}

IOReturn
DhdEther::getHardwareAddress(IOEthernetAddress *addr)
{
	wl_ioctl_t ioc;
	char buff[] = "cur_etheraddr";
	IOReturn err;

	ioc.cmd = WLC_GET_VAR;
	ioc.buf = buff;
	ioc.len = sizeof(buff);
	ioc.set = false;

	err = dhd_prot_ioctl(m_dhd, &ioc, buff, sizeof(buff));
	if (err)
		return kIOReturnError;

	bcopy(buff, addr, sizeof(*addr));
	return kIOReturnSuccess;
}

IOReturn
DhdEther::wlioctl_hook(wl_ioctl_t *ioc, IOByteCount iocSize)
{
	IOReturn err = kIOReturnSuccess;

	IOLockLock(m_ioctlLock);
	if (ioc->len > WLC_IOCTL_MAXLEN) {
		ioc->len = WLC_IOCTL_MAXLEN;
	}

	err = copyin((user_addr_t)ioc->buf, m_ioctlBuf, ioc->len);
	if (err) {
		IOLog("DhdEther::wlioctl_hook: copin err=%d\n", err);
		err = kIOReturnError;
		goto err;
	}

	err = dhd_prot_ioctl(m_dhd, ioc, m_ioctlBuf, ioc->len);
	if (err) {
		IOLog("dhd_prot_ioctl failed=%d\n", err);
		err = kIOReturnError;
		goto err;
	}

	err = copyout(m_ioctlBuf, (user_addr_t)ioc->buf, ioc->len);
	if (err) {
		IOLog("DhdEther::wlioctl_hook: copyout err=%d\n", err);
		err = kIOReturnError;
		goto err;
	}

err:
	IOLockUnlock(m_ioctlLock);
	return err;
}

IOReturn
DhdEther::dhdioctl_hook(dhd_ioctl_t *ioc, IOByteCount iocSize)
{
	IOReturn err = kIOReturnSuccess;
	void *usrbuf;

	IOLockLock(m_ioctlLock);
	if (ioc->driver == DHD_IOCTL_MAGIC) {
		err = copyin((user_addr_t)ioc->buf, m_ioctlBuf, ioc->len);
		usrbuf = ioc->buf;
		if (err) {
			IOLog("DhdEther::dhdioctl_hook: copin err=%d\n", err);
			err = kIOReturnError;
			goto err;
		}

		ioc->buf = m_ioctlBuf; /* in case kernel attempts to access user-space buf ptr */
		err = dhd_ioctl(m_dhd, ioc, m_ioctlBuf, ioc->len);
		if (err) {
			IOLog("dhd_ioctl failed=%d\n", err);
			err = kIOReturnError;
			goto err;
		}

		ioc->buf = usrbuf;
		err = copyout(m_ioctlBuf, (user_addr_t)ioc->buf, ioc->len);
		if (err) {
			IOLog("DhdEther::dhdioctl_hook: copout err=%d\n", err);
			err = kIOReturnError;
			goto err;
		}
	}

err:
	IOLockUnlock(m_ioctlLock);
	return err;
}

IOReturn
DhdEther::enable(IONetworkInterface *netif)
{
	if (m_etherIfEnab == true)
		return kIOReturnSuccess;

	if (publishEtherMedium() == false)
		return kIOReturnError;

	m_outpQ->setCapacity(m_outpQSize);
	m_outpQ->start();
	m_etherIfEnab = true;

	return kIOReturnSuccess;
}

IOReturn
DhdEther::disable(IONetworkInterface *netif)
{
	if (m_etherIfEnab == false)
		return kIOReturnSuccess;

	m_outpQ->setCapacity(0);
	m_outpQ->stop();
	m_outpQ->flush();
	m_etherIfEnab = false;

	setLinkStatus(0, 0);
	return kIOReturnSuccess;
}

bool
DhdEther::configureInterface(IONetworkInterface *netif)
{
	if (super::configureInterface(netif) == false)
		return false;

	/* Get network statistics objects */
	IONetworkParameter *param = netif->getParameter(kIONetworkStatsKey);
	if (!param || !(m_netStats = (IONetworkStats *) param->getBuffer()))
		return false;

	param = netif->getParameter(kIOEthernetStatsKey);
	if (!param || !(m_etherStats = (IOEthernetStats *) param->getBuffer()))
		return false;

	m_netStats->outputPackets = 0;
	m_netStats->outputErrors = 0;
	m_netStats->inputPackets = 0;
	m_netStats->inputErrors = 0;

	return true;
}

bool
DhdEther::publishEtherMedium()
{
	OSDictionary *mediumDict = 0;
	IONetworkMedium	*medium;

	mediumDict = OSDictionary::withCapacity(1);
	if (!mediumDict)
		return false;

	medium = IONetworkMedium::medium(kIOMediumEthernetAuto, 0);
	IONetworkMedium::addMedium(mediumDict, medium);
	medium->release();

	if (publishMediumDictionary(mediumDict) != true)
		return false;

	medium = IONetworkMedium::getMediumWithType(mediumDict, kIOMediumEthernetAuto);
	setCurrentMedium(medium);
	setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium, 100 * 1000000);

	mediumDict->release();
	mediumDict = 0;
	return true;
}

void
DhdEther::ll_enq(linklist_t *list, linkitem_t *item)
{
	if (m_listLock == NULL)
		return;

	IOLockLock(m_listLock);
	item->next = NULL;
	if (list->tail) {
		list->tail->next = item;
		list->tail = item;
	}
	else
		list->head = list->tail = item;
	IOLockUnlock(m_listLock);
}

linkitem_t *
DhdEther::ll_deq(linklist_t *list)
{
	linkitem_t *item;

	if (m_listLock == NULL)
		return NULL;

	IOLockLock(m_listLock);
	item = list->head;
	if (item) {
		list->head = list->head->next;
		item->next = NULL;

		if (list->head == NULL)
			list->tail = list->head;
	}
	IOLockUnlock(m_listLock);
	return item;
}

int
DhdEther::ll_cnt(linklist_t *list)
{
	linkitem_t *item;
	int cnt = 0;

	if (m_listLock == NULL)
		return 0;

	IOLockLock(m_listLock);
	for (item = list->head; item; item = item->next)
		cnt++;
	IOLockUnlock(m_listLock);

	return cnt;
}

extern "C" {

dhd_pub_t *
dhd_attach(osl_t *osh, struct dhd_bus *bus, uint bus_hdrlen)
{
	dhd_pub_t *dhd;

	if (!(dhd = (dhd_pub_t *) MALLOC(osh, sizeof(dhd_pub_t))))
		return NULL;

	bzero(dhd, sizeof(dhd_pub_t));
	dhd->osh = osh;

	/* Link to bus module */
	dhd->bus = bus;
	dhd->hdrlen = bus_hdrlen;

	/* Attach and link in the protocol */
	if (dhd_prot_attach(dhd) != 0) {
		goto fail;
	}

	return (dhd_pub_t *)dhd;

fail:
	if (dhd) {
		if (dhd->prot)
			dhd_prot_detach(dhd);
	}

	MFREE(osh, dhd, sizeof(dhd_pub_t));
	return NULL;
}

void
dhd_detach(dhd_pub_t *dhdp)
{
	dhd_pub_t *dhd = (dhd_pub_t *)dhdp;
	if (dhd) {
		if (dhdp->prot)
			dhd_prot_detach(dhdp);

		MFREE(dhdp->osh, dhd, sizeof(dhd_pub_t));
	}
}

osl_t *
dhd_osl_attach(void *pdev, uint bustype)
{
	return osl_attach((IOPCIDevice *)pdev);
}

void
dhd_osl_detach(osl_t *osh)
{
	osl_detach(osh);
}

/*
 * Generalized timeout mechanism.  Uses spin sleep with exponential back-off.  Usage:
 *
 *      dhd_timeout_start(&tmo, usec);
 *      while (!dhd_timeout_expired(&tmo))
 *              if (poll_something())
 *                      break;
 *      if (dhd_timeout_expired(&tmo))
 *              fatal();
 */

void
dhd_timeout_start(dhd_timeout_t *tmo, uint usec)
{
	tmo->limit = usec;
	tmo->increment = 0;
	tmo->elapsed = 0;
	tmo->tick = 10000;
}

int
dhd_timeout_expired(dhd_timeout_t *tmo)
{
	/* Does nothing the first call */
	if (tmo->increment == 0) {
		tmo->increment = 1;
		return 0;
	}

	if (tmo->elapsed >= tmo->limit)
		return 1;

	/* Add the delay that's about to take place */
	tmo->elapsed += tmo->increment;

	/* XXX TBD: switch to task tick when increment gets large enough (see dhd_linux.c) */

	OSL_DELAY(tmo->increment);
	tmo->increment *= 2;
	if (tmo->increment > tmo->tick)
		tmo->increment = tmo->tick;

	return 0;
}

/*
 * OS specific functions required to implement DHD driver in OS independent way
 */
int
dhd_os_proto_block(dhd_pub_t * pub)
{
	return 0;
}

int
dhd_os_proto_unblock(dhd_pub_t * pub)
{
	return 0;
}

void
dhd_bus_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
}

void
dhd_bus_clearcounts(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
}

int
dhd_bus_iovar_op(dhd_pub_t *dhdp, const char *name,
	void *params, int plen, void *arg, int len, bool set)
{
	return 0;
}

void
dhd_os_wd_timer(void *bus, uint wdtick)
{
}

uint dhd_watchdog_ms = 0;
} /* extern C */
