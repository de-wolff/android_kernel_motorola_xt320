/*
 * DHD USB Ethernet for MacOS
 *
 * $Copyright (C) 2007 Broadcom Corporation$
 *
 * $Id: dhd_usb_macos.h,v 1.2 2007-07-10 22:39:52 lut Exp $
 *
 */
#ifndef __DHD_USB_MACOS_H_
#define __DHD_USB_MACOS_H_

#include "dhd_macos.h"

extern "C" {
	#include "dhdioctl.h"
	#include "wlioctl.h"
	#include "bcmutils.h"
	#include "dngl_stats.h"
	#include "dhd.h"
}

typedef struct {
	UInt32 notification;
	UInt32 reserved;
} intr_t;

const int sTxQLen = 30;
const int sRxQLen = 30;
const int sReqTimeout = 3000; /* 3 sec */
const UInt32 sIntRespTimeout = 1000000; /* 1 sec */

class DhdUsbEther : public DhdEther
{
	OSDeclareDefaultStructors(DhdUsbEther)

protected:
	IOUSBInterface		*m_usbIf;

	/*
	 * Static member functions
	 */
	static void cntlCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining);
	static void intrCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining);
	static void txCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining);
	static void rxCmpltCb(void *target, void *param, IOReturn ret, UInt32 remaining);

public:
	/*
	 * Static member variables
	 */
	static IOUSBDevice	*m_usbNub;
	static IOLock		*m_intLock;
	static bool		m_intEvt;
	static IOUSBCompletion	m_cntlCmplt;

	IOUSBPipe		*m_rxPipe;
	IOUSBPipe		*m_txPipe;
	IOUSBPipe		*m_intPipe;
	bool			m_intPipeActive;
	UInt16			m_rxMPS;
	UInt16			m_txMPS;

	IOBufferMemoryDescriptor	*m_intDesc;
	IOUSBCompletion			m_rxCmplt;
	IOUSBCompletion			m_txCmplt;
	IOUSBCompletion			m_intCmplt;

	linklist_t			m_txList;
	linkitem_t			m_txDescQ[sTxQLen];
	IOBufferMemoryDescriptor	*m_rxDescQ[sRxQLen];

	virtual bool init(OSDictionary *properties);
	virtual void free();
	virtual bool start(IOService *provider);
	virtual void stop(IOService *provider);
	virtual bool terminate(IOOptionBits options = 0);

	UInt32 outputPacket(mbuf_t m, void *param);
	void rxPkt(IOBufferMemoryDescriptor *desc, size_t len);

	/*
	 * DhdEther methods - implement these
	 */
	virtual IOReturn enable(IONetworkInterface *netif);
	virtual IOReturn disable(IONetworkInterface *netif);
	virtual IOReturn setPromiscuousMode(IOEnetPromiscuousMode mode);
	virtual const OSString *newVendorString(void) const;
	virtual const OSString *newModelString(void) const;
	virtual const OSString *newRevisionString(void) const;
};
#endif /* __DHD_USB_MACOS_H_ */
