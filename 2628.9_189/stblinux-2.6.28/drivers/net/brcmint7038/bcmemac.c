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
// File Name  : bcmemac.c
//
// Description: This is Linux network driver for the BCM 
// 				EMAC Internal Ethenet Controller onboard the 7110
//               
// Updates    : 09/26/2001  jefchiao.  Created.
//**************************************************************************

#define CARDNAME    "BCMINTMAC"
#define VERSION     "2.0"
#define VER_STR     "v" VERSION " " __DATE__ " " __TIME__

// Turn this on to allow Hardware Multicast Filter
#define MULTICAST_HW_FILTER


#define MOD_DEC_USE_COUNT
#define MOD_INC_USE_COUNT

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#if defined(CONFIG_MODVERSIONS) && ! defined(MODVERSIONS)
   #include <linux/modversions.h> 
   #define MODVERSIONS
#endif  
#endif

#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#if LINUX_VERSION_CODE >= 0x020411      /* 2.4.17 */
#include <linux/slab.h>
#else
#include <linux/malloc.h>
#endif
#include <linux/string.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>

#include <linux/kthread.h>

#include <asm/mipsregs.h>
#include <asm/cacheflush.h>
#include <asm/brcmstb/common/brcmstb.h>
#include <asm/brcmstb/common/brcm-pm.h>

#include "bcmmii.h"
#include "bcmemac.h"
#include "if_net.h"

#include <linux/stddef.h>

#ifdef USE_PROC
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#define PROC_ENTRY_NAME     "driver/ethinfo"
#endif

#ifdef CONFIG_BCMINTEMAC_NETLINK
#include <linux/rtnetlink.h>
#endif

extern unsigned long getPhysFlashBase(void);

static int netdev_ethtool_ioctl(struct net_device *dev, void *useraddr);
static int bcmemac_enet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static int bcmIsEnetUp(struct net_device *dev);

#define POLLTIME_100MS      (HZ/10)
#define POLLTIME_10MS	    (HZ/100)
#define POLLCNT_1SEC        (HZ/POLLTIME_100MS)
#define POLLCNT_FOREVER     ((int) 0x80000000)

#define EMAC_TX_WATERMARK   0x180
#define VLAN_DISABLE        0
#define VLAN_ENABLE         1

#define MAKE4(x)   ((x[3] & 0xFF) | ((x[2] & 0xFF) << 8) | ((x[1] & 0xFF) << 16) | ((x[0] & 0xFF) << 24))
#define MAKE2(x)   ((x[1] & 0xFF) | ((x[0] & 0xFF) << 8))

#if LINUX_VERSION_CODE >= 0x020405      /* starting from 2.4.5 */
#define skb_dataref(x)   (&skb_shinfo(x)->dataref)
#else
#define skb_dataref(x)   skb_datarefp(x)
#endif

/* Ignore the link status if the MSB of the PHY ID is set */
#define IGNORE_LINK_STATUS(phy_id) ((phy_id) & 0x10)

#if defined(CONFIG_BRCM_PM)

#define BCMEMAC_AWAKE		0
#define BCMEMAC_SLEEPING	1

#define BCMEMAC_POWER_ON(dev) do \
	{ \
		if(g_sleep_flag == BCMEMAC_SLEEPING) \
			bcmemac_power_on(NULL); \
	} while(0)

static int bcmemac_power_on(void *arg);

#else /* CONFIG_BRCM_PM */

#define BCMEMAC_POWER_ON(dev) do { } while(0)

#endif /* CONFIG_BRCM_PM */

/*
 * IP Header Optimization, on 7401B0 and on-wards
 * We present an aligned buffer to the DMA engine, but ask it
 * to transfer the data with a 2-byte offset from the top.
 * The idea is to have the IP payload aligned on a 4-byte boundary
 * because the IP payload follows a 14-byte Ethernet header
 */

/*
 * Private fields for 7401B0 on-wards
 * Set the offset into the data buffer (bits 9-10 of RX_CONTROL)
 */
#define RX_CONFIG_PKT_OFFSET_SHIFT		9
#define RX_CONFIG_PKT_OFFSET_MASK		0x0000_0600

#define ENET_POLL_DONE      			0x80000000

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//      External, indirect entry points. 
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
//      Called for "ifconfig ethX up" & "down"
// --------------------------------------------------------------------------
static int bcmemac_net_open(struct net_device * dev);
static int bcmemac_net_close(struct net_device * dev);
// --------------------------------------------------------------------------
//      Watchdog timeout
// --------------------------------------------------------------------------
static void bcmemac_net_timeout(struct net_device * dev);
// --------------------------------------------------------------------------
//      Packet transmission. 
// --------------------------------------------------------------------------
static int bcmemac_net_xmit(struct sk_buff * skb, struct net_device * dev);
// --------------------------------------------------------------------------
//      Allow proc filesystem to query driver statistics
// --------------------------------------------------------------------------
static struct net_device_stats * bcmemac_net_query(struct net_device * dev);
// --------------------------------------------------------------------------
//      Set address filtering mode
// --------------------------------------------------------------------------
static void bcm_set_multicast_list(struct net_device * dev);
// --------------------------------------------------------------------------
//      Set the hardware MAC address.
// --------------------------------------------------------------------------
static int bcm_set_mac_addr(struct net_device *dev, void *p);

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//      Interrupt routines
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
static irqreturn_t bcmemac_net_isr(int irq, void *);
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//      dev->poll() method
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
static int bcmemac_enet_poll(struct napi_struct *napi, int budget);
// --------------------------------------------------------------------------
//  Bottom half service routine. Process all received packets.                  
// --------------------------------------------------------------------------
static uint32 bcmemac_rx(void *ptr, uint32 budget);


// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//      Internal routines
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
/* Add an address to the ARL register */
static void write_mac_address(struct net_device *dev);
/* Remove registered netdevice */
static void bcmemac_init_cleanup(struct net_device *dev);
/* Allocate and initialize tx/rx buffer descriptor pools */
static int bcmemac_init_dev(BcmEnet_devctrl *pDevCtrl);
static int bcmemac_uninit_dev(BcmEnet_devctrl *pDevCtrl);
/* Assign the Rx descriptor ring */
static int assign_rx_buffers(BcmEnet_devctrl *pDevCtrl);
/* Initialize driver's pools of receive buffers and tranmit headers */
static int init_buffers(BcmEnet_devctrl *pDevCtrl);
/* Initialise the Ethernet Switch control registers */
static int init_emac(BcmEnet_devctrl *pDevCtrl);
/* Initialize the Ethernet Switch MIB registers */
static void clear_mib(volatile EmacRegisters *emac);

/* update an address to the EMAC perfect match registers */
static void perfectmatch_update(BcmEnet_devctrl *pDevCtrl, const uint8 * pAddr, bool bValid);

#ifdef MULTICAST_HW_FILTER
/* clear perfect match registers except given pAddr (PR10861) */
static void perfectmatch_clean(BcmEnet_devctrl *pDevCtrl, const uint8 * pAddr);
#endif

/* write an address to the EMAC perfect match registers */
static void perfectmatch_write(volatile EmacRegisters *emac, int reg, const uint8 * pAddr, bool bValid);
/* Initialize DMA control register */
static void init_IUdma(BcmEnet_devctrl *pDevCtrl);
/* Reclaim transmit frames which have been sent out */
static void tx_reclaim_timer(unsigned long arg);
/* rx poll timer */
static void rx_poll_timer(unsigned long arg);
/* Add a Tx control block to the pool */
static void txCb_enq(BcmEnet_devctrl *pDevCtrl, Enet_Tx_CB *txCbPtr);
/* Remove a Tx control block from the pool*/
static Enet_Tx_CB *txCb_deq(BcmEnet_devctrl *pDevCtrl);
static bool haveIPHdrOptimization(void);

#ifdef USE_PROC
static int dev_proc_engine(void *data, loff_t pos, char *buf);
static ssize_t eth_proc_read(struct file *file, char *buf, size_t count, loff_t *pos);
#endif

#ifdef DUMP_DATA
/* Display hex base data */
static void dumpHexData(unsigned char *head, int len);
/* dumpMem32 dump out the number of 32 bit hex data  */
static void dumpMem32(uint32 * pMemAddr, int iNumWords);
#endif

#ifdef CONFIG_BCMINTEMAC_NETLINK
static void bcmemac_link_change_task(struct work_struct *work);
#endif

#ifdef CONFIG_BRCM_PM
static int g_sleep_flag = BCMEMAC_AWAKE;
#endif
static int g_num_devs = 0;
static struct net_device *g_devs[BCMEMAC_MAX_DEVS];
static uint8 g_flash_eaddr[ETH_ALEN];

/* --------------------------------------------------------------------------
    Name: bcmemac_get_free_txdesc
 Purpose: Get Current Available TX desc count
-------------------------------------------------------------------------- */
int bcmemac_get_free_txdesc( struct net_device *dev ){
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    return pDevCtrl->txFreeBds;
}

static inline void IncFlowCtrlAllocRegister(BcmEnet_devctrl *pDevCtrl) 
{
    volatile unsigned long* pAllocReg = &pDevCtrl->dmaRegs->flowctl_ch1_alloc;

    /* Make sure we don't go over bound */
    if (*pAllocReg < NR_RX_BDS) {
        *pAllocReg = 1;
    }
}

static bool haveIPHdrOptimization(void)
{
    bool bhaveIPHeaderOpt;

    // Which revisions of the chip have the fix.
#ifdef CONFIG_MIPS_BCM7438
#define FIXED_REV	0x74380010	/* FIXME */
    volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xb0404000;
#elif defined CONFIG_MIPS_BCM7038
    #define FIXED_REV	0x70380025	//FIXME BCM7038C5?
    volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xb0404000;
#elif defined( CONFIG_MIPS_BCM7318 )
    #define FIXED_REV	0x73180013	/****** 7318B3? *********/
    volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xfffe0600;
#elif defined( CONFIG_MIPS_BCM7400 ) 
    #define FIXED_REV	0x74000001	/****** FIX ME *********/
    volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xb0404000;
#elif defined( CONFIG_MIPS_BCM7401 ) || defined( CONFIG_MIPS_BCM7402 ) || \
      defined( CONFIG_MIPS_BCM7402S )
    #define FIXED_REV   0x74010010      /****** 7401B0 is first chip with the fix *********/
                                    /* Should also work with a true 7402 chip */
    volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xb0404000;
#elif defined(CONFIG_MIPS_BCM7403 ) || defined(CONFIG_MIPS_BCM7452)
    #define FIXED_REV       0x74010010
    volatile unsigned long* pSundryRev = (volatile unsigned long*) 0xb0404000;
#else
    /* 7405, 7325, 7335, newer platforms are all fixed */
    #define FIXED_REV   0x0
    volatile unsigned long* pSundryRev = NULL;
#endif

    if(FIXED_REV) {
	    bhaveIPHeaderOpt = *pSundryRev >= FIXED_REV;
	    printk("SUNDRY revision = %08lx, have IP Hdr Opt=%d\n", *pSundryRev, bhaveIPHeaderOpt? 1 : 0);
    } else {
	    bhaveIPHeaderOpt = 1;
    }

    return bhaveIPHeaderOpt;
}

#ifdef DUMP_DATA
/*
 * dumpHexData dump out the hex base binary data
 */
static void dumpHexData(unsigned char *head, int len)
{
    int i;
    unsigned char *curPtr = head;

    for (i = 0; i < len; ++i)
    {
        if (i % 16 == 0)
            printk("\n");       
        printk("0x%02X, ", *curPtr++);
    }
    printk("\n");

}

/*
 * dumpMem32 dump out the number of 32 bit hex data 
 */
static void dumpMem32(uint32 * pMemAddr, int iNumWords)
{
    int i = 0;
    static char buffer[80];

    sprintf(buffer, "%08X: ", (UINT)pMemAddr);
    printk(buffer);
    while (iNumWords) {
        sprintf(buffer, "%08X ", (UINT)*pMemAddr++);
        printk(buffer);
        iNumWords--;
        i++;
        if ((i % 4) == 0 && iNumWords) {
            sprintf(buffer, "\n%08X: ", (UINT)pMemAddr);
            printk(buffer);
        }
    }
    printk("\n");
}
#endif

