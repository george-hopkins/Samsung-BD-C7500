/*  linux/drivers/mmc/host/brcm_sdhci.c - Broadcom SD Controller Driver
 *
 *     Copyright (c) 1999-2009, Broadcom Corporation
 *     All Rights Reserved
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed with the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Thanks to Pierre Ossman for guiding.
 *
 */

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/mmc/host.h>
#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <asm/brcmstb/common/brcmstb.h>


#ifdef CONFIG_MIPS_BCM7635
#include <asm/brcmstb/brcm97635/bchp_vcxo_ctl_misc.h>
#include <asm/brcmstb/brcm97635/bchp_sdio.h>
#include <asm/brcmstb/brcm97635/bchp_hif_intr2.h>
#elif CONFIG_MIPS_BCM7630
#include <asm/brcmstb/brcm97630/bchp_sdio.h>
#else
#error -- platform not defined
#endif // CONFIG_MIPS_BCM7635/CONFIG_MIPS_BCM7630

#include <linux/stat.h>

#include "sdhci.h"

#define DRV_DEV_NAME "brcm_sdhci"
typedef volatile unsigned int * ioAddr;

#ifdef CONFIG_MIPS_BCM7635
/* this addresses PR 2847, by which we need to read a 0 yielding value 
 register, to avoid poluting other HIF block registers. see PR 2847. */
extern unsigned int readl_pr_2847(void __iomem * addr);
extern unsigned int readw_pr_2847(void __iomem * addr);
extern unsigned int readb_pr_2847(void __iomem * addr);

#define readl readl_pr_2847
#define readw readw_pr_2847
#define readb readb_pr_2847    

#endif // CONFIG_MIPS_BCM7635

typedef struct brcm_sdhci_private_s {
	struct platform_device * pdev;
} brcm_sdhci_private_t, * brcm_sdhci_private_p;

static struct platform_device brcm_sdhci_device = {
	.name = DRV_DEV_NAME,
	.id = -1, // to indicate there's only one. 
};

static int brcm_sdhci_enable_dma(struct sdhci_host *host);
static struct sdhci_ops brcm_sdhci_ops = {
	.enable_dma	= brcm_sdhci_enable_dma,
};

// this is for attribute read/write
#define BRCM_SDHCI_MAX_ATTR_BUF (256)
struct sysfs_ops * brcm_kobj_sysfs_ops = NULL;

static struct sysfs_ops * brcm_sdhci_get_sysfs_ops (struct kobject * kobj) {

	struct kobj_type * kobj_type_p = NULL;
	
	BUG_ON(kobj == NULL);
	kobj_type_p = get_ktype(kobj);
	BUG_ON(kobj_type_p == NULL);	
	return brcm_kobj_sysfs_ops = kobj_type_p->sysfs_ops;
	
}

static void brcm_sdhci_set_sysfs_ops (struct kobject * kobj, struct sysfs_ops * sysfs_ops) {

	struct kobj_type * kobj_type_p = NULL;
	
	BUG_ON(kobj == NULL);
	kobj_type_p = get_ktype(kobj);
	BUG_ON(kobj_type_p == NULL);	
	kobj_type_p->sysfs_ops = sysfs_ops;
	
}

typedef struct brcm_sdhci_attribute_s {
	u32 reg_offset;
	struct attribute attr;
} brcm_sdhci_attribute_t, * brcm_sdhci_attribute_p;

#define BRCM_NUM_REG_ATTR (28)

