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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/notifier.h>
#include "bcmemac.h"
#include <linux/stddef.h>
#include <asm/r4kcache.h>

static struct net_device_stats * vnet_query(struct net_device * dev);
void vnet_module_cleanup(void);
int vnet_probe(void);

extern int get_num_vports(void);
extern struct net_device* get_vnet_dev(int port);
extern struct net_device* set_vnet_dev(int port, struct net_device *dev);
static struct net_device* vnet_dev[MAX_NUM_OF_VPORTS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static struct net_device_stats *vnet_query(struct net_device * dev)
{
    return (struct net_device_stats *)netdev_priv(dev);
}

int vnet_probe(void)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    int status;
    int port;

    static int probed = 0;

    if (probed ++ != 0)
        return -ENXIO;

    vnet_dev[0] = get_vnet_dev(0);

    if (vnet_dev[0] == NULL)
        return -ENXIO;

    for (port = 1; port < get_num_vports(); port++)
    {
        dev = alloc_etherdev(sizeof(struct net_device_stats));
        if (dev == NULL)
            continue;

        memset(netdev_priv(dev), 0, sizeof(struct net_device_stats));

        dev_alloc_name(dev, dev->name);
        SET_MODULE_OWNER(dev);
        sprintf(dev->name, "%s.%d", vnet_dev[0]->name, port);
        dev->open                   = vnet_dev[0]->open;
        dev->stop                   = vnet_dev[0]->stop;
        dev->hard_start_xmit        = vnet_dev[0]->hard_start_xmit;
        dev->tx_timeout             = vnet_dev[0]->tx_timeout;
        dev->set_mac_address        = vnet_dev[0]->set_mac_address;
        dev->set_multicast_list     = vnet_dev[0]->set_multicast_list;
        dev->hard_header            = vnet_dev[0]->hard_header;
        dev->hard_header_len        = vnet_dev[0]->hard_header_len;
        dev->do_ioctl               = vnet_dev[0]->do_ioctl;
        dev->get_stats              = vnet_query;
        dev->base_addr              = port;
        dev->header_cache_update    = NULL;
        dev->hard_header_cache      = NULL;
        dev->hard_header_parse      = NULL;

        status = register_netdev(dev);

        if (status != 0)
        {
            unregister_netdev(dev);
            free_netdev(dev);
            return status;
        }

        vnet_dev[port] = dev;
        set_vnet_dev(port, dev);

        memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);
        memmove(sockaddr.sa_data, vnet_dev[0]->dev_addr, ETH_ALEN);
        dev->set_mac_address(dev, &sockaddr);

        printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
            );
    }

    return 0;
}

void vnet_module_cleanup(void)
{
    int port;

    synchronize_net();

    for (port = 1; port < get_num_vports(); port++)
    {
        set_vnet_dev(port, NULL);

        if (vnet_dev[port] != NULL)
        {
            unregister_netdev(vnet_dev[port]);
            free_netdev(vnet_dev[port]);
        }
    }
}


/* End of file */
