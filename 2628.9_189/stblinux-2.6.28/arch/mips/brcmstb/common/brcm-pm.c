/*---------------------------------------------------------------------------

    Copyright (c) 2001-2007 Broadcom Corporation                 /\
                                                          _     /  \     _
    _____________________________________________________/ \   /    \   / \_
                                                            \_/      \_/  
    
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    File: brcm-pm.c

    Description: 
    Power management for Broadcom STB/DTV peripherals

    when        who         what
    -----       ---         ----
    20071030    cernekee    initial version
    20071219    cernekee    add DDR self-refresh support; switch to sysfs
 ------------------------------------------------------------------------- */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/brcmstb/common/brcmstb.h>
#include <asm/brcmstb/common/brcm-pm.h>

static atomic_t usb_count = ATOMIC_INIT(1);
static atomic_t enet_count = ATOMIC_INIT(1);
static atomic_t sata_count = ATOMIC_INIT(1);

static int (*sata_off_cb)(void *) = NULL;
static int (*sata_on_cb)(void *) = NULL;
static void *sata_cb_arg;

static int (*enet_off_cb)(void *) = NULL;
static int (*enet_on_cb)(void *) = NULL;
static void *enet_cb_arg;

static int (*ehci_off_cb)(void *) = NULL;
static int (*ehci_on_cb)(void *) = NULL;
static void *ehci_cb_arg;

static int (*ohci_off_cb)(void *) = NULL;
static int (*ohci_on_cb)(void *) = NULL;
static void *ohci_cb_arg;

/*
 * LOW-LEVEL FUNCTIONS
 */

/*
 * usb, enet, sata are READ ONLY
 * 0  = no drivers present, device is powered off
 * >0 = driver(s) present, device is in use
 */

static void usb_enable(void)
{
	printk(KERN_INFO "brcm-pm: enabling power to USB block\n");

#if defined(BCHP_CLK_PM_CTRL_2_DIS_USB_216M_CLK_MASK)
	BDEV_UNSET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_USB_216M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);
#endif

	BDEV_UNSET(BCHP_CLK_PM_CTRL, BCHP_CLK_PM_CTRL_DIS_USB_108M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL);

	BDEV_SET(BCHP_USB_CTRL_UTMI_CTL_1,
		BCHP_USB_CTRL_UTMI_CTL_1_PHY_PWDNB_MASK |
		BCHP_USB_CTRL_UTMI_CTL_1_PHY1_PWDNB_MASK);
	BDEV_RD(BCHP_USB_CTRL_UTMI_CTL_1);

	BDEV_SET(BCHP_USB_CTRL_PLL_CTL_1,
		BCHP_USB_CTRL_PLL_CTL_1_PLL_PWRDWNB_MASK);
	BDEV_RD(BCHP_USB_CTRL_PLL_CTL_1);

	BDEV_SET(BCHP_USB_CTRL_UTMI_CTL_1,
		BCHP_USB_CTRL_UTMI_CTL_1_UTMI_PWDNB_MASK |
		BCHP_USB_CTRL_UTMI_CTL_1_UTMI1_PWDNB_MASK);
	BDEV_RD(BCHP_USB_CTRL_PLL_CTL_1);
}

static void usb_disable(void)
{
	printk(KERN_INFO "brcm-pm: disabling power to USB block\n");

	BDEV_UNSET(BCHP_USB_CTRL_UTMI_CTL_1,
		BCHP_USB_CTRL_UTMI_CTL_1_UTMI_PWDNB_MASK |
		BCHP_USB_CTRL_UTMI_CTL_1_UTMI1_PWDNB_MASK);
	BDEV_RD(BCHP_USB_CTRL_UTMI_CTL_1);

	BDEV_UNSET(BCHP_USB_CTRL_PLL_CTL_1,
		BCHP_USB_CTRL_PLL_CTL_1_PLL_PWRDWNB_MASK |
		BCHP_USB_CTRL_PLL_CTL_1_XTAL_PWRDWNB_MASK);
	BDEV_RD(BCHP_USB_CTRL_PLL_CTL_1);

	BDEV_UNSET(BCHP_USB_CTRL_UTMI_CTL_1,
		BCHP_USB_CTRL_UTMI_CTL_1_PHY_PWDNB_MASK |
		BCHP_USB_CTRL_UTMI_CTL_1_PHY1_PWDNB_MASK);
	BDEV_RD(BCHP_USB_CTRL_UTMI_CTL_1);

	BDEV_SET(BCHP_CLK_PM_CTRL, BCHP_CLK_PM_CTRL_DIS_USB_108M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL);

#if defined(BCHP_CLK_PM_CTRL_2_DIS_USB_216M_CLK_MASK)
	BDEV_SET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_USB_216M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);
