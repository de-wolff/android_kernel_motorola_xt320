/*
 * Linux Broadcom BCM47xx GPIO char driver
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: linux_gpio.h,v 13.3 2007-02-01 22:51:59 csm Exp $
 */

#ifndef _linux_gpio_h_
#define _linux_gpio_h_

struct gpio_ioctl {
	uint32 mask;
	uint32 val;
};

#define GPIO_IOC_MAGIC  'G'

/* reserve/release a gpio to the caller */
#define  GPIO_IOC_RESERVE	_IOWR(GPIO_IOC_MAGIC, 1, struct gpio_ioctl)
#define  GPIO_IOC_RELEASE	_IOWR(GPIO_IOC_MAGIC, 2, struct gpio_ioctl)
/* ioctls to read/write the gpio registers */
#define  GPIO_IOC_OUT		_IOWR(GPIO_IOC_MAGIC, 3, struct gpio_ioctl)
#define  GPIO_IOC_IN		_IOWR(GPIO_IOC_MAGIC, 4, struct gpio_ioctl)
#define  GPIO_IOC_OUTEN		_IOWR(GPIO_IOC_MAGIC, 5, struct gpio_ioctl)

#endif	/* _linux_gpio_h_ */
