/*
 *  BCMSDH Function Driver for SDIO-Linux
 *
 * $ Copyright Dual License Broadcom Corporation $
 *
 * $Id: bcmsdh_fd_linux.c,v 1.10 2008-11-25 21:02:44 csm Exp $
 */

#include <typedefs.h>
#include <pcicfg.h>
#include <bcmutils.h>
#include <sdio.h>	/* SDIO Specs */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* to get msglevel bit values */

#include <linux/sched.h>	/* request_irq() */

#undef INLINE
#include <linux/sdio/ctsystem.h>

#include <linux/sdio/sdio_busdriver.h>
#include <linux/sdio/_sdio_defs.h>
#include <linux/sdio/sdio_lib.h>

#include <bcmsdh_fd.h>

extern void sdiohfd_devintr_off(sdioh_info_t *sd);
extern void sdiohfd_devintr_on(sdioh_info_t *sd);

#define DESCRIPTION "bcmsdh_fd Driver"
#define AUTHOR "Broadcom Corporation"

/* module param defaults */
static int sdio_manfID = 0;
static int sdio_manfcode = 0;
static int sdio_funcno = 0;
static int sdio_class = 0;
static int clockoverride = 0;

module_param(sdio_manfID, int, 0644);
MODULE_PARM_DESC(sdio_manfID, "SDIO manufacturer ID override");

module_param(sdio_manfcode, int, 0644);
MODULE_PARM_DESC(sdio_manfcode, "SDIO manufacturer Code overide");

module_param(sdio_funcno, int, 0644);
MODULE_PARM_DESC(sdio_funcno, "SDIO function number override");

module_param(sdio_class, int, 0644);
MODULE_PARM_DESC(sdio_class, "SDIO function class override");

module_param(clockoverride, int, 0644);
MODULE_PARM_DESC(clockoverride, "SDIO card clock override");

PBCMSDH_FD_INSTANCE gInstance;

/* Maximum number of bcmsdh_fd devices supported by driver */
#define BCMSDH_FD_MAX_DEVICES 1

BOOL Probe(PSDFUNCTION pFunction, PSDDEVICE pDevice);
void Remove(PSDFUNCTION pFunction, PSDDEVICE pDevice);
static void CleanupInstance(PBCMSDH_FD_CONTEXT  pFunctionContext,
                                                        PBCMSDH_FD_INSTANCE pInstance);

BOOL ProbeF2(PSDFUNCTION pFunction, PSDDEVICE pDevice);
void RemoveF2(PSDFUNCTION pFunction, PSDDEVICE pDevice);

/* devices we support, null terminated */
static SD_PNP_INFO Ids[] = {
{.SDIO_ManufacturerID = 0x492,	  /* 4325 bg SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 1,
.SDIO_FunctionClass = 0},
{.SDIO_ManufacturerID = 0x493,	  /* 4325 abg SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 1,
.SDIO_FunctionClass = 0},
{.SDIO_ManufacturerID = 0x4325,	  /* specific SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 1,
.SDIO_FunctionClass = 0},
{.SDIO_ManufacturerID = 0x4329,   /* specific SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 1,
.SDIO_FunctionClass = 0},
{}
};

/* devices we support, null terminated */
static SD_PNP_INFO IdsF2[] = {

{.SDIO_ManufacturerID = 0x492,	  /* 4325 bg SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 2,
.SDIO_FunctionClass = 0},
{.SDIO_ManufacturerID = 0x493,	  /* 4325 abg SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 2,
.SDIO_FunctionClass = 0},
{.SDIO_ManufacturerID = 0x4325,	  /* specific SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 2,
.SDIO_FunctionClass = 0},
{.SDIO_ManufacturerID = 0x4329,   /* specific SDIO card */
.SDIO_ManufacturerCode = 0x2d0,
.SDIO_FunctionNo = 2,
.SDIO_FunctionClass = 0},
{}
};

/* driver wide data */
static BCMSDH_FD_CONTEXT FunctionContext = {
	.Function.pName	   = "bcmsdh_fd",
	.Function.Version  = CT_SDIO_STACK_VERSION_CODE,
	.Function.MaxDevices = BCMSDH_FD_MAX_DEVICES,
	.Function.NumDevices = 0,
	.Function.pIds	   = Ids,
	.Function.pProbe   = Probe,
	.Function.pRemove  = Remove,
	.Function.pSuspend = NULL,
	.Function.pResume  = NULL,
	.Function.pWake	   = NULL,
	.Function.pContext = &FunctionContext,
	};

static BCMSDH_FD_CONTEXT FunctionF2Context = {
	.Function.pName	   = "bcmsdh_fd_f2",
	.Function.Version  = CT_SDIO_STACK_VERSION_CODE,
	.Function.MaxDevices = BCMSDH_FD_MAX_DEVICES,
	.Function.NumDevices = 0,
	.Function.pIds	   = IdsF2,
	.Function.pProbe   = ProbeF2,
	.Function.pRemove  = RemoveF2,
	.Function.pSuspend = NULL,
	.Function.pResume  = NULL,
	.Function.pWake	   = NULL,
	.Function.pContext = &FunctionF2Context,
	};


static int numInstances = 0;

struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
};


int
sdiohfd_osinit(sdioh_info_t *sd)
{
	struct sdos_info *sdos;

	sdos = (struct sdos_info*)MALLOC(sd->osh, sizeof(struct sdos_info));
	sd->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	spin_lock_init(&sdos->lock);
	return BCME_OK;
}

void
sdiohfd_osfree(sdioh_info_t *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	MFREE(sd->osh, sdos, sizeof(struct sdos_info));
}

/* Interrupt enable/disable */
SDIOH_API_RC
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	ulong flags;
	struct sdos_info *sdos;

	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	/* Ensure atomicity for enable/disable calls */
	spin_lock_irqsave(&sdos->lock, flags);

	sd->client_intr_enabled = enable;
	if (enable)
		sdiohfd_devintr_on(sd);
	else
		sdiohfd_devintr_off(sd);

	spin_unlock_irqrestore(&sdos->lock, flags);

	return SDIOH_API_RC_SUCCESS;
}


