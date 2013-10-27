/*
 * Header file for save-restore functionality in driver
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: Exp $
 */

#ifndef _SAVERESTORE_H
#define _SAVERESTORE_H

#define SRCTL43239_BANK_SIZE(sr_cntrl) ((sr_cntrl & 0x7F0) >> 4)
#define SRCTL43239_BANK_NUM(sr_cntrl) (sr_cntrl & 0xF)

extern CONST uint32 sr_source_code[];
extern CONST uint sr_source_codesz;

/* Function prototypes */
void sr_download_firmware(si_t *si_h);
void sr_engine_enable(si_t *si_h, bool enable);
uint32 sr_chipcontrol(si_t *si_h, uint32 mask, uint32 val);
void sr_save_restore_init(si_t *si_h);

#endif /* _SAVERESTORE_H */
