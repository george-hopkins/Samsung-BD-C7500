/*
 * Flash mapping for BCM7xxx boards
 *
 * Copyright (C) 2001 Broadcom Corporation
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
 * THT				10-23-2002
 * Steven J. Hill	09-25-2001
 * Mark Huang		09-15-2001
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <linux/init.h>
#include <../arch/mips/brcmstb/common/cfe_xiocb.h>
#include <linux/string.h>

extern int gFlashSize;

#ifdef CONFIG_BCM93730
#define WINDOW_ADDR 0x1e000000

#elif defined(CONFIG_MIPS_BRCM97XXX)
extern unsigned long getPhysFlashBase(void);

#define WINDOW_ADDR getPhysFlashBase()

#else
#error ("bcm9xxxx-flash.c: Unsupported architecture\n")
#endif

#ifdef CONFIG_MIPS_BCM7315_BBX
/* 
 * The 7315BBX only has a 4MB flash
 */
#define WINDOW_SIZE 0x00400000	/* 4 MB flash */
#else
/* 
 * All other 97XXX boards.  May have 2 flash chips, but we only use 1.
 * and since they are of different sizes, no interleaving.
 */
#define WINDOW_SIZE (0x20000000	 - WINDOW_ADDR)

#endif


#define BUSWIDTH 2

static struct mtd_info *bcm9XXXX_mtd;

#ifdef CONFIG_MTD_COMPLEX_MAPPINGS
extern int bcm7401Cx_rev;
extern int bcm7118Ax_rev;
extern int bcm7403Ax_rev;

static inline void bcm9XXXX_map_copy_from_16bytes(void *to, unsigned long from, ssize_t len)
{
	static DEFINE_SPINLOCK(bcm9XXXX_lock);
	unsigned long flags;
	
	spin_lock_irqsave(&bcm9XXXX_lock, flags);
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	memcpy_fromio(to, from, len);
	spin_unlock_irqrestore(&bcm9XXXX_lock, flags);
}

static map_word bcm9XXXX_map_read(struct map_info *map, unsigned long ofs)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 ||
	                            bcm7403Ax_rev == 0x20  )
	{	
		map_word r;
		static DEFINE_SPINLOCK(bcm9XXXX_lock);
		unsigned long flags;
	
		spin_lock_irqsave(&bcm9XXXX_lock, flags);
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		r = inline_map_read(map, ofs);
		spin_unlock_irqrestore(&bcm9XXXX_lock, flags);
		return r;
	}
	else
		return inline_map_read(map, ofs);
}

static void bcm9XXXX_map_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 ||
	                            bcm7403Ax_rev == 0x20  )
	{
		if(len > 16)
		{
			while(len >= 16)
			{
				bcm9XXXX_map_copy_from_16bytes(to, map->virt + from, 16);
				(to)   += 4;
				from   += 4;

				len -= 16;
			}
		}
	
		if(len > 0)
		bcm9XXXX_map_copy_from_16bytes(to, map->virt + from, len);
	}	
	else	
		memcpy_fromio(to, map->virt + from, len);
}

static void bcm9XXXX_map_write(struct map_info *map, const map_word d, unsigned long ofs)
{
	inline_map_write(map, d, ofs);
}

static void bcm9XXXX_map_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	inline_map_copy_to(map, to, from, len);
}
#endif

struct map_info bcm9XXXX_map
= {
	name: "Broadcom 9xxxx mapped flash",
	// size: WINDOW_SIZE, // THT: Now a value to be determined at run-time.
	bankwidth: BUSWIDTH,

// jipeng - enable this for 7401C0 & 7118A0
#ifdef	CONFIG_MTD_COMPLEX_MAPPINGS
	read: bcm9XXXX_map_read,
	copy_from: bcm9XXXX_map_copy_from,
	write: bcm9XXXX_map_write,
	copy_to: bcm9XXXX_map_copy_to
#endif 
};

/*
 * Don't define DEFAULT_SIZE_MB if the platform does not support a standard partition table.
 * Defining it will allow the user to specify flashsize=nnM at boot time for non-standard flash size, however.
 */

