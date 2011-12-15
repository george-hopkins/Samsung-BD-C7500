/* linux/drivers/mtd/nand/brcm_nand.c
 *
 * Copyright 2008 Broadcom Corp
 *
 *
 * Derived from drivers/mtd/nand/brcm_nand.c
 *	http://blackfin.uclinux.org/
 *	Bryan Wu <bryan.wu@analog.com>
 *
 * Derived from drivers/mtd/nand/s3c2410.c
 * Copyright (c) 2007 Ben Dooks <ben@simtec.co.uk>
 *
 * Derived from drivers/mtd/nand/cafe.c
 * Copyright  2006 Red Hat, Inc.
 * Copyright  2006 David Woodhouse <dwmw2@infradead.org>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/bitops.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/dma.h>
#include <asm/cacheflush.h>
#include <asm/nand.h>

#define DRV_NAME	"brcm-nand"
#define DRV_VERSION	"0.9"
#define DRV_AUTHOR	"Broadcom.com"
#define DRV_DESC	"Broadcom on-chip NAND FLash Controller Driver"

