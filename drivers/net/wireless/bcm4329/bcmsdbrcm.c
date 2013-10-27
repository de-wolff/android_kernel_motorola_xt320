/*
 *  BRCM SDIO HOST CONTROLLER driver
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmsdbrcm.c,v 1.51.6.2 2010-08-27 19:15:38 dlhtohdl Exp $
 */

#include <typedefs.h>

#include <bcmdevs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <osl.h>
#include <hndsoc.h>

#include <siutils.h>
#include <sbhnddma.h>
#include <hnddma.h>

#include <sdio.h>
#include <sbsdioh.h>

#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <bcmsdbrcm.h>	/* SDIO Specs */
#include <sdiovar.h>

#define CMD5_RETRIES	200
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
	bool 		sd_blockmode;    	/* sd_blockmode == FALSE => 64 Byte Cmd 53s.
	                                         * This needs to be on for sd_multiblock to
	                                         * be effective
	                                         */
	bool 		use_dev_ints;		/* If this is false, make sure to turn on
	                                         * polling hack in wl_linux.c:wl_timer().
	                                         */

	int 		sd_mode;		/* SD1/SD4/SPI */
	int 		dev_block_size[SDIOD_MAX_IOFUNCS];		/* Blocksize */

	uint16 		dev_rca;		/* Current Address */
	uint8 		num_funcs;		/* Supported functions on dev */
	uint32 		com_cis_ptr;
	uint32 		func_cis_ptr[SDIOD_MAX_IOFUNCS];

	uint32 		data_xfer_count;	/* Current transfer */

	/* XXX - going away with HNDDMA */
	void		*dma_buf;
	ulong		dma_phys;

	hnddma_t	*di;		/* dma engine software state */
	si_t 		*sih;		/* si utils handle */

	uint		id;			/* CODEC/SDIOH device core ID */
	uint		rev;		/* CODEC/SDIOH device core revision */
	uint		unit;		/* CODEC/SDIOH unit number */

	int 		r_cnt;			/* rx count */
	int 		t_cnt;			/* tx_count */
	int 		intrcount;		/* Client interrupts */

};

/* XXX module params (maybe move to bcmsdbus, parse CIS to get block size) */
uint sd_hiok = FALSE;		/* Use hi-speed mode if available? */
uint sd_sdmode = SDIOH_MODE_SD4;	/* SD Mode to use (SPI, SD1, SD4) */
uint sd_f2_blocksize = 64;		/* Default blocksize */

#ifdef TESTDONGLE
uint sd_divisor = 0;		/* Default 48MHz/4 = 12MHz for dongle */
#else
uint sd_divisor = EXT_CLK;		/* Default 25MHz clock (extclk) otherwise */
#endif

uint sd_power = 1;		/* Default to SD Slot powered ON */
uint sd_clock = 1;		/* Default to SD Clock turned ON */
uint sd_pci_slot = 0xFFFFffff; /* Used to force selection of a particular PCI slot */		

static bool trap_errs = FALSE;		/* ASSERTs immediately upon detecting an SDIO error. */

/* Prototypes */
static int sdbrcm_start_clock(sdioh_info_t *sd);
static int sdbrcm_start_power(sdioh_info_t *sd);
static int sdbrcm_bus_mode(sdioh_info_t *sd, int mode);
static int sdbrcm_set_highspeed_mode(sdioh_info_t *sd, bool HSMode);
static int sdbrcm_dev_enablefunctions(sdioh_info_t *sd);
static void sdbrcm_cmd_getrsp(sdioh_info_t *sd, uint32 *rsp_buffer, int count);
static int sdbrcm_cmd_issue(sdioh_info_t *sd, bool use_dma, uint32 cmd, uint32 arg);
static int sdbrcm_dev_regread(sdioh_info_t *sd, int fnum, uint32 regaddr,
                               int regsize, uint32 *data);
static int sdbrcm_dev_regwrite(sdioh_info_t *sd, int fnum, uint32 regaddr,
                                                                int regsize, uint32 data);
static int sdbrcm_driver_init(sdioh_info_t *sd);
static int sdbrcm_reset(sdioh_info_t *sd, bool host_reset, bool dev_reset);
static int sdbrcm_dev_buf(sdioh_info_t *sd, int rw,
                          int fnum, bool fifo, uint32 addr,
                          int nbytes, uint32 *data);