/* --------------------------------------------------------------------------
    Name: bcmemac_net_open
 Purpose: Open and Initialize the EMAC on the chip
-------------------------------------------------------------------------- */
static int bcmemac_net_open(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    ASSERT(pDevCtrl != NULL);

    BCMEMAC_POWER_ON(dev);

    TRACE(("%s: bcmemac_net_open, EMACConf=%x, &RxDMA=%x, rxDma.cfg=%x\n", 
                dev->name, pDevCtrl->emac->config, &pDevCtrl->rxDma, pDevCtrl->rxDma->cfg));

    MOD_INC_USE_COUNT;

    /* disable ethernet MAC while updating its registers */
    pDevCtrl->emac->config |= EMAC_DISABLE ;
    while(pDevCtrl->emac->config & EMAC_DISABLE);

    pDevCtrl->emac->config |= EMAC_ENABLE;  
    pDevCtrl->dmaRegs->controller_cfg |= IUDMA_ENABLE;         

    // Tell Tx DMA engine to start from the top
#ifdef IUDMA_INIT_WORKAROUND
    {
        unsigned long diag_rdbk;

        pDevCtrl->dmaRegs->enet_iudma_diag_ctl = 0x100; /* enable to read diags. */
        diag_rdbk = pDevCtrl->dmaRegs->enet_iudma_diag_rdbk;
        pDevCtrl->txDma->descPtr = (uint32)CPHYSADDR(pDevCtrl->txFirstBdPtr);
        pDevCtrl->txNextBdPtr = pDevCtrl->txFirstBdPtr + ((diag_rdbk>>(16+1))&DESC_MASK);
    }
#endif
	
    /* Initialize emac registers */
    if (pDevCtrl->bIPHdrOptimize) {
        pDevCtrl->emac->rxControl = EMAC_FC_EN | EMAC_UNIFLOW  | (2<<RX_CONFIG_PKT_OFFSET_SHIFT);  
    } 
    else {
        pDevCtrl->emac->rxControl = EMAC_FC_EN | EMAC_UNIFLOW /* |EMAC_PROM*/ ;   // Don't allow Promiscuous
    }
    pDevCtrl->rxDma->cfg |= DMA_ENABLE;

    atomic_inc(&pDevCtrl->devInUsed);
    pDevCtrl->dmaRegs->enet_iudma_r5k_irq_msk |= DMA_DONE;

    if(!timer_pending(&pDevCtrl->timer)) {
    	pDevCtrl->timer.expires = jiffies + POLLTIME_10MS;
	add_timer_on(&pDevCtrl->timer, 0);
    }

    pDevCtrl->linkState = bcmIsEnetUp(pDevCtrl->dev);
    if (pDevCtrl->linkState)
        netif_carrier_on(pDevCtrl->dev);
    else
        netif_carrier_off(pDevCtrl->dev);

    napi_enable(&pDevCtrl->napi);

    // Start the network engine
    netif_start_queue(dev);

    return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void bcmemac_poll (struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
	/* disable_irq is not very nice, but with the funny lockless design
	   we have no other choice. */

    BCMEMAC_POWER_ON(dev);

    /* Run TX Queue */
    bcmemac_net_xmit(NULL, pDevCtrl->dev);
    /* Run RX Queue */
    bcmemac_rx(pDevCtrl);
    /* Check for Interrupts */
    bcmemac_net_isr (dev->irq, dev);
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

/* --------------------------------------------------------------------------
    Name: bcmemac_net_close
 Purpose: Stop communicating with the outside world
    Note: Caused by 'ifconfig ethX down'
-------------------------------------------------------------------------- */
static int bcmemac_net_close(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    Enet_Tx_CB *txCBPtr;

    ASSERT(pDevCtrl != NULL);

    TRACE(("%s: bcmemac_net_close\n", dev->name));

    netif_stop_queue(dev);
    napi_disable(&pDevCtrl->napi);

    MOD_DEC_USE_COUNT;

    pDevCtrl->rxDma->cfg &= ~DMA_ENABLE;
    pDevCtrl->emac->config |= EMAC_DISABLE;           

    atomic_dec(&pDevCtrl->devInUsed);
    pDevCtrl->dmaRegs->enet_iudma_r5k_irq_msk &= ~DMA_DONE;

    del_timer_sync(&pDevCtrl->timer);
    del_timer_sync(&pDevCtrl->rx_timer);

    /* free the skb in the txCbPtrHead */
    while (pDevCtrl->txCbPtrHead)  {
        pDevCtrl->txFreeBds += pDevCtrl->txCbPtrHead->nrBds;

        if(pDevCtrl->txCbPtrHead->skb)
            dev_kfree_skb (pDevCtrl->txCbPtrHead->skb);

        txCBPtr = pDevCtrl->txCbPtrHead;

        /* Advance the current reclaim pointer */
        pDevCtrl->txCbPtrHead = pDevCtrl->txCbPtrHead->next;

        /* Finally, return the transmit header to the free list */
        txCb_enq(pDevCtrl, txCBPtr);
    }
    /* Adjust the tail if the list is empty */
    if(pDevCtrl->txCbPtrHead == NULL)
        pDevCtrl->txCbPtrTail = NULL;

    pDevCtrl->txNextBdPtr = pDevCtrl->txFirstBdPtr = pDevCtrl->txBds;
    return 0;
}

/* --------------------------------------------------------------------------
    Name: bcmemac_net_timeout
 Purpose: 
-------------------------------------------------------------------------- */
static void bcmemac_net_timeout(struct net_device * dev)
{
    ASSERT(dev != NULL);

    TRACE(("%s: bcmemac_net_timeout\n", dev->name));

    dev->trans_start = jiffies;

    netif_wake_queue(dev);
}

/* --------------------------------------------------------------------------
    Name: bcm_set_multicast_list
 Purpose: Set the multicast mode, ie. promiscuous or multicast
-------------------------------------------------------------------------- */
static void bcm_set_multicast_list(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);

    BCMEMAC_POWER_ON(dev);

    TRACE(("%s: bcm_set_multicast_list: %08X\n", dev->name, dev->flags));

    /* Promiscous mode */
    if (dev->flags & IFF_PROMISC) {
		pDevCtrl->emac->rxControl |= EMAC_PROM;   
    } else {
		pDevCtrl->emac->rxControl &= ~EMAC_PROM;
    }

#ifndef MULTICAST_HW_FILTER
    /* All Multicast packets (PR10861 Check for any multicast request) */
    if (dev->flags & IFF_ALLMULTI || dev->mc_count) {
        pDevCtrl->emac->rxControl |= EMAC_ALL_MCAST;
    } else {
        pDevCtrl->emac->rxControl &= ~EMAC_ALL_MCAST;
    }
#else
    {
        struct dev_mc_list *dmi = dev->mc_list;
        /* PR10861 - Filter specific group Multicast packets (R&C 2nd Ed., p463) */
        if (dev->flags & IFF_ALLMULTI  || dev->mc_count>(MAX_PMADDR-1)) {
            perfectmatch_clean((BcmEnet_devctrl *)pDevCtrl->dev->priv, dev->dev_addr);
            pDevCtrl->emac->rxControl |= EMAC_ALL_MCAST;
            return;
        } else {
            pDevCtrl->emac->rxControl &= ~EMAC_ALL_MCAST;
        }

        /* No multicast? Just get our own stuff */
        if (dev->mc_count == 0) 
            return;

        /* Store multicast addresses in the prefect match registers */
        perfectmatch_clean((BcmEnet_devctrl *)pDevCtrl->dev->priv, dev->dev_addr);
        for(dmi = dev->mc_list;	dmi ; dmi = dmi->next)
            perfectmatch_update((BcmEnet_devctrl *)pDevCtrl->dev->priv, dmi->dmi_addr, 1);
    }
#endif
}

/* --------------------------------------------------------------------------
    Name: bcmemac_net_query
 Purpose: Return the current statistics. This may be called with the card 
          open or closed.
-------------------------------------------------------------------------- */
static struct net_device_stats *
bcmemac_net_query(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;

    ASSERT(pDevCtrl != NULL);

    return &(pDevCtrl->stats);
}

/*
 * tx_reclaim_timer: reclaim transmit frames which have been sent out
 */
static void tx_reclaim_timer(unsigned long arg)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)arg;
    int bdfilled;
    int linkState;

    if (atomic_read(&pDevCtrl->rxDmaRefill) != 0) {
        atomic_set(&pDevCtrl->rxDmaRefill, 0);
        /* assign packet buffers to all available Dma descriptors */
        bdfilled = assign_rx_buffers(pDevCtrl);
    }

    /* Reclaim transmit Frames which have been sent out */
    bcmemac_net_xmit(NULL, pDevCtrl->dev);

    pDevCtrl->linkstatus_polltimer++;
    if ( pDevCtrl->linkstatus_polltimer >= POLLCNT_1SEC ) {
        linkState = bcmIsEnetUp(pDevCtrl->dev);

        if (linkState != pDevCtrl->linkState) {
            if (linkState != 0) {
                if (pDevCtrl->EnetInfo.ucPhyType == BP_ENET_EXTERNAL_SWITCH) {
                    if (pDevCtrl->linkState == 0)
                    {
                        pDevCtrl->emac->txControl |= EMAC_FD;
                        pDevCtrl->rxDma->cfg |= DMA_ENABLE;                    
                    }
                } else {
                    unsigned long v = mii_read(pDevCtrl->dev, pDevCtrl->EnetInfo.ucPhyAddress, MII_AUX_CTRL_STATUS);
                    if( (v & MII_AUX_CTRL_STATUS_FULL_DUPLEX) != 0) {
                        pDevCtrl->emac->txControl |= EMAC_FD;
                    }
                    else {
                        pDevCtrl->emac->txControl &= ~EMAC_FD;
                    }
                    pDevCtrl->dmaRegs->flowctl_ch1_alloc = (IUDMA_CH1_FLOW_ALLOC_FORCE | NR_RX_BDS);
                    pDevCtrl->rxDma->cfg |= DMA_ENABLE;
                }

#ifdef CONFIG_BCMINTEMAC_NETLINK
                if (pDevCtrl->linkState == 0) {
                    netif_carrier_on(pDevCtrl->dev);
                    schedule_work(&pDevCtrl->link_change_task);
                    printk(KERN_CRIT "%s Link UP.\n",pDevCtrl->dev->name);
                }
#else
                if (pDevCtrl->linkState == 0) {
                    netif_carrier_on(pDevCtrl->dev);
                    printk(KERN_CRIT "%s Link UP.\n",pDevCtrl->dev->name);
                }
#endif
            } else {
#ifdef CONFIG_BCMINTEMAC_NETLINK
                if (pDevCtrl->linkState != 0) {
			netif_carrier_off(pDevCtrl->dev);
			schedule_work(&pDevCtrl->link_change_task);
			printk(KERN_CRIT "%s Link DOWN.\n",pDevCtrl->dev->name);
                }
#else
                if (pDevCtrl->linkState != 0)
                {
                    netif_carrier_off(pDevCtrl->dev);
                    pDevCtrl->rxDma->cfg &= ~DMA_ENABLE;
                    printk(KERN_CRIT "%s Link DOWN.\n",pDevCtrl->dev->name);
                }
#endif
            }

            /* Wake up the user mode application that monitors link status. */
            pDevCtrl->linkState = linkState;
        }
    }

    pDevCtrl->timer.expires = jiffies + POLLTIME_10MS;
    add_timer_on(&pDevCtrl->timer, 0);
}

#ifdef CONFIG_BCMINTEMAC_NETLINK
static void bcmemac_link_change_task(struct work_struct *work)
{
    BcmEnet_devctrl *pDevCtrl;
    
    if (work == NULL) {
        printk(KERN_ERR "%s work is NULL\n", __FUNCTION__);
        return;
    }

    pDevCtrl = container_of(work, struct BcmEnet_devctrl, link_change_task);

    if (pDevCtrl == NULL) {
        printk(KERN_ERR "%s pDevCtrl is NULL\n", __FUNCTION__);
        return;
    }
    
    rtnl_lock();
    
	if (pDevCtrl->linkState) {
		pDevCtrl->dev->flags |= IFF_RUNNING;
		rtmsg_ifinfo(RTM_NEWLINK, pDevCtrl->dev, IFF_RUNNING);
	}
	else {
		clear_bit(__LINK_STATE_START, &pDevCtrl->dev->state);
		pDevCtrl->dev->flags &= ~IFF_RUNNING;
		rtmsg_ifinfo(RTM_DELLINK, pDevCtrl->dev, IFF_RUNNING);
		set_bit(__LINK_STATE_START, &pDevCtrl->dev->state);
	}
	rtnl_unlock();
}
#endif

/*
 * txCb_enq: add a Tx control block to the pool
 */
static void txCb_enq(BcmEnet_devctrl *pDevCtrl, Enet_Tx_CB *txCbPtr)
{
    txCbPtr->next = NULL;

    if (pDevCtrl->txCbQTail) {
        pDevCtrl->txCbQTail->next = txCbPtr;
        pDevCtrl->txCbQTail = txCbPtr;
    }
    else {
        pDevCtrl->txCbQHead = pDevCtrl->txCbQTail = txCbPtr;
    }
}

/*
 * txCb_deq: remove a Tx control block from the pool
 */
static Enet_Tx_CB *txCb_deq(BcmEnet_devctrl *pDevCtrl)
{
    Enet_Tx_CB *txCbPtr;

    if ((txCbPtr = pDevCtrl->txCbQHead)) {
        pDevCtrl->txCbQHead = txCbPtr->next;
        txCbPtr->next = NULL;

        if (pDevCtrl->txCbQHead == NULL)
            pDevCtrl->txCbQTail = NULL;
    }
    else {
        txCbPtr = NULL;
    }
    return txCbPtr;
}

/* --------------------------------------------------------------------------
    Name: bcmemac_xmit_check
 Purpose: Reclaims TX descriptors
-------------------------------------------------------------------------- */
int bcmemac_xmit_check(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    Enet_Tx_CB *txCBPtr;
    Enet_Tx_CB *txedCBPtr;
    unsigned long flags,ret;

    ASSERT(pDevCtrl != NULL);

    /*
     * Obtain exclusive access to transmitter.  This is necessary because
     * we might have more than one stack transmitting at once.
     */
    spin_lock_irqsave(&pDevCtrl->lock, flags);
        
    txCBPtr = NULL;

    /* Reclaim transmitted buffers */
    while (pDevCtrl->txCbPtrHead)  {
        if (EMAC_SWAP32(pDevCtrl->txCbPtrHead->lastBdAddr->length_status) & (DMA_OWN)) {
            break;
        }
        pDevCtrl->txFreeBds += pDevCtrl->txCbPtrHead->nrBds;

        if(pDevCtrl->txCbPtrHead->skb) {
            dev_kfree_skb_any (pDevCtrl->txCbPtrHead->skb);
        }

        txedCBPtr = pDevCtrl->txCbPtrHead;

        /* Advance the current reclaim pointer */
        pDevCtrl->txCbPtrHead = pDevCtrl->txCbPtrHead->next;

        /* 
         * Finally, return the transmit header to the free list.
         * But keep one around, so we don't have to allocate again
         */
        txCb_enq(pDevCtrl, txedCBPtr);
    }

    /* Adjust the tail if the list is empty */
    if(pDevCtrl->txCbPtrHead == NULL)
        pDevCtrl->txCbPtrTail = NULL;

    netif_wake_queue(dev);

    if(pDevCtrl->txCbQHead && (pDevCtrl->txFreeBds>0))
        ret = 0;
    else
        ret = 1;

    spin_unlock_irqrestore(&pDevCtrl->lock, flags);
    return ret;
}

/* --------------------------------------------------------------------------
    Name: bcmemac_net_xmit
 Purpose: Send ethernet traffic
-------------------------------------------------------------------------- */
static int bcmemac_net_xmit(struct sk_buff * skb, struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    Enet_Tx_CB *txCBPtr;
    Enet_Tx_CB *txedCBPtr;
    volatile DmaDesc *firstBdPtr;
    unsigned long flags;

    ASSERT(pDevCtrl != NULL);

    BCMEMAC_POWER_ON(dev);

    /*
     * Obtain exclusive access to transmitter.  This is necessary because
     * we might have more than one stack transmitting at once.
     */
    spin_lock_irqsave(&pDevCtrl->lock, flags);
        
    txCBPtr = NULL;

    /* Reclaim transmitted buffers */
    while (pDevCtrl->txCbPtrHead)  {
        if (EMAC_SWAP32(pDevCtrl->txCbPtrHead->lastBdAddr->length_status) & DMA_OWN) {
            break;
        }

        pDevCtrl->txFreeBds += pDevCtrl->txCbPtrHead->nrBds;

        if(pDevCtrl->txCbPtrHead->skb) {
            dev_kfree_skb_any (pDevCtrl->txCbPtrHead->skb);
        }

        txedCBPtr = pDevCtrl->txCbPtrHead;

        /* Advance the current reclaim pointer */
        pDevCtrl->txCbPtrHead = pDevCtrl->txCbPtrHead->next;

        /* 
         * Finally, return the transmit header to the free list.
         * But keep one around, so we don't have to allocate again
         */
        if (txCBPtr == NULL) {
            txCBPtr = txedCBPtr;
        }
        else {
            txCb_enq(pDevCtrl, txedCBPtr);
        }
    }

    /* Adjust the tail if the list is empty */
    if(pDevCtrl->txCbPtrHead == NULL)
        pDevCtrl->txCbPtrTail = NULL;

    if (skb == NULL) {
        if (txCBPtr != NULL) {
            txCb_enq(pDevCtrl, txCBPtr);
        }
		netif_wake_queue(dev);
        spin_unlock_irqrestore(&pDevCtrl->lock, flags);
        return 0;
    }

    TRACE(("bcmemac_net_xmit, txCfg=%08x, txIntStat=%08x\n", pDevCtrl->txDma->cfg, pDevCtrl->txDma->intStat));
    if (txCBPtr == NULL) {
        txCBPtr = txCb_deq(pDevCtrl);
    }

    /*
     * Obtain a transmit header from the free list.  If there are none
     * available, we can't send the packet. Discard the packet.
     */
    if (txCBPtr == NULL) {
        netif_stop_queue(dev);
        spin_unlock_irqrestore(&pDevCtrl->lock, flags);
	/* set a timer to start the xmit descriptors reclaim */
	mod_timer(&pDevCtrl->timer, jiffies + 1);
        return 1;
    }

    txCBPtr->nrBds = 1;
    txCBPtr->skb = skb;

    /* If we could not queue this packet, free it */
    if (pDevCtrl->txFreeBds < txCBPtr->nrBds) {
        TRACE(("%s: bcmemac_net_xmit low on txFreeBds\n", dev->name));
        txCb_enq(pDevCtrl, txCBPtr);
        netif_stop_queue(dev);
        spin_unlock_irqrestore(&pDevCtrl->lock, flags);
        return 1;
    }


    firstBdPtr = pDevCtrl->txNextBdPtr;
    /* store off the last BD address used to enqueue the packet */
    txCBPtr->lastBdAddr = pDevCtrl->txNextBdPtr;

    /* assign skb data to TX Dma descriptor */
    /*
     * Add the buffer to the ring.
     * Set addr and length of DMA BD to be transmitted.
     */
    dma_cache_wback_inv((unsigned long)skb->data, skb->len);

    pDevCtrl->txNextBdPtr->address = EMAC_SWAP32((uint32)virt_to_phys(skb->data));
    pDevCtrl->txNextBdPtr->length_status  = EMAC_SWAP32((((unsigned long)((skb->len < ETH_ZLEN) ? ETH_ZLEN : skb->len))<<16));
    /*
     * Set status of DMA BD to be transmitted and
     * advance BD pointer to next in the chain.
     */
    if (pDevCtrl->txNextBdPtr == pDevCtrl->txLastBdPtr) {
        pDevCtrl->txNextBdPtr->length_status |= EMAC_SWAP32(DMA_WRAP);
        pDevCtrl->txNextBdPtr = pDevCtrl->txFirstBdPtr;
    }
    else {
        pDevCtrl->txNextBdPtr->length_status |= EMAC_SWAP32(0);
        pDevCtrl->txNextBdPtr++;
    }
#ifdef DUMP_DATA
    printk("bcmemac_net_xmit: len %d", skb->len);
    dumpHexData(skb->data, skb->len);
#endif
    /*
     * Turn on the "OWN" bit in the first buffer descriptor
     * This tells the switch that it can transmit this frame.
     */
    firstBdPtr->length_status |= EMAC_SWAP32(DMA_OWN | DMA_SOP | DMA_EOP | DMA_APPEND_CRC);

    /* Decrement total BD count */
    pDevCtrl->txFreeBds -= txCBPtr->nrBds;

    if ( (pDevCtrl->txFreeBds == 0) || (pDevCtrl->txCbQHead == NULL) ) {
        TRACE(("%s: bcmemac_net_xmit no transmit queue space -- stopping queues\n", dev->name));
        netif_stop_queue(dev);
    }
    /*
     * Packet was enqueued correctly.
     * Advance to the next Enet_Tx_CB needing to be assigned to a BD
     */
    txCBPtr->next = NULL;
    if(pDevCtrl->txCbPtrHead == NULL) {
        pDevCtrl->txCbPtrHead = txCBPtr;
        pDevCtrl->txCbPtrTail = txCBPtr;
    }
    else {
        pDevCtrl->txCbPtrTail->next = txCBPtr;
        pDevCtrl->txCbPtrTail = txCBPtr;
    }

    /* Enable DMA for this channel */
    pDevCtrl->txDma->cfg |= DMA_ENABLE;

    /* update stats */
    pDevCtrl->stats.tx_bytes += ((skb->len < ETH_ZLEN) ? ETH_ZLEN : skb->len);
    pDevCtrl->stats.tx_bytes += 4;
    pDevCtrl->stats.tx_packets++;

    dev->trans_start = jiffies;

    spin_unlock_irqrestore(&pDevCtrl->lock, flags);

    return 0;
}

