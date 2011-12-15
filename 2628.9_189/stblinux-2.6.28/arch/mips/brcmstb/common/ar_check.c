
/*
 *
 * Copyright (C) 2007-2009 Broadcom Corporation
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
 *
 */

/*
 * Address range checker support
 * for 7601B0, 7635 
 * 7601 A0 has different memory client assignments
 */

#include <linux/autoconf.h>
#include <linux/types.h>
#include <linux/module.h>

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kallsyms.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>

#include <asm/brcmstb/common/brcmstb.h>


/*
 * Address Range Checker
 */

#define BCM_LINUX_MEMARB_INTR  \
	(BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_ARB_ARCH_CPU_INTR_SHIFT + 32 + 1) /* 45 */


#define ARB_TOP_CONFIG_REGS_ARCH_L2 		0x0040be00
#define ARB_TOP_ARCH_REGS_CNTRL_REGi_ARRAY_BASE 0x0040bc00

struct	arch_l2 {
	volatile unsigned int int_status;
	volatile unsigned int int_set;
	volatile unsigned int int_clear;
	volatile unsigned int mask_status;
	volatile unsigned int mask_set;
	volatile unsigned int mask_clear;
};

#ifdef CONFIG_MIPS_BCM7601
#define CLIENT_AEGIS		16
#define CLIENT_EDU		38
#define CLIENT_ENET		40
#define CLIENT_USB0		41
#define CLIENT_USB1		42
#define CLIENT_SATA		43
#define CLIENT_MIPS0_R		44
#define CLIENT_MIPS0_W		45
#define CLIENT_SHARF		46
#define CLIENT_TRACELOG		51
#define CLIENT_REFRESH_0	52
#define CLIENT_REFRESH_1	53
#endif

#ifdef 	CONFIG_MIPS_BCM7630
#define CLIENT_AEGIS		16
#define CLIENT_SHARF		36
#define CLIENT_EDU		37
#define CLIENT_SDIO		39
#define CLIENT_UART_DMA		42
#define CLIENT_ENET		43
#define CLIENT_USB0		44
#define CLIENT_USB1		45
#define CLIENT_MIPS0_R		46
#define CLIENT_MIPS0_W		47
#define CLIENT_OFE_ARM		48
#define CLIENT_OFE_PER		49
#define CLIENT_OFE_HOST		50
#define CLIENT_OFE_DEC		51
#define CLIENT_TRACELOG		53
#define CLIENT_REFRESH_0	54
#define CLIENT_REFRESH_1	55
#endif

#ifdef 	CONFIG_MIPS_BCM7635
#define CLIENT_AEGIS		16
#define CLIENT_EDU		40
#define CLIENT_SDIO		42
#define CLIENT_ENET		43
#define CLIENT_USB0		44
#define CLIENT_USB1		45
#define CLIENT_SATA		46
#define CLIENT_SATA2		47
#define CLIENT_MIPS0_R		48
#define CLIENT_MIPS0_W		49
#define CLIENT_SHARF		50
#define CLIENT_OFE_ARM		54
#define CLIENT_OFE_PER		55
#define CLIENT_OFE_HOST		56
#define CLIENT_OFE_DEC		57
#define CLIENT_TRACELOG		59
#define CLIENT_REFRESH_0	60
#define CLIENT_REFRESH_1	61

#endif


unsigned int clients[] = {
	CLIENT_EDU,
	CLIENT_ENET,
	CLIENT_USB0,
	CLIENT_USB1,
#ifdef CONFIG_SYS_HAS_SATA
	CLIENT_SATA,
#ifdef CONFIG_MIPS_BCM7635
	CLIENT_SATA2,
#endif
#endif

#ifdef CONFIG_SYS_HAS_SDIO
	CLIENT_SDIO,
#endif

#ifdef CONFIG_SYS_HAS_VIDE_OFE
	CLIENT_OFE_HOST,
#endif
	CLIENT_MIPS0_R,
	CLIENT_MIPS0_W,
	CLIENT_TRACELOG,
	CLIENT_REFRESH_0,
	CLIENT_REFRESH_1,
};