static int sdbrcm_set_dev_block_size(sdioh_info_t *sd, int fnum, int blocksize);
static void sdbrcm_print_respq(sdioh_info_t *sd);
#ifdef BCMDBG
static void sdbrcm_dumpregs(sdioh_info_t *sd);
#endif
static void sdbrcm_print_sderror(sdioh_info_t *sd);
static void sdbrcm_print_sdints(sdioh_info_t *sd);
static void sdbrcm_print_rsp5_flags(uint8 resp_flags);

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
		sd_info(("sdioh_attach: rejecting non-broadcom device 0x%08x\n",
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

	/* XXX call siutils functions to do this... */
	/* make sure pci regsva window pointing to sdioh-codec core */
	OSL_PCI_WRITE_CONFIG(osh, PCI_BAR0_WIN, sizeof(uint32), (SI_ENUM_BASE +
	                                                         SDIOH_SB_ENUM_OFFSET));

	if (!(sd->sih = si_attach(SDIOH_FPGA_ID, osh, (void *)sd->sdioh_regs,
	                          PCI_BUS, NULL, NULL, 0))) {
		sd_err(("si_attach() failed.\n"));
	}

	sd->id = si_coreid(sd->sih);
	sd->rev = si_corerev(sd->sih);
	sd->unit = 0;

	if ((sd->id != CODEC_CORE_ID) || (sd->rev < 4)) {
		sd_err(("Not a SDIO host\n"));
		goto sdioh_attach_fail;
	}

	sd_err(("sdbrcm%d: found SDIO host(rev %d) in pci device, membase %p\n",
	        sd->unit, sd->rev, sd->sdioh_regs));

	/* XXX THIS SECTION SEEMS FPGA-SPECIFIC, SO PUT INTMASK HERE TOO */
	/* Enable interrupts from the sdioh core onto the PCI bus */
	/* XXX si_pci_setup() config pci ... */
	OSL_PCI_WRITE_CONFIG(osh, PCI_INT_MASK, sizeof(uint32), 0x800);

	sd->irq = irq;

	if (sd->sdioh_regs == NULL) {
		sd_err(("%s:ioremap() failed\n", __FUNCTION__));
		goto sdioh_attach_fail;
	}

	sd_info(("%s:sd->sdioh_regs = %p\n", __FUNCTION__, sd->sdioh_regs));

	/* Set defaults */
	sd->sd_blockmode = FALSE; /* no real block mode until DMA is implemented. */
	sd->use_dev_ints = TRUE;
	sd->sd_use_dma = FALSE; /* DMA not implemented yet. */

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

	/* free sb handle */
	si_detach(sd->sih);
	sd->sih = NULL;

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
	uint32 intrstatus;

	intrstatus = R_REG(sd->osh, &(sd->sdioh_regs->intstatus));
	return !!(intrstatus & INT_DEV_INT);
}
#endif

uint
sdioh_query_iofnum(sdioh_info_t *sd)
{
	return sd->num_funcs;
}

/* IOVar table */
enum {
	IOV_MSGLEVEL = 1,
	IOV_BLOCKMODE,
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
	IOV_CLOCK
};

const bcm_iovar_t sdioh_iovars[] = {
	{"sd_msglevel",	IOV_MSGLEVEL, 	0,	IOVT_UINT32,	0 },
	{"sd_blockmode", IOV_BLOCKMODE,	0,	IOVT_BOOL,	0 },
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

	case IOV_GVAL(IOV_BLOCKMODE):
		int_val = (int32)sd->sd_blockmode;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_BLOCKMODE):
		sd->sd_blockmode = bool_val;
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
			W_REG(osh, &(regs->mode),
			      R_REG(osh, &(regs->mode)) & ~(MODE_CLK_OUT_EN | MODE_CLK_DIV_MASK));

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
			bcopy(&int_val, arg, sizeof(int_val));
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
		bcopy(&int_val, arg, sizeof(int_val));
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

int
sdioh_cis_read(sdioh_info_t *sd, uint fnum, uint8 *cisd, uint32 length)
{
	uint32 count;
	int offset;
	uint32 cis_byte;
	uint8 *cis = cisd;

	ASSERT(fnum <= sd->num_funcs);

	sd_trace(("%s: Func %d\n", __FUNCTION__, fnum));

	if (!sd->func_cis_ptr[fnum]) {
		return (BCME_ERROR);
	}

	sdbrcm_lock(sd);
	offset =  sd->func_cis_ptr[fnum];
	for (count = 0; count < length; count++) {
		if (sdbrcm_dev_regread (sd, 0, offset, 1, &cis_byte) < 0) {
			sd_err(("%s: regread failed: Can't read CIS\n", __FUNCTION__));
			sdbrcm_unlock(sd);
			return (BCME_ERROR);
		}

		*cis = (uint8)(cis_byte & 0xff);
		cis++;
		offset++;
	}

	sdbrcm_unlock(sd);
	return (BCME_OK);
}

int
sdioh_request_byte(sdioh_info_t *sd, uint rw, uint fnum, uint regaddr, uint8 *byte)
{
	int status;
	uint32 cmd_arg;
	uint32 rsp5;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	ASSERT(fnum <= sd->num_funcs);

	sdbrcm_lock(sd);

	sd_info(("%s: F%d[addr] 0x%x\n",
		__FUNCTION__, fnum, regaddr));

	cmd_arg = 0;
	cmd_arg = SFIELD(cmd_arg, CMD52_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, CMD52_REG_ADDR, regaddr);
	cmd_arg = SFIELD(cmd_arg, CMD52_RW_FLAG, rw == SDIOH_READ ? 0 : 1);
	cmd_arg = SFIELD(cmd_arg, CMD52_RAW, 0);
	cmd_arg = SFIELD(cmd_arg, CMD52_DATA, rw == SDIOH_READ ? 0 : *byte);

	/* XXX Check what compiler generates on the above. */
	if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, SDIOH_CMD_52, cmd_arg)) != BCME_OK) {
		sdbrcm_unlock(sd);
		return status;
	}

	sdbrcm_cmd_getrsp(sd, &rsp5, 1);

	if (GFIELD(rsp5, RSP5_FLAGS) !=	SD_RSP_R5_IO_CURRENTSTATE0)
		sd_err(("%s: rsp5 flags is 0x%x\t %d\n",
			__FUNCTION__, GFIELD(rsp5, RSP5_FLAGS), fnum));

	if (rw == SDIOH_READ)
		*byte = GFIELD(rsp5, RSP5_DATA);

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
		addr, (fifo) ? "(FIFO)" : "(INCR)", buflen_u, sd->r_cnt, sd->t_cnt, pkt));

	/* Break buffer down into blocksize chunks:
	 * Bytemode: 1 block at a time.
	 * Blockmode: Multiples of blocksizes at a time w/ max of SD_PAGE.
	 * Both: leftovers are handled last (will be sent via bytemode.
	 */
	while (buflen > 0) {
		if (sd->sd_blockmode) {
			/* XXX This is all changing for BRCM/DMA/Blockmode */
			/* Max xfer is Page size */
			len = MIN(SD_PAGE, buflen);

			/* Round down to a block boundry */
			if (buflen > sd->dev_block_size[fnum])
				len = (len/sd->dev_block_size[fnum]) *
					sd->dev_block_size[fnum];
			/* End of changes for BRCM/DMA/Blockmode */
		} else {
			/* Byte mode: One block at a time */
			/* negotiated block size. */
			/* non-blockmode still stuck at 64bytes for BRCM controller. */
			len = MIN(sd->dev_block_size[fnum], buflen);
		}

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
	int retries;

	uint16 cmd_reg;
	uint32 cmd_arg;
	uint32 rsp5;
	uint8 rflags;

	uint32 pending_ints;
	uint32 errstatus;

	osl_t *osh;
	sdioh_regs_t *regs;

	sd_trace(("%s: Enter\n", __FUNCTION__));

	osh = sd->osh;
	regs = sd->sdioh_regs;

	/* Argument is write to F0 (CCCR) IOAbort with function number */
	cmd_arg = 0;
	cmd_arg = SFIELD(cmd_arg, CMD52_FUNCTION, SDIO_FUNC_0);
	cmd_arg = SFIELD(cmd_arg, CMD52_REG_ADDR, SDIOD_CCCR_IOABORT);
	cmd_arg = SFIELD(cmd_arg, CMD52_RW_FLAG, SD_IO_OP_WRITE);
	cmd_arg = SFIELD(cmd_arg, CMD52_RAW, 0);
	cmd_arg = SFIELD(cmd_arg, CMD52_DATA, fnum);

	/* Command is CMD52 write */
	cmd_reg = 0;
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_R5);
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 0);
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 0);
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 1);
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_EXP_BSY, 1); /* XXX */
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 1);
	cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, SDIOH_CMD_52);

	/* XXX Copied from cmd_issue(), but no SPI response handling! */
	if (sd->sd_mode == SDIOH_MODE_SPI) {
		cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 1);
		cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 1);
	}

	/* Wait until the host controller is finished with previous command.
	 * (Equivalent to the CMD_INHIBIT wait in the standard host.)
	 * XXX Not sure this is relevant in a single-threaded driver, only
	 * case where it'd seem to apply is if a previous command bailed
	 * way early...
	 */
	retries = RETRIES_SMALL;
	while (GFIELD(R_REG(osh, &(regs->intstatus)), INT_HOST_BUSY)) {
		if (retries == RETRIES_SMALL)
			sd_err(("%s: Waiting for Host Busy, intstatus 0x%08x\n",
			        __FUNCTION__, R_REG(osh, &regs->intstatus)));
		if (!--retries) {
			sd_err(("%s: Host Busy timeout, intstatus 0x%08x\n",
			        __FUNCTION__, R_REG(osh, &regs->intstatus)));
			if (trap_errs)
				ASSERT(0);
			err = BCME_SDIO_ERROR;
			goto done;
		}
	}

	/* Setup and issue the SDIO command */
	W_REG(osh, &(regs->cmddat), cmd_reg);

	/* writing the cmdl register starts the SDIO bus transaction. */
	W_REG(osh, &(regs->cmdl), cmd_arg);

	/* In interrupt mode return, expect later CMDATDONE interrupt */
	if (!sd->polled_mode)
		return err;

	/* Otherwise, wait for the command to complete */
	retries = RETRIES_LARGE;
	do {
		pending_ints = R_REG(osh, &(regs->intstatus));
	} while (--retries && (GFIELD(pending_ints, INT_CMD_DAT_DONE) == 0));

	/* Timeout should be considered an error */
	if (!retries) {
		sd_err(("%s: CMD_DAT_DONE timeout\n", __FUNCTION__));
		sdbrcm_print_respq(sd);
		if (trap_errs)
			ASSERT(0);
		err = BCME_SDIO_ERROR;
	}

	/* Clear Command Complete interrupt */
	pending_ints = SFIELD(0, INT_CMD_DAT_DONE, 1);
	W_REG(osh, &(regs->intstatus), pending_ints);

	/* Check for Errors */
	if ((errstatus = R_REG(osh, &regs->error)) != 0) {
		sd_err(("%s: error after ABORT%s: errstatus 0x%08x\n",
		        __FUNCTION__, (err ? " (timed out)" : ""), errstatus));
		sdbrcm_print_respq(sd);
		err = BCME_SDIO_ERROR;

		if (trap_errs)
			ASSERT(0);
	}

	/* If command failed don't bother looking at response */
	if (err)
		goto done;

	/* Otherwise, check the response */
	sdbrcm_cmd_getrsp(sd, &rsp5, 1);
	rflags = GFIELD(rsp5, RSP5_FLAGS);

	if (rflags & SD_RSP_R5_ERRBITS) {
		sd_err(("%s: R5 flags include errbits: 0x%02x\n", __FUNCTION__, rflags));

		/* The CRC error flag applies to the previous command */
		if (rflags & (SD_RSP_R5_ERRBITS & ~SD_RSP_R5_COM_CRC_ERROR)) {
			err = BCME_SDIO_ERROR;
			goto done;
		}
	}

	if (((rflags & (SD_RSP_R5_IO_CURRENTSTATE0 | SD_RSP_R5_IO_CURRENTSTATE1)) != 0x10) &&
	    ((rflags & (SD_RSP_R5_IO_CURRENTSTATE0 | SD_RSP_R5_IO_CURRENTSTATE1)) != 0x20)) {
		sd_err(("%s: R5 flags has bad state: 0x%02x\n", __FUNCTION__, rflags));
		err = BCME_SDIO_ERROR;
		goto done;
	}

	if (GFIELD(rsp5, RSP5_STUFF)) {
		sd_err(("%s: rsp5 stuff is 0x%x: should be 0\n",
		        __FUNCTION__, GFIELD(rsp5, RSP5_STUFF)));
		err = BCME_SDIO_ERROR;
		goto done;
	}

done:
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
	return TRUE;
}

int
sdioh_stop(sdioh_info_t *sd)
{
	return TRUE;
}

