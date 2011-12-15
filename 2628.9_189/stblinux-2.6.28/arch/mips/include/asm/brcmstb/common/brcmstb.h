/*
 * arch/mips/brcm/setup.c
 *
 * Copyright (C) 2001 Broadcom Corporation
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
 *
 * Setup for Broadcom eval boards
 *
 * 10-01-2003   THT    Created
 */


#ifndef _BRCMSTB_H
#define _BRCMSTB_H



#include <linux/autoconf.h>

#define BRCM_ENDIAN_LITTLE CONFIG_CPU_LITTLE_ENDIAN


#if defined(CONFIG_MIPS_BCM7405A0)
#include <asm/brcmstb/brcm97405a0/bcmuart.h>
#include <asm/brcmstb/brcm97405a0/bcmtimer.h>
#include <asm/brcmstb/brcm97405a0/bcmebi.h>
#include <asm/brcmstb/brcm97405a0/int1.h>
#include <asm/brcmstb/brcm97405a0/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97405a0/board.h>
#include <asm/brcmstb/brcm97405a0/bchp_irq0.h>
#include <asm/brcmstb/brcm97405a0/bcmintrnum.h>
#include <asm/brcmstb/brcm97405a0/bchp_nand.h>
#include <asm/brcmstb/brcm97405a0/bchp_ebi.h>
#include <asm/brcmstb/brcm97405a0/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97405a0/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97405a0/bchp_usb_ehci.h>
#include <asm/brcmstb/brcm97405a0/bchp_usb_ehci1.h>
#include <asm/brcmstb/brcm97405a0/bchp_usb_ohci.h>
#include <asm/brcmstb/brcm97405a0/bchp_usb_ohci1.h>
#include <asm/brcmstb/brcm97405a0/bchp_pcix_bridge.h>
#include <asm/brcmstb/brcm97405a0/bchp_clk.h>
#include <asm/brcmstb/brcm97405a0/bchp_memc_0_ddr.h>

#elif defined(CONFIG_MIPS_BCM7405B0)
#include <asm/brcmstb/brcm97405b0/bcmuart.h>
#include <asm/brcmstb/brcm97405b0/bcmtimer.h>
#include <asm/brcmstb/brcm97405b0/bcmebi.h>
#include <asm/brcmstb/brcm97405b0/int1.h>
#include <asm/brcmstb/brcm97405b0/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97405b0/board.h>
#include <asm/brcmstb/brcm97405b0/bchp_irq0.h>
#include <asm/brcmstb/brcm97405b0/bcmintrnum.h>
#include <asm/brcmstb/brcm97405b0/bchp_nand.h>
#include <asm/brcmstb/brcm97405b0/bchp_ebi.h>
#include <asm/brcmstb/brcm97405b0/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97405b0/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97405b0/bchp_usb_ehci.h>
#include <asm/brcmstb/brcm97405b0/bchp_usb_ehci1.h>
#include <asm/brcmstb/brcm97405b0/bchp_usb_ohci.h>
#include <asm/brcmstb/brcm97405b0/bchp_usb_ohci1.h>
#include <asm/brcmstb/brcm97405b0/bchp_pcix_bridge.h>
#include <asm/brcmstb/brcm97405b0/bchp_clk.h>
#include <asm/brcmstb/brcm97405b0/bchp_bmips4380.h>
#include <asm/brcmstb/brcm97405b0/bchp_memc_0_ddr.h>

#elif defined(CONFIG_MIPS_BCM7440B0)
#include <asm/brcmstb/brcm97440b0/bcmuart.h>
#include <asm/brcmstb/brcm97440b0/bcmtimer.h>
#include <asm/brcmstb/brcm97440b0/bcmebi.h>
#include <asm/brcmstb/brcm97440b0/int1.h>
#include <asm/brcmstb/brcm97440b0/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97440b0/board.h>
#include <asm/brcmstb/brcm97440b0/bchp_irq0.h>
#include <asm/brcmstb/brcm97440b0/bchp_irq1.h>
#include <asm/brcmstb/brcm97440b0/bcmintrnum.h>
#include <asm/brcmstb/brcm97440b0/bchp_nand.h>
#include <asm/brcmstb/brcm97440b0/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97440b0/bchp_usb_ehci.h>
#include <asm/brcmstb/brcm97440b0/bchp_usb_ohci.h>
#include <asm/brcmstb/brcm97440b0/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97440b0/bchp_sharf_top.h>
#include <asm/brcmstb/brcm97440b0/bchp_sharf_mem_dma0.h>
#include <asm/brcmstb/brcm97440b0/bchp_sharf_mem_dma1.h>
#include <asm/brcmstb/brcm97440b0/bchp_fgx_to_hif_intr2.h>

