/*
 * EFI boot loader OS Abstraction Layer.
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: efi_osl.h,v 13.19.8.1 2010-10-19 02:09:05 kiranm Exp $
 */

#ifndef _efi_osl_h_
#define _efi_osl_h_

#include <typedefs.h>
#include <bcmstdlib.h>
#include "Tiano.h"
#include "EfiDriverLib.h"
#include "Pci22.h"
#include "EfiPrintLib.h"

#include EFI_PROTOCOL_DEFINITION(PciIo)

/* Some standard defines using EFI Driver library. This is important to keep final size small */
#define strstr(s, find) EfiAsciiStrStr((CHAR8*)s, (CHAR8*)find)
#define strcat(dst, src) EfiAsciiStrCat((CHAR8*)(dst), (CHAR8*)(src))
#define strcmp(s1, s2)  EfiAsciiStrCmp((CHAR8*)(s1), (CHAR8*)(s2))
#define strlen(s) (uint)EfiAsciiStrLen((CHAR8*)(s))
#define strcpy(dst, src) EfiAsciiStrCpy((CHAR8*)(dst), (CHAR8*)(src))
#define strncpy(dst, src, len) EfiAsciiStrnCpy((CHAR8*)(dst), (CHAR8*)(src), (len))
#define bcopy(src, dst, len) EfiCopyMem((VOID *)(dst), (VOID *)(src), (len))
#define bzero(b, len) EfiZeroMem((VOID*)(b), (len))
#define bcmp(b1, b2, len) EfiCompareMem((VOID *)(b1), (VOID*)(b2), (len))
#define putc(c) Aprint("%c", c)
#define memcmp(s1, s2, n) EfiCompareMem((VOID*)(s1), (VOID*)(s2), (n))

extern void *memset(void *dest, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);

/* The magic cookie */
/* XXX Make a GUID */
#define OS_HANDLE_MAGIC		0x1234abcd /* Magic number  for osl_t */

/* XXX Do this for now during bringup */
#undef ASSERT
#ifndef BCMDBG
#undef ASSERT
#define ASSERT(exp) do { } while (0)
#endif

/* Implemented by EFI Library */
/* assert and panic */
#ifdef BCMDBG_ASSERT
extern void osl_assert(char *exp, char *file, int line);
#define ASSERT(exp) \
	do { if (!(exp)) osl_assert(#exp, __FILE__, __LINE__); } while (0)
extern void osl_assert(char *exp, char *file, int line);
#endif /* BCMDBG_ASSERT */

#ifdef BCMINTERNAL
extern CHAR16 EFI_GetKey();
#define PAUSE EFI_GetKey();
#endif

/* PCMCIA attribute space access macros */
#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) ASSERT(0)
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) ASSERT(0)

/* PCI configuration space access macros */
/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size) \
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val) \
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint size, uint offset);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	osl_pci_bus(osh)
#define OSL_PCI_SLOT(osh)	osl_pci_slot(osh)
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);

/* Host/bus architecture-specific byte swap */
#ifndef IL_BIGENDIAN
#define BUS_SWAP32(v)		(v)
#else
#define BUS_SWAP32(v)		htol32(v) /* Apple's target platform needs it */
#endif

struct osl_pubinfo {
	uint8 *vaddr;
};

#define OSL_PUB(osh) ((struct osl_pubinfo *)(osh))
#define PCIBARIDX 0   /* PCI Bar to be used for memory-mapped IO operations */
#define PCIREADCNT 1  /*  Count for Memory-mapped IO operations (always single ) */
/* register access macros */
/* XXX BIGENDIAN?? */
/* IO bus mapping routines */
extern UINT32 osl_reg_rd32(osl_t *osh, UINT64 offset);
extern UINT16 osl_reg_rd16(osl_t *osh, UINT64 offset);
extern UINT8 osl_reg_rd8(osl_t *osh, UINT64 offset);