#endif
}

static void enet_enable(void)
{
	printk(KERN_INFO "brcm-pm: enabling power to ENET block\n");

#if defined(BCHP_CLK_PM_CTRL_2_DIS_ENET_216M_CLK_MASK)
	BDEV_UNSET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_ENET_216M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);
#endif

	BDEV_UNSET(BCHP_CLK_PM_CTRL, BCHP_CLK_PM_CTRL_DIS_ENET_108M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL);

	BDEV_UNSET(BCHP_CLK_PM_CTRL_1,
		BCHP_CLK_PM_CTRL_1_DIS_ENET_25M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_1);
}

static void enet_disable(void)
{
	printk(KERN_INFO "brcm-pm: disabling power to ENET block\n");

	BDEV_SET(BCHP_CLK_PM_CTRL_1,
		BCHP_CLK_PM_CTRL_1_DIS_ENET_25M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_1);

	BDEV_SET(BCHP_CLK_PM_CTRL, BCHP_CLK_PM_CTRL_DIS_ENET_108M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL);

#if defined(BCHP_CLK_PM_CTRL_2_DIS_ENET_216M_CLK_MASK)
	BDEV_SET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_ENET_216M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);
#endif

}

static void sata_enable(void)
{
	printk(KERN_INFO "brcm-pm: enabling power to SATA block\n");

	BDEV_UNSET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_SATA_PCI_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);

#if defined(BCHP_CLK_PM_CTRL_2_DIS_SATA_216M_CLK_MASK)
	BDEV_UNSET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_SATA_216M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);
#endif

	BDEV_UNSET(BCHP_CLK_PM_CTRL, BCHP_CLK_PM_CTRL_DIS_SATA_108M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL);

	BDEV_UNSET(BCHP_SUN_TOP_CTRL_GENERAL_CTRL_1,
		BCHP_SUN_TOP_CTRL_GENERAL_CTRL_1_sata_ana_pwrdn_MASK);
	BDEV_RD(BCHP_SUN_TOP_CTRL_GENERAL_CTRL_1);
}

static void sata_disable(void)
{
	printk(KERN_INFO "brcm-pm: disabling power to SATA block\n");

	BDEV_SET(BCHP_SUN_TOP_CTRL_GENERAL_CTRL_1,
		BCHP_SUN_TOP_CTRL_GENERAL_CTRL_1_sata_ana_pwrdn_MASK);
	BDEV_RD(BCHP_SUN_TOP_CTRL_GENERAL_CTRL_1);

	BDEV_SET(BCHP_CLK_PM_CTRL, BCHP_CLK_PM_CTRL_DIS_SATA_108M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL);

	BDEV_SET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_SATA_PCI_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);

#if defined(BCHP_CLK_PM_CTRL_2_DIS_SATA_216M_CLK_MASK)
	BDEV_SET(BCHP_CLK_PM_CTRL_2,
		BCHP_CLK_PM_CTRL_2_DIS_SATA_216M_CLK_MASK);
	BDEV_RD(BCHP_CLK_PM_CTRL_2);
#endif

}

/*
 * /sys/devices/platform/brcm-pm/ddr:
 *
 * 0  = no power management
 * >0 = enter self-refresh mode after <val> DDR clocks of inactivity
 */

static uint32_t ddr_get(void)
{
	uint32_t reg, inact_cnt, pdn_en, val;

	reg = BDEV_RD(BCHP_MEMC_0_DDR_POWER_DOWN_MODE);
	pdn_en = (reg & BCHP_MEMC_0_DDR_POWER_DOWN_MODE_PDN_EN_MASK) >>
		BCHP_MEMC_0_DDR_POWER_DOWN_MODE_PDN_EN_SHIFT;
	inact_cnt = (reg & BCHP_MEMC_0_DDR_POWER_DOWN_MODE_INACT_CNT_MASK) >>
		BCHP_MEMC_0_DDR_POWER_DOWN_MODE_INACT_CNT_SHIFT;
	val = pdn_en ? inact_cnt : 0;
	return(val);
}

