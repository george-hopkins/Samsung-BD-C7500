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
 * $brcm_Workfile: bchp_sharf_mem_dma0.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 5/14/09 1:50a $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Wed May 13 17:30:26 2009
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
 * $brcm_Log: /magnum/basemodules/chp/7630/rdb/b0/bchp_sharf_mem_dma0.h $
 * 
 * Hydra_Software_Devel/1   5/14/09 1:50a tdo
 * PR55087: Initial check in
 *
 ***************************************************************************/

#ifndef BCHP_SHARF_MEM_DMA0_H__
#define BCHP_SHARF_MEM_DMA0_H__

/***************************************************************************
 *SHARF_MEM_DMA0 - SHARF_MEM_DMA Channel 0 Registers
 ***************************************************************************/
#define BCHP_SHARF_MEM_DMA0_FIRST_DESC           0x01404100 /* SHARF_MEM_DMA First Descriptor Address Register */
#define BCHP_SHARF_MEM_DMA0_CTRL                 0x01404104 /* SHARF_MEM_DMA Control Register */
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL            0x01404108 /* SHARF_MEM_DMA Wake Control Register */
#define BCHP_SHARF_MEM_DMA0_STATUS               0x01404110 /* SHARF_MEM_DMA Status Register */
#define BCHP_SHARF_MEM_DMA0_CUR_DESC             0x01404114 /* SHARF_MEM_DMA  Current Descriptor Address Register */
#define BCHP_SHARF_MEM_DMA0_CUR_BYTE             0x01404118 /* SHARF_MEM_DMA  Current Byte Count Register */
#define BCHP_SHARF_MEM_DMA0_SCRATCH              0x0140411c /* SHARF_MEM_DMA Scratch Register */

/***************************************************************************
 *FIRST_DESC - SHARF_MEM_DMA First Descriptor Address Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: FIRST_DESC :: ADDR [31:00] */
#define BCHP_SHARF_MEM_DMA0_FIRST_DESC_ADDR_MASK                   0xffffffff
#define BCHP_SHARF_MEM_DMA0_FIRST_DESC_ADDR_SHIFT                  0

/***************************************************************************
 *CTRL - SHARF_MEM_DMA Control Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: CTRL :: reserved0 [31:01] */
#define BCHP_SHARF_MEM_DMA0_CTRL_reserved0_MASK                    0xfffffffe
#define BCHP_SHARF_MEM_DMA0_CTRL_reserved0_SHIFT                   1

/* SHARF_MEM_DMA0 :: CTRL :: RUN [00:00] */
#define BCHP_SHARF_MEM_DMA0_CTRL_RUN_MASK                          0x00000001
#define BCHP_SHARF_MEM_DMA0_CTRL_RUN_SHIFT                         0

/***************************************************************************
 *WAKE_CTRL - SHARF_MEM_DMA Wake Control Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: WAKE_CTRL :: reserved0 [31:02] */
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL_reserved0_MASK               0xfffffffc
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL_reserved0_SHIFT              2

/* SHARF_MEM_DMA0 :: WAKE_CTRL :: WAKE_MODE [01:01] */
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL_WAKE_MODE_MASK               0x00000002
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL_WAKE_MODE_SHIFT              1

/* SHARF_MEM_DMA0 :: WAKE_CTRL :: WAKE [00:00] */
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL_WAKE_MASK                    0x00000001
#define BCHP_SHARF_MEM_DMA0_WAKE_CTRL_WAKE_SHIFT                   0

/***************************************************************************
 *STATUS - SHARF_MEM_DMA Status Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: STATUS :: reserved0 [31:02] */
#define BCHP_SHARF_MEM_DMA0_STATUS_reserved0_MASK                  0xfffffffc
#define BCHP_SHARF_MEM_DMA0_STATUS_reserved0_SHIFT                 2

/* SHARF_MEM_DMA0 :: STATUS :: DMA_STATUS [01:00] */
#define BCHP_SHARF_MEM_DMA0_STATUS_DMA_STATUS_MASK                 0x00000003
#define BCHP_SHARF_MEM_DMA0_STATUS_DMA_STATUS_SHIFT                0
#define BCHP_SHARF_MEM_DMA0_STATUS_DMA_STATUS_Idle                 0
#define BCHP_SHARF_MEM_DMA0_STATUS_DMA_STATUS_Busy                 1
#define BCHP_SHARF_MEM_DMA0_STATUS_DMA_STATUS_Sleep                2
#define BCHP_SHARF_MEM_DMA0_STATUS_DMA_STATUS_Reserved             3

/***************************************************************************
 *CUR_DESC - SHARF_MEM_DMA  Current Descriptor Address Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: CUR_DESC :: ADDR [31:00] */
#define BCHP_SHARF_MEM_DMA0_CUR_DESC_ADDR_MASK                     0xffffffff
#define BCHP_SHARF_MEM_DMA0_CUR_DESC_ADDR_SHIFT                    0

/***************************************************************************
 *CUR_BYTE - SHARF_MEM_DMA  Current Byte Count Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: CUR_BYTE :: reserved0 [31:25] */
#define BCHP_SHARF_MEM_DMA0_CUR_BYTE_reserved0_MASK                0xfe000000
#define BCHP_SHARF_MEM_DMA0_CUR_BYTE_reserved0_SHIFT               25

/* SHARF_MEM_DMA0 :: CUR_BYTE :: COUNT [24:00] */
#define BCHP_SHARF_MEM_DMA0_CUR_BYTE_COUNT_MASK                    0x01ffffff
#define BCHP_SHARF_MEM_DMA0_CUR_BYTE_COUNT_SHIFT                   0

/***************************************************************************
 *SCRATCH - SHARF_MEM_DMA Scratch Register
 ***************************************************************************/
/* SHARF_MEM_DMA0 :: SCRATCH :: SCRATCH_BIT [31:00] */
#define BCHP_SHARF_MEM_DMA0_SCRATCH_SCRATCH_BIT_MASK               0xffffffff
#define BCHP_SHARF_MEM_DMA0_SCRATCH_SCRATCH_BIT_SHIFT              0

#endif /* #ifndef BCHP_SHARF_MEM_DMA0_H__ */

/* End of File */
