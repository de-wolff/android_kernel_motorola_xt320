/*
 * bn_bcmcrypto.h: Header file to avoid symbol conflict when linking with openssl.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: bn_bcmcrypto.h,v 1.1.4.1.2.1 2010-02-22 21:56:35 tomhsu Exp $
 */

#ifndef _BN_BCMCRYPTO_H_
#define _BN_BCMCRYPTO_H_

#define BN_add			BCMCrypto_BN_add
#define BN_add_word		BCMCrypto_BN_add_word
#define bn_add_words		BCMCrypto_bn_add_words
#define BN_bin2bn		BCMCrypto_BN_bin2bn
#define BN_bn2bin		BCMCrypto_BN_bn2bin
#define BN_clear_free		BCMCrypto_BN_clear_free
#define bn_cmp_words		BCMCrypto_bn_cmp_words
#define BN_copy			BCMCrypto_BN_copy
#define BN_CTX_end		BCMCrypto_BN_CTX_end
#define BN_CTX_free		BCMCrypto_BN_CTX_free
#define BN_CTX_get		BCMCrypto_BN_CTX_get
#define BN_CTX_init		BCMCrypto_BN_CTX_init
#define BN_CTX_new		BCMCrypto_BN_CTX_new
#define BN_CTX_start		BCMCrypto_BN_CTX_start
#define BN_div			BCMCrypto_BN_div
#define bn_div_words		BCMCrypto_bn_div_words
#define bn_expand2		BCMCrypto_bn_expand2
#define bn_expand_internal	BCMCrypto_bn_expand_internal
#define BN_free			BCMCrypto_BN_free
#define BN_from_montgomery	BCMCrypto_BN_from_montgomery
#define BN_init			BCMCrypto_BN_init
#define BN_is_bit_set		BCMCrypto_BN_is_bit_set
#define BN_lshift		BCMCrypto_BN_lshift
#define BN_lshift1		BCMCrypto_BN_lshift1
#define BN_mod_exp_mont		BCMCrypto_BN_mod_exp_mont
#define BN_mod_exp_mont_word	BCMCrypto_BN_mod_exp_mont_word
#define BN_mod_inverse		BCMCrypto_BN_mod_inverse
#define BN_mod_mul_montgomery	BCMCrypto_BN_mod_mul_montgomery
#define BN_MONT_CTX_free	BCMCrypto_BN_MONT_CTX_free
#define BN_MONT_CTX_init	BCMCrypto_BN_MONT_CTX_init
#define BN_MONT_CTX_new		BCMCrypto_BN_MONT_CTX_new
#define BN_MONT_CTX_set		BCMCrypto_BN_MONT_CTX_set
#define BN_mul			BCMCrypto_BN_mul
#define bn_mul_add_words	BCMCrypto_bn_mul_add_words
#define bn_mul_comba4		BCMCrypto_bn_mul_comba4
#define bn_mul_comba8		BCMCrypto_bn_mul_comba8
#define bn_mul_high		BCMCrypto_bn_mul_high
#define bn_mul_normal		BCMCrypto_bn_mul_normal
#define bn_mul_part_recursive	BCMCrypto_bn_mul_part_recursive
#define bn_mul_recursive	BCMCrypto_bn_mul_recursive
#define BN_mul_word		BCMCrypto_BN_mul_word
#define bn_mul_words		BCMCrypto_bn_mul_words
#define BN_new			BCMCrypto_BN_new
#define BN_nnmod		BCMCrypto_BN_nnmod
#define BN_num_bits		BCMCrypto_BN_num_bits
#define BN_num_bits_word	BCMCrypto_BN_num_bits_word
#define bnrand			BCMCrypto_bnrand
#define BN_rand			BCMCrypto_BN_rand
#define bn_rand_fn		BCMCrypto_bn_rand_fn
#define BN_register_RAND	BCMCrypto_BN_register_RAND
#define BN_rshift		BCMCrypto_BN_rshift
#define BN_rshift1		BCMCrypto_BN_rshift1
#define BN_set_bit		BCMCrypto_BN_set_bit
#define BN_set_word		BCMCrypto_BN_set_word
#define BN_sqr			BCMCrypto_BN_sqr
#define bn_sqr_comba4		BCMCrypto_bn_sqr_comba4
#define bn_sqr_comba8		BCMCrypto_bn_sqr_comba8
#define bn_sqr_normal		BCMCrypto_bn_sqr_normal
#define bn_sqr_recursive	BCMCrypto_bn_sqr_recursive
#define bn_sqr_words		BCMCrypto_bn_sqr_words
#define BN_sub			BCMCrypto_BN_sub
#define BN_sub_word		BCMCrypto_BN_sub_word
#define bn_sub_words		BCMCrypto_bn_sub_words
#define BN_uadd			BCMCrypto_BN_uadd
#define BN_ucmp			BCMCrypto_BN_ucmp
#define BN_usub			BCMCrypto_BN_usub
#define BN_value_one		BCMCrypto_BN_value_one

#define RAND_bytes		BCMCrypto_RAND_bytes

#define dh_bn_mod_exp		BCMCrypto_dh_bn_mod_exp
#define DH_compute_key		BCMCrypto_DH_compute_key
#define DH_free			BCMCrypto_DH_free
#define DH_generate_key		BCMCrypto_DH_generate_key
#define DH_init			BCMCrypto_DH_init
#define DH_new			BCMCrypto_DH_new

#define rijndaelDecrypt		BCMCrypto_rijndaelDecrypt
#define rijndaelEncrypt		BCMCrypto_rijndaelEncrypt
#define rijndaelKeySetupDec	BCMCrypto_rijndaelKeySetupDec
#define rijndaelKeySetupEnc	BCMCrypto_rijndaelKeySetupEnc
#define Te0			BCMCrypto_Te0
#define Te1			BCMCrypto_Te1
#define Te2			BCMCrypto_Te2
#define Te3			BCMCrypto_Te3
#define Te4			BCMCrypto_Te4
#define Td0			BCMCrypto_Td0
#define Td1			BCMCrypto_Td1
#define Td2			BCMCrypto_Td2
#define Td3			BCMCrypto_Td3
#define Td4			BCMCrypto_Td4

#define SHA224			BCMCrypto_SHA224
#define SHA224_Init		BCMCrypto_SHA224_Init
#define SHA256			BCMCrypto_SHA256
#define SHA256_Final		BCMCrypto_SHA256_Final
#define SHA256_Init		BCMCrypto_SHA256_Init
#define SHA256_Transform	BCMCrypto_SHA256_Transform
#define SHA256_Update		BCMCrypto_SHA256_Update
#define SHA256_version		BCMCrypto_SHA256_version

#endif /* _BN_BCMCRYPTO_H_ */
