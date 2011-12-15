/*
 *
 *  drivers/mtd/brcmnand/bcm7xxx-nand.c
 *
    Copyright (c) 2005-2006 Broadcom Corporation                 
    
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    File: bcm7xxx-nand.c

    Description: 
    This is a device driver for the Broadcom NAND flash for bcm97xxx boards.
when	who what
-----	---	----
051011	tht	codings derived from OneNand generic.c implementation.
 */
 
#include <linux/platform_device.h>
#include <linux/proc_fs.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>

#include <linux/mtd/partitions.h>
#include "cfe_xiocb.h"


#include <asm/io.h>
//#include <asm/mach/flash.h>

#include <linux/mtd/brcmnand.h>
#include "brcmnand_priv.h"

#define DRIVER_NAME	"bcm7xxx-nand"

#define DRIVER_INFO "Broadcom STB NAND controller"


#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_3_0
/*
        There could be more than one device for any version of
        the nand controller version 1.0 and above, but we 
        have never done that, so both the info and the nPartitionQty
        have only one entry
*/
static int nPartitionQty[MAX_NAND_CS] = {0};
static struct brcmnand_info* info[MAX_NAND_CS] = {NULL};

#else //CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3

static int nPartitionQty[MAX_NAND_CS] = {0,0,0,0};
static struct brcmnand_info* info[MAX_NAND_CS] = {NULL,NULL,NULL,NULL};

#endif

static struct mtd_partition bcm7XXX_nandX_parts[] = 
{
    { name: "nand",	          offset: 0x0,				    size: 0x0        },	
//  { name: "bbt",  	      offset: 0x0ff00000,			size: 0x00100000 },
}; 
 
struct brcmnand_info {
    struct mtd_info		mtd;
    struct mtd_partition*	parts;
    struct brcmnand_chip	brcmnand;
};


struct platform_device* platformdev = NULL; 

/*
    This function is mainly used by brcmnand_get_device_exclusive(),
    which looks only for cs0, so it is okay for us to return the handle to 
    the 1st nand device only.
*/
void* get_brcmnand_handle(void)
{
    int i;
    void* handle = NULL;
    
    for (i = 0; i < MAX_NAND_CS; i++)
    {
        if (info[i] != NULL)
        {   //Return the 1st handle:
            handle = (void*)&info[i]->brcmnand;
            break;
        }
    }
    return handle;
}

/* 
 * Size and offset are variable, depending on the size of the chip, but 
 * cfe_kernel always starts at 1FC0_0000 and is 4MB size.
 * The entire reserved area (kernel + CFE + BBT) occupies the last 8 MB of the flash.
 */