/* --------------------------------------------------------------------------
    Name: bcmemac_xmit_fragment
 Purpose: Send ethernet traffic Buffer DESC and submit to UDMA
-------------------------------------------------------------------------- */
int bcmemac_xmit_fragment( int ch, unsigned char *buf, int buf_len, 
                           unsigned long tx_flags , struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    Enet_Tx_CB *txCBPtr;
    volatile DmaDesc *firstBdPtr;
	
    ASSERT(pDevCtrl != NULL);
    txCBPtr = txCb_deq(pDevCtrl);
    /*
     * Obtain a transmit header from the free list.  If there are none
     * available, we can't send the packet. Discard the packet.
     */
    if (txCBPtr == NULL) {
        return 1;
    }

    txCBPtr->nrBds = 1;
    txCBPtr->skb = NULL;

    /* If we could not queue this packet, free it */
    if (pDevCtrl->txFreeBds < txCBPtr->nrBds) {
        TRACE(("%s: bcmemac_net_xmit low on txFreeBds\n", dev->name));
        txCb_enq(pDevCtrl, txCBPtr);
        return 1;
    }

	/*--------first fragment------*/
    firstBdPtr = pDevCtrl->txNextBdPtr;
    /* store off the last BD address used to enqueue the packet */
    txCBPtr->lastBdAddr = pDevCtrl->txNextBdPtr;

    /* assign skb data to TX Dma descriptor */
    /*
     * Add the buffer to the ring.
     * Set addr and length of DMA BD to be transmitted.
     */
    pDevCtrl->txNextBdPtr->address = EMAC_SWAP32((uint32)virt_to_phys(buf));
    pDevCtrl->txNextBdPtr->length_status  = EMAC_SWAP32((((unsigned long)buf_len)<<16));	
    /*
     * Set status of DMA BD to be transmitted and
     * advance BD pointer to next in the chain.
     */
    if (pDevCtrl->txNextBdPtr == pDevCtrl->txLastBdPtr) {
        pDevCtrl->txNextBdPtr->length_status |= EMAC_SWAP32(DMA_WRAP);
        pDevCtrl->txNextBdPtr = pDevCtrl->txFirstBdPtr;
    }
    else {
        pDevCtrl->txNextBdPtr++;
    }

    /*
     * Turn on the "OWN" bit in the first buffer descriptor
     * This tells the switch that it can transmit this frame.
     */	
    firstBdPtr->length_status &= ~EMAC_SWAP32(DMA_SOP |DMA_EOP | DMA_APPEND_CRC);
    firstBdPtr->length_status |= EMAC_SWAP32(DMA_OWN | tx_flags | DMA_APPEND_CRC);
   
    /* Decrement total BD count */
    pDevCtrl->txFreeBds -= txCBPtr->nrBds;

	/*
     * Packet was enqueued correctly.
     * Advance to the next Enet_Tx_CB needing to be assigned to a BD
     */
    txCBPtr->next = NULL;
    if(pDevCtrl->txCbPtrHead == NULL) {
        pDevCtrl->txCbPtrHead = txCBPtr;
        pDevCtrl->txCbPtrTail = txCBPtr;
    }
    else{
        pDevCtrl->txCbPtrTail->next = txCBPtr;
        pDevCtrl->txCbPtrTail = txCBPtr;
    }

     /* Enable DMA for this channel */
    pDevCtrl->txDma->cfg |= DMA_ENABLE;

   /* update stats */
    pDevCtrl->stats.tx_bytes += buf_len; //((skb->len < ETH_ZLEN) ? ETH_ZLEN : skb->len);
    pDevCtrl->stats.tx_bytes += 4;
    pDevCtrl->stats.tx_packets++;

    dev->trans_start = jiffies;


    return 0;
}

EXPORT_SYMBOL(bcmemac_xmit_fragment);

/* --------------------------------------------------------------------------
    Name: bcmemac_xmit_multibuf
 Purpose: Send ethernet traffic in multi buffers (hdr, buf, tail)
-------------------------------------------------------------------------- */
int bcmemac_xmit_multibuf( int ch, unsigned char *hdr, int hdr_len, unsigned char *buf, int buf_len, unsigned char *tail, int tail_len, struct net_device *dev)
{
    unsigned long flags;
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    
    while(bcmemac_xmit_check(dev));

    /*
     * Obtain exclusive access to transmitter.  This is necessary because
     * we might have more than one stack transmitting at once.
     */
    spin_lock_irqsave(&pDevCtrl->lock, flags);

    /* Header + Optional payload in two parts */
    if((hdr_len> 0) && (buf_len > 0) && (tail_len > 0) && (hdr) && (buf) && (tail)){ 
        /* Send Header */
        while(bcmemac_xmit_fragment( ch, hdr, hdr_len, DMA_SOP, dev))
            bcmemac_xmit_check(dev);
        /* Send First Fragment */  
        while(bcmemac_xmit_fragment( ch, buf, buf_len, 0, dev))
            bcmemac_xmit_check(dev);
        /* Send 2nd Fragment */ 	
        while(bcmemac_xmit_fragment( ch, tail, tail_len, DMA_EOP, dev))
            bcmemac_xmit_check(dev);
    }
    /* Header + Optional payload */
    else if((hdr_len> 0) && (buf_len > 0) && (hdr) && (buf)){
        /* Send Header */
        while(bcmemac_xmit_fragment( ch, hdr, hdr_len, DMA_SOP, dev))
            bcmemac_xmit_check(dev);
        /* Send First Fragment */
        while(bcmemac_xmit_fragment( ch, buf, buf_len, DMA_EOP, dev))
            bcmemac_xmit_check(dev);
    }
    /* Header Only (includes payload) */
    else if((hdr_len> 0) && (hdr)){ 
        /* Send Header */
        while(bcmemac_xmit_fragment( ch, hdr, hdr_len, DMA_SOP | DMA_EOP, dev))
            bcmemac_xmit_check(dev);
    }
    else{
    	spin_unlock_irqrestore(&pDevCtrl->lock, flags);
        return 0; /* Drop the packet */
    }
    spin_unlock_irqrestore(&pDevCtrl->lock, flags);
    return 0;
}

/*
 * Atlanta PR5760 fix:
 *               // PR5760 - If there are no more packets available, and the last
 *               // one we saw had an overflow, we need to assume that the EMAC
 *               // has wedged and reset it.  This is the workaround for a
 *               // variant of this problem, where the MAC can stop sending us
 *               // packets altogether (the "silent wedge" condition).
 */
static int gNumberOfOverflows = 0;
static int gNoDescCount = 0;
static  int loopCount = 0;  // This should be local to the rx routine, but we need it here so that 
					    // dump_emac() can display it.  It is not really a global var.
static atomic_t resetting_EMAC = ATOMIC_INIT(0);

// PR5760 - if there are too many packets with the overflow bit set,
// then reset the EMAC.  We arrived at a threshold of 8 packets based
// on empirical analysis and testing (smaller values are too aggressive
// and larger values wait too long).
#define OVERFLOW_THRESHOLD 8
#define NODESC_THRESHOLD 20
#define RX_ETH_DATA_BUFF_SIZE       ENET_MAX_MTU_SIZE

/* These should be part of pDevCtrl, as they are not reset */
static int gNumResetsErrors = 0;
static int gNumNoDescErrors = 0;
static int gNumOverflowErrors = 0;
/* For debugging */
static unsigned int gLastErrorDmaFlag, gLastDmaFlag;

void
bcmemac_disable_ints(BcmEnet_devctrl *pDevCtrl)
{
    //disable_irq(pDevCtrl->rxIrq);
    pDevCtrl->dmaRegs->enet_iudma_r5k_irq_msk &= ~0x2;

    /* mask Rx interrupts */
    pDevCtrl->rxDma->intMask &= ~DMA_DONE; 
	
}
void
bcmemac_enable_ints(BcmEnet_devctrl *pDevCtrl)
{
    //enable_irq(pDevCtrl->rxIrq);
    pDevCtrl->dmaRegs->enet_iudma_r5k_irq_msk |= 0x2;

    /* enable Rx interrupts */
    pDevCtrl->rxDma->intMask |= DMA_DONE;
   
}

void
ResetEMAConErrors(BcmEnet_devctrl *pDevCtrl)
{
    if (1 == atomic_add_return(1, &resetting_EMAC)) {
        // Clear the overflow counter.
        gNumberOfOverflows = 0;
        gNumResetsErrors++;
        // Set the disable bit, wait for h/w to clear it, then set
        // the enable bit.
        pDevCtrl->emac->config |= EMAC_DISABLE;
        while (pDevCtrl->emac->config & EMAC_DISABLE);
            pDevCtrl->emac->config |= EMAC_ENABLE;   
    }
    // Otherwise another thread is resetting it.
    else {
        printk(KERN_NOTICE "ResetEMAConErrors: Another thread is resetting the EMAC, count=%d\n", 
        atomic_read(&resetting_EMAC));
    }
    atomic_dec(&resetting_EMAC);
}

static void rx_poll_timer(unsigned long arg)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)arg;
    netif_rx_schedule(pDevCtrl->dev, &pDevCtrl->napi);
}

#if defined (CONFIG_MIPS_BCM_NDVD)
int	bcmemac_work_totals[NR_RX_BDS];
#endif

static int bcmemac_enet_poll(struct napi_struct *napi, int budget)
{   
    BcmEnet_devctrl *pDevCtrl = container_of(napi, BcmEnet_devctrl, napi);
    struct net_device *dev;
    uint32 work_done;

    dev = pDevCtrl->dev;
    BCMEMAC_POWER_ON(dev);

    work_done = bcmemac_rx(pDevCtrl, budget);
    work_done &= ~ENET_POLL_DONE;

#if defined (CONFIG_MIPS_BCM_NDVD)

    bcmemac_work_totals[ (work_done > NR_RX_BDS-1)?NR_RX_BDS-1:work_done]++;

    if (work_done == 0) {
	netif_rx_complete(dev, napi);
        bcmemac_enable_ints(pDevCtrl);
	return work_done;;
    }

    if (work_done < budget ) {
	netif_rx_complete(dev, napi);
	mod_timer(&pDevCtrl->rx_timer, jiffies + 1);
    }

#else

    if (work_done < budget ) {
	netif_rx_complete(dev, napi);
        bcmemac_enable_ints(pDevCtrl);
    }

#endif

    return work_done;;
}

/*
 * bcmemac_net_isr: Acknowledge interrupts and check if any packets have
 *                  arrived
 */
static irqreturn_t bcmemac_net_isr(int irq, void *dev_id)
{
    BcmEnet_devctrl *pDevCtrl = dev_id;

    /*
     * Disable IRQ and use NAPI polling loop.  IRQ will be re-enabled after all
     * packets are processed.
     */

    pDevCtrl->rxDma->intStat = DMA_DONE;            // clr interrupt

    bcmemac_disable_ints(pDevCtrl);

    netif_rx_schedule(pDevCtrl->dev, &pDevCtrl->napi);

    return IRQ_HANDLED;
}

/*
 *  bcmemac_rx: Process all received packets.
 */
