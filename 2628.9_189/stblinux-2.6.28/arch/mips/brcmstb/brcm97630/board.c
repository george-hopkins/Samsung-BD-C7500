/*
 * arch/mips/brcmstb/brcm97630/board.c
 *
 * Copyright (C) 2004-2009 Broadcom Corporation
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
 *
 * Board Specific routines for Broadcom eval boards
 *
 */

#include <linux/autoconf.h>
// For module exports
#include <linux/module.h>
#include <asm/bootinfo.h>

#include <linux/bug.h>

#include <asm/brcmstb/common/brcmstb.h>
#include <../arch/mips/brcmstb/common/cfe_xiocb.h>



#ifndef CONFIG_DISCONTIGMEM
extern unsigned long memSize;
#endif

/*
 * single memory bank configs
 */
struct mem_region mem_regions_128[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x08000000 },		/* 128 mb bank 0 */
	{0,0},
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_128[MAX_BMEM_REGIONS] = {
	{paddr:	0x02000000, size: 0x05d00000 },			/* 32/93 mb split */
	{paddr: 0x07d00000, size: 0x00300000},			/* top 3 mb for FE */
	{0,0},	
	{0,0}

};


struct mem_region mem_regions_256_x1[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x10000000 },		/* 256 mb bank 0 */
	{0,0},
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_256_x1[MAX_BMEM_REGIONS] = {
	{paddr:	0x02300000, size: 0x05d00000 },			/* 35/93 mb split */
	{paddr:	0x09900000, size: 0x06400000 },			/* 25/100 mb split */
	{paddr: 0x0fd00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}
};

	/*
	 * 384 mb on a single DDR controler.
	 * Hardware is actually a 512 MB.
	 * Appears as two regions
	 */
struct mem_region mem_regions_384_x1[MAX_MEM_REGIONS] = {

	{ paddr: 0x00000000,	size: 0x08000000 },		/* 128 mb bank 0 */
	{ paddr: 0x50000000,	size: 0x10000000 },		/* 256 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_384_x1[MAX_BMEM_REGIONS] = {
	{paddr:	0x02800000, size: 0x05800000 },			/* 40/88 mb split */
	{paddr:	0x53900000, size: 0x0c400000 },			/* 57/196 mb split */
	{paddr: 0x5fd00000, size: 0x00300000 },			/* top 3 mb for FE */	
	{0,0}

};
	/*
	 * 512 mb on a single DDR controler.
	 * Appears as two regions
	 */
struct mem_region mem_regions_512_x1[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x10000000 },		/* 256 mb bank 0 */
	{ paddr: 0x50000000,	size: 0x10000000 },		/* 256 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_512_x1[MAX_BMEM_REGIONS] = {
	{paddr:	0x0A800000, size: 0x05800000 },			/* 168/88 mb split */
	{paddr:	0x53900000, size: 0x0c400000 },			/* 57/196 mb split */
	{paddr: 0x5fd00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}
};

struct mem_region bmem_regions_512_3d_x1[MAX_BMEM_REGIONS] = {
	{paddr:	0x06400000, size: 0x09c00000 },			/* 100/156 mb split */
	{paddr:	0x50000000, size: 0x0fd00000 },			/* 0/252 mb split */
	{paddr: 0x5fd00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}

};

/*
 * dual memory bank configs
 */


struct mem_region mem_regions_256[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x08000000 },		/* 128 mb bank 0 */
	{ paddr: 0x20000000,	size: 0x08000000 },		/* 128 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_256[MAX_BMEM_REGIONS] = {
	{paddr:	0x02300000, size: 0x05d00000 },			/* 35/93 mb split */
	{paddr:	0x21900000, size: 0x06400000 },			/* 25/100 mb split */
	{paddr: 0x27d00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}

};

struct mem_region mem_regions_384[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x08000000 },		/* 128 mb bank 0 */
	{ paddr: 0x20000000,	size: 0x10000000 },		/* 256 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_384[MAX_BMEM_REGIONS] = {
	{paddr:	0x02800000, size: 0x05800000 },			/* 40/88 mb split */
	{paddr:	0x23900000, size: 0x0c400000 },			/* 57/196 mb split */
	{paddr: 0x2fd00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}

};

struct mem_region mem_regions_384_v2[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x10000000 },		/* 256 mb bank 0 */
	{ paddr: 0x20000000,	size: 0x08000000 },		/* 128 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_384_v2[MAX_BMEM_REGIONS] = {
	{paddr:	0x04800000, size: 0x0b800000 },			/* 72/184 mb split */
	{paddr:	0x22800000, size: 0x05500000 },			/* 40/85  mb split */
	{paddr: 0x27d00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}

};

