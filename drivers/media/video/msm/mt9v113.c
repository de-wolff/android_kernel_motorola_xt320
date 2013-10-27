/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "mt9v113.h"

#define MT9V113_MASTER_CLK_RATE 24000000
#define SENSOR_DEBUG		0

struct mt9v113_ctrl {
	const struct msm_camera_sensor_info *sensordata;
	uint32_t sensormode;
	enum mt9v113_resolution_t prev_res;
	enum mt9v113_resolution_t pict_res;
	enum mt9v113_resolution_t curr_res;
};

struct mt9v113_work {
	struct work_struct work;
};
//const struct msm_camera_sensor_info *g_sensor_info;
static DECLARE_WAIT_QUEUE_HEAD(mt9v113_wait_queue);
static struct  mt9v113_work *mt9v113_sensorw;
static struct mt9v113_ctrl *mt9v113_ctrl;
static struct  i2c_client *mt9v113_client;
static int32_t g_settle_cnt = 7;
static bool CSI_CONFIG = false;
DEFINE_MUTEX(mt9v113_mutex);

extern struct mt9v113_reg mt9v113_regs;
/************************* I2C function **************************/
static int32_t mt9v113_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

#if SENSOR_DEBUG
	if (length == 2)
		printk("msm_io_i2c_w: 0x%04x 0x%04x\n",
			*(u16 *) txdata, *(u16 *) (txdata + 2));
	else if (length == 4)
		printk("msm_io_i2c_w: 0x%04x\n", *(u16 *) txdata);
	else
		printk("msm_io_i2c_w: length = %d\n", length);
#endif
	if (i2c_transfer(mt9v113_client->adapter, msg, 1) < 0) {
		printk("mt9v113_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9v113_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9v113_width width)
{
	int32_t rc = -EIO;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0xFF00)>>8;
		buf[3] = (wdata & 0x00FF);

		rc = mt9v113_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = waddr;
		buf[1] = wdata;
		rc = mt9v113_i2c_txdata(saddr, buf, 2);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		printk(
		"i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		waddr, wdata);

	return rc;
}

static int32_t mt9v113_i2c_write_table(
	struct mt9v113_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = mt9v113_i2c_write(mt9v113_client->addr,
			reg_conf_tbl->waddr, reg_conf_tbl->wdata,
			reg_conf_tbl->width);
		if (rc < 0)
			break;
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}

static int mt9v113_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

#if SENSOR_DEBUG
	if (length == 2)
		printk("msm_io_i2c_r: 0x%04x 0x%04x\n",
			*(u16 *) rxdata, *(u16 *) (rxdata + 2));
	else if (length == 4)
		printk("msm_io_i2c_r: 0x%04x\n", *(u16 *) rxdata);
	else
		printk("msm_io_i2c_r: length = %d\n", length);
#endif

	if (i2c_transfer(mt9v113_client->adapter, msgs, 2) < 0) {
		printk("mt9v113_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9v113_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum mt9v113_width width)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9v113_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		printk("mt9v113_i2c_read failed!\n");

	return rc;
}

static int32_t mt9v113_start_stream(void)
{
	int32_t rc = 0;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x002A, WORD_LEN);
	if (rc < 0){
			goto config_fail;
	}
	mdelay(100);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	if (rc < 0){
			goto config_fail;
	}
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
	if (rc < 0){
			goto config_fail;
	}
	mdelay(50);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	if (rc < 0){
			goto config_fail;
	}
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
	if (rc < 0){
			goto config_fail;
	}
	mdelay(50);

	rc = mt9v113_i2c_write_table(&mt9v113_regs.config_third[0], mt9v113_regs.config_third_size);
	if (rc < 0){
			goto config_fail;
	}
	mdelay(50);

	return 0;

config_fail:
	printk("failed\n");
	return rc;
}

/************************ MT9V113 power up ***********************/
static int mt9v113_power_up(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	rc = gpio_request(data->sensor_pwd, "mt9v113_pwd");
	if (rc < 0) {
		printk("mt9v113_power_up--gpio_request error\n");
	}
	gpio_direction_output(data->sensor_pwd, 0);
	return 0;
}

static int32_t mt9v113_power_down(const struct msm_camera_sensor_info *data)
{
	gpio_direction_output(data->sensor_pwd, 1);
	gpio_free(data->sensor_pwd);
	return 0;
}

static int mt9v113_detect(void)
{
    uint16_t id = 0;

    if (mt9v113_i2c_read(mt9v113_client->addr, 0x0000, &id, WORD_LEN))
        goto err_detect;

    if (id == 0x2280) {
        printk("mt9v113_detect detect success, reg = 0x0, value = 0x%x\n", id);
        return 0;
    }

err_detect:
    printk("detect failed\n");
    return -ENODEV;
}

static int mt9v113_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	rc = mt9v113_detect();
	if (rc < 0)
		goto probe_fail;
	return rc;

probe_fail:
	printk("mt9v113_sensor_init_probe failed\n");
	return rc;
}

static int32_t mt9v113_reg_config(int update_type, int rt)
{
	int32_t rc = 0;

	if (update_type == REG_INIT){
		rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x4028, WORD_LEN);
		if (rc < 0)	
			goto config_fail;
		mdelay(100);

		rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0011, WORD_LEN);
		if (rc < 0)	
			goto config_fail;
		mdelay(10);

		rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0010, WORD_LEN);
			if (rc < 0)	
				goto config_fail;
		mdelay(10);
				
		rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x402E, WORD_LEN);
			if (rc < 0)	
				goto config_fail;
		mdelay(100);
			
		rc = mt9v113_i2c_write_table(&mt9v113_regs.config_first[0], mt9v113_regs.config_first_size);
		if (rc < 0)
			goto config_fail;
		mdelay(10);

		rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0x304B, WORD_LEN);
			if (rc < 0)	
				goto config_fail;
		mdelay(10);

		rc = mt9v113_i2c_write_table(&mt9v113_regs.config_second[0], mt9v113_regs.config_second_size);
		if (rc < 0){
			printk("config4 failed\n");
			goto config_fail;
		}
		mdelay(50);

		return rc;
	}
	else if (update_type == UPDATE_PERIODIC){
		if (rt == RES_PREVIEW) {
			//To config preview register
			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
			if (rc < 0)	
				goto config_fail;

			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
			if (rc < 0)	
				goto config_fail;

			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
			if (rc < 0)	
				goto config_fail;

			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0001, WORD_LEN);
			if (rc < 0)	
				goto config_fail;
		} else {
			//To config capture register
			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
			if (rc < 0)	
				goto config_fail;

			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0002, WORD_LEN);
			if (rc < 0)
				goto config_fail;

			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
			if (rc < 0)
				goto config_fail;

			rc =	mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0002, WORD_LEN);
			if (rc < 0)
				goto config_fail;
		}
		return rc;
	}

