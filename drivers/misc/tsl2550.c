/*
 *  tsl2550.c - Linux kernel modules for ambient light sensor
 *
 *  Copyright (C) 2007 Rodolfo Giometti <giometti@linux.it>
 *  Copyright (C) 2007 Eurotech S.p.A. <info@eurotech.it>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/input.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>

#include <asm/uaccess.h>
#include <mach/vreg.h>

#define TSL2550_DEBUG 		1
#define TSL2550_DRV_NAME	"tsl2550"
#define DRIVER_VERSION		"1.2"
#define TSL2550_INT_GPIO 		17

#define LIGHT_IOM		 'i'

#define LIGHT_IOC_GET_PFLAG		_IOR(LIGHT_IOM, 0x00, short)
#define LIGHT_IOC_SET_PFLAG		_IOW(LIGHT_IOM, 0x01, short)

#define LIGHT_IOC_GET_LFLAG		_IOR(LIGHT_IOM, 0x10, short)
#define LIGHT_IOC_SET_LFLAG		_IOW(LIGHT_IOM, 0x11, short)

#if TSL2550_DEBUG
#define TSLDBG(format, ...)	\
		printk(KERN_INFO "TSL2550 " format , ## __VA_ARGS__)
#else
#define TSLDBG(format, ...)
#endif

static atomic_t	p_flag;
static atomic_t	l_flag;

static struct work_struct tsl2550_irq_wq;
static int tsl2550_irq;
static struct input_dev *tsl2550_input = NULL;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif

static bool isGood = false;
static bool tsl2550_is_sleep = false;
#if defined(CONFIG_SENSORS_LTR501ALS)
extern bool already_have_p_sensor;
#endif
	
struct tsl2550_data {
	struct i2c_client *client;
	struct mutex update_lock;
};

static struct tsl2550_data *tsl2550_data=NULL;
//static DEFINE_MUTEX(tsl550_mutex);//clf delete
/*
 * LUX calculation
 */
 static const u32 luxdata[] = {
	1, 1, 1, 2, 2, 2, 3, 4,
	4, 5, 6, 7, 9 ,11 ,13 ,16 ,
	19, 22, 27, 32, 39, 46, 56, 67,
	80, 96, 116, 139, 167, 200, 240, 289,
	346, 416, 499, 599, 720, 864, 1037, 1245,
	1495, 1795, 2154, 2586, 3105, 3728, 4475, 5372,
	6449, 7743, 9295, 11159, 13396, 16082, 19307, 23178,
	27826, 33405, 40103, 48144, 57797, 69386, 83298, 100000,
};

static void update_tsl_state(void);

static void tsl2550_irq_do_work(struct work_struct *work)
{
	int val;	
	int status;
//	int flag_p = atomic_read(&p_flag);
//	int flag_l = atomic_read(&l_flag);
	
	status = i2c_smbus_read_byte_data(tsl2550_data->client, 0x03);
	val = i2c_smbus_read_byte_data(tsl2550_data->client, 0x05);
	
//	TSLDBG("flag_p = %d   flag_l = %d  status = %d   val = %d\n", flag_p, flag_l, status, val);
	if (status < 0 || val < 0){
		printk("i2c read error\n");
		return;
	}
		
	status &= 0x03;
	switch(status)
	{			
		case 1:
			//TSLDBG("DLS interrupt---val = %x, status =  %d\n", val, status);
			val = val & 0x3F;
			input_report_abs(tsl2550_input, ABS_MISC, luxdata[val]);
			break;
			
		case 2:
			TSLDBG("DPS interrupt---val = %x, status =  %d\n", val, status);
			val = val & 0x80;
			//the sensor couldn't sleep when something is in close proximity to it.
			input_report_abs(tsl2550_input,ABS_DISTANCE, val);
			break;
			
		case 3:
			TSLDBG("DPS and DLS interrupt---val =%x, status =  %d\n", val, status);
			input_report_abs(tsl2550_input,ABS_MISC, luxdata[val & 0x3f]);
			input_report_abs(tsl2550_input,ABS_DISTANCE, (val & 0x80));
			break;
			
		default:
			TSLDBG("TSL2550 no interrupt status:%d\n", status);
			break;
	}
	
	input_sync(tsl2550_input);	
	enable_irq(tsl2550_irq);
	
}

static irqreturn_t tsl2550_interrupt(int irq, void *dev_id)
{	
	disable_irq_nosync(tsl2550_irq);
	schedule_work(&tsl2550_irq_wq);

	return IRQ_HANDLED;
}

