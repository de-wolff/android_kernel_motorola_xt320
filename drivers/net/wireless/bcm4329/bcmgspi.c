/*
 * Broadcom SPI Host Controller driver and BCMSDH to gSPI Protocol Conversion Layer
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmgspi.c,v 1.20.6.1 2010-08-26 19:11:09 dlhtohdl Exp $
 */

#include <typedefs.h>

#include <bcmdevs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <osl.h>
#include <hndsoc.h>

#include <siutils.h>
#include <sbsdio.h>
#include <sbchipc.h>
#include <spid.h>
#include <sbhnddma.h>
#include <hnddma.h>

#include <sdio.h>

#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <bcmsdbrcm.h>	/* SDIO Specs */
#include <sdiovar.h>

#include <bcmspih.h>
#include <bcmgspi.h>
#include <proto/gspi.h>

/* XXX Get correct number after experimenting on BCM4329 */
#define F0_RESPONSE_DELAY	0x40
#define F1_RESPONSE_DELAY	0x40
#define F2_RESPONSE_DELAY	F0_RESPONSE_DELAY

#define GSPI_F0_RESP_DELAY		4
#define GSPI_F1_RESP_DELAY		F1_RESPONSE_DELAY
#define GSPI_F2_RESP_DELAY		4
#define GSPI_F3_RESP_DELAY		4

#define CMDLEN			4

uint8	spi_outbuf[SPI_MAX_PKT_LEN];
uint8	spi_inbuf[SPI_MAX_PKT_LEN];

#define RESP_DELAY_PKT_LEN	SPI_MAX_PKT_LEN
uint8	spi_outrespdelaybuf[RESP_DELAY_PKT_LEN];
uint8	spi_inrespdelaybuf[RESP_DELAY_PKT_LEN];

int bcmpcispi_dump = 0;		/* Set to dump complete trace of all SPI bus transactions */

uint sd_sdmode = SDIOH_MODE_SPI;		/* Use SD4 mode by default */

#define NUM_RESP_WORDS	1

/* XXX - try to get rid of this. */
#if defined(linux)
#include <pcicfg.h>
#endif

/* XXX - get rid of this... */
#define SD_PAGE 4096

/* Globals */

#ifdef BCMDBG
uint sd_msglevel = SDH_ERROR_VAL;
#else
uint sd_msglevel = 0;
#endif /* BCMDBG */

struct sdioh_info {
	struct sdioh_pubinfo pub;

	osl_t 		*osh;			/* osh handler */
	sdioh_regs_t	*sdioh_regs;		/* Host Controller Registers */

	sdioh_cb_fn_t	intr_handler;		/* registered interrupt handler */
	void		*intr_handler_arg;	/* argument to call interrupt handler */
	uint32		intmask;		/* Current active interrupts */

	uint 		irq;			/* bus-specific irq */


	bool 		polled_mode;		/* polling for command completion */
	bool		sd_use_dma;         	/* DMA on CMD53 */
	bool 		use_dev_ints;		/* If this is false, make sure to turn on
	                                         * polling hack in wl_linux.c:wl_timer().
	                                         */

	int 		sd_mode;		/* SD1/SD4/SPI */
	int 		dev_block_size[SPI_MAX_IOFUNCS];		/* Blocksize */
	uint16 		dev_rca;		/* Current Address */
	uint8 		num_funcs;		/* Supported functions on dev */
	uint32 		com_cis_ptr;

	uint32 		data_xfer_count;	/* Current transfer */

	/* XXX - going away with HNDDMA */
	void		*dma_buf;
	ulong		dma_phys;

	hnddma_t	*di;		/* dma engine software state */
	si_t 		*sih;		/* sb utils handle */

	uint		id;			/* CODEC/SDIOH device core ID */
	uint		rev;		/* CODEC/SDIOH device core revision */
	uint		unit;		/* CODEC/SDIOH unit number */

	int 		r_cnt;			/* rx count */
	int 		t_cnt;			/* tx_count */
	int 		intrcount;		/* Client interrupts */
	uint32		card_dstatus;
	uint32		wordlen;
	uint32		prev_fun;
	bool		resp_delay_all;
	uint32		chip;
	uint32		chiprev;

	struct spierrstats_t spierrstats;
	bool		resp_delay_new;

};

/* XXX module params (maybe move to bcmsdbus, parse CIS to get block size) */
uint sd_hiok = FALSE;		/* Use hi-speed mode if available? */
uint sd_f2_blocksize = 64;		/* Default blocksize */

#ifdef TESTDONGLE
uint sd_divisor = 0;		/* Default 48MHz/4 = 12MHz for dongle */
#else
uint sd_divisor = EXT_CLK;		/* Default 25MHz clock (extclk) otherwise */
#endif

uint sd_power = 1;		/* Default to SD Slot powered ON */
uint sd_clock = 1;		/* Default to SD Clock turned ON */
uint sd_pci_slot = 0xFFFFffff; /* Used to force selection of a particular PCI slot */


/* Prototypes */
static int sdbrcm_start_clock(sdioh_info_t *sd);
static bool sdbrcm_start_power(sdioh_info_t *sd);
static int sdbrcm_bus_mode(sdioh_info_t *sd, int mode);
static int sdbrcm_set_highspeed_mode(sdioh_info_t *sd, bool hsmode);
static int sdbrcm_cmd_issue(sdioh_info_t *sd, bool use_dma, uint32 cmd_arg,
                       uint32 *data, uint32 datalen);
static int sdbrcm_dev_regread(sdioh_info_t *sd, int fnum, uint32 regaddr,
                               int regsize, uint32 *data);

static int sdbrcm_dev_regread_fixedaddr(sdioh_info_t *sd, int fnum, uint32 regaddr, int regsize,
                                 uint32 *data);
static int sdbrcm_dev_regwrite(sdioh_info_t *sd, int fnum, uint32 regaddr,
                                                                int regsize, uint32 data);
static int sdbrcm_driver_init(sdioh_info_t *sd);
static int sdbrcm_reset(sdioh_info_t *sd, bool host_reset, bool dev_reset);
static int sdbrcm_dev_buf(sdioh_info_t *sd, int rw,
                          int fnum, bool fifo, uint32 addr,
                          int nbytes, uint32 *data);
static int sdbrcm_set_dev_block_size(sdioh_info_t *sd, int fnum, int blocksize);
static int sdbrcm_get_dev_block_size(sdioh_info_t *sd);
static void sdbrcm_cmd_getdstatus(sdioh_info_t *sd, uint32 *dstatus_buffer);
static void sdbrcm_update_stats(sdioh_info_t *sd, uint32 cmd_arg);
#ifdef BCMDBG
static void sdbrcm_dumpregs(sdioh_info_t *sd);
#endif

static bool sdbrcm_hw_attach(sdioh_info_t *sd);
static bool sdbrcm_hw_detach(sdioh_info_t *sd);
static void sdbrcm_sendrecv(sdioh_info_t *sd, uint16 *msg_out, uint16 *msg_in, int msglen);
/*
 *  Public entry points & extern's
 */