extern void osl_reg_wr32(osl_t *osh, UINT64 offset, UINT32 Value32);
extern void osl_reg_wr16(osl_t *osh, UINT64 offset, UINT16 Value16);
extern void osl_reg_wr8(osl_t *osh, UINT64 offset, UINT8 Value8);

#define rreg(_w, osh, r) \
	osl_reg_rd##_w(osh, (UINT64)(((uintptr)(r)) - ((uintptr)(OSL_PUB(osh)->vaddr))))

#define wreg(_w, osh, r, v) \
	osl_reg_wr##_w(osh, (UINT64)(((uintptr)(r)) - ((uintptr)(OSL_PUB(osh)->vaddr))), (v))

#define	R_REG(osh, r)	((sizeof *(r) == sizeof (uint32))? rreg(32, (osh), (r)): \
			 ((sizeof(*(r)) == sizeof(uint16))? rreg(16, (osh), (r)): \
			  rreg(8, (osh), (r))))

#define W_REG(osh, r, v)     do {\
			if (sizeof *(r) == sizeof (uint32)) \
				wreg(32, (osh), (r), (UINT32)(v));	\
			else if (sizeof *(r) == sizeof (uint16))\
				wreg(16, (osh), (r), (UINT16)(v));	\
			else \
				wreg(8, (osh), (r), (UINT8)(v));	\
			} while (0)

#define	AND_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) | (v))

extern osl_t * osl_attach(IN EFI_HANDLE pdev,
                          IN uint8 *vaddr,
                          IN EFI_PCI_IO_PROTOCOL  *PciIoIntf,
                          IN uint ntxs,
                          IN uint nrxs,
                          BOOLEAN piomode,
                          IN struct spktq *txdone);

/* XXX This is used when folding packet implemenation ndshared.c. It's little bit overkill for
 * EFI for now. It's actually required to be done for breaking sequencing issue for 30-bit DMA
 * Capability with shared lb alloc
 */
extern EFI_STATUS osl_pktinit(osl_t *osh);

extern void osl_detach(osl_t *osh);

extern void osl_pktfreesetcb(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);

#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfreesetcb((osh), (tx_fn), (tx_ctx))

#ifdef BCMDBG_MEM

#define	MALLOC(osh, size)	osl_debug_malloc((osh), (size), __LINE__, __FILE__)
#define	MFREE(osh, addr, size)	osl_debug_mfree((osh), (addr), (size), __LINE__, __FILE__)
#define MALLOCED(osh)		osl_malloced((osh))
#define	MALLOC_DUMP(osh, b) osl_debug_memdump((osh), (b))
extern void *osl_debug_malloc(osl_t *osh, uint size, int line, char* file);
extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, char* file);
struct bcmstrbuf;
extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *b);

#else
/* general purpose memory allocation */
#define	MALLOC(osh, size)	osl_malloc((osh), (size))
#define	MFREE(osh, addr, size)	osl_mfree((osh), (addr), (size))
#define	MALLOCED(osh)		osl_malloced((osh))
#define	MALLOC_DUMP(osh, b)	do {} while 0
#endif /* BCMDBG_MEM */

#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))
extern void * osl_malloc(osl_t *osh, uint size);
extern void osl_mfree(osl_t *osh, void *addr, uint size);
extern uint osl_malloced(osl_t *osh);
extern uint osl_malloc_failed(osl_t *osh);

/* uncached/cached virtual address */
#define	OSL_UNCACHED(va)	(va)
#define	OSL_CACHED(va)		(va)

/* get processor cycle count */
#define OSL_GETCYCLES(x)	((x) = 0)

/* microsecond delay */
#define	OSL_DELAY(usec)		gBS->Stall(usec)

#define OSL_ERROR(bcmerror)	osl_error(bcmerror)

/* Convenient macro for pretty-printing helpful message EFI_STATUS */
#define EFI_ERRMSG(_Status, _Tag) \
	DEBUG((EFI_D_ERROR, "Tag: %a Error Status:%r\n", (_Tag), (_Status)))

/* map/unmap physical to virtual I/O */
#define	REG_MAP(pa, size)	((void *)NULL)
#define	REG_UNMAP(va)		do { } while (0)

