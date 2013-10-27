/*
 * Misc useful routines to access NIC local SROM/OTP .
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: bcmsrom.h,v 13.37.104.1 2010-12-03 21:30:28 kiranm Exp $
 */

#ifndef	_bcmsrom_h_
#define	_bcmsrom_h_

#include <bcmsrom_fmt.h>

/* Prototypes */
extern int srom_var_init(si_t *sih, uint bus, void *curmap, osl_t *osh,
                         char **vars, uint *count);
extern void srom_var_deinit(si_t *sih);

extern int srom_read(si_t *sih, uint bus, void *curmap, osl_t *osh,
                     uint byteoff, uint nbytes, uint16 *buf,
                     bool check_crc);

extern int srom_write(si_t *sih, uint bus, void *curmap, osl_t *osh,
                      uint byteoff, uint nbytes, uint16 *buf);

extern int srom_otp_cisrwvar(si_t *sih, osl_t *osh, char *vars, int *count);
#if defined(WLTEST) || defined(BCMDBG) || defined(BCMINTERNAL)
extern int srom_otp_write_region_crc(si_t *sih, uint nbytes, uint16* buf16, bool write);
#endif /* WLTEST || BCMDBG || BCMINTERNAL */

/* parse standard PCMCIA cis, normally used by SB/PCMCIA/SDIO/SPI/OTP
 *   and extract from it into name=value pairs
 */
extern int srom_parsecis(osl_t *osh, uint8 **pcis, uint ciscnt,
                         char **vars, uint *count);

#if defined(BCMUSBDEV) || defined(BCMSDIODEV)
/* Return sprom size in 16-bit words */
extern uint srom_size(si_t *sih, osl_t *osh);
#endif

#endif	/* _bcmsrom_h_ */