config_fail:
	printk("failed\n");
	return rc;
}

static void csi_config(void)
{
	struct msm_camera_csi_params csi_params;
	if (!CSI_CONFIG) {
		msm_camio_vfe_clk_rate_set(192000000);
		csi_params.lane_cnt = 1;
		csi_params.data_format = CSI_8BIT;
		csi_params.lane_assign = 0xe4;
		csi_params.dpcm_scheme = 0;
		csi_params.settle_cnt = g_settle_cnt;
		msm_camio_csi_config(&csi_params);
		mdelay(20);
		CSI_CONFIG = true;
	}
}

static int32_t mt9v113_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;

	if (update_type == REG_INIT) {
		mt9v113_reg_config(update_type, rt);
		CSI_CONFIG = false;
	} else if (update_type == UPDATE_PERIODIC) {
		if (rt == RES_PREVIEW) {
			// Config mt9v113 to Preview
			mt9v113_reg_config(update_type, rt);
			mdelay(20);

			csi_config();
			mdelay(100);

			mt9v113_start_stream();
			mdelay(30);		
		} else {
			// Config mt9v113 to Capture
			mt9v113_reg_config(update_type, rt);
			mdelay(20);
		}
	}
	return rc;
}

int mt9v113_sensor_open_init(const struct msm_camera_sensor_info *info)
{
	int32_t rc = 0;

	mt9v113_ctrl = kzalloc(sizeof(struct mt9v113_ctrl), GFP_KERNEL);
	if (!mt9v113_ctrl) {
		printk("mt9v113_init failed!\n");
		rc = -ENOMEM;
		goto kzalloc_fail;
	}

	mt9v113_ctrl->prev_res = QTR_SIZE;
	mt9v113_ctrl->pict_res = FULL_SIZE;
	if (info)
		mt9v113_ctrl->sensordata = info;

	mt9v113_power_up(mt9v113_ctrl->sensordata);
	mdelay(20);
	msm_camio_clk_rate_set(MT9V113_MASTER_CLK_RATE);
	rc = mt9v113_sensor_init_probe(info);
	if (rc < 0) 
		goto init_fail;

	if (mt9v113_ctrl->prev_res == QTR_SIZE)
		rc = mt9v113_sensor_setting(REG_INIT, RES_PREVIEW);
	else
		rc = mt9v113_sensor_setting(REG_INIT, RES_CAPTURE);
	if (rc < 0)
		goto init_fail;

	return rc;

init_fail:
	mt9v113_power_down(info);
kzalloc_fail:
	printk("failed\n");
	return rc;
}

static int mt9v113_sensor_release(void) 
{
	mutex_lock(&mt9v113_mutex);
	mt9v113_power_down(mt9v113_ctrl->sensordata);
	mdelay(20);
	kfree(mt9v113_ctrl);
	mt9v113_ctrl = NULL;
	printk("mt9v113_sensor_release completed\n");
	mutex_unlock(&mt9v113_mutex);
	return 0;
}

static void mt9v113_get_pict_fps(uint16_t fps, uint16_t *pfps) 
{
}

static uint16_t mt9v113_get_prev_lines_pf(void)
{
	return 0;
}

