/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/camera.h>
#include "ov7692.h"
#include <mach/gpio.h>
/*=============================================================
	SENSOR REGISTER DEFINES
==============================================================*/
#define Q8    0x00000100

/* Omnivision8810 product ID register address */
#define REG_OV7692_MODEL_ID_MSB                       0x0A
#define REG_OV7692_MODEL_ID_LSB                       0x0B

#define OV7692_MODEL_ID                       0x7692
/* Omnivision8810 product ID */

/* Time in milisecs for waiting for the sensor to reset */
#define OV7692_RESET_DELAY_MSECS    66
#define OV7692_DEFAULT_CLOCK_RATE   24000000
/* Registers*/

/* Color bar pattern selection */
#define OV7692_COLOR_BAR_PATTERN_SEL_REG     0x82
/* Color bar enabling control */
#define OV7692_COLOR_BAR_ENABLE_REG           0x601
/* Time in milisecs for waiting for the sensor to reset*/
#define OV7692_RESET_DELAY_MSECS    66


/*============================================================================
							DATA DECLARATIONS
============================================================================*/
/*  96MHz PCLK @ 24MHz MCLK */
struct reg_addr_val_pair_struct ov7692_init_settings_array[] = {
	{0x12, 0x80},
	{0x0e, 0x08},
	{0x69, 0x52},
	{0x1e, 0xb3},
	{0x48, 0x42},
	{0xff, 0x01},
	{0xae, 0xa0},
	{0xa8, 0x26},
	{0xb4, 0xc0},
	{0xb5, 0x40},
	{0xff, 0x00},
	{0x0c, 0x00},
	{0x62, 0x10},
	{0x12, 0x00},
	{0x17, 0x65},
	{0x18, 0xa4},
	{0x19, 0x0a},
	{0x1a, 0xf6},
	{0x3e, 0x30},
	{0x64, 0x0a},
	{0xff, 0x01},
	{0xb4, 0xc0},
	{0xff, 0x00},
	{0x67, 0x20},
	{0x81, 0xff},
	{0xcc, 0x02},
	{0xcd, 0x80},
	{0xce, 0x01},
	{0xcf, 0xe0},
	{0xc8, 0x02},
	{0xc9, 0x80},
	{0xca, 0x01},
	{0xcb, 0xe0},
	{0xd0, 0x48},
	{0x82, 0x03},
	{0x0e, 0x00},
	{0x70, 0x00},
	{0x71, 0x34},
	{0x74, 0x28},
	{0x75, 0x98},
	{0x76, 0x00},
	{0x77, 0x64},
	{0x78, 0x01},
	{0x79, 0xc2},
	{0x7a, 0x4e},
	{0x7b, 0x1f},
	{0x7c, 0x00},
	{0x11, 0x00},
	{0x20, 0x00},
	{0x21, 0x23},
//	{0x50, 0x9a}, //20pfs
	{0x50, 0x80}, //25pfs
	{0x51, 0x80},
	{0x4c, 0x7d},
	{0x0e, 0x00},
	{0x15, 0x94},
//	{0x2A, 0x60}, //20pfs
	{0x2A, 0x30}, //25pfs
//	{0x2B, 0x16}, //20pfs
	{0x2B, 0xA7}, //25pfs
	{0x2C, 0x00},
	//lens shading
	{0x80, 0x7f},
	{0x85, 0x10},
	{0x86, 0x00},
	{0x87, 0x00},
	{0x88, 0x00},
	{0x89, 0x35},
	{0x8a, 0x30},
	{0x8b, 0x30}, 
	//color matrix
	{0xbb, 0x7a},
	{0xbc, 0x69},
	{0xbd, 0x11},
	{0xbe, 0x13},
	{0xbf, 0x81},
	{0xc0, 0x96},
	{0xc1, 0x1e}, 
	//edge +De noise
	{0xb4, 0x33},
	{0xb5, 0x0d},
	{0xb6, 0x02},
	{0xb8, 0x06},
	{0xb9, 0x02},
	{0xba, 0x08},
	{0xD2, 0x02},
	{0xDA, 0x4A},
	{0xDB, 0x4A},
	//UV adjust
	{0x5a, 0x14},
	{0x5b, 0xa2},
	{0x5c, 0x70},
	{0x5d, 0x20},
	//AEC/AGC
	{0x24, 0x84},
	{0x25, 0x70},
	{0x26, 0xa4},
	//Gamma
	{0xa3, 0x17},
	{0xa4, 0x20},
	{0xa5, 0x34},
	{0xa6, 0x57},
	{0xa7, 0x68},
	{0xa8, 0x77},
	{0xa9, 0x85},
	{0xaa, 0x91},
	{0xab, 0x9c},
	{0xac, 0xa7},
	{0xad, 0xb9},
	{0xae, 0xc8},
	{0xaf, 0xdd},
	{0xb0, 0xea},
	{0xb1, 0xf1},
	{0xb2, 0x06},
	//AWB
	{0x8c, 0x5e},
	{0x8d, 0x11},
	{0x8e, 0x12},
	{0x8f, 0x19},
	{0x90, 0x50},
	{0x91, 0x20},
	{0x92, 0x96},
	{0x93, 0x80},
	{0x94, 0x13},
	{0x95, 0x1b},
	{0x96, 0xff},
	{0x97, 0x00},
	{0x98, 0x3d},
	{0x99, 0x36},
	{0x9a, 0x51},
	{0x9b, 0x43},
	{0x9c, 0xf0},
	{0x9d, 0xf0},
	{0x9e, 0xf0},
	{0x9f, 0xff},
	{0xa0, 0x68},
	{0xa1, 0x62},
	{0xa2, 0x0e},
	{0x14, 0x21}, //8x gain(0x21) 
	{0x13, 0xf7},

//	{0x30, 0x07}, //20pfs
//	{0x31, 0x82}, //20pfs
};

