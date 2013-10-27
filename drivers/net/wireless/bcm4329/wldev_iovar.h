/*
 * Linux wl IOVAR/IOCTL
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: wl_cfg80211.c,v 1.1.4.1.2.14 2011-02-09 01:40:07 eccopark Exp $
 */
#ifndef __WLDEV_IOVAR_H__
#define __WLDEV_IOVAR_H__
 
/** wl_dev_ioctl - get/set IOCTLs, will call net_device's do_ioctl (or 
    netdev_ops->ndo_do_ioctl in new kernels)
    @dev: the net_device handle
 */

s32 wldev_ioctl(
	struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set);


/** Retrieve named IOVARs, this function calls wl_dev_ioctl with 
    WLC_GET_VAR IOCTL code
 */
s32 wldev_iovar_getbuf(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen);

/** Set named IOVARs, this function calls wl_dev_ioctl with 
    WLC_SET_VAR IOCTL code
 */
s32 wldev_iovar_setbuf(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen);


/** The following function can be implemented if there is a need for bsscfg
    indexed IOVARs
 */

/** Retrieve named and bsscfg indexed IOVARs, this function calls wl_dev_ioctl with 
    WLC_GET_VAR IOCTL code
 */
s32 wldev_iovar_getbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx);

/** Set named and bsscfg indexed IOVARs, this function calls wl_dev_ioctl with 
    WLC_SET_VAR IOCTL code
 */
s32 wldev_iovar_setbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx);
 
#endif /* __WLDEV_IOVAR_H__ */

