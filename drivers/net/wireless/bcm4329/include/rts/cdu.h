/*
 * Copyright 1997 Epigram, Inc.
 *
 * $Id: cdu.h,v 1.11 1999-11-16 02:37:26 nn Exp $
 *
 * cdu.h - defs for circular data unit
 */

#ifndef _CDU_H_
#define _CDU_H_

#include <assert.h>
#include "typedefs.h"
#include "bcmendian.h"

typedef uint32 cdu_header_t;

/* cdu related constants and macros */
#define CDU_SYNC_MASK   0xff000000
#define CDU_SYNC        0x6e000000

#define CDU_TYPE_MASK   0x00ff0000
#define CDU_DATA        0x00010000
#define CDU_SOF         0x00020000
#define CDU_EOF         0x00040000
#define CDU_ERR         0x00080000

/* values that can be used in NEW_CDU_HEADER */
#define CDU_TYPE_DATA        0x01
#define CDU_TYPE_SOF         0x02
#define CDU_TYPE_EOF         0x04
#define CDU_TYPE_ERR         0x08

#define CDU_LENGTH_MASK 0x0000ffff

/* 
 * since we're dealing with cdu headers as uint32s, we need endian
 * dependent load/store macros to ensure big-endian byte ordering
 * in the byte stream.
 *
 * FIXME: should use ntohl(), htonl(),
 *        [BYTE_ORDER shouldn't appear in this file]
 */

#if (BYTE_ORDER == LITTLE_ENDIAN)

#define LOAD_CDU_HEADER(pbuf, header) \
    assert((((uint32)(pbuf)) & 3) == 0); \
    (header) = swap32(((cdu_header_t *)pbuf)[0])

#define STORE_CDU_HEADER(pbuf, header) \
    assert((((uint32)(pbuf)) & 3) == 0); \
    ((cdu_header_t *)pbuf)[0] = swap32(header)

#define LOAD_CDU_LENGTH(pbuf, length) \
    (assert((((uint32)pbuf) & 1) == 0), length = swap16(((uint16 *)pbuf)[1]))

#define READ_CDU_LENGTH(pbuf) \
    (assert((((uint32)pbuf) & 1) == 0), swap16(((uint16 *)pbuf)[1]))

#define STORE_CDU_LENGTH(pbuf, length) \
    (assert((((uint)pbuf) & 1) == 0), ((uint16 *)pbuf)[1] = swap16((uint16)((length) & 65535)))

#else /* (BYTE_ORDER == BIG_ENDIAN) */

#define LOAD_CDU_HEADER(pbuf, header) \
    assert((((uint32)(pbuf)) & 3) == 0); \
    (header) = ((cdu_header_t *)pbuf)[0]

#define STORE_CDU_HEADER(pbuf, header) \
    assert((((uint32)(pbuf)) & 3) == 0); \
    ((cdu_header_t *)pbuf)[0] = (header)

/* like other loads, with a target, but also an expression of type uint16 */
#define LOAD_CDU_LENGTH(pbuf, length) \
    (assert((((uint32)pbuf) & 1) == 0), length = ((uint16 *)pbuf)[1])

/* an expression of type uint16 which doesn't need a temp variable */
#define READ_CDU_LENGTH(pbuf) \
    (assert((((uint32)pbuf) & 1) == 0), ((uint16 *)pbuf)[1])

#define STORE_CDU_LENGTH(pbuf, length) \
    (assert((((uint32)pbuf) & 1) == 0), ((uint16 *)pbuf)[1] = (uint16)((uint16)length & (uint16)65535))

#endif /* (BYTE_ORDER == LITTLE_ENDIAN) */

#define CDU_SYNC_VALID(header) \
    (((header) & CDU_SYNC_MASK) == CDU_SYNC)

/* extract the type byte and shift to LSB */
#define CDU_TYPE_BYTE(header) \
    (((header) & CDU_TYPE_MASK) >> 16)

/* set the type with the byte constants */
#define SET_CDU_TYPE_BYTE(header, type_byte)             \
    do {                                                 \
        (header) &= ~CDU_TYPE_MASK;                      \
        (header) |= ((type_byte) << 16) & CDU_TYPE_MASK; \
    } while (0)

/* extract the cdu type */
#define CDU_TYPE(header) \
    ((header) & CDU_TYPE_MASK)

/* set the type with the CDU_* constants */
#define SET_CDU_TYPE(header, type)          \
    do {                                    \
        (header) &= ~CDU_TYPE_MASK;         \
        (header) |= (type) & CDU_TYPE_MASK; \
    } while (0)

#define NEW_CDU_HEADER(type, length) \
    ((assert(((type) < 256) && ((length) < 65536))), \
     (CDU_SYNC | ((type) << 16) | (length)))

#define CDU_LENGTH(header) \
    ((header) & CDU_LENGTH_MASK)

#define SET_CDU_LENGTH(header, length)          \
    do {                                        \
        (header) &= ~CDU_LENGTH_MASK;           \
        (header) |= (length) & CDU_LENGTH_MASK; \
    } while (0)

/* 
 * the end of a cdu must be padded up to a multiple of 4 bytes.  Even though
 * the pad bytes are "don't care" bytes, we pad with zeros for consistency.
 * This doesn't cost much and allows for checking data files using tools like
 * 'cmp' without need to extract/ignore the don't care bytes.
 */

#define FILL_CDU_PAD(pbuf)                  \
    do {                                    \
        while ((((uint32)pbuf) & 3) != 0) { \
            ((uint8 *)pbuf)[0] = 0;         \
            ((uint8 *)pbuf)++;              \
        }                                   \
    } while (0)

#define SKIP_CDU_PAD(pbuf) \
    ((void *)((((uint32)pbuf) + 3) & ~3))

/*
 * "open" a cdu - save a pointer to where the header will be stored , init
 * the header, and update the buffer pointer.
 */

#define OPEN_DATA_CDU(pheader, header, pbuf) \
    assert((((uint32)(pbuf)) & 3) == 0);     \
    (pheader) = (cdu_header_t *)(pbuf);      \
    (header) = CDU_SYNC | CDU_DATA;          \
    (pbuf) = (void*)(((uint32)pbuf) + sizeof(cdu_header_t))

/*
 * "close" a cdu - add the final cdu length to the header and store it
 * at the location specified by the previously saved pointer.
 * Increment the pbuf to the next 4-byte boundary.
 */

#define CLOSE_CDU(pheader, header, pbuf)                                 \
    SET_CDU_LENGTH(header, (pbuf) - ((uint8 *)((pheader) + 1)));         \
    STORE_CDU_HEADER(pheader, header);                                   \
    FILL_CDU_PAD(pbuf)

#endif /* _CDU_H_ */
