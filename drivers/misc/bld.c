/* drivers/misc/bld.c
 *
 * Copyright 2011  Ezekeel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/bld.h>
#include <linux/timer.h>
#include <linux/earlysuspend.h>

static bool bld_enabled = false;

static struct bld_implementation * bld_imp = NULL;

static unsigned int dimmer_delay = 5000;

static struct timer_list dimmer_timer;

#define BACKLIGHTDIMMER_VERSION 3

static void bld_early_suspend(struct early_suspend * h)
{
    del_timer_sync(&dimmer_timer);

    return;
}

static void bld_late_resume(struct early_suspend * h)
{
    touchkey_pressed();

    return;
}

static struct early_suspend bld_suspend_data = 
    {
  .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
  .suspend = bld_early_suspend,
  .resume = bld_late_resume,
    };

static void bld_dim_backlights(void)
{
    if (bld_imp)
  {
      bld_imp->dim();
  }
}

static ssize_t backlightdimmer_status_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", (bld_enabled ? 1 : 0));
}

static ssize_t backlightdimmer_status_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    unsigned int data;

    if(sscanf(buf, "%u\n", &data) == 1) 
  {
      pr_devel("%s: %u \n", __FUNCTION__, data);
      
      if (data == 1) 
    {
        pr_info("%s: BLD function enabled\n", __FUNCTION__);

        bld_enabled = true;

        touchkey_pressed();
    } 
      else if (data == 0) 
    {
        pr_info("%s: BLD function disabled\n", __FUNCTION__);

        bld_enabled = false;

        del_timer_sync(&dimmer_timer);
    } 
      else 
    {
        pr_info("%s: invalid input range %u\n", __FUNCTION__, data);
    }
  } 
    else 
  {
      pr_info("%s: invalid input\n", __FUNCTION__);
  }

    return size;
}

static ssize_t backlightdimmer_delay_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", dimmer_delay);
}

static ssize_t backlightdimmer_delay_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    unsigned int data;

    if(sscanf(buf, "%u\n", &data) == 1) 
  {
      dimmer_delay = data;

      pr_info("BLD delay set to %u\n", dimmer_delay);

      touchkey_pressed();
  } 
    else 
  {
      pr_info("%s: invalid input\n", __FUNCTION__);
  }

    return size;
}

static ssize_t backlightdimmer_version(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", BACKLIGHTDIMMER_VERSION);
}

static DEVICE_ATTR(enabled, S_IRUGO | S_IWUGO, backlightdimmer_status_read, backlightdimmer_status_write);
static DEVICE_ATTR(delay, S_IRUGO | S_IWUGO, backlightdimmer_delay_read, backlightdimmer_delay_write);
static DEVICE_ATTR(version, S_IRUGO , backlightdimmer_version, NULL);

static struct attribute *bld_notification_attributes[] = 
    {
  &dev_attr_enabled.attr,
  &dev_attr_delay.attr,
  &dev_attr_version.attr,
  NULL
    };

static struct attribute_group bld_notification_group = 
    {
  .attrs  = bld_notification_attributes,
    };

static struct miscdevice bld_device = 
    {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "backlightdimmer",
    };

void register_bld_implementation(struct bld_implementation * imp)
{
    bld_imp = imp;
}
EXPORT_SYMBOL(register_bld_implementation);

void touchkey_pressed(void)
{
    if (bld_enabled)
  {
      if (dimmer_delay == 0)
    {
        bld_dim_backlights();
    }
      else
    {
        del_timer_sync(&dimmer_timer);
        
        dimmer_timer.expires = jiffies + msecs_to_jiffies(dimmer_delay);
        
        add_timer(&dimmer_timer);
    }
  }
      
    return;
}
EXPORT_SYMBOL(touchkey_pressed);

static int __init bld_control_init(void)
{
    int ret;

    pr_info("%s misc_register(%s)\n", __FUNCTION__, bld_device.name);

    ret = misc_register(&bld_device);

    if (ret) 
  {
      pr_err("%s misc_register(%s) fail\n", __FUNCTION__, bld_device.name);

      return 1;
  }

    if (sysfs_create_group(&bld_device.this_device->kobj, &bld_notification_group) < 0) 
  {
      pr_err("%s sysfs_create_group fail\n", __FUNCTION__);
      pr_err("Failed to create sysfs group for device (%s)!\n", bld_device.name);
  }

    init_timer(&dimmer_timer);
    dimmer_timer.function = bld_dim_backlights;

    register_early_suspend(&bld_suspend_data);

    return 0;
}

device_initcall(bld_control_init);
