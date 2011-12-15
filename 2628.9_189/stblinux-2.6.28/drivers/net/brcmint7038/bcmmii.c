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
//**************************************************************************
// File Name  : bcmmii.c
//
// Description: Broadcom PHY/Ethernet Switch Configuration
//               
//**************************************************************************

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/stddef.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include "boardparms.h"
#include "intemac_map.h"
#include "bcmemac.h"
#include "bcmmii.h"

typedef enum {
    MII_100MBIT     = 0x0001,
    MII_FULLDUPLEX  = 0x0002,
    MII_AUTONEG     = 0x0004,
}   MII_CONFIG;

#define EMAC_MDC            0x1f

/* local prototypes */
static MII_CONFIG mii_getconfig(struct net_device *dev);
static MII_CONFIG mii_autoconfigure(struct net_device *dev);
static void mii_setup(struct net_device *dev);
static void mii_soft_reset(struct net_device *dev, int PhyAddr);

#ifdef PHY_LOOPBACK
static void mii_loopback(struct net_device *dev);
#endif

/* probe for an external PHY via MDIO; return PHY address */
int mii_probe(unsigned long base_addr)
{
	volatile EmacRegisters *emac = (volatile EmacRegisters *)base_addr;
	int i, j;

	for(i = 0; i < 32; i++) {
		emac->config |= EMAC_EXT_PHY;
		emac->mdioFreq = EMAC_MII_PRE_EN | EMAC_MDC;
		udelay(10);
		emac->mdioData = MDIO_RD | (i << MDIO_PMD_SHIFT) |
			(MII_BMSR << MDIO_REG_SHIFT);
		for(j = 0; j < 500; j++) {
#if defined (CONFIG_MIPS_BCM_NDVD)
			schedule_timeout_uninterruptible(usecs_to_jiffies(10));
#else
			udelay(10);
#endif
			if(emac->intStatus & EMAC_MDIO_INT) {
				uint16_t data = emac->mdioData & 0xffff;

				emac->intStatus |= EMAC_MDIO_INT;
				if(data != 0x0000 && data != 0xffff)
					return(i);	/* found something */
				break;
			}
		}
	}
	return(BCMEMAC_NO_PHY_ID);
}

/* read a value from the MII */
int mii_read(struct net_device *dev, int phy_id, int location) 
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    volatile EmacRegisters *emac = pDevCtrl->emac;

    emac->mdioData = MDIO_RD | (phy_id << MDIO_PMD_SHIFT) | (location << MDIO_REG_SHIFT);

#if defined (CONFIG_MIPS_BCM_NDVD)
	while ( ! (emac->intStatus & EMAC_MDIO_INT) ) {
		if (!in_atomic()) {
			yield();
		}
	}
#else
    while ( ! (emac->intStatus & EMAC_MDIO_INT) )
        ;
#endif
    emac->intStatus |= EMAC_MDIO_INT;
    return emac->mdioData & 0xffff;
}

/* write a value to the MII */
void mii_write(struct net_device *dev, int phy_id, int location, int val)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    volatile EmacRegisters *emac = pDevCtrl->emac;
    uint32 data;

    data = MDIO_WR | (phy_id << MDIO_PMD_SHIFT) | (location << MDIO_REG_SHIFT) | val;
    emac->mdioData = data;

#if defined (CONFIG_MIPS_BCM_NDVD)
	while ( ! (emac->intStatus & EMAC_MDIO_INT) ) {
		if (!in_atomic()) {
			yield();
		}
	}
#else
    while ( ! (emac->intStatus & EMAC_MDIO_INT) )
        ;
#endif
    emac->intStatus |= EMAC_MDIO_INT;
}

#ifdef PHY_LOOPBACK
/* set the MII loopback mode */
static void mii_loopback(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    uint32 val;

    TRACE(("mii_loopback\n"));

    val = mii_read(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_BMCR);
    /* Disable autonegotiation */
    val &= ~BMCR_ANENABLE;
    /* Enable Loopback */
    val |= BMCR_LOOPBACK;
    mii_write(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_BMCR, val);
}
#endif

int mii_linkstatus(struct net_device *dev, int phy_id)
{
    int val;

    val = mii_read(dev, phy_id, MII_BMSR);
    /* reread: the link status bit is sticky */
    val = mii_read(dev, phy_id, MII_BMSR);

    if (val & BMSR_LSTATUS)
        return 1;
    else
        return 0;
}

void mii_linkstatus_start(struct net_device *dev, int phy_id)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    volatile EmacRegisters *emac = pDevCtrl->emac;
    int location = MII_BMSR;

    emac->mdioData = MDIO_RD | (phy_id << MDIO_PMD_SHIFT) |
        (location << MDIO_REG_SHIFT);
}

int mii_linkstatus_check(struct net_device *dev, int *up)
{
    int done = 0;
    int val;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    volatile EmacRegisters *emac = pDevCtrl->emac;

    if( emac->intStatus & EMAC_MDIO_INT ) {

        emac->intStatus |= EMAC_MDIO_INT;
        val = emac->mdioData & 0xffff;
        *up = ((val & BMSR_LSTATUS) == BMSR_LSTATUS) ? 1 : 0;
        done = 1;
    }

    return( done );
}

void mii_enablephyinterrupt(struct net_device *dev, int phy_id)
{
    mii_write(dev, phy_id, MII_INTERRUPT, 
        MII_INTR_ENABLE | MII_INTR_MASK_FDX | MII_INTR_MASK_LINK_SPEED);
}

