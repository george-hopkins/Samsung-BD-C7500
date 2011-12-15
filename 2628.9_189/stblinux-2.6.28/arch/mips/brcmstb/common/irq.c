/*
 * arch/mips/brcm/irq.c
 *
 * Copyright (C) 2001-2006 Broadcom Corporation
 *                    Steven J. Hill <shill@broadcom.com>
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
 * Interrupt routines for Broadcom eval boards
 *
 * 10-23-2001   SJH    Created
 * 10-25-2001   SJH    Added comments on interrupt handling
 * 09-29-2003   QY     Added support for bcm97038
 * 06-03-2005   THT    Ported to 2.6.12
 * 03-10-2006   TDT    Modified for SMP
 * 09-21-2006   TDT    Ported to 2.6.18
 * 11-07-2007   KPC    Cleaned up UART IRQs, add SMP affinity
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpumask.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/brcmstb/common/brcmstb.h>
#include <asm/smp.h>

#ifdef CONFIG_REMOTE_DEBUG
#include <asm/gdb-stub.h>
extern void breakpoint(void);
#endif

#ifdef CONFIG_SMP
static int next_cpu[NR_IRQS] = { [0 ... NR_IRQS-1] = 0 };
#define NEXT_CPU(irq) next_cpu[irq]
#else
#define NEXT_CPU(irq) 0
#endif

#ifndef CPUINT1C_TP1
#define CPUINT1C_TP1 CPUINT1C
#endif

#if defined(BCHP_HIF_CPU_INTR1_INTR_W2_STATUS)
#define L1_IRQS		96
#elif defined(BCHP_HIF_CPU_INTR1_INTR_W1_STATUS)
#define L1_IRQS		64
#endif

/*
 * For interrupt map, see include/asm-mips/brcmstb/<plat>/bcmintrnum.h
 */

/*
 * INTC (aka L1 interrupt) functions
 */

static void brcm_intc_enable(unsigned int irq)
{
	unsigned int shift;
	unsigned long flags;
	volatile Int1Control *l1 = NEXT_CPU(irq) ? CPUINT1C_TP1 : CPUINT1C;

	local_irq_save(flags);
	if (irq > 0 && irq <= 32)
	{
		shift = irq - 1;
		l1->IntrW0MaskClear = (0x1UL<<shift);
	}
	else if (irq > 32 && irq <= 32+32)
	{
		shift = irq - 32 - 1;
		l1->IntrW1MaskClear = (0x1UL<<shift);
	}
#if L1_IRQS > 64
	else if (irq > 64 && irq <= 32+32+32)
	{
		shift = irq - 64 - 1;
		l1->IntrW2MaskClear = (0x1UL<<shift);
	}
#endif
	local_irq_restore(flags);
}

static void brcm_intc_disable(unsigned int irq)
{
	unsigned int shift;
	unsigned long flags;

	local_irq_save(flags);
	if (irq > 0 && irq <= 32)
	{
		shift = irq - 1;
		CPUINT1C->IntrW0MaskSet = (0x1UL<<shift);
		CPUINT1C_TP1->IntrW0MaskSet = (0x1UL<<shift);
	}
	else if (irq > 32 && irq <= 32+32)
	{
		shift = irq - 32 -1;
		CPUINT1C->IntrW1MaskSet = (0x1UL<<shift);
		CPUINT1C_TP1->IntrW1MaskSet = (0x1UL<<shift);
	}
#if L1_IRQS > 64
	else if (irq > 64 && irq <= 32+32+32)
	{
		shift = irq - 64 -1;
		CPUINT1C->IntrW2MaskSet = (0x1UL<<shift);
		CPUINT1C_TP1->IntrW2MaskSet = (0x1UL<<shift);
	}
#endif
	local_irq_restore(flags);

}

static unsigned int brcm_intc_startup(unsigned int irq)
{ 
	brcm_intc_enable(irq);
	return 0; /* never anything pending */
}

static void brcm_intc_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_intc_enable(irq);
}