struct mem_region mem_regions_512[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x10000000 },		/* 256 mb bank 0 */
	{ paddr: 0x20000000,	size: 0x10000000 },		/* 256 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region bmem_regions_512[MAX_BMEM_REGIONS] = {
	{paddr:	0x0A800000, size: 0x05800000 },			/* 168/88 mb split */
	{paddr:	0x23900000, size: 0x0c400000 },			/* 57/196 mb split */
	{paddr: 0x2fd00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}

};

struct mem_region bmem_regions_512_3d[MAX_BMEM_REGIONS] = {
	{paddr:	0x06400000, size: 0x09c00000 },			/* 100/156 mb split */
	{paddr:	0x20000000, size: 0x0fd00000 },			/* 0/252 mb split */
	{paddr: 0x2fd00000, size: 0x00300000 },			/* top 3 mb for FE */
	{0,0}

};
/*
 * Defaults
 */
struct mem_region mem_regions_cfe[MAX_MEM_REGIONS] = {
	{ paddr: 0x00000000,	size: 0x10000000 },		/* 256 mb bank 0 */
	{ paddr: 0x20000000,	size: 0x10000000 },		/* 256 mb bank 1 */
	{0,0},
	{0,0}
};

struct mem_region *bmem_regions = (struct mem_region *)0;
struct mem_region *mem_regions  = mem_regions_cfe;

static int memcfg, mem_banks, cfg3d;

unsigned long dramSize;

void __init
get_memory_config(void)
{

#ifdef CONFIG_BOOT_RAW
	/* no CFE
         * basic 128/128 mem cfg
	 */
	if (memcfg == 0)
		memcfg = 256;

#else
	/*
	** Dynamic structure initialization
	**
	** Process flash partition map from CFE HW INFO
	*/
	xiocb_boardinfo_t *boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;
	cfe_xuint_t member;
	int i;
	unsigned long memSize[CFE_NUM_DDR_CONTROLLERS] = {0, 0 };

	

	if (cfe_min_rev(boardinfo->bi_ver_magic)) {

		printk(KERN_INFO "%s: Take memory configuration from CFE HWinfo\n", __func__);

		if (CFE_NUM_DDR_CONTROLLERS > 4) {
			printk(KERN_ERR "%s: CFE_NUM_DDR_CONTROLLERS exceeds linux limitation of 4 !\n",
				__func__);
			BUG();
		}

		for (i = 0; i < CFE_NUM_DDR_CONTROLLERS; i++) {
			member = boardinfo->bi_ddr_bank_size[i];
			printk(KERN_NOTICE"-- DDR Bank %d: %d MB\n", i, (unsigned int)member);
			memSize[i] = (unsigned long)member;
			memSize[i] <<= 20;
		}
	} else {
		printk(KERN_ERR "%s: no CFE!\n", __func__);
		memcfg = 128;
		return;
	}

	

	/*
	 * NOTE: CFE_NUM_DDR_CONTROLLERS < MAX_MEM_REGIONS
	 */
	for (i = 0; i < CFE_NUM_DDR_CONTROLLERS; i++) 
		    mem_regions_cfe[i].size = memSize[i];

#endif	// CONFIG_BOOT_RAW
	
}


