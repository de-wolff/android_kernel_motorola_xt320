/*
 *  'Standard' SDIO HOST CONTROLLER driver - linux portion
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmsdstd_ndis.c,v 1.13.72.4 2010-11-12 19:45:09 kiranm Exp $
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <sdio.h>	/* Standard SD Device Register Map */
#include <sdioh.h>	/* Standard SD Host Control Register Map */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* to get msglevel bit values */
#include <bcmsdstd.h>

struct sdos_info {
	sdioh_info_t *sd;
	NDIS_SPIN_LOCK lock;
};

extern bool check_client_intr(sdioh_info_t *sd);
extern void sdstd_wreg16(sdioh_info_t *sd, uint reg, uint16 data);
extern uint16 sdstd_rreg16(sdioh_info_t *sd, uint reg);
extern uint sdstd_msglevel;
extern void *bcmsdh_get_sdioh(bcmsdh_info_t *sdh);
void sdstd_isr(
	PBOOLEAN InterruptRecognized,
	PBOOLEAN QueueMiniportHandleInterrupt,
	NDIS_HANDLE MiniportAdapterContext);
void sdstd_dpc(NDIS_HANDLE MiniportAdapterContext);
void sdstd_status_intr_enable(void *sdh, bool enable);

/* Polling hack handler */
static void
polling_hack(PVOID a1, NDIS_HANDLE arg, PVOID a2, PVOID a3)
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

/* Register with NDIS for interrupts */
int
sdstd_register_irq(sdioh_info_t *sd, uint irq)
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

#ifndef NDIS60
		status = NdisMRegisterInterrupt(sh->intr, sh->adapterhandle, sd->irq, sh->irq_level,
			TRUE, TRUE, NdisInterruptLevelSensitive);
#else
		/* FIX: Use NdisMRegisterInterruptEx() for NDIS60 */
		status = NDIS_STATUS_FAILURE;
#endif
		if (NDIS_ERROR(status)) {
			sd_err(("%s: NdisMRegisterInterrupt error 0x%x\n",  __FUNCTION__, status));
#ifndef NDIS60
			sh->intr = NULL;
#endif
			return ERROR;
		}
	}
	return SUCCESS;
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

	/* Did the SD host interrupt ? */
	if (!(sdstd_rreg16(sd, SD_IntrStatus) & CLIENT_INTR))
		return;

#ifndef BCMDONGLEHOST
	/* Disable interrupt from sd host */
	sdstd_wreg16(sd, SD_IntrSignalEnable, ~CLIENT_INTR);
#else /* BCMDONGLEHOST */

	/* Did the SD host interrupt ? */
	if (!(sdstd_rreg16(sd, SD_IntrSignalEnable) & CLIENT_INTR))
		return;

	sdstd_status_intr_enable(sh->sdh, FALSE);

	if (sh->isr_cb == NULL) {
		sd_err(("%s: isr_cb is NULL!\n", __FUNCTION__));
		return;
	}

	/* Call the isr handler of the DHD module */
	sh->isr_cb(InterruptRecognized, QueueMiniportHandleInterrupt, sh->isr_arg);
#endif /* BCMDONGLEHOST */

	sd->local_intrcount++;

#ifndef BCMDONGLEHOST
	/* Tell NDIS to schedule the DPC; For dongle, let isr_cb() decide this */
	*QueueMiniportHandleInterrupt = TRUE;
	*InterruptRecognized = TRUE;
#endif
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

	ASSERT(sh->sdh);

	if (!sh->attached)
		return;

	sd = bcmsdh_get_sdioh(sh->sdh);

	ASSERT(sd);

#ifdef BCMDONGLEHOST
	sh->dpc_cb(sh->dpc_arg);
#else /* BCMDONGLEHOST */
	NdisAcquireSpinLock(sh->ndlock);

	/* check for client interrupts run service routine if recognized */
	check_client_intr(sd);

	/* Reenable SD host interrupt */
	sdstd_wreg16(sd, SD_IntrSignalEnable, CLIENT_INTR);

	NdisReleaseSpinLock(sh->ndlock);

	/* Call DPC as appropriate */
	if ((sh->InterruptRecognized == TRUE) &&
		(sh->QueueMiniportHandleInterrupt == TRUE))
			sh->dpc_cb(sh);
#endif /* BCMDONGLEHOST */
}

void
sdstd_free_irq(uint irq, sdioh_info_t *sd)
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
#ifndef NDIS60 /* FIX: */
	else {
		if (sh->intr)
			NdisMDeregisterInterrupt(sh->intr);
	}
#endif
}

/* Map Host controller registers */

