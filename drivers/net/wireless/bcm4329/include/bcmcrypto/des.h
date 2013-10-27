/*
 * Interface definitions for DES.
 * Copied from des-ka9q-1.0-portable. a public domain DES implementation.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: des.h,v 1.4 2006-06-14 21:10:12 gmo Exp $
 */

#ifndef _DES_H_
#define _DES_H_

typedef unsigned long DES_KS[16][2];	/* Single-key DES key schedule */

void BCMROMFN(deskey)(DES_KS, unsigned char *, int);

void BCMROMFN(des)(DES_KS, unsigned char *);

#endif /* _DES_H_ */
