/*
 * Broadcom Dongle Host Driver (DHD), NDIS-specific network interface
 *
 * $Copyright (C) 2003-2005 Broadcom Corporation$
 *
 * $Id: dhd_ndis.c,v 1.119.2.20 2011-02-03 20:22:49 yangj Exp $
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <osl.h>

#include <epivers.h>
#include <bcmutils.h>

#include <wlioctl.h>

#include <bcmendian.h>

#include <proto/ethernet.h>
#include <dngl_stats.h>
#include <dhd.h>
#ifdef BCMDBUS
#include <dbus.h>
#else
#include <dhd_bus.h>
#endif
#include <dhd_proto.h>
#include <dhd_dbg.h>

#include <siutils.h>
#include <bcmendian.h>
#include <sbconfig.h>
#include <sbpcmcia.h>
#if !defined(UNDER_CE)
#include <bcmsrom.h>
#endif /* UNDER_CE */
#include <proto/802.11.h>
#include <proto/802.1d.h>
#include <proto/bcmip.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_key.h>
#include <wlc_channel.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc_pio.h>
#include <wlc.h>
#include <wlc_ethereal.h>
#include <wlc_wowl.h>
#include <wl_oid.h>
#include <wl_export.h>

#if defined(NDIS60)
#include <oidencap.h>
#endif

#include <wl_ndis.h>
#include <wl_ndconfig.h>
#include <ndiserrmap.h>
#include <proto/802.1d.h>
#if defined(EXT_STA) && defined(UNDER_CE)
#include <proto/eapol.h>
#endif

#ifdef UNDER_CE
#include <halether.h>
#include <bldver.h>
#include <bcmsdpcm.h>
#endif /* UNDER_CE */

#ifndef UNDER_CE
#define ETHER_MTU 1500
#endif

#ifdef WLBTAMP
#include <proto/bt_amp_hci.h>
#include <proto/802.11_bta.h>
#include <dhd_bta.h>
#if defined(WLBTWUSB)
#include <bt_int.h>
#elif defined(WLBTCEAMP11)
#include <btceamp11.h>
#endif
#endif /* WLBTAMP */

#define QLEN		128	/* bulk rx and tx queue lengths */
#define PRIOMASK	7 	/* XXX FixMe: should come from elsewhere...
				 * MAXPRIO?  PKTQ_MAX_PREC? WLC? Other?
				 */
#ifdef UNDER_CE
#define CE_THREAD_PRIORITY_MAX 	255
#define CE_THREAD_PRIORITY_MIN 		0
#endif /* UNDER_CE */

#ifdef NDIS60
typedef enum _workitem_state {
	WORKITEM_STATE_FREE = 0,
	WORKITEM_STATE_BUSY = 1
} workitem_state;
#endif /* NDIS60 */

#define CAPABILITY_HIDDEN_NETWORK               0x00000001
#define IFNAMSIZ	16

/* Interface control information */
typedef struct dhd_if {
	struct dhd_info *info;			/* back pointer to dhd_info */
	/* OS/stack specifics */
	int 			idx;			/* iface idx in dongle */
	int 			state;			/* interface state */
	char			name[IFNAMSIZ+1]; /*  interface name */
	uint8		mac_addr[ETHER_ADDR_LEN];	/* assigned MAC address */
	void			*msgque;			/* CE Msgqueue */
	uint8			bssidx;			/* bsscfg index for users */
} dhd_if_t; /* Local private structure (extension of pub) */
typedef struct dhd_info {
	dhd_pub_t pub;

	/* OS/stack specifics */
#ifdef UNDER_CE
	int  ref_count;
	CRITICAL_SECTION proto_sem;
	HANDLE ioctl_resp_wait;
	NDIS_EVENT sendup_event;
	HANDLE sendup_handle;		/* sendup thread handle */
	uint sendup_thread_id;		/* sendup thread id */
	struct pktq	rxq;
#if defined(NDIS60)
	HANDLE event_process_handle;
	uint event_process_thread_id;		/* sendup thread id */
	NDIS_EVENT host_event;
	struct pktq eventq;
#endif /* NDIS60 */
#else /* UNDER_CE */
	KMUTEX	async_sem;
	KMUTEX	proto_sem;
	NDIS_EVENT ioctl_resp_wait;
#endif /* UNDER_CE */
	dhd_if_t *iflist[DHD_MAX_IFS];

	NDIS_MINIPORT_TIMER h_wd_timer;
	bool wd_timer_valid;

	/* holds the pointer to wl structure created in wl_ndis.c */
	void *wl;

#if defined(NDIS60) && !defined(UNDER_CE)
	/* To handle asynchronous request */
	workitem_state asyncReqWorkItemState;
	WDFWORKITEM asyncReqWorkItem;
	NDIS_SPIN_LOCK asyncReqLock;
	LIST_ENTRY asyncReqList;

	NDIS_SPIN_LOCK asyncFreeReqLock;
	LIST_ENTRY asyncFreeReqList;
#endif /* NDIS60 && !UNDER_CE */
} dhd_info_t;

#if defined(BCMSDIO)
extern void sdstd_status_intr_enable(void *sdh, bool enable);
extern void dhdsdio_isr(void *arg);
extern int bcmsdh_probe(shared_info_t *sh, void* bar, void** regsva, uint irq,
                        void ** dhd, void *wl);
extern void bcmsdh_remove(osl_t *osh, void *instance, shared_info_t *sh);
extern bool bcmsdh_reenumerate(void *sdh);
#elif defined(BCMDHDUSB)
extern void wl_reload_init(wl_info_t *wl);
extern void wl_ind_scan_confirm(wl_info_t *wl, NDIS_STATUS StatusBuffer);

#else
#error "No bus defined!"
#endif /* BCMSDIO BCMDHDUSB */

#if defined(NDIS60)
void dhd_rxpkt_cb(async_req_t *req, void *arg);
static int dhd_async_req_init(dhd_pub_t *dhdp);
static int dhd_async_req_deinit(dhd_pub_t *dhdp);
#endif

#ifdef BCMDONGLEHOST
extern void wl_process_mid_txq(wl_info_t *wl);
#endif

#ifdef EXT_STA
static void wl_timer(PVOID systemspecific1, NDIS_HANDLE context,
        PVOID systemspecific2, PVOID systemspecific3);
static NDIS_STATUS wl_ndis_reassoc(wl_info_t *wl);
#endif

static int dhd_open(dhd_info_t *dhd, wl_info_t *wl);
static void dhd_dpc(void *arg);
static int dhd_stop(dhd_info_t *dhd);

#ifdef UNDER_CE
static void dhd_sendup_flush(dhd_pub_t *dhdp);
void dhd_ind_scan_confirm(void *h, bool status);
#if defined(EXT_STA)
extern void wl_ndis_extsta_register_events(char *eventmask);
#endif /* EXT_STA */
#endif /* UNDER_CE */
#ifndef UNDER_CE
static int dhd_os_acquire_mutex(PKMUTEX mutex);
static int dhd_os_release_mutex(PKMUTEX mutex);
#endif /* UNDER_CE */

#if defined(SIMPLE_ISCAN)
#if defined(BCMDONGLEHOST) && defined(IL_BIGENDIAN)
#include <bcmendian.h>
#define htod32(i) (bcmswap32(i))
#define htod16(i) (bcmswap16(i))
#define dtoh32(i) (bcmswap32(i))
#define dtoh16(i) (bcmswap16(i))
#define htodchanspec(i) htod16(i)
#define dtohchanspec(i) dtoh16(i)
#else
#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i
#define htodchanspec(i) i
#define dtohchanspec(i) i
#endif /* BCMDONGLEHOST && IL_BIGENDINA */
extern void wl_ind_scan_confirm(wl_info_t *wl, NDIS_STATUS StatusBuffer);
extern iscan_buf_t *dhd_iscan_result_buf(void);
extern void wl_flush_bss_lists(wl_info_t *wl);
#endif /* SIMPLE_ISCAN */
/* IOCTL response timeout */
unsigned long dhd_ioctl_timeout_msec = IOCTL_RESP_TIMEOUT;

#ifdef DHD_DEBUG
/* Console poll interval */
/* XXX: if other than 0 is used, it screws up the current measurements, so set to 0 default */
/* XXX: 250ms is a suggested alternate value */
uint dhd_console_ms = 0;
#endif /* DHD_DEBUG */

/* The following are specific to the SDIO dongle */

#if defined(BCMSDIO) && !defined(BCMDBUS)
/* Idle timeout for backplane clock */
int dhd_idletime = DHD_IDLETIME_TICKS;
#endif /* BCMSDIO && !BCMDBUS */

#if (defined(BCMSDIO) && !defined(BCMDBUS)) || defined(BCMDHDUSB)
/* Watchdog interval */
#if defined(UNDER_CE)
#define DEFAULT_WATCHDOG_TIMER_INTERVAL_BATTERY 10
#define DEFAULT_WATCHDOG_TIMER_INTERVAL_AC      250

uint dhd_watchdog_battery_ms = DEFAULT_WATCHDOG_TIMER_INTERVAL_BATTERY;
uint dhd_watchdog_ac_ms = DEFAULT_WATCHDOG_TIMER_INTERVAL_AC;
/* Set to battery interval by default */
uint dhd_watchdog_ms = DEFAULT_WATCHDOG_TIMER_INTERVAL_BATTERY;
#else
uint dhd_watchdog_ms = 250;
#endif /* UNDER_CE */
#endif /* (BCMSDIO && !BCMDBUS) || BCMDHDUSB */

uint mfgtest_mode = 0; /* Test mode flag to determine if mfgtest firmware should be used */

#if !defined(BCMDBUS) || defined(BCMSDIO)

/* Pkt filter mode control */
uint dhd_master_mode = TRUE;

/* Pkt filte enable control */
uint dhd_pkt_filter_enable = FALSE;

/* ARP offload agent mode */
uint dhd_arp_mode = 0;

/* ARP offload enable */
uint dhd_arp_enable = FALSE;

#endif /* !defined(BCMDBUS) || defined(BCMSDIO) */

#if !defined(BCMDBUS)

/* Use polling */
uint dhd_poll = FALSE;

/* Use interrupts */
uint dhd_intr = TRUE;

/*  Pkt filter init setup */
uint dhd_pkt_filter_init = 1;

/* Contorl fw roaming. 0 enables roam, 1 disables roam */
uint dhd_roam_disable = 0;

/* Control radio state */
uint dhd_radio_up = 0;

/* SDIO Drive Strength (in milliamps) */
uint dhd_sdiod_drive_strength = 6;
#endif /* !defined(BCMDBUS) */

uint wl_msg_level = DHD_ERROR_VAL;
uint wl_msg_level2 = 0;

#ifdef BCMSDIO
/* Tx/Rx bounds */
extern uint dhd_txbound;
extern uint dhd_rxbound;
#endif

#if defined(BCMSDIO) && !defined(BCMDBUS)
/* Small race window between isr()/dpc() and mhalt() so need to
 * use global for now until we get a better handle on this; Maybe
 * rework the isr()/dpc() similarly to NIC.
 */
static bool g_in_dpc = FALSE;
static bool g_mhalted = FALSE;
#endif /* BCMSDIO */

#ifdef BCMSLTGT
uint htclkratio = 10;
#endif

#ifndef BCMDBUS
#ifdef SDTEST
/* Echo packet generator (pkts/s) */
uint dhd_pktgen = 0;

/* Echo packet len (0 => sawtooth, max 2040) */
uint dhd_pktgen_len = 0;
#endif
#endif /* !BCMDBUS */

/* Version string to report */
#ifdef BCMDBG
#define DHD_COMPILED "\nCompiled in " SRCBASE
#else
#define DHD_COMPILED
#endif

#ifdef BCMDBUS
#define dhd_bus_pub(bus)	(dhd_pub_t *)(bus)
#define DBUS_NRXQ		256
#define DBUS_NTXQ		256
#ifdef EXT_STA
/* FIX: Vista apparently needs extra headroom */
#define EXTRA_HR      16
#else
#define EXTRA_HR      0
#endif /* EXT_STA */

static void
dhd_dbus_send_complete(void *handle, void *info, int status)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;
	wl_info_t *wl;
	void *pkt;

	if (dhd == NULL)
		return;

	if (status == DBUS_OK) {
		dhd->pub.dstats.tx_packets++;
	} else {
		DHD_ERROR(("TX error=%d\n", status));
		dhd->pub.dstats.tx_errors++;
	}

	pkt = info;
	if (pkt) {
		wl = dhd->wl;

		if (status == DBUS_OK) {
			ND_TXLOCK(wl);
			PKTFREE(dhd->pub.osh, pkt, TRUE);
			ND_TXUNLOCK(wl);
		} else {
#ifndef NDIS60
			ND_TXLOCK(wl);
			PKTFREE(dhd->pub.osh, pkt, TRUE);
			ND_TXUNLOCK(wl);
#else
			PKTFREE(dhd->pub.osh, pkt, TRUE);
#endif
		}
	}

	if (status == DBUS_OK)
		wl_sendcomplete(dhd->wl);
	else
		DHD_ERROR(("tx_complete: error=0x%x\n", status));
}

static void
dhd_dbus_pkt_recv(void *handle, void *pkt)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;
	dhd_pub_t *dhdp;
	wl_info_t *wl;
#ifdef NDIS60
	async_req_t *free_req;
#endif /* NDIS60 */
	int ifidx = 0;

	if (dhd == NULL)
		return;

	wl = dhd->wl;
	if (wl == NULL)
		return;

	dhdp = &dhd->pub;

	/* Pull BDC header head so it's common to both XP and Vista */
	if (dhd_prot_hdrpull(dhdp, &ifidx, pkt) != 0) {
		ND_RXLOCK(wl);
		PKTFREE(dhdp->osh, pkt, FALSE);
		ND_RXUNLOCK(wl);
		return;
	}

#ifndef NDIS60
	dhd_rx_frame(dhdp, ifidx, pkt, 1, 0);
#else
	if (IsListEmpty(&dhd->asyncFreeReqList)) {
		ND_RXLOCK(wl);
		PKTFREE(dhd->pub.osh, pkt, FALSE);
		ND_RXUNLOCK(wl);
		return;
	} else {
		free_req = (async_req_t *) NdisInterlockedRemoveHeadList(
			&dhd->asyncFreeReqList,
			&dhd->asyncFreeReqLock);

		ASSERT(free_req);
	}

	bzero(free_req, sizeof(async_req_t));
	free_req->cb = dhd_rxpkt_cb;
	free_req->arg = dhd;
	free_req->ifidx = ifidx;
	free_req->parms.rxpkt.pkt = pkt;

	dhd_async_req_schedule(wl, free_req);
#endif /* NDIS60 */
	return;
}

static void
dhd_dbus_buf_recv(void *handle, uint8 *buf, int len)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;
	dhd_pub_t *dhdp;
	wl_info_t *wl;
	osl_t *osh;
	int extra_hr = EXTRA_HR;
	void *pkt;

	if (dhd == NULL)
		return;

	wl = dhd->wl;
	if (wl == NULL)
		return;

	dhdp = &dhd->pub;
	osh = dhdp->osh;

	ASSERT(len < 1600);
	ND_RXLOCK(wl);
	if (!(pkt = PKTGET(osh, len + extra_hr, FALSE))) {
		/* Give up on data, request rtx of events */
		DHD_ERROR(("%s: PKTGET failed: len %d \n", __FUNCTION__, len));
		ND_RXUNLOCK(wl);
		goto err;
	}
	ND_RXUNLOCK(wl);

#ifdef EXT_STA
	/* reserve some room for Windows upper layer */
	PKTPULL(osh, pkt, extra_hr);
#endif /* EXT_STA */

	bcopy(buf, PKTDATA(osh, pkt), len);

	PKTSETLEN(osh, pkt, len);

	dhd_dbus_pkt_recv(dhd, pkt);
err:
	return;
}

static void
dhd_dbus_txflowcontrol(void *handle, bool onoff)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;

	if (dhd == NULL)
		return;

	dhd_txflowcontrol(&dhd->pub, 0, onoff);
}
static void
dhd_dbus_errhandler(void *handle, int err)
{
}

static void
dhd_dbus_ctl_complete(void *handle, int type, int status)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;

	if (dhd == NULL)
		return;

	if (type == DBUS_CBCTL_READ) {
		if (status == DBUS_OK)
			dhd->pub.rx_ctlpkts++;
		else
			dhd->pub.rx_ctlerrs++;
	} else if (type == DBUS_CBCTL_WRITE) {
		if (status == DBUS_OK)
			dhd->pub.tx_ctlpkts++;
		else
			dhd->pub.tx_ctlerrs++;
	}

	dhd_prot_ctl_complete(&dhd->pub);
}

static void
dhd_dbus_state_change(void *handle, int state)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;
	wl_info_t *wl;

	if (dhd == NULL)
		return;

	wl = (wl_info_t *)dhd->wl;

	if (state == DBUS_STATE_DOWN) {
		DHD_ERROR(("%s: DBUS is down\n", __FUNCTION__));
		dhd->pub.busstate = DHD_BUS_DOWN;
	} else if (state == DBUS_STATE_DISCONNECT) {
		DHD_ERROR(("%s: DBUS is disconnected! \n", __FUNCTION__));
		dhd->pub.busstate = DHD_BUS_DOWN;
	} else if (state == DBUS_STATE_PNP_FWDL) {
		DHD_ERROR(("%s: DBUS firmware re-downloaded\n", __FUNCTION__));
	} else {
		DHD_ERROR(("%s: DBUS current state=%d\n", __FUNCTION__, state));
	}
}

static void *
dhd_dbus_pktget(void *handle, uint len, bool send)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;
	wl_info_t *wl;
	void *p = NULL;

	if (dhd == NULL)
		return NULL;

	wl = (wl_info_t *)dhd->wl;

	if (send == TRUE) {
		ND_TXLOCK(wl);
		p = PKTGET(dhd->pub.osh, len, TRUE);
		ND_TXUNLOCK(wl);
	} else {
		ND_RXLOCK(wl);
		p = PKTGET(dhd->pub.osh, len, FALSE);
		ND_RXUNLOCK(wl);
	}

	return p;
}

static void
dhd_dbus_pktfree(void *handle, void *p, bool send)
{
	dhd_info_t *dhd = (dhd_info_t*) handle;
	wl_info_t *wl;

	if (dhd == NULL)
		return;

	wl = (wl_info_t *)dhd->wl;

	if (send == TRUE) {
		ND_TXLOCK(wl);
		PKTFREE(dhd->pub.osh, p, TRUE);
		ND_TXUNLOCK(wl);

		/* wl_mhalt() is called only after all NDIS pkts are completed
		 * DBUS calls send_complete() then calls dhd_dbus_pktfree()
		 * so complete any pending pkts during this stage as well.
		 */
		wl_sendcomplete(wl);
	} else {
		ND_RXLOCK(wl);
		PKTFREE(dhd->pub.osh, p, FALSE);
		ND_RXUNLOCK(wl);
	}
}


static dbus_callbacks_t dhd_dbus_cbs = {
	dhd_dbus_send_complete,
	dhd_dbus_buf_recv,
	dhd_dbus_pkt_recv,
	dhd_dbus_txflowcontrol,
	dhd_dbus_errhandler,
	dhd_dbus_ctl_complete,
	dhd_dbus_state_change,
	dhd_dbus_pktget,
	dhd_dbus_pktfree
};

