/*
 * NetBSD  2.0 OS Independent Layer
 *
 * $Copyright (C) 2001-2005 Broadcom Corporation$
 *
 * $Id: bsd_osl.h,v 13.34.8.1 2010-10-19 02:09:05 kiranm Exp $
 */

#ifndef _bsd_osl_h_
#define _bsd_osl_h_

#include <typedefs.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <sys/resource.h>
#include <machine/param.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <sys/mbuf.h>

/* The magic cookie */
#define OS_HANDLE_MAGIC		0x1234abcd /* Magic number  for osl_t */

/* Assert */
#ifdef BCMDBG_ASSERT
#define ASSERT(exp) \
	do { if (!(exp)) osl_assert(#exp, __FILE__, __LINE__); } while (0)
extern void osl_assert(const char *exp, const char *file, int line);
#else
#define	ASSERT(exp)		do {} while (0)
#endif /* BCMDBG_ASSERT */

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size) \
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val) \
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint size, uint offset);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCMCIA attribute space access macros, not suppotred */
#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) 	ASSERT(0)
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) 	ASSERT(0)

/* OSL initialization */
extern osl_t *osl_attach(void *pdev, bool pkttag, const char *dev_name, bus_space_tag_t space,
                         bus_space_handle_t handle, uint8 *vaddr);
extern void osl_detach(osl_t *osh);

extern void osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)

/* Host/bus architecture-specific byte swap */
#ifndef IL_BIGENDIAN
#define BUS_SWAP32(v)		(v)
#else
#define BUS_SWAP32(v)		htol32(v) /* Apple's target platform needs it */
#endif

#ifndef bsd_osl_c
/* Undefine the generic BSD kernel MALLOC and MFREE macros to avoid clash
 *
 * Do this only if we are not in bsd_osl.c itself.
 */
#undef MALLOC
#undef MFREE

#ifdef BCMDBG_MEM
#define	MALLOC(osh, size)	osl_debug_malloc((osh), (size), __LINE__, __FILE__)
#define	MFREE(osh, addr, size)	osl_debug_mfree((osh), (addr), (size), __LINE__, __FILE__)
#define MALLOCED(osh)		osl_malloced((osh))
#define	MALLOC_DUMP(osh, b) osl_debug_memdump((osh), (b))

#else /* BCMDBG_MEM */

#define	MALLOC(osh, size)	osl_malloc((osh), (size))
#define	MFREE(osh, addr, size)	osl_mfree((osh), (addr), (size))
#define MALLOCED(osh)		osl_malloced((osh))

#endif	/* BCMDBG_MEM */
#endif /* bsd_osl_c */

#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))

extern void *osl_debug_malloc(osl_t *osh, uint size, int line, const char* file);
extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, const char* file);
struct bcmstrbuf;
extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *b);
extern void *osl_malloc(osl_t *osh, uint size);
extern void osl_mfree(osl_t *osh, void *addr, uint size);
extern uint osl_malloced(osl_t *osh);
extern uint osl_malloc_failed(osl_t *osh);

/* Allocate/free shared (dma-able) consistent memory */

#define	DMA_CONSISTENT_ALIGN	PAGE_SIZE

#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap), (dmah))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa), (dmah))

extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align_bits, uint *alloced,
	ulong *pap, osldma_t **dmah);
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, ulong pa, osldma_t **dmah);

/* Map/unmap direction */
#define	DMA_TX	1 	/* DMA TX flag */
#define	DMA_RX	2	/* DMA RX flag */

/* Map/unmap shared (dma-able) memory */

#define	DMA_MAP(osh, va, size, direction, p, dmah) \
	osl_dma_map((osh), (va), (size), (direction), (dmah))
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah) \
	osl_dma_unmap((osh), (pa), (size), (direction), (dmah))

extern uint osl_dma_map(osl_t *osh, void *va, uint size, int direction, hnddma_seg_map_t  *dmah);
extern void osl_dma_unmap(osl_t *osh, uint pa, uint size, int direction, hnddma_seg_map_t *dmah);

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) do {} while (0)

/* map/unmap physical to virtual, not supported */

#define	REG_MAP(pa, size)	((void *)NULL)
#define	REG_UNMAP(va)		ASSERT(0)

/* NetBSD needs 2 handles the bus_space_tag at attach time
 * and the bus_space_handle
 */