static int tsl2550_init_client(struct i2c_client *client)
{
   	if (i2c_smbus_write_byte_data(client,0x00,0x03) < 0){
		TSLDBG("tsl2550_init_client i2c error\n");
		return -1;
   	}
	mdelay(200);
	i2c_smbus_write_byte_data(client,0x01,0x0);
   	i2c_smbus_write_byte_data(client,0x04,0x19);
	i2c_smbus_write_byte_data(client,0x08,0x0c);
   	//i2c_smbus_write_byte_data(client,0x00,0x02);	
   	return 0;
}

static int tsl2550_init_input(void)
{
	int result = 0;
	
	tsl2550_input = input_allocate_device();
	if (tsl2550_input == NULL) {
		TSLDBG("%s: not enough memory for input device\n", __func__);
		result = -ENOMEM;
		goto fail_alloc_input;
	}
	tsl2550_input->name = "lightsensor";
	
	//setting the supported event type and code
	set_bit(EV_ABS, tsl2550_input->evbit);
	//set_bit(ABS_DISTANCE, tsl2550_data->input->absbit);
	//set_bit(ABS_MISC, tsl2550_data->input->absbit);
	input_set_abs_params(tsl2550_input, ABS_MISC, 0, 100000, 0, 0);
	input_set_abs_params(tsl2550_input, ABS_DISTANCE, 0, 128, 0, 0);	

	result = input_register_device(tsl2550_input);
	if (result != 0) {
		TSLDBG("%s: failed to register input device\n", __func__);
		goto fail_input_dev_reg;
	}	
	
	return 0;
	
fail_input_dev_reg:
	input_free_device(tsl2550_input);	
fail_alloc_input:	
	return result;
	
}
static int tsl2550_open(struct inode *inode, struct file *file)
{
	return 0;
	/*
	int ret;
	
   	ret = i2c_smbus_write_byte_data(tsl2550_data->client, 0x00, 0x02);
	mdelay(10);
	return ret;
	*/
}

static int tsl2550_release(struct inode *inode, struct file *file)
{
	return 0;
	/*
	int ret;	
   	ret = i2c_smbus_write_byte_data(tsl2550_data->client, 0x00, 0x03);
	mdelay(10);

	return ret;
	*/
}

static void update_tsl_state()
{
	int flag_p = atomic_read(&p_flag);
	int flag_l = atomic_read(&l_flag);

	//only use P-sensor when p_flag & l_flag == 1, can not open both, add by otis 
	printk("update_tsl_state\n");
	if (flag_p && flag_l)
	{
		if (i2c_smbus_write_byte_data(tsl2550_data->client,0x00,0x01) != 0)
		{
			TSLDBG("%s: i2c write error, and set DLS and DPS enable failed!\n", __FUNCTION__);
			return;
		}
	}
	else if(flag_l)
	{
		if (i2c_smbus_write_byte_data(tsl2550_data->client,0x00,0x00) != 0)
		{
			TSLDBG("%s: i2c write error, and set DLS enable failed!\n", __FUNCTION__);
			return;
		}
	}
	else if (flag_p)
	{
		if (i2c_smbus_write_byte_data(tsl2550_data->client,0x00,0x01) != 0)
		{
			TSLDBG("%s: i2c write error, and set DPS enable failed!\n", __FUNCTION__);
			return;
		}
	}
	else
	{
		if (i2c_smbus_write_byte_data(tsl2550_data->client,0x00,0x03) != 0)
		{
			TSLDBG("%s: i2c write error, and set function 02 failed!\n", __FUNCTION__);
			return;
		}
	}
	mdelay(20);
}

static long tsl2550_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *pa = (void __user *)arg;
	short flag = 0;

	switch (cmd) {
		case LIGHT_IOC_SET_PFLAG:
			if (copy_from_user(&flag, pa, sizeof(flag)))
				return -EFAULT;
			if (flag < 0 || flag > 1)
				return -EINVAL;
			//TSLDBG("tsl2550_ioctl LIGHT_IOC_SET_PFLAG flag = %d\n", flag);
			atomic_set(&p_flag, flag);
			update_tsl_state();
			break;
		case LIGHT_IOC_GET_PFLAG:
			flag = atomic_read(&p_flag);
			//TSLDBG("tsl2550_ioctl LIGHT_IOC_GET_PFLAG flag = %d\n", flag);
			if (copy_to_user(pa, &flag, sizeof(flag)))
				return -EFAULT;
			break;
		case LIGHT_IOC_SET_LFLAG:
			if (copy_from_user(&flag, pa, sizeof(flag)))
				return -EFAULT;
			if (flag < 0 || flag > 1)
				return -EINVAL;
			//TSLDBG("tsl2550_ioctl LIGHT_IOC_SET_LFLAG flag = %d\n", flag);
			atomic_set(&l_flag, flag);
			if(!tsl2550_is_sleep){
				update_tsl_state();
			}
			break;
		case LIGHT_IOC_GET_LFLAG:
			flag = atomic_read(&l_flag);
			//TSLDBG("tsl2550_ioctl LIGHT_IOC_GET_LFLAG flag = %d\n", flag);
			if (copy_to_user(pa, &flag, sizeof(flag)))
				return -EFAULT;
			break;
		default:
			break;
	}
	return 0;
}

