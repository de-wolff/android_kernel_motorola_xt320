/*
 * NDIS-specific driver data structure fields and defines
 * shared between il and et drivers
 *
 * Copyright(c) 2001 Broadcom Corporation
 * $Id: ndshared.h,v 1.86 2009-06-30 01:17:03 mthawani Exp $
 *
 */

#ifndef	_NDSHARED_H
#define	_NDSHARED_H
#if defined (NDIS40)
#include <ndis41.h>
#elif defined (NDIS50) || defined(NDIS51) || defined(NDIS60) || defined(NDIS620)
#include <ndis.h>
#else
#error "Unsupported version of NDIS..."
#endif /* defined (NDIS40) */

#ifndef UNDER_CE
#define MAX_PATH	128
#endif /* UNDER_CE */

#if defined(NDIS60) || defined(NDIS620)
#if (defined(BCMDONGLEHOST) || !defined(WLC_LOW)) && !defined(UNDER_CE)
#include <wdf.h>
#include <WdfMiniport.h>
#endif /* defined(BCMDONGLEHOST) || !defined(WLC_LOW) */

#if !defined(UNDER_CE)
#define BCMNDIS6	/* defined for NDIS 6.x */
#endif /* !UNDER_CE */
#define ND_PKT	NET_BUFFER_LIST
#else
#define ND_PKT	NDIS_PACKET
#endif /* BCMNDIS6 */

#ifdef UNDER_CE
#include <pkfuncs.h>
#include <cardserv.h>
#include <cardapi.h>

#define RX_FLOW_CTL_LOW 80
#define RX_FLOW_CTL_HIGH 128

/* DHD rx flow control mode */
enum dhd_rx_flow_control_mode
{
	RX_FLOW_CONTROL_OFF,
	RX_FLOW_CONTROL_RXBOUND,	/* Rx bound trigger flow control */
	RX_FLOW_CONTROL_WATERMARK,   /* Rx free buffer count water mark trigger flow control */
	RX_FLOW_CONTROL_MAX
};

typedef struct pcmcia_func {
	GETFIRSTTUPLE pfnGetFirstTuple;
	GETNEXTTUPLE pfnGetNextTuple;
	GETTUPLEDATA pfnGetTupleData;
	GETPARSEDTUPLE pfnGetParsedTuple;
	REGISTERCLIENT pfnRegisterClient;
	DEREGISTERCLIENT pfnDeregisterClient;
	REQUESTWINDOW pfnRequestWindow;
	RELEASEWINDOW pfnReleaseWindow;
	MAPWINDOW pfnMapWindow;
	REQUESTCONFIG pfnRequestConfig;
	RELEASECONFIG pfnReleaseConfig;
} pcmcia_func_t;
/* workaround for WinCE NdisReadxxx/NdisWritexxx issue in code optimization */
#undef NdisReadRegisterUshort
#undef NdisReadRegisterUlong
#undef NdisWriteRegisterUshort
#undef NdisWriteRegisterUlong
#define NdisReadRegisterUshort(address, ptr) *(ptr) = READ_REGISTER_USHORT(address)
#define NdisReadRegisterUlong(address, ptr) *(ptr) = READ_REGISTER_ULONG(address)
#define NdisWriteRegisterUshort(address, value) WRITE_REGISTER_USHORT(address, value)
#define NdisWriteRegisterUlong(address, value) WRITE_REGISTER_ULONG(address, value)
/* WinCE does not support NdisMGetDmaAlignment though it supports NDSI 5.01 */
#ifdef NdisMGetDmaAlignment
#undef NdisMGetDmaAlignment
#endif /* NdisMGetDmaAlignment */
#if defined (_MIPS_)
INLINE uint
NdisMGetDmaAlignment(void *MiniportAdapterHandle)
{
	volatile uint cacheline, config;
	/* read CP0 register 16, select 1 */
	__asm(".word 0x40028001");
	__asm("sw v0, 0(%0)", &config);
	if ((cacheline = ((config >> 10) & 7)))
		cacheline = 2 << cacheline;
	return cacheline;
}
#else	/* non MIPS system under CE */
/* XXX - cacheline hard coded! */
#if !defined(UNDER_CE)
INLINE uint NdisMGetDmaAlignment(void *MiniportAdapterHandle) {return 1;}
#endif /* UNDER_CE */
#endif /* defined (_MIPS_) */
#ifdef BCMDBG
#if _WIN32_WCE == 420
/* __FUNCTION__ not defined in WinCE 4.20 */
char* str_line(char *file, uint line);
#define __FUNCTION__ str_line(__FILE__, __LINE__)
#endif /* _WIN32_WCE == 420 */
#endif /* BCMDBG */
#endif /* UNDER_CE */