static struct mtd_partition bcm9XXXX_parts[CFE_MAX_FLASH_PARTITIONS] = {
#if 1
	/*
	** This is a default map - if we retrieved flash partition
	** information from CFE the partition map will be dynamically
	** set up in init_bcm9XXXX_map(). If we are booted with
	** 'flashsize=64m', the size of member [0] is adjusted, as
	** are the offsets of all remaining members of the partition.
	*/
#define DEFAULT_SIZE_MB 32 /* 32MB flash */
	{ name: "rootfs",       offset: 0,          size:  177*128*1024 },
	{ name: "rawnvr",       offset: 0x01620000, size:      128*1024 },
	{ name: "splash",       offset: 0x01640000, size:   40*128*1024 },
	{ name: "pstor",        offset: 0x01b40000, size:    6*128*1024 },
	{ name: "cfe",          offset: 0x01C00000, size:    4*128*1024 },
	{ name: "vmlinux",      offset: 0x01C80000, size:   26*128*1024 },
	{ name: "drmregion",    offset: 0x01fc0000, size:      128*1024 },
	{ name: "scratch",      offset: 0x01fe0000, size:      126*1024 },
	{ name: "config",       offset: 0x01fff800, size:           144 },
	{ name: "nvram",        offset: 0x01fff890, size:          1904 },
#else
    /*
    ** For CFE versions before 1.3.1
    */
#define DEFAULT_SIZE_MB 64 /* 64MB flash */

        { name: "rootfs",       offset: 0,          size:   60 * 1024 * 1024 },
        { name: "cfe",          offset: 0x03C00000, size:  512 * 1024        },
        { name: "kernel",       offset: 0x03C80000, size: 3328 * 1024        },
        { name: "drmregion",    offset: 0x03fc0000, size:  128 * 1024        },
        { name: "nvram",        offset: 0x03fe0000, size:  126 * 1024        },
        { name: "macadr",       offset: 0x03ff8000, size:    2 * 1024        },
        { name: "pstor",        offset: 0x03a00000, size:    2 * 1024 * 1024 },
#endif
};


