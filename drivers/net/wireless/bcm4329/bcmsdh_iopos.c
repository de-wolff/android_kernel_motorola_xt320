/*
 * SDIO access interface for drivers - IOPOS portion
 *
 * $Copyright(c) 2005 Broadcom Corporation$
 * $Id: bcmsdh_iopos.c,v 1.6 2008-12-01 21:56:19 nvalji Exp $
 */

#define __UNDEF_NO_VERSION__

#include <typedefs.h>
#include <osl.h>
#include <pcicfg.h>
#include <bcmdevs.h>
#include <bcmdefs.h>
#include <bcmsdh.h>
#include <sdio_iopos.h>

/* SDIO Host Controller info */
struct bcmsdh_hc {
	osl_t *osh;
	void *regs;		/* SDIO Host Controller address */
	bcmsdh_info_t *sdh;	/* SDIO Host Controller handle */
	void *ch;
};
static struct bcmsdh_hc sd_inf_struct, *sd_info = &sd_inf_struct;

/* driver info, initialized when bcmsdioh_register is called */
static bcmsdh_driver_t drvinfo = {NULL, NULL};

#if defined(BCMDBG) || defined(BCMDBG_ERR)
#define SDLX_MSG(x)	printf x
#else
#define SDLX_MSG(x)
#endif

int
bcmsdh_register(bcmsdh_driver_t *driver)
{
	drvinfo = *driver;
	return (1);
}

/* 
 * Already found Arasan controller in a slot and got address of regs.
 * Now try to attach to wireless card.
 */
void *
bcmsdh_pci_probe(uint vendor, uint device, uint bus, uint slot, uint func, uint bustype, uint regs)
{
	osl_t *osh = NULL;
	uint base_addr;

	/* This might be a supportable device but we won't know
	 * until we go over the SDIO bus and see whats is there
	 */

	/* allocate SDIO Host Controller state info */
	if (!(osh = osl_attach(bus, slot, func, bustype, FALSE))) {
		SDLX_MSG(("%s: osl_attach failed\n", __FUNCTION__));
		goto err;
	}
	sd_info->osh = osh;

	if (!(sd_info->sdh = bcmsdh_attach(osh, (void *)regs, (void *)&base_addr, 0))) {
		if (slot != 0)  /* Only complain about soldered slot */
			SDLX_MSG(("%s: %d: bcmsdh_attach failed\n", __FUNCTION__, slot));
		goto err;
	}

	/* try to attach to the target device */
	if (!(sd_info->ch = drvinfo.attach(VENDOR_BROADCOM,
	      BCM4318_D11G_ID, bus, slot, func, SDIO_BUS, (void *)base_addr, NULL,
	      sd_info->sdh))) {
		SDLX_MSG(("%s: device attach failed\n", __FUNCTION__));
		goto err;
	}
	return (sd_info->ch);

err:
	if (sd_info->sdh)
		bcmsdh_detach(sd_info->osh, sd_info->sdh);
	if (osh)
		osl_detach(osh);
	return NULL;
}


#ifdef LATER
/* 
 * Detach from target devices and SDIO Host Controller
 */
void
bcmsdh_remove()
{
	drvinfo.detach(sd_info->ch);
	bcmsdh_detach(sd_info->osh, sd_info->sdh);
	osl_detach(sd_info->osh);
}
#endif
