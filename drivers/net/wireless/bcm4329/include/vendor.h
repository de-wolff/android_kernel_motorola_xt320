/*
 * Windows/NDIS common macros that are the same for all brands (so far...)
 *
 * Copyright(c) 2001 Broadcom Corporation
 * $Id: vendor.h,v 1.79 2010-01-06 02:04:44 scottz Exp $
 */

#ifndef _VENDOR_H_
#define _VENDOR_H_

#define WIDE1(x)      L ## x
#define WIDE(x)      WIDE1(x)
#define TOSTR1(x)    # x
#define TOSTR(x)     TOSTR1(x)


/* we don't do a branded version of epindd ... yet */
#define NDD_FILE_BASE			"epindd"
#define NDD_DEV_BASE			NDD_FILE_BASE

#define NDD_DRIVER_NAME			NDD_FILE_BASE ".VXD"
#define NDD_DRIVER_SYS			NDD_FILE_BASE ".sys"
#define NDD_DRIVER_SERVICE		NDD_DEV_BASE
#define NDD_NT_DEVICE_NAME		WIDE("\\Device\\") WIDE(NDD_DEV_BASE)
#define NDD_DOS_DEVICE_NAME		WIDE("\\DosDevices\\") WIDE(NDD_DEV_BASE)

/* The RELAY macros depend upon RELAY_FILE_BASE & RELAY_DEV_BASE */
#define RELAY_NT_NAME           	RELAY_FILE_BASE ".SYS"
#define RELAY_9X_NAME           	RELAY_FILE_BASE ".VXD"
#define RELAY_NT_FILE           	"\\\\.\\" RELAY_DEV_BASE
#define RELAY_9X_FILE           	"\\\\.\\" RELAY_FILE_BASE ".VXD"
#define RELAY_DEVICE_NAME       	"\\Device\\" RELAY_DEV_BASE
#define RELAY_DOS_DEVICE_NAME   	"\\DosDevices\\" RELAY_DEV_BASE
#define RELAY_WIDE_PROT_NAME    	WIDE(RELAY_DEV_BASE)

/* The HWD macros depend upon HWD_FILE_BASE & HWD_DEV_BASE */
#define HWD_DRIVER_NAME			HWD_FILE_BASE ".VXD"
#define HWD_DRIVER_SYS			HWD_FILE_BASE ".SYS"
#define HWD_DRIVER_SERVICE		HWD_DEV_BASE
#define HWD_NT_DEVICE_NAME		WIDE("\\Device\\") WIDE(HWD_DEV_BASE)
#define HWD_DOS_DEVICE_NAME		WIDE("\\DosDevices\\") WIDE(HWD_DEV_BASE)

/* These seem to mostly be used by the NDI installer and the 
 * coinstaller to help recontruct registry keys when things go bad.
 */
#define DRIVER_KEY			WIDE(NIC_DEV_BASE)
#define DRIVER_NAME			WIDE(NIC_FILE_BASE) WIDE(".SYS")
#define DEFAULT_SERVICE			NIC_DEV_BASE
#define ENET_SERVICE			ENIC_DEV_BASE
#define DELETE_VALUE_NAME		NIC_DEV_BASE " File Delete"
#define NIC_DRIVER_SYS          	NIC_FILE_BASE ".SYS"

#define VEN_PRODUCTVERSION_STR  	VEN_FILEVERSION_STR

#define FILEVERSION_STR         	VEN_FILEVERSION_STR
#define PRODUCTVERSION_STR      	VEN_PRODUCTVERSION_STR
#define PRODUCTNAME             	VEN_PRODUCTNAME
#define COMPANYNAME             	VEN_COMPANYNAME

