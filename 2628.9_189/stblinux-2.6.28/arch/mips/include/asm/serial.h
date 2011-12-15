/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1999 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#ifndef _ASM_SERIAL_H
#define _ASM_SERIAL_H

#include <asm/brcmstb/common/serial.h>

/*
 * This assumes you have a 1.8432 MHz clock for your UART.
 *
 * It'd be nice if someone built a serial card with a 24.576 MHz
 * clock, since the 16550A is capable of handling a top speed of 1.5
 * megabits/second; but this requires the faster clock.
 */
#ifndef BASE_BAUD
#define BASE_BAUD (1843200 / 16)
#endif

#if defined(CONFIG_MIPS_BRCM97XXX) 

#include <asm/brcmstb/common/serial.h>
#include <linux/serial_core.h>


#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)


/*
 * 16550A style UARTs
 */

#define _BRCM_16550_INIT(int, base)			\
	{ baud_base:  ( BRCM_BASE_BAUD_16550 ),		\
	  irq: int,								\
	  flags: STD_COM_FLAGS|UPF_SPD_SHI,  /* 115200 */  \
	  iomem_base: (u8 *) base,				\
	  iomem_reg_shift: 2,					\
	  io_type: SERIAL_IO_MEM /* 4 byte aligned */ \
	}
#if defined(CONFIG_MIPS_BCM7440) 
#define BRCM_SERIAL_PORT_DEFNS				\
	_BRCM_16550_INIT(BRCM_SERIAL1_IRQ, BRCM_SERIAL1_BASE),      \
	_BRCM_16550_INIT(BRCM_SERIAL2_IRQ, BRCM_SERIAL2_BASE),	    \
	_BRCM_16550_INIT(BRCM_SERIAL3_IRQ, BRCM_SERIAL3_BASE),	    \
	_BRCM_16550_INIT(BRCM_SERIAL4_IRQ, BRCM_SERIAL4_BASE),	
 
#elif defined(CONFIG_MIPS_BRCM76XX)
#define BRCM_SERIAL_PORT_DEFNS				\
	_BRCM_16550_INIT(BRCM_SERIAL1_IRQ, BRCM_SERIAL1_BASE),      \
	_BRCM_16550_INIT(BRCM_SERIAL2_IRQ, BRCM_SERIAL2_BASE),	    \
	_BRCM_16550_INIT(BRCM_SERIAL3_IRQ, BRCM_SERIAL3_BASE),

#elif defined (CONFIG_MIPS_BCM7405)
/* 3 16550A compatible UARTs */
#define BRCM_SERIAL_PORT_DEFNS				\
	_BRCM_16550_INIT(BRCM_SERIAL1_IRQ, BRCM_SERIAL1_BASE),		\
	_BRCM_16550_INIT(BRCM_SERIAL2_IRQ, BRCM_SERIAL2_BASE),      \
	_BRCM_16550_INIT(BRCM_SERIAL3_IRQ, BRCM_SERIAL3_BASE),	

#endif

#endif

#define SERIAL_PORT_DFNS                                \
        BRCM_SERIAL_PORT_DEFNS  

	

#endif /* _ASM_SERIAL_H */