/* defined for MIPS CPU */
#define KSEG0_BASE 0x80000000	/* kseg0 base address */
#define KSEG1_BASE 0xA0000000	/* kseg1 base address */

#if defined (BCMENET)
#include "et_ndis.h"
#elif defined (WLSIM)
#include "wl_sim.h"
#elif defined (BCMWL)
#else
#error "Unsupported NDIS driver requested..."
#endif /* defined (BCMENET) */

#ifdef	BCMDBG
extern int *nd_msg_level;
#define	ND_ERROR(args)	if (!(*nd_msg_level & 1)); else printf args
#define	ND_TRACE(args)	if (!(*nd_msg_level & 2)); else printf args
#else	/* BCMDBG */
#define	ND_ERROR(args)
#define	ND_TRACE(args)
#endif	/* BCMDBG */

/* locking wrappers */
#ifdef BCMDONGLEHOST
#define ND_LOCK(context)
#define ND_UNLOCK(context)
#define ND_RXLOCK(context)			NdisAcquireSpinLock(&(context)->rx_lock)
#define ND_RXUNLOCK(context)		NdisReleaseSpinLock(&(context)->rx_lock)
#else /* BCMDONGLEHOST */
#ifdef WLC_LOW
#define ND_LOCK(context)	{ \
					if (ND_ALL_PASSIVE_ENAB(context)) \
						shared_sem_acquire(&(context)->perimeter_sem); \
					else \
						NdisAcquireSpinLock(&(context)->lock); \
							}
#define ND_UNLOCK(context)	{ \
					if (ND_ALL_PASSIVE_ENAB(context)) \
						shared_sem_release(&(context)->perimeter_sem); \
					else \
						NdisReleaseSpinLock(&(context)->lock); \
							}
#else
#define ND_LOCK(context)			shared_sem_acquire(&(context)->perimeter_sem)
#define ND_UNLOCK(context)			shared_sem_release(&(context)->perimeter_sem)
#endif /* WLC_LOW */

#define ND_RXLOCK(context)
#define ND_RXUNLOCK(context)
#endif /* BCMDONGLEHOST */

#if (defined(EXT_STA) && defined(WLC_LOW)) || defined(BCMDONGLEHOST)
#define ND_TXLOCK(context)			NdisAcquireSpinLock(&(context)->tx_lock)
#define ND_TXUNLOCK(context)		NdisReleaseSpinLock(&(context)->tx_lock)
#else
#define ND_TXLOCK(context)
#define ND_TXUNLOCK(context)
#endif /* BCMDONGLEHOST */

#ifndef NDIS_ERROR
#define NDIS_ERROR(status)	(status != NDIS_STATUS_SUCCESS)
#endif /* NDIS_ERROR */

#ifndef BCMNDIS6
#define	NEXTP(p)	(*(PNDIS_PACKET*)(&(p)->MiniportReserved[0]))
#define INTERFACE(p)	(*(wl_if_t **)(&(p)->MiniportReserved[sizeof(PVOID)]))
#define PKTP(p)		(*(struct lbuf **)(&(p)->ProtocolReserved[0]))
#else
#define	NEXTP(p)	(*(PNET_BUFFER_LIST*)(&(NET_BUFFER_LIST_MINIPORT_RESERVED(p))[0]))
#define INTERFACE(p)	(*(wl_if_t **)(&(NET_BUFFER_LIST_MINIPORT_RESERVED(p))[1]))
#define PKTP(p)		(*(struct lbuf **)(&(NET_BUFFER_LIST_PROTOCOL_RESERVED(p))[0]))
#endif