#define MAX_RX                  0x0fffffff //to disable it
static uint32 bcmemac_rx(void *ptr, uint32 budget)
{
    BcmEnet_devctrl *pDevCtrl = ptr;
    struct sk_buff *skb;
    unsigned long dmaFlag;
    unsigned char *pBuf;
    int len;
    int bdfilled;
    unsigned int packetLength = 0;
    uint32 rxpktgood = 0;
    uint32 rxpktprocessed = 0;
    uint32 rxpktmax = budget + (budget / 2);
    int brcm_hdr_len = 0;
    int brcm_fcs_len = 0;

    dmaFlag = ((EMAC_SWAP32(pDevCtrl->rxBdReadPtr->length_status)) & 0xffff);
    gLastDmaFlag = dmaFlag;
    loopCount = 0;

    /* Loop here for each received packet */
    while(!(dmaFlag & DMA_OWN) && (dmaFlag & DMA_EOP)) {
        if (dmaFlag & DMA_OWN) {
            break;
        }
        rxpktprocessed++;


        // Stop when we hit a buffer with no data, or a BD w/ no buffer.
        // This implies that we caught up with DMA, or that we haven't been
        // able to fill the buffers.
        if ( (pDevCtrl->rxBdReadPtr->address == (uint32) NULL)) {
            // PR5760 - If there are no more packets available, and the last
            // one we saw had an overflow, we need to assume that the EMAC
            // has wedged and reset it.  This is the workaround for a
            // variant of this problem, where the MAC can stop sending us
            // packets altogether (the "silent wedge" condition).
            if (gNumberOfOverflows > 0) {
                printk(KERN_CRIT "Handling last packet overflow, resetting pDevCtrl->emac->config, ovf=%d\n", gNumberOfOverflows);
                ResetEMAConErrors(pDevCtrl);
            }

            break;
        }

        if (dmaFlag & (EMAC_CRC_ERROR | EMAC_OV | EMAC_NO | EMAC_LG |EMAC_RXER)) {
            if (dmaFlag & EMAC_CRC_ERROR) {
                pDevCtrl->stats.rx_crc_errors++;
            }
            if (dmaFlag & EMAC_OV) {
                pDevCtrl->stats.rx_over_errors++;
            }
            if (dmaFlag & EMAC_NO) {
                pDevCtrl->stats.rx_frame_errors++;
            }
            if (dmaFlag & EMAC_LG) {
                pDevCtrl->stats.rx_length_errors++;
            }
            pDevCtrl->stats.rx_dropped++;
            pDevCtrl->stats.rx_errors++;

            if( rxpktprocessed < rxpktmax ) {
                continue;
            }

            /* Debug */
            gLastErrorDmaFlag = dmaFlag;

            /* THT Starting with 7110B0 (Atlanta's PR5760), look for resetting the
             * EMAC on overflow condition
             */
            if ((dmaFlag & DMA_OWN) == 0) {
                uint32 bufferAddress;

                // PR5760 - keep track of the number of overflow packets.
                if (dmaFlag & EMAC_OV) {
                    gNumberOfOverflows++;
                    gNumOverflowErrors++;
                }

                // Keep a running total of the packet length.
                packetLength += ((EMAC_SWAP32(pDevCtrl->rxBdReadPtr->length_status))>>16);

                // Pull the buffer from the BD and clear the BD so that it can be
                // reassigned later.
                bufferAddress = (uint32) EMAC_SWAP32(pDevCtrl->rxBdReadPtr->address);
                pDevCtrl->rxBdReadPtr->address = 0;

                pBuf = (unsigned char *)phys_to_virt(bufferAddress);
                skb = (struct sk_buff *)*(unsigned long *)(pBuf-4);
                atomic_set(&skb->users, 1);

                if (atomic_read(&pDevCtrl->rxDmaRefill) == 0) {
                    bdfilled = assign_rx_buffers(pDevCtrl);
                }

                /* Advance BD ptr to next in ring */
                IncRxBDptr(pDevCtrl->rxBdReadPtr, pDevCtrl);

                // If this is the last buffer in the packet, stop.
                if (dmaFlag & DMA_EOP) {
                    packetLength = 0;
                }
            }

            // Clear the EMAC block receive logic for oversized packets.  On
            // a really, really long packet (often caused by duplex
            // mismatches), the EMAC_LG bit may not be set, so I need to
            // check for this condition separately.
            if ((packetLength >= 2048) ||

            // PR5760 - if there are too many packets with the overflow bit set,
            // then reset the EMAC.  We arrived at a threshold of 8 packets based
            // on empirical analysis and testing (smaller values are too aggressive
            // and larger values wait too long).
                (gNumberOfOverflows > OVERFLOW_THRESHOLD)) {
                ResetEMAConErrors(pDevCtrl);
            }

            //Read more until EOP or good packet
            dmaFlag = ((EMAC_SWAP32(pDevCtrl->rxBdReadPtr->length_status)) & 0xffff);
            gLastDmaFlag = dmaFlag; 
            
            /* Exit if we surpass number of packets */
            continue;
        }/* if error packet */

        gNumberOfOverflows = 0;
        gNoDescCount = 0;

        pBuf = (unsigned char *)(phys_to_virt(EMAC_SWAP32(pDevCtrl->rxBdReadPtr->address)));

#if defined(CONFIG_MIPS_BCM7405)
        /*
         * THT: Invalidate the RAC cache again, since someone may have read near the vicinity
         * of the buffer.  This is necessary because the RAC cache is much larger than the CPU cache
         */
        bcm_inv_rac_all();
#endif

        len = ((EMAC_SWAP32(pDevCtrl->rxBdReadPtr->length_status))>>16);
        /* Null the BD field to prevent reuse */
        pDevCtrl->rxBdReadPtr->length_status &= EMAC_SWAP32(0xffff0000); //clear status.
        pDevCtrl->rxBdReadPtr->address = EMAC_SWAP32(0);

        /* Advance BD ptr to next in ring */
        IncRxBDptr(pDevCtrl->rxBdReadPtr, pDevCtrl);
        // Recover the SKB pointer saved during assignment.
        skb = (struct sk_buff *)*(unsigned long *)(pBuf-4);

        dmaFlag = ((EMAC_SWAP32(pDevCtrl->rxBdReadPtr->length_status))&0xffff);
        gLastDmaFlag = dmaFlag;

        skb_pull(skb, brcm_hdr_len);
		/* Remove 2 bytes trailer if in promiscous mode, for PR44081 */
#ifdef CONFIG_NET_POLL_CONTROLLER

	skb_trim(skb, len - ETH_CRC_LEN - brcm_hdr_len - brcm_fcs_len - 2);
#else
		if(pDevCtrl->emac->rxControl & EMAC_PROM)
			skb_trim(skb, len - ETH_CRC_LEN - brcm_hdr_len - brcm_fcs_len - 2);
		else
        	skb_trim(skb, len - ETH_CRC_LEN - brcm_hdr_len - brcm_fcs_len);
#endif

#ifdef DUMP_DATA
        printk("bcmemac_rx :");
        dumpHexData(skb->data, 32);
#endif

        /* Finish setting up the received SKB and send it to the kernel */
        skb->dev = pDevCtrl->dev;
        skb->protocol = eth_type_trans(skb, pDevCtrl->dev);

        /* Allocate a new SKB for the ring */
        if (atomic_read(&pDevCtrl->rxDmaRefill) == 0) {
            bdfilled = assign_rx_buffers(pDevCtrl);
        }

        pDevCtrl->stats.rx_packets++;
        pDevCtrl->stats.rx_bytes += len;
        rxpktgood++;

        /* Notify kernel */
        netif_receive_skb(skb);
        
        if (--budget <= 0) {
            break;
        }
    }

    if (dmaFlag & DMA_OWN) {
        rxpktgood |= ENET_POLL_DONE;
    }
    pDevCtrl->dev->last_rx = jiffies;

    /* Reclaim TX Queue */
    bcmemac_net_xmit(NULL, pDevCtrl->dev);

    return rxpktgood;
}


/*
 * Set the hardware MAC address.
 */
static int bcm_set_mac_addr(struct net_device *dev, void *p)
{
    struct sockaddr *addr=p;

    BCMEMAC_POWER_ON(dev);

    if(netif_running(dev))
        return -EBUSY;

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

    write_mac_address(dev);

    return 0;
}

/*
 * write_mac_address: store MAC address into EMAC perfect match registers                   
 */
void write_mac_address(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)dev->priv;
    volatile uint32 data32bit;

    ASSERT(pDevCtrl != NULL);

    data32bit = pDevCtrl->emac->config;
    if (data32bit & EMAC_ENABLE) {
        /* disable ethernet MAC while updating its registers */
        pDevCtrl->emac->config &= ~EMAC_ENABLE ;           
        pDevCtrl->emac->config |= EMAC_DISABLE ;           
        while(pDevCtrl->emac->config & EMAC_DISABLE);     
    }

    /* add our MAC address to the perfect match register */
    perfectmatch_update((BcmEnet_devctrl *)dev->priv, dev->dev_addr, 1);

    if (data32bit & EMAC_ENABLE) {
        pDevCtrl->emac->config = data32bit;
    }
}


/*******************************************************************************
*
* skb_headerinit
*
* Reinitializes the socket buffer.  Lifted from skbuff.c
*
* RETURNS: None.
*/

static inline void skb_headerinit(void *p, void  *cache, 
        unsigned long flags)
{
    struct sk_buff *skb = p;

    skb->next = NULL;
    skb->prev = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
    skb->list = NULL;
    skb->stamp.tv_sec=0;    /* No idea about time */
    skb->security = 0;  /* By default packets are insecure */
#endif
    skb->sk = NULL;
    skb->dev = NULL;
    skb->dst = NULL;
    /* memset(skb->cb, 0, sizeof(skb->cb)); */
    skb->pkt_type = PACKET_HOST;    /* Default type */
    skb->ip_summed = 0;
    skb->priority = 0;
    skb->destructor = NULL;

#ifdef CONFIG_NETFILTER
    skb->nfmark = 0;
    skb->nfct = NULL;
#ifdef CONFIG_NETFILTER_DEBUG
    skb->nf_debug = 0;
#endif
#endif
#ifdef CONFIG_NET_SCHED
    skb->tc_index = 0;
#endif
}

/*
 * Alignment seems to have little real effect on througput.
 * Large alignement values, > 128, make the sk_buff jump from 2k to 4k bytes,
 * so avoid that if possible.
 * -- jfraser
 */

#define Rx_ALIGN 16
#define Rx_ALIGN_MASK (Rx_ALIGN-1)

static void handleAlignment(BcmEnet_devctrl *pDevCtrl, struct sk_buff* skb)
{
    /* Even that the EMAC now accept any buffers aligned on a 4-B boundary, we still gain
     * better performance if we align the buffer on a 256-B boundary
     */
    // We will reserve 20 bytes, and wants that skb->data to be on 
    // a 256B boundary.

    volatile unsigned long boundary, curData, resHeader;

    curData = (unsigned long) skb->data;
    boundary = (curData+20+Rx_ALIGN-1) & ~Rx_ALIGN_MASK;
    resHeader = boundary - curData;

    // If we don't have room for 20B, go to next boundary.  Sanity check
    if (resHeader < 20) {
	boundary += Rx_ALIGN;
    }
    resHeader = boundary - curData - 4;

    // This advances skb->data 4B shy of the boundary,
    // and also includes the 16B headroom commented out below,
    // by virtue of our sanity check above.
    skb_reserve(skb, resHeader);

    /* reserve space ditto __dev_alloc_skb */
    // skb_reserve(skb, 16);

    *(unsigned int *)skb->data = (unsigned int)skb;
    skb_reserve(skb, 4);

    // Make sure it is on Rx_ALIGN boundary, should never happen if our
    // calculation was correct.
    if ((unsigned long) skb->data & Rx_ALIGN_MASK) {
        printk("$$$ skb buffer 0x%lx is NOT aligned on %dB boundary\n", (unsigned long) skb->data, Rx_ALIGN );
    }

    if (pDevCtrl->bIPHdrOptimize) {
        /* Advance the data pointer 2 bytes */
        skb_reserve(skb, 2);
    }
}

/*
 * assign_rx_buffers: Beginning with the first receive buffer descriptor
 *                  used to receive a packet since this function last
 *                  assigned a buffer, reassign buffers to receive
 *                  buffer descriptors, and mark them as EMPTY; available
 *                  to be used again.
 *
 */
static int assign_rx_buffers(BcmEnet_devctrl *pDevCtrl)
{
    struct sk_buff *skb;
    uint16  bdsfilled=0;
    int devInUsed;
    int i;

    /*
     * check assign_rx_buffers is in process?
     * This function may be called from various context, like timer
     * or bcmemac_rx
     */
    if(test_and_set_bit(0, &pDevCtrl->rxbuf_assign_busy)) {
        return bdsfilled;
    }

    /* loop here for each buffer needing assign */
    for (;;)
    {
        /* exit if no receive buffer descriptors are in "unused" state */
        if(EMAC_SWAP32(pDevCtrl->rxBdAssignPtr->address) != 0)
        {
            break;
        }

        skb = pDevCtrl->skb_pool[pDevCtrl->nextskb++];
        if (pDevCtrl->nextskb == MAX_RX_BUFS)
            pDevCtrl->nextskb = 0;

        /* check if skb is free */
        if (skb_shared(skb) || 
            atomic_read(skb_dataref(skb)) > 1) {
            /* find next free skb */
            for (i = 0; i < MAX_RX_BUFS; i++) {
                skb = pDevCtrl->skb_pool[pDevCtrl->nextskb++];
                if (pDevCtrl->nextskb == MAX_RX_BUFS)
                    pDevCtrl->nextskb = 0;
                if ((skb_shared(skb) == 0) && 
                    atomic_read(skb_dataref(skb)) <= 1) {
                    /* found a free skb */
                    break;
                }
            }
            if (i == MAX_RX_BUFS) {
                /* no free skb available now */
                /* rxDma is empty, set flag and let timer function to refill later */
                atomic_set(&pDevCtrl->rxDmaRefill, 1);
                break;
            }
        }

        atomic_set(&pDevCtrl->rxDmaRefill, 0);
        skb_headerinit(skb, NULL, 0);  /* clean state */

        /* init the skb, in case other app. modified the skb pointer */
        skb->data = skb->tail = skb->head;
        skb->end = skb->data + (skb->truesize - sizeof(struct sk_buff));
        skb->len = 0;
        skb->data_len = 0;
        skb->cloned = 0;

        handleAlignment(pDevCtrl, skb);

        skb_put(skb, pDevCtrl->rxBufLen);

        /*
         * Set the user count to two so when the upper layer frees the
         * socket buffer, only the user count is decremented.
         */
		if(atomic_read(&skb->users) == 1)
        	atomic_inc(&skb->users);

        /* kept count of any BD's we refill */
        bdsfilled++;

        dma_cache_wback_inv((unsigned long)skb->data, pDevCtrl->rxBufLen);


        /* assign packet, prepare descriptor, and advance pointer */
        if (pDevCtrl->bIPHdrOptimize) {
            unsigned long dmaAddr = (unsigned long) skb->data-2;
            if (dmaAddr & Rx_ALIGN_MASK) {
                printk("@@@@@@@@@@@@@@@ DMA Address %08x not aligned\n", (unsigned int) dmaAddr);
            }
            pDevCtrl->rxBdAssignPtr->address = EMAC_SWAP32((uint32)
		virt_to_phys((volatile void *)dmaAddr));
        }
        else {
            pDevCtrl->rxBdAssignPtr->address = EMAC_SWAP32((uint32)virt_to_phys(skb->data));
        }
        pDevCtrl->rxBdAssignPtr->length_status  = EMAC_SWAP32((pDevCtrl->rxBufLen<<16));

        IncFlowCtrlAllocRegister(pDevCtrl);

        /* turn on the newly assigned BD for DMA to use */
        if (pDevCtrl->rxBdAssignPtr == pDevCtrl->rxLastBdPtr) {
            pDevCtrl->rxBdAssignPtr->length_status |= EMAC_SWAP32(DMA_OWN | DMA_WRAP);
            pDevCtrl->rxBdAssignPtr = pDevCtrl->rxFirstBdPtr;
        }
        else {
            pDevCtrl->rxBdAssignPtr->length_status |= EMAC_SWAP32(DMA_OWN);
            pDevCtrl->rxBdAssignPtr++;
        }
    }

    /*
     * restart DMA in case the DMA ran out of descriptors
     */
    devInUsed = atomic_read(&pDevCtrl->devInUsed);
    if (devInUsed > 0) {
        pDevCtrl->rxDma->cfg |= DMA_ENABLE;
    }

    clear_bit(0, &pDevCtrl->rxbuf_assign_busy);

    return bdsfilled;
}

/*
 * perfectmatch_write: write an address to the EMAC perfect match registers
 */
static void perfectmatch_write(volatile EmacRegisters *emac, int reg, const uint8 * pAddr, bool bValid)
{
    volatile uint32 *pmDataLo;
    volatile uint32 *pmDataHi;
  
    switch (reg) {
    case 0:
        pmDataLo = &emac->pm0DataLo;
        pmDataHi = &emac->pm0DataHi;
        break;
    case 1:
        pmDataLo = &emac->pm1DataLo;
        pmDataHi = &emac->pm1DataHi;
        break;
    case 2:
        pmDataLo = &emac->pm2DataLo;
        pmDataHi = &emac->pm2DataHi;  		/* PR 10861 - fixed wrong value here */
        break;
    case 3:
        pmDataLo = &emac->pm3DataLo;
        pmDataHi = &emac->pm3DataHi;
        break;
    case 4:
        pmDataLo = &emac->pm4DataLo;
        pmDataHi = &emac->pm4DataHi;
        break;
    case 5:
        pmDataLo = &emac->pm5DataLo;
        pmDataHi = &emac->pm5DataHi;
        break;
    case 6:
        pmDataLo = &emac->pm6DataLo;
        pmDataHi = &emac->pm6DataHi;  		/* PR 10861 - fixed wrong value here */
        break;
    case 7:
        pmDataLo = &emac->pm7DataLo;
        pmDataHi = &emac->pm7DataHi;
        break;		
    default:
        return;
    }
    /* Fill DataHi/Lo */
    if (bValid == 1)
        *pmDataLo = MAKE4((pAddr + 2));
    *pmDataHi = MAKE2(pAddr) | (bValid << 16);
}

