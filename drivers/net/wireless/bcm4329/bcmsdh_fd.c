/*
 *  BCMSDH Function Driver for SDIO-Linux
 *
 * $ Copyright Dual License Broadcom Corporation $
 *
 * $Id: bcmsdh_fd.c,v 1.17.64.1 2010-08-26 19:11:13 dlhtohdl Exp $
 */
#include <typedefs.h>

#include <bcmdevs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <osl.h>
#include <siutils.h>
#include <sdio.h>	/* SDIO Device and Protocol Specs */
#include <sdioh.h>	/* SDIO Host Controller Specification */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* ioctl/iovars */

#undef INLINE
#define DBG_DECLARE 1
#include <linux/sdio/ctsystem.h>

#include <linux/sdio/sdio_busdriver.h>
#include <linux/sdio/_sdio_defs.h>
#include <linux/sdio/sdio_lib.h>

#include "bcmsdh_fd.h"

#ifndef BCMSDH_MODULE
extern int sdio_function_init(void);
extern void sdio_function_cleanup(void);
#endif /* BCMSDH_MODULE */

void IRQHandler(PVOID pContext);

extern PBCMSDH_FD_INSTANCE gInstance;

uint sd_sdmode = SDIOH_MODE_SD4;	/* Use SD4 mode by default */
uint sd_f2_blocksize = 512;		/* Default blocksize */

uint sd_divisor = 2;			/* Default 48MHz/2 = 24MHz */

uint sd_power = 1;		/* Default to SD Slot powered ON */
uint sd_clock = 1;		/* Default to SD Clock turned ON */
uint sd_hiok = FALSE;	/* Don't use hi-speed mode by default */
uint sd_msglevel = 0x01;
uint sd_use_dma = TRUE;

#define DMA_ALIGN_MASK	0x03

int sdiohfd_card_regread(sdioh_info_t *sd, int func, uint32 regaddr, int regsize, uint32 *data);
static int
sdiohfd_card_regwrite(sdioh_info_t *sd, int func, uint32 regaddr, int regsize, uint32 data);


static int
sdiohfd_card_enablefuncs(sdioh_info_t *sd)
{
	sd_trace(("%s\n", __FUNCTION__));

	/* Get the Card's common CIS address */
	sd->com_cis_ptr = SDDEVICE_GET_SDIO_COMMON_CISPTR(gInstance->pDevice[1]);
	sd->func_cis_ptr[0] = sd->com_cis_ptr;
	sd_info(("%s: Card's Common CIS Ptr = 0x%x\n", __FUNCTION__, sd->com_cis_ptr));

	sd->func_cis_ptr[1] = SDDEVICE_GET_SDIO_FUNC_CISPTR(gInstance->pDevice[1]);
	sd->func_cis_ptr[2] = SDDEVICE_GET_SDIO_FUNC_CISPTR(gInstance->pDevice[2]);

	return FALSE;
}

static int
sdiohfd_set_highspeed_mode(sdioh_info_t *sd, bool HSMode)
{
	uint32 regdata;
	int status;

	if (HSMode == TRUE) {

		sd_info(("Attempting to enable High-Speed mode.\n"));

		if ((status = sdiohfd_card_regread(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
		                                 1, &regdata)) != SUCCESS) {
			return BCME_SDIO_ERROR;
		}
		if (regdata & SDIO_SPEED_SHS) {
			sd_info(("Device supports High-Speed mode.\n"));

			regdata |= SDIO_SPEED_EHS;

			sd_info(("Writing %08x to Card at %08x\n",
			         regdata, SDIOD_CCCR_SPEED_CONTROL));
			if ((status = sdiohfd_card_regwrite(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
			                                  1, regdata)) != BCME_OK) {
				return BCME_SDIO_ERROR;
			}

			if ((status = sdiohfd_card_regread(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
			                                 1, &regdata)) != BCME_OK) {
				return BCME_SDIO_ERROR;
			}

			sd_info(("Read %08x from Card at %08x\n",
			         regdata, SDIOD_CCCR_SPEED_CONTROL));

			sd_err(("High-speed clocking mode enabled.\n"));
		}
		else {
			sd_err(("Device does not support High-Speed Mode.\n"));
		}
	} else {
		/* Force off device bit */
		if ((status = sdiohfd_card_regread(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
		                                 1, &regdata)) != BCME_OK) {
			return status;
		}
		if (regdata & SDIO_SPEED_EHS) {
			regdata &= ~SDIO_SPEED_EHS;
			if ((status = sdiohfd_card_regwrite(sd, 0, SDIOD_CCCR_SPEED_CONTROL,
			                                  1, regdata)) != BCME_OK) {
				return status;
			}
		}

		sd_err(("High-speed clocking mode disabled.\n"));
	}

	return BCME_OK;
}