sdioh_info_t *
sdioh_attach(osl_t *osh, void *regsva, uint irq)
{
	sdioh_info_t *sd;

	sd_trace(("%s\n", __FUNCTION__));

	/* Double-check that it's a broadcom chip */
	/* XXX Should actually confirm that device has a Silicon Backplane? */
	if ((OSL_PCI_READ_CONFIG(osh, PCI_CFG_VID, 4) & 0xffff) != VENDOR_BROADCOM) {
		sd_err(("sdioh_attach: rejecting non-broadcom device 0x%08x\n",
		         OSL_PCI_READ_CONFIG(osh, PCI_CFG_VID, 4)));
		return NULL;
	}

	if ((sd = (sdioh_info_t *)MALLOC(osh, sizeof(sdioh_info_t))) == NULL) {
		sd_err(("sdioh_attach: out of memory, malloced %d bytes\n", MALLOCED(osh)));
		return NULL;
	}

	bzero((char *)sd, sizeof(sdioh_info_t));
	sd->osh = osh;
	if (sdbrcm_osinit(sd, osh) != 0) {
		sd_err(("sdioh_attach: sdbrcm_osinit() failed\n"));
		goto sdioh_attach_fail;
	}

	sd->sdioh_regs = (sdioh_regs_t *)sdbrcm_reg_map(osh, (uintptr)regsva, PCI_BAR0_WINSZ);

	if (!(sd->sih = si_attach(BCM_SPIH_ID, osh, (void *)sd->sdioh_regs,
	                          PCI_BUS, NULL, NULL, 0))) {
		sd_err(("si_attach() failed.\n"));
	}

	if (!si_setcore(sd->sih, SPIH_CORE_ID, 0))
		sd_err(("Could not point to spih core\n"));
	sd->id = si_coreid(sd->sih);
	sd->rev = si_corerev(sd->sih);
	sd->unit = 0;

	if (sd->id != SPIH_CORE_ID) {
		sd_err(("Not a BCM SPI host. Id/Rev is 0x%x/0x%x\n", sd->id, sd->rev));
		goto sdioh_attach_fail;
	}

	sd_err(("sdbrcm%d: found SPI host(rev %d) in pci device, membase %p\n",
	        sd->unit, sd->rev, sd->sdioh_regs));

	sd->irq = irq;

	if (sd->sdioh_regs == NULL) {
		sd_err(("%s:ioremap() failed\n", __FUNCTION__));
		goto sdioh_attach_fail;
	}

	sd_info(("%s:sd->sdioh_regs = %p\n", __FUNCTION__, sd->sdioh_regs));

	/* Set defaults */
	sd->use_dev_ints = TRUE;
	sd->sd_use_dma = FALSE; /* DMA not implemented yet. */

	/* Spi device default is 16bit mode.  For synopsis hostcontroller we keep it as
	 * 16bit mode
	 */
	sd->wordlen = 2;

	si_core_reset(sd->sih, 0, 0);

	/* Needed for NDIS as its OSL checks for correct dma address mode
	 * This value is normally set by wlc_attach() which has yet to run
	 */
	if (sd->sd_use_dma) {
		OSL_DMADDRWIDTH(osh, 32);
		/* XXX replace with dma_attach for HNDDMA */

		if ((sd->di = dma_attach(osh, "sdbcm", sd->sih,
		        (void *)&sd->sdioh_regs->dmaregs.xmt, (void *)&sd->sdioh_regs->dmaregs.rcv,
		        NTXD, NRXD, RXBUFSZ, -1, NRXBUFPOST, HWRXOFF,
		        &sd_msglevel)) == NULL) {

			sd_err(("sdbrcm%d: dma_attach failed\n", sd->unit));
			goto sdioh_attach_fail;
		}
		sd_err(("DMA attach succeeded.\n"));

	}

	if (!sdbrcm_hw_attach(sd)) {
		sd_err(("sdbrcm%d %s: sdbrcm_hw_attach() failed()\n", sd->unit, __FUNCTION__));
		if (sd->sdioh_regs)
			sdbrcm_reg_unmap(osh, (uintptr)sd->sdioh_regs, PCI_BAR0_WINSZ);
		/* XXX: Check for free's tbd */
		goto sdioh_attach_fail;
	}

	/* Enable interrupts from the sdioh core onto the PCI bus */
	/* XXX si_pci_setup() config pci ... */
	/* Write this reg after programming spih polarity register to avoid false pci-intr */
	OSL_PCI_WRITE_CONFIG(osh, PCI_INT_MASK, sizeof(uint32), 0x400);

	if (sdbrcm_driver_init(sd) != BCME_OK) {
		sd_err(("sdbrcm%d %s: sdbrcm_driver_init() failed()\n", sd->unit, __FUNCTION__));
		if (sd->sdioh_regs)
			sdbrcm_reg_unmap(osh, (uintptr)sd->sdioh_regs, PCI_BAR0_WINSZ);
		goto sdioh_attach_fail;
	}

	/* XXX move this to per-port. */
	if (sdbrcm_register_irq(sd, irq) != BCME_OK) {
		sd_err(("%s: sdbrcm_register_irq() failed for irq = %d\n", __FUNCTION__, irq));
		sdbrcm_free_irq(sd->irq, sd);
		if (sd->sdioh_regs)
			sdbrcm_reg_unmap(osh, (uintptr)sd->sdioh_regs, PCI_BAR0_WINSZ);
		goto sdioh_attach_fail;
	}

	sd_trace(("%s: Done\n", __FUNCTION__));

	return sd;

sdioh_attach_fail:
	if (sd) {
		sdbrcm_osfree(sd, sd->osh);
		MFREE(sd->osh, sd, sizeof(sdioh_info_t));
	}
	return (NULL);
}

int
sdioh_detach(osl_t *osh, sdioh_info_t *sd)
{
	if (sd == NULL) {
		return (BCME_ERROR);
	}

	sd_trace(("%s\n", __FUNCTION__));

	/* disable Host Controller interrupts */
	W_REG(sd->osh, &(sd->sdioh_regs->intmask), 0);

	/* disable spih interupts */
	W_REG(osh, &sd->sdioh_regs->imr, 0x0);

	sd_trace(("%s: freeing irq %d\n", __FUNCTION__, sd->irq));
	sdbrcm_free_irq(sd->irq, sd); /* move to per-port */
	sdbrcm_reset(sd, 1, sd->pub.dev_init_done);

	/* XXX sd_unmap_dma(sd); */
	if (sd->di) {
		dma_detach(sd->di);
		sd->di = NULL;
	}

	/* put the core back into reset */
	if (sd->sih)
		si_core_disable(sd->sih, 0);

	/* free si handle */
	si_detach(sd->sih);
	sd->sih = NULL;

	sdbrcm_hw_detach(sd);

	/* XXX move map/unmap to per-port */
	sdbrcm_reg_unmap(osh, (uintptr)sd->sdioh_regs, PCI_BAR0_WINSZ);
	sd->sdioh_regs = NULL;
	MFREE(sd->osh, sd, sizeof(sdioh_info_t));

	return (BCME_OK);
}

/* Configure callback to dev when we recieve dev interrupt */
int
sdioh_interrupt_register(sdioh_info_t *sd, sdioh_cb_fn_t fn, void *argh)
{
	sd_trace(("%s: Entering\n", __FUNCTION__));
	sd->intr_handler = fn;
	sd->intr_handler_arg = argh;
	sd->pub.intr_registered = (fn != NULL);
	return (BCME_OK);
}

int
sdioh_interrupt_deregister(sdioh_info_t *sd)
{
	sd_trace(("%s: Entering\n", __FUNCTION__));
	sd->intr_handler = NULL;
	sd->intr_handler_arg = NULL;
	sd->pub.intr_registered = FALSE;
	return (BCME_OK);
}

int
sdioh_interrupt_query(sdioh_info_t *sd, bool *onoff)
{
	sd_trace(("%s: Entering\n", __FUNCTION__));
	*onoff = sd->pub.dev_intr_enabled;
	return (BCME_OK);
}

#if defined(DHD_DEBUG) || defined(BCMDBG)
bool
sdioh_interrupt_pending(sdioh_info_t *sd)
{
	return FALSE;
}
#endif

/* XXX parse the CIS */
int
sdioh_query_device(sdioh_info_t *sd)
{
#ifdef TESTDONGLE
	return SDIOD_FPGA_ID;
#else
	return BCM4318_D11G_ID;
#endif
}

/* Copy of dstatus bits of last spi-transaction for other layers to use. */
extern uint32
sdioh_get_dstatus(sdioh_info_t *sd)
{
	return sd->card_dstatus;
}

extern void
sdioh_chipinfo(sdioh_info_t *sd, uint32 chip, uint32 chiprev)
{
	sd->chip = chip;
	sd->chiprev = chiprev;
}

void
sdioh_dwordmode(sdioh_info_t *sd, bool set)
{

}

uint
sdioh_query_iofnum(sdioh_info_t *sd)
{
	return sd->num_funcs;
}

/* IOVar table */
enum {
	IOV_MSGLEVEL = 1,
	IOV_BLOCKSIZE,
	IOV_DMA,
	IOV_USEINTS,
	IOV_NUMINTS,
	IOV_NUMLOCALINTS,
	IOV_HOSTREG,
	IOV_DEVREG,
	IOV_DIVISOR,
	IOV_SDMODE,
	IOV_HISPEED,
	IOV_HCIREGS,
	IOV_POWER,
	IOV_CLOCK,
	IOV_SPIERRSTATS,
	IOV_RESP_DELAY_ALL
};

const bcm_iovar_t sdioh_iovars[] = {
	{"sd_msglevel",	IOV_MSGLEVEL, 	0,	IOVT_UINT32,	0 },
	{"sd_blocksize", IOV_BLOCKSIZE, 0,	IOVT_UINT32,	0 }, /* ((fn << 16) | size) */
	{"sd_dma",	IOV_DMA,	0,	IOVT_BOOL,	0 },
	{"sd_ints",	IOV_USEINTS,	0,	IOVT_BOOL,	0 },
	{"sd_numints",	IOV_NUMINTS,	0,	IOVT_UINT32,	0 },
	{"sd_numlocalints", IOV_NUMLOCALINTS, 0, IOVT_UINT32,	0 },
	{"sd_hostreg",	IOV_HOSTREG,	0,	IOVT_BUFFER,	sizeof(sdreg_t) },
	{"sd_devreg", IOV_DEVREG,	0,	IOVT_BUFFER,	sizeof(sdreg_t)	},
	{"sd_divisor",	IOV_DIVISOR,	0,	IOVT_UINT32,	0 },
	{"sd_power",	IOV_POWER,	0,	IOVT_UINT32,	0 },
	{"sd_clock",	IOV_CLOCK,	0,	IOVT_UINT32,	0 },
	{"sd_mode",	IOV_SDMODE,	0,	IOVT_UINT32,	100},
	{"sd_highspeed",	IOV_HISPEED,	0,	IOVT_UINT32,	0},
#ifdef BCMDBG
	{"sd_hciregs",	IOV_HCIREGS,	0,	IOVT_BUFFER,	0 },
#endif
	{"spi_errstats", IOV_SPIERRSTATS, 0, IOVT_BUFFER, sizeof(struct spierrstats_t) },
	{"spi_respdelay",	IOV_RESP_DELAY_ALL,	0,	IOVT_BOOL,	0 },
	{NULL, 0, 0, 0, 0 }
};

