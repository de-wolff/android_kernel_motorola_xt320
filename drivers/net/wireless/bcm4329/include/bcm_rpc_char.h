/*
 * A simple linux style char driver for device driver testing
 *
 * $Copyright (C) 2008 Broadcom Corporation$
 *
 * $Id: bcm_rpc_char.h,v 13.1 2008-02-26 23:57:02 mzhu Exp $
 */

#ifndef _bcm_rpc_char_h_
#define _bcm_rpc_char_h_

typedef struct chardev_bus chardev_bus_t;
typedef void (*chardev_rx_fn_t)(void* context, char* data, uint len);

extern chardev_bus_t* chardev_attach(osl_t *osh);
extern void chardev_detach(chardev_bus_t* cbus);
extern void chardev_register_callback(chardev_bus_t* cbus, chardev_rx_fn_t rx_data, void *rx_ctx);
extern int chardev_send(chardev_bus_t* cbus, void* data, uint len);

#endif /* _bcm_rpc_char_h_ */
