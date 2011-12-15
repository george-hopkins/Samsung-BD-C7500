/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 * 
 * Very simple boot loader codes for Broadcom's PCI slave boards.
 *
 * Copyright (C) 2006 Broadcom Corp.
 * Written by Ton Truong and Lee Tokuda
 */

#if 0
// Already included in head.S which include us 

#include <linux/config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>

// #ifdef CONFIG_MIPS_BCM7402S
#include <asm/brcmstb/brcm97401a0/bcmmips.h>
#include <asm/brcmstb/brcm97401a0/bcmuart.h>

#define BCHP_SUN_TOP_CTRL_SW_RESET 	            		0x00404010
#define BCHP_SUN_TOP_CTRL_SEMAPHORE_8          	0x00404048
#define BCHP_SUN_GISB_ARB_TIMER                      	0x0040600c
#define BCHP_SUN_GISB_ARB_REQ_MASK               		0x00406008
#define BCHP_FMISC_SOFT_RESET                        	0x00104000 
#define BCHP_MEMC_0_DDR_PLL_FREQ_DIVIDER_RESET	0x0010683c
#define BCHP_MEMC_0_DDR_PLL_FREQ_CNTL            	0x00106818
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5         	0x00404098
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8         	0x004040a4 

#define BCHP_SUN_TOP_CTRL_RESET_CTRL			0x00404008
//#define BCHP_SUN_TOP_CTRL_SW_RESET			0x00404014
#define BCHP_VCXO_0_RM_CONTROL                   		0x00181700 /* Rate Manager Controls */
#define UART_ADR_BASE		0xb04001a0		// UARTB

#endif


        /* defines */
#define INITIAL_SR      (ST0_CU0 | ST0_CU1 | ST0_BEV | ST0_ERL)     

#define RelAddrPrintChar(reg, c) \
		li		a0, c;			\
		la		reg, _writeasm;	\
		add		reg, reg, s3;	\
		jalr	ra, reg;		\
		nop;
		
#define RelAddrPrintHex32(reg)	\
		la		reg, uart_out_hex32;  \
		add		reg, reg, s3;	\
		jalr	ra, reg;		\
		nop;
		
#define RelAddrPrintString(reg, s) \
		la		a0, s;			\
		add		a0, a0, s3;		\
		la		reg, uart_out_str; \
		add		reg, reg, s3;	\
		jalr	ra, reg;		\
		nop;
		


        

/*******************************************************************************
*
* pci_slave_init - entry point for PCI slave
*
*/
  	.globl  pci_slave_init
  	.ent    pci_slave_init
pci_slave_init:
        .set    noreorder
 
		li		t0, INITIAL_SR /* (SR_CU0|SR_CU1|SR_BEV|SR_ERL) */
		mtc0	t0, CP0_STATUS
        
        	mtc0    zero,CP0_CAUSE           # clear software interrupts
        	mtc0	zero, CP0_WATCHLO
		mtc0	zero, CP0_WATCHHI
        	nop                             # paranoia

		/**----- check code in RAM or ROM/flash ------------------------**/


#if 1
/* THT: We need this for the calibration, but we should do this from the host side */
		/* Soft reset VCX0 */
		li   	t0,0xb0181700		//BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_VCXO_0_RM_CONTROL)
		lw   	t3, 0(t0)

		li  	t2, 0x00001000		/* VCXO_0_RM_CTRL[12] |= 1 */
		or  	t3, t3,t2
		sw   	t3, 0(t0)

		li		t2, 0x00002000		/* VCXO_0_RM_CTRL[13] |= 1 */
		or  	t3, t3,t2
		sw   	t3, 0(t0)

		li 		t2, 0xFFFFDFFF		/* VCXO_0_RM_CTRL[13] &= 0 */
		and  	t3, t3,t2
		sw   	t3, 0(t0)

		li		t2, 0x00002000		/* VCXO_0_RM_CTRL[13] |= 1 */
		or  	t3, t3,t2
		sw    	t3, 0(t0)

		li		t2, 0xFFFFEFFF		/* VCXO_0_RM_CTRL[12] &= 0 */
		and  	t3, t3,t2
		sw    	t3, 0(t0)

		/* Set VCXO 0 for 27 MHz */
  #if BCHP_CHIP==7400
		lw   	t3, 0x14(t0)
		and  	t3,t3,~0xfffffffc /* 31:02 Control */
		or   	t3,t3, 0x00002000 /* 0x800 << 2 for 27 MHz */
		sw   	t3, 0x14(t0) 
  #endif
