/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MT9V113_H
#define MT9V113_H

#include <mach/camera.h>
#include <linux/types.h>

enum mt9v113_width {
        WORD_LEN,
        BYTE_LEN
};

struct mt9v113_i2c_reg_conf {
        unsigned short waddr;
        unsigned short wdata;
        enum mt9v113_width width;
        unsigned short mdelay_time;
};

enum mt9v113_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	INVALID_SIZE
};

enum mt9v113_setting {
	RES_PREVIEW,
	RES_CAPTURE
};

enum mt9v113_reg_update {
	/* Sensor registers that need to be updated during initialization */
	REG_INIT,
	/* Sensor registers that needs periodic I2C writes */
	UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	UPDATE_ALL,
	/* Not valid update */
	UPDATE_INVALID
};

struct mt9v113_reg {
	const struct mt9v113_i2c_reg_conf const *config_first;
	uint16_t config_first_size;
	const struct mt9v113_i2c_reg_conf const *config_second;
	uint16_t config_second_size;
	const struct mt9v113_i2c_reg_conf const *config_third;
	uint16_t config_third_size;
};

#endif /* #define MT9V113_H */