#ifdef CONFIG_SYS_HAS_VIDE_OFE
/*
 * OFE Clients only.
 */
unsigned int ofe_clients[] = {
#ifdef notdef
	/*
	 * Protect all FE memory from non-OFE
	 * clients.
	 */
	CLIENT_OFE_ARM,
	CLIENT_OFE_PER,
	CLIENT_OFE_HOST,
	CLIENT_OFE_DEC,
#else
	/*
	 * Protect FE firmware image from
	 * potential corruption.
	 */
	CLIENT_OFE_ARM,
#endif
};
#endif


/* 
 * there are only 4 arch sets, but they are spaced out as if there were 8
*/
#define MODE_NON_EXCLUSIVE	0x00
#define MODE_EXCLUSIVE		0x01
#define MODE_ULTRA_EXCLUSIVE	0x02
#define READ_CHECK		0x04
#define WRITE_CHECK		0x08
#define WRITE_ABORT		0x10

struct addr_range_checker {
	volatile unsigned int cntrl[8];
	volatile unsigned int addr_low[8];
	volatile unsigned int addr_high[8];
	volatile unsigned int read_rights_0[8];		/* clients  0 - 31 */
	volatile unsigned int read_rights_1[8];		/* clients 32 - 63 */
	volatile unsigned int read_rights_2[8];		/* clients 64 - 95 */
	volatile unsigned int write_rights_0[8];	/* clients  0 - 31 */
	volatile unsigned int unused[8];
	volatile unsigned int write_rights_1[8];	/* clients 32 - 63 */
	volatile unsigned int write_rights_2[8];	/* clients 64 - 95 */
	volatile unsigned int viol_addr[8];
	volatile unsigned int viol_info[8];
	volatile unsigned int status_clear[8];
	volatile unsigned int alias_cntrl;
	volatile unsigned int alias_viol_addr;
	volatile unsigned int alias_viol_info;
	volatile unsigned int alias_status_clear;
};

extern unsigned long _stext, _etext;
extern struct mem_region *bmem_regions;

/*
 * Static globals
 */
static int arc_disable = 0;
static struct addr_range_checker save_arch;
static struct addr_range_checker *arch;


static int __init ar_check_early(char *p)
{
	arc_disable = 1;
	return 0;
}
early_param("arc_disable", ar_check_early);