/*
 * perfectmatch_update: update an address to the EMAC perfect match registers
 */
static void perfectmatch_update(BcmEnet_devctrl *pDevCtrl, const uint8 * pAddr, bool bValid)
{
    int i;
    unsigned int aged_ref;
    int aged_entry;

    /* check if this address is in used */
    for (i = 0; i < MAX_PMADDR; i++) {
        if (pDevCtrl->pmAddr[i].valid == 1) {
            if (memcmp (pDevCtrl->pmAddr[i].dAddr, pAddr, ETH_ALEN) == 0) {
                if (bValid == 0) {
                    /* clear the valid bit in PM register */
                    perfectmatch_write(pDevCtrl->emac, i, pAddr, bValid);
                    /* clear the valid bit in pDevCtrl->pmAddr */
                    pDevCtrl->pmAddr[i].valid = bValid;
                } else {
                    pDevCtrl->pmAddr[i].ref++;
                }
                return;
            }
        }
    }
    if (bValid == 1) {
        for (i = 0; i < MAX_PMADDR; i++) {
            /* find an available entry for write pm address */
            if (pDevCtrl->pmAddr[i].valid == 0) {
                break;
            }
        }
        if (i == MAX_PMADDR) {
            aged_ref = 0xFFFFFFFF;
            aged_entry = 0;
            /* aged out an entry */
            for (i = 0; i < MAX_PMADDR; i++) {
                if (pDevCtrl->pmAddr[i].ref < aged_ref) {
                    aged_ref = pDevCtrl->pmAddr[i].ref;
                    aged_entry = i;
                }
            }
            i = aged_entry;
        }
        /* find a empty entry and add the address */
        perfectmatch_write(pDevCtrl->emac, i, pAddr, bValid);

        /* update the pDevCtrl->pmAddr */
        pDevCtrl->pmAddr[i].valid = bValid;
        memcpy(pDevCtrl->pmAddr[i].dAddr, pAddr, ETH_ALEN);
        pDevCtrl->pmAddr[i].ref = 1;
    }
}

#ifdef MULTICAST_HW_FILTER
/*
 * perfectmatch_clean: Clear EMAC perfect match registers not matched by pAddr (PR10861)
 */
static void perfectmatch_clean(BcmEnet_devctrl *pDevCtrl, const uint8 * pAddr)
{
    int i;

    /* check if this address is in used */
    for (i = 0; i < MAX_PMADDR; i++) {
        if (pDevCtrl->pmAddr[i].valid == 1) {
            if (memcmp (pDevCtrl->pmAddr[i].dAddr, pAddr, ETH_ALEN) != 0) {
                /* clear the valid bit in PM register */
                perfectmatch_write(pDevCtrl->emac, i, pAddr, 0);
                /* clear the valid bit in pDevCtrl->pmAddr */
                pDevCtrl->pmAddr[i].valid = 0;
            }
        }
    }
}
#endif

/*
 * clear_mib: Initialize the Ethernet Switch MIB registers
 */
static void clear_mib(volatile EmacRegisters *emac)
{
    int                   i;
    volatile uint32       *pt;

    pt = (uint32 *)&emac->tx_mib;
    for( i = 0; i < NumEmacTxMibVars; i++ ) {
        *pt++ = 0;
    }

    pt = (uint32 *)&emac->rx_mib;;
    for( i = 0; i < NumEmacRxMibVars; i++ ) {
        *pt++ = 0;
    }
}

/*
 * init_emac: Initializes the Ethernet Switch control registers
 */
static int init_emac(BcmEnet_devctrl *pDevCtrl)
{
    volatile EmacRegisters *emac;

    TRACE(("bcmemacenet: init_emac\n"));

    pDevCtrl->emac = (volatile EmacRegisters * const)(pDevCtrl->dev->base_addr);
    emac = pDevCtrl->emac;

    if (pDevCtrl->EnetInfo.ucPhyType == BP_ENET_EXTERNAL_PHY) {
	/* enable external PHY (pinmux was already set up) */
	emac->config |= EMAC_EXT_PHY;
    }

    /* Initialize the Ethernet Switch MIB registers */
    clear_mib(pDevCtrl->emac);

    /* disable ethernet MAC while updating its registers */
    emac->config = EMAC_DISABLE ;
    while(emac->config & EMAC_DISABLE);

    /* issue soft reset, wait for it to complete */
    emac->config = EMAC_SOFT_RESET;
    while (emac->config & EMAC_SOFT_RESET);

    if (mii_init(pDevCtrl->dev))
        return -EFAULT;

    /* Initialize emac registers */
    /* Ethernet header optimization, reserve 2 bytes at head of packet */
    if (pDevCtrl->bIPHdrOptimize) {
        unsigned int packetOffset = (2 << RX_CONFIG_PKT_OFFSET_SHIFT);
        emac->rxControl = EMAC_FC_EN | EMAC_UNIFLOW | packetOffset;
    }
    else {
        emac->rxControl = EMAC_FC_EN | EMAC_UNIFLOW;  // THT dropped for RR support | EMAC_PROM;   // allow Promiscuous
    }

#ifdef MAC_LOOPBACK
    emac->rxControl |= EMAC_LOOPBACK;
#endif
    emac->rxMaxLength = ENET_MAX_MTU_SIZE+(pDevCtrl->bIPHdrOptimize ? 2: 0);
    emac->txMaxLength = ENET_MAX_MTU_SIZE;

    /* tx threshold = abs(63-(0.63*EMAC_DMA_MAX_BURST_LENGTH)) */
    emac->txThreshold = EMAC_TX_WATERMARK;  // THT PR12238 as per David Ferguson: Turning off RR, Was 32
    emac->mibControl  = EMAC_NO_CLEAR;
    emac->intMask = 0;              /* mask all EMAC interrupts*/

    TRACE(("done init emac\n"));

    return 0;

}

/*
 * init_dma: Initialize DMA control register
 */
static void init_IUdma(BcmEnet_devctrl *pDevCtrl)
{
    TRACE(("bcmemacenet: init_dma\n"));

    /*
     * initialize IUDMA controller register
     */
	/*
	 * Do not enable flow control (pause frames).
	 * If the system crashes and the controller is not reset,
	 * once the receive buffers are exhausted, 
	 * it will send a pause frame for every frame received and
	 * jam the network.
	 */
    //pDevCtrl->dmaRegs->controller_cfg = DMA_FLOWC_CH1_EN;
    pDevCtrl->dmaRegs->controller_cfg = 0;
    pDevCtrl->dmaRegs->flowctl_ch1_thresh_lo = DMA_FC_THRESH_LO;
    pDevCtrl->dmaRegs->flowctl_ch1_thresh_hi = DMA_FC_THRESH_HI;
#ifdef CONFIG_MIPS_BCM7405A0
    /* connect emac0->internal PHY (HW bug workaround) */
    pDevCtrl->dmaRegs->enet_iudma_tstctl |= (1 << 13);
#elif defined(BRCM_EMAC_1_SUPPORTED)
    /* connect emac0->internal PHY, emac1->external MII pins */
    pDevCtrl->dmaRegs->enet_iudma_tstctl &= ~(1 << 13);
#endif
    // transmit
    pDevCtrl->txDma->cfg = 0;       /* initialize first (will enable later) */
    pDevCtrl->txDma->maxBurst = DMA_MAX_BURST_LENGTH; /*DMA_MAX_BURST_LENGTH;*/

    pDevCtrl->txDma->intMask = 0;   /* mask all ints */
    /* clr any pending interrupts on channel */
    pDevCtrl->txDma->intStat = DMA_DONE|DMA_NO_DESC|DMA_BUFF_DONE;
    /* set to interrupt on packet complete */
    pDevCtrl->txDma->intMask = 0;

	pDevCtrl->txDma->descPtr = (uint32)CPHYSADDR(pDevCtrl->txFirstBdPtr);

    // receive
    pDevCtrl->rxDma->cfg = 0;  // initialize first (will enable later)
    pDevCtrl->rxDma->maxBurst = DMA_MAX_BURST_LENGTH; /*DMA_MAX_BURST_LENGTH;*/

    pDevCtrl->rxDma->descPtr = (uint32)CPHYSADDR(pDevCtrl->rxFirstBdPtr);
    pDevCtrl->rxDma->intMask = 0;   /* mask all ints */
    /* clr any pending interrupts on channel */
    pDevCtrl->rxDma->intStat = DMA_DONE|DMA_NO_DESC|DMA_BUFF_DONE;
    /* set to interrupt on packet complete and no descriptor available */
    pDevCtrl->rxDma->intMask = DMA_DONE; //|DMA_NO_DESC;
}

/*
 *  init_buffers: initialize driver's pools of receive buffers
 *  and tranmit headers
 */
static int init_buffers(BcmEnet_devctrl *pDevCtrl)
{
    struct sk_buff *skb;
    int bdfilled;
    int i;

    /* set initial state of all BD pointers to top of BD ring */
    pDevCtrl->txCbPtrHead = pDevCtrl->txCbPtrTail = NULL;

    /* allocate recieve buffer pool */
    for (i = 0; i < MAX_RX_BUFS; i++) {
        /* allocate a new SKB for the ring */
        /* 4 bytes for skb pointer, Rx_ALIGN for alignment, plus 2 bytes for IP header optimization */
        skb = dev_alloc_skb(pDevCtrl->rxBufLen + 4 + Rx_ALIGN );
        if (skb == NULL)
        {
            printk(KERN_NOTICE CARDNAME": Low memory.\n");
            break;
        }
        /* save skb pointer */
        pDevCtrl->skb_pool[i] = skb;
    }

    if (i < MAX_RX_BUFS) {
        /* release allocated receive buffer memory */
        for (i = 0; i < MAX_RX_BUFS; i++) {
            if (pDevCtrl->skb_pool[i] != NULL) {
                dev_kfree_skb (pDevCtrl->skb_pool[i]);
                pDevCtrl->skb_pool[i] = NULL;
            }
        }
        return -ENOMEM;
    }
    /* init the next free skb index */
    pDevCtrl->nextskb = 0;
    atomic_set(&pDevCtrl->rxDmaRefill, 0);
    clear_bit(0, &pDevCtrl->rxbuf_assign_busy);

    /* assign packet buffers to all available Dma descriptors */
    bdfilled = assign_rx_buffers(pDevCtrl);
    if (bdfilled > 0) {
        //printk("init_buffers: %d descriptors initialized\n", bdfilled);
    }
    // This avoid depending on flowclt_alloc which may go negative during init
    pDevCtrl->dmaRegs->flowctl_ch1_alloc = IUDMA_CH1_FLOW_ALLOC_FORCE | bdfilled;
    //printk("init_buffers: %08lx descriptors initialized, from flowctl\n",
    //	pDevCtrl->dmaRegs->flowctl_ch1_alloc);

    return 0;
}

/*
 * bcmemac_init_dev: initialize Ethernet Switch device,
 * allocate Tx/Rx buffer descriptors pool, Tx control block pool.
 */
static int bcmemac_init_dev(BcmEnet_devctrl *pDevCtrl)
{
    int i;
    int nrCbs;
    void *p;
    Enet_Tx_CB *txCbPtr;

#ifdef BCM_LINUX_CPU_ENET_1_IRQ
    pDevCtrl->rxIrq = pDevCtrl->devnum ? BCM_LINUX_CPU_ENET_1_IRQ : BCM_LINUX_CPU_ENET_IRQ;
#else
    pDevCtrl->rxIrq = BCM_LINUX_CPU_ENET_IRQ;
#endif
    /* setup buffer/pointer relationships here */
    pDevCtrl->nrTxBds = NR_TX_BDS;
    pDevCtrl->nrRxBds = NR_RX_BDS;
    pDevCtrl->rxBufLen = ENET_MAX_MTU_SIZE + (pDevCtrl->bIPHdrOptimize ? 2: 0);

    /* init rx/tx dma channels */
    pDevCtrl->dmaRegs = (DmaRegs *)(pDevCtrl->dev->base_addr + EMAC_DMA_OFFSET);
    pDevCtrl->rxDma = &pDevCtrl->dmaRegs->chcfg[EMAC_RX_CHAN];
    pDevCtrl->txDma = &pDevCtrl->dmaRegs->chcfg[EMAC_TX_CHAN];
    pDevCtrl->rxBds = (DmaDesc *) (pDevCtrl->dev->base_addr + EMAC_RX_DESC_OFFSET);
    pDevCtrl->txBds = (DmaDesc *) (pDevCtrl->dev->base_addr + EMAC_TX_DESC_OFFSET);
	
    /* alloc space for the tx control block pool */
    nrCbs = pDevCtrl->nrTxBds; 
    if (!(p = kmalloc(nrCbs*sizeof(Enet_Tx_CB), GFP_KERNEL))) {
        return -ENOMEM;
    }
    memset(p, 0, nrCbs*sizeof(Enet_Tx_CB));
    pDevCtrl->txCbs = (Enet_Tx_CB *)p;/* tx control block pool */

    /* initialize rx ring pointer variables. */
    pDevCtrl->rxBdAssignPtr = pDevCtrl->rxBdReadPtr =
                pDevCtrl->rxFirstBdPtr = pDevCtrl->rxBds;
    pDevCtrl->rxLastBdPtr = pDevCtrl->rxFirstBdPtr + pDevCtrl->nrRxBds - 1;

    /* init the receive buffer descriptor ring */
    for (i = 0; i < pDevCtrl->nrRxBds; i++)
    {
        (pDevCtrl->rxFirstBdPtr + i)->length_status = EMAC_SWAP32((pDevCtrl->rxBufLen<<16));
        (pDevCtrl->rxFirstBdPtr + i)->address = EMAC_SWAP32(0);
    }
    pDevCtrl->rxLastBdPtr->length_status |= EMAC_SWAP32(DMA_WRAP);

    /* init transmit buffer descriptor variables */
    pDevCtrl->txNextBdPtr = pDevCtrl->txFirstBdPtr = pDevCtrl->txBds;
    pDevCtrl->txLastBdPtr = pDevCtrl->txFirstBdPtr + pDevCtrl->nrTxBds - 1;

    /* clear the transmit buffer descriptors */
    for (i = 0; i < pDevCtrl->nrTxBds; i++)
    {
        (pDevCtrl->txFirstBdPtr + i)->length_status = EMAC_SWAP32((0<<16));
        (pDevCtrl->txFirstBdPtr + i)->address = EMAC_SWAP32(0);
    }
    pDevCtrl->txLastBdPtr->length_status |= EMAC_SWAP32(DMA_WRAP);
    pDevCtrl->txFreeBds = pDevCtrl->nrTxBds;

    /* initialize the receive buffers and transmit headers */
    if (init_buffers(pDevCtrl)) {
        kfree((void *)pDevCtrl->txCbs);
        return -ENOMEM;
    }

    for (i = 0; i < nrCbs; i++)
    {
        txCbPtr = pDevCtrl->txCbs + i;
        txCb_enq(pDevCtrl, txCbPtr);
    }

    /* init dma registers */
    init_IUdma(pDevCtrl);

    /* init switch control registers */
    if (init_emac(pDevCtrl)) {
        kfree((void *)pDevCtrl->txCbs);
        return -EFAULT;
    }

#ifdef IUDMA_INIT_WORKAROUND
    {
        unsigned long diag_rdbk;

        pDevCtrl->dmaRegs->enet_iudma_diag_ctl = 0x100; /* enable to read diags. */
        diag_rdbk = pDevCtrl->dmaRegs->enet_iudma_diag_rdbk;

        pDevCtrl->rxBdAssignPtr = pDevCtrl->rxBdReadPtr = pDevCtrl->rxFirstBdPtr + ((diag_rdbk>>1)&DESC_MASK);
        pDevCtrl->txNextBdPtr = pDevCtrl->txFirstBdPtr + ((diag_rdbk>>(16+1))&DESC_MASK);
    }
#endif	

    /* if we reach this point, we've init'ed successfully */
    return 0;
}