static struct file_operations tsl2550_fops = {
	.owner = THIS_MODULE,
	.open = tsl2550_open,
	.release = tsl2550_release,
	.unlocked_ioctl = tsl2550_ioctl,
};

static struct miscdevice tsl2550_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lightsensor",
	.fops = &tsl2550_fops,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tsl2550_early_suspend(struct early_suspend *h)
{
	int flag = atomic_read(&p_flag);
	
	TSLDBG("%s: %d----flag == 0\n", __func__, __LINE__);
	if (!flag){
		i2c_smbus_write_byte_data(tsl2550_data->client,0x00,0x0b);
		mdelay(10);
	}
	tsl2550_is_sleep = true;
}

static void tsl2550_late_resume(struct early_suspend *h)
{
	int flag = atomic_read(&p_flag);
	
	TSLDBG("%s: %d----flag == 0\n", __func__, __LINE__);
	if (!flag){
		update_tsl_state();
	}

	tsl2550_is_sleep = false;
}
#endif

static int tsl2550_read_id(void)
{
 	if((0x0==i2c_smbus_read_byte_data(tsl2550_data->client, 0x01))
		&&(0x19==i2c_smbus_read_byte_data(tsl2550_data->client, 0x04))
		&&(0x0c==i2c_smbus_read_byte_data(tsl2550_data->client, 0x08))){
		isGood = true;
 	}
	return isGood;	

	/*
	if(isGood) 
		return isGood;
	int id=0x00;
    int num=0;
	for (num=0;num<1;num++)
	{
        id = i2c_smbus_read_byte_data(tsl2550_data->client, 0x03);//0x03 read only
		if( (id&0x03)!=0 ){
			isGood = true;
			return isGood;
		}
		mdelay(10);
        printk("tsl2550_read_id =0x%x\n",isGood);
	}
	return isGood;
	*/
     
}
bool tsl2550_sensor_id(void)
{
	TSLDBG("tsl2550_sensor_id =%d\n",isGood);
	return isGood;
  /* if (tsl2550_read_id()!=0x00)
        {
        return true;
        }
        else
        {
        return false;
        }
	*/
}
 
 EXPORT_SYMBOL(tsl2550_sensor_id);

static int tsl2550_init_device(void){
	if (-1 == tsl2550_init_client(tsl2550_data->client)){
		printk("tsl2550_init_client error \n");
		return -1;
	}
	mdelay(200);
	if (false == tsl2550_read_id()){
		printk("tsl2550_read_id error \n");
		return -1;
	}	

	return 0;
}

static int __init tsl2550_power_up(void){
	static struct vreg *vreg_l10;
	int rc;
	
	vreg_l10 = vreg_get(NULL, "emmc");
	if (IS_ERR(vreg_l10)) {
		pr_err("%s: vreg_get for L10 failed\n", __func__);
		return PTR_ERR(vreg_l10);
	}

	rc = vreg_set_level(vreg_l10, 1800);
	if (rc) {
		pr_err("%s: vreg set level failed (%d) for l10\n",
		       __func__, rc);
		goto vreg_put_l10;
	}

	rc = vreg_enable(vreg_l10);
	if (rc < 0) {
		pr_err("%s: vreg enable failed (%d)\n",
		       __func__, rc);
		goto vreg_put_l10;
	}
	printk("tsl2550_power_up\n");
vreg_put_l10:
	vreg_put(vreg_l10);

	return rc;
}

static int tsl2550_power_down(void){
	static struct vreg *vreg_l10;
	int rc;
	
	vreg_l10 = vreg_get(NULL, "emmc");
	if (IS_ERR(vreg_l10)) {
		pr_err("%s: vreg_get for L10 failed\n", __func__);
		return PTR_ERR(vreg_l10);
	}

	rc = vreg_disable(vreg_l10);
	if (rc < 0) {
		pr_err("%s: vreg disable failed (%d)\n",
		       __func__, rc);
		goto vreg_put_l10;
	}
	printk("tsl2550_power_down\n");
vreg_put_l10:
	vreg_put(vreg_l10);

	return rc;
}