void *
dhd_dbus_probe_cb(void *arg, const char *desc, uint32 bustype, uint32 hdrlen)
{
	wl_info_t *wl = arg;
	shared_info_t *sh = &wl->sh;
	osl_t *osh = sh->osh;
	int ret = 0;
	dbus_attrib_t attrib;
	dhd_pub_t *pub = NULL;
	int async_req_init = FALSE;

	DHD_ERROR(("%s: Enter osh=0x%p\n", __FUNCTION__, osh));

	/* Attach to the dhd/OS interface */
	if (!(pub = dhd_attach(osh, NULL /* bus */, hdrlen))) {
		DHD_ERROR(("%s: dhd_attach failed\n", __FUNCTION__));
		goto fail;
	}

	/* FIX: not great way to pass back dhd handle to wl
	 */
	wl->dhd = pub;

#ifdef NDIS60
	pub->wdfDevice = sh->wdfdevice;

	/* FIX: Redo this when we have common async serialization
	 * between NIC and DHD
	 */
	if (dhd_async_req_init(pub)) {
		DHD_ERROR(("dhd_async_req_init failed\n"));
		goto fail;
	}
	async_req_init = TRUE;
#endif /* NDIS60 */

	/* FIX: use pub->rxz */
	pub->dbus = dbus_attach(osh, 1600, DBUS_NRXQ, DBUS_NTXQ, pub, &dhd_dbus_cbs, sh);
	if (pub->dbus) {
		dbus_get_attrib(pub->dbus, &attrib);
		DHD_ERROR(("DBUS: vid=0x%x pid=0x%x devid=0x%x bustype=0x%x mtu=%d\n",
			attrib.vid, attrib.pid, attrib.devid, attrib.bustype, attrib.mtu));
	} else {
		DHD_ERROR(("DBUS attach failed \n"));
		ret = -1;
		goto fail;
	}

	OSL_DMADDRWIDTH(sh->osh, 32);
	sh->attached = TRUE;

	if (dhd_open((dhd_info_t *) pub, wl)) {
		DHD_ERROR(("%s: dhd_open failed\n", __FUNCTION__));
		goto fail;
	}

	return pub;
fail:
	if (pub) {
		if (pub->dbus) {
			dbus_detach(pub->dbus);
			pub->dbus = NULL;
		}
#ifdef NDIS60
		if (async_req_init == TRUE)
			dhd_async_req_deinit(pub);
#endif
		dhd_detach(pub);
		dhd_free(pub);
	}

	wl->dhd = NULL;
	return NULL;
}

void
dhd_dbus_disconnect_cb(void *arg)
{
	dhd_info_t *dhd = (dhd_info_t*) arg;
	int err;
	wl_info_t *wl;
	dhd_pub_t *pub;
	uint down = 1;

	if (dhd == NULL)
		return;

	wl = dhd->wl;
	pub = &dhd->pub;

	wl_ioctl(wl, WLC_DOWN, (char *)&down, sizeof(down));
#if defined(BCMDHDUSB)
	if (wl->dnglResetAfterDisable) {
		err = wl_reboot(wl);
		if (err) {
			DHD_ERROR(("dngl reboot failed 0x%x\n", err));
		}
	}
#endif

	dhd_stop(dhd);

	if (pub->dbus) {
		dbus_detach(pub->dbus);
		pub->dbus = NULL;
	}

#ifdef NDIS60
	dhd_async_req_deinit(pub);
#endif
	dhd_detach(pub);
	dhd_free(pub);

}
#endif /* BCMDBUS */

#define dhd_bus_dhd(bus) (dhd_info_t *)dhd_bus_pub(bus)

#if defined(NDIS60) && !defined(UNDER_CE)

int
dhd_async_req_schedule(wl_info_t *wl, async_req_t *req)
{
	dhd_info_t *dhd = dhd_bus_dhd(wl->dhd);
	if (!req)
		return -1;

	NdisInitializeListHead(&req->list);
	NdisInterlockedInsertTailList(&dhd->asyncReqList,
		&req->list,
		&dhd->asyncReqLock);

	NdisAcquireSpinLock(&dhd->asyncReqLock);
	/* Do not enqueue the same work item if it's busy */
	if (InterlockedCompareExchange((PULONG) &dhd->asyncReqWorkItemState,
		WORKITEM_STATE_BUSY, WORKITEM_STATE_FREE) == WORKITEM_STATE_FREE) {

		if (dhd->pub.busstate != DHD_BUS_DOWN)
		WdfWorkItemEnqueue(dhd->asyncReqWorkItem);
		else
			DHD_ERROR(("DHD is down...skip enqueue\n"));
	} else {
		WL_TRACE(("Async Work Item is BUSY!!! \n"));
	}
	NdisReleaseSpinLock(&dhd->asyncReqLock);

	return 0;
}

VOID
dhd_async_req_workitem
#ifndef NDIS60
	(PNDIS_WORK_ITEM workItem, PVOID context)
#else /* !NDIS60 */
	(WDFWORKITEM workItem)
#endif
{
	dhd_info_t *dhd;
	uint32 result;
	osl_t *osh;
	async_req_t *req;
#ifndef NDIS60
	dhd = (dhd_info_t *)context;
#else
	dhd_workitem_context_t *dhd_context;

	dhd_context = dhd_get_dhd_workitem_context(workItem);
	dhd = (dhd_info_t *) dhd_context->dhd_pub;
#endif

	ASSERT(dhd);
	osh = dhd->pub.osh;

	while ((!IsListEmpty(&dhd->asyncReqList)) && (dhd->pub.busstate != DHD_BUS_DOWN)) {
		req = (async_req_t *) NdisInterlockedRemoveHeadList(
			&dhd->asyncReqList, &dhd->asyncReqLock);
		if (!req)
			break;

		ASSERT(req->cb);
		req->cb(req, req->arg);
	}

	result = InterlockedExchange((PLONG)&dhd->asyncReqWorkItemState,
		WORKITEM_STATE_FREE);
	ASSERT(result == WORKITEM_STATE_BUSY);

}

/* This initializes a queue to serialize bus access.
 * OIDs and events from dongle can simultaneously access the bus.
 * This mechanism serializes both and any other that needs bus access.
 * This can also be used as a generic mechanism for asynchronous
 * execution at PASSIVE_LEVEL.
 */
static int
dhd_async_req_init(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *) dhdp;
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	NTSTATUS ntStatus;
	WDF_WORKITEM_CONFIG workitemConfig;
	WDF_OBJECT_ATTRIBUTES attributes;
	dhd_workitem_context_t *dhd_context;
	async_req_t *free_req;
	int i;

	if (dhd == NULL)
		goto fail;

	dhd->asyncReqWorkItemState = WORKITEM_STATE_FREE;
	NdisInitializeListHead(&dhd->asyncReqList);
	NdisAllocateSpinLock(&dhd->asyncReqLock);
	NdisInitializeListHead(&dhd->asyncFreeReqList);
	NdisAllocateSpinLock(&dhd->asyncFreeReqLock);

	for (i = 0; i < DBUS_NRXQ; i++) {
		free_req = MALLOC(dhd->pub.osh, sizeof(async_req_t));

		if (!free_req) {
			DHD_ERROR(("Malloc failed for req i=%d \n", i));
			break;
		}
		NdisInitializeListHead(&free_req->list);
		NdisInterlockedInsertTailList(&dhd->asyncFreeReqList,
			&free_req->list,
			&dhd->asyncFreeReqLock);
	}

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, dhd_workitem_context_t);

	attributes.ParentObject = dhd->pub.wdfDevice;

	WDF_WORKITEM_CONFIG_INIT(&workitemConfig, dhd_async_req_workitem);

	ntStatus = WdfWorkItemCreate(&workitemConfig,
		&attributes,
		&dhd->asyncReqWorkItem);

	if (!NT_SUCCESS(ntStatus)) {
		DHD_ERROR(("WdfWorkItemCreate failed for asyncReqWorkItem 0x%x\n", ntStatus));
		goto fail;
	}

	dhd_context = dhd_get_dhd_workitem_context(dhd->asyncReqWorkItem);
	dhd_context->dhd_pub = &dhd->pub;

	return 0;

fail:
	/* FIX: MALLOC is unrolled in dhd_async_req_deinit */
	return -1;
}

static int
dhd_async_req_deinit(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *) dhdp;
	async_req_t *req;
	async_req_t *free_req;

	if (dhd == NULL)
		return -1;

	if (dhd->asyncReqWorkItem)
		WdfWorkItemFlush(dhd->asyncReqWorkItem);

	while (!IsListEmpty(&dhd->asyncReqList)) {
		req = (async_req_t *) NdisInterlockedRemoveHeadList(
			&dhd->asyncReqList,
			&dhd->asyncReqLock);
		if (!req)
			break;

		ASSERT(req->cb);
		req->cb(req, req->arg);
	}

	while (!IsListEmpty(&dhd->asyncFreeReqList)) {
		free_req = (async_req_t *) NdisInterlockedRemoveHeadList(
			&dhd->asyncFreeReqList,
			&dhd->asyncFreeReqLock);

		ASSERT(free_req);
		MFREE(dhd->pub.osh, free_req, sizeof(async_req_t));
	}

	NdisFreeSpinLock(&dhd->asyncReqLock);
	NdisFreeSpinLock(&dhd->asyncFreeReqLock);

	return 0;
}
#endif /* NDIS60 && !UNDER_CE */

void
dhd_isr(bool *InterruptRecognized, bool *QueueMiniportHandleInterrupt, void *arg)
{
#ifdef UNDER_CE
	/* Dynamically adjust current priority and restore */
#if !defined(NDIS60)
	int ori_prio;
#endif
	dhd_info_t *dhd_info = dhd_bus_dhd((struct dhd_bus *)arg);
	wl_info_t *wl = dhd_info->wl;
	*QueueMiniportHandleInterrupt = FALSE;
#else
	*QueueMiniportHandleInterrupt = TRUE;
#endif /* UNDER_CE */
	*InterruptRecognized = TRUE;

#if defined(BCMSDIO) && !defined(BCMDBUS)
	dhdsdio_isr(arg);

	if (g_mhalted == TRUE)
		*QueueMiniportHandleInterrupt = FALSE;
#endif /* BCMSDIO */
#ifdef UNDER_CE
	/*
	 * Call the DPC within the ISR in case of CE to prevent
	 * MSFT SDIO stack from doing few extra SDIO access to
	 * read interrupt status and enable/disable interrupts
	 * and also to avoid the scheduling delay
	 */
#if !defined(NDIS60)
	ori_prio = CeGetThreadPriority(GetCurrentThread());
	CeSetThreadPriority(GetCurrentThread(), wl->sh.rxdpc_prio);
#endif
	dhd_dpc(dhd_info);
#if !defined(NDIS60)
	CeSetThreadPriority(GetCurrentThread(), ori_prio);
#endif
#endif /* UNDER_CE */
}

#ifdef UNDER_CE
#if defined(NDIS60)
uint
dhd_rxflow_mode(dhd_pub_t *dhdp, uint mode, bool set)
{
	return 0;
}
void
dhd_rx_flow(void *dhdp, bool enable)
{
}

#else
uint
dhd_rxflow_mode(dhd_pub_t *dhdp, uint mode, bool set)
{
	dhd_info_t *dhd_info = (dhd_info_t *) dhdp;
	wl_info_t *wl = (wl_info_t *)dhd_info->wl;
	if (set)
		wl->sh.rxflowMode = mode;
	return wl->sh.rxflowMode;
}

void
dhd_rx_flow(void *dhdp, bool enable)
{
	dhd_info_t *dhd_info = (dhd_info_t *) dhdp;
	wl_info_t *wl = (wl_info_t *)dhd_info->wl;

	if (enable) {
		dhd_thread_priority(dhdp, (uint)DHD_RXDPC,
			&(wl->sh.RxflowRxDPCPriority), TRUE);
		dhd_thread_priority(dhdp, DHD_SENDUP,
			&(wl->sh.RxflowRXThreadPriority), TRUE);
	} else {
		dhd_thread_priority(dhdp, DHD_RXDPC,
			&(wl->sh.RxDPCPriority), TRUE);
		dhd_thread_priority(dhdp, DHD_SENDUP,
			&(wl->sh.RXThreadPriority), TRUE);
	}
}
#endif /* NDIS60 */

void
dhd_sendup_indicate(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd_info = (dhd_info_t *) dhdp;
	/* Signal the sendup thread */
	NdisSetEvent(&dhd_info->sendup_event);
}

static void
dhd_sendup(void *h)
{
	dhd_info_t *dhd = (dhd_info_t *)h;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	/* Read the priority from registry */
	CeSetThreadPriority(GetCurrentThread(), wl->sh.sendup_prio);
	DHD_ERROR(("RX thread priority = %d\n", wl->sh.sendup_prio));

	while (TRUE) {
		NdisWaitEvent(&dhd->sendup_event, 0);	/* wait forever */
		NdisResetEvent(&dhd->sendup_event);		/* reset the event */
		DHD_TRACE(("%s: Enter\n", __FUNCTION__));
		dhd_sendup_flush((dhd_pub_t *)h);
	}
}

static void
dhd_sendup_flush(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd_info = (dhd_info_t *) dhdp;
	uint prec_out, num;
	void *pkt, *start_pkt, *pnext;

	num = prec_out = 0;
	pkt = start_pkt = pnext = NULL;

	dhd_os_sdlock_sndup_rxq(dhdp);

	start_pkt = pkt = pktq_deq(&dhd_info->rxq, &prec_out);

	while (pkt) {
		pnext = pktq_deq(&dhd_info->rxq, &prec_out);
		num++;

		if (!pnext)
			break;

		PKTSETNEXT(wl->sh.osh, pkt, pnext);
		pkt = pnext;
	}
	dhd_os_sdunlock_sndup_rxq(dhdp);

	if (num)
		wl_sendup(dhd_info->wl, NULL, start_pkt, num);
}

uint32
dhd_rx_flow_hilimit(dhd_pub_t *dhdp, uint32 val)
{
	dhd_info_t *dhd = (dhd_info_t *) dhdp;
	wl_info_t *wl = dhd->wl;

	if (val)
		wl->sh.rxflowHi = val;

	return wl->sh.rxflowHi;
}

uint32
dhd_rx_flow_lowlimit(dhd_pub_t *dhdp, uint32 val)
{
	dhd_info_t *dhd = (dhd_info_t *) dhdp;
	wl_info_t *wl = dhd->wl;

	if (val)
		wl->sh.rxflowLow = val;

	return wl->sh.rxflowLow;
}

void
dhd_thread_priority(dhd_pub_t *dhdp, uint thread, uint *prio, bool set)
{
	dhd_info_t *dhd = (dhd_info_t *) dhdp;
	wl_info_t *wl = dhd->wl;
	if (set) {
		if ((*prio >= CE_THREAD_PRIORITY_MIN) && (*prio <= CE_THREAD_PRIORITY_MAX)) {
			switch (thread) {
				case (DHD_DPC):
					wl->sh.dpc_prio = *prio;
					CeSetThreadPriority(wl->sh.dpc_handle,
						wl->sh.dpc_prio);
					break;
				case (DHD_RXDPC):
					wl->sh.rxdpc_prio = *prio;
					break;
				case (DHD_SENDUP):
					wl->sh.sendup_prio = *prio;
					CeSetThreadPriority(dhd->sendup_handle,
						wl->sh.sendup_prio);
					break;
				case (DHD_WATCHDOG):
					wl->sh.wd_prio = *prio;
					break;
			}
		}
	} else {
		switch (thread) {
			case (DHD_DPC):
				*prio = wl->sh.dpc_prio;
				break;
			case (DHD_RXDPC):
				*prio = wl->sh.rxdpc_prio;
				break;
			case (DHD_SENDUP):
				*prio  = wl->sh.sendup_prio;
				break;
			case (DHD_WATCHDOG):
				*prio = wl->sh.wd_prio;
				break;
		}
	}
}

void
dhd_thread_quantum(dhd_pub_t *dhdp, uint thread, uint *tq, bool set)
{
	dhd_info_t *dhd = (dhd_info_t *) dhdp;
	wl_info_t *wl = dhd->wl;
	if (set) {
		switch (thread) {
			case (DHD_DPC):
				if (!CeSetThreadQuantum(wl->sh.dpc_handle, (DWORD)*tq))
					DHD_ERROR(("%s: set dpc failed\n", __FUNCTION__));
				break;
			case (DHD_SENDUP):
				if (!CeSetThreadQuantum(dhd->sendup_handle, (DWORD)*tq))
					DHD_ERROR(("%s: set sendup failed\n", __FUNCTION__));
				break;
		}
	} else {
		switch (thread) {
			case (DHD_DPC):
				*tq = (uint) CeGetThreadQuantum(wl->sh.dpc_handle);
				break;
			case (DHD_SENDUP):
				*tq = (uint) CeGetThreadQuantum(dhd->sendup_handle);
				break;
		}
	}
}
#endif /* UNDER_CE */

int
dhd_probe(shared_info_t *sh, void *wl, void* bar, void **regsva, uint irq, void **dhd)
{
	NDIS_STATUS status;
	int ret = 0;
	void *bus;
	dhd_info_t *dhd_info;

	if (sh == NULL)
		goto err;

#if defined(BCMSDIO) && !defined(BCMDBUS)
	g_in_dpc = FALSE;
	g_mhalted = FALSE;
#endif

#ifndef BCMDBUS
	/* Assign drive strength read from registry before calling bcmsdh_probe */
	if (sh->DrvStrength)
		dhd_sdiod_drive_strength = sh->DrvStrength;

	dhd_poll = sh->EnablePoll;
	dhd_intr = !dhd_poll;
	DHD_ERROR(("Driver is operating in %s mode\n", dhd_intr ? "Interrupt" : "Poll"));

	ret = dhd_bus_register();
	if (ret == -1)
		goto err;

#if defined(BCMSDIO)
	ret = bcmsdh_probe(sh, bar, regsva, irq, dhd, wl);
#endif /* BCMSDIO */
	if (ret == -1)
		goto err;

	bus = *dhd;
	dhd_info = dhd_bus_dhd(bus);

#ifndef NDIS60
#ifdef UNDER_CE
	NdisInitializeEvent(&sh->dpc_event);
#endif
	/* register our ISR */
	status = shared_interrupt_register(0, 0,
		0, 0,
		0,
		0,
		0,
		sh, dhd_isr, bus, dhd_dpc, dhd_info);

	if (NDIS_ERROR(status)) {
		DHD_ERROR(("wl%d: NdisMRegisterInterrupt error 0x%x\n", sh->unit, status));
		goto err;
	}
#else
#ifdef UNDER_CE
	status = shared_interrupt_register(0, sh, dhd_isr, bus, dhd_dpc, dhd_info);
#endif /* UNDER_CE */
#endif /* !NDIS60 */

	sh->attached = TRUE;
#ifdef BCMSDIO
	sh->dhd = dhd_info;
#endif /* BCMSDIO */
	dhd_info->pub.info = dhd_info;

	if (dhd_open(dhd_info, (wl_info_t *)wl)) {
		DHD_ERROR(("%s: dhd_open failed\n", __FUNCTION__));
		ret = -1;
		goto err;
	}
#endif /* !BCMDBUS */

err:
	return ret;
}

void
dhd_remove(osl_t *osh, void *instance, shared_info_t *sh)
{
#if defined(BCMSDIO) && !defined(BCMDBUS)
	g_mhalted = TRUE;

#ifndef NDIS60
	/* Do this asap because we don't want isr to fire */
	shared_interrupt_deregister(sh->intr, sh);
#endif

	/* XXX dpc() is scheduled from QueueMiniportHandleInterrupt and
	 * XXX shared_dpc_schedule().  NDIS can bring down the driver
	 * XXX while dpc() is active so need to wait til dpc() completes
	 * XXX before free'ing resources via dhd_detach(), etc.
	 * XXX Is DHD dpc() operation very different from NIC?
	 */
	if (g_in_dpc == TRUE) {
		NdisMSleep(1000 * 1000); /* 1sec */
	}
	bcmsdh_remove(osh, instance, sh);
	dhd_bus_unregister();
#endif /* BCMSDIO && !BCMDBUS */
}

#if defined(NDIS51) || defined(NDIS60)
void
dhd_pnp_surp_remove(void *instance)
{
#ifdef BCMDBUS
	dhd_pub_t *dhdp = dhd_bus_pub(instance);
	dbus_pnp_disconnect(dhdp->dbus);
#endif
}
#endif /* NDIS51 || NDIS60  */

char *
dhd_ifname(dhd_pub_t *dhdp, int ifidx)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp;

	return "<no ifname>";
}

int
dhd_sendpkt(dhd_pub_t *dhdp, int ifidx, void *pktbuf)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp;
	int ret = 0;
	osl_t *osh = dhdp->osh;
	wl_info_t *wl = dhd->wl;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* Reject if down */
	if (!dhdp->up || (dhdp->busstate == DHD_BUS_DOWN)) {
		/* This can happen after PnP surprise remove event */
		dhd_txcomplete(dhdp, pktbuf, FALSE);
		PKTFREE(osh, pktbuf, TRUE);
		ND_TXUNLOCK(wl);
		wl_sendcomplete(wl);
		ND_TXLOCK(wl);
		return -BCME_NOTUP;
	}

	/* If the protocol uses a data header, apply it */
	dhd_prot_hdrpush(dhdp, ifidx, pktbuf);

	/* Use bus module to send data frame */
