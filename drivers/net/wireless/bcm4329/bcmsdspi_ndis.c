/*
 * Broadcom SPI Host Controller Driver - WinXP Per-port
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmsdspi_ndis.c,v 1.7 2008-10-23 21:56:05 michaelw Exp $
 */

#include <typedefs.h>
#include <bcmutils.h>

#include <sdio.h>		/* SDIO Specs */
#include <bcmsdbus.h>		/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>		/* to get msglevel bit values */

#include <bcmsdspi.h>
#include <bcmspi.h>

struct sdos_info {
	sdioh_info_t *sd;
	NDIS_SPIN_LOCK lock;
};

void sdstd_dpc(NDIS_HANDLE MiniportAdapterContext);
void sdstd_isr(
	PBOOLEAN InterruptRecognized,
	PBOOLEAN QueueMiniportHandleInterrupt,
	NDIS_HANDLE MiniportAdapterContext);
void sdstd_status_intr_enable(void *sdh, bool enable);

extern void *bcmsdh_get_sdioh(bcmsdh_info_t *sdh);

/* Polling hack handler */
void polling_hack(PVOID a1, NDIS_HANDLE arg, PVOID a2, PVOID a3)
{
	shared_info_t *sh;
	volatile BOOLEAN InterruptRecognized = FALSE;
	volatile BOOLEAN QueueMiniportHandleInterrupt = FALSE;

	sh = (shared_info_t *)arg;

	/* Temporary polling hack */

	if (sh->in_isr)
		return;

	if (!sh->attached)
		return;

	sh->in_isr = TRUE;

	NdisAcquireSpinLock(sh->ndlock);
	sdstd_isr((PBOOLEAN)&InterruptRecognized,
		(PBOOLEAN)&QueueMiniportHandleInterrupt, (NDIS_HANDLE)sh);
	NdisReleaseSpinLock(sh->ndlock);

	if ((QueueMiniportHandleInterrupt == TRUE) && (InterruptRecognized == TRUE))
		sdstd_dpc((NDIS_HANDLE)sh);

	sh->in_isr = FALSE;

}

/* Interrupt handler */
void
sdstd_isr(
	PBOOLEAN InterruptRecognized,
	PBOOLEAN QueueMiniportHandleInterrupt,
	NDIS_HANDLE MiniportAdapterContext)
{
	sdioh_info_t *sd;
	shared_info_t *sh;
	bool ours;
	int is_dev_intr;

	*QueueMiniportHandleInterrupt = FALSE;
	*InterruptRecognized = FALSE;

	if (!MiniportAdapterContext)
		return;

	sh = (shared_info_t *)MiniportAdapterContext;

	if (!sh->attached)
		return;

	ASSERT(sh->sdh);
	sd = bcmsdh_get_sdioh(sh->sdh);

	ASSERT(sd);

	if (!sd->card_init_done)
		return;

	ours = spi_check_client_intr(sd, &is_dev_intr);

	/* Tell NDIS to schedule the DPC */
	if (is_dev_intr == TRUE) {
		*QueueMiniportHandleInterrupt = TRUE;
	}
	if (ours)
		*InterruptRecognized = TRUE;
}

/* DPC handler */
void
sdstd_dpc(NDIS_HANDLE MiniportAdapterContext)
{
	sdioh_info_t *sd;
	shared_info_t *sh;
	bool ours;

	if (!MiniportAdapterContext)
		return;

	sh = (shared_info_t *)MiniportAdapterContext;

	ASSERT(sh);
	ASSERT(sh->sdh);

	if (!sh->attached)
		return;

	sd = bcmsdh_get_sdioh(sh->sdh);

	ASSERT(sd);

#ifdef BCMDONGLEHOST
	if (sh->dpc_cb && sh->dpc_arg) {
		sh->dpc_cb(sh->dpc_arg);
	}
#else /* BCMDONGLEHOST */
	NdisAcquireSpinLock(sh->ndlock);

	/* check for client interrupts run service routine if recognized */
	spi_check_client_intr(sd);

	/* Reenable SD host interrupt */
	sdstd_wreg16(sd, SD_IntrSignalEnable, CLIENT_INTR);

	NdisReleaseSpinLock(sh->ndlock);

	/* Call DPC as appropriate */
	if ((sh->InterruptRecognized == TRUE) &&
		(sh->QueueMiniportHandleInterrupt == TRUE))
			sh->dpc_cb(sh);
#endif /* BCMDONGLEHOST */
}

/* Register with OS for interrupts */
int
spi_register_irq(sdioh_info_t *sd, uint irq)
{
	shared_info_t *sh;
	NDIS_STATUS status;

	sh = osl_get_sh(sd->osh);

	if (sh->poll_mode) {
		sh->poll_timer = MALLOC(sh->osh, sizeof(NDIS_MINIPORT_TIMER));
		if (sh->poll_timer == NULL) {
			sd_err(("Unable to allocate NDIS miniport Timer\n", __FUNCTION__));
			return ERROR;
		}
		sh->in_isr = FALSE;
		NdisMInitializeTimer(sh->poll_timer, sh->adapterhandle, polling_hack, (PVOID) sh);
		NdisMSetPeriodicTimer(sh->poll_timer,2); /* Poll every 2 milliseconds */
	}
	else {
		sd_trace(("Entering %s: irq == %d\n", __FUNCTION__, irq));

		status = NdisMRegisterInterrupt(sh->intr, sh->adapterhandle, sd->irq, sh->irq_level,
			TRUE, TRUE, NdisInterruptLevelSensitive);
		if (NDIS_ERROR(status)) {
			sd_err(("%s: NdisMRegisterInterrupt error 0x%x\n",  __FUNCTION__, status));
			sh->intr = NULL;
			return ERROR;
		}
	}
	return SUCCESS;
}

