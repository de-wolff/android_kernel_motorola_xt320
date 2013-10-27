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

#ifndef MT9T013_H
#define MT9T013_H

#include <mach/camera.h>
#include <linux/types.h>

//extern struct mt9t013_reg mt9t013_regs; /* from mt9t013_reg.c */

enum mt9t113_width {
        WORD_LEN,
        BYTE_LEN
};

struct mt9t113_i2c_reg_conf {
        unsigned short waddr;
        unsigned short wdata;
        enum mt9t113_width width;
        unsigned short mdelay_time;
};

enum mt9t113_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	INVALID_SIZE
};

enum mt9t113_setting {
	RES_PREVIEW,
	RES_CAPTURE
};

enum mt9t113_reg_update {
	/* Sensor registers that need to be updated during initialization */
	REG_INIT,
	/* Sensor registers that needs periodic I2C writes */
	UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	UPDATE_ALL,
	/* Not valid update */
	UPDATE_INVALID
};
#if 1
struct mt9t113_reg {
        const struct mt9t113_i2c_reg_conf const *config1;
        uint16_t config1_size;
        const struct mt9t113_i2c_reg_conf const *config2;
        uint16_t config2_size;
        const struct mt9t113_i2c_reg_conf const *config3;
        uint16_t config3_size;
        const struct mt9t113_i2c_reg_conf const *config4;
        uint16_t config4_size;
#if 0
        const struct mt9t113_i2c_reg_conf const *config5;
        uint16_t config5_size;
#endif
        const struct mt9t113_i2c_reg_conf const *preview;
        uint16_t preview_size;
        const struct mt9t113_i2c_reg_conf const *capture;
        uint16_t capture_size;
};
#endif
#if 0
// for new config
struct mt9t113_reg {
        const struct mt9t113_i2c_reg_conf const *config1;
        uint16_t config1_size;
        const struct mt9t113_i2c_reg_conf const *config2;
        uint16_t config2_size;
        const struct mt9t113_i2c_reg_conf const *config3;
        uint16_t config3_size;
        const struct mt9t113_i2c_reg_conf const *config4;
        uint16_t config4_size;
        const struct mt9t113_i2c_reg_conf const *config5;
        uint16_t config5_size;
        const struct mt9t113_i2c_reg_conf const *config6;
        uint16_t config6_size;
        const struct mt9t113_i2c_reg_conf const *config7;
        uint16_t config7_size;
        const struct mt9t113_i2c_reg_conf const *preview;
        uint16_t preview_size;
        const struct mt9t113_i2c_reg_conf const *capture;
        uint16_t capture_size;
};
#endif
/************* Below no used ***************/
struct reg_struct {
	uint16_t vt_pix_clk_div;        /*  0x0300 */
	uint16_t vt_sys_clk_div;        /*  0x0302 */
	uint16_t pre_pll_clk_div;       /*  0x0304 */
	uint16_t pll_multiplier;        /*  0x0306 */
	uint16_t op_pix_clk_div;        /*  0x0308 */
	uint16_t op_sys_clk_div;        /*  0x030A */
	uint16_t scale_m;               /*  0x0404 */
	uint16_t row_speed;             /*  0x3016 */
	uint16_t x_addr_start;          /*  0x3004 */
	uint16_t x_addr_end;            /*  0x3008 */
	uint16_t y_addr_start;        	/*  0x3002 */
	uint16_t y_addr_end;            /*  0x3006 */
	uint16_t read_mode;             /*  0x3040 */
	uint16_t x_output_size;         /*  0x034C */
	uint16_t y_output_size;         /*  0x034E */
	uint16_t line_length_pck;       /*  0x300C */
	uint16_t frame_length_lines;	/*  0x300A */
	uint16_t coarse_int_time; 		/*  0x3012 */
	uint16_t fine_int_time;   		/*  0x3014 */
};

#if 0
struct mt9t113_reg {
        const struct register_address_value_pair *prev_snap_reg_settings;
        uint16_t prev_snap_reg_settings_size;
        const struct register_address_value_pair *noise_reduction_reg_settings;
        uint16_t noise_reduction_reg_settings_size;
        const struct register_address_value_pair *pga_reg_settings;
        uint16_t pga_reg_settings_size;
        const struct mt9t113_i2c_reg_conf *plltbl;
        uint16_t plltbl_size;
        const struct mt9t113_i2c_reg_conf *stbl;
        uint16_t stbl_size;
        const struct mt9t113_i2c_reg_conf *rftbl;
        uint16_t rftbl_size;
};

struct mt9t013_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
};

struct mt9t013_reg {
	struct reg_struct const *reg_pat;
	uint16_t reg_pat_size;
	struct mt9t013_i2c_reg_conf const *ttbl;
	uint16_t ttbl_size;
	struct mt9t013_i2c_reg_conf const *lctbl;
	uint16_t lctbl_size;
	struct mt9t013_i2c_reg_conf const *rftbl;
	uint16_t rftbl_size;
};
#endif

#endif /* #define MT9T013_H */
