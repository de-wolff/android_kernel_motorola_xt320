/*
 * Linux User Mode Independent Layer
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: usermode_osl.h,v 13.6.10.1 2010-10-19 02:09:05 kiranm Exp $
 */


#ifndef _usermode_osl_h_
#define _usermode_osl_h_

#include <typedefs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Global ASSERT type */
extern uint32 g_assert_type;

extern osl_t *osl_attach(void *pdev, int dmamemfd, uint bustype, bool pkttag, uint bus,
	uint slot, uint func);
extern void osl_detach(osl_t *osh);

extern void wl_syslog(char *fmt, ...);
#define printf wl_syslog

#ifdef BCMDBG_ASSERT
	#define ASSERT(exp) \
	  do { if (!(exp)) osl_assert(#exp, __FILE__, __LINE__); } while (0)
extern void osl_assert(char *exp, char *file, int line);
#else
	#ifdef __GNUC__
		#define GCC_VERSION \
			(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
		#if GCC_VERSION > 30100
			#define ASSERT(exp)	do {} while (0)
		#else
			#define ASSERT(exp)
		#endif
	#endif /* __GNUC__ */
#endif /* BCMDBG_ASSERT */


#define	OSL_DELAY(usec)		osl_delay(usec)
extern void osl_delay(uint usec);

#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) \
	osl_pcmcia_read_attr((osh), (offset), (buf), (size))
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) \
	osl_pcmcia_write_attr((osh), (offset), (buf), (size))
extern void osl_pcmcia_read_attr(osl_t *osh, uint offset, void *buf, int size);
extern void osl_pcmcia_write_attr(osl_t *osh, uint offset, void *buf, int size);

#define	OSL_PCI_READ_CONFIG(osh, offset, size) \
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val) \
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint offset, uint size);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);


#define OSL_PCI_BUS(osh)	osl_pci_bus(osh)
#define OSL_PCI_SLOT(osh)	osl_pci_slot(osh)
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);


typedef struct {
	bool pkttag;
	uint pktalloced;
	bool mmbus;
	pktfree_cb_fn_t tx_fn;
	void *tx_ctx;
} osl_pubinfo_t;

#define PKTFREESETCB(osh, _tx_fn, _tx_ctx)		\
	do {						\
	   ((osl_pubinfo_t*)osh)->tx_fn = _tx_fn;	\
	   ((osl_pubinfo_t*)osh)->tx_ctx = _tx_ctx;	\
	} while (0)


#define BUS_SWAP32(v)		(v)

#ifndef PAGE_SIZE
#define	PAGE_SIZE	4096
#endif

#define MALLOC_DUMP(osh, b)
#define MALLOC(osh, size)	osl_malloc((osh), (size))
#define MFREE(osh, addr, size)	osl_mfree((osh), (addr), (size))
#define MALLOCED(osh)		osl_malloced((osh))
extern void *osl_malloc(osl_t *osh, uint size);
extern void osl_mfree(osl_t *osh, void *addr, uint size);
extern uint osl_malloced(osl_t *osh);

#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))
extern uint osl_malloc_failed(osl_t *osh);

#define	DMA_CONSISTENT_ALIGN	osl_dma_consistent_align()
#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa))
extern uint osl_dma_consistent_align(void);
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align, uint *tot, ulong *pap);
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, ulong pa);

#define	DMA_TX	1
#define	DMA_RX	2

#define	DMA_MAP(osh, va, size, direction, p, dmah) \
	osl_dma_map((osh), (p))
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah) \
	osl_dma_unmap((osh), (p))
extern uint osl_dma_map(osl_t *osh, void *p);
extern void osl_dma_unmap(osl_t *osh, void *p);

#define OSL_DMADDRWIDTH(osh, addrwidth) do {} while (0)

#define SELECT_BUS_WRITE(osh, mmap_op, bus_op) mmap_op
#define SELECT_BUS_READ(osh, mmap_op, bus_op) mmap_op


#define OSL_SYSUPTIME()		(0)

#define OSL_ERROR(bcmerror)	osl_error(bcmerror)
extern int osl_error(int bcmerror);

#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))

#define wreg32(r, v)	(*(volatile uint32 *)(r) = (uint32)(v))
#define rreg32(r)	(*(volatile uint32 *)(r))
#define wreg16(r, v)	(*(volatile uint16 *)(r) = (uint16)(v))
#define rreg16(r)	(*(volatile uint16 *)(r))
#define wreg8(r, v)	(*(volatile uint8 *)(r) = (uint8)(v))
#define rreg8(r)	(*(volatile uint8 *)(r))

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

#define	AND_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) | (v))

#define OSL_UNCACHED(va)	osl_uncached((va))
extern void *osl_uncached(void *va);


