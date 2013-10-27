/*  Date: 2011/1/28 12:00:00
 *  Revision: 2.0
 */

/*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */


/* file BMA250.c
   brief This file contains all function implementations for the BMA250 in linux

*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <asm/uaccess.h>
#include <linux/device.h>	/* for struct device */
#include <linux/delay.h>

#define BMA250_DEBUG 0
#define SENSOR_NAME 			"bma250"
#define GRAVITY_EARTH           9806550
#define ABSMIN_2G               (-GRAVITY_EARTH * 2)
#define ABSMAX_2G               (GRAVITY_EARTH * 2)
#define BMA250_MAX_DELAY		200
#define BMA250_CHIP_ID			3

#if BMA250_DEBUG
#define BMADBG(format, ...)	\
		printk(KERN_INFO "BMA250 " format "\n", ## __VA_ARGS__)
#else
#define BMADBG(format, ...)
#endif

//add by otis, for debug
#define BMA_DEBUG 1
#if  BMA_DEBUG
#define	BMA_PR(fmt, arg...) printk(KERN_INFO fmt, ##arg)
#else
#define	BMA_PR(fmt, arg...)
#endif



// const value
#define         C_Null_U8X                              (unsigned char)0
#define         C_Zero_U8X                              (unsigned char)0
#define         C_One_U8X                               (unsigned char)1
#define         C_Two_U8X                               (unsigned char)2
#define         C_Three_U8X                             (unsigned char)3
#define         C_Four_U8X                              (unsigned char)4
#define         C_Five_U8X                              (unsigned char)5
#define         C_Six_U8X                               (unsigned char)6
#define         C_Seven_U8X                             (unsigned char)7
#define         C_Eight_U8X                             (unsigned char)8
#define         C_Nine_U8X                              (unsigned char)9
#define         C_Ten_U8X                               (unsigned char)10
#define         C_Eleven_U8X                            (unsigned char)11
#define         C_Twelve_U8X                            (unsigned char)12
#define         C_Sixteen_U8X                           (unsigned char)16
#define         C_TwentyFour_U8X                        (unsigned char)24
#define         C_ThirtyTwo_U8X                         (unsigned char)32
#define         C_Hundred_U8X                           (unsigned char)100
#define         C_OneTwentySeven_U8X                    (unsigned char)127
#define         C_TwoFiftyFive_U8X                      (unsigned char)255
#define         C_TwoFiftySix_U16X                      (unsigned short)256


#define BMA250_RANGE_SET		0x02	// 00b:2g
										// 01b:4g
										// 10b:8g
										// 11b:16g
/*set 31.25HZ to decrease the shaking rang for google sky map*/
#define BMA250_BW_SET			0x0a	// 00xxxb: 7.81Hz,	01000b: 7.81Hz
										// 01001b: 15.63Hz,	01010b: 31.25hz
										// 01011b: 62.5Hz,	01100b: 125Hz
										// 01101b: 250Hz,	01110b: 500Hz
										// 01111b: 1000Hz,	1xxxxb: 1000Hz

/* range and bandwidth */

#define BMA250_RANGE_2G			0x0
#define BMA250_RANGE_4G			0x1
#define BMA250_RANGE_8G			0x2
#define BMA250_RANGE_16G		0x3

#define BMA250_BW_7_81HZ        0x08       /**< sets bandwidth to LowPass 7.81  HZ \see bma250_set_bandwidth() */
#define BMA250_BW_15_63HZ       0x09       /**< sets bandwidth to LowPass 15.63 HZ \see bma250_set_bandwidth() */
#define BMA250_BW_31_25HZ       0x0A       /**< sets bandwidth to LowPass 31.25 HZ \see bma250_set_bandwidth() */
#define BMA250_BW_62_50HZ       0x0B       /**< sets bandwidth to LowPass 62.50 HZ \see bma250_set_bandwidth() */
#define BMA250_BW_125HZ         0x0C       /**< sets bandwidth to LowPass 125HZ \see bma250_set_bandwidth() */
#define BMA250_BW_250HZ         0x0D       /**< sets bandwidth to LowPass 250HZ \see bma250_set_bandwidth() */
#define BMA250_BW_500HZ         0x0E       /**< sets bandwidth to LowPass 500HZ \see bma250_set_bandwidth() */
#define BMA250_BW_1000HZ        0x0F       /**< sets bandwidth to LowPass 1000HZ \see bma250_set_bandwidth() */


// register list
#define BMA250_CHIP_ID_REG                      0x00
#define BMA250_VERSION_REG                      0x01
#define BMA250_X_AXIS_LSB_REG                   0x02
#define BMA250_X_AXIS_MSB_REG                   0x03
#define BMA250_Y_AXIS_LSB_REG                   0x04
#define BMA250_Y_AXIS_MSB_REG                   0x05
#define BMA250_Z_AXIS_LSB_REG                   0x06
#define BMA250_Z_AXIS_MSB_REG                   0x07
#define BMA250_TEMP_RD_REG                      0x08
#define BMA250_STATUS1_REG                      0x09
#define BMA250_STATUS2_REG                      0x0A
#define BMA250_STATUS_TAP_SLOPE_REG             0x0B
#define BMA250_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA250_RANGE_SEL_REG                    0x0F
#define BMA250_BW_SEL_REG                       0x10
#define BMA250_MODE_CTRL_REG                    0x11
#define BMA250_LOW_NOISE_CTRL_REG               0x12
#define BMA250_DATA_CTRL_REG                    0x13
#define BMA250_RESET_REG                        0x14
#define BMA250_INT_ENABLE1_REG                  0x16
#define BMA250_INT_ENABLE2_REG                  0x17
#define BMA250_INT1_PAD_SEL_REG                 0x19
#define BMA250_INT_DATA_SEL_REG                 0x1A
#define BMA250_INT2_PAD_SEL_REG                 0x1B
#define BMA250_INT_SRC_REG                      0x1E
#define BMA250_INT_SET_REG                      0x20
#define BMA250_INT_CTRL_REG                     0x21
#define BMA250_LOW_DURN_REG                     0x22
#define BMA250_LOW_THRES_REG                    0x23
#define BMA250_LOW_HIGH_HYST_REG                0x24
#define BMA250_HIGH_DURN_REG                    0x25
#define BMA250_HIGH_THRES_REG                   0x26
#define BMA250_SLOPE_DURN_REG                   0x27
#define BMA250_SLOPE_THRES_REG                  0x28
#define BMA250_TAP_PARAM_REG                    0x2A
#define BMA250_TAP_THRES_REG                    0x2B
#define BMA250_ORIENT_PARAM_REG                 0x2C
#define BMA250_THETA_BLOCK_REG                  0x2D
#define BMA250_THETA_FLAT_REG                   0x2E
#define BMA250_FLAT_HOLD_TIME_REG               0x2F
#define BMA250_STATUS_LOW_POWER_REG             0x31
#define BMA250_SELF_TEST_REG                    0x32
#define BMA250_EEPROM_CTRL_REG                  0x33
#define BMA250_SERIAL_CTRL_REG                  0x34
#define BMA250_CTRL_UNLOCK_REG                  0x35
#define BMA250_OFFSET_CTRL_REG                  0x36
#define BMA250_OFFSET_PARAMS_REG                0x37
#define BMA250_OFFSET_FILT_X_REG                0x38
#define BMA250_OFFSET_FILT_Y_REG                0x39
#define BMA250_OFFSET_FILT_Z_REG                0x3A
#define BMA250_OFFSET_UNFILT_X_REG              0x3B
#define BMA250_OFFSET_UNFILT_Y_REG              0x3C
#define BMA250_OFFSET_UNFILT_Z_REG              0x3D
#define BMA250_SPARE_0_REG                      0x3E
#define BMA250_SPARE_1_REG                      0x3F

// Control MACRO
#define BMA250_GET_BITSLICE(regvar, bitname)\
                        (regvar & bitname##__MSK) >> bitname##__POS


#define BMA250_SET_BITSLICE(regvar, bitname, val)\
                  (regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK)



/* DATA REGISTERS */

#define BMA250_CHIP_ID__POS             0
#define BMA250_CHIP_ID__MSK             0xFF
#define BMA250_CHIP_ID__LEN             8
#define BMA250_CHIP_ID__REG             BMA250_CHIP_ID_REG

#define BMA250_NEW_DATA_X__POS          0
#define BMA250_NEW_DATA_X__LEN          1
#define BMA250_NEW_DATA_X__MSK          0x01
#define BMA250_NEW_DATA_X__REG          BMA250_X_AXIS_LSB_REG

#define BMA250_ACC_X_LSB__POS           6
#define BMA250_ACC_X_LSB__LEN           2
#define BMA250_ACC_X_LSB__MSK           0xC0
#define BMA250_ACC_X_LSB__REG           BMA250_X_AXIS_LSB_REG

#define BMA250_ACC_X_MSB__POS           0
#define BMA250_ACC_X_MSB__LEN           8
#define BMA250_ACC_X_MSB__MSK           0xFF
#define BMA250_ACC_X_MSB__REG           BMA250_X_AXIS_MSB_REG

#define BMA250_NEW_DATA_Y__POS          0
#define BMA250_NEW_DATA_Y__LEN          1
#define BMA250_NEW_DATA_Y__MSK          0x01
#define BMA250_NEW_DATA_Y__REG          BMA250_Y_AXIS_LSB_REG

#define BMA250_ACC_Y_LSB__POS           6
#define BMA250_ACC_Y_LSB__LEN           2
#define BMA250_ACC_Y_LSB__MSK           0xC0
#define BMA250_ACC_Y_LSB__REG           BMA250_Y_AXIS_LSB_REG

#define BMA250_ACC_Y_MSB__POS           0
#define BMA250_ACC_Y_MSB__LEN           8
#define BMA250_ACC_Y_MSB__MSK           0xFF
#define BMA250_ACC_Y_MSB__REG           BMA250_Y_AXIS_MSB_REG

#define BMA250_NEW_DATA_Z__POS          0
#define BMA250_NEW_DATA_Z__LEN          1
#define BMA250_NEW_DATA_Z__MSK          0x01
#define BMA250_NEW_DATA_Z__REG          BMA250_Z_AXIS_LSB_REG

#define BMA250_ACC_Z_LSB__POS           6
#define BMA250_ACC_Z_LSB__LEN           2
#define BMA250_ACC_Z_LSB__MSK           0xC0
#define BMA250_ACC_Z_LSB__REG           BMA250_Z_AXIS_LSB_REG

#define BMA250_ACC_Z_MSB__POS           0
#define BMA250_ACC_Z_MSB__LEN           8
#define BMA250_ACC_Z_MSB__MSK           0xFF
#define BMA250_ACC_Z_MSB__REG           BMA250_Z_AXIS_MSB_REG


/* CONTROL BITS */
#define BMA250_EN_LOW_POWER__POS          6
#define BMA250_EN_LOW_POWER__LEN          1
#define BMA250_EN_LOW_POWER__MSK          0x40
#define BMA250_EN_LOW_POWER__REG          BMA250_MODE_CTRL_REG

/* RANGE */
#define BMA250_RANGE_SEL__POS             0
#define BMA250_RANGE_SEL__LEN             4
#define BMA250_RANGE_SEL__MSK             0x0F
#define BMA250_RANGE_SEL__REG             BMA250_RANGE_SEL_REG

/* BANDWIDTH dependend definitions */
#define BMA250_BANDWIDTH__POS             0
#define BMA250_BANDWIDTH__LEN             5
#define BMA250_BANDWIDTH__MSK             0x1F
#define BMA250_BANDWIDTH__REG             BMA250_BW_SEL_REG

/* WAKE UP */
#define BMA250_EN_SUSPEND__POS            7
#define BMA250_EN_SUSPEND__LEN            1
#define BMA250_EN_SUSPEND__MSK            0x80
#define BMA250_EN_SUSPEND__REG            BMA250_MODE_CTRL_REG

/**    FAST COMPENSATION READY FLAG          **/
#define BMA250_FAST_COMP_RDY_S__POS             4
#define BMA250_FAST_COMP_RDY_S__LEN             1
#define BMA250_FAST_COMP_RDY_S__MSK             0x10
#define BMA250_FAST_COMP_RDY_S__REG             BMA250_OFFSET_CTRL_REG

/**    FAST COMPENSATION FOR X,Y,Z AXIS      **/
#define BMA250_EN_FAST_COMP__POS                5
#define BMA250_EN_FAST_COMP__LEN                2
#define BMA250_EN_FAST_COMP__MSK                0x60
#define BMA250_EN_FAST_COMP__REG                BMA250_OFFSET_CTRL_REG

/**     COMPENSATION TARGET                  **/
#define BMA250_COMP_TARGET_OFFSET_X__POS        1
#define BMA250_COMP_TARGET_OFFSET_X__LEN        2
#define BMA250_COMP_TARGET_OFFSET_X__MSK        0x06
#define BMA250_COMP_TARGET_OFFSET_X__REG        BMA250_OFFSET_PARAMS_REG

#define BMA250_COMP_TARGET_OFFSET_Y__POS        3
#define BMA250_COMP_TARGET_OFFSET_Y__LEN        2
#define BMA250_COMP_TARGET_OFFSET_Y__MSK        0x18
#define BMA250_COMP_TARGET_OFFSET_Y__REG        BMA250_OFFSET_PARAMS_REG

#define BMA250_COMP_TARGET_OFFSET_Z__POS        5
#define BMA250_COMP_TARGET_OFFSET_Z__LEN        2
#define BMA250_COMP_TARGET_OFFSET_Z__MSK        0x60
#define BMA250_COMP_TARGET_OFFSET_Z__REG        BMA250_OFFSET_PARAMS_REG

/* mode settings */
#define BMA250_MODE_NORMAL      0
#define BMA250_MODE_LOWPOWER    1
#define BMA250_MODE_SUSPEND     2

#define BMA250_IOC_MAGIC 'b'
#define BMA250_READ_ACCEL_XYZ		_IOWR(BMA250_IOC_MAGIC, 22, short)
#define BMA250_READ_INSTALL_DIR	_IOWR(BMA250_IOC_MAGIC, 24, int[1])
#define BMA250_GET_CALIBRATION        _IOWR(BMA250_IOC_MAGIC, 25, short)
#define BMA250_SET_CALIBRATION        _IOWR(BMA250_IOC_MAGIC, 26, short)

#define POLL_INTERVAL		100

struct bma250acc{
	short x;
	short y;
	short z;
} ;

struct bma250_data {
	struct i2c_client *client;
	unsigned char mode;
	struct bma250acc value;
	struct device dev;
};

static struct bma250_data *bma250_data = NULL;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif
#if BMA250_DEBUG
static struct class *bma250_class;
#endif
static bool isSleep = false;
static int bma250_chip_id;
static unsigned char bma250_range;
//static DEFINE_MUTEX(bma250_mutex);//clf delete

static int bma250_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0){
		BMA_PR("bma250_smbus_read_byte error NUM[%d]", dummy);
		return -1;
	}
	*data = dummy & 0x000000ff;

	return 0;
}

