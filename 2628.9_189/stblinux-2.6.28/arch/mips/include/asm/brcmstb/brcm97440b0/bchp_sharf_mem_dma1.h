/***************************************************************************
 *     Copyright (c) 1999-2007, Broadcom Corporation
 *     All Rights Reserved
 *     Confidential Property of Broadcom Corporation
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
 * $brcm_Workfile: bchp_sharf_mem_dma1.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 9/10/07 2:23p $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Mon Sep 10 11:36:30 2007
 *                 MD5 Checksum         fb285bf6233707394ba42557465343cf
 *
 * Compiled with:  RDB Utility          combo_header.pl
 *                 RDB Parser           3.0
 *                 unknown              unknown
 *                 Perl Interpreter     5.008004
 *                 Operating System     linux
 *
 * Revision History:
 *
 * $brcm_Log: /magnum/basemodules/chp/7440/rdb/c0/bchp_sharf_mem_dma1.h $
 * 
 * Hydra_Software_Devel/1   9/10/07 2:23p tdo
 * PR34699: 7440C0: RDB to be checked into magnum
 *
 ***************************************************************************/

#ifndef BCHP_SHARF_MEM_DMA1_H__
#define BCHP_SHARF_MEM_DMA1_H__

/***************************************************************************
 *SHARF_MEM_DMA1 - SHARF_MEM_DMA Channel 1 Registers
 ***************************************************************************/
#define BCHP_SHARF_MEM_DMA1_FIRST_DESC           0x01b10200 /* SHARF_MEM_DMA First Descriptor Address Register */
#define BCHP_SHARF_MEM_DMA1_CTRL                 0x01b10204 /* SHARF_MEM_DMA Control Register */
#define BCHP_SHARF_MEM_DMA1_WAKE_CTRL            0x01b10208 /* SHARF_MEM_DMA Wake Control Register */
#define BCHP_SHARF_MEM_DMA1_STATUS               0x01b10210 /* SHARF_MEM_DMA Status Register */
#define BCHP_SHARF_MEM_DMA1_CUR_DESC             0x01b10214 /* SHARF_MEM_DMA  Current Descriptor Address Register */
#define BCHP_SHARF_MEM_DMA1_CUR_BYTE             0x01b10218 /* SHARF_MEM_DMA  Current Byte Count Register */
#define BCHP_SHARF_MEM_DMA1_SCRATCH              0x01b1021c /* SHARF_MEM_DMA Scratch Register */

#endif /* #ifndef BCHP_SHARF_MEM_DMA1_H__ */

/* End of File */