#ifdef CONFIG_SMP
static void brcm_intc_set_affinity(unsigned int irq, cpumask_t dest)
{
	unsigned int shift;
	unsigned long flags;

	local_irq_save(flags);
	if (irq > 0 && irq <= 32)
	{
		shift = irq - 1;

		if(cpu_isset(0,dest)) {
			CPUINT1C->IntrW0MaskClear = (0x1UL<<shift);
			CPUINT1C_TP1->IntrW0MaskSet = (0x1UL<<shift);
			next_cpu[irq] = 0;
		} else {
			CPUINT1C_TP1->IntrW0MaskClear = (0x1UL<<shift);
			CPUINT1C->IntrW0MaskSet = (0x1UL<<shift);
			next_cpu[irq] = 1;
		}
	}
	else if (irq > 32 && irq <= 64)
	{
		shift = irq - 32 - 1;
		next_cpu[irq] = 0;

		if(cpu_isset(0,dest)) {
			CPUINT1C->IntrW1MaskClear = (0x1UL<<shift);
			CPUINT1C_TP1->IntrW1MaskSet = (0x1UL<<shift);
			next_cpu[irq] = 0;
		} else {
			CPUINT1C_TP1->IntrW1MaskClear = (0x1UL<<shift);
			CPUINT1C->IntrW1MaskSet = (0x1UL<<shift);
			next_cpu[irq] = 1;
		}
	}
#if L1_IRQS > 64
	else if (irq > 64 && irq <= 96)
	{
		shift = irq - 64 - 1;
		next_cpu[irq] = 0;

		if(cpu_isset(0,dest)) {
			CPUINT1C->IntrW2MaskClear = (0x1UL<<shift);
			CPUINT1C_TP1->IntrW2MaskSet = (0x1UL<<shift);
			next_cpu[irq] = 0;
		} else {
			CPUINT1C_TP1->IntrW2MaskClear = (0x1UL<<shift);
			CPUINT1C->IntrW2MaskSet = (0x1UL<<shift);
			next_cpu[irq] = 1;
		}
	}
#endif
	local_irq_restore(flags);
}
#endif /* CONFIG_SMP */

/*
 * THT: These INTC disable the interrupt before calling the IRQ handle_irq
 */
static struct irq_chip brcm_intc_type = {
	name: "BCM INTC",
	startup: brcm_intc_startup,
	shutdown: brcm_intc_disable,
	enable: brcm_intc_enable,
	disable: brcm_intc_disable,
	ack: brcm_intc_disable,
	end: brcm_intc_end,
#ifdef CONFIG_SMP
	set_affinity: brcm_intc_set_affinity,
#endif /* CONFIG_SMP */
	NULL
};

/*
 * Move the interrupt to the other TP, to balance load (if affinity permits)
 */
static void flip_tp(int irq)
{
#ifdef CONFIG_SMP
	int tp = smp_processor_id();
	volatile Int1Control *local_l1, *remote_l1;
	unsigned long mask = 1 << ((irq - 1) & 0x1f);

	if(tp == 0) {
		local_l1 = CPUINT1C;
		remote_l1 = CPUINT1C_TP1;
	} else {
		local_l1 = CPUINT1C_TP1;
		remote_l1 = CPUINT1C;
	}

	if(cpu_isset(tp ^ 1, irq_desc[irq].affinity)) {
		next_cpu[irq] = tp ^ 1;
		if(irq >= 1 && irq <= 32) {
			local_l1->IntrW0MaskSet = mask;
			remote_l1->IntrW0MaskClear = mask;
		}
		if(irq >= 33 && irq <= 64) {
			local_l1->IntrW1MaskSet = mask;
			remote_l1->IntrW1MaskClear = mask;
		}
#if L1_IRQS > 64
		if(irq >= 65 && irq <= 96) {
			local_l1->IntrW2MaskSet = mask;
			remote_l1->IntrW2MaskClear = mask;
		}
#endif
	}
#endif /* CONFIG_SMP */
}

