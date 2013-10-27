/* hmac_sha256.h
 * Code copied from openssl distribution and
 * Modified just enough so that compiles and runs standalone
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: hmac_sha256.h,v 1.3.598.1 2010-09-02 18:09:38 scottz Exp $
 */
/* ====================================================================
 * Copyright (c) 2004 The OpenSSL Project.  All rights reserved
 * according to the OpenSSL license [found in ../../LICENSE].
 * ====================================================================
 */
void hmac_sha256(const void *key, int key_len,
                 const unsigned char *text, size_t text_len,
                 unsigned char *digest,
                 unsigned int *digest_len);
void hmac_sha256_n(const void *key, int key_len,
                   const unsigned char *text, size_t text_len,
                   unsigned char *digest,
                   unsigned int digest_len);
void sha256(const unsigned char *text, size_t text_len, unsigned char *digest,
            unsigned int digest_len);
