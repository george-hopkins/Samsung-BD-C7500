/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Bcm97635 Board specific code.
 *
 * Copyright 2004-2009 Broadcom Corp.
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
 * Revision Log
 * who when    what
 */


#include <linux/autoconf.h>
#include <linux/module.h>
#include <asm/bootinfo.h>
#include <asm/brcmstb/common/brcmstb.h>

#include <asm/brcmstb/brcm97635/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97635/boardmap.h>

#define PCI_BUS_MASTER  BCHP_PCI_CFG_STATUS_COMMAND_BUS_MASTER_MASK
#define PCI_IO_ENABLE   BCHP_PCI_CFG_STATUS_COMMAND_MEMORY_SPACE_MASK
#define PCI_MEM_ENABLE  BCHP_PCI_CFG_STATUS_COMMAND_IO_SPACE_MASK

#if	defined( CONFIG_MIPS_BRCM97XXX )
#if	defined( CONFIG_CPU_BIG_ENDIAN )
#define	CPU2PCI_CPU_PHYS_MEM_WIN_BYTE_ALIGN	2
#else
#define	CPU2PCI_CPU_PHYS_MEM_WIN_BYTE_ALIGN	0
#endif
#endif

#define REG(x) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+(x))

void
arch_early_misc_setup(void)
{

  
	// THT & Chanshine: Setup PCIX Bridge

printk(KERN_INFO "######### SUN_TOP_CTRL_SW_RESET @%08x = %08lx\n", 
	0xb0000000+0x00404014, *((volatile unsigned long *)REG(0x00404014)));



	/* 
	 * this platform has no PCI
	 */
}

void kill_port(int port)
{       
	/* 
	 * Stub routine.
	 */     
	printk(KERN_INFO "SATA: DISABLE TRANSMITTER FOR PORT %d (not supported for 7635 build)\n", port);
}


EXPORT_SYMBOL(kill_port);