/* macros for setting and getting packet priority */
#ifdef WDM
#ifndef BCMNDIS6
#define SET_NDIS_PRIORITY(p, pri) \
	NDIS_PER_PACKET_INFO_FROM_PACKET((p), Ieee8021pPriority) = (void*)(uintptr)(pri)
#define GET_NDIS_PRIORITY(p) \
	(uint)(uintptr)NDIS_PER_PACKET_INFO_FROM_PACKET((p), Ieee8021pPriority)
#else /* !BCMNDIS6 */
/* XXX - May need to do more with this (e.g., OOB container for VLAN tag) */
static INLINE int32 get_NetBufferListInfo_WMMInfo(ND_PKT *p)
{
	NDIS_NET_BUFFER_LIST_8021Q_INFO tag;
	tag.Value = NET_BUFFER_LIST_INFO(p, Ieee8021QNetBufferListInfo);
	return (tag.WLanTagHeader.WMMInfo);
}
#define SET_NDIS_PRIORITY(p, pri) \
	NET_BUFFER_LIST_INFO(p, Ieee8021QNetBufferListInfo) = \
		(void*)(uintptr)(pri<<16)
#define GET_NDIS_PRIORITY(p) get_NetBufferListInfo_WMMInfo(p)
#endif /* !BCMNDIS6 */
#else /* WDM */
#define SET_NDIS_PRIORITY(p, pri)	/* nothing */
#define GET_NDIS_PRIORITY(p)		0
#endif /* WDM */

/* shared memory page bookkeeping */
typedef struct {
	char			*va;	/* kernel virtual address */
	NDIS_PHYSICAL_ADDRESS	pa;	/* physical address */
} page_t;

/*
 * Local packet buffer structure.
 *
 * lbufs are 2Kbytes in size to contain a full sized ETHER_MAX_LEN frame,
 * not cross 4Kbyte physical address DMA boundaries, and be efficiently carved
 * from 4Kbyte pages from shared (DMA-able) memory.
 *
 * From the NDIS NdisMStartBufferPhysicalMapping man page:
 *
 *	"For small transfer requests, such as those up to 256K in
 *	length, a miniport can achieve higher performance by copying
 *	the data to be transmitted into a staging buffer in the
 *	shared memory space already allocated with NdisMAllocateSharedMemory
 *	or NdisMAllocateSharedMemoryAsync. Because the NIC driver
 *	already has mapped virtual and physical addresses for such
 *	a shared memory range, it need not call NdisM..BufferPhysicalMapping
 *	functions for such small DMA transfers."
 */

#ifdef BCMDONGLEHOST
#define LBUFSZ		3072	/* largest reasonable packet buffer, driver uses for ethernet MTU */
#else /* BCMDONGLEHOST */
#define LBUFSZ		2048	/* largest reasonable packet buffer, driver uses for ethernet MTU */
#endif /* BCMDONGLEHOST */
#define LBDATASZ	(LBUFSZ - sizeof(struct lbuf))
#define BPP		(PAGE_SIZE / LBUFSZ)	/* number of buffers per page */

struct lbfree;				/* forward declaration */

struct lbuf {
	struct lbuf	*next;		/* pointer to next lbuf if in a chain */
	struct lbuf	*link;		/* pointer to next lbuf if in a list */
	uchar		*head;		/* start of buffer */
	uchar		*end;		/* end of buffer */
	uchar		*data;		/* start of data */
	uchar		*tail;		/* end of data */
	uint		len;		/* nbytes of data */
	uint		priority;	/* packet priority */
#ifdef EXT_STA
	uint		exempt;		/* encryption exemption */
#endif /* EXT_STA */
	dmaaddr_t	pa;		/* low 32bits of physical address of head */
	ND_PKT		*p;		/* ndis packet associated with us */
	uint16		flags;
	uchar		pkttag[OSL_PKTTAG_SZ]; /* pkttag area  */
	struct lbfree	*l;		/* lbuf's parent freelist */
};