void
init_ar_checker(void)
{
	int i, mask_lo, mask_hi;
	volatile struct	arch_l2 *l2;

	l2 = (volatile struct arch_l2 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + ARB_TOP_CONFIG_REGS_ARCH_L2);

	l2->int_clear  = 0x1ff;	/* clear status */
	l2->mask_clear = 0x1ff; /* clear interrupts */

	arch = (struct addr_range_checker *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET
						+ ARB_TOP_ARCH_REGS_CNTRL_REGi_ARRAY_BASE);

//	printk(KERN_ERR "%s addr 0x%p info @ 0x%p addr @ 0x%p clear@ 0x%p\n", __FUNCTION__,
//		arch, arch->viol_info, arch->viol_addr, arch->status_clear);

	mask_lo = 0;
	mask_hi = 0;
	for (i = 0; i < sizeof(clients)/sizeof (int); i++) {
		if (clients[i]  < 32)
			mask_lo |= 1 << clients[i];
		else
			mask_hi |= 1 << (clients[i] - 32);
	}
	
#if 0
	/* disable all write clients */
	mask_lo = 0;
	mask_hi = 0;
#endif

	for (i = 0; i < ARC_MAX; i++) {
		arch->cntrl[i] = 0;
		arch->addr_low[i] = 0;
		arch->addr_high[i] = 0;
		arch->status_clear[i] = 1;

		/* any client can read */
		arch->read_rights_0[i]	= 0xffffffff;
		arch->read_rights_1[i]	= 0xffffffff;
		arch->read_rights_2[i]	= 0xffffffff;

		/* limit which clients can write */
		arch->write_rights_0[i]	= mask_lo;
		arch->write_rights_1[i]	= mask_hi;
		arch->write_rights_2[i]	= 0;
	}


#if 1
/*
 * no one should write in the kernel text address space
 * Note: setting sw breakpoints does write in the text space
 * truncate _etext to a page boundary
 */
	printk(KERN_INFO "%s _stext 0x%p _etext 0x%p\n", __FUNCTION__, &_stext, &_etext);

	//arch->addr_low[ARC_KERNEL_TEXT] 	= (unsigned int )&_stext & 0x7fffff00;	/* kernel text */
	arch->addr_low[ARC_KERNEL_TEXT] 	= 0x1000;
	arch->addr_high[ARC_KERNEL_TEXT]	= (unsigned int )&_etext & 0x7ffff000;
	arch->write_rights_0[ARC_KERNEL_TEXT]	= 0;
	arch->write_rights_1[ARC_KERNEL_TEXT]	= 0;
	arch->write_rights_2[ARC_KERNEL_TEXT]	= 0;
	arch->cntrl[ARC_KERNEL_TEXT] 		= MODE_NON_EXCLUSIVE | WRITE_CHECK;
#endif

#if 1	
	/*
	 * from _etext to the begining of bmem[0]
	 * Round _etext up to the next page boundary
	 * Granularity of checker is 128 bytes.
	 */

	//arch->cntrl[ARC_KERNEL_MEM_0] 		= MODE_NON_EXCLUSIVE | READ_CHECK | WRITE_CHECK | WRITE_ABORT;

	if (bmem_regions[0].paddr &&  bmem_regions[0].size ) {
		/* MODE_NON_EXCLUSIVE should always be set */
		arch->cntrl[ARC_KERNEL_MEM_0] 		= MODE_NON_EXCLUSIVE | WRITE_CHECK;
		arch->addr_low[ARC_KERNEL_MEM_0] 	= ((unsigned int )&_etext + 0x1000) & 0x7ffff000;
		arch->addr_high[ARC_KERNEL_MEM_0]	= bmem_regions[0].paddr - 128;
	}

	/*
	 * second DDR from 0x20000000 to beginning of bmem[1]
	 */
	if (bmem_regions[1].paddr &&  bmem_regions[1].size ) {
		arch->cntrl[ARC_KERNEL_MEM_1] 		= MODE_NON_EXCLUSIVE | WRITE_CHECK;
		arch->addr_low[ARC_KERNEL_MEM_1] 	= 0x20000000;
		arch->addr_high[ARC_KERNEL_MEM_1]	= bmem_regions[1].paddr - 128;
	}
#endif

#ifdef CONFIG_SYS_HAS_VIDE_OFE

	{
		int last_idx = 0;

		/*
		 * Find the last bmem entry
		 * The last BMEM entry will be for the FE.
		 */
		for (i = 0; i < MAX_BMEM_REGIONS; i++) {
			if (bmem_regions[i].size)
				last_idx = i;
		}

		if (bmem_regions[last_idx].size == (3 * 1024 * 1024)) {
			/*
			 * For the last BMEM region,
			 * only OFE clients should have write access.
			 */
			mask_lo = 0;
			mask_hi = 0;
			for (i = 0; i < sizeof(ofe_clients)/sizeof (int); i++) {
				if (ofe_clients[i]  < 32)
					mask_lo |= 1 << ofe_clients[i];
				else
					mask_hi |= 1 << (ofe_clients[i] - 32);
			}

			arch->addr_low[ARC_FE] 		= bmem_regions[last_idx].paddr;
			arch->write_rights_0[ARC_FE]	= mask_lo;
			arch->write_rights_1[ARC_FE]	= mask_hi;
			arch->write_rights_2[ARC_FE]	= 0;
#ifdef notdef
			/*
			 * Protect all FE memory from non-OFE
			 * clients.
			 */
			arch->addr_high[ARC_FE]		= bmem_regions[last_idx].paddr + bmem_regions[last_idx].size - 128;
			arch->cntrl[ARC_FE] 		= MODE_NON_EXCLUSIVE | WRITE_CHECK;
#else
			/*
			 * Protect FE firmware image from
			 * potential corruption.
			 */
			arch->addr_high[ARC_FE]		= bmem_regions[last_idx].paddr + 0x100000 - 128;
			arch->cntrl[ARC_FE] 		= MODE_NON_EXCLUSIVE | WRITE_ABORT;
#endif
		}
		else
			printk(KERN_WARNING "%s: NO FE ARC: BMEM entry for FE firmware not found!\n",
			       __FUNCTION__);
	}
#endif


	for (i = 0; i < ARC_MAX; i++) {
	    printk(KERN_WARNING "checker_%d control 0x%08x addr low 0x%08x hi 0x%08x \n",
		i, arch->cntrl[i], arch->addr_low[i], arch->addr_high[i]); 

	    printk(KERN_WARNING "read rights _0 0x%08x _1 0x%08x write rights _0 0x%08x _1 0x%08x \n",
		arch->read_rights_0[i], arch->read_rights_1[i],
		arch->write_rights_0[i], arch->write_rights_1[i]);
	}
}