int
sdioh_waitlockfree(sdioh_info_t *sd)
{
	return TRUE;
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
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	/* XXX get rid of this when we can */
	if (!sd)
		return TRUE;

	sdbrcm_lock(sd);

	/* Reset device */
	if (dev_reset) {
		sd_info(("%s: Resetting SD Card\n", __FUNCTION__));
		if (sdbrcm_dev_regwrite(sd, 0, SDIOD_CCCR_IOABORT,
		                        1, IO_ABORT_RESET_ALL) != BCME_OK) {
			sd_err(("%s: Cannot write to dev reg 0x%x\n",
			        __FUNCTION__, SDIOD_CCCR_IOABORT));
		}
		sd->dev_rca = 0;
		sd->pub.dev_init_done = FALSE;
	}

	/* Reset host controller */
	if (host_reset) {
		uint32 curmode;

		sd_info(("sdioh_host_reset\n"));

		curmode = R_REG(osh, &(regs->devcontrol));

		/* Clear SDIOH Mode bit in DEVCONTROL register to 
		 * reset SDIOH, then assert it again.
		 */
		curmode &= ~CODEC_DEVCTRL_SDIOH;
		W_REG(osh, &(regs->devcontrol), curmode);
		sd_trace(("%s: SDIOH Controller Reset\n", __FUNCTION__));

		/* XXX check this with the BRCM HC Spec */
		OSL_DELAY(100);

		/* XXX  why is this not a core reset squence */
		curmode |= CODEC_DEVCTRL_SDIOH;
		W_REG(osh, &(regs->devcontrol), curmode);


		/* set HC timeout registers */
		W_REG(osh, &(regs-> rbto), SD_TIMEOUT);
		W_REG(osh, &(regs-> rdto), SD_TIMEOUT);
		W_REG(osh, &(regs-> arvm), ARVM_MASK);

		/* A reset should reset bus back to 1 bit mode */
		sd->sd_mode = SDIOH_MODE_SD1;
	}
	sdbrcm_unlock(sd);
	return TRUE;
}

/* Disable device interrupt wihtoug going over the SD bus */
void
sdbrcm_devintr_off(sdioh_info_t *sd)
{
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_dev_ints));
	if (sd->use_dev_ints) {
		sd->intmask &= ~INT_DEV_INT;
		W_REG(sd->osh, &(sd->sdioh_regs->intmask), sd->intmask);
		R_REG(sd->osh, &(sd->sdioh_regs->intmask));
	}
}

/* Enable device interrupt without going over the SD bus */
void
sdbrcm_devintr_on(sdioh_info_t *sd)
{
	ASSERT(sd->pub.lockcount == 0);
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_dev_ints));
	if (sd->use_dev_ints) {
		sd_trace(("%s: host status 0x%08x mask 0x%08x\n",
		          __FUNCTION__, R_REG(sd->osh, &(sd->sdioh_regs->intstatus)),
		          R_REG(sd->osh, &(sd->sdioh_regs->intmask))));
		sd->intmask |= INT_DEV_INT;
		W_REG(sd->osh, &(sd->sdioh_regs-> intmask), sd->intmask);
	}
}

static int
sdbrcm_host_init(sdioh_info_t *sd)
{

	uint32		reg32;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;


	/* Reset Host Controller but not the attached device. */
	sdbrcm_reset(sd, 1, 0);

	/* Read SD4/SD1 mode */
	if ((reg32 = R_REG(osh, &(regs->mode)))) {
		/* XXX clean this up, use sd mode value and change if ncecesary. */
		if ((reg32 & 0x03) == SD1_MODE) {
			sd_info(("%s: Host cntrlr already in 1 bit mode: 0x%x\n",
			        __FUNCTION__,  reg32));
		}
	}

	/* Default power on mode is SD1 */
	sd->sd_mode = SDIOH_MODE_SD1;
	sd->polled_mode = TRUE; /* going away */
	sd->pub.host_init_done = TRUE;
	sd->pub.dev_init_done = FALSE;

	return (BCME_OK);
}

static int
sdbrcm_get_ocr(sdioh_info_t *sd, uint32 *cmd_arg, uint32 *cmd_rsp)
{
	unsigned int countdown, status;

	/* Get the Card's Operation Condition.  Occasionally the board
	 * takes a while to become ready
	 */
	countdown = CMD5_RETRIES;
	do {
		*cmd_rsp = 0;
		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
		                               SDIOH_CMD_5, *cmd_arg)) != BCME_OK) {
			sd_err(("%s: CMD5 failed\n", __FUNCTION__));
			return status;
		}

		sdbrcm_cmd_getrsp(sd, cmd_rsp, NUM_RESP_WORDS);

		/* Make the SPI-mode RSP4 look like the SD RSP4 */
		if (sd->sd_mode == SDIOH_MODE_SPI) {
			*cmd_rsp = *cmd_rsp << 24;
		}

		if (!GFIELD(*cmd_rsp, RSP4_CARD_READY))
			sd_trace(("%s: Waiting for dev to become ready\n", __FUNCTION__));
	} while ((!GFIELD(*cmd_rsp, RSP4_CARD_READY)) && --countdown);
	if (!countdown)
		return (BCME_ERROR);

	return (BCME_OK);
}

static int
sdbrcm_dev_init(sdioh_info_t *sd)
{
	uint32 cmd_arg, cmd_rsp;
	int status;
	uint8 fn_ints;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_info(("%s() intstatus=0x%08x\n",
	         __FUNCTION__,
	         R_REG(osh, &(regs->intstatus))));

	/* Clear any pending ints */
	W_REG(osh, &(regs->intstatus), 0xffffffff);

	sd_info(("%s() after clearing intstatus=0x%08x\n",
	         __FUNCTION__,
	         R_REG(osh, &(regs->intstatus))));

	/* Enable both Normal and Error Status.  This does not enable
	 * interrupts, it only enables the status bits to
	 * become 'live'
	 */

	/* XXX creat a define for default interrupt mask */
	sd_info(("%s() intmask=0x%08x\n",
	         __FUNCTION__,
	         R_REG(osh, &(regs->intmask))));

	sd->intmask &= ~INT_DEV_INT;
	W_REG(osh, &(regs->intmask), sd->intmask);

	if (!sdbrcm_start_clock(sd)) {
		sd_err(("sdbrcm_start_clock failed\n"));
		return (BCME_ERROR);
	}

	sd->sd_mode = sd_sdmode;

	/* In SPI mode, issue CMD0 first */
	if (sd->sd_mode == SDIOH_MODE_SPI) {
		uint32 curmode, newmode;

		curmode = R_REG(osh, &(regs->mode));
		newmode = (curmode & ~MODE_OP_MASK);
		W_REG(osh, &(regs->mode), newmode);

		sd_err(("Host controller set to SPI mode.\n"));

		/* XXX figure out this delay. */
		OSL_DELAY(20); /* I get errors without delay or the printf */
		sd_info(("SDMODE switch from 0x%08x to 0x%08x\n", curmode, newmode));

		cmd_arg = 0;
		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
		                               SDIOH_CMD_0, cmd_arg)) != BCME_OK) {
			sd_err(("BCMSDIOH: devinit: CMD0 failed!\n"));
			return status;
		}
	}

	if (!sdbrcm_start_power(sd)) {
		sd_err(("sdbrcm_start_power failed\n"));
		return (BCME_ERROR);
	}

	/* check number of functions here, not in start_power */
	if (sd->num_funcs == 0) {
		sd_err(("%s: No IO functions!\n", __FUNCTION__));
		return (BCME_ERROR);
	}

	/* XXX comment here */
	if (sd->sd_mode != SDIOH_MODE_SPI) {
		uint32 rsp6_status;

		/* Card is operational. Ask it to send an RCA */
		cmd_arg = 0;
		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
		                               SDIOH_CMD_3, cmd_arg)) != BCME_OK) {
			sd_err(("%s: CMD3 failed!\n", __FUNCTION__));
			return status;
		}

		/* Verify the dev status returned with the cmd response */

		sdbrcm_cmd_getrsp(sd, &cmd_rsp, NUM_RESP_WORDS);

		rsp6_status = GFIELD(cmd_rsp, RSP6_STATUS);
		if (GFIELD(rsp6_status, RSP6STAT_COM_CRC_ERROR) ||
		    GFIELD(rsp6_status, RSP6STAT_ILLEGAL_CMD) ||
		    GFIELD(rsp6_status, RSP6STAT_ERROR)) {
			sd_err(("%s: CMD3 response error. Response = 0x%x!\n",
			        __FUNCTION__, rsp6_status));
			return (BCME_ERROR);
		}

		/* Save the Card's RCA */
		sd->dev_rca = GFIELD(cmd_rsp, RSP6_IO_RCA);
		sd_info(("RCA is 0x%x\n", sd->dev_rca));

		/* Select the dev */
		cmd_arg = SFIELD(0, CMD7_RCA, sd->dev_rca);
		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
		                               SDIOH_CMD_7, cmd_arg)) != BCME_OK) {
			sd_err(("%s: CMD7 failed!\n", __FUNCTION__));
			return status;
		}

		sdbrcm_cmd_getrsp(sd, &cmd_rsp, NUM_RESP_WORDS);

		if (cmd_rsp != SDIOH_CMD7_EXP_STATUS) {
			sd_err(("%s: CMD7 response error. Response = 0x%x!\n",
			        __FUNCTION__, cmd_rsp));
			return (BCME_ERROR);
		}
	}

	/* enable function 1 */
	sdbrcm_dev_enablefunctions(sd);

	if (!sdbrcm_bus_mode(sd, sd_sdmode)) {
		sd_err(("sdbrcm_bus_mode failed\n"));
		return (BCME_ERROR);
	}

	/* Change to High-Speed Mode if both host and device support it. */
	sdbrcm_set_highspeed_mode(sd, sd_hiok);

	/* XXX read blocksize from CIS.  This will move to the BCMSDH layer. */
	sdbrcm_set_dev_block_size(sd, 1, BLOCK_SIZE_4318);

	fn_ints = INTR_CTL_FUNC1_EN;

	if (sd->num_funcs >= 2) {
		/* XXX Device side can't handle 512 yet */
		/* XXX do we really want to enable interrupts here? */
		sdbrcm_set_dev_block_size(sd, 2, sd_f2_blocksize /* BLOCK_SIZE_4328 */);
		fn_ints |= INTR_CTL_FUNC2_EN;
	}

	/* Enable/Disable Client interrupts */
	if (sdbrcm_dev_regwrite(sd, 0, SDIOD_CCCR_INTEN, 1,
	                        (fn_ints | INTR_CTL_MASTER_EN)) != BCME_OK) {
		sd_err(("%s: Could not enable ints in CCCR\n", __FUNCTION__));
		return (BCME_ERROR);
	}

	sd->pub.dev_init_done = TRUE;

	return (BCME_OK);
}