int sdiohfd_dma_attach(sdioh_info_t *sd)
{
	PSDDMA_DESCRIPTION pDmaDescrip = SDGET_DMA_DESCRIPTION(gInstance->pDevice[0]);

	if (pDmaDescrip == NULL) {
		sd_err(("%s: DMA not supported by this host controller.\n", __FUNCTION__));
		return 0;
	}

	sd_err(("DMA Information:\n"));
	sd_err(("    Flags = 0x%04x [ %s ]\n", pDmaDescrip->Flags,
	        (pDmaDescrip->Flags & SDDMA_DESCRIPTION_FLAG_SGDMA) ?
	        "Scatter/Gather" : "Single"));
	sd_err(("    MaxDescriptors = %d\n", pDmaDescrip->MaxDescriptors));
	sd_err(("    MaxBytesPerDescriptor = %d\n", pDmaDescrip->MaxBytesPerDescriptor));
	sd_err(("    AddressAlignment = 0x%08x\n", pDmaDescrip->AddressAlignment));
	sd_err(("    LengthAlignment = 0x%08x\n", pDmaDescrip->LengthAlignment));

	sd->max_dma_len = pDmaDescrip->MaxBytesPerDescriptor;
	sd->max_dma_descriptors = pDmaDescrip->MaxDescriptors;

	return 0;
}

int sdiohfd_dma_detach(sdioh_info_t *sd)
{
	return 0;
}


/*
 *	Public entry points & extern's
 */
extern sdioh_info_t *
sdioh_attach(osl_t *osh, void *bar0, uint irq)
{
	int Status = 0;
	sdioh_info_t *sd;
	SDIO_STATUS sd_status;

	sd_err(("%s\n", __FUNCTION__));

#ifndef BCMSDH_MODULE
	sd_status = sdio_function_init();
	if (sd_status != 0) {
		sd_err(("%s: not able to register SDIO functions, error %d\n",
			__FUNCTION__, sd_status));
		return NULL;
	}
#endif /* BCMSDH_MODULE */

	if (gInstance == NULL) {
		sd_err(("%s: SDIO Device not present\n", __FUNCTION__));
		return NULL;
	}

	if ((sd = (sdioh_info_t *)MALLOC(osh, sizeof(sdioh_info_t))) == NULL) {
		sd_err(("sdioh_attach: out of memory, malloced %d bytes\n", MALLOCED(osh)));
		return NULL;
	}
	bzero((char *)sd, sizeof(sdioh_info_t));
	sd->osh = osh;
	if (sdiohfd_osinit(sd) != 0) {
		sd_err(("%s:sdiohfd_osinit() failed\n", __FUNCTION__));
		MFREE(sd->osh, sd, sizeof(sdioh_info_t));
		return NULL;
	}

	sd->num_funcs = 2;
	sd->sd_blockmode = TRUE;
	sd->use_client_ints = TRUE;
	sd->client_block_size[0] = 64;
	sd_status =  SDLIB_SetFunctionBlockSize(gInstance->pDevice[0], 64);
	sd->client_block_size[1] = 64;
	sd_status =  SDLIB_SetFunctionBlockSize(gInstance->pDevice[1], 64);
	sd->client_block_size[2] = sd_f2_blocksize;
	sd_status =  SDLIB_SetFunctionBlockSize(gInstance->pDevice[2], sd_f2_blocksize);

	sdiohfd_set_highspeed_mode(sd, sd_hiok);

	sdiohfd_card_enablefuncs(sd);

	sdiohfd_dma_attach(sd);

	/* register irq */
	SDDEVICE_SET_IRQ_HANDLER(gInstance->pDevice[1], IRQHandler, sd);

	Status = SDLIB_IssueConfig(gInstance->pDevice[1],	SDCONFIG_FUNC_UNMASK_IRQ, NULL, 0);
	if (!SDIO_SUCCESS((Status))) {
		sd_err(("%s: failed to unmask IRQ %d\n", __FUNCTION__, Status));
	}

	sd_err(("%s: Done\n", __FUNCTION__));
	return sd;
}


extern SDIOH_API_RC
sdioh_detach(osl_t *osh, sdioh_info_t *sd)
{
	sd_trace(("%s\n", __FUNCTION__));

	sdiohfd_dma_detach(sd);

#ifndef BCMSDH_MODULE
	sdio_function_cleanup();
#endif
	if (sd) {

		/* deregister irq */
		sdiohfd_osfree(sd);

		MFREE(sd->osh, sd, sizeof(sdioh_info_t));
	}
	return SDIOH_API_RC_SUCCESS;
}

/* Configure callback to client when we recieve client interrupt */
extern SDIOH_API_RC
sdioh_interrupt_register(sdioh_info_t *sd, sdioh_cb_fn_t fn, void *argh)
{
	sd_trace(("%s: Entering\n", __FUNCTION__));
	sd->intr_handler = fn;
	sd->intr_handler_arg = argh;
	sd->intr_handler_valid = TRUE;
	return SDIOH_API_RC_SUCCESS;
}

extern SDIOH_API_RC
sdioh_interrupt_deregister(sdioh_info_t *sd)
{
	sd_trace(("%s: Entering\n", __FUNCTION__));
	sd->intr_handler_valid = FALSE;
	sd->intr_handler = NULL;
	sd->intr_handler_arg = NULL;
	return SDIOH_API_RC_SUCCESS;
}

extern SDIOH_API_RC
sdioh_interrupt_query(sdioh_info_t *sd, bool *onoff)
{
	sd_trace(("%s: Entering\n", __FUNCTION__));
	*onoff = sd->client_intr_enabled;
	return SDIOH_API_RC_SUCCESS;
}

#if defined(DHD_DEBUG) || defined(BCMDBG)
extern bool
sdioh_interrupt_pending(sdioh_info_t *sd)
{
	return (0);
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
	IOV_CLOCK,
	IOV_RXCHAIN
};

