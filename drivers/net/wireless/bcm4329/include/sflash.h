/*
 * Broadcom SiliconBackplane chipcommon serial flash interface
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: sflash.h,v 13.10 2008-03-04 22:17:11 gmo Exp $
 */

#ifndef _sflash_h_
#define _sflash_h_

#include <typedefs.h>
#include <sbchipc.h>

struct sflash {
	uint blocksize;		/* Block size */
	uint numblocks;		/* Number of blocks */
	uint32 type;		/* Type */
	uint size;		/* Total size in bytes */
};

/* Utility functions */
extern int sflash_poll(si_t *sih, chipcregs_t *cc, uint offset);
extern int sflash_read(si_t *sih, chipcregs_t *cc,
                       uint offset, uint len, uchar *buf);
extern int sflash_write(si_t *sih, chipcregs_t *cc,
                        uint offset, uint len, const uchar *buf);
extern int sflash_erase(si_t *sih, chipcregs_t *cc, uint offset);
extern int sflash_commit(si_t *sih, chipcregs_t *cc,
                         uint offset, uint len, const uchar *buf);
extern struct sflash *sflash_init(si_t *sih, chipcregs_t *cc);

#endif /* _sflash_h_ */