int
sdbrcm_start_clock(sdioh_info_t *sd)
{
	uint32 divisor_reg;
	uint32 mode_reg;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_info(("Clock Freq = 25 Mhz\n"));

	/*  Disable the HC clock and clear clock divisor bits */
	W_REG(osh, &(regs->mode),
		R_REG(osh, &(regs->mode)) &
	                ~(MODE_CLK_OUT_EN | MODE_CLK_DIV_MASK));

	/* Set divisor */
	if ((sd_divisor > 15) && (sd_divisor != EXT_CLK))
	{
		sd_err(("Invalid clock divisor target: %d\n", sd_divisor));
		return FALSE;
	}

	if (sd_divisor != EXT_CLK)
	{ /* Use internal clock */
		divisor_reg = (sd_divisor  & 0x0F) << 4;

		sd_err(("%s: Using clock divisor of 0x%x "
		        "(reg_value=0x%08x = %d Hz)\n",
		        __FUNCTION__,
		        sd_divisor, divisor_reg, 25000000 / (2 << (sd_divisor  & 0x0F))));

		mode_reg = R_REG(osh, &(regs->mode));
		mode_reg &= ~MODE_CLK_DIV_MASK;
		mode_reg &= ~MODE_USE_EXT_CLK;
		mode_reg |= divisor_reg;
		W_REG(osh, &(regs->mode), mode_reg);

	}
	else
	{ /* Use external Clock */
		sd_info(("%s: Using EXTERNAL clock.\n", __FUNCTION__));

		mode_reg = R_REG(osh, &(regs->mode));
		mode_reg |= MODE_USE_EXT_CLK;
		W_REG(osh, &(regs->mode), mode_reg);

	}

	/* enable SDIO clock output */
	W_REG(osh, &(regs->mode),
	    R_REG(osh, &(regs->mode)) | (MODE_CLK_OUT_EN));

	sd_info(("Final Mode register is 0x%x\n", R_REG(osh, &(regs->mode))));
	return TRUE;
}

int
sdbrcm_start_power(sdioh_info_t *sd)
{
	uint32 cmd_arg;
	uint32 cmd_rsp;

	sd_trace(("%s: BRCM SDIOH Controller is permanently powered at 3.3 Volts.\n",
	          __FUNCTION__));

	/* Get the Card's Operation Condition.  Occasionally the board
	 * takes a while to become ready
	 */
	cmd_arg = 0;
	cmd_rsp = 0;
	if (sdbrcm_get_ocr(sd, &cmd_arg, &cmd_rsp) != BCME_OK) {
		sd_err(("%s: Failed to get OCR bailing\n", __FUNCTION__));
		return FALSE;
	}
	sd_info(("mem_present = %d\n", GFIELD(cmd_rsp, RSP4_MEM_PRESENT)));
	sd_info(("num_funcs = %d\n", GFIELD(cmd_rsp, RSP4_NUM_FUNCS)));
	sd_info(("dev_ready = %d\n", GFIELD(cmd_rsp, RSP4_CARD_READY)));
	sd_info(("OCR = 0x%x\n", GFIELD(cmd_rsp, RSP4_IO_OCR)));

	/* Verify that the dev supports I/O mode */
	if (GFIELD(cmd_rsp, RSP4_NUM_FUNCS) == 0) {
		sd_err(("%s: Card does not support I/O\n", __FUNCTION__));
		return (BCME_ERROR);
	}
	sd->num_funcs = GFIELD(cmd_rsp, RSP4_NUM_FUNCS); /* XXX set above then check. */

	/* Examine voltage: BRCM only supports 3.3 volts,
	 * so look for 3.2-3.3 Volts and also 3.3-3.4 volts.
	 * Pg 10 SDIO spec v1.10
	 */
	if ((GFIELD(cmd_rsp, RSP4_IO_OCR) & (0x3 << 20)) == 0) { /* #defines for these */
		sd_err(("This device does not support 3.3 volts!\n"));
		return (BCME_ERROR);
	}
	sd_info(("Leaving bus power at 3.3 Volts\n"));

	/* XXX comment and fix magic value */
	cmd_arg = SFIELD(0, CMD5_OCR, 0xfff000);
	cmd_rsp = 0;
	sdbrcm_get_ocr(sd, &cmd_arg, &cmd_rsp);  /* XXX why get if we're setting? change fn name? */
	sd_info(("OCR = 0x%x\n", GFIELD(cmd_rsp, RSP4_IO_OCR)));
	return TRUE;
}

int
sdbrcm_bus_mode(sdioh_info_t *sd, int new_mode)
{
	uint32 regdata;
	int status;
	uint32 curmode, newmode;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_trace(("%s\n", __FUNCTION__));
	if (sd->sd_mode == new_mode) {
		sd_err(("%s: Already at width %d\n", __FUNCTION__, new_mode));
	}

	/* Set dev side via reg 0x7 in CCCR */
	if ((status = sdbrcm_dev_regread(sd, 0, SDIOD_CCCR_BICTRL, 1, &regdata)) != BCME_OK)
		return status;
	regdata &= ~BUS_SD_DATA_WIDTH_MASK; /* XXX are device bits defined for GFIELD/SFIELD? */

	if (new_mode == SDIOH_MODE_SD4) {
		sd_info(("Changing to SD4 Mode\n"));
		regdata |= SD4_MODE;
	} else if (new_mode == SDIOH_MODE_SD1) {
		sd_info(("Changing to SD1 Mode\n"));
	} else {
		sd_info(("In SPI Mode.\n"));
	}

	if ((status = sdbrcm_dev_regwrite(sd, 0, SDIOD_CCCR_BICTRL, 1, regdata)) != BCME_OK)
		return status;

	/* XXX comment here */
	curmode = R_REG(osh, &(regs->mode));
	newmode = (curmode & ~MODE_OP_MASK) | new_mode;
	W_REG(osh, &(regs->mode), newmode);
	sd->sd_mode = new_mode;

	/* XXX figure out this delay. */
	OSL_DELAY(20); /* I get errors without delay or the printf */
	sd_info(("SDMODE switch from 0x%08x to 0x%08x\n", curmode, newmode));

	return TRUE;
}