const bcm_iovar_t sdioh_iovars[] = {
	{"sd_msglevel", IOV_MSGLEVEL,	0,	IOVT_UINT32,	0 },
	{"sd_blockmode", IOV_BLOCKMODE, 0,	IOVT_BOOL,	0 },
	{"sd_blocksize", IOV_BLOCKSIZE, 0,	IOVT_UINT32,	0 }, /* ((fn << 16) | size) */
	{"sd_dma",	IOV_DMA,	0,	IOVT_BOOL,	0 },
	{"sd_ints", 	IOV_USEINTS,	0,	IOVT_BOOL,	0 },
	{"sd_numints",	IOV_NUMINTS,	0,	IOVT_UINT32,	0 },
	{"sd_numlocalints", IOV_NUMLOCALINTS, 0, IOVT_UINT32,	0 },
	{"sd_hostreg",	IOV_HOSTREG,	0,	IOVT_BUFFER,	sizeof(sdreg_t) },
	{"sd_devreg",	IOV_DEVREG, 	0,	IOVT_BUFFER,	sizeof(sdreg_t) },
	{"sd_divisor",	IOV_DIVISOR,	0,	IOVT_UINT32,	0 },
	{"sd_power",	IOV_POWER,	0,	IOVT_UINT32,	0 },
	{"sd_clock",	IOV_CLOCK,	0,	IOVT_UINT32,	0 },
	{"sd_mode", 	IOV_SDMODE, 	0,	IOVT_UINT32,	100},
	{"sd_highspeed", IOV_HISPEED,	0,	IOVT_UINT32,	0 },
	{"sd_rxchain",  IOV_RXCHAIN,    0, 	IOVT_BOOL,	0 },
#ifdef BCMDBG
	{"sd_hciregs",	IOV_HCIREGS,	0,	IOVT_BUFFER,	0 },
#endif
	{NULL, 0, 0, 0, 0 }
};

int
sdioh_iovar_op(sdioh_info_t *si, const char *name,
                           void *params, int plen, void *arg, int len, bool set)
{
	const bcm_iovar_t *vi = NULL;
	int bcmerror = 0;
	int val_size;
	int32 int_val = 0;
	bool bool_val;
	uint32 actionid;

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

	/* XXX Copied from dhd, copied from wl; certainly overkill here? */
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
		int_val = (int32)si->sd_blockmode;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_BLOCKMODE):
		si->sd_blockmode = (bool)int_val;
		/* Haven't figured out how to make non-block mode with DMA */
		break;

	case IOV_GVAL(IOV_BLOCKSIZE):
		if ((uint32)int_val > si->num_funcs) {
			bcmerror = BCME_BADARG;
			break;
		}
		int_val = (int32)si->client_block_size[int_val];
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_BLOCKSIZE):
	{
		uint func = ((uint32)int_val >> 16);
		uint blksize = (uint16)int_val;
		uint maxsize;
		SDIO_STATUS status;

		if (func > si->num_funcs) {
			bcmerror = BCME_BADARG;
			break;
		}

		/* XXX These hardcoded sizes are a hack, remove after proper CIS parsing. */
		switch (func) {
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
		si->client_block_size[func] = blksize;
		status =  SDLIB_SetFunctionBlockSize(gInstance->pDevice[func], blksize);

		break;
	}

	case IOV_GVAL(IOV_RXCHAIN):
		int_val = TRUE;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_GVAL(IOV_DMA):
		int_val = (int32)si->sd_use_dma;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_DMA):
		si->sd_use_dma = (bool)int_val;
		break;

	case IOV_GVAL(IOV_USEINTS):
		int_val = (int32)si->use_client_ints;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_USEINTS):
		si->use_client_ints = (bool)int_val;
		if (si->use_client_ints)
			si->intmask |= CLIENT_INTR;
		else
			si->intmask &= ~CLIENT_INTR;

		break;

	case IOV_GVAL(IOV_DIVISOR):
		int_val = (uint32)sd_divisor;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_DIVISOR):
		sd_divisor = int_val;
		break;

	case IOV_GVAL(IOV_POWER):
		int_val = (uint32)sd_power;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_POWER):
		sd_power = int_val;
		break;

	case IOV_GVAL(IOV_CLOCK):
		int_val = (uint32)sd_clock;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_CLOCK):
		sd_clock = int_val;
		break;

	case IOV_GVAL(IOV_SDMODE):
		int_val = (uint32)sd_sdmode;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_SDMODE):
		sd_sdmode = int_val;
		break;

	case IOV_GVAL(IOV_HISPEED):
		int_val = (uint32)sd_hiok;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_SVAL(IOV_HISPEED):
		sd_hiok = int_val;
		bcmerror = sdiohfd_set_highspeed_mode(si, (bool)sd_hiok);
		break;

	case IOV_GVAL(IOV_NUMINTS):
		int_val = (int32)si->intrcount;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_GVAL(IOV_NUMLOCALINTS):
		int_val = (int32)0;
		bcopy(&int_val, arg, val_size);
		break;

	case IOV_GVAL(IOV_HOSTREG):
	{
		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_ptr = (sdreg_t *)params;

		if (sd_ptr->offset < SD_SysAddr || sd_ptr->offset > SD_MaxCurCap) {
			sd_err(("%s: bad offset 0x%x\n", __FUNCTION__, sd_ptr->offset));
			bcmerror = BCME_BADARG;
			break;
		}

		sd_trace(("%s: rreg%d at offset %d\n", __FUNCTION__,
		                  (sd_ptr->offset & 1) ? 8 : ((sd_ptr->offset & 2) ? 16 : 32),
		                  sd_ptr->offset));
		if (sd_ptr->offset & 1)
			int_val = 8; /* sdiohfd_rreg8(si, sd_ptr->offset); */
		else if (sd_ptr->offset & 2)
			int_val = 16; /* sdiohfd_rreg16(si, sd_ptr->offset); */
		else
			int_val = 32; /* sdiohfd_rreg(si, sd_ptr->offset); */

		bcopy(&int_val, arg, sizeof(int_val));
		break;
	}

	case IOV_SVAL(IOV_HOSTREG):
	{
		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_ptr = (sdreg_t *)params;

		if (sd_ptr->offset < SD_SysAddr || sd_ptr->offset > SD_MaxCurCap) {
			sd_err(("%s: bad offset 0x%x\n", __FUNCTION__, sd_ptr->offset));
			bcmerror = BCME_BADARG;
			break;
		}

		sd_trace(("%s: wreg%d value 0x%08x at offset %d\n", __FUNCTION__, sd_ptr->value,
		                  (sd_ptr->offset & 1) ? 8 : ((sd_ptr->offset & 2) ? 16 : 32),
		                  sd_ptr->offset));
		break;
	}

	case IOV_GVAL(IOV_DEVREG):
	{
		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_ptr = (sdreg_t *)params;
		uint8 data;

		if (sdioh_cfg_read(si, sd_ptr->func, sd_ptr->offset, &data)) {
			bcmerror = BCME_SDIO_ERROR;
			break;
		}

		int_val = (int)data;
		bcopy(&int_val, arg, sizeof(int_val));
		break;
	}

	case IOV_SVAL(IOV_DEVREG):
	{
		/* XXX Should copy for alignment reasons */
		sdreg_t *sd_ptr = (sdreg_t *)params;
		uint8 data = (uint8)sd_ptr->value;

		if (sdioh_cfg_write(si, sd_ptr->func, sd_ptr->offset, &data)) {
			bcmerror = BCME_SDIO_ERROR;
			break;
		}
		break;
	}

	default:
		bcmerror = BCME_UNSUPPORTED;
		break;
	}
