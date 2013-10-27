/*******************************************************************************
 * $Id: flags.h,v 1.2 1997-06-13 22:55:11 khp Exp $
 * flags.h - flag types and ops
 ******************************************************************************/

#ifndef _FLAGS_H_
#define _FLAGS_H_

typedef volatile uint8  flags8_t;
typedef volatile uint16 flags16_t;
typedef volatile uint32 flags32_t;

#define SET_FLAG(pflags, flag)    ((*pflags) |= (flag))
#define CLR_FLAG(pflags, flag)    ((*pflags) &= (~flag));
#define FLAG_IS_SET(pflags, flag) (((*pflags) & (flag)) != 0)
#define FLAG_IS_CLR(pflags, flag) (((*pflags) & (flag)) == 0)

#define WAIT_FLAG_SET(pflags, flag)   \
    while (((*pflags) & (flag)) == 0) \
        Sleep(1)

#define WAIT_FLAG_CLR(pflags, flag)   \
    while (((*pflags) & (flag)) != 0) \
        Sleep(1)

#endif /* _FLAGS_H_ */