static void brcm_intc_dispatch(struct pt_regs *regs, volatile Int1Control *l1)
{
	unsigned int pendingIrqs0,pendingIrqs1,pendingIrqs2, shift,irq;

	pendingIrqs0 = l1->IntrW0Status & ~(l1->IntrW0MaskStatus);
	pendingIrqs1 = l1->IntrW1Status & ~(l1->IntrW1MaskStatus);
#if L1_IRQS > 64
        pendingIrqs2 = l1->IntrW2Status & ~(l1->IntrW2MaskStatus);
#endif

	for (irq=1; irq<=32; irq++)
	{
		shift = irq - 1;
		if ((0x1 << shift) & pendingIrqs0) {
			do_IRQ(irq, regs);
			flip_tp(irq);
		}
	}
	for (irq = 32+1; irq <= 32+32; irq++)
	{
		shift = irq - 32 - 1;
		if ((0x1 << shift) & pendingIrqs1) {
			do_IRQ(irq, regs);
			flip_tp(irq);
		}
	}
#if L1_IRQS > 64
	for (irq = 64+1; irq <= 64+32; irq++)
	{
		shift = irq - 64 - 1;
		if ((0x1 << shift) & pendingIrqs2) {
			do_IRQ(irq, regs);
			flip_tp(irq);
		}
	}
#endif
}

/* IRQ2 = L1 interrupt for TP0 */
static void brcm_mips_int2_dispatch(struct pt_regs *regs)
{
	clear_c0_status(STATUSF_IP2);
	brcm_intc_dispatch(regs, CPUINT1C);
	set_c0_status(STATUSF_IP2);
}

#ifdef CONFIG_SMP
/* IRQ3 = L1 interrupt for TP1 */
static void brcm_mips_int3_dispatch(struct pt_regs *regs)
{
	clear_c0_status(STATUSF_IP3);
	brcm_intc_dispatch(regs, CPUINT1C_TP1);
	set_c0_status(STATUSF_IP3);
}
#endif

/*
 * Performance counter interrupt
 */

#ifdef CONFIG_OPROFILE
static int performance_enabled = 0;
static void brcm_mips_performance_enable(unsigned int irq)
{
	/* Interrupt line shared with timer so don't really enable/disable it */
	performance_enabled = 1;
}

static void brcm_mips_performance_disable(unsigned int irq)
{
	/* Interrupt line shared with timer so don't really enable/disable it */
	performance_enabled = 0;
}

static void brcm_mips_performance_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_mips_performance_startup(unsigned int irq)
{ 
	brcm_mips_performance_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_mips_performance_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_mips_performance_enable(irq);
}

static struct irq_chip brcm_mips_performance_type = {
	typename: "BCM MIPS PERF",
	startup: brcm_mips_performance_startup,
	shutdown: brcm_mips_performance_disable,
	enable: brcm_mips_performance_enable,
	disable: brcm_mips_performance_disable,
	ack: brcm_mips_performance_ack,
	end: brcm_mips_performance_end,
	NULL
};

void brcm_mips_performance_dispatch(struct pt_regs *regs)
{
	if(performance_enabled)
		do_IRQ(BCM_LINUX_PERFCOUNT_IRQ, regs);
}

#endif

#ifdef CONFIG_OPROFILE
int test_all_counters(void);
#endif

/*
 * IRQ7 - MIPS timer interrupt
 */

void brcm_mips_int7_dispatch(struct pt_regs *regs)
{		
#ifdef CONFIG_OPROFILE
	if( performance_enabled && test_all_counters() ) 
	{
		brcm_mips_performance_dispatch(regs);
	}
#endif
	do_IRQ(smp_processor_id() ? BCM_LINUX_SYSTIMER_1_IRQ :
		BCM_LINUX_SYSTIMER_IRQ, regs);
}

static void brcm_mips_int7_enable(unsigned int irq)
{
	set_c0_status(STATUSF_IP7);
}

static void brcm_mips_int7_disable(unsigned int irq)
{
	clear_c0_status(STATUSF_IP7);
}

static void brcm_mips_int7_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_mips_int7_startup(unsigned int irq)
{ 
	brcm_mips_int7_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_mips_int7_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_mips_int7_enable(irq);
}

static struct irq_chip brcm_mips_int7_type = {
	name: "BCM MIPS TIMER",
	startup: brcm_mips_int7_startup,
	shutdown: brcm_mips_int7_disable,
	enable: brcm_mips_int7_enable,
	disable: brcm_mips_int7_disable,
	ack: brcm_mips_int7_ack,
	end: brcm_mips_int7_end,
	NULL
};