#define LBPP (PAGE_SIZE / sizeof(struct lbuf))	/* number of lbuf per page */

/*
 * Simple, static list of free lbufs.
 * For reasonable memory utilization, we alloc and split whole pages
 * into 2Kbyte buffers.
 */
typedef struct lbfree {
	struct lbuf	*free;		/* the linked list */
	uint		total;		/* # total packets */
	uint		count;		/* # packets currently on free */
	ulong		headroom;    /* XXXX: Do we really need 4 bytes but helps alignment */
	uint		size;		/* # bytes packet buffer memory */
	page_t		*pages;		/* pointer to array of backing pages */
	uint		npages;		/* # pages in pages vector */
};

struct npq {
	ND_PKT	*head;
	ND_PKT	*tail;
};

typedef struct shared_sem {
	uint val;
	NDIS_SPIN_LOCK lock;
	NDIS_EVENT event;
} shared_sem_t;

#ifdef	BCMSDIO
#define NdisInterfaceSDIO	NdisInterfaceInternal	/* update when Ndis includes SDIO bus */
#endif /* BCMSDIO */

typedef struct shared_info {
	char		id[3];			/* "il" or "et" or "wl" */
	uint		unit;			/* device instance number */
	NDIS_ENVIRONMENT_TYPE OS;	/* OS type from NdisReadConfiguration */

	NDIS_HANDLE	adapterhandle;		/* MiniportAdapterHandle */
	NDIS_HANDLE	confighandle;		/* ConfigurationHandle */
#ifdef BCMNDIS6
	NDIS_HANDLE	adaptercontext;		/* MiniportAdapterContext */
	NDIS_HANDLE	dmahandle;		/* Scatter/Gather DMA Handle */
#if (defined(BCMDONGLEHOST) || !defined(WLC_LOW)) && !defined(UNDER_CE)
	WDFDEVICE	wdfdevice;
#endif
#endif /* BCMNDIS6 */

	NDIS_INTERFACE_TYPE	BusType;	/* Bus type */

	void 		*osh;			/* pointer to OS handle */
	void		*FDO;			/* Our Functional Device Object for WDM calls */
	void		*PDO;			/* Physical Device Object for WDM calls */

	uint		slot;			/* device pci slot number */
	uint		cacheline;		/* processor cache line size */

	struct npq	txdone;			/* queue of completed tx packets */

	struct lbfree	txfree;			/* tx packet freelist */
	struct lbfree	rxfree;			/* rx packet freelist */
	uint		dmaddrwidth;		/* XXX Address Width of the DMA engine */

	NDIS_HANDLE	rxpacketpool;		/* rx packet descriptor pool handle */
#ifndef BCMNDIS6
	NDIS_HANDLE	rxbufferpool;		/* rx buffer descriptor pool handle */
#endif /* !BCMNDIS6 */
#ifdef UNDER_CE
	void		*attrva;		/* PCMCIA attribute memory base address */
#endif /* UNDER_CE */
	void		*dhd;
#ifdef BCMSDIO
	void		*sdh;			/* pointer to sdio bus handler */
	void		(*isr_cb)(PBOOLEAN, PBOOLEAN, NDIS_HANDLE);	/* isr call back */
	void		*isr_arg;		/* isr call back argument */
	HANDLE		dpc_handle;		/* dpc thread handle */
	uint		dpc_thread_id;		/* dpc thread id */
	NDIS_EVENT	dpc_event;		/* event to wake dpc */
	BOOLEAN		InterruptRecognized;
	BOOLEAN		QueueMiniportHandleInterrupt;
	BOOLEAN		in_isr;
	uint		irq_level;
	BOOLEAN		poll_mode;
	NDIS_MINIPORT_TIMER		*poll_timer;
	NDIS_SPIN_LOCK			*ndlock;
#endif /* BCMSDIO */
#if defined(BCMSDIO) || defined(BCMDHDUSB)
	BOOLEAN		attached;
	void		(*dpc_cb)(NDIS_HANDLE);	/* dpc call back */
	void		*dpc_arg;		/* dpc call back argument */
#ifndef BCMNDIS6
	PNDIS_MINIPORT_INTERRUPT	intr;
#endif /* !BCMNDIS6 */
	NDIS_TIMER  dpc_reshed_timer; /* Used to reschedule DPC for bounded process and DHD */
#endif /* BCMSDIO || BCMDHDUSB */
	uint         DPCPriority;
	uint		 RxDPCPriority;
	uint         WatchdogPriority;
	uint		 RXThreadPriority;
#if defined(UNDER_CE)
	uint		OidPriority;			/* Store registry value */
	uint		EventPriority;		/* Store registry value */
#endif /* defined(UNDER_CE) */
	uint         DrvStrength;
	uint		 EnablePoll;
	uint		rxdpc_prio;		/* Local DHD rx dpc thread priority value */
	uint		sendup_prio;		/* Local DHD send up thread priority value */
	uint		dpc_prio;		/* Local DHD dpc priority thread value */
	uint		wd_prio;		/* Local DHD watch dog thread priority value */
	uint		RxflowRxDPCPriority;	/* Registry rx dpc thread priority level rx flow */
	uint		RxflowRXThreadPriority; /* Registry send up thread priority level rx flow */
	uint		rxflowMode;			/* Rx flow control mode */
	uint		rxflowHi;	/* Rx flow control free buffer high water mark */
	uint		rxflowLow;	/* Rx flow control free buffer low water mark */
	bool		rxflow;		/* Rx flow control on/off state flag */
	CHAR		 dongleImagePath[MAX_PATH];
	CHAR		 sromImagePath[MAX_PATH];
#ifdef AIROPEEK
	void		*airopeek;		/* AIROPEEK */
#endif /* AIROPEEK */
} shared_info_t;