static int __devinit tsl2550_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int  err = 0;
	int  reset_count= 0;
	struct i2c_adapter *adapter = NULL;	

#if defined(CONFIG_SENSORS_LTR501ALS)
	if (true == already_have_p_sensor){
		printk("already have another sensor\n");
		err = -1;
		goto exit;
	}
#endif

	adapter = to_i2c_adapter(client->dev.parent);
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WRITE_BYTE
					    | I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
		err = -EIO;
		goto exit;
	}

	tsl2550_data = kzalloc(sizeof(struct tsl2550_data), GFP_KERNEL);
	if (!tsl2550_data) {
		err = -ENOMEM;
		goto exit;
	}
	tsl2550_data->client = client;
	i2c_set_clientdata(client, tsl2550_data);

	mutex_init(&tsl2550_data->update_lock);

	while (-1 == tsl2550_init_device() && reset_count < 5){
		tsl2550_power_down();
		mdelay(200);
		tsl2550_power_up();
		mdelay(500);
		reset_count ++;
	}

	if (false == isGood){
		printk("tsl2550 is false!!!!\n");
		err = -1;
		goto exit_kfree;
	}
	
	INIT_WORK(&tsl2550_irq_wq, tsl2550_irq_do_work);
	gpio_tlmm_config(GPIO_CFG(TSL2550_INT_GPIO, 0, 
					GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, 
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_request(TSL2550_INT_GPIO, "interrupt");
	gpio_direction_output(TSL2550_INT_GPIO, 1);
	tsl2550_irq = gpio_to_irq(TSL2550_INT_GPIO);
	err = request_irq(tsl2550_irq, tsl2550_interrupt, 
							IRQF_TRIGGER_LOW, "tsl2550", NULL);
	if (err != 0) {
		TSLDBG("%s: cannot register irq\n", __func__);
		goto exit_kfree;
	}
	
	tsl2550_init_input();
	
	err = misc_register(&tsl2550_device);
	if (err) {
		TSLDBG("tsl2550_probe: tsl2550_device register failed\n");
		goto fail_misc_register;
	}

	#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	early_suspend.suspend = tsl2550_early_suspend;
	early_suspend.resume = tsl2550_late_resume;
	register_early_suspend(&early_suspend);
	#endif
	return 0;
	
fail_misc_register:
	input_unregister_device(tsl2550_input);
exit_kfree:
	kfree(tsl2550_data);
exit:
	tsl2550_power_down();
	return err;
}

static int __devexit tsl2550_remove(struct i2c_client *client)
{
	/* Power down the device */
	free_irq(tsl2550_irq, NULL);
	input_unregister_device(tsl2550_input);
	misc_deregister(&tsl2550_device);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static int tsl2550_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int flag = atomic_read(&p_flag);

	if (flag){
		disable_irq_nosync(tsl2550_irq);
		enable_irq_wake(tsl2550_irq);
	}
	TSLDBG("tsl2550 -----flag=%d--------%s\n", flag,  __func__);
	return 0;
}

static int tsl2550_resume(struct i2c_client *client)
{
	int flag = atomic_read(&p_flag);
	
	TSLDBG("tsl2550 -----flag=%d--------%s\n", flag, __func__);
	if(flag){
		enable_irq(tsl2550_irq);
		disable_irq_wake(tsl2550_irq);
	}
	return 0;
}

void tsl2550_shutdown(struct i2c_client *client)
{
	printk("tsl2550_shutdown\n");
	i2c_smbus_write_byte_data(tsl2550_data->client,0x00,0x0b);
}

static const struct i2c_device_id tsl2550_id[] = {
	{ "ltr505", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tsl2550_id);

static struct i2c_driver tsl2550_driver = {
	.driver = {
		.name	= TSL2550_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= tsl2550_probe,
	.remove	= __devexit_p(tsl2550_remove),
	.id_table = tsl2550_id,
	.suspend = tsl2550_suspend,
	.resume = tsl2550_resume,
	.shutdown = tsl2550_shutdown,
};

static int __init tsl2550_init(void)
{
	return i2c_add_driver(&tsl2550_driver);
}

static void __exit tsl2550_exit(void)
{
	i2c_del_driver(&tsl2550_driver);
}

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("TSL2550 ambient light sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(tsl2550_init);
module_exit(tsl2550_exit);
