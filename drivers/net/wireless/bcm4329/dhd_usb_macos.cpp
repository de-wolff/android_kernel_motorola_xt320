/*
 * DHD USB Ethernet for MacOS
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhd_usb_macos.cpp,v 1.2 2007-07-10 22:39:52 lut Exp $
 *
 */
#include <IOKit/usb/IOUSBInterface.h>
#include "dhd_usb_macos.h"

extern "C"
{
	#include "epivers.h"
}

/*
 * MacOS specific
 */
#define super DhdEther
OSDefineMetaClassAndStructors(DhdUsbEther, DhdEther)

/*
 * Static class members
 */
bool DhdUsbEther::m_intEvt = false;
IOUSBDevice* DhdUsbEther::m_usbNub = NULL;
IOLock* DhdUsbEther::m_intLock = NULL;
IOUSBCompletion DhdUsbEther::m_cntlCmplt;

/*
 * Class Members
 */
bool
DhdUsbEther::init(OSDictionary *properties)
{
	IOBufferMemoryDescriptor *memDesc;

	if (super::init(properties, sTxQLen) == false)
		goto err;

	m_usbIf = NULL;
	m_rxPipe = NULL;
	m_txPipe = NULL;
	m_intPipe = NULL;
	m_intDesc = NULL;
	m_txList.head = m_txList.tail = NULL;
	bzero(m_txDescQ, sizeof(m_txDescQ));
	bzero(m_rxDescQ, sizeof(m_rxDescQ));

	m_intLock = IOLockAlloc();
	if (!m_intLock)
		goto err;

	m_intDesc = IOBufferMemoryDescriptor::withCapacity(PAGE_SIZE, kIODirectionIn);
	if (!m_intDesc)
		goto err;

	for (int i = 0; i < sTxQLen; i++) {
		memDesc = IOBufferMemoryDescriptor::withCapacity(PAGE_SIZE, kIODirectionIn);
		if (!memDesc)
			goto err;
		m_txDescQ[i].data = (void *)memDesc;
		m_txDescQ[i].next = NULL;
		ll_enq(&m_txList, &m_txDescQ[i]);
	}

	for (int i = 0; i < sRxQLen; i++) {
		memDesc = IOBufferMemoryDescriptor::withCapacity(PAGE_SIZE, kIODirectionIn);
		if (!memDesc)
			goto err;
		m_rxDescQ[i] = memDesc;
	}

	return true;

err:
	/* if init() fails, free() is called automatically */
	return false;
}

void
DhdUsbEther::free()
{
	linkitem_t *item;

	if (m_intDesc) {
		m_intDesc->release();
		m_intDesc = NULL;
	}

	if (m_intLock) {
		IOLockFree(m_intLock);
		m_intLock = NULL;
	}

	while (item = ll_deq(&m_txList)) {
		IOBufferMemoryDescriptor *desc = (IOBufferMemoryDescriptor *)item->data;
		desc->release();
		item->data = NULL;
		item->next = NULL;
	}
	m_txList.head = m_txList.tail = NULL;

	/* Sanity check for m_txList */
	for (int i = 0; i < sTxQLen; i++) {
		IOBufferMemoryDescriptor *desc = (IOBufferMemoryDescriptor *)m_txDescQ[i].data;

		if (desc) {
			IOLog("Missed tx desc=%p!\n", desc);
			desc->release();
		}
		m_txDescQ[i].data = NULL;
		m_txDescQ[i].next = NULL;
	}

	for (int i = 0; i < sRxQLen; i++) {
		if (m_rxDescQ[i]) {
			m_rxDescQ[i]->release();
			m_rxDescQ[i] = NULL;
		}
	}

	super::free();
}