exit:

	/* XXX Remove protective lock after clients all clean... */
	return bcmerror;
}

extern SDIOH_API_RC
sdioh_cfg_read(sdioh_info_t *sd, uint fnc_num, uint32 addr, uint8 *data)
{
	SDIOH_API_RC status;
	/* No lock needed since sdioh_request_byte does locking */
	status = sdioh_request_byte(sd, SDIOH_READ, fnc_num, addr, data);
	return status;
}

extern SDIOH_API_RC
sdioh_cfg_write(sdioh_info_t *sd, uint fnc_num, uint32 addr, uint8 *data)
{
	/* No lock needed since sdioh_request_byte does locking */
	SDIOH_API_RC status;
	status = sdioh_request_byte(sd, SDIOH_WRITE, fnc_num, addr, data);
	return status;
}

extern SDIOH_API_RC
sdioh_cis_read(sdioh_info_t *sd, uint func, uint8 *cisd, uint32 length)
{
	uint32 count;
	int offset;
	uint32 foo;
	uint8 *cis = cisd;

	sd_trace(("%s: Func = %d\n", __FUNCTION__, func));

	if (!sd->func_cis_ptr[func]) {
		bzero(cis, length);
		sd_err(("%s: no func_cis_ptr[%d]\n", __FUNCTION__, func));
		return SDIOH_API_RC_FAIL;
	}

	sd_err(("%s: func_cis_ptr[%d]=0x%04x\n", __FUNCTION__, func, sd->func_cis_ptr[func]));

	for (count = 0; count < length; count++) {
		offset =  sd->func_cis_ptr[func] + count;
		if (sdiohfd_card_regread (sd, 0, offset, 1, &foo) < 0) {
			sd_err(("%s: regread failed: Can't read CIS\n", __FUNCTION__));
			return SDIOH_API_RC_FAIL;
		}

		*cis = (uint8)(foo & 0xff);
		cis++;
	}

	return SDIOH_API_RC_SUCCESS;
}

extern SDIOH_API_RC
sdioh_request_byte(sdioh_info_t *sd, uint rw, uint func, uint regaddr, uint8 *byte)
{
	SDIO_STATUS status = SDIO_STATUS_SUCCESS;

	sd_info(("%s: rw=%d, func=%d, addr=0x%05x\n", __FUNCTION__, rw, func, regaddr));

	status = SDLIB_IssueCMD52(gInstance->pDevice[func], func,
	                          regaddr, byte, 1, rw);
	if (!SDIO_SUCCESS(status)) {
		sd_err(("bcmsdh_fd: Failed to %s byte, Err:%d",
		                        rw ? "Write" : "Read", status));
	}

	return SDIOH_API_RC_SUCCESS;
}