#ifdef BCMDBUS
#ifdef BCMSDIO
	ret = dbus_send_pkt(dhdp->dbus, pktbuf, NULL /* pktinfo */);
#else
	ret = dbus_send_buf(dhdp->dbus, PKTDATA(osh, pktbuf),
		PKTLEN(osh, pktbuf), pktbuf /* pktinfo */);
#endif
	if (ret == DBUS_ERR_TXFAIL) {
		dhd_txcomplete(dhdp, pktbuf, FALSE);
		PKTFREE(osh, pktbuf, TRUE);
		return -1;
	}
#else
	ret = dhd_bus_txdata(dhdp->bus, pktbuf);

#ifndef NDIS60
	/* Since the TX is queued and really transmitted from DPC,
	   schedule DPC manually to start sending the packets
	*/
	shared_dpc_schedule(&wl->sh);
#endif
#endif /* BCMDBUS */

	/* XXX Bus modules may have different "native" error spaces? */
	/* XXX USB is native linux and it'd be nice to retain errno  */
	/* XXX meaning, but SDIO is not so we'd need an OSL_ERROR.   */
	if (ret)
		dhdp->dstats.tx_dropped++;
	else
		dhdp->tx_packets++;

	/* Return ok: we always eat the packet */
	return ret;
}

/* Enter with TXLOCK held */
int
dhd_start_xmit(void *bus, void *pktbuf)
{
	dhd_pub_t *dhdp = dhd_bus_pub(bus);

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	return dhd_sendpkt(dhdp, 0, pktbuf);
}

void
dhd_event(struct dhd_info *dhd, char *evpkt, int evlen, int ifidx)
{
	dhd_if_t *ifp;

	DHD_TRACE(("dhd_event: idx %d\n", ifidx));

#if 0
	wlsavetrace(wlgettrace() + 9999);
#endif

	if (ifidx != 0 && !(ifp = dhd->iflist[ifidx]))
		return;

#if 0
	wlsavetrace(wlgettrace() + 1000);
#endif

	dhd_vif_sendup(dhd, ifidx, evpkt, evlen);
}

void
dhd_vif_sendup(struct dhd_info *dhd, int ifidx, uchar *cp, int len)
{
#if 0
	dhd_if_t *ifp;

	wlsavetrace(wlgettrace() + 100);
	if (!(ifp = dhd->iflist[ifidx]))
		return;
#endif
/* XXX : for Out of driver supplicant only, not needed for WP7S yet */
#if defined(UNDER_CE) && !defined(NDIS60)
	DHD_TRACE(("dhd_vif_sendup: sending up %d bytes on ifidx %d\n", len, ifidx));

	/* XXX: how does big windows sendup ? */
	/* "Well Known" queue names */
	if (ifidx == 0) {
		MsgQueueSend((HANDLE)dhd_sendup_info[0].msgqueue_handle, (LPVOID) cp, len);
	} else {
		/* Need to add info for disambiguation when we have multiple vifs */
		MsgQueueSend((HANDLE)dhd_sendup_info[1].msgqueue_handle, (LPVOID) cp, len);
	}
#endif /* UNDER_CE && !NDIS60 */
}

void
dhd_txflowcontrol(dhd_pub_t *dhdp, int ifidx, bool state)
{
	dhd_info_t *dhd_info = dhdp->info;

	DHD_TRACE(("Flow control changed = %d for idx %d\n", state, ifidx));

	dhdp->txoff = state;

	wl_txflowcontrol((wl_info_t *)dhd_info->wl, NULL, state, ALLPRIO);
}

void
dhd_rx_frame(dhd_pub_t *dhdp, int ifidx, void *pktbuf, int numpkt, uint8 chan)
{
	struct ether_header *eh;
	uint16 type;
	wl_event_msg_t event;
	void *data, *pnext;
	wl_info_t *wl;
	uint len;
	int i;
	dhd_info_t *dhd_info;
	int bcmerror;
#if defined(EXT_STA) && defined(UNDER_CE)
	uchar *pkt_ptr = NULL;
#endif /* EXT_STA && UNDER_CE */
#ifdef DHD_NDIS_OID
	wlc_event_t wlc_e;
#endif /* DHD_NDIS_OID */

	dhd_info =  (dhd_info_t *)dhdp;
	wl = dhd_info->wl;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	for (i = 0; pktbuf && i < numpkt; i++, pktbuf = pnext) {
		pnext = PKTNEXT(dhdp->osh, pktbuf);
		PKTSETNEXT(dhdp->osh, pktbuf, NULL);

		len = PKTLEN(dhdp->osh, pktbuf);

		eh = (struct ether_header *)PKTDATA(dhdp->osh, pktbuf);
		type  = ntoh16(eh->ether_type);

		/* Get the protocol, maintain skb around eth_type_trans() */
		/* Process special event packets and then discard them */
		/* XXX Decide on a better way to fit this in */
#if defined(UNDER_CE) && defined(NDIS60)
		if (chan == SDPCM_EVENT_CHANNEL) {
			DHD_TRACE(("%s: queueing pkt\n", __FUNCTION__));
			/* Place the received packet into queue for later delivery to NDIS stack */
			/* Priority is not used, choose a fixed value so packets stay in order */
			if (ifidx == 0) {
				dhd_os_sdlock_eventq(dhdp);
				if (!pktq_full(&dhd_info->eventq)) {
					DHD_TRACE(("%s: ENQ PKT DATA %x \n", __FUNCTION__, pktbuf));
					pktq_penq(&dhd_info->eventq, PRIOMASK, pktbuf);
					dhd_os_sdunlock_eventq(dhdp);
					NdisSetEvent(&dhd_info->host_event);
					continue;
				} else {
					dhd_os_sdunlock_eventq(dhdp);
					DHD_ERROR(("%s: out of event queue\n", __FUNCTION__));
					ND_RXLOCK(wl);
					PKTFREE(dhdp->osh, pktbuf, FALSE);
					ND_RXUNLOCK(wl);
					continue;
				}
			} else {
				DHD_ERROR(("%s: wrong ifidx %d!\n", __FUNCTION__,
					ifidx));
				ND_RXLOCK(wl);
				PKTFREE(dhdp->osh, pktbuf, FALSE);
				ND_RXUNLOCK(wl);
				continue;
			}
#else /* UNDER_CE && NDIS60 */
		if (type == ETHER_TYPE_BRCM) {
			bcmerror = wl_host_event(dhdp, &ifidx, (uint8 *)eh, &event, &data);
			/* event already in host order, no subsequent conversion needed */
			wl_event_to_host_order(&event);
			if (bcmerror == BCME_OK) {
#ifdef WLBTAMP
				if (event.event_type == WLC_E_BTA_HCI_EVENT)
					dhd_bta_doevt(dhdp, data, event.datalen);
#endif
#ifdef DHD_NDIS_OID
				/*
				 * Only the events for primary interface get to user
				 */
				if (ifidx == 0) {
					memcpy(&wlc_e.event, &event, sizeof(wl_event_msg_t));
					wl_oid_event(wl->oid, &wlc_e);
					memcpy(&event, &wlc_e.event, sizeof(wl_event_msg_t));
				}
#endif /* DHD_NDIS_OID */
				if (ifidx == 0)
					wl_ndisevent(wl, NULL, &event, (void *)data);

				ND_RXLOCK(wl);
				PKTFREE(dhdp->osh, pktbuf, FALSE);
				ND_RXUNLOCK(wl);

				continue;
			}
#endif /* UNDER_CE && NDIS60 */
		}

		dhdp->dstats.rx_bytes += len;
		dhdp->rx_packets++; /* Local count */

#ifdef EXT_STA
		/* XXX WM7 : check for length(at least 802.3), ifidx, valid data point */
		if (ifidx != 0 ||
			len < (ETHER_HDR_LEN + DNGL_RXCTXT_SIZE) ||
			((uintptr)PKTDATA(dhdp->osh, pktbuf)) == 0xdeadbeef ||
			(len + PKTHEADROOM(dhdp->osh, pktbuf)) > LBDATASZ) {
			DHD_ERROR(("%s: invalid packet format tossed, "
			           "ifidx %d len %d lb->data %p\n",
			           __FUNCTION__, ifidx, len, PKTDATA(dhdp->osh, pktbuf)));
			ND_RXLOCK(wl);
			PKTFREE(wl->sh.osh, pktbuf, FALSE);
			ND_RXUNLOCK(wl);
			continue;
		}
		/*
		* sizeof(rx_ctxt_t), 48, is larger on the dongle than Vista (45)
		* Fixed for now and find cleaner solution later
		*/
		PKTSETLEN(dhdp->osh, pktbuf, len - DNGL_RXCTXT_SIZE);
#endif /* EXT_STA */
#if defined(EXT_STA) && defined(UNDER_CE)
		/* XXX WM7: M1 packet early sendup workaround */
		pkt_ptr = (uchar *)PKTDATA(dhdp->osh, pktbuf);
		if ((pkt_ptr[30] == 0x88) &&
		    (pkt_ptr[31] == 0x8e) &&
		    (pkt_ptr[33] == EAPOL_KEY)) {
			printf("%s: Received M1/M3\n",
			       __FUNCTION__, pkt_ptr[30], pkt_ptr[31], pkt_ptr[33]);
		}

		if (!wl->pub->associated || (wl->blocks & IND_BLOCK_CONNECT) ||
			(wl->blocks & IND_BLOCK_ROAM)) {
			if ((pkt_ptr[30] == 0x88) && (pkt_ptr[31] == 0x8e) &&
				(pkt_ptr[33] == EAPOL_KEY)) {
				printf("%s: Cache M1 ETHER_TYPE 0x%X 0x%X TYPE 0x%X\n",
					__FUNCTION__, pkt_ptr[30], pkt_ptr[31], pkt_ptr[33]);
				DHD_INFO(("%s: Cache M1 ETHER_TYPE 0x%X 0x%X TYPE 0x%X\n",
					__FUNCTION__, pkt_ptr[30], pkt_ptr[31], pkt_ptr[33]));
				/* XXX WP7 TODO: double check ifidx */
				/* ((struct lbuf *)pktbuf)->ifidx = ifidx; */
				ND_RXLOCK(wl);
				if (wl->temp_eapkey) {
					/* free the previous one */
					PKTFREE(wl->sh.osh, wl->temp_eapkey, FALSE);
				}
				wl->temp_eapkey = pktbuf;
				ND_RXUNLOCK(wl);
				continue;
			}
		}
#endif /* EXT_STA && UNDER_CE */
#ifdef WLBTAMP
		/* Forward HCI ACL data to BT stack */
		if ((type < ETHER_TYPE_MIN) && (len >= RFC1042_HDR_LEN)) {
			struct dot11_llc_snap_header *lsh =
			        (struct dot11_llc_snap_header *)&eh[1];

			if (bcmp(lsh, BT_SIG_SNAP_MPROT, DOT11_LLC_SNAP_HDR_LEN - 2) == 0 &&
			    ntoh16(lsh->type) == BTA_PROT_L2CAP) {
				amp_hci_ACL_data_t *ACL_data = (amp_hci_ACL_data_t *)&lsh[1];
#ifdef BCMDBG
				if (DHD_BTA_ON())
					dhd_bta_hcidump_ACL_data(dhdp, ACL_data, FALSE);
#endif
#if defined(WLBTWUSB) || defined(WLBTCEAMP11)
				BtKernForwardData(wl, (PVOID)ACL_data,
				                  (ULONG)len - RFC1042_HDR_LEN);
#endif
				ND_RXLOCK(wl);
				PKTFREE(dhdp->osh, pktbuf, FALSE);
				ND_RXUNLOCK(wl);
				continue;
			} else if (ntoh16(lsh->type) == BTA_PROT_SECURITY) {
					dhd_vif_sendup(dhd_info, ifidx, (uchar *) eh, len);
			}
		}
#endif /* WLBTAMP */

#ifdef UNDER_CE
		/* Place the received packet into queue for later delivery to NDIS stack */
		/* Priority is not used, choose a fixed value so packets stay in order */
		dhd_os_sdlock_sndup_rxq((dhd_pub_t *)dhd_info);
		if (!pktq_full(&dhd_info->rxq)) {
			pktq_penq(&dhd_info->rxq, PRIOMASK, pktbuf);
		} else {
			DHD_ERROR(("%s: out of rxq queue\n", __FUNCTION__));
			dhdp->dstats.rx_dropped++;
			ND_RXLOCK(wl);
			PKTFREE(dhdp->osh, pktbuf, FALSE);
			ND_RXUNLOCK(wl);
			dhd_os_sdunlock_sndup_rxq((dhd_pub_t *)dhd_info);
			break;
		}
		dhd_os_sdunlock_sndup_rxq((dhd_pub_t *)dhd_info);
#else /* UNDER_CE */
		/* XXX WL here makes sure data is 4-byte aligned? */
		wl_sendup(wl, NULL, pktbuf, 1);
#endif /* UNDER_CE */
	}
}

/* send up locally generated event */
void
dhd_sendup_event(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data)
{
	dhd_info_t *dhd_info = (dhd_info_t *)dhdp;
	wl_info_t *wl = dhd_info->wl;
	/* convert event to host order */
	wl_event_to_host_order(event);
	/* send the event up for processing */
	wl_ndisevent(wl, NULL, event, data);
}

void
dhd_txcomplete(dhd_pub_t *dhdp, void *txp, bool success)
{
	int ifidx;
#ifdef WLBTAMP
	struct ether_header *eh;
	uint16 type;
	uint len;
#endif

	/* XXX where does this stuff belong to? */
	dhd_prot_hdrpull(dhdp, &ifidx, txp);

#ifdef WLBTAMP
	/* XXX Use packet tag when it is available to identify its type */
	/* Crack open the packet and check to see if it is BT HCI ACL data packet.
	 * If yes generate packet completion event.
	 */
	len = PKTLEN(dhdp->osh, txp);

	eh = (struct ether_header *)PKTDATA(dhdp->osh, txp);
	type  = ntoh16(eh->ether_type);

	/* Generate ACL data tx completion event locally to avoid SDIO bus transaction */
	if ((type < ETHER_TYPE_MIN) && (len >= RFC1042_HDR_LEN)) {
		struct dot11_llc_snap_header *lsh = (struct dot11_llc_snap_header *)&eh[1];

		if (bcmp(lsh, BT_SIG_SNAP_MPROT, DOT11_LLC_SNAP_HDR_LEN - 2) == 0 &&
		    ntoh16(lsh->type) == BTA_PROT_L2CAP) {

			dhd_bta_tx_hcidata_complete(dhdp, txp, success);
		}
	}
#endif /* WLBTAMP */
}

void dhd_watchdog(PVOID systemspecific1, NDIS_HANDLE context,
	PVOID systemspecific2, PVOID systemspecific3)
{
	dhd_info_t *dhd;
	wl_info_t *wl;

	if (context) {
		dhd = (dhd_info_t *)context;
		ASSERT(context);
		wl = (wl_info_t *)dhd->wl;
#ifdef UNDER_CE
		CeSetThreadPriority(GetCurrentThread(), wl->sh.WatchdogPriority);
#endif /* UNDER_CE */
#if defined(UNDER_CE) && defined(NDIS60)
		if (wl->tx_mid_q_count != wl->txq_count) {
			DHD_INFO(("%s: kick midq timer, tx_mid_q_count %d txq_count %d\n",
				__FUNCTION__, wl->tx_mid_q_count, wl->txq_count));
			DHD_ERROR(("%s: kick midq timer, tx_mid_q_count %d txq_count %d\n",
				__FUNCTION__, wl->tx_mid_q_count, wl->txq_count));

			wl_add_timer(wl, wl->process_mid_txq_timer, 0, FALSE);
		}
#endif /* UNDER_CE && NDIS60 */
		/* Call the bus module watchdog */
#ifndef BCMDBUS
		dhd_bus_watchdog(&dhd->pub);
#endif /* BCMDBUS */

		/* Count the tick for reference */
		dhd->pub.tickcnt++;
	}
}

/* XXX For the moment, local ioctls will return BCM errors */
/* XXX Others return linux codes, need to be changed... */
static int
dhd_ioctl_entry(dhd_info_t *dhd, int ifidx, char *pInBuf)
{
	dhd_ioctl_t *ioc;
	int bcmerror = 0;
	int len = 0;
	void *buf = NULL;
#ifdef BCMDONGLEHOST
	wl_info_t *wl;
#endif	/* BCMDONGLEHOST */
#ifdef UNDER_CE
#if (CE_MAJOR_VER >= 0x0006)
	HRESULT status;
#endif /* CE_MAJOR_VER >= 0x0006 */
#endif /* UNDER_CE */

#ifdef BCMDONGLEHOST
	wl = (wl_info_t *)dhd->wl;
#endif	/* BCMDONGLEHOST */

	ioc = (dhd_ioctl_t *)pInBuf;

	if (ioc->buf) {
		len = MIN(ioc->len, DHD_IOCTL_MAXLEN);
#ifdef UNDER_CE
#if (CE_MAJOR_VER <= 0x0005)
		/* When CETK 5.0 attempts unbinding adapter using ProtocolUnbindAdapter,
		* it tries to clear the multicast list (OID_802_3_MULTICAST_LIST)
		* in the miniport driver. It is observed that during this time,
		* MapCallerPtr returns a non-null, but incorrectly mapped pointer,
		* using which causes a crash in dhd. However, any number of usage of the same
		* OID (OID_802_3_MULTICAST_LIST) prior to this point succeeds without any problem.
		*
		* In case of OID_802_3_MULTICAST_LIST, an internal buffer (created in wl_iovar_op)
		* is used instead of external input buffer (received in wl_msetinformation).
		* Hence we can avoid MapCallerPtr call during this point, preventing the crash.
		* But we need to figure out the exact cause of MapCallerPtr behavior
		* during ProtocolUnbindAdapter under CE 5.0.
		*/
		if (GetCallerProcess() != GetOwnerProcess())
			buf = MapCallerPtr(ioc->buf, ioc->len);
		else
			buf = ioc->buf;
#else /* CE_MAJOR_VER <= 0x0005 */
#if defined(NDIS60)
	 /* CE6.0 driver running in kernel mode does not need to map buffer */
		buf = ioc->buf;
#else
		status = CeOpenCallerBuffer(&buf, ioc->buf, ioc->len, ARG_IO_PTR, FALSE);
		if (status != S_OK) {
			DHD_ERROR(("BCMSDLDR: CeOpenCallerBuffer failed status = %d\n", status));
			bcmerror = -BCME_BADADDR;
			goto err;
		}
#endif /* NDIS60 */
#endif /* CE_MAJOR_VER <= 0x0005 */
#else /* UNDER_CE */
		buf = ioc->buf;
#endif /* UNDER_CE */

		if (!buf) {
			DHD_ERROR(("BCMSDLDR: Error mapping the ioctl buffer\n"));
			bcmerror = -BCME_BADADDR;
			goto err;
		}
	}

	/* check for local dhd ioctl and handle it */
	if (ioc->driver == DHD_IOCTL_MAGIC) {
		bcmerror = dhd_ioctl(&dhd->pub, ioc, buf, len);
		if (bcmerror)
			dhd->pub.bcmerror = bcmerror;
		goto err;
	}

	/* send to dongle (must be up, and wl) */
	if (!dhd->pub.up || (dhd->pub.busstate != DHD_BUS_DATA)) {
		bcmerror = -BCME_NOTUP;
		goto err;
	}

	if (!dhd->pub.iswl) {
		bcmerror = -BCME_NOTUP;
		goto err;
	}

#if defined(BCMDONGLEHOST) && defined(EXT_STA)
	switch (ioc->cmd) {
	case OID_DOT11_CIPHER_KEY_MAPPING_KEY:
		/* Serialize M4 send and set key OID to prevent M4 encryption */
		wl_process_mid_txq(wl);
		break;
	default:
		break;
	}
#endif	/* BCMDONGLEHOST */
	bcmerror = dhd_wl_ioctl(&dhd->pub, 0, (wl_ioctl_t *)ioc, buf, len);
	if (bcmerror) {
		dhd->pub.bcmerror = bcmerror;
		goto err;
	}
#if defined(BCMDONGLEHOST) && defined(EXT_STA)
	if (ioc->set) {

		switch (ioc->cmd) {
		case OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM:
			wl->oid->NDIS_auth = ((DOT11_AUTH_ALGORITHM_LIST *)buf)->AlgorithmIds[0];
			break;
		case OID_DOT11_DESIRED_BSS_TYPE:
			wl->oid->NDIS_infra =
				(*(DOT11_BSS_TYPE *)buf == dot11_BSS_type_infrastructure)?
				Ndis802_11Infrastructure : Ndis802_11IBSS;
			break;
		case OID_DOT11_RESET_REQUEST:
			wl->oid->NDIS_infra = Ndis802_11Infrastructure;
			break;
		case OID_DOT11_CIPHER_KEY_MAPPING_KEY:
#if defined(EXT_STA) && defined(UNDER_CE)
			/* XXX WM7 fix: Set flag to allow G2 encryption */
			DHD_TRACE(("%s: Set flag to allow G2 encryption\n", __FUNCTION__));
			wl->pwk = 1;
#endif /* EXT_STA && UNDER_CE */
			break;
		default:
			break;
		}
	}
#endif	/* BCMDONGLEHOST */

err:
	if (buf) {
#ifdef UNDER_CE
#if (CE_MAJOR_VER <= 0x0005)
		if (GetCallerProcess() != GetOwnerProcess())
			UnMapPtr(buf);
#else /* CE_MAJOR_VER <= 0x0005 */
#if !defined(NDIS60)
		CeCloseCallerBuffer(buf, ioc->buf, ioc->len, ARG_IO_PTR);
#endif /* !NDIS60 */
#endif /* CE_MAJOR_VER <= 0x0005 */
#endif  /* UNDER_CE */
	}

	if (bcmerror > BCME_LAST)
		return OSL_ERROR(bcmerror);
	else {
		/* Must be NDIS errors so pass to host */
		return bcmerror;
	}
}

static int
dhd_stop(dhd_info_t *dhd)
{
	bool canceled;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* Set state and stop OS transmissions */
	dhd->pub.up = 0;

	/* Stop the protocol module */
	dhd_prot_stop(&dhd->pub);

	/* Stop the bus module */
#ifdef BCMDBUS
	dbus_stop(dhd->pub.dbus);
	dhd->pub.busstate = DHD_BUS_DOWN;
#else
	dhd_bus_stop(dhd->pub.bus, TRUE);
	if (dhd->wd_timer_valid == TRUE)
		NdisMCancelTimer(&dhd->h_wd_timer, &canceled);

#endif /* BCMDBUS */

	return 0;
}

static char dongleimagepathkey[256];
static char sromimagepathkey[256];
static char pktfilterkey[] = "PKTFilterXX";
static ndis_config_t dhd_configs[] = {
	{ "DriverVersion", NdisParameterString, 0, 0, NULL },
	{ "Capabilities", NdisParameterInteger, 0, 0, NULL },
	{ dongleimagepathkey, NdisParameterString, 0, 0, NULL },
	{ sromimagepathkey, NdisParameterString, 0, 0, NULL },
	{ "DHDMsgLevel", NdisParameterInteger, 0, 0, NULL },
	{ "WLMsgLevel", NdisParameterInteger, 0, 0, NULL },
	{ "TestMode", NdisParameterInteger, 0, 0, NULL },
	{ "MfgtestDongleImagePath", NdisParameterString, 0, 0, NULL },
	{ "DongleImagePath", NdisParameterString, 0, 0, NULL },
	{ "SROMImagePath", NdisParameterString, 0, 0, NULL },
	{ "EnableAOE", NdisParameterInteger, 0, 0, NULL },
	{ "AOEMode", NdisParameterInteger, 0, 0, NULL },
	{ "EnablePKTFilter", NdisParameterInteger, 0, 0, NULL },
	{ "PKTFilterMode", NdisParameterInteger, 0, 0, NULL },
	{ "EnableOOBInterrupt", NdisParameterInteger, 0, 0, NULL },
	{ "DongleConsoleMs", NdisParameterInteger, 0, 0, NULL },
	{ "DbgOutput", NdisParameterInteger, 0, 0, NULL },
	{ "PNPMethod", NdisParameterInteger, 0, 0, NULL },
	{ "WatchdogPeriodOnBattery", NdisParameterInteger, 0, 0, NULL },
	{ "WatchdogPeriodOnAC", NdisParameterInteger, 0, 0, NULL },
	{ "DPCPriority", NdisParameterInteger, 0, 0, NULL },
	{ "RXThreadPriority", NdisParameterInteger, 0, 0, NULL },
	{ "OidPriority", NdisParameterInteger, 0, 0, NULL },
	{ "EventPriority", NdisParameterInteger, 0, 0, NULL },
	{ "BTCoexTimeout", NdisParameterInteger, 0, 0, NULL },
	{ "BTCoexMargin", NdisParameterInteger, 0, 0, NULL },
	{ "ScanAgingThreshold", NdisParameterInteger, 0, 0, NULL },
	{ NULL, 0, 0, 0, NULL }
};

#if defined (UNDER_CE)
static void
dhd_event_process(void *h)
{
	dhd_info_t *dhd = (dhd_info_t *)h;
	void *data, *pkt = 0;
	uint prec_out, num = 0;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	wl_event_msg_t event;
	dhd_pub_t *dhdp;
	int ifidx;
	int bcmerror;
	struct ether_header *eh;
	uint len = 0;
	dhdp = (dhd_pub_t *)h;

	/* Read the priority from registry */
	CeSetThreadPriority(GetCurrentThread(), wl->sh.EventPriority);
	DHD_TRACE(("dhd_event_process thread priority = %d\n",
		CeGetThreadPriority(GetCurrentThread())));

	while (TRUE) {
		DHD_TRACE(("%s - Waiting\n", __FUNCTION__));

		NdisWaitEvent(&dhd->host_event, 0); /* wait forever */
		NdisResetEvent(&dhd->host_event);		/* reset the event */

		DHD_TRACE(("%s - Sending up into the NDIS stack\n", __FUNCTION__));

		num = 0;
		do {
			dhd_os_sdlock_eventq((dhd_pub_t *)dhd);
			DHD_TRACE(("%s: DEQ eventq %d\n",
				__FUNCTION__, pktq_mlen(&dhd->eventq, 8)));
			pkt = pktq_deq(&dhd->eventq, &prec_out);
			dhd_os_sdunlock_eventq((dhd_pub_t *)dhd);

			if (pkt) {
				PKTSETNEXT(wl->sh.osh, pkt, NULL);
				len = PKTLEN(dhdp->osh, pkt);
				eh = (struct ether_header *)PKTDATA(dhdp->osh, pkt);
				/* XXX WP7 GOLD: check for eh 0xdeadbeef */
				if (eh && ((uintptr)eh) != 0xdeadbeef &&
				    len >= sizeof(bcm_event_t) &&
					(len + PKTHEADROOM(dhdp->osh, pkt)) <= LBDATASZ) {
					bcmerror = wl_host_event(dhdp, &ifidx,
						(uint8 *)eh, &event, &data);
					wl_event_to_host_order(&event);
					if (bcmerror == BCME_OK) {
						wl_ndisevent(wl, wl->oid->wlcif->wlif,
						             &event, (void *)data);
					}
				} else {
					DHD_ERROR(("%s: invalid event pkt tossed len %d data %p\n",
						__FUNCTION__, len, eh));
				}
			} else {
				break;
			}
			ND_RXLOCK(wl);
			PKTFREE(dhdp->osh, pkt, FALSE);
			ND_RXUNLOCK(wl);
		} while (TRUE);
	}
}
#endif /* defined (UNDER_CE) */ 

static void
dhd_update_ver2registry(dhd_info_t *dhd, wl_info_t *wl)
{
	char firmVerBuf[128];
	char verBuf[512];
	char *pVerBuf = verBuf;
	char *pFirmwareVersion;
	char *pFirmwareBuildTime = firmVerBuf;
	char matchPattern[] = "version";
	NDIS_HANDLE confighandle = NULL;
	NDIS_STATUS status;
#ifdef NDIS60
	NDIS_CONFIGURATION_OBJECT configobj;
#endif /* NDIS60 */
	shared_info_t *sh = &wl->sh;
	uint capabilities;
	int ret = 0;

	/* open the registry */
#ifndef NDIS60
	NdisOpenConfiguration(&status, &confighandle, sh->confighandle);
#else
	bzero(&configobj, sizeof(NDIS_CONFIGURATION_OBJECT));

	configobj.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
	configobj.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
	configobj.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);

	configobj.NdisHandle = sh->adapterhandle;
	configobj.Flags = 0;

	status = NdisOpenConfigurationEx(&configobj, &confighandle);
#endif /* !NDIS60 */
	if (NDIS_ERROR(status)) {
		DHD_ERROR(("dhd_update_ver2registry: NdisOpenConfiguration error 0x%x\n", status));
		NdisWriteErrorLogEntry(sh->adapterhandle,
		                       NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER, 1, 24);
		return;
	}

	ASSERT(confighandle);

	/* query for 'ver' to get version info from firmware */
	memset(firmVerBuf, 0, sizeof(firmVerBuf));
	bcm_mkiovar("ver", 0, 0, firmVerBuf, sizeof(firmVerBuf));
	ret = dhd_wl_ioctl_cmd(&dhd->pub, WLC_GET_VAR, firmVerBuf, sizeof(firmVerBuf), FALSE, 0);

	if (ret < 0) {
		goto done;
	}

	pFirmwareVersion = bcmstrstr(firmVerBuf, matchPattern);
	pFirmwareVersion[0] = '\0';
	pFirmwareVersion += strlen(matchPattern);

	pVerBuf += sprintf(pVerBuf, "Broadcom 802.11 DCF-only Driver version %s (%s %s) ",
	                   EPI_VERSION_STR, __DATE__, __TIME__);
	pVerBuf += sprintf(pVerBuf, "Firmware version %s (%s)",
	                   pFirmwareVersion, pFirmwareBuildTime);

	wl_writeparam(dhd_configs, "DriverVersion", (void*)verBuf, confighandle, wl, NULL);

	/* save the capabilities for this driver version */
	capabilities = CAPABILITY_HIDDEN_NETWORK;
	wl_writeparam(dhd_configs, "Capabilities", (void*)&capabilities, confighandle, wl, NULL);

done:
	/* close the registry */
	if (confighandle) {
		NdisCloseConfiguration(confighandle);
	}
}

