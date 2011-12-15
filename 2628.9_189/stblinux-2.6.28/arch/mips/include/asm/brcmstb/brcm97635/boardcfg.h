/***************************************************************************
 *     Copyright (c) 1999-2009, Broadcom Corporation
 *     All Rights Reserved
 *     
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed with the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#ifndef BOARDCFG_H
#define BOARDCFG_H


/*************************************************************************/
/* Board ID String                                                       */
/*************************************************************************/
#define BOARD_ID_STR					"BCM97601"

/*************************************************************************/
/* MIPS Clock.                                                           */
/*************************************************************************/
#define CPU_CLOCK_RATE         			497000000		/* Hz */
#define XTALFREQ            			27000000

/************************************************************************/
/* Board specific stuff                                                 */
/************************************************************************/
#define BCM_SCRATCH_REGISTER 0xb040401c		/* SUN_TOP_CTRL_UNCLEARED_SCRATCH */

/* #define USE_UART_B_OUTPUT	*/				/* define this to use uart b */
/* #define DDR_32_BIT_MODE	*/			/* define this for 32-bit DDR */

/************************************************************************/
/* Endian Swapping Macros                                               */
/************************************************************************/

#ifndef SWAP_END16
 #define SWAP_END16(x) ( (((x)&0xff00)>>8) | (((x)&0x00ff)<<8) )
#endif

#ifndef SWAP_END32
#define SWAP_END32(x) ( (((x)&0xff000000)>>24) |(((x)&0x00ff0000)>>8) | (((x)&0x0000ff00)<<8)  |(((x)&0x000000ff)<<24) )               
#endif

#ifdef _LITTLE_ENDIAN_
#define BCM_LITTLE_ENDIAN_WORD(x)     x
#define BCM_LITTLE_ENDIAN_HALF(x)     x
#else
#define BCM_LITTLE_ENDIAN_WORD(x)     SWAP_END32(x)
#define BCM_LITTLE_ENDIAN_HALF(x)     SWAP_END16(x)
#endif

#ifdef USE_UART_B_OUTPUT
#define uart_out			uartb_out
#define uart_out_hex32		uartb_out_hex32
#define uart_out_str		uartb_out_str
#define uart_in				uartb_in
#else
#define uart_out			uarta_out
#define uart_out_hex32		uarta_out_hex32
#define uart_out_str		uarta_out_str
#define uart_in				uarta_in
#endif

#endif
