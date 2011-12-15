/***************************************************************************
 *     Copyright (c) 1999-2009, Broadcom Corporation
 *     All Rights Reserved
 *     Confidential Property of Broadcom Corporation
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
 * $brcm_Workfile: bchp_vide_bus_master.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 5/14/09 2:06a $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Wed May 13 17:31:27 2009
 *                 MD5 Checksum         397c7e3e68fca5836958d6d3dcfe31f9
 *
 * Compiled with:  RDB Utility          combo_header.pl
 *                 RDB Parser           3.0
 *                 unknown              unknown
 *                 Perl Interpreter     5.008008
 *                 Operating System     linux
 *
 * Revision History:
 *
 * $brcm_Log: /magnum/basemodules/chp/7630/rdb/b0/bchp_vide_bus_master.h $
 * 
 * Hydra_Software_Devel/1   5/14/09 2:06a tdo
 * PR55087: Initial check in
 *
 ***************************************************************************/

#ifndef BCHP_VIDE_BUS_MASTER_H__
#define BCHP_VIDE_BUS_MASTER_H__

/***************************************************************************
 *VIDE_BUS_MASTER
 ***************************************************************************/
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS          0x00290200 /* Command/Status */
#define BCHP_VIDE_BUS_MASTER_PRD_PTR             0x00290204 /* PRD Pointer */
#define BCHP_VIDE_BUS_MASTER_DBG_PRES_ADDR       0x00290210 /* Debug: Present Addr */
#define BCHP_VIDE_BUS_MASTER_DBG_XFER_SIZE       0x00290214 /* Debug: Xfer Size */
#define BCHP_VIDE_BUS_MASTER_DBG_PRD_PTR         0x00290218 /* Debug: PRD Ptr */
#define BCHP_VIDE_BUS_MASTER_DBG_STATE           0x0029021c /* Debug: State */

/***************************************************************************
 *CMD_STATUS - Command/Status
 ***************************************************************************/
/* VIDE_BUS_MASTER :: CMD_STATUS :: reserved0 [31:16] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_MASK             0xffff0000
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_SHIFT            16

/* VIDE_BUS_MASTER :: CMD_STATUS :: Simplex [15:15] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Simplex_MASK               0x00008000
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Simplex_SHIFT              15

/* VIDE_BUS_MASTER :: CMD_STATUS :: Dev1DMA [14:14] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dev1DMA_MASK               0x00004000
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dev1DMA_SHIFT              14

/* VIDE_BUS_MASTER :: CMD_STATUS :: Dev0DMA [13:13] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dev0DMA_MASK               0x00002000
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dev0DMA_SHIFT              13

/* VIDE_BUS_MASTER :: CMD_STATUS :: reserved1 [12:11] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_MASK             0x00001800
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_SHIFT            11

/* VIDE_BUS_MASTER :: CMD_STATUS :: Int [10:10] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Int_MASK                   0x00000400
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Int_SHIFT                  10

/* VIDE_BUS_MASTER :: CMD_STATUS :: Err [09:09] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Err_MASK                   0x00000200
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Err_SHIFT                  9

/* VIDE_BUS_MASTER :: CMD_STATUS :: Active [08:08] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Active_MASK                0x00000100
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Active_SHIFT               8

/* VIDE_BUS_MASTER :: CMD_STATUS :: reserved2 [07:04] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_MASK             0x000000f0
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_SHIFT            4

/* VIDE_BUS_MASTER :: CMD_STATUS :: Dir [03:03] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dir_MASK                   0x00000008
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dir_SHIFT                  3

/* VIDE_BUS_MASTER :: CMD_STATUS :: reserved3 [02:01] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_MASK             0x00000006
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_SHIFT            1

/* VIDE_BUS_MASTER :: CMD_STATUS :: Start [00:00] */
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Start_MASK                 0x00000001
#define BCHP_VIDE_BUS_MASTER_CMD_STATUS_Start_SHIFT                0

/***************************************************************************
 *PRD_PTR - PRD Pointer
 ***************************************************************************/
/* VIDE_BUS_MASTER :: PRD_PTR :: PRD_Ptr [31:02] */
#define BCHP_VIDE_BUS_MASTER_PRD_PTR_PRD_Ptr_MASK                  0xfffffffc
#define BCHP_VIDE_BUS_MASTER_PRD_PTR_PRD_Ptr_SHIFT                 2

/* VIDE_BUS_MASTER :: PRD_PTR :: reserved0 [01:00] */
#define BCHP_VIDE_BUS_MASTER_PRD_PTR_reserved0_MASK                0x00000003
#define BCHP_VIDE_BUS_MASTER_PRD_PTR_reserved0_SHIFT               0

/***************************************************************************
 *DBG_PRES_ADDR - Debug: Present Addr
 ***************************************************************************/
/* VIDE_BUS_MASTER :: DBG_PRES_ADDR :: PresAddr [31:00] */
#define BCHP_VIDE_BUS_MASTER_DBG_PRES_ADDR_PresAddr_MASK           0xffffffff
#define BCHP_VIDE_BUS_MASTER_DBG_PRES_ADDR_PresAddr_SHIFT          0

/***************************************************************************
 *DBG_XFER_SIZE - Debug: Xfer Size
 ***************************************************************************/
/* VIDE_BUS_MASTER :: DBG_XFER_SIZE :: XferSize [31:00] */
#define BCHP_VIDE_BUS_MASTER_DBG_XFER_SIZE_XferSize_MASK           0xffffffff
#define BCHP_VIDE_BUS_MASTER_DBG_XFER_SIZE_XferSize_SHIFT          0

/***************************************************************************
 *DBG_PRD_PTR - Debug: PRD Ptr
 ***************************************************************************/
/* VIDE_BUS_MASTER :: DBG_PRD_PTR :: PrdPtr [31:00] */
#define BCHP_VIDE_BUS_MASTER_DBG_PRD_PTR_PrdPtr_MASK               0xffffffff
#define BCHP_VIDE_BUS_MASTER_DBG_PRD_PTR_PrdPtr_SHIFT              0

/***************************************************************************
 *DBG_STATE - Debug: State
 ***************************************************************************/
/* VIDE_BUS_MASTER :: DBG_STATE :: State [31:00] */
#define BCHP_VIDE_BUS_MASTER_DBG_STATE_State_MASK                  0xffffffff
#define BCHP_VIDE_BUS_MASTER_DBG_STATE_State_SHIFT                 0

#endif /* #ifndef BCHP_VIDE_BUS_MASTER_H__ */

/* End of File */
