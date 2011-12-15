/*
 * drivers/mtd/brcmnand/brcmnand_priv.h
 *
 *  Copyright (c) 2005,2006 Broadcom Corp.
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
 * Data structures for Broadcom NAND controller
 * 
 * when		who		what
 * 20060729	tht		Original coding
 */


#ifndef _BRCMNAND_PRIV_H_
#define _BRCMNAND_PRIV_H_


#include <asm/brcmstb/common/bcmtypes.h>
#include <linux/mtd/brcmnand.h> 
#include <asm/brcmstb/common/brcmstb.h>

typedef enum{
    EDU_INVALID,
    EDU_POLL,
    EDU_INTR,
    EDU_TASKLET
}edu_configuration_t;

typedef struct {
//Return values:
    //for reads & writes:
    uint32_t intr_status;
    bool needBBT;
    
    int opComplete;
    int error;

    //for reads:    
    int dataRead;
    int oobRead;

    //for writes:
    int written;
    int oobWritten;
    int done;
    int padlen;

//Input values:
    int cmd;
    int len;
    int winslice;
    int retries;

    uint8_t* pOobBuf;
    uint8_t* pDataBuf;

    struct mtd_info* pMtd;
    L_OFF_T offset;
} eduTaskletData_t;


#include <linux/irq.h>
#include <linux/wait.h>

#include <linux/spinlock.h>

#include <linux/interrupt.h>

#include "edu.h"
#include "eduproto.h"

#define RETURN_RD_ECC_STATE(err) \
    ((err == -EECCCOR) ? 0 : ((err == -EECCUNCOR)? -EBADMSG : err))


#if BRCM_CONFIG_MIPS_BCM7XXX
#include <asm/system.h> // For sync on MIPS24K
    #define PLATFORM_IOFLUSH_WAR()    __sync()
#else 
    #define PLATFORM_IOFLUSH_WAR()    
#endif


#define HAMMING_CODE_LEN                      3
#define NAND_CONTROLLER_DATA_BOUNDARY       512 
#define NAND_CONTROLLER_OOB_BOUNDARY         16

#define BRCMEDU_DMA_TRANSFER_ERROR           ( 5)
#define BRCMEDU_CORRECTABLE_ECC_ERROR        ( 4)
#define BRCMNAND_CORRECTABLE_ECC_ERROR       ( 2)
#define BRCMNAND_SUCCESS                     ( 1)

#define BRCMNAND_UNCORRECTABLE_ECC_ERROR     (-2)
#define BRCMNAND_TIMED_OUT                   (-3)
#define BRCMEDU_UNCORRECTABLE_ECC_ERROR      (-4)
#define BRCMEDU_MEM_BUS_ERROR                (-5)

// Level 1 interrupt where EDU INTR2 is hanging off
#define BCM_LINUX_CPU_INTR1_IRQ		(1+BCHP_HIF_CPU_INTR1_INTR_W0_STATUS_HIF_CPU_INTR_SHIFT)

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")
typedef struct eduData {
    struct mtd_info* mtd;
}eduData_t;


typedef struct eduIsrData {

    spinlock_t lock; // For SMP and future double buffering on Read.

    int cmd;	// 1 == Read, 0 == Write

	uint32_t mask;	/* Clear status mask */
	uint32_t expect;	/* Status on success */
	uint32_t error;	/* Status on error */
	uint32_t intr;		/* Interrupt bits */
	uint32_t status; 	/* Status read during ISR.  There may be several interrupts before completion */
	int opComplete;	/* Completion criterium */

	/* For debugging only */
	uint32_t flashAddr;
	uint32_t dramAddr;
	
    bool bWait;
} eduIsrData_t;

extern eduIsrData_t gEduIsrData;

void ISR_init(void);

uint32_t ISR_wait_for_completion(void);
uint32_t ISR_cache_is_valid(uint32_t clearMask);

static inline void ISR_enable_irq(void)
{
	// Clear status bits
	edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_CLEAR, gEduIsrData.mask);

	// Enable interrupt
	edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_MASK_CLEAR, gEduIsrData.intr);

}

static inline void ISR_disable_irq(uint32_t mask)
{
	/* Disable L2 interrupts */
	edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_MASK_SET, mask);
}



#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0

/*
 * 64 bit arithmetics 
 */
#include "asm-generic/gcclib.h"
#include "asm-generic/longlong.h"
#include <linux/bitmap.h>

