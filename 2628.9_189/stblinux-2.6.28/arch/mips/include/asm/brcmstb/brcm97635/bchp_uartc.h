/***************************************************************************
 *     Copyright (c) 1999-2009, Broadcom Corporation
 *     All Rights Reserved
 *
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
 * $brcm_Workfile: bchp_uartc.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 12/2/08 8:02p $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Tue Dec  2 13:55:22 2008
 *                 MD5 Checksum         90975bbcdf76045bb6bd092e6bdf481b
 *
 * Compiled with:  RDB Utility          combo_header.pl
 *                 RDB Parser           3.0
 *                 unknown              unknown
 *                 Perl Interpreter     5.008008
 *                 Operating System     linux
 *
 * Revision History:
 *
 * $brcm_Log: /magnum/basemodules/chp/7635/rdb/a0/bchp_uartc.h $
 * 
 * Hydra_Software_Devel/1   12/2/08 8:02p albertl
 * PR48688: Initial revision.
 *
 ***************************************************************************/

#ifndef BCHP_UARTC_H__
#define BCHP_UARTC_H__

/***************************************************************************
 *UARTC - UART C
 ***************************************************************************/
#define BCHP_UARTC_RBR                           0x00400b80 /* Receive Buffer Register */
#define BCHP_UARTC_THR                           0x00400b80 /* Transmit Holding Register */
#define BCHP_UARTC_DLH                           0x00400b84 /* Divisor Latch High */
#define BCHP_UARTC_DLL                           0x00400b80 /* Divisor Latch Low */
#define BCHP_UARTC_IER                           0x00400b84 /* Interrupt Enable Register */
#define BCHP_UARTC_IIR                           0x00400b88 /* Interrupt Identity Register */
#define BCHP_UARTC_FCR                           0x00400b88 /* FIFO Control Register */
#define BCHP_UARTC_LCR                           0x00400b8c /* Line Control Register */
#define BCHP_UARTC_MCR                           0x00400b90 /* Modem Control Register */
#define BCHP_UARTC_LSR                           0x00400b94 /* Line Status Register */
#define BCHP_UARTC_MSR                           0x00400b98 /* Modem Status Register */
#define BCHP_UARTC_SCR                           0x00400b9c /* Scratchpad Register */

#endif /* #ifndef BCHP_UARTC_H__ */

/* End of File */