#if 0
void do_stop(void)
{
	        __asm__(".set\tmips32r2\n\t"
                "ehb\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
                ".set\tmips0");

}
#endif

/*
 * API to determine if a particular Address Range Checker
 * is currently enabled, or was enabled and was last
 * disabled by a call to disable_ar_check().
 *
 * Returns  0 if not enabled.
 * Returns  1 if enabled.
 * Returns -1 if input ar_idx is invalid
 */
int
ar_enabled(int ar_idx)
{
	struct addr_range_checker *saved_arch = &save_arch;

	/*
	 * Check if the specified ARC is currently
	 * or was previously enabled.
	 */
	if (ar_idx < 0 || ar_idx >= ARC_MAX)
		return -1;

	if (!arc_disable &&
	    (arch->cntrl[ar_idx]  ||
	     saved_arch->cntrl[ar_idx]))
		return 1;

	/* Is/was not enabled */
	return 0;
}

/*
 * API to dynamically disable a particular ARC.
 *
 * Returns 0 on success
 */
int
disable_ar_check(int ar_idx)
{
	struct addr_range_checker *saved_arch = &save_arch;

	if (ar_idx < 0 || ar_idx >= ARC_MAX)
		return -1;
	
	if (ar_enabled(ar_idx)) {
		saved_arch->addr_low[ar_idx] 		= arch->addr_low[ar_idx];
		saved_arch->addr_high[ar_idx]		= arch->addr_high[ar_idx];
		saved_arch->write_rights_0[ar_idx]	= arch->write_rights_0[ar_idx];
		saved_arch->write_rights_1[ar_idx]	= arch->write_rights_1[ar_idx];
		saved_arch->write_rights_2[ar_idx]	= arch->write_rights_2[ar_idx];
		saved_arch->cntrl[ar_idx] 		= arch->cntrl[ar_idx];

		arch->addr_low[ar_idx] 		= 0;
		arch->addr_high[ar_idx]		= 0;
		arch->write_rights_0[ar_idx]	= 0;
		arch->write_rights_1[ar_idx]	= 0;
		arch->write_rights_2[ar_idx]	= 0;
		arch->cntrl[ar_idx]		= 0;
	}
	return 0;
}

/*
 * API to dynamically re-enable a particular ARC.
 *
 * Returns 0 on success
 */
