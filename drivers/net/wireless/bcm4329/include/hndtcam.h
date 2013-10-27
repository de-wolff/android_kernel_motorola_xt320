/*
 * HND SOCRAM TCAM software interface.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: hndtcam.h,v 1.3.104.3 2010-10-25 16:00:01 maheshd Exp $
 */
#ifndef _hndtcam_h_
#define _hndtcam_h_

/*
 * 0 - 1
 * 1 - 2 Consecutive locations are patched
 * 2 - 4 Consecutive locations are patched
 * 3 - 8 Consecutive locations are patched
 * 4 - 16 Consecutive locations are patched
 * Define default to patch 2 locations
 */

#ifdef  PATCHCOUNT
#define SRPC_PATCHCOUNT PATCHCOUNT
#else
#define PATCHCOUNT 0
#define SRPC_PATCHCOUNT PATCHCOUNT
#endif

#ifdef BCM_BOOTLOADER
/* Patch count is hardcoded to 0 for boot loaders. */
#undef SRPC_PATCHCOUNT
#define SRPC_PATCHCOUNT	0
#endif

/* N Consecutive location to patch */
#define SRPC_PATCHNLOC (1 << (SRPC_PATCHCOUNT))

#define PATCHHDR(_p)		__attribute__ ((__section__ (".patchhdr."#_p))) _p
#define PATCHENTRY(_p)		__attribute__ ((__section__ (".patchentry."#_p))) _p

typedef struct {
	uint32	data[SRPC_PATCHNLOC];
} patch_entry_t;

typedef struct {
	void		*addr;		/* patch address */
	uint32		len;		/* bytes to patch in entry */
	patch_entry_t	*entry;		/* patch entry data */
} patch_hdr_t;

/* patch values and address structure */
typedef struct patchaddrvalue {
	uint32	addr;
	uint32	value;
} patchaddrvalue_t;

extern void *socram_regs;
extern uint32 socram_rev;

extern void hnd_patch_init(void *srp);
extern void hnd_tcam_write(void *srp, uint16 index, uint32 data);
extern void hnd_tcam_read(void *srp, uint16 index, uint32 *content);
void * hnd_tcam_init(void *srp, int no_addrs);
extern void hnd_tcam_disablepatch(void *srp);
extern void hnd_tcam_enablepatch(void *srp);
#ifdef CONFIG_XIP
extern void hnd_tcam_bootloader_load(void *srp, char *pvars);
#else
extern void hnd_tcam_load(void *srp, const  patchaddrvalue_t *patchtbl);
#endif /* CONFIG_XIP */
extern void BCMATTACHFN(hnd_tcam_load_default)(void);
extern void hnd_tcam_reclaim(void);

#endif /* _hndtcam_h_ */