/* Pkttag flag should be part of public information */
struct osl_pubinfo {
	bus_space_tag_t 	space;
	bus_space_handle_t	handle;
	uint8 *vaddr;
};

#define OSL_PUB(osh) ((struct osl_pubinfo *)(osh))

/* IO bus mapping routines */
#define rreg32(osh, r)	bus_space_read_4(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				(bus_size_t)(((uintptr)(r)) - \
				((uintptr)(OSL_PUB(osh)->vaddr))))
#define rreg16(osh, r)	bus_space_read_2(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				(bus_size_t)(((uintptr)(r)) - \
				((uintptr)(OSL_PUB(osh)->vaddr))))
#define rreg8(osh, r)	bus_space_read_1(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				(bus_size_t)(((uintptr)(r)) - \
				((uintptr)(OSL_PUB(osh)->vaddr))))

#define wreg32(osh, r, v)	bus_space_write_4(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				  (bus_size_t)((uintptr)(r) - \
					((uintptr)(OSL_PUB(osh)->vaddr))), (uint32)(v))
#define wreg16(osh, r, v)	bus_space_write_2(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				  (bus_size_t)((uintptr)(r) - \
					((uintptr)(OSL_PUB(osh)->vaddr))), (uint16)(v))
#define wreg8(osh, r, v)	bus_space_write_1(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				  (bus_size_t)((uintptr)(r) - \
					((uintptr)(OSL_PUB(osh)->vaddr))), (uint8)(v))

#define	R_REG(osh, r)	((sizeof *(r) == sizeof (uint32))? rreg32((osh), (r)):\
			(uint32)((sizeof(*(r)) == sizeof(uint16))? rreg16((osh), (r)):\
			rreg8((osh), (r))))

#define W_REG(osh, r, v)     do {\
			if (sizeof *(r) == sizeof (uint32)) \
				wreg32((osh), (r), (v)); \
			else if (sizeof *(r) == sizeof (uint16))\
				wreg16((osh), (r), (v)); \
			else \
				wreg8((osh), (r), (v)); \
			} while (0)


#define	AND_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) | (v))

/* Shared memory access macros */
#define	R_SM(a)		*(a)
#define	W_SM(a, v)	(*(a) = (v))
#define	BZERO_SM(a, len)	bzero((char*)a, len)

/* Uncached/cached virtual address */
#define OSL_UNCACHED(va)	ASSERT(0)
#define OSL_CACHED(va)		ASSERT(0)

/* Get processor cycle count */
#ifdef __i386__
#define	OSL_GETCYCLES(x)	__asm__ __volatile__("rdtsc" : "=a" (x) : : "edx")
#else
#error	Platform not supported
#endif /* #ifdef __i386__ */

/* dereference an address that may target abort */
#define	BUSPROBE(val, addr)	osl_busprobe(&(val), (addr))
extern int osl_busprobe(uint32 *val, uint32 addr);

/* Microsecond delay */
#define	OSL_DELAY(usec)		DELAY((usec))

static INLINE void *
osl_pktlink(void *m)
{
	ASSERT(((struct mbuf *)(m))->m_flags & M_PKTHDR);
	return ((struct mbuf *)(m))->m_nextpkt;
}

static INLINE void
osl_pktsetlink(void *m, void *x)
{
	ASSERT(((struct mbuf *)(m))->m_flags & M_PKTHDR);
	((struct mbuf *)(m))->m_nextpkt = (struct mbuf *)(x);
}

static INLINE void *
osl_pkttag(void *m)
{
	ASSERT(((struct mbuf *)(m))->m_flags & M_PKTHDR);
	return (void *)M_GETCTX((struct mbuf *) m, struct mbuf *);
}

