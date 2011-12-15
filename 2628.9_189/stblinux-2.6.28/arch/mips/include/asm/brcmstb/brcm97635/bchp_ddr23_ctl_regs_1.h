/***************************************************************************
 *     Copyright (c) 1999-2008, Broadcom Corporation
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
 * $brcm_Workfile: bchp_ddr23_ctl_regs_1.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 12/2/08 5:49p $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Tue Dec  2 13:56:34 2008
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
 * $brcm_Log: /magnum/basemodules/chp/7635/rdb/a0/bchp_ddr23_ctl_regs_1.h $
 * 
 * Hydra_Software_Devel/1   12/2/08 5:49p albertl
 * PR48688: Initial revision.
 *
 ***************************************************************************/

#ifndef BCHP_DDR23_CTL_REGS_1_H__
#define BCHP_DDR23_CTL_REGS_1_H__

/***************************************************************************
 *DDR23_CTL_REGS_1 - DDR23 controller registers 1
 ***************************************************************************/
#define BCHP_DDR23_CTL_REGS_1_REVISION           0x01804000 /* DDR23 Controller revision register */
#define BCHP_DDR23_CTL_REGS_1_CTL_STATUS         0x01804004 /* DDR23 Controller status register */
#define BCHP_DDR23_CTL_REGS_1_PARAMS1            0x01804010 /* DDR23 Controller Configuration Set #1 */
#define BCHP_DDR23_CTL_REGS_1_PARAMS2            0x01804014 /* DDR23 Controller Configuration Set #2 */
#define BCHP_DDR23_CTL_REGS_1_PARAMS3            0x01804018 /* DDR23 Controller Configuration Set #3 */
#define BCHP_DDR23_CTL_REGS_1_REFRESH            0x0180401c /* DDR23 Controller Automated Refresh Configuration */
#define BCHP_DDR23_CTL_REGS_1_REFRESH_CMD        0x01804020 /* Host Initiated Refresh Control */
#define BCHP_DDR23_CTL_REGS_1_PRECHARGE_CMD      0x01804024 /* Host Initiated Precharge Control */
#define BCHP_DDR23_CTL_REGS_1_LOAD_MODE_CMD      0x01804028 /* Host Initiated Load Mode Control */
#define BCHP_DDR23_CTL_REGS_1_LOAD_EMODE_CMD     0x0180402c /* Host Initiated Load Extended Mode Control */
#define BCHP_DDR23_CTL_REGS_1_LOAD_EMODE2_CMD    0x01804030 /* Host Initiated Load Extended Mode #2 Control */
#define BCHP_DDR23_CTL_REGS_1_LOAD_EMODE3_CMD    0x01804034 /* Host Initiated Load Extended Mode #3 Control */
#define BCHP_DDR23_CTL_REGS_1_ZQ_CALIBRATE       0x01804038 /* Host Initiated ZQ Calibration Cycle */
#define BCHP_DDR23_CTL_REGS_1_CMD_STATUS         0x0180403c /* Host Command Interface Status */
#define BCHP_DDR23_CTL_REGS_1_LATENCY            0x01804040 /* DDR2 Controller Access Latency Control */
#define BCHP_DDR23_CTL_REGS_1_SEMAPHORE0         0x01804044 /* Semaphore Register #0 */
#define BCHP_DDR23_CTL_REGS_1_SEMAPHORE1         0x01804048 /* Semaphore Register #1 */
#define BCHP_DDR23_CTL_REGS_1_SEMAPHORE2         0x0180404c /* Semaphore Register #2 */
#define BCHP_DDR23_CTL_REGS_1_SEMAPHORE3         0x01804050 /* Semaphore Register #3 */
#define BCHP_DDR23_CTL_REGS_1_SCRATCH            0x01804058 /* Scratch Register */
#define BCHP_DDR23_CTL_REGS_1_STRIPE_WIDTH       0x01804060 /* Stripe Width */
#define BCHP_DDR23_CTL_REGS_1_STRIPE_HEIGHT_0    0x01804070 /* Stripe Height for picture buffers 0 through 23 */
#define BCHP_DDR23_CTL_REGS_1_STRIPE_HEIGHT_1    0x01804074 /* Stripe Height for picture buffers 24 through 27 */
#define BCHP_DDR23_CTL_REGS_1_STRIPE_HEIGHT_2    0x01804078 /* Stripe Height for picture buffers 28 through 31 */
#define BCHP_DDR23_CTL_REGS_1_LBIST_CNTRL        0x018040a0 /* Logic-Bist Control and Status */
#define BCHP_DDR23_CTL_REGS_1_LBIST_SEED         0x018040a4 /* Logic-Bist PRPG Seed Value */
#define BCHP_DDR23_CTL_REGS_1_LBIST_STATUS       0x018040a8 /* Logic-Bist Control and Status */
#define BCHP_DDR23_CTL_REGS_1_VDL_CTL            0x018040b0 /* Dynamic VDL Changes Control */
#define BCHP_DDR23_CTL_REGS_1_VDL_ADR_BASE       0x018040b4 /* Dynamic VDL Changes Base Address */
#define BCHP_DDR23_CTL_REGS_1_VDL_ADR_END        0x018040b8 /* Dynamic VDL Changes End Address */
#define BCHP_DDR23_CTL_REGS_1_PMON_CTL           0x018040c0 /* Performance Monitoring Control */
#define BCHP_DDR23_CTL_REGS_1_PMON_PERIOD        0x018040c4 /* Performance Monitoring Period Control */
#define BCHP_DDR23_CTL_REGS_1_PMON_CYCLE_CNT     0x018040c8 /* Performance Monitoring Active Cycles Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_IDLE_CNT      0x018040cc /* Performance Monitoring Idle Cycles Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_RD_CNT1       0x018040d0 /* Performance Monitoring Data Channel #1 Read Accesses Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_RD_CNT2       0x018040d4 /* Performance Monitoring Data Channel #2 Read Accesses Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_RD_CNT3       0x018040d8 /* Performance Monitoring Data Channel #3 Read Accesses Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_WR_CNT1       0x018040dc /* Performance Monitoring Data Channel #1 Write Accesses Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_WR_CNT2       0x018040e0 /* Performance Monitoring Data Channel #2 Write Accesses Count */
#define BCHP_DDR23_CTL_REGS_1_PMON_WR_CNT3       0x018040e4 /* Performance Monitoring Data Channel #3 Write Accesses Count */
#define BCHP_DDR23_CTL_REGS_1_DDR_TM             0x018040e8 /* RAM Macro TM Control */

/***************************************************************************
 *PICT_BASE%i - Picture Buffer Base Address Ram
 ***************************************************************************/
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_ARRAY_BASE                0x01804100
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_ARRAY_START               0
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_ARRAY_END                 63
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_ARRAY_ELEMENT_SIZE        32

/***************************************************************************
 *PICT_BASE%i - Picture Buffer Base Address Ram
 ***************************************************************************/
/* DDR23_CTL_REGS_1 :: PICT_BASEi :: address [31:12] */
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_address_MASK              0xfffff000
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_address_SHIFT             12

/* DDR23_CTL_REGS_1 :: PICT_BASEi :: reserved0 [11:00] */
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_reserved0_MASK            0x00000fff
#define BCHP_DDR23_CTL_REGS_1_PICT_BASEi_reserved0_SHIFT           0


#endif /* #ifndef BCHP_DDR23_CTL_REGS_1_H__ */

/* End of File */
