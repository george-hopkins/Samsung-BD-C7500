/*
<:copyright-gpl 
 Copyright 2005-2006 Broadcom Corp. All Rights Reserved. 
 
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
3/10/06	TDT	Created based on Bcm4350 FPGA SMP Linux
4/12/06 TDT Rewrote logic to decouple IPC messages by using seperate interrupts
*/
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/reboot.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/pmon.h>
#include <asm/time.h>
#include <asm/cpu.h>
#include <asm/smp.h>

#include <asm/brcmstb/common/brcmstb.h>

// Marcos that we use on the 7400
#define read_c0_brcm_config_0()	__read_32bit_c0_register($22, 0)
#define write_c0_brcm_config_0(val) __write_32bit_c0_register($22, 0, val)

#define read_c0_cmt_interrupt()		__read_32bit_c0_register($22, 1)
#define write_c0_cmt_interrupt(val)	__write_32bit_c0_register($22, 1, val)

#define read_c0_cmt_control()		__read_32bit_c0_register($22, 2)
#define write_c0_cmt_control(val)	__write_32bit_c0_register($22, 2, val)

// Debug macros
#define read_other_tp_status()		__read_32bit_c0_register($12, 7)
#define read_other_tp_cause()		__read_32bit_c0_register($13, 7)

// used for passing boot arguments...
struct BootConfig {
    unsigned long signature;
    unsigned long ncpus;
    unsigned long func_addr;
    unsigned long sp;
    unsigned long gp;
    unsigned long arg;
} g_boot_config;
struct BootConfig * boot_config = (struct BootConfig * ) &g_boot_config;

extern cpumask_t phys_cpu_present_map;		/* Bitmask of available CPUs */
extern void brcm_timerx_setup(struct irqaction *);
extern irqreturn_t timerx_interrupt(int, void *, struct pt_regs *);

void __init plat_smp_setup(void)
{
       int i, num;

        cpus_clear(phys_cpu_present_map);
        cpu_set(0, phys_cpu_present_map);
        __cpu_number_map[0] = 0;
        __cpu_logical_map[0] = 0;

        for (i = 1, num = 0; i < NR_CPUS; i++) {
			 cpu_set(i, phys_cpu_present_map);
			__cpu_number_map[i] = ++num;
			__cpu_logical_map[num] = i;
         }

}

void prom_init_secondary(void)
{
    // don't do anything...
}

// Handle interprocessor messages
static irqreturn_t brcm_smp_call_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/* SMP_CALL_FUNCTION */
	smp_call_function_interrupt();

    // we need to clear the interrupt...
    {
        register u32 temp;
        temp = read_c0_cause();
        temp &= ~CAUSEF_IP0;
        write_c0_cause(temp);
    }
    
	return IRQ_HANDLED;
}

static irqreturn_t brcm_reschedule_call_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    // we need to clear the interrupt...
	register u32 temp;
	temp = read_c0_cause();
	temp &= ~CAUSEF_IP1;
	write_c0_cause(temp);
    
	return IRQ_HANDLED;
}

static struct irqaction brcm_ipc_action0 = {
	.handler	= brcm_smp_call_interrupt,
	.flags		= IRQF_DISABLED,
	.mask		= CPU_MASK_NONE,
	.name		= "ipc0",
	.next		= NULL,
	.dev_id		= brcm_smp_call_interrupt,
};

static struct irqaction brcm_ipc_action1 = {
	.handler	= brcm_reschedule_call_interrupt,
	.flags		= IRQF_DISABLED,
	.mask		= CPU_MASK_NONE,
	.name		= "ipc1",
	.next		= NULL,
	.dev_id		= brcm_reschedule_call_interrupt,
};

static struct irqaction dummy_irq = {
	.handler	= timerx_interrupt,
	.flags		= SA_INTERRUPT,
	.mask		= CPU_MASK_NONE,
	.name		= "timer TP1",
	.next		= NULL,
};

// for DT-MIPS this always runs from 2nd CPU
// FIX this later so it can run from 3rd/4th, etc. CPU...
void prom_smp_finish(void)
{
	printk("TP%d: prom_smp_finish enabling sw_irq\n", smp_processor_id());

	setup_irq(BCM_LINUX_IPC_0_IRQ, &brcm_ipc_action0);
	setup_irq(BCM_LINUX_IPC_1_IRQ, &brcm_ipc_action1);
	set_c0_status(IE_SW0 | IE_SW1 | IE_IRQ1 | ST0_IE);

	printk("TP%d: prom_smp_finish enabling local timer_interrupt\n",
		smp_processor_id());
        brcm_timerx_setup(&dummy_irq);
}