#endif
2:        

        	li      a0, '.'
        	bal     _writeasm
        	nop



		/**----- setup CONFIG registers ---------------------------------**/
		
		.set	noreorder
		li      a0, 'c'
		bal     _writeasm
		nop

		mfc0	t0, CP0_CONFIG
		nop
		and		t0, ~0x07
		or		t0, 3		/* cached, write-back */
		mtc0	t0, CP0_CONFIG
		nop
		
        	li      a0, 'U'
        	bal     _writeasm
		nop

		/**----- Initialize the cache -----------------------------------**/

		/* initialize cache and turn it on */
		bal		__cacheInit
		nop
        
        li      a0, 'C'
        bal     _writeasm
        nop
        

		/**----- Pre-cache -----------------------------------**/
		
		/* fill cache with "clear exception vector offset" code */
		la		a0, clear_exc_vec_offset
		li		a1, 0x40
		bal		cacheIfill
		nop


        li      a0, '1'
        bal     _writeasm
        nop
        
		la		t0, clear_exc_vec_offset
		jr		t0
		nop

1:

clear_exc_vec_offset:        
		/* set EXCEPTION_VECTOR_OFFSET register to 0 */
		li   	t0, 0xb0000404			//BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_MISB_BRIDGE_EXCEPTION_VECTOR_OFFSET)
		li		t1, 0
		sw		t1, 0(t0)
		
		/* load reinit_cache address */
		la		t0, reinit_cache
		
		/* convert to uncached address */
		li		t1, 0x20000000
		or		t0, t0, t1
		jr		t0
		nop
		nop
		nop
		nop
		nop
		
		
reinit_cache:		
	        li      a0, '2'
        	bal     _writeasm
       	 nop
       	 
		/* initialize cache and turn it on */
		bal 	__cacheInit
		nop
        
        	li      a0, '\r'
        	bal     _writeasm
        	nop


        	li      a0, '\n'
        	bal     _writeasm
        	nop
        	
		la	ra,return_from_pci_slave_init
		j ra
		nop
		
		.set reorder
END(pci_slave_init)

/*******************************************************************************
*
* BcmMsg - Message that is displayed to serial terminal if _writeasm function
*          enabled.
*
*/
        .align  2
        .ent BcmMsg
BcmMsg:
        move    a1, ra

        li      a0, '\r'
        bal     _writeasm
        nop
        li      a0, '\n'
        bal     _writeasm
        nop
        li      a0, 'B'
        bal     _writeasm
        nop
        li      a0, 'C'
        bal     _writeasm
        nop
        li      a0, 'M'
        bal     _writeasm
        nop
        li      a0, '9'
        bal     _writeasm
        nop
        li      a0, '7'
        bal     _writeasm
        nop
        li      a0, '4'
        bal     _writeasm
        nop
        li      a0, '0'
        bal     _writeasm
        nop
        #if BCHP_CHIP==7400
        li      a0, '0'
        #else
        li      a0, '1'
        #endif
        nop
        bal     _writeasm
        move    a0, a2                  #Identify msg src
        bal     _writeasm
        nop

        li      a0, '\r'
        bal     _writeasm
        nop
        li      a0, '\n'
        bal     _writeasm
        nop

        move    ra, a1
        jr      ra
        .end BcmMsg
        