#define LONGLONG_TO_BITS (sizeof(L_OFF_T)*BITS_PER_UNIT)

static inline L_OFF_T
__ll_constructor(long hi, unsigned long low)
{
	DIunion ull;

	ull.s.high = hi;
	ull.s.low = low;

	return ull.ll;
}



/*
 * Multiply 2 32-bit integer, result is 64bit
 */
static inline L_OFF_T
__ll_mult32(unsigned int left, unsigned int right)
{
	DIunion llw;
	
	umul_ppmm(llw.s.high, llw.s.low, left, right);
	return llw.ll;
}

/*
 * returns (left - right)
 */
static inline L_OFF_T 
__ll_sub(L_OFF_T left, L_OFF_T right)
{
	DIunion l, r, diff;

	l.ll = left;
	r.ll = right;

	sub_ddmmss(diff.s.high, diff.s.low, l.s.high, l.s.low, r.s.high, r.s.low);
	return diff.ll;
}

/*
 * returns (left - right)
 */
static inline L_OFF_T 
__ll_sub32(L_OFF_T left, int  right)
{
	DIunion l, r, diff;

	l.ll = left;
	r.s.low = right;
	r.s.high = 0;

	sub_ddmmss(diff.s.high, diff.s.low, l.s.high, l.s.low, r.s.high, r.s.low);
	return diff.ll;
}

/*
 * returns (int) (left - right)
 * Call this when we are sure that the result fits inside a 32bit integer,
 * trap error and halt kernel otherwise
 */
static inline int 
__ll_isub(L_OFF_T left, L_OFF_T right)
{
	DIunion l, r, diff;

	l.ll = left;
	r.ll = right;

	sub_ddmmss(diff.s.high, diff.s.low, l.s.high, l.s.low, r.s.high, r.s.low);
	// Could be negative too!  BUG_ON( diff.s.high != 0 );
	return (int) diff.ll;
}

/* 
 * returns (left + right)
 */
static inline L_OFF_T 
__ll_add(L_OFF_T left, L_OFF_T right)
{
	DIunion l, r, sum;

	l.ll = left;
	r.ll = right;

	add_ssaaaa(sum.s.high, sum.s.low, l.s.high, l.s.low, r.s.high, r.s.low);
	return sum.ll;
}

/*
 * returns (left + right), with right being a 32-bit integer
 */
static inline L_OFF_T 
__ll_add32(L_OFF_T left, int right)
{
	DIunion l, r, sum;

	l.ll = left;
	r.s.high = 0;
	r.s.low = right;

	add_ssaaaa(sum.s.high, sum.s.low, l.s.high, l.s.low, r.s.high, r.s.low);
	return sum.ll;
}

/*
 * Returns (ll >> shift)
 */
static inline L_OFF_T
__ll_RightShift(L_OFF_T ll, int shift)
{
	DIunion src, res;

	src.ll = ll;
	bitmap_shift_right((unsigned long*) &res, (unsigned long*) &src, shift, LONGLONG_TO_BITS);
	return res.ll;
}

/*
 * Returns (ul << shift) with ul a 32-bit unsigned integer.  Returned value is a 64bit integer
 */
static inline L_OFF_T
__ll_LeftShift32(unsigned long ul, int shift)
{
	DIunion src, res;

	src.s.low = ul;
	src.s.high = 0;
	bitmap_shift_left((unsigned long*) &res, (unsigned long*) &src, shift, LONGLONG_TO_BITS);
	return res.ll;
}

/*
 * Returns (left & right)
 */
static inline UL_OFF_T
__ll_and(UL_OFF_T left, UL_OFF_T right)
{
	UL_OFF_T res;
	bitmap_and((unsigned long*) &res, (unsigned long*) &left, (unsigned long*) &right, LONGLONG_TO_BITS);
	return res;
}

/*
 * Returns (left & right), with right being a 32-bit unsigned integer
 */
static inline UL_OFF_T
__ll_and32(UL_OFF_T left, unsigned long right)
{
	UL_OFF_T res;
	UL_OFF_T llr = __ll_constructor(0, right);
	bitmap_and((unsigned long*) &res, (unsigned long*) &left, (unsigned long*) &llr, LONGLONG_TO_BITS);
	return res;
}

/*
 * Returns (left & right)
 */
static inline UL_OFF_T
__ll_xor(UL_OFF_T left, UL_OFF_T right)
{
	UL_OFF_T res;
	bitmap_xor((unsigned long*) &res, (unsigned long*) &left, (unsigned long*) &right, LONGLONG_TO_BITS);
	return res;
}