extern NDIS_STATUS
shared_allocpage(
	IN shared_info_t *shared,
	IN BOOLEAN shared_mem,
	IN BOOLEAN cached,
	OUT page_t *page
);

extern NDIS_STATUS
shared_lb_alloc(
	IN shared_info_t *shared,
	IN struct lbfree *l,
	IN uint total,
	IN BOOLEAN shared_mem,
	IN BOOLEAN cached,
	IN BOOLEAN piomode,
	IN BOOLEAN data_buf
);

extern void
shared_lb_free(
	IN shared_info_t *shared,
	IN struct lbfree *l,
	IN BOOLEAN shared_mem,
	IN BOOLEAN cached
);

extern NDIS_STATUS
shared_lb_addpage(
	IN shared_info_t *shared,
	IN struct lbfree *l,
	IN BOOLEAN piomode,
	page_t *page,
	IN uint ipp,
	IN uint lbdatasz
);

extern struct lbuf *
shared_lb_get(
	IN shared_info_t *shared,
	IN struct lbfree *l
);

extern void
shared_lb_put(
	IN shared_info_t *shared,
	IN struct lbfree *l,
	IN struct lbuf *lb
);

extern NDIS_STATUS
shared_flush(
	IN shared_info_t *shared,
	IN uchar *va,
	IN dmaaddr_t pa,
	IN ULONG len,
	IN BOOLEAN writetodevice
);

extern void
shared_enq(
	IN struct npq *q,
	IN ND_PKT *p,
	IN bool lifo
);

extern ND_PKT *
shared_deq(
	IN struct npq *q
);

extern void
shared_free_pkt(
	IN ND_PKT *p
);

extern void
shared_free_ndispacket(
	IN shared_info_t *shared,
	IN ND_PKT *p
);

extern struct lbuf*
shared_txlb_convert(
	IN shared_info_t *sh,
	IN ND_PKT *p
);

void shared_sem_init(shared_sem_t *sem, uint init_val);
void shared_sem_free(shared_sem_t *sem);
void shared_sem_acquire(shared_sem_t *sem);
void shared_sem_release(shared_sem_t *sem);

void
shared_dpc_schedule(shared_info_t *sh);