void __init
set_memory_config(void)
{
	int i;
	int ddr_size, ddr_banks;
	
	if (memcfg)
	    printk(KERN_INFO "%s: memory config = %d\n", __func__, memcfg);

	if (mem_banks)
	    printk(KERN_INFO "%s: memory banks  = %d\n", __func__, mem_banks);

	if (cfg3d)
	    printk(KERN_INFO "%s: cfg3d = %d\n", __func__, cfg3d);

	/* 
	 * get physical  mem info:
	 * calculate DDR total size and the number of banks
	 */
	ddr_size  = 0;
	ddr_banks = 0;

	for (i = 0; i < MAX_MEM_REGIONS; i++)  {
		ddr_size += mem_regions_cfe[i].size;
		if (mem_regions_cfe[i].size)
			ddr_banks++;
	}

	printk(KERN_INFO "ddr_banks: %d size %dMb (0x%08x)\n", ddr_banks,
		ddr_size >> 20, ddr_size);

 	/*
	 * sanity check the overrides
	 */
	if (memcfg > (ddr_size >> 20)) {
		printk(KERN_CRIT "%s: memcfg %d is greater than real memory %d\n",
			__func__, memcfg, ddr_size);
		memcfg = (ddr_size >> 20);
	}
		
	if (mem_banks > ddr_banks) {
		printk(KERN_CRIT "%s: membanks %d is greater than real DDR banks %d\n",
			__func__, mem_banks, ddr_banks);
		mem_banks = ddr_banks;
	} 

	if (mem_banks)
		ddr_banks = mem_banks;
	
	/*
	 * check for valid config.
	 */
	switch(memcfg) {
	    /* defined configs */
	    case 128:
	    case 256:
	    case 384:
	    case 512:
		break;

	    default: 
		printk(KERN_ERR "%s: Unknown memcfg option %d\n", __func__, memcfg);
		mem_regions  = mem_regions_cfe;
	    case 0:
		memcfg = ddr_size >> 20;
		break;

	}
	
	switch(memcfg) {

	     default:
		printk(KERN_ERR "\n%s: Unknown memory config %d\n", __func__, memcfg);
		printk(KERN_ERR "%s using 128mb config\n\n", __func__);

	    case 128:
		mem_regions  = mem_regions_128;
		bmem_regions = bmem_regions_128;
		break;

	    case 256:
		if (ddr_banks == 1) {
			mem_regions  = mem_regions_256_x1;
			bmem_regions = bmem_regions_256_x1;
		} else {
			mem_regions  = mem_regions_256;
			bmem_regions = bmem_regions_256;
		}
		break;

	    case 384:
		if (ddr_banks == 1) {
			mem_regions  = mem_regions_384_x1;
			bmem_regions = bmem_regions_384_x1;
		} else {
		    if (mem_regions_cfe[1].size == 0x08000000) {
			/* bank 0 - 256 mb, bank 1 - 128mb */
			mem_regions  = mem_regions_384_v2;
			bmem_regions = bmem_regions_384_v2;

		    } else {
			/* bank 0 - 128 mb, bank 1 - 256 */
			mem_regions  = mem_regions_384;
			bmem_regions = bmem_regions_384;
		    }
		}
		break;
	
	    case 512:
		if (ddr_banks == 1) {
			mem_regions  = mem_regions_512_x1;
			if (cfg3d)
				bmem_regions = bmem_regions_512_3d_x1;
			else
				bmem_regions = bmem_regions_512_x1;
		} else {
			mem_regions  = mem_regions_512;
			if (cfg3d)
				bmem_regions = bmem_regions_512_3d;
			else
				bmem_regions = bmem_regions_512;
		}
		break;
	}


	dramSize = 0;
	for (i = 0; i < MAX_MEM_REGIONS; i++) 
		dramSize += mem_regions[i].size;

	for (i = 0; i < MAX_MEM_REGIONS; i++) {
		if (mem_regions[i].size) {
			add_memory_region(mem_regions[i].paddr, mem_regions[i].size, BOOT_MEM_RAM);
		}
	}
}


static int __init memcfg_early(char *p)
{
	memcfg = memparse(p, &p);

	if (memcfg > (1 << 20))
		memcfg >>= 20;
	return 0;
}
early_param("memcfg", memcfg_early);

static int __init mem_banks_early(char *p)
{
	mem_banks = memparse(p, &p);

	if (mem_banks > MAX_MEM_REGIONS)
		mem_banks = MAX_MEM_REGIONS;

	return 0;
}
early_param("membanks", mem_banks_early);

static int __init cfg3d_early(char *p)
{
	cfg3d = 1;
	return 0;
}
early_param("cfg3d", cfg3d_early);


/*
 * This code is somewhat simplified by the fact that a BMEM reserved region
 * can not span multiple memory regions.
 * However, a memory region, and thus a BMEM region, might span multiple zones
 */
#define addr_to_pfn(x) ((x) >> PAGE_SHIFT)

unsigned int
get_zone_reserved_size (struct zone *zone)
{
	unsigned int bmem_start_pfn, bmem_end_pfn;
	unsigned int zone_start_pfn, zone_end_pfn;
	unsigned int start, end, size, i;

	BUG_ON(!zone);
	BUG_ON(!bmem_regions);

	size = 0;

	zone_start_pfn = zone->zone_start_pfn;
	zone_end_pfn   = zone_start_pfn + zone->spanned_pages;

	for (i = 0; i < MAX_BMEM_REGIONS; i++) {
	    if (bmem_regions[i].size == 0)
		continue;

	    bmem_start_pfn = addr_to_pfn(bmem_regions[i].paddr);
	    bmem_end_pfn = bmem_start_pfn + addr_to_pfn(bmem_regions[i].size);

	    if (bmem_end_pfn < zone_start_pfn)
		continue;

	    if (bmem_start_pfn >= zone_end_pfn)
		continue;

	    
	    if (bmem_start_pfn > zone_start_pfn)
		start = bmem_start_pfn;
	    else
		start = zone_start_pfn;

	    if (bmem_end_pfn <= zone_end_pfn)
		end = bmem_end_pfn;
	    else
		end = zone_end_pfn;

	     size += end - start;
	}

	return(size);
}


/*
 * Return total current memory size known to the kernel.
 */
unsigned long get_RAM_size(void)
{
	printk(KERN_DEBUG "%s: Returning %d MB\n", __func__, (unsigned)(dramSize>>20));
	return (dramSize);
}