static bool OV7692_CSI_CONFIG;
/* 816x612, 24MHz MCLK 96MHz PCLK */
uint32_t OV7692_FULL_SIZE_WIDTH        = 640;
uint32_t OV7692_FULL_SIZE_HEIGHT       = 480;

uint32_t OV7692_QTR_SIZE_WIDTH         = 640;
uint32_t OV7692_QTR_SIZE_HEIGHT        = 480;

uint32_t OV7692_HRZ_FULL_BLK_PIXELS    = 16;
uint32_t OV7692_VER_FULL_BLK_LINES     = 12;
uint32_t OV7692_HRZ_QTR_BLK_PIXELS     = 16;
uint32_t OV7692_VER_QTR_BLK_LINES      = 12;

struct ov7692_work_t {
	struct work_struct work;
};
static struct  ov7692_work_t *ov7692_sensorw;
static struct  i2c_client *ov7692_client;
struct ov7692_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;
	uint32_t sensormode;
	uint32_t fps_divider;		/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;	/* init to 1 * 0x00000400 */
	uint32_t fps;
    uint32_t anti;
	int32_t  curr_lens_pos;
	uint32_t curr_step_pos;
	uint32_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint32_t total_lines_per_frame;
	enum ov7692_resolution_t prev_res;
	enum ov7692_resolution_t pict_res;
	enum ov7692_resolution_t curr_res;
	enum ov7692_test_mode_t  set_test;
	unsigned short imgaddr;
};
static struct ov7692_ctrl_t *ov7692_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(ov7692_wait_queue);
DEFINE_MUTEX(ov7692_mut);

/*=============================================================*/

