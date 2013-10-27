/*
 * RPC - Transport layer
 *   HOST may glue it to DBUS layer
 *   CLIENT may glue it to its bus driver
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: bcm_rpc_tp.h,v 13.34 2009-03-10 08:43:40 vibes Exp $
 */

#ifndef _bcm_rpc_tp_h_
#define _bcm_rpc_tp_h_
#include <bcm_rpc.h>

#define DBUS_RX_BUFFER_SIZE_RPC	(2100)	/* rxbufsize for dbus_attach, linux only for now */

#define BCM_RPC_TP_ENCAP_LEN	4	/* TP header is 4 bytes */

#define BCM_RPC_TP_HOST_AGG_MASK	0xffff0000
#define BCM_RPC_TP_HOST_AGG_SHIFT	16
#define BCM_RPC_TP_HOST_AGG_AMPDU	0x00010000	/* HOST->DNGL ampdu aggregation */
#define BCM_RPC_TP_HOST_AGG_TEST	0x00100000	/* HOST->DNGL test aggregation */
#define BCM_RPC_TP_DNGL_AGG_MASK	0x0000ffff
#define BCM_RPC_TP_DNGL_AGG_DPC		0x00000001	/* DNGL->HOST data aggregation */
#define BCM_RPC_TP_DNGL_AGG_FLOWCTL	0x00000002	/* DNGL->HOST tx flowcontrol agg */
#define BCM_RPC_TP_DNGL_AGG_TEST	0x00000010	/* DNGL->HOST test agg */

/* XXX these tunables need to be tuned, normally 16mpdu/ampdu, 1.5K each, max ampdu = 24KB
 *   but host rx memory is limited.
 */
#define BCM_RPC_TP_DNGL_AGG_MAX_SFRAME	3       /* max agg subframes, must be <= USB_NTXD */
#define BCM_RPC_TP_DNGL_AGG_MAX_BYTE	4000    /* max agg bytes */

/* Tx aggregation size limit for transport.
 * XXX these tunables need to be tuned, normally 16mpdu/ampdu, 1.5K each, max ampdu = 24KB
 *   but dongle rx memory is limited. Set to max size the reciever end can take.
 *   May depend on bus type (USB/SDIO) or other configuration items.
 */
#define BCM_RPC_TP_HOST_AGG_MAX_SFRAME  3	/* max agg subframes, AMPDU only, 3 is enough */
#define BCM_RPC_TP_HOST_AGG_MAX_BYTE    3400    /* max agg bytes; to fit 2+ tcp/udp pkts. Each one:
						 * 802.3pkt + 802.11 hdr + rpc hdr + tp hdr < 1700B
						 * Need to be in sync with dongle usb rx dma
						 *  rxbufsize(USBBULK_RXBUF_GIANT in usbdev_sb.c)
						 */
/* TP-DBUS pkts flowcontrol */
#define BCM_RPC_TP_DBUS_NTXQ	50	/* queue size for TX on bulk OUT, aggregation possible */
#define BCM_RPC_TP_DBUS_NRXQ	50	/* queue size for RX on bulk IN, aggregation possible */
#define BCM_RPC_TP_DBUS_NRXQ_CTRL	1	/* queue size for RX on ctl EP0 */

#define BCM_RPC_TP_DBUS_NRXQ_PKT	(BCM_RPC_TP_DBUS_NRXQ * BCM_RPC_TP_DNGL_AGG_MAX_SFRAME)
#define BCM_RPC_TP_DBUS_NTXQ_PKT	(BCM_RPC_TP_DBUS_NTXQ * BCM_RPC_TP_HOST_AGG_MAX_SFRAME)

typedef struct rpc_transport_info rpc_tp_info_t;

typedef void (*rpc_tx_complete_fn_t)(void*, rpc_buf_t *, int status);
typedef void (*rpc_rx_fn_t)(void*, rpc_buf_t*);

#ifdef WLC_LOW
typedef void (*rpc_txflowctl_cb_t)(void *ctx, bool on);
#endif

#if defined(NDIS)
extern void bcm_rpc_tp_surp_remove(rpc_tp_info_t * rpcb);
extern void bcm_rpc_tp_sleep(rpc_tp_info_t * rpcb);
extern void bcm_rpc_tp_resume(rpc_tp_info_t * rpcb);
extern int bcm_rpc_tp_shutdown(rpc_tp_info_t * rpcb);
extern rpc_tp_info_t *bcm_rpc_tp_attach(osl_t * osh, shared_info_t *shared, void *bus);
extern bool bcm_rpc_tp_tx_flowctl_get(rpc_tp_info_t *rpci);
#else
extern rpc_tp_info_t *bcm_rpc_tp_attach(osl_t * osh, void *bus);
#endif
extern void bcm_rpc_tp_detach(rpc_tp_info_t * rpcb);
extern void bcm_rpc_tp_down(rpc_tp_info_t *rpcb);
extern void bcm_rpc_tp_watchdog(rpc_tp_info_t *rpcb);

extern int bcm_rpc_tp_buf_send(rpc_tp_info_t * rpcb, rpc_buf_t *buf);