static int
dhd_open(dhd_info_t *dhd, wl_info_t *wl)
{
	int ret = -1;
	int i = 0;
#ifdef UNDER_CE
	int dhd_WAR44971;
	uint dhd_glom;
#endif /* UNDER_CE */
	shared_info_t *sh = &wl->sh;
#ifdef BCMSDIO
	NDIS_HANDLE confighandle = NULL;
	NDIS_STATUS status;
	PNDIS_CONFIGURATION_PARAMETER param;
#ifdef NDIS60
	NDIS_CONFIGURATION_OBJECT configobj;
#endif /* NDIS60 */
	uint chip;
#endif	/* BCMSDIO */

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

#ifdef BCMSDIO
	/* firmware and nvram files paths. chip specific paths take precedence */
#ifdef BCMDBUS
	chip = 0;
#else
	chip = dhd_bus_chip(dhd->pub.bus);
#endif
	/* open the registry */
#ifndef NDIS60
	NdisOpenConfiguration(&status, &confighandle, sh->confighandle);
#else
	bzero(&configobj, sizeof(NDIS_CONFIGURATION_OBJECT));

	configobj.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
	configobj.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
	configobj.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);

	configobj.NdisHandle = sh->adapterhandle;
	configobj.Flags = 0;

	status = NdisOpenConfigurationEx(&configobj, &confighandle);
#endif /* !NDIS60 */
	if (NDIS_ERROR(status)) {
		DHD_ERROR(("dhd_open: NdisOpenConfiguration error 0x%x\n", status));
		NdisWriteErrorLogEntry(sh->adapterhandle,
		                       NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER, 1, 24);
		return -1;
	}
	ASSERT(confighandle);

	if ((param = wl_readparam(dhd_configs, "DHDMsgLevel", confighandle, wl, NULL))) {
		dhd_msg_level = param->ParameterData.IntegerData;
		printf("dhd_msg_level set to %u\n", dhd_msg_level);
	}

	/* Debug messages in WL_xx debug macros (wl_ndis.c) */
	if ((param = wl_readparam(dhd_configs, "WLMsgLevel", confighandle, wl, NULL))) {
		wl_msg_level = param->ParameterData.IntegerData;
		printf("wl_msg_level set to %u\n", wl_msg_level);
	}

	/* EnableArpOffload allows to control the arp offload feature in the firmware
	*/
	if ((param = wl_readparam(dhd_configs, "EnableAOE", confighandle, wl, NULL))) {
		dhd_arp_enable = param->ParameterData.IntegerData ? TRUE : FALSE;
	}
	DHD_INFO(("EnableArpOffload set to %d from the registry\n", dhd_arp_enable));

	/* Controls ArpOffload mode
	*/
	if ((param = wl_readparam(dhd_configs, "AOEMode", confighandle, wl, NULL))) {
		dhd_arp_mode = param->ParameterData.IntegerData;
	}
	DHD_INFO(("ArpOffload mode set to 0x%x from the registry\n", dhd_arp_mode));

	/* Controls Pkt filter enable/disable
	*/
	if ((param = wl_readparam(dhd_configs, "EnablePKTFilter", confighandle, wl, NULL))) {
		dhd_pkt_filter_enable = param->ParameterData.IntegerData;
	}
	DHD_INFO(("EnablePktfilter set to 0x%x from the registry\n", dhd_pkt_filter_enable));

	/* Controls Pkt filter mode
	*/
	if ((param = wl_readparam(dhd_configs, "PKTFilterMode", confighandle, wl, NULL))) {
		dhd_master_mode = param->ParameterData.IntegerData;
	}
	DHD_INFO(("EnablePktfilter mode set to 0x%d from the registry\n", dhd_master_mode));

	if (dhd_pkt_filter_enable) {
		do {
			/* read pkt filter defination */
			snprintf(pktfilterkey, sizeof(pktfilterkey), "PKTFilter%02d", i);
			if ((param = wl_readparam(dhd_configs, pktfilterkey,
				confighandle, wl, NULL))) {

				if (!(dhd->pub.pktfilter[i] = MALLOC(dhd->pub.osh,
					param->ParameterData.StringData.Length + 1))) {
					DHD_ERROR(("%s: kmalloc failed\n", __FUNCTION__));
					break;
				}

				wchar2ascii(dhd->pub.pktfilter[i],
					param->ParameterData.StringData.Buffer,
					param->ParameterData.StringData.Length,
					param->ParameterData.StringData.Length);
			} else {
				break;
			}
			DHD_INFO(("%s is set to %s\n", pktfilterkey, dhd->pub.pktfilter[i]));
		} while (++i);

		dhd->pub.pktfilter_count = i;
		dhd_pkt_filter_enable = i ? 1 : 0;
	}
#if defined(EXT_STA) && defined (UNDER_CE)
	/* Scan results aging threshold in second */
	if ((param = wl_readparam(dhd_configs, "ScanAgingThreshold", confighandle, wl, NULL))) {
		wl->scan_aging_threshold = param->ParameterData.IntegerData;
	} else {
		/* Default 30 seconds */
		wl->scan_aging_threshold = 30;
	}
	DHD_INFO(("ScanAgingThreshold set to %d from the registry\n", wl->scan_aging_threshold));

	/* Debug message output destination */
	if ((param = wl_readparam(dhd_configs, "DbgOutput", confighandle, wl, NULL))) {
		if (param->ParameterData.IntegerData < DBG_OUTPUT_MAX) {
			 g_dbgOutputIndex = param->ParameterData.IntegerData;
		}
	}
	DHD_INFO(("DbgOutput set to %d from the registry\n", g_dbgOutputIndex));

	/* PNP handling option */
	if ((param = wl_readparam(dhd_configs, "PNPMethod", confighandle, wl, NULL))) {
		wl->PNPMethod = param->ParameterData.IntegerData;
	} else {
		wl->PNPMethod = PNP_NORMAL;
	}
	DHD_INFO(("PNPMethod set to %d from the registry\n", wl->PNPMethod));

	/* WatchdogPeriodOnBattery handling option */
	if ((param = wl_readparam(dhd_configs, "WatchdogPeriodOnBattery",
	                          confighandle, wl, NULL))) {
		dhd_watchdog_battery_ms = param->ParameterData.IntegerData;
	}
	DHD_INFO(("WatchdogPeriodOnBattery set to %d from the registry\n",
	          dhd_watchdog_battery_ms));

	/* WatchdogPeriodOnAC handling option */
	if ((param = wl_readparam(dhd_configs, "WatchdogPeriodOnAC", confighandle, wl, NULL))) {
		dhd_watchdog_ac_ms = param->ParameterData.IntegerData;
	}
	DHD_INFO(("WatchdogPeriodOnAC set to %d from the registry\n", dhd_watchdog_ac_ms));


	/* OidPriority handling option */
	if ((param = wl_readparam(dhd_configs, "OidPriority", confighandle, wl, NULL))) {
		wl->sh.OidPriority = param->ParameterData.IntegerData;
	} else {
		wl->sh.OidPriority = 0;
	}
	DHD_INFO(("OidPriority set to %d from the registry\n", wl->sh.OidPriority));

	/* EventPriority handling option */
	if ((param = wl_readparam(dhd_configs, "EventPriority", confighandle, wl, NULL))) {
		wl->sh.EventPriority = param->ParameterData.IntegerData;
	} else {
		wl->sh.EventPriority = 132;
	}
	DHD_INFO(("EventPriority set to %d from the registry\n", wl->sh.EventPriority));

	if ((param = wl_readparam(dhd_configs, "DPCPriority", confighandle, wl, NULL))) {
		wl->sh.DPCPriority = param->ParameterData.IntegerData;
		wl->sh.dpc_prio = param->ParameterData.IntegerData;
	} else {
		wl->sh.DPCPriority = 251;
		wl->sh.dpc_prio = 251;
	}
	DHD_INFO(("DPCPriority set to %d from the registry\n", wl->sh.DPCPriority));

	if ((param = wl_readparam(dhd_configs, "RXThreadPriority", confighandle, wl, NULL))) {
		wl->sh.RXThreadPriority = param->ParameterData.IntegerData;
		wl->sh.sendup_prio = param->ParameterData.IntegerData;
	} else {
		wl->sh.RXThreadPriority = 135;
		wl->sh.sendup_prio = 135;
	}
	DHD_INFO(("RXThreadPriority set to %d from the registry\n", wl->sh.RXThreadPriority));

#if defined(BTCOEX_DHCP)
	/* BTCoexTimeout handling option */
	if ((param = wl_readparam(dhd_configs, "BTCoexTimeout", confighandle, wl, NULL))) {
		wl->BTCoexTimeout = param->ParameterData.IntegerData;
	} else {
		wl->BTCoexTimeout = 0;
	}

	DHD_INFO(("wl->BTCoexTimeout set to %d from the registry\n", wl->BTCoexTimeout));

	/* BTCoexMargin handling option */
	if ((param = wl_readparam(dhd_configs, "BTCoexMargin", confighandle, wl, NULL))) {
		wl->BTCoexMargin = param->ParameterData.IntegerData;
	} else {
		wl->BTCoexMargin = 50000;
	}

	DHD_INFO(("wl->BTCoexMargin set to %d from the registry\n", wl->BTCoexMargin));
#endif /* defined(BTCOEX_DHCP) */
#endif /* EXT_STA && UNDER_CE */

#ifdef DHD_DEBUG
	/* Dongle console message poll interval */
	if ((param = wl_readparam(dhd_configs, "DongleConsoleMs", confighandle, wl, NULL))) {
		 dhd_console_ms = param->ParameterData.IntegerData;
	}
	DHD_INFO(("DongleConsoleMs set to 0x%x ms from the registry\n", dhd_console_ms));