uint32 *
sdstd_reg_map(osl_t *osh, int32 pa, int size)
{
	return osl_reg_map(osh, (uint32) pa, (uint) size);
}

void
sdstd_reg_unmap(osl_t *osh, int32 va, int size)
{
	osl_reg_unmap(osh, (uint32 *) va, (uint) size);
}

int
sdstd_osinit(sdioh_info_t *sd)
{
	struct sdos_info *sdos;

	sdos = (struct sdos_info*)MALLOC(sd->osh, sizeof(struct sdos_info));
	sd->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	/* spin_lock_init(&sdos->lock); */
	NdisAllocateSpinLock(&sdos->lock);
	return BCME_OK;
}

void
sdstd_osfree(sdioh_info_t *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	NdisFreeSpinLock(&sdos->lock);
	MFREE(sd->osh, sdos, sizeof(struct sdos_info));
}

bool
sdstd_hc_poll_mode(osl_t *osh)
{
	shared_info_t *sh;
	sh = osl_get_sh(osh);
	return sh->poll_mode;
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
	NdisAcquireSpinLock(&sdos->lock);

	sd->client_intr_enabled = enable;
	if (enable && !sd->lockcount)
		sdstd_devintr_on(sd);
	else
		sdstd_devintr_off(sd);

	/* spin_unlock_irqrestore(&sdos->lock, flags); */
	NdisReleaseSpinLock(&sdos->lock);

	return SDIOH_API_RC_SUCCESS;
}

/* Protect against reentrancy (disable device interrupts while executing) */
void
sdstd_lock(sdioh_info_t *sd)
{
	ulong flags;
	struct sdos_info *sdos;

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	sd_trace(("%s: %d\n", __FUNCTION__, sd->lockcount));

	/* spin_lock_irqsave(&sdos->lock, flags); */
	NdisAcquireSpinLock(&sdos->lock);

	if (sd->lockcount) {
		sd_err(("%s: Already locked!\n", __FUNCTION__));
		ASSERT(sd->lockcount == 0);
	}
	sdstd_devintr_off(sd);
	sd->lockcount++;
	/* spin_unlock_irqrestore(&sdos->lock, flags); */
	NdisReleaseSpinLock(&sdos->lock);
}

/* Enable client interrupt */
void
sdstd_unlock(sdioh_info_t *sd)
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
		sdstd_devintr_on(sd);
	}
	/* spin_unlock_irqrestore(&sdos->lock, flags); */
	NdisReleaseSpinLock(&sdos->lock);
}

void
sdstd_waitlockfree(sdioh_info_t *sd)
{
}

void
sdstd_status_intr_enable(void *sdh, bool enable)
{
	sdioh_info_t *sd;
	int save_intr_stat_enable;

	sd = bcmsdh_get_sdioh(sdh);

	save_intr_stat_enable = sdstd_rreg16(sd, SD_IntrStatusEnable);
	if (TRUE == enable)
		sdstd_wreg16(sd, SD_IntrStatusEnable, save_intr_stat_enable | CLIENT_INTR);
	else
		sdstd_wreg16(sd, SD_IntrStatusEnable, save_intr_stat_enable & ~CLIENT_INTR);
	sdstd_rreg16(sd, SD_IntrStatusEnable);
}

int
sdstd_waitbits(sdioh_info_t *sd, uint16 norm, uint16 err, bool yield, uint16 *bits)
{
	struct sdos_info *sdos;

	sdos = (struct sdos_info *)sd->sdos_info;

	sd_trace(("%s: int 0x%04x, err 0x%04x, yield %d\n", __FUNCTION__, norm, err, yield));

	/* Clear the "interrupt happened" flag and last intrstatus */
	sd->got_hcint = FALSE;
	sd->last_intrstatus = 0;

	/* XXX Yielding not implemented yet, use spin wait */
	sdstd_spinbits(sd, norm, err);

	sd_trace(("%s: last_intrstatus 0x%04x\n", __FUNCTION__, sd->last_intrstatus));

	*bits = sd->last_intrstatus;

	return 0;
}

#ifdef SDHOST3
void
sdstd_3_start_tuning(sdioh_info_t *sd)
{
	sd_err(("%s: NOT IMPLEMENTED\n", __FUNCTION__));
}

void
sdstd_3_osinit_tuning(sdioh_info_t *sd)
{
	sd_err(("%s: NOT IMPLEMENTED\n", __FUNCTION__));
}

void
sdstd_3_osclean_tuning(sdioh_info_t *sd)
{
	sd_err(("%s: NOT IMPLEMENTED\n", __FUNCTION__));
}
#endif /* DSHOST3 */