/*
#************************************************************************
#* _writeasm: Write character to Serial Port                            *
#*                                                                      *
#*      Syntax: _writeasm (char)                                        *
#*        Note:                                                         *
#************************************************************************
*/
      .align  2
        .globl _writeasm
        .ent _writeasm

_writeasm:  
        li      t0, UART_ADR_BASE
#if BCHP_CHIP==7400
        li      t2, THRE
1:      lw      t1, UART_SDW_LSR(t0)
#else
        li      t2, TXDREGEMT
1:      lw      t1, UART_TXSTAT(t0)
#endif
        and     t1, t1, t2
        bne     t1, t2, 1b
        nop
        #if BCHP_CHIP==7400
        sw      a0, UART_SDW_THR(t0)
        #else
        sw      a0, UART_TXDATA(t0)
        #endif
        j       ra
        nop
		.set   reorder

        .end _writeasm

      .align  2
        .globl uartb_out
        .ent uartb_out

uartb_out:
        li      t0, UARTB_ADR_BASE
        #if BCHP_CHIP==7400
        li      t2, THRE
1:      lw      t1, UART_SDW_LSR(t0)
        #else
        li      t2, TXDREGEMT
1:      lw      t1, UART_TXSTAT(t0)
        #endif
        and     t1, t1, t2
        bne     t1, t2, 1b
        nop
        #if BCHP_CHIP==7400
        sw      a0, UART_SDW_THR(t0)
        #else
        sw      a0, UART_TXDATA(t0)
        #endif
        j       ra
        nop
		.set   reorder

        .end uartb_out


/************************************************************************
 *                                                                      *
 *     uart_out_hex32:  Print out a 32-bit value in hex                *
 *                                                                      *
 *     SYNTAX: void  uart_out_hex32(unsigned short s)                  *
 *     RETURNS:                                                         *
 *                                                                      *
 ************************************************************************/

	.globl	uart_out_hex32
 	.ent	uart_out_hex32
uart_out_hex32: 

   .set noreorder
   li    t0, 32 
   li    t3, UART_ADR_BASE
   #if BCHP_CHIP==7400
   li    t4, THRE
   #else
   li    t4, 1
   #endif
uart_out_hex32_1:
   addiu t0, t0, -4
   srlv  t1, a0, t0   // t1=a0>>t0
   andi  t1, t1, 0xf  // t1=(a0>>t0) && 0xf
   addi  t2, t1, -10
   bgez  t2, uart_out_hex32_2
	nop
   addi  t2, t1, '0'
   b     uart_out_hex32_3
	nop
uart_out_hex32_2:
   addi  t2, t2, 'A'

uart_out_hex32_3:
   //output t2
   #if BCHP_CHIP==7400
   lw    t1, UART_SDW_LSR(t3)
   #else
   lw    t1, UART_TXSTAT(t3)
   #endif
   and   t1, t1, t4
   bne   t1, t4, uart_out_hex32_3
    nop
   #if BCHP_CHIP==7400
   sw    t2, UART_SDW_THR(t3)
   #else
   sw    t2, UART_TXDATA(t3)
   #endif

   bne   t0, $0, uart_out_hex32_1
	nop
   jr    ra
    nop
   .set reorder    
	.end	uart_out_hex32


/*
#************************************************************************
#* initSerial:  Initialize Serial Port to 115200,N,8,1                  *
#* 		There are three UART ports and UART1/B is recommened to be used *
#* 		for debug port                                                  *                  *
#*      Syntax: initSerial(void)                                        *
#*        Note:                                                         *
#************************************************************************
*/

        .align  2
        .ent initSerial
initSerial:
        .set    noreorder


