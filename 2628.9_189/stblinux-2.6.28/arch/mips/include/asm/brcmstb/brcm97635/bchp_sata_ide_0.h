/***************************************************************************
 *
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
 * $brcm_Workfile: bchp_sata_ide_0.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 12/2/08 7:25p $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Tue Dec  2 13:54:58 2008
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
 * $brcm_Log: /magnum/basemodules/chp/7635/rdb/a0/bchp_sata_ide_0.h $
 * 
 * Hydra_Software_Devel/1   12/2/08 7:25p albertl
 * PR48688: Initial revision.
 *
 ***************************************************************************/

#ifndef BCHP_SATA_IDE_0_H__
#define BCHP_SATA_IDE_0_H__

/***************************************************************************
 *SATA_IDE_0 - Primary IDE Register Space 0
 ***************************************************************************/
#define BCHP_SATA_IDE_0_IDE_COMMAND              0x520200 /* Bus-master IDE Command Register */
#define BCHP_SATA_IDE_0_IDE_STATUS               0x520201 /* Bus-master IDE Status Register */
#define BCHP_SATA_IDE_0_DES_TABLE                0x00520204 /* Descriptor Table Pointer & Transfer Address Register */

/***************************************************************************
 *IDE_COMMAND - Bus-master IDE Command Register
 ***************************************************************************/
/* SATA_IDE_0 :: IDE_COMMAND :: reserved0 [07:04] */
#define BCHP_SATA_IDE_0_IDE_COMMAND_reserved0_MASK                 0xf0
#define BCHP_SATA_IDE_0_IDE_COMMAND_reserved0_SHIFT                4

/* SATA_IDE_0 :: IDE_COMMAND :: IDE_Control [03:03] */
#define BCHP_SATA_IDE_0_IDE_COMMAND_IDE_Control_MASK               0x08
#define BCHP_SATA_IDE_0_IDE_COMMAND_IDE_Control_SHIFT              3

/* SATA_IDE_0 :: IDE_COMMAND :: reserved1 [02:01] */
#define BCHP_SATA_IDE_0_IDE_COMMAND_reserved1_MASK                 0x06
#define BCHP_SATA_IDE_0_IDE_COMMAND_reserved1_SHIFT                1

/* SATA_IDE_0 :: IDE_COMMAND :: IDE_Start_Stop [00:00] */
#define BCHP_SATA_IDE_0_IDE_COMMAND_IDE_Start_Stop_MASK            0x01
#define BCHP_SATA_IDE_0_IDE_COMMAND_IDE_Start_Stop_SHIFT           0

/***************************************************************************
 *IDE_STATUS - Bus-master IDE Status Register
 ***************************************************************************/
/* SATA_IDE_0 :: IDE_STATUS :: Simplex_Only [07:07] */
#define BCHP_SATA_IDE_0_IDE_STATUS_Simplex_Only_MASK               0x80
#define BCHP_SATA_IDE_0_IDE_STATUS_Simplex_Only_SHIFT              7

/* SATA_IDE_0 :: IDE_STATUS :: Secondary_DMA [06:06] */
#define BCHP_SATA_IDE_0_IDE_STATUS_Secondary_DMA_MASK              0x40
#define BCHP_SATA_IDE_0_IDE_STATUS_Secondary_DMA_SHIFT             6

/* SATA_IDE_0 :: IDE_STATUS :: Primary_DMA [05:05] */
#define BCHP_SATA_IDE_0_IDE_STATUS_Primary_DMA_MASK                0x20
#define BCHP_SATA_IDE_0_IDE_STATUS_Primary_DMA_SHIFT               5

/* SATA_IDE_0 :: IDE_STATUS :: reserved0 [04:03] */
#define BCHP_SATA_IDE_0_IDE_STATUS_reserved0_MASK                  0x18
#define BCHP_SATA_IDE_0_IDE_STATUS_reserved0_SHIFT                 3

/* SATA_IDE_0 :: IDE_STATUS :: IDE_Interrupt [02:02] */
#define BCHP_SATA_IDE_0_IDE_STATUS_IDE_Interrupt_MASK              0x04
#define BCHP_SATA_IDE_0_IDE_STATUS_IDE_Interrupt_SHIFT             2

/* SATA_IDE_0 :: IDE_STATUS :: IDE_DMA_Error [01:01] */
#define BCHP_SATA_IDE_0_IDE_STATUS_IDE_DMA_Error_MASK              0x02
#define BCHP_SATA_IDE_0_IDE_STATUS_IDE_DMA_Error_SHIFT             1

/* SATA_IDE_0 :: IDE_STATUS :: Bus_Master_IDE_Active [00:00] */
#define BCHP_SATA_IDE_0_IDE_STATUS_Bus_Master_IDE_Active_MASK      0x01
#define BCHP_SATA_IDE_0_IDE_STATUS_Bus_Master_IDE_Active_SHIFT     0

/***************************************************************************
 *DES_TABLE - Descriptor Table Pointer & Transfer Address Register
 ***************************************************************************/
/* SATA_IDE_0 :: DES_TABLE :: Base_Address [31:02] */
#define BCHP_SATA_IDE_0_DES_TABLE_Base_Address_MASK                0xfffffffc
#define BCHP_SATA_IDE_0_DES_TABLE_Base_Address_SHIFT               2

/* SATA_IDE_0 :: DES_TABLE :: reserved0 [01:00] */
#define BCHP_SATA_IDE_0_DES_TABLE_reserved0_MASK                   0x00000003
#define BCHP_SATA_IDE_0_DES_TABLE_reserved0_SHIFT                  0

#endif /* #ifndef BCHP_SATA_IDE_0_H__ */

/* End of File */
