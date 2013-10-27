/**
 * Emulator of a wl driver hooking the wlc data path
 * to a bcm5700 driver.
 * $Copyright (C) 2003 Broadcom Corporation$
 *
 * $Id: wl_bcm57emu.h,v 13.2 2005-10-28 01:56:41 pmoutarl Exp $
*/

#ifndef _WL_BCM57EMU_H_
#define _WL_BCM57EMU_H_
extern void * wlcemu_wlccreate(void *pdev);
extern int wlcemu_receive_skb(void *w, struct sk_buff *skb);
extern int wlcemu_start_xmit(struct sk_buff *skb, struct net_device *dev);
extern int wlcemu_ioctl(void *, char **);
extern void *wlcemu_pktget(void *, int);
extern int wlcemu_txfifo(void *wlc, uint fifo, void *p);

#undef W_REG
#undef R_REG
#undef PIO_ENAB
#undef DEVICEREMOVED

#define DEVICEREMOVED(wlc) 0
#define PIO_ENAB(wlc) 0
#define W_REG(a,b) do { if (0) {*a = b;}} while (0)
#define R_REG(x) (0?*x:0)

#endif /* _WL_BCM57EMU_H_ */