#ifdef USE_PROC

#define PROC_DUMP_DMADESC_STATUS    0
#define PROC_DUMP_EMAC_REGISTERS    1

#define PROC_DUMP_TXBD_1of6         2
#define PROC_DUMP_TXBD_2of6         3
#define PROC_DUMP_TXBD_3of6         4
#define PROC_DUMP_TXBD_4of6         5
#define PROC_DUMP_TXBD_5of6         6
#define PROC_DUMP_TXBD_6of6         7

#define PROC_DUMP_RXBD_1of6         8
#define PROC_DUMP_RXBD_2of6         9
#define PROC_DUMP_RXBD_3of6         10
#define PROC_DUMP_RXBD_4of6         11
#define PROC_DUMP_RXBD_5of6         12
#define PROC_DUMP_RXBD_6of6         13

#define PROC_DUMP_SKB_1of6          14
#define PROC_DUMP_SKB_2of6          15
#define PROC_DUMP_SKB_3of6          16
#define PROC_DUMP_SKB_4of6          17
#define PROC_DUMP_SKB_5of6          18
#define PROC_DUMP_SKB_6of6          19


/*
 * bcmemac_net_dump - display EMAC information
 */
int bcmemac_net_dump(BcmEnet_devctrl *pDevCtrl, char *buf, int reqinfo)
{
    int len = 0;
#if 0
    int  i;
    struct sk_buff *skb;
    static int bufcnt;
#endif

    switch (reqinfo) {

#if 0

/*----------------------------- TXBD --------------------------------*/
    case PROC_DUMP_TXBD_1of6: /* tx DMA BD descriptor ring 1 of 6 */
        len += sprintf(&buf[len], "\ntx buffer descriptor ring status.\n");
        len += sprintf(&buf[len], "BD\tlocation\tlength\tstatus addr\n");
//	 len += sprintf(&buf[len], "First 1/6 of %d buffers\n", pDevCtrl->nrTxBds);
        for (i = 0; i < pDevCtrl->nrTxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->txBds[i],
                   (pDevCtrl->txBds[i].length_status>>16),
                   (pDevCtrl->txBds[i].length_status&0xffff),
                   (pDevCtrl->txBds[i].address));
        }
        break;

    case PROC_DUMP_TXBD_2of6: /* tx DMA BD descriptor ring, 2 of 6 */
        for (i = pDevCtrl->nrTxBds/6; i < pDevCtrl->nrTxBds/3; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->txBds[i],
                   (pDevCtrl->txBds[i].length_status>>16),
                   (pDevCtrl->txBds[i].length_status&0xffff),
                   (pDevCtrl->txBds[i].address));
        }
        break;

   case PROC_DUMP_TXBD_3of6: /* tx DMA BD descriptor ring, 3 of 6 */
        for (i = pDevCtrl->nrTxBds/3; i < pDevCtrl->nrTxBds/2; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->txBds[i],
                   (pDevCtrl->txBds[i].length_status>>16),
                   (pDevCtrl->txBds[i].length_status&0xffff),
                   (pDevCtrl->txBds[i].address));
        }
        break;

   case PROC_DUMP_TXBD_4of6: /* tx DMA BD descriptor ring, 4 of 6 */
        for (i = pDevCtrl->nrTxBds/2; i < 2*pDevCtrl->nrTxBds/3; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->txBds[i],
                   (pDevCtrl->txBds[i].length_status>>16),
                   (pDevCtrl->txBds[i].length_status&0xffff),
                   (pDevCtrl->txBds[i].address));
        }
        break;

  case PROC_DUMP_TXBD_5of6: /* tx DMA BD descriptor ring, 5 of 6 */
        for (i = 2*pDevCtrl->nrTxBds/3; i < 5*pDevCtrl->nrTxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->txBds[i],
                   (pDevCtrl->txBds[i].length_status>>16),
                   (pDevCtrl->txBds[i].length_status&0xffff),
                   (pDevCtrl->txBds[i].address));
        }
        break;

 case PROC_DUMP_TXBD_6of6: /* tx DMA BD descriptor ring, 5 of 6 */
        for (i = 5*pDevCtrl->nrTxBds/6; i < pDevCtrl->nrTxBds; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->txBds[i],
                   (pDevCtrl->txBds[i].length_status>>16),
                   (pDevCtrl->txBds[i].length_status&0xffff),
                   (pDevCtrl->txBds[i].address));
        }
        break;

/*----------------------------- RXBD --------------------------------*/
    case PROC_DUMP_RXBD_1of6: /* rx DMA BD descriptor ring, 1 of 6 */
        len += sprintf(&buf[len], "\nrx buffer descriptor ring status.\n");
        len += sprintf(&buf[len], "BD\tlocation\tlength\tstatus\n");
        for (i = 0; i < pDevCtrl->nrRxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(unsigned int)&pDevCtrl->rxBds[i],
                   (pDevCtrl->rxBds[i].length_status>>16),
                   (pDevCtrl->rxBds[i].length_status&0xffff),
				   (pDevCtrl->rxBds[i].address));
        }
        break;

   case PROC_DUMP_RXBD_2of6: /* rx DMA BD descriptor ring, 2 of 6 */
        for (i = pDevCtrl->nrRxBds/6; i < 2*pDevCtrl->nrRxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(int)&pDevCtrl->rxBds[i],
                   (pDevCtrl->rxBds[i].length_status>>16),
                   (pDevCtrl->rxBds[i].length_status&0xffff),
				   (pDevCtrl->rxBds[i].address));
        }
		break;

   case PROC_DUMP_RXBD_3of6: /* rx DMA BD descriptor ring, 3 of 6 */
        for (i = 2*pDevCtrl->nrRxBds/6; i < 3*pDevCtrl->nrRxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(int)&pDevCtrl->rxBds[i],
                   (pDevCtrl->rxBds[i].length_status>>16),
                   (pDevCtrl->rxBds[i].length_status&0xffff),
				   (pDevCtrl->rxBds[i].address));
        }
		break;

  case PROC_DUMP_RXBD_4of6: /* rx DMA BD descriptor ring, 4 of 6 */
        for (i = 3*pDevCtrl->nrRxBds/6; i < 4*pDevCtrl->nrRxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(int)&pDevCtrl->rxBds[i],
                   (pDevCtrl->rxBds[i].length_status>>16),
                   (pDevCtrl->rxBds[i].length_status&0xffff),
				   (pDevCtrl->rxBds[i].address));
        }
		break;

  case PROC_DUMP_RXBD_5of6: /* rx DMA BD descriptor ring, 5 of 6 */
        for (i = 4*pDevCtrl->nrRxBds/6; i < 5*pDevCtrl->nrRxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(int)&pDevCtrl->rxBds[i],
                   (pDevCtrl->rxBds[i].length_status>>16),
                   (pDevCtrl->rxBds[i].length_status&0xffff),
				   (pDevCtrl->rxBds[i].address));
        }
		break;

  case PROC_DUMP_RXBD_6of6: /* rx DMA BD descriptor ring, 5 of 6 */
        for (i = 5*pDevCtrl->nrRxBds/6; i < 6*pDevCtrl->nrRxBds/6; i++)
        {
            len += sprintf(&buf[len], "%03d\t%08x\t%04d\t%04x %08lx\n",
                   i,(int)&pDevCtrl->rxBds[i],
                   (pDevCtrl->rxBds[i].length_status>>16),
                   (pDevCtrl->rxBds[i].length_status&0xffff),
				   (pDevCtrl->rxBds[i].address));
        }
        break;
#endif

    case PROC_DUMP_DMADESC_STATUS:  /* DMA descriptors pointer and status */
        len += sprintf(&buf[len], "\nrx pointers:\n");
        len += sprintf(&buf[len], "DmaDesc *rxBds:\t\t%08x\n",(unsigned int)pDevCtrl->rxBds);
        len += sprintf(&buf[len], "DmaDesc *rxBdAssignPtr:\t%08x\n",(unsigned int)pDevCtrl->rxBdAssignPtr);
        len += sprintf(&buf[len], "DmaDesc *rxBdReadPtr:\t%08x\n",(unsigned int)pDevCtrl->rxBdReadPtr);
        len += sprintf(&buf[len], "DmaDesc *rxLastBdPtr:\t%08x\n",(unsigned int)pDevCtrl->rxLastBdPtr);
        len += sprintf(&buf[len], "DmaDesc *rxFirstBdPtr:\t%08x\n",(unsigned int)pDevCtrl->rxFirstBdPtr);

        len += sprintf(&buf[len], "\ntx pointers:\n");
        len += sprintf(&buf[len], "DmaDesc *txBds:\t\t%08x\n",(unsigned int)pDevCtrl->txBds);
        len += sprintf(&buf[len], "DmaDesc *txLastBdPtr:\t%08x\n",(unsigned int)pDevCtrl->txLastBdPtr);
        len += sprintf(&buf[len], "DmaDesc *txFirstBdPtr:\t%08x\n",(unsigned int)pDevCtrl->txFirstBdPtr);
        len += sprintf(&buf[len], "DmaDesc *txNextBdPtr:\t%08x\n",(unsigned int)pDevCtrl->txNextBdPtr);
        len += sprintf(&buf[len], "Enet_Tx_CB *txCbPtrHead:%08x\n",(unsigned int)pDevCtrl->txCbPtrHead);
        if (pDevCtrl->txCbPtrHead)
            len += sprintf(&buf[len], "txCbPtrHead->lastBdAddr:\t%08x\n",(unsigned int)pDevCtrl->txCbPtrHead->lastBdAddr);
        len += sprintf(&buf[len], "EnetTxCB *txCbPtrTail:\t%08x\n",(unsigned int)pDevCtrl->txCbPtrTail);
        if (pDevCtrl->txCbPtrTail)
            len += sprintf(&buf[len], "txCbPtrTail->lastBdAddr:\t%08x\n",(unsigned int)pDevCtrl->txCbPtrTail->lastBdAddr);
        len += sprintf(&buf[len], "txFreeBds (TxBDs %d):\t%d\n", NR_TX_BDS, (unsigned int) pDevCtrl->txFreeBds);

        len += sprintf(&buf[len], "\ntx DMA Channel Config\t\t%08x\n", (unsigned int) pDevCtrl->txDma->cfg);
        len += sprintf(&buf[len], "tx DMA Intr Control/Status\t%08x\n", (unsigned int) pDevCtrl->txDma->intStat);
        len += sprintf(&buf[len], "tx DMA Intr Mask\t\t%08x\n", (unsigned int) pDevCtrl->txDma->intMask);
        //len += sprintf(&buf[len], "tx DMA Ring Offset\t\t%d\n", (unsigned int) pDevCtrl->txDma->unused[0]);

        len += sprintf(&buf[len], "\nrx DMA Channel Config\t\t%08x\n", (unsigned int) pDevCtrl->rxDma->cfg);
        len += sprintf(&buf[len], "rx DMA Intr Control/Status\t%08x\n", (unsigned int) pDevCtrl->rxDma->intStat);
        len += sprintf(&buf[len], "rx DMA Intr Mask\t\t%08x\n", (unsigned int) pDevCtrl->rxDma->intMask);
        //len += sprintf(&buf[len], "rx DMA Ring Offset\t\t%d\n", (unsigned int) pDevCtrl->rxDma->unused[0]);
        len += sprintf(&buf[len], "rx DMA controller_cfg\t\t%08x\n", (unsigned int) pDevCtrl->dmaRegs->controller_cfg);
        len += sprintf(&buf[len], "rx DMA Threshhold Lo\t\t%d\n", (unsigned int) pDevCtrl->dmaRegs->flowctl_ch1_thresh_lo);
        len += sprintf(&buf[len], "rx DMA Threshhold Hi\t\t%d\n", (unsigned int) pDevCtrl->dmaRegs->flowctl_ch1_thresh_hi);
        len += sprintf(&buf[len], "rx DMA Buffer Alloc\t\t%d\n", (unsigned int) pDevCtrl->dmaRegs->flowctl_ch1_alloc);
        break;

#if 0
/*----------------------------- SKBs --------------------------------*/
    case PROC_DUMP_SKB_1of6: /* skb buffer usage, 1 of 6 */
        bufcnt = 0;
        len += sprintf(&buf[len], "\nskb\taddress\t\tuser\tdataref\n");
        for (i = 0; i < MAX_RX_BUFS/6; i++) {
            skb = pDevCtrl->skb_pool[i];
	     if (skb) {
                len += sprintf(&buf[len], "%d\t%08x\t%d\t%d\n",
                        i, (int)skb, atomic_read(&skb->users), atomic_read(skb_dataref(skb)));
            	  if ((atomic_read(&skb->users) == 2) && (atomic_read(skb_dataref(skb)) == 1))
                    bufcnt++;
	     	}
        }
        //len += sprintf(&buf[len], "\nnumber rx buffer available %d\n", bufcnt);
        break;

    case PROC_DUMP_SKB_2of6: /* skb buffer usage, part 2 of 6 */
        len += sprintf(&buf[len], "\nskb\taddress\t\tuser\tdataref\n");
        for (i = MAX_RX_BUFS/6; i < 2*MAX_RX_BUFS/6; i++) {
            skb = pDevCtrl->skb_pool[i];
            len += sprintf(&buf[len], "%d\t%08x\t%d\t%d\n",
                        i, (int)skb, atomic_read(&skb->users), atomic_read(skb_dataref(skb)));
            if ((atomic_read(&skb->users) == 2) && (atomic_read(skb_dataref(skb)) == 1))
                bufcnt++;
        }
        //len += sprintf(&buf[len], "\nnumber rx buffer available %d\n", bufcnt);
        break;

    case PROC_DUMP_SKB_3of6: /* skb buffer usage, part 3 of 6 */
        len += sprintf(&buf[len], "\nskb\taddress\t\tuser\tdataref\n");
        for (i = MAX_RX_BUFS/3; i < MAX_RX_BUFS/2; i++) {
            skb = pDevCtrl->skb_pool[i];
            len += sprintf(&buf[len], "%d\t%08x\t%d\t%d\n",
                        i, (int)skb, atomic_read(&skb->users), atomic_read(skb_dataref(skb)));
            if ((atomic_read(&skb->users) == 2) && (atomic_read(skb_dataref(skb)) == 1))
                bufcnt++;
        }
        //len += sprintf(&buf[len], "\nnumber rx buffer available %d\n", bufcnt);
        break;

    case PROC_DUMP_SKB_4of6: /* skb buffer usage, part 4 of 4 */
        len += sprintf(&buf[len], "\nskb\taddress\t\tuser\tdataref\n");
        for (i = MAX_RX_BUFS/2; i < 2*MAX_RX_BUFS/3; i++) {
            skb = pDevCtrl->skb_pool[i];
            len += sprintf(&buf[len], "%d\t%08x\t%d\t%d\n",
                        i, (int)skb, atomic_read(&skb->users), atomic_read(skb_dataref(skb)));
            if ((atomic_read(&skb->users) == 2) && (atomic_read(skb_dataref(skb)) == 1))
                bufcnt++;
        }
        //len += sprintf(&buf[len], "\nnumber rx buffer available %d\n", bufcnt);
        break;
		
    case PROC_DUMP_SKB_5of6: /* skb buffer usage, part 4 of 4 */
        len += sprintf(&buf[len], "\nskb\taddress\t\tuser\tdataref\n");
        for (i = 2*MAX_RX_BUFS/3; i < 5*MAX_RX_BUFS/6; i++) {
            skb = pDevCtrl->skb_pool[i];
            len += sprintf(&buf[len], "%d\t%08x\t%d\t%d\n",
                        i, (int)skb, atomic_read(&skb->users), atomic_read(skb_dataref(skb)));
            if ((atomic_read(&skb->users) == 2) && (atomic_read(skb_dataref(skb)) == 1))
                bufcnt++;
        }
        //len += sprintf(&buf[len], "\nnumber rx buffer available %d\n", bufcnt);
        break;
		
    case PROC_DUMP_SKB_6of6: /* skb buffer usage, part 4 of 4 */
        len += sprintf(&buf[len], "\nskb\taddress\t\tuser\tdataref\n");
        for (i = 5*MAX_RX_BUFS/6; i < MAX_RX_BUFS; i++) {
            skb = pDevCtrl->skb_pool[i];
            len += sprintf(&buf[len], "%d\t%08x\t%d\t%d\n",
                        i, (int)skb, atomic_read(&skb->users), atomic_read(skb_dataref(skb)));
            if ((atomic_read(&skb->users) == 2) && (atomic_read(skb_dataref(skb)) == 1))
                bufcnt++;
        }
        len += sprintf(&buf[len], "\nnumber rx buffer available %d\n", bufcnt);
	    bufcnt = 0; /* Reset */
        break;