extern SDIOH_API_RC
sdioh_request_word(sdioh_info_t *sd, uint cmd_type, uint rw, uint func, uint addr,
                                   uint32 *word, uint nbytes)
{
	PSDREQUEST	pReq = NULL;
	SDIO_STATUS status;

	sd_info(("%s: cmd_type=%d, rw=%d, func=%d, addr=0x%05x, nbytes=%d\n",
	         __FUNCTION__, cmd_type, rw, func, addr, nbytes));

	/* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(gInstance->pDevice[func]);
	if (pReq == NULL) {
		return SDIO_STATUS_NO_RESOURCES;
	}

	/* initialize the command argument bits, see CMD53 SDIO spec. */
	SDIO_SET_CMD53_ARG(pReq->Argument,
					   (rw ? CMD53_WRITE : CMD53_READ),	  /* write */
					   func, /* function number */
					   CMD53_BYTE_BASIS,	   /* set to byte mode */
					   CMD53_FIXED_ADDRESS,	   /*  fixed address */
					   addr, /* 17-bit register address */
					   CMD53_CONVERT_BYTE_BASIS_BLK_LENGTH_PARAM(nbytes));

	pReq->pDataBuffer = word;
	pReq->Command = CMD53;
	pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS |
		SDREQ_FLAGS_DATA_SHORT_TRANSFER |
		(rw ? SDREQ_FLAGS_DATA_WRITE : 0);


	pReq->BlockCount = 1;	 /* byte mode is always 1 block */
	pReq->BlockLen = nbytes;

	/* send the CMD53 out synchronously */
	status = SDDEVICE_CALL_REQUEST_FUNC(gInstance->pDevice[func], pReq);

	if (!SDIO_SUCCESS(status)) {
		sd_info(("bcmsdh_fd Function: Synch CMD53 %s failed %d \n",
		                       (rw ? "Write" : "Read"), status));
	}

	sd_info(("%s: word=%08x\n", __FUNCTION__, *word));


	/* free the request */
	SDDeviceFreeRequest(gInstance->pDevice[func], pReq);

	return (SDIOH_API_RC_SUCCESS);
}

