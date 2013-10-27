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

#include "mt9v113.h"
#include <linux/kernel.h>

struct mt9v113_i2c_reg_conf const mt9v113_config_first[] = {
	{0x98C, 0x02F0},
	{0x990, 0x0000},
	{0x98C, 0x02F2},
	{0x990, 0x0210},
	{0x98C, 0x02F4},
	{0x990, 0x001A},
	{0x98C, 0x2145}, 		// MCU_ADDRESS [SEQ_ADVSEQ_CALLLIST_5]
	{0x990, 0x02F4}, 		// MCU_DATA_0
	{0x98C, 0xA134}, 		// MCU_ADDRESS [SEQ_ADVSEQ_STACKOPTIONS]
	{0x990, 0x0001}, 		// MCU_DATA_0
					//BITFIELD= 0x31E0 , 2, 0 // core only tags defects. SOC will correct them.
	{0x31E0, 0x0001}, 	      
	{0x001A, 0x0018}, 	// RESET_AND_MISC_CONTROL
	{0x3400, 0x7A28}, 	// MIPI_CONTROL
	{0x321C, 0x0003}, 	// OFIFO_CONTROL_STATUS
	{0x001E, 0x0777}, 					// PAD_SLEW
	{0x0016, 0x42DF}, 				// CLOCKS_CONTROL [1]
	//Load=PLL Setting
	{0x0014, 0x2145}, 	
	{0x0014, 0x2145}, 	
	{0x0010, 0x0523}, 	
	{0x0012, 0x0000}, 	
	{0x0014, 0x244B},
};

