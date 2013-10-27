/*
 * leds-msm-pmic.c - MSM PMIC LEDs driver.
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <mach/rpc_pmapp.h>
#include <mach/pmic.h>

enum led_type {
    INVALID_LED = -1,
    RED_LED,
    GREEN_LED,
    BLUE_LED,
    AMBER_LED,
    KEYBOARD_LED,
    NUM_LEDS,
};
static unsigned long blink[NUM_LEDS];

extern int mpp_config_leds_control(unsigned mpp, unsigned config);
extern int mpp_config_i_sink(unsigned mpp, unsigned config);

static void msm_leds_brightness_set(struct led_classdev *led_cdev,
                                           enum led_brightness value)
{
    int ret = 0;
    printk("msm_leds_brightness_set %d\n", value);
    if (!strcmp("keyboard-backlight", led_cdev->name)){
        ret = mpp_config_i_sink(PM_MPP_5, 
            (PM_MPP__I_SINK__LEVEL_25mA << 16) |
            ((LED_OFF==value) ? PM_MPP__I_SINK__SWITCH_DIS : PM_MPP__I_SINK__SWITCH_ENA));
    } else if (!strcmp("red", led_cdev->name)){
        ret = mpp_config_leds_control(1, (LED_OFF==value) ? 0 : 2);
    } else if (!strcmp("green", led_cdev->name)) {
        ret = mpp_config_leds_control(2, (LED_OFF==value) ? 0 : 2);
    } else if (!strcmp("blue", led_cdev->name)){
        ret = mpp_config_leds_control(3, (LED_OFF==value) ? 0 : 2);
    } else if (!strcmp("amber", led_cdev->name)){
        ret = mpp_config_leds_control(3, (LED_OFF==value) ? 0 : 2);
    } else {
        dev_err(led_cdev->dev, "can't set led light\n");
        return;
    }

    if(ret) {
        dev_err(led_cdev->dev, "can't set led light,ret=%d\n",ret);
    } else {
        led_cdev->brightness = value;
    }
}

static struct led_classdev msm_leds[] = {
    [GREEN_LED] = {
        .name = "green",
        .brightness = LED_OFF,
        .brightness_set = msm_leds_brightness_set,
    },
    [RED_LED] = {
        .name = "red",
        .brightness = LED_OFF,
        .brightness_set = msm_leds_brightness_set,
    },
    [BLUE_LED] = {
        .name = "blue",
        .brightness = LED_OFF,
        .brightness_set = msm_leds_brightness_set,
    },
    [AMBER_LED] = {
        .name = "amber",
        .brightness = LED_OFF,
        .brightness_set = msm_leds_brightness_set,
    },
    [KEYBOARD_LED] = {
        .name = "keyboard-backlight",
        .brightness = LED_OFF,
        .brightness_set = msm_leds_brightness_set,
    },
};

static ssize_t leds_blink_store(struct device *dev, struct device_attribute *attr,
                                                   const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    ssize_t ret = -EINVAL;
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    size_t count = after - buf;
    printk("leds_blink_store %s\n", buf);

    if (isspace(*after))
        count++;
    if (count == size) {
        ret = count;
        if (!strcmp("red", led_cdev->name)) {
            blink[RED_LED] = state;
            ret = mpp_config_leds_control(1,  state ? 1 : 0);
        } else if (!strcmp("green", led_cdev->name)) {
            blink[GREEN_LED] = state;
            ret = mpp_config_leds_control(2,  state ? 1 : 0);
        } else {
            dev_err(led_cdev->dev, "can't blink the led light:%s\n", led_cdev->name);
        }
    }

    return ret;
} 

static ssize_t leds_blink_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    unsigned long tmp = 0;
    struct led_classdev *led_cdev = dev_get_drvdata(dev);

    if (!strcmp("red", led_cdev->name)) {
        tmp = blink[RED_LED];
    } else if (!strcmp("green", led_cdev->name)) {
        tmp = blink[GREEN_LED];
    }

    return sprintf(buf, "%ld\n", tmp);
}

DEVICE_ATTR(blink, 0770, leds_blink_show, leds_blink_store);

static int msm_pmic_led_probe(struct platform_device *pdev)
{
    int rc;
    int i = 0;

    for (i = RED_LED; i < NUM_LEDS; i++) {
        rc = led_classdev_register(&pdev->dev, &msm_leds[i]);
        if (rc) {
            dev_err(&pdev->dev, "unable to register led class driver\n");
        }
        msm_leds_brightness_set(&msm_leds[i], LED_OFF);
        blink[i] = 0;
    }
    rc = device_create_file(msm_leds[RED_LED].dev, &dev_attr_blink);
    if (rc) {
        dev_err(&pdev->dev, "unable to create file blink RED\n");
    }
    rc = device_create_file(msm_leds[GREEN_LED].dev, &dev_attr_blink);
    if (rc) {
        dev_err(&pdev->dev, "unable to create file blink GREEN\n");
    }

    msm_leds_brightness_set(&msm_leds[KEYBOARD_LED], LED_FULL);

    return 0;
}

static int __devexit msm_pmic_led_remove(struct platform_device *pdev)
{
    int i = 0;

    for (i = 0; i < NUM_LEDS; i++) {
        msm_leds_brightness_set(&msm_leds[i], LED_OFF);
        device_remove_file(msm_leds[i].dev, &dev_attr_blink);
        led_classdev_unregister(&msm_leds[i]);
    }

    return 0;
}

#ifdef CONFIG_PM
static int msm_pmic_led_suspend(struct platform_device *dev,
        pm_message_t state)
{
    led_classdev_suspend(&msm_leds[KEYBOARD_LED]);

    return 0;
}

static int msm_pmic_led_resume(struct platform_device *dev)
{
    led_classdev_resume(&msm_leds[KEYBOARD_LED]);

    return 0;
}
#else
#define msm_pmic_led_suspend NULL
#define msm_pmic_led_resume NULL
#endif


static struct platform_driver msm_pmic_led_driver = {
    .probe      = msm_pmic_led_probe,
    .remove     = __devexit_p(msm_pmic_led_remove),
    .suspend    = msm_pmic_led_suspend,
    .resume     = msm_pmic_led_resume,
    .driver     = {
    .name       = "pmic-leds",
    .owner      = THIS_MODULE,
    },
};

static int __init msm_pmic_led_init(void)
{
    return platform_driver_register(&msm_pmic_led_driver);
}
module_init(msm_pmic_led_init);

static void __exit msm_pmic_led_exit(void)
{
    platform_driver_unregister(&msm_pmic_led_driver);
}
module_exit(msm_pmic_led_exit);

MODULE_DESCRIPTION("MSM PMIC LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-leds");