/* Define our own company and product name */
#define VER_COMPANYNAME_STR		VEN_COMPANYNAME "\0"
#define VER_PRODUCTNAME_STR		VEN_PRODUCTNAME "\0"
#define VER_LEGALCOPYRIGHT_YEARS	"1998-2010"
#define LEGALCOPYRIGHT			COMPANYNAME
#define VER_LEGALCOPYRIGHT_STR		VER_LEGALCOPYRIGHT_YEARS ", " LEGALCOPYRIGHT \
					" All Rights Reserved."
#define VER_PRODUCTVERSION_STR		VEN_PRODUCTVERSION_STR "\0"
#define VER_PRODUCTVERSION		VEN_FILEVERSION_NUM

#define CPANEL_NAME			CPANEL_FILE_BASE ".EXE"

#if defined(BCM_USB)
#define BCM_PLATFORM			"USB"
#else
#define BCM_PLATFORM			"PCI"
#endif

#define JOIN2(a, b)			a ## b
#define JOIN(a, b)			JOIN2(a, b)

#if defined(BCMENET)
#define VEN_COMPANYNAME			"Broadcom Corporation"
#define VEN_PRODUCTNAME			"Broadcom " TOSTR(BCM_CHIP) " " BCM_PLATFORM \
					" 10/100 Ethernet Network Adapter"
#define VEN_DRIVER_DESCRIPTION		"Broadcom " TOSTR(BCM_CHIP) \
					" 10/100 Ethernet Network Adapter Driver"
#define VEN_FILEVERSION_NUM     	EPI_MAJOR_VERSION, EPI_MINOR_VERSION, EPI_RC_NUMBER, \
					EPI_INCREMENTAL_NUMBER
#if defined(BCMINTERNAL)
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER) \
					" (BROADCOM INTERNAL DRIVER)\0"
#else
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER) "\0"
#endif
#define ENIC_DEV_BASE			TOSTR(ENIC_FILE_BASE)
#define NIC_NDIS50_DRIVER		ENIC_DEV_BASE ".SYS"

#else	/* !BCMENET */

#if defined(COMS)
#define VEN_REGENTRY			"3Com"
#define VEN_COMPANYNAME			"3Com Corporation"
#if defined(BCM_USB)
#define VEN_PRODUCTNAME			"3Com 3C420 HomeConnect(tm) 10M "  BCM_PLATFORM \
					" Phoneline Adapter"
#define VEN_DRIVER_DESCRIPTION		"3Com 3C420 HomeConnect(tm) 10M " BCM_PLATFORM " NIC"
#else
#define VEN_PRODUCTNAME			"3Com 3C410 HomeConnect(tm) 10M "  BCM_PLATFORM \
					" Phoneline Adapter"
#define VEN_DRIVER_DESCRIPTION		"3Com 3C410 HomeConnect(tm) 10M " BCM_PLATFORM " NIC"
#endif
#define COMS_BUILD_VERSION		TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER)
#define VEN_FILEVERSION_NUM     	EPI_MAJOR_VERSION, EPI_MINOR_VERSION, EPI_RC_NUMBER, \
					EPI_INCREMENTAL_NUMBER
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					COMS_BUILD_VERSION "\0"

#define DIAG_B0_NIC_DLL			"3C410NIC.DLL"
#define HWACCESS_DLL			"3C410HW.DLL"
#define CPANEL_FILE_BASE		"3C410DIA"
#define EPI_CTRL_DLL			"3C410CTL.DLL"

#define RELAY_FILE_BASE         	"3C410CTL"
#define RELAY_DEV_BASE          	"COMSCTL"

#define HWD_FILE_BASE           	"3C410IOA"
#define HWD_DEV_BASE            	"COMSIOA"

#if defined(BCM_USB)
#define NDI_DLL				"3C420NDI.DLL"
#define NIC_DEV_BASE			"3C420"
#define NIC_FILE_BASE			"3C420ND5"
#define FILEDESCRIPTION         	"3Com 3C420 HomeConnect(tm) 10M USB Phoneline Adapter"
#else
#define NDI_DLL				"3C410NDI.DLL"
#define NIC_DEV_BASE			"3C410"
#define NIC_FILE_BASE			"3C410ND"
#endif


