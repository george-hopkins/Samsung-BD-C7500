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

when		who 	what
-----		---	----
12/12/07 	jipeng 	copy from Malta codes and modify for BRCM paltform
*/

/* ripped from arch/mips/mips-boards/malta/malta_smp.c */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>

#include <asm/atomic.h>
#include <asm/cpu.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/hardirq.h>
#include <asm/mmu_context.h>
#include <asm/smp.h>
#ifdef CONFIG_MIPS_MT_SMTC
#include <asm/smtc_ipi.h>
#endif /* CONFIG_MIPS_MT_SMTC */

/* VPE/SMP Prototype implements platform interfaces directly */
#if !defined(CONFIG_MIPS_MT_SMP)

/*
 * Cause the specified action to be performed on a targeted "CPU"
 */

void core_send_ipi(int cpu, unsigned int action)
{
/* "CPU" may be TC of same VPE, VPE of same CPU, or different CPU */
#ifdef CONFIG_MIPS_MT_SMTC
	smtc_send_ipi(cpu, LINUX_SMP_IPI, action);
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Platform "CPU" startup hook
 */

void prom_boot_secondary(int cpu, struct task_struct *idle)
{
#ifdef CONFIG_MIPS_MT_SMTC
	smtc_boot_secondary(cpu, idle);
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Post-config but pre-boot cleanup entry point
 */

void prom_init_secondary(void)
{
#ifdef CONFIG_MIPS_MT_SMTC
        void smtc_init_secondary(void);
	int myvpe;

	/* Don't enable I/O interrupts (IP2) for secondary VPEs, only use int2 (I/O), int1 (IPI) and int7 (timer) for now */
	myvpe = read_c0_tcbind() & TCBIND_CURVPE;
	if (myvpe != 0) {
		/* Ideally, this should be done only once per VPE, but... */
		clear_c0_status(STATUSF_IP2);
		set_c0_status(STATUSF_IP0 | STATUSF_IP1 | STATUSF_IP7);
	}

        smtc_init_secondary();
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Platform SMP pre-initialization
 *
 * As noted above, we can assume a single CPU for now
 * but it may be multithreaded.
 */

void plat_smp_setup(void)
{

	if (read_c0_config3() & (1<<2))
		mipsmt_build_cpu_map(0);
}

void __init plat_prepare_cpus(unsigned int max_cpus)
{
	if (read_c0_config3() & (1<<2))
		mipsmt_prepare_cpus();
}

/*
 * SMP initialization finalization entry point
 */

void prom_smp_finish(void)
{
#ifdef CONFIG_MIPS_MT_SMTC
	smtc_smp_finish();
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Hook for after all CPUs are online
 */

void prom_cpus_done(void)
{
}

#endif /* ! CONFIG_MIPS_MT_SMP */

