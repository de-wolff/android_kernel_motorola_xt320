/*
 *  stub for bcmsdh.h functions
 *
 * $Copyright (C) 2003 Broadcom Corporation$
 *
 * $Id: bcmsdh_stubs.c,v 1.18.402.1 2010-08-27 19:15:41 dlhtohdl Exp $
 */

#include <typedefs.h>
#include <osl.h>
#include <bcmsdh.h>
#include <bcmutils.h>

struct bcmsdh_info {
	uint	i;	/* dummy element stub */
};

bcmsdh_info_t *bcmsdh_attach(osl_t *osh, void *cfghdl, void **regsva, uint irq) { return NULL; }
int bcmsdh_detach(osl_t *osh, void *sdh) { return 0; }

bool bcmsdh_intr_query(void *sdh) { return 0; }
int bcmsdh_intr_enable(void *sdh) { return BCME_ERROR; }
int bcmsdh_intr_disable(void *sdh) { return 0; }
bool bcmsdh_intr_pending(void *sdh) { return FALSE; }
int bcmsdh_intr_reg(void *sdh, bcmsdh_cb_fn_t fn, void *argh) { return 0; }
int bcmsdh_intr_dereg(void *sdh) { return 0; }

int bcmsdh_devremove_reg(void *sdh, bcmsdh_cb_fn_t fn, void *argh) { return 0; }

uint8 bcmsdh_cfg_read(void *sdh, uint fuc, uint32 addr, int *err)
{ return (err ? (*err = 0) : 0); }
void bcmsdh_cfg_write(void *sdh, uint fuc, uint32 addr, uint8 data, int *err)
{ if (err) *err = 0; }
#if 0
uint8 bcmsdh_cfg_raw(void *sdh, uint fuc, uint32 addr, uint8 data, int *err)
{ return (err ? (*err = 0) : 0); }
#endif
int bcmsdh_cis_read(void *sdh, uint fuc, uint8 *cis, uint length) { return 0; }
uint32 bcmsdh_cfg_read_word(void *sdh, uint fnc_num, uint32 addr, int *err)
{ return 0; }
void bcmsdh_cfg_write_word(void *sdh, uint fnc_num, uint32 addr, uint32 data, int *err)
{ return; }
uint32 bcmsdh_reg_read(void *sdh, uint32 addr, uint size) { return 0; }
uint32 bcmsdh_reg_write(void *sdh, uint32 addr, uint size, uint32 data) { return 0; }

int bcmsdh_recv_buf(void *sdh, uint32 addr, uint fn, uint flags,
                    uint8 *buf, uint nbytes, void *pkt, bcmsdh_cmplt_fn_t complete, void *handle)
{return BCME_ERROR; }
int bcmsdh_send_buf(void *sdh, uint32 addr, uint fn, uint flags,
                    uint8 *buf, uint nbytes, void *pkt, bcmsdh_cmplt_fn_t complete, void *handle)
{return BCME_ERROR; }

int bcmsdh_iovar_set(void *sdh, int param, char *buf, int len) { return BCME_UNSUPPORTED; }
int bcmsdh_iovar_get(void *sdh, int *param, char *buf, int len) { return BCME_UNSUPPORTED; }

int bcmsdh_waitlockfree(void *sdh) {return 0; }