/*
 * utils.h: prototypes & defs for run-time utils
 *
 * Copyright 1998, Epigram, Inc.
 *
 * $Id: utils.h,v 8.3 1998-11-20 20:50:52 stafford Exp $
 */

#ifndef _RTS_UTILS_H_
#define _RTS_UTILS_H_

extern int hex2bytes(char *, unsigned int, unsigned char *);
extern void bytes2hex(unsigned char *, unsigned int, char *);
extern int hexid2bytes(char *, unsigned int, unsigned char *);
extern void bytes2hexid(unsigned char *, unsigned int, char *);

unsigned int processor_count(void);

#endif /* _RTS_UTILS_H_ */