#endif /* DHD_DEBUG */
	if ((param = wl_readparam(dhd_configs, "TestMode", confighandle, wl, NULL))) {
		 mfgtest_mode = param->ParameterData.IntegerData;
	} else {
		mfgtest_mode = 0; /* Production by default */
	}
	DHD_INFO(("TestMode set to 0x%x ms from the registry\n", mfgtest_mode));
	DHD_ERROR(("TestMode set to 0x%x ms from the registry\n", mfgtest_mode));

	if (mfgtest_mode) {
		/* Load mfgtest image */
		 if ((param = wl_readparam(dhd_configs, "MfgtestDongleImagePath",
		                           confighandle, wl, NULL))) {
			wchar2ascii(fw_path, param->ParameterData.StringData.Buffer,
				param->ParameterData.StringData.Length,
				sizeof(fw_path));
		 }
	} else {
		/* Load production image */
		/* read dongle image path if it exists */
		if (chip > 40000)
			snprintf(dongleimagepathkey, sizeof(dongleimagepathkey),
			         "%dDongleImagePath", chip);
		else
			snprintf(dongleimagepathkey, sizeof(dongleimagepathkey),
			         "%04XDongleImagePath", chip);

		if ((param = wl_readparam(dhd_configs, dongleimagepathkey,
		                          confighandle, wl, NULL)) ||
		    (param = wl_readparam(dhd_configs, "DongleImagePath",
		                          confighandle, wl, NULL))) {
			wchar2ascii(fw_path, param->ParameterData.StringData.Buffer,
			            param->ParameterData.StringData.Length,
			            sizeof(fw_path));
		}
		DHD_INFO(("firmware path set to %s\n", fw_path));
	}

	/* read srom image path if it exists */
	if (chip > 40000)
		snprintf(sromimagepathkey, sizeof(sromimagepathkey), "%dSROMImagePath", chip);
	else
		snprintf(sromimagepathkey, sizeof(sromimagepathkey), "%04XSROMImagePath", chip);

	if ((param = wl_readparam(dhd_configs, sromimagepathkey, confighandle, wl, NULL)) ||
	    (param = wl_readparam(dhd_configs, "SROMImagePath", confighandle, wl, NULL))) {

		wchar2ascii(nv_path, param->ParameterData.StringData.Buffer,
			param->ParameterData.StringData.Length,
			sizeof(nv_path));
	}

	DHD_INFO(("nvram path set to %s\n", nv_path));

	/* close the registry */
	NdisCloseConfiguration(confighandle);

	dhd->wl = wl;

#ifndef BCMDBUS
	/* download image and nvram to the dongle */
	if (!(dhd_bus_download_firmware(dhd->pub.bus, dhd->pub.osh,
	                                fw_path, nv_path))) {
		DHD_ERROR(("%s: dhd_bus_download_firmware failed. firmware = %s nvram = %s\n",
		           __FUNCTION__, fw_path, nv_path));
		return -1;
	}
#endif
#endif	/* BCMSDIO */

#ifdef UNDER_CE
	NdisInitializeEvent(&sh->dpc_event);
	NdisResetEvent(&sh->dpc_event);

	/* XXX Tx needs priority queue, where to determine levels? */
	/* XXX Should it try to do WLC mapping, or just pass through? */
	pktq_init(&dhd->rxq, (PRIOMASK+1), QLEN);
	pktq_init(&dhd->eventq, (PRIOMASK+1), QLEN);

	NdisInitializeEvent(&dhd->sendup_event);
	NdisResetEvent(&dhd->sendup_event);
	/* XXX - should move to ndishared sublayer */
	dhd->sendup_handle = CreateThread(NULL,		/* security attributes */
			0,		/* initial stack size */
			(LPTHREAD_START_ROUTINE)dhd_sendup, /* Main() function */
			dhd,		/* arg to reader thread */
			0,		/* creation flags */
			&dhd->sendup_thread_id); /* returned thread id */
	if (!dhd->sendup_handle)
		return NDIS_STATUS_FAILURE;
	NdisInitializeEvent(&dhd->host_event);
	NdisResetEvent(&dhd->host_event);
	/* XXX - should move to ndishared sublayer */
	dhd->event_process_handle = CreateThread(NULL,		/* security atributes */
			0,		/* initial stack size */
			(LPTHREAD_START_ROUTINE)dhd_event_process, /* Main() function */
			dhd,		/* arg to reader thread */
			0,		/* creation flags */
			&dhd->event_process_thread_id); /* returned thread id */
	if (!dhd->event_process_handle)
		return NDIS_STATUS_FAILURE;

#endif /* UNDER_CE */

	/* Bring up the bus */
#ifdef BCMDBUS
	if ((ret = dbus_up(dhd->pub.dbus)) != 0)
		return ret;

	dhd->pub.busstate = DHD_BUS_DATA;
#else
	/* Bring up the bus */
	if ((ret = dhd_bus_init(&dhd->pub, TRUE)) != 0) {
		DHD_ERROR(("%s: dhd_bus_init failed\n",  __FUNCTION__));
		return ret;
	}
#endif /* BCMDBUS */

	/* If bus is not ready, can't come up */
	if (dhd->pub.busstate != DHD_BUS_DATA) {
		DHD_ERROR(("%s: bus is not ready. state = %d\n", __FUNCTION__, dhd->pub.busstate));
		return -1; /* XXX fix me, not a valid error code */
	}

#ifndef BCMDBUS
	dhd_os_wd_timer(dhd, dhd_watchdog_ms);
#endif

	/* Bus is ready, do any protocol initialization */
	/* XXX Since prot_init can sleep, should module count surround it? */
	dhd_prot_init(&dhd->pub);

	/* update the registry with the host and dongle driver versions */
	dhd_update_ver2registry(dhd, wl);

	DHD_ERROR(("Dongles MAC address = %02X:%02X:%02X:%02X:%02X:%02X\n",
		dhd->pub.mac.octet[0], dhd->pub.mac.octet[1], dhd->pub.mac.octet[2],
		dhd->pub.mac.octet[3], dhd->pub.mac.octet[4], dhd->pub.mac.octet[5]));

#ifdef UNDER_CE
	/* XXXX??? We need to resolve this for CE port, 
	 * Disable these war for the time being
	 */
	/*
	dhd_WAR44971 = FALSE;
	wl_iovar_op(wl, "war44971", &dhd_WAR44971, sizeof(dhd_WAR44971), IOV_SET);
	*/

	/* Turn off glom by default for CE */
	dhd_glom = 0;
	ret = wl_iovar_op(wl, "bus:txglom", &dhd_glom, sizeof(dhd_glom), IOV_SET);
	/* printf ("bus:txglom iovar ret = %d\n", ret); */
#endif /* UNDER_CE */

	/* Allow transmit calls */
	dhd->pub.up = 1;

	return 0;
}

osl_t *
dhd_osl_attach(void *pdev, uint bustype)
{
	return 0;
}

void
dhd_osl_detach(osl_t *osh)
{
	return;
}

/*
 * Generalized timeout mechanism.  Uses spin sleep with exponential back-off.  Usage:
 *
 *      dhd_timeout_start(&tmo, usec);
 *      while (!dhd_timeout_expired(&tmo))
 *              if (poll_something())
 *                      break;
 *      if (dhd_timeout_expired(&tmo))
 *              fatal();
 */

void
dhd_timeout_start(dhd_timeout_t *tmo, uint usec)
{
	tmo->limit = usec;
	tmo->increment = 0;
	tmo->elapsed = 0;
	tmo->tick = 10000;
}

int
dhd_timeout_expired(dhd_timeout_t *tmo)
{
	/* Does nothing the first call */
	if (tmo->increment == 0) {
		tmo->increment = 1;
		return 0;
	}

	if (tmo->elapsed >= tmo->limit)
		return 1;

	/* Add the delay that's about to take place */
	tmo->elapsed += tmo->increment;

	/* XXX TBD: switch to task tick when increment gets large enough (see dhd_linux.c) */

	OSL_DELAY(tmo->increment);
	tmo->increment *= 2;
	if (tmo->increment > tmo->tick)
		tmo->increment = tmo->tick;

	return 0;
}

int
dhd_ifname2idx(dhd_info_t *dhd, char *name)
{
	int i = DHD_MAX_IFS;

	ASSERT(dhd);

	if (name == NULL || *name == '\0')
		return 0;

	while (--i > 0)
		if (dhd->iflist[i] && !strncmp(dhd->iflist[i]->name, name, IFNAMSIZ))
			break;

	DHD_TRACE(("%s: return idx %d for \"%s\"\n", __FUNCTION__, i, name));

	return i;	/* default - the primary interface */
}

int
dhd_add_if(dhd_info_t *dhd, int ifidx, void *handle, char *name,
	uint8 *mac_addr, uint32 flags, uint8 bssidx)
{
	dhd_if_t *ifp;

	DHD_TRACE(("%s: idx %d, handle->%p\n", __FUNCTION__, ifidx, handle));

	ASSERT(dhd && (ifidx < DHD_MAX_IFS));

	ifp = dhd->iflist[ifidx];
	if (!ifp && !(ifp = MALLOC(dhd->pub.osh, sizeof(dhd_if_t)))) {
		DHD_ERROR(("%s: OOM - dhd_if_t\n", __FUNCTION__));
		return -ENOMEM;
	}

	memset(ifp, 0, sizeof(dhd_if_t));
	ifp->info = dhd;
	dhd->iflist[ifidx] = ifp;
	strncpy(ifp->name, name, IFNAMSIZ);
	ifp->name[IFNAMSIZ] = '\0';
	if (mac_addr != NULL)
		memcpy(&ifp->mac_addr, mac_addr, ETHER_ADDR_LEN);

	ifp->state = WLC_E_IF_ADD;
	ifp->idx = ifidx;
	ifp->bssidx = bssidx;

	return 0;
}

void
dhd_del_if(dhd_info_t *dhd, int ifidx)
{
	dhd_if_t *ifp;

	DHD_TRACE(("%s: idx %d\n", __FUNCTION__, ifidx));

	ASSERT(dhd && ifidx && (ifidx < DHD_MAX_IFS));
	ifp = dhd->iflist[ifidx];
	if (!ifp) {
		DHD_ERROR(("%s: Null interface\n", __FUNCTION__));
		return;
	}

	ifp->state = WLC_E_IF_DEL;
	ifp->idx = ifidx;
	return;
}

dhd_pub_t *
dhd_attach(osl_t *osh, struct dhd_bus *bus, uint bus_hdrlen)
{
	dhd_info_t *dhd = NULL;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (!(dhd = MALLOC(osh, sizeof(dhd_info_t)))) {
		DHD_ERROR(("malloc of dhd_info_t failed!\n"));
		goto fail;
	}

	memset(dhd, 0, sizeof(dhd_info_t));
	dhd->pub.osh = osh;

	/* Link to the info module */
	dhd->pub.info = dhd;

	/* Link to bus module */
	dhd->pub.bus = bus;
	dhd->pub.hdrlen = bus_hdrlen;

#ifdef UNDER_CE
	InitializeCriticalSection(&dhd->proto_sem);

	dhd->ioctl_resp_wait = CreateSemaphore(0, 0, 1, 0);
	if (!dhd->ioctl_resp_wait) {
		DHD_ERROR(("CreateSemaphore failed\n"));
		goto fail;
	}
#else /* UNDER_CE */
	KeInitializeMutex(&dhd->proto_sem, 0);
	NdisInitializeEvent(&dhd->ioctl_resp_wait);
#endif /* UNDER_CE */

	/* Attach and link in the protocol */
	if (dhd_prot_attach(&dhd->pub) != 0) {
		DHD_ERROR(("dhd_prot_attach failed\n"));
		goto fail;
	}

	/* XXX Set up an MTU change notifier as per linux/notifier.h? */
	/* XXX VBR: What is the right value here */
	dhd->pub.rxsz = ETHER_MTU + 14 + dhd->pub.hdrlen;

	return (dhd_pub_t *)dhd;

fail:
	dhd_detach((dhd_pub_t *)dhd);
	dhd_free((dhd_pub_t *)dhd);

	return NULL;
}

int
dhd_bus_start(dhd_pub_t *dhdp)
{
	/* Nothing to do */
	return 0;
}

int
dhd_net_attach(dhd_pub_t *dhdp, int ifidx)
{
	/* Nothing to do */
	return 0;
}

void
dhd_detach(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (dhd) {

#ifndef BCMDBUS
		/* XXX fix: dbus_detach() happened already....
		 * Need to re-order somehow
		 */
		dhd_stop(dhd);
#endif

		if (dhd->pub.prot)
			dhd_prot_detach(&dhd->pub);

#ifdef UNDER_CE
		if (dhd->ioctl_resp_wait)
			CloseHandle(dhd->ioctl_resp_wait);

		DeleteCriticalSection(&dhd->proto_sem);

		if (dhd->sendup_handle)
			CloseHandle(dhd->sendup_handle);
		if (dhd->event_process_handle)
			CloseHandle(dhd->event_process_handle);
		NdisFreeEvent(&dhd->sendup_event);
		NdisFreeEvent(&dhd->host_event);
#endif /* UNDER_CE */
	}
}

void
dhd_free(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp;
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (dhd && dhd->pub.osh)
		MFREE(dhd->pub.osh, dhd, sizeof(dhd_info_t));
}

#ifdef DHD_NDIS_OID
NDIS_STATUS
dhd_oid_attach(wl_info_t *wl)
{
	/* Place holder */
	return NDIS_STATUS_SUCCESS;
}
void
dhd_oid_detach(wl_info_t *wl)
{
	/* Place holder */
}

NDIS_STATUS
dhd_oid_init(wl_info_t *wl)
{
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;
	uint bandlist[3];
	wl_bss_config_t default_bss;
	wl_oid_t *oid = wl->oid;
	ASSERT(oid != NULL);

#ifdef STA
	/* init the gmode state to an invalid value */
	oid->gmode_ibss = -1;
	oid->gmode_prev = -1;
#endif /* STA */

	/* indicate unconditional link up for IBSS */
	oid->cmn->legacy_link = FALSE;

	/* XXX Assume TRUE */
	oid->cmn->WPA = TRUE;
	/* force link up: disabled (i.e. indicate link down (if so) to NDIS) */
	oid->forcelink = FALSE;
	/* User bandlock */
	oid->cmn->bandlock = WLC_BAND_AUTO;

	/* time zero NDIS defaults (assumed by wl_set_infra func above) */
	oid->NDIS_infra = Ndis802_11Infrastructure;
	oid->NDIS_auth = Ndis802_11AuthModeOpen;

	/* Assume reclaim image */
	oid->cmn->reclaim = 1;
	oid->cmn->quiet = FALSE;
	oid->cmn->scan_request_pending = FALSE;
	oid->set_ssid_request_pending = FALSE;

	wl_iovar_op(wl, "perm_etheraddr", &wl->wlc->perm_etheraddr, ETHER_ADDR_LEN, IOV_GET);
	wl_iovar_op(wl, "cur_etheraddr", &wl->pub->cur_etheraddr, ETHER_ADDR_LEN, IOV_GET);
	wl_iovar_op(wl, "default_bss_info", &default_bss, sizeof(wl_bss_config_t), IOV_GET);
	wl->wlc->default_bss->atim_window = (uint16)default_bss.atim_window;
	wl->wlc->default_bss->beacon_period = (uint16)default_bss.beacon_period;
	wl->wlc->default_bss->chanspec = (chanspec_t)default_bss.chanspec;

	wl->oid->bsscfg.wlc = wl->wlc;

	/* Get number of bands */
	wl_ioctl(wl, WLC_GET_BANDLIST, bandlist, sizeof(bandlist));
	wl->pub->_nbands = bandlist[0];

	return status;
}

NDIS_STATUS
wl_queryoid(wl_info_t *wl, NDIS_OID oid, PVOID buf, ULONG buflen,
	PULONG bytes_written, PULONG bytes_needed, int ifidx)
{
	/* XXX TODO: added ifidx */
	NDIS_STATUS status = NDIS_STATUS_FAILURE;
#if defined(UNDER_CE)
	if (wl->power_state == NdisDeviceStateD0)
#endif /* UNDER_CE */
		status = wl_query_oid(wl->oid, oid, buf, buflen, bytes_written, bytes_needed);

	return status;
}

NDIS_STATUS
wl_setoid(wl_info_t *wl, NDIS_OID oid, PVOID buf, ULONG buflen,
	PULONG bytes_read, PULONG bytes_needed, int ifidx)
{
	/* XXX TODO: added ifidx */
	NDIS_STATUS status = NDIS_STATUS_FAILURE;
#if defined(UNDER_CE)
	if (wl->power_state == NdisDeviceStateD0)
#endif /* UNDER_CE */
		status = wl_set_oid(wl->oid, oid, buf, buflen, bytes_read, bytes_needed);

	return status;
}

void
dhd_iovar_post_process(wl_info_t *wl, const char *name, void *buf, uint len, bool set)
{
	/* Only cache after oid attach */
	if (wl->wlc && wl->oid) {
		if (!strcmp(name, "ibss_allowed")) {
			wl->wlc->ibss_allowed = *(bool *)buf;
		}
	}
}

void
dhd_ioctl_post_process(wl_info_t *wl, uint cmd, char *buf, uint buflen)
{
	/* Only cache after oid attach */
	if (wl->wlc && wl->oid) {
		switch (cmd) {
			case WLC_SET_INFRA:{
				int infra = *(int *)buf;
				if (!(AP_ENAB(&wl->wlc->pub) && infra == 0)) {
					wl->wlc->default_bss->infra = infra?1:0;
				}

				break;
			}
			default:
				break;
		}
	}
}
#else /* DHD_NDIS_OID */
NDIS_STATUS
wl_queryoid(wl_info_t *wl, NDIS_OID oid, PVOID buf, ULONG buflen,
	PULONG bytes_written, PULONG bytes_needed, int ifidx)
{
	dhd_ioctl_t local_ioc;
	int status;

	memset(&local_ioc, 0, sizeof(local_ioc));
	local_ioc.cmd = oid;
	local_ioc.buf = buf;		/* XXX do we need a maxlen buffer here */
	local_ioc.len = buflen;

	status = dhd_ioctl_entry(dhd_bus_dhd(wl->dhd), ifidx, (char *)&local_ioc);
	if (status != 0) {
#if defined(BCMDONGLEHOST) && defined(EXT_STA)
		/* OID_GEN_STATISTICS (0x20106) is not supported in dongle */
		if (OID_GEN_STATISTICS != oid)
			DHD_TRACE(("wl_queryoid failed 0x%x oid=0x%x, needed=%d\n",
				status, oid, local_ioc.needed));
		else {
			DHD_TRACE(("wl_queryoid: unsupported oid=0x%x\n", oid));
		}
#endif /* defined(BCMDONGLEHOST) && defined(EXT_STA) */

		*bytes_written = 0;
		*bytes_needed = local_ioc.needed;
		DHD_TRACE(("%s: Oid 0x%x status %d failed\n", __FUNCTION__, oid, status));
		return status;
	}

	if (local_ioc.used > buflen)
		*bytes_written = 0;
	else
		*bytes_written = local_ioc.used;
	*bytes_needed = local_ioc.needed;	/* XXX is this set correctly? */

	if (status != 0)
		DHD_TRACE(("%s: oid 0x%x status %d failed\n", __FUNCTION__, oid, status));

	return status;
}

