/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * Written by Ralf Baechle and Andreas Busse
 * Copyright (C) 1994 - 99, 2003, 06 Ralf Baechle
 * Copyright (C) 1996 Paul M. Antoine
 * Modified for DECStation and hence R3000 support by Paul M. Antoine
 * Further modifications by David S. Miller and Harald Koerfgen
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Kevin Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 */
#include <linux/init.h>
#include <linux/threads.h>

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/irqflags.h>
#include <asm/regdef.h>
#include <asm/page.h>
#include <asm/pgtable-bits.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>

#include <kernel-entry-init.h>

	/*
	 * inputs are the text nasid in t1, data nasid in t2.
	 */
	.macro MAPPED_KERNEL_SETUP_TLB
#ifdef CONFIG_MAPPED_KERNEL
	/*
	 * This needs to read the nasid - assume 0 for now.
	 * Drop in 0xffffffffc0000000 in tlbhi, 0+VG in tlblo_0,
	 * 0+DVG in tlblo_1.
	 */
	dli	t0, 0xffffffffc0000000
	dmtc0	t0, CP0_ENTRYHI
	li	t0, 0x1c000		# Offset of text into node memory
	dsll	t1, NASID_SHFT		# Shift text nasid into place
	dsll	t2, NASID_SHFT		# Same for data nasid
	or	t1, t1, t0		# Physical load address of kernel text
	or	t2, t2, t0		# Physical load address of kernel data
	dsrl	t1, 12			# 4K pfn
	dsrl	t2, 12			# 4K pfn
	dsll	t1, 6			# Get pfn into place
	dsll	t2, 6			# Get pfn into place
	li	t0, ((_PAGE_GLOBAL|_PAGE_VALID| _CACHE_CACHABLE_COW) >> 6)
	or	t0, t0, t1
	mtc0	t0, CP0_ENTRYLO0	# physaddr, VG, cach exlwr
	li	t0, ((_PAGE_GLOBAL|_PAGE_VALID| _PAGE_DIRTY|_CACHE_CACHABLE_COW) >> 6)
	or	t0, t0, t2
	mtc0	t0, CP0_ENTRYLO1	# physaddr, DVG, cach exlwr
	li	t0, 0x1ffe000		# MAPPED_KERN_TLBMASK, TLBPGMASK_16M
	mtc0	t0, CP0_PAGEMASK
	li	t0, 0			# KMAP_INX
	mtc0	t0, CP0_INDEX
	li	t0, 1
	mtc0	t0, CP0_WIRED
	tlbwi
#else
	mtc0	zero, CP0_WIRED
#endif
	.endm

	/*
	 * For the moment disable interrupts, mark the kernel mode and
	 * set ST0_KX so that the CPU does not spit fire when using
	 * 64-bit addresses.  A full initialization of the CPU's status
	 * register is done later in per_cpu_trap_init().
	 */
	.macro	setup_c0_status set clr
	.set	push
#ifdef CONFIG_MIPS_MT_SMTC
	/*
	 * For SMTC, we need to set privilege and disable interrupts only for
	 * the current TC, using the TCStatus register.
	 */
	mfc0	t0, CP0_TCSTATUS
	/* Fortunately CU 0 is in the same place in both registers */
	/* Set TCU0, TMX, TKSU (for later inversion) and IXMT */
	li	t1, ST0_CU0 | 0x08001c00
	or	t0, t1
	/* Clear TKSU, leave IXMT */
	xori	t0, 0x00001800
	mtc0	t0, CP0_TCSTATUS
	_ehb
	/* We need to leave the global IE bit set, but clear EXL...*/
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU0 | ST0_EXL | ST0_ERL | \set | \clr
	xor	t0, ST0_EXL | ST0_ERL | \clr
	mtc0	t0, CP0_STATUS
#else
	mfc0	t0, CP0_STATUS
	or	t0, ST0_CU0|\set|0x1f|\clr
	xor	t0, 0x1f|\clr
	mtc0	t0, CP0_STATUS
	.set	noreorder
	sll	zero,3				# ehb
#endif
	.set	pop
	.endm

	.macro	setup_c0_status_pri
#ifdef CONFIG_64BIT
	setup_c0_status ST0_KX 0
#else
	setup_c0_status 0 0
#endif
	.endm

	.macro	setup_c0_status_sec