static int bma250_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0){
		BMA_PR("bma250_smbus_write_byte error NUM[%d]", dummy);
		return -1;
	}
	return 0;
}

static int bma250_smbus_read_byte_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	s32 dummy;
	dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
	if (dummy < 0){
		BMA_PR("bma250_smbus_read_byte_block error NUM[%d]", dummy);
		return -1;
	}
	return 0;
}

static int bma250_read_accel(struct bma250acc *acc)
{
	int comres;
	unsigned char data[6] = {0};
	
	if (bma250_data->client == NULL) {
		comres = -1;
	} else {
	      comres = bma250_smbus_read_byte_block(bma250_data->client, BMA250_ACC_X_LSB__REG, &data[0], 6);

		acc->x = BMA250_GET_BITSLICE(data[0],BMA250_ACC_X_LSB) | (BMA250_GET_BITSLICE(data[1],BMA250_ACC_X_MSB)<<BMA250_ACC_X_LSB__LEN);
		acc->x = acc->x << (sizeof(short)*8-(BMA250_ACC_X_LSB__LEN + BMA250_ACC_X_MSB__LEN));
		acc->x = acc->x >> (sizeof(short)*8-(BMA250_ACC_X_LSB__LEN + BMA250_ACC_X_MSB__LEN));

		acc->y = BMA250_GET_BITSLICE(data[2],BMA250_ACC_Y_LSB) | (BMA250_GET_BITSLICE(data[3],BMA250_ACC_Y_MSB)<<BMA250_ACC_Y_LSB__LEN);
		acc->y = acc->y << (sizeof(short)*8-(BMA250_ACC_Y_LSB__LEN + BMA250_ACC_Y_MSB__LEN));
		acc->y = acc->y >> (sizeof(short)*8-(BMA250_ACC_Y_LSB__LEN + BMA250_ACC_Y_MSB__LEN));

		acc->z = BMA250_GET_BITSLICE(data[4],BMA250_ACC_Z_LSB) | (BMA250_GET_BITSLICE(data[5],BMA250_ACC_Z_MSB)<<BMA250_ACC_Z_LSB__LEN);
		acc->z = acc->z << (sizeof(short)*8-(BMA250_ACC_Z_LSB__LEN + BMA250_ACC_Z_MSB__LEN));
		acc->z = acc->z >> (sizeof(short)*8-(BMA250_ACC_Z_LSB__LEN + BMA250_ACC_Z_MSB__LEN));

		bma250_data->value = *acc;
	}
	
	return comres;
}

