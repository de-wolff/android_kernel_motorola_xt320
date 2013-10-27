/*
 * eCos 2.0 OS Independent Layer
 *
 * Copyright(c) 2006 Broadcom Corp.
 * $Id: ecos_osl.h,v 1.13.8.1 2010-10-19 02:09:05 kiranm Exp $
 */
#ifndef __ECOS_OSL_H__
#define __ECOS_OSL_H__

#include <typedefs.h>
#include <sys/param.h>

#define	ASSERT(exp)		do {} while (0)

#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) 	bzero(buf, size)
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size)	do {} while (0)

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size)	\
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val)	\
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint offset, uint size);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	(0)
#define OSL_PCI_SLOT(osh)	(0)

/* Host/Bus architecture specific swap. Noop for little endian systems, 
 * possible swap on big endian
 */

#define BUS_SWAP32(v)		(v)


#undef free
#undef malloc

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MALLOC
#undef MALLOC
#endif /* MALLOC */
#define	MALLOC(osh, size)	malloc(size)
#ifdef MFREE
#undef MFREE
#endif /* MFREE */
#define	MFREE(osh, addr, size)	free(addr)
#define	MALLOCED(osh)		(0)
#define	MALLOC_FAILED(osh)	(0)
#define	MALLOC_DUMP(osh, b)

#ifdef WL_NVRAM
extern char *CFG_wlget(char *name);
extern int CFG_wlset(char *name, char *value);

#ifdef nvram_safe_get
#undef nvram_safe_get
#endif /* nvram_safe_get */
#define nvram_safe_get CFG_wlget

#ifdef nvram_get
#undef nvram_get
#endif /* nvram_get */
#define nvram_get CFG_wlget

#ifdef nvram_set
#undef nvram_set
#endif /* nvram_set */
#define nvram_set CFG_wlset
#endif /* WL_NVRAM */

/* register access macros */
#define wreg32(r, v)	(*(volatile uint32 *)(r) = (v))
#define rreg32(r)	(*(volatile uint32 *)(r))
#define wreg16(r, v)	(*(volatile uint16 *)(r) = (v))
#define rreg16(r)	(*(volatile uint16 *)(r))
#define wreg8(r, v)	(*(volatile uint8 *)(r) = (v))
#define rreg8(r)	(*(volatile uint8 *)(r))

/* register access macros */
#define R_REG(osh, r) (\
	sizeof(*(r)) == sizeof(uint8) ? rreg8((volatile uint8 *)(r)) : \
	sizeof(*(r)) == sizeof(uint16) ? rreg16((volatile uint16 *)(r)) : \
	rreg32((volatile uint32 *)(r)) \
)
#define W_REG(osh, r, v) do { \
	switch (sizeof(*(r))) { \
	case sizeof(uint8): wreg8((volatile uint8 *)(r), (uint8)(v)); break; \
	case sizeof(uint16): wreg16((volatile uint16 *)(r), (uint16)(v)); break; \
	case sizeof(uint32): wreg32((volatile uint32 *)(r), (uint32)(v)); break; \
	} \
} while (0)

#define	AND_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) | (v))


/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)	bcopy((char*)(src), (char*)(dst), (len))
#define	bcmp(b1, b2, len)	bcmp((char*)(b1), (char*)(b2), (len))
#define	bzero(b, len)		bzero((char*)b, (len))

/* uncached/cached virtual address */
#ifdef __mips__
#define OSL_UNCACHED(va)	(((uint32)(va) & 0x1fffffff) | 0xa0000000)
#define OSL_CACHED(va)		(((uint32)(va) & 0x1fffffff) | 0x80000000)
#define IS_KSEG2(_x)		(((_x) & 0xC0000000) == 0xC0000000)
#else
#define IS_KSEG2(_x)		0
#define OSL_UNCACHED(va)	(va)
#define OSL_CACHED(va)		(va)
#endif /* __mips__ */

/* dereference and address that may cause a bus exception */
#define BUSPROBE(x, y)		({ (x) = R_REG(NULL, (y)); 0; })

/* map/unmap physical to virtual */
/* 	- assume a 1:1 mapping if KSEG2 addresses are used */
#define	REG_MAP(pa, size)	((void *)(IS_KSEG2(pa) ? (pa) : OSL_UNCACHED(pa)))
#define	REG_UNMAP(va)		do {} while (0) /* nop */


#define DMA_CONSISTENT_ALIGN	sizeof(int)

#define DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah)	\
	osl_dma_alloc_consistent(osh, size, (align), (tot), pap)
#define DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent(osh, size, (void*)va, pa)
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, ulong *pap);
extern void osl_dma_free_consistent(osl_t *osh, uint size, void *va, ulong pa);