static int ov7692_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 1,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(ov7692_client->adapter, msgs, 2) < 0) {
		printk("ov7692_i2c_rxdata failed!\n");
		return -EIO;
	}
	return 0;
}
static int32_t ov7692_i2c_txdata(unsigned short saddr,
				unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = 2,
			.buf = txdata,
		 },
	};
	if (i2c_transfer(ov7692_client->adapter, msg, 1) < 0) {
		printk("ov7692_i2c_txdata faild\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov7692_i2c_read(uint8_t raddr,
	uint8_t *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[1];
	if (!rdata)
		return -EIO;
	memset(buf, 0, sizeof(buf));
	buf[0] = raddr;
	rc = ov7692_i2c_rxdata(ov7692_client->addr, buf, rlen);
	if (rc < 0) {
		printk("ov7692_i2c_read 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = buf[0];
	return rc;
}

static int32_t ov7692_i2c_write_b_sensor(uint8_t waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[2];
	memset(buf, 0, sizeof(buf));

	buf[0] = waddr;
	buf[1] = bdata;
	rc = ov7692_i2c_txdata(ov7692_client->addr, buf, 2);
	if (rc < 0)
		printk("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			waddr, bdata);
	return rc;
}

/************************ OV7692 power up ***********************/
static int ov7692_power_up(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	rc = gpio_request(data->sensor_pwd, "ov7692_pwd");
	if (rc < 0) {
		goto powerup_fail;
	}
	gpio_direction_output(data->sensor_pwd, 0);
	return 0;
powerup_fail:
	printk("%s failed\n", __func__);
	return rc;
}

static int32_t ov7692_power_down(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	//disable mipi
	ov7692_i2c_write_b_sensor(0xff, 0x01);
	ov7692_i2c_write_b_sensor(0xb5, 0x70);
	ov7692_i2c_write_b_sensor(0xff, 0x00);

	//sw standby by
	ov7692_i2c_write_b_sensor(0x0e, 0x08);
	msleep(20);

	rc = gpio_direction_output(data->sensor_pwd, 1);
	if (rc < 0)
		goto powerdown_fail;
	gpio_free(data->sensor_pwd);
	msleep(10);
	return 0;
powerdown_fail:
	printk("%s failed\n", __func__);
	return rc;
}

static int32_t ov7692_sensor_setting(int update_type, int rt)
{
	int32_t i, array_length;
	int32_t rc = 0;
	struct msm_camera_csi_params ov7692_csi_params;
	switch (update_type) {
	case REG_INIT:
		OV7692_CSI_CONFIG = 0;
		array_length = sizeof(ov7692_init_settings_array) /
			sizeof(ov7692_init_settings_array[0]);
		for (i = 0; i < array_length; i++) {
			rc = ov7692_i2c_write_b_sensor(
				ov7692_init_settings_array[i].reg_addr,
				ov7692_init_settings_array[i].reg_val);
			if (rc < 0)
				return rc;
		}

		msleep(10);
		return rc;
		break;
	case UPDATE_PERIODIC:
		if (!OV7692_CSI_CONFIG) {
			ov7692_csi_params.lane_cnt = 1;
			ov7692_csi_params.data_format = CSI_8BIT;
			ov7692_csi_params.lane_assign = 0xe4;
			ov7692_csi_params.dpcm_scheme = 0;
			ov7692_csi_params.settle_cnt = 0x14;

			rc = msm_camio_csi_config(&ov7692_csi_params);
			msleep(10);
			array_length = sizeof(ov7692_init_settings_array) /
				sizeof(ov7692_init_settings_array[0]);
			for (i = 0; i < array_length; i++) {
				rc = ov7692_i2c_write_b_sensor(
					ov7692_init_settings_array[i].reg_addr,
					ov7692_init_settings_array[i].reg_val);
				if (rc < 0)
					return rc;
			}
		
			OV7692_CSI_CONFIG = 1;
			msleep(20);
			return rc;
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int32_t ov7692_video_config(int mode)
{
	int32_t rc = 0;
	int rt;
	/* change sensor resolution if needed */
	rt = RES_PREVIEW;

	if (ov7692_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;
	ov7692_ctrl->curr_res = ov7692_ctrl->prev_res;
	ov7692_ctrl->sensormode = mode;
	return rc;
}

#define TEST 0
static void ov7692_config_low_framerate(void)
{
	ov7692_i2c_write_b_sensor(0x2a, 0x60);
	ov7692_i2c_write_b_sensor(0x2b, 0x16);
	ov7692_i2c_write_b_sensor(0x30, 0x07);
	ov7692_i2c_write_b_sensor(0x31, 0x83);
}

static int ov7692_set_scene_mode(int value)
{
	int rc = 0;

	printk("ov7692: set scene mode value = %d\n", value);
	switch (value) {
		case SCENE_AUTO:
			printk("ov7692: set scene auto.\n");
#if TEST
			ov7692_i2c_write_b_sensor(0x11, 0x00);
			ov7692_i2c_write_b_sensor(0x15, 0xcc);
#else
			ov7692_i2c_write_b_sensor(0x15, 0x02);
			ov7692_i2c_write_b_sensor(0x2d, 0x00);
			ov7692_i2c_write_b_sensor(0x2e, 0x00);
#endif
			msleep(20);
			ov7692_i2c_write_b_sensor(0x15, 0x94);
			break;
		case SCENE_NIGHT:
			printk("ov7692: set scene night.\n");
#if TEST
			ov7692_i2c_write_b_sensor(0x11, 0x07);
			ov7692_i2c_write_b_sensor(0x29, 0x50);
			ov7692_i2c_write_b_sensor(0x15, 0x00);
			ov7692_i2c_write_b_sensor(0x2d, 0x00);
			ov7692_i2c_write_b_sensor(0x2e, 0x00);
#else
			ov7692_i2c_write_b_sensor(0x15, 0xc4);
#endif
			break;
		case SCENE_NIGHT_OFF:
			printk("ov7692: set scene night off.\n");
			break;
		case SCENE_BEACH:
			ov7692_config_low_framerate();
			printk("ov7692: set scene low framerate.\n");
			break;
		default:
			break;
	}
	if (rc < 0)
		printk("ov7692: set scene error\n");

	return 0;
}

static int ov7692_set_antibanding(int value)
{
#if !TEST
    int rc = 0;
    CDBG("value = %d\n", value);
    switch (value) {
        case CAMERA_ANTIBANDING_60HZ:
		ov7692_i2c_write_b_sensor(0x2A, 0x30);
		ov7692_i2c_write_b_sensor(0x2B, 0xa7);
		ov7692_i2c_write_b_sensor(0x50, 0x6A);
		ov7692_i2c_write_b_sensor(0x51, 0x6A);
		ov7692_i2c_write_b_sensor(0x21, 0x44);
            break;
        case CAMERA_ANTIBANDING_50HZ:
		ov7692_i2c_write_b_sensor(0x2A, 0x30);
		ov7692_i2c_write_b_sensor(0x2B, 0xa7);
		ov7692_i2c_write_b_sensor(0x50, 0x80);
		ov7692_i2c_write_b_sensor(0x51, 0x80);
            break;
        case CAMERA_ANTIBANDING_AUTO:
		ov7692_i2c_write_b_sensor(0x2A, 0x30);
		ov7692_i2c_write_b_sensor(0x2B, 0xa7);
		ov7692_i2c_write_b_sensor(0x50, 0x80);
		ov7692_i2c_write_b_sensor(0x51, 0x80);
		ov7692_i2c_write_b_sensor(0x21, 0x23);
            break;
        default:
            break;
    }

    if (rc < 0)
        printk("%s error\n", __func__);
    return rc;
#else
    return 0;
#endif
}
static int32_t ov7692_set_sensor_mode(int mode,
	int res)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = ov7692_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
	case SENSOR_RAW_SNAPSHOT_MODE:
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}


static int ov7692_set_sensor_contrast(int index)
{
	int8_t temp;

	printk("%s(), index = %d\n", __func__, index);
	switch (index) {
		case 0:
			//Contrast +3
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x30);
			ov7692_i2c_write_b_sensor(0xd3, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp &= 0xfb;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 1:
			//Contrast +3
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x2c);
			ov7692_i2c_write_b_sensor(0xd3, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp &= 0xfb;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 2:
			//Contrast +2 
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x28);
			ov7692_i2c_write_b_sensor(0xd3, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp &= 0xfb;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 3:
			//Contrast +1
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x24);
			ov7692_i2c_write_b_sensor(0xd3, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp &= 0xfb;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 4:
			//Contrast 0
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x20);
			ov7692_i2c_write_b_sensor(0xd3, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp &= 0xfb;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 5:
			//Contrast -1
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x1c);
			ov7692_i2c_write_b_sensor(0xd3, 0x20);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp |= 0x04;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 6:
			//Contrast -2
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x18);
			ov7692_i2c_write_b_sensor(0xd3, 0x48);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp |= 0x04;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 7:
			//Contrast -3
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x14);
			ov7692_i2c_write_b_sensor(0xd3, 0x80);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp |= 0x04;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		case 8:
			//Contrast -4
			ov7692_i2c_read(0x81, &temp, 1);
			temp |= 0x33;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0xd5, 0x20);
			ov7692_i2c_write_b_sensor(0xd4, 0x10);
			ov7692_i2c_write_b_sensor(0xd3, 0xd0);
			ov7692_i2c_write_b_sensor(0xd2, 0x04);
			ov7692_i2c_read(0xdc, &temp, 1);
			temp |= 0x04;
			ov7692_i2c_write_b_sensor(0xdc, temp);
			break;
		default:
			break;
	}
	return 0;
}


static int32_t ov7692_set_sensor_effect(uint8_t effect) 
{
	int32_t rc = 0;
#if TEST
	uint8_t temp;
#endif
	printk("ov7692: enter %s\n", __func__);
	switch (effect) {
		case EFFECT_NONE:
			printk("ov7692: effect none.\n");
#if TEST
			ov7692_i2c_read(0x81, &temp, 1);
			temp &= 0xdf;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0x28, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x00);
			ov7692_i2c_write_b_sensor(0xda, 0x80);
			ov7692_i2c_write_b_sensor(0xdb, 0x80);
#else
			ov7692_i2c_write_b_sensor(0x81, 0xff);
			ov7692_i2c_write_b_sensor(0xd2, 0x06);
			ov7692_i2c_write_b_sensor(0xda, 0x80);
			ov7692_i2c_write_b_sensor(0xda, 0x80);
#endif
			break;
		case EFFECT_NEGATIVE:
			printk("ov7692: effect negative.\n");
#if TEST
			ov7692_i2c_read(0x81, &temp, 1);
			temp &= 0x20;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0x28, 0x80);
			ov7692_i2c_write_b_sensor(0xd2, 0x00);
			ov7692_i2c_write_b_sensor(0xda, 0x80);
			ov7692_i2c_write_b_sensor(0xdb, 0x80);
#else
			ov7692_i2c_write_b_sensor(0x81, 0xff);
			ov7692_i2c_write_b_sensor(0xd2, 0x66);
			ov7692_i2c_write_b_sensor(0xda, 0x80);
			ov7692_i2c_write_b_sensor(0xdb, 0x80);
#endif
			break;
		case EFFECT_SEPIA:
			printk("ov7692: effect sepia.\n");
#if TEST
			ov7692_i2c_read(0x81, &temp, 1);
			temp &= 0x20;
			ov7692_i2c_write_b_sensor(0x81, temp);
			ov7692_i2c_write_b_sensor(0x28, 0x00);
			ov7692_i2c_write_b_sensor(0xd2, 0x18);
			ov7692_i2c_write_b_sensor(0xda, 0x40);
			ov7692_i2c_write_b_sensor(0xdb, 0xa0);
#else
			ov7692_i2c_write_b_sensor(0x81, 0xff);
			ov7692_i2c_write_b_sensor(0xd2, 0x1e);
			ov7692_i2c_write_b_sensor(0xda, 0x40);
			ov7692_i2c_write_b_sensor(0xdb, 0xa0);
#endif
			break;
		default:
			break;
	}
	if (rc < 0)
		printk("ov7692: %s Error\n", __func__);
	return 0;
}

static int ov7692_set_sensor_wb(int index)
{
	printk("%s(), index = %d\n", __func__, index);
#if !TEST
	switch (index) {
		case 1:
			//Auto
			ov7692_i2c_write_b_sensor(0x13, 0xf7); //AWB on
			break;
		case 3:
			//Home
			ov7692_i2c_write_b_sensor(0x13, 0xf5); //AWB off
			ov7692_i2c_write_b_sensor(0x01, 0x66);
			ov7692_i2c_write_b_sensor(0x02, 0x40);
			break;
		case 5:
			//Sunny
			ov7692_i2c_write_b_sensor(0x13, 0xf5); //AWB off
			ov7692_i2c_write_b_sensor(0x01, 0x43);
			ov7692_i2c_write_b_sensor(0x02, 0x5d);

			break;
		case 4:
			//Office
			ov7692_i2c_write_b_sensor(0x13, 0xf5); //AWB off
			ov7692_i2c_write_b_sensor(0x01, 0x56);
			ov7692_i2c_write_b_sensor(0x02, 0x50);
			break;
		case 6:
			//Cloudy
			ov7692_i2c_write_b_sensor(0x13, 0xf5); //AWB off
			ov7692_i2c_write_b_sensor(0x01, 0x48);
			ov7692_i2c_write_b_sensor(0x02, 0x63);
			break;
		default:
			break;
	}
#endif
	return 0;
}

static int ov7692_set_effect(int mode, int value)
{
    int rc = 0;
    printk("ov7692: enter %s, mode = %d value = %d\n", __func__, mode, value);
	switch (mode){
		case CFG_SET_BESTSHOT_MODE:
			rc = ov7692_set_scene_mode(value);
			break;
		case CFG_SET_ANTIBANDING:
	            	rc = ov7692_set_antibanding(value);
			break;
		default:
			printk("ov7692: No such effect\n");
			break;
	}
	if (rc < 0)
		printk("ov7692: set effect Error\n");

    return rc;
}


static int ov7692_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	uint8_t model_id_msb, model_id_lsb = 0;
	uint16_t model_id;
	int32_t rc = 0;
	/*The reset pin is not physically connected to the sensor.
	The standby pin will do the reset hence there is no need
	to request the gpio reset*/

	/* Read sensor Model ID: */
	rc = ov7692_i2c_read(REG_OV7692_MODEL_ID_MSB, &model_id_msb, 1);
	if (rc < 0)
		goto init_probe_fail;
	rc = ov7692_i2c_read(REG_OV7692_MODEL_ID_LSB, &model_id_lsb, 1);
	if (rc < 0)
		goto init_probe_fail;
	model_id = (model_id_msb << 8) | ((model_id_lsb & 0x00FF)) ;
	printk("ov7692 model_id = 0x%x, 0x%x, 0x%x\n",
		 model_id, model_id_msb, model_id_lsb);
	/* 4. Compare sensor ID to OV7692 ID: */
	if (model_id != OV7692_MODEL_ID) {
		rc = -ENODEV;
		goto init_probe_fail;
	}
	goto init_probe_done;

init_probe_fail:
	printk(" ov7692_probe_init_sensor fails\n");
	return rc;
init_probe_done:
	return rc;
}