#ifdef CONFIG_SMP

/*
 * IRQ0, IRQ1 - software interrupts for interprocessor signaling
 */

static void brcm_mips_int0_enable(unsigned int irq)
{
	set_c0_status(STATUSF_IP0);
}

static void brcm_mips_int0_disable(unsigned int irq)
{
	clear_c0_status(STATUSF_IP0);
}

static void brcm_mips_int0_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_mips_int0_startup(unsigned int irq)
{ 
	brcm_mips_int0_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_mips_int0_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_mips_int0_enable(irq);
}

static struct irq_chip brcm_mips_int0_type = {
	name: "BCM MIPS INT0",
	startup: brcm_mips_int0_startup,
	shutdown: brcm_mips_int0_disable,
	enable: brcm_mips_int0_enable,
	disable: brcm_mips_int0_disable,
	ack: brcm_mips_int0_ack,
	end: brcm_mips_int0_end,
	NULL
};

void brcm_mips_int0_dispatch(struct pt_regs *regs)
{
	do_IRQ(BCM_LINUX_IPC_0_IRQ,regs);
}


static void brcm_mips_int1_enable(unsigned int irq)
{
	set_c0_status(STATUSF_IP1);
}

static void brcm_mips_int1_disable(unsigned int irq)
{
	clear_c0_status(STATUSF_IP1);
}

static void brcm_mips_int1_ack(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch */
}

static unsigned int brcm_mips_int1_startup(unsigned int irq)
{ 
	brcm_mips_int1_enable(irq);

	return 0; /* never anything pending */
}

static void brcm_mips_int1_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		brcm_mips_int1_enable(irq);
}

static struct irq_chip brcm_mips_int1_type = {
	name: "BCM MIPS INT1",
	startup: brcm_mips_int1_startup,
	shutdown: brcm_mips_int1_disable,
	enable: brcm_mips_int1_enable,
	disable: brcm_mips_int1_disable,
	ack: brcm_mips_int1_ack,
	end: brcm_mips_int1_end,
	NULL
};

void brcm_mips_int1_dispatch(struct pt_regs *regs)
{
	do_IRQ(BCM_LINUX_IPC_1_IRQ,regs);
}

#endif /* CONFIG_SMP */

/*
 * Broadcom specific IRQ setup
 */