bool
DhdUsbEther::start(IOService *provider)
{
	IOUSBFindInterfaceRequest ifFindReq;
	IOUSBFindEndpointRequest epFindReq;
	IOReturn err;
	const IOUSBConfigurationDescriptor *cfgdesc;

	if (!super::start(provider))
		goto err;

	m_usbNub = OSDynamicCast(IOUSBDevice, provider);
	if (!m_usbNub) {
		IOLog("%s(%p) - provider not USB device!\n", getName(), this);
		goto err;
	}

	if (m_usbNub->GetNumConfigurations() < 1)
		goto err;

	cfgdesc = m_usbNub->GetFullConfigurationDescriptor(0);
	if (!cfgdesc)
		goto err;

	if (!m_usbNub->open(this))
		goto err;
	m_usbNub->retain();

	err = m_usbNub->SetConfiguration(this, cfgdesc->bConfigurationValue, true);
	if (err)
		goto err;

	ifFindReq.bInterfaceClass = kUSBVendorSpecificClass;
	ifFindReq.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	ifFindReq.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	ifFindReq.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	m_usbIf = m_usbNub->FindNextInterface(NULL, &ifFindReq);
	if (!m_usbIf)
		goto err;

	if (!m_usbIf->open(this))
		goto err;
	m_usbIf->retain();

	epFindReq.type = kUSBInterrupt;
	epFindReq.direction = kUSBIn;
	m_intPipe = m_usbIf->FindNextPipe(NULL, &epFindReq);
	if (!m_intPipe)
		goto err;

	m_intCmplt.target = this;
	m_intCmplt.action = intrCmpltCb;
	m_intCmplt.parameter = m_intDesc;

	m_intPipe->Read(m_intDesc, 0, 0, m_intDesc->getLength(), &m_intCmplt);
	m_intPipeActive = true;

	m_cntlCmplt.target = this;
	m_cntlCmplt.action = cntlCmpltCb;
	m_cntlCmplt.parameter = NULL;

	epFindReq.type = kUSBBulk;
	epFindReq.direction = kUSBIn;
	m_rxPipe = m_usbIf->FindNextPipe(NULL, &epFindReq);
	if (!m_rxPipe)
		goto err;

	m_rxMPS = epFindReq.maxPacketSize;

	m_rxCmplt.target = this;
	m_rxCmplt.action = rxCmpltCb;
	m_rxCmplt.parameter = NULL;

	epFindReq.type = kUSBBulk;
	epFindReq.direction = kUSBOut;
	m_txPipe = m_usbIf->FindNextPipe(NULL, &epFindReq);
	if (!m_txPipe)
		goto err;

	m_txMPS = epFindReq.maxPacketSize;

	m_txCmplt.target = this;
	m_txCmplt.action = txCmpltCb;
	m_txCmplt.parameter = NULL;

	if (netifAttach() == false)
		goto err;

	return true;

err:
	/* if start() fails, detach() is called automatically */
	return false;
}

void
DhdUsbEther::stop(IOService *provider)
{
	if (m_usbNub) {
		m_usbNub->close(this);
		m_usbNub->release();
		m_usbNub = NULL;
	}

	if (m_usbIf) {
		m_usbIf->close(this);
		m_usbIf->release();
		m_usbIf = NULL;
	}

	netifDetach();

	super::stop(provider);
}

bool
DhdUsbEther::terminate(IOOptionBits options)
{
	/* Need terminate to properly cleanup if cable
	 * is disconnected suddenly
	 */
	if (m_usbNub) {
		m_usbNub->close(this);
		m_usbNub->release();
		m_usbNub = NULL;
	}

	return super::terminate(options);
}

IOReturn
DhdUsbEther::setPromiscuousMode(IOEnetPromiscuousMode mode)
{
	wl_ioctl_t ioc;
	int err, val = false;

	ioc.cmd = WLC_SET_PROMISC;
	ioc.buf = &val;
	ioc.len = sizeof(val);
	ioc.set = true;

	if ((mode == kIOEnetPromiscuousModeOn) || (mode == kIOEnetPromiscuousModeAll))
		val = true;

	err = dhd_prot_ioctl(m_dhd, &ioc, &val, sizeof(val));
	if (err)
		return kIOReturnError;

	return kIOReturnSuccess;
}

const OSString*
DhdUsbEther::newVendorString() const
{
	return OSString::withCString((const char *)"DHD USB Ethernet");
}

const OSString*
DhdUsbEther::newModelString() const
{
	return OSString::withCString("DHD USB");
}

const OSString*
DhdUsbEther::newRevisionString() const
{
	return OSString::withCString(EPI_VERSION_STR);
}

IOReturn
DhdUsbEther::enable(IONetworkInterface *netif)
{
	IOReturn ret = kIOReturnSuccess;

	ret = super::enable(netif);

	if (m_intPipeActive == false) {
		m_intPipe->Read(m_intDesc, 0, 0, m_intDesc->getLength(), &m_intCmplt);
	}

	/* Start receiving USB bulk packets */
	for (int i = 0; i < sRxQLen; i++) {
		m_rxCmplt.parameter = m_rxDescQ[i];
		ret = m_rxPipe->Read(m_rxDescQ[i], 0, 0,
			m_rxDescQ[i]->getLength(), &m_rxCmplt);
	}

	return kIOReturnSuccess;
}