#ifdef CONFIG_64BIT
	setup_c0_status ST0_KX ST0_BEV
#else
	setup_c0_status 0 ST0_BEV
#endif
	.endm

#ifndef CONFIG_NO_EXCEPT_FILL
	/*
	 * Reserved space for exception handlers.
	 * Necessary for machines which link their kernels at KSEG0.
	 */
	.fill	0x400
#endif

EXPORT(_stext)

#ifdef CONFIG_BOOT_RAW
	/*
	 * Give us a fighting chance of running if execution beings at the
	 * kernel load address.  This is needed because this platform does
	 * not have a ELF loader yet.
	 */
FEXPORT(__kernel_entry)
	j	kernel_entry
#endif

	__REF

NESTED(kernel_entry, 16, sp)			# kernel entry point

	kernel_entry_setup			# cpu specific setup

	setup_c0_status_pri

	/* We might not get launched at the address the kernel is linked to,
	   so we jump there.  */
	PTR_LA	t0, 0f
	jr	t0
0:

	/*
	 * need to initialize watch point registers
         * so that you don't get an unexpected watch point trap
	 */
       
	mtc0    zero, CP0_CAUSE
	mtc0    zero, CP0_WATCHLO, 0
	mtc0    zero, CP0_WATCHHI, 0
	mtc0    zero, CP0_WATCHLO, 1
	mtc0    zero, CP0_WATCHHI, 1
	mtc0    zero, CP0_WATCHLO, 2
	mtc0    zero, CP0_WATCHHI, 2
	mtc0    zero, CP0_WATCHLO, 3
	mtc0    zero, CP0_WATCHHI, 3
	ehb

#if defined(CONFIG_BOOT_RAW) && defined(CONFIG_MIPS_BCM_NDVD)	
	/* 
         * initialize the cache
	 * Everything in cache is LOST at this point. 
	 * cacheALibInit jumps to kseg 1 and disables the cache
	 */
	jal	cacheALibInit
	nop

#if 0
	jal	set_cache_size
	nop
#else
	nop
	nop
#endif

	/* now turn on the cache */
	MFC0    t0, CP0_CONFIG, 0
	li      t1, ~0x00000007
	and     t0, t1              	# Clear existing cache enable state
	or      t0, 0x03            	# Enable WB dcache
	MTC0    t0, CP0_CONFIG, 0
	ehb

	/* force kseg 0 */

	la	    t0, 1f		# jump  to cacheable space
	li	    t1, ~0x20000000
	and	    t0, t1
        j       	t0
        nop
1:
#endif


#ifdef CONFIG_MIPS_MT_SMTC
	/*
	 * In SMTC kernel, "CLI" is thread-specific, in TCStatus.
	 * We still need to enable interrupts globally in Status,
	 * and clear EXL/ERL.
	 *
	 * TCContext is used to track interrupt levels under
	 * service in SMTC kernel. Clear for boot TC before
	 * allowing any interrupts.
	 */
	mtc0	zero, CP0_TCCONTEXT

	mfc0	t0, CP0_STATUS
	ori	t0, t0, 0xff1f
	xori	t0, t0, 0x001e
	mtc0	t0, CP0_STATUS
#endif /* CONFIG_MIPS_MT_SMTC */

        /*
         * Clear BSS.
         * This has the side effect of flushing the cache.
         * If kernel_entry was entered with the cache enabled,
	 * the cache has to be flushed before it is reinitialized,
	 * or the cache contents will be lost.
	 * This also means we need to flush the last part of BSS,
	 * so write the first  64K a second time.
	 */

	PTR_LA		t0, __bss_start		# clear .bss
	LONG_S		zero, (t0)
	PTR_LA		t1, __bss_stop - LONGSIZE
1:
	PTR_ADDIU	t0, LONGSIZE
	LONG_S		zero, (t0)
	bne		t0, t1, 1b

	PTR_LA		t0, __bss_start		# clear .bss
	LONG_S		zero, (t0)
	PTR_LA		t1, __bss_start - LONGSIZE + (1 << 16)
2:
	PTR_ADDIU	t0, LONGSIZE
	LONG_S		zero, (t0)
	bne		t0, t1, 2b