/* map/unmap direction */
#define	DMA_TX			1	/* TX direction for DMA */
#define	DMA_RX			2	/* RX direction for DMA */

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) osl_dmaddrwidth((osh), (addrwidth))
extern void osl_dmaddrwidth(osl_t *osh, uint addrwidth);

#define	PKTBUFSZ	LBDATASZ

/* dereference an address that may cause a bus exception */
#define	BUSPROBE(val, addr)	osl_busprobe(&(val), (uint32)(addr))
extern int osl_busprobe(uint32 *val, uint32 addr);

/* allocate/free shared (dma-able) consistent (uncached) memory */
#define	DMA_CONSISTENT_ALIGN	4096		/* 4k alignment */
#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap), (dmah))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa), (dmah))
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align, uint *tot,
	ulong *pap, osldma_t **dmah);
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, ulong pa, osldma_t **dmah);

#define	DMA_MAP(osh, va, size, direction, p, dmah) \
		osl_dma_map((osh), (p))
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah) \
		osl_dma_unmap((osh), (direction), (p))
extern uint osl_dma_map(osl_t *osh, void *p);
extern void osl_dma_unmap(osl_t *osh, int direction, void *p);

/* shared (dma-able) memory access macros */
#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	EfiZeroMem((VOID*)(r), (len))

/* Map to be retained between 'Map' and 'Unmap' API used for Efi */
struct osl_dmainfo {
	VOID *Map;
};

/* shared memory page bookkeeping */
typedef struct {
	uint8 *va;			/* kernel virtual address */
	EFI_PHYSICAL_ADDRESS	pa;	/* physical address */
	osldma_t PageMap;		/* Map to be saved */
} page_t;

/*
 * Local packet buffer structure.
 *
 * lbufs are 2Kbytes in size to contain a full sized ETHER_MAX_LEN frame,
 * not cross 4Kbyte physical address DMA boundaries, and be efficiently carved
 * from 4Kbyte pages from shared (DMA-able) memory.
 *
 */

#define LBUFSZ		2048	/* largest reasonable packet buffer, driver uses for ethernet MTU */
#define LBDATASZ	(LBUFSZ - sizeof(struct lbuf))
#define BPP		(EFI_PAGE_SIZE / LBUFSZ)	/* number of buffers per page */

struct lbfree;				/* forward declaration */

/* Construct intermediate packet that holds information about EFI Buffer */
/* This intermediate packet would make it easy to use pktq implementation in
 * bcmutils thereby reducing library size.
 */
typedef struct EFI_PKT {
	struct EFI_PKT *Link;
	/* Following fields are input for Tx pkt, output for Rx pkt */
	void *Buffer;
	UINTN HeaderSize;
	UINTN BufferSize;
	EFI_MAC_ADDRESS *SrcAddr;
	EFI_MAC_ADDRESS *DestAddr;
	UINT16 *Protocol;
};

struct lbuf {
	struct lbuf	*link;	/* pointer to next lbuf if in a list NEEDS TO BE FIRST ELEMENT */
	struct lbuf	*next;	/* pointer to next lbuf if in a chain */
	uchar		*head;	/* start of buffer */
	uchar		*end;	/* end of buffer */
	uchar		*data;	/* start of data */
	uchar		*tail;	/* end of data */
	uint		len;	/* nbytes of data */
	uint		priority;	/* packet priority */
	ulong		pa;	/* low 32bits of physical address of head */
	struct EFI_PKT *	p;		/* EFI packet associated with us */
	uchar		pkttag[OSL_PKTTAG_SZ]; /* pkttag area  */
	struct lbfree	*l;		/* lbuf's parent freelist */
};

#ifdef BCMWLUNDI
typedef VOID (*DELAY_API)(UINT64, UINTN);
typedef VOID (*VIRT2PHY_API)(UINT64, UINT64, UINT64);
typedef VOID (*BLOCK_API)(UINT64, UINT32);
typedef VOID (*MEMIO_API)(UINT64, UINT8, UINT8, UINT64, UINT64);