struct mt9v113_i2c_reg_conf const mt9v113_config_second[] = {
	{0x0014, 0xB04A},
	//Char settings and fine tuning
	{0x098C, 0xAB1F}, 	// MCU_ADDRESS [HG_LLMODE]
	{0x0990, 0x00C7}, 	// MCU_DATA_0
	{0x098C, 0xAB31}, 	// MCU_ADDRESS [HG_NR_STOP_G]
	{0x0990, 0x001E}, 	// MCU_DATA_0
	{0x098C, 0x274F}, 	// MCU_ADDRESS [MODE_DEC_CTRL_B]
	{0x0990, 0x0004}, 	// MCU_DATA_0
	{0x098C, 0x2741}, 	// MCU_ADDRESS [MODE_DEC_CTRL_A]
	{0x0990, 0x0004}, 	// MCU_DATA_0
	{0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0054}, 	// MCU_DATA_0
	{0x098C, 0xAB21}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH1]
	{0x0990, 0x0046}, 	// MCU_DATA_0
	{0x098C, 0xAB22}, 	// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0002}, 	// MCU_DATA_0
	{0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0005}, 	// MCU_DATA_0
	{0x098C, 0x2B28}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]
	{0x0990, 0x170C}, 	// MCU_DATA_0
	{0x098C, 0x2B2A}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]
	{0x0990, 0x3E80}, 	// MCU_DATA_0
	//AWB AND CCM
	{0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
	{0x0990, 0x0315}, 	// MCU_DATA_0
	{0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
	{0x0990, 0xFDDC}, 	// MCU_DATA_0
	{0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
	{0x0990, 0x003A}, 	// MCU_DATA_0
	{0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
	{0x0990, 0xFF58}, 	// MCU_DATA_0
	{0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
	{0x0990, 0x02B7}, 	// MCU_DATA_0
	{0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
	{0x0990, 0xFF31}, 	// MCU_DATA_0
	{0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
	{0x0990, 0xFF4C}, 	// MCU_DATA_0
	{0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
	{0x0990, 0xFE4C}, 	// MCU_DATA_0
	{0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
	{0x0990, 0x039E}, 	// MCU_DATA_0
	{0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
	{0x0990, 0x001C}, 	// MCU_DATA_0
	{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
	{0x0990, 0x0039}, 	// MCU_DATA_0
	{0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]
	{0x0990, 0x007F}, 	// MCU_DATA_0
	{0x098C, 0x231E}, 	// MCU_ADDRESS [AWB_CCM_RL_1]
	{0x0990, 0xFF77}, 	// MCU_DATA_0
	{0x098C, 0x2320}, 	// MCU_ADDRESS [AWB_CCM_RL_2]
	{0x0990, 0x000A}, 	// MCU_DATA_0
	{0x098C, 0x2322}, 	// MCU_ADDRESS [AWB_CCM_RL_3]
	{0x0990, 0x0020}, 	// MCU_DATA_0
	{0x098C, 0x2324}, 	// MCU_ADDRESS [AWB_CCM_RL_4]
	{0x0990, 0x001B}, 	// MCU_DATA_0
	{0x098C, 0x2326}, 	// MCU_ADDRESS [AWB_CCM_RL_5]
	{0x0990, 0xFFC6}, 	// MCU_DATA_0
	{0x098C, 0x2328}, 	// MCU_ADDRESS [AWB_CCM_RL_6]
	{0x0990, 0x0086}, 	// MCU_DATA_0
	{0x098C, 0x232A}, 	// MCU_ADDRESS [AWB_CCM_RL_7]
	{0x0990, 0x00B5}, 	// MCU_DATA_0
	{0x098C, 0x232C}, 	// MCU_ADDRESS [AWB_CCM_RL_8]
	{0x0990, 0xFEC3}, 	// MCU_DATA_0
	{0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
	{0x0990, 0x0001}, 	// MCU_DATA_0
	{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
	{0x0990, 0xFFEF}, 	// MCU_DATA_0
	{0x098C, 0xA348}, 	// MCU_ADDRESS [AWB_GAIN_BUFFER_SPEED]
	{0x0990, 0x0008}, 	// MCU_DATA_0
	{0x098C, 0xA349}, 	// MCU_ADDRESS [AWB_JUMP_DIVISOR]
	{0x0990, 0x0002}, 	// MCU_DATA_0
	{0x098C, 0xA34A}, 	// MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x0090}, 	// MCU_DATA_0
	{0x098C, 0xA34B}, 	// MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00FF}, 	// MCU_DATA_0
	{0x098C, 0xA34C}, 	// MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0075}, 	// MCU_DATA_0
	{0x098C, 0xA34D}, 	// MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x00EF}, 	// MCU_DATA_0
	{0x098C, 0xA351}, 	// MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x0000}, 	// MCU_DATA_0
	{0x098C, 0xA352}, 	// MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x007F}, 	// MCU_DATA_0
	{0x098C, 0xA354}, 	// MCU_ADDRESS [AWB_SATURATION]
	{0x0990, 0x0043}, 	// MCU_DATA_0
	{0x098C, 0xA355}, 	// MCU_ADDRESS [AWB_MODE]
	{0x0990, 0x0001}, 	// MCU_DATA_0
	{0x098C, 0xA35D}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MIN]
	{0x0990, 0x0078}, 	// MCU_DATA_0
	{0x098C, 0xA35E}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MAX]
	{0x0990, 0x0086}, 	// MCU_DATA_0
	{0x098C, 0xA35F}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MIN]
	{0x0990, 0x007E}, 	// MCU_DATA_0
	{0x098C, 0xA360}, 	// MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MAX]
	{0x0990, 0x0082}, 	// MCU_DATA_0
	{0x098C, 0x2361}, 	// MCU_ADDRESS [AWB_CNT_PXL_TH]
	{0x0990, 0x0040}, 	// MCU_DATA_0
	{0x098C, 0xA363}, 	// MCU_ADDRESS [AWB_TG_MIN0]
	{0x0990, 0x00D2}, 	// MCU_DATA_0
	{0x098C, 0xA364}, 	// MCU_ADDRESS [AWB_TG_MAX0]
	{0x0990, 0x00F6}, 	// MCU_DATA_0
	{0x098C, 0xA302}, 	// MCU_ADDRESS [AWB_WINDOW_POS]
	{0x0990, 0x0000}, 	// MCU_DATA_0
	{0x098C, 0xA303}, 	// MCU_ADDRESS [AWB_WINDOW_SIZE]
	{0x0990, 0x00EF}, 	// MCU_DATA_0
	{0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0024}, 	// MCU_DATA_0
	//Timing
	{0x98C, 0x2703},	//Output Width (A)
	{0x990, 0x0280},	//      = 640
	{0x98C, 0x2705},	//Output Height (A)
	{0x990, 0x01E0},	//      = 480
	{0x98C, 0x2707},	//Output Width (B)
	{0x990, 0x0280},	//      = 640
	{0x98C, 0x2709},	//Output Height (B)
	{0x990, 0x01E0},	//      = 480
	{0x98C, 0x270D},	//Row Start (A)
	{0x990, 0x000	}, //      = 0
	{0x98C, 0x270F},	//Column Start (A)
	{0x990, 0x000	}, //      = 0
	{0x98C, 0x2711},	//Row End (A)
	{0x990, 0x1E7	}, //      = 487
	{0x98C, 0x2713},	//Column End (A)
	{0x990, 0x287	}, //      = 647
	{0x98C, 0x2715},	//Row Speed (A)
	{0x990, 0x0001},	//      = 1
	{0x98C, 0x2717},	//Read Mode (A)
	{0x990, 0x0026},	//      = 38
	{0x98C, 0x2719},	//sensor_fine_correction (A)
	{0x990, 0x001A},	//      = 26
	{0x98C, 0x271B},	//sensor_fine_IT_min (A)
	{0x990, 0x006B},	//      = 107
	{0x98C, 0x271D},	//sensor_fine_IT_max_margin (A)
	{0x990, 0x006B},	//      = 107
	{0x98C, 0x271F},	//Frame Lines (A)
	{0x990, 0x02B4},	//      = 692
	{0x98C, 0x2721},	//Line Length (A)
	{0x990, 0x034A},	//      = 842
	{0x98C, 0x2723},	//Row Start (B)
	{0x990, 0x000}, //      = 0
	{0x98C, 0x2725},	//Column Start (B)
	{0x990, 0x000}, //      = 0
	{0x98C, 0x2727},	//Row End (B)
	{0x990, 0x1E7}, //      = 487
	{0x98C, 0x2729},	//Column End (B)
	{0x990, 0x287}, //      = 647
	{0x98C, 0x272B},	//Row Speed (B)
	{0x990, 0x0001},	//      = 1
	{0x98C, 0x272D},	//Read Mode (B)
	{0x990, 0x0026},	//      = 38
	{0x98C, 0x272F},	//sensor_fine_correction (B)
	{0x990, 0x001A},	//      = 26
	{0x98C, 0x2731},	//sensor_fine_IT_min (B)
	{0x990, 0x006B},	//      = 107
	{0x98C, 0x2733},	//sensor_fine_IT_max_margin (B)
	{0x990, 0x006B},	//      = 107
	{0x98C, 0x2735},	//Frame Lines (B)
	{0x990, 0x02C5},	//      = 709
	{0x98C, 0x2737},	//Line Length (B)
	{0x990, 0x034A},	//      = 842
	{0x98C, 0x2739},	//Crop_X0 (A)
	{0x990, 0x0000},	//      = 0
	{0x98C, 0x273B},	//Crop_X1 (A)
	{0x990, 0x027F},	//      = 639
	{0x98C, 0x273D},	//Crop_Y0 (A)
	{0x990, 0x0000},	//      = 0
	{0x98C, 0x273F},	//Crop_Y1 (A)
	{0x990, 0x01DF},	//      = 479
	{0x98C, 0x2747},	//Crop_X0 (B)
	{0x990, 0x0000},	//      = 0
	{0x98C, 0x2749},	//Crop_X1 (B)
	{0x990, 0x027F},	//      = 639
	{0x98C, 0x274B},	//Crop_Y0 (B)
	{0x990, 0x0000},	//      = 0
	{0x98C, 0x274D},	//Crop_Y1 (B)
	{0x990, 0x01DF},	//      = 479
	{0x98C, 0x222D},	//R9 Step
	{0x990, 0x0057},	//      = 87
	{0x98C, 0xA408},	//search_f1_50
	{0x990, 0x1C	}, //      = 28
	{0x98C, 0xA409},	//search_f2_50
	{0x990, 0x1E	}, //      = 30
	{0x98C, 0xA40A},	//search_f1_60
	{0x990, 0x21	}, //      = 33
	{0x98C, 0xA40B},	//search_f2_60
	{0x990, 0x23	}, //      = 35
	{0x98C, 0x2411},	//R9_Step_60 (A)
	{0x990, 0x0057},	//      = 87
	{0x98C, 0x2413},	//R9_Step_50 (A)
	{0x990, 0x0068},	//      = 104
	{0x98C, 0x2415},	//R9_Step_60 (B)
	{0x990, 0x0057},	//      = 87
	{0x98C, 0x2417},	//R9_Step_50 (B)
	{0x990, 0x0068},	//      = 104
	{0x98C, 0xA404},	//FD Mode
	{0x990, 0x10	}, //      = 16
	{0x98C, 0xA40D},	//Stat_min
	{0x990, 0x02	}, //      = 2
	{0x98C, 0xA40E},	//Stat_max
	{0x990, 0x03	}, //      = 3
	{0x98C, 0xA410},	//Min_amplitude
	{0x990, 0x0A	}, //      = 10
	//Data format YUV Cr-Y-Cb-Y
	{0x098C, 0x2755}, 	// MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
	{0x0990, 0x0001}, 	// MCU_DATA_0
};

struct mt9v113_i2c_reg_conf const mt9v113_config_third[] = {
	// MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, 	// MCU_DATA_0
	{0x098C, 0xAB3D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x000A}, 	// MCU_DATA_0
	{0x098C, 0xAB3E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x001F}, 	// MCU_DATA_0
	{0x098C, 0xAB3F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x003E}, 	// MCU_DATA_0
	{0x098C, 0xAB40}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x006B}, 	// MCU_DATA_0
	{0x098C, 0xAB41}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x0092}, 	// MCU_DATA_0
	{0x098C, 0xAB42}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x00AC}, 	// MCU_DATA_0
	{0x098C, 0xAB43}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x00BE}, 	// MCU_DATA_0
	{0x098C, 0xAB44}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00CB}, 	// MCU_DATA_0
	{0x098C, 0xAB45}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00D5}, 	// MCU_DATA_0
	{0x098C, 0xAB46}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00DD}, 	// MCU_DATA_0
	{0x098C, 0xAB47}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00E4}, 	// MCU_DATA_0
	{0x098C, 0xAB48}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00E9}, 	// MCU_DATA_0
	{0x098C, 0xAB49}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00EE}, 	// MCU_DATA_0
	{0x098C, 0xAB4A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00F2}, 	// MCU_DATA_0
	{0x098C, 0xAB4B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00F6}, 	// MCU_DATA_0
	{0x098C, 0xAB4C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F9}, 	// MCU_DATA_0
	{0x098C, 0xAB4D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00FC}, 	// MCU_DATA_0
	{0x098C, 0xAB4E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, 	// MCU_DATA_0
};

struct mt9v113_reg mt9v113_regs = {
	.config_first = &mt9v113_config_first[0],
	.config_first_size = ARRAY_SIZE(mt9v113_config_first),
	.config_second = &mt9v113_config_second[0],
	.config_second_size = ARRAY_SIZE(mt9v113_config_second),
	.config_third = &mt9v113_config_third[0],
	.config_third_size = ARRAY_SIZE(mt9v113_config_third),
};