int
sdioh_iovar_op(sdioh_info_t *sd, const char *name,
               void *params, int plen, void *arg, int len, bool set)
{
	const bcm_iovar_t *vi = NULL;
	int bcmerror = 0;
	int val_size;
	int32 int_val = 0;
	bool bool_val;
	uint32 actionid;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	ASSERT(name);
	ASSERT(len >= 0);

	/* Get must have return space; Set does not take qualifiers */
	ASSERT(set || (arg && len));
	ASSERT(!set || (!params && !plen));

	sd_trace(("%s: Enter (%s %s)\n", __FUNCTION__, (set ? "set" : "get"), name));

	if ((vi = bcm_iovar_lookup(sdioh_iovars, name)) == NULL) {
		bcmerror = BCME_UNSUPPORTED;
		goto exit;
	}

	if ((bcmerror = bcm_iovar_lencheck(vi, arg, len, set)) != 0)
		goto exit;

	/* Set up params so get and set can share the convenience variables */
	if (params == NULL) {
		params = arg;
		plen = len;
	}

	if (vi->type == IOVT_VOID)
		val_size = 0;
	else if (vi->type == IOVT_BUFFER)
		val_size = len;
	else
		val_size = sizeof(int);

	if (plen >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;

	actionid = set ? IOV_SVAL(vi->varid) : IOV_GVAL(vi->varid);
	switch (actionid) {
	case IOV_GVAL(IOV_MSGLEVEL):
		int_val = (int32)sd_msglevel;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_MSGLEVEL):
		sd_msglevel = int_val;
		break;

	case IOV_GVAL(IOV_BLOCKSIZE):
		if ((uint32)int_val > sd->num_funcs) {
			bcmerror = BCME_BADARG;
			break;
		}
		int_val = (int32)sd->dev_block_size[int_val];
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_BLOCKSIZE):
	{
		/* Function number is stored in the upper 16 bits of int_val */
		uint fnum = ((uint32)int_val >> 16);
		uint blksize = (uint16)int_val;
		uint maxsize;

		if (fnum > sd->num_funcs) {
			bcmerror = BCME_BADARG;
			break;
		}

		/* XXX These hardcoded sizes are a hack, remove after proper CIS parsing. */
		switch (fnum) {
		case 0: maxsize = 32; break;
		case 1: maxsize = BLOCK_SIZE_4318; break;
		case 2: maxsize = BLOCK_SIZE_4328; break;
		default: maxsize = 0;
		}
		if (blksize > maxsize) {
			bcmerror = BCME_BADARG;
			break;
		}
		if (!blksize) {
			blksize = maxsize;
		}

		/* Now set it */
		sdbrcm_lock(sd);
		bcmerror = sdbrcm_set_dev_block_size(sd, fnum, blksize);
		sdbrcm_unlock(sd);
		break;
	}

	case IOV_GVAL(IOV_DMA):
		int_val = (int32)sd->sd_use_dma;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_DMA):
		sd->sd_use_dma = int_val;
		break;

	case IOV_GVAL(IOV_USEINTS):
		int_val = (int32)sd->use_dev_ints;
		bcopy(&int_val, arg, val_size);
		break;

	/* XXX remove when debugging is finished. */
	case IOV_SVAL(IOV_USEINTS):
		sd->use_dev_ints = int_val;
		if (sd->use_dev_ints)
			sd->intmask |= INT_DEV_INT;
		else
			sd->intmask &= ~INT_DEV_INT;
		break;

	/* XXX change these to a single dump */
	case IOV_GVAL(IOV_NUMINTS):
		int_val = (int32)sd->intrcount;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_GVAL(IOV_NUMLOCALINTS):
		int_val = (int32)sd->pub.local_intrcount;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_GVAL(IOV_DIVISOR):
		int_val = (uint32)sd_divisor;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_DIVISOR):
		sd_divisor = int_val;
		if (!sdbrcm_start_clock(sd)) {
			sd_err(("set clock failed!\n"));
			bcmerror = BCME_ERROR;
		}
		break;

	case IOV_GVAL(IOV_POWER):
		int_val = (uint32)sd_power;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_POWER):
		sd_power = int_val;
		if (sd_power == 1) {
			sd_err(("No software power control.\n"
			        "You need to turn the Host Controller Power Switch ON.\n"));
		} else {
			sd_err(("No software power control.\n"
			        "You need to turn the Host Controller Power Switch OFF.\n"));
		}
		break;

	case IOV_GVAL(IOV_CLOCK):
		int_val = (uint32)sd_clock;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_CLOCK):
		sd_clock = int_val;
		if (sd_clock == 1) {
			if (!sdbrcm_start_clock(sd)) {
				sd_err(("set clock failed!\n"));
				bcmerror = BCME_ERROR;
			}
			sd_err(("SD Clock turned ON.\n"));
		} else {
			sd_err(("SD Clock turned OFF.\n"));
		}
		break;

	case IOV_GVAL(IOV_SDMODE):
		int_val = (uint32)sd_sdmode;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_SDMODE):
		sd_sdmode = int_val;

		if (!sdbrcm_bus_mode(sd, sd_sdmode)) {
			sd_err(("sdbrcm_bus_width failed\n"));
			bcmerror = BCME_ERROR;
		}
		break;

	case IOV_GVAL(IOV_HISPEED):
		int_val = (uint32)sd_hiok;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_HISPEED):
		sd_hiok = int_val;

		if (!sdbrcm_set_highspeed_mode(sd, sd_hiok)) {
			sd_err(("Failed changing highspeed mode to %d.\n", sd_hiok));
			bcmerror = BCME_ERROR;
		}
		break;


#ifdef BCMDBG
	case IOV_GVAL(IOV_HOSTREG):
	case IOV_SVAL(IOV_HOSTREG):
	{
		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_regs = (sdreg_t *)params;

		if (sd_regs->offset > sizeof(sdioh_regs_t))
		{
			sd_err(("%s: bad offset 0x%x\n", __FUNCTION__, sd_regs->offset));
			bcmerror = BCME_BADARG;
			break;
		}

		if (!ISALIGNED(sd_regs->offset, sizeof(uint32)))
		{
			sd_err(("%s: bad offset 0x%x\n", __FUNCTION__, sd_regs->offset));
			bcmerror = BCME_BADARG;
			break;
		}

		if (actionid == IOV_GVAL(IOV_HOSTREG)) {
			sd_trace(("%s: R_REG(%p)\n",
			          __FUNCTION__, (void *)((uintptr)regs+sd_regs->offset)));
			int_val = R_REG(osh, (uint32*)((uintptr)regs + sd_regs->offset));
			bcopy(&int_val, arg, val_size);
		}
		else {
			sd_trace(("%s: W_REG(%p)\n",
			          __FUNCTION__, (void *)((uintptr)regs+sd_regs->offset)));
			W_REG(osh, (uint32*)((uintptr)regs + sd_regs->offset),
			      (uint32)sd_regs->value);
		}

		break;
	}

	case IOV_GVAL(IOV_DEVREG):
	{
		uint8 data;

		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_regs = (sdreg_t *)params;

		if ((uint)sd_regs->func > sd->num_funcs) {
			bcmerror = BCME_BADARG;
			break;
		}

		int_val = 0;
		if (sdioh_cfg_read(sd, sd_regs->func, sd_regs->offset, &data)) {
			bcmerror = BCME_SDIO_ERROR;
			break;
		}

		int_val = (int)data;
		bcopy(&int_val, arg, val_size);
		break;
	}

	case IOV_SVAL(IOV_DEVREG):  /* CMD52 8-bit interface */
	{
		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_regs = (sdreg_t *)params;
		uint8 data = sd_regs->value;

		if ((uint)sd_regs->func > sd->num_funcs) {
			bcmerror = BCME_BADARG;
			break;
		}
		if (sdioh_cfg_write(sd, sd_regs->func,
		                    sd_regs->offset, (uint8*)&data)) {
			bcmerror = BCME_SDIO_ERROR;
			break;
		}
		break;
	}

	case IOV_GVAL(IOV_HCIREGS):  /* convert this to a generic dump */
	{
		struct bcmstrbuf b;
		bcm_binit(&b, arg, len);

		sdbrcm_lock(sd);
		bcm_bprintf(&b, "SDINTSTATUS:       0x%08x\n",
		            R_REG(osh, &(regs->intstatus)));
		bcm_bprintf(&b, "SDINTMASK:         0x%08x\n",
		            R_REG(osh, &(regs->intmask)));
		sdbrcm_unlock(sd);
		sdbrcm_dumpregs(sd);
		if (!b.size)
			bcmerror = BCME_BUFTOOSHORT;
		break;
	}
