/*
<:copyright-gpl 
 Copyright 2006 Broadcom Corp. All Rights Reserved. 
 
 This program is free software; you can distribute it and/or modify it 
 under the terms of the GNU General Public License (Version 2) as 
 published by the Free Software Foundation. 
 
 This program is distributed in the hope it will be useful, but WITHOUT 
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 for more details. 
 
 You should have received a copy of the GNU General Public License along 
 with this program; if not, write to the Free Software Foundation, Inc., 
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA. 
:>
*/
/*
    These are the platform specific functions that need to be implemented when SMP
    is enabled in the kernel build.

when	who what
-----	---	----
9/21/06	TDT	Created
*/
#ifndef __ASM_MACH_BRCMSTB_KERNEL_ENTRY_H
#define __ASM_MACH_BRCMSTB_KERNEL_ENTRY_H

#ifdef CONFIG_MIPS_BCM7402S
#include <asm/brcmstb/brcm97401a0/bcmmips.h>
#include <asm/brcmstb/brcm97401a0/bcmuart.h>

#define BCHP_SUN_TOP_CTRL_SW_RESET 	            0x00404010
#define BCHP_SUN_TOP_CTRL_SEMAPHORE_8          	0x00404048
#define BCHP_SUN_GISB_ARB_TIMER                 0x0040600c
#define BCHP_SUN_GISB_ARB_REQ_MASK              0x00406008
#define BCHP_FMISC_SOFT_RESET                   0x00104000 
#define BCHP_MEMC_0_DDR_PLL_FREQ_DIVIDER_RESET	0x0010683c
#define BCHP_MEMC_0_DDR_PLL_FREQ_CNTL           0x00106818
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5        0x00404098
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8        0x004040a4 

#define BCHP_SUN_TOP_CTRL_RESET_CTRL			0x00404008
//#define BCHP_SUN_TOP_CTRL_SW_RESET			0x00404014
#define BCHP_VCXO_0_RM_CONTROL                  0x00181700 /* Rate Manager Controls */
#define UART_ADR_BASE							0xb04001a0		// UARTB

#endif

	.macro kernel_entry_setup
#ifdef CONFIG_BRCM_PCI_SLAVE
xxx:
        li      a0, '7'
         li      t0, UART_ADR_BASE
        li      t2, TXDREGEMT
1:      lw      t1, UART_TXSTAT(t0)
        and     t1, t1, t2
        bne     t1, t2, 1b
        nop
        sw      a0, UART_TXDATA(t0)
 
        nop
        //b	xxx
        nop
        
	bal		pci_slave_init
	nop

return_from_pci_slave_init:

#else
	setup_c0_status_pri
#endif

#ifdef CONFIG_MIPS_BRCM97XXX
#if defined(CONFIG_SMP) && ! defined(CONFIG_MIPS_MT)
	.extern  g_boot_config
	# Check to see which TP we are running on.
	mfc0    $8, $22, 3
	srl     $8, $8, 31
	beqz     $8, 1f
	nop
	
	# ---------------------------------	
	# We are in TP1
	# ---------------------------------	
InTp1:	
	# struct BootConfig {
	#     unsigned long signature;   //  0
	#     unsigned long ncpus;       //  4
	#     unsigned long func_addr;   //  8
	#     unsigned long sp;          // 12
	#     unsigned long gp;          // 16
	#     unsigned long arg;         // 20
	# }
	la       t0,	g_boot_config	# grab boot config data...
	lw       sp,	12(t0)
	lw       gp,	16(t0)
	lw       t1,	 8(t0)
	lw       a0,	20(t0)
	jr       t1
	nop
#endif // SMP

#if ! defined(CONFIG_MIPS_BCM7402S) && ! defined(CONFIG_MIPS_BRCM_IKOS)
1:	jal	cfe_start
	nop
#endif

#endif // BRCM97XXX
.endm

/*
 * Do SMP slave processor setup necessary before we can savely execute C code.
 */
        .macro  smp_slave_setup
        .endm

#ifdef CONFIG_MIPS_BCM7402S
#include "../brcmstb/common/reset.S"
#include "../brcmstb/common/cacheALib.S"
#endif

#endif /* __ASM_MACH_BRCMSTB_KERNEL_ENTRY_H */