#ifdef EXT_STA
void
shared_indicate_status(NDIS_HANDLE adapterhandle, NDIS_PORT_NUMBER PortNumber,
	NDIS_STATUS StatusCode, void *StatusBuffer, ULONG StatusBufferSize);
#endif /* EXT_STA */

#ifndef BCMNDIS6
NDIS_STATUS
shared_interrupt_register(OUT PNDIS_MINIPORT_INTERRUPT Interrupt,
				NDIS_HANDLE             MiniportAdapterHandle,
				UINT                    InterruptVector,
				UINT                    InterruptLevel,
				BOOLEAN                 RequestIsr,
				BOOLEAN                 SharedInterrupt,
				NDIS_INTERRUPT_MODE     InterruptMode,
				shared_info_t *sh,
				void (*isr_cb)(PBOOLEAN, PBOOLEAN, NDIS_HANDLE),
				void *isr_arg,
				void (*dpc_cb)(NDIS_HANDLE),
				void *dpc_arg);
#else
#if defined(UNDER_CE)
NDIS_STATUS
shared_interrupt_register(NDIS_HANDLE MiniportAdapterHandle,
shared_info_t *sh,
void (*isr_cb)(PBOOLEAN, PBOOLEAN, NDIS_HANDLE),
void *isr_arg,
void (*dpc_cb)(NDIS_HANDLE),
void *dpc_arg);
#else /* UNDER_CE */
NDIS_STATUS
shared_interrupt_register(NDIS_HANDLE MiniportAdapterHandle,
	NDIS_HANDLE MiniportInterruptContext,
	MINIPORT_ISR_HANDLER isr,
	MINIPORT_INTERRUPT_DPC_HANDLER dpc,
	MINIPORT_DISABLE_INTERRUPT_HANDLER intrsoff,
	MINIPORT_DISABLE_INTERRUPT_HANDLER wlc_intrson,
	NDIS_HANDLE *phandle);
#endif /* UNDER_CE */
#endif /* !BCMNDIS6 */

#ifndef BCMNDIS6
void
shared_interrupt_deregister(PNDIS_MINIPORT_INTERRUPT intr,
			      shared_info_t *sh);
#else
void
shared_interrupt_deregister(NDIS_HANDLE intr_handle);
#endif /* BCMNDIS6 */

#ifndef BCMNDIS6
BOOLEAN
shared_interrupt_synchronize(PNDIS_MINIPORT_INTERRUPT pintr,
			      shared_info_t *sh,
			      void (*SyncFunc)(void *),
			      void *SyncCxt);
#else
BOOLEAN
shared_interrupt_synchronize(NDIS_HANDLE intr,
	void (*SyncFunc)(void *),
	void *SyncCxt);
#endif /* !BCMNDIS6 */

#ifdef NDIS_DMAWAR
/* Alternate Mm* API needed for 30-bit DMA engine WAR */
NTKERNELAPI
PHYSICAL_ADDRESS
MmGetPhysicalAddress(
	IN PVOID BaseAddress);

NTKERNELAPI
PVOID
MmAllocateContiguousMemorySpecifyCache(
	IN SIZE_T NumberOfBytes,
	IN PHYSICAL_ADDRESS LowestAcceptableAddress,
	IN PHYSICAL_ADDRESS HighestAcceptableAddress,
	IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
	IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
VOID
MmFreeContiguousMemorySpecifyCache(
	IN PVOID BaseAddress,
	IN SIZE_T NumberOfBytes,
	IN MEMORY_CACHING_TYPE CacheType);
#endif /* NDIS_DMAWAR */

#ifdef UNDER_CE
extern HMODULE
PcmciaLoadDll(
	IN pcmcia_func_t *pFunc
);

extern void *
PcmciaOpen(
	IN osl_t *osh,
	IN void **pRegBaseAddr,
	IN void **pAttrBaseAddr
);

extern void
PcmciaClose(
	IN osl_t *osh,
	IN void *client
);
#endif /* UNDER_CE */

#endif /* _NDSHARED_H */