static SDIOH_API_RC
sdioh_request_packet(sdioh_info_t *sd, uint fix_inc, uint write, uint func,
                     uint addr, void *pkt)
{
	PSDREQUEST	pReq = NULL;
	SDIO_STATUS status;
	int len;
	int buflen = 0;
	bool fifo = (fix_inc == SDIOH_DATA_FIX);
	bool use_blockmode = sd->sd_blockmode;
	int num_blocks = 0;
	int leftover_bytes = 0;
	uint xferred_bytes = 0;
	dma_addr_t hdma[16];
	uint pkt_count = 0;
	uint i;
	void *pnext;

	sd_trace(("%s: Enter\n", __FUNCTION__));

	ASSERT(pkt);

	i = 0;

	/* Traverse the packet chain to figure out how many packets
	 * there are, figure out the total data transfer length,
	 * and make sure the packets are aligned properly for DMA.
	 */
	for (pnext = pkt; pnext; pnext = PKTNEXT(sd->osh, pnext)) {
		pkt_count++;
		sd_data(("%s: %p[%d]: %p len 0x%04x (%d)\n", __FUNCTION__,
		         pnext, pkt_count, (uint8*)PKTDATA(sd->osh, pnext),
		         PKTLEN(sd->osh, pnext), PKTLEN(sd->osh, pnext)));
		buflen += PKTLEN(sd->osh, pnext);

		/* Make sure the packet is aligned properly. If it isn't, then this
		 * is the fault of sdioh_request_buffer() which is supposed to give
		 * us something we can work with.
		 */
		ASSERT(((uint32)(PKTDATA(sd->osh, pkt)) & DMA_ALIGN_MASK) == 0);
	}

	/* allocate request to send to host controller */
	pReq = SDDeviceAllocRequest(gInstance->pDevice[func]);
	if (pReq == NULL) {
		return SDIO_STATUS_NO_RESOURCES;
	}

	/* For less than one, or exactly one block, use bytemode. */
	if (buflen <= sd->client_block_size[func]) {
		use_blockmode = FALSE;
	}

	if (use_blockmode == TRUE) {
		len = MIN(sd->client_block_size[func], buflen);
		if (len < sd->client_block_size[func]) {
			num_blocks = 0;
			leftover_bytes = len;
		} else {
			num_blocks = buflen / len;
			leftover_bytes = buflen - (num_blocks * len);
		}
	} else {
		num_blocks = 0;
		leftover_bytes = buflen;
	}

	sd_info(("%s: %s: buflen=%d, num_blocks=%d, leftover_bytes=%d, use_blockmode=%d\n",
		__FUNCTION__,
		write ? "Tx" : "Rx",
		buflen, num_blocks, leftover_bytes, use_blockmode));

	/* Break buffer down into blocksize chunks:
	 * Bytemode: 1 block at a time.
	 * Blockmode: Multiples of blocksizes at a time.
	 * Both: leftovers are handled last (will be sent via bytemode.
	 */
	while (buflen > 0) {
		uint32	xfr_len;
		uint32	SGCount = 0;

		/* Byte mode: One block at a time */
		len = MIN(sd->client_block_size[func], buflen);
		xfr_len = ((num_blocks == 0) ? len : (num_blocks * len));

		/* initialize the command argument bits, see CMD53 SDIO spec. */
		SDIO_SET_CMD53_ARG(pReq->Argument,
		   (write ? CMD53_WRITE : CMD53_READ),	  /* write */
		   func, /* function number */
		   (num_blocks == 0) ? CMD53_BYTE_BASIS : CMD53_BLOCK_BASIS,
		   (fifo) ? CMD53_FIXED_ADDRESS : CMD53_INCR_ADDRESS,
		   addr, /* 17-bit register address */
		   (num_blocks == 0) ? CMD53_CONVERT_BYTE_BASIS_BLK_LENGTH_PARAM(len) :
		   num_blocks);

		/* Set up the DMA descriptors.  The packets must be properly aligned by
		 * this point.  This is enforced above.
		 */
		for (pnext = pkt; pnext; pnext = PKTNEXT(sd->osh, pnext)) {
			uint rounded_len = PKTLEN(sd->osh, pnext);

			rounded_len += (rounded_len % 4);

			/* Here we break up the packet and map to separate SGList
			 * entries, if the packet is larger than DMA_MAX_LENGTH.
			 */
			for (i = 0; i < ((rounded_len / DMA_MAX_LEN) + 1); i++) {
				uint chunk_len;

				chunk_len = rounded_len - (i*DMA_MAX_LEN);
				chunk_len = MIN(chunk_len, DMA_MAX_LEN);

				hdma[SGCount] = dma_map_single(
					SD_GET_OS_DEVICE(gInstance->pDevice[0]),
					((uint8*)PKTDATA(sd->osh, pnext)) +
					i*DMA_MAX_LEN + xferred_bytes,
					chunk_len,
					write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

				sd->SGList[SGCount].page = virt_to_page(
					((uint8*)PKTDATA(sd->osh, pnext)) +
					i*DMA_MAX_LEN + xferred_bytes);
				sd->SGList[SGCount].offset = hdma[SGCount] -
					page_to_phys(sd->SGList[SGCount].page);
				sd->SGList[SGCount].length = chunk_len;

				sd_info(("%s: %s Mapped %p[%d], len=%d\n", __FUNCTION__,
					(write) ? "TX" : "RX",
					pnext, SGCount, chunk_len));

				SGCount++;

				if (SGCount > sd->max_dma_descriptors) {
					sd_err(("%s: Exceeded maximum available DMA descriptors.\n",
					        __FUNCTION__));
					break;
				}
			}
		}

		pReq->pDataBuffer  = (PVOID)&(sd->SGList);
		pReq->DescriptorCount = SGCount;
		pReq->Command = CMD53;
		pReq->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS |
			(write ? SDREQ_FLAGS_DATA_WRITE : 0);

		pReq->Flags |= SDREQ_FLAGS_DATA_DMA;

		pReq->BlockCount = (num_blocks == 0) ? 1 : num_blocks;
		pReq->BlockLen = len;

		/* send the CMD53 out synchronously */
		status = SDDEVICE_CALL_REQUEST_FUNC(gInstance->pDevice[func], pReq);
		if (!SDIO_SUCCESS(status)) {
			sd_info(("bcmsdh_fd Function: Synch CMD53 %s failed %d \n",
			         (write ? "Write" : "Read"), status));
			/* free the request */
			SDDeviceFreeRequest(gInstance->pDevice[func], pReq);

			return (SDIOH_API_RC_FAIL);
		}

		/* The transfer is complete, so unmap it the chain.
		 */
		for (i = 0; i < SGCount; i++) {
			dma_unmap_single(SD_GET_OS_DEVICE(gInstance->pDevice[0]),
				hdma[i],
				sd->SGList[i].length,
				write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		}

		/* Account for how much data was transferred, which determines
		 * whether or not we have more to transfer (leftover_bytes).
		 */
		if (num_blocks == 0) {
			buflen -= len;
			xferred_bytes += len;
			if (!fifo) {
				addr += len;
			}
		} else {
			buflen -= num_blocks * len;
			xferred_bytes += num_blocks * len;
			if (!fifo) {
				addr += num_blocks * len;
			}
			num_blocks = 0;
		}
	}

	/* free the request */
	SDDeviceFreeRequest(gInstance->pDevice[func], pReq);

	sd_trace(("%s: Exit\n", __FUNCTION__));
	return SDIOH_API_RC_SUCCESS;
}


/*
 * This function takes a buffer or packet, and fixes everything up so that in the
 * end, a DMA-able packet is created.
 *
 * A buffer does not have an associated packet pointer, and may or may not be aligned.
 * A packet may consist of a single packet, or a packet chain.  If it is a packet chain,
 * then all the packets in the chain must be properly aligned.  If the packet data is not
 * aligned, then there may only be one packet, and in this case, it is copied to a new
 * aligned packet.
 *
 */
extern SDIOH_API_RC
sdioh_request_buffer(sdioh_info_t *sd, uint pio_dma, uint fix_inc, uint write, uint func,
                     uint32 addr, uint reg_width, uint32 buflen_u, uint8 *buffer, void *pkt)
{
	SDIOH_API_RC Status;
	void *mypkt = NULL;

	sd_trace(("%s: Enter\n", __FUNCTION__));

	/* Case 1: we don't have a packet. */
	if (pkt == NULL) {
		sd_data(("%s: Creating new %s Packet, len=%d\n",
		         __FUNCTION__, write ? "TX" : "RX", buflen_u));
		if (!(mypkt = PKTGET(sd->osh, buflen_u, write ? TRUE : FALSE))) {
			sd_err(("%s: PKTGET failed: len %d\n",
			           __FUNCTION__, buflen_u));
		}

		/* For a write, copy the buffer data into the packet. */
		if (write) {
			bcopy(buffer, PKTDATA(sd->osh, mypkt), buflen_u);
		}

		Status = sdioh_request_packet(sd, fix_inc, write, func, addr, mypkt);

		/* For a read, copy the packet data back to the buffer. */
		if (!write) {
			bcopy(PKTDATA(sd->osh, mypkt), buffer, buflen_u);
		}

		PKTFREE(sd->osh, mypkt, write ? TRUE : FALSE);
	} else if (((uint32)(PKTDATA(sd->osh, pkt)) & DMA_ALIGN_MASK) != 0) {
		/* Case 2: We have a packet, but it is unaligned. */

		/* In this case, we cannot have a chain. */
		ASSERT(PKTNEXT(sd->osh, pkt) == NULL);

		sd_data(("%s: Creating aligned %s Packet, len=%d\n",
		         __FUNCTION__, write ? "TX" : "RX", PKTLEN(sd->osh, pkt)));
		if (!(mypkt = PKTGET(sd->osh, PKTLEN(sd->osh, pkt), write ? TRUE : FALSE))) {
			sd_err(("%s: PKTGET failed: len %d\n",
			           __FUNCTION__, PKTLEN(sd->osh, pkt)));
		}

		/* For a write, copy the buffer data into the packet. */
		if (write) {
			bcopy(PKTDATA(sd->osh, pkt),
			      PKTDATA(sd->osh, mypkt),
			      PKTLEN(sd->osh, pkt));
		}

		Status = sdioh_request_packet(sd, fix_inc, write, func, addr, mypkt);

		/* For a read, copy the packet data back to the buffer. */
		if (!write) {
			bcopy(PKTDATA(sd->osh, mypkt),
			      PKTDATA(sd->osh, pkt),
			      PKTLEN(sd->osh, mypkt));
		}

		PKTFREE(sd->osh, mypkt, write ? TRUE : FALSE);
	} else { /* case 3: We have a packet and it is aligned. */
		sd_data(("%s: Aligned %s Packet, direct DMA\n",
		         __FUNCTION__, write ? "Tx" : "Rx"));
		Status = sdioh_request_packet(sd, fix_inc, write, func, addr, pkt);
	}

	return (Status);
}

extern int
sdioh_abort(sdioh_info_t *sd, uint fnum)
{
	sd_trace(("%s: Enter\n", __FUNCTION__));
	SDLIB_IssueIOAbort(gInstance->pDevice[fnum]);
	sd_trace(("%s: Exit\n", __FUNCTION__));
	return SDIOH_API_RC_SUCCESS;
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

/* Reset and re-initialize the device */
int sdioh_sdio_reset(sdioh_info_t *si)
{
	sd_trace(("%s: Enter\n", __FUNCTION__));
	sd_trace(("%s: Exit\n", __FUNCTION__));
	return SDIOH_API_RC_SUCCESS;
}

/* Disable device interrupt */
void
sdiohfd_devintr_off(sdioh_info_t *sd)
{
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_client_ints));
	sd->intmask &= ~CLIENT_INTR;
}

/* Enable device interrupt */
void
sdiohfd_devintr_on(sdioh_info_t *sd)
{
	SDIO_STATUS status;

	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_client_ints));
	sd->intmask |= CLIENT_INTR;
	status = SDLIB_IssueConfig(gInstance->pDevice[1], SDCONFIG_FUNC_ACK_IRQ, NULL, 0);
}

