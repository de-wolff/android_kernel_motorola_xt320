/*
 * NDIS OS Independent Layer
 *
 * Copyright(c) 2001 Broadcom Corp.
 * $Id: ndis_osl.h,v 1.120.8.1 2010-10-19 02:09:05 kiranm Exp $
 */

#ifndef _ndis_osl_h_
#define _ndis_osl_h_

#include <typedefs.h>
#include <ndshared.h>
#ifdef BCMSDIO
#include <bcmsdh.h>
#endif /* def BCMSDIO */

/* make WHQL happy */
#ifndef MEMORY_TAG
#define MEMORY_TAG 'MCRB'	/* BRCM reversed */
#endif /* MEMORY_TAG */
#ifdef BCMNDIS6
#define TIMER_TAG 'RMTB'	/* timer tag */
#endif /* BCMNDIS6 */
#ifdef WDM
#define NdisAllocateMemory(__va, __len, __dummy1, __dummy2) \
	NdisAllocateMemoryWithTag(__va, __len, (ULONG) MEMORY_TAG)
#endif /* WDM */

#ifndef BCMINTERNAL
#define USE_MS_ASSERT
#endif /* BCMINTERNAL */

#ifdef _PREFAST_
#define ASSERT(exp)	__assume(exp)
#else /* _PREFAST_ */
#ifndef USE_MS_ASSERT
#if defined(BCMDBG_ASSERT) || defined(BCMASSERT_LOG)
#undef ASSERT
#define ASSERT(exp)     if (exp); else osl_assert(#exp, __FILE__, __LINE__)
extern void osl_assert(char *exp, char *file, int line);
#else
#undef ASSERT
#define	ASSERT(exp)
#endif /* BCMASSERT_LOG || BCMASSERT_LOG */
#endif /* USE_MS_ASSERT */
#endif /* _PREFAST_ */

#define	DMA_TX	1	/* TX direction for DMA */
#define	DMA_RX	2	/* RX direction for DMA */

#if !defined (COOKIE_SHARED)
#define COOKIE_SHARED(cookie)		(cookie)
#endif /* !defined (COOKIE_SHARED) */

#ifdef BCMSDIO
#define OSL_SDIO_READ_ATTR(osh, offset, buf, size) \
	bcmsdh_cfg_read(NULL, (offset), (buf), (size), NULL)
#define OSL_SDIO_WRITE_ATTR(osh, offset, buf, size) \
	bcmsdh_cfg_write(NULL, (offset), (buf), (size), NULL)
#endif /* def BCMSDIO */

#define OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) osl_pcmcia_read_attr((osh), (offset), (buf), \
				(size))
#define OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) osl_pcmcia_write_attr((osh), (offset), \
				(buf), (size))
extern void osl_pcmcia_read_attr(osl_t *osh, uint offset, void *buf, int size);
extern void osl_pcmcia_write_attr(osl_t *osh, uint offset, void *buf, int size);

#define	OSL_PCI_READ_CONFIG(osh, offset, size)		osl_pci_read_config((osh), (offset), (size))
#define OSL_PCI_WRITE_CONFIG(osh, offset, size, val) osl_pci_write_config((osh), (offset), (size), \
				(val))
extern uint32 osl_pci_read_config(osl_t *osh, uint size, uint offset);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

#define OSL_PCI_BUS(osh)	(0)
#define OSL_PCI_SLOT(osh)	(0)

#define	OSL_UNCACHED(va)	(va)
#define	OSL_CACHED(va)		(va)

/* won't need this until we boot NDIS on one of our chips */
#define	REG_MAP(pa, size)	NULL			/* not supported */
#define	REG_UNMAP(va)		0			/* not supported */

#if defined (NDIS40)
extern char * _cdecl strncpy(char *dest, const char *src, uint n);
#endif /* defined (NDIS40) */

#ifndef USEWDK
#ifndef EXT_STA
extern unsigned long _cdecl DbgPrint(char *fmt, ...);
#endif /* EXT_STA */
#endif /* USEWDK */

extern int sprintf(char *buf, const char *fmt, ...);
#define	printf	DbgPrint