static int
sdbrcm_set_highspeed_mode(sdioh_info_t *sd, bool HSMode)
{
	uint32 regdata;
	int status;
	uint32 curmode, newmode;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	/* Set dev and host into High-speed mode if desired. */
	if (HSMode == TRUE) {
		sd_info(("Attempting to enable High-Speed mode.\n"));

		if ((status = sdbrcm_dev_regread(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
		                                 1, &regdata)) != BCME_OK) {
			return status;
		}
		if (regdata & SDIO_SPEED_SHS) {
			sd_info(("Device supports High-Speed mode.\n"));

			regdata |= SDIO_SPEED_EHS;

			sd_info(("Writing %08x to Card at %08x\n",
			         regdata, SDIOD_CCCR_SPEED_CONTROL));
			if ((status = sdbrcm_dev_regwrite(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
			                                  1, regdata)) != BCME_OK) {
				return status;
			}

			if ((status = sdbrcm_dev_regread(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
			                                 1, &regdata)) != BCME_OK) {
				return status;
			}

			sd_info(("Read %08x to Card at %08x\n", regdata, SDIOD_CCCR_SPEED_CONTROL));

			/* XXX comment here */
			curmode = R_REG(osh, &(regs->mode));
			newmode = (curmode & ~MODE_HIGHSPEED_EN) | MODE_HIGHSPEED_EN;
			sd_info(("Writing %08x to Host MODE reg.\n", newmode));
			W_REG(osh, &(regs->mode), newmode);

			/* Check to see if the HIGHSPEED_EN bit write took.  Only the latest */
			/* SDIOH Controllers Support High-Speed Mode. (but they don't work in */
			/* HS mode yet. */
			if ((R_REG(osh, &(regs->mode)) & MODE_HIGHSPEED_EN) == 0) {
				sd_err(("Host does not support High-Speed mode.\n"));
			}

			sd_err(("High-speed clocking mode enabled.\n"));
		}
		else {
			sd_err(("Device does not support High-Speed Mode.\n"));
		}
	} else {
		sd_err(("Low-speed clocking mode enabled.\n"));
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

static int
sdbrcm_dev_enablefunctions(sdioh_info_t *sd)
{
	int status;
	uint32 regdata;
	uint32 regaddr, fbraddr;
	uint8 fnum;
	uint8 *ptr;

	sd_trace(("%s\n", __FUNCTION__));

	/* Get the Card's common CIS address */
	ptr = (uint8 *) &sd->com_cis_ptr;
	for (regaddr = SDIOD_CCCR_CISPTR_0; regaddr <= SDIOD_CCCR_CISPTR_2; regaddr++) {
		if ((status = sdbrcm_dev_regread(sd, 0, regaddr, 1, &regdata)) != BCME_OK)
			return status;

		*ptr++ = (uint8) regdata;
	}

	/* XXX make endian safe. - ltoh */
	/* Only the lower 17-bits are valid */
	sd->com_cis_ptr &= 0x0001FFFF;	/* XXX #define for 17-bit mask in sdio.h */
	sd->func_cis_ptr[0] = sd->com_cis_ptr;  /* get rid of com_cis_pointer */
	sd_info(("%s: Card's Common CIS Ptr = 0x%x\n", __FUNCTION__, sd->com_cis_ptr));

	/* Get the CIS address for each function */
	for (fbraddr = SDIOD_FBR_STARTADDR, fnum = 1;
	     fnum <= sd->num_funcs;
	     fnum++, fbraddr += SDIOD_FBR_SIZE) {
		ptr = (uint8 *) &sd->func_cis_ptr[fnum];
		for (regaddr = SDIOD_FBR_CISPTR_0; regaddr <= SDIOD_FBR_CISPTR_2; regaddr++) {
			if ((status = sdbrcm_dev_regread(sd, 0, regaddr + fbraddr,
			                                  1, &regdata)) != BCME_OK)
				return status;

			*ptr++ = (uint8) regdata;
		}

		/* XXX fix endianness - ltoh */
		/* Only the lower 17-bits are valid */
		sd->func_cis_ptr[fnum] &= 0x0001FFFF;
		sd_info(("%s: Function %d CIS Ptr = 0x%x\n", __FUNCTION__, fnum,
		         sd->func_cis_ptr[fnum]));
	}

	/* Enable function 1 on the dev */
	regdata = SDIO_FUNC_ENABLE_1;
	if ((status = sdbrcm_dev_regwrite(sd, 0, SDIOD_CCCR_IOEN, 1, regdata)) != BCME_OK)
		return status;

	return (BCME_OK);
}

/* Read dev reg */
static int
sdbrcm_dev_regread(sdioh_info_t *sd, int fnum, uint32 regaddr, int regsize, uint32 *data)
{
	int status;
	uint32 cmd_arg = 0;
	uint32 rsp5;
	int count = 0;
	osl_t *osh;
	sdioh_regs_t *regs;
	volatile uint32 pending_ints;

	ASSERT(fnum <= sd->num_funcs);

	osh = sd->osh;
	regs = sd->sdioh_regs;

	if ((fnum == 0) || (regsize == 1)) {
		/* Function 0 only supports CMD52, and byte accesses are only supported by CMD52. */
		 sd_trace(("%s: 52 fnum %d, addr 0x%x, size %d\n",
		             __FUNCTION__, fnum, regaddr, regsize));

		cmd_arg = SFIELD(cmd_arg, CMD52_FUNCTION, fnum);
		cmd_arg = SFIELD(cmd_arg, CMD52_REG_ADDR, (regaddr+(4*count)));
		cmd_arg = SFIELD(cmd_arg, CMD52_RW_FLAG, SDIOH_XFER_TYPE_READ);
		cmd_arg = SFIELD(cmd_arg, CMD52_RAW, 0);
		cmd_arg = SFIELD(cmd_arg, CMD52_DATA, 0);

		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
			SDIOH_CMD_52, cmd_arg)) != BCME_OK) {
			return status;
		}

		sdbrcm_cmd_getrsp(sd, &rsp5, NUM_RESP_WORDS);

		if (GFIELD(rsp5, RSP5_FLAGS) != SD_RSP_R5_IO_CURRENTSTATE0)
			sd_err(("%s: rsp5 flags is 0x%x\t %d\n",
				__FUNCTION__, GFIELD(rsp5, RSP5_FLAGS), fnum));

		*data = GFIELD(rsp5, RSP5_DATA);
	} else {
		if (R_REG(osh, &(regs->fifoctl)) & FIFO_RCV_BUF_RDY) {
			sd_err(("%s: fifo ready before start of regread, clearing\n",
			        __FUNCTION__));
			W_REG(osh, &(regs->fifoctl), FIFO_RCV_BUF_RDY);
		}

		/* For anything other than Function 0 or byte accesses, use CMD53 */
		cmd_arg = SFIELD(cmd_arg, CMD53_BYTE_BLK_CNT, regsize);
		cmd_arg = SFIELD(cmd_arg, CMD53_OP_CODE, 1);	/* SDIO spec v 1.10, Sec 5.3 */
		cmd_arg = SFIELD(cmd_arg, CMD53_BLK_MODE, 0);
		cmd_arg = SFIELD(cmd_arg, CMD53_FUNCTION, fnum);
		cmd_arg = SFIELD(cmd_arg, CMD53_REG_ADDR, regaddr);
		cmd_arg = SFIELD(cmd_arg, CMD53_RW_FLAG, SDIOH_XFER_TYPE_READ);

		sd->data_xfer_count = regsize;

		/* sdbrcm_cmd_issue() retunrs with the command complete bit
		 * in the ISR already cleared
		 */
		status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, SDIOH_CMD_53, cmd_arg);

		/* XXX see similar comment above, propagate error, etc. */
		sdbrcm_cmd_getrsp(sd, &rsp5, NUM_RESP_WORDS);

		if ((status == BCME_OK) &&
		    (GFIELD(rsp5, RSP5_FLAGS) != SD_RSP_R5_IO_CURRENTSTATE0))
			sd_err(("%s: rsp5 flags is 0x%x\t %d\n",
				__FUNCTION__, GFIELD(rsp5, RSP5_FLAGS), fnum));

		/* Mark fifo buffer ready and set all byte enables to VALID. */
		if (R_REG(osh, &(regs->fifoctl)) & FIFO_RCV_BUF_RDY)
			W_REG(osh, &(regs->fifoctl), FIFO_RCV_BUF_RDY | FIFO_VALID_ALL);
		else
			sd_err(("%s: fifo not ready at end of regread, "
			        "status 0x%08x flags 0x%08x\n", __FUNCTION__,
			        status, GFIELD(rsp5, RSP5_FLAGS)));

		/* At this point we have Buffer Ready, so read the data */
		*data = R_REG(osh, &(regs->fifodata));
		if (regsize == 2) {
			*data &= 0xFFFF;
		}

		/* Clear the status bits */
		W_REG(osh, &(regs->intstatus), pending_ints);
	}

	return status;
}

