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
 * $brcm_Workfile: bchp_ddr23_phy_byte_lane_3.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 5/14/09 12:57a $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Wed May 13 17:29:16 2009
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
 * $brcm_Log: /magnum/basemodules/chp/7630/rdb/b0/bchp_ddr23_phy_byte_lane_3.h $
 * 
 * Hydra_Software_Devel/1   5/14/09 12:57a tdo
 * PR55087: Initial check in
 *
 ***************************************************************************/

#ifndef BCHP_DDR23_PHY_BYTE_LANE_3_H__
#define BCHP_DDR23_PHY_BYTE_LANE_3_H__

/***************************************************************************
 *DDR23_PHY_BYTE_LANE_3 - DDR23 DDR23 byte lane #3 control registers
 ***************************************************************************/
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION      0x01805100 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE 0x01805104 /* Byte lane VDL calibration control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS    0x01805108 /* Byte lane VDL calibration status register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0 0x01805110 /* Read DQSP VDL static override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1 0x01805114 /* Read DQSN VDL static override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2 0x01805118 /* Read Enable VDL static override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3 0x0180511c /* Write data and mask VDL static override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4 0x01805120 /* Read DQSP VDL dynamic override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5 0x01805124 /* Read DQSN VDL dynamic override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6 0x01805128 /* Read Enable VDL dynamic override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7 0x0180512c /* Write data and mask VDL dynamic override control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL  0x01805130 /* Byte Lane read channel control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_STATUS 0x01805134 /* Read fifo status register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_CLEAR 0x01805138 /* Read fifo status clear register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL 0x0180513c /* Idle mode SSTL pad control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL 0x01805140 /* SSTL pad drive characteristics control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_PAD_DISABLE 0x01805144 /* Clock pad disable register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_WR_PREAMBLE_MODE 0x01805148 /* Write cycle preamble control register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL 0x0180514c /* Clock Regulator control register */