#endif /* BCMDBG */

	case IOV_GVAL(IOV_SPIERRSTATS):
	{
		bcopy(&sd->spierrstats, arg, sizeof(struct spierrstats_t));
		break;
	}

	case IOV_SVAL(IOV_SPIERRSTATS):
	{
		bzero(&sd->spierrstats, sizeof(struct spierrstats_t));
		break;
	}

	case IOV_GVAL(IOV_RESP_DELAY_ALL):
		int_val = (int32)sd->resp_delay_all;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_RESP_DELAY_ALL):
		sd->resp_delay_all = int_val;

		if (sd->resp_delay_all)
			int_val = STATUS_ENABLE|RESP_DELAY_ALL;
		else {
			if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_RESPONSE_DELAY, 1,
			     F1_RESPONSE_DELAY) != SUCCESS) {
				sd_err(("%s: Unable to set response delay.\n", __FUNCTION__));
				bcmerror = BCME_SDIO_ERROR;
				break;
			}
			int_val = STATUS_ENABLE;
		}

		if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_STATUS_ENABLE, 1, int_val)
		     != SUCCESS) {
			sd_err(("%s: Unable to set response delay.\n", __FUNCTION__));
			bcmerror = BCME_SDIO_ERROR;
			break;
		}
		break;

	default:
		bcmerror = BCME_UNSUPPORTED;
		break;
	}
exit:

	/* XXX Remove protective lock after devs all clean... */
	return bcmerror;
}

/* XXX should be able to go away. */
int
sdioh_cfg_read(sdioh_info_t *sd, uint fnum, uint32 addr, uint8 *data)
{
	int status;

	ASSERT(fnum <= sd->num_funcs);

	/* No lock needed since sdioh_request_byte does locking */
	status = sdioh_request_byte(sd, SDIOH_READ, fnum, addr, data);
	return status;
}

/* XXX should be able to go away. */
int
sdioh_cfg_write(sdioh_info_t *sd, uint fnum, uint32 addr, uint8 *data)
{
	/* No lock needed since sdioh_request_byte does locking */
	int status;

	ASSERT(fnum <= sd->num_funcs);

	status = sdioh_request_byte(sd, SDIOH_WRITE, fnum, addr, data);
	return status;
}

/* Cis reads are only on F1 incase of gSPI. */
int
sdioh_cis_read(sdioh_info_t *sd, uint fnum, uint8 *cisd, uint32 length)
{
	uint32 count;
	int offset;
	uint32 cis_byte;
	uint16 *cis = (uint16 *)cisd;
	uint bar0 = SI_ENUM_BASE;
	int status;
	uint8 data;

	sd_trace(("%s: Func %d\n", __FUNCTION__, fnum));

	sdbrcm_lock(sd);

	/* Set si window address to 0x18000000 */
	data = (bar0 >> 8) & SBSDIO_SBADDRLOW_MASK;
	status = sdioh_cfg_write(sd, SDIO_FUNC_1, SBSDIO_FUNC1_SBADDRLOW, &data);
	if (status == 0) {
		data = (bar0 >> 16) & SBSDIO_SBADDRMID_MASK;
		status = sdioh_cfg_write(sd, SDIO_FUNC_1, SBSDIO_FUNC1_SBADDRMID, &data);
	} else {
		sdbrcm_unlock(sd);
		sd_err(("Unable to set si-addr-windows\n"));
		return (BCME_ERROR);
	}
	if (status == 0) {
		data = (bar0 >> 24) & SBSDIO_SBADDRHIGH_MASK;
		status = sdioh_cfg_write(sd, SDIO_FUNC_1, SBSDIO_FUNC1_SBADDRHIGH, &data);
	} else {
		sdbrcm_unlock(sd);
		sd_err(("Unable to set si-addr-windows\n"));
		return (BCME_ERROR);
	}

	offset =  CC_SROM_OTP; /* OTP offset in chipcommon. */
	for (count = 0; count < length/2; count++) {
		if (sdbrcm_dev_regread (sd, SDIO_FUNC_1, offset, 2, &cis_byte) < 0) {
			sd_err(("%s: regread failed: Can't read CIS\n", __FUNCTION__));
			sdbrcm_unlock(sd);
			return (BCME_ERROR);
		}

		*cis = cis_byte;
		cis++;
		offset += 2;
	}

	sdbrcm_unlock(sd);
	return (BCME_OK);
}

int
sdioh_request_byte(sdioh_info_t *sd, uint rw, uint fnum, uint regaddr, uint8 *byte)
{
	int status;
	uint32 cmd_arg;
	uint32 dstatus;
	uint32 data = (uint32)(*byte);

	sdbrcm_lock(sd);

	cmd_arg = 0;
	cmd_arg = SFIELD(cmd_arg, SPI_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, SPI_ACCESS, 1);	/* Incremental access */
	cmd_arg = SFIELD(cmd_arg, SPI_REG_ADDR, regaddr);
	cmd_arg = SFIELD(cmd_arg, SPI_RW_FLAG, rw == SDIOH_READ ? 0 : 1);
	cmd_arg = SFIELD(cmd_arg, SPI_LEN, 1);

	if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
	                              cmd_arg, &data, 1)) != SUCCESS) {
		sdbrcm_unlock(sd);
		return status;
	}

	sd_info(("brcmsdbcm0: %s: %c: F%d[%04x] 0x%08x\n",
		__FUNCTION__, ((rw == SDIOH_READ) ? 'R' : 'W'),
		fnum,
		regaddr, data));

	if (rw == SDIOH_READ)
		*byte = data;

	sdbrcm_cmd_getdstatus(sd, &dstatus);
	if (dstatus)
		sd_data(("dstatus =0x%x\n", dstatus));

	sdbrcm_unlock(sd);
	return (BCME_OK);
}

/* XXX remove cmd_type and nbytes args */
int
sdioh_request_word(sdioh_info_t *sd, uint cmd_type, uint rw, uint fnum, uint addr,
                   uint32 *word, uint nbytes)
{
	int status;

	sdbrcm_lock(sd);

	if (rw == SDIOH_READ)
		status = sdbrcm_dev_regread(sd, fnum, addr, nbytes, word);
	else
		status = sdbrcm_dev_regwrite(sd, fnum, addr, nbytes, *word);

	sd_info(("brcmsdbcm0: %s: %c: F%d[%04x] 0x%08x\n",
		__FUNCTION__, ((rw == SDIOH_READ) ? 'R' : 'W'),
		fnum,
		addr, *word));

	sdbrcm_unlock(sd);
	return (status == BCME_OK ?  BCME_OK : BCME_ERROR);
}

int
sdioh_request_buffer(sdioh_info_t *sd, uint pio_dma, uint fix_inc, uint rw, uint fnum,
                     uint addr, uint reg_width, uint buflen_u, uint8 *buffer, void *pkt)
{	/* XXX get rid of _u. */
	int len;
	int buflen = (int)buflen_u;
	bool fifo = (fix_inc == SDIOH_DATA_FIX);

	sdbrcm_lock(sd);

	ASSERT(fnum <= sd->num_funcs);
	ASSERT(reg_width == 4);		/* buffers can only be read/written on word boundaries. */
	ASSERT(buflen_u < (1 << 30));  /* XXX fix this */
	ASSERT(sd->dev_block_size[fnum]);

	sd_data(("%s: %c @%x: %s len %d r_cnt %d t_cnt %d, pkt @0x%p\n",
		__FUNCTION__, rw == SDIOH_READ ? 'R' : 'W',
		addr, (fifo) ? "(INCR)" : "(FIFO)", buflen_u, sd->r_cnt, sd->t_cnt, pkt));

	/* Break buffer down into blocksize chunks:
	 * Bytemode: 1 block at a time.
	 * Both: leftovers are handled last (will be sent via bytemode.
	 */
	while (buflen > 0) {
		len = MIN(sd->dev_block_size[fnum], buflen);
		if (sdbrcm_dev_buf(sd, rw, fnum, fifo, addr, len, (uint32 *)buffer) != BCME_OK) {
			sd_err(("%s: sdbrcm_dev_buf %s failed\n",
				__FUNCTION__, rw == SDIOH_READ ? "Read" : "Write"));
			sdbrcm_unlock(sd);
			return (BCME_ERROR);
		}
		buffer += len;
		buflen -= len;
		if (!fifo)
			addr += len;
	}
	sdbrcm_unlock(sd);
	return (BCME_OK);
}

/* XXX Copied guts of request_byte and cmd_issue.  Might make sense to fold this into
 * those by passing another parameter indicating command type (abort).  [But maybe
 * keeping it separate is better -- if called in internally on command failure it's less
 * recursion to wrap your head around?]
 */
static
int sdbrcm_abort(sdioh_info_t *sd, uint fnum)
{
	int err = 0;

	osl_t *osh;
	sdioh_regs_t *regs;

	sd_trace(("%s: Enter\n", __FUNCTION__));

	osh = sd->osh;
	regs = sd->sdioh_regs;

	/* Argument is write to F0 (CCCR) IOAbort with function number */

	/* XXX Copied from cmd_issue(), but no SPI response handling! */
	/* Wait until the host controller is finished with previous command.
	 * (Equivalent to the CMD_INHIBIT wait in the standard host.)
	 * XXX Not sure this is relevant in a single-threaded driver, only
	 * case where it'd seem to apply is if a previous command bailed
	 * way early...
	 */

	return err;
}

extern int
sdioh_abort(sdioh_info_t *sd, uint fnum)
{
	int ret;

	sdbrcm_lock(sd);
	ret = sdbrcm_abort(sd, fnum);
	sdbrcm_unlock(sd);

	return ret;
}

int
sdioh_start(sdioh_info_t *sd, int stage)
{
	return SUCCESS;
}