NDIS_STATUS
wl_setoid(wl_info_t *wl, NDIS_OID Oid, PVOID buf, ULONG buflen,
	PULONG bytes_read, PULONG bytes_needed, int ifidx)
{
	dhd_ioctl_t local_ioc;
	int status;

	memset(&local_ioc, 0, sizeof(local_ioc));
	local_ioc.cmd = Oid;
	local_ioc.buf = buf;
	local_ioc.len = buflen;
	local_ioc.set = TRUE;

	status = dhd_ioctl_entry(dhd_bus_dhd(wl->dhd), ifidx, (char *)&local_ioc);
	if (status != 0) {
		DHD_ERROR(("wl_setoid failed = 0x%x Oid=0x%x!\n", status, Oid));
		local_ioc.len = 0;
	}

	*bytes_read = local_ioc.len;
	*bytes_needed = local_ioc.needed;	/* XXX is this set correctly? */

	if (status != 0)
		DHD_TRACE(("%s: Oid 0x%x failed\n", __FUNCTION__, Oid));

	return status;
}
#endif /* DHD_NDIS_OID */
int
wl_iovar_op(wl_info_t *wl, const char *name, void *buf, uint len, bool set)
{
	dhd_ioctl_t local_ioc;
	int iovar_len, tot_len, status;
	char *maxbuf;

	iovar_len = strlen(name) + 1;
	tot_len = iovar_len + len;
	if ((maxbuf = MALLOC(wl->sh.osh, tot_len)) == NULL) {
		DHD_ERROR(("%s: malloc of size %d failed!\n", __FUNCTION__, tot_len));
		return BCME_NOMEM;
	}

	bcm_strcpy_s(maxbuf, tot_len, name);
	memcpy(&maxbuf[iovar_len], buf, len);

	memset(&local_ioc, 0, sizeof(local_ioc));
	local_ioc.cmd = set ? WLC_SET_VAR : WLC_GET_VAR;
	local_ioc.buf = maxbuf;
	local_ioc.len = tot_len;
	local_ioc.set = set;

	/* XXXX Hack should go away when we fix the dhd_WAR44971 for CE */
	if (!strcmp(name, "war44971")) {
		local_ioc.driver = DHD_IOCTL_MAGIC;
		local_ioc.cmd = set ? DHD_SET_VAR : DHD_GET_VAR;
	}

	status = dhd_ioctl_entry(dhd_bus_dhd(wl->dhd), 0, (char *)&local_ioc);

	if (!set && (status == BCME_OK)) {
		bcopy(maxbuf, buf, len);
	}
#ifdef DHD_NDIS_OID
	if (status == BCME_OK) {
		dhd_iovar_post_process(wl, name, buf, len, set);
	}
#endif /* DHD_NDIS_OID */
	MFREE(wl->sh.osh, maxbuf, tot_len);
	return status;
}

int
wl_ioctl(wl_info_t *wl, uint cmd, char *buf, uint buflen)
{
	dhd_ioctl_t local_ioc;
	int status;

	memset(&local_ioc, 0, sizeof(local_ioc));
	local_ioc.cmd = cmd;
	local_ioc.buf = buf;
	local_ioc.len = buflen;
	if (cmd == WLC_SET_VAR)
		local_ioc.set = TRUE;
	status = dhd_ioctl_entry(dhd_bus_dhd(wl->dhd), 0, (char *)&local_ioc);
#ifdef DHD_NDIS_OID
	if (status == 0) {
		dhd_ioctl_post_process(wl, cmd, buf, buflen);
	}
#endif /* DHD_NDIS_OID */
	return status;
}

int
dhd_bus_ioctl(wl_info_t *wl, int oid, char *buf, uint buflen,
	PULONG bytes_read, PULONG bytes_needed, bool set)
{
	dhd_ioctl_t local_ioc;
	int ret;

	memset(&local_ioc, 0, sizeof(local_ioc));
	local_ioc.cmd = oid;
	local_ioc.buf = buf;
	local_ioc.len = buflen;
	local_ioc.set = set;
	local_ioc.driver = DHD_IOCTL_MAGIC;

	ret = dhd_ioctl_entry(dhd_bus_dhd(wl->dhd), 0, (char *)&local_ioc);

	*bytes_read = local_ioc.len;
	*bytes_needed = local_ioc.needed;	/* XXX is this set correctly? */

	return ret;
}

static void
dhd_dpc(void *arg)
{
	dhd_info_t *dhd = (dhd_info_t *)arg;
	wl_info_t *wl = dhd->wl;
	struct dhd_bus *bus = dhd->pub.bus;

#ifndef BCMDBUS
	g_in_dpc = TRUE;
	if (g_mhalted == FALSE) {
		if (dhd_bus_dpc(bus))
		shared_dpc_schedule(&wl->sh);
	}

	wl_sendcomplete(wl);
	g_in_dpc = FALSE;
#endif /* BCMDBUS */

}

void
dhd_sched_dpc(dhd_pub_t *dhdp)
{
#if defined(UNDER_CE) && defined(EXT_STA)
	/* XXXX ???
	 * Scheduling the DPC here is very sensitive
	 * to the thread priority of DPC thread. If the
	 * priority is high enough then receive doesn't work
	 * properly
	 */
	dhd_info_t *dhd = (dhd_info_t *)dhdp;
	wl_info_t *wl = dhd->wl;

	/* For CE back round thread schedules the DPC */
	shared_dpc_schedule(&wl->sh);
#endif
}

uint
wl_getstat(wl_info_t *wl, NDIS_OID Oid)
{
	dhd_pub_t *dhdp = dhd_bus_pub(wl->dhd);

	if (dhdp->up) {
		/* Use the protocol to get dongle stats */
		dhd_prot_dstats(dhdp);
	}

	switch (Oid) {
	case OID_GEN_XMIT_OK:
		return dhdp->dstats.tx_packets;

	case OID_GEN_RCV_OK:
		return dhdp->dstats.rx_packets;

	case OID_GEN_XMIT_ERROR:
		return dhdp->dstats.tx_errors;

	case OID_GEN_RCV_ERROR:
		return dhdp->dstats.rx_errors;

	default:
		return 0;
	}
}

void
wl_link_up(wl_info_t *wl)
{
#ifndef EXT_STA
	wl_indicate_link_up(wl);
#endif /* EXT_STA */
}

void
wl_link_down(wl_info_t *wl)
{
#ifndef EXT_STA
	wl_indicate_link_down(wl);
#endif /* EXT_STA */
}

int
wl_up(wl_info_t *wl)
{
	/* In case of dongle control for when wl up should happen. This allows
	   some settings like wme, country code etc. to be performed before the
	   wl up in case of a reclaim image. Mainly used for testing, controlled
	   via "AllowIntfUp" registry entry
	*/
	if (TRUE == wl->AllowIntfUp) {
		uint up = 1;
		int err;
		err = wl_ioctl(wl, WLC_UP, (char *)&up, sizeof(up));
#if defined(BCMDHDUSB)
		/* Retry once more */
		if (err) {
			err = wl_ioctl(wl, WLC_UP, (char *)&up, sizeof(up));
		}
#endif /* BCMDHDUSB */
		return err;
	}

	return 0;
}

void
wl_down(wl_info_t *wl)
{
	uint down = 1;
	dhd_info_t *dhd = dhd_bus_dhd(wl->dhd);

	if (dhd && dhd->pub.dongle_isolation == FALSE)
		wl_ioctl(wl, WLC_DOWN, (char *)&down, sizeof(down));
	wl_flushtxq(wl, NULL);
	wl_sendcomplete(wl);
}

int
wl_reboot(wl_info_t *wl)
{
	dhd_info_t *dhd = dhd_bus_dhd(wl->dhd);

	if (dhd && dhd->pub.dongle_isolation == FALSE)
		return wl_ioctl(wl, WLC_REBOOT, NULL, 0);
	else
		return 0;
}

/*
 * called by NDIS during restart/shutdown/crash, driver maybe in any state
 * need to put chip in known state and other hardware in clean state if possible
 */
void
wl_shutdown(wl_info_t *wl)
{
	/* XXX need to shutdown the dongle */
	dhd_info_t *dhd = dhd_bus_dhd(wl->dhd);

	if (dhd && dhd->pub.dongle_isolation == TRUE)
		return;
}

#ifdef UNDER_CE
int
dhd_mPnPeventnotify(wl_info_t *wl, PNET_DEVICE_PNP_EVENT NetDevicePnPEvent)
{
	NDIS_DEVICE_PNP_EVENT  PnPEvent = NetDevicePnPEvent->DevicePnPEvent;
	PVOID InformationBuffer = NetDevicePnPEvent->InformationBuffer;
	dhd_info_t *dhd = dhd_bus_dhd(wl->dhd);

	DHD_TRACE(("Entered %s\n", __FUNCTION__));

	switch (PnPEvent) {
	case NdisDevicePnPEventSurpriseRemoved:
		break;
	case NdisDevicePnPEventPowerProfileChanged:
		if (*((ULONG*)InformationBuffer) == NdisPowerProfileBattery) {
			/* Switch to Battery timer mode for power saving */
			DHD_TRACE(("%s: NdisPowerProfile - Battery: Watchdog timer %d -> %d\n",
				__FUNCTION__, dhd_watchdog_ms, dhd_watchdog_battery_ms));
			dhd_watchdog_ms = dhd_watchdog_battery_ms;
		} else {
			/* Enble maximum performance NdisPowerProfileAcOnline */
			DHD_TRACE(("%s: NdisPowerProfile - Ac: Watchdog timer %d -> %d\n",
				__FUNCTION__, dhd_watchdog_ms, dhd_watchdog_ac_ms));
			dhd_watchdog_ms = dhd_watchdog_ac_ms;
		}
		break;
	default:
		DHD_ERROR(("%s: PnPEventNotify: unknown PnP event %x \n", __FUNCTION__, PnPEvent));
		break;
	}

	return 0;
}

#define IOCTL_GET_WLAN_PM_STS	CTL_CODE(FILE_DEVICE_HAL, 5005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_WLAN_PM_STS	CTL_CODE(FILE_DEVICE_HAL, 5006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#if defined(NDIS60)
bool
wl_pnp_reinit_ndis(wl_info_t *wl)
{
	/* Need to explicitly register for events in dongle mode */
	/* by default want to see events MIC_ERROR, NDIS_LINK and PMKID_CACHE */
	char eventmask[WL_EVENTING_MASK_LEN];
	HKEY confighandle = NULL;
	int infra;
	NDIS_STATUS status;
	shared_info_t *sh = &wl->sh;
	bool ret = FALSE;
	int dhd_glom;
	int rc = 0;

	dhd_info_t *dhd = dhd_bus_dhd(wl->dhd);
	dhd_prot_init(&dhd->pub);
	dhd_glom = 0;
	rc = wl_iovar_op(wl, "bus:txglom", &dhd_glom, sizeof(dhd_glom), IOV_SET);
	wl_ndis_extsta_register_events(eventmask);
	rc = wl_iovar_op(wl, "event_msg", eventmask, sizeof(eventmask), IOV_SET);
	infra = 1;
	wl_ioctl(wl, WLC_SET_INFRA, (char *)&infra, sizeof(infra));

	/* avoid premature wlc_up() in wl_readconfigoverrides() */
	wl_iovar_setint(wl, "down_override", TRUE);

	/* open the registry */
	if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Comm\\BCMSDDHD1\\Parms",
		0, KEY_ALL_ACCESS, &confighandle)) != ERROR_SUCCESS) {
		DHD_ERROR(("BCMSDLDR:Failed to open path %s; %d \n",
			"\\Comm\\BCMSDDHD1\\Parms", status));
		goto error;
	}

	wl->context = REG_ACCESS_POWER_ON_OFF;
	/* read reg entries that may override defaults set in wlc_attach() */
	wl_readconfigoverrides((void *) wl, confighandle, sh->id, sh->unit, sh->OS);
	wl_scanoverrides(wl, confighandle, sh->id, sh->unit);
	wl_read_auto_config(wl);

	RegCloseKey(confighandle);

	ND_LOCK(wl);

	/*
	   Initialize chip. This could be dummy call in case of dongle,
	   if "AllowIntfUp" registry entry is set to '1'
	*/
	if (wl_up(wl)) {
		ND_UNLOCK(wl);
		DHD_ERROR(("wl%d: %s: wl_up error 0x%x\n", wl->sh.unit, __FUNCTION__, status));
		goto error;
	}

	ND_UNLOCK(wl);

	/* permits up onwards */
	wl_iovar_setint(wl, "down_override", FALSE);

	ret = TRUE;

error:
	return ret;
}

NDIS_STATUS
wl_pnp_set_power_reset(wl_info_t *wl, int devicestate)
{
	BOOL ret = FALSE;
	DWORD outBuf;
	DWORD byteReturned;
	dhd_info_t *dhd;
	DWORD inBuf = 1;
	int wlanoff_val = 0;
	bool canceled;
	char buf[20] = "disassoc";
	uint glom_mode = 0;
	HKEY confighandle = NULL;
	NDIS_STATUS status;
	PNDIS_CONFIGURATION_PARAMETER param;

	/* Platform specific */
	HANDLE h = CreateFile(wl->sdHostCtrlName, 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	dhd = dhd_bus_dhd(wl->dhd);

	switch (devicestate)
	{
	case NdisDeviceStateD0:
		DHD_ERROR(("**** BCMSDDHD off==>on ****\n"));

		/* Platform specific */

		inBuf = 1;
		if (h != INVALID_HANDLE_VALUE) {
			ret = DeviceIoControl(h,
				IOCTL_SET_WLAN_PM_STS,
				&inBuf,
				sizeof(DWORD),
				&outBuf,
				sizeof(DWORD),
				&byteReturned,
				NULL);
			if (!ret)
				DHD_ERROR(("wl%d: %s ERROR : IOCTL_GET_WLAN_STATUS return FALSE\n",
				wl->sh.unit, __FUNCTION__));
		}

		/* May need some delay based on customer platform */
		Sleep(200);

		/* Need to read the register for new firmware again */
		wl->context = REG_ACCESS_POWER_ON_OFF;
		if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Comm\\BCMSDDHD1\\Parms",
			0, KEY_ALL_ACCESS, &confighandle)) != ERROR_SUCCESS) {
			DHD_ERROR(("BCMSDLDR:Failed to open path %s; %d \n",
				"\\Comm\\BCMSDDHD1\\Parms", status));
		} else {
			/* Proceed in reading registry */
			if ((param = wl_readparam(dhd_configs, "TestMode",
			                          confighandle, wl, NULL))) {
				mfgtest_mode = param->ParameterData.IntegerData;
			} else {
				mfgtest_mode = 0; /* Production by default */
			}
			DHD_ERROR(("TestMode set to %d from the registry\n", mfgtest_mode));

			if (mfgtest_mode) {
				/* Load mfgtest image */
				 if ((param = wl_readparam(dhd_configs, "MfgtestDongleImagePath",
				                           confighandle, wl, NULL))) {
					wchar2ascii(fw_path, param->ParameterData.StringData.Buffer,
						param->ParameterData.StringData.Length,
						sizeof(fw_path));
				 }
			} else {
				/* Load production image */
				if ((param = wl_readparam(dhd_configs, "DongleImagePath",
				                          confighandle, wl, NULL))) {

					wchar2ascii(fw_path, param->ParameterData.StringData.Buffer,
						param->ParameterData.StringData.Length,
						sizeof(fw_path));
				}
			}
			DHD_ERROR(("firmware path set to %s\n", fw_path));
			RegCloseKey(confighandle);
		}

		if (bcmsdh_reenumerate(NULL) == FALSE) {
			DHD_ERROR(("wl%d: Failed re-enumeration\n", wl->sh.unit, __FUNCTION__));
		}

		wlanoff_val = 0;
		wl_iovar_op(wl, "devreset", &wlanoff_val, sizeof(wlanoff_val), IOV_SET);

		dhd_os_wd_timer(dhd, dhd_watchdog_ms);

		Sleep(1000);
		/* Re initilize NDIS related setting from registry and other IOCTLS */
		if (wl_pnp_reinit_ndis(wl) == FALSE) {
			DHD_ERROR(("wl%d: Failed re-initilize NDIS\n", wl->sh.unit, __FUNCTION__));
		}

		break;

	case NdisDeviceStateD3:
		DHD_ERROR(("**** BCMSDDHD on==>off ****\n"));

		if (wl->in_scan) {
			/* In case of a pending scan request:
			*  free the scan buffer,
			*  abort the scan
			*  and send a failed scan complete notification to NDIS
			*/
			dhd_iscan_request(wl->dhd, WL_SCAN_ACTION_ABORT);
			dhd_ind_scan_confirm(wl->dhd, FALSE);
		}

		/* Proactively disassoc */
		wl_ioctl(wl, WLC_DISASSOC, 0, 0);
		/* May need sleep to quench stray timer, even though we killed the timer */
		Sleep(1000);

		/* Make sure we kill the timer after some sleep so that
		 * watchdog timer will get chance to trun off HT back
		 * plane clock to save power while Wi-Fi is turned of
		 */
		dhd_os_wd_timer(dhd, 0);
		dhd_bus_watchdog(&dhd->pub);

		wlanoff_val = 1;
		wl_iovar_op(wl, "devreset", &wlanoff_val, sizeof(wlanoff_val), IOV_SET);

		/* Platform specific */
		if (h != INVALID_HANDLE_VALUE) {
			inBuf = 0;
			ret = DeviceIoControl(h,
				IOCTL_SET_WLAN_PM_STS,
				&inBuf,
				sizeof(DWORD),
				&outBuf,
				sizeof(DWORD),
				&byteReturned,
				NULL);
			if (!ret)
				DHD_ERROR(("wl%d: %s ERROR : IOCTL_GET_WLAN_STATUS return FALSE\n",
					wl->sh.unit, __FUNCTION__));
		}
		break;
	}

	if (h) {
		CloseHandle(h);
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
wl_pnp_set_power_normal(wl_info_t *wl, int devicestate)
{
	dhd_info_t *dhd;
	bool canceled;

	dhd = dhd_bus_dhd(wl->dhd);

	switch (devicestate) {
	case NdisDeviceStateD0:
		DHD_ERROR(("**** BCMSDDHD off==>on ****\n"));

		dhd_os_wd_timer(dhd, dhd_watchdog_ms);

		break;

	case NdisDeviceStateD3:
		DHD_ERROR(("**** BCMSDDHD on==>off ****\n"));

		/* Proactively disassoc */
		wl_ioctl(wl, WLC_DISASSOC, 0, 0);

		/* Make sure we kill the timer after some sleep so that
		 * watchdog timer will get chance to trun off HT back
		 * plane clock to save power while Wi-Fi is turned of
		 */
		dhd_os_wd_timer(dhd, 0);
		dhd_bus_watchdog(&dhd->pub);

		/* Flush iscan results */
		dhd_iscan_free_buf(wl->dhd, 0);

		break;
	}
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
wl_pnp_set_power(wl_info_t *wl, int devicestate)
{
	if (wl->PNPMethod == PNP_NORMAL)
		return wl_pnp_set_power_normal(wl, devicestate);
	else
		return wl_pnp_set_power_reset(wl, devicestate);
}
#else /* NDIS60 */
bool
wl_pnp_reinit_ndis(wl_info_t *wl)
{
	/* Need to explicitly register for events in dongle mode */
	/* by default want to see events MIC_ERROR, NDIS_LINK and PMKID_CACHE */
	char eventmask[WL_EVENTING_MASK_LEN];
	char iovbuf[WL_EVENTING_MASK_LEN + 12];	/* Room for "event_msgs" + '\0' + bitvec */
	HKEY confighandle = NULL;
	int infra;
	NDIS_STATUS status;
	shared_info_t *sh = &wl->sh;
	bool ret = FALSE;
#ifndef WL_IW_USE_ISCAN
	ulong bytes_needed, bytes_read;
#endif /* !WL_IW_USE_ISCAN */
	bcm_mkiovar("event_msgs", "", 0, iovbuf, sizeof(iovbuf));
	wl_ioctl(wl, WLC_GET_VAR, iovbuf, sizeof(iovbuf));
	bcopy(iovbuf, eventmask, WL_EVENTING_MASK_LEN);
#if defined(WL_IW_USE_ISCAN)
	setbit(eventmask, WLC_E_SCAN_COMPLETE);
#endif
#ifdef DHD_NDIS_OID
	setbit(eventmask, WLC_E_QUIET_START);
	setbit(eventmask, WLC_E_QUIET_END);
	setbit(eventmask, WLC_E_BEACON_RX);
	setbit(eventmask, WLC_E_SET_SSID);
	setbit(eventmask, WLC_E_LINK);
#endif /* DHD_NDIS_OID */
	setbit(eventmask, WLC_E_MIC_ERROR);
	setbit(eventmask, WLC_E_NDIS_LINK);
	setbit(eventmask, WLC_E_PMKID_CACHE);
	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	wl_ioctl(wl, WLC_SET_VAR, iovbuf, sizeof(iovbuf));

	/* default infrastructure mode is Infrastructure */
	infra = 1;
	wl_ioctl(wl, WLC_SET_INFRA, (char *)&infra, sizeof(infra));
#ifdef DHD_NDIS_OID
	dhd_oid_init(wl);
#endif /* DHD_NDIS_OID */
	/* avoid premature wlc_up() in wl_readconfigoverrides() */
	wl_iovar_setint(wl, "down_override", TRUE);

	/* open the registry */
	if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Comm\\BCMSDDHD1\\Parms",
		0, KEY_ALL_ACCESS, &confighandle)) != ERROR_SUCCESS) {
		DHD_ERROR(("BCMSDLDR:Failed to open path %s; %d \n",
			"\\Comm\\BCMSDDHD1\\Parms", status));
		goto error;
	}

	wl->context = REG_ACCESS_POWER_ON_OFF;
	/* read reg entries that may override defaults set in wlc_attach() */
	wl_readconfigoverrides((void *) wl, confighandle, sh->id, sh->unit, sh->OS);
	wl_scanoverrides(wl, confighandle, sh->id, sh->unit);

	/* read the autoconnect configuration params */
	wl_read_auto_config(wl, confighandle, sh->adapterhandle);

	RegCloseKey(confighandle);

	ND_LOCK(wl);

	/*
	   Initialize chip. This could be dummy call in case of dongle,
	   if "AllowIntfUp" registry entry is set to '1'
	*/
	if (wl_up(wl)) {
		ND_UNLOCK(wl);
		DHD_ERROR(("wl%d: %s: wl_up error 0x%x\n", wl->sh.unit, __FUNCTION__, status));
		goto error;
	}

	ND_UNLOCK(wl);

	/* permits up onwards */
	wl_iovar_setint(wl, "down_override", FALSE);

	/* Need to issue this as opportunistic reqest to scan, as in case of power on/off cycle
	   we find some time NDIS layer send the first OID_802_11_BSSID_LIST_SCAN request
	   during the off -> on transition very late which results in network "Unavailable" message
	*/
