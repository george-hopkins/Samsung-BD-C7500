/*
 * Determine CPU clock speed for BCM97xxx boards
 *
 * Copyright (C) 2002,2003,2004,2005 Broadcom Corporation
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
 * THT from original Codes written by Matt Carlson
 */

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>


/* So as not to pick up C stuffs */
#define BCMTIMER_STRUCT_H	
#define BCMINTC_STRUCT_H
#define BCMDMA_H
#define BCMEBI_H
#define BCMUPG_H

#if defined(CONFIG_MIPS_BRCM76XX) || defined(CONFIG_MIPS_BCM7405) 
#define _ASMLANGUAGE


	#if defined(CONFIG_MIPS_BCM7405A0)
	#include <asm/brcmstb/brcm97405a0/bcmtimer.h>

	#elif defined(CONFIG_MIPS_BCM7405B0)
	#include <asm/brcmstb/brcm97405b0/bcmtimer.h>


	#elif defined(CONFIG_MIPS_BCM7440B0)
	#include <asm/brcmstb/brcm97440b0/bcmtimer.h>

	#elif defined(CONFIG_MIPS_BCM7440A0)
	#include <asm/brcmstb/brcm97440a0/bcmtimer.h>

	#elif defined(CONFIG_MIPS_BCM7601)
	#include <asm/brcmstb/brcm97601/bcmtimer.h>

	#elif defined(CONFIG_MIPS_BCM7630)
	#include <asm/brcmstb/brcm97630/bcmtimer.h>

	#elif defined(CONFIG_MIPS_BCM7635)
	#include <asm/brcmstb/brcm97635/bcmtimer.h>

	#else
	#error "Unsupported 7405/7440/7601/7630/7635 revision"
	#endif

#define TIMER_0_CTL TIMER_TIMER0_CTRL

#else
#error "unknown BCM STB chip!!!"
#endif

	.text
	.set	noreorder
	.set	noat
	.align	5

#define TIMER_0_STAT 0x14

#define TIMER_ENABLE 0x80000000
#define TIMER_CNTDWN 0x40000000

	/*
	 *  This function returns the number of CP0 ticks that have elapsed
	 *  over an 1/8th of a second interval.  This interval is determined
	 *  by the periph timer.  To get the CPU speed (in MHz), perform the
	 *  following calculation :
	 *
	 *  cpu_speed = (int)( GetMIPSFreq() * 27 * 1000 / 0x337F98 * 2) * 1000;
	 *
	 *  NOTES :   0x337F98 = 1 / 8 sec at 27Mhz
	 *            The MIPS counter increments once every _other_ instruction.
	 */


	LEAF(GetMIPSFreq)

		.set noreorder

	# Save ra.  LOADREL can stomp on it.
		move    	t3, ra

		li      	t0, TIMR_ADR_BASE

		# Reset the countdown timer
		li      	t1, 0x1
		sw      	t1, 0x0(t0)

		# Prefetch the critical loop
		la	t1, getstart
.set push
.set mips3
		cache   	0x14, 0x00(t1)
		cache   	0x14, 0x10(t1)
		cache   	0x14, 0x20(t1)
.set pop

getstart:

	# Set up the timer
#ifdef CONFIG_MIPS_BCM_NDVD
		# use 1/128 second, faster for the HW emulator
		li      	t1, 0x00337F9 | TIMER_CNTDWN | TIMER_ENABLE
#else
		li      	t1, 0x00337F98 | TIMER_CNTDWN | TIMER_ENABLE
#endif
		sw      	t1, TIMER_0_CTL(t0)

	# Set the CP0 Count register to zero.
	# This will be our MIPS timer.

		mtc0    	zero, CP0_COUNT
		nop

		li      	t2, 0x1

	# Wait for the timer to expire
1:
		lw      	t1, 0x0(t0)
		and     	t1, t1, t2
		beqz    	t1, 1b
		nop

	# Read the number of ticks that passed on the MIPS counter.

		mfc0    	v0, CP0_COUNT
		nop

	# Restore ra
		move    	ra, t3

		j   		ra
		nop

		.set reorder

END(GetMIPSFreq)


