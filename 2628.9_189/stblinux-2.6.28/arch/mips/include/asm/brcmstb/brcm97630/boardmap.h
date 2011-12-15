/*
 *     Copyright (c) 1999-2009, Broadcom Corporation
 *     All Rights Reserved
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
 */

#ifndef BOARDMAP_H
#define BOARDMAP_H

/*****************************************************************************/
/*                    Include common chip definitions                        */
/*****************************************************************************/
#include "bcmmips.h"
#include "bchp_common.h"

/*****************************************************************************/
/*                    MIPS Physical Memory Map                               */
/*****************************************************************************/
#define CPU_PHYS_SDRAM_BASE	        0x00000000	/* SDRAM Base */
#define CPU_PHYS_ROM_BASE           0x1FC00000	/* ROM */
#define CPU_PHYS_FLASH_BASE         0x1C000000
#define CPU_PHYS_FPGA_BASE          0x1A000000
#define CPU_PHYS_1394_BASE          0x19000000
#define CPU_PHYS_POD_BASE			0x19800000


/*****************************************************************************/
/*                      MIPS Virtual Memory Map                              */
/*                                                                           */
/* Note that the addresses above are physical addresses and that programs    */
/* have to use converted addresses defined below:                            */
/*****************************************************************************/
#define DRAM_BASE_CACHE		BCM_PHYS_TO_K0(CPU_PHYS_SDRAM_BASE)   /* cached DRAM */
#define DRAM_BASE_NOCACHE	BCM_PHYS_TO_K1(CPU_PHYS_SDRAM_BASE)   /* uncached DRAM */
#define ROM_BASE_CACHE		BCM_PHYS_TO_K0(CPU_PHYS_ROM_BASE)
#define ROM_BASE_NOCACHE	BCM_PHYS_TO_K1(CPU_PHYS_ROM_BASE)
#define FLASH_BASE_NOCACHE  BCM_PHYS_TO_K1(CPU_PHYS_FLASH_BASE)
#define FPGA_BASE_NOCACHE   BCM_PHYS_TO_K1(CPU_PHYS_FPGA_BASE)
#define IEEE1394_BASE_NOCACHE   BCM_PHYS_TO_K1(CPU_PHYS_1394_BASE)


/*****************************************************************************/
/* Include chip specific .h files                                            */
/*****************************************************************************/



#endif