int
sdioh_stop(sdioh_info_t *sd)
{
	return SUCCESS;
}

int
sdioh_waitlockfree(sdioh_info_t *sd)
{
	return SUCCESS;
}


#ifdef BCMINTERNAL
int
sdioh_test_diag(sdioh_info_t *sd)
{
	sd_err(("%s: Implement me\n", __FUNCTION__));
	return (BCME_OK);
}
#endif /* BCMINTERNAL */

/*
 * Private/Static work routines
 */
static int
sdbrcm_reset(sdioh_info_t *sd, bool host_reset, bool dev_reset)
{
	return TRUE;
}

/* Disable device interrupts */
void
sdbrcm_devintr_off(sdioh_info_t *sd)
{
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_dev_ints));
	if (sd->use_dev_ints) {
		/* Program the 0x0006/7 reg of func0 to disable gSPI device interrupts */
		if (sdbrcm_dev_regwrite(sd, 0, SPID_INTR_EN_REG, 2, 0x0) != SUCCESS)
			sd_err(("%s: Could not enable ints in spi device\n", __FUNCTION__));
	}
}

/* Enable device interrupts */
void
sdbrcm_devintr_on(sdioh_info_t *sd)
{
	ASSERT(sd->pub.lockcount == 0);
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_dev_ints));
	if (sd->use_dev_ints) {
		/* Program the 0x0006/7 reg of func0 to enable gSPI device interrupts */
		if (sdbrcm_dev_regwrite(sd, 0, SPID_INTR_EN_REG, 2, 0x60E7) != SUCCESS)
			sd_err(("%s: Could not enable ints in spi device\n", __FUNCTION__));
	}
}

static int
sdbrcm_host_init(sdioh_info_t *sd)
{

	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	/* Reset Host Controller but not the attached device. */
	sdbrcm_reset(sd, 1, 0);

	sd->sd_mode = SDIOH_MODE_SPI;
	sd->polled_mode = TRUE; /* going away */
	sd->pub.host_init_done = TRUE;
	sd->pub.dev_init_done = FALSE;

	return (BCME_OK);
}

static bool
sdbrcm_test_dev(sdioh_info_t *sd)
{
	uint32 regdata;
	int status;
	int j;
#ifdef BCMDBG
	uint8 regbuf[32];

	bzero(spi_outbuf, SPI_MAX_PKT_LEN);
	bzero(spi_inbuf, SPI_MAX_PKT_LEN);
	bzero(regbuf, 32);
#endif /* BCMDBG */

	/* Due to a silicon testability issue, the first command from the Host to the device will
	 * get corrupted (first bit will be lost). So the Host should poll the device with a safe
	 * read request. ie: The Host should try to read F0 addr 0x14 using the Fixed address mode
	 * (This will prevent a unintended write command to be detected by device)
	 */
	if ((status = sdbrcm_dev_regread_fixedaddr(sd, 0, SPID_TEST_READ, 4, &regdata)) != SUCCESS)
		return FALSE;
	sd_err(("Silicon testability issue: 16bit LE regdata = 0x%x.  Is it 0xadadadad ?\n",
	        regdata));

	if ((status = sdbrcm_dev_regread(sd, 0, SPID_TEST_READ, 4, &regdata)) != SUCCESS)
		return FALSE;
	sd_err(("16bit LE regdata = 0x%x\n", regdata));

	if (regdata == TEST_RO_DATA_32BIT_LE) {
		sd_err(("Read spid passed. Value read = 0x%x\n", regdata));
		sd_err(("Spid had power-on cycle\n"));
	} else {
		sd_err(("Read spid failed. Value read = 0x%x\n", regdata));
		return FALSE;
	}

#ifdef BCMDBG
	/* Read default F0 registers */
	sd_err(("Reading default values of first 32(8bit) F0 spid regs\n"));
	sdbrcm_dev_regread(sd, 0, SPID_CONFIG, 32, (uint32 *)regbuf);
	for (j = 0; j < 32; j++)
		sd_err(("regbuf[%d]=0x%x	", j, regbuf[j]));
	sd_err(("\n"));
#endif /* BCMDBG */

#define RW_PATTERN1	0xA0A1A2A3
#define RW_PATTERN2	0x4B5B6B7B

	regdata = RW_PATTERN1;
	if ((status = sdbrcm_dev_regwrite(sd, 0, SPID_TEST_RW, 4, regdata)) != SUCCESS)
		return FALSE;
	regdata = 0;
	if ((status = sdbrcm_dev_regread(sd, 0, SPID_TEST_RW, 4, &regdata)) != SUCCESS)
		return FALSE;
	if (regdata != RW_PATTERN1) {
		sd_err(("Write-Read spid failed. Value wrote = 0xA0A1A2A3, Value read = 0x%x\n",
			regdata));
		return FALSE;
	} else
		sd_err(("R/W spid passed. Value read = 0x%x\n", regdata));

	regdata = RW_PATTERN2;
	if ((status = sdbrcm_dev_regwrite(sd, 0, SPID_TEST_RW, 4, regdata)) != SUCCESS)
		return FALSE;
	regdata = 0;
	if ((status = sdbrcm_dev_regread(sd, 0, SPID_TEST_RW, 4, &regdata)) != SUCCESS)
		return FALSE;
	if (regdata != RW_PATTERN2) {
		sd_err(("Write-Read spid failed. Value wrote = 0x4B5B6B7B, Value read = 0x%x\n",
			regdata));
		return FALSE;
	} else
		sd_err(("R/W spid passed. Value read = 0x%x\n", regdata));

	return TRUE;
}

static int
sdbrcm_dev_init(sdioh_info_t *sd)
{
	uint8 fn_ints;
	osl_t *osh;

	osh = sd->osh;

	if (!sdbrcm_start_clock(sd)) {
		sd_err(("sdbrcm_start_clock failed\n"));
		return (BCME_ERROR);
	}

	/* Clear history of data-not-available bit, if any. */
	if (sdioh_cfg_read(sd, 0, SPID_INTR_REG, &fn_ints)) {
		sd_err(("cfg_read of intr-reg to clear stale data-not-available bit failed\n"));
		return (BCME_ERROR);
	}
	sd_err(("intr-reg=0x%1x\n", fn_ints));

	fn_ints |= DATA_UNAVAILABLE;
	if (sdioh_cfg_write(sd, 0, SPID_INTR_REG, &fn_ints)) {
		return (BCME_ERROR);
	}
	sd_err(("Cleared data-not-available bit in intr-reg. Val=0x%x\n", fn_ints));

	if (!sdbrcm_start_power(sd)) {
		sd_err(("sdbrcm_start_power failed\n"));
		return (BCME_ERROR);
	}

	sd->num_funcs = SPI_MAX_IOFUNCS;

	sdbrcm_get_dev_block_size(sd);

	/* Change to High-Speed Mode if both host and device support it. */
	sdbrcm_set_highspeed_mode(sd, sd_hiok);

	{
		uint32 f1_respdelay = 0;
		sdbrcm_dev_regread(sd, SPI_FUNC_0, SPID_RESP_DELAY_F1, 1, &f1_respdelay);
		if ((f1_respdelay == 0) || (f1_respdelay == 0xFF)) {
			/* older sdiodevice core and has no separte resp delay for each of */
			sd_err(("older corerev < 4 so use the same resp delay for all funcs\n"));
			sd->resp_delay_new = FALSE;
		}
		else {
			/* older sdiodevice core and has no separte resp delay for each of */
			sd->resp_delay_new = TRUE;
			sd_err(("new corerev >= 4 so set the resp delay for each of the funcs\n"));
			sd_trace(("resp delay for funcs f0(%d), f1(%d), f2(%d), f3(%d)\n",
				GSPI_F0_RESP_DELAY, GSPI_F1_RESP_DELAY,
				GSPI_F2_RESP_DELAY, GSPI_F3_RESP_DELAY));
			if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_RESP_DELAY_F0, 1,
				GSPI_F0_RESP_DELAY) != SUCCESS) {
				sd_err(("%s: Unable to set response delay value for F0.\n",
					__FUNCTION__));
				return ERROR;
			}
			if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_RESP_DELAY_F1, 1,
				GSPI_F1_RESP_DELAY) != SUCCESS) {
				sd_err(("%s: Unable to set response delay value for F1.\n",
					__FUNCTION__));
				return ERROR;
			}
			if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_RESP_DELAY_F2, 1,
				GSPI_F2_RESP_DELAY) != SUCCESS) {
				sd_err(("%s: Unable to set response delay value for F2.\n",
					__FUNCTION__));
				return ERROR;
			}
			if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_RESP_DELAY_F3, 1,
				GSPI_F3_RESP_DELAY) != SUCCESS) {
				sd_err(("%s: Unable to set response delay value for F3.\n",
					__FUNCTION__));
				return ERROR;
			}
			sd->resp_delay_all = FALSE;
			sd->prev_fun = 10; /* Giving arbitrary number to avoid misled as Fun0 */
			goto done;
		}

	}
	/* XXX:Keep initial value for resp_delay_all as FALSE during bringup.
	 * Make it TRUE after switching the device to highspeed mode.
	 */
	sd->resp_delay_all = FALSE;
	sd->prev_fun = 10; /* Giving arbitrary number to avoid being misled as Fun0 */
	if (sd->resp_delay_all == FALSE) {
		if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_RESPONSE_DELAY, 1,
		     F1_RESPONSE_DELAY) != SUCCESS) {
				sd_err(("%s: Unable to set response delay value.\n", __FUNCTION__));
				return ERROR;
			}
	} else {
		/* Enable response delay for all functions */
		if (sdbrcm_dev_regwrite(sd, SPI_FUNC_0, SPID_STATUS_ENABLE, 1,
		    STATUS_ENABLE|RESP_DELAY_ALL) != SUCCESS) {
			sd_err(("%s: Unable to set response delay for all fun's.\n", __FUNCTION__));
			return ERROR;
		}
	}