int
reenable_ar_check(int ar_idx, unsigned int base_addr)
{
	struct addr_range_checker *saved_arch = &save_arch;

	if (ar_idx < 0 || ar_idx >= ARC_MAX)
		return -1;

	if (ar_enabled(ar_idx)) {
		/*
		 * If the protected region has relocated, the new
		 * base address will be passed in.
		 * Passed-in base_addr argument presumes 32-bit
		 * physical addressing.
		 */
		if (base_addr &&
		    base_addr != saved_arch->addr_low[ar_idx])
		{
			unsigned int size;

			size = saved_arch->addr_high[ar_idx] -
			       saved_arch->addr_low[ar_idx];
			arch->addr_low[ar_idx]	= base_addr;
			arch->addr_high[ar_idx] = arch->addr_low[ar_idx] + size;

			printk(KERN_INFO "RELOCATED: checker_%d control 0x%08x addr low 0x%08x hi 0x%08x \n",
			       ar_idx, arch->cntrl[ar_idx], arch->addr_low[ar_idx], arch->addr_high[ar_idx]); 

			printk(KERN_INFO "RELOCATED: read rights _0 0x%08x _1 0x%08x write rights _0 0x%08x _1 0x%08x \n",
			       arch->read_rights_0[ar_idx], arch->read_rights_1[ar_idx],
			       arch->write_rights_0[ar_idx], arch->write_rights_1[ar_idx]);
		}
		else {
			arch->addr_low[ar_idx]	= saved_arch->addr_low[ar_idx];
			arch->addr_high[ar_idx]	= saved_arch->addr_high[ar_idx];
		}
		arch->write_rights_0[ar_idx]	= saved_arch->write_rights_0[ar_idx];
		arch->write_rights_1[ar_idx]	= saved_arch->write_rights_1[ar_idx];
		arch->write_rights_2[ar_idx]	= saved_arch->write_rights_2[ar_idx];
		arch->cntrl[ar_idx] 		= saved_arch->cntrl[ar_idx];

		saved_arch->addr_low[ar_idx] 		= 0;
		saved_arch->addr_high[ar_idx]		= 0;
		saved_arch->write_rights_0[ar_idx]	= 0;
		saved_arch->write_rights_1[ar_idx]	= 0;
		saved_arch->write_rights_2[ar_idx]	= 0;
		saved_arch->cntrl[ar_idx]		= 0;
	}
	return 0;
}

void
read_ar_check(void)
{
	unsigned int info, client, len, mode, write, addr, status;
	int i;
	struct pt_regs  *regs;

	volatile struct	arch_l2 *l2;

	l2 = (volatile struct arch_l2 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + ARB_TOP_CONFIG_REGS_ARCH_L2);

	status = l2->int_status;
	l2->int_clear  = 0x1ff;	/* clear status */

	if (!status)
		return;

//	printk(KERN_INFO "%s int status 0x%x\n",  __FUNCTION__, status);
	
	addr = 0;

	for (i = 0; i < ARC_MAX; i++) {
	    if ((status & (1 << i)) == 0 ||
		arch->cntrl[i] == 0)
		continue;

	    info = arch->viol_info[i];
	    addr = arch->viol_addr[i];

	    arch->status_clear[i] = 1;
	    arch->status_clear[i] = 0;

	    info >>= 8;
	    write = info & 1;

	    info >>= 1;
	    mode  = info & 0x7;

	    info >>= 3;
	    len = info & 0x1ff;

	    info >>= 12;
	    client = info & 0xff;


	    printk(KERN_ERR "%s: checker_%d 0x%lx Client %d len %d mode 0x%x %s addr 0x%08x\n",
		    __FUNCTION__, i, jiffies,
		    client, len, mode, write?"write":"read", addr);
	}
	
	regs = get_irq_regs();

#if 0	/* turn on for kernel debugging */
	printk("epc   : %08lx ", regs->cp0_epc);
	print_symbol("%s\n", regs->cp0_epc);
	printk("ra    : %08lx ",  regs->regs[31]);
	print_symbol("%s\n", regs->regs[31]);

	//show_regs(regs);

	dump_stack();
	//show_stack(current, (unsigned long *)regs->regs[29]);
	//do_stop();
#endif
}

irqreturn_t
brcm_mem_arb_intr(int irq, void *dev_id)
{
	read_ar_check();
	return IRQ_HANDLED;
}

int __init   archecker_init(void)
{

	if (!arc_disable) {
		/*
		 * create /proc/hwdebug
		 */
		init_ar_checker();
		if (request_irq(BCM_LINUX_MEMARB_INTR, brcm_mem_arb_intr, IRQF_DISABLED, "mem_arbiter", NULL))
			printk(KERN_ERR "%s: request_irq failed\n", __FUNCTION__);
	}
	return(0);
}

void  __exit  archecker_exit(void)
{
	printk("%s\n", __FUNCTION__);
}

EXPORT_SYMBOL(ar_enabled);
EXPORT_SYMBOL(disable_ar_check);
EXPORT_SYMBOL(reenable_ar_check);

module_init(archecker_init);
module_exit(archecker_exit);
MODULE_LICENSE("GPL");