#if !defined(CONFIG_BOOT_RAW) && defined(CONFIG_MIPS_BCM_NDVD)
	/* 
	 * Initialize the cache.
	 * Everything is cache is LOST at this point
	 * cacheALibInit jumps to kseg 1 and disables the cache
	 */
	jal	cacheALibInit
	nop

#if 0
	jal	set_cache_size
	nop
#else
	nop
	nop
#endif

	/* now turn on the cache */
	MFC0    t0, CP0_CONFIG, 0
	li      t1, ~0x00000007
	and     t0, t1              # Clear existing cache enable state
	or      t0, 0x03            # Enable WB dcache
	MTC0    t0, CP0_CONFIG, 0
	ehb

#endif

	LONG_S		a0, fw_arg0		# firmware arguments
	LONG_S		a1, fw_arg1
	LONG_S		a2, fw_arg2
	LONG_S		a3, fw_arg3

	MTC0		zero, CP0_CONTEXT	# clear context register
	PTR_LA		$28, init_thread_union
	PTR_LI		sp, _THREAD_SIZE - 32
	PTR_ADDU	sp, $28
	set_saved_sp	sp, t0, t1
	PTR_SUBU	sp, 4 * SZREG		# init stack pointer

	j		start_kernel
	END(kernel_entry)

#
# reduce cache size
#
NESTED(set_cache_size, 16, sp)			
        .set    noreorder

	# set bit 19 in config regester 0 to enable writing config register 1
	mfc0    t0, CP0_CONFIG, 0
	lui	t1, 0x8
	or	t0, t0, t1
	mtc0	t0, CP0_CONFIG, 0
	nop

	# change number of cache sets per way in config reg 1
	# not all sizes are valid on all chip implementations
	# IS: bits 24:22	I$
	# DS: bits 15:13	D$
	# 0:  64 ( 8KB)		Does not appear to work on r24k
	# 1: 128 (16KB)
	# 2: 256 (32KB)
	# 3: 512 (64KB)
	# 4-7: reserved
#define CACHE_SIZE 80

	mfc0	t0, CP0_CONFIG, 1
	lui	t1, 0xfe3f	# ~IS bits
	ori	t1, 0x1fff	# ~DS bits
	and	t0, t0, t1	# clear bits

#if (CACHE_SIZE == 80)		// 64K I, 16K D
	lui	t1, 0xc0	# IS = 3
	ori	t1, 0x2000	# DS = 1
#elif (CACHE_SIZE == 32)	// 32K I, 32K D
	lui	t1, 0x80	# IS = 2
	ori	t1, 0x4000	# DS = 2
#elif (CACHE_SIZE == 16)	// 16K I, 16K D
	lui	t1, 0x40	# IS = 1
	ori	t1, 0x2000	# DS = 1
#elif (CACHE_SIZE == 8)		// 8K I, 8K D
	move	t1, zero	# IS = 0
	nop			# DS = 0
#else
	lui	t1, 0xc0	# IS = 3
	ori	t1, 0x6000	# DS = 3
#endif

	or	t0, t1, t0	# set bits
	mtc0	t0, CP0_CONFIG, 1

	ehb

	j	ra
	END(set_cache_size)

	__FINIT

#ifndef CONFIG_HOTPLUG_CPU
	__CPUINIT
#endif

#ifdef CONFIG_SMP
/*
 * SMP slave cpus entry point.  Board specific code for bootstrap calls this
 * function after setting up the stack and gp registers.
 */
NESTED(smp_bootstrap, 16, sp)
#ifdef CONFIG_MIPS_MT_SMTC
	/*
	 * Read-modify-writes of Status must be atomic, and this
	 * is one case where CLI is invoked without EXL being
	 * necessarily set. The CLI and setup_c0_status will
	 * in fact be redundant for all but the first TC of
	 * each VPE being booted.
	 */
	DMT	10	# dmt t2 /* t0, t1 are used by CLI and setup_c0_status() */
	jal	mips_ihb
#endif /* CONFIG_MIPS_MT_SMTC */
	setup_c0_status_sec
	smp_slave_setup
#ifdef CONFIG_MIPS_MT_SMTC
	andi	t2, t2, VPECONTROL_TE
	beqz	t2, 2f
	EMT		# emt
2:
#endif /* CONFIG_MIPS_MT_SMTC */
	j	start_secondary
	END(smp_bootstrap)
#endif /* CONFIG_SMP */

	__FINIT