void mii_clearphyinterrupt(struct net_device *dev, int phy_id)
{
    mii_read(dev, phy_id, MII_INTERRUPT);
}

/* return the current MII configuration */
static MII_CONFIG mii_getconfig(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    uint32 val;
    MII_CONFIG eConfig = 0;

    TRACE(("mii_getconfig\n"));

    /* Read the Link Partner Ability */
    val = mii_read(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_LPA);
    if (val & LPA_100FULL) {          /* 100 MB Full-Duplex */
        eConfig = (MII_100MBIT | MII_FULLDUPLEX);
    } else if (val & LPA_100HALF) {   /* 100 MB Half-Duplex */
        eConfig = MII_100MBIT;
    } else if (val & LPA_10FULL) {    /* 10 MB Full-Duplex */
        eConfig = MII_FULLDUPLEX;
    } 

    return eConfig;
}

/* Auto-Configure this MII interface */
static MII_CONFIG mii_autoconfigure(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    int i;
    int val;
    MII_CONFIG eConfig;

    TRACE(("mii_autoconfigure\n"));

    /* enable and restart autonegotiation */
    val = mii_read(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_BMCR);
    val |= (BMCR_ANRESTART | BMCR_ANENABLE);
    mii_write(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_BMCR, val);

    /* wait for it to finish */
    for (i = 0; i < 2000; i++) {
#if defined (CONFIG_MIPS_BCM_NDVD)
        msleep(1);
#else
        mdelay(1);
#endif
        val = mii_read(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_BMSR);
#if defined(CONFIG_BCM5325_SWITCH) && (CONFIG_BCM5325_SWITCH == 1)
        if ((val & BMSR_ANEGCOMPLETE) || ((val & BMSR_LSTATUS) == 0)) { 
#else
        if (val & BMSR_ANEGCOMPLETE) {
#endif
            break;
        }
    }

    eConfig = mii_getconfig(dev);
    if (val & BMSR_ANEGCOMPLETE) {
        eConfig |= MII_AUTONEG;
    } 

    return eConfig;
}

static void mii_setup(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    MII_CONFIG eMiiConfig;

    eMiiConfig = mii_autoconfigure(dev);

    if (! (eMiiConfig & MII_AUTONEG)) {
        printk(": Auto-negotiation timed-out\n");
    }

    if ((eMiiConfig & (MII_100MBIT | MII_FULLDUPLEX)) == (MII_100MBIT | MII_FULLDUPLEX)) {
        printk(": 100 MB Full-Duplex (auto-neg)\n");
    } else if (eMiiConfig & MII_100MBIT) {
        printk(": 100 MB Half-Duplex (auto-neg)\n");
    } else if (eMiiConfig & MII_FULLDUPLEX) {
        printk(": 10 MB Full-Duplex (auto-neg)\n");
    } else {
        printk(": 10 MB Half-Duplex (assumed)\n");
    }

#ifdef PHY_LOOPBACK
    /* Enable PHY loopback */
    mii_loopback(dev);
#endif

    /* Check for FULL/HALF duplex */
    if (eMiiConfig & MII_FULLDUPLEX) {
        pDevCtrl->emac->txControl = EMAC_FD;
    }
}

/* reset the MII */
static void mii_soft_reset(struct net_device *dev, int PhyAddr) 
{
    int val;

    mii_write(dev, PhyAddr, MII_BMCR, BMCR_RESET);
    udelay(10); /* wait ~10usec */
    do {
        val = mii_read(dev, PhyAddr, MII_BMCR);
    } while (val & BMCR_RESET);
}

int mii_init(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    volatile EmacRegisters *emac;
    int data32;
    char *phytype = "";
    char *setup = "";

    switch(pDevCtrl->EnetInfo.ucPhyType) {
        case BP_ENET_INTERNAL_PHY:
            phytype = "Internal PHY";
            break;

        case BP_ENET_EXTERNAL_PHY:
            phytype = "External PHY";
            break;

        default:
            printk(KERN_INFO ": Unknown PHY type\n");
            return -1;
    }
    switch (pDevCtrl->EnetInfo.usConfigType) {
        case BP_ENET_CONFIG_MDIO:
            setup = "MDIO";
            break;

        default:
            setup = "Undefined Interface";
            break;
    }
    printk(KERN_INFO "Config %s Through %s", phytype, setup);

    emac = pDevCtrl->emac;
    switch(pDevCtrl->EnetInfo.ucPhyType) {

        case BP_ENET_INTERNAL_PHY:
            /* init mii clock, do soft reset of phy, default is 10Base-T */
            emac->mdioFreq = EMAC_MII_PRE_EN | EMAC_MDC;
            /* reset phy */
            mii_soft_reset(dev, pDevCtrl->EnetInfo.ucPhyAddress);
            mii_setup(dev);
            break;

        case BP_ENET_EXTERNAL_PHY:
            emac->config |= EMAC_EXT_PHY;
            emac->mdioFreq = EMAC_MII_PRE_EN | EMAC_MDC;
            /* reset phy */
            mii_soft_reset(dev, pDevCtrl->EnetInfo.ucPhyAddress);

            data32 = mii_read(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_ADVERTISE);
            data32 |= ADVERTISE_FDFC; /* advertise flow control capbility */
            mii_write(dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_ADVERTISE, data32);

            mii_setup(dev);
            break;
        default:
            break;
    }
    return 0;
}
// End of file