int bma250_get_cal_ready(unsigned char *calrdy )
{
	int comres = 0 ;
	unsigned char data;

	if (bma250_data->client == NULL) {
		comres = -1;
      } else {
		comres = bma250_smbus_read_byte(bma250_data->client,  BMA250_OFFSET_CTRL_REG, &data);
		if (comres < 0) return comres;
		data = BMA250_GET_BITSLICE(data, BMA250_FAST_COMP_RDY_S);
		*calrdy = data;
      }

	return comres;
}

int bma250_set_cal_trigger(unsigned char caltrigger)
{
	int comres = 0;
	unsigned char data;

	if (bma250_data->client == NULL) {
		comres = -1;
      } else {
		comres = bma250_smbus_read_byte(bma250_data->client, BMA250_EN_FAST_COMP__REG, &data);
		if (comres < 0) return comres;
		data = BMA250_SET_BITSLICE(data, BMA250_EN_FAST_COMP, caltrigger );
		comres = bma250_smbus_write_byte(bma250_data->client, BMA250_EN_FAST_COMP__REG, &data);
		if (comres < 0) return comres;
	}

	return comres;
}

int bma250_set_offset_target_x(unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte(bma250_data->client, BMA250_COMP_TARGET_OFFSET_X__REG, &data);
		if (comres < 0) return comres;
		data = BMA250_SET_BITSLICE(data, BMA250_COMP_TARGET_OFFSET_X, offsettarget );
		comres = bma250_smbus_write_byte(bma250_data->client, BMA250_COMP_TARGET_OFFSET_X__REG, &data);
		if (comres < 0) return comres;
	}

	return comres;
}

