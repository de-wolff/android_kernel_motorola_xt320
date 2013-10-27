/*
 * prf.h
 * PRF function used in WPA and TGi
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: prf.h,v 1.6 2007-04-25 21:01:48 jkoenig Exp $
 */

#ifndef _PRF_H_
#define _PRF_H_

#include <typedefs.h>

/* lengths in Bytes */
#define PRF_MAX_I_D_LEN	128
#define PRF_MAX_KEY_LEN	64
#define PRF_OUTBUF_LEN	80

extern int BCMROMFN(PRF)(unsigned char *key, int key_len, unsigned char *prefix,
                         int prefix_len, unsigned char *data, int data_len,
                         unsigned char *output, int len);

extern int BCMROMFN(fPRF)(unsigned char *key, int key_len, unsigned char *prefix,
                          int prefix_len, unsigned char *data, int data_len,
                          unsigned char *output, int len);

extern void BCMROMFN(hmac_sha1)(unsigned char *text, int text_len, unsigned char *key,
                                int key_len, unsigned char *digest);

extern void BCMROMFN(hmac_md5)(unsigned char *text, int text_len, unsigned char *key,
                               int key_len, unsigned char *digest);

#endif /* _PRF_H_ */
