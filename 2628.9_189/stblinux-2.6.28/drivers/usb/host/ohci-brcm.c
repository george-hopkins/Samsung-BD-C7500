/*
 * Copyright (C) 2009 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* force non-byteswapping reads/writes on LE and BE alike */
#define CONFIG_USB_OHCI_BIG_ENDIAN_MMIO		1
#define readl_be(x)				__raw_readl(x)
#define writel_be(x, y)				__raw_writel(x, y)

#include <linux/platform_device.h>
#include <linux/autoconf.h>
#include <linux/mmdebug.h>
#include <asm/irq.h>
#include <asm/brcmstb/common/brcmstb.h>
#define OHCI_BRCM
#include "brcmusb.h"

static struct platform_driver	ohci_hcd_brcm_driver;
#define PLATFORM_DRIVER		ohci_hcd_brcm_driver

/* for global USB settings, see arch/mips/brcmstb/bchip.c */

static int ohci_brcm_reset(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	ohci->flags |= OHCI_QUIRK_BE_MMIO;
	ohci_hcd_init(ohci);
	return ohci_init(ohci);
}

static int ohci_brcm_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	struct ohci_regs __iomem *regs;

	regs = hcd->regs;

	ohci_writel(ohci, 1, &regs->cmdstatus);
	ohci_readl(ohci, &regs->cmdstatus);
	mdelay(10);

	ohci_hcd_init(ohci);
	ohci_init(ohci);
	ohci_run(ohci);
	hcd->state = HC_STATE_RUNNING;
	return 0;
}

#ifdef CONFIG_PM
int ohci_power(void *ohci, int is_on)
{
	struct ohci_hcd *ohcip = (struct ohci_hcd *)ohci;
	int port;
	int temp;

	temp = roothub_a(ohcip);

	/* If NoPowerSwitching is true, just return */
	if (temp & RH_A_NPS) {
		return 0;
	}

	/* If port switched, switch each port */
	if (temp & RH_A_PSM) {
		for (port = ohcip->num_ports; port > 0; ) {
			(void) ohci_hub_control(ohci_to_hcd(ohcip),
									is_on ? SetPortFeature : ClearPortFeature,
									USB_PORT_FEAT_POWER,
									port--, NULL, 0);
		}
	}
	/* Else use Global Power */
	else {
		temp = roothub_status(ohcip);
		if (is_on) {
			temp |= RH_HS_LPSC;	 /* SetGlobalPower */
		}
		else {
			temp |= RH_HS_LPS;	 /* ClearGlobalPower */
		}
		ohci_writel(ohcip, temp, &ohcip->regs->roothub.status);
	}
	return 0;
}

#endif  /* CONFIG_PM */


static const struct hc_driver ohci_brcm_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"Broadcom STB OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.reset =		ohci_brcm_reset,
	.start =		ohci_brcm_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif  /*CONFIG_PM */
	.start_port_reset =	ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/

#define resource_len(r) (((r)->end - (r)->start) + 1)
static int ohci_hcd_brcm_probe(struct platform_device *pdev)
{
	struct resource *res = NULL;
	struct usb_hcd *hcd = NULL;
	int irq = -1;
	int ret;

	if (usb_disabled())
		return -ENODEV;

	/* clear this to enable Port Power management */
	distrust_firmware = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err("platform_get_resource error.");
		return -ENODEV;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		err("platform_get_irq error.");
		return -ENODEV;
	}

#ifndef CONFIG_USB_EHCI_HCD
	// Init common registers for board and HC specific issues
	brcm_usb_init();
#endif

	/* initialize hcd */
	hcd = usb_create_hcd(&ohci_brcm_hc_driver, &pdev->dev, (char *)hcd_name);
	if (!hcd) {
		err("Failed to create hcd");
		return -ENOMEM;
	}

	hcd->regs = ioremap(res->start, res->end - res->start);
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_len(res);
	ret = usb_add_hcd(hcd, irq, IRQF_DISABLED);
	if (ret != 0) {
		err("Failed to add hcd");
		usb_put_hcd(hcd);
		iounmap(hcd->regs);
		return ret;
	}

	return ret;
}

static int ohci_hcd_brcm_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);
	iounmap(hcd->regs);

	return 0;
}

static struct platform_driver ohci_hcd_brcm_driver = {
	.probe		= ohci_hcd_brcm_probe,
	.remove		= ohci_hcd_brcm_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.name	= "ohci-brcm",
		.owner	= THIS_MODULE,
	},
};

MODULE_ALIAS("ohci-brcm");
