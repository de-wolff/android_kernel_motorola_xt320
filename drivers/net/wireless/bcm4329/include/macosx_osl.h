/*
 * Mac OS X Independent Layer
 *
 * $Copyright (c) 2005 Broadcom Corporation$
 *
 * Copyright (c) 2002 Apple Computer, Inc.  All rights reserved.
 *
 * $Id: macosx_osl.h,v 1.53.4.1 2010-10-19 02:09:05 kiranm Exp $
 */

#ifndef _macosx_osl_h_
#define _macosx_osl_h_

#include <typedefs.h>
#include <sys/cdefs.h>
__BEGIN_DECLS
#include <sys/kpi_mbuf.h>
__END_DECLS
#include <IOKit/IOLib.h>
#ifdef  __cplusplus
#include <IOKit/pci/IOPCIDevice.h>
#else
typedef void IOPCIDevice;
#endif

#ifdef USE_KLOG
#include "KernelDebugging.h"
/* KernelDebugging.h defines ASSERT, but we want the panic() version */
#ifdef ASSERT
#undef ASSERT
#endif
#endif /* USE_KLOG */

/* assert and panic */
#ifdef BCMDBG_ASSERT
#define ASSERT(exp)  \
	((exp) ? (void)0 : osl_assert(__FILE__, __LINE__, # exp))
#else /* BCMDBG_ASSERT */
#define	ASSERT(exp)		do {} while (0)
#endif /* BCMDBG_ASSERT */

#define OSL_LOG osl_log
extern void osl_log(char * tag, char *fmt, ...);

/* verify some compile settings */
#ifdef __BIG_ENDIAN__
#ifndef IL_BIGENDIAN
#error "IL_BIGENDIAN was not defined for a big-endian compile"
#endif
#else
#ifdef IL_BIGENDIAN
#error "IL_BIGENDIAN was defined for a little-endian compile"
#endif
#endif /* __BIG_ENDIAN__ */

#ifndef DMA
#error "DMA not defined"
#endif
#ifndef MACOSX
#error "MACOSX not defined"
#endif

#ifdef USE_KLOG
#define printf(ARGS...)		KernelDebugLogInternal(2, 'pciW' , ## ARGS)
#else
#define printf			IOLog
#endif /* USE_KLOG */

/* microsecond delay */
#define	OSL_DELAY(us)		IODelay(us)

#define OSL_GETCYCLES(c)	(void)(c = 0)

/* PCMCIA attribute space access macros */
#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size)	ASSERT(0)
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size)	ASSERT(0)

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size)		\
		osl_pci_read_config((osh), offset, size)
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val)	\
		osl_pci_write_config((osh), offset, size, val)

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	0
#define OSL_PCI_SLOT(osh)	0

/* host/bus architecture specific swap */
#define BUS_SWAP32(v)		htol32(v)

/* uncached/cached virtual address */
#define	OSL_UNCACHED(va)	(va)
#define	OSL_CACHED(va)		(va)

/* map/unmap virtual to physical */
#define REG_MAP(pa, size)	((void*)(uintptr)(pa))
#define REG_UNMAP(pa)		(void)0

/* register access macros */
#define	R_REG(osh, r)	(uint32)((sizeof *(r) == sizeof(uint32)) ?		\
				OSReadLittleInt32((uint32*)(r), 0)		\
				: ((sizeof *(r) == sizeof(uint16)) ?		\
					OSReadLittleInt16((uint16*)(r), 0)	\
					: *(volatile uint8*)(r)))

#define	W_REG(osh, r, v)	do {								\
				if (sizeof *(r) == sizeof (uint32))				\
					OSWriteLittleInt32((uint32*)(r), 0, (v));		\
				else if (sizeof *(r) == sizeof (uint16))			\
					OSWriteLittleInt16((uint16*)(r), 0, (uint16)(v));	\
				else								\
					*(volatile uint8*)(r) = (uint8)(v);			\
				OSSynchronizeIO();						\
			} while (0)

#define	AND_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) | (v))


/* shared memory access macros */
#define	R_SM(a)			(*(a))
#define	W_SM(a, v)		(*(a) = (v))
#define	BZERO_SM(a, len)	bzero(a, len)

#ifdef MALLOC
#undef MALLOC
#endif
#ifdef MFREE
#undef MFREE
#endif

#ifdef BCMDBG_MEM
#define MALLOC(osh, size)	osl_debug_malloc((osh), (size), __LINE__, __FILE__)
#define MFREE(osh, addr, size)	osl_debug_mfree((osh), (addr), (size), __LINE__, __FILE__)
#define MALLOCED(osh)		osl_malloced((osh))
#define MALLOC_DUMP(osh, b) 	osl_debug_memdump((osh), (b))

#else

#define	MALLOC(osh, size)		osl_malloc(osh, size)
#define	MFREE(osh, addr, size)		osl_mfree(osh, addr, size)
#define MALLOCED(osh)			osl_malloced(osh)
/* JS:   need to define/implement these debug features */
#define MALLOC_DUMP(osh, b)
#endif /* BCMDBG_MEM */

#define MALLOC_FAILED(osh)		osl_malloc_failed(osh)

#define BUSPROBE(val, addr)		val = R_REG(addr)

/* allocate/free shared (dma-able) consistent memory */
#define DMA_CONSISTENT_ALIGN		PAGE_SIZE
#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pa, dmah)	\
		osl_dma_alloc_consistent(osh, size, (align), (tot), pa, (void **)dmah)
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah)	\
		osl_dma_free_consistent(osh, va, size, dmah)

/* map/unmap direction */
#define	DMA_TX	1
#define	DMA_RX	0