#define OSL_GETCYCLES(x)	((x) = osl_getcycles())
extern uint osl_getcycles(void);

#define	REG_MAP(pa, size)	osl_reg_map((pa), (size))
#define	REG_UNMAP(va)		osl_reg_unmap((va))
extern void *osl_reg_map(uint32 pa, uint size);
extern void osl_reg_unmap(void *va);

#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	bzero((r), (len))


/* **********Packet Processing Routines START ********* */

#define LBDATASZ	2048
#define	PKTBUFSZ	2048  	/* visible form wlc.c */

struct lbuf {
	struct lbuf	*link;	/* pointer to next lbuf if in a list NEEDS TO BE FIRST ELEMENT */
	struct lbuf	*next;	/* pointer to next lbuf if in a chain */
	ulong		paddr;	/* physical address */

	uchar 		*vaddr;	/* virtual address */
	uchar		*end;	/* end of buffer */
	uchar		*data;	/* start of data */
	uint		len;	/* nbytes of data */
	uint		priority;	/* packet priority */
	uchar		pkttag[OSL_PKTTAG_SZ]; /* pkttag area  */
};


#define	PKTGET(osh, len, send)		osl_pktget((osh), (len))
#define	PKTFREE(osh, lb, send)		osl_pktfree((osh), (lb), (send))
#define	PKTDATA(osh, lb)		osl_pktdata((osh), (lb))
#define	PKTLEN(osh, lb)			osl_pktlen((osh), (lb))
#define PKTHEADROOM(osh, lb)		osl_pktheadroom((osh), (lb))
#define PKTTAILROOM(osh, lb)		osl_pkttailroom((osh), (lb))
#define	PKTNEXT(osh, lb)		osl_pktnext((osh), (lb))
#define	PKTSETNEXT(osh, lb, x)		osl_pktsetnext((lb), (x))
#define	PKTSETLEN(osh, lb, len)		osl_pktsetlen((osh), (lb), (len))
#define	PKTPUSH(osh, lb, bytes)		osl_pktpush((osh), (lb), (bytes))
#define	PKTPULL(osh, lb, bytes)		osl_pktpull((osh), (lb), (bytes))
#define	PKTDUP(osh, lb)			osl_pktdup((osh), (lb))
#define PKTTAG(lb)			osl_pkttag((lb))
#define PKTFRMNATIVE(osh, lb)		osl_pkt_frmnative((osh), (lb))
#define PKTTONATIVE(osh, pkt)		osl_pkt_tonative((osh), (pkt))
#define	PKTLINK(lb)			osl_pktlink((lb))
#define	PKTSETLINK(lb, x)		osl_pktsetlink((lb), (x))
#define	PKTPRIO(lb)			osl_pktprio((lb))
#define	PKTSETPRIO(lb, x)		osl_pktsetprio((lb), (x))
#define PKTSHARED(lb)			osl_pktshared((lb))
#define PKTALLOCED(osh)			osl_pktalloced((osh))
#define PKTSETPOOL(osh, lb, x, y)	do {} while (0)
#define PKTPOOL(osh, lb)		FALSE
#define PKTLIST_DUMP(osh, buf)
#define PKTSHRINK(osh, m)		(m)

extern void *osl_pktget(osl_t *osh, uint len);
extern void osl_pktfree(osl_t *osh, void *lb, bool send);
extern uchar *osl_pktdata(osl_t *osh, void *lb);
extern uint osl_pktlen(osl_t *osh, void *lb);
extern uint osl_pktheadroom(osl_t *osh, void *lb);
extern uint osl_pkttailroom(osl_t *osh, void *lb);
extern void *osl_pktnext(osl_t *osh, void *lb);
extern void osl_pktsetnext(void *lb, void *x);
extern void osl_pktsetlen(osl_t *osh, void *lb, uint len);
extern uchar *osl_pktpush(osl_t *osh, void *lb, int bytes);
extern uchar *osl_pktpull(osl_t *osh, void *lb, int bytes);
extern void *osl_pktdup(osl_t *osh, void *lb);
extern void *osl_pkttag(void *lb);
extern void *osl_pktlink(void *lb);
extern void osl_pktsetlink(void *lb, void *x);
extern uint osl_pktprio(void *lb);
extern void osl_pktsetprio(void *lb, uint x);
extern void *osl_pkt_frmnative(osl_t *osh, void *lb);
extern struct lbuf *osl_pkt_tonative(osl_t *osh, void *pkt);
extern bool osl_pktshared(void *lb);
extern uint osl_pktalloced(osl_t *osh);
extern uint pci_config_read(uint bus, uint slot, uint fun, uint addr);
extern uint pci_config_write(uint bus, uint slot, uint fun, uint addr, uint data);

#endif	/* _usermode_osl_h_ */