int
isr_sdbrcm_check_dev_intr(struct sdioh_pubinfo *sd_pub)
{
	uint32 old_int, cur_int;
	bool ours;
	sdioh_info_t *sd;

	sd = (sdioh_info_t *)sd_pub;

	cur_int = R_REG(sd->osh, &(sd->sdioh_regs->intstatus));
	cur_int &= sd->intmask;

	if (cur_int & INT_DEV_INT) {
		old_int = R_REG(sd->osh, &(sd->sdioh_regs->intmask));

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
		W_REG(sd->osh, &(sd->sdioh_regs->intstatus), INT_DEV_INT);
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
	/* XXX apply same comments from regread() */
	int status;
	uint32 cmd_arg, rsp5, flags;
	osl_t *osh;
	sdioh_regs_t *regs;

	ASSERT(fnum <= sd->num_funcs);

	osh = sd->osh;
	regs = sd->sdioh_regs;

	cmd_arg = 0;

	if ((fnum == 0) || (regsize == 1)) {
		cmd_arg = SFIELD(cmd_arg, CMD52_FUNCTION, fnum);
		cmd_arg = SFIELD(cmd_arg, CMD52_REG_ADDR, regaddr);
		cmd_arg = SFIELD(cmd_arg, CMD52_RW_FLAG, SDIOH_XFER_TYPE_WRITE);
		cmd_arg = SFIELD(cmd_arg, CMD52_RAW, 0);
		cmd_arg = SFIELD(cmd_arg, CMD52_DATA, data & 0xff);
		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma, SDIOH_CMD_52,
		                               cmd_arg)) != BCME_OK)
			return status;

		sdbrcm_cmd_getrsp(sd, &rsp5, NUM_RESP_WORDS);

		flags = GFIELD(rsp5, RSP5_FLAGS);
		if (flags && (flags != SD_RSP_R5_IO_CURRENTSTATE0))
			sd_err(("%s: rsp5.rsp5.flags = 0x%x, expecting "
			        "SD_RSP_R5_IO_CURRENTSTATE0\n",
			        __FUNCTION__,  flags));
	}
	else {
		cmd_arg = SFIELD(cmd_arg, CMD53_BYTE_BLK_CNT, regsize);
		cmd_arg = SFIELD(cmd_arg, CMD53_OP_CODE, 1);	/* SDIO spec v1.10,
		                                                 * Sec 5.3 Not FIFO
		                                                 */
		cmd_arg = SFIELD(cmd_arg, CMD53_BLK_MODE, 0);
		cmd_arg = SFIELD(cmd_arg, CMD53_FUNCTION, fnum);
		cmd_arg = SFIELD(cmd_arg, CMD53_REG_ADDR, regaddr);
		cmd_arg = SFIELD(cmd_arg, CMD53_RW_FLAG, SDIOH_XFER_TYPE_WRITE);

		sd->data_xfer_count = regsize;

		/* At this point we have Buffer Ready, so write the data */
		if (regsize == 2)
			W_REG(osh, &(regs->fifodata), (uint16) data);
		else
			W_REG(osh, &(regs->fifodata), data);

		/* XXX fix up endianness */

		/* sdbrcm_cmd_issue() returns with the command complete bit
		 * in the ISR already cleared
		 */
		if ((status = sdbrcm_cmd_issue(sd, sd->sd_use_dma,
		                               SDIOH_CMD_53, cmd_arg)) != BCME_OK)
			return status;

		sdbrcm_cmd_getrsp(sd, &rsp5, NUM_RESP_WORDS);

		if (GFIELD(rsp5, RSP5_FLAGS) != SD_RSP_R5_IO_CURRENTSTATE0)
			sd_err(("%s: rsp5 flags = 0x%x, expecting 0x10\n",
			        __FUNCTION__,  GFIELD(rsp5, RSP5_FLAGS)));

	}
	return (BCME_OK);
}

/* count is the number of 32-bit words */
static void
sdbrcm_cmd_getrsp(sdioh_info_t *sd, uint32 *rsp_buffer, int count)
{
	int rsp_count;
	uint32 rsp_temp, rsp_new;
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	ASSERT(count <= 4);

	rsp_temp = R_REG(osh, &(regs->respq));

	if (sd->sd_mode == SDIOH_MODE_SPI) {
		/* In SPI Mode, convert the SPI Response to a SD Response. */
		*rsp_buffer = ((rsp_temp >> 16) & 0x0000FFFF) | 0x1000;
	} else {
		/* Since the BRCM controller's response queue includes the S, D, and 6 Reserved bits
		 * as bits [31:24], we essentially need to shift the 5-word response queue
		 * left by a byte.
		 */
		for (rsp_count = 0; rsp_count < count; rsp_count++) {
			rsp_new = R_REG(osh, &(regs->respq));
			*rsp_buffer = (rsp_temp << 8) | (rsp_new >> 24);
			rsp_temp = rsp_new;
			sd_info(("%s: response queue [%d]: 0x%08x\n",
			         __FUNCTION__, rsp_count, *rsp_buffer));
			rsp_buffer++;
		}
	}
}