/* Read client card reg */
int
sdiohfd_card_regread(sdioh_info_t *sd, int func, uint32 regaddr, int regsize, uint32 *data)
{

	if ((func == 0) || (regsize == 1)) {
		uint8 temp;

		sdioh_request_byte(sd, SDIOH_READ, func, regaddr, &temp);
		*data = temp;
		*data &= 0xff;
		sd_data(("%s: byte read data=0x%02x\n",
		         __FUNCTION__, *data));
	} else {
		sdioh_request_word(sd, 0, SDIOH_READ, func, regaddr, data, regsize);
		if (regsize == 2)
			*data &= 0xffff;

		sd_data(("%s: word read data=0x%08x\n",
		         __FUNCTION__, *data));
	}

	return SUCCESS;
}


/* Write client card reg */
static int
sdiohfd_card_regwrite(sdioh_info_t *sd, int func, uint32 regaddr, int regsize, uint32 data)
{

	if ((func == 0) || (regsize == 1)) {
		uint8 temp;

		temp = data & 0xff;
		sdioh_request_byte(sd, SDIOH_READ, func, regaddr, &temp);
		sd_data(("%s: byte write data=0x%02x\n",
		         __FUNCTION__, data));
	} else {
		if (regsize == 2)
			data &= 0xffff;

		sdioh_request_word(sd, 0, SDIOH_READ, func, regaddr, &data, regsize);

		sd_data(("%s: word write data=0x%08x\n",
		         __FUNCTION__, data));
	}

	return SUCCESS;
}


#ifdef BCMINTERNAL
extern SDIOH_API_RC
sdioh_test_diag(sdioh_info_t *sd)
{
	sd_trace(("%s: Enter\n", __FUNCTION__));
	sd_trace(("%s: Exit\n", __FUNCTION__));
	return (0);
}
#endif /* BCMINTERNAL */