int bma250_set_offset_target_y(unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte(bma250_data->client, BMA250_COMP_TARGET_OFFSET_Y__REG, &data);
		if (comres < 0) return comres;
		data = BMA250_SET_BITSLICE(data, BMA250_COMP_TARGET_OFFSET_Y, offsettarget );
		comres = bma250_smbus_write_byte(bma250_data->client, BMA250_COMP_TARGET_OFFSET_Y__REG, &data);
		if (comres < 0) return comres;
      }

	return comres;
}

int bma250_set_offset_target_z(unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte(bma250_data->client, BMA250_COMP_TARGET_OFFSET_Z__REG, &data);
		if (comres < 0) return comres;
		data = BMA250_SET_BITSLICE(data, BMA250_COMP_TARGET_OFFSET_Z, offsettarget );
		comres = bma250_smbus_write_byte(bma250_data->client, BMA250_COMP_TARGET_OFFSET_Z__REG, &data);
		if (comres < 0) return comres;
      }

	return comres;
}

static int bma250_calibrate(unsigned char data[])
{
	int ret = 0;
	unsigned char ch = 0;
	unsigned char *tmp = data;

      //calibrate x axis
	if (bma250_set_offset_target_x(0) == -1)
		return -EINVAL;
	if (bma250_set_cal_trigger(1) == -1){
		return -EINVAL;
	}
	while (ch != 1){
		if (bma250_get_cal_ready(&ch) == -1){
			printk("bma250_get_cal_ready x error");
			return -EINVAL;
		}
		printk("bma250_get_cal_ready x ch = %d\n", ch);
		mdelay(10);
	}
	ret = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_X_REG, &tmp[0]);
	if (ret < 0) return ret;
	ret = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_X_REG, &tmp[1]);
	if (ret < 0) return ret;

      //calibrate y axis
	ch = 0;
	if (bma250_set_offset_target_y(0) == -1)
		return -EINVAL;
	if (bma250_set_cal_trigger(2) == -1){
		return -EINVAL;
	}
	while (ch != 1){
		if (bma250_get_cal_ready(&ch) == -1){
			printk("bma250_get_cal_ready y error");
			return -EINVAL;
		}
		printk("bma250_get_cal_ready y ch = %d\n", ch);
		mdelay(10);
	}
	ret = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_Y_REG, &tmp[2]);
	if (ret < 0) return ret;
	ret = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Y_REG, &tmp[3]);
	if (ret < 0) return ret;

      //calibrate z axis
	ch = 0;
	if (bma250_set_offset_target_z(2) == -1)
		return -EINVAL;
	if (bma250_set_cal_trigger(3) == -1){
		return -EINVAL;
	}
	while (ch != 1){
		if (bma250_get_cal_ready(&ch) == -1){
			printk("bma250_get_cal_ready z error");
			return -EINVAL;
		}
		printk("bma250_get_cal_ready z ch = %d\n", ch);
		mdelay(10);
	}
	ret = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_Z_REG, &tmp[4]);
	if (ret < 0) return ret;
	ret = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Z_REG, &tmp[5]);
	if (ret < 0) return ret;
	BMADBG("data = %d %d %d %d %d %d\n", data[0], data[1], data[2], data[3], data[4], data[5]);

	return ret;
}