static void __devinit 
brcmnand_setup_mtd_partitions(struct brcmnand_info* nandinfo, int cs)
{
    xiocb_boardinfo_t *boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;
    cfe_xuint_t member[3], nElements;
    u_int32_t partitions_total_size = 0;

    struct mtd_info* mtd;
    unsigned long size;
    char name[6]; //
    int i = 0;
    int j = 0;
    int bBbt = 0;   //Check for bad block table partition

    int nTmpPartitionQty = 0;

    unsigned long kFlashBase = 0;	
    
    mtd = &nandinfo->mtd;
    size = mtd->size; // mtd->size may be different than nandinfo->size
                                // Relies on this being called after brcmnand_scan

    printk(KERN_INFO "- Flash Type: %s\n", boardinfo->bi_flash_type == FLASH_XTYPE_NOR ? "NOR" : "NAND");

    if (cs == 0)
    {
        nElements     = boardinfo->bi_config_flash_numparts;
        nPartitionQty[cs] = (int)nElements;
        nTmpPartitionQty = nPartitionQty[cs];
        printk(KERN_INFO "- Number of Flash Partitions: %d\n", nPartitionQty[cs]);
        printk(KERN_INFO "nPartitionQty:%d, sizeof(struct mtd_partition): %d\n", nPartitionQty[cs], sizeof(struct mtd_partition));

        nandinfo->parts = kmalloc((nPartitionQty[cs] + 1 ) * sizeof(struct mtd_partition), GFP_KERNEL);
        memset(nandinfo->parts, 0, ((nPartitionQty[cs] + 1 ) * sizeof(struct mtd_partition)));
        if (!nandinfo->parts)
        {
            printk (KERN_ERR "Cannot allocate memory for nandinfo->parts\n");
            return;
        }
        printk (KERN_INFO "nandinfo->parts: %08X\n", (int)nandinfo->parts);
        for (i = 0; i < nPartitionQty[cs]; i++) {
            member[0] = boardinfo->bi_flash_partmap[i].base;
            member[1] = boardinfo->bi_flash_partmap[i].offset;
            member[2] = boardinfo->bi_flash_partmap[i].size;
            printk(KERN_INFO "-   Partition %d: Name: %s, base 0x%08x, offset 0x%08x, size 0x%08x\n",
                        i,
                        boardinfo->bi_flash_partmap[i].part_name,
                        (unsigned)member[0],
                        (unsigned)member[1],
                        (unsigned)member[2]);
        }

        kFlashBase = boardinfo->bi_flash_partmap[0].base;

        /*
        ** Overwrite the compiled-in defaults if we've obtained
        ** partition info straight from CFE.
        **
        ** CFE Magic number encodes the version...
        ** require CFE Version 1.6.x minimum
        */
        if (boardinfo->bi_flash_type == FLASH_XTYPE_NAND) 
        {
            printk(KERN_INFO "%s: Configure NAND flash partition map from CFE"
                             " - [%d] partitions reported]\n", 
                             __FUNCTION__, nPartitionQty[cs]);

            /*
            ** String manipulations:
            **  1. Strip leading "flash<x>." from CFE partition name
            */
            for (i = 0, j = 0; i < nTmpPartitionQty; i++, j++) 
            {

                char *partName;

                if (boardinfo->bi_flash_partmap[i].size == 0) {
                    /*
                    ** No zero-sized partitions allowed.
                    */
                    printk(KERN_WARNING "No zero-sized partitions allowed, partition %d\n", i);
                    nPartitionQty[cs] --;
                    j--;
                    continue; //Skip this partition
                }

                if (boardinfo->bi_flash_partmap[i].base != kFlashBase)
                {
                            member[0] = boardinfo->bi_flash_partmap[i].base + 
                                            boardinfo->bi_flash_partmap[i].offset -
                                                kFlashBase;
                }
                else
                {
                     member[0] = boardinfo->bi_flash_partmap[i].offset;
                }

                member[1] = boardinfo->bi_flash_partmap[i].size;
                
                partName = strstr(boardinfo->bi_flash_partmap[i].part_name, ".");

                if (partName == (char *)NULL)
                {
                    partName = &boardinfo->bi_flash_partmap[i].part_name[0];
                }
                else 
                {
                    partName += 1;

                    if  (strcmp(partName, "all") == 0)
                    {
                        printk(KERN_INFO "'all' partition size: %d\n", (int)member[1]);
                        partName = "all";
                    }
                    else if ( (strcmp(partName, "bbt") == 0) || (strcmp(partName, "kreserv") == 0))
                    {
                        printk(KERN_INFO "Hiding Bad Block Table Partition from /proc/mtd\n");
                        nPartitionQty[cs] --;
                        j--;
                        bBbt = 1;
                        continue; //Skip this partitionc
                    }
                }


                nandinfo->parts[j].name = kmalloc(strlen(partName)+1, GFP_KERNEL);
                strcpy(nandinfo->parts[j].name, partName);

                nandinfo->parts[j].offset = (u_int32_t)member[0];
                nandinfo->parts[j].size   = (u_int32_t)member[1];

                // For now every partition uses the same oobinfo
                nandinfo->parts[j].oobsel = &mtd->oobinfo;
                
                if  (strcmp(partName, "all") != 0)
                {
                    partitions_total_size += nandinfo->parts[j].size;
                }

            }

            if (!bBbt)
            {
                printk(KERN_ERR "!!!!Panic: no space to put Bad Block Table (BBT)!!!!\n");
            }

            printk(KERN_INFO "NAND flash partitions total size: %d\n", partitions_total_size);
        }
        else {
            printk(KERN_ERR "%s: FLASH TYPE is not NAND!!!!!\n", __FUNCTION__);
            return;
        }
    }
    else //for all other devices
    {
    //Add 'nand2' partition
        nPartitionQty[cs] = ARRAY_SIZE(bcm7XXX_nandX_parts);
        
        nandinfo->parts = bcm7XXX_nandX_parts;
        sprintf(name, "nand%d", cs);
        strcpy(nandinfo->parts[0].name, name);
        
        nandinfo->parts[0].size = size - (8 * nandinfo->mtd.erasesize);
        nandinfo->parts[0].offset = 0;
        nandinfo->parts[0].oobsel = &mtd->oobinfo;        

        printk(KERN_NOTICE "%s: Statically defined partition map - %d partitions for chip %d\n", __FUNCTION__, nPartitionQty[cs], cs);
        printk(KERN_INFO "Part[%d] name=%s, size=%x, offset=%x\n", i, 
            nandinfo->parts[0].name, nandinfo->parts[0].size, 
            nandinfo->parts[0].offset);
            
        bBbt = 1; //Bad Block Table has been taken care of (see above)

    }
}
static int __devinit brcmnand_probe(struct platform_device *pdev)
{
    int i;
    uint32_t flash_id;
    
    int err = 0;
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0    
    xiocb_boardinfo_t *boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;
#endif //#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0    

    for (i = 0; i < MAX_NAND_CS ;i++)
    {
        
        printk(KERN_INFO"%s: Calling brcmnand_prescan\n", __FUNCTION__);    

        flash_id = 0;
        if (brcmnand_prescan(i, &flash_id) == -1)
        {
            //this device does not exist, so free it and continue the for loop:
            printk(KERN_INFO"%s: Device [%d] does not exist\n", __FUNCTION__, i);
            info[i] = NULL;
            continue;  
        }
        
        //Device exists, so allocate space for its structure:
        info[i] = kmalloc(sizeof(struct brcmnand_info), GFP_KERNEL);
        
        if (!info[i])
            return -ENOMEM;

        memset(info[i], 0, sizeof(struct brcmnand_info));

        info[i]->brcmnand.numchips = 1; // We only support 1 chip per mtd
        info[i]->brcmnand.chip_shift = 0; // Only 1 chip
        info[i]->brcmnand.regs = pdev->resource[0].start;
        info[i]->brcmnand.priv = &info[i]->mtd;

        //info->brcmnand.mmcontrol = NULL;  // THT: Sync Burst Read TBD.  pdata->mmcontrol; 

        info[i]->mtd.name = pdev->dev.bus_id;
        info[i]->mtd.priv = &info[i]->brcmnand;
        info[i]->mtd.owner = THIS_MODULE;

        /* Enable the following for a flash based bad block table */
        info[i]->brcmnand.options |= NAND_USE_FLASH_BBT;

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0
        if (i == 0)
        {

            /*Verify if the 1st partition is cfe.  If it is, then set xor_invert_val
            to 0x1FC00000, else to 0x0 */
//            info[i]->brcmnand.xor_invert_val = 0x1FC00000;

            if ( ((strcmp(boardinfo->bi_flash_partmap[0].part_name, "flash0.cfe") == 0) ||
                  (strcmp(boardinfo->bi_flash_partmap[0].part_name, "flash0.ucfe") == 0))&&
                 (boardinfo->bi_flash_partmap[0].base == 0) &&
                 (boardinfo->bi_flash_partmap[0].offset == 0) )
            {
                info[i]->brcmnand.xor_invert_val = 0x1FC00000;
            }
            else
            {
               info[i]->brcmnand.xor_invert_val = 0x0;
            }
        }
        else
        {
            info[i]->brcmnand.xor_invert_val = 0x0;
        }
#else //CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0        
        info[i]->brcmnand.xor_invert_val = 0x0;
#endif//CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0                

                
        printk(KERN_INFO"%s: Calling brcmnand_scan\n", __FUNCTION__);    
        if (brcmnand_scan(&info[i]->mtd, i)) 
        {
            err = -ENXIO;
            goto out_free_info;
        }

        //printk("	brcmnand_setup_mtd_partitions\n");
        brcmnand_setup_mtd_partitions(info[i],i);
    
        //add_mtd_partitions calls add_mtd_device for each partition, which are slaves of the &info[i]->mtd
        add_mtd_partitions(&info[i]->mtd, info[i]->parts, nPartitionQty[i]);

        //printk("	dev_set_drvdata\n");	
        dev_set_drvdata(&pdev->dev, &info);
        //printk("<-- brcmnand_probe\n");
    }

    return 0;

out_free_info:
    for (i = 0; i < MAX_NAND_CS; i++)
    {
        if (info[i])
        {
            kfree(info[i]);
            info[i] = NULL;
        }
    }

    return err;
}