IOReturn
DhdUsbEther::disable(IONetworkInterface *netif)
{
	if (m_txPipe)
		m_txPipe->Abort();

	if (m_rxPipe)
		m_rxPipe->Abort();

	if (m_intPipe) {
		m_intPipe->Abort();
		m_intPipeActive = false;
	}

	return super::disable(netif);
}

void
DhdUsbEther::rxCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining)
{
	DhdUsbEther *thisPtr = (DhdUsbEther *)target;

	if (!thisPtr)
		return;

	if (ret == kIOReturnSuccess) {
			thisPtr->rxPkt((IOBufferMemoryDescriptor *)param, PAGE_SIZE - remaining);
	}

	if (ret != kIOReturnAborted) {
		IOBufferMemoryDescriptor *desc = (IOBufferMemoryDescriptor *)param;

		/* reload descriptor */
		thisPtr->m_rxCmplt.parameter = desc;
		thisPtr->m_rxPipe->Read(desc, 0, 0, desc->getLength(), &thisPtr->m_rxCmplt);
	} else {
		if (thisPtr->m_rxPipe->GetPipeStatus() == kIOUSBPipeStalled) {
			IOLog("rxPipe stalled err=0x%x\n", ret);
		}
	}
}

void
DhdUsbEther::txCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining)
{
	linkitem_t *item = (linkitem_t *)param;
	DhdUsbEther *thisPtr = (DhdUsbEther *)target;
	IOGatedOutputQueue *outpQ;

	if (!thisPtr)
		return;

	thisPtr->ll_enq(&thisPtr->m_txList, item);

	if ((ret != kIOReturnSuccess) && (ret != kIOReturnAborted)) {
		IOLog("txCmpltCb: interrupt err=0x%x!\n", ret);
		if (thisPtr->m_txPipe->GetPipeStatus() == kIOUSBPipeStalled) {
			IOLog("txPipe stalled err=0x%x\n", ret);
		}
	}

	outpQ = (IOGatedOutputQueue *)thisPtr->getOutputQueue();
	if (outpQ->getState() & IOBasicOutputQueue::kStateOutputStalled) {
		/* restart it */
		outpQ->service(IOBasicOutputQueue::kServiceAsync);
	}
}

void
DhdUsbEther::cntlCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining)
{
	if ((ret != kIOReturnSuccess) && (ret != kIOReturnAborted)) {
		IOLog("cntlCmpltCb err=0x%x\n", ret);
	}
}

void
DhdUsbEther::intrCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining)
{
	DhdUsbEther *thisPtr = (DhdUsbEther *)target;
	IOBufferMemoryDescriptor *desc = (IOBufferMemoryDescriptor *)param;
	intr_t *td = (intr_t *)desc->getBytesNoCopy();

	if (!thisPtr)
		return;

	if (td->notification != 1) {
		IOLog("intrCmpltCb: unexpected notification=%d\n", td->notification);
	}

	thisPtr->m_intEvt = true;
	IOLockWakeup(m_intLock, &thisPtr->m_intEvt, true);

	if (ret != kIOReturnAborted) {
		thisPtr->m_intCmplt.parameter = (IOBufferMemoryDescriptor *)param;
		thisPtr->m_intPipe->Read(desc, 0, 0, desc->getLength(), &thisPtr->m_intCmplt);
	}

	if ((ret != kIOReturnSuccess) && (ret != kIOReturnAborted)) {
		IOLog("intrCmpltCb: interrupt err=0x%x!\n", ret);
		if (thisPtr->m_intPipe->GetPipeStatus() == kIOUSBPipeStalled) {
			IOLog("intPipe is stalled \n");
			ret = thisPtr->m_intPipe->ClearPipeStall(false);
			if (ret != kIOReturnSuccess)
				IOLog("intPipe ClearPipeStall failed=0x%x\n", ret);
		}
	}
}