int ov7692_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	ov7692_ctrl = kzalloc(sizeof(struct ov7692_ctrl_t), GFP_KERNEL);
	if (!ov7692_ctrl) {
		rc = -ENOMEM;
		goto init_done;
	}
	ov7692_ctrl->fps_divider = 1 * 0x00000400;
	ov7692_ctrl->pict_fps_divider = 1 * 0x00000400;
	ov7692_ctrl->fps = 30 * Q8;
	ov7692_ctrl->set_test = TEST_OFF;
	ov7692_ctrl->prev_res = QTR_SIZE;
	ov7692_ctrl->pict_res = FULL_SIZE;
	ov7692_ctrl->curr_res = INVALID_SIZE;
    ov7692_ctrl->anti = CAMERA_ANTIBANDING_50HZ;

	if (data)
		ov7692_ctrl->sensordata = data;

	/* enable mclk first */

	msm_camio_clk_rate_set(24000000);
	msleep(20);
	rc = ov7692_power_up(ov7692_ctrl->sensordata);
	if (rc < 0)
		goto init_fail;
	msleep(10);
        ov7692_i2c_write_b_sensor(0x12, 0x80);
	msleep(30);
	rc = ov7692_probe_init_sensor(data);
	if (rc < 0) {
		goto init_fail;
	}

	rc = ov7692_sensor_setting(REG_INIT, RES_PREVIEW);
	if (rc < 0)
		goto init_fail;
	else
		goto init_done;

