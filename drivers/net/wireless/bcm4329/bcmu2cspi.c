/*
 * Broadcom SPI over Diolan U2C USB->SPI Host Controller, low-level hardware driver
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmu2cspi.c,v 1.2 2007-08-28 08:55:13 sridhara Exp $
 *
 * WARNING: do not check in until purged of GPL code
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#include <typedefs.h>
#include <bcmutils.h>
#include <linuxver.h>

#include <sdio.h>	/* SDIO Specs */
#include <bcmsdbus.h>
#include <sdiovar.h>	/* to get msglevel bit values */

#include <bcmsdspi.h>
#include <bcmspi.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define KERNEL26
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define KERNEL26
#define USB_ALLOC_URB()		usb_alloc_urb(0, GFP_ATOMIC)
#define USB_SUBMIT_URB(urb)	usb_submit_urb(urb, GFP_ATOMIC)
#define USB_UNLINK_URB(urb)	usb_kill_urb(urb)
#define USB_BUFFER_ALLOC(dev, size, mem, dma) \
				usb_buffer_alloc(dev, size, mem, dma)
#define USB_BUFFER_FREE(dev, size, data, dma) \
				usb_buffer_free(dev, size, data, dma)
#define USB_QUEUE_BULK		0
#define CALLBACK_ARGS		struct urb *urb, struct pt_regs *regs
#define CONFIGDESC(usb)		(&((usb)->actconfig)->desc)
#define IFPTR(usb, idx)		((usb)->actconfig->interface[idx])
#define IFALTS(usb, idx)	(IFPTR((usb), (idx))->altsetting[0])
#define IFDESC(usb, idx)	IFALTS((usb), (idx)).desc
#define IFEPDESC(usb, idx, ep)	(IFALTS((usb), (idx)).endpoint[ep]).desc
#define INTERVAL_DECODE(usb, interval) \
				(interval)	/* usb_fill_int_urb decodes interval in 2.6 */
#else /* 2.4 */
#define USB_ALLOC_URB()		usb_alloc_urb(0)
#define USB_SUBMIT_URB(urb)	usb_submit_urb(urb)
#define USB_UNLINK_URB(urb)	usb_unlink_urb(urb)
#define USB_BUFFER_ALLOC(dev, size, mem, dma) \
				kmalloc(size, mem)
#define USB_BUFFER_FREE(dev, size, data, dma) \
				kfree(data)
#define CALLBACK_ARGS		struct urb *urb
#define CONFIGDESC(usb)		((usb)->actconfig)
#define IFPTR(usb, idx)		(&(usb)->actconfig->interface[idx])
#define IFALTS(usb, idx)	((usb)->actconfig->interface[idx].altsetting[0])
#define IFDESC(usb, idx)	IFALTS((usb), (idx))
#define IFEPDESC(usb, idx, ep)	(IFALTS((usb), (idx)).endpoint[ep])
#define INTERVAL_DECODE(usb, interval) \
			(((usb)->speed == USB_SPEED_HIGH) ? (1 << ((interval) - 1)) : (interval))
#endif /* 2.4 */

/*
 * U2C interface
 */

typedef enum
{
	U2C_SUCCESS = 0,                        /* API Function was successful */
	U2C_BAD_PARAMETER = 1,                  /* API Function got bad input parameter */
	U2C_HARDWARE_NOT_FOUND = 2,             /* U2C Device not found */
	U2C_TRANSACTION_FAILED = 4,             /* SPI transaction failed */
	U2C_SLAVE_OPEN_FOR_WRITE_FAILED = 5,    /* I2C Slave did not ack write slave address */
	U2C_SLAVE_OPEN_FOR_READ_FAILED = 6,     /* I2C Slave did not ack read slave address */
	U2C_SENDING_MEMORY_ADDRESS_FAILED = 7,  /* I2C Slave did not ack internal addr output */
	U2C_SENDING_DATA_FAILED = 8,            /* I2C Slave did not ack data output */
	U2C_NOT_IMPLEMENTED = 9,                /* Operation is not implemented by U2C API */
	U2C_NO_ACK = 10,                        /* Got no ACK from Slave */
	U2C_DEVICE_BUSY = 11,                   /* U2C Device Busy */
	U2C_MEMORY_ERROR = 12,                  /* Memory operation (like malloc) failed */
	U2C_UNKNOWN_ERROR = 13,                 /* Undocumented error */
} U2C_RESULT;

