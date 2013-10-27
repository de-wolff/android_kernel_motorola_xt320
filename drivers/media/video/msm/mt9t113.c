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
#include "mt9t113.h"

//#define MT9T113_MASTER_CLK_RATE 12000000
#define MT9T113_MASTER_CLK_RATE 24000000
#define SENSOR_DEBUG		0
#define SENSOR_RESET		89

#define DRIVER_NAME ""
#define PRINTK1(f, x...) \
        printk(DRIVER_NAME " [%s() line=%d]: " f, __func__, __LINE__, ## x)

struct mt9t113_ctrl {
	const struct msm_camera_sensor_info *sensordata;
	uint32_t sensormode;
	enum mt9t113_resolution_t prev_res;
	enum mt9t113_resolution_t pict_res;
	enum mt9t113_resolution_t curr_res;
};

struct mt9t113_work {
	struct work_struct work;
};
const struct msm_camera_sensor_info *g_sensor_info;
static DECLARE_WAIT_QUEUE_HEAD(mt9t113_wait_queue);
static struct  mt9t113_work *mt9t113_sensorw;
//static uint16_t prev_frame_length_lines;
//static uint16_t snap_frame_length_lines;
static struct mt9t113_ctrl *mt9t113_ctrl;
static struct  i2c_client *mt9t113_client;
static int32_t g_settle_cnt = 7;
static bool CSI_CONFIG = false;
DEFINE_MUTEX(mt9t113_mutex);
extern struct mt9t113_reg mt9t113_regs;
/************************* I2C function **************************/
static int32_t mt9t113_i2c_txdata(unsigned short saddr,
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
	if (i2c_transfer(mt9t113_client->adapter, msg, 1) < 0) {
		printk("mt9t113_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9t113_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9t113_width width)
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

		rc = mt9t113_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = waddr;
		buf[1] = wdata;
		rc = mt9t113_i2c_txdata(saddr, buf, 2);
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

static int32_t mt9t113_i2c_write_table(
	struct mt9t113_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = mt9t113_i2c_write(mt9t113_client->addr,
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

static int mt9t113_i2c_rxdata(unsigned short saddr,
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

	if (i2c_transfer(mt9t113_client->adapter, msgs, 2) < 0) {
		printk("mt9t113_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9t113_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum mt9t113_width width)
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

		rc = mt9t113_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		printk("mt9t113_i2c_read failed!\n");

	return rc;
}

/************************ MT9T113 Control Function ***********************/
static int mt9t113_power_up(void)
{
	gpio_set_value(g_sensor_info->sensor_pwd, 0);
	gpio_set_value(g_sensor_info->sensor_reset, 1);
	return 0;
}

static void mt9t113_reg_reset(void)
{
	PRINTK1("\n");
//	mt9t113_i2c_write(mt9t113_client->addr, 0x0018, 0x4129, WORD_LEN);
	mt9t113_i2c_write(mt9t113_client->addr, 0x0018, 0x012a, WORD_LEN);
	msleep(1);
	mt9t113_i2c_write(mt9t113_client->addr, 0x0018, 0x4029, WORD_LEN);
	msleep(5);
}

static void mt9t113_gpio_reset(void)
{
	PRINTK1("\n");
	gpio_set_value(g_sensor_info->sensor_reset, 0);
	msleep(20);
	gpio_set_value(g_sensor_info->sensor_reset, 1);
	msleep(20);
}

static int mt9t113_detect(void)
{
    uint16_t id;
    if (mt9t113_i2c_read(mt9t113_client->addr, 0x0000, &id, WORD_LEN))
        goto err_detect;

    if (id == 0x4680) {
        PRINTK1("detect success, reg = 0x0, value = 0x%x\n", id);
        return 0;
    }
err_detect:
    PRINTK1("detect failed\n");
    return -ENODEV;
}

static int mt9t113_gpio_free(const struct msm_camera_sensor_info *data)
{
	gpio_free(g_sensor_info->flash_data->flash_src->_fsrc.current_driver_src.led1);
	gpio_free(g_sensor_info->flash_data->flash_src->_fsrc.current_driver_src.led2);
	gpio_free(g_sensor_info->sensor_reset);
	gpio_free(g_sensor_info->sensor_pwd);
	return 0;
}

static int32_t mt9t113_stop_stream(void)
{
	return 0;
}

static int32_t mt9t113_start_stream(void)
{
	mt9t113_i2c_write(mt9t113_client->addr, 0x0018, 0x002A, WORD_LEN);
	msleep(200);
	mt9t113_i2c_write_table(&mt9t113_regs.config4[0], mt9t113_regs.config4_size);
	return 0;
}

static void mt9t113_reset_sensor(void) 
{
	PRINTK1("\n");
	mt9t113_reg_reset();
}
static int32_t mt9t113_reg_config(int update_type, int rt)
{
	long rc;
	if (update_type == REG_INIT) {
		rc = mt9t113_i2c_write_table(&mt9t113_regs.config1[0], mt9t113_regs.config1_size);
		if (rc < 0)
			goto config_fail;
		msleep(100);

		rc = mt9t113_i2c_write_table(&mt9t113_regs.config2[0], mt9t113_regs.config2_size);
		if (rc < 0)
			goto config_fail;
		msleep(200);
		rc = mt9t113_i2c_write_table(&mt9t113_regs.config3[0], mt9t113_regs.config3_size);
		if (rc < 0)
			goto config_fail;
		msleep(30);

	} else {

		if (rt == RES_PREVIEW) {
			PRINTK1("PREVIEW\n");		
			rc = mt9t113_i2c_write_table(&mt9t113_regs.preview[0], mt9t113_regs.preview_size);
			if (rc < 0) {
				PRINTK1("###### failed #####\n");
				goto config_fail;
			}
			msleep(100);
		} else {
			PRINTK1("CAPTURE\n");
			rc = mt9t113_i2c_write_table(&mt9t113_regs.capture[0], mt9t113_regs.capture_size);
			if (rc < 0) {
				PRINTK1("###### failed #####\n");
				goto config_fail;
			}
			msleep(100);
		}
	}
	return rc;

config_fail:
	PRINTK1("failed\n");
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
		msleep(20);
		CSI_CONFIG = true;
	}
}

static int32_t mt9t113_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;
	mt9t113_stop_stream();
	msleep(30);
	if (update_type == REG_INIT) {
		mt9t113_reset_sensor();
		mt9t113_reg_config(update_type, rt);
		CSI_CONFIG = false;
	} else if (update_type == UPDATE_PERIODIC) {
		if (rt == RES_PREVIEW) {
			// Config mt9t113 to Preview
			mt9t113_reg_config(update_type, rt);
			msleep(100);
			csi_config();
			PRINTK1("\n");
			msleep(150);
			mt9t113_start_stream();
			PRINTK1("\n");
			msleep(30);

		} else {
			// Config mt9t113 to Capture
			mt9t113_reg_config(update_type, rt);
			msleep(20);
		}
	}
	return rc;
}

int mt9t113_sensor_open_init(const struct msm_camera_sensor_info *info)
{
	int32_t rc = 0;
	PRINTK1("\n");
	mt9t113_ctrl = kzalloc(sizeof(struct mt9t113_ctrl), GFP_KERNEL);
	if (!mt9t113_ctrl) {
		PRINTK1("mt9t113_init failed!\n");
		rc = -ENOMEM;
		goto init_fail;
	}

	mt9t113_ctrl->prev_res = QTR_SIZE;
	mt9t113_ctrl->pict_res = FULL_SIZE;
	if (info)
		mt9t113_ctrl->sensordata = info;

	msm_camio_clk_rate_set(MT9T113_MASTER_CLK_RATE);

	rc = mt9t113_detect();
	if (rc < 0) {
		PRINTK1("detect failed!\n");
		goto init_fail;
	}

	mt9t113_gpio_reset();
	if (mt9t113_ctrl->prev_res == QTR_SIZE)
		rc = mt9t113_sensor_setting(REG_INIT, RES_PREVIEW);
	else
		rc = mt9t113_sensor_setting(REG_INIT, RES_CAPTURE);
	if (rc < 0)
		goto init_fail;
	PRINTK1("done\n");
	return rc;

init_fail:
	PRINTK1("failed\n");
	mt9t113_gpio_free(info);
	return rc;
}

static int32_t mt9t113_power_down(void)
{
	mt9t113_gpio_free(NULL);
	return 0;
}

static int mt9t113_sensor_release(void) 
{
	mutex_lock(&mt9t113_mutex);
	mt9t113_power_down();
	msleep(20);
	kfree(mt9t113_ctrl);
	mt9t113_ctrl = NULL;
	PRINTK1("completed\n");
	mutex_unlock(&mt9t113_mutex);
	return 0;
}

static void mt9t113_get_pict_fps(uint16_t fps, uint16_t *pfps) 
{
//	*pfps = -1;	
//	PRINTK1("Pending\n");
}

static uint16_t mt9t113_get_prev_lines_pf(void)
{
#if 0
	PRINTK1("Pending\n");
	if (mt9t113_ctrl->prev_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
#endif
	return 0;
}

static int32_t mt9t113_video_config(int mode)
{
	int32_t rc = 0;
	int rt;
	PRINTK1("\n");
	/* change sensor resolution if needed */
	if (mt9t113_ctrl->prev_res == QTR_SIZE)
		rt = RES_PREVIEW;
	else
		rt = RES_CAPTURE;
	if (mt9t113_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;

	mt9t113_ctrl->curr_res = mt9t113_ctrl->prev_res;
	mt9t113_ctrl->sensormode = mode;
	return 0;
}

static int32_t mt9t113_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	PRINTK1("\n");
	/*change sensor resolution if needed */
	if (mt9t113_ctrl->curr_res != mt9t113_ctrl->pict_res) {
		if (mt9t113_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (mt9t113_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}
	mt9t113_ctrl->curr_res = mt9t113_ctrl->pict_res;
	mt9t113_ctrl->sensormode = mode;
	return rc;

}

static int32_t mt9t113_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	PRINTK1("\n");
	/* change sensor resolution if needed */
	if (mt9t113_ctrl->curr_res != mt9t113_ctrl->pict_res) {
		if (mt9t113_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (mt9t113_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	mt9t113_ctrl->curr_res = mt9t113_ctrl->pict_res;
	mt9t113_ctrl->sensormode = mode;
	return rc;

}

static int32_t mt9t113_set_sensor_mode(int mode)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = mt9t113_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		rc = mt9t113_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		rc = mt9t113_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int32_t mt9t113_set_default_focus(uint8_t af_step) 
{
//	PRINTK1("Pending\n");
	return 0;
}

static int32_t mt9t113_sensor_config(void __user *argp) 
{
	struct sensor_cfg_data cdata;
	long rc = 0;
	if (copy_from_user(&cdata, (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	
	mutex_lock(&mt9t113_mutex);
	PRINTK1("cfg type = %d\n", cdata.cfgtype);
	switch (cdata.cfgtype) {
		case CFG_GET_PICT_FPS:
			mt9t113_get_pict_fps(
					cdata.cfg.gfps.prevfps,
					&(cdata.cfg.gfps.pictfps));

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PREV_L_PF:
			cdata.cfg.prevl_pf =
				mt9t113_get_prev_lines_pf();

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
#if 0
		case CFG_GET_PREV_P_PL:
			cdata.cfg.prevp_pl =
				mt9t113_get_prev_pixels_pl();

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PICT_L_PF:
			cdata.cfg.pictl_pf =
				mt9t113_get_pict_lines_pf();

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PICT_P_PL:
			cdata.cfg.pictp_pl =
				mt9t113_get_pict_pixels_pl();
			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PICT_MAX_EXP_LC:
			cdata.cfg.pict_max_exp_lc =
				mt9t113_get_pict_max_exp_lc();

			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_SET_FPS:
		case CFG_SET_PICT_FPS:
			rc = mt9t113_set_fps(&(cdata.cfg.fps));
			break;
		case CFG_SET_EXP_GAIN:
			rc = mt9t113_write_exp_gain(cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;
		case CFG_SET_PICT_EXP_GAIN:
			rc = mt9t113_set_pict_exp_gain(cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;
#endif
		case CFG_SET_MODE:
			rc = mt9t113_set_sensor_mode(cdata.mode);
			break;
		case CFG_PWR_DOWN:
			rc = mt9t113_power_down();
			break;
#if 0
		case CFG_MOVE_FOCUS:
			rc = mt9t113_move_focus(cdata.cfg.focus.dir,
					cdata.cfg.focus.steps);
			break;
		case CFG_SET_DEFAULT_FOCUS:
			rc = mt9t113_set_default_focus(cdata.cfg.focus.steps);
			break;
		case CFG_GET_AF_MAX_STEPS:
			cdata.max_steps = S5K4E1_TOTAL_STEPS_NEAR_TO_FAR;
			if (copy_to_user((void *)argp,
						&cdata,
						sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
#endif
		case CFG_SET_EFFECT:
			rc = mt9t113_set_default_focus(cdata.cfg.effect);
			break;
		default:
			rc = -EFAULT;
			break;
	}
	mutex_unlock(&mt9t113_mutex);
	return rc;
}

static void mt9t113_flash_init(void)
{
	int rc;
	rc = gpio_request(g_sensor_info->flash_data->flash_src->_fsrc.current_driver_src.led1, "mt9t113_flash_en");
	if (rc < 0) {
		PRINTK1("GPIO request error\n");
	}
	gpio_direction_output(g_sensor_info->flash_data->flash_src->_fsrc.current_driver_src.led1, 0);

	rc = gpio_request(g_sensor_info->flash_data->flash_src->_fsrc.current_driver_src.led2, "mt9t113_flash_sel");
	if (rc < 0) {
		PRINTK1("GPIO request error\n");
	}
	gpio_direction_output(g_sensor_info->flash_data->flash_src->_fsrc.current_driver_src.led2, 0);
}

static int mt9t113_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mt9t113_wait_queue);
	return 0;
}

static int mt9t113_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_fail;
	}

	mt9t113_sensorw = kzalloc(sizeof(struct mt9t113_work), GFP_KERNEL);
	if (!mt9t113_sensorw) {
		rc = -ENOMEM;
		goto probe_fail;
	}

	i2c_set_clientdata(client, mt9t113_sensorw);
	mt9t113_init_client(client);
	mt9t113_client = client;

	PRINTK1("succeeded!\n");
	return rc;

probe_fail:
	kfree(mt9t113_sensorw);
	mt9t113_sensorw = NULL;
	PRINTK1("fail\n");
	return rc;
}

static int __exit mt9t113_i2c_remove(struct i2c_client *client)
{
        struct  mt9t113_work *sensorw = i2c_get_clientdata(client);
        mt9t113_client = NULL;
        kfree(sensorw);
        return 0;
}

static const struct i2c_device_id mt9t113_i2c_id[] = {
	{ "mt9t113", 0},
};

static struct i2c_driver mt9t113_i2c_driver = {
	.id_table = mt9t113_i2c_id,
	.probe  = mt9t113_i2c_probe,
	.remove = __exit_p(mt9t113_i2c_remove),
	.driver = {
		.name = "mt9t113",
	},
};

static int mt9t113_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = i2c_add_driver(&mt9t113_i2c_driver);
	if (rc < 0 || mt9t113_client == NULL) {
		rc = -ENOTSUPP;
		PRINTK1("I2C add driver failed");
		goto probe_fail;
	}

	rc = gpio_request(info->sensor_pwd, "mt9t113_pwd");
	if (rc < 0) {
		goto probe_fail;
	}
	gpio_direction_output(info->sensor_pwd, 0);

	rc = gpio_request(info->sensor_reset, "mt9t113_reset");
	if (rc < 0) {
		goto probe_fail;
	}
	gpio_direction_output(info->sensor_reset, 0);

	g_sensor_info = info;

	msm_camio_clk_rate_set(MT9T113_MASTER_CLK_RATE);
	msleep(30);

	mt9t113_flash_init();
#if 0
	rc = mt9t113_sensor_init_probe(info);
	if (rc < 0) {
		goto probe_fail;
	}
#endif
	s->s_init = mt9t113_sensor_open_init;
	s->s_release = mt9t113_sensor_release;
	s->s_config  = mt9t113_sensor_config;
	s->s_mount_angle = 90;
	mt9t113_power_up();
	msleep(20);
	return rc;

probe_fail:
	PRINTK1("probe failed\n");
	i2c_del_driver(&mt9t113_i2c_driver);
	mt9t113_gpio_free(info);
	return rc;
}

static int __devinit mt9t113_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9t113_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = mt9t113_probe,
	.driver = {
		.name = "msm_camera_mt9t113",
		.owner = THIS_MODULE,
	},
};

static int __init mt9t113_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9t113_init);
MODULE_DESCRIPTION("3.1 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
