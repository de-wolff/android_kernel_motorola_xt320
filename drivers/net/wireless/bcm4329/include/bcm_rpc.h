/*
 * RPC module header file
 *
 * $Copyright (C) 2008 Broadcom Corporation $
 *
 * $Id: bcm_rpc.h,v 13.17 2009-04-17 22:33:57 mzhu Exp $
 */

#ifndef _BCM_RPC_H_
#define _BCM_RPC_H_

#include <typedefs.h>
#include <rpc_osl.h>

typedef struct rpc_info rpc_info_t;
typedef struct rpc_buf rpc_buf_t;
struct rpc_transport_info;
typedef void (*rpc_dispatch_cb_t)(void *ctx, struct rpc_buf* buf);
typedef void (*rpc_resync_cb_t)(void *ctx);
typedef void (*rpc_down_cb_t)(void *ctx);
typedef void (*rpc_txdone_cb_t)(void *ctx, struct rpc_buf* buf);
extern struct rpc_info *bcm_rpc_attach(void *pdev, osl_t *osh, struct rpc_transport_info *rpc_th);

#ifdef NDIS
extern void bcm_rpc_sleep(struct rpc_info *rpc);
extern bool bcm_rpc_resume(struct rpc_info *rpc);
extern int bcm_rpc_shutdown(struct rpc_info *rpc);
#endif
extern void bcm_rpc_detach(struct rpc_info *rpc);
extern void bcm_rpc_down(struct rpc_info *rpc);
extern void bcm_rpc_watchdog(struct rpc_info *rpc);

extern struct rpc_buf *bcm_rpc_buf_alloc(struct rpc_info *rpc, int len);
extern void bcm_rpc_buf_free(struct rpc_info *rpc, struct rpc_buf *b);
/* get rpc transport handle */
extern struct rpc_transport_info *bcm_rpc_tp_get(struct rpc_info *rpc);


/* callback for: data_rx, down, resync */
extern void bcm_rpc_rxcb_init(struct rpc_info *rpc, void *ctx, rpc_dispatch_cb_t cb,
                              void *dnctx, rpc_down_cb_t dncb, rpc_resync_cb_t resync_cb,
                              rpc_txdone_cb_t);
extern void bcm_rpc_rxcb_deinit(struct rpc_info *rpci);

/* HOST or CLIENT rpc call, requiring no return value */
extern int bcm_rpc_call(struct rpc_info *rpc, struct rpc_buf *b);

/* HOST rpc call, demanding return.
 *   The thread may be suspended and control returns back to OS
 *   The thread will resume(waked up) on either the return signal received or timeout
 *     The implementation details depend on OS
 */
extern struct rpc_buf *bcm_rpc_call_with_return(struct rpc_info *rpc, struct rpc_buf *b);

/* CLIENT rpc call to respond to bcm_rpc_call_with_return, requiring no return value */
extern int bcm_rpc_call_return(struct rpc_info *rpc, struct rpc_buf *retb);

extern uint bcm_rpc_buf_header_len(struct rpc_info *rpci);

#define RPC_PKTLOG_SIZE		50 /* Depth of the history */
#define RPC_PKTLOG_RD_LEN	3
#define RPC_PKTLOG_DUMP_SIZE	150 /* dump size should be more than the product of above two */
#ifdef WLC_HIGH
extern int bcm_rpc_pktlog_get(struct rpc_info *rpci, uint32 *buf, uint buf_size, bool send);
extern int bcm_rpc_dump(rpc_info_t *rpci, struct bcmstrbuf *b);
#endif

/* HIGH/BMAC: bit 15-8: RPC module, bit 7-0: TP module */
#define RPC_ERROR_VAL	0x0001
#define RPC_TRACE_VAL	0x0002
#define RPC_PKTTRACE_VAL 0x0004
#define RPC_PKTLOG_VAL	0x0008
extern void bcm_rpc_msglevel_set(struct rpc_info *rpci, uint16 msglevel, bool high_low);

#endif /* _BCM_RPC_H_ */