#define U2C_SPI_MAXLEN		256		/* Max length of read and/or write */

/* SPI bus frequency values */
#define U2C_SPI_FREQ_200KHZ  	0
#define U2C_SPI_FREQ_100KHZ  	1
#define U2C_SPI_FREQ_83KHZ  	2
#define U2C_SPI_FREQ_71KHZ  	3
#define U2C_SPI_FREQ_62KHZ  	4
#define U2C_SPI_FREQ_50KHZ  	6
#define U2C_SPI_FREQ_25KHZ  	16
#define U2C_SPI_FREQ_10KHZ  	46
#define U2C_SPI_FREQ_5KHZ   	96
#define U2C_SPI_FREQ_2KHZ   	242

/* SPI bus configuration values */
#define U2C_SPI_CONFIG_CPOL_0	0
#define U2C_SPI_CONFIG_CPOL_1	0x1
#define U2C_SPI_CONFIG_CPHA_0	0
#define U2C_SPI_CONFIG_CPHA_1	0x2
#define U2C_SPI_CONFIG_ENABLE	0
#define U2C_SPI_CONFIG_DISABLE	0x4

typedef struct {
	uint16 MajorVersion;
	uint16 MinorVersion;
} U2C_VERSION_INFO;

#define DIMAX_VENDOR    	0x0abf
#define DIMAX_PRODUCT   	0x3370
#define DIMAX_IF        	0x00
#define DIMAX_IN_EP     	0x84
#define DIMAX_OUT_EP    	0x02
#define DIMAX_TIMEOUT   	100	/* ms */

#define MAX_INPUT_BUFLEN     	257	/* Max length of the data we are reading from device */
#define MAX_OUTPUT_BUFLEN    	264	/* Max length of the data we are sending to device */

typedef struct {
	uint8 InputBuffer[MAX_INPUT_BUFLEN]; 	/* Buffer received from device */
	uint32 InputBufferLength;		/* Length of the buffer received from device */
	uint8 OutputBuffer[MAX_OUTPUT_BUFLEN]; 	/* Buffer sent to device */
	uint32 OutputBufferLength;   		/* Length of the buffer sent to device */
} DEVICE_REQUEST;

typedef struct bcmu2c_info_s {
	sdioh_info_t *sd;
	spinlock_t lock;
	struct usb_device *usb;
	struct urb *urb;
	uint pipe_out, pipe_in;
} bcmu2c_info_t;

static sdioh_info_t *sd_current;

/* Status routines */
U2C_RESULT U2C_GetSerialNum(bcmu2c_info_t *bi, uint32 *pSerialNum);
U2C_RESULT U2C_GetFirmwareVersion(bcmu2c_info_t *bi, U2C_VERSION_INFO *pVersion);

/* GPIO routines */
U2C_RESULT U2C_SetIoDirection(bcmu2c_info_t *bi, uint32 Value, uint32 Mask);
U2C_RESULT U2C_GetIoDirection(bcmu2c_info_t *bi, uint32 *pValue);
U2C_RESULT U2C_IoWrite(bcmu2c_info_t *bi, uint32 Value, uint32 Mask);
U2C_RESULT U2C_IoRead(bcmu2c_info_t *bi, uint32 *pValue);

/* SPI routines */
U2C_RESULT U2C_SpiSetConfig(bcmu2c_info_t *bi, uint32 Config);
U2C_RESULT U2C_SpiGetConfig(bcmu2c_info_t *bi, uint32 *pConfig);
U2C_RESULT U2C_SpiSetFreq(bcmu2c_info_t *bi, uint8 Frequency);
U2C_RESULT U2C_SpiGetFreq(bcmu2c_info_t *bi, uint8 *pFrequency);
U2C_RESULT U2C_SpiReadWrite(bcmu2c_info_t *bi,
                            uint8 *pOutBuffer, uint8 *pInBuffer, uint16 Length);
