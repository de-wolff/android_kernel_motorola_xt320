/*
 * PLT code specific data structure
 * Broadcom 802.11abg Networking Device Driver
 *
 * Definitions subject to change without notice.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: wlplt.h,v 1.5.126.2 2010-05-12 20:11:18 csm Exp $
 */
#ifndef _wlplt_h_
#define _wlplt_h_

#define PLT_PARAMS_MAX		60	/* Maximum size of any PLT in/out structure */
#define PLT_ETHER_ADDR_LEN	6

#define PLT_SRATE_NONHTMODE	0
#define PLT_SRATE_HTMODE	1

#define PLT_SRATE_IGI_800ns	0
#define PLT_SRATE_IGI_400ns	1

#define PLT_PREAMBLE_LONG	0	/* legacy LONG / MIMO Mixed_mode */
#define PLT_PREAMBLE_SHORT	1	/* legacy SHORT / MIMO Greenfield */
#define PLT_PREAMBLE_AUTO	-1

#define PLT_TXPWR_CTRL_OPEN	0
#define PLT_TXPWR_CTRL_CLOSED	1

#define PLT_CSUPP_OFF		0
#define PLT_CSUPP_ON		1

#define PLT_SEQNU_OFF		0
#define PLT_SEQNU_ON		1

#define PLT_BTCX_RFACTIVE	0
#define PLT_BTCX_TXCONF		1
#define PLT_BTCX_TXSTATUS	2

#define PLT_CBUCKMODE_PWM	0
#define PLT_CBUCKMODE_BURST	1
#define PLT_CBUCKMODE_LPPWM	2

/* XXX: please look out for Alignment issues when changing the variables */
typedef struct wl_plt_srate {
	uint8		mode;		/* PLT_SRATE_NONHTMODE/HTMODE */
	uint8		igi;		/* PLT_SRATE_IGI_800ns/400ns */
	uint8		m_idx;		/* mcs index for HT mode */
	uint8		pad[1];
	uint32		tx_rate;	/* WLC_RATE_xxx */
} wl_plt_srate_t;

typedef struct wl_plt_continuous_tx {
	uint8 		band;		/* WLC_BAND_2G/5G */
	uint8		channel;
	int8		preamble;	/* PLT_PREAMBLE_SHORT/LONG */
	uint8		carrier_suppress; /* PLT_CSUPP_ON/OFF */ /* XXX: valid for only 2Mbps */
	uint8		pwrctl;		/* PLT_TXPWR_CTRL_OPEN/CLOSED loop */
	int8		power;		/* XXX: still under discussion */
	int8		pad[2];
	uint32		ifs;		/* Inter frame spacing in usec */
	wl_plt_srate_t	srate;
} wl_plt_continuous_tx_t;

typedef struct wl_plt_txper_start {
	uint8 		band;		/* WLC_BAND_2G/5G */
	uint8		channel;
	uint8		preamble;	/* PLT_PREAMBLE_SHORT/LONG */
	uint8		seq_ctl;	/* PLT_SEQNU_OFF/ON */
	uint8		pwrctl;		/* PLT_TXPWR_CTRL_OPEN/CLOSED loop */
	uint8		pad[1];
	uint16		length;
	uint8		dest_mac[PLT_ETHER_ADDR_LEN];
	uint8		src_mac[PLT_ETHER_ADDR_LEN];
	uint32		nframes;
	wl_plt_srate_t	srate;
} wl_plt_txper_start_t;

typedef struct wl_plt_rxper_start {
	uint8		band;
	uint8		channel;
	uint8		seq_ctl;	/* PLT_SEQNU_OFF/ON */
	uint8		dst_mac[PLT_ETHER_ADDR_LEN];
	uint8		pad[3];
} wl_plt_rxper_start_t;

typedef struct wl_plt_rxper_results {
	uint32		frames;
	uint32		lost_frames;
	uint32		fcs_errs;
	uint32		plcp_errs;
	uint8		snr;
	uint8		rssi;
	uint8		pad[2];
} wl_plt_rxper_results_t;

typedef struct wl_plt_channel {
	uint8		band;
	uint8		channel;
} wl_plt_channel_t;

#define WLPLT_SUBCARRIER_MAX			56
#define WLPLT_SUBCARRIER_CENTRE			0
#define WLPLT_SUBCARRIER_LEFT_OF_CENTER		28
#define WLPLT_SUBCARRIER_FREQ_SPACING		312500

typedef struct wl_plt_tx_tone {
	wl_plt_channel_t	plt_channel;
	uint8			sub_carrier_idx;
	uint8			rsvd[1];
} wl_plt_tx_tone_t;

typedef struct wl_plt_desc {
	uchar		build_type[8];
	uchar		build_ver[32];
	uint		chipnum;
	uint		chiprev;
	uint		boardrev;
	uint		boardid;
	uint		ucoderev;
} wl_plt_desc_t;

#endif /* _wlplt_h_ */