/* pick up osl required snprintf/vsnprintf */
#include <bcmstdlib.h>

#if !defined (WLSIM)

/* register access macros */
#if defined(OSLREGOPS)

#define REGOPSSET(osh, rreg, wreg, ctx)		osl_regops_set(osh, rreg, wreg, ctx)

#define R_REG(osh, r) (\
	sizeof(*(r)) == sizeof(uint8) ? osl_readb((osh), (volatile uint8*)(r)) : \
	sizeof(*(r)) == sizeof(uint16) ? osl_readw((osh), (volatile uint16*)(r)) : \
	osl_readl((osh), (volatile uint32*)(r)) \
)
#define W_REG(osh, r, v) do { \
	switch (sizeof(*(r))) { \
	case sizeof(uint8):	osl_writeb((osh), (volatile uint8*)(r), (uint8)(v)); break; \
	case sizeof(uint16):	osl_writew((osh), (volatile uint16*)(r), (uint16)(v)); break; \
	case sizeof(uint32):	osl_writel((osh), (volatile uint32*)(r), (uint32)(v)); break; \
	} \
} while (0)

extern void osl_regops_set(osl_t *osh, osl_rreg_fn_t rreg, osl_wreg_fn_t wreg, void *ctx);
extern uint8 osl_readb(osl_t *osh, volatile uint8 *r);
extern uint16 osl_readw(osl_t *osh, volatile uint16 *r);
extern uint32 osl_readl(osl_t *osh, volatile uint32 *r);
extern void osl_writeb(osl_t *osh, volatile uint8 *r, uint8 v);
extern void osl_writew(osl_t *osh, volatile uint16 *r, uint16 v);
extern void osl_writel(osl_t *osh, volatile uint32 *r, uint32 v);

#else /* OSLREGOPS */

static __inline uint32
readl(volatile uint32 *address)
{
	volatile uint32 v;

	NdisReadRegisterUlong((PULONG)address, (PULONG)&v);
	return (v);
}

static __inline uint16
readw(volatile uint16 *address)
{
	volatile uint16 v;

	NdisReadRegisterUshort((PUSHORT)address, (PUSHORT)&v);
	return (v);
}

static __inline uint8
readb(volatile uint8 *address)
{
	volatile uint8 v;

	NdisReadRegisterUchar((PUCHAR)address, (PUCHAR)&v);
	return (v);
}

#undef writew
#define writew(value, address) NdisWriteRegisterUshort((uint16 *)(address), (value))
#undef writel
#define writel(value, address)	NdisWriteRegisterUlong((uint32 *)(address), (value))
#undef writec
#define writeb(value, address)	NdisWriteRegisterUchar((uint8 *)(address), (value))


#ifndef BCMSDIO
#define	R_REG(osh, r)	((sizeof *(r) == sizeof(uint32)) ? readl((volatile uint32*)(r)): \
			(uint32)((sizeof *(r) == sizeof(uint16)) ? readw((volatile uint16*)(r)): \
			readb((volatile uint8*)(r))))

#ifdef WAR10501
/* PR10501 WAR: write to a SB register after write to a register */	\
#include <sbconfig.h>
#define	W_REG(osh, r, v)						\
	do {							\
		if (sizeof *(r) == sizeof(uint32))		\
			writel((v), (r));			\
		else if (sizeof *(r) == sizeof(uint16))   					\
			writew((uint16)(v), (r));		\
		else    					\
			writeb((uint8)(v), (r));		\
		/* has to be a valid chip core access */	\
		ASSERT((readl((uint32*)(((uint32)(r) & ~(SI_CORE_SIZE - 1)) | \
	(uint32)(&((sbconfig_t*)SBCONFIGOFF)->sbidhigh))) >> SBIDH_VC_SHIFT) == SB_VEND_BCM);	\
		writel(0, (((uint32)(r) & ~(SI_CORE_SIZE - 1)) | \
		(uint32)(&((sbconfig_t*)SBCONFIGOFF)->sbidlow)));	\
	} while (0)
