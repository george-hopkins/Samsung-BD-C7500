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
	Setup timer interrupt handlers for secondary CPUs (threads). This allows
	us to not have to modify the main mips timer code.
when	who what
-----	---	----
3/24/06	TDT	Created
*/
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/timex.h>

#include <asm/mipsregs.h>
#include <asm/brcmstb/common/brcmstb.h>

// SMP Version
#ifdef CONFIG_SMP

void local_timer_interrupt(int , void *, struct pt_regs *);

extern unsigned int mips_hpt_frequency;

/* how many counter cycles in a jiffy */
static unsigned long cycles_per_jiffy;

/* Cycle counter value at the previous timer interrupt.. */
static unsigned int timerhi, timerlo;

/* expirelo is the count value for next CPU timer interrupt */
static unsigned int expirelo;

/*
 * Timer ack for an R4k-compatible timer of a known frequency.
 */
static void c0_timer_ack(void)
{
	unsigned int count;

	/* Ack this timer interrupt and set the next one.  */
	expirelo += cycles_per_jiffy;
	write_c0_compare(expirelo);

	/* Check to see if we have missed any timer interrupts.  */
	count = read_c0_count();
	if ((count - expirelo) < 0x7fffffff) {
		/* missed_timer_count++; */
		expirelo = count + cycles_per_jiffy;
		write_c0_compare(expirelo);
	}
}

/* For use both as a high precision timer and an interrupt source.  */
static void c0_hpt_timer_init(unsigned int count)
{
	count = read_c0_count() - count;
	expirelo = (count / cycles_per_jiffy + 1) * cycles_per_jiffy;
	write_c0_count(expirelo - cycles_per_jiffy);
	write_c0_compare(expirelo);
	write_c0_count(count);
}

/*
 * High-level timer interrupt service routines.  This function
 * is set as irqaction->handler and is invoked through do_IRQ.
 */
irqreturn_t timerx_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long j;
	unsigned int count;

	count = read_c0_count();
	c0_timer_ack();

	/* Update timerhi/timerlo for intra-jiffy calibration. */
	timerhi += count < timerlo;			/* Wrap around */
	timerlo = count;


	/*
	 * If jiffies has overflown in this timer_interrupt, we must
	 * update the timer[hi]/[lo] to make fast gettimeoffset funcs
	 * quotient calc still valid. -arca
	 *
	 * The first timer interrupt comes late as interrupts are
	 * enabled long after timers are initialized.  Therefore the
	 * high precision timer is fast, leading to wrong gettimeoffset()
	 * calculations.  We deal with it by setting it based on the
	 * number of its ticks between the second and the third interrupt.
	 * That is still somewhat imprecise, but it's a good estimate.
	 * --macro
	 */
	j = jiffies;
	if (j < 4) {
		static unsigned int prev_count;
		static int hpt_initialized;

		switch (j) {
		case 0:
			timerhi = timerlo = 0;
			c0_hpt_timer_init(count);
			break;
		case 2:
			prev_count = count;
			break;
		case 3:
			if (!hpt_initialized) {
				unsigned int c3 = 3 * (count - prev_count);

				timerhi = 0;
				timerlo = c3;
				c0_hpt_timer_init(count - c3);
				hpt_initialized = 1;
			}
			break;
		default:
			break;
		}
	}
	/*
	 * In UP mode, we call local_timer_interrupt() to do profiling
	 * and process accouting.
	 *
	 * In SMP mode, local_timer_interrupt() is invoked by appropriate
	 * low-level local timer interrupt handler.
	 */
	local_timer_interrupt(irq, dev_id, regs);

	return IRQ_HANDLED;
}

void __init brcm_timerx_setup(struct irqaction *irq)
{
	unsigned long count_r4k_cur;

	/* Needs to be calculated by first cpu */
	BUG_ON(!mips_hpt_frequency);

	/* Calculate cache parameters.  */
	cycles_per_jiffy = (mips_hpt_frequency + HZ / 2) / HZ;
	
	/* set count register to cause an interrupt */
	count_r4k_cur = (read_c0_count() + 1000);
	write_c0_compare(count_r4k_cur);

	/* we are using the cpu counter for timer interrupts */
	irq->dev_id = (void *) irq;

	setup_irq(BCM_LINUX_SYSTIMER_1_IRQ, irq);
	set_c0_status(IE_IRQ5);
}
#endif