void prom_cpus_done(void)
{
	// Enable interrupts on TP0
	set_c0_status(IE_SW0 | IE_SW1 | ST0_IE);
}

void __init plat_prepare_cpus(unsigned int max_cpus)
{
	/* TODO - Currently 7400 has no boot options for 1 or 2 threads, this needs to come from CFE */
	boot_config->ncpus = 1;
  	if (!boot_config->ncpus) {
        printk("plat_prepare_cpus: leaving 2nd thread disabled...\n");
    } else {
        printk("plat_prepare_cpus: ENABLING 2nd Thread...\n");
        cpu_set(1, phys_cpu_present_map);
    }

#if defined(CONFIG_MIPS_7400A0)
    /* 7400A0 CMT "uncached assesses blocking d-cache from other TP" workaround */
	{
        volatile unsigned long * pCfg = (unsigned long * ) 0xbfa0001c;
        printk("enable 7400A0 CMT 'uncached assesses blocking d-cache from other TP' workaround\n");
		printk("0x2000000=>0xBFA0001C\n");
		*pCfg |= (1<<25);
     }
#endif

}

void core_send_ipi(int cpu, unsigned int action)
{
    register unsigned long temp, value;
	unsigned long flags;

	local_irq_save (flags);
	
	switch (action) {
	case SMP_CALL_FUNCTION:
		value = C_SW0;
		break;
	case SMP_RESCHEDULE_YOURSELF:
		value = C_SW1;
		break;
	default:
		return;
	}
	// trigger interrupt on the other TP...
	temp = read_c0_cause();
	temp |= value;
	write_c0_cause(temp);

	local_irq_restore(flags);
}

/*
 * Setup the PC, SP, and GP of a secondary processor and start it
 * running!
 */
void prom_boot_secondary(int cpu, struct task_struct *idle)
{
    register unsigned long temp;

    printk("TP%d: prom_boot_secondary: switching to non-blocking mode...\n", smp_processor_id());
    // disable non-blocking mode
    temp = read_c0_brcm_config_0();
    // PR22809 Add NBK and weak order flags
	temp |= 0x30000;
    write_c0_brcm_config_0(temp);
  
    // Configure the s/w interrupt that TP0 will use to kick TP1.  By default,
    // s/w interrupt 0 will be vectored to the TP that generated it, so we need
    // to set the interrupt routing bit to make it cross over to the other TP.
    // This is bit 15 (SIR) of CP0 Reg 22 Sel 1. We also want to make h/w IRQ 1
    // go to TP1. To do this, we need to set the appropriate bit in the h/w
    // interrupt crossover. To set the h/w interrupt crossover, we need to set 
    // a different bit in that same CP0 register.  The XIR bits (31:27) 
    // control h/w IRQ 4..0, respectively.
    printk("TP%d: prom_boot_secondary: adjusting s/w interrupt crossover\n", smp_processor_id());
    
    temp = read_c0_cmt_interrupt();
    temp |= 0x10018000;
    write_c0_cmt_interrupt(temp);

    printk("TP%d: prom_boot_secondary: c0 22,1=%lx\n", smp_processor_id(),temp);
    
	// first save the boot data...
    boot_config->func_addr = (unsigned long) smp_bootstrap;      // function 
    boot_config->sp        = (unsigned long) __KSTK_TOS(idle);    // sp
    boot_config->gp        = (unsigned long) idle->thread_info;   // gp
    boot_config->arg       = 0;
     
    printk("TP%d: prom_boot_secondary: Kick off 2nd CPU, and adjust TLB serialization...\n", smp_processor_id());
    temp = read_c0_cmt_control();

    temp |= 0x01;           // Takes second CPU	out of reset

	// The following flags can be adjusted to suit performance and debugging needs
    //temp |= (1<<4);           // TP0 priority
    //temp |= (1<<5);           // TP1 priority

#ifdef CONFIG_MIPS_BCM7400A0
    // adjust tlb serialization
    temp &= ~(0x0F << 16);   // mask off old tlb serialization thingie...
//    temp |= (0x06 << 16);   // adjust tlb serialization (only exception serialization)
//    temp |= (0x04 << 16);   // adjust tlb serialization (full serialization)
//    temp |= (0x00 << 16);   // adjust tlb serialization (no serialization)
//    temp |= (0x07 << 16);   // adjust tlb serialization (USE THIS!)
    temp |= (0x01 << 16);   // adjust tlb serialization (no serialization RECOMMENDED!)
#endif
    
//	temp |= (0x01 << 30);	// Debug and profile TP1 thread

    printk("TP%d: prom_boot_secondary: cp0 22,2 => %08lx\n", smp_processor_id(), temp);
    write_c0_cmt_control(temp);
}