static int bma250_restore_calibration_data(unsigned char data[])
{
	int ret = 0;
	unsigned char *tmp = data;

	ret = bma250_smbus_write_byte(bma250_data->client, BMA250_OFFSET_FILT_X_REG, &tmp[0]);
	if (ret < 0) return ret;
	ret = bma250_smbus_write_byte(bma250_data->client, BMA250_OFFSET_UNFILT_X_REG, &tmp[1]);
	if (ret < 0) return ret;
	ret = bma250_smbus_write_byte(bma250_data->client, BMA250_OFFSET_FILT_Y_REG, &tmp[2]);
	if (ret < 0) return ret;
	ret = bma250_smbus_write_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Y_REG, &tmp[3]);
	if (ret < 0) return ret;
	ret = bma250_smbus_write_byte(bma250_data->client, BMA250_OFFSET_FILT_Y_REG, &tmp[4]);
	if (ret < 0) return ret;
	ret = bma250_smbus_write_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Y_REG, &tmp[5]);
	if (ret < 0) return ret;

	return ret;
}

static int bma250_open(struct inode *client, struct file *file)
{
	BMADBG("enter bma250_ open\n");
	return 0;
}

static int bma250_release(struct inode *inode, struct file *file)
{
	BMADBG("enter bma250_release\n");
	return 0;
}

