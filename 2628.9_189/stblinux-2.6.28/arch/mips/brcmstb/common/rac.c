/*
 * arch/mips/brcmstb/common/rac.c
 *
 * Copyright (C) 2007 Broadcom Corporation
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
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <asm/mipsregs.h>
#include <asm/bug.h>
#include <asm/addrspace.h>
#include <asm/cacheflush.h>
#include <asm/brcmstb/common/brcmstb.h>

extern unsigned long rac_config0, rac_config1, rac_address_range;
extern unsigned int par_val2;

#define CORE_OFS(x) ((x) - BCHP_BMIPS4380_RAC_CONFIG)

int __init
rac_setting(int value)
{
	unsigned long rac_value;
	unsigned long cba = __read_32bit_c0_register($22, 6);

	cba = (cba & 0xfffc0000) + KSEG1;

	rac_config0 = cba + CORE_OFS(BCHP_BMIPS4380_RAC_CONFIG);
	rac_config1 = cba + CORE_OFS(BCHP_BMIPS4380_RAC_CONFIG1);
	rac_address_range = cba + CORE_OFS(BCHP_BMIPS4380_RAC_ADDR_RANGE);

	switch(value) {
		case 0xff:	/* don't touch settings */
			return(0);
		case 0:		/* disable RAC */
			rac_value = 0x00;
			break;
		case 1:		/* I-RAC only */
			rac_value = 0x15;
			break;
		case 2:		/* D-RAC only */
			rac_value = 0x1a;
			break;
		case 3:		/* I/D RAC without selective invalidate */
			rac_value = 0x0f;
			break;
		case 11:	/* I/D RAC */
			rac_value = 0x1f;
			break;
		default:	/* invalid */
			return(-1);
	}
	bcm_inv_rac_all();
	DEV_WR(rac_config0, (DEV_RD(rac_config0) & ~0x1f) | rac_value);
	DEV_WR(rac_config1, (DEV_RD(rac_config1) & ~0x1f) | rac_value);
	DEV_WR(rac_address_range, par_val2);

	return(0);
}
