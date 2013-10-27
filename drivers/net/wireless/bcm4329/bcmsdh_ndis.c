/*
 * SDIO access interface for drivers - NDIS specific (pci only)
 *
 * $Copyright(c) 2005 Broadcom Corporation$
 * $Id: bcmsdh_ndis.c,v 1.14 2009-10-20 03:07:14 hharte Exp $
 */

#define __UNDEF_NO_VERSION__

#include <typedefs.h>

#include <osl.h>
#include <pcicfg.h>
#include <bcmdevs.h>

#include <wlioctl.h>
#include <proto/ethernet.h>

#ifdef BCMDONGLEHOST
#include <dngl_stats.h>
#include <dhd.h>
#endif /* BCMDONGLEHOST */

/** 
 * SDIO Host Controller info
 */
typedef struct bcmsdioh_hc bcmsdioh_hc_t;
struct bcmsdioh_hc {
	bcmsdioh_hc_t *next;
	struct pci_dev *dev;		/* pci device handle */
	osl_t *osh;
	void *regs;			        /* SDIO Host Controller address */
    bcmsdh_info_t *sdh;		    /* SDIO Host Controller handle */
    void *ch;
};
static bcmsdioh_hc_t *sdhcinfo = NULL;

#ifdef BCMDONGLEHOST
/* driver info, initialized when bcmsdh_register is called */
static bcmsdh_driver_t drvinfo = {NULL, NULL};
#endif /* BCMDONGLEHOST */

/* debugging macros */
#if defined(BCMDBG) || defined(BCMDBG_ERR)
#define SDLX_MSG(x)	printf x
#else
#define SDLX_MSG(x)
#endif /* defined(BCMDBG) || defined(BCMDBG_ERR) */

/** 
 * Checks to see if vendor and device IDs match a supported SDIO Host Controller.
 */
bool
bcmsdioh_chipmatch(uint16 vendor, uint16 device)
{
	/* Add other vendors (ie sdnow, etc) and devices as required */
	if (device == SDIOH_FPGA_ID && vendor == VENDOR_BROADCOM) {
		return (TRUE);
	}
	/* For now still accept the old devid */
	if (device == 0x4380 && vendor == VENDOR_BROADCOM) {
		return (TRUE);
	}

	/* Check for Arasan host controller */
	if (vendor == 0x1095)
		return (TRUE);

	return (FALSE);
}

#ifdef BCMDONGLEHOST

int
bcmsdh_probe(shared_info_t *sh, void* bar, void** regsva, uint irq, int * dhd, void * wl)
{
	bcmsdh_info_t	*sdh;
	int				ret = -1;
	void			* dhd_bus = NULL;
	uint32			vendevid;

#ifdef UNDER_CE
#ifdef NDIS60
	sdh = bcmsdh_attach(sh->osh, sh->adapterhandle, regsva, 0);
	if (sdh == NULL) {
		goto err;
	}
#else
	sdh = bcmsdh_attach(sh->osh, sh->confighandle, regsva, (int)wl);
	if (sdh == NULL) {
		goto err;
	}
#endif /* NDIS60 */
#else
	sdh = bcmsdh_attach(sh->osh, bar, regsva, irq);
	if (sdh == NULL) {
		goto err;
	}
#endif /* UNDER_CE */

	sh->sdh = sdh;

	/* Read the vendor/device ID from the CIS */
	vendevid = bcmsdh_query_device(sdh);

	/* try to attach to the target device */
	if (!(dhd_bus = drvinfo.attach((vendevid >> 16), (vendevid & 0xFFFF),
	                               0, 0, 0, 0, * regsva, sh->osh, sh->sdh))) {

		SDLX_MSG(("%s: device attach failed\n", __FUNCTION__));
		goto err;
	}

	ret = 0;

	/* error handling */
err:
	* dhd = (int)dhd_bus;
	return ret;
}

void
bcmsdh_remove(osl_t *osh, void *instance, shared_info_t *sh)
{
	if (sh) {
		drvinfo.detach(instance);

		if (sh->sdh)
			bcmsdh_detach(osh, sh->sdh);
	}
}

int
bcmsdh_register(bcmsdh_driver_t *driver)
{
	drvinfo = *driver;

	return 0;
}

void
bcmsdh_unregister(void)
{
	memset(&drvinfo, 0, sizeof(drvinfo));
}

#endif /* BCMDONGLEHOST */