UInt32
DhdUsbEther::outputPacket(mbuf_t m, void  *param)
{
	size_t pktmplen;
	linkitem_t *item;
	IOBufferMemoryDescriptor *desc;
	size_t len = 0;
	UInt32 err = kIOReturnOutputSuccess;
	UInt8 *buffPtr;
	void *pktbuf, *hdrpkt, *pktmp;

	if (!(pktbuf = PKTFRMNATIVE(m_osh, m)))
		return kIOReturnOutputDropped;

	item = ll_deq(&m_txList);
	if (item == NULL)
		return kIOReturnOutputStall;

	desc = (IOBufferMemoryDescriptor *)item->data;

	if (PKTHEADROOM(m_osh, pktbuf) < m_dhd->hdrlen) {
		/* Not enough headroom so allocate for it */
		if ((hdrpkt = PKTGET(m_osh, m_dhd->hdrlen, false)) == NULL) {
			err = kIOReturnOutputDropped;
			m_netStats->outputErrors++;
			goto exit;
		}
		PKTSETNEXT(m_osh, hdrpkt, pktbuf);
		pktbuf = hdrpkt;
	}

	/* If the protocol uses a data header, apply it.
	 * Pushing hdr can increase tot len by hdrlen
	 */
	dhd_prot_hdrpush(m_dhd, pktbuf);

	buffPtr = (UInt8 *)desc->getBytesNoCopy();
	pktmp = pktbuf;
	len = 0;
	do {
		pktmplen = PKTLEN(m_osh, pktmp);
		len += pktmplen;
		if (len > PAGE_SIZE) {
			IOLog("pktbuf chain exceed page size(%d>%d).\n", (int)len, PAGE_SIZE);
			err = kIOReturnOutputDropped;
			m_netStats->outputErrors++;
			goto exit;
		}
		bcopy(PKTDATA(m_osh, pktmp), buffPtr, pktmplen);
		buffPtr += pktmplen;
	} while (pktmp = PKTNEXT(m_osh, pktmp));

	desc->setLength((vm_size_t)len);
	m_txCmplt.parameter = item;

	err = m_txPipe->Write(desc,
		sReqTimeout, /* noDataTimeout ms */
		sReqTimeout, /* completionTimeout ms */
		(IOByteCount)len,
		&m_txCmplt);

	if (err != kIOReturnSuccess) {
		IOLog("txPipe write failed=%d\n", err);
		err = kIOReturnOutputDropped;
		m_netStats->outputErrors++;
		goto exit;
	}
	item = NULL;
	m_netStats->outputPackets++;

exit:
	/* return unused item back to q */
	if (item) {
		IOLog("outputPacket err: return unused item=%p\n", item);
		ll_enq(&m_txList, item);
	}
	PKTFREE(m_osh, pktbuf, false);
	return err;
}

void
DhdUsbEther::rxPkt(IOBufferMemoryDescriptor *desc, size_t len)
{
	mbuf_t m;
	void *pktbuf;

	pktbuf = PKTGET(m_osh, len, false);
	if (!pktbuf) {
		IOLog("rxPkt no mbuf\n");
		m_netStats->inputErrors++;
		return;
	}

	PKTSETLEN(m_osh, pktbuf, len);
	bcopy(desc->getBytesNoCopy(), PKTDATA(m_osh, pktbuf), len);

	if (dhd_prot_hdrpull(m_dhd, pktbuf) != 0) {
		IOLog("dhd_prot_hdrpull failed\n");
		m_netStats->inputErrors++;
		goto err;
	}

	m = PKTTONATIVE(m_osh, pktbuf);
	m_etherIf->inputPacket(m, len);
	m_netStats->inputPackets++;
	return;

err:
	PKTFREE(m_osh, pktbuf, false);
	return;
}

extern "C" {

int
dhd_bus_txctl(struct dhd_bus *bus, uchar *msg, uint msglen)
{
	IOUSBDevRequest devReq;
	int err;
	AbsoluteTime deadline;
	kern_return_t kret;

	devReq.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
	devReq.bRequest = 0;
	devReq.wValue = 0;
	devReq.wIndex = 0;
	devReq.wLength = msglen;
	devReq.pData = msg;

	DhdUsbEther::m_intEvt = false;

	err = DhdUsbEther::m_usbNub->DeviceRequest(&devReq, sReqTimeout, 0,
		&DhdUsbEther::m_cntlCmplt);
	if (err != kIOReturnSuccess)
		return -1;

	clock_interval_to_deadline(sIntRespTimeout, kMicrosecondScale, (uint64_t*)&deadline);
	kret = IOLockSleepDeadline(DhdUsbEther::m_intLock, &DhdUsbEther::m_intEvt,
		deadline, THREAD_UNINT);
	if (kret == THREAD_TIMED_OUT) {
		IOLog("Interrupt resp timeout.\n");
	}

	return 0;
}

int dhd_bus_rxctl(struct dhd_bus *bus, uchar *msg, uint msglen)
{
	IOUSBDevRequest devReq;
	int err = 0;

	devReq.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
	devReq.bRequest = 0;
	devReq.wValue = 0;
	devReq.wIndex = 0;
	devReq.wLength = msglen;
	devReq.pData = msg;

	err = DhdUsbEther::m_usbNub->DeviceRequest(&devReq, sReqTimeout, 0);
	if (err != kIOReturnSuccess)
		return -1;

	return 0;
}

} /* extern C */