U2C_RESULT U2C_SpiWrite(bcmu2c_info_t *bi, uint8 *pOutBuffer, uint16 Length);
U2C_RESULT U2C_SpiRead(bcmu2c_info_t *bi, uint8 *pInBuffer, uint16 Length);

static U2C_RESULT bcmu2c_transaction(bcmu2c_info_t *bi, DEVICE_REQUEST *req);
static U2C_RESULT bcmu2c_fw_rc(int rc);

/* Disable device interrupt */
void
sdspi_devintr_off(sdioh_info_t *sd)
{
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_client_ints));
}

/* Enable device interrupt */
void
sdspi_devintr_on(sdioh_info_t *sd)
{
	ASSERT(sd->lockcount == 0);
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_client_ints));
}

bool
sdspi_start_clock(sdioh_info_t *sd, uint16 new_sd_divisor)
{
	sd_err(("Clock rate not settable from driver.\n"));
	return TRUE;
}

bool
sdspi_controller_highspeed_mode(sdioh_info_t *sd, bool hsmode)
{
	sd_err(("USB-SPI Does not support high-speed mode.\n"));
	return FALSE;
}

bool
check_client_intr(sdioh_info_t *sd, int *is_dev_intr)
{
	ASSERT(sd);
	return TRUE;
}

static void
bcmu2c_detach(bcmu2c_info_t *bi)
{
	if (bi->urb != NULL)
		usb_free_urb(bi->urb);
}

#if 0
static void
hexdump(char *pfx, unsigned char *msg, int msglen)
{
	int i, col;
	char buf[80];

	ASSERT(strlen(pfx) + 49 <= sizeof(buf));

	col = 0;

	for (i = 0; i < msglen; i++, col++) {
		if (col % 16 == 0)
			strcpy(buf, pfx);
		sprintf(buf + strlen(buf), "%02x", msg[i]);
		if ((col + 1) % 16 == 0)
			printf("%s\n", buf);
		else
			sprintf(buf + strlen(buf), " ");
	}

	if (col % 16 != 0)
		printf("%s\n", buf);
}
#endif /* 0 */

#ifdef KERNEL26
static int bcmu2c_probe(struct usb_interface *intf,
                        const struct usb_device_id *id);
static void bcmu2c_disconnect(struct usb_interface *intf);
#else
static void *bcmu2c_probe(struct usb_device *usb, unsigned int ifnum,
                          const struct usb_device_id *id);
static void bcmu2c_disconnect(struct usb_device *usb, void *ptr);
#endif

static struct usb_device_id bcmu2c_id_table[];
static struct usb_driver bcmu2c_driver;

#ifdef KERNEL26
static int
bcmu2c_probe(struct usb_interface *intf,
             const struct usb_device_id *id)
#else
static void *
bcmu2c_probe(struct usb_device *usb, unsigned int ifnum,
             const struct usb_device_id *id)