static int
sdbrcm_cmd_issue(sdioh_info_t *sd, bool use_dma, uint32 cmd, uint32 arg)
{
	uint32 cmd_reg;
	uint32 cmd_arg;
	uint16 xfer_reg;
	osl_t *osh;
	sdioh_regs_t *regs;
	uint16 blocksize;
	uint16 blockcount;
	int fnum;
	uint32 bytes;
	uint32 pending_ints;
	uint32 errstatus;
	unsigned int countdown;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	ASSERT(R_REG(osh, &(regs->error)) == 0);

	if ((sd->sd_mode == SDIOH_MODE_SPI) &&
		((cmd == SDIOH_CMD_3) || (cmd == SDIOH_CMD_7) || (cmd == SDIOH_CMD_15))) {
			sd_err(("%s: Cmd %d is not for SPI\n", __FUNCTION__, cmd));
			return (BCME_ERROR);
	}

	cmd_reg = 0;
	switch (cmd) {
		case SDIOH_CMD_0:       /* Set Card to Idle State - No Response */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_NONE);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);
			/* XXX check what the assembly looks like */
			break;

		case SDIOH_CMD_3:	/* Ask dev to send RCA - Response R6 */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_R6);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);
			break;

		case SDIOH_CMD_5:	/* Send Operation condition - Response R4 */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_R4);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DMAMODE, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ARC_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_EXP_BSY, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);
			break;

		case SDIOH_CMD_7:	/* Select dev - Response R1 */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_R1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);
			break;

		case SDIOH_CMD_15:	/* Set dev to inactive state - Response None */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_NONE);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 1);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);
			break;

		case SDIOH_CMD_52:	/* IO R/W Direct (single byte) - Response R5 */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_R5);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);
			break;

		case SDIOH_CMD_53:	/* IO R/W Extended (multiple bytes/blocks) */
			cmd_arg = arg;
			xfer_reg = 0;

			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_RESP_TYPE, RESP_TYPE_R5);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 0);
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATA_EN, 1);

			if (GFIELD(cmd_arg, CMD53_RW_FLAG) == SDIOH_XFER_TYPE_READ)
				cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 0);
			else
				cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_DATWR, 1);

			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_ABORT_EN, 0);

			/* XXX - hardcode this "cmd" */
			cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX, cmd);

			use_dma = sd->sd_use_dma && GFIELD(cmd_arg, CMD53_BLK_MODE);

			/* XXX need DMA in bytemode */
			if (GFIELD(cmd_arg, CMD53_BLK_MODE)) {

				sd_info(("%s: SDIOH_CMD_53: Block Mode.\n", __FUNCTION__));

				fnum = GFIELD(cmd_arg, CMD53_FUNCTION);
				blocksize = MIN((int)sd->data_xfer_count,
				                sd->dev_block_size[fnum]);
				blockcount = GFIELD(cmd_arg, CMD53_BYTE_BLK_CNT);

				sd_trace(("%s: Writing Block count %d, block size %d bytes\n",
				                        __FUNCTION__, blockcount, blocksize));

				bytes = GFIELD(cmd_arg, CMD53_BYTE_BLK_CNT);
				cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_TR_COUNT, bytes);

			} else {	/* Non block mode */
				bytes = GFIELD(cmd_arg, CMD53_BYTE_BLK_CNT);
				cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_TR_COUNT, bytes);

				if (use_dma) {
				}
				else {
					sd_info(("%s: SDIOH_CMD_53: Non-block mode, Non-DMA.\n",
					         __FUNCTION__));
				}
			}
			break;

		default:
			sd_err(("%s: Unknown command\n", __FUNCTION__));
			return (BCME_ERROR);
	}

	if (sd->sd_mode == SDIOH_MODE_SPI) {
		cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_CRC_DIS, 1);
		cmd_reg = SFIELD(cmd_reg, SDIOH_CMD_INDEX_DIS, 1);
	}

	/* Setup and issue the SDIO command */
	W_REG(osh, &(regs->cmddat), cmd_reg);

	/* writing the cmdl register starts the SDIO bus transaction. */
	W_REG(osh, &(regs->cmdl), arg);

	/* If we are in polled mode, wait for the command to complete.
	 * In interrupt mode, return immediately. The calling function will
	 * know that the command has completed when the CMDATDONE interrupt
	 * is asserted
	 */
	if (sd->polled_mode) {
		pending_ints = 0;
		countdown = RETRIES_LARGE;

		do {
			/* XXX do some delays for longer reads/writes */
			pending_ints = R_REG(osh, &(regs->intstatus));
		} while  (--countdown && (GFIELD(pending_ints, INT_CMD_DAT_DONE) == 0));

		if (!countdown) {
			sd_err(("%s: CMD_COMPLETE timeout\n",
			    __FUNCTION__));

			sdbrcm_print_respq(sd);

			if (trap_errs)
				ASSERT(0);
			return (BCME_ERROR);
		}

		/* Clear Command Complete interrupt */
		pending_ints = SFIELD(0, INT_CMD_DAT_DONE, 1);
		W_REG(osh, &(regs->intstatus), pending_ints);

		/* Check for Errors */
		if ((errstatus =
		      R_REG(osh, &(regs->error))) != 0) {

			sdbrcm_print_respq(sd);

			W_REG(osh, &(regs->error), errstatus);
			if (trap_errs)
				ASSERT(0);
			return (BCME_ERROR);
		}
	}
	return (BCME_OK);
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
	uint32 rsp5;
	uint flags;
	int num_blocks, blocksize = 0;
	bool local_blockmode, local_dma;
	bool read = rw == SDIOH_READ ? 1 : 0;
	osl_t *osh;
	sdioh_regs_t *regs;
	int bytes, i;

	ASSERT(fnum <= sd->num_funcs);

	osh = sd->osh;
	regs = sd->sdioh_regs;

	ASSERT(nbytes);

	cmd_arg = 0;
	sd_data(("%s: %s 53 addr 0x%x, len %d bytes, r_cnt %d t_cnt %d\n",
		__FUNCTION__, read ? "Rd" : "Wr", addr, nbytes, sd->r_cnt, sd->t_cnt));

	if (read)
		sd->r_cnt++;
	else
		sd->t_cnt++;

	local_blockmode = sd->sd_blockmode;
	local_dma = sd->sd_use_dma;

	num_blocks =  1;
	blocksize = nbytes;
	cmd_arg = SFIELD(cmd_arg, CMD53_BYTE_BLK_CNT, nbytes);
	cmd_arg = SFIELD(cmd_arg, CMD53_BLK_MODE, 0);

	if (local_dma && !read) {
	/* XXX must go away */
		bcopy(data, sd->dma_buf, nbytes);
	}

	/* SDIO spec v 1.10, Sec 5.3  */
	cmd_arg = SFIELD(cmd_arg, CMD53_OP_CODE, (fifo ? 0 : 1));

	cmd_arg = SFIELD(cmd_arg, CMD53_FUNCTION, fnum);
	cmd_arg = SFIELD(cmd_arg, CMD53_REG_ADDR, addr);
	cmd_arg = SFIELD(cmd_arg, CMD53_RW_FLAG,
	                 (read ? SDIOH_XFER_TYPE_READ : SDIOH_XFER_TYPE_WRITE));

	sd->data_xfer_count = nbytes;

	/* XXX PIO Read should start with fifo empty */
	if (!local_dma && read && (R_REG(osh, &(regs->fifoctl)) & FIFO_RCV_BUF_RDY)) {
		sd_err(("%s: fifo ready at start of PIO read, clearing\n", __FUNCTION__));
		W_REG(osh, &(regs->fifoctl), FIFO_RCV_BUF_RDY);
	}

	/* This is a PIO Write. */
	if ((!local_dma) && (!read)) {

		for (i = 0; i < num_blocks; i++) {
			int words;

			/* At this point we have Buffer Ready, so write the
			 * data 4 bytes at a time
			 */
			for (words = blocksize/sizeof(uint32); words; words--) {
				sd_data(("Writing word [%d]=0x%08x\n",
				         blocksize - (words * 4), *(data)));
				W_REG(osh, &(regs->fifodata), *(data++));
			}

			/* Handle < 4 bytes.  wlc_pio.c currently (as of 12/20/05) truncates buflen
			 * to be evenly divisable by 4.  However dongle passes arbitrary lengths,
			 * so handle it here
			 */
			bytes = blocksize % 4;
			if (bytes) { /* XXX clean up try to do with single write see wlc_pio.c */
				switch (bytes) {
				case 1:
					/* W 8 bits */
					W_REG(osh, &(regs->fifodata), *(data++));
					break;
				case 2:
					/* W 16 bits */
					W_REG(osh, &(regs->fifodata), *(data++));
					break;
				case 3:
					/* W 24 bits:
					 * SD_BufferDataPort0[0-15] | SD_BufferDataPort1[16-23]
					 */
					W_REG(osh, &(regs->fifodata), *(data++));
					break;
				default:
					sd_err(("%s: Unexpected bytes leftover %d\n",
						__FUNCTION__, bytes));
					ASSERT(0);
					break;
				}
			}
		}
	}	/* End PIO processing */

	/* sdbrcm_cmd_issue() returns with the command complete bit
	 * in the ISR already cleared
	 */
	status = sdbrcm_cmd_issue(sd, local_dma, SDIOH_CMD_53, cmd_arg);
	sdbrcm_cmd_getrsp(sd, &rsp5, NUM_RESP_WORDS);
	flags = GFIELD(rsp5, RSP5_FLAGS);

	if ((status == BCME_OK) && (flags != SD_RSP_R5_IO_CURRENTSTATE0)) {
		sd_err(("%s: Rsp5: nbytes %d, dma %d blockmode %d, read %d numblocks %d, "
			"blocksize %d\n",
			__FUNCTION__, nbytes, local_dma, local_dma, read, num_blocks, blocksize));

		if (flags & SD_RSP_R5_OUT_OF_RANGE)
			sd_err(("%s: rsp5: Command not accepted: arg out of range 0x%x, "
				"bytes %d dma %d\n",
				__FUNCTION__, flags, GFIELD(cmd_arg, CMD53_BYTE_BLK_CNT),
				GFIELD(cmd_arg, CMD53_BLK_MODE)));
		if (flags & SD_RSP_R5_ERROR)
			sd_err(("%s: Rsp5: General Error\n", __FUNCTION__));

		sd_err(("%s: rsp5 flags = 0x%x, expecting 0x10 returning error\n",
		        __FUNCTION__,  flags));
		if (trap_errs)
			ASSERT(0);
		status = BCME_ERROR;
	}

	/* The following is a PIO Read. */
	if ((!local_dma) && (read)) {
		if ((R_REG(osh, &(regs->fifoctl)) & FIFO_RCV_BUF_RDY))
			W_REG(osh, &(regs->fifoctl), FIFO_RCV_BUF_RDY | FIFO_VALID_ALL);
		else
			sd_err(("%s: fifo not ready at end of PIO read, "
			        "status 0x%08x flags 0x%08x\n", __FUNCTION__, status, flags));

		for (i = 0; i < num_blocks; i++) {
			int words;
			/* At this point we have Buffer Ready, so write the
			 * data 4 bytes at a time
			 */
			for (words = blocksize/4; words; words--) {
				*(data) = R_REG(osh, &(regs->fifodata));
				sd_data(("Block %d: fifo[%d]=0x%08x\n", i, words, *(data)));
				data++;
			}

			/* Handle < 4 bytes.  wlc_pio.c currently (as of 12/20/05) truncates buflen
			 * to be evenly divisable by 4.  However dongle passes arbitrary lengths,
			 * so handle it here
			 */
			bytes = blocksize % 4;
			if (bytes) {
				switch (bytes) { /* XXX same comments from above, + endian */
				case 1:
					/* R/W 8 bits */
					*(data++) = R_REG(osh,
					                        &(regs->fifodata));
					break;
				case 2:
					/* R/W 16 bits */
					*(data++) = R_REG(osh,
					                        &(regs->fifodata));
					break;
				case 3:
					/* R/W 24 bits:
					 * SD_BufferDataPort0[0-15] | SD_BufferDataPort1[16-23]
					 */
					*(data++) = R_REG(osh,
					                        &(regs->fifodata));
					break;
				default:
					sd_err(("%s: Unexpected bytes leftover %d\n",
						__FUNCTION__, bytes));
					ASSERT(0);
					break;
				}
			}
		}
	}	/* End PIO processing */

	/* Fetch data */
	if (local_dma && read) {
		/* XXX must go away */
		bcopy(sd->dma_buf, data, nbytes);
	}
	return status;
}

