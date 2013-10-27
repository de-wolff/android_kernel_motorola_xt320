/*
 * IOPOS (Nintendo Revolution) Abstraction Layer.
 *
 * $Copyright (C) 2006 Broadcom Corporation$
 *
 * $Id: iopos_osl.h,v 13.14.8.1 2010-10-19 02:09:05 kiranm Exp $
 */

#ifndef _iopos_osl_h_
#define _iopos_osl_h_

#include <typedefs.h>
#include <ioslibc.h>
#include <iosiobuf.h>

#ifdef BCMSDIO
#include <bcmsdh.h>
#endif

/* assert & debugging */
#ifdef BCMDBG_ASSERT
#define ASSERT(exp) \
	do { if (!(exp)) osl_assert(#exp, __FILE__, __LINE__); } while (0)
extern void osl_assert(char *exp, char *file, int line);
#else /* BCMDBG_ASSERT */
#define ASSERT(exp)     do {} while (0)
#endif /* BCMDBG_ASSERT */


/* PCMCIA attribute space access macros; No PCMCIA support */
#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) \
	ASSERT(0)
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) \
	ASSERT(0)

#define	OSL_PCI_READ_CONFIG(osh, offset, size)	\
	(0)
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val)	\
	ASSERT(0)

#define OSL_PCI_BUS(osh)	(0)
#define OSL_PCI_SLOT(osh)	(0)

/* register access macros */

#ifdef BCMSDIO
#define wreg32(r, v)	bcmsdh_reg_write(NULL, (uint32)r, 4, (v))
#define rreg32(r)	bcmsdh_reg_read(NULL, (uint32)r, 4)
#define wreg16(r, v)	bcmsdh_reg_write(NULL, (uint32)r, 2, (v))
#define rreg16(r)	bcmsdh_reg_read(NULL, (uint32)r, 2)
#define wreg8(r, v)	bcmsdh_reg_write(NULL, (uint32)r, 1, (v))
#define rreg8(r)	bcmsdh_reg_read(NULL, (uint32)r, 1)
#else
#define wreg32(r, v)	(*(volatile uint32 *)(r) = (uint32)(v))
#define rreg32(r)	(*(volatile uint32 *)(r))
#define wreg16(r, v)	(*(volatile uint16 *)(r) = (uint16)(v))
#define rreg16(r)	(*(volatile uint16 *)(r))
#define wreg8(r, v)	(*(volatile uint8 *)(r) = (uint8)(v))
#define rreg8(r)	(*(volatile uint8 *)(r))
#endif

#define R_REG(osh, r) ({ \
	__typeof(*(r)) __osl_v; \
	switch (sizeof(*(r))) { \
	case sizeof(uint8):	__osl_v = rreg8((r)); break; \
	case sizeof(uint16):	__osl_v = rreg16((r)); break; \
	case sizeof(uint32):	__osl_v = rreg32((r)); break; \
	} \
	__osl_v; \
})

#define W_REG(osh, r, v) do { \
	switch (sizeof(*(r))) { 	\
	case sizeof(uint8):	wreg8((r), (v)); break; \
	case sizeof(uint16):	wreg16((r), (v)); break; \
	case sizeof(uint32):	wreg32((r), (v)); break; \
	} \
} while (0)

#define	AND_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) | (v))

/* general purpose memory allocation */
#ifdef BCMDBG_MEM

#define	MALLOC(osh, size)	osl_debug_malloc((osh), (size), __LINE__, __FILE__)
#define	MFREE(osh, addr, size)	osl_debug_mfree((osh), (addr), (size), __LINE__, __FILE__)
#define	MALLOC_DUMP(osh, buf) osl_debug_memdump((osh), (buf))

extern void *osl_debug_malloc(osl_t *osh, uint size, int line, char* file);
extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, char* file);
struct bcmstrbuf;
extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *buf);

#else /* BCMDBG_MEM */

#define	MALLOC(osh, size)	osl_malloc((osh), (size))
#define	MFREE(osh, addr, size)	osl_mfree((osh), (addr), (size))
#define	MALLOC_DUMP(osh, buf, sz)

#endif /* BCMDBG_MEM */

#define	MALLOCED(osh)		osl_malloced((osh))
#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))

/* microsecond delay */
#define	OSL_DELAY(usec)		osl_delay(usec)

/* host/bus architecture-specific address byte swap */
/* XXX */
#define BUS_SWAP32(v)		(v)

IOSTime IOS_GetTimer(void);
/* get processor cycle count */
#define OSL_GETCYCLES(x)	((x) = IOS_GetTimer())	/* 1cycle = 0.59us */

#define OSL_ERROR(bcmerror)	osl_error(bcmerror)

/* dereference an address that may cause a bus exception */
#define BUSPROBE(val, addr)     (val = R_REG(NULL, addr))

/* allocate/free shared (dma-able) consistent (uncached) memory */
/* comment */
#define	DMA_CONSISTENT_ALIGN	4096

#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa))
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align, uint *tot, ulong *pap);
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, ulong pa);

/* map/unmap direction */
#define	DMA_TX			1	/* TX direction for DMA */
#define	DMA_RX			2	/* RX direction for DMA */

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) do {} while (0)

/* map/unmap physical to virtual I/O */
#define	REG_MAP(pa, size)	((void *)(pa))
#define	REG_UNMAP(va)

/* map/unmap shared (dma-able) memory */
/* XXX */
#define DMA_MAP(osh, va, size, direction, p, dmah) \
	({ASSERT(0); (NULL);})
#define DMA_UNMAP(osh, pa, size, direction, p, dmah) \
	ASSERT(0)

/* shared (dma-able) memory access macros */
#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	memset((r), '\0', (len))

