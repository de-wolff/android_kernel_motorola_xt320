/*******************************************************************************
 * $Id: crc8.h,v 7.2 1999-11-01 19:36:06 csw Exp $
 * crc8.h - a function to compute crc8 for VIP headers
 ******************************************************************************/

#ifndef _RTS_CRC8_H_
#define _RTS_CRC8_H_ 1

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRC8_INIT_VALUE	0x0   /* Initial CRC8 checksum value */
#define CRC8_GOOD_VALUE	0x0   /* Good final CRC8 checksum value */

uint8 crc8(uint8 *, uint32);

#ifdef __cplusplus
}
#endif

#endif /* _RTS_CRC8_H_ */
