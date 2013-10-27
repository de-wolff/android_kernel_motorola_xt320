/*
 * Copyright (c) 2007 Apple Computer, Inc.  All rights reserved.
 *
 * Module Name:
 *
 *	apple80211
 *
 * Abstract:
 *
 *	
 *
 */
 
#ifndef _APPLE80211_H_
#define _APPLE80211_H_

#define APPLE_80211_PROTOCOL_GUID \
			{0x71B4903C, 0x14EC, 0x42C4, 0xBD, 0xC6, 0xCE,0x14,0x49,0x93,0x0E,0x49}


//???? What's the purpose of APPLE_80211_MODE? 
typedef struct {
	UINT32						State;
	EFI_MAC_ADDRESS				HwAddress;
} APPLE_80211_MODE;


typedef EFI_STATUS (EFIAPI *APPLE_80211_IOCTL) (
								IN		void						*This,
								IN		EFI_GUID					*SelectorGuid,
								IN		VOID						*InParams,
								IN		UINTN						InSize,
								OUT		VOID						*OutParams,
								IN OUT	UINT32						*OutSize
								);

typedef struct _APPLE_80211_PROTOCOL_ {
	APPLE_80211_IOCTL		Ioctl;
	APPLE_80211_MODE		*mode;
} APPLE_80211_PROTOCOL;


// Scan Request
//		IN:		APPLE_80211_SCANPARAMS *scanreq
//		OUT:	(none)
//		RETURN:	success, busy, err
#define APPLE_80211_IOCTL_SCAN_GUID \
			{0x60BD5F23, 0xDBDC, 0x4AC4, 0x9B, 0x13, 0x61,0x62,0xED,0xB5,0xE1,0xD3}

#define APPLE80211_MAX_SSID_LEN		32
#define APPLE80211_MAX_CHANNELS		64

typedef struct {
	UINT32			SsidLen;
	UINT8			Ssid[ APPLE80211_MAX_SSID_LEN ];
	UINT16			ActiveDwellTime;				/* ms per chan, 0 -> use default */
	UINT16			PassiveDwellTime;				/* ms per chan, 0 -> use default */
	UINT32			NumChannels;			/* 0 -> all available channels */
	UINT32			Channels[ APPLE80211_MAX_CHANNELS ];
} APPLE_80211_SCANPARAMS;


// Scan Result
//		IN:		APPLE_80211_SR_REQ *req
//		OUT:	APPLE_80211_SCANRESULT *result
//		RETURN:	success, busy, err
#define APPLE_80211_IOCTL_SCANRESULT_GUID \
			{0x580B0A0E, 0xF4D2, 0x44FF, 0xAF, 0x26, 0x41,0x4B,0x82,0x0F,0x02,0x97}

typedef struct 
{
	UINT32		index;
	UINT8		ie[256];	// Information Element filter (when length byte != 0)
} APPLE_80211_SR_REQ;

typedef struct 
{
	UINT32				channel;	// 0 when index exceeds # of networks found
	UINT16				noise;
	UINT16				rssi;
	UINT16				cap;		// capabilities
	EFI_MAC_ADDRESS		bssid;
	UINT16				ssid_len;
	UINT8				ssid[ APPLE80211_MAX_SSID_LEN ];
	UINT32				advCap;		// advanced capabilities
} APPLE_80211_SCANRESULT;

// bitfield def's for advanced capabilities field in scan results
enum apple80211_cap
{
	APPLE80211_CAP_ESS			= 0x0001,	// Infra-BSS
	APPLE80211_CAP_IBSS			= 0x0002,	// IBSS
	APPLE80211_CAP_PRIVACY		= 0x0010	// Security
};

// bitfield def's for advanced capabilities field in scan results
enum apple80211_advcap
{
	APPLE80211_AC_WEP			= 0x0001,	// WEP
	APPLE80211_AC_WPA			= 0x0002,	// original WPA
	APPLE80211_AC_WPA2			= 0x0004,	// WPA2
	APPLE80211_AC_PSK			= 0x0008,	// WPA1/2 PSK
	APPLE80211_AC_Enterprise	= 0x0010,	// WPA1/2 Enterprise (EAP)
	APPLE80211_AC_TKIP			= 0x0020,	// TKIP
	APPLE80211_AC_AES			= 0x0040,	// AES-CCM
};


// Joining
//		IN:		APPLE_80211_JOINPARAMS *joinParams
//		OUT:	(none)
//		RETURN:	success, err
#define APPLE_80211_IOCTL_JOIN_GUID \
			{0x2B7682F6, 0x4088, 0x4C6E, 0x90, 0xDC, 0x7A,0x2C,0x3B,0x76,0x58,0x3E}

#define APPLE80211_KEY_BUFF_LEN		32
#define APPLE80211_MAX_RSN_IE_LEN	257		// 255 + type and length bytes

// Low level 802.11 authentication types
enum apple80211_authtype_lower
{
	APPLE80211_AUTHTYPE_OPEN		= 1,	// open
	APPLE80211_AUTHTYPE_SHARED		= 2,	// shared key
	APPLE80211_AUTHTYPE_CISCO		= 3,	// cisco net eap
};