/***************************************************************************
 *REVISION - Byte lane revision register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: REVISION :: reserved0 [31:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION_reserved0_MASK         0xffff0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION_reserved0_SHIFT        16

/* DDR23_PHY_BYTE_LANE_3 :: REVISION :: MAJOR [15:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION_MAJOR_MASK             0x0000ff00
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION_MAJOR_SHIFT            8

/* DDR23_PHY_BYTE_LANE_3 :: REVISION :: MINOR [07:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION_MINOR_MASK             0x000000ff
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION_MINOR_SHIFT            0

/***************************************************************************
 *VDL_CALIBRATE - Byte lane VDL calibration control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_CALIBRATE :: reserved0 [31:05] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_reserved0_MASK    0xffffffe0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_reserved0_SHIFT   5

/* DDR23_PHY_BYTE_LANE_3 :: VDL_CALIBRATE :: calib_clocks [04:04] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_clocks_MASK 0x00000010
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_clocks_SHIFT 4

/* DDR23_PHY_BYTE_LANE_3 :: VDL_CALIBRATE :: calib_test [03:03] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_test_MASK   0x00000008
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_test_SHIFT  3

/* DDR23_PHY_BYTE_LANE_3 :: VDL_CALIBRATE :: calib_always [02:02] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_always_MASK 0x00000004
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_always_SHIFT 2

/* DDR23_PHY_BYTE_LANE_3 :: VDL_CALIBRATE :: calib_once [01:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_once_MASK   0x00000002
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_once_SHIFT  1

/* DDR23_PHY_BYTE_LANE_3 :: VDL_CALIBRATE :: calib_fast [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_fast_MASK   0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_CALIBRATE_calib_fast_SHIFT  0

/***************************************************************************
 *VDL_STATUS - Byte lane VDL calibration status register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_STATUS :: reserved0 [31:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_reserved0_MASK       0xffffc000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_reserved0_SHIFT      14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_STATUS :: calib_total [13:04] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_calib_total_MASK     0x00003ff0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_calib_total_SHIFT    4

/* DDR23_PHY_BYTE_LANE_3 :: VDL_STATUS :: reserved1 [03:02] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_reserved1_MASK       0x0000000c
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_reserved1_SHIFT      2

/* DDR23_PHY_BYTE_LANE_3 :: VDL_STATUS :: calib_lock [01:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_calib_lock_MASK      0x00000002
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_calib_lock_SHIFT     1

/* DDR23_PHY_BYTE_LANE_3 :: VDL_STATUS :: calib_idle [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_calib_idle_MASK      0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_STATUS_calib_idle_SHIFT     0

/***************************************************************************
 *VDL_OVERRIDE_0 - Read DQSP VDL static override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_0 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_0_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_1 - Read DQSN VDL static override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_1 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_1_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_2 - Read Enable VDL static override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_2 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_2_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_3 - Write data and mask VDL static override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_3 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_3_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_4 - Read DQSP VDL dynamic override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_4 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_4_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_5 - Read DQSN VDL dynamic override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_5 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_5_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_6 - Read Enable VDL dynamic override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_6 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_6_ovr_step_SHIFT   0

/***************************************************************************
 *VDL_OVERRIDE_7 - Write data and mask VDL dynamic override control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: reserved0 [31:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved0_MASK   0xfffe0000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved0_SHIFT  17

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: ovr_en [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_en_MASK      0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_en_SHIFT     16

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: reserved1 [15:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved1_MASK   0x0000c000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved1_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: ovr_fine_fall [13:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_fine_fall_MASK 0x00003000
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_fine_fall_SHIFT 12

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: reserved2 [11:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved2_MASK   0x00000c00
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved2_SHIFT  10

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: ovr_fine_rise [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_fine_rise_MASK 0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_fine_rise_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: reserved3 [07:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved3_MASK   0x000000c0
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_reserved3_SHIFT  6

/* DDR23_PHY_BYTE_LANE_3 :: VDL_OVERRIDE_7 :: ovr_step [05:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_step_MASK    0x0000003f
#define BCHP_DDR23_PHY_BYTE_LANE_3_VDL_OVERRIDE_7_ovr_step_SHIFT   0

/***************************************************************************
 *READ_CONTROL - Byte Lane read channel control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: reserved0 [31:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_reserved0_MASK     0xfffffc00
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_reserved0_SHIFT    10

/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: rd_data_dly [09:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_rd_data_dly_MASK   0x00000300
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_rd_data_dly_SHIFT  8

/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: reserved1 [07:04] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_reserved1_MASK     0x000000f0
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_reserved1_SHIFT    4

/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: dq_odt_enable [03:03] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_dq_odt_enable_MASK 0x00000008
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_dq_odt_enable_SHIFT 3

/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: dq_odt_adj [02:02] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_dq_odt_adj_MASK    0x00000004
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_dq_odt_adj_SHIFT   2

/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: rd_enb_odt_enable [01:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_rd_enb_odt_enable_MASK 0x00000002
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_rd_enb_odt_enable_SHIFT 1

/* DDR23_PHY_BYTE_LANE_3 :: READ_CONTROL :: rd_enb_odt_adj [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_rd_enb_odt_adj_MASK 0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_CONTROL_rd_enb_odt_adj_SHIFT 0

/***************************************************************************
 *READ_FIFO_STATUS - Read fifo status register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: READ_FIFO_STATUS :: reserved0 [31:04] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_STATUS_reserved0_MASK 0xfffffff0
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_STATUS_reserved0_SHIFT 4

/* DDR23_PHY_BYTE_LANE_3 :: READ_FIFO_STATUS :: status [03:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_STATUS_status_MASK    0x0000000f
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_STATUS_status_SHIFT   0

/***************************************************************************
 *READ_FIFO_CLEAR - Read fifo status clear register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: READ_FIFO_CLEAR :: reserved0 [31:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_CLEAR_reserved0_MASK  0xfffffffe
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_CLEAR_reserved0_SHIFT 1

/* DDR23_PHY_BYTE_LANE_3 :: READ_FIFO_CLEAR :: clear [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_CLEAR_clear_MASK      0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_READ_FIFO_CLEAR_clear_SHIFT     0

/***************************************************************************
 *IDLE_PAD_CONTROL - Idle mode SSTL pad control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: idle [31:31] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_idle_MASK      0x80000000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_idle_SHIFT     31

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: reserved0 [30:20] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_reserved0_MASK 0x7ff00000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_reserved0_SHIFT 20

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dm_rxenb [19:19] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_rxenb_MASK  0x00080000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_rxenb_SHIFT 19

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dm_iddq [18:18] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_iddq_MASK   0x00040000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_iddq_SHIFT  18

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dm_reb [17:17] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_reb_MASK    0x00020000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_reb_SHIFT   17

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dm_oeb [16:16] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_oeb_MASK    0x00010000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_oeb_SHIFT   16

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dq_rxenb [15:15] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_rxenb_MASK  0x00008000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_rxenb_SHIFT 15

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dq_iddq [14:14] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_iddq_MASK   0x00004000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_iddq_SHIFT  14

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dq_reb [13:13] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_reb_MASK    0x00002000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_reb_SHIFT   13

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dq_oeb [12:12] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_oeb_MASK    0x00001000
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_oeb_SHIFT   12

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: read_enb_rxenb [11:11] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_rxenb_MASK 0x00000800
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_rxenb_SHIFT 11

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: read_enb_iddq [10:10] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_iddq_MASK 0x00000400
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_iddq_SHIFT 10

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: read_enb_reb [09:09] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_reb_MASK 0x00000200
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_reb_SHIFT 9

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: read_enb_oeb [08:08] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_oeb_MASK 0x00000100
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_oeb_SHIFT 8

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dqs_rxenb [07:07] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_rxenb_MASK 0x00000080
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_rxenb_SHIFT 7

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dqs_iddq [06:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_iddq_MASK  0x00000040
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_iddq_SHIFT 6

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dqs_reb [05:05] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_reb_MASK   0x00000020
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_reb_SHIFT  5

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: dqs_oeb [04:04] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_oeb_MASK   0x00000010
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_oeb_SHIFT  4

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: clk_rxenb [03:03] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_rxenb_MASK 0x00000008
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_rxenb_SHIFT 3

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: clk_iddq [02:02] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_iddq_MASK  0x00000004
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_iddq_SHIFT 2

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: clk_reb [01:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_reb_MASK   0x00000002
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_reb_SHIFT  1

/* DDR23_PHY_BYTE_LANE_3 :: IDLE_PAD_CONTROL :: clk_oeb [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_oeb_MASK   0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_oeb_SHIFT  0

/***************************************************************************
 *DRIVE_PAD_CTL - SSTL pad drive characteristics control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: reserved0 [31:06] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_reserved0_MASK    0xffffffc0
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_reserved0_SHIFT   6

/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: rt60b_ddr_read_enb [05:05] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_rt60b_ddr_read_enb_MASK 0x00000020
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_rt60b_ddr_read_enb_SHIFT 5

/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: rt60b [04:04] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_rt60b_MASK        0x00000010
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_rt60b_SHIFT       4

/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: sel_sstl18 [03:03] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_sel_sstl18_MASK   0x00000008
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_sel_sstl18_SHIFT  3

/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: seltxdrv_ci [02:02] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_seltxdrv_ci_MASK  0x00000004
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_seltxdrv_ci_SHIFT 2

/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: selrxdrv [01:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_selrxdrv_MASK     0x00000002
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_selrxdrv_SHIFT    1

/* DDR23_PHY_BYTE_LANE_3 :: DRIVE_PAD_CTL :: slew [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_slew_MASK         0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_DRIVE_PAD_CTL_slew_SHIFT        0

/***************************************************************************
 *CLOCK_PAD_DISABLE - Clock pad disable register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: CLOCK_PAD_DISABLE :: reserved0 [31:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_PAD_DISABLE_reserved0_MASK 0xfffffffe
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_PAD_DISABLE_reserved0_SHIFT 1

/* DDR23_PHY_BYTE_LANE_3 :: CLOCK_PAD_DISABLE :: clk_pad_dis [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_PAD_DISABLE_clk_pad_dis_MASK 0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_PAD_DISABLE_clk_pad_dis_SHIFT 0

/***************************************************************************
 *WR_PREAMBLE_MODE - Write cycle preamble control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: WR_PREAMBLE_MODE :: reserved0 [31:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_WR_PREAMBLE_MODE_reserved0_MASK 0xfffffffe
#define BCHP_DDR23_PHY_BYTE_LANE_3_WR_PREAMBLE_MODE_reserved0_SHIFT 1

/* DDR23_PHY_BYTE_LANE_3 :: WR_PREAMBLE_MODE :: mode [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_WR_PREAMBLE_MODE_mode_MASK      0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_WR_PREAMBLE_MODE_mode_SHIFT     0

/***************************************************************************
 *CLOCK_REG_CONTROL - Clock Regulator control register
 ***************************************************************************/
/* DDR23_PHY_BYTE_LANE_3 :: CLOCK_REG_CONTROL :: reserved0 [31:02] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL_reserved0_MASK 0xfffffffc
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL_reserved0_SHIFT 2

/* DDR23_PHY_BYTE_LANE_3 :: CLOCK_REG_CONTROL :: half_power [01:01] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL_half_power_MASK 0x00000002
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL_half_power_SHIFT 1

/* DDR23_PHY_BYTE_LANE_3 :: CLOCK_REG_CONTROL :: pwrdn [00:00] */
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL_pwrdn_MASK    0x00000001
#define BCHP_DDR23_PHY_BYTE_LANE_3_CLOCK_REG_CONTROL_pwrdn_SHIFT   0

#endif /* #ifndef BCHP_DDR23_PHY_BYTE_LANE_3_H__ */

/* End of File */
