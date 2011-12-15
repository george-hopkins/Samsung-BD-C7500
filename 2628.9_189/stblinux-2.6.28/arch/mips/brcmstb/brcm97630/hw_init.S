/**********************************************************************
*
*
*	File:         hw_init.S
* Copyright (C) 1998-2005 Broadcom Corporation
*
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
**	Description: HW init.
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
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/cacheops.h>

	.text
	.align	2

#define C0_CONFIG		    $16


/* 7440B0 */
#define ICACHE_SIZE         0x10000      # 64K
#define ICACHE_LINESIZE     0x20
#define DCACHE_SIZE         0x10000      # 64K
#define DCACHE_LINESIZE     0x20

  	# Cache initialization routine
	# Long and needed on HW 
	# Can be skipped if using magic simulation cache flush
	


/*---------------------------------------------------------------------------
    Name: _cacheInit()
 Purpose: Initialize the I & D cache
    Uses: t0, t1, t2
---------------------------------------------------------------------------*/

    .set    noreorder
  	.globl  cacheALibInit
  	.ent    cacheALibInit

cacheALibInit:
        /* force kseg1 */
        la      t0, 1f
        or      t0, 0xa0000000
        j       t0
        nop
1:

	move    t2, ra              # Save return address
	mfc0    t0, C0_CONFIG
	li      t1, ~0x00000007
	and     t0, t1              # Clear existing cache enable state
	
	#    or      t0, 0x01            # Enable WT dcache
	#    or      t0, 0x02            # Disable dcache
	#    or      t0, 0x03            # Enable WB dcache
	or      t0, 0x02            	# uncached
	mtc0    t0, C0_CONFIG
	ehb


        mtc0    zero, CP0_TAGLO		// L1 I
	mtc0	zero, CP0_TAGLO, 2	// L1 D
	mtc0	zero, CP0_TAGLO, 4	// L2
        mtc0    zero, CP0_TAGLO, 1	// DATALO L1 I
	mtc0	zero, CP0_TAGLO, 3	// DATA L1 D
	mtc0	zero, CP0_TAGLO, 5	// DATA L2
        ehb

	jal     _cacheIflush
	nop
	
	jal     _cacheDflush
	nop


	move    ra, t2
	j	    ra
	nop

	.set    reorder
	.end    cacheALibInit


/*---------------------------------------------------------------------------
    Name: _cacheIflush()
 Purpose: Flush the Instruction Cache
    Uses: gp, t0
---------------------------------------------------------------------------*/

	.set    noreorder
  	.globl  _cacheIflush
  	.ent    _cacheIflush

_cacheIflush:
        /* force kseg1 */
        la      t0, 1f
        or      t0, 0xa0000000
        j       t0
        nop
1:
	li      t0, ICACHE_SIZE 
  	sub     t0, ICACHE_LINESIZE
	lui	t1, 0x8000	# get a kseg0 address for cacheops

iFlushLoop:
	nop
	nop
	nop
	nop
#  	.word 0xbd080000    //cache   0x8, 0x0(t0)    # invalidate set
	cache	0x8, 0x0(t1)
  	nop
  	nop
  	nop
  	nop
	add	t1, ICACHE_LINESIZE
	bne     t0, zero, iFlushLoop
  	sub     t0, ICACHE_LINESIZE

  	j	    ra
  	nop

	.set    reorder
	.end    _cacheIflush

/*---------------------------------------------------------------------------
    Name: _cacheDflush()
 Purpose: Flush the Data Cache
    Uses: gp, t0, t1
---------------------------------------------------------------------------*/

  	.globl  _cacheDflush
  	.ent    _cacheDflush
	.set    noreorder

_cacheDflush:

	la	    t0, cachedAddress	# jump cacheable
	li	    t1, ~0x20000000
	and	    t0, t1
	jr	    t0
	nop

cachedAddress:
  	li      t0, DCACHE_SIZE
  	sub     t0, t0, DCACHE_LINESIZE
	lui	t1, 0x8000	# get a kseg0 address for cacheops

dFlushLoop:
	cache	0x9, 0x0(t1)	# index store tag - D$
	nop
	nop
	nop
	nop

	add	t1, DCACHE_LINESIZE
  	bne     t0, zero, dFlushLoop
  	sub     t0, DCACHE_LINESIZE

	la	    t0, exitDFlush		# return uncacheable
	li	    t1, 0x20000000
	or	    t0, t1
	jr	    t0
	nop

exitDFlush:
  	j	    ra
  	nop

	.set    reorder
	.end    _cacheDflush