#elif defined(CONFIG_MIPS_BCM7601)
#include <asm/brcmstb/brcm97601/bcmuart.h>
#include <asm/brcmstb/brcm97601/bcmtimer.h>
#include <asm/brcmstb/brcm97601/bcmebi.h>
#include <asm/brcmstb/brcm97601/int1.h>
#include <asm/brcmstb/brcm97601/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97601/board.h>
#include <asm/brcmstb/brcm97601/bchp_irq0.h>
#include <asm/brcmstb/brcm97601/bchp_irq1.h>
#include <asm/brcmstb/brcm97601/bcmintrnum.h>
#include <asm/brcmstb/brcm97601/bchp_nand.h>
#include <asm/brcmstb/brcm97601/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97601/bchp_usb_ehci.h>
#include <asm/brcmstb/brcm97601/bchp_usb_ohci.h>
#include <asm/brcmstb/brcm97601/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97601/bchp_pcix_bridge.h>
#include <asm/brcmstb/brcm97601/bchp_sharf_top.h>
#include <asm/brcmstb/brcm97601/bchp_sharf_mem_dma0.h>
#include <asm/brcmstb/brcm97601/bchp_sharf_mem_dma1.h>
#include <asm/brcmstb/brcm97601/bchp_shift_intr2.h>
#include <asm/brcmstb/brcm97601/bchp_enet_top.h>
#include <asm/brcmstb/brcm97601/bchp_wol_top.h>

#elif defined(CONFIG_MIPS_BCM7630)
#include <asm/brcmstb/brcm97630/bcmuart.h>
#include <asm/brcmstb/brcm97630/bcmtimer.h>
#include <asm/brcmstb/brcm97630/bcmebi.h>
#include <asm/brcmstb/brcm97630/int1.h>
#include <asm/brcmstb/brcm97630/board.h>
#include <asm/brcmstb/brcm97630/bchp_irq0.h>
#include <asm/brcmstb/brcm97630/bchp_irq1.h>
#include <asm/brcmstb/brcm97630/bcmintrnum.h>
#include <asm/brcmstb/brcm97630/bchp_hif_intr2.h>
#include <asm/brcmstb/brcm97630/bchp_hif_cpu_intr1.h>
#include <asm/brcmstb/brcm97630/bchp_nand.h>
#include <asm/brcmstb/brcm97630/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97630/bchp_usb_ehci.h>
#include <asm/brcmstb/brcm97630/bchp_usb_ohci.h>
#include <asm/brcmstb/brcm97630/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97630/bchp_sharf_top.h>
#include <asm/brcmstb/brcm97630/bchp_sharf_mem_dma0.h>
#include <asm/brcmstb/brcm97630/bchp_sharf_mem_dma1.h>
#include <asm/brcmstb/brcm97630/bchp_common.h>
#include <asm/brcmstb/brcm97630/bchp_ofe_armcr4_bridge.h>
#include <asm/brcmstb/brcm97630/bchp_ofe_datapath_intr2.h>
#include <asm/brcmstb/brcm97630/bchp_vide_host_reg.h>
#include <asm/brcmstb/brcm97630/bchp_vide_ctl.h>
#include <asm/brcmstb/brcm97630/bchp_vide_bus_master.h>
#include <asm/brcmstb/brcm97630/bchp_enet_top.h>
#include <asm/brcmstb/brcm97630/bchp_wol_top.h>
#include <asm/brcmstb/brcm97630/bchp_clk.h>