static void ddr_set(uint32_t val)
{
	if(! val) {
		BDEV_WR(BCHP_MEMC_0_DDR_POWER_DOWN_MODE, 0);
	} else {
		BDEV_WR(BCHP_MEMC_0_DDR_POWER_DOWN_MODE,
			BCHP_MEMC_0_DDR_POWER_DOWN_MODE_PDN_EN_MASK |
			BCHP_MEMC_0_DDR_POWER_DOWN_MODE_PDN_MODE_MASK |
			((val << BCHP_MEMC_0_DDR_POWER_DOWN_MODE_INACT_CNT_SHIFT)
			& BCHP_MEMC_0_DDR_POWER_DOWN_MODE_INACT_CNT_MASK));
	}
}

/*
 * API FOR DRIVERS
 */

void brcm_pm_usb_add(void)
{
	if(atomic_inc_return(&usb_count) == 1)
		usb_enable();
}
EXPORT_SYMBOL(brcm_pm_usb_add);

void brcm_pm_usb_remove(void)
{
	if(atomic_dec_return(&usb_count) == 0)
		usb_disable();
}
EXPORT_SYMBOL(brcm_pm_usb_remove);

void brcm_pm_enet_add(void)
{
	if(atomic_inc_return(&enet_count) == 1)
		enet_enable();
}
EXPORT_SYMBOL(brcm_pm_enet_add);

void brcm_pm_enet_remove(void)
{
	if(atomic_dec_return(&enet_count) == 0)
		enet_disable();
}
EXPORT_SYMBOL(brcm_pm_enet_remove);

void brcm_pm_sata_add(void)
{
	if(atomic_inc_return(&sata_count) == 1)
		sata_enable();
}
EXPORT_SYMBOL(brcm_pm_sata_add);

void brcm_pm_sata_remove(void)
{
	if(atomic_dec_return(&sata_count) == 0)
		sata_disable();
}
EXPORT_SYMBOL(brcm_pm_sata_remove);

#define DECLARE_PM_REGISTER(func, name) \
	void func(int (*off_cb)(void *), int (*on_cb)(void *), void *arg) \
	{ \
		BUG_ON(name##_on_cb || name##_off_cb); \
		name##_on_cb = on_cb; \
		name##_off_cb = off_cb; \
		name##_cb_arg = arg; \
	} \
	EXPORT_SYMBOL(func)

#define DECLARE_PM_UNREGISTER(func, name) \
	void func(void) \
	{ \
		BUG_ON(! name##_on_cb || ! name##_off_cb); \
		name##_on_cb = name##_off_cb = NULL; \
	} \
	EXPORT_SYMBOL(func)

DECLARE_PM_REGISTER(brcm_pm_register_sata, sata);
DECLARE_PM_UNREGISTER(brcm_pm_unregister_sata, sata);

DECLARE_PM_REGISTER(brcm_pm_register_enet, enet);
DECLARE_PM_UNREGISTER(brcm_pm_unregister_enet, enet);

DECLARE_PM_REGISTER(brcm_pm_register_ehci, ehci);
DECLARE_PM_UNREGISTER(brcm_pm_unregister_ehci, ehci);

DECLARE_PM_REGISTER(brcm_pm_register_ohci, ohci);
DECLARE_PM_UNREGISTER(brcm_pm_unregister_ohci, ohci);


/*
 * API FOR USER PROGRAMS
 */

#define DECLARE_PM_SHOW(_func, _expr) \
	static ssize_t _func(struct device *dev, \
		struct device_attribute *attr, char *buf) \
		{ \
			return(sprintf(buf, "%d\n", (_expr))); \
		}

DECLARE_PM_SHOW(usb_show, atomic_read(&usb_count) ? 1 : 0);
DECLARE_PM_SHOW(enet_show, atomic_read(&enet_count));
DECLARE_PM_SHOW(sata_show, atomic_read(&sata_count));
DECLARE_PM_SHOW(ddr_show, ddr_get());

static ssize_t ddr_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	ddr_set(simple_strtoul(buf, NULL, 0));
	return(n);
}

static ssize_t sata_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	if(! sata_off_cb || ! sata_on_cb) {
		printk(KERN_WARNING "brcm-pm: SATA callback not registered\n");
		return(n);
	}

	if(simple_strtoul(buf, NULL, 0) == 0)
		sata_off_cb(sata_cb_arg);
	else
		sata_on_cb(sata_cb_arg);

	return(n);
}