/* Unregister the irq */
void
spi_free_irq(uint irq, sdioh_info_t *sd)
{
	shared_info_t *sh;

	sh = osl_get_sh(sd->osh);

	if (sh->poll_mode) {
		/* Cancel Polling timer */
		if (sh->poll_timer) {
			BOOLEAN done = FALSE;
			uint x;
			sh->attached = FALSE;
			for (x = 0; (x < 1000) && !done; x++)
				{
				NdisMCancelTimer(sh->poll_timer, &done);
				OSL_DELAY(5);
				}
			if (!done)
				printf("%s: Warning polltimer not cancelled successfully.\n",
					__FUNCTION__);
			else
				printf("%s: Polltimer cancelled, count=%d.\n", __FUNCTION__, x);
			MFREE(sh->osh, sh->poll_timer, sizeof(NDIS_MINIPORT_TIMER));
			sh->poll_timer = NULL;
		}
	}
	else {
		if (sh->intr)
			NdisMDeregisterInterrupt(sh->intr);
		else
			ASSERT(0);
	}
}

/* Map Host controller registers */

uint32 *
spi_reg_map(osl_t *osh, uintptr addr, int size)
{
	return osl_reg_map(osh, (uint32) addr, (uint) size);
}

void
spi_reg_unmap(osl_t *osh, uintptr addr, int size)
{
	osl_reg_unmap(osh, (uint32 *) addr, (uint) size);
}

int
spi_osinit(sdioh_info_t *sd)
{
	struct sdos_info *sdos;

	sdos = (struct sdos_info*)MALLOC(sd->osh, sizeof(struct sdos_info));
	sd->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	NdisAllocateSpinLock(&sdos->lock);
	return BCME_OK;
}

void
spi_osfree(sdioh_info_t *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	NdisFreeSpinLock(&sdos->lock);
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

	if (!(sd->host_init_done && sd->card_init_done)) {
		sd_err(("%s: Card & Host are not initted - bailing\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	/* Ensure atomicity for enable/disable calls */
	/* spin_lock_irqsave(&sdos->lock, flags); */
	/* XXX This function can be called from an ISR too?
	NdisAcquireSpinLock(&sdos->lock);
	*/

	sd->client_intr_enabled = enable;
	if (enable && !sd->lockcount)
		spi_devintr_on(sd);
	else
		spi_devintr_off(sd);

	/* spin_unlock_irqrestore(&sdos->lock, flags); */
	/* XXX
	NdisReleaseSpinLock(&sdos->lock);
	*/

	return SDIOH_API_RC_SUCCESS;
}

/* Protect against reentrancy (disable device interrupts while executing) */
void
spi_lock(sdioh_info_t *sd)
{
	ulong flags;
	struct sdos_info *sdos;

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	sd_trace(("%s: %d\n", __FUNCTION__, sd->lockcount));

	/* spin_lock_irqsave(&sdos->lock, flags); */
	NdisAcquireSpinLock(&sdos->lock);

#if 0
	if (sd->lockcount) {
		sd_err(("%s: Already locked!\n", __FUNCTION__));
		ASSERT(sd->lockcount == 0);
	}
#endif
	spi_devintr_off(sd);
	sd->lockcount++;
	/* spin_unlock_irqrestore(&sdos->lock, flags); */
	NdisReleaseSpinLock(&sdos->lock);
}

/* Enable client interrupt */
void
spi_unlock(sdioh_info_t *sd)
{
	ulong flags;
	struct sdos_info *sdos;

	sd_trace(("%s: %d, %d\n", __FUNCTION__, sd->lockcount, sd->client_intr_enabled));
	ASSERT(sd->lockcount > 0);

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	/* spin_lock_irqsave(&sdos->lock, flags); */
	NdisAcquireSpinLock(&sdos->lock);

	if (--sd->lockcount == 0 && sd->client_intr_enabled) {
		spi_devintr_on(sd);
	}
	/* spin_unlock_irqrestore(&sdos->lock, flags); */
	NdisReleaseSpinLock(&sdos->lock);
}

void
sdstd_status_intr_enable(void *sdh, bool enable)
{
	sdioh_info_t *sd;

	sd = bcmsdh_get_sdioh(sdh);
	if (enable)
		spi_unlock(sd);
	else
		spi_lock(sd);
}

/* XXX Needs to be properly implemented for NDIS.  */
void spi_waitbits(sdioh_info_t *sd, bool yield)
{
	spi_spinbits(sd);
}
