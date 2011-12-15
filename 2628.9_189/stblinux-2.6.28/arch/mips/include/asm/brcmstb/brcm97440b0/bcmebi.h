/***********************************************************************/
/*                                                                     */
/*   MODULE:  bcmebi.h                                                 */
/*   DATE:    Jan 29, 2002                                             */
/*   PURPOSE: Definitions for EBI block                                */
/*                                                                     */
/***********************************************************************/

/*     Copyright (c) 1999-2005, Broadcom Corporation
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
 */

#ifndef BCMEBI_H
#define BCMEBI_H

#include "bcmmips.h"
#include "bchp_common.h"
#include "bchp_ebi.h"

#if !defined _ASMLANGUAGE
#ifdef __cplusplus
extern "C" {
#endif
#endif


/* CSxCNTL settings */
#define EBI_TA_WAIT         0x40000000
#define EBI_WAIT_MASK       0x1F000000  /* mask for wait states */
#define EBI_WAIT_SHIFT      24          /* shift for wait states */
#define EBI_WAIT(n)         (((n) << EBI_WAIT_SHIFT) & EBI_WAIT_MASK)
#define EBI_THOLD_MASK      0x00f00000
#define EBI_THOLD_SHIFT     20
#define EBI_THOLD(n)        (((n) << EBI_THOLD_SHIFT) & EBI_THOLD_MASK)
#define EBI_TSETUP_MASK     0x000f0000
#define EBI_TSETUP_SHIFT    16
#define EBI_TSETUP(n)       (((n) << EBI_TSETUP_SHIFT) & EBI_TSETUP_MASK)
#define EBI_CSHOLD          0x00008000
#define EBI_SPLITCS         0x00004000
#define EBI_MASKEN          0x00002000
#define EBI_NESAMPLE        0x00001000
#define EBI_M68K            0x00000800
#define EBI_REV_END         0x00000400  /* Reverse Endian */
#define EBI_RE              0x00000400  /* Reverse Endian */
#define EBI_FIFO            0x00000200  /* enable fifo */
#define EBI_TS_SEL          0x00000100  /* drive tsize, not bs_b */
#define EBI_TS_TA_MODE      0x00000080  /* use TS/TA mode */
#define EBI_POLARITY        0x00000040  /* set to invert cs polarity */
#define EBI_WREN            0x00000020  /* enable posted writes */
#define EBI_WORD_WIDE       0x00000010  /* 16-bit peripheral, else 8 */
#define EBI_MSINH           0x00000008
#define EBI_MSINL           0x00000004
#define EBI_TSEN            0x00000002
#define EBI_ENABLE          0x00000001  /* enable this range */

/* EBICONFIG settings */
#define EBI_MASTER_ENABLE   0x80000000  /* allow external masters */
#define EBI_EXT_MAST_PRIO   0x40000000  /* maximize ext master priority */
#define EBI_CTRL_ENABLE     0x20000000
#define EBI_TA_ENABLE       0x10000000

/* EBI DMA control register settings */
#define EBI_TX_INV_IRQ_EN       0x00080000
#define EBI_RX_INV_IRQ_EN       0x00040000
#define EBI_TX_PKT_DN_IRQ_EN    0x00020000
#define EBI_RX_PKT_DN_IRQ_EN    0x00010000
#define EBI_TX_INV_CLR          0x00001000
#define EBI_RX_INV_CLR          0x00000800
#define EBI_CHAINING            0x00000400
#define EBI_HALF_WORD           0x00000100
#define EBI_TX_PKT_DN_CLR       0x00000080
#define EBI_RX_PKT_DN_CLR       0x00000040
#define EBI_TX_BUF_DN_CLR       0x00000020
#define EBI_RX_BUF_DN_CLR       0x00000010
#define EBI_TX_BUF_DN_IRQ_EN    0x00000008
#define EBI_RX_BUF_DN_IRQ_EN    0x00000004
#define EBI_TX_EN               0x00000002
#define EBI_RX_EN               0x00000001

/* EBI DMA status register settings */
#define EBI_TX_INV_DESC         0x00000020
#define EBI_RX_INV_DESC         0x00000010
#define EBI_TX_PKT_DN           0x00000008
#define EBI_RX_PKT_DN           0x00000004
#define EBI_TX_BUF_DN           0x00000002
#define EBI_RX_BUF_DN           0x00000001


#if !defined _ASMLANGUAGE
#ifdef __cplusplus
}
#endif
#endif

#endif /* BCMEBI_H */
