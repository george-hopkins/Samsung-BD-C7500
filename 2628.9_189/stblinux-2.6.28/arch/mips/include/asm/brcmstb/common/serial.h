/*
 * include/asm/brcm/serial.h
 *
 * Copyright (C) 2001 Broadcom Corporation
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
 * UART defines for Broadcom eval boards
 *
 * 10-19-2001   SJH    Created 
 */

#ifndef __ASM_BRCM_SERIAL_H
#define __ASM_BRCM_SERIAL_H

#include <asm/brcmstb/common/brcmstb.h>

/*
 * UART base addresses
 */
#define BRCM_SERIAL1_BASE	UARTA_ADR_BASE
#define BRCM_SERIAL2_BASE	UARTB_ADR_BASE
#define BRCM_SERIAL3_BASE	UARTC_ADR_BASE

#if defined(CONFIG_MIPS_BCM7440)
#define BRCM_SERIAL4_BASE	UARTD_ADR_BASE
#endif


/*
 * IRQ stuff
 */
#define BRCM_SERIAL1_IRQ	BCM_LINUX_UARTA_IRQ
#define BRCM_SERIAL2_IRQ	BCM_LINUX_UARTB_IRQ
#define BRCM_SERIAL3_IRQ	BCM_LINUX_UARTC_IRQ
#if defined(CONFIG_MIPS_BCM7440)
#define BRCM_SERIAL4_IRQ	BCM_LINUX_UARTD_IRQ
#endif

#define UAIRQ			UPG_UA_IRQ //0x10
#define UBIRQ			UPG_UB_IRQ //0x08

/*
 * UPG Clock is determined by:
 * 				XTAL_FREQ * SYSPLL_FSEL / PB_SEL_VALUE
 *					= 24MHz * 9 / 8 = 27MHz
 */


/*
 * UART register offsets
 */
#if  defined(CONFIG_MIPS_BRCM76XX) || defined(CONFIG_MIPS_BCM7405)

// baud rate = (serial_clock_freq) / (16 * divisor).  
// The serial clock freq is 81MHz by default.
// For 115200, divisor = 44

// For Ikos, it is 14 however.
#define XTALFREQ1			81000000

#ifdef CONFIG_MIPS_BRCM_IKOS
#define SERIAL_DIVISOR_LSB	14
#else
#define SERIAL_DIVISOR_LSB	44
#endif

#define SERIAL_DIVISOR_MSB	0
#define SERIAL_DIVISOR		(SERIAL_DIVISOR_LSB | (SERIAL_DIVISOR_MSB << 8))		
#define BRCM_BASE_BAUD_16550		(115200 * SERIAL_DIVISOR)

#ifdef BASE_BAUD
#undef BASE_BAUD
#endif
#define BASE_BAUD			BRCM_BASE_BAUD_16550

#else

#define BRCM_BASE_BAUD		(XTALFREQ1/16) //1687500		/* (UPG Clock / 16) */
#endif

#define UART_RECV_STATUS	UART_RXSTAT //0x03	/* UART recv status register */
#define UART_RECV_DATA		UART_RXDATA //0x02	/* UART recv data register */
#define UART_BAUDRATE_HI	UART_BAUDHI //0x07	/* UART baudrate register */
#define UART_BAUDRATE_LO	UART_BAUDLO //0x06	/* UART baudrate register */
#define UART_XMIT_STATUS	UART_TXSTAT //0x05	/* UART xmit status register */
#define UART_XMIT_DATA		UART_TXDATA //0x04	/* UART xmit data register */

/*
 * UART control register definitions
 */
#define UART_PODD		PODD //1	/* odd parity */
#define UART_RE			RXEN //2	/* receiver enable */
#define UART_TE			TXEN //4	/* transmitter enable */
#define UART_PAREN		PAREN //8	/* parity enable */
#define UART_BIT8M		BITM8 //16	/* 8 bits character */

/*
 * Receiver status and control register definitions
 */
#define UART_RIE		RXINTEN //2	/* receiver interrupt enable */
#define UART_RDRF		RXDATARDY //4	/* receiver data register full flag */
#define UART_OVRN		OVERRUNERR //8	/* data overrun error */
#define UART_FE			FRAMEERR //16	/* framing error */
#define UART_PE			PARERR //32	/* parity error */

/*
 * Transmitter status and control register definitions
 */
#define UART_TDRE		TXDREGEMT //1	/* transmit data register empty flag */
#define UART_IDLE		IDLE //2	/* transmit in idle state   */
#define UART_TIE		TXINTEN //4	/* transmit interrupt enable */

#endif /* __ASM_BRCM_SERIAL_H */