#endif

    case PROC_DUMP_EMAC_REGISTERS: /* EMAC registers */
        len += sprintf(&buf[len], "\nEMAC registers\n");
        len += sprintf(&buf[len], "rx pDevCtrl->emac->rxControl\t\t%08x\n", (unsigned int) pDevCtrl->emac->rxControl);
        len += sprintf(&buf[len], "rx config register\t0x%08x\n", (int)pDevCtrl->emac->rxControl);
        len += sprintf(&buf[len], "rx max length register\t0x%08x\n", (int)pDevCtrl->emac->rxMaxLength);
        len += sprintf(&buf[len], "tx max length register\t0x%08x\n", (int)pDevCtrl->emac->txMaxLength);
        len += sprintf(&buf[len], "interrupt mask register\t0x%08x\n", (int)pDevCtrl->emac->intMask);
        len += sprintf(&buf[len], "interrupt register\t0x%08x\n", (int)pDevCtrl->emac->intStatus);
        len += sprintf(&buf[len], "control register\t0x%08x\n", (int)pDevCtrl->emac->config);
        len += sprintf(&buf[len], "tx control register\t0x%08x\n", (int)pDevCtrl->emac->txControl);
        len += sprintf(&buf[len], "tx watermark register\t0x%08x\n", (int)pDevCtrl->emac->txThreshold);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 0, (int)pDevCtrl->emac->pm0DataHi, (int)pDevCtrl->emac->pm0DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 1, (int)pDevCtrl->emac->pm1DataHi, (int)pDevCtrl->emac->pm1DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 2, (int)pDevCtrl->emac->pm2DataHi, (int)pDevCtrl->emac->pm2DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 3, (int)pDevCtrl->emac->pm3DataHi, (int)pDevCtrl->emac->pm3DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 4, (int)pDevCtrl->emac->pm4DataHi, (int)pDevCtrl->emac->pm4DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 5, (int)pDevCtrl->emac->pm5DataHi, (int)pDevCtrl->emac->pm5DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 6, (int)pDevCtrl->emac->pm6DataHi, (int)pDevCtrl->emac->pm6DataLo);
        len += sprintf(&buf[len], "pm%d\t0x%08x 0x%08x\n", 7, (int)pDevCtrl->emac->pm7DataHi, (int)pDevCtrl->emac->pm7DataLo);
        break;

    default:
        break;
    }

    return len;
}

static int dev_proc_engine(void *data, loff_t pos, char *buf)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)data;
    int len = 0;

    if (pDevCtrl == NULL)
        return len;

    if (pos >= PROC_DUMP_DMADESC_STATUS && pos <= PROC_DUMP_SKB_6of6) {
        len = bcmemac_net_dump(pDevCtrl, buf, pos);
    }
    return len;
}

/*
 *  read proc interface
 */
static ssize_t eth_proc_read(struct file *file, char *buf, size_t count,
        loff_t *pos)
{
    const struct inode *inode = file->f_dentry->d_inode;
    const struct proc_dir_entry *dp = PDE(inode);
    char *page;
    int len = 0, x, left;

    page = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!page)
        return -ENOMEM;
    left = PAGE_SIZE - 256;
    if (count < left)
        left = count;

    for (;;) {
        x = dev_proc_engine(dp->data, *pos, &page[len]);
        if (x == 0)
            break;
        if ((x + len) > left)
            x = -EINVAL;
        if (x < 0) {
            break;
        }
        len += x;
        left -= x;
        (*pos)++;
        if (left < 256)
            break;
    }
    if (len > 0 && copy_to_user(buf, page, len))
        len = -EFAULT;
    kfree(page);
    return len;
}

/*
 * /proc/driver/eth_rtinfo
 */
static struct file_operations eth_proc_operations = {
        read: eth_proc_read, /* read_proc */
};


#define BUFFER_LEN PAGE_SIZE


void bcmemac_dump(BcmEnet_devctrl *pDevCtrl)
{
    // We may have to do dynamic allocation here, since this is run from
    // somebody else's stack
    char buffer[BUFFER_LEN];
    int len, i;

    if (pDevCtrl == NULL) {
        return; // EMAC not initialized
    }

    /* First qtr of TX ring */
    printk("\ntx buffer descriptor ring status.\n");
    printk("BD\tlocation\tlength\tstatus addr\n");
    printk("%d TX buffers\n", pDevCtrl->nrTxBds);
    for (i = 0; i < pDevCtrl->nrTxBds; i++)
    {
        len += printk("%03d\t%08x\t%04ld\t%04lx %08lx\n",
                        i,(unsigned int)&pDevCtrl->txBds[i],
                        (pDevCtrl->txBds[i].length_status>>16),
                        (pDevCtrl->txBds[i].length_status&0xffff),
                        pDevCtrl->txBds[i].address);
    }

    printk("\nrx buffer descriptor ring status.\n");
    printk("BD\tlocation\tlength\tstatus\n");
    for (i = 0; i < pDevCtrl->nrRxBds; i++)
    {
        len += printk("%03d\t%08x\t%04ld\t%04lx %08lx\n",
                    i,(int)&pDevCtrl->rxBds[i],
                    (pDevCtrl->rxBds[i].length_status>>16),
                    (pDevCtrl->rxBds[i].length_status&0xffff),
                    pDevCtrl->rxBds[i].address);
    }

    /* DMA descriptor pointers and status */
    printk("\n\n=========== DMA descriptrs pointers and status ===========\n");
    len = bcmemac_net_dump(pDevCtrl, buffer, PROC_DUMP_DMADESC_STATUS);
    if (len >= BUFFER_LEN) printk("************ bcmemac_dump: ERROR: Buffer too small, PROC_DUMP_DMADESC_STATUS need %d\n", len);
    buffer[len] = '\0';
    printk("%s\n\n", buffer);

    /* EMAC Registers */
    printk("\n\n=========== EMAC Registers  ===========\n");
    len = bcmemac_net_dump(pDevCtrl, buffer, PROC_DUMP_EMAC_REGISTERS);
    if (len >= BUFFER_LEN) printk("************ bcmemac_dump: ERROR: Buffer too small, PROC_DUMP_EMAC_REGISTERS need %d\n", len);
    buffer[len] = '\0';
    printk("%s\n\n", buffer);

    printk("Other Debug info: Loop Count=%d, #rx_errors=%lu, #resets=%d, #overflow=%d,#NO_DESC=%d\n", 
            loopCount,  pDevCtrl->stats.rx_errors, gNumResetsErrors, gNumOverflowErrors, gNumNoDescErrors);
    printk("Last dma flag=%08x, last error dma flag = %08x, devInUsed=%d, linkState=%d\n",
            gLastDmaFlag, gLastErrorDmaFlag, /*gTimerScheduled, */atomic_read(&pDevCtrl->devInUsed), pDevCtrl->linkState);
}

#endif

EXPORT_SYMBOL(bcmemac_dump);


static void bcmemac_getMacAddr(void)
{
	extern int gNumHwAddrs;
	extern unsigned char* gHwAddrs[];
	int i;
	
   	if (gNumHwAddrs >= 1) {
		for (i=0; i < 6; i++) {
			g_flash_eaddr[i] = (uint8) gHwAddrs[0][i];
		}

		printk(KERN_INFO
		    "bcmemac: MAC address %02X:%02X:%02X:%02X:%02X:%02X"
		    "fetched from bootloader\n",
			g_flash_eaddr[0],g_flash_eaddr[1],g_flash_eaddr[2],
			g_flash_eaddr[3],g_flash_eaddr[4],g_flash_eaddr[5]
			);
   	}
	else {
		printk(KERN_ERR "%s: No MAC addresses defined\n", __FUNCTION__);
	}

}

#if defined(CONFIG_BRCM_PM)

/*
 * Power management
 */

static int bcmemac_power_on(void *arg)
{
	int i;

	if(g_sleep_flag == BCMEMAC_AWAKE)
		return(-1);
	
	brcm_pm_enet_add();
	for(i = 0; i < g_num_devs; i++) {
		BcmEnet_devctrl *pDevCtrl = netdev_priv(g_devs[i]);
		enable_irq(pDevCtrl->rxIrq);
	}
	g_sleep_flag = BCMEMAC_AWAKE;
	return(0);
}

static int bcmemac_power_off(void *arg)
{
	BcmEnet_devctrl *pDevCtrl;
	int active = 0, i;

	if(g_sleep_flag == BCMEMAC_SLEEPING)
		return(-1);
	
	for(i = 0; i < g_num_devs; i++) {
		pDevCtrl = netdev_priv(g_devs[i]);
		active += (atomic_read(&pDevCtrl->devInUsed) > 0) ? 1 : 0;
	}
	if(active > 0) {
		printk(KERN_INFO CARDNAME
			": can't sleep with %d interface(s) active\n",
			active);
		return(-1);
	} else {
		g_sleep_flag = BCMEMAC_SLEEPING;
		for(i = 0; i < g_num_devs; i++) {
			pDevCtrl = netdev_priv(g_devs[i]);
			disable_irq(pDevCtrl->rxIrq);
		}
		brcm_pm_enet_remove();
	}
	return(0);
}

#endif /* CONFIG_BRCM_PM */

static void bcmemac_dev_setup(struct net_device *dev, int devnum, int phy_id)
{
    int ret;
    ETHERNET_MAC_INFO EnetInfo;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
#ifdef USE_PROC
    struct proc_dir_entry *p;
    char proc_name[32];
#endif

    if (pDevCtrl == NULL) {
        printk((KERN_ERR CARDNAME ": unable to allocate device context\n"));
        return;
    }

    /* initialize the context memory */
    memset(pDevCtrl, 0, sizeof(BcmEnet_devctrl));
    /* back pointer to our device */
    pDevCtrl->dev = dev;
    pDevCtrl->linkstatus_phyport = -1;
    pDevCtrl->devnum = devnum;
    dev->base_addr = devnum ? ENET_MAC_1_ADR_BASE : ENET_MAC_ADR_BASE;

    printk("Broadcom internal 7038 10/100 Ethernet Network Device\n");

    if( BpGetEthernetMacInfo( &EnetInfo) != BP_SUCCESS ) {
        printk(KERN_DEBUG CARDNAME" board id not set\n");
        return;
    }
    memcpy(&(pDevCtrl->EnetInfo), &EnetInfo, sizeof(ETHERNET_MAC_INFO));
    if(phy_id != BCMEMAC_NO_PHY_ID)
        pDevCtrl->EnetInfo.ucPhyAddress = phy_id;

	/* L.Sun, Removed EXTERN_SWITCH condition */
	pDevCtrl->bIPHdrOptimize = haveIPHdrOptimization();
	
    memcpy(dev->dev_addr, g_flash_eaddr, ETH_ALEN);
    /* create a unique address for each interface */
    dev->dev_addr[4] += devnum;

    if ((ret = bcmemac_init_dev(pDevCtrl))) {
        bcmemac_uninit_dev(pDevCtrl);
        return;
    }
#ifdef USE_PROC
    /* create a /proc entry for display driver runtime information */
    sprintf(proc_name, PROC_ENTRY_NAME);
    if ((p = create_proc_entry(proc_name, 0, NULL)) == NULL) {
        printk((KERN_ERR CARDNAME ": unable to create proc entry!\n"));
        bcmemac_uninit_dev(pDevCtrl);
        return;
    }
    p->proc_fops = &eth_proc_operations;
    p->data = pDevCtrl;

#endif



    spin_lock_init(&pDevCtrl->lock);
    atomic_set(&pDevCtrl->devInUsed, 0);
#ifdef USE_BH
    pDevCtrl->task.routine = bcmemac_rx;
    pDevCtrl->task.data = (void *)pDevCtrl;
#endif
    /*
     * Setup the timer
     */
    init_timer(&pDevCtrl->timer);
    pDevCtrl->timer.data = (unsigned long)pDevCtrl;
    pDevCtrl->timer.function = tx_reclaim_timer;

    init_timer(&pDevCtrl->rx_timer);
    pDevCtrl->rx_timer.data = (unsigned long)pDevCtrl;
    pDevCtrl->rx_timer.function = rx_poll_timer;

#ifdef DUMP_DATA
    printk(KERN_INFO CARDNAME ": CPO BRCMCONFIG: %08X\n", read_c0_diag(/*$22*/));
    printk(KERN_INFO CARDNAME ": CPO MIPSCONFIG: %08X\n", read_c0_config(/*$16*/));
    printk(KERN_INFO CARDNAME ": CPO MIPSSTATUS: %08X\n", read_c0_status(/*$12*/));
#endif	
    ether_setup(dev);
   	dev->irq                    = pDevCtrl->rxIrq;
   	dev->open                   = bcmemac_net_open;
   	dev->stop                   = bcmemac_net_close;
   	dev->hard_start_xmit        = bcmemac_net_xmit;
   	dev->tx_timeout             = bcmemac_net_timeout;
   	dev->watchdog_timeo         = 2*HZ;
   	dev->get_stats              = bcmemac_net_query;
	dev->set_mac_address        = bcm_set_mac_addr;
   	dev->set_multicast_list     = bcm_set_multicast_list;
	dev->do_ioctl               = &bcmemac_enet_ioctl;
	netif_napi_add(dev, &pDevCtrl->napi, bcmemac_enet_poll, NR_RX_BDS);

#ifdef CONFIG_NET_POLL_CONTROLLER
        dev->poll_controller        = &bcmemac_poll;
#endif
    // These are default settings
    write_mac_address(dev);

    // Let boot setup info overwrite default settings.
    netdev_boot_setup_check(dev);

    /* setup the rx irq */
    /* register the interrupt service handler */
    /* At this point dev is not initialized yet, so use dummy name */
    if (request_irq(pDevCtrl->rxIrq, bcmemac_net_isr, IRQF_SHARED, dev->name, pDevCtrl))
		return;

    bcmemac_enable_ints(pDevCtrl);

    TRACE(("bcmemacenet: bcmemac_net_probe dev 0x%x\n", (unsigned int)dev));

    //SET_MODULE_OWNER(dev);
}

static void bcmemac_null_setup(struct net_device *dev)
{
}

/*
 *      bcmemac_net_probe: - Probe Ethernet switch and allocate device
 */
static int __init bcmemac_net_probe(struct platform_device *pdev, int phy_id)
{
    int ret;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl;
    int devnum;

    devnum = pdev->id;

#if defined(BRCM_EMAC_1_SUPPORTED)
        /* on dual EMAC systems, EMAC_0 is internal and EMAC_1 is external */
        if (BpSetBoardId(devnum ? "EXT_PHY" : "INT_PHY") != BP_SUCCESS)
#else
        if (BpSetBoardId("INT_PHY") != BP_SUCCESS)
#endif
            return -ENODEV;

    /* NOTE: BcmEnet_devctrl struct is allocated with kzalloc */
    dev = alloc_netdev(sizeof(struct BcmEnet_devctrl), "eth%d", bcmemac_null_setup);

    if (dev == NULL) {
	printk(KERN_ERR CARDNAME": Unable to allocate net_device structure!\n");
	return -ENODEV;
    }
    bcmemac_dev_setup(dev, devnum, phy_id);

    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);

    if (0 != (ret = register_netdev(dev))) {
        bcmemac_uninit_dev(pDevCtrl);
        return ret;
    }