#define NIC_NDIS30_DRIVER       	"3C410ND3.SYS"
#define NIC_NDIS40_DRIVER		"3C410ND4.SYS"
#if defined(BCM_USB)
#define NIC_NDIS50_DRIVER		"3C420ND5.SYS"
#else
#define NIC_NDIS50_DRIVER       	"3C410ND5.SYS"
#endif

#define VEN_COINSTALLER_NAME    	"3C410COI.DLL"

/* this needs to match ProductSoftwareName in the oemsetup.inf file */
#define PERFMON_SERVICE_NAME    	"HC10X"

#define PROPERTIES_NAME			"HomeConnect(tm) Properties"
#define LOGO_TEXT			"HomeConnect"
#define	DIAG_ICON_NAME			"HomeConnect"
#define DIAG_CAPTION			"3Com HomeConnect(tm) Information and Diagnostics"

#define COMS_MAJOR_VERSION      	TOSTR(EPI_MAJOR_VERSION)
#define COMS_MINOR_VERSION      	TOSTR(EPI_MINOR_VERSION)
#define COMS_VER_STRING         	COMS_MAJOR_VERSION "." COMS_MINOR_VERSION "\0." \
					COMS_BUILD_VERSION "\0"

#define DEFAULT_PROPERTIES_NAME		"3Com HomeConnect Properties"
#define DEFAULT_LOGO_TEXT		"3Com"
#define	DEFAULT_DIAG_ICON_NAME		"HomeConnect"
#define	DEFAULT_DIAGNOSTICS_TITLE	"HomeConnect Information and Diagnostics"
#define DEFAULT_COMPANY_URL		"http://www.3com.com/"

#ifdef VER_LEGALCOPYRIGHT_STR
#undef VER_LEGALCOPYRIGHT_STR
#endif
#define VER_LEGALCOPYRIGHT_STR		"Copyright \251 3Com Corp. " VER_LEGALCOPYRIGHT_YEARS

#define DEFAULT_LEGALCOPYRIGHT		VER_LEGALCOPYRIGHT_STR

#elif defined(NETGEAR)

#define VEN_REGENTRY			"NETGEAR"
#define VEN_COMPANYNAME			"NETGEAR, Inc."
#define VEN_PRODUCTNAME			"NETGEAR PA301 Phoneline10X " BCM_PLATFORM " Card"
#define VEN_DRIVER_DESCRIPTION		"NETGEAR PA301 Phoneline10X NIC"
#define VEN_FILEVERSION_NUM     	EPI_MAJOR_VERSION, EPI_MINOR_VERSION, EPI_RC_NUMBER, \
					EPI_INCREMENTAL_NUMBER
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER) "\0"

#define DIAG_B0_NIC_DLL			"PA301NB0.DLL"
#define HWACCESS_DLL			"PA301HW.DLL"
#define CPANEL_FILE_BASE		"PA301DIA"
#define NDI_DLL				"PA301NDI.DLL"
#define EPI_CTRL_DLL			"PA301CTL.DLL"

#define RELAY_FILE_BASE         	"PA301RLY"
#define RELAY_DEV_BASE          	RELAY_FILE_BASE

#define HWD_FILE_BASE           	"PA301IOA"
#define HWD_DEV_BASE            	HWD_FILE_BASE

#if defined(BCM_USB)
#define NIC_DEV_BASE			"PA301U"
#define NIC_FILE_BASE			"PA301U"
#else
#define NIC_DEV_BASE			"PA301ND"
#define NIC_FILE_BASE			"PA301ND"
#endif
#define NIC_NDIS30_DRIVER       	"PA301ND3.SYS"
#define NIC_NDIS40_DRIVER       	"PA301ND4.SYS"
#if defined(BCM_USB)
#define NIC_NDIS50_DRIVER       	"PA301U.SYS"
#else
#define NIC_NDIS50_DRIVER       	"PA301ND5.SYS"
#endif

