/*
 *
 * Copyright (c) 2002-2005 Broadcom Corporation 
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
 *
*/
#ifndef _BCMMII_H_
#define _BCMMII_H_

#include <asm/brcmstb/common/bcmtypes.h>

/****************************************************************************
    Internal PHY registers
****************************************************************************/

#define MII_AUX_CTRL_STATUS                 0x18
#define MII_TX_CONTROL                      0x19
#define MII_INTERRUPT                       0x1A
#define MII_AUX_STATUS3                     0x1C
#define MII_BRCM_TEST                       0x1f

/* MII Auxiliary control/status register */
#define MII_AUX_CTRL_STATUS_FULL_DUPLEX     0x0001
#define MII_AUX_CTRL_STATUS_SP100_INDICATOR 0x0002  /* Speed indication */

/* MII TX Control register. */
#define MII_TX_CONTROL_PGA_FIX_ENABLE       0x0100

/* MII Interrupt register. */
#define MII_INTR_ENABLE                     0x4000
#define MII_INTR_MASK_FDX                   0x0800
#define MII_INTR_MASK_LINK_SPEED            0x0400

/* MII Auxilliary Status 3 register. */
#define MII_AUX_STATUS3_MSE_MASK            0xFF00

/* Broadcom Test register. */
#define MII_BRCM_TEST_HARDRESET             0x0200
#define MII_BRCM_TEST_SHADOW_ENABLE         0x0080
#define MII_BRCM_TEST_10BT_SERIAL_NODRIB    0x0008
#define MII_BRCM_TEST_100TX_POWERDOWN       0x0002
#define MII_BRCM_TEST_10BT_POWERDOWN        0x0001

/* Advertisement control register. */
#define ADVERTISE_FDFC                      0x0400  /* MII Flow Control */

/****************************************************************************
    Prototypes
****************************************************************************/

extern int mii_init(struct net_device *dev);
extern int mii_probe(unsigned long base_addr);
extern int mii_read(struct net_device *dev, int phy_id, int location);
extern void mii_write(struct net_device *dev, int phy_id, int location, int data);
extern int mii_linkstatus(struct net_device *dev, int phy_id);
void mii_linkstatus_start(struct net_device *dev, int phy_id);
int mii_linkstatus_check(struct net_device *dev, int *up);
extern void mii_enablephyinterrupt(struct net_device *dev, int phy_id);
extern void mii_clearphyinterrupt(struct net_device *dev, int phy_id);

#endif /* _BCMMII_H_ */
