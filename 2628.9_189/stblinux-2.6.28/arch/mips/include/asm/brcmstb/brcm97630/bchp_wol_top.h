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
 * $brcm_Workfile: bchp_wol_top.h $
 * $brcm_Revision: Hydra_Software_Devel/1 $
 * $brcm_Date: 5/14/09 2:09a $
 *
 * Module Description:
 *                     DO NOT EDIT THIS FILE DIRECTLY
 *
 * This module was generated magically with RDB from a source description
 * file. You must edit the source file for changes to be made to this file.
 *
 *
 * Date:           Generated on         Wed May 13 17:30:32 2009
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
 * $brcm_Log: /magnum/basemodules/chp/7630/rdb/b0/bchp_wol_top.h $
 * 
 * Hydra_Software_Devel/1   5/14/09 2:09a tdo
 * PR55087: Initial check in
 *
 ***************************************************************************/

#ifndef BCHP_WOL_TOP_H__
#define BCHP_WOL_TOP_H__

/***************************************************************************
 *WOL_TOP
 ***************************************************************************/
#define BCHP_WOL_TOP_OVERALL_EN                  0x00090000 /* Control wake on LAN feature */
#define BCHP_WOL_TOP_PSSWD_LS                    0x00090004 /* Least significant four bytes of Password */
#define BCHP_WOL_TOP_PSSWD_MS                    0x00090008 /* Most significant two bytes of Password */
#define BCHP_WOL_TOP_PSSWD_EN                    0x00090010 /* Enable bits for password bytes */

/***************************************************************************
 *OVERALL_EN - Control wake on LAN feature
 ***************************************************************************/
/* WOL_TOP :: OVERALL_EN :: reserved0 [31:17] */
#define BCHP_WOL_TOP_OVERALL_EN_reserved0_MASK                     0xfffe0000
#define BCHP_WOL_TOP_OVERALL_EN_reserved0_SHIFT                    17

/* WOL_TOP :: OVERALL_EN :: Interrupt_Status [16:16] */
#define BCHP_WOL_TOP_OVERALL_EN_Interrupt_Status_MASK              0x00010000
#define BCHP_WOL_TOP_OVERALL_EN_Interrupt_Status_SHIFT             16

/* WOL_TOP :: OVERALL_EN :: reserved1 [15:02] */
#define BCHP_WOL_TOP_OVERALL_EN_reserved1_MASK                     0x0000fffc
#define BCHP_WOL_TOP_OVERALL_EN_reserved1_SHIFT                    2

/* WOL_TOP :: OVERALL_EN :: Interrupt_Enable [01:01] */
#define BCHP_WOL_TOP_OVERALL_EN_Interrupt_Enable_MASK              0x00000002
#define BCHP_WOL_TOP_OVERALL_EN_Interrupt_Enable_SHIFT             1

/* WOL_TOP :: OVERALL_EN :: Overall_Enable [00:00] */
#define BCHP_WOL_TOP_OVERALL_EN_Overall_Enable_MASK                0x00000001
#define BCHP_WOL_TOP_OVERALL_EN_Overall_Enable_SHIFT               0

/***************************************************************************
 *PSSWD_LS - Least significant four bytes of Password
 ***************************************************************************/
/* WOL_TOP :: PSSWD_LS :: password_2 [31:24] */
#define BCHP_WOL_TOP_PSSWD_LS_password_2_MASK                      0xff000000
#define BCHP_WOL_TOP_PSSWD_LS_password_2_SHIFT                     24

/* WOL_TOP :: PSSWD_LS :: password_3 [23:16] */
#define BCHP_WOL_TOP_PSSWD_LS_password_3_MASK                      0x00ff0000
#define BCHP_WOL_TOP_PSSWD_LS_password_3_SHIFT                     16

/* WOL_TOP :: PSSWD_LS :: password_4 [15:08] */
#define BCHP_WOL_TOP_PSSWD_LS_password_4_MASK                      0x0000ff00
#define BCHP_WOL_TOP_PSSWD_LS_password_4_SHIFT                     8

/* WOL_TOP :: PSSWD_LS :: password_5 [07:00] */
#define BCHP_WOL_TOP_PSSWD_LS_password_5_MASK                      0x000000ff
#define BCHP_WOL_TOP_PSSWD_LS_password_5_SHIFT                     0

/***************************************************************************
 *PSSWD_MS - Most significant two bytes of Password
 ***************************************************************************/
/* WOL_TOP :: PSSWD_MS :: reserved0 [31:16] */
#define BCHP_WOL_TOP_PSSWD_MS_reserved0_MASK                       0xffff0000
#define BCHP_WOL_TOP_PSSWD_MS_reserved0_SHIFT                      16

/* WOL_TOP :: PSSWD_MS :: password_0 [15:08] */
#define BCHP_WOL_TOP_PSSWD_MS_password_0_MASK                      0x0000ff00
#define BCHP_WOL_TOP_PSSWD_MS_password_0_SHIFT                     8

/* WOL_TOP :: PSSWD_MS :: password_1 [07:00] */
#define BCHP_WOL_TOP_PSSWD_MS_password_1_MASK                      0x000000ff
#define BCHP_WOL_TOP_PSSWD_MS_password_1_SHIFT                     0

/***************************************************************************
 *PSSWD_EN - Enable bits for password bytes
 ***************************************************************************/
/* WOL_TOP :: PSSWD_EN :: reserved0 [31:06] */
#define BCHP_WOL_TOP_PSSWD_EN_reserved0_MASK                       0xffffffc0
#define BCHP_WOL_TOP_PSSWD_EN_reserved0_SHIFT                      6

/* WOL_TOP :: PSSWD_EN :: en_0 [05:05] */
#define BCHP_WOL_TOP_PSSWD_EN_en_0_MASK                            0x00000020
#define BCHP_WOL_TOP_PSSWD_EN_en_0_SHIFT                           5

/* WOL_TOP :: PSSWD_EN :: en_1 [04:04] */
#define BCHP_WOL_TOP_PSSWD_EN_en_1_MASK                            0x00000010
#define BCHP_WOL_TOP_PSSWD_EN_en_1_SHIFT                           4

/* WOL_TOP :: PSSWD_EN :: en_2 [03:03] */
#define BCHP_WOL_TOP_PSSWD_EN_en_2_MASK                            0x00000008
#define BCHP_WOL_TOP_PSSWD_EN_en_2_SHIFT                           3

/* WOL_TOP :: PSSWD_EN :: en_3 [02:02] */
#define BCHP_WOL_TOP_PSSWD_EN_en_3_MASK                            0x00000004
#define BCHP_WOL_TOP_PSSWD_EN_en_3_SHIFT                           2

/* WOL_TOP :: PSSWD_EN :: en_4 [01:01] */
#define BCHP_WOL_TOP_PSSWD_EN_en_4_MASK                            0x00000002
#define BCHP_WOL_TOP_PSSWD_EN_en_4_SHIFT                           1

/* WOL_TOP :: PSSWD_EN :: en_5 [00:00] */
#define BCHP_WOL_TOP_PSSWD_EN_en_5_MASK                            0x00000001
#define BCHP_WOL_TOP_PSSWD_EN_en_5_SHIFT                           0

#endif /* #ifndef BCHP_WOL_TOP_H__ */

/* End of File */