#define VEN_COINSTALLER_NAME    	"PA301COI.DLL"
/* this needs to match ProductSoftwareName in the oemsetup.inf file */
#define PERFMON_SERVICE_NAME    	"PA301ND"

#define PROPERTIES_NAME			"NETGEAR PA301 Properties"
#define LOGO_TEXT			"NETGEAR"
#define	DIAG_ICON_NAME			"NETGEAR"
#define DIAG_CAPTION			"NETGEAR PA301 Information and Diagnostics"

#define DEFAULT_PROPERTIES_NAME		"NETGEAR Phoneline10X Properties"
#define DEFAULT_LOGO_TEXT		"NETGEAR"
#define	DEFAULT_DIAG_ICON_NAME		"NETGEAR"
#define	DEFAULT_DIAGNOSTICS_TITLE	"Phoneline10X Information and Diagnostics"
#define DEFAULT_COMPANY_URL		"http://netgear.baynetworks.com/"
#define DEFAULT_LEGALCOPYRIGHT		VER_LEGALCOPYRIGHT_STR

#elif defined(INTEL)

#define VEN_REGENTRY			"Intel"
#define VEN_COMPANYNAME			"Intel Corporation"
#define VEN_PRODUCTNAME			"Home network"
#define VEN_DRIVER_DESCRIPTION		"Intel(R) AnyPoint(tm) NIC"
#define FILEDESCRIPTION			"Home network software component"
#define VEN_FILEVERSION_NUM     	EPI_MAJOR_VERSION, EPI_MINOR_VERSION, EPI_RC_NUMBER, \
					EPI_INCREMENTAL_NUMBER
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER) "\0"

#define DIAG_B0_NIC_DLL			"HNBCPNIC.DLL"
#define HWACCESS_DLL			"HNBCPHW.DLL"
#define CPANEL_FILE_BASE		"HNBCPDIA"
#define EPI_CTRL_DLL			"HNBCPCTL.DLL"

#define RELAY_FILE_BASE         	"HNBCPCTL"
#define RELAY_DEV_BASE          	"HNBHCCTL"

#define HWD_FILE_BASE           	"HNBCPIOA"
#define HWD_DEV_BASE            	"HNBHCIOA"

#if defined(BCM_USB)
#define NIC_DEV_BASE			"HNBCU"
#define NIC_FILE_BASE			"HNBCU_5"
#define NDI_DLL				"HNBCUNDI.DLL"
#define NIC_NDIS50_DRIVER       	"HNBCU_5.SYS"
#define VEN_COINSTALLER_NAME    	"HNBCUCOI.DLL"
#else
#define NIC_DEV_BASE			"HNBCP"
#define NIC_FILE_BASE			"HNBCPND"
#define NDI_DLL				"HNBCPNDI.DLL"
#define NIC_NDIS50_DRIVER       	"HNBCP_5.SYS"
#define VEN_COINSTALLER_NAME    	"HNBCPCOI.DLL"
#endif

#define NIC_NDIS30_DRIVER       	"HNBCP_3.SYS"
#define NIC_NDIS40_DRIVER       	"HNBCP_4.SYS"

/* this needds to match ProductSoftwareName in the oemsetup.inf file */
#define PERFMON_SERVICE_NAME    	"HNBCP"

/* Intel uses no diagnostics applet */
#define NO_DIAG

#define PROPERTIES_NAME			"AnyPoint(tm) Properties"
#define LOGO_TEXT			"AnyPoint"
#define	DIAG_ICON_NAME			"AnyPoint"
#define DIAG_CAPTION			"Intel HomeConnect(tm) Information and Diagnostics"

