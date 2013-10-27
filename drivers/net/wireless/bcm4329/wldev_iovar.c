/*
 * Linux wl IOVAR/IOCTL
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: wl_cfg80211.c,v 1.1.4.1.2.14 2011-02-09 01:40:07 eccopark Exp $
 */

 #include <wlioctl.h>
 #include <bcmutils.h>

 s32 wldev_ioctl(
	struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set)
{
	s32 ret = 0;
	struct ifreq ifr;
	struct wl_ioctl ioc;
	mm_segment_t fs;
	s32 err = 0;

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = cmd;
	ioc.buf = arg;
	ioc.len = len;
#ifdef DONGLEOVERLAYS
	ioc.action = set;
#else
	ioc.set = set;
#endif	
	strcpy(ifr.ifr_name, dev->name);
	ifr.ifr_data = (caddr_t)&ioc;

	fs = get_fs();
	set_fs(get_ds());
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
	err = dev->do_ioctl(dev, &ifr, SIOCDEVPRIVATE);
#else
	err = dev->netdev_ops->ndo_do_ioctl(dev, &ifr, SIOCDEVPRIVATE);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31) */
	set_fs(fs);

	return ret;
}

/** Format a iovar buffer, not bsscfg indexed. The bsscfg index will be
    taken care of in dhd_ioctl_entry. Internal use only, not exposed to
    wl_iw, wl_cfg80211 and wl_cfgp2p
 */
static s32 wldev_mkiovar(
	s8 *iovar_name, s8 *param, s32 paramlen,
	s8 *iovar_buf, u32 buflen)
{
	s32 iolen = 0;
	
	iolen = bcm_mkiovar(iovar_name, param, paramlen, iovar_buf, buflen);	
 	return iolen;
}

s32 wldev_iovar_getbuf(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen)
{
	s32 ret = 0;
	s32 iovar_len = 0;

	iovar_len = wldev_mkiovar(iovar_name, param, paramlen, buf, buflen);
 	ret = wldev_ioctl(dev, WLC_GET_VAR, buf, iovar_len, FALSE);
	return ret;
}


s32 wldev_iovar_setbuf(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen)
{
	s32 ret = 0;
	s32 iovar_len;
 	
	iovar_len = wldev_mkiovar(iovar_name, param, paramlen, buf, buflen);
	ret = wldev_ioctl(dev, WLC_SET_VAR, buf, iovar_len, FALSE);
 	return ret;
}
  
/** Format a bsscfg indexed iovar buffer. The bsscfg index will be
    taken care of in dhd_ioctl_entry. Internal use only, not exposed to
    wl_iw, wl_cfg80211 and wl_cfgp2p
 */
#ifdef WLDEV_IOCTL_BSSCFG
static s32 wl_dev_mkiovar_bsscfg(
	const s8 *iovar_name, s8 *param, s32 paramlen,
	s8 *iovar_buf, s32 buflen, s32 bssidx)
{
	return -ENOSYS;
}

s32 wl_dev_iovar_getbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx)
{
	return -ENOSYS;
}

s32 wl_dev_iovar_setbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name, 
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx)
{
	return -ENOSYS;
}
#endif