/*
 * Probe - a device potentially for us
 *
 * notes: probe is called when the bus driver has located a card for us to support.
 *		  We accept the new device by returning TRUE.
*/
BOOL
Probe(PSDFUNCTION pFunction, PSDDEVICE pDevice)
{
	PBCMSDH_FD_CONTEXT pFunctionContext =
	(PBCMSDH_FD_CONTEXT)pFunction->pContext;

	BOOL		  accept;
	PBCMSDH_FD_INSTANCE pNewInstance = NULL;
	int i;

	sd_trace(("bcmsdh_fd: %s\n", __FUNCTION__));

	accept = FALSE;

	/* make sure this is a device we can handle
	 * the card must match the manufacturer and card ID
	 */
	for (i = 0; pFunctionContext->Function.pIds[i].SDIO_ManufacturerID != 0; i++) {
		if ((pDevice->pId[0].SDIO_ManufacturerID ==
		     pFunctionContext->Function.pIds[i].SDIO_ManufacturerID) &&
			(pDevice->pId[0].SDIO_ManufacturerCode ==
		     pFunctionContext->Function.pIds[i].SDIO_ManufacturerCode)) {

			accept = TRUE;
		}
		/* check for class */
		if (pFunctionContext->Function.pIds[i].SDIO_FunctionClass != 0) {
			if (pDevice->pId[0].SDIO_FunctionClass ==
				pFunctionContext->Function.pIds[i].SDIO_FunctionClass) {
				accept = TRUE;
			}
		}
	}

	if (!accept) {
		sd_trace(("bcmsdh_fd: %s - not our card (0x%X/0x%X/0x%X/0x%X)\n",
		          __FUNCTION__,
		          pDevice->pId[0].SDIO_ManufacturerID,
		          pDevice->pId[0].SDIO_ManufacturerCode,
		          pDevice->pId[0].SDIO_FunctionNo,
		          pDevice->pId[0].SDIO_FunctionClass));
		return FALSE;
	}

	sd_trace(("bcmsdh_fd: %s - card matched (0x%X/0x%X/0x%X/0x%X)\n",
	          __FUNCTION__,
	          pDevice->pId[0].SDIO_ManufacturerID,
	          pDevice->pId[0].SDIO_ManufacturerCode,
	          pDevice->pId[0].SDIO_FunctionNo,
	          pDevice->pId[0].SDIO_FunctionClass));
	accept = FALSE;
	do {
		if (numInstances == 0) {
			/* create a new instance of a device and iniinitialize the device */
			pNewInstance = (PBCMSDH_FD_INSTANCE)KernelAlloc(
				sizeof(BCMSDH_FD_INSTANCE));
			if (pNewInstance == NULL) {
				break;
			}
			ZERO_POBJECT(pNewInstance);
			gInstance = pNewInstance;
			pNewInstance->pDevice[0] = pDevice;
			pNewInstance->pDevice[1] = pDevice;
			pNewInstance->pDevice[2] = pDevice;
		} else {
				pNewInstance = gInstance;
		}

		if (!SDIO_SUCCESS(client_init(pFunctionContext, pNewInstance, pDevice))) {
			break;
		}

		if (numInstances == 0) {
			/* add it to the list */
			pFunctionContext->pInstance = pNewInstance;
		}

		accept = TRUE;
	} while (FALSE);

	if (!accept && (pNewInstance != NULL)) {
		CleanupInstance(pFunctionContext, pNewInstance);
	}

	sd_trace(("%s: exit\n", __FUNCTION__));

	return accept;
}