/* callback for tx_complete, rx_pkt */
extern void bcm_rpc_tp_register_cb(rpc_tp_info_t * rpcb, rpc_tx_complete_fn_t txcmplt,
	void* tx_context, rpc_rx_fn_t rxpkt, void* rx_context, rpc_osl_t *rpc_osh);
extern void bcm_rpc_tp_deregister_cb(rpc_tp_info_t * rpcb);

/* Buffer manipulation */
extern uint bcm_rpc_buf_tp_header_len(rpc_tp_info_t * rpcb);
extern rpc_buf_t *bcm_rpc_tp_buf_alloc(rpc_tp_info_t * rpcb, int len);
extern void bcm_rpc_tp_buf_free(rpc_tp_info_t * rpcb, rpc_buf_t *buf);
extern int bcm_rpc_buf_len_get(rpc_tp_info_t * rpcb, rpc_buf_t* b);
extern int bcm_rpc_buf_totlen_get(rpc_tp_info_t * rpcb, rpc_buf_t* b);
extern int bcm_rpc_buf_len_set(rpc_tp_info_t * rpcb, rpc_buf_t* b, uint len);
extern rpc_buf_t *bcm_rpc_buf_next_get(rpc_tp_info_t * rpcb, rpc_buf_t* b);
extern void bcm_rpc_buf_next_set(rpc_tp_info_t * rpcb, rpc_buf_t* b, rpc_buf_t *nextb);
extern unsigned char* bcm_rpc_buf_data(rpc_tp_info_t * rpcb, rpc_buf_t* b);
extern unsigned char* bcm_rpc_buf_push(rpc_tp_info_t * rpcb, rpc_buf_t* b, uint delta);
extern unsigned char* bcm_rpc_buf_pull(rpc_tp_info_t * rpcb, rpc_buf_t* b, uint delta);
extern void bcm_rpc_tp_buf_release(rpc_tp_info_t * rpcb, rpc_buf_t *buf);
extern void bcm_rpc_tp_buf_cnt_adjust(rpc_tp_info_t * rpcb, int adjust);
#ifdef WLC_HIGH
/* RPC call_with_return */
extern int bcm_rpc_tp_recv_rtn(rpc_tp_info_t *rpcb);
extern int bcm_rpc_tp_get_device_speed(rpc_tp_info_t *rpc_th);
#ifdef BCMDBG
extern int bcm_rpc_tp_dump(rpc_tp_info_t *rpcb, struct bcmstrbuf *b);
#endif
#endif	/* WLC_HIGH */

#ifdef WLC_LOW
/* intercept USB pkt to parse RPC header: USB driver rx-> wl_send -> this -> wl driver */
extern void bcm_rpc_tp_rx_from_dnglbus(rpc_tp_info_t *rpc_th, struct lbuf *lb);

/* RPC callreturn pkt, go to USB driver tx */
extern int bcm_rpc_tp_send_callreturn(rpc_tp_info_t * rpc_th, rpc_buf_t *b);

extern void bcm_rpc_tp_dump(rpc_tp_info_t *rpcb);
extern void bcm_rpc_tp_txflowctl(rpc_tp_info_t *rpcb, bool state, int prio);
extern void bcm_rpc_tp_txflowctlcb_init(rpc_tp_info_t *rpc_th, void *ctx, rpc_txflowctl_cb_t cb);
extern void bcm_rpc_tp_txflowctlcb_deinit(rpc_tp_info_t *rpc_th);
extern void bcm_rpc_tp_txq_wm_set(rpc_tp_info_t *rpc_th, uint8 hiwm, uint8 lowm);
extern void bcm_rpc_tp_txq_wm_get(rpc_tp_info_t *rpc_th, uint8 *hiwm, uint8 *lowm);
#endif /* WLC_LOW */

extern void bcm_rpc_tp_agg_set(rpc_tp_info_t *rpcb, uint32 reason, bool set);
extern void bcm_rpc_tp_agg_limit_set(rpc_tp_info_t *rpc_th, uint8 sf, uint16 bytes);
extern void bcm_rpc_tp_agg_limit_get(rpc_tp_info_t *rpc_th, uint8 *sf, uint16 *bytes);


#define BCM_RPC_TP_MSG_LEVEL_MASK	0x00ff
/* dongle msg level */
#define RPC_TP_MSG_DNGL_ERR_VAL		0x0001	/* DNGL TP error msg */
#define RPC_TP_MSG_DNGL_DBG_VAL		0x0002	/* DNGL TP dbg msg */
#define RPC_TP_MSG_DNGL_AGG_VAL		0x0004	/* DNGL TP agg msg */
#define RPC_TP_MSG_DNGL_DEA_VAL		0x0008	/* DNGL TP deag msg */

/* host msg level */
#define RPC_TP_MSG_HOST_ERR_VAL		0x0001	/* DNGL TP error msg */
#define RPC_TP_MSG_HOST_DBG_VAL		0x0002	/* DNGL TP dbg msg */
#define RPC_TP_MSG_HOST_AGG_VAL		0x0004	/* DNGL TP agg msg */
#define RPC_TP_MSG_HOST_DEA_VAL		0x0008	/* DNGL TP deag msg */

extern void bcm_rpc_tp_msglevel_set(rpc_tp_info_t *rpc_th, uint8 msglevel, bool high_low);

#endif /* _bcm_rpc_tp_h_ */
