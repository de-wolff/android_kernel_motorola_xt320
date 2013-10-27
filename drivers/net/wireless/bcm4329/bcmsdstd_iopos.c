/*
 *  'Standard' SDIO HOST CONTROLLER driver - linux portion
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmsdstd_iopos.c,v 1.7.406.2 2010-11-11 08:57:44 csm Exp $
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <sdio.h>	/* SDIO Specs */
#include <sdioh.h>	/* SDIO Host Controller Specification */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */

#include <ahb_fdl_defs.h> /* Need to get SD1_BASE */
#include <ios.h>
#include <iostypes.h>
#include <sdiovar.h>  	/* ioctl/iovars */

#define WL_EVENT_SD 	2
#define SD_INTR_ID 	7	/* Used as signature. Actual value is unimportant. */

#include <bcmsdstd.h>

extern bool check_client_intr(sdioh_info_t *sd);
extern void sdstd_wreg16(sdioh_info_t *sd, uint reg, uint16 data);
extern uint16 sdstd_rreg16(sdioh_info_t *sd, uint reg);
extern uint sdstd_msglevel;

/* Map Host controller registers */
uint32 *
sdstd_reg_map(osl_t *osh, int32 addr, int size)
{
	return (uint32 *)REG_MAP(addr, size);
}

void
sdstd_reg_unmap(osl_t *osh, int32 addr, int size)
{
	REG_UNMAP((void*)(uintptr)addr);
}


/* Same as regular routines, except no printfs, for debugging only */
uint32
priv_sdstd_rreg(sdioh_info_t *sd, uint reg)
{
	volatile uint32 data = *(volatile uint32 *)(sd->mem_space + reg);
	return data;
}
inline void
priv_sdstd_wreg(sdioh_info_t *sd, uint reg, uint32 data)
{
	*(volatile uint32 *)(sd->mem_space + reg) = (volatile uint32)data;

}

extern sdioh_info_t *glob_sd;

void
sdstd_isr(void *foo)
{
	sdioh_info_t *sd;

	sd = glob_sd;
	sd->local_intrcount++;

	if (!sd->card_init_done) {
		sd_err(("%s: Hey Bogus intr...not even initted: \n", __FUNCTION__));
		return;
	} else {
		check_client_intr(sd);
		return;
	}
}

void
sd_sync_dma(sdioh_info_t *sd, bool read, int nbytes)
{
	if (read) {
		IOS_FlushMem(sd->flush_desc);
		IOS_InvalidateDCache(sd->dma_buf, nbytes);
	} else {
		IOS_FlushDCache(sd->dma_buf, nbytes);
		IOS_InvalidateRdb(sd->inv_desc);
	}
}

void
sd_init_dma(sdioh_info_t *sd)
{
	if (sd->mem_space == (char *)SD1_BASE) {
		sd->flush_desc = IOS_WB_SD1;
		sd->inv_desc = IOS_RB_SD1;
	} else {
		ASSERT(sd->mem_space == (char *)SD0_BASE);
		sd->flush_desc = IOS_WB_SD0;
		sd->inv_desc = IOS_RB_SD0;
	}
}

void
sdstd_free_irq(uint irq, sdioh_info_t *sd)
{
	sd_trace(("%s\n", __FUNCTION__));
	return;
}

void
sd_register_qid(IOSMessageQueueId wl_qid)
{
}
int
sdstd_register_irq(sdioh_info_t *sd, uint xx)
{
	return SUCCESS;
}

/* Interrupt enable/disable */
SDIOH_API_RC
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));

	if (!(sd->host_init_done && sd->card_init_done)) {
		sd_err(("%s: Card & Host are not initted - bailing\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	sd->client_intr_enabled = enable;
	if (enable && !sd->lockcount)
		sdstd_devintr_on(sd);
	else
		sdstd_devintr_off(sd);

	return SDIOH_API_RC_SUCCESS;
}

/* Protect against reentrancy (disable device interrupts while executing) */
void
sdstd_lock(sdioh_info_t *sd)
{
	sd_trace(("%s: %d\n", __FUNCTION__, sd->lockcount));

	if (sd->lockcount) {
		sd_err(("%s: Already locked!\n", __FUNCTION__));
		ASSERT(sd->lockcount == 0);
	}
	sdstd_devintr_off(sd);
	sd->lockcount++;
}

/* Enable client interrupt */
void
sdstd_unlock(sdioh_info_t *sd)
{
	ASSERT(sd->lockcount > 0);
	sd_trace(("%s: %d, %d\n", __FUNCTION__, sd->lockcount, sd->client_intr_enabled));

	if (--sd->lockcount == 0 && sd->client_intr_enabled) {
		sdstd_devintr_on(sd);
	}
}

void
sdstd_waitlockfree(sdioh_info_t *sd)
{
}

int
sdstd_waitbits(sdioh_info_t *sd, uint16 norm, uint16 err, bool yield, uint16 *bits)
{
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