static int __devexit brcmnand_remove(struct platform_device *pdev)
{
    int i;
    
    struct brcmnand_info **info = dev_get_drvdata(&pdev->dev);
    
    printk(KERN_DEBUG"%s: pdev = 0x%p, info = 0x%p\n", __FUNCTION__, pdev, info);
    if (info) 
    {
        for (i = 0; i < MAX_NAND_CS; i++)
        {
            printk(KERN_DEBUG"%s: info[%d] = 0x%p\n", __FUNCTION__, i, info[i]);
            if (info[i])
            {
                if (info[i]->parts)
                {
                    printk(KERN_DEBUG"%s: info[%d]: deleting partitions\n", __FUNCTION__, i);
                    del_mtd_partitions(&info[i]->mtd);
                }
                printk(KERN_DEBUG"%s: info[%d]: Calling brcmnand_release\n", __FUNCTION__, i);
                brcmnand_release(&info[i]->mtd);
                printk(KERN_DEBUG"%s: freeing info[%d]\n", __FUNCTION__, i);
                kfree(info[i]);
                info[i] = NULL;
            }
        }
    }
    
    printk(KERN_DEBUG"%s: setting pdev-dev to NULL\n", __FUNCTION__);
    dev_set_drvdata(&pdev->dev, NULL);
    return 0;
}


