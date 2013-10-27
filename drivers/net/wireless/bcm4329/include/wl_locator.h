/*
 * WIFI LOCATOR API definition
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: wl_locator.h,v 13.4 2005-10-10 23:41:55 kiranm Exp $
 */

#ifndef _wl_locator_h
#define _wl_locator_h

/* band types */
#define LOCATOR_BAND_5G		1	/* 5 Ghz */
#define LOCATOR_BAND_2G		2	/* 2.4 Ghz */
#define LOCATOR_BAND_ALL	3	/* all bands */

/* scan completion criteria */
#define ANY_RSSI		0	/* Return RSSI on first Matching AP */
#define STRONG_RSSI		1	/* Return RSSI of Strongest AP */

/* scan type */
#define LOCATOR_MODE_BASIC	0	/* Basic Mode */
#define LOCATOR_MODE_ADVANCED	1	/* Advanced Mode */

/* Locator Error codes */
#define LOCATOR_ERR_INVALIDPARAM	1	/* Invalid Input parameter */
#define LOCATOR_ERR_DEVICENOTFOUND	2	/* No supported device found */
#define LOCATOR_ERR_INTERNALERR		3	/* Internal Error */

/* 
 * channel bitmap array size max channel num 216, so minimum 216 bits
 * round it of to 224 and it would need MAXCHANNEL/(NBBY*4)
*/
#define CHANNELBITMAP_ARRAYSIZE	(MAXCHANNEL/(NBBY*4))	/* bit map dword array size */

struct locator_ssidlist {
	unsigned char SSID_len; /* ssid len */
	unsigned char SSID[32]; /* SSID */
};

struct locator_params {
	unsigned long timer_ioaddr; /* ACPI timer base IOaddress */
	unsigned long channel_bitmap[CHANNELBITMAP_ARRAYSIZE]; /* channel list bitmap */
	unsigned long band;	    /* operating band */
	unsigned long operating_mode;    /* Basic or Advanced Mode */
	unsigned long scan_comp_criteria; /* scan completion criteria */
	unsigned long dbg_rtn_addr;	/* putc routine address */
	unsigned long abortcheck_rtn_addr;	/* user abort check routine address */
	struct locator_ssidlist *ssid_list_tbl[32]; /* tbl of ssid_list ptr */
};

/* returns RSSI or error code */
int wifilocator_scan(struct locator_params *locator_api);

#endif /* _wl_locator_h */