done:
	sd->pub.dev_init_done = TRUE;

	return (BCME_OK);
}

int
sdbrcm_start_clock(sdioh_info_t *sd)
{
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_info(("Clock Freq = 48 Mhz\n"));

	return TRUE;
}

bool
sdbrcm_start_power(sdioh_info_t *sd)
{
	if (!sdbrcm_test_dev(sd)) {
		sd_err(("sdbrcm_test_dev failed\n"));
		return FALSE;
	}

	return TRUE;
}

int
sdbrcm_bus_mode(sdioh_info_t *sd, int new_mode)
{
	sd_trace(("%s\n", __FUNCTION__));
	if (sd->sd_mode == new_mode) {
		sd_info(("In SPI Mode.\n"));
	} else {
		sd_err(("%s: Errrr - This is spi! Can not set to %d\n", __FUNCTION__, new_mode));
	}

	return TRUE;
}

static int
sdbrcm_set_highspeed_mode(sdioh_info_t *sd, bool hsmode)
{
	uint32 regdata;
	int status;

	if ((status = sdbrcm_dev_regread(sd, 0, SPID_CONFIG,
	                                 1, &regdata)) != SUCCESS)
		return status;

	sd_trace(("In %s spid-ctrl = 0x%x \n", __FUNCTION__, regdata));


	if (hsmode == TRUE) {
		sd_err(("Attempting to enable High-Speed mode.\n"));

		if (regdata & HIGH_SPEED_MODE) {
			sd_err(("Device is already in High-Speed mode.\n"));
			return status;
		} else {
			regdata |= HIGH_SPEED_MODE;
			sd_err(("Writing %08x to device at %08x\n", regdata, SPID_CONFIG));
			if ((status = sdbrcm_dev_regwrite(sd, 0, SPID_CONFIG,
			                                  1, regdata)) != SUCCESS) {
				return status;
			}
		}
	} else {
		sd_err(("Attempting to disable High-Speed mode.\n"));

		if (regdata & HIGH_SPEED_MODE) {
			regdata &= (~HIGH_SPEED_MODE);
			sd_err(("Writing %08x to device at %08x\n", regdata, SPID_CONFIG));
			if ((status = sdbrcm_dev_regwrite(sd, 0, SPID_CONFIG,
			                                  1, regdata)) != SUCCESS)
				return status;
		}
		 else {
			sd_err(("Device is already in Low-Speed mode.\n"));
			return status;
		}
	}

	return TRUE;

}


static int
sdbrcm_driver_init(sdioh_info_t *sd)
{
	sd_trace(("%s\n", __FUNCTION__));
	if ((sdbrcm_host_init(sd)) != BCME_OK) {
		return (BCME_ERROR);
	}

	if (sdbrcm_dev_init(sd) != BCME_OK) {
		return (BCME_ERROR);
	}

	return (BCME_OK);
}

int
sdioh_sdio_reset(sdioh_info_t *sd)
{
	/* Reset the attached device */
	sdbrcm_reset(sd, 0, 1);
	return sdbrcm_driver_init(sd);
}

/* Read dev reg */
static int
sdbrcm_dev_regread(sdioh_info_t *sd, int fnum, uint32 regaddr, int regsize, uint32 *data)
{

	int status;
	uint32 cmd_arg;
	uint32 dstatus;

	ASSERT(regsize);

	if (fnum == 2)
		sd_err(("Reg access on F2 will generate error indication in dstatus bits.\n"));

	cmd_arg = 0;
	cmd_arg = SFIELD(cmd_arg, SPI_RW_FLAG, 0);
	cmd_arg = SFIELD(cmd_arg, SPI_ACCESS, 1);	/* Incremental access */
	cmd_arg = SFIELD(cmd_arg, SPI_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, SPI_REG_ADDR, regaddr);
	cmd_arg = SFIELD(cmd_arg, SPI_LEN, regsize);

	sd_data(("%s cmd_arg = 0x%x\n", __FUNCTION__, cmd_arg));

	if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, cmd_arg, data, regsize))
	    != SUCCESS)
		return status;

	sdbrcm_cmd_getdstatus(sd, &dstatus);
	if (dstatus)
		sd_data(("dstatus =0x%x\n", dstatus));
	return SUCCESS;
}

static int
sdbrcm_dev_regread_fixedaddr(sdioh_info_t *sd, int fnum, uint32 regaddr, int regsize, uint32 *data)
{

	int status;
	uint32 cmd_arg;
	uint32 dstatus;

	ASSERT(regsize);

	if (fnum == 2)
		sd_err(("Reg access on F2 will generate error indication in dstatus bits.\n"));

	cmd_arg = 0;
	cmd_arg = SFIELD(cmd_arg, SPI_RW_FLAG, 0);
	cmd_arg = SFIELD(cmd_arg, SPI_ACCESS, 0);	/* Fixed access */
	cmd_arg = SFIELD(cmd_arg, SPI_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, SPI_REG_ADDR, regaddr);
	cmd_arg = SFIELD(cmd_arg, SPI_LEN, regsize);

	sd_err(("%s cmd_arg = 0x%x\n", __FUNCTION__, cmd_arg));

	if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, cmd_arg, data, regsize))
	    != SUCCESS)
		return status;

	sdbrcm_cmd_getdstatus(sd, &dstatus);
	sd_data(("dstatus =0x%x\n", dstatus));
	return SUCCESS;
}
int
isr_sdbrcm_check_dev_intr(struct sdioh_pubinfo *sd_pub)
{
	uint32 cur_int = 0;
	bool ours;
	sdioh_info_t *sd;

	sd = (sdioh_info_t *)sd_pub;

	cur_int = R_REG(sd->osh, &(sd->sdioh_regs->combinedintstat));
	sd_err(("combinedintstat = 0x%x in %s\n", cur_int, __FUNCTION__));

	if (cur_int) {
		/* XXX use_dev_ints going away */
		if (sd->pub.dev_intr_enabled && sd->use_dev_ints) {
			sd->intrcount++;
			ASSERT(sd->intr_handler);
			ASSERT(sd->intr_handler_arg);
			(sd->intr_handler)(sd->intr_handler_arg);
		} else {
			sd_err(("%s: Not ready for intr: enabled %d, handler %p\n",
				__FUNCTION__, sd->pub.dev_intr_enabled, sd->intr_handler));
		}
		ours = TRUE;
	} else {
		ours = FALSE;
	}
	return ours;
}

/* write a dev register */
static int
sdbrcm_dev_regwrite(sdioh_info_t *sd, int fnum, uint32 regaddr, int regsize, uint32 data)
{
	int status;
	uint32 cmd_arg, dstatus;

	ASSERT(regsize);

	cmd_arg = 0;

	cmd_arg = SFIELD(cmd_arg, SPI_RW_FLAG, 1);
	cmd_arg = SFIELD(cmd_arg, SPI_ACCESS, 1);	/* Incremental access */
	cmd_arg = SFIELD(cmd_arg, SPI_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, SPI_REG_ADDR, regaddr);
	cmd_arg = SFIELD(cmd_arg, SPI_LEN, regsize);

	sd_data(("%s cmd_arg = 0x%x\n", __FUNCTION__, cmd_arg));

	if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, cmd_arg, &data, regsize))
	    != SUCCESS)
		return status;

	sdbrcm_cmd_getdstatus(sd, &dstatus);
	if (dstatus)
		sd_data(("dstatus =0x%x\n", dstatus));

	return (BCME_OK);
}

void
sdbrcm_cmd_getdstatus(sdioh_info_t *sd, uint32 *dstatus_buffer)
{
	*dstatus_buffer = sd->card_dstatus;
#if 0
	uint8 fn_ints;
	if (*dstatus_buffer & DATA_UNAVAILABLE) {
		if (sdioh_cfg_read(sd, 0, SPID_INTR_REG, &fn_ints)) {
			sd_err(("cfg_read of intr-reg to clear stale data-not-available "
			        "bit failed\n"));
			return;
		}
		sd_err(("intr-reg=0x%1x\n", fn_ints));
		fn_ints |= DATA_UNAVAILABLE;
		if (sdioh_cfg_write(sd, 0, SPID_INTR_REG, &fn_ints)) {
			return;
		}
		sd_err(("Cleared data-not-available bit in intr-reg. Val=0x%x\n", fn_ints));
	}
#endif /* 0 */
}

uint32 dstatus_count = 0;