/* map/unmap shared (dma-able) memory */
#define	DMA_MAP(osh, va, size, direction, p, dmah) \
		osl_dma_map(osh, va, size, direction, (mbuf_t)(p), (dmah))
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah)	/* nop */

#define OSL_DMADDRWIDTH(osh, width)	osl_dma_addrwidth(osh, width)

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	2044

/* packet primitives */
#define	PKTGET(osh, len, send)		osl_pktget(osh, len)
#define	PKTFREE(osh, m, send)		osl_pktfree(osh, (mbuf_t)(m), (send))
#define	PKTDUP(osh, m)			osl_pktdup(osh, (mbuf_t)(m))
#define	PKTDATA(osh, m)			((uint8*)mbuf_data((mbuf_t)(m)))
#define	PKTLEN(osh, m)			(int)mbuf_len((mbuf_t)(m))
#define	PKTSETLEN(osh, m, len)		mbuf_setlen((mbuf_t)(m), len)
#define PKTHEADROOM(osh, m)		((uint32)mbuf_leadingspace((mbuf_t)(m)))
#define PKTTAILROOM(osh, m)		((uint32)mbuf_trailingspace((mbuf_t)(m)))
#define	PKTPUSH(osh, m, nbytes)		osl_pktpush(osh, (mbuf_t)(m), nbytes)
#define	PKTPULL(osh, m, nbytes)		osl_pktpull(osh, (mbuf_t)(m), nbytes)
#define	PKTNEXT(osh, m)			mbuf_next((mbuf_t)(m))
#define	PKTSETNEXT(osh, m, n)		mbuf_setnext((mbuf_t)(m), (mbuf_t)(n))
#define	PKTLINK(m)			mbuf_nextpkt((mbuf_t)(m))
#define	PKTSETLINK(m, n)		mbuf_setnextpkt((mbuf_t)(m), (mbuf_t)(n))
#define	PKTTAG(m)			osl_pkt_gettag((mbuf_t)(m))
#define PKTFRMNATIVE(osh, m)		osl_pkt_frmnative(osh, m)
#define PKTTONATIVE(osh, m)		osl_pkt_tonative(osh, (mbuf_t)(m))
#define	PKTPRIO(m)			osl_getprio((m))
#define	PKTSETPRIO(m, n)		osl_setprio((m), (n))
#define PKTSHARED(m)                    mbuf_mclhasreference((mbuf_t)(m))
#define PKTALLOCED(osh)			(0)
#define PKTSUMNEEDED(m)			(0)
#define PKTSETSUMGOOD(m, x)		(0)
#define PKTSETPOOL(osh, m, x, y)	do {} while (0)
#define PKTPOOL(osh, m)			FALSE
#ifdef BCMDBG_PKT /* pkt logging for debugging */
#define PKTLIST_DUMP(osh, buf)          ((void)buf)
#else /* BCMDBG_PKT */
#define PKTLIST_DUMP(osh, buf)
#endif /* BCMDBG_PKT */
#define PKTSHRINK(osh, m)		(m)

#define OSL_ERROR(e)			osl_error(e)

/* get system up time in miliseconds */
#define OSL_SYSUPTIME()			osl_sysuptime()

__BEGIN_DECLS

/* OSL initialization */
extern osl_t*	osl_attach(IOPCIDevice *pcidev);
extern void	osl_detach(osl_t *osh);
extern errno_t	osl_alloc_private_mbufs(osl_t *osh);

extern void osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)

/* PCI configuration space access */
extern uint32	osl_pci_read_config(osl_t *osh, uint offset, uint size);
extern void	osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* memory allocation */
#ifdef BCMDBG_MEM
extern void *osl_debug_malloc(osl_t *osh, uint size, int line, const char* file);
extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, const char* file);
struct bcmstrbuf;
extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *b);
#endif /* BCMDBG_MEM */

extern void*	osl_malloc(osl_t *osh, uint size);
extern void	osl_mfree(osl_t *osh, void *addr, uint size);
extern uint	osl_malloced(osl_t *osh);
extern uint	osl_malloc_failed(osl_t *osh);

/* map/unmap shared (dma-able) memory */
extern dmaaddr_t	osl_dma_map(osl_t *osh, void *va, int size, uint direction, mbuf_t mbuf,
                                    hnddma_seg_map_t *dmah);
extern void*	osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align, uint *tot,
	dmaaddr_t *pa, void **dmah);
extern void		osl_dma_free_consistent(osl_t *osh, void *va, uint size, void *dmah);
extern void		osl_dma_addrwidth(osl_t *osh, uint width);

/* packet primitives */
extern mbuf_t	osl_pktget(osl_t *osh, uint len);
extern void	osl_pktfree(osl_t *osh, mbuf_t m, bool send);
extern mbuf_t	osl_pktdup(osl_t *osh, mbuf_t m);
extern uint8*	osl_pktpush(osl_t *osh, mbuf_t m, uint nbytes);
extern uint8*	osl_pktpull(osl_t *osh, mbuf_t m, uint nbytes);
extern void*	osl_pkt_gettag(mbuf_t m);
extern mbuf_t	osl_pkt_frmnative(osl_t *osh, mbuf_t m);
extern mbuf_t	osl_pkt_tonative(osl_t *osh, mbuf_t m);

extern int osl_getprio(mbuf_t m);
extern void osl_setprio(mbuf_t m, int prio);

extern int osl_error(int bcmerror);
extern void osl_assert(const char *file, int line, const char *exp);
extern uint32 osl_sysuptime();

/* strrchr proto not in Mac OS X Kernel.framework strings.h */
char *strrchr(const char *s, int c);

extern uint32 g_assert_type;
__END_DECLS

#endif	/* _macosx_osl_h_ */