// Higher level authentication used after 802.11 association complete
enum apple80211_authtype_upper
{
	APPLE80211_AUTHTYPE_NONE		= 0,	//	No upper auth
	APPLE80211_AUTHTYPE_WPA			= 1,	//	WPA
	APPLE80211_AUTHTYPE_WPA_PSK		= 2,	//	WPA PSK
	APPLE80211_AUTHTYPE_WPA2		= 3,	//	WPA2
	APPLE80211_AUTHTYPE_WPA2_PSK	= 4,	//	WPA2 PSK
	APPLE80211_AUTHTYPE_LEAP		= 5,	//	LEAP
	APPLE80211_AUTHTYPE_8021X		= 6,	//	802.1x
	APPLE80211_AUTHTYPE_WPS			= 7,	//	WiFi Protected Setup
};

enum apple80211_cipher_type {  
	APPLE80211_CIPHER_NONE	  = 0,		// open network
	APPLE80211_CIPHER_WEP_40  = 1,		// 40 bit WEP
	APPLE80211_CIPHER_WEP_104 = 2,		// 104 bit WEP
	APPLE80211_CIPHER_TKIP	  = 3,		// TKIP (WPA)
	APPLE80211_CIPHER_AES_OCB = 4,		// AES (OCB)
	APPLE80211_CIPHER_AES_CCM = 5,		// AES (CCM)
	APPLE80211_CIPHER_PMK	  = 6,		// PMK
};

typedef struct {
	UINT32			CipherType;			// apple80211_cipher_type
	UINT16			KeyFlags;
	UINT16			KeyIndex;
	CHAR8			Password[65];			// NULL if using actual key
	UINT32			KeyLen;				// 0 if using password
	UINT8			Key[ APPLE80211_KEY_BUFF_LEN ];
} APPLE_80211_KEY;

enum apple80211_bsstype
{
	APPLE80211_BSSTYPE_IBSS		= 1,	//	IBSS (adhoc)
	APPLE80211_BSSTYPE_INFRA	= 2,	//	Infra-BSS (AP)
	APPLE80211_BSSTYPE_ANY		= 3,	//	Any mode
	APPLE80211_BSSTYPE_NONE		= 255	//	disassociate and/or abort associate
};

typedef struct {
	UINT16			bssType;
	UINT16			LowerAuth;
	UINT16			UpperAuth;
	UINT32			SsidLen;
	UINT8			Ssid[ APPLE80211_MAX_SSID_LEN ];
	EFI_MAC_ADDRESS	bssid;
	APPLE_80211_KEY	Key;	
} APPLE_80211_JOINPARAMS;


// Current State
//		IN:		(none)
//		OUT:	UINT32 *state
//		RETURN:	success, err
#define APPLE_80211_IOCTL_STATE_GUID \
			{0x514C8994, 0x2910, 0x47C9, 0x9F, 0xB6, 0x7C,0x7C,0x5E,0xAC,0x02,0x6C}

enum apple80211_state {
		// joining states
	APPLE80211_S_UNDEF	= 0,			// default state (unpowered?)
	APPLE80211_S_IDLE	= 1,			// powered & idle
	APPLE80211_S_SCAN	= 2,			// scanning
	APPLE80211_S_ASSOC	= 3,			// associating (includes 80211 Auth)
	APPLE80211_S_AUTH	= 4,			// upper level authenticating
	APPLE80211_S_RUN	= 5,			// associated (and upper level auth complete)
		// failure states
	APPLE80211_S_BADKEY		= 6,		// incorrect password
	APPLE80211_S_BADAUTH	= 7,		// auth method not supported
	APPLE80211_S_AUTHFAIL	= 8,		// authentication failure
	APPLE80211_S_ASSOCFAIL	= 9,		// association failure
	APPLE80211_S_NONETWORK	= 10,		// desired ssid not found
	APPLE80211_S_OTHER		= 11,		// indeterminate failure
};


// Current Network Info
//		IN:		(none)
//		OUT:	APPLE_80211_INFO *info
//		return:	success, err
#define APPLE_80211_IOCTL_INFO_GUID \
			{0x2AE26F3D, 0x6258, 0x4C4A, 0xBC, 0x18, 0xAD,0xF9,0x96,0xFD,0x98,0x71}

typedef struct {
	UINT32			Channel;			// 0 if not associated
	UINT32			SsidLen;
	UINT8			Ssid[ APPLE80211_MAX_SSID_LEN ];
	EFI_MAC_ADDRESS	bssid;
	UINT16			Noise;
	UINT16			RSSI;
} APPLE_80211_INFO;


// Create IBSS
//		IN:		APPLE_80211_IBSSPARAMS *ibssParams
//		OUT:	(none)
//		return:	success, err
#define APPLE_80211_IOCTL_IBSS_GUID \
			{0xAE12F51C, 0x87A9, 0x4092, 0xAA, 0xE5, 0x7C,0x50,0x68,0xB3,0x9F,0x0C}

typedef struct {
	UINT32			ssidLen;
	UINT8			ssid[ APPLE80211_MAX_SSID_LEN ];
	UINT32			channel;
	UINT32			features;			// placeholder, not used at the moment
	UINT32			keyLen;				// keyLen is 5 or 13 (or 0 if no security)
	UINT8			key[ APPLE80211_KEY_BUFF_LEN ];
} APPLE_80211_IBSSPARAMS;

extern EFI_GUID gBcmwlIoctlGuid;
extern EFI_GUID gApple80211ProtocolGuid;
extern EFI_GUID gApple80211IoctlScanGuid;
extern EFI_GUID gApple80211IoctlScanResultGuid;
extern EFI_GUID gApple80211IoctlJoinGuid;
extern EFI_GUID gApple80211IoctlStateGuid;
extern EFI_GUID gApple80211IoctlInfoGuid;
extern EFI_GUID gApple80211IoctlIBSSGuid;
#endif _APPLE80211_H_