/* create a bcmsdh_fd instance */
SDIO_STATUS client_init(PBCMSDH_FD_CONTEXT pFuncContext,
                               PBCMSDH_FD_INSTANCE pInstance,
                               PSDDEVICE pDevice)
{
	SDCONFIG_FUNC_ENABLE_DISABLE_DATA fData;
	SDCONFIG_FUNC_SLOT_CURRENT_DATA	  slotCurrent;
	SDIO_STATUS status = SDIO_STATUS_SUCCESS;
	struct SDIO_FUNC_EXT_FUNCTION_TPL_1_1 funcTuple;
	UINT32			nextTpl;
	UINT8			tplLength;
	uint	func = 0;
	ZERO_OBJECT(fData);
	ZERO_OBJECT(slotCurrent);

	do {

		func = SDDEVICE_GET_SDIO_FUNCNO(pDevice);

		pInstance->pDevice[func] = pDevice;
		nextTpl = SDDEVICE_GET_SDIO_FUNC_CISPTR(pDevice);
		tplLength = sizeof(funcTuple);

		/* go get the function Extension tuple */
		status = SDLIB_FindTuple(pDevice,
		                         CISTPL_FUNCE,
		                         &nextTpl,
		                         (PUINT8)&funcTuple,
		                         &tplLength);

		if (!SDIO_SUCCESS(status)) {
			sd_err(("bcmsdh_fd: Failed to get FuncE Tuple: %d \n",
				status));
			/* pick some reason number */
			slotCurrent.SlotCurrent = 200;
		}

		if (slotCurrent.SlotCurrent == 0) {
			/* use the operational power (8-bit) value of current in mA as default */
			slotCurrent.SlotCurrent = funcTuple.CommonInfo.OpMaxPwr;
			if (tplLength > sizeof(funcTuple.CommonInfo)) {
				/* we have a 1.1 tuple */
				sd_trace(("bcmsdh_fd: 1.1 Tuple Found \n"));
				 /* check for HIPWR mode */
				if (SDDEVICE_GET_CARD_FLAGS(pDevice) & CARD_HIPWR) {
					/* use the maximum operational power (16 bit )
					 * from the tuple
					 */
					slotCurrent.SlotCurrent =
						CT_LE16_TO_CPU_ENDIAN(funcTuple.HiPwrMaxPwr);
				}
			}
		}

		if (slotCurrent.SlotCurrent == 0) {
			sd_info(("bcmsdh_fd: FUNCE tuple indicates greater than"
			         " 200ma OpMaxPwr current! \n"));
			/* try something higher than 200ma */
			slotCurrent.SlotCurrent = 300;
		}
		sd_trace(("bcmsdh_fd: Allocating Slot current: %d mA\n",
			slotCurrent.SlotCurrent));
		status = SDLIB_IssueConfig(pDevice,
		                           SDCONFIG_FUNC_ALLOC_SLOT_CURRENT,
		                           &slotCurrent,
		                           sizeof(slotCurrent));
		if (!SDIO_SUCCESS((status))) {
			sd_err(("bcmsdh_fd Function: failed to allocate "
			        "slot current %d\n",
			        status));
			if (status == SDIO_STATUS_NO_RESOURCES) {
				sd_err(("bcmsdh_fd Function: Remaining Slot "
				        "Current: %d mA\n",
				        slotCurrent.SlotCurrent));
			}
			break;
		}

		status = SDLIB_IssueConfig(pDevice,
		                           SDCONFIG_FUNC_NO_IRQ_PEND_CHECK,
		                           NULL,
		                           0);

		if (func != 2) {
			/* enable the card */
			fData.EnableFlags = SDCONFIG_ENABLE_FUNC;
			fData.TimeOut = 500;
			status = SDLIB_IssueConfig(pDevice,
			                           SDCONFIG_FUNC_ENABLE_DISABLE,
			                           &fData,
			                           sizeof(fData));
			if (!SDIO_SUCCESS((status))) {
				sd_err(("bcmsdh_fd Function: Initialize, failed to "
				        "enable function %d\n",
				        status));
				break;
			}
		}
	} while (FALSE);

	return status;
}

/* bcmsdh_fd interrupt handler */
void IRQHandler(PVOID pContext)
{
	sdioh_info_t *sd;

	sd_trace(("bcmsdh_fd Function: ***IRQHandler\n"));

	sd = (sdioh_info_t *)pContext;

	ASSERT(sd != NULL);

	if (sd->client_intr_enabled && sd->use_client_ints) {
		sd->intrcount++;
		ASSERT(sd->intr_handler);
		ASSERT(sd->intr_handler_arg);
		(sd->intr_handler)(sd->intr_handler_arg);
	} else {
		sd_err(("%s: Not ready for intr: enabled %d, handler %p\n",
		        __FUNCTION__, sd->client_intr_enabled, sd->intr_handler));
	}
}


/*
 * client_detach - delete an instance
*/
void client_detach(PBCMSDH_FD_CONTEXT pFuncContext,
                   PBCMSDH_FD_INSTANCE pInstance)
{
	SDCONFIG_FUNC_ENABLE_DISABLE_DATA fData;

	pFuncContext->pInstance = NULL;

	ZERO_OBJECT(fData);

	/* try to disable the function */
	fData.EnableFlags = SDCONFIG_DISABLE_FUNC;
	fData.TimeOut = 500;
	SDLIB_IssueConfig(pInstance->pDevice[0],
	                                  SDCONFIG_FUNC_ENABLE_DISABLE,
	                                  &fData,
	                                  sizeof(fData));

	/* free slot current if we allocated any */
	SDLIB_IssueConfig(pInstance->pDevice[0],
	                                  SDCONFIG_FUNC_FREE_SLOT_CURRENT,
	                                  NULL,
	                                  0);

}