static long bma250_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *pa = (void __user *)arg;
	struct bma250acc acc;
	short vec[3] = {0};
	unsigned char calibration_data[6] = {0};
	int result=0;
	int install_dir=7;

	//mutex_lock(&bma250_mutex);//clf delete
	switch (cmd) {
		case BMA250_READ_INSTALL_DIR:
			#if defined(CONFIG_CELLON_PRJ_C8092)
				install_dir = 0;
			#else
				install_dir = 7;
			#endif
			if (copy_to_user(pa, &install_dir, sizeof(install_dir))) {
				return -EFAULT;
			}
			break;

		case BMA250_READ_ACCEL_XYZ:
			result=0;
			if (isSleep)
			{
				printk("bma250_ioctl: BMA250_READ_ACCEL_XYZ  --return--\n");
				return 0;
			}
			
			if (bma250_read_accel(&acc) != 0) {
				printk("bma250 data read failed\n");
				return -EFAULT;
			}
			
			vec[0] = acc.x;
			vec[1] = acc.y;
			vec[2] = acc.z;
			BMADBG("---bma250:[X - %d] [Y - %d] [Z - %d]---\n", vec[0], vec[1], vec[2]);
			if (copy_to_user(pa, vec, sizeof(vec))) {
				return -EFAULT;
			}
			break;
        case BMA250_GET_CALIBRATION:
            result = bma250_calibrate(calibration_data);
            if (result < 0) return result;
            if (copy_to_user(pa, calibration_data, sizeof(calibration_data))) {
                return -EFAULT;
            }
            break;

        case BMA250_SET_CALIBRATION:
            if (copy_from_user(calibration_data, pa, sizeof(calibration_data)))
                return -EFAULT;
            result = bma250_restore_calibration_data(calibration_data);
            if (result < 0) return result;
            break;
	
		default:
			break;
	}
	//mutex_unlock(&bma250_mutex);//clf delete
	return 0;
}

static struct file_operations bma250_fops = {
	.owner = THIS_MODULE,
	.open = bma250_open,
	.release = bma250_release,
	.unlocked_ioctl = bma250_ioctl,
};

static struct miscdevice bma250_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "bma250",
	.fops = &bma250_fops,
};
//#define CONFIG_PM//clf debug
#if BMA250_DEBUG||defined(CONFIG_HAS_EARLYSUSPEND)||defined(CONFIG_PM)
static int bma250_set_mode(unsigned char Mode)
{
	int comres = 0;
	unsigned char data = 0;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else
	{
		if (Mode < 3){
			comres = bma250_smbus_read_byte(bma250_data->client, BMA250_EN_LOW_POWER__REG, &data);
			switch (Mode)
			{
				case BMA250_MODE_NORMAL:
					data  = BMA250_SET_BITSLICE(data, BMA250_EN_LOW_POWER, C_Zero_U8X);
					data  = BMA250_SET_BITSLICE(data, BMA250_EN_SUSPEND, C_Zero_U8X);
					break;
				case BMA250_MODE_LOWPOWER:
					data  = BMA250_SET_BITSLICE(data, BMA250_EN_LOW_POWER, C_One_U8X);
					data  = BMA250_SET_BITSLICE(data, BMA250_EN_SUSPEND, C_Zero_U8X);
					break;
				case BMA250_MODE_SUSPEND:
					data  = BMA250_SET_BITSLICE(data, BMA250_EN_LOW_POWER, C_Zero_U8X);
					data  = BMA250_SET_BITSLICE(data, BMA250_EN_SUSPEND, C_One_U8X);
					break;
				default:
					break;
			}
			comres += bma250_smbus_write_byte(bma250_data->client, BMA250_EN_LOW_POWER__REG, &data);            
			bma250_data->mode = (unsigned char) Mode;
		}
		else
		{
			comres = -1;
		}
	}

	return comres;
}
#endif

static int bma250_set_range(unsigned char Range)
{
	int comres = 0;
	unsigned char data = 0;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else {
		if (Range <= BMA250_RANGE_16G)
		{
			bma250_range=Range;
			comres = bma250_smbus_read_byte(bma250_data->client, BMA250_RANGE_SEL_REG, &data);
			switch (Range)
			{
				case C_Zero_U8X:
					data  = BMA250_SET_BITSLICE(data, BMA250_RANGE_SEL, C_Three_U8X);	// Zero or Three?  // gaoys.
					break;
				case C_One_U8X:
					data  = BMA250_SET_BITSLICE(data, BMA250_RANGE_SEL, C_Five_U8X);
					break;
				case C_Two_U8X:
					data  = BMA250_SET_BITSLICE(data, BMA250_RANGE_SEL, C_Eight_U8X);
					break;
				case C_Three_U8X:
					data  = BMA250_SET_BITSLICE(data, BMA250_RANGE_SEL, C_Twelve_U8X);
					break;
				default:
					break;
			}
			comres += bma250_smbus_write_byte(bma250_data->client, BMA250_RANGE_SEL_REG, &data);		            
		} else {
			comres = -1;
		}
	}	

	return comres;
}

#if BMA250_DEBUG
static int bma250_get_range(unsigned char *Range)
{
	int comres = 0;
	unsigned char data = 0;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else{
 		comres = bma250_smbus_read_byte(bma250_data->client, BMA250_RANGE_SEL__REG, &data);
		*Range = BMA250_GET_BITSLICE(data, BMA250_RANGE_SEL);
	}

	return comres;
}
#endif