#ifdef CONFIG_BCMINTEMAC_NETLINK
 	INIT_WORK(&pDevCtrl->link_change_task, bcmemac_link_change_task);
        printk(KERN_INFO "init link_change_task\n");
#endif

    if(ret == 0) {
	platform_set_drvdata(pdev, dev);
        g_devs[devnum] = dev;
        g_num_devs++;
    }
    

    return ret;
}

/* get ethernet port's status; return nonzero for Link up, 0 for Link down */
static int bcmIsEnetUp(struct net_device *dev)
{
    static int sem = 0;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    int linkState = pDevCtrl->linkState;

    if( sem == 0 )
    {
        sem = 1;

        if( (pDevCtrl->EnetInfo.ucPhyType == BP_ENET_EXTERNAL_SWITCH ) || (pDevCtrl->EnetInfo.ucPhyType == BP_ENET_EXTERNAL_PHY))
        {
            /* Call non-blocking functions to read the link state. */
            if( pDevCtrl->linkstatus_phyport == -1 )
            {
                /* Start reading the link state of each switch port. */
                pDevCtrl->linkstatus_phyport = 0;
                mii_linkstatus_start(dev, pDevCtrl->EnetInfo.ucPhyAddress |
                    pDevCtrl->linkstatus_phyport);
            }
            else
            {
                int ls = 0;

                /* Check if the link state is done being read for a particular
                 * switch port.
                 */
                if( mii_linkstatus_check(dev, &ls) )
                {
                    if(IGNORE_LINK_STATUS(pDevCtrl->EnetInfo.ucPhyAddress))
                        ls = 1;
                    /* The link state for one switch port has been read.  Save
                     * it in a temporary holder.
                     */
                    pDevCtrl->linkstatus_holder |=
                        ls << pDevCtrl->linkstatus_phyport++;

                    if( pDevCtrl->linkstatus_phyport ==
                        pDevCtrl->EnetInfo.numSwitchPorts )
                    {
                        /* The link state for all switch ports has been read.
                         * Return the current link state.
                         */
                        linkState = pDevCtrl->linkstatus_holder;
                        pDevCtrl->linkstatus_holder = 0;
                        pDevCtrl->linkstatus_polltimer = 0;
                        pDevCtrl->linkstatus_phyport = -1;
                    }
                    else
                    {
                        /* Start to read the link state for the next switch
                         * port.
                         */
                        mii_linkstatus_start(dev,
                            pDevCtrl->EnetInfo.ucPhyAddress |
                            pDevCtrl->linkstatus_phyport);
                    }
                }
                else
                    if( pDevCtrl->linkstatus_polltimer > (3 * POLLCNT_1SEC) )
                    {
                        /* Timeout reading MII status. Reset link state check. */
                        pDevCtrl->linkstatus_holder = 0;
                        pDevCtrl->linkstatus_polltimer = 0;
                        pDevCtrl->linkstatus_phyport = -1;
                    }
            }
        }
        else
        {
            linkState = mii_linkstatus(dev, pDevCtrl->EnetInfo.ucPhyAddress);
            pDevCtrl->linkstatus_polltimer = 0;
        }

        sem = 0;
    }

    return linkState;
}

/*
 * Generic cleanup handling data allocated during init. Used when the
 * module is unloaded or if an error occurs during initialization
 */
static void bcmemac_init_cleanup(struct net_device *dev)
{
    TRACE(("%s: bcmemac_init_cleanup\n", dev->name));

#ifdef USE_PROC
    remove_proc_entry("driver/eth_rtinfo", NULL);
#endif
}

/* Uninitialize tx/rx buffer descriptor pools */
static int bcmemac_uninit_dev(BcmEnet_devctrl *pDevCtrl)
{
    Enet_Tx_CB *txCBPtr;
    char proc_name[32];
    int i;

    if (pDevCtrl) {
        /* disable DMA */
        pDevCtrl->txDma->cfg = 0;
        pDevCtrl->rxDma->cfg = 0;

        /* free the irq */
        if (pDevCtrl->rxIrq)
        {
            pDevCtrl->dmaRegs->enet_iudma_r5k_irq_msk &= ~0x2;
            disable_irq(pDevCtrl->rxIrq);
            free_irq(pDevCtrl->rxIrq, pDevCtrl);
        }

        /* free the skb in the txCbPtrHead */
        while (pDevCtrl->txCbPtrHead)  {
            pDevCtrl->txFreeBds += pDevCtrl->txCbPtrHead->nrBds;

            if(pDevCtrl->txCbPtrHead->skb)
                dev_kfree_skb (pDevCtrl->txCbPtrHead->skb);

            txCBPtr = pDevCtrl->txCbPtrHead;

            /* Advance the current reclaim pointer */
            pDevCtrl->txCbPtrHead = pDevCtrl->txCbPtrHead->next;

            /* Finally, return the transmit header to the free list */
            txCb_enq(pDevCtrl, txCBPtr);
        }

        bcmemac_init_cleanup(pDevCtrl->dev);
        /* release allocated receive buffer memory */
        for (i = 0; i < MAX_RX_BUFS; i++) {
            if (pDevCtrl->skb_pool[i] != NULL) {
				/* skb->users was increased by 1 in assign_rx_buffers,we force to 
				 * decrease it before free the skb, this prevent memory leaks!
				 */
				if(atomic_read(&(pDevCtrl->skb_pool[i]->users)) > 1)
				{
					atomic_dec(&(pDevCtrl->skb_pool[i]->users));
				}
                dev_kfree_skb (pDevCtrl->skb_pool[i]);
                pDevCtrl->skb_pool[i] = NULL;
            }
        }
        /* free the transmit buffer descriptor */
        if (pDevCtrl->txBds) {
            pDevCtrl->txBds = NULL;
        }
        /* free the receive buffer descriptor */
        if (pDevCtrl->rxBds) {
            pDevCtrl->rxBds = NULL;
        }
        /* free the transmit control block pool */
        if (pDevCtrl->txCbs) {
            kfree(pDevCtrl->txCbs);
            pDevCtrl->txCbs = NULL;
        }
#ifdef USE_PROC
        sprintf(proc_name, PROC_ENTRY_NAME);
        remove_proc_entry(proc_name, NULL);
#endif

        if (pDevCtrl->dev) {
            if (pDevCtrl->dev->reg_state != NETREG_UNINITIALIZED)
                unregister_netdev(pDevCtrl->dev);

			free_netdev(pDevCtrl->dev);
        }
    }
   
    return 0;
}

static int netdev_ethtool_ioctl(struct net_device *dev, void *useraddr)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    u32 ethcmd;
    

    ASSERT(pDevCtrl != NULL);   
        
    if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
        return -EFAULT;
    
    switch (ethcmd) {
    /* get driver-specific version/etc. info */
    case ETHTOOL_GDRVINFO: {
        struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};

        strncpy(info.driver, CARDNAME, sizeof(info.driver)-1);
        strncpy(info.version, VERSION, sizeof(info.version)-1);
        if (copy_to_user(useraddr, &info, sizeof(info)))
            return -EFAULT;
        return 0;
        }
    default:
        break;
    }
    
    return -EOPNOTSUPP;    
}   

static int bcmemac_enet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    struct mii_ioctl_data *mii;
    unsigned long flags;
    int val = 0;

    BCMEMAC_POWER_ON(dev);

    /* we can add sub-command in ifr_data if we need to in the future */
    switch (cmd)
    {
    case SIOCGMIIPHY:       /* Get address of MII PHY in use. */
        mii = (struct mii_ioctl_data *)&rq->ifr_data;
        mii->phy_id = pDevCtrl->EnetInfo.ucPhyAddress;

    case SIOCGMIIREG:       /* Read MII PHY register. */
        spin_lock_irqsave(&pDevCtrl->lock, flags);
        mii = (struct mii_ioctl_data *)&rq->ifr_data;
        mii->val_out = mii_read(dev, mii->phy_id & 0x1f, mii->reg_num & 0x1f);
        spin_unlock_irqrestore(&pDevCtrl->lock, flags);
        break;

    case SIOCSMIIREG:       /* Write MII PHY register. */
        spin_lock_irqsave(&pDevCtrl->lock, flags);
        mii = (struct mii_ioctl_data *)&rq->ifr_data;
        mii_write(dev, mii->phy_id & 0x1f, mii->reg_num & 0x1f, mii->val_in);
        spin_unlock_irqrestore(&pDevCtrl->lock, flags);
        break;

    case SIOCGLINKSTATE:
        rq->ifr_flags = pDevCtrl->linkState;
        break;

    case SIOCETHTOOL:
        return netdev_ethtool_ioctl(dev, (void *) rq->ifr_data);
    }

    return val;
}

static int __bcmemac_probe(struct platform_device *pdev)
{
    int status;
    static int once = 0;
    status = -ENODEV;

    if (once == 0) {
	bcmemac_getMacAddr();
	once = 1;
    }

    TRACE(("bcmemacenet: bcmemac_module_init\n"));
    if (pdev->id == 0)
	status = bcmemac_net_probe(pdev, BCMEMAC_NO_PHY_ID);

#if defined(CONFIG_BCMINTEMAC_7038_EXTMII) && \
    (defined(CONFIG_MIPS_BCM7405B0) || defined(CONFIG_MIPS_BCM7335))
    /* probe EMAC_1 (this might fail) */

    if (pdev->id  == 1) {
        int phy_id;

        init_pinmux();
        phy_id = mii_probe(ENET_MAC_1_ADR_BASE);

        if(phy_id != BCMEMAC_NO_PHY_ID) {
            printk("bcmemac: detected PHY at ID %d on EMAC_1\n", phy_id);
            status bcmemac_net_probe(pdev, phy_id);
        } else {
            printk("bcmemac: no PHY detected on EMAC_1, disabling\n");
        }
    }
#endif

    return status;
}


#if defined (CONFIG_MIPS_BCM_NDVD) && !defined(MODULE)

/* Make the ethernet init code asynchronous by creating a kernel thread
 * and cause it to yield during MII auto neg and thus allow other drivers
 * to initialize. This saves significant boot time (about a second).
 */

static int bcmemac_init_thread(void *arg)
{
	struct platform_device *pdev;

	pdev = (struct platform_device *)arg;

	__bcmemac_probe(pdev);
	do_exit(0);
	return 0;
}
	
static int __devinit bcmemac_probe(struct platform_device *pdev)
{
	struct task_struct *th = kthread_create(bcmemac_init_thread, pdev, "bcmemac init thread");

	if (IS_ERR(th)) {
		printk(KERN_ERR "ERROR in bcmemac Ethernet driver: Unable to start init thread\n");
		return -1;
	}
	wake_up_process(th);
	return 0;
}

#else
static int __devinit bcmemac_probe(struct platform_device *pdev)
{
	__bcmemac_probe(pdev);
	return 0;
}

#endif

static int __devexit bcmemac_remove(struct platform_device *pdev)
{
	struct net_device *net_dev;
	BcmEnet_devctrl *pDevCtrl;

	net_dev = platform_get_drvdata(pdev);

	pDevCtrl = netdev_priv(net_dev);
	bcmemac_uninit_dev(pDevCtrl);

#ifdef CONFIG_BRCM_PM
	brcm_pm_unregister_enet();
	brcm_pm_enet_remove();
#endif
	return(0);
}

#ifdef CONFIG_PM

/*
 * Wakeup On Lan
 */

typedef struct wol {
	unsigned long	overall_en;
	unsigned long	passwd_ls;
	unsigned long	passwd_ms;
	unsigned long	unused_0x0c;
	unsigned long	passwd_en;
} wol_t;

extern int power_wol;

static void
wol_enable(void)
{
	volatile wol_t *wol;
	
	wol = (volatile wol_t *) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_WOL_TOP_REG_START);

	wol->passwd_ls = 0;
	wol->passwd_ms = 0;
	wol->passwd_en = 0;
	wol->overall_en = BCHP_WOL_TOP_OVERALL_EN_Overall_Enable_MASK
			| BCHP_WOL_TOP_OVERALL_EN_Interrupt_Enable_MASK;

}

static void
wol_disable(void)
{
	volatile wol_t *wol;

	wol = (volatile wol_t *) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_WOL_TOP_REG_START);

	if (wol->overall_en & BCHP_WOL_TOP_OVERALL_EN_Interrupt_Status_MASK)
		printk(KERN_DEBUG "%s: got wakeup packet\n", __func__);

	wol->overall_en = BCHP_WOL_TOP_OVERALL_EN_Interrupt_Status_MASK;
	wol->overall_en = 0;
}
 

static int
bcmemac_suspend_late(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *net_dev;

	net_dev = platform_get_drvdata(pdev);

	printk(KERN_DEBUG "%s: enter\n", __func__);

	bcmemac_net_close(net_dev);

	if (power_wol)
		wol_enable();

	return 0;
}

static int
bcmemac_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return 0;
}

static int
bcmemac_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return 0;
}

static int
bcmemac_resume_early(struct platform_device *pdev)
{
	struct net_device *net_dev;

	net_dev = platform_get_drvdata(pdev);

	printk(KERN_DEBUG "%s: enter\n", __func__);

	if (power_wol)
		wol_disable();

	bcmemac_net_open(net_dev);

	return 0;
}

#endif


static char bcmemac_name[] = "bcm7038";

static struct platform_driver bcmemac_driver = {
	.driver = {
	    .name   = bcmemac_name,
	    .owner = THIS_MODULE,
	},
        .probe		= bcmemac_probe,
        .remove		= __devexit_p(bcmemac_remove),
#ifdef CONFIG_PM
	.suspend	= bcmemac_suspend,
	.suspend_late	= bcmemac_suspend_late,
	.resume_early	= bcmemac_resume_early,
	.resume		= bcmemac_resume,
#endif
};

static struct platform_device *bcmemac_device;

static int __init bcmemac_module_init(void)
{
	int error;

	error = platform_driver_register(&bcmemac_driver);

	if (error) {
	    printk(KERN_ERR "%s: %s platform_driver_probe failed %d\n",
			 __func__, bcmemac_name, error);
	    return error;
	}
	
	bcmemac_device = platform_device_alloc(bcmemac_name, 0);

	if (!bcmemac_device) {
	    printk(KERN_ERR" %s: %s: platform_device_alloc failed\n",
			__func__, bcmemac_name);
	    platform_driver_unregister(&bcmemac_driver);
	    return -ENOMEM;
	}

        error = platform_device_add(bcmemac_device);

        if (error) {
	    printk(KERN_ERR "%s: %s: platform_device_alloc failed\n",
			__func__, bcmemac_name);
	    platform_device_put(bcmemac_device);
	    bcmemac_device = NULL;
	    return error;
        }

#ifdef CONFIG_BRCM_PM
	brcm_pm_enet_add();
	brcm_pm_register_enet(bcmemac_power_off, bcmemac_power_on, NULL);
#endif
	return 0;
}



static void __exit bcmemac_module_exit(void)
{

	if (bcmemac_device) 
		platform_device_unregister(bcmemac_device);

	bcmemac_device = NULL;

	platform_driver_unregister(&bcmemac_driver);
}

module_init(bcmemac_module_init);
module_exit(bcmemac_module_exit);

EXPORT_SYMBOL(bcmemac_get_free_txdesc);
EXPORT_SYMBOL(bcmemac_xmit_multibuf);
EXPORT_SYMBOL(bcmemac_xmit_check);