void __init arch_init_irq(void)
{
	int irq;

	CPUINT1C->IntrW0MaskSet = 0xffffffff;
	CPUINT1C_TP1->IntrW0MaskSet = 0xffffffff;

	CPUINT1C->IntrW1MaskSet = 0xffffffff;
	CPUINT1C_TP1->IntrW1MaskSet = 0xffffffff;
#if L1_IRQS > 64
	CPUINT1C_TP1->IntrW2MaskSet = 0xffffffff;
	CPUINT1C->IntrW2MaskSet = 0xffffffff;
#endif
	
	change_c0_status(ST0_IE, 0);
 	
	/* Setup timer interrupt */
	irq_desc[BCM_LINUX_SYSTIMER_IRQ].status = IRQ_DISABLED;
	irq_desc[BCM_LINUX_SYSTIMER_IRQ].action = 0;
	irq_desc[BCM_LINUX_SYSTIMER_IRQ].depth = 1;
	irq_desc[BCM_LINUX_SYSTIMER_IRQ].chip = &brcm_mips_int7_type;

#ifdef CONFIG_SMP
	/* Setup timer interrupt */
	irq_desc[BCM_LINUX_SYSTIMER_1_IRQ].status = IRQ_DISABLED;
	irq_desc[BCM_LINUX_SYSTIMER_1_IRQ].action = 0;
	irq_desc[BCM_LINUX_SYSTIMER_1_IRQ].depth = 1;
	irq_desc[BCM_LINUX_SYSTIMER_1_IRQ].chip = &brcm_mips_int7_type;

	/* S/W IPC interrupts */
	irq_desc[BCM_LINUX_IPC_0_IRQ].status = IRQ_DISABLED;
	irq_desc[BCM_LINUX_IPC_0_IRQ].action = 0;
	irq_desc[BCM_LINUX_IPC_0_IRQ].depth = 1;
	irq_desc[BCM_LINUX_IPC_0_IRQ].chip = &brcm_mips_int0_type;

	irq_desc[BCM_LINUX_IPC_1_IRQ].status = IRQ_DISABLED;
	irq_desc[BCM_LINUX_IPC_1_IRQ].action = 0;
	irq_desc[BCM_LINUX_IPC_1_IRQ].depth = 1;
	irq_desc[BCM_LINUX_IPC_1_IRQ].chip = &brcm_mips_int1_type;
#endif

	/* Install all the 7xxx IRQs */
	for (irq = 1; irq <= 96; irq++) 
	{
		irq_desc[irq].status = IRQ_DISABLED;
		irq_desc[irq].action = 0;
		irq_desc[irq].depth = 1;
		irq_desc[irq].chip = &brcm_intc_type;
#ifdef CONFIG_SMP
		irq_desc[irq].affinity = cpumask_of_cpu(0);
#endif
	}

#ifdef CONFIG_OPROFILE
	/* profile IRQ */
	irq_desc[BCM_LINUX_PERFCOUNT_IRQ].status = IRQ_DISABLED;
	irq_desc[BCM_LINUX_PERFCOUNT_IRQ].action = 0;
	irq_desc[BCM_LINUX_PERFCOUNT_IRQ].depth = 1;
	irq_desc[BCM_LINUX_PERFCOUNT_IRQ].chip = &brcm_mips_performance_type;
	//brcm_mips_performance_enable(0);
#endif
	
	/* enable IRQ2 (this runs on TP0).  IRQ3 enabled during TP1 boot. */
	set_c0_status(STATUSF_IP2);

	/* enable L2 interrupts for UARTA, B, C */
	BDEV_SET(BCHP_IRQ0_IRQEN, BCHP_IRQ0_IRQEN_uarta_irqen_MASK |
		BCHP_IRQ0_IRQEN_uartb_irqen_MASK |
		BCHP_IRQ0_IRQEN_uartc_irqen_MASK);
}

asmlinkage void plat_irq_dispatch(struct pt_regs *regs)
{
	unsigned int pending = read_c0_cause() & read_c0_status();

	if (pending & STATUSF_IP7)
		brcm_mips_int7_dispatch(regs);
	else if (pending & STATUSF_IP2)
		brcm_mips_int2_dispatch(regs);
#ifdef CONFIG_SMP
	else if (pending & STATUSF_IP3)
		brcm_mips_int3_dispatch(regs);
	else if (pending & STATUSF_IP0)
		brcm_mips_int0_dispatch(regs);
	else if (pending & STATUSF_IP1)
		brcm_mips_int1_dispatch(regs);
#endif
	else
		spurious_interrupt(regs);

}

void dump_INTC_regs(void)
{
	unsigned int pendingIrqs0,pendingIrqs1,pendingIrqs2;
	unsigned int gDebugPendingIrq0 = 0,
		gDebugPendingIrq1 = 0,
		gDebugPendingIrq2 = 0;

	pendingIrqs0 = CPUINT1C->IntrW0Status;
	pendingIrqs0 &= ~(CPUINT1C->IntrW0MaskStatus);
	pendingIrqs1 = CPUINT1C->IntrW1Status;
	pendingIrqs1 &= ~(CPUINT1C->IntrW1MaskStatus);
#if L1_IRQS > 64
	pendingIrqs2 = CPUINT1C->IntrW2Status;
	pendingIrqs2 &= ~(CPUINT1C->IntrW2MaskStatus);
#endif

	printk("last pending0=%08x, last pending1=%08x, last pending2=%08x\n"
		"curPending0=%08x, curPending1=%08x, curPending2=%08x\n", 
		gDebugPendingIrq0, gDebugPendingIrq1, gDebugPendingIrq2,
		pendingIrqs0, pendingIrqs1, pendingIrqs2);
}
EXPORT_SYMBOL(dump_INTC_regs);