int __init init_bcm9XXXX_map(void)
{
#if 1
	int numPartitions = 10;     /* initialize to number of paritions statically defined above */
#else
	int numPartitions = 7;      /* initialize to number of paritions statically defined above */
#endif

	printk(KERN_NOTICE "BCM97XXX flash device: 0x%08lx at 0x%08lx\n", WINDOW_SIZE, WINDOW_ADDR);
	bcm9XXXX_map.size = WINDOW_SIZE;

#if defined (CONFIG_MIPS_BCM_NDVD) && !defined (CONFIG_MIPS_BCM7405)
	/*
	** Dynamic structure initialization
	**
	** Process flash partition map from CFE HW INFO
	*/
	xiocb_boardinfo_t *boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;
	cfe_xuint_t member[3], nElements;
	int i;
	u_int32_t partitions_total_size = 0;

	if (cfe_min_rev(boardinfo->bi_ver_magic)) {

		printk("- Flash Type: %s\n", boardinfo->bi_flash_type == FLASH_XTYPE_NOR ? "NOR" : "NAND");

		nElements     = boardinfo->bi_config_flash_numparts;
		numPartitions = (int)nElements;
		printk("- Number of Flash Partitions: %d\n", numPartitions);

		for (i = 0; i < numPartitions; i++) {
			member[0] = boardinfo->bi_flash_partmap[i].base;
			member[1] = boardinfo->bi_flash_partmap[i].offset;
			member[2] = boardinfo->bi_flash_partmap[i].size;
			printk("-   Partition %d: Name: %s, base 0x%08x, offset 0x%08x, size 0x%08x\n",
						i,
						boardinfo->bi_flash_partmap[i].part_name,
						(unsigned)member[0],
						(unsigned)member[1],
						(unsigned)member[2]);
		}
		printk("\n\n");

		/*
		** Overwrite the compiled-in defaults if we've obtained
		** partition info straight from CFE.
		**
		** CFE Magic number encodes the version...
		** require CFE Version 1.5.x minimum
		*/
		if (boardinfo->bi_flash_type == FLASH_XTYPE_NOR) {

			printk("%s: Configure NOR flash partition map from CFE data - %d partitions reported\n", __FUNCTION__, numPartitions);

			/*
			** String manipulations:
			**  1. Strip leading "flash<x>." from CFE partition name
			**  2. Convert the following partition name strings by existing convention:
			**      - CFE "avail0" -> Linux "rootfs"
			**      - CFE "kernel" -> Linux "vmlinux"
			*/
			for (i = 0; i < numPartitions; i++) {

				char *partName;

				if (boardinfo->bi_flash_partmap[i].size == 0) {
					/*
					** No zero-sized partitions allowed.
					*/
					numPartitions -= 1;
					continue;
				}

				member[0] = boardinfo->bi_flash_partmap[i].offset;
				member[1] = boardinfo->bi_flash_partmap[i].size;

				partName = strstr(boardinfo->bi_flash_partmap[i].part_name, ".");

				if (partName == (char *)NULL)
					partName = &boardinfo->bi_flash_partmap[i].part_name[0];
				else {
					partName += 1;

					if (strcmp(partName, "avail0") == 0)
						partName = "rootfs";
					else if (strcmp(partName, "kernel") == 0)
						partName = "vmlinux";
					else if (strcmp(partName, "avail1") == 0) {
						/*
						** Don't want to double-count if there's
						** a flash1.avail1 partition.
						*/
						numPartitions -= 1;
						continue;
					}
				}

				strcpy(bcm9XXXX_parts[i].name, partName);
				bcm9XXXX_parts[i].offset = (u_int32_t)member[0];
				bcm9XXXX_parts[i].size   = (u_int32_t)member[1];

				partitions_total_size += bcm9XXXX_parts[i].size;
			}

			/*
			** Make sure that the total size of all partitions obtained
			** from CFE does not exceed DEFAULT_SIZE_MB, or we're in for
			** trouble ahead !
			*/
			if (partitions_total_size > (DEFAULT_SIZE_MB << 20)) {
				printk("%s: CFE FLASH PARTITION TOTAL SIZE EXCEEDS %d MB, LINUX MODIFICATION REQUIRED !!!!!\n", __FUNCTION__);
				BUG();
			}
		}
		else if (boardinfo->bi_flash_type == FLASH_XTYPE_NAND){
		    printk("%s: CFE reports NAND flash\n", __FUNCTION__);
		}
		else{
			printk("%s: CFE REPORTS INVALID FLASH TYPE !!!!!\n", __FUNCTION__);
			printk(">>>> Retain statically defined partition map - %d partitions\n", __FUNCTION__, numPartitions);
		}
	}
	else {
		printk("%s: CFE VERSION MUST BE 1.5.x OR LATER !!!!!\n", __FUNCTION__);
		printk("%s: Retain statically defined partition map - %d partitions\n", __FUNCTION__, numPartitions);
	}

#endif // defined (CONFIG_MIPS_BCM_NDVD)

	/* Adjust partition table */
#ifdef DEFAULT_SIZE_MB
	if (WINDOW_SIZE != (DEFAULT_SIZE_MB << 20)) {
		int i;
		
		bcm9XXXX_parts[0].size += WINDOW_SIZE - (DEFAULT_SIZE_MB << 20);
		//printk("Part[0] name=%s, size=%x, offset=%x\n", bcm9XXXX_parts[0].name, bcm9XXXX_parts[0].size, bcm9XXXX_parts[0].offset);
		for (i = 1; i < numPartitions; i++) {
			bcm9XXXX_parts[i].offset += WINDOW_SIZE - (DEFAULT_SIZE_MB << 20);
			//printk("Part[%d] name=%s, size=%x, offset=%x\n", i, bcm9XXXX_parts[i].name, bcm9XXXX_parts[i].size, bcm9XXXX_parts[i].offset);
		}
	}
#endif // DEFAULT_SIZE_MB

	bcm9XXXX_map.virt = (void *)ioremap(WINDOW_ADDR, WINDOW_SIZE);

	if (!bcm9XXXX_map.virt) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	
	bcm9XXXX_mtd = do_map_probe("cfi_probe", &bcm9XXXX_map);
	if (!bcm9XXXX_mtd) {
		iounmap((void *)bcm9XXXX_map.virt);
		return -ENXIO;
	}
		
	add_mtd_partitions(bcm9XXXX_mtd, bcm9XXXX_parts, numPartitions);
	bcm9XXXX_mtd->owner = THIS_MODULE;
	return 0;
}

void __exit cleanup_bcm9XXXX_map(void)
{
	if (bcm9XXXX_mtd) {
		del_mtd_partitions(bcm9XXXX_mtd);
		map_destroy(bcm9XXXX_mtd);
	}
	if (bcm9XXXX_map.virt) {
		iounmap((void *)bcm9XXXX_map.virt);
		bcm9XXXX_map.virt = 0;
	}
}

module_init(init_bcm9XXXX_map);
module_exit(cleanup_bcm9XXXX_map);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steven J. Hill <shill@broadcom.com>");
MODULE_DESCRIPTION("Broadcom 7xxx MTD map driver");