#endif
{
	struct usb_endpoint_descriptor *endpoint;
	bcmu2c_info_t *bi = NULL;
	int ep;
	int ret;
	uint32 serial_no;
	U2C_VERSION_INFO fw_version;
#ifdef KERNEL26
	struct usb_device *usb = interface_to_usbdev(intf);
#else
	int if_claimed = 0;
#endif

	sd_trace(("%s: enter\n", __FUNCTION__));

	if (sd_current == NULL) {
		printk(KERN_ERR "bcmu2c_probe: can't determine sdh instance\n");
		ret = -ENXIO;
		goto fail;
	}

	if (!usb) {
		printk(KERN_ERR "bcmu2c_probe: failed to get usb device\n");
		ret = -ENXIO;
		goto fail;
	}

	/* Allocate private state */
	if (!(bi = kmalloc(sizeof(bcmu2c_info_t), GFP_KERNEL))) {
		printk(KERN_ERR "bcmu2c_probe: out of memory for bcmu2c_info_t\n");
		ret = -ENOMEM;
		goto fail;
	}

	memset(bi, 0, sizeof(bcmu2c_info_t));

	bi->sd = sd_current;
	bi->usb = usb;

	if ((bi->urb = USB_ALLOC_URB()) == NULL) {
		printk(KERN_ERR "bcmu2c_probe: out of memory for URB\n");
		ret = -ENOMEM;
		goto fail;
	}

	sd_current->controller = bi;

#ifdef KERNEL26
	usb_set_intfdata(intf, bi);
#endif

	/* Default error code while checking device */
	ret = -ENXIO;

	/* Check that the device supports only one configuration */
	if (usb->descriptor.bNumConfigurations != 1) {
		printk(KERN_ERR "bcmu2c_probe: invalid number of configurations %d\n",
		       usb->descriptor.bNumConfigurations);
		goto fail;
	}

	/* Check device class */
	if (usb->descriptor.bDeviceClass != USB_CLASS_PER_INTERFACE) {
		printk(KERN_ERR "bcmu2c_probe: unsupported device class %d\n",
		       usb->descriptor.bDeviceClass);
		goto fail;
	}

	/* Check that the configuration supports one interface */
	if (CONFIGDESC(usb)->bNumInterfaces != 1) {
		printk(KERN_ERR "bcmu2c_probe: invalid number of interfaces %d\n",
		       CONFIGDESC(usb)->bNumInterfaces);
		goto fail;
	}

	/* Check interface */

#ifndef KERNEL26
	if (usb_interface_claimed(IFPTR(usb, 0))) {
		printk(KERN_ERR "bcmu2c_probe: interface already claimed\n");
		goto fail;
	}
#endif

	if (IFDESC(usb, 0).bInterfaceClass != USB_CLASS_VENDOR_SPEC ||
	    IFDESC(usb, 0).bInterfaceSubClass != 0 ||
	    IFDESC(usb, 0).bInterfaceProtocol != 0) {
		printk(KERN_ERR
		       "bcmu2c_probe: invalid interface 0: "
		       "class %d; subclass %d; proto %d\n",
		       IFDESC(usb, 0).bInterfaceClass,
		       IFDESC(usb, 0).bInterfaceSubClass,
		       IFDESC(usb, 0).bInterfaceProtocol);
		goto fail;
	}

#ifndef KERNEL26
	/* Claim interface */
	usb_driver_claim_interface(&bcmu2c_driver, IFPTR(usb, 0), bi);
	if_claimed = 1;
#endif

	/* Check data endpoints and get pipes */

	if (IFDESC(usb, 0).bNumEndpoints != 2) {
		printk(KERN_ERR "bcmu2c_probe: invalid number of endpoints %d",
		       IFDESC(usb, 0).bNumEndpoints);
		goto fail;
	}

	for (ep = 0; ep < 2; ep++) {
		endpoint = &IFEPDESC(usb, 0, ep);
		if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) !=
		    USB_ENDPOINT_XFER_BULK) {
			printk(KERN_ERR "bcmu2c_probe: invalid data endpoint %d", ep);
			goto fail;
		}

		if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
			bi->pipe_in = usb_rcvbulkpipe(usb,
			                              endpoint->bEndpointAddress &
			                              USB_ENDPOINT_NUMBER_MASK);
		else
			bi->pipe_out = usb_sndbulkpipe(usb,
			                               endpoint->bEndpointAddress &
			                               USB_ENDPOINT_NUMBER_MASK);
	}

	sd_trace(("%s: success\n", __FUNCTION__));

	/* Read U2C board serial number and firmware version */

	if ((ret = U2C_GetSerialNum(bi, &serial_no)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_GetSerialNum failed: %d\n", ret);
		goto fail;
	}

	if ((ret = U2C_GetFirmwareVersion(bi, &fw_version)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_GetFirmwareVersion failed: %d\n", ret);
		goto fail;
	}

	printk("bcmu2c_probe: U2C serial number 0x%x, firmware version %d.%d\n",
	       serial_no,
	       fw_version.MajorVersion, fw_version.MinorVersion);

	/* Configure SPI clock */

	if ((ret = U2C_SpiSetConfig(bi, (U2C_SPI_CONFIG_CPOL_0 |
	                                 U2C_SPI_CONFIG_CPHA_0 |
	                                 U2C_SPI_CONFIG_ENABLE))) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_SpiSetConfig failed: %d\n", ret);
		goto fail;
	}

	if ((ret = U2C_SpiSetFreq(bi, U2C_SPI_FREQ_200KHZ)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_SpiSetFreq failed: %d\n", ret);
		goto fail;
	}

	/* Assert chip select; set PA5 to be an output and set it to zero */

	if ((ret = U2C_SetIoDirection(bi, 0x20, 0x20)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_SetIoDirection failed: %d\n", ret);
		goto fail;
	}

	if ((ret = U2C_IoWrite(bi, 0, 0x20)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_IoWrite failed: %d\n", ret);
		goto fail;
	}

#if 0
	printk("Performing CMD 0:\n");

	uint8 buf_out[32], buf_in[32];
	buf_out[0] = 0x40;
	buf_out[1] = 0;
	buf_out[2] = 0;
	buf_out[3] = 0;
	buf_out[4] = 0;
	buf_out[5] = 0x95;
	buf_out[6] = 0xff;	/* Room for result */
	buf_out[7] = 0xff;
	buf_out[8] = 0xff;
	buf_out[9] = 0xff;

	hexdump("OUT: ", buf_out, 10);

	if ((ret = U2C_SpiReadWrite(bi, buf_out, buf_in, 10)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_SpiReadWrite failed: %d\n", ret);
		goto fail;
	}

	hexdump("IN : ", buf_in, 10);

	printk("Performing CMD 5:\n");

	buf_out[0] = 0x45;
	buf_out[1] = 0;
	buf_out[2] = 0;
	buf_out[3] = 0;
	buf_out[4] = 0;
	buf_out[5] = 0x95;	/* CRC is actually don't-care for CMD 5 */
	buf_out[6] = 0xff;	/* Room for result */
	buf_out[7] = 0xff;
	buf_out[8] = 0xff;
	buf_out[9] = 0xff;

	hexdump("OUT: ", buf_out, 10);

	if ((ret = U2C_SpiReadWrite(bi, buf_out, buf_in, 10)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_SpiReadWrite failed: %d\n", ret);
		goto fail;
	}

	hexdump("IN : ", buf_in, 10);

	uint32 gpio_val;
	if ((ret = U2C_IoRead(bi, &gpio_val)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_IoRead failed: %d\n", ret);
		goto fail;
	}

	printk("GPIOs = 0x%x\n", gpio_val);

	/* Deassert chip select */ // xxx move to detach

	if ((ret = U2C_IoWrite(bi, 0x20, 0x20)) != U2C_SUCCESS) {
		printk(KERN_ERR "bcmu2c_probe: U2C_IoWrite failed: %d\n", ret);
		goto fail;
	}
#endif /* 0 */

	/* Success */
#ifdef KERNEL26
	return 0;
#else
	usb_inc_dev_use(usb);
	return bi;
#endif

fail:
	printk(KERN_ERR "bcmu2c_probe: failed (errno=%d)\n", ret);

#ifndef KERNEL26
	if (if_claimed)
		usb_driver_release_interface(&bcmu2c_driver, IFPTR(usb, 0));
#endif

	bcmu2c_detach(bi);
	kfree(bi);

	sd_trace(("%s: fail\n", __FUNCTION__));

#ifdef KERNEL26
	usb_set_intfdata(intf, NULL);
	return ret;
#else
	return NULL;
#endif
}

#ifdef KERNEL26
static void
bcmu2c_disconnect(struct usb_interface *intf)
#else
static void
bcmu2c_disconnect(struct usb_device *usb, void *ptr)
#endif
{
#ifdef KERNEL26
	bcmu2c_info_t *bi = usb_get_intfdata(intf);
	struct usb_device *usb = interface_to_usbdev(intf);
#else
	bcmu2c_info_t *bi = (bcmu2c_info_t *)ptr;
#endif

	sd_trace(("%s: enter\n", __FUNCTION__));

	if (bi == NULL || usb == NULL) {
		err("bcmu2c_disconnect: null structure (bi=%p usb=%p)", bi, usb);
		return;
	}

	bcmu2c_detach(bi);
	kfree(bi);

#ifndef KERNEL26
	usb_driver_release_interface(&bcmu2c_driver, IFPTR(usb, 0));
	usb_dec_dev_use(usb);
#endif

	sd_trace(("%s: exit\n", __FUNCTION__));
}

static struct usb_device_id bcmu2c_id_table[] = {
	{ USB_DEVICE(DIMAX_VENDOR, DIMAX_PRODUCT) },
	{ }
};

static struct usb_driver bcmu2c_driver = {
	name:		"bcmu2c",
	probe:		bcmu2c_probe,
	disconnect:	bcmu2c_disconnect,
	id_table:	bcmu2c_id_table
};

/*
 * This is kind of backward.  sdspi_hw_attach registers a USB driver, which causes
 * bcmu2c_probe to be called right away.  But bcmu2c_probe needs a way to associate the
 * found USB device with the sdioh instance.  Currently this is hacked using a global
 * 'sd_current'.
 */

bool
sdspi_hw_attach(sdioh_info_t *sd)
{
	/* sd_msglevel |= SDH_TRACE_VAL; */

	sd_trace(("%s: enter\n", __FUNCTION__));

	if (sd_current != NULL) {
		printk(KERN_ERR "sdspi_hw_attach: device busy\n");
		return FALSE;
	}

	sd_current = sd;

	usb_register(&bcmu2c_driver);

	sd_trace(("%s: exit\n", __FUNCTION__));
	return TRUE;
}

bool
sdspi_hw_detach(sdioh_info_t *sd)
{
	sd_trace(("%s: enter\n", __FUNCTION__));

	usb_deregister(&bcmu2c_driver);

	sd_current = NULL;

	sd_trace(("%s: exit\n", __FUNCTION__));
	return TRUE;
}

void
sdspi_sendrecv(sdioh_info_t *sd, uint8 *msg_out, uint8 *msg_in, int msglen)
{
	bcmu2c_info_t *bi = sd->controller;
	int offset, n, ret;

	sd_trace(("%s: enter\n", __FUNCTION__));

	sd_trace(("sd=%p bi=%p msg_out=%p msg_in=%p msglen=%d\n",
	          sd, bi, msg_out, msg_in, msglen));

	/* Break up large transfers to stay under U2C_SPI_MAXLEN restriction */
	for (offset = 0; offset < msglen; offset += n) {
		if ((n = msglen - offset) > U2C_SPI_MAXLEN)
			n = U2C_SPI_MAXLEN;

		if ((ret = U2C_SpiReadWrite(bi,
		                            msg_out + offset,
		                            msg_in + offset,
		                            (uint16)n)) != U2C_SUCCESS) {
			printk(KERN_ERR "sdspi_sendrecv: U2C_SpiReadWrite failed: %d\n", ret);
			break;
		}
	}

	sd_trace(("%s: exit\n", __FUNCTION__));
}

/*
 * Convert FW status codes to U2C_RESULT
 */
static U2C_RESULT
bcmu2c_fw_rc(int rc)
{
	switch (rc) {
	case 0:
		return U2C_SUCCESS;
	case 1:
		return U2C_TRANSACTION_FAILED;
	case 2:
		return U2C_SLAVE_OPEN_FOR_WRITE_FAILED;
	case 3:
		return U2C_SLAVE_OPEN_FOR_READ_FAILED;
	case 4:
		return U2C_SENDING_MEMORY_ADDRESS_FAILED;
	case 5:
		return U2C_SENDING_DATA_FAILED;
	case 6:
		return U2C_NOT_IMPLEMENTED;
	case 7:
		return U2C_NO_ACK;
	default:
		return U2C_UNKNOWN_ERROR;
	}
}

static U2C_RESULT
bcmu2c_transaction(bcmu2c_info_t *bi, DEVICE_REQUEST *req)
{
	int rc, actual_len;

	rc = usb_bulk_msg(bi->usb, bi->pipe_out,
	                  req->OutputBuffer, req->OutputBufferLength, &actual_len,
	                  DIMAX_TIMEOUT * HZ / 1000);

	if (rc != 0)
		return U2C_TRANSACTION_FAILED;

	if (actual_len != req->OutputBufferLength)
		return U2C_TRANSACTION_FAILED;

	rc = usb_bulk_msg(bi->usb, bi->pipe_in,
	                  req->InputBuffer, MAX_INPUT_BUFLEN, &actual_len,
	                  DIMAX_TIMEOUT * HZ / 1000);

	if (rc != 0)
		return U2C_TRANSACTION_FAILED;

	if (req->InputBufferLength == 0)
		req->InputBufferLength = actual_len;
	else if (actual_len != req->InputBufferLength)
		return U2C_TRANSACTION_FAILED;

	/* Status is returned by fw in last byte */
	return bcmu2c_fw_rc(req->InputBuffer[req->InputBufferLength - 1]);
}

/*
 * Status routines
 */

U2C_RESULT
U2C_GetSerialNum(bcmu2c_info_t *bi, uint32 *pSerialNum)
{
	U2C_RESULT rc;
	DEVICE_REQUEST req;

	req.OutputBufferLength = 1;
	req.OutputBuffer[0] = 0x0B;
	req.InputBufferLength = 5;

	rc = bcmu2c_transaction(bi, &req);
	memcpy(pSerialNum, req.InputBuffer, 4);
	return rc;
}

U2C_RESULT
U2C_GetFirmwareVersion(bcmu2c_info_t *bi, U2C_VERSION_INFO *pVersion)
{
	U2C_RESULT rc;
	DEVICE_REQUEST req;

	req.OutputBufferLength = 1;
	req.OutputBuffer[0] = 0x0A;
	req.InputBufferLength = 3;

	rc = bcmu2c_transaction(bi, &req);
	pVersion->MajorVersion = req.InputBuffer[0];
	pVersion->MinorVersion = req.InputBuffer[1];
	return rc;
}

/*
 * GPIO routines
 */

U2C_RESULT
U2C_SetIoDirection(bcmu2c_info_t *bi, uint32 Value, uint32 Mask)
{
	DEVICE_REQUEST req;
	Value = Value & Mask;
	Mask = ~Mask;
	req.OutputBufferLength = 7;
	req.OutputBuffer[0] = 0x17;
	req.OutputBuffer[1] = (uint8)Mask;
	req.OutputBuffer[2] = (uint8)Value;
	req.OutputBuffer[3] = (uint8)(Mask >> 8);
	req.OutputBuffer[4] = (uint8)(Value >> 8);
	req.OutputBuffer[5] = (uint8)(Mask >> 16);
	req.OutputBuffer[6] = (uint8)(Value >> 16);
	req.InputBufferLength = 1;
	return bcmu2c_transaction(bi, &req);
}

U2C_RESULT
U2C_GetIoDirection(bcmu2c_info_t *bi, uint32 *pValue)
{
	U2C_RESULT rc;
	DEVICE_REQUEST req;
	req.OutputBufferLength = 1;
	req.OutputBuffer[0] = 0x18;
	req.InputBufferLength = 4;
	rc = bcmu2c_transaction(bi, &req);
	*pValue = req.InputBuffer[0];
	(*pValue) |= (req.InputBuffer[1] << 8);
	(*pValue) |= (req.InputBuffer[2] << 16);
	return rc;
}

U2C_RESULT
U2C_IoWrite(bcmu2c_info_t *bi, uint32 Value, uint32 Mask)
{
	DEVICE_REQUEST req;
	Value = Value & Mask;
	Mask = ~Mask;
	req.OutputBufferLength = 7;
	req.OutputBuffer[0] = 0x19;
	req.OutputBuffer[1] = (uint8)Mask;
	req.OutputBuffer[2] = (uint8)Value;
	req.OutputBuffer[3] = (uint8)(Mask >> 8);
	req.OutputBuffer[4] = (uint8)(Value >> 8);
	req.OutputBuffer[5] = (uint8)(Mask >> 16);
	req.OutputBuffer[6] = (uint8)(Value >> 16);
	req.InputBufferLength = 1;
	return bcmu2c_transaction(bi, &req);
}

U2C_RESULT
U2C_IoRead(bcmu2c_info_t *bi, uint32 *pValue)
{
	U2C_RESULT rc;
	DEVICE_REQUEST req;
	req.OutputBufferLength = 1;
	req.OutputBuffer[0] = 0x1A;
	req.InputBufferLength = 4;
	rc = bcmu2c_transaction(bi, &req);
	*pValue = req.InputBuffer[0];
	(*pValue) |= (req.InputBuffer[1] << 8);
	(*pValue) |= (req.InputBuffer[2] << 16);
	return rc;
}

/*
 * SPI configuration routines
 */

U2C_RESULT
U2C_SpiSetConfig(bcmu2c_info_t *bi, uint32 Config)
{
	DEVICE_REQUEST req;
	req.OutputBufferLength = 2;
	req.OutputBuffer[0] = 0x1D;
	req.OutputBuffer[1] = (uint8)Config;
	req.InputBufferLength = 1;
	return bcmu2c_transaction(bi, &req);
}

U2C_RESULT
U2C_SpiGetConfig(bcmu2c_info_t *bi, uint32 *pConfig)
{
	DEVICE_REQUEST req;
	U2C_RESULT rc;
	req.OutputBufferLength = 1;
	req.OutputBuffer[0] = 0x1E;
	req.InputBufferLength = 2;
	rc = bcmu2c_transaction(bi, &req);
	*pConfig = req.InputBuffer[0];
	return rc;
}

U2C_RESULT
U2C_SpiSetFreq(bcmu2c_info_t *bi, uint8 Frequency)
{
	DEVICE_REQUEST req;
	req.OutputBufferLength = 2;
	req.OutputBuffer[0] = 0x1F;
	req.OutputBuffer[1] = Frequency;
	req.InputBufferLength = 1;
	return bcmu2c_transaction(bi, &req);
}

U2C_RESULT
U2C_SpiGetFreq(bcmu2c_info_t *bi, uint8 *pFrequency)
{
	DEVICE_REQUEST req;
	U2C_RESULT rc;
	req.OutputBufferLength = 1;
	req.OutputBuffer[0] = 0x20;
	req.InputBufferLength = 2;
	rc = bcmu2c_transaction(bi, &req);
	*pFrequency = req.InputBuffer[0];
	return rc;
}

/*
 * SPI data transfer routines
 */

U2C_RESULT
U2C_SpiReadWrite(bcmu2c_info_t *bi, uint8 *pOutBuffer, uint8 *pInBuffer, uint16 Length)
{
	DEVICE_REQUEST req;
	U2C_RESULT rc;
	if ((Length == 0) || (Length > U2C_SPI_MAXLEN))
		return U2C_BAD_PARAMETER;
	req.OutputBufferLength = Length + 2;
	req.OutputBuffer[0] = 0x21;
	req.OutputBuffer[1] = (uint8)Length;
	memcpy(req.OutputBuffer + 2, pOutBuffer, Length);
	req.InputBufferLength = Length + 1;
	rc = bcmu2c_transaction(bi, &req);
	memcpy(pInBuffer, req.InputBuffer, Length);
	return rc;
}

U2C_RESULT
U2C_SpiWrite(bcmu2c_info_t *bi, uint8 *pOutBuffer, uint16 Length)
{
	DEVICE_REQUEST req;
	if ((Length == 0) || (Length > U2C_SPI_MAXLEN))
		return U2C_BAD_PARAMETER;
	req.OutputBufferLength = Length + 2;
	req.OutputBuffer[0] = 0x23;
	req.OutputBuffer[1] = (uint8)Length;
	memcpy(req.OutputBuffer + 2, pOutBuffer, Length);
	req.InputBufferLength = 1;
	return bcmu2c_transaction(bi, &req);
}

U2C_RESULT
U2C_SpiRead(bcmu2c_info_t *bi, uint8 *pInBuffer, uint16 Length)
{
	DEVICE_REQUEST req;
	U2C_RESULT rc;
	if ((Length == 0) || (Length > U2C_SPI_MAXLEN))
		return U2C_BAD_PARAMETER;
	req.OutputBufferLength = 2;
	req.OutputBuffer[0] = 0x22;
	req.OutputBuffer[1] = (uint8)Length;
	req.InputBufferLength = Length + 1;
	rc = bcmu2c_transaction(bi, &req);
	memcpy(pInBuffer, req.InputBuffer, Length);
	return rc;
}
