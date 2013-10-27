#ifdef BCMCCX

/*
 * ccx.h.c
 * Header file for CCX crypto functions
 *
 * $Copyright Broadcom Corporation$
 *
 * $Id: ccx.h,v 1.9 2006-06-14 21:10:12 gmo Exp $
 */

#ifndef _CCX_H_
#define _CCX_H_

extern void BCMROMFN(CKIP_key_permute)(uint8 *PK,		/* output permuted key */
				       const uint8 *CK,		/* input CKIP key */
				       uint8 toDsFromDs,	/* input toDs/FromDs bits */
				       const uint8 *piv);	/* input pointer to IV */

extern int BCMROMFN(wsec_ckip_mic_compute)(const uint8 *CK, const uint8 *pDA, const uint8 *pSA,
                                           const uint8 *pSEQ, const uint8 *payload, int payloadlen,
                                           const uint8 *p2, int len2, uint8 pMIC[]);
extern int BCMROMFN(wsec_ckip_mic_check)(const uint8 *CK, const uint8 *pDA, const uint8 *pSA,
                                         const uint8 *payload, int payloadlen, uint8 mic[]);

/* CCX v2 */

/* Key lengths in bytes */
#define CCKM_GK_LEN	48
#define CCKM_KRK_LEN	16
#define CCKM_BTK_LEN	32
#define CCKM_TKIP_PTK_LEN	64
#define CCKM_CKIP_PTK_LEN	CCKM_TKIP_PTK_LEN
#define CCKM_CCMP_PTK_LEN	48

#endif /* _CCX_H_ */

#endif /* BCMCCX */