#else
#ifdef WAR19310		/* pcmcia rev5: consecutive writes are corruptted on slow clock */
#define	W_REG(osh, r, v)						\
	do {							\
		if (sizeof *(r) == sizeof(uint32))		\
			writel((v), (r));			\
		else if (sizeof *(r) == sizeof(uint16))   					\
			writew((uint16)(v), (r));		\
		else    					\
			writeb((uint8)(v), (r));		\
		OSL_DELAY(2);					\
	} while (0)
#else
#define	W_REG(osh, r, v)						\
	do {							\
		if (sizeof *(r) == sizeof(uint32))		\
			writel((v), (r));			\
		else if (sizeof *(r) == sizeof(uint16))   					\
			writew((uint16)(v), (r));		\
		else    					\
			writeb((uint8)(v), (r));		\
	} while (0)
#endif /* WAR19310 */
#endif /* WAR10501 */

#else /* ndef BCMSDIO */
#ifdef UNDER_CE
#define	R_REG(osh, r)	bcmsdh_reg_read(NULL, (uint32)r, sizeof(*(r)))
#else
#define R_REG(osh, r) (\
	sizeof(*(r)) == sizeof(uint8) ? \
		(uint8)(bcmsdh_reg_read(NULL, (uint32)r, sizeof(*(r))) & 0xff) : \
	sizeof(*(r)) == sizeof(uint16) ? \
		(uint16)(bcmsdh_reg_read(NULL, (uint32)r, sizeof(*(r))) & 0xffff) : \
	bcmsdh_reg_read(NULL, (uint32)r, sizeof(*(r))) \
)
#endif /* UNDER_CE */
#define	W_REG(osh, r, v)	bcmsdh_reg_write(NULL, (uint32)r, sizeof(*(r)), (v))
#endif /* BCMSDIO */

#endif /* OSLREGOPS */

#define	AND_REG(osh, r, v)	W_REG(osh, (r), (R_REG(osh, r) & (v)))
#define	OR_REG(osh, r, v)	W_REG(osh, (r), (R_REG(osh, r) | (v)))

/* Host/Bus architecture specific swap. Noop for little endian systems, possible swap on big endian
 */
#define BUS_SWAP32(v)	(v)

/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)	NdisMoveMemory((dst), (src), (len))
#define	bcmp(b1, b2, len)	(!NdisEqualMemory((b1), (b2), (len)))
#define	bzero(b, len)		NdisZeroMemory((b), (len))

#endif /* WLSIM */

/* OSL initialization */
extern osl_t *osl_attach(void *pdev, NDIS_HANDLE adapter_handle);
extern void osl_detach(osl_t *osh);

extern void osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)

#ifdef BCMDBG_MEM

#define MALLOC(osh, size)       osl_debug_malloc((osh), (size), __LINE__, __FILE__)
#define MFREE(osh, addr, size)  osl_debug_mfree((osh), (addr), (size), __LINE__, __FILE__)
#define MALLOCED(osh)           osl_malloced((osh))
#define MALLOC_DUMP(osh, b) osl_debug_memdump((osh), (b))
extern void *osl_debug_malloc(osl_t *osh, uint size, int line, char* file);
extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, char* file);
extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *b);

#else /* BCMDBG_MEM */

#define	MALLOC(osh, size)	osl_malloc((osh), (size))
#define	MFREE(osh, addr, size)	osl_mfree((osh), (char*)(addr), (size))
#define	MALLOCED(osh)		osl_malloced((osh))
#define	MALLOC_DUMP(osh, b)

#endif  /* BCMDBG_MEM */

#define NATIVE_MALLOC(osh, size)	osl_malloc_native((osh), (size))
#define NATIVE_MFREE(osh, addr, size)	osl_mfree_native((osh), (char*)(addr), (size))


#define MALLOC_FAILED(osh)	osl_malloc_failed((osh))

extern void *osl_malloc(osl_t *osh, uint size);
extern void osl_mfree(osl_t *osh, char *addr, uint size);
extern uint osl_malloced(osl_t *osh);
extern uint osl_malloc_failed(osl_t *osh);