/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ		2048

#define _IOBUF(x)		((IOSIobuf *)(x))

/* packet primitives */
#define	PKTGET(osh, len, send)		osl_pktget(osh, len)
#define	PKTFREE(osh, iob, send)		osl_pktfree(osh, _IOBUF(iob), send)
#define	PKTDATA(osh, iob)		IOS_IOB_DATA(_IOBUF(iob))
#define	PKTLEN(osh, iob)		IOS_IOB_DATA_LEN(_IOBUF(iob))
#define	PKTHEADROOM(osh, iob)		(IOS_IOB_DATA(_IOBUF(iob)) - IOS_IOB_HEAD(_IOBUF(iob)))
#define	PKTTAILROOM(osh, iob)		(IOS_IOB_END(_IOBUF(iob)) - IOS_IOB_TAIL(_IOBUF(iob)))
#define	PKTPUSH(osh, iob, bytes)	osl_pktpush(osh, _IOBUF(iob), bytes)
#define	PKTPULL(osh, iob, bytes)	osl_pktpull(osh, _IOBUF(iob), bytes)
#define	PKTDUP(osh, iob)		osl_pktdup(osh, _IOBUF(iob))
#define	PKTNEXT(osh, iob)		IOS_IOB_NEXT(_IOBUF(iob))
#define	PKTSETNEXT(osh, iob, x)		(IOS_IOB_NEXT(_IOBUF(iob)) = (x))
#define	PKTSETLEN(osh, iob, len)	(IOS_IOB_DATA_LEN(_IOBUF(iob)) = len)
#define	PKTTAG(iob)			((void *)(((IOSIobuf *)(iob))->priv))
#define	PKTLINK(iob)			IOS_IOB_LINK(_IOBUF(iob))
#define	PKTSETLINK(iob, x)		(IOS_IOB_LINK(_IOBUF(iob)) = (x))
#define	PKTPRIO(iob)			((_IOBUF(iob))->priority)
#define	PKTSETPRIO(iob, x)		((_IOBUF(iob))->priority = (x))
#define PKTSHARED(iob)                  (0)
#define PKTFRMNATIVE(osh, iob)  osl_pkt_frmnative(osh, iob)
#define PKTTONATIVE(osh, pkt)	osl_pkt_tonative(osh, pkt)
#define PKTSETPOOL(osh, m, x, y)	do {} while (0)
#define PKTPOOL(osh, m)			FALSE
#define PKTSHRINK(osh, m)		(m)

/* XXX */
#define PKTALLOCED(osh)		(0)
#ifdef BCMDBG_PKT /* pkt logging for debugging */
#define PKTLIST_DUMP(osh, buf) 		osl_pktlist_dump(osh, buf)
#else /* BCMDBG_PKT */
#define PKTLIST_DUMP(osh, buf)
#endif /* BCMDBG_PKT */

#ifdef BCMDBG
#define PKTDUMP(osh, iob) \
	do { \
		printf("iob 0x%x, data 0x%x, head 0x%x, buflen %d, datalen %d, priv 0x%x\n", \
		(uintptr)iob, (uintptr)(_IOBUF(iob))->data, (uintptr)(_IOBUF(iob))->head, \
		(_IOBUF(iob))->bufLen, (_IOBUF(iob))->dataLen, (uintptr)(_IOBUF(iob))->priv); \
	} while (0);

#else

#define PKTDUMP(osh, iob)

#endif /* BCMDBG */

static INLINE void *
osl_pktpush(osl_t *osh, IOSIobuf *iob, uint bytes)
{
	ASSERT((IOS_IOB_DATA(iob) - bytes) >= IOS_IOB_HEAD(iob));
	IOS_IOB_DATA(iob) -= bytes;
	IOS_IOB_DATA_LEN(iob) += bytes;
	return IOS_IOB_DATA(iob);
}

static INLINE void *
osl_pktpull(osl_t *osh, IOSIobuf *iob, uint bytes)
{
	ASSERT(bytes <= IOS_IOB_DATA_LEN(iob));
	IOS_IOB_DATA(iob) += bytes;
	IOS_IOB_DATA_LEN(iob) -= bytes;
	return IOS_IOB_DATA(iob);
}

extern void osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)

extern osl_t *osl_attach(uint bus, uint slot, uint func, uint bustype, bool pkttag);
extern void osl_detach(osl_t *osh);

extern void *osl_malloc(osl_t *osh, uint size);
extern void osl_mfree(osl_t *osh, void *addr, uint size);
extern uint osl_malloced(osl_t *osh);
extern uint osl_malloc_failed(osl_t *osh);
extern int osl_error(int bcmerror);
extern void osl_delay(uint usec);
extern void *osl_pktdup(osl_t *osh, IOSIobuf *iob);
extern void *osl_pktget(osl_t *osh, uint len);
extern void osl_pktfree(osl_t *osh, void *p, bool send);
extern struct IOSIobuf *osl_pkt_tonative(osl_t *osh, void *pkt);
extern void *osl_pkt_frmnative(osl_t *osh, IOSIobuf *iob);
extern uint osl_pci_read_config(void *osh, uint offset, uint size);
extern void osl_pci_write_config(void *osh, uint offset, uint size, uint val);
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);
extern uint pci_config_read(uint bus, uint slot, uint fun, uint addr);
extern uint pci_config_write(uint bus, uint slot, uint fun, uint addr, uint data);

/* XXX */
extern int sprintf(char *buf, const char *fmt, ...);
extern char *strcpy(char *dest, const char *src);
extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern char *strcat(char *d, const char *s);

/* Global ASSERT type */
extern uint32 g_assert_type;
#endif	/* _iopos_osl_h_ */