static brcm_sdhci_attribute_t attr_reg_table[BRCM_NUM_REG_ATTR] = {
{
	.reg_offset = BCHP_SDIO_SDMA,
	.attr = {
		.name = "attr_reg_sdma",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_BLOCK,
	.attr = {
		.name = "attr_reg_block",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_ARGUMENT,
	.attr = {
		.name = "attr_reg_argument",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_CMD_MODE,
	.attr = {
		.name = "attr_reg_cmd_mode",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_RESP_01,
	.attr = {
		.name = "attr_reg_resp_01",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_RESP_23,
	.attr = {
		.name = "attr_reg_resp_23",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_RESP_45,
	.attr = {
		.name = "attr_reg_resp_45",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_RESP_67,
	.attr = {
		.name = "attr_reg_resp_67",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_BUFFDATA,
	.attr = {
		.name = "attr_reg_buffdata",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_STATE,
	.attr = {
		.name = "attr_reg_state",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_CTRL_SET0,
	.attr = {
		.name = "attr_reg_ctrl_set0",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_CTRL_SET1,
	.attr = {
		.name = "attr_reg_ctrl_set1",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_INT_STATUS,
	.attr = {
		.name = "attr_reg_int_status",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_INT_STATUS_ENA,
	.attr = {
		.name = "attr_reg_int_status_ena",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_INT_SIGNAL_ENA,
	.attr = {
		.name = "attr_reg_int_signal_ena",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_AUTOCMD12_STAT,
	.attr = {
		.name = "attr_reg_autocmd12_stat",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_CAPABLE,
	.attr = {
		.name = "attr_reg_capable",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_CAPABLE_RSVD,
	.attr = {
		.name = "attr_reg_capable_rsvd",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_POWER_CAPABLE,
	.attr = {
		.name = "attr_reg_power_capable",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_POWER_CAPABLE_RSVD,
	.attr = {
		.name = "attr_reg_power_capable_rsvd",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_FORCE_EVENTS,
	.attr = {
		.name = "attr_reg_force_events",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_ADMA_ERR_STAT,
	.attr = {
		.name = "attr_reg_adma_err_stat",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_ADMA_SYSADDR_LO,
	.attr = {
		.name = "attr_reg_adma_sysaddr_lo",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_ADMA_SYSADDR_HI,
	.attr = {
		.name = "attr_reg_adma_sysaddr_hi",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_BOOT_TIMEOUT,
	.attr = {
		.name = "attr_reg_boot_timeout",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_DEBUG_SELECT,
	.attr = {
		.name = "attr_reg_debug_select",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_SPI_INTERRUPT,
	.attr = {
		.name = "attr_reg_spi_interrupt",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
},
{
	.reg_offset = BCHP_SDIO_VERSION_STATUS,
	.attr = {
		.name = "attr_reg_version_status",
		.owner = THIS_MODULE,
		.mode = S_IWUSR | S_IRUGO,
	},
}
};

static ssize_t brcm_sdhci_show_attr(struct kobject * kobj, struct attribute * attribute, char * buffer){

	u32 * reg_addr = NULL;
	u32 reg_val = 0xdeadbeef;
        
	brcm_sdhci_attribute_p sdhci_attribute = 
            container_of(attribute, struct brcm_sdhci_attribute_s, attr);
	
	if ((sdhci_attribute->reg_offset < BCHP_SDIO_REG_START) || 
		(sdhci_attribute->reg_offset > BCHP_SDIO_REG_END)) 
		return (ssize_t)-ENXIO;

	reg_addr = (u32 *) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + sdhci_attribute->reg_offset);
                
	reg_val = readl(reg_addr);
		        
	return snprintf(buffer, PAGE_SIZE, "%08x\n", reg_val);
	
}

static ssize_t brcm_sdhci_store_attr(struct kobject * kobj, struct attribute * attribute, 
        const char * buffer, size_t size){

	ioAddr reg_addr = NULL;
	u32 reg_val = 0xdeadbeef;

	brcm_sdhci_attribute_p sdhci_attribute = 
            container_of(attribute, struct brcm_sdhci_attribute_s, attr);

        if (!capable(CAP_SYS_ADMIN)) return -EPERM;
        
	if ((sdhci_attribute->reg_offset < BCHP_SDIO_REG_START) || 
		(sdhci_attribute->reg_offset > BCHP_SDIO_REG_END)) 
		return (ssize_t)-ENXIO;
        
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + sdhci_attribute->reg_offset);       
                
	reg_val = simple_strtoul(buffer, NULL, 0x10);
                
        *reg_addr = reg_val;
        
	return sizeof(u32);
	
}

static int brcm_sdhci_remove_attributes (struct kobject * kobj, int reg_attr){
    
    int ret = 0;
    int idx = BRCM_NUM_REG_ATTR - 1;
    
    if (kobj == NULL) {
            return -EINVAL;
    }
    
    if ((reg_attr < 0) || (reg_attr >= BRCM_NUM_REG_ATTR)) 
            return -EINVAL;
    
    for (idx = reg_attr; idx >= 0; idx--) {
        sysfs_remove_file(kobj, &(attr_reg_table[idx].attr));    
    }

    return ret;
    
}

static int brcm_sdhci_create_attributes (struct kobject * kobj){

    int ret = 0;
    int idx = 0;
    
    for (idx = 0; idx < BRCM_NUM_REG_ATTR; idx++) {
        ret = sysfs_create_file(kobj, &(attr_reg_table[idx].attr));
        if (ret) {
            if (brcm_sdhci_remove_attributes(kobj, idx - 1)){
                dev_warn(&brcm_sdhci_device.dev, "brcm_sdhci_remove_attributes returns error.\n");
            }
            break;
        } else {
            dev_dbg(&brcm_sdhci_device.dev, "Succesfully registered %s.\n", 
                attr_reg_table[idx].attr.name);

        }
    }
    
    return ret;

}

// this is our customized sysfs_ops struct.
static struct sysfs_ops brcm_sdhci_sysfs_ops = {
	.show = brcm_sdhci_show_attr,
	.store = brcm_sdhci_store_attr
};

// the device_release method.
void brcm_sdhci_dev_release(struct device * dev){

	dev_dbg(&brcm_sdhci_device.dev, "Succesfully released %s.\n", DRV_DEV_NAME);
	
}

static int brcm_sdhci_pinmux(void)
{

	ioAddr reg_addr = NULL;
	u32 reg_val = 0x00000000;

#ifdef CONFIG_MIPS_BCM7635	
	/*
	enable SDIO lines via pinmux registers (choose SDIO vs GPIO)
	GPIO 42:36 mapped to SUN_TOP_CTRL_PIN_MUX_CTRL_12[31:4]
	GPIO 44:43 mapped to SUN_TOP_CTRL_PIN_MUX_CTRL_13[7:0]
	*/
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + 
		BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12);
	reg_val = *reg_addr; 
        reg_val &= BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_35_MASK; 
        reg_val |= 
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_42_SDIO_DAT1 << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_42_SHIFT) |
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_41_SDIO_DAT0 << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_41_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_40_SDIO_CMD << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_40_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_39_SDIO_CLK << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_39_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_38_SDIO_PRES << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_38_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_37_SDIO_WPROT << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_37_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_36_SDIO_PWR << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_36_SHIFT);
	*reg_addr = reg_val;
        
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + 
		BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13);
	reg_val = *reg_addr; 
        reg_val &= 
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_50_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_49_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_48_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_47_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_46_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_45_MASK; 
        reg_val |= 
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_44_SDIO_DAT3 << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_44_SHIFT)|
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_43_SDIO_DAT2 << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_43_SHIFT);
	*reg_addr = reg_val;
#elif defined(CONFIG_MIPS_BCM7630)
	/*
	enable SDIO lines via pinmux registers (choose SDIO vs GPIO)
	GPIO 39:36 mapped to SUN_TOP_CTRL_PIN_MUX_CTRL_11[31:16]
	GPIO 44:40 mapped to SUN_TOP_CTRL_PIN_MUX_CTRL_12[19:0]
	*/
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + 
		BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11);
	reg_val = *reg_addr; 
        reg_val &= 
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_32_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_33_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_34_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_35_MASK;
        reg_val |= 
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_39_SDIO_CLK << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_39_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_38_SDIO_PRES << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_38_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_37_SDIO_WPROT << 
                BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_37_SHIFT)|
            (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_36_SDIO_PWR << 
            	BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11_gpio_36_SHIFT);
	*reg_addr = reg_val;
        
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + 
		BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12);
	reg_val = *reg_addr; 
        reg_val &= 
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_47_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_46_MASK |
        BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_45_MASK; 
        reg_val |= 
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_44_SDIO_DAT3 << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_44_SHIFT)|
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_43_SDIO_DAT2 << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_43_SHIFT)|	
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_42_SDIO_DAT1 << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_42_SHIFT) |
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_41_SDIO_DAT0 << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_41_SHIFT)|
        (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_40_SDIO_CMD << 
            BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_12_gpio_40_SHIFT);    
	*reg_addr = reg_val;	
#else
#error -- platform not defined
	return -ENODEV;
#endif // CONFIG_MIPS_BCM7635/CONFIG_MIPS_BCM7630	
	
	mmiowb();
        
	return 0;
	
}

/*****************************************************************************\
 *                                                                           *
 * SDHCI core callbacks                                                      *
 *                                                                           *
\*****************************************************************************/

static int brcm_sdhci_enable_dma(struct sdhci_host *host)
{

	brcm_sdhci_private_p brcm_sdhci_private = NULL;
	struct platform_device * pdev = NULL;
	
	brcm_sdhci_private = sdhci_priv(host);
	pdev = brcm_sdhci_private->pdev;

	if (host->flags & SDHCI_USE_DMA) {
		dev_warn(&pdev->dev, "DMA(capable) already enabled.\n");
	}

	return 0;
	
}

/*****************************************************************************\
 *                                                                           *
 * Device probing/removal                                                    *
 *                                                                           *
\*****************************************************************************/

static int __devinit brcm_sdhci_probe(struct platform_device *pdev)
{
	struct sdhci_host *host = NULL;
	brcm_sdhci_private_p brcm_sdhci_private = NULL;
	int ret;
	ioAddr reg_addr = NULL;
	u32 reg_val = 0x00000000;
	
	BUG_ON(pdev == NULL);
        
        ret = brcm_sdhci_pinmux();
	if (ret) {
		goto bail_out;
	}        
		
	host = sdhci_alloc_host(&pdev->dev, sizeof(brcm_sdhci_private_t));
	if (IS_ERR(host)) {
		ret = -ENOMEM;
		goto bail_out;
	}
        
	brcm_sdhci_private = sdhci_priv(host);
	BUG_ON(brcm_sdhci_private == NULL);
	brcm_sdhci_private->pdev = pdev;

	host->hw_name = DRV_DEV_NAME;
	host->ops = &brcm_sdhci_ops;
	host->flags = 0x00000000;
	host->quirks = 0x00000000;	
        // here we workaround our controller's limitations.
	//host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
	//host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
	host->quirks |= SDHCI_QUIRK_BROKEN_TIMEOUT_VAL;
	
        // mask the interrupt, so we do not get a spurious
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_HIF_INTR2_CPU_MASK_SET);
        *reg_addr = BCHP_HIF_INTR2_CPU_MASK_SET_SDIO_INTR_MASK;
	// clear it.
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_HIF_INTR2_CPU_CLEAR);
        *reg_addr = BCHP_HIF_INTR2_CPU_CLEAR_SDIO_INTR_MASK;
			
	host->irq = BCM_LINUX_SDIO_IRQ; 
	
	host->ioaddr = (void __iomem *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SDIO_REG_START);
	dev_dbg(&brcm_sdhci_device.dev, "%s, ioaddr %px.\n", DRV_DEV_NAME, host->ioaddr);
	
	ret = sdhci_add_host(host);
	if (ret)
		goto release;        
							
	// enable the interrupt, the handler is installed
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_HIF_INTR2_CPU_MASK_CLEAR);
        *reg_addr = BCHP_HIF_INTR2_CPU_MASK_CLEAR_SDIO_INTR_MASK;
	
	ret = platform_device_add_data(pdev, host, sizeof(struct sdhci_host));
	if (ret)
		goto remove_host;
			
        // device attribute creation is broken. disable it for now.
#if 0	
        brcm_kobj_sysfs_ops = brcm_sdhci_get_sysfs_ops (&pdev->dev.kobj);	
	brcm_sdhci_set_sysfs_ops (&pdev->dev.kobj, &brcm_sdhci_sysfs_ops);
	ret = brcm_sdhci_create_attributes(&pdev->dev.kobj);
	if (ret){
                dev_warn(&pdev->dev, "brcm_sdhci_create_attributes returns %d.\n", ret);
		goto remove_host;                
        }
#endif
			
	dev_dbg(&brcm_sdhci_device.dev, "Succesfully probed %s.\n", DRV_DEV_NAME);
		
	return 0;
		
remove_host:
	sdhci_remove_host(host, 0);	
	
release:
	sdhci_free_host(host);
	
bail_out:
	// mask the interrupt, so we do not get a spurious
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_HIF_INTR2_CPU_MASK_SET);
        *reg_addr = BCHP_HIF_INTR2_CPU_MASK_SET_SDIO_INTR_MASK;
        
	// clear it.
	reg_addr = (ioAddr) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_HIF_INTR2_CPU_CLEAR);
        *reg_addr = BCHP_HIF_INTR2_CPU_CLEAR_SDIO_INTR_MASK;

	return ret;

}

static int __devexit brcm_sdhci_remove(struct platform_device *pdev)
{
        int ret = 0;
	struct sdhci_host *host = NULL;
	
	BUG_ON(pdev == NULL);
	host = pdev->dev.platform_data;
	BUG_ON(pdev == NULL);
	sdhci_remove_host(host, 0);
	sdhci_free_host(host);	
        
        // device attribute creation is broken. disable it for now.
#if 0        
	brcm_sdhci_set_sysfs_ops (&pdev->dev.kobj, brcm_kobj_sysfs_ops);
	ret = brcm_sdhci_remove_attributes(&pdev->dev.kobj, BRCM_NUM_REG_ATTR - 1);
	if (ret){
                dev_warn(&pdev->dev, "brcm_sdhci_remove_attributes returns %d.\n", ret);
		return ret;                
        }
#endif
	
	dev_dbg(&brcm_sdhci_device.dev, "Succesfully removed %s.\n", DRV_DEV_NAME);
	
	return ret;	
	
}

#ifdef CONFIG_PM
	static int brcm_sdhci_platform_suspend(struct platform_device * pdev, pm_message_t state){
		return 0;
	}
	static int brcm_sdhci_platform_resume(struct platform_device * pdev){
		return 0;
	}
#else
	#define brcm_sdhci_platform_suspend NULL
	#define brcm_sdhci_platform_resume NULL
#endif

static struct platform_driver brcm_sdhci_driver = {
	.probe		= brcm_sdhci_probe,
	.remove		= brcm_sdhci_remove,
	.suspend	= brcm_sdhci_platform_suspend,
	.resume		= brcm_sdhci_platform_resume,
	.driver		= {
		.name	= DRV_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

/*****************************************************************************\
 *                                                                           *
 * Driver init/exit                                                          *
 *                                                                           *
\*****************************************************************************/

static int __init brcm_sdhci_drv_init(void)
{
	int result;
	
	printk(KERN_INFO ": %s driver\n", DRV_DEV_NAME);
	
	result = platform_driver_register(&brcm_sdhci_driver);
	if (result < 0)
		return result;
	
	brcm_sdhci_device.dev.release = brcm_sdhci_dev_release;
	result = platform_device_register(&brcm_sdhci_device);
	if (result < 0) {
		goto drv_unregister;
	}

	dev_dbg(&brcm_sdhci_device.dev, "Succesfully registered %s.\n", DRV_DEV_NAME);
	
	return 0;
		
drv_unregister:	
	platform_driver_unregister(&brcm_sdhci_driver);
	
	return result;
}

static void __exit brcm_sdhci_drv_exit(void)
{
	
	platform_device_unregister(&brcm_sdhci_device);
	platform_driver_unregister(&brcm_sdhci_driver);
	
	dev_dbg(&brcm_sdhci_device.dev, "Succesfully unregistered %s.\n", DRV_DEV_NAME);
	
}

module_init(brcm_sdhci_drv_init);
module_exit(brcm_sdhci_drv_exit);

MODULE_AUTHOR("Ovidiu Nastai <onastai@broadcom.com>");
MODULE_DESCRIPTION("Broadcom Secure Digital Host Controller Interface driver");
MODULE_LICENSE("GPL");