#define DEFAULT_PROPERTIES_NAME		"Intel AnyPoint(tm) Properties"
#define DEFAULT_LOGO_TEXT		"Intel"
#define	DEFAULT_DIAG_ICON_NAME		"AnyPoint"
#define	DEFAULT_DIAGNOSTICS_TITLE	"AnyPoint(tm) Information and Diagnostics"
#define DEFAULT_COMPANY_URL		"http://www.intel.com/"
#ifdef VER_LEGALCOPYRIGHT_STR
#undef VER_LEGALCOPYRIGHT_STR
#endif
#define VER_LEGALCOPYRIGHT_STR		"Copyright (C) 1996-2000, Intel Corporation\0"

#else	/* unbranded release */

#define VEN_REGENTRY			"Broadcom"
#define VEN_COMPANYNAME			"Broadcom Corporation"
#define VEN_PRODUCTNAME			"Broadcom iLine10(tm) " BCM_PLATFORM " Network Adapter"
#define VEN_DRIVER_DESCRIPTION		"Broadcom iLine10(tm) Network Adapter Driver"
#define VEN_FILEVERSION_NUM     	EPI_MAJOR_VERSION, EPI_MINOR_VERSION, EPI_RC_NUMBER, \
					EPI_INCREMENTAL_NUMBER
#if defined(BCMINTERNAL)
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER) \
					" (BROADCOM INTERNAL DRIVER)\0"
#else
#define VEN_FILEVERSION_STR     	TOSTR(EPI_MAJOR_VERSION) "." TOSTR(EPI_MINOR_VERSION) "." \
					TOSTR(EPI_RC_NUMBER) "." TOSTR(EPI_INCREMENTAL_NUMBER) "\0"
#endif

#define DIAG_B0_NIC_DLL			"BCM42DB0.DLL"
#define HWACCESS_DLL			"BCM42IOA.DLL"
#define CPANEL_FILE_BASE		"BCM42DIA"
#define NDI_DLL				"BCM42NDI.DLL"
#define EPI_CTRL_DLL			"BCM42CTL.DLL"

#define RELAY_FILE_BASE         	"BCM42RLY"
#define RELAY_DEV_BASE          	RELAY_FILE_BASE

#define HWD_FILE_BASE           	"BCM42XHW"
#define HWD_DEV_BASE            	HWD_FILE_BASE

#if defined(BCM_USB)
#define NIC_DEV_BASE			"BCM42U"
#define NIC_FILE_BASE			"BCM42U"
#else
#define NIC_FILE_BASE			"BCM42XX"
#define NIC_DEV_BASE			NIC_FILE_BASE
#endif

#define NIC_NDIS30_DRIVER       	"BCM42XX3.SYS"
#define NIC_NDIS40_DRIVER       	"BCM42XX4.SYS"
#if defined(BCM_USB)
#define NIC_NDIS50_DRIVER       	"BCM42U.SYS"
#else
#define NIC_NDIS50_DRIVER       	"BCM42XX5.SYS"
#endif

#define VEN_COINSTALLER_NAME    	"BCM42COI.DLL"
/* this needs to match ProductSoftwareName in the oemsetup.inf file */
#define PERFMON_SERVICE_NAME    	"BCM42XX"

#define PROPERTIES_NAME			"Broadcom iLine(tm) Properties"
#define LOGO_TEXT			"Broadcom"
#define	DIAG_ICON_NAME			"iLine10"
#define DIAG_CAPTION			"Broadcom ILine10(tm) Information and Diagnostics"

#define DEFAULT_PROPERTIES_NAME		"Broadcom iLine(tm) Properties"
#define DEFAULT_LOGO_TEXT		"Broadcom"
#define	DEFAULT_DIAG_ICON_NAME		"iLine10"
#define	DEFAULT_DIAGNOSTICS_TITLE	"iLine10 Information and Diagnostics"
#define DEFAULT_COMPANY_URL		"http://www.Broadcom.com/"
#define DEFAULT_LEGALCOPYRIGHT		VER_LEGALCOPYRIGHT_STR

#endif	/* unbranded release */
#endif	/* BCMENET */

#endif /* _VENDOR_H_ */