static int32_t mt9v113_video_config(int mode)
{
	int32_t rc = 0;
	int rt;

	/* change sensor resolution if needed */
	if (mt9v113_ctrl->prev_res == QTR_SIZE)
		rt = RES_PREVIEW;
	else
		rt = RES_CAPTURE;

	if (mt9v113_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;

	mt9v113_ctrl->curr_res = mt9v113_ctrl->prev_res;
	mt9v113_ctrl->sensormode = mode;
	return 0;
}

static int32_t mt9v113_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;

	/*change sensor resolution if needed */
	if (mt9v113_ctrl->curr_res != mt9v113_ctrl->pict_res) {
		if (mt9v113_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
	
		if (mt9v113_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}
	mt9v113_ctrl->curr_res = mt9v113_ctrl->pict_res;
	mt9v113_ctrl->sensormode = mode;
	return rc;

}

static int32_t mt9v113_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;

	/* change sensor resolution if needed */
	if (mt9v113_ctrl->curr_res != mt9v113_ctrl->pict_res) {
		if (mt9v113_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;

		if (mt9v113_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	mt9v113_ctrl->curr_res = mt9v113_ctrl->pict_res;
	mt9v113_ctrl->sensormode = mode;
	return rc;

}

static int32_t mt9v113_set_sensor_mode(int mode)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		printk("%s preview\n", __func__);
		rc = mt9v113_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		printk("%s snapshot\n", __func__);
		rc = mt9v113_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		printk("%s raw snapshot\n", __func__);
		rc = mt9v113_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int32_t mt9v113_set_default_focus(uint8_t af_step) 
{
	printk("\n");
	return 0;
}

static int32_t mt9v113_sensor_config(void __user *argp) 
{
	struct sensor_cfg_data cdata;
	long rc = 0;
	if (copy_from_user(&cdata, (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	
	mutex_lock(&mt9v113_mutex);
	printk("cfg type = %d\n", cdata.cfgtype);
	switch (cdata.cfgtype) {
		case CFG_GET_PICT_FPS:
			mt9v113_get_pict_fps(
					cdata.cfg.gfps.prevfps,
					&(cdata.cfg.gfps.pictfps));

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PREV_L_PF:
			cdata.cfg.prevl_pf =
				mt9v113_get_prev_lines_pf();

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_SET_MODE:
			rc = mt9v113_set_sensor_mode(cdata.mode);
			break;
		case CFG_PWR_DOWN:
			rc = mt9v113_power_down(mt9v113_ctrl->sensordata);
			break;
		case CFG_SET_EFFECT:
			rc = mt9v113_set_default_focus(cdata.cfg.effect);
			break;
		default:
			rc = -EFAULT;
			break;
	}
	mutex_unlock(&mt9v113_mutex);
	return rc;
}

static int mt9v113_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mt9v113_wait_queue);
	return 0;
}

static int mt9v113_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_fail;
	}

	mt9v113_sensorw = kzalloc(sizeof(struct mt9v113_work), GFP_KERNEL);
	if (!mt9v113_sensorw) {
		rc = -ENOMEM;
		goto probe_fail;
	}

	i2c_set_clientdata(client, mt9v113_sensorw);
	mt9v113_init_client(client);
	mt9v113_client = client;

	printk("succeeded!\n");
	return rc;

probe_fail:
	kfree(mt9v113_sensorw);
	mt9v113_sensorw = NULL;
	printk("fail\n");
	return rc;
}

static int __exit mt9v113_i2c_remove(struct i2c_client *client)
{
        struct  mt9v113_work *sensorw = i2c_get_clientdata(client);
        mt9v113_client = NULL;
        kfree(sensorw);
        return 0;
}

static const struct i2c_device_id mt9v113_i2c_id[] = {
	{ "mt9v113", 0},
};

static struct i2c_driver mt9v113_i2c_driver = {
	.id_table = mt9v113_i2c_id,
	.probe  = mt9v113_i2c_probe,
	.remove = __exit_p(mt9v113_i2c_remove),
	.driver = {
		.name = "mt9v113",
	},
};

static int mt9v113_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = i2c_add_driver(&mt9v113_i2c_driver);
	if (rc < 0 || mt9v113_client == NULL) {
		rc = -ENOTSUPP;
		printk("I2C add driver failed");
		goto add_driver_fail;
	}

	mt9v113_power_up(info);
	
	msm_camio_clk_rate_set(MT9V113_MASTER_CLK_RATE);

	rc = mt9v113_sensor_init_probe(info);
	if (rc < 0) {
		goto probe_fail;
	}

	s->s_init = mt9v113_sensor_open_init;
	s->s_release = mt9v113_sensor_release;
	s->s_config  = mt9v113_sensor_config;
	s->s_mount_angle = 90;
	mt9v113_power_down(info);
	msleep(20);
	return rc;

probe_fail:
	i2c_del_driver(&mt9v113_i2c_driver);
	mt9v113_power_down(info);
add_driver_fail:
	printk("mt9v113_sensor_probe failed\n");
	return rc;
}

static int __devinit mt9v113_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9v113_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = mt9v113_probe,
	.driver = {
		.name = "msm_camera_mt9v113",
		.owner = THIS_MODULE,
	},
};

static int __init mt9v113_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9v113_init);
MODULE_DESCRIPTION("0.3 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
