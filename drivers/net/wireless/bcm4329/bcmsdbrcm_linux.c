/*
 *  BRCM SDIO HOST CONTROLLER driver - linux portion
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmsdbrcm_linux.c,v 1.15 2008-05-28 18:31:12 hharte Exp $
 */

#include <typedefs.h>
#include <pcicfg.h>

#include <bcmutils.h>
#include <sdio.h>	/* SDIO Specs */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <siutils.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <sbsdioh.h>
#include <sdiovar.h>
#include <bcmsdbrcm.h>
#include <linux/sched.h>	/* request_irq() */

struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
};

/* Interrupt handler */
static irqreturn_t
sdbrcm_isr(int irq, void *dev_id
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
, struct pt_regs *ptregs
#endif
)
{
	struct sdioh_pubinfo *sd;
	bool ours;

	sd = (struct sdioh_pubinfo *)dev_id;
	sd->local_intrcount++;

	if (!sd->dev_init_done) {
		sd_err(("%s: Hey Bogus intr...not even initted: irq %d\n", __FUNCTION__, irq));
		ours = FALSE;
	} else {
		ours = isr_sdbrcm_check_dev_intr(sd);
	}
	return IRQ_RETVAL(ours);
}

/* Register with Linux for interrupts */
int
sdbrcm_register_irq(sdioh_info_t *sd, uint irq)
{
	sd_trace(("Entering %s: irq == %d\n", __FUNCTION__, irq));
	if (request_irq(irq, sdbrcm_isr, IRQF_SHARED, "bcmsdbrcm", sd) < 0) {
		sd_err(("%s: request_irq() failed\n", __FUNCTION__));
		return (BCME_ERROR);
	}
	return (BCME_OK);
}

/* Free Linux irq */
void
sdbrcm_free_irq(uint irq, sdioh_info_t *sd)
{
	free_irq(irq, sd);
}

/* Map Host controller registers */

uint32 *
sdbrcm_reg_map(osl_t *osh, int32 addr, int size)
{
	return (uint32 *)REG_MAP(addr, size);
}

void
sdbrcm_reg_unmap(osl_t *osh, int32 addr, int size)
{
	REG_UNMAP((void*)(uintptr)addr);
}

int
sdbrcm_osinit(sdioh_info_t *sd, osl_t *osh)
{
	struct sdioh_pubinfo *sdp = (struct sdioh_pubinfo*)sd;
	struct sdos_info *sdos;

	sdos = (struct sdos_info*)MALLOC(osh, sizeof(struct sdos_info));
	sdp->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	spin_lock_init(&sdos->lock);
	return BCME_OK;
}

void
sdbrcm_osfree(sdioh_info_t *sd, osl_t *osh)
{
	struct sdioh_pubinfo *sdp = (struct sdioh_pubinfo*)sd;
	struct sdos_info *sdos;
	ASSERT(sdp && sdp->sdos_info);

	sdos = (struct sdos_info *)sdp->sdos_info;
	MFREE(osh, sdos, sizeof(struct sdos_info));
}

/* Interrupt enable/disable */
int
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	struct sdioh_pubinfo *sdp = (struct sdioh_pubinfo*)sd;
	struct sdos_info *sdos;
	ulong flags;

	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));

	sdos = (struct sdos_info *)sdp->sdos_info;
	ASSERT(sdos);

	if (!(sdp->host_init_done && sdp->dev_init_done)) {
		sd_err(("%s: Card & Host are not initted - bailing\n", __FUNCTION__));
		return (BCME_ERROR);
	}

	if (enable && !(sdp->intr_registered)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return (BCME_ERROR);
	}

	/* Ensure atomicity for enable/disable calls */
	spin_lock_irqsave(&sdos->lock, flags);

	sdp->dev_intr_enabled = enable;
	if (enable && !sdp->lockcount)
		sdbrcm_devintr_on(sd);
	else
		sdbrcm_devintr_off(sd);

	spin_unlock_irqrestore(&sdos->lock, flags);

	return (BCME_OK);
}

/* Protect against reentrancy (disable device interrupts while executing) */
void
sdbrcm_lock(sdioh_info_t *sd)
{
	struct sdioh_pubinfo *sdp = (struct sdioh_pubinfo*)sd;
	struct sdos_info *sdos;
	ulong flags;

	sd_trace(("%s: %d\n", __FUNCTION__, sdp->lockcount));

	sdos = (struct sdos_info *)sdp->sdos_info;
	ASSERT(sdos);

	spin_lock_irqsave(&sdos->lock, flags);
	if (sdp->lockcount) {
		sd_err(("%s: Already locked!\n", __FUNCTION__));
		ASSERT(sdp->lockcount == 0);
	}
	sdbrcm_devintr_off(sd);
	sdp->lockcount++;
	spin_unlock_irqrestore(&sdos->lock, flags);
}

/* Enable dev interrupt */
void
sdbrcm_unlock(sdioh_info_t *sd)
{
	struct sdioh_pubinfo *sdp = (struct sdioh_pubinfo*)sd;
	struct sdos_info *sdos;
	ulong flags;

	sd_trace(("%s: %d, %d\n", __FUNCTION__, sdp->lockcount, sdp->dev_intr_enabled));
	ASSERT(sdp->lockcount > 0);

	sdos = (struct sdos_info *)sdp->sdos_info;
	ASSERT(sdos);

	spin_lock_irqsave(&sdos->lock, flags);
	if (--sdp->lockcount == 0 && sdp->dev_intr_enabled) {
		sdbrcm_devintr_on(sd);
	}
	spin_unlock_irqrestore(&sdos->lock, flags);
}
