
/*
 * arch/mips/brcmstb/common/cfe_entry.S
 *
 * Copyright (C) 2001 Broadcom Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Low level interrupt code for Broadcom eval boards
 *
 */
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>


#define CFE_SEAL	0x43464531


	.text
	.set	noreorder
	.set	noat
	.align	5



		EXPORT(firmhandl)
firmhandl:
		.word  0
			
		EXPORT(firmentry)
firmentry:
		.word  0
			
		EXPORT(returnloc)
returnloc:
		.word  0
			
		EXPORT(cfe_seal)
cfe_seal:
		.word	0
		#		EXPORT(ret_value)

ret_value:
		.word	0

		.text
###################################################################################
		LEAF(cfe_start)
		.set	at
#ifdef CONFIG_KGDB
	# PR33365: enable JTAG interface for hot attach
		lui k0, 0xff30
		li  k1, 0x1a
		sw  k1, 0(k0)
#endif
		
	#Check to see if we were really launched by CFE
		sw  a3, cfe_seal
		li  t0, CFE_SEAL
		bne t0, a3, done
		nop

	# Store the parameters sent to us by CFE.

		sw  a0, firmhandl
		sw  a2, firmentry

done:    
		j	ra
		nop
		END(cfe_start)
###################################################################################
		NESTED(cfe_call, 64, sp)
#		LEAF(cfe_call)	
		#SAVE_TEMP
		sw   	ra, returnloc
#		move	v0,zero		#Always return success, since CFE ret is not trusted
#		
#if 1
		addiu sp,sp,-32
		sw	t0,4(sp)
		sw	t1,8(sp)
		sw	t2,12(sp)
		sw	t3,16(sp)
		sw	t4,20(sp)
		sw	a2,24(sp)
		sw	a3,28(sp)
#endif
		
	
#if 0		
        /* force kseg1, at this point, cache is not necessarily initialized */
        la      t0, 1f
        or      t0, 0xa0000000
        j       t0
        nop
1:
#endif
			
		#lw   a1, a0
		# lw   a1, cfeparam
		move	a1,a0
		lw   		a0, firmhandl
		lw   		t0, firmentry

#if 0
		#Don't do it if not called from CFE
		beq   t0,zero,not_from_CFE
		nop
#endif

		jal t0
		nop
#		sw	v0,ret_value
		nop

restore_all_and_ret:	#.set noat
		addiu	sp,sp,32
		lw	t0,4(sp)
		lw	t1,8(sp)
		lw	t2,12(sp)
		lw	t3,16(sp)
		lw	t4,20(sp)
		lw	a2,24(sp)
		lw	a3,28(sp)
#		RESTORE_TEMP
#		lw	v0,ret_value
		lw	ra, returnloc
		j	ra
		nop
		#.set at

not_from_CFE:
		li	v0,-1
		j	restore_all_and_ret
		nop
		END(cfe_call)