#ifndef WL_IW_USE_ISCAN
	wl_setoid(wl, OID_802_11_BSSID_LIST_SCAN, NULL, 0, &bytes_read, &bytes_needed, NULL);
#endif /* !WL_IW_USE_ISCAN */
	ret = TRUE;

error:
	return ret;
}

#define IOCTL_GET_WLAN_PM_STS	CTL_CODE(FILE_DEVICE_HAL, 5005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_WLAN_PM_STS	CTL_CODE(FILE_DEVICE_HAL, 5006, METHOD_BUFFERED, FILE_ANY_ACCESS)

NDIS_STATUS
wl_pnp_set_power(wl_info_t *wl, int devicestate)
{
	BOOL ret = FALSE;
	DWORD outBuf;
	DWORD byteReturned;
	dhd_info_t *dhd;
	DWORD inBuf = 1;
	int wlanoff_val = 0;
	bool canceled;
	char buf[20] = "disassoc";

	HANDLE h = CreateFile(wl->sdHostCtrlName, 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	dhd = dhd_bus_dhd(wl->dhd);
	wl->power_state = devicestate;
	switch (devicestate) {
	case NdisDeviceStateD0:
		DHD_ERROR(("**** BCMSDDHD off==>on ****\n"));
		dhd->ref_count++;
		if (dhd->ref_count != 1) {
			break;
		}
		inBuf = 1;
		if (h != INVALID_HANDLE_VALUE) {
			ret = DeviceIoControl(h,
				IOCTL_SET_WLAN_PM_STS,
				&inBuf,
				sizeof(DWORD),
				&outBuf,
				sizeof(DWORD),
				&byteReturned,
				NULL);
			if (!ret)
				DHD_ERROR(("wl%d: %s ERROR : IOCTL_GET_WLAN_STATUS return FALSE\n",
				wl->sh.unit, __FUNCTION__));
		}
		/* May need some delay based on customer platform */
		Sleep(200);

		if (bcmsdh_reenumerate(NULL) == FALSE)
			DHD_ERROR(("wl%d: Failed re-enumeration\n", wl->sh.unit, __FUNCTION__));

		wlanoff_val = 0;
		wl_iovar_op(wl, "devreset", &wlanoff_val, sizeof(wlanoff_val), IOV_SET);

		dhd_os_wd_timer(dhd, dhd_watchdog_ms);

		/* Re initialize NDIS related setting from registry and other IOCTLS */
		if (wl_pnp_reinit_ndis(wl) == FALSE)
			DHD_ERROR(("wl%d: Failed re-initialize NDIS\n", wl->sh.unit, __FUNCTION__));
#if defined(WL_IW_USE_ISCAN)
		/* Reinitialize iscan state */
		wl->iscan->timer_on = 0;
		wl->iscan->iscan_state = ISCAN_STATE_IDLE;
		wl->iscan->timer_retry_cnt = ISCAN_RETRY_CNT;
#endif /* WL_IW_USE_ISCAN */
#ifdef WLBTAMP
#ifdef WLBTCEAMP11
		BtKernStart(wl);
#endif
#endif /* WLBTAMP */
		break;

	case NdisDeviceStateD3:
		DHD_ERROR(("**** BCMSDDHD on==>off ****\n"));
		if (dhd->ref_count == 0)
			break;
		else
			dhd->ref_count--;
		if (dhd->ref_count != 0) {
			break;
		}
#ifdef WLBTAMP
#ifdef WLBTCEAMP11
		BtKernStop(wl);
#endif
#endif /* WLBTAMP */
		/* Proactively disassoc */
		wl_ioctl(wl, WLC_DISASSOC, 0, 0);

		/* Indicate link down */
		wl_link_down(wl);

		if (dhd->wd_timer_valid == TRUE) {
			NdisMCancelTimer(&dhd->h_wd_timer, &canceled);
			dhd->wd_timer_valid = FALSE;
		}
#ifdef WL_IW_USE_ISCAN
		if (wl->iscan->timer_on) {
			wl_iscan_del_timer(wl->iscan->timer);
			wl->iscan->timer_on = 0;
			wl->iscan->iscan_state = ISCAN_STATE_IDLE;
		}
#endif /* WL_IW_USE_ISCAN */
		/* May need sleep to quench stray timer, even though we killed the timer */
		Sleep(1000);

		wlanoff_val = 1;
		wl_iovar_op(wl, "devreset", &wlanoff_val, sizeof(wlanoff_val), IOV_SET);

		inBuf = 0;
		if (h != INVALID_HANDLE_VALUE) {
			ret = DeviceIoControl(h,
				IOCTL_SET_WLAN_PM_STS,
				&inBuf,
				sizeof(DWORD),
				&outBuf,
				sizeof(DWORD),
				&byteReturned,
				NULL);
			if (!ret)
				DHD_ERROR(("wl%d: %s ERROR : IOCTL_GET_WLAN_STATUS return FALSE\n",
					wl->sh.unit, __FUNCTION__));
		}
		break;
	}

	if (h)
		CloseHandle(h);

	return NDIS_STATUS_SUCCESS;
}
#endif /* NDIS60 */
#else /* UNDER_CE */
/* DHD specific PNP processing */
NDIS_STATUS
wl_pnp_set_power(wl_info_t *wl, int devicestate)
{
	dhd_pub_t *dhdp = dhd_bus_pub(wl->dhd);
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;
	int reloaded = 0;
#if defined(BCMDHDUSB)
	switch (devicestate) {
	case NdisDeviceStateD1:
	case NdisDeviceStateD2:
	case NdisDeviceStateD3: /* Sleep */
		dbus_pnp_sleep(dhdp->dbus);
		/* Call common pnp handler */
		status = wl_shared_pnp_set_power(wl, devicestate, TRUE);
		break;

	case NdisDeviceStateD0: /* Normal */
		dbus_pnp_resume(dhdp->dbus, &reloaded);
		if (reloaded)
		{
			reloaded = 1;
			DHD_ERROR(("%s: Dongle image reloaded\n", __FUNCTION__));
#ifdef EXT_STA
			wl_reload_init(wl);
#endif
			wl_up(wl);
#ifdef EXT_STA
			wl_ndis_reassoc(wl);
#endif
		} else {
			DHD_ERROR(("%s: Image not reloaded on resume, power not reset\n",
			           __FUNCTION__));
			reloaded = FALSE;
		}
		if (!reloaded) {
			wl_shared_pnp_set_power(wl, devicestate, 1);
		}
		break;

	default:
		break;
	}
#endif /* BCMDHDUSB */
	return status;
}
#endif /* UNDER_CE */

#ifdef EXT_STA
struct wl_timer *
wl_init_timer(wl_info_t *wl, void (*fn)(void* arg), void* arg, const char *name)
{
	struct wl_timer *t;

	ASSERT(wl->ntimers < MAX_TIMERS);

	t = &wl->timer[wl->ntimers++];

	bzero(t, sizeof(struct wl_timer));
	t->wl = wl;
	t->fn = fn;
	t->arg = arg;

#if 0
	if ((t->name = MALLOC(wl->sh.osh, strlen(name) + 1)))
		bcm_strcpy_s(t->name, strlen(name) + 1, name);
#endif

	NdisMInitializeTimer(&t->ndis_timer, wl->sh.adapterhandle,
		wl_timer, (PVOID)t);

	return t;
}

void
wl_add_timer(wl_info_t *wl, struct wl_timer *t, uint ms, int periodic)
{
	ASSERT(t >= &wl->timer[0]);
	ASSERT(t < &wl->timer[MAX_TIMERS]);

	ASSERT(t->fn);

	/* add same timer twice is not supported */
	/* XXX ASSERT(!t->set);  */
	/* XXX Removed this due the addition of mid-txq, which currently
	  * might call add_timer several times before it actually gets to run
	  * TODO: Add a flag so we don't call add_timer multiple times and
	  * can reinstate this ASSERT
	  */

	/* do not check driver "up" for timer to be added when driver is "down" */
	t->periodic = periodic;
	if (!t->set) {
		t->set = TRUE;
		if (periodic) {
			NdisMSetPeriodicTimer(&t->ndis_timer, ms);
			WL_TRACE(("Set periodic timer \n"));
		} else {
			NdisMSetTimer(&t->ndis_timer, ms);
			WL_TRACE(("Set timer \n"));
		}
	} else {
		WL_TRACE(("Timer already set\n"));
	}
}

bool
wl_del_timer(wl_info_t *wl, struct wl_timer *t)
{
	bool canceled;

	ASSERT(t >= &wl->timer[0]);
	ASSERT(t < &wl->timer[MAX_TIMERS]);

	ASSERT(t->fn);

	canceled = TRUE;

	if (t->set) {
		NdisMCancelTimer(&t->ndis_timer, &canceled);
		t->set = FALSE;
		t->periodic = FALSE;
	}

#if 0
	if (!canceled)
		WL_INFORM(("wl%d: Failed to delete timer %s\n", wl->pub->unit, t->name));
#endif

	return (canceled);
}

void
wl_free_timer(wl_info_t *wlh, struct wl_timer *timer)
{
	/* NOOP for windows. */
}

static void
wl_timer(
	IN PVOID systemspecific1,
	IN NDIS_HANDLE context,
	IN PVOID systemspecific2,
	IN PVOID systemspecific3
)
{
	struct wl_timer *t;
	wl_info_t *wl;

	t = (struct wl_timer *)context;
	wl = t->wl;

	ND_LOCK(wl);

	if (wl->callbacks)
		wl->callbacks--;

	if (t->set && !wl->timer_stop) {
		if (!t->periodic)
			t->set = FALSE;

		(*t->fn)(t->arg);
	}

	ND_UNLOCK(wl);
}
#endif /* EXT_STA */

#ifndef UNDER_CE
int
dhd_os_acquire_mutex(PKMUTEX mutex)
{
	NTSTATUS ntStatus;
	LARGE_INTEGER timeout;
	PLARGE_INTEGER ptimeout = &timeout;

	if (KeGetCurrentIrql() == DISPATCH_LEVEL) {
		/* If attaching at DISPATCH_LEVEL, do not specify timeout
		 * since it can not sleep.
		 */
		bzero(&timeout, sizeof(timeout));
	} else {
		timeout.QuadPart = -10000 * 7000;
	}

	ntStatus = KeWaitForMutexObject(mutex,
		Executive,
		KernelMode,
		FALSE,
		ptimeout);

	if (ntStatus != STATUS_SUCCESS) {
		DHD_ERROR(("ERROR: acquire mutex failed=0x%x\n", ntStatus));
		return 0;
	}

	return 1;
}

int
dhd_os_release_mutex(PKMUTEX mutex)
{
	KeReleaseMutex(mutex, FALSE);
	return 0;
}
#endif /* UNDER_CE */

/*
 * OS specific functions required to implement DHD driver in OS independent way
 */
int
dhd_os_proto_block(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	if (dhd) {
#ifdef UNDER_CE
		EnterCriticalSection(&dhd->proto_sem);
#else
		return (dhd_os_acquire_mutex(&dhd->proto_sem));
#endif /* UNDER_CE */
	}

	return 1;
}

#if defined(EXT_STA) && !defined(UNDER_CE)
/* This function exists because PR55307 (Support DHD/USB Vista roaming_cmn when USB port power
 * is cut off) has not been implemented yet. This function is called when the power is cut to the
 * USB device during standby/hibernate. The mechanism is to fake a roaming start indication and
 * force NDIS to timeout and restart the association in the prior context in order to populate
 * the wlc state, which was blown away during power off.
*/
static NDIS_STATUS
wl_ndis_reassoc(wl_info_t *wl)
{
	DOT11_ROAMING_START_PARAMETERS StatusBuffer;
	ASSERT(wl);

	if (!wl->pub->associated)
		return NDIS_STATUS_SUCCESS;

	WL_ASSOC(("%s: Faking NDIS_STATUS_DOT11_ROAMING_START to restore wlc state to the dongle\n",
	          __FUNCTION__));

	bzero(&StatusBuffer, sizeof(DOT11_ROAMING_START_PARAMETERS));
	StatusBuffer.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	StatusBuffer.Header.Revision = DOT11_ROAMING_START_PARAMETERS_REVISION_1;
	StatusBuffer.Header.Size = sizeof(DOT11_ROAMING_START_PARAMETERS);
	StatusBuffer.uRoamingReason =  DOT11_ASSOC_STATUS_ROAMING_ASSOCIATION_LOST;

	/* XXX SHADY COME UP WITH SOMETHING BETTER */
	shared_indicate_status(wl->sh.adapterhandle, 0, NDIS_STATUS_DOT11_ROAMING_START,
		&StatusBuffer, sizeof(StatusBuffer));

	return NDIS_STATUS_SUCCESS;
}
#endif /* EXT_STA && !UNDER_CE */

int
dhd_os_proto_unblock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	if (dhd) {
#ifdef UNDER_CE
		LeaveCriticalSection(&dhd->proto_sem);
#else
		dhd_os_release_mutex(&dhd->proto_sem);
#endif /* UNDER_CE */
	}

	return 1;
}

unsigned int
dhd_os_get_ioctl_resp_timeout(void)
{
	return ((unsigned int)dhd_ioctl_timeout_msec);
}

void
dhd_os_set_ioctl_resp_timeout(unsigned int timeout_msec)
{
	dhd_ioctl_timeout_msec = (unsigned long)timeout_msec;
}

int
dhd_os_ioctl_resp_wait(dhd_pub_t *pub, uint *condition, bool *pending)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	int ret;

	* pending = FALSE;

#ifdef UNDER_CE
	if (dhd) {
		ret = WaitForSingleObject(dhd->ioctl_resp_wait, dhd_ioctl_timeout_msec);
		if (ret == WAIT_TIMEOUT)
			return 0;
		else
			return 1;
	}
#else /* UNDER_CE */
	if (dhd) {
		unsigned long timeout;

		if (*condition)
			return 1;

		timeout = dhd_ioctl_timeout_msec;
		NdisResetEvent(&dhd->ioctl_resp_wait);

#ifdef BCMSLTGT
		timeout *= htclkratio;
#endif /* BCMSLTGT */

		if (TRUE == NdisWaitEvent(&dhd->ioctl_resp_wait, timeout))
			return 1;
		else
			return 0;
	}
#endif /* UNDER_CE */

	return -1;
}

int
dhd_os_ioctl_resp_wake(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;

#ifdef UNDER_CE
	if (dhd) {
		ReleaseSemaphore(dhd->ioctl_resp_wait, 1, 0);
		return 0;
	}
#else /* UNDER_CE */
	if (dhd) {
		NdisSetEvent(&dhd->ioctl_resp_wait);

		return 0;
	}
#endif /* UNDER_CE */

	return -1;
}

void
dhd_os_wd_timer(void *dhdh, uint wdtick)
{
	bool canceled;
	dhd_info_t *dhd = dhdh;
	static uint save_dhd_watchdog_ms = 0;
	static bool first = TRUE;

	/* don't start the wd until fw is loaded */
	if (dhd->pub.busstate == DHD_BUS_DOWN)
		return;

	if (first) {
		NdisMInitializeTimer(&dhd->h_wd_timer, ((wl_info_t *)dhd->wl)->sh.adapterhandle,
			dhd_watchdog, (PVOID)dhd);
		first = FALSE;
	}

	/* Totally stop the timer */
	if (!wdtick && dhd->wd_timer_valid == TRUE) {
		NdisMCancelTimer(&dhd->h_wd_timer, &canceled);

		dhd->wd_timer_valid = FALSE;
		save_dhd_watchdog_ms = wdtick;
		return;
	}

	if (wdtick) {
		dhd_watchdog_ms = wdtick;
		if (save_dhd_watchdog_ms != dhd_watchdog_ms) {

			if (dhd->wd_timer_valid == TRUE)
				/* Stop timer and restart at new value */
				NdisMCancelTimer(&dhd->h_wd_timer, &canceled);

			/* Create timer again when watchdog period is
			   dynamically changed or in the first instance
			*/
			NdisMSetTimer(&dhd->h_wd_timer, wdtick);
		} else {
			/* Re arm the timer, at last watchdog period */
			NdisMSetTimer(&dhd->h_wd_timer, wdtick);
		}

		dhd->wd_timer_valid = TRUE;
		save_dhd_watchdog_ms = wdtick;
	}
}

void *
dhd_os_open_image(char *filename)
{
#ifdef UNDER_CE
	FILE *fr = NULL;

	fr = fopen(filename, "rb");
	if (fr == NULL) {
		DHD_ERROR(("%s: Can't open image file %s\n", __FUNCTION__, filename));
	}

	return fr;
#else /* UNDER_CE */
	HANDLE fr = 0;
	IO_STATUS_BLOCK iostatus;
	OBJECT_ATTRIBUTES obj_att;
	ANSI_STRING	ansi_file;
	UNICODE_STRING	uni_file;
	NTSTATUS status;

	NdisZeroMemory(&ansi_file, sizeof(ansi_file));
	NdisInitAnsiString(&ansi_file, filename);

	NdisZeroMemory(&uni_file, sizeof(uni_file));
	uni_file.MaximumLength = (ansi_file.Length + 1) * 2;

	if (NDIS_STATUS_SUCCESS == NdisAllocateMemory(&uni_file.Buffer,
		uni_file.MaximumLength, 0, -1)) {

		NdisAnsiStringToUnicodeString(&uni_file, &ansi_file);

		InitializeObjectAttributes(&obj_att, &uni_file,
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		if ((status = ZwCreateFile(&fr, GENERIC_READ, &obj_att, &iostatus, NULL,
			FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			0, 0)) != STATUS_SUCCESS)
			DHD_ERROR(("%s: Can't open file %s, status = 0x%x\n",
				__FUNCTION__, filename, status));

		NdisFreeMemory(uni_file.Buffer, (ansi_file.Length + 1) * 2, 0);
	}

	return fr;
#endif /* UNDER_CE */
}

int
dhd_os_get_image_block(char *buf, int len, void *image)
{
	int ret = 0;

#ifdef UNDER_CE
	FILE *fr = (FILE *)image;

	if (fr)
		ret = fread(buf, sizeof(uint8), len, fr);

	return ret;

#else /* UNDER_CE */
	IO_STATUS_BLOCK	read;

	if (image != NULL &&
	    STATUS_SUCCESS == ZwReadFile((HANDLE)image, NULL, NULL, NULL,
	                                 &read, buf, len, NULL, NULL))
		ret = (int)read.Information;

	return ret;
#endif /* UNDER_CE */
}

void
dhd_os_close_image(void *image)
{
#ifdef UNDER_CE
	FILE *fr = (FILE *)image;

	if (fr)
		fclose(fr);
#else /* UNDER_CE */
	if (image)
		ZwClose((HANDLE)image);
#endif /* UNDER_CE */
}

#ifdef DHD_ALLIRQ

void
dhd_os_sdlock(dhd_pub_t *pub)
{
}

void
dhd_os_sdunlock(dhd_pub_t *pub)
{
}

void
dhd_os_sdisrlock(dhd_pub_t *pub)
{
}

void
dhd_os_sdisrunlock(dhd_pub_t *pub)
{
}

#else /* DHD_ALLIRQ */

void
dhd_os_sdlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	if (wl) {
		NdisAcquireSpinLock(&wl->dhdlock);
#if defined(BCMSDIO) && !defined(UNDER_CE)
		sdstd_status_intr_enable(wl->sh.sdh, FALSE);
#endif /* BCMSDIO */
	}
}