#define DMA_TX			0		/* TX direction */
#define DMA_RX			1		/* RX direction */
#define DMA_MAP(osh, va, size, direction, p, dmah) \
	osl_dma_map(osh, (void*)va, size, direction)
#define DMA_UNMAP(osh, pa, size, direction, p, dmah)	/* nop */
extern void *osl_dma_map(osl_t *osh, void *va, uint size, uint direction);

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth)	do {} while (0)

/* microsecond delay */
#define	OSL_DELAY(us)	udelay(us)
extern void udelay(int delay);

/* shared memory access macros */
#define	R_SM(a)		*(a)
#define	W_SM(a, v)	(*(a) = (v))
#define	BZERO_SM(a, len)	bzero((char*)a, len)


void*	osl_attach(void *pdev, uint bustype, bool pkttag);
void	osl_detach(osl_t *osh);
void	osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
void*	osl_pktget(osl_t *osh, uint len, bool send);
void	osl_pktfree(osl_t *osh, void *m, bool send);
void*	osl_pktdup(osl_t *osh, void *m);
uchar*	osl_pktpush(void *m, int bytes);
uchar*	osl_pktpull(void *m, int bytes);
void*	osl_pkt_frmnative(osl_t *osh, void *m);
void*	osl_pkt_tonative(osl_t *osh, void *p);
void*	osl_pkttag_alloc(osl_t *osh);
void	osl_pktsettag(void *m, void *n);
void	osl_pkttag_free(osl_t *osh, void *pkttag);
uchar*	osl_pktdata(osl_t *osh, void *m);
uint	osl_pktlen(osl_t *osh, void *m);
void*	osl_pktnext(osl_t *osh, void *m);
void	osl_pktsetnext(osl_t *osh, void *m, void *n);
void	osl_pktsetlen(osl_t *osh, void *m, int dlen);
void*	osl_pktlink(void *m);
void	osl_pktsetlink(void *m, void *n);
void*	osl_pkttag(void *m);
uint	osl_pktheadroom(osl_t *osh, void *m);
uint	osl_pkttailroom(osl_t *osh, void *m);
bool	osl_ptkshared(void *m);
uint	osl_pktprio(void *m);
void	osl_pktsetprio(void *m, uint x);

/* packet primitives */
#define	PKTGET(osh, len, send)		osl_pktget(osh, len, send)
#define	PKTFREE(osh, m, send)		osl_pktfree(osh, m, send)
#define	PKTDATA(osh, m)			osl_pktdata(osh, m)
#define	PKTLEN(osh, m)			osl_pktlen(osh, m)
#define	PKTNEXT(osh, m)			osl_pktnext(osh, m)
#define	PKTSETNEXT(osh, m, n)		osl_pktsetnext(osh, m, n)
#define	PKTSETLEN(osh, m, dlen)		osl_pktsetlen(osh, m, dlen)
#define	PKTPUSH(osh, m, nbytes)		osl_pktpush(m, nbytes)
#define	PKTPULL(osh, m, nbytes)		osl_pktpull(m, nbytes)
#define	PKTDUP(osh, m)			osl_pktdup(osh, m)
#define	PKTTAG(m)			osl_pkttag((m))
#define	PKTLINK(m)			osl_pktlink(m)
#define	PKTSETLINK(m, n)		osl_pktsetlink(m, n)

#define PKTFRMNATIVE(osh, m)		osl_pkt_frmnative(osh, m)
#define PKTTONATIVE(osh, p)		osl_pkt_tonative(osh, p)

#define	PKTPRIO(m)			osl_pktprio((m))
#define	PKTSETPRIO(m, n)		osl_pktsetprio((m), n)

#define PKTHEADROOM(osh, m)		osl_pktheadroom(osh, m)
#define PKTTAILROOM(osh, m)		osl_pkttailroom(osh, m)
#define PKTSHARED(m)			osl_ptkshared(m)

#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)
#define PKTALLOCED(osh)				(0)
#define PKTSETPOOL(osh, m, x, y)	do {} while (0)
#define PKTPOOL(osh, m)			FALSE
#define PKTSHRINK(osh, m)		(m)


/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	2048		/* largest reasonable packet buffer,
					 * driver uses for ethernet MTU
					 */

#ifdef BCMDBG_PKT
#define PKTLIST_DUMP(osh, buf) 		((void) buf)
#else /* BCMDBG_PKT */
#define PKTLIST_DUMP(osh, buf)
#endif /* BCMDBG_PKT */

/* Global ASSERT type */
extern uint32 g_assert_type;

#endif /* __ECOS_OSL_H__ */