extern void *osl_malloc_native(osl_t *osh, uint size);
extern void osl_mfree_native(osl_t *osh, char *addr, uint size);

#define	DMA_CONSISTENT_ALIGN	sizeof(int)
#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah)\
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align,
	uint *tot, dmaaddr_t *pap);
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah)\
	osl_dma_free_consistent((osh), (va), (size), (pa))
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, dmaaddr_t pa);

#define	DMA_MAP(osh, va, size, direction, p, dmah)\
	osl_dma_map((osh), (size), (direction), (p))
extern dmaaddr_t osl_dma_map(osl_t *osh, uint size, int direction, void *lb);
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah)\
	osl_dma_unmap((osh), (size), (direction), (p))
extern void osl_dma_unmap(osl_t *osh, uint size, int direction, void *lb);

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) osl_dmaddrwidth((osh), (addrwidth))
extern void osl_dmaddrwidth(osl_t *osh, uint addrwidth);
/* XXX goes away when ndshared pkt is folded into OSL */
extern bool osl_dmaddr_valid(osl_t *osh, ulong loPart, ulong hiPart);

#define	OSL_DELAY(usec)	NdisStallExecution(usec)

#if defined (UNDER_CE)
enum dbg_output {
	DBG_OUTPUT_KITL,
	DBG_OUTPUT_BTSTACK,
	DBG_OUTPUT_LOG,
	DBG_OUTPUT_MAX
};
#if defined(WLBTCEAMP11)
extern void LogViaStackRB(const char *fmt_str, ...);
#define logf LogViaStackRB
extern void LogViaStack(const char *fmt_str, ...);
extern void LogIntoFile(const char *fmt_str, ...);
#endif /* WLBTCEAMP11 */
/* Note this works for Intermec CN30 platform for other platforms may require 
 * other implementation?
 */
#undef	printf
extern uint g_dbgOutputIndex;
extern void (*dbgOutputFunc[])(fmt, ...);
#define	printf	dbgOutputFunc[g_dbgOutputIndex]

#if defined (_MIPS_)
INLINE uint
osl_getcycles()
{
	volatile uint cycles;
	__asm("mfc0 v0, $9");
	__asm("sw v0, 0(%0)", &cycles);
	return (cycles * 2);
}
#elif defined (_ARM_) || defined (_X86_)
#ifndef UNDER_CE
extern DWORD GetTickCount(void);
#endif /* UNDER_CE */
extern BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
extern BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
INLINE uint
osl_getcycles()
{
	LARGE_INTEGER cnt;
	QueryPerformanceCounter(&cnt);
	return cnt.LowPart;
}
#endif /* defined (_MIPS_) */
#define OSL_GETCYCLES(x) ((x) = osl_getcycles())

#else	/* non CE system */

#if defined (_X86_)
INLINE uint
osl_getcycles()
{
	volatile uint cycles;
	__asm {
		PUSH eax
		PUSH edx
		_emit 0x0f
		_emit 0x31
		MOV cycles, eax
		POP edx
		POP eax
	}
	return cycles;
}
#define OSL_GETCYCLES(x) ((x) = osl_getcycles())
#else
#define OSL_GETCYCLES(x) ((x) = 0)
#endif /* defined (_X86_) */

#endif /* defined (UNDER_CE) */

#if !defined (WLSIM)	/* shared memory access macros */
#define	R_SM(a)		*(a)
#define	W_SM(a, v)	(*(a) = (v))
#define	BZERO_SM(a, len)	NdisZeroMemory((a), (len))
#endif /* WLSIM */

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	LBDATASZ