void
dhd_os_sdunlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	if (wl) {
#if defined(BCMSDIO) && !defined(UNDER_CE)
		sdstd_status_intr_enable(wl->sh.sdh, TRUE);
#endif  /* BCMSDIO */
		NdisReleaseSpinLock(&wl->dhdlock);
	}
}

/* WP7 TODO: not implemented in original WM7 TOT */
void
dhd_os_sdlock_txq(dhd_pub_t *pub)
{
#if !defined(UNDER_CE)
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	if (wl)
		NdisAcquireSpinLock(&wl->dhd_tx_queue_lock);
#endif /* UNDER_CE */
}

void
dhd_os_sdunlock_txq(dhd_pub_t *pub)
{
#if !defined(UNDER_CE)
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = (wl_info_t *)dhd->wl;

	if (wl)
		NdisReleaseSpinLock(&wl->dhd_tx_queue_lock);
#endif /* UNDER_CE */
}

void
dhd_os_sdlock_rxq(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = dhd->wl;

	ND_RXLOCK(wl);
}

void
dhd_os_sdunlock_rxq(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = dhd->wl;

	ND_RXUNLOCK(wl);
}
/* WP7 TODO: not there in original */
void
dhd_os_sdlock_sndup_rxq(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = dhd->wl;

	if (wl)
		NdisAcquireSpinLock(&wl->dhd_rx_queue_lock);
}

void
dhd_os_sdunlock_sndup_rxq(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = dhd->wl;

	if (wl)
		NdisReleaseSpinLock(&wl->dhd_rx_queue_lock);
}
#if defined(UNDER_CE)
void
dhd_os_sdlock_eventq(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = dhd->wl;

	if (wl)
		NdisAcquireSpinLock(&wl->dhd_evq_lock);
}

void
dhd_os_sdunlock_eventq(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)pub;
	wl_info_t *wl = dhd->wl;

	if (wl)
		NdisReleaseSpinLock(&wl->dhd_evq_lock);
}
#endif /* UNDER_CE */
#endif /* DHD_ALLIRQ */

#ifdef BCMDBUS
void
dhd_bus_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
}

int
dhd_bus_iovar_op(dhd_pub_t *dhdp, const char *name,
        void *params, int plen, void *arg, int len, bool set)
{
	return 0;
}
void
dhd_bus_clearcounts(dhd_pub_t *dhdp)
{
}

#if defined(NDIS60)
void
dhd_rxpkt_cb(async_req_t *req, void *arg)
{
	dhd_info_t *dhd =  arg;
	wl_info_t *wl = dhd->wl;
	void *pkt;

	ASSERT(req);
	ASSERT(dhd);

	pkt = req->parms.rxpkt.pkt;
	ASSERT(pkt);

	dhd_rx_frame(&dhd->pub, req->ifidx, pkt, 1, 0);

	/* Insert back to rxpkt async free list */
	bzero(req, sizeof(async_req_t));
	NdisInitializeListHead(&req->list);
	NdisInterlockedInsertTailList(&dhd->asyncFreeReqList,
		&req->list,
		&dhd->asyncFreeReqLock);
}
#endif /* NDIS60 */

#endif /* BCMDBUS */

void
dhd_wait_for_event(dhd_pub_t *dhd, bool *lockvar)
{
	return;
}

void
dhd_wait_event_wakeup(dhd_pub_t *dhd)
{
	return;
}

int
dhd_change_mtu(dhd_pub_t *dhdp, int new_mtu, int ifidx)
{
	return BCME_UNSUPPORTED;
}

void
dhd_iscan_parse_current_results(void *h);

void
dhd_ind_scan_confirm(void *h, bool status)
{
#if defined(EXT_STA) && defined(UNDER_CE)
	dhd_info_t *dhd_info = dhd_bus_dhd(h);
	DHD_ISCAN(("%s: ISCAN in scan confirm\n", __FUNCTION__));
	if (status == TRUE) {
		dhd_iscan_parse_current_results(h);
	}
	/* Both success and error status, flush iscan results */
	dhd_iscan_free_buf(h, 0);
	wl_ind_scan_confirm(dhd_info->wl, status ? NDIS_STATUS_SUCCESS :
		NDIS_STATUS_REQUEST_ABORTED);
#endif /* EXT_STA */
}

int
dhd_iscan_in_progress(void *h)
{
	dhd_info_t *dhd_info = dhd_bus_dhd(h);
	wl_info_t *wl = dhd_info->wl;

	return wl->in_scan;
}

void
dhd_init_timer(void *h, PNDIS_TIMER_FUNCTION fn, void *arg, NDIS_MINIPORT_TIMER *pndis_timer)
{
	dhd_info_t *dhd_info = dhd_bus_dhd(h);
	wl_info_t *wl = (wl_info_t *)dhd_info->wl;

	NdisMInitializeTimer(pndis_timer, wl->sh.adapterhandle,	fn, arg);
}

void
dhd_release_timer(void *h, NDIS_MINIPORT_TIMER *pndis_timer)
{
	/* TBD */
}

int
dhd_del_timer(void *dhdp, NDIS_MINIPORT_TIMER *pndis_timer)
{
	bool canceled = TRUE;

	NdisMCancelTimer(pndis_timer, &canceled);
	return 0;
}

int
dhd_add_timer(void *dhdp, NDIS_MINIPORT_TIMER *pndis_timer, int delay)
{
	NdisMSetTimer(pndis_timer, delay);

	return 0;
}


#ifdef DHD_DEBUG
#ifdef UNDER_CE
int
write_to_file(dhd_pub_t *dhd, uint8 *buf, int size)
{
	int ret = 0;
	FILE *pFile;

	/* Open physical file to write */
	pFile = fopen("mem_dump", "wb");
	if (pFile == NULL) {
		printf("%s: Open file error\n", __FUNCTION__);
		ret = -1;
		goto exit;
	}

	/* Write buf to physical file */
	fwrite(buf, sizeof(*buf), size, pFile);

exit:
	/* Handle buf free before exit */
	if (buf) {
		MFREE(dhd->osh, buf, size);
	}
	if (pFile) {
		fclose(pFile);
	}
	return ret;
}
#else
typedef struct {
	osl_t *osh;
	uint8 *buf;
	int size;
	PIO_WORKITEM witem;
} WR_FILE, *PWR_FILE;

VOID
wi_callback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          context)
{
	PWR_FILE wrf = (PWR_FILE)context;
	HANDLE fr = 0;
	IO_STATUS_BLOCK iostatus;
	OBJECT_ATTRIBUTES obj_att;
	UNICODE_STRING	uni_file;
	NTSTATUS status;

	PAGED_CODE();
	RtlInitUnicodeString(&uni_file, L"\\DosDevices\\C:\\mem_dump");
	InitializeObjectAttributes(&obj_att, &uni_file,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwCreateFile(&fr, GENERIC_WRITE, &obj_att, &iostatus, NULL,
		FILE_ATTRIBUTE_NORMAL, 0, FILE_OVERWRITE_IF,
		FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (status != STATUS_SUCCESS) {
		printf("%s: open file error (%d)\n", __FUNCTION__, status);
		goto exit;
	}

	status = ZwWriteFile(fr, NULL, NULL, NULL,
		&iostatus, wrf->buf, wrf->size, NULL, NULL);
	if (status != STATUS_SUCCESS) {
		printf("%s: write file error (%d)\n", __FUNCTION__, status);
	}

exit:
	MFREE(wrf->osh, wrf->buf, wrf->size);
	IoFreeWorkItem(wrf->witem);
	ExFreePool(wrf);
	ZwClose(fr);
}

int
write_to_file(dhd_pub_t *dhd, uint8 *buf, int size)
{
	PWR_FILE wrf = (PWR_FILE) ExAllocatePool(NonPagedPool, sizeof(WR_FILE));
	if (wrf == NULL) {
		printf("%s: out of memory in NonPagedPool (%d bytes)\n",
			__FUNCTION__, sizeof(WR_FILE));
		MFREE(dhd->osh, buf, size);
		return -1;
	}

	wrf->osh = dhd->osh;
	wrf->buf = buf;
	wrf->size = size;
	wrf->witem = IoAllocateWorkItem(dhd->fdo);
	if (wrf->witem == NULL) {
		printf("%s: out of memory\n", __FUNCTION__);
		MFREE(dhd->osh, buf, size);
		ExFreePool(wrf);
		return -1;
	}

	IoQueueWorkItem(wrf->witem, (PIO_WORKITEM_ROUTINE)wi_callback, DelayedWorkQueue, wrf);

	return 0;
}
#endif /* UNDER_CE */
#endif /* DHD_DEBUG */
#if defined(SIMPLE_ISCAN)
/* Return TRUE if pass timestamp check else FALSE */
bool
dhd_time_stamp_check(ULONGLONG cur_time, ULONGLONG past_time, uint32 threshold)
{
	bool rc = FALSE;
	/* threshold in second */
	if (threshold == 0) {
		/* No threshold contraint */
		rc = TRUE;
	}
	if (cur_time >= past_time) {
		/* Timestamp in 100ns unit */
		if ((cur_time - past_time)/SYSTEM_TIME_TO_SEC_DIV <= threshold) {
			rc = TRUE;
		}
	}
	return rc;
}

void
dhd_init_bss_list(void *dhdp, bss_list_t *list)
{
	dhd_info_t *dhd_info = dhd_bus_dhd(dhdp);
	wl_info_t *wl = (wl_info_t *)dhd_info->wl;

	list->head = NULL;
	list->num_entry = 0;
	list->total_ie_length = 0;
}

void
dhd_free_bss_list(void *dhdp, bss_list_t *list)
{
	dhd_info_t *dhd_info;
	bss_record_t *current_bss;
	bss_record_t *next_bss;

	if (!list) {
		return;
	}

	current_bss = list->head;
	if (dhdp) {
		dhd_info = dhd_bus_dhd(dhdp);
		while (current_bss) {
			next_bss = current_bss->next;
			MFREE(dhd_info->pub.osh, current_bss,
				OFFSETOF(bss_record_t, bss) + current_bss->bss.length);
			current_bss = next_bss;
		}
	}

	if (!current_bss) {
		list->num_entry = 0;
		list->head = NULL;
		list->total_ie_length = 0;
	}
}

void
dhd_migrate_bss_list(bss_list_t *src_list, bss_list_t *dest_list)
{
	dest_list->head = src_list->head;
	dest_list->num_entry = src_list->num_entry;
	dest_list->total_ie_length = src_list->total_ie_length;
}

void
dhd_add_bss_list_head(bss_list_t *list, bss_record_t *new_bss)
{
	/* Insert to head for efficiency */
	if (list->head == NULL) {
		new_bss->next = NULL;
		list->head = new_bss;
	} else {
		new_bss->next = list->head;
		list->head = new_bss;
	}
	list->num_entry++;
	list->total_ie_length += new_bss->bss.ie_length;
}

/* Return TRUE if duplicate found, FALSE otherwise */
bool
dhd_check_duplicate_bss_list(void *dhdp, bss_list_t *list, wl_bss_info_t *bi_match)
{

	dhd_info_t *dhd_info = dhd_bus_dhd(dhdp);
	bool match_found = FALSE;
	int i = 0;
	bss_record_t *current_record;

	if (!list) {
		return FALSE;
	}
	current_record = list->head;
	while (current_record) {
		if (bi_match->chanspec == current_record->bss.chanspec &&
		    !bcmp(bi_match->BSSID.octet, current_record->bss.BSSID.octet, ETHER_ADDR_LEN) &&
		    !bcmp(bi_match->SSID, current_record->bss.SSID, bi_match->SSID_len) &&
		    bi_match->SSID_len == current_record->bss.SSID_len) {
			DHD_ISCAN(("%s: Duplicate found %s\n", __FUNCTION__, bi_match->SSID));
			match_found = TRUE;
			break;
		}
		current_record = current_record->next;
	}
	return match_found;
}

void
dhd_add_bss_list(dhd_info_t *dhd_info, bss_list_t *list,
                 UNALIGNED wl_bss_info_t *bi, bool check_unique, ULONGLONG time_stamp)
{
	bss_record_t *current_bss;
	bss_record_t *new_bss;
	int alloc_size;
	if (!list) {
		return;
	}
	alloc_size = OFFSETOF(bss_record_t, bss) + bi->length;
	new_bss = (bss_record_t *) MALLOC(dhd_info->pub.osh, alloc_size);

	memcpy(&new_bss->bss, bi, bi->length);
	new_bss->next = NULL;
	new_bss->timestamp = time_stamp;
	new_bss->valid = 1;
	DHD_ISCAN(("%s: Add BSS %s\n", __FUNCTION__, new_bss->bss.SSID));
	if (check_unique) {
		if (dhd_check_duplicate_bss_list(dhd_info, list, &(new_bss->bss))) {
			MFREE(dhd_info->pub.osh, new_bss, alloc_size);
			return;
		}
	}

	dhd_add_bss_list_head(list, new_bss);
}

void
dhd_dump_list(wl_info_t *wl, bss_list_t *list)
{
	bss_record_t *current_bss = list->head;
	int index = 0;
	if (list == &(wl->current_scan_list)) {
		printf("----Dump CURRENT----\n");
	} else {
		printf("----Dump CACHE   ----\n");
	}
	while (current_bss) {
		printf("[%d]%s timestamp %llu\n",
		       index, current_bss->bss.SSID, current_bss->timestamp);
		index++;
		current_bss = current_bss->next;
	}
}

/* Go through cache results, free if duplicate or stale, otherwise add
 * to recent, then save recent to cache
 */
void
dhd_iscan_merge_cache_results(void *h, bss_list_t *recent_list,
                              bss_list_t *cache_list, ULONGLONG cur_time, ULONGLONG threshold)
{
	dhd_info_t *dhd_info = dhd_bus_dhd(h);
	wl_info_t *wl = (wl_info_t *) dhd_info->wl;
	bss_record_t *current_bss;
	bss_record_t *next_bss;

	if (!cache_list || !recent_list) {
		return;
	}

	current_bss = cache_list->head;
	while (current_bss) {
		next_bss = current_bss->next;
		if (!dhd_time_stamp_check(cur_time, current_bss->timestamp, threshold) ||
			dhd_check_duplicate_bss_list(dhd_info, recent_list, &(current_bss->bss))) {
			/* Fail timestamp check or duplicate, free */
			MFREE(wl->sh.osh, current_bss,
			      OFFSETOF(bss_record_t, bss) + current_bss->bss.length);
		} else {
			/* Add to recent list */
			current_bss->valid = 1;
			dhd_add_bss_list_head(recent_list, current_bss);
		}
		current_bss = next_bss;
	}
	/* Migrate recent list to cache list */
	dhd_migrate_bss_list(recent_list, cache_list);

	/* Purge recent list */
	recent_list->head = NULL;
	recent_list->num_entry = 0;
	recent_list->total_ie_length = 0;
}

/* Parse current iscan results, create new bss list with unique entries */
void
dhd_iscan_parse_current_results(void *h)
{
	dhd_info_t *dhd_info = dhd_bus_dhd(h);
	wl_info_t *wl = (wl_info_t *) dhd_info->wl;
	int free_size;
	DOT11_BYTE_ARRAY *byte_array;
	iscan_buf_t *iscan_cur;
	wl_iscan_results_t *list;
	wl_scan_results_t *results;
	wl_bss_info_t UNALIGNED *bi;
	uint32 i;
	LARGE_INTEGER time_stamp;

	dhd_iscan_lock();
	DHD_ISCAN(("%s:\n", __FUNCTION__));

	NdisGetCurrentSystemTime(&time_stamp);

	/* If current_scan_results contains results from previous scan, save in cache */
	if (wl->current_scan_list.num_entry > 0 && wl->current_scan_list.head) {
		DHD_ISCAN(("%s: Offload current scan to cache scan\n", __FUNCTION__));
		dhd_iscan_merge_cache_results(h, &(wl->current_scan_list), &(wl->cache_scan_list),
			(ULONGLONG)time_stamp.QuadPart, wl->scan_aging_threshold);
	}

	/* Parse new scan results */
	iscan_cur = dhd_iscan_result_buf();
	while (iscan_cur) {
		list = (wl_iscan_results_t *) iscan_cur->iscan_buf;
		if (!list) {
			break;
		}
		results = (wl_scan_results_t *) &(list->results);
		if (!results) {
			break;
		}

		if (results->version != WL_BSS_INFO_VERSION) {
			DHD_ERROR(("%s: results->version %d != WL_BSS_INFO_VERSION\n",
			__FUNCTION__, results->version));
			break;
		}

		bi = results->bss_info;
		DHD_ISCAN(("%s: Result count %d\n", __FUNCTION__, results->count));
		for (i = 0; i < results->count; i++) {
			if (!bi) {
				break;
			}
			dhd_add_bss_list(dhd_info, &(wl->current_scan_list), bi,
			                 TRUE, (ULONGLONG)time_stamp.QuadPart);
			bi = (wl_bss_info_t *)(((uintptr)bi) + dtoh32(bi->length));
		}
		iscan_cur = iscan_cur->next;
	}

	dhd_iscan_unlock();
}

int
dhd_iscan_request(void *dhdp, uint16 action)
{
	int rc = -1;
	dhd_info_t *dhd_info = dhd_bus_dhd(dhdp);
	wl_info_t *wl = dhd_info->wl;
	wl_iscan_params_t params;

	switch (action) {
		case WL_SCAN_ACTION_START:
		case WL_SCAN_ACTION_CONTINUE:
			DHD_ISCAN(("%s: ISCAN WL_SCAN_ACTION_START or CONTINUE\n", __FUNCTION__));
			if (wl->pIscan_params && wl->iscan_param_size >=
			    sizeof(wl_iscan_params_t)) {
				wl->pIscan_params ->action = htod16(action);
				rc = dhd_iscan_issue_request(dhdp, wl->pIscan_params,
					wl->iscan_param_size);
			} else {
				DHD_ISCAN(("%s: Iscan parameters not initialized\n", __FUNCTION__));
			}
			break;
		case WL_SCAN_ACTION_ABORT:
			DHD_ISCAN(("%s: ISCAN WL_SCAN_ACTION_ABORT\n", __FUNCTION__));
			memset(&params, 0, sizeof(wl_iscan_params_t));
			memcpy(&params.params.bssid, &ether_bcast, ETHER_ADDR_LEN);

			params.params.bss_type = DOT11_BSSTYPE_ANY;
			params.params.scan_type = DOT11_SCANTYPE_ACTIVE;

			params.params.nprobes = htod32(-1);
			params.params.active_time = htod32(-1);
			params.params.passive_time = htod32(-1);
			params.params.home_time = htod32(-1);
			params.params.channel_num = htod32(0);

			params.version = htod32(ISCAN_REQ_VERSION);
			params.action = htod16(action);
			params.scan_duration = htod16(0);

			rc = dhd_iscan_issue_request(dhdp, &params, sizeof(wl_iscan_params_t));
			break;
		default:
			DHD_ISCAN(("%s: Unknown action %d\n", __FUNCTION__, action));
			break;
	}

	return rc;
}

#endif /* SIMPLE_ISCAN */