/*
 * Returns (left & right), with right being a 32-bit unsigned integer
 */
static inline UL_OFF_T
__ll_xor32(UL_OFF_T left, unsigned long right)
{
	UL_OFF_T res;
	UL_OFF_T llr = __ll_constructor(0, right);
	bitmap_xor((unsigned long*) &res, (unsigned long*) &left, (unsigned long*) &llr, LONGLONG_TO_BITS);
	return res;
}

/*
 * Allow inline printing of 64 bit integer values
 */
static inline char*
__ll_sprintf(char* msg, L_OFF_T offset, unsigned long xor_invert_val)
{

	DIunion llw, llw_xor;
	llw.ll = offset;

    if (xor_invert_val == 0x1FC00000)
    {	  
        llw_xor.ll = __ll_xor32(offset, 0x1FC00000);

        sprintf(msg, "[phys=%08X:%08X, logic=%08X:%08X]",
                        llw.s.high, llw.s.low, 
                            llw_xor.s.high, llw_xor.s.low);
    }
    else
    {
        llw.ll = offset;
	    sprintf(msg, "%08x:%08x", llw.s.high, llw.s.low);
    }

	return msg;
}


static inline int
__ll_is_positive(L_OFF_T ll)
{
	DIunion u;

	u.ll = ll;
	return ((int) u.s.high > 0);
}

static inline int
__ll_is_zero(L_OFF_T ll)
{
	DIunion u;

	u.ll = ll;
	return (u.s.high == 0 && u.s.low == 0);
}

static inline int
__ll_is_greater(L_OFF_T left, L_OFF_T right)
{
	return __ll_is_positive(__ll_sub(left, right));
}

static inline int
__ll_is_less(L_OFF_T left, L_OFF_T right)
{
	return __ll_is_positive(__ll_sub(right, left));
}


/*
 * Returns low DWord
 */
static inline uint32_t
__ll_low(L_OFF_T ll)
{
	DIunion ull;

	ull.ll = ll;
	return (uint32_t) ull.s.low;
}

/*
 * Returns high DWord
 */
static inline int32_t
__ll_high(L_OFF_T ll)
{
	DIunion ull;

	ull.ll = ll;
	return (int32_t) ull.s.high;
}


#else // #if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0

/* Stubs for 32-bit arithmetics */

#define __ll_sprintf(msg, offset) (sprintf(msg, "%08x", offset), msg)
#define __ll_mult32(left, right) (left*right)
#define __ll_sub(left, right) (left - right)
#define __ll_sub32(left, right) (left - right)
#define __ll_isub(left, right) (left - right)
#define __ll_add(left, right) (left + right)
#define __ll_add32(left, right) (left + right)
#define __ll_RightShift(l, shift) ((unsigned long) l >> shift)
#define __ll_LeftShift32(l, shift) ((unsigned long) l << shift)
#define __ll_and(left, right) ((unsigned long) left & (unsigned long) right)
#define __ll_and32(left, right) ((unsigned long) left & right)
#define __ll_is_positive(v) (v > 0)
#define __ll_is_zero(v) (v == 0)
#define __ll_is_greater(l,r) (l > r)
#define __ll_is_less(l,r) (l < r)
#define __ll_low(l)	(l)
#define __ll_high(l)	(0)
#define __ll_constructor(h,l)	(l)
#endif //#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0



extern int brcmnand_prescan(int cs, uint32_t* flash_id);

/**
 * brcmnand_scan - [BrcmNAND Interface] Scan for the BrcmNAND device
 * @param mtd		MTD device structure
 * @param maxchips	Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 *
 * THT: For now, maxchips should always be 1.
 */
extern int brcmnand_scan(struct mtd_info *mtd , int maxchips );


/**
 * brcmnand_release - [BrcmNAND Interface] Free resources held by the BrcmNAND device
 * @param mtd		MTD device structure
 */
extern void brcmnand_release(struct mtd_info *mtd);

/* BrcmNAND BBT interface */
extern int brcmnand_scan_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd);
extern int brcmnand_default_bbt(struct mtd_info *mtd);

extern int brcmnand_update_bbt (struct mtd_info *mtd, loff_t offs);

extern void* get_brcmnand_handle(void);

extern void print_oobbuf(const unsigned char* buf, int len);
extern void print_databuf(const unsigned char* buf, int len);

#endif //#ifndef _BRCMNAND_PRIV_H_