// Please use BMA250_BW_7_81HZ..etc...
static int bma250_set_bandwidth(unsigned char BW)
{
	int comres = 0;
	unsigned char data = 0;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else
	{
		if (BW > 0x07 && BW < 0x10) 
	        {       
			comres = bma250_smbus_read_byte(bma250_data->client, BMA250_BANDWIDTH__REG, &data);
			data = BMA250_SET_BITSLICE(data, BMA250_BANDWIDTH, BW);
 			comres += bma250_smbus_write_byte(bma250_data->client, BMA250_BANDWIDTH__REG, &data);
			BMADBG("---bma250_set_bandwidth---bw=%d\n", BW);

		} else{
			comres = -1;
		}
	}

	return comres;
}

#if BMA250_DEBUG
static int bma250_get_bandwidth(unsigned char *BW)
{
	int comres = 0;
	unsigned char data = 0;

	if (bma250_data->client == NULL) {
		comres = -1;
	} else{
		comres = bma250_smbus_read_byte(bma250_data->client, BMA250_BANDWIDTH__REG, &data);
		*BW = BMA250_GET_BITSLICE(data, BMA250_BANDWIDTH);
	}

	return comres;
}

static ssize_t bma250_mode_show(struct class *class, struct class_attribute *attr, char *buf)
{
	unsigned char data = 0;

	data = bma250_data->mode;

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_mode_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned long data = 0;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_mode((unsigned char) data) < 0)
		return -EINVAL;

	return count;
}
static ssize_t bma250_range_show(struct class *class, struct class_attribute *attr, char *buf)
{
	unsigned char data = 0;

	if (bma250_get_range(&data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_range_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned long data = 0;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_range((unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_bandwidth_show(struct class *class, struct class_attribute *attr, char *buf)
{
	unsigned char data = 0;

	if (bma250_get_bandwidth(&data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_bandwidth_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned long data = 0;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_bandwidth((unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_value_show(struct class *class, struct class_attribute *attr, char *buf)
{
	struct bma250acc acc_value;

	acc_value = bma250_data->value;

	return sprintf(buf, "%d %d %d\n", acc_value.x, acc_value.y,
			acc_value.z);
}

static ssize_t bma250_calibration_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned char data[6]= {0};
	unsigned char ch = 0;
	int comres = 0;

	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_X_REG, &data[0]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_X_REG, &data[1]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_Y_REG, &data[2]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Y_REG, &data[3]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_Z_REG, &data[4]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Z_REG, &data[5]);
	printk("before calibration data = %d %d %d %d %d %d\n", data[0], data[1], data[2], data[3], data[4], data[5]);

	if (bma250_set_offset_target_x(0) == -1)
		return -EINVAL;
	if (bma250_set_cal_trigger(1) == -1){
		return -EINVAL;
	}
	while (ch != 1){
		if (bma250_get_cal_ready(&ch) == -1){
			printk("bma250_get_cal_ready x error");
			return -EINVAL;
		}
		printk("bma250_get_cal_ready x ch = %d\n", ch);
		mdelay(10);
	}
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_X_REG, &data[0]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_X_REG, &data[1]);
	ch = 0;
	if (bma250_set_offset_target_y(0) == -1)
		return -EINVAL;
	if (bma250_set_cal_trigger(2) == -1){
		return -EINVAL;
	}
	while (ch != 1){
		if (bma250_get_cal_ready(&ch) == -1){
			printk("bma250_get_cal_ready y error");
			return -EINVAL;
		}
		printk("bma250_get_cal_ready y ch = %d\n", ch);
		mdelay(10);
	}
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_Y_REG, &data[2]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Y_REG, &data[3]);

	ch = 0;
	if (bma250_set_offset_target_z(2) == -1)
		return -EINVAL;
	if (bma250_set_cal_trigger(3) == -1){
		return -EINVAL;
	}
	while (ch != 1){
		if (bma250_get_cal_ready(&ch) == -1){
			printk("bma250_get_cal_ready z error");
			return -EINVAL;
		}
		printk("bma250_get_cal_ready z ch = %d\n", ch);
		mdelay(10);
	}
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_FILT_Z_REG, &data[4]);
	comres = bma250_smbus_read_byte(bma250_data->client, BMA250_OFFSET_UNFILT_Z_REG, &data[5]);
	BMADBG("data = %d %d %d %d %d %d\n", data[0], data[1], data[2], data[3], data[4], data[5]);

	return count;
}

static CLASS_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_range_show, bma250_range_store);
static CLASS_ATTR(bandwidth, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_bandwidth_show, bma250_bandwidth_store);
static CLASS_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_mode_show, bma250_mode_store);
static CLASS_ATTR(value, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_value_show, NULL);
static CLASS_ATTR(calibration, S_IRUGO|S_IWUSR|S_IWGRP,
		NULL, bma250_calibration_store);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h)
{
	int result = -1;
	result = bma250_set_mode(BMA250_MODE_SUSPEND);
	if (result == 0)
		isSleep = true;
	BMADBG("bma250_early_suspend --isSleep=%d\n", isSleep);
}

static void bma250_late_resume(struct early_suspend *h)
{
	int result = -1;

	result = bma250_set_mode(BMA250_MODE_NORMAL);
	if (result == 0)
		isSleep = false;
	BMADBG("bma250_late_resume --isSleep=%d\n", isSleep);
}
#endif

static int bma250_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int err = 0;
	int tempvalue = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		BMADBG("i2c_check_functionality error\n");
		goto exit;
	}
	
	bma250_data = kzalloc(sizeof(struct bma250_data), GFP_KERNEL);
	if (!bma250_data) {
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, bma250_data);
	bma250_data->client = client;

	//softreset
	i2c_smbus_write_byte_data(bma250_data->client, BMA250_RESET_REG, 0xB6);
	mdelay(10);
	
	tempvalue = i2c_smbus_read_word_data(client, BMA250_CHIP_ID_REG);
	bma250_chip_id = tempvalue&0x00FF;
	if (bma250_chip_id != BMA250_CHIP_ID) {
		printk("bma250 i2c error %d \n", bma250_chip_id);
		err = -1;
		goto kfree_exit;
	}

	bma250_set_bandwidth(BMA250_BW_SET) ; //bandwith : 31.25Hz
	bma250_set_range(BMA250_RANGE_SET);//range: 8g

	printk("BMA250_EN_LOW_POWER__REG=[%x], bandwide [%x], range [%x]\n",
		i2c_smbus_read_byte_data(bma250_data->client, BMA250_EN_LOW_POWER__REG),
		i2c_smbus_read_byte_data(bma250_data->client, BMA250_BANDWIDTH__REG),
		i2c_smbus_read_byte_data(bma250_data->client, BMA250_RANGE_SEL__REG));

	
	#if BMA250_DEBUG
	bma250_class = class_create(THIS_MODULE, "bma250");
	if (IS_ERR(bma250_class)) {
		BMADBG("Unable to create bma250 class; errno = %ld\n", PTR_ERR(bma250_class));
		goto kfree_exit;
	}
	
	if (class_create_file(bma250_class, &class_attr_range) < 0){
		BMADBG("Failed to create range info file");
		goto error_class1;
	}
	if (class_create_file(bma250_class, &class_attr_bandwidth) < 0){
		BMADBG("Failed to create bandwith info file");
		goto error_class2;
	}
	if (class_create_file(bma250_class, &class_attr_mode) < 0){
		BMADBG("Failed to create mode info file");
		goto error_class3;
	}
	if (class_create_file(bma250_class, &class_attr_value) < 0){
		BMADBG("Failed to create value info file");
		goto error_class4;
	}
	if (class_create_file(bma250_class, &class_attr_calibration) < 0){
		BMADBG("Failed to create value info file");
	}
	#endif

	err = misc_register(&bma250_device);
	#if BMA250_DEBUG
	if (err)
		goto error_class5;
	#else
	if (err)
		goto kfree_exit;
	#endif

	#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	early_suspend.suspend = bma250_early_suspend;
	early_suspend.resume = bma250_late_resume;
	register_early_suspend(&early_suspend);
	#endif

	BMADBG("---bma250_probe successful---\n");
	return 0;

#if BMA250_DEBUG
error_class5:
	class_remove_file(bma250_class, &class_attr_value);
error_class4:
	class_remove_file(bma250_class, &class_attr_mode);
error_class3:
	class_remove_file(bma250_class, &class_attr_bandwidth);
error_class2:
	class_remove_file(bma250_class, &class_attr_range);
error_class1:
	class_destroy(bma250_class);
#endif

kfree_exit:
	kfree(bma250_data);
exit:
	return err;
}