#ifdef CONFIG_MIPS_BCM7402S
		/* Set MUX to select UART1/B pin functions */
		li   t0,BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5)
		li   t3,0x00048000 /* 20:18 --> 1 for RXD1 17:15 --> 1 for TXD1	*/
		sw   t3,0(t0)    

		/* Set MUX to select UART2/C pin functions */
		li   t0,BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8)
		li   t3,0x00040200 /* 20:18 --> 1 for RXD2 11:09 --> 1 for TXD2	*/
		sw   t3,0(t0)
   
		/* set baud rate */
		li		t0, UART_ADR_BASE
		li		t3, BAUD_115200_LO 
		sw		t3, UART_BAUDLO(t0)
		li		t3, BAUD_115200_HI 
		sw		t3, UART_BAUDHI(t0)

		/* 8 bits, 1 stop, no parity, enable tx/rx */
		move	t1, zero
		sw		t1, UART_RXSTAT(t0)
		sw		t1, UART_TXSTAT(t0)
		li		t1, BITM8 | RXEN | TXEN
		sw		t1, UART_CONTROL(t0)   

		/* Set MUX to select UART2/C pin functions */
		li   t0,BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8)
		li   t3,0x00080400 /* 20:18 --> 1 for TXD2 11:09 --> 1 for RXD2 */
		sw   t3,0(t0)  

		/* set baud rate */
		li		t0, UARTC_ADR_BASE
		li		t3, BAUD_115200_LO 
		sw		t3, UART_BAUDLO(t0)
		li		t3, BAUD_115200_HI 
		sw		t3, UART_BAUDHI(t0)

		/* 8 bits, 1 stop, no parity, enable tx/rx */
		move	t1, zero
		sw		t1, UART_RXSTAT(t0)
		sw		t1, UART_TXSTAT(t0)
		li		t1, BITM8 | RXEN | TXEN
		sw		t1, UART_CONTROL(t0)


#elif defined( CONFIG_MIPS_BCM7402B0S )
		/* Set MUX to select UART0/B pin functions */
		li   t0,BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7)
		li   t3,0x00012000 /* 14:12 --> 1 for RXD1 11:09 --> 1 for TXD1	*/
		sw   t3,0(t0)    

		/* Set MUX to select UART1/C pin functions */
		li   t0,BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_10)
		li   t3,0x00040200 /* 20:18 --> 1 for RXD2 11:09 --> 1 for TXD2	*/
		sw   t3,0(t0)
   
		/* set baud rate */
		li		t0, UART_ADR_BASE
		li		t3, BAUD_115200_LO 
		sw		t3, UART_BAUDLO(t0)
		li		t3, BAUD_115200_HI 
		sw		t3, UART_BAUDHI(t0)

		/* 8 bits, 1 stop, no parity, enable tx/rx */
		move	t1, zero
		sw		t1, UART_RXSTAT(t0)
		sw		t1, UART_TXSTAT(t0)
		li		t1, BITM8 | RXEN | TXEN
		sw		t1, UART_CONTROL(t0)   

		/* Set MUX to select UART2/C pin functions */
		li   t0,BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_10)
		li   t3,0x00080400 /* 20:18 --> 1 for TXD2 11:09 --> 1 for RXD2 */
		sw   t3,0(t0)  

#if 0
/* UART1/C (/dev/ttyS1) will be set by Linux */
		/* set baud rate */
	

		/* 8 bits, 1 stop, no parity, enable tx/rx */
		
#endif

#else
#error "Unsupported platform"
#endif

        j		ra
        nop
        .set	reorder
        .end initSerial



.globl wait
.ent wait
.set noreorder
.set at
wait:

    /* Set CPU clock to 1/8th speed: Bits 23:24 = 11 */
	mfc0    a0, $22, 4        # 22,4
	li		a1,0x01800000
	or		a0,a0,a1
	mtc0    a0, $22, 4        # 22,4
	
    wait
    nop
    j 		ra
    nop

.set noat
.set reorder
.end wait


.globl power_up_mips
.ent power_up_mips
.set noreorder
.set at
power_up_mips:

    /* Set CPU clock to 1/8th speed: Bits 23:24 = 11 */
	mfc0    a0, $22, 4        # 22,4
	li		a1,~0x01800000
	and		a0,a0,a1
	mtc0    a0, $22, 4        # 22,4
    nop
    j 		ra
    nop

.set noat
.set reorder
.end power_up_mips


