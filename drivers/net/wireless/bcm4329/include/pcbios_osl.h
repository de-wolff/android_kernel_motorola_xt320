/*
 *  PCBIOS OS independent Layer
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: pcbios_osl.h,v 13.27.8.1 2010-10-19 02:09:05 kiranm Exp $
 */

#ifndef _pcbios_osl_h_
#define _pcbios_osl_h_

#include <typedefs.h>

#ifdef LINUX_USER_APP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else /* LINUX_USER_APP */
#include <bcmstdlib.h>
#endif /* LINUX_USER_APP */

#ifdef BCMDBG
extern void osl_assert(char *exp, char *file, int line);
#define ASSERT(exp) (((int)(exp)) ? : osl_assert(#exp, __FILE__, __LINE__))
#else /* !BCMDBG */
#define	ASSERT(exp)
#endif /* !BCMDBG */

#define wreg32(r, v)	(*(volatile uint32 *)(r) = (uint32)(v))
#define rreg32(r)	(*(volatile uint32 *)(r))
#define wreg16(r, v)	(*(volatile uint16 *)(r) = (uint16)(v))
#define rreg16(r)	(*(volatile uint16 *)(r))
#define wreg8(r, v)	(*(volatile uint8 *)(r) = (uint8)(v))
#define rreg8(r)	(*(volatile uint8 *)(r))

#ifdef IL_BIGENDIAN
#error "IL_BIGENDIAN defined: BigEndian Mode not support for PCBIOS OSL implementation"
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

#define	MALLOC(osh, size)	osl_malloc(size)
#define	MFREE(osh, addr, size)	osl_free(addr, size)
#define MALLOCED(osh)           (0)
#define MALLOC_FAILED(osh)      (0)
#define	MALLOC_DUMP(osh, b)

extern void *osl_malloc(uint size);
extern void osl_free(void *addr, uint size);

#define	OSL_DELAY(usec)	osl_usleep(usec)
extern void osl_usleep(unsigned int usec);

/* shared (dma-able) memory access macros */
#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	memset((r), '\0', (len))

/* **********Packet Processing Routines START ********* */

/* generic packet structure */
#define LBUFSZ		2048 /* LBuf size */
#define LBDATASZ	(LBUFSZ - sizeof(struct lbuf))
/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	LBDATASZ /* PKT Buf Size */

struct lbuf {
	struct lbuf	*next;		/* pointer to next lbuf if in a chain */
	struct lbuf	*link;		/* pointer to next lbuf if in a list */
	unsigned char	*head;		/* start of buffer */
	unsigned char	*end;		/* end of buffer */
	unsigned char	*data;		/* start of data */
	unsigned char	*tail;		/* end of data */
	unsigned int	len;		/* nbytes of data */
	uchar		pkttag[OSL_PKTTAG_SZ]; /* pkttag area */
};


/* packet primitives */
#define	PKTGET(osh, len, send)		((void*)osl_pktget((len), send))
#define	PKTFREE(osh, lb, send)		osl_pktfree((osh), (struct lbuf*)(lb), send)
#define	PKTDATA(osh, lb)		(((struct lbuf*)(lb))->data)
#define	PKTLEN(osh, lb)			(((struct lbuf*)(lb))->len)
#define PKTHEADROOM(osh, lb)		(PKTDATA(osh, lb)-(((struct lbuf*)(lb))->head))
#define PKTTAILROOM(osh, lb)		((((struct lbuf*)(lb))->end)-(((struct lbuf*)(lb))->tail))
#define	PKTNEXT(osh, lb)		(((struct lbuf*)(lb))->next)
#define	PKTSETNEXT(osh, lb, x)		(((struct lbuf*)(lb))->next = (struct lbuf*)(x))
#define	PKTSETLEN(osh, lb, len)		osl_pktsetlen((struct lbuf*)(lb), (len))
#define	PKTPUSH(osh, lb, bytes)		osl_pktpush((struct lbuf*)(lb), (bytes))
#define	PKTPULL(osh, lb, bytes)		osl_pktpull((struct lbuf*)(lb), (bytes))
#define	PKTDUP(osh, lb)			osl_pktdup((struct lbuf*)(lb))

#define	PKTLINK(lb)			(((struct lbuf*)(lb))->link)
#define	PKTSETLINK(lb, x)		(((struct lbuf*)(lb))->link = (struct lbuf*)(x))
#define	PKTPRIO(lb)			(0)
#define	PKTSETPRIO(lb, x)		do {} while (0)
#define	PKTTAG(lb)			(((void *) ((struct lbuf *)(lb))->pkttag))
#define PKTSHARED(lb)                   (0)
#define PKTALLOCED(osh)			(0)
#define PKTSETPOOL(osh, lb, x, y)	do {} while (0)
#define PKTPOOL(osh, lb)		FALSE
#define PKTLIST_DUMP(osh, buf)
#define PKTSHRINK(osh, m)		(m)
extern struct lbuf *osl_pktget(unsigned int len, bool send);
extern void osl_pktfree(osl_t *osh, struct lbuf *lb, bool send);
extern void osl_pktsetlen(struct lbuf *lb, unsigned int len);
extern unsigned char *osl_pktpush(struct lbuf *lb, unsigned int bytes);
extern unsigned char *osl_pktpull(struct lbuf *lb, unsigned int bytes);
extern struct lbuf *osl_pktdup(struct lbuf *lb);
extern int osl_error(int bcmerror);
extern void *osl_attach(ulong, ulong, ulong, uint, uint, uint);
extern void osl_detach(void *osh);

extern void osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)

/* **********Packet Processing Routines END ********* */

#define	REG_MAP(pa, size)	((void *)(uintptr)(pa))
#define	REG_UNMAP(va)

#define	DMA_TX	1 /* DMA_TX */
#define	DMA_RX	2 /* DMA_RX */

/* PCI Access routines */

#define	OSL_PCI_READ_CONFIG(osh, offset, size)	\
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val)	\
	osl_pci_write_config((osh), (offset), (size), (val))

extern unsigned int osl_pci_read_config(void *osh, unsigned int size, unsigned int offset);
extern void osl_pci_write_config(void *osh, unsigned int offset,
	unsigned int size, unsigned int val);
extern unsigned int pci_config_read(unsigned int bus, unsigned int slot,
	unsigned int fun, unsigned int addr);
extern unsigned int pci_config_write(unsigned int bus, unsigned int slot,
	unsigned int fun, unsigned int addr, unsigned int data);

#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) ASSERT(0)
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) ASSERT(0)

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) do {} while (0)


#define OSL_PCI_BUS(osh)        (0)
#define OSL_PCI_SLOT(osh)       (0)

int32 osl_timeinms();

/* Global ASSERT type */
extern uint32 g_assert_type;
#endif	/* _pcbios_osl_h_ */
