/**********************************************************************
**
** 	Broadcom Corp. Confidential
** 	Copyright 1998, 1999, 2000 Broadcom Corp.  All Rights Reserved.
**
**
**	File:         hw_init.S
**	Description: temp hack for cache init. 
**	Created:      QY
**
**
**
**	REVISION:
**
**		
**
**
**********************************************************************/
//#ifdef MIPS_SDE
//#include "archMips.h"
//#endif
//#include "mips.h"

	.text
	.align	2

#define C0_BRCM			    $22
//#define C0_CONFIG		    $16
#define Lock_I			    0x14

#define ICACHE_SIZE         0x8000      # 32K
#define ICACHE_LINESIZE     0x10

/*#define DCACHE_SIZE         0x1000      # 4K*/
#define DCACHE_SIZE         0x8000      # 32K
#define DCACHE_LINESIZE     0x10



/*---------------------------------------------------------------------------
    Name: __cacheInit()
 Purpose: Initialize the I & D cache
    Uses: a0, a1
---------------------------------------------------------------------------*/

    .set    noreorder
  	.globl  __cacheInit
  	.ent    __cacheInit
//    .set    mips3

__cacheInit:
        /* force kseg1 */
//        la      t0, 1f
//        li      t1, 0xa0000000
//        or      t0, t1
//        j       t0
//        nop
//1:

    move    a1, ra              # Save return address
    mfc0    t0, CP0_CONFIG
    li      t1, ~0x00000007
    and     t0, t1              # Clear existing cache enable state

#    or      t0, 0x01            # Enable WT dcache
#    or      t0, 0x02            # Disable dcache
    or      t0, 0x03            # Enable WB dcache
	mtc0    t0, CP0_CONFIG
	nop
	nop

//	li      t0, 0xC0000000		# enabled caches
	li      t0, 0xE3800400		# enabled caches
	mtc0    t0, C0_BRCM 
	nop
	nop

    bal     __cacheIflush
    nop

    bal     __cacheDflush
    nop

//////////////////////////
//	mfc0    t0, C0_CONFIG
//    li      t1, ~0x00000007
//    and     t0, t1              # Clear existing cache enable state

//#    or      t0, 0x01            # Enable WT dcache
//    or      t0, 0x02            # Disable dcache
//#    or      t0, 0x03            # Enable WB dcache
//	mtc0    t0, C0_CONFIG
//	nop
//	nop
//////////////////////////

    move    ra, a1
  	j	    ra
  	nop

    .set    reorder
	.end    __cacheInit


/*---------------------------------------------------------------------------
    Name: __cacheIflush()
 Purpose: Flush the Instruction Cache
    Uses: gp, t0
---------------------------------------------------------------------------*/

    .set    noreorder
  	.globl  __cacheIflush
  	.ent    __cacheIflush

__cacheIflush:
        /* force kseg1 */
//        la      t0, 1f
//        li      t1, 0xa0000000
//        or		t0, t1
//        j       t0
//        nop
//1:

        mtc0    zero, CP0_TAGLO
        nop
        nop

        .word 0x4080e001    # mtc0 zero,C0_TAGLO,1 - unsupported MIPS32 Instr
        nop
        nop

	li      t0, ICACHE_SIZE 
  	sub     t0, ICACHE_LINESIZE

iFlushLoop:
	nop
	nop
	nop
	nop
  	.word 0xbd080000    //cache   0x8, 0x0(t0)    # invalidate set
  	nop
  	nop
  	nop
  	nop
    bne     t0, zero, iFlushLoop
  	sub     t0, ICACHE_LINESIZE

  	j	    ra
  	nop

    .set    reorder
	.end    __cacheIflush

/*---------------------------------------------------------------------------
    Name: _cacheDflush()
 Purpose: Flush the Data Cache
    Uses: gp, t0, t1
---------------------------------------------------------------------------*/

  	.globl  __cacheDflush
  	.ent    __cacheDflush
	.set    noreorder

__cacheDflush:
        mtc0    zero, CP0_TAGLO
        nop
        nop

        .word 0x4080e001    # mtc0 zero,C0_TAGLO,1 - unsupported MIPS32 Instr
        nop
        nop
//
//    la	    t0, cachedAddress	# jump cacheable
//	li	    t1, ~0x20000000
//	and	    t0, t1
//	jr	    t0
//	nop

//cachedAddress:
  	li      t0, DCACHE_SIZE
  	sub     t0, t0, DCACHE_LINESIZE

dFlushLoop:
	nop
	nop
	nop
	nop
  	.word 0xbd090000    //cache   0x9, 0x0(t0)        # invalidate set
	nop
	nop
	nop
	nop

  	bne     t0, zero, dFlushLoop
  	sub     t0, DCACHE_LINESIZE

//    la	    t0, exitDFlush		# return uncacheable
//	li	    t1, 0x20000000
//	or	    t0, t1
//	jr	    t0
//	nop

exitDFlush:
  	j	    ra
  	nop

    .set    reorder
	.end    __cacheDflush



/******************************************************************************
 * Function: fill_I_cache_lines
 * 
 * Desrciption: Fills the I cache lines with the code from flash, also fill the
 * cache tag with the physical address of the code.
 *
 * Arguments: 
 *	a0: Address to fetch code from (flash address)
 *	a1: Number of Bytes to copy.
 * 
 *
*****************************************************************************/ 

.global cacheIfill
.ent 	cacheIfill
cacheIfill:

.set noreorder

	/* adjust byte count for 16 byte alignment */
	li		t0, 0x0f
	and		t0, a0, t0
	add		a1, a1, t0

	/* 16-byte align source address */
	li		t0, 0xfffffff0
	and		a0, a0, t0
	
	/* convert KSEG0 address to physical address, store in a0 */
	li		t0, 0x1FFFFFFF
	and		a0, a0, t0
	
1:	 
	/* fetch and lock line address in a0 */
	cache	Lock_I, 0x0(a0)

	/* increment dest addr in a0 */
	addiu	a0, ICACHE_LINESIZE
	
	/* decrement the copy size in a1 */
	addi	a1, -ICACHE_LINESIZE

	/* keep looping while a1 > 0 */                                             
	bgez	a1, 1b
	nop
	
	jr		ra
	nop

.set reorder	
.end cacheIfill