#ifdef CONFIG_PM

static int
brcmnand_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return 0;
}

static int
brcmnand_suspend_late(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return 0;
}

static int
brcmnand_resume_early(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return 0;
}

static int
brcmnand_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return 0;
}

#endif

static char brcmnand_name[] = DRIVER_NAME;

static struct platform_driver brcmnand_driver = {
	.driver = {
	    .name   = brcmnand_name,
	    .owner = THIS_MODULE,
	},
        .probe		= brcmnand_probe,
        .remove		= __devexit_p(brcmnand_remove),
#ifdef CONFIG_PM
	.suspend	= brcmnand_suspend,
	.suspend_late	= brcmnand_suspend_late,
	.resume_early	= brcmnand_resume_early,
	.resume		= brcmnand_resume,
#endif
};

static struct platform_device *brcmnand_device;

static int __init brcmnand_init(void)
{
	int error;
	struct resource devRes[1];

	printk(KERN_DEBUG "%s enter\n", __func__);

	error = platform_driver_register(&brcmnand_driver);

	if (error)  {
	    printk(KERN_ERR "%s: %s platform_driver_probe failed %d\n",
			 __func__, brcmnand_name, error);
	    return error;
	}
	
	brcmnand_device = platform_device_alloc(brcmnand_name, -1);

	if (!brcmnand_device) {
	    printk(KERN_ERR" %s: %s: platform_device_alloc failed\n",
			__func__, brcmnand_name);
	    platform_driver_unregister(&brcmnand_driver);
	    return -ENOMEM;
	}

	memset(devRes, 0, sizeof(devRes));
	devRes[0].name = "brcmnand-base";
	devRes[0].start = BRCMNAND_CTRL_REGS;
	devRes[0].end = BRCMNAND_CTRL_REGS_END;
	devRes[0].flags = IORESOURCE_MEM;

	error = platform_device_add_resources(brcmnand_device, devRes, ARRAY_SIZE(devRes));

	if (error) {
	    printk(KERN_ERR" %s: %s: platform_add_resources failed\n",
			__func__, brcmnand_name);
	    platform_device_put(brcmnand_device);
	    brcmnand_device = NULL;
	    return error;
        }

        error = platform_device_add(brcmnand_device);

        if (error) {
	    printk(KERN_ERR" %s: %s: platform_device_alloc failed\n",
			__func__, brcmnand_name);
	    platform_device_put(brcmnand_device);
	    brcmnand_device = NULL;
	    return error;
        }

	return 0;
}

extern void edu_exit(void);

static void __exit brcmnand_exit(void)
{
	printk(KERN_DEBUG "%s enter\n", __func__);

	if (brcmnand_device) 
		platform_device_unregister(brcmnand_device);

	brcmnand_device = NULL;

	platform_driver_unregister(&brcmnand_driver);

	edu_exit();
	
}
module_init(brcmnand_init);
module_exit(brcmnand_exit);

MODULE_ALIAS(DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ton Truong <ttruong@broadcom.com>");
MODULE_DESCRIPTION("Broadcom NAND flash driver");