static void
sdbrcm_update_stats(sdioh_info_t *sd, uint32 cmd_arg)
{
	uint32 dstatus = sd->card_dstatus;
	struct spierrstats_t *spierrstats = &sd->spierrstats;

	/* Store dstatus of last few gSPI transactions */
	spierrstats->dstatus[dstatus_count % NUM_PREV_TRANSACTIONS] = dstatus;
	spierrstats->spicmd[dstatus_count % NUM_PREV_TRANSACTIONS] = cmd_arg;
	dstatus_count++;

	/* dstatus is also available in F0 addr 0x8 */

	if (dstatus & STATUS_DATA_NOT_AVAILABLE) {
		spierrstats->dna++;
		sd_err(("Read data not available.\n"));
	}

	if (dstatus & STATUS_UNDERFLOW) {
		spierrstats->rdunderflow++;
		sd_err(("FIFO underflow happened due to current (F2/F3) read command.\n"));
	}

	if (dstatus & STATUS_OVERFLOW) {
		spierrstats->wroverflow++;
		sd_err(("FIFO overflow happened due to current (F1/F2/F3) write command.\n"));
	}

	if (dstatus & STATUS_F2_INTR) {
		spierrstats->f2interrupt++;
		sd_data(("Interrupt from F2.  SW should clear corresponding IntStatus bits\n"));
	}

	if (dstatus & STATUS_F3_INTR) {
		spierrstats->f3interrupt++;
		sd_err(("Interrupt from F3.  SW should clear corresponding IntStatus bits\n"));
	}

	if (dstatus & STATUS_F2_RX_READY) {
		spierrstats->f2rxnotready++;
		sd_data(("F2 FIFO is ready to receive data (FIFO empty)\n"));
	}

	if (dstatus & STATUS_F3_RX_READY) {
		spierrstats->f3rxnotready++;
		sd_err(("F3 FIFO is ready to receive data (FIFO empty)\n"));
	}

	if (dstatus & STATUS_HOST_CMD_DATA_ERR) {
		spierrstats->hostcmddataerr++;
		sd_err(("Error in CMD or Host data, detected by CRC/Checksum (optional)\n"));
	}

	if (dstatus & STATUS_F2_PKT_AVAILABLE) {
		spierrstats->f2pktavailable++;
		sd_trace(("Packet is available/ready in F2 TX FIFO\n"));
		sd_trace(("Packet length = %d\n",
		          (dstatus & STATUS_F2_PKT_LEN_MASK) >> STATUS_F2_PKT_LEN_SHIFT));
	}

	if (dstatus & STATUS_F3_PKT_AVAILABLE) {
		spierrstats->f3pktavailable++;
		sd_err(("Packet is available/ready in F3 TX FIFO\n"));
		sd_err(("Packet length = %d\n",
		        (dstatus & STATUS_F3_PKT_LEN_MASK) >> STATUS_F3_PKT_LEN_SHIFT));
	}
}

/* Program the response delay corresponding to the function */
static int
sdbrcm_prog_resp_delay(sdioh_info_t *sd, int fnum, uint32 resp_delay)
{
	uint32 cmd_arg, dstatus;
	uint32 datalen = 1;


	if (sd->resp_delay_all == FALSE)
		return (BCME_OK);

	if (sd->prev_fun == fnum)
		return (BCME_OK);

	sd_err(("%s: fnum=%d, resp_delay=%d\n", __FUNCTION__, fnum, resp_delay));
	sd_err(("Programming the response delay now.\n"));

	cmd_arg = 0;

	cmd_arg = SFIELD(cmd_arg, SPI_RW_FLAG, 1);
	cmd_arg = SFIELD(cmd_arg, SPI_ACCESS, 1);	/* Incremental access */
	cmd_arg = SFIELD(cmd_arg, SPI_FUNCTION, SPI_FUNC_0);
	cmd_arg = SFIELD(cmd_arg, SPI_REG_ADDR, SPID_RESPONSE_DELAY);
	cmd_arg = SFIELD(cmd_arg, SPI_LEN, datalen);

	sd_err(("%s cmd_arg = 0x%x\n", __FUNCTION__, cmd_arg));

#ifdef BCMDBG
	/* Fill up buffers with a value that generates known dutycycle on MOSI/MISO lines. */
	memset(spi_outrespdelaybuf, 0xee, RESP_DELAY_PKT_LEN);
	memset(spi_inrespdelaybuf, 0xee, RESP_DELAY_PKT_LEN);
#endif /* BCMDBG */

	/* Set up and issue the SPI command.  MSByte goes out on bus first.  Increase datalen
	 * according to the wordlen mode(16/32bit) the device is in.
	 */
	ASSERT(sd->wordlen == 4 || sd->wordlen == 2);
	datalen = ROUNDUP(datalen, sd->wordlen);

	/* Start by copying command in the spi-outbuffer */
	bcopy(&cmd_arg, spi_outrespdelaybuf, 4);

	/* for Write, put the data into the output buffer  */
	if (GFIELD(cmd_arg, SPI_RW_FLAG) == 1)
		bcopy(&resp_delay, spi_outrespdelaybuf + 4, datalen);

	/* +4 for cmd, +4 for dstatus, resp_delay only for Func-reads */
	sdbrcm_sendrecv(sd, (uint16 *)spi_outrespdelaybuf, (uint16 *)spi_inrespdelaybuf,
	                  datalen + 8);

	bcopy(&spi_inrespdelaybuf[datalen + CMDLEN], &sd->card_dstatus, 4);

	sdbrcm_update_stats(sd, cmd_arg);

	sdbrcm_cmd_getdstatus(sd, &dstatus);
	if (dstatus)
		sd_err(("dstatus after resp-delay programming = 0x%x\n", dstatus));

	/* Remember function for which to avoid reprogramming resp-delay in next iteration */
	sd->prev_fun = fnum;

	return (BCME_OK);
}

static int
sdbrcm_cmd_issue(sdioh_info_t *sd, bool use_dma, uint32 cmd_arg, uint32 *data, uint32 datalen)
{
	uint32	resp_delay = 0;

#ifdef BCMDBG
	/* Fill up buffers with a value that generates known dutycycle on MOSI/MISO lines. */
	memset(spi_outbuf, 0xee, SPI_MAX_PKT_LEN);
	memset(spi_inbuf, 0xee, SPI_MAX_PKT_LEN);
#endif /* BCMDBG */

	/* Set up and issue the SPI command.  MSByte goes out on bus first.  Increase datalen
	 * according to the wordlen mode(16/32bit) the device is in.
	 */
	ASSERT(sd->wordlen == 4 || sd->wordlen == 2);
	datalen = ROUNDUP(datalen, sd->wordlen);

	/* Start by copying command in the spi-outbuffer */
	bcopy(&cmd_arg, spi_outbuf, 4);

	/* for Write, put the data into the output buffer  */
	if (GFIELD(cmd_arg, SPI_RW_FLAG) == 1)
		bcopy(data, spi_outbuf + 4, datalen);

	/* Append resp-delay number of bytes and clock them out for F0/1/2 reads. */
	if ((GFIELD(cmd_arg, SPI_RW_FLAG) == 0)) {
		int func = GFIELD(cmd_arg, SPI_FUNCTION);
		switch (func) {
			case 0:
				if (sd->resp_delay_new)
					resp_delay = GSPI_F0_RESP_DELAY;
				else
					resp_delay = sd->resp_delay_all ? F0_RESPONSE_DELAY : 0;
				break;
			case 1:
				if (sd->resp_delay_new)
					resp_delay = GSPI_F1_RESP_DELAY;
				else
					resp_delay = F1_RESPONSE_DELAY;
				break;
			case 2:
				if (sd->resp_delay_new)
					resp_delay = GSPI_F2_RESP_DELAY;
				else
					resp_delay = sd->resp_delay_all ? F2_RESPONSE_DELAY : 0;
				break;
			default:
				ASSERT(0);
				break;
		}
		/* Program response delay */
		if (sd->resp_delay_new == FALSE)
			sdbrcm_prog_resp_delay(sd, func, resp_delay);
	}
	/* +4 for cmd, +4 for dstatus, resp_delay only for F1-reads */
	sdbrcm_sendrecv(sd, (uint16 *)spi_outbuf, (uint16 *)spi_inbuf,
	                  datalen + 8 + resp_delay);

	/* for Read, get the data into the input buffer */
	if (datalen != 0) {
		if (GFIELD(cmd_arg, SPI_RW_FLAG) == 0) /* if read cmd */
			bcopy(spi_inbuf + CMDLEN + resp_delay, data, datalen);
	}

	bcopy(&spi_inbuf[datalen + CMDLEN + resp_delay], &sd->card_dstatus, 4);

	sdbrcm_update_stats(sd, cmd_arg);

	return SUCCESS;
}

/*
 * On entry: If single block or non-block, buffersize <= blocksize.
 * If Mulitblock, buffersize is unlimited.
 * Question is how to handle the leftovers in either single or multiblock.
 * I think the caller should break the buffer up so this routine will always
 * use blocksize == buffersize to handle the end piece of the buffer
 */