/* Packet primitives */
#define	PKTGET(osh, len, send)	osl_pktget((osh), (len), (send))
#define	PKTFREE(osh, m, send)	osl_pktfree((osh), (m), (send))
#define	PKTDATA(osh, m)		((struct mbuf *)(m))->m_data
#define	PKTDUP(osh, m)		osl_pktdup((osh), (m))
#define PKTHEADROOM(osh, m)	M_LEADINGSPACE((struct mbuf *)(m))
#define PKTTAILROOM(osh, m)	M_TRAILINGSPACE((struct mbuf *)(m))
#define	PKTNEXT(osh, m)		((struct mbuf *)(m))->m_next
#define	PKTSETNEXT(osh, m, x)	osl_pktsetnext((osh), (m), (x))
#define	PKTLEN(osh, m)		((struct mbuf *)((m)))->m_len
#define	PKTSETLEN(osh, m, len)	((struct mbuf *)((m)))->m_len = (len)
#define	PKTPUSH(osh, m, bytes)	osl_pktpush((osh), (m), (bytes))
#define	PKTPULL(osh, m, bytes)	osl_pktpull((osh), (m), (bytes))
#define	PKTTAG(m)		osl_pkttag((m))
#define	PKTLINK(m)		osl_pktlink((m))
#define	PKTSETLINK(m, x)	osl_pktsetlink((m), (x))
#define PKTFRMNATIVE(osh, m)	osl_pkt_frmnative((osh), (struct mbuf *)(m))
#define PKTTONATIVE(osh, p)	osl_pkt_tonative((osh), (p))
#define PKTSHARED(p)            MCLISREFERENCED((struct mbuf *)(p))
#define PKTALLOCED(osh)		(0)
#define PKTSETPOOL(osh, m, x, y)	do {} while (0)
#define PKTPOOL(osh, m)			FALSE
#ifdef BCMDBG_PKT
#define PKTLIST_DUMP(osh, buf)          ((void)buf)
#else /* BCMDBG_PKT */
#define PKTLIST_DUMP(osh, buf)
#endif /* BCMDBG_PKT */
#define PKTSHRINK(osh, m)	(m)

/* Native mbuf packet tag ids */

#define PACKET_TAG_BRCM			PCI_VENDOR_BROADCOM
#define PACKET_TAG_BRCM_PKTPRIO		PACKET_TAG_BRCM + 1

static INLINE uint
osl_pktprio(void *mbuf)
{

	struct m_tag	*mtag;
	struct mbuf 	*m  = (struct mbuf *)mbuf;

	ASSERT(m);
	ASSERT(m->m_flags & M_PKTHDR);

	mtag = m_tag_find(m, PACKET_TAG_BRCM_PKTPRIO, NULL);

	if (mtag == NULL)
		return 0;
	else
		return *(uint *)(mtag + 1);
}

/* Packet priority tag added if it did not exist before
 * m_tags will survive movement across interfaces.
 * Removed as part of mbuf free operation
 */
static INLINE void
osl_pktsetprio(void *mbuf, uint x)
{

	struct m_tag	*mtag;
	struct mbuf 	*m = (struct mbuf *)mbuf;


	ASSERT(m);
	ASSERT(m->m_flags & M_PKTHDR);

	/* Look for tag , if not present create it */

	mtag = m_tag_find(m, PACKET_TAG_BRCM_PKTPRIO, NULL);

	if (mtag == NULL) {
		mtag = m_tag_get(PACKET_TAG_BRCM_PKTPRIO,
			sizeof(uint), M_NOWAIT);
		if (mtag == NULL)
			return;
		/* Attach m_tag to mbuf */
		m_tag_prepend(m, mtag);
	}

	*(uint *)(mtag + 1) = x;
}

#define	PKTPRIO(m)		osl_pktprio((m))
#define	PKTSETPRIO(m, x)	osl_pktsetprio((m), (x))

/* OSL packet primitive functions  */
extern void *osl_pktget(osl_t *osh, uint len, bool send);
extern uchar *osl_pktpush(osl_t *osh, void *m, int bytes);
extern uchar *osl_pktpull(osl_t *osh, void *m, int bytes);
extern struct mbuf *osl_pkt_tonative(osl_t *osh, void *p);
extern void *osl_pkt_frmnative(osl_t *osh, struct mbuf *m);
extern void osl_pktfree(osl_t *osh, void *m, bool send);
extern void osl_pktsetlink(void *m, void *x);
extern void *osl_pktdup(osl_t *osh, void *m);
extern void osl_pktsetnext(osl_t *osh, void *m, void *x);

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	MCLBYTES /* packet size */

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	osl_pci_bus(osh)
#define OSL_PCI_SLOT(osh)	osl_pci_slot(osh)
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);

/* Translate bcmerrors into NetBSD errors */
#define OSL_ERROR(bcmerror)	osl_error(bcmerror)
extern int osl_error(int bcmerror);

/* Global ASSERT type */
extern uint32 g_assert_type;


#endif	/* _bsd_osl_h_ */
