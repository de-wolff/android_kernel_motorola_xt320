/*
 * OID encapsulation defines for user-mode to driver interface.
 *
 * Definitions subject to change without notice.
 *
 * $Copyright (C) 2001-2003 Broadcom Corporation$
 *
 * $Id: oidencap.h,v 13.8 2007-07-14 02:07:36 dmo Exp $
 */

#ifndef _oidencap_h_
#define	_oidencap_h_

/*
 * NOTE: same as OID_EPI_BASE defined in epiioctl.h
 */
#define OID_BCM_BASE					0xFFFEDA00

/*
 * These values are now set in stone to preserve forward
 * binary compatibility.
 */
#define	OID_BCM_SETINFORMATION 			(OID_BCM_BASE + 0x3e)
#define	OID_BCM_GETINFORMATION 			(OID_BCM_BASE + 0x3f)
#ifdef WLFIPS
#define OID_FSW_FIPS_MODE			(OID_BCM_BASE + 0x40)
#endif /* WLFIPS */
#define OID_DHD_IOCTLS					(OID_BCM_BASE + 0x41)

#if defined(BCMCCX) && defined(CCX_SDK)
#define OID_BCM_CCX	0x00181000	/* based on BRCM_OUI value */
#endif /* BCMCCX && CCX_SDK */

#define	OIDENCAP_COOKIE	0xABADCEDE

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

/* 
 * In the following two structs keep cookie as last element
 * before data. This allows struct validation when fields
 * are added or deleted.  The data immediately follows the
 * structure and is required to be 4-byte aligned.
 *
 * OID_BCM_SETINFORMATION uses setinformation_t
 * OID_BCM_GETINFORMATION uses getinformation_t
*/
typedef struct _setinformation {
    ULONG cookie;	/* OIDENCAP_COOKIE */
    ULONG oid;	/* actual OID value for set */
} setinformation_t;

#define SETINFORMATION_SIZE			(sizeof(setinformation_t))
#define SETINFORMATION_DATA(shdr)		((UCHAR *)&(shdr)[1])

typedef struct _getinformation {
    ULONG oid;	/* actual OID value for query */
    ULONG len;	/* length of response buffer, including this header */
    ULONG cookie;	/* OIDENCAP_COOKIE; altered by driver if more data available */
} getinformation_t;

#define GETINFORMATION_SIZE			(sizeof(getinformation_t))
#define GETINFORMATION_DATA(ghdr)		((UCHAR *)&(ghdr)[1])

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#endif /* _oidencap_h_ */