bool  bma250_sensor_id(void)
{
        if (bma250_chip_id==BMA250_CHIP_ID){
                return true;
        }
        else{
                return false;
        }
}
EXPORT_SYMBOL(bma250_sensor_id);

static int bma250_remove(struct i2c_client *client)
{
	//sysfs_remove_group(&client->dev.kobj, &bma250_attribute_group);
	#if BMA250_DEBUG
	class_destroy(bma250_class);
	class_remove_file(bma250_class, &class_attr_range);
	class_remove_file(bma250_class, &class_attr_bandwidth);
	class_remove_file(bma250_class, &class_attr_mode);
	class_remove_file(bma250_class, &class_attr_value);
	#endif

	kfree(bma250_data);
	return 0;
}

static const struct i2c_device_id bma250_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma250_id);

static struct i2c_driver bma250_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SENSOR_NAME,
	},
	.id_table	= bma250_id,
	.remove		= bma250_remove,
	.probe	= bma250_probe,
};

static int __init bma250_init(void)
{
	return i2c_add_driver(&bma250_driver);
}

static void __exit bma250_exit(void)
{
	i2c_del_driver(&bma250_driver);
	//class_destroy(bma250_class);
}

MODULE_DESCRIPTION("BMA250 driver");

module_init(bma250_init);
module_exit(bma250_exit);