static ssize_t enet_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	if(! enet_off_cb || ! enet_on_cb) {
		printk(KERN_WARNING "brcm-pm: ENET callback not registered\n");
		return(n);
	}

	if(simple_strtoul(buf, NULL, 0) == 0)
		enet_off_cb(enet_cb_arg);
	else
		enet_on_cb(enet_cb_arg);

	return(n);
}

static ssize_t usb_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	if((! ehci_off_cb || ! ehci_on_cb) &&
	   (! ohci_off_cb || ! ohci_on_cb)) {
		printk(KERN_WARNING "brcm-pm: USB callback not registered\n");
		return(n);
	}

	if(simple_strtoul(buf, NULL, 0) == 0) {
		if(ehci_off_cb)
			ehci_off_cb(ehci_cb_arg);
		if(ohci_off_cb)
			ohci_off_cb(ohci_cb_arg);
	} else {
		if(ehci_on_cb)
			ehci_on_cb(ehci_cb_arg);
		if(ohci_on_cb)
			ohci_on_cb(ohci_cb_arg);
	}

	return(n);
}

DEVICE_ATTR(usb,0644,usb_show,usb_store);
DEVICE_ATTR(enet,0644,enet_show,enet_store);
DEVICE_ATTR(sata,0644,sata_show,sata_store);
DEVICE_ATTR(ddr,0644,ddr_show,ddr_store);

static struct platform_driver brcm_pm_platform_driver = {
	.driver		= {
		.name	= "brcm-pm",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device *brcm_pm_platform_device;

static int __init brcm_pm_init(void)
{
	int ret;
	struct platform_device *dev;
	struct device *cdev;

	/* power down all devices if nobody is using them */
	brcm_pm_usb_remove();
	brcm_pm_enet_remove();
	brcm_pm_sata_remove();

	ret = platform_driver_register(&brcm_pm_platform_driver);
	if(ret) {
		printk(KERN_ERR
			"brcm-pm: unable to register platform driver\n");
		goto bad;
	}

	brcm_pm_platform_device = dev = platform_device_alloc("brcm-pm", -1);
	if(! dev) {
		printk("brcm-pm: unable to allocate platform device\n");
		ret = -ENODEV;
		goto bad2;
	}

	ret = platform_device_add(dev);
	if(ret) {
		printk("brcm-pm: unable to add platform device\n");
		goto bad3;
		return(-ENODEV);
	}

	cdev = &dev->dev;
	device_create_file(cdev, &dev_attr_usb);
	device_create_file(cdev, &dev_attr_enet);
	device_create_file(cdev, &dev_attr_sata);
	device_create_file(cdev, &dev_attr_ddr);

	return(0);

bad3:
	platform_device_put(dev);
bad2:
	platform_driver_unregister(&brcm_pm_platform_driver);
bad:
	return(ret);
}

static void __exit brcm_pm_exit(void)
{
	struct platform_device *dev = brcm_pm_platform_device;
	struct device *cdev = &dev->dev;

	device_remove_file(cdev, &dev_attr_usb);
	device_remove_file(cdev, &dev_attr_enet);
	device_remove_file(cdev, &dev_attr_sata);
	device_remove_file(cdev, &dev_attr_ddr);

	platform_device_unregister(brcm_pm_platform_device);
	platform_driver_unregister(&brcm_pm_platform_driver);

	/* power up all devices, then exit */
	brcm_pm_usb_add();
	brcm_pm_enet_add();
	brcm_pm_sata_add();
	ddr_set(0);
}

module_init(brcm_pm_init);
module_exit(brcm_pm_exit);