static int
sdbrcm_set_dev_block_size(sdioh_info_t *sd, int fnum, int block_size)
{
	int base;

	ASSERT(fnum <= sd->num_funcs);

	sd_info(("%s: Setting block size %d, fnum %d\n", __FUNCTION__, block_size, fnum));
	sd->dev_block_size[fnum] = block_size;

	/* Set the block size in the SDIO Card register */
	base = fnum * SDIOD_FBR_SIZE;
	sdbrcm_dev_regwrite(sd, 0, base+SDIOD_CCCR_BLKSIZE_0, 1, block_size & 0xff);
	sdbrcm_dev_regwrite(sd, 0, base+SDIOD_CCCR_BLKSIZE_1, 1, (block_size >> 8) & 0xff);

	/* Do not set the block size in the SDIO Host register, that
	 * is fnum dependent and will get done on an individual
	 * transaction basis
	 */

	return (BCME_OK);
}

static void
sdbrcm_print_respq(sdioh_info_t *sd)
{
	osl_t *osh;
	sdioh_regs_t *regs;
	uint32 r1, r2, r3, r4, r5;
	uint8 resp_flags;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_err(("\n-----ERROR SUMMARY-----\n"));
	sdbrcm_print_sderror(sd);
	sdbrcm_print_sdints(sd);

	r1 = R_REG(osh, &(regs->respq));
	r2 = R_REG(osh, &(regs->respq));
	r3 = R_REG(osh, &(regs->respq));
	r4 = R_REG(osh, &(regs->respq));
	r5 = R_REG(osh, &(regs->respq));

	resp_flags = r1 & 0xFF;

	sdbrcm_print_rsp5_flags(resp_flags);

	sd_err(("Host Controller Response Queue:\n"
	        "    0x%08x\n    0x%08x\n    0x%08x\n    0x%08x\n    0x%08x\n",
	        r1, r2, r4, r5, r5));
	sd_err(("-----------------------\n\n"));
}

#define PR_BIT_SET(reg, bit) \
	if (reg & bit) { sd_err((# bit " ")); }

static void
sdbrcm_print_rsp5_flags(uint8 resp_flags)
{
	sd_err(("R5 Flags 0x%02x [ ", resp_flags));

	PR_BIT_SET(resp_flags, SD_RSP_R5_COM_CRC_ERROR);
	PR_BIT_SET(resp_flags, SD_RSP_R5_ILLEGAL_COMMAND);
	switch (resp_flags & 0x30) {
		case 0x00:
			sd_err(("DISABLED "));
			break;
		case 0x10:
			sd_err(("COMMAND "));
			break;
		case 0x20:
			sd_err(("TRANSFER "));
			break;
		case 0x30:
			sd_err(("RFU "));
			break;
	}

	PR_BIT_SET(resp_flags, SD_RSP_R5_ERROR);
	PR_BIT_SET(resp_flags, SD_RSP_R5_RFU);
	PR_BIT_SET(resp_flags, SD_RSP_R5_FUNC_NUM_ERROR);
	PR_BIT_SET(resp_flags, SD_RSP_R5_OUT_OF_RANGE);

	sd_err(("]\n"));
}

static void
sdbrcm_print_sderror(sdioh_info_t *sd)
{
	osl_t *osh;
	sdioh_regs_t *regs;
	uint32 sd_error;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_error = R_REG(osh, &(regs->error));

	if (sd_error) {
		sd_err(("SdError 0x%08x [ ", sd_error));

		PR_BIT_SET(sd_error, ERROR_RSP_CRC);
		PR_BIT_SET(sd_error, ERROR_RSP_TIME);
		PR_BIT_SET(sd_error, ERROR_RSP_DBIT);
		PR_BIT_SET(sd_error, ERROR_RSP_EBIT);
		PR_BIT_SET(sd_error, ERROR_DAT_CRC);
		PR_BIT_SET(sd_error, ERROR_DAT_SBIT);
		PR_BIT_SET(sd_error, ERROR_DAT_EBIT);
		PR_BIT_SET(sd_error, ERROR_DAT_RSP_S);
		PR_BIT_SET(sd_error, ERROR_DAT_RSP_E);
		PR_BIT_SET(sd_error, ERROR_DAT_RSP_UNKNOWN);
		PR_BIT_SET(sd_error, ERROR_DAT_RSP_TURNARD);
		PR_BIT_SET(sd_error, ERROR_DAT_READ_TO);
		PR_BIT_SET(sd_error, ERROR_SPI_TOKEN_UNK);
		PR_BIT_SET(sd_error, ERROR_SPI_TOKEN_BAD);
		PR_BIT_SET(sd_error, ERROR_SPI_ET_OUTRANGE);
		PR_BIT_SET(sd_error, ERROR_SPI_ET_ECC);
		PR_BIT_SET(sd_error, ERROR_SPI_ET_CC);
		PR_BIT_SET(sd_error, ERROR_SPI_ET_ERR);
		PR_BIT_SET(sd_error, ERROR_AUTO_RSP_CHK);
		PR_BIT_SET(sd_error, ERROR_RSP_BUSY_TO);
		PR_BIT_SET(sd_error, ERROR_RSP_CMDIDX_BAD);

		sd_err(("]\n"));
	} else {
		sd_err(("SdError: No errors\n"));
	}
}

static void
sdbrcm_print_sdints(sdioh_info_t *sd)
{
	osl_t *osh;
	sdioh_regs_t *regs;
	uint32 sd_ints;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_ints = R_REG(osh, &(regs->intstatus));

	if (sd_ints) {
		sd_err(("SdIntstatus 0x%08x [ ", sd_ints));

		PR_BIT_SET(sd_ints, INT_CMD_DAT_DONE);
		PR_BIT_SET(sd_ints, INT_HOST_BUSY);
		PR_BIT_SET(sd_ints, INT_DEV_INT);
		PR_BIT_SET(sd_ints, INT_ERROR_SUM);
		PR_BIT_SET(sd_ints, INT_CARD_INS);
		PR_BIT_SET(sd_ints, INT_CARD_GONE);
		PR_BIT_SET(sd_ints, INT_CMDBUSY_CUTTHRU);
		PR_BIT_SET(sd_ints, INT_CMDBUSY_APPEND);
		PR_BIT_SET(sd_ints, INT_CARD_PRESENT);
		PR_BIT_SET(sd_ints, INT_STD_PCI_DESC);
		PR_BIT_SET(sd_ints, INT_STD_PCI_DATA);
		PR_BIT_SET(sd_ints, INT_STD_DESC_ERR);
		PR_BIT_SET(sd_ints, INT_STD_RCV_DESC_UF);
		PR_BIT_SET(sd_ints, INT_STD_RCV_FIFO_OF);
		PR_BIT_SET(sd_ints, INT_STD_XMT_FIFO_UF);
		PR_BIT_SET(sd_ints, INT_RCV_INT);
		PR_BIT_SET(sd_ints, INT_XMT_INT);

		sd_err(("]\n"));
	} else {
		sd_err(("SdIntstatus: No pending interrupts.\n"));
	}
}

#ifdef BCMDBG
static void
sdbrcm_dumpregs(sdioh_info_t *sd)
{
	osl_t *osh;
	sdioh_regs_t *regs;

	osh = sd->osh;
	regs = sd->sdioh_regs;

	sd_err(("devcontrol 0x%08x\n"
			"      mode 0x%08x\n"
			"     delay 0x%08x\n"
			"      rdto 0x%08x\n"
			"      rbto 0x%08x\n"
			"      test 0x%08x\n"
			"      arvm 0x%08x\n"
			"     error 0x%08x\n"
			" errormask 0x%08x\n"
			"    cmddat 0x%08x\n"
			"      cmdl 0x%08x\n"
			" intstatus 0x%08x\n"
			"   intmask 0x%08x\n"
			" blocksize 0x%08x\n",
			R_REG(osh, &(regs->devcontrol)),
			R_REG(osh, &(regs->mode)),
			R_REG(osh, &(regs->delay)),
			R_REG(osh, &(regs->rdto)),
			R_REG(osh, &(regs->rbto)),
			R_REG(osh, &(regs->test)),
			R_REG(osh, &(regs->arvm)),
			R_REG(osh, &(regs->error)),
			R_REG(osh, &(regs->errormask)),
			R_REG(osh, &(regs->cmddat)),
			R_REG(osh, &(regs->cmdl)),
			R_REG(osh, &(regs->intstatus)),
			R_REG(osh, &(regs->intmask)),
			R_REG(osh, &(regs->blocksize))));

}
#endif /* BCMDBG */