/* packet primitives */
#define	PKTGET(osh, len, send)		osl_pktget((osh), (len), (send))
extern void *osl_pktget(osl_t *osh, uint len, bool send);
#define	PKTFREE(osh, lb, send)		osl_pktfree((osh), (lb), (send))
extern void osl_pktfree(osl_t *osh, void *p, bool send);
#define	PKTDATA(osh, lb)		((struct lbuf *)(lb))->data
#define	PKTLEN(osh, lb)			((struct lbuf *)(lb))->len
#define PKTHEADROOM(osh, lb)		(PKTDATA(osh, lb)-(((struct lbuf *)(lb))->head))
#define PKTTAILROOM(osh, lb)		((((struct lbuf *)(lb))->end)-(((struct lbuf *)(lb))->tail))
#define	PKTNEXT(osh, lb)		((struct lbuf *)(lb))->next
#define	PKTSETNEXT(osh, lb, x)		((struct lbuf *)(lb))->next = (struct lbuf*)(x)
#define	PKTSETLEN(osh, lb, len)		osl_pktsetlen((osh), (lb), (len))
extern void osl_pktsetlen(osl_t *osh, void *lb, uint len);
#define	PKTPUSH(osh, lb, bytes)		osl_pktpush((osh), (lb), (bytes))
extern uchar *osl_pktpush(osl_t *osh, void *lb, uint bytes);
#define	PKTPULL(osh, lb, bytes)		osl_pktpull((osh), (lb), (bytes))
extern uchar *osl_pktpull(osl_t *osh, void *lb, uint bytes);
#define	PKTDUP(osh, lb)			osl_pktdup((osh), (lb))
extern void *osl_pktdup(osl_t *osh, void *lb);
#define	PKTTAG(lb)			(((void *) ((struct lbuf *)(lb))->pkttag))
#define	PKTLINK(lb)			((struct lbuf *)(lb))->link
#define	PKTSETLINK(lb, x)		((struct lbuf *)(lb))->link = (struct lbuf*)(x)
#define	PKTPRIO(lb)			((struct lbuf *)(lb))->priority
#define	PKTSETPRIO(lb, x)		((struct lbuf *)(lb))->priority = (x)
#define	PKTFRMNATIVE(osh, x)		osl_pkt_frmnative((osh), (x))
#define	PKTTONATIVE(osh, lb)	osl_pkt_tonative((osh), (struct lbuf *)(lb))
#define PKTSHARED(lb)                   (0)
#define PKTALLOCED(osh)			(0)
#define PKTSETPOOL(osh, lb, x, y)	do {} while (0)
#define PKTPOOL(osh, lb)		FALSE
#define PKTLIST_DUMP(osh, buf)
#ifdef EXT_STA
#define	PKTEXEMPT(lb)			((struct lbuf *)(lb))->exempt
#define PKTSETEXEMPT(lb, x)		((struct lbuf *)(lb))->exempt = (x)
#endif /* EXT_STA */
#define PKTSETHEADROOM(l, x) (l.headroom = x);
extern int osl_pktsumneeded(struct lbuf *lb);
#define PKTSUMNEEDED(lb)		osl_pktsumneeded((lb))
#define PKTSHRINK(osh, m)		(m)

#define LBF_SUM_GOOD	0x1
#define PKTSETSUMGOOD(lb, x)		((struct lbuf *)(lb))->flags = ((x) ? \
						(((struct lbuf *)(lb))->flags | LBF_SUM_GOOD) : \
						(((struct lbuf *)(lb))->flags & ~LBF_SUM_GOOD))
extern void* osl_pkt_frmnative(osl_t *osh, ND_PKT *p);
extern ND_PKT *osl_pkt_tonative(osl_t *osh, struct lbuf *lb);
#define OSL_ERROR(bcmerror)	bcmerror2ndisstatus(bcmerror)

#if defined(BCMSDIO) && (!defined(UNDER_CE))
/* Reg map/unmap. Required by Arasan standard SDIO controller */
extern void *osl_reg_map(osl_t *osh, uint32 pa, uint size);
extern void osl_reg_unmap(osl_t *osh, void *va, uint size);
extern shared_info_t *osl_get_sh(osl_t *osh);
#endif /* defined(BCMSDIO) && (!defined(UNDER_CE)) */

/* get system up time in miliseconds */
extern uint32 osl_systemuptime(void);
#define OSL_SYSUPTIME()		osl_systemuptime()

/* Global ASSERT type */
extern uint32 g_assert_type;

#endif	/* _ndis_osl_h_ */