init_fail:
	printk(" ov7692_sensor_open_init fail\n");
	ov7692_power_down(ov7692_ctrl->sensordata);
	kfree(ov7692_ctrl);
	return rc;
init_done:
	return rc;
}

static int ov7692_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov7692_wait_queue);
	return 0;
}

static const struct i2c_device_id ov7692_i2c_id[] = {
	{"ov7692", 0},
	{ }
};

static int ov7692_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		goto probe_failure;
	}

	ov7692_sensorw = kzalloc(sizeof(struct ov7692_work_t), GFP_KERNEL);
	if (!ov7692_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov7692_sensorw);
	ov7692_init_client(client);
	ov7692_client = client;

	return 0;

probe_failure:
	printk("ov7692_i2c_probe failed! rc = %d\n", rc);
	return rc;
}

static int __exit ov7692_remove(struct i2c_client *client)
{
	struct ov7692_work_t_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	ov7692_client = NULL;
	kfree(sensorw);
	return 0;
}

static struct i2c_driver ov7692_i2c_driver = {
	.id_table = ov7692_i2c_id,
	.probe  = ov7692_i2c_probe,
	.remove = __exit_p(ov7692_i2c_remove),
	.driver = {
		.name = "ov7692",
	},
};

int ov7692_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;
	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&ov7692_mut);
	CDBG("ov7692_sensor_config: cfgtype = %d\n",
	cdata.cfgtype);
	switch (cdata.cfgtype) {
	case CFG_SET_MODE:
		rc = ov7692_set_sensor_mode(cdata.mode,
			cdata.rs);
		break;
	case CFG_SET_CONTRAST:
		rc = ov7692_set_sensor_contrast(cdata.cfg.contrast);
		break;
	case CFG_SET_WB:
		rc = ov7692_set_sensor_wb(cdata.cfg.wb);
		break;
    case CFG_SET_EFFECT:
        rc = ov7692_set_sensor_effect(cdata.cfg.effect);
        break;
	case CFG_PWR_DOWN:
		rc = ov7692_power_down(ov7692_ctrl->sensordata);
		break;
	default:
		rc = 0;
		printk("%s(), not support the effect\n", __func__);
		break;
	}

	mutex_unlock(&ov7692_mut);

	return rc;
}
static int ov7692_sensor_release(void)
{
	int rc = -EBADF;
	mutex_lock(&ov7692_mut);
	ov7692_power_down(ov7692_ctrl->sensordata);
	kfree(ov7692_ctrl);
	ov7692_ctrl = NULL;
	printk("ov7692_release completed\n");
	mutex_unlock(&ov7692_mut);

	return rc;
}

