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
/*                   CPU to PCI Bridge Memory Map                            */
/*****************************************************************************/

#define CPU2PCI_CPU_PHYS_MEM_WIN_BASE     0xd0000000

/* Allow CPU to access PCI memory addresses 0xd0000000 to 0xdfffffff */
#define CPU2PCI_PCI_PHYS_MEM_WIN0_BASE    0xd0000000
#define CPU2PCI_PCI_PHYS_MEM_WIN1_BASE    0xd8000000
#define CPU2PCI_PCI_PHYS_MEM_WIN2_BASE    0xe0000000
#define CPU2PCI_PCI_PHYS_MEM_WIN3_BASE    0xe8000000

#define CPU2PCI_CPU_PHYS_IO_WIN_BASE      0xf0000000

/* Allow CPU to access PCI I/O addresses 0xe0000000 to 0xe05fffff */
#ifdef LITTLE_ENDIAN
#define CPU2PCI_PCI_PHYS_IO_WIN0_BASE     0x00000000
#define CPU2PCI_PCI_PHYS_IO_WIN1_BASE     0x00200000
#define CPU2PCI_PCI_PHYS_IO_WIN2_BASE     0x00400000
#else
#define CPU2PCI_PCI_PHYS_IO_WIN0_BASE     0x00000002
#define CPU2PCI_PCI_PHYS_IO_WIN1_BASE     0x00200002
#define CPU2PCI_PCI_PHYS_IO_WIN2_BASE     0x00400002
#endif


/*****************************************************************************/
/*                      PCI Physical Memory Map                              */
/*****************************************************************************/

/* PCI physical memory map */
#define PCI_7401_PHYS_ISB_WIN_BASE    0x10000000
#define PCI_7401_PHYS_MEM_WIN0_BASE   0x00000000
#define PCI_7401_PHYS_MEM_WIN1_BASE   0x02000000
#define PCI_7401_PHYS_MEM_WIN2_BASE   0x04000000

#define PCI_1394_PHYS_MEM_WIN0_BASE   0xd0000000

#define PCI_DEVICE_ID_EXT       0x0d
#define PCI_DEVICE_ID_1394      0x0e
#define PCI_DEVICE_ID_MINI      0x04
#define PCI_DEVICE_ID_SATA      0 /* On 2ndary PCI bus */

#define PCI_IDSEL_EXT           (0x10000 << PCI_DEVICE_ID_EXT)
#define PCI_IDSEL_1394          (0x10000 << PCI_DEVICE_ID_1394)
#define PCI_IDSEL_MINI          (0x10000 << PCI_DEVICE_ID_MINI)
#define PCI_IDSEL_SATA          (0x10000 << PCI_DEVICE_ID_SATA)

#define PCI_DEV_NUM_EXT         (PCI_DEVICE_ID_EXT  << 11)
#define PCI_DEV_NUM_1394        (PCI_DEVICE_ID_1394 << 11)
#define PCI_DEV_NUM_MINI        (PCI_DEVICE_ID_MINI << 11)
#define PCI_DEV_NUM_SATA        (PCI_DEVICE_ID_SATA << 11)

/* SATA device */
#define PCS0_OFS				0x200
#define PCS1_OFS				0x240
#define SCS0_OFS				0x280
#define SCS1_OFS				0x2c0
#define BM_OFS					0x300
#define MMIO_OFS				0xb0510000 
#define PCI_SATA_PHYS_REG_BASE	(0xb0520000 + PCS0_OFS)

#define DRAM_SIZE (1024*1024*1024)

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


#define PCI_MEM_WIN_BASE    0xd0000000
#define PCI_MEM_WIN_SIZE    0x08000000
#define PCI_IO_WIN_BASE     0xf0000000
#define PCI_IO_WIN_SIZE     0x02000000

/*****************************************************************************/
/* Include chip specific .h files                                            */
/*****************************************************************************/



#endif