/*
 * Remove - our device is being removed
*/
void
Remove(PSDFUNCTION pFunction, PSDDEVICE pDevice)
{
	PBCMSDH_FD_CONTEXT pFunctionContext =
	                        (PBCMSDH_FD_CONTEXT)pFunction->pContext;
	PBCMSDH_FD_INSTANCE pInstance;

	sd_trace(("+bcmsdh_fd: %s\n", __FUNCTION__));

	pInstance = pFunctionContext->pInstance;

	if (pInstance != NULL) {
		sd_trace(("bcmsdh_fd: Removing instance: 0x%X From %s()\n",
		          (INT)pInstance, __FUNCTION__));
		CleanupInstance(pFunctionContext, pInstance);
		gInstance = NULL;
	} else {
		sd_err(("bcmsdh_fd: could not find matching instance!\n"));
	}
}

void
IRQHandlerF2(PVOID pContext)
{
	PSDDEVICE pDevice;
	SDIO_STATUS	  status = SDIO_STATUS_DEVICE_ERROR;

	pDevice = (PSDDEVICE)pContext;

	status = SDLIB_IssueConfig(pDevice, SDCONFIG_FUNC_ACK_IRQ, NULL, 0);
}

BOOL
ProbeF2(PSDFUNCTION pFunction, PSDDEVICE pDevice)
{
	sd_trace(("bcmsdh_fd: %s\n", __FUNCTION__));
	if (gInstance != NULL) {
		gInstance->pDevice[2] = pDevice;
	} else {
		sd_err(("%s: Function 1 is not yet initialized!\n", __FUNCTION__));
	}

	return (TRUE);
}

/*
 * Remove - our device is being removed
*/
void
RemoveF2(PSDFUNCTION pFunction, PSDDEVICE pDevice)
{
	sd_trace(("bcmsdh_fd: %s\n", __FUNCTION__));
}

static void
CleanupInstance(PBCMSDH_FD_CONTEXT pFunctionContext,
                PBCMSDH_FD_INSTANCE pInstance)
{
	client_detach(pFunctionContext, pInstance);
	gInstance = NULL;
	KernelFree(pInstance);
}


/*
 * module init
*/
int
sdio_function_init(void)
{
	int status;

	sd_trace(("%s: Enter\n", __FUNCTION__));

	gInstance = NULL;

	/* register with bus driver core */
	status = SDIOErrorToOSError(SDIO_RegisterFunction(&FunctionContext.Function));
	status = SDIOErrorToOSError(SDIO_RegisterFunction(&FunctionF2Context.Function));

	return (status);
}

/*
 * module cleanup
*/
void
sdio_function_cleanup(void)
{
	sd_trace(("%s: Enter\n", __FUNCTION__));
	/* unregister, this will call Remove() for each device */
	SDIO_UnregisterFunction(&FunctionContext.Function);
	SDIO_UnregisterFunction(&FunctionF2Context.Function);

	sd_trace(("%s: Exit\n", __FUNCTION__));
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_AUTHOR(AUTHOR);

#ifdef BCMSDH_MODULE
static int __init
bcmsdh_module_init(void)
{
	int error = 0;
	sdio_function_init();
	return error;
}

static void __exit
bcmsdh_module_cleanup(void)
{
	sdio_function_cleanup();
}

module_init(bcmsdh_module_init);
module_exit(bcmsdh_module_cleanup);

#endif /* BCMSDH_MODULE */