static int
sdbrcm_dev_buf(sdioh_info_t *sd, int rw, int fnum, bool fifo,
                uint32 addr, int nbytes, uint32 *data)
{
	int status;
	uint32 cmd_arg;
	bool read = rw == SDIOH_READ ? 0 : 1;

	cmd_arg = 0;

	sd_data(("%s: %s fnum %d, %s, addr 0x%x, len %d bytes, r_cnt %d t_cnt %d\n",
	         __FUNCTION__, read ? "Wr" : "Rd", fnum, "INCR",
	         addr, nbytes, sd->r_cnt, sd->t_cnt));

	ASSERT(nbytes);

	ASSERT(nbytes <= sd->dev_block_size[fnum]);

	if (read) sd->t_cnt++; else sd->r_cnt++;

	/* F2 transfers happen on 0 addr */
	addr = (fnum == 2) ? 0 : addr;

	cmd_arg = SFIELD(cmd_arg, SPI_ACCESS, 1);	/* Incremental access */
	cmd_arg = SFIELD(cmd_arg, SPI_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, SPI_REG_ADDR, addr);
	cmd_arg = SFIELD(cmd_arg, SPI_RW_FLAG, read);
	/* Create command with '0' size for full block of 2048 bytes */
	sd->data_xfer_count = MIN(sd->dev_block_size[fnum], nbytes);
	cmd_arg = SFIELD(cmd_arg, SPI_LEN,
	                 sd->data_xfer_count == BLOCK_SIZE_F2 ? 0 : sd->data_xfer_count);

	if ((fnum == 2) && (fifo == 1)) {
		sd_data(("%s: %s fnum %d, %s, addr 0x%x, len %d bytes, r_cnt %d t_cnt %d\n",
		         __FUNCTION__, read ? "Wr" : "Rd", fnum, "INCR",
		         addr, nbytes, sd->r_cnt, sd->t_cnt));
	}

	/* sdspi_cmd_issue() returns with the command complete bit
	 * in the ISR already cleared
	 */
	if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, cmd_arg,
	     data, nbytes)) != SUCCESS) {
		sd_err(("%s: cmd_issue failed for %s\n", __FUNCTION__,
			(read ? "read" : "write")));
		return status;
	}

	/* gSPI expects that hw-header-len is equal to spi-command-len */
	if ((fnum == 2)) {
		sd_data(("size mismatch problem:spilen=0x%x,hwhdrlen= 0x%x\n",
		  sd->data_xfer_count, (*data & 0xffff)));

		ASSERT((uint16)sd->data_xfer_count == (uint16)(*data & 0xffff));
		ASSERT((uint16)sd->data_xfer_count == (uint16)(~((*data & 0xffff0000) >> 16)));
	}

	return SUCCESS;
}

static int sdbrcm_get_dev_block_size(sdioh_info_t *sd)
{
	uint32 regdata[2];
	int status;

	/* Find F1/F2/F3 max packet size */
	if ((status = sdbrcm_dev_regread(sd, 0, SPID_F1_INFO_REG,
	                                 8, regdata)) != SUCCESS) {
		return status;
	}

	sd_err(("pkt_size regdata[0] = 0x%x, regdata[1] = 0x%x\n",
	        regdata[0], regdata[1]));

	sd->dev_block_size[1] = (regdata[0] & F1_MAX_PKT_SIZE) >> 2;
	sd_err(("Func1 blocksize = %d \n", sd->dev_block_size[1]));
	ASSERT(sd->dev_block_size[1] == BLOCK_SIZE_F1);

	sd->dev_block_size[2] = ((regdata[0] >> 16) & F2_MAX_PKT_SIZE) >> 2;
	sd_err(("Func2 blocksize = %d \n", sd->dev_block_size[2]));
	ASSERT(sd->dev_block_size[2] == BLOCK_SIZE_F2);

	sd->dev_block_size[3] = (regdata[1] & F3_MAX_PKT_SIZE) >> 2;
	sd_err(("Func3 blocksize = %d \n", sd->dev_block_size[3]));
	ASSERT(sd->dev_block_size[3] == BLOCK_SIZE_F3);

	return 0;
}


static int
sdbrcm_set_dev_block_size(sdioh_info_t *sd, int fnum, int block_size)
{

	ASSERT(fnum <= sd->num_funcs);

	sd_info(("%s: Setting block size %d, fnum %d\n", __FUNCTION__, block_size, fnum));
	sd->dev_block_size[fnum] = block_size;

	return (BCME_OK);
}

#define PR_BIT_SET(reg, bit) \
	if (reg & bit) { sd_err((# bit " ")); }

#ifdef BCMDBG
static void
sdbrcm_dumpregs(sdioh_info_t *sd)
{
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;
}
#endif /* BCMDBG */

/* Attach to BCM-SPI Host Controller Hardware */
static bool
sdbrcm_hw_attach(sdioh_info_t *sd)
{
	osl_t *osh;

	sd_trace(("%s: enter\n", __FUNCTION__));

	osh = sd->osh;

	sd_err(("Mapped SPI Controller regs to BAR0 at %p\n", sd->sdioh_regs));


	/* Disable spih before configuring it */
	W_REG(osh, &sd->sdioh_regs->ssienr, 0x0);

	/* Initializing master in tx-rx mode */
	W_REG(osh, &sd->sdioh_regs->ctrlr0, 0xcf);

	/* set sclk division */
	W_REG(osh, &sd->sdioh_regs->baudr, 0x2);

	/* set interrupt polarity for spi_intr */
	W_REG(osh, &sd->sdioh_regs->propctrl, 0x2);

	/* enable the slave */
	W_REG(osh, &sd->sdioh_regs->ser, 0x1);

	/* disable spih interupts */
	W_REG(osh, &sd->sdioh_regs->imr, 0x0);

	/* Enable spih */
	W_REG(osh, &sd->sdioh_regs->ssienr, 0x1);

	sd_trace(("%s: exit\n", __FUNCTION__));
	return TRUE;
}

/* Detach and return PCI-SPI Hardware to unconfigured state */
static bool
sdbrcm_hw_detach(sdioh_info_t *sd)
{
	osl_t *osh = sd->osh;
	sdioh_regs_t *regs = sd->sdioh_regs;

	sd_trace(("%s: enter\n", __FUNCTION__));

	/* disable the slave */
	W_REG(osh, &sd->sdioh_regs->ser, 0x0);

	/* disable spih */
	W_REG(osh, &sd->sdioh_regs->ssienr, 0x0);

	/* Disable interrupts through PCI Core. */
	sdbrcm_reg_unmap(osh, (uintptr)regs, sizeof(sdioh_regs_t));

	sd_trace(("%s: exit\n", __FUNCTION__));
	return TRUE;
}

static void
hexdump(char *pfx, unsigned char *msg, int msglen)
{
	int i, col;
	char buf[80];

	ASSERT(strlen(pfx) + 49 <= sizeof(buf));

	col = 0;

	for (i = 0; i < msglen; i++, col++) {
		if (col % 16 == 0)
			strcpy(buf, pfx);
		sprintf(buf + strlen(buf), "%02x", msg[i]);
		if ((col + 1) % 16 == 0)
			printf("%s\n", buf);
		else
			sprintf(buf + strlen(buf), " ");
	}

	if (col % 16 != 0)
		printf("%s\n", buf);
}

/* Send/Receive an SPI Packet */
static void
sdbrcm_sendrecv(sdioh_info_t *sd, uint16 *msg_out, uint16 *msg_in, int msglen)
{
	osl_t *osh = sd->osh;
	sdioh_regs_t *regs = sd->sdioh_regs;

	uint32 count;
	uint16 spi_data_out;
	uint16 spi_data_in;
	uint8 sr;
	uint8 risr;
	uint32 i;

	sd_trace(("%s: enter\n", __FUNCTION__));

	if (bcmpcispi_dump) {
		sd_err(("SENDRECV(len=%d)\n", msglen));
		hexdump(" OUT: ", (unsigned char *)msg_out, msglen);
	}

	/* Sandwich fifo(dr) writes between control logic such that fifo is first filled and 
	 * then all the contents of it are dumped to the bus.  Follow steps mentioned below.
	 * -disable slave
	 * -fill the fifo
	 * -enable slave
	 * -poll on risr to monitor tx-fifo empty
	 */
	W_REG(osh, &sd->sdioh_regs->ser, 0x0);
	for (count = 0; count < msglen/2; count++) {
		spi_data_out = ((uint16)((uint16 *)msg_out)[count]);
		W_REG(osh, &regs->dr, spi_data_out);
	}
	W_REG(osh, &sd->sdioh_regs->ser, 0x1);
	risr = R_REG(osh, &regs->risr);
	for (i = 0; i < 1000; i++) {
		if ((risr & 0x1) == 0x1)
			break;
		OSL_DELAY(2000);
		risr = R_REG(osh, &regs->risr);
	}

	/* Wait for write fifo to empty... */
	sr = R_REG(osh, &regs->sr);
	for (i = 0; i < 1000; i++) {
		if ((sr & 0x1) == 0x0)
			break;
		OSL_DELAY(2000);
		sr = R_REG(osh, &regs->sr);
	}

	for (count = 0; count < msglen/2; count++) {
		spi_data_in = R_REG(osh, &regs->dr);
		((uint16 *)msg_in)[count] = spi_data_in;
	}

	if (bcmpcispi_dump) {
		hexdump(" IN : ", (unsigned char *)msg_in, msglen);
	}
}