static int ov7692_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = i2c_add_driver(&ov7692_i2c_driver);
	if (rc < 0 || ov7692_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_fail;
	}

	rc = ov7692_power_up(info);
	if (rc < 0)
		goto fail;
	msleep(10);
	msm_camio_clk_rate_set(24000000);
	msleep(10);
        ov7692_i2c_write_b_sensor(0x12, 0x80);
	msleep(30);
	rc = ov7692_probe_init_sensor(info);
	if (rc < 0)
		goto probe_fail;
	rc = ov7692_sensor_setting(REG_INIT, RES_PREVIEW);
	if (rc < 0)
		goto probe_fail;
	s->s_init = ov7692_sensor_open_init;
	s->s_release = ov7692_sensor_release;
	s->s_config  = ov7692_sensor_config;
    s->s_set_effect = ov7692_set_effect;
	s->s_camera_type = FRONT_CAMERA_2D;
	s->s_mount_angle = 270;
	msleep(10);
	rc = ov7692_power_down(info);
	if (rc < 0)
		goto fail;
        //info->pdata->camera_gpio_off();
        //msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
	printk("%s success\n", __func__);
	return rc;

probe_fail:
	printk("ov7692_sensor_probe: SENSOR PROBE FAILS!\n");
	ov7692_power_down(info);
fail:
	i2c_del_driver(&ov7692_i2c_driver);
	return rc;
}

static int __ov7692_probe(struct platform_device *pdev)
{

	return msm_camera_drv_start(pdev, ov7692_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov7692_probe,
	.driver = {
		.name = "msm_camera_ov7692",
		.owner = THIS_MODULE,
	},
};

static int __init ov7692_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov7692_init);

MODULE_DESCRIPTION("OMNI VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
