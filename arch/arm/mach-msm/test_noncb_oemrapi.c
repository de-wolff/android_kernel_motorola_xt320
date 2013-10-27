/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * OEM RAPI CLIENT Driver source file
 */
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <mach/msm_rpcrouter.h>
#include <mach/oem_rapi_client.h>

#if defined(CONFIG_GSENSOR_BMA250)
extern bool bma250_sensor_id(void);
#endif

#if defined(CONFIG_SENSORS_TSL2550)
extern bool tsl2550_sensor_id(void);
#endif

#if defined(CONFIG_SENSORS_LTR501ALS)
extern bool ltr501_sensor_id(void);
#endif

#if defined(CONFIG_COMPASS_AKM8975)
extern bool akm8975_sensor_id(void);
#endif

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("AnnaDing");
MODULE_VERSION("1.0");

static struct msm_rpc_client *client;
static int oem_rapi_test_noncb_call(void) 
{
	struct oem_rapi_client_streaming_func_arg arg;
	struct oem_rapi_client_streaming_func_ret ret;
	char* input;
	int ret_val= 0;
	int len=10;
	char *s1="p";//success
	char *s2="f";//fail
	input = kmalloc(len, GFP_KERNEL);
	memset(input, 0, len);

#if defined(CONFIG_COMPASS_AKM8975)
	if (akm8975_sensor_id()==true){
		strcat(input,s1);				 	
	}else{
		strcat(input,s2);			
	}
#else
		strcat(input,s2);
#endif

#if defined(CONFIG_GSENSOR_BMA250)
	if (bma250_sensor_id()==true){
		strcat(input,s1);
	}else{
		strcat(input,s2);
	}
#else
		strcat(input,s2);
#endif

#if defined(CONFIG_SENSORS_TSL2550) && defined(CONFIG_SENSORS_LTR501ALS)
	if (true == tsl2550_sensor_id() || true == ltr501_sensor_id()){
		strcat(input,s1);
	}else{
		strcat(input,s2);
	}
#elif	defined(CONFIG_SENSORS_TSL2550)
	if (true == tsl2550_sensor_id() ){
		strcat(input,s1);
	}else{
		strcat(input,s2);
	}
#elif	defined(CONFIG_SENSORS_LTR501ALS)
	if (true == ltr501_sensor_id()){
		strcat(input,s1);
	}else{
		strcat(input,s2);
	}
#else
		strcat(input,s2);
#endif
	printk("%s input string =%s,%d\r\n", __func__, input, strlen(input));//send a string to ARM9
		
	arg.event= OEM_RAPI_CLIENT_EVENT_ARM9_DATA_SET;
	arg.cb_func= NULL;
	arg.handle= (void *)0;
	arg.in_len= strlen(input)+1;
	arg.input= input;
	arg.out_len_valid= 0;
	arg.output_valid= 0;
	arg.output_size= 0;
	ret.out_len= NULL;
	ret.output= NULL;
	ret_val= oem_rapi_client_streaming_function(client, &arg, &ret);
	if (ret_val)
		printk("oem rapi client test fail %d\n", ret_val);
	else
		printk("oem rapi client test passed\n");
	return 0;	
}

static int oem_rapi_test_probe(struct platform_device *pdev)
{
	client = oem_rapi_client_init();

	if(client!=NULL){
		oem_rapi_test_noncb_call();
	}
	oem_rapi_client_close();
	return 0;
}

static struct platform_driver oem_rapi_driver = {
	.probe = oem_rapi_test_probe,
	.driver = {
		.name = "oemrapitest",
		.owner = THIS_MODULE,
	},
};

static int __init oem_rapi_test_noncb_init(void) {
	platform_driver_register(&oem_rapi_driver);
	return 0;
}

static void __exit oem_rapi_test_noncb_exit(void){
	platform_driver_unregister(&oem_rapi_driver);
}

MODULE_LICENSE("GPL");
module_init(oem_rapi_test_noncb_init);
module_exit(oem_rapi_test_noncb_exit);