#elif defined(CONFIG_MIPS_BCM7635)
#include <asm/brcmstb/brcm97635/bcmuart.h>
#include <asm/brcmstb/brcm97635/bcmtimer.h>
#include <asm/brcmstb/brcm97635/bcmebi.h>
#include <asm/brcmstb/brcm97635/int1.h>
#include <asm/brcmstb/brcm97635/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97635/board.h>
#include <asm/brcmstb/brcm97635/bchp_irq0.h>
#include <asm/brcmstb/brcm97635/bchp_irq1.h>
#include <asm/brcmstb/brcm97635/bcmintrnum.h>
#include <asm/brcmstb/brcm97635/bchp_nand.h>
#include <asm/brcmstb/brcm97635/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97635/bchp_usb_ehci.h>
#include <asm/brcmstb/brcm97635/bchp_usb_ohci.h>
#include <asm/brcmstb/brcm97635/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97635/bchp_pcix_bridge_0.h>
#include <asm/brcmstb/brcm97635/bchp_pcix_bridge_1.h>
#include <asm/brcmstb/brcm97635/bchp_sharf_top.h>
#include <asm/brcmstb/brcm97635/bchp_sharf_mem_dma0.h>
#include <asm/brcmstb/brcm97635/bchp_sharf_mem_dma1.h>
#include <asm/brcmstb/brcm97635/bchp_shift_intr2.h>
#include <asm/brcmstb/brcm97635/bchp_common.h>
#include <asm/brcmstb/brcm97635/bchp_ofe_armcr4_bridge.h>
#include <asm/brcmstb/brcm97635/bchp_ofe_datapath_intr2.h>
#include <asm/brcmstb/brcm97635/bchp_vide_host_reg.h>
#include <asm/brcmstb/brcm97635/bchp_vide_ctl.h>
#include <asm/brcmstb/brcm97635/bchp_vide_bus_master.h>
#include <asm/brcmstb/brcm97635/bchp_enet_top.h>
#include <asm/brcmstb/brcm97635/bchp_wol_top.h>
#include <asm/brcmstb/brcm97635/bchp_clk.h>

#else
#error "unknown BCM STB chip!!!"
#endif

#ifndef __ASSEMBLY__
extern void (*irq_setup)(void);
extern void uart_puts(const char*);


typedef int (*walk_cb_t)(unsigned long paddr, unsigned long size, long type, void* cbdata);
extern int brcm_walk_boot_mem_map(void* cbdata, walk_cb_t walk_cb);
extern unsigned long get_RAM_size(void);
extern unsigned long get_RSVD_size(void);

#define DEV_RD(x) (*((volatile unsigned long *)(x)))
#define DEV_WR(x, y) do { *((volatile unsigned long *)(x)) = (y); } while(0)
#define DEV_UNSET(x, y) do { DEV_WR((x), DEV_RD(x) & ~(y)); } while(0)
#define DEV_SET(x, y) do { DEV_WR((x), DEV_RD(x) | (y)); } while(0)

#define BDEV_RD(x) (DEV_RD((x) | 0xb0000000UL))
#define BDEV_WR(x, y) do { DEV_WR((x) | 0xb0000000UL, (y)); } while(0)
#define BDEV_UNSET(x, y) do { BDEV_WR((x), BDEV_RD(x) & ~(y)); } while(0)
#define BDEV_SET(x, y) do { BDEV_WR((x), BDEV_RD(x) | (y)); } while(0)

#endif

struct mem_region {
	phys_t	 paddr;
	phys_t	 size;
};
#define MAX_MEM_REGIONS		4
#define MAX_BMEM_REGIONS	4

extern struct mem_region *mem_regions;
extern struct mem_region *bmem_regions;

extern void get_memory_config(void);
extern void set_memory_config(void);

/*
 * Address Range Checker
 */
enum {
	ARC_KERNEL_TEXT = 0,
	ARC_KERNEL_MEM_0,
	ARC_KERNEL_MEM_1,
	ARC_FE,
	ARC_MAX
};

extern int ar_enabled(int ar_idx);
extern int disable_ar_check(int ar_idx);
extern int reenable_ar_check(int ar_idx, unsigned int base_addr);

#endif /*__BRCMSTB_H */