typedef VOID (*MAP_API)(UINT64, UINT64, UINT32, UINT32, UINT64);
typedef VOID (*UNMAP_API)(UINT64, UINT64, UINT32, UINT32, UINT64);
typedef VOID (*SYNC_API)(UINT64, UINT64, UINT32, UINT32, UINT64);
#endif /* BCMWLUNDI */

/*
 * Simple, static list of free lbufs.
 * For reasonable memory utilization, we alloc and split whole pages
 * into 2Kbyte buffers.
 */
typedef struct lbfree {
	struct lbuf	*free;		/* the linked list */
	uint		total;		/* # total packets */
	uint		count;		/* # packets currently on free */
	uint		size;		/* # bytes packet buffer memory */
	page_t		*pages;		/* pointer to array of backing pages */
	uint		npages;		/* # pages in pages vector */
} lbfree_t;

extern void* osl_pktget(osl_t *osh, uint len, bool send);
extern void osl_pktfree(osl_t *osh, void *p, bool send);
extern void osl_pktsetlen(osl_t *osh, void *p, uint len);
extern void* osl_pktpush(osl_t *osh, struct lbuf *lb, uint bytes);
extern void * osl_pktpull(osl_t *osh, struct lbuf *lb, uint bytes);
extern void* osl_pktdup(osl_t *osh, struct lbuf *lb);
extern void * osl_pkt_frmnative(osl_t *osh,  struct EFI_PKT *snp_pkt);
extern struct EFI_PKT * osl_pkt_tonative(osl_t *osh, struct lbuf* lb);

/* packet primitives */
#define	PKTGET(osh, len, send)	osl_pktget((osh), (len), (send))
#define	PKTFREE(osh, p, send)	osl_pktfree((osh), (p), (send))
#define	PKTDATA(osh, p)		((struct lbuf *)(p))->data
#define	PKTLEN(osh, p)		((struct lbuf *)(p))->len
#define PKTHEADROOM(osh, p)	(PKTDATA(osh, p)-(((struct lbuf *)(p))->head))
#define PKTTAILROOM(osh, p)	((((struct lbuf *)(p))->end)-(((struct lbuf *)(p))->tail))
#define	PKTNEXT(osh, p)		((struct lbuf *)(p))->next
#define	PKTSETNEXT(osh, p, x)	((struct lbuf *)(p))->next = (struct lbuf*)(x)
#define	PKTSETLEN(osh, p, len)	osl_pktsetlen((osh), (p), (uint)(len))
#define	PKTPUSH(osh, p, bytes)	osl_pktpush((osh), (p), (uint)(bytes))
#define	PKTPULL(osh, p, bytes)	osl_pktpull((osh), (p), (uint)(bytes))
#define	PKTDUP(osh, p)		osl_pktdup((osh), (p))
#define	PKTTAG(p)		(((void *) ((struct lbuf *)(p))->pkttag))
#define	PKTLINK(p)		(void*)(*((uint *)p))
#define	PKTSETLINK(p, x)	(*((uint *)p)) = (uintptr)(x)
#define	PKTPRIO(p)		((struct lbuf *)(p))->priority
#define	PKTSETPRIO(p, x)	((struct lbuf *)(p))->priority = (x)
#define	PKTFRMNATIVE(osh, x)	osl_pkt_frmnative((osh), (x))
#define	PKTTONATIVE(osh, lb)	osl_pkt_tonative((osh), (struct lbuf *)(lb))
#define PKTSHARED(p)		(0)
#define PKTALLOCED(osh)		(0)
#define PKTSETPOOL(osh, p, x, y)	do {} while (0)
#define PKTPOOL(osh, p)			FALSE
#define PKTLIST_DUMP(osh, buf)
#define PKTSHRINK(osh, m)		(m)

extern EFI_STATUS osl_error(int bcmerror);

/* Global ASSERT type */
extern uint32 g_assert_type;
#endif	/* _efi_osl_h_ */
