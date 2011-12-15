/*
 * drivers/mtd/brcmnand/brcmnand.h
 *
 *  Copyright (c) 2005,2009 Broadcom Corp.
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
 * Data structures for Broadcom NAND controller
 * 
 * when        who        what
 * 20060729    tht        Original coding
 */


#ifndef _BRCM_NAND_H_
#define _BRCM_NAND_H_

#include <linux/mtd/nand.h>
#include <linux/types.h>
#include <asm/brcmstb/common/brcmstb.h>

#ifdef CONFIG_MIPS_BCM7440B0
#include <asm/brcmstb/brcm97440b0/bchp_ebi.h>  // For CS registers
#include <asm/brcmstb/brcm97440b0/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97440b0/bchp_nand.h>
#include <asm/brcmstb/brcm97440b0/bchp_common.h>
#include <asm/brcmstb/brcm97440b0/bchp_edu.h>
#include <asm/brcmstb/brcm97440b0/bchp_hif_intr2.h>
#include <asm/brcmstb/brcm97440b0/bcmintrnum.h>
#elif defined(CONFIG_MIPS_BCM7601)
#include <asm/brcmstb/brcm97601/bchp_ebi.h>  // For CS registers
#include <asm/brcmstb/brcm97601/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97601/bchp_nand.h>
#include <asm/brcmstb/brcm97601/bchp_common.h>
#include <asm/brcmstb/brcm97601/bchp_edu.h>
#include <asm/brcmstb/brcm97601/bchp_hif_intr2.h>
#include <asm/brcmstb/brcm97601/bcmintrnum.h>
#elif defined(CONFIG_MIPS_BCM7635)
#include <asm/brcmstb/brcm97635/bchp_ebi.h>  // For CS registers
#include <asm/brcmstb/brcm97635/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97635/bchp_nand.h>
#include <asm/brcmstb/brcm97635/bchp_common.h>
#include <asm/brcmstb/brcm97635/bchp_edu.h>
#include <asm/brcmstb/brcm97635/bchp_hif_intr2.h>
#include <asm/brcmstb/brcm97635/bcmintrnum.h>
#elif defined(CONFIG_MIPS_BCM7630)
#include <asm/brcmstb/brcm97630/bchp_ebi.h>  // For CS registers
#include <asm/brcmstb/brcm97630/bchp_sun_top_ctrl.h>
#include <asm/brcmstb/brcm97630/bchp_nand.h>
#include <asm/brcmstb/brcm97630/bchp_common.h>
#include <asm/brcmstb/brcm97630/bchp_edu.h>
#include <asm/brcmstb/brcm97630/bchp_hif_intr2.h>
#include <asm/brcmstb/brcm97630/bcmintrnum.h>
#else
#error Platform not supported
#endif

#define BRCM_CONFIG_MIPS_BCM7XXX  (defined(CONFIG_MIPS_BCM7440B0) || defined(CONFIG_MIPS_BCM7601) || \
                                   defined(CONFIG_MIPS_BCM7635)   || defined(CONFIG_MIPS_BCM7630)) 

#define BRCM_CONFIG_MIPS_BCM76XX  (defined(CONFIG_MIPS_BCM7601) || defined(CONFIG_MIPS_BCM7635) ||  defined(CONFIG_MIPS_BCM7630))

/*
 * Conversion between Kernel Kconfig and Controller version number
 */

#define CONFIG_MTD_BRCMNAND_VERS_0_0        0
#define CONFIG_MTD_BRCMNAND_VERS_0_1        1    //0.1    7401Cx, 7400B0, 7118A0, 7403 * 7451, 7452, 3563
#define CONFIG_MTD_BRCMNAND_VERS_1_0        2    //1.0    7440B0, 7405a0 (also 7118C0 running as 1.0 in 2.6.12)
#define CONFIG_MTD_BRCMNAND_VERS_2_0        3    //2.0    7325
#define CONFIG_MTD_BRCMNAND_VERS_2_1        4    //2.1    7335
#define CONFIG_MTD_BRCMNAND_VERS_2_2        5    //2.2    7118C0 & 7405b0 (in 2.6.18) /* Not implemented for 2.6.12 */ 
#define CONFIG_MTD_BRCMNAND_VERS_3_0        6    //3.0    3548a0 (in 2.6.18), 7601a0 & 7601b0 (in 2.6.18, 2.6.24)
#define CONFIG_MTD_BRCMNAND_VERS_3_2        7    //3.2    Device ID decode update, 7413b0
#define CONFIG_MTD_BRCMNAND_VERS_3_3        8    //3.3    Expand ACC/CONFIG to allow per chip registers, 7420b0, 7635a0
#define CONFIG_MTD_BRCMNAND_VERS_3_4        9    //3.4    Expand ACC/CONFIG to allow per chip registers, 7630

#define BRCMNAND_VERSION(major, minor)    ((major<<8) | minor)

#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0)
    #define L_OFF_T          int32_t
    #define UL_OFF_T         uint32_t
    #define MAX_NAND_CS    1
    #define MAX_NAND_CS_PER_BRCMNAND_CHIP MAX_NAND_CS
#elif (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    #define L_OFF_T          int64_t
    #define UL_OFF_T        uint64_t
    #define MAX_NAND_CS    1
    #define MAX_NAND_CS_PER_BRCMNAND_CHIP MAX_NAND_CS
#else //CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3)
    #define L_OFF_T          int64_t
    #define UL_OFF_T        uint64_t

    #define MAX_NAND_CS    4   /*There could be up to 8, but only 4 sets of registers 
                                are available in the CONFIG_MTD_BRCMNAND_VERS_3_3.*/
    #define MAX_NAND_CS_PER_BRCMNAND_CHIP 1
#endif


//ST NAND flashes
#ifndef FLASHTYPE_NUMONYX
    #define FLASHTYPE_NUMONYX            0x20
#endif
#define NUMONYX_NAND128W3A           0x73
#define NUMONYX_NAND256R3A           0x35
#define NUMONYX_NAND256W3A           0x75
#define NUMONYX_NAND256R4A           0x45
#define NUMONYX_NAND256W4A           0x55
#define NUMONYX_NAND512R3A           0x36    //Used on Bcm97400
#define NUMONYX_NAND512W3A           0x76
#define NUMONYX_NAND512R4A           0x46
#define NUMONYX_NAND512W4A           0x56
#define NUMONYX_NAND01GR3A           0x39
#define NUMONYX_NAND01GW3A           0x79
#define NUMONYX_NAND01GR4A           0x49
#define NUMONYX_NAND01GW4A           0x59
#define NUMONYX_NAND01GR3B           0xA1
#define NUMONYX_NAND01GW3B           0xF1
#define NUMONYX_NAND01GR4B           0xB1
#define NUMONYX_NAND01GW4B           0xC1
#define NUMONYX_NAND02GR3B           0xAA
#define NUMONYX_NAND02GW3B           0xDA
#define NUMONYX_NAND02GR4B           0xBA
#define NUMONYX_NAND02GW4B           0xCA
#define NUMONYX_NAND04GR3B           0xAC
#define NUMONYX_NAND04GW3B           0xDC
#define NUMONYX_NAND04GR4B           0xBC
#define NUMONYX_NAND04GW4B           0xCC
#define NUMONYX_NAND08GR3B           0xA3
#define NUMONYX_NAND08GW3B           0xD3
#define NUMONYX_NAND08GR4B           0xB3
#define NUMONYX_NAND08GW4B           0xC3
#define NUMONYX_NAND08GW3C           0xD3


//Samsung NAND flash
#define FLASHTYPE_SAMSUNG       0xEC
#define SAMSUNG_K9F1G08R0A      0xA1
#define SAMSUNG_K9F1G08U0A      0xF1
#define SAMSUNG_K9F2G08U1A      0xF1
#define SAMSUNG_K9F2G08U0A      0xDA
#define SAMSUNG_K9F8G08U0M      0xD3
#define SAMSUNG_K9F4G08U0B      0xDC    //K9F4G08U0B is new generation of K9F4G08U0M
#define SAMSUNG_K9G8G08U0B      0xD3
#define SAMSUNG_K9GAG08U0M      0xD5
//K9F5608(R/U/D)0D
#define SAMSUNG_K9F5608R0D      0x35
#define SAMSUNG_K9F5608U0D      0x75
#define SAMSUNG_K9F5608D0D      0x75
//K9F1208(R/B/U)0B
#define SAMSUNG_K9F1208R0B      0x36
#define SAMSUNG_K9F1208B0B      0x76
#define SAMSUNG_K9F1208U0B      0x76


/*--------- Chip ID decoding for Samsung MLC NAND flashes -----------------------*/
#define SAMSUNG_K9LBG08U0M    0xD7

#define SAMSUNG_3RDID_INT_CHIPNO_MASK    NAND_CI_CHIPNR_MSK

#define SAMSUNG_3RDID_CELLTYPE_MASK    NAND_CI_CELLTYPE_MSK
#define SAMSUNG_3RDID_CELLTYPE_SLC    0x00
#define SAMSUNG_3RDID_CELLTYPE_4LV    0x04
#define SAMSUNG_3RDID_CELLTYPE_8LV    0x08
#define SAMSUNG_3RDID_CELLTYPE_16LV    0x0C

// Low level MLC test as compared to the high level test in mtd-abi.h
#define NAND_IS_MLC(chip) ((chip)->cellinfo & NAND_CI_CELLTYPE_MSK)

#define SAMSUNG_3RDID_MASK            0x30
#define SAMSUNG_3RDID_NOP_1            0x00
#define SAMSUNG_3RDID_NOP_2            0x10
#define SAMSUNG_3RDID_NOP_4            0x20
#define SAMSUNG_3RDID_NOP_8            0x30

#define SAMSUNG_3RDID_INTERLEAVE        0x40

#define SAMSUNG_3RDID_CACHE_PROG        0x80

#define SAMSUNG_4THID_PAGESIZE_MASK    0x03
#define SAMSUNG_4THID_PAGESIZE_1KB    0x00
#define SAMSUNG_4THID_PAGESIZE_2KB    0x01
#define SAMSUNG_4THID_PAGESIZE_4KB    0x02
#define SAMSUNG_4THID_PAGESIZE_8KB    0x03

#define SAMSUNG_4THID_BLKSIZE_MASK    0x30
#define SAMSUNG_4THID_PAGESIZE_64KB    0x00
#define SAMSUNG_4THID_PAGESIZE_128KB    0x10
#define SAMSUNG_4THID_PAGESIZE_256KB    0x20
#define SAMSUNG_4THID_PAGESIZE_512KB    0x30

#define SAMSUNG_4THID_OOBSIZE_MASK    0x04
#define SAMSUNG_4THID_OOBSIZE_8B        0x00
#define SAMSUNG_4THID_OOBSIZE_16B    0x04

#define SAMSUNG_5THID_NRPLANE_MASK    0x0C
#define SAMSUNG_5THID_NRPLANE_1        0x00
#define SAMSUNG_5THID_NRPLANE_2        0x04
#define SAMSUNG_5THID_NRPLANE_4        0x08
#define SAMSUNG_5THID_NRPLANE_8        0x0C

#define SAMSUNG_5THID_PLANESZ_MASK    0x70
#define SAMSUNG_5THID_PLANESZ_64Mb    0x00
#define SAMSUNG_5THID_PLANESZ_128Mb    0x10
#define SAMSUNG_5THID_PLANESZ_256Mb    0x20
#define SAMSUNG_5THID_PLANESZ_512Mb    0x30
#define SAMSUNG_5THID_PLANESZ_1Gb    0x40
#define SAMSUNG_5THID_PLANESZ_2Gb    0x50
#define SAMSUNG_5THID_PLANESZ_4Gb    0x60
#define SAMSUNG_5THID_PLANESZ_8Gb    0x70

/*--------- Chip ID decoding for SLC NAND flashes -----------------------*/
#define SLC_4THID_PAGESIZE_MASK                 0x03
#define SLC_4THID_PAGESIZE_SHIFT                0
#define SLC_4THID_PAGESIZE_1KB                  0x00
#define SLC_4THID_PAGESIZE_2KB                  0x01
#define SLC_4THID_PAGESIZE_4KB                  0x02
#define SLC_4THID_PAGESIZE_8KB                  0x03

#define SLC_4THID_OOBSIZE_MASK                  0x04
#define SLC_4THID_OOBSIZE_SHIFT                 2
#define SLC_4THID_OOBSIZE_8B                    0x00
#define SLC_4THID_OOBSIZE_16B                   0x01

#define SLC_4THID_BLKSIZE_MASK                  0x30
#define SLC_4THID_BLKSIZE_SHIFT                 4
#define SLC_4THID_BLKSIZE_64KB                  0x00
#define SLC_4THID_BLKSIZE_128KB                 0x01
#define SLC_4THID_BLKSIZE_256KB                 0x02
#define SLC_4THID_BLKSIZE_512KB                 0x03

#define SLC_4THID_ORG_MASK                      0x40
#define SLC_4THID_ORG_SHIFT                     6
#define SLC_4THID_ORG_X8                        0x00
#define SLC_4THID_ORG_X16                       0x01
/*--------- END Samsung MLC NAND flashes -----------------------*/

//Hynix NAND flashes
#define FLASHTYPE_HYNIX         0xAD
//Hynix HY27(U/S)S(08/16)561A
#define HYNIX_HY27US08561A      0x75
#define HYNIX_HY27US16561A      0x55
#define HYNIX_HY27SS08561A      0x35
#define HYNIX_HY27SS16561A      0x45
//Hynix HY27(U/S)S(08/16)121A
#define HYNIX_HY27US08121A      0x76
#define HYNIX_HY27US16121A      0x56
#define HYNIX_HY27SS08121A      0x36
#define HYNIX_HY27SS16121A      0x46
//Hynix HY27(U/S)F(08/16)1G2M
#define HYNIX_HY27UF081G2M      0xF1
#define HYNIX_HY27UF161G2M      0xC1
#define HYNIX_HY27SF081G2M      0xA1
#define HYNIX_HY27SF161G2M      0xAD

#define HYNIX_H27U1G8F2B            0xF1
#define HYNIX_3RD_ID_H27U1G8F2B     0x00  /*This differentiates the H27U1G8F2B 
                                              from the HY27UF081G2A */
#define HYNIX_HY27UF081G2A      0xF1
#define HYNIX_3RD_ID_HY27UF081G2A   0x80  /*This differentiates the HY27UF081G2A 
                                              from the H27U1G8F2B*/
#define HYNIX_HY27UF082G2B      0xDA

#define HYNIX_HY27UF084G2B      0xDC    //512MByte device - new generation
#define HYNIX_HY27UF084G2M      0xDC    //512MByte device - old generation
#define HYNIX_HY27UG088G5M      0xDC    /*1GByte device - old generation: 
                                            We will need to read its 5th id byte
                                            if we're to support it 
                                        */


/* Hynix MLC flashes, same infos as Samsung, except the 5th Byte */
#define HYNIX_HY27UT088G2A    0xD3
#define HYNIX_H27UAGT2M       0xD5

/* Number of Planes, same as Samsung */

/* Plane Size */
#define HYNIX_5THID_PLANESZ_MASK    0x70
#define HYNIX_5THID_PLANESZ_512Mb    0x00
#define HYNIX_5THID_PLANESZ_1Gb    0x10
#define HYNIX_5THID_PLANESZ_2Gb    0x20
#define HYNIX_5THID_PLANESZ_4Gb    0x30
#define HYNIX_5THID_PLANESZ_8Gb    0x40
#define HYNIX_5THID_PLANESZ_RSVD1    0x50
#define HYNIX_5THID_PLANESZ_RSVD2    0x60
#define HYNIX_5THID_PLANESZ_RSVD3    0x70


/*--------- END Hynix MLC NAND flashes -----------------------*/

/*--------- Toshiba  NAND----------------------------------*/
#define FLASHTYPE_TOSHIBA          0x98
#define TOSHIBA_TC58NVG0S3ETA00    0xD1
#define TOSHIBA_TC58NVG1S3ETA00    0xDA

/*----------END Toshiba  NAND------------------------------*/

//Micron flashes
#define FLASHTYPE_MICRON        0x2C
//MT29F2G(08/16)AAB
#define MICRON_MT29F2G08AAB     0xDA
#define MICRON_MT29F2G16AAB     0xCA

//Spansion flashes
#ifndef FLASHTYPE_SPANSION
    #define FLASHTYPE_SPANSION      0x01
#endif
/* Large Page */
#define SPANSION_S30ML01GP_08   0xF1    //x8 mode
#define SPANSION_S30ML01GP_16   0xC1    //x16 mode
#define SPANSION_S30ML02GP_08   0xDA    //x8 mode
#define SPANSION_S30ML02GP_16   0xCA    //x16 mode
#define SPANSION_S30ML04GP_08   0xDC    //x8 mode
#define SPANSION_S30ML04GP_16   0xCC    //x16 mode

/* Small Page */
#define SPANSION_S30ML512P_08   0x76    //64MB x8 mode
#define SPANSION_S30ML512P_16   0x56    //64MB x16 mode
#define SPANSION_S30ML256P_08   0x75    //32MB x8 mode
#define SPANSION_S30ML256P_16   0x55    //32MB x16 mode
#define SPANSION_S30ML128P_08   0x73    //x8 mode
#define SPANSION_S30ML128P_16   0x53    //x16 mode

//Command Opcode
#define OP_PAGE_READ                0x01000000
#define OP_SPARE_AREA_READ          0x02000000
#define OP_STATUS_READ              0x03000000
#define OP_PROGRAM_PAGE             0x04000000
#define OP_PROGRAM_SPARE_AREA       0x05000000
#define OP_COPY_BACK                0x06000000
#define OP_DEVICE_ID_READ           0x07000000
#define OP_BLOCK_ERASE              0x08000000
#define OP_FLASH_RESET              0x09000000
#define OP_BLOCKS_LOCK              0x0A000000
#define OP_BLOCKS_LOCK_DOWN         0x0B000000
#define OP_BLOCKS_UNLOCK            0x0C000000
#define OP_READ_BLOCKS_LOCK_STATUS  0x0D000000

//NAND flash controller 
#define NFC_FLASHCACHE_SIZE     512

#define BRCMNAND_CTRL_REGS        (KSEG1ADDR(0x10000000 + BCHP_NAND_REVISION))
#define BRCMNAND_CTRL_REGS_END    (KSEG1ADDR(0x10000000 + BCHP_NAND_BLK_WR_PROTECT))


/**
 * brcmnand_state_t - chip states
 * Enumeration for BrcmNAND flash chip state
 */
typedef enum {
    BRCMNAND_FL_READY = FL_READY,
    BRCMNAND_FL_READING = FL_READING,
    BRCMNAND_FL_WRITING = FL_WRITING,
    BRCMNAND_FL_ERASING = FL_ERASING,
    BRCMNAND_FL_SYNCING = FL_SYNCING,
    BRCMNAND_FL_CACHEDPRG = FL_CACHEDPRG,
    FL_UNLOCKING,
    FL_LOCKING,
    FL_RESETING,
    FL_OTPING,
    BRCMNAND_FL_PM_SUSPENDED = FL_PM_SUSPENDED,
    BRCMNAND_FL_EXCLUSIVE,    // Exclusive access to NOR flash, prevent all NAND accesses.
    BRCMNAND_FL_XIP,            // Exclusive access to XIP part of the flash
    BRCMNAND_FL_WRITING_OOB
} brcmnand_state_t;

//#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
/*
 * ECC levels, corresponding to BCHP_NAND_ACC_CONTROL_ECC_LEVEL
 */
typedef enum {
    BRCMNAND_ECC_DISABLE     = 0u,
    BRCMNAND_ECC_BCH_1        = 1u,
    BRCMNAND_ECC_BCH_2        = 2u,
    BRCMNAND_ECC_BCH_3        = 3u,
    BRCMNAND_ECC_BCH_4        = 4u,
    BRCMNAND_ECC_BCH_5        = 5u,
    BRCMNAND_ECC_BCH_6        = 6u,
    BRCMNAND_ECC_BCH_7        = 7u,
    BRCMNAND_ECC_BCH_8        = 8u,
    BRCMNAND_ECC_BCH_9        = 9u,
    BRCMNAND_ECC_BCH_10        = 10u,
    BRCMNAND_ECC_BCH_11        = 11u,
    BRCMNAND_ECC_BCH_12        = 12u,
    BRCMNAND_ECC_RESVD_1    = 13u,
    BRCMNAND_ECC_RESVD_2    = 14u,
    BRCMNAND_ECC_HAMMING    = 15u,
} brcmnand_ecc_level_t;

/*
 * Number of required ECC bytes per 512B slice
 */
static const unsigned int brcmnand_eccbytes[16] = {
    [BRCMNAND_ECC_DISABLE]    = 0,
    [BRCMNAND_ECC_BCH_1]    = 2,
    [BRCMNAND_ECC_BCH_2]    = 4,
    [BRCMNAND_ECC_BCH_3]    = 5,
    [BRCMNAND_ECC_BCH_4]    = 7,
    [BRCMNAND_ECC_BCH_5]    = 9,
    [BRCMNAND_ECC_BCH_6]    = 10,
    [BRCMNAND_ECC_BCH_7]    = 12,
    [BRCMNAND_ECC_BCH_8]    = 13,
    [BRCMNAND_ECC_BCH_9]    = 15,
    [BRCMNAND_ECC_BCH_10]    = 17,
    [BRCMNAND_ECC_BCH_11]    = 18,
    [BRCMNAND_ECC_BCH_12]    = 20,
    [BRCMNAND_ECC_RESVD_1]    = 0,
    [BRCMNAND_ECC_RESVD_2]    = 0,
    [BRCMNAND_ECC_HAMMING]    = 3,
};

/*
    The lock needs to be maintained at the driver level, because otherwise we 
    could be acessing one device (using CMD_EXT_ADDRESS) while we are accessing 
    the other one
*/
struct brcmnand_driver{
    spinlock_t          driver_lock;    
    wait_queue_head_t   wq;
    brcmnand_state_t    state;
};
/**
 * struct brcmnand_chip - BrcmNAND Private Flash Chip Data
 * @param xor_invert_val    [INTERN] Indicates XOR inversion value
 * @param base        [BOARDSPECIFIC] address to access Broadcom NAND controller
 * @param chipsize    [INTERN] the size of one chip for multichip arrays
 * @param device_id    [INTERN] device ID
 * @param verstion_id    [INTERN] version ID
 * @param options    [BOARDSPECIFIC] various chip options. They can partly be set to inform brcmnand_scan about
 * @param erase_shift    [INTERN] number of address bits in a block
 * @param page_shift    [INTERN] number of address bits in a page
 * @param ppb_shift    [INTERN] number of address bits in a pages per block
 * @param page_mask    [INTERN] a page per block mask
 * @cellinfo:            [INTERN] MLC/multichip data from chip ident
 * @param readw        [REPLACEABLE] hardware specific function for read short
 * @param writew    [REPLACEABLE] hardware specific function for write short
 * @param command    [REPLACEABLE] hardware specific function for writing commands to the chip
 * @param wait        [REPLACEABLE] hardware specific function for wait on ready
 * @param read_bufferram    [REPLACEABLE] hardware specific function for BufferRAM Area
 * @param write_bufferram    [REPLACEABLE] hardware specific function for BufferRAM Area
 * @param read_word    [REPLACEABLE] hardware specific function for read register of BrcmNAND
 * @param write_word    [REPLACEABLE] hardware specific function for write register of BrcmNAND
 * @param scan_bbt    [REPLACEALBE] hardware specific function for scaning Bad block Table
 * @param chip_lock    [INTERN] spinlock used to protect access to this structure and the chip
 * @param wq        [INTERN] wait queue to sleep on if a BrcmNAND operation is in progress
 * @param state        [INTERN] the current state of the BrcmNAND device
 * @param autooob    [REPLACEABLE] the default (auto)placement scheme
 * @param bbm        [REPLACEABLE] pointer to Bad Block Management
 * @param priv        [OPTIONAL] pointer to private chip date
 */
struct brcmnand_chip {
    unsigned long        regs;    /* Register page */
    unsigned char __iomem        *vbase; /* Virtual address of start of flash */
    unsigned long         pbase; // Physical address of vbase
    unsigned long        device_id;
    
    unsigned long       nandConfigReg;      //Nand Configuration Register Address
    unsigned long       nandAccControlReg;  //Nand AccControl Register Address
    unsigned long       nandTiming1Reg;     //Nand Timing1 Register Address
    unsigned long       nandTiming2Reg;     //Nand Timing2 Register Address
    
    unsigned long       xor_invert_val;
    
    uint8_t* pLocalPage; 
    uint8_t* pLocalOob;
    uint8_t* pEccMask; // Will be initialized during probe

    
    //THT: In BrcmNAND, the NAND controller  keeps track of the 512B Cache
    // so there is no need to manage the buffer ram.
    //unsigned int        bufferram_index;
    //struct brcmnand_bufferram    bufferram;

    int (*command)(struct mtd_info *mtd, int cmd, loff_t address, size_t len);
    int (*wait)(struct mtd_info *mtd, int state, uint32_t* pStatus);
    /*
    int (*read_bufferram)(struct mtd_info *mtd,
            unsigned char *buffer, int offset, size_t count);
    int (*write_bufferram)(struct mtd_info *mtd,
            const unsigned char *buffer, int offset, size_t count);
    */
    
    unsigned short (*read_word)(void __iomem *addr);
    void (*write_word)(unsigned short value, void __iomem *addr);

    // THT: Sync Burst Read, not supported.
    //void (*mmcontrol)(struct mtd_info *mtd, int sync_read);

    // Private methods exported from BBT
    int (*block_bad)(struct mtd_info *mtd, loff_t ofs, int getchip);    
    int (*block_markbad)(struct mtd_info *mtd, loff_t ofs);
    int (*scan_bbt)(struct mtd_info *mtd);
    int (*erase_blocks)(struct mtd_info *mtd, struct erase_info *instr, int allowbbt);


    uint32_t (*ctrl_read) (uint32_t command);
    void (*ctrl_write) (uint32_t command, uint32_t val);
    void (*ctrl_writeAddr)(struct brcmnand_chip* chip, L_OFF_T addr, int cmdEndAddr);

    /* THT: Private methods exported to BBT */
    int (*read_raw)(struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen);
    int (*read_pageoob)(struct mtd_info *mtd, L_OFF_T offset,
        u_char* oob_buf, int* retooblen, struct nand_oobinfo *oobsel, int autoplace, int raw);
    int (*write_is_complete)(struct mtd_info *mtd, int* outp_needBBT);

    int (*read_page_oob)(struct mtd_info *mtd, uint8_t* outp_oob, uint32_t page);
    int (*write_page_oob)(struct mtd_info *mtd,  const uint8_t* inp_oob, uint32_t page);
    /*
     * THT: Same as the mtd calls with same name, except that locks are 
     * expected to be already held by caller.  Mostly used by BBT codes
     */
    int (*read_oob) (struct mtd_info *mtd, loff_t from,
             struct mtd_oob_ops *ops);
    int (*write_oob) (struct mtd_info *mtd, loff_t to,
             struct mtd_oob_ops *ops);


    /* mfi: Private methods exported to poll/interrupt/tasklet:*/
    int (*verify_ecc) (struct brcmnand_chip* chip, int state, uint32_t intr);
    unsigned int (*deduce_blockOffset)(struct mtd_info* mtd, L_OFF_T offset, 
        L_OFF_T* pBlockOffset);
    int (*refresh_block)(struct mtd_info* mtd, 
                            const unsigned char* pData, 
                            const unsigned char* pOob, 
                            unsigned int nDataPageNumber, 
                            L_OFF_T blockOffset,
                            bool bReportErr);
                            
    int (*process_error)(struct mtd_info* mtd,
                            uint8_t* buffer, 
                            uint8_t* oobarea, 
                            int ecc);
                            
    int (*process_read_isr_status)(struct brcmnand_chip *this, 
                                    uint32_t intr_status);
                                    
    int (*process_write_isr_status)(struct brcmnand_chip *this, 
                                    uint32_t intr_status, 
                                    int* outp_needBBT);
    
    void (*hamming_calc_ecc)(uint8_t *data, uint8_t *ecc_code);
    void (*hamming_code_fix)(struct mtd_info *mtd, L_OFF_T offset, int raw);
    
    int (*EDU_read)(uint32_t buffer, uint32_t EDU_ldw);
    int (*EDU_read_raw)(uint32_t buffer, uint32_t EDU_ldw);
    int (*EDU_write)(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);
    uint32_t (*EDU_wait_for_completion)(void);
    int (*EDU_read_and_wait)(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);
    void* (*EDU_read_get_data)(uint32_t virtual_addr_buffer);

    int (*ctrl_write_is_complete)(struct mtd_info *mtd, int* outp_needBBT);
    int (*convert_oobfree_to_fsbuf)(struct mtd_info* mtd, u_char* raw_oobbuf, u_char* out_fsbuf, int fslen, struct nand_oobinfo* oobsel, int autoplace);


    int (*read_raw_cache)(struct mtd_info* mtd, void* buffer, uint8_t* oobarea, L_OFF_T offset, int ignoreBBT);

    int (*read_page)(struct mtd_info* mtd, void* buffer, uint8_t* oobarea, L_OFF_T offset, int len, int* pDataRead, int* pOobRead);

    int (*write_page)(struct mtd_info* mtd, int startSlice, void* buffer, uint8_t* oobarea, L_OFF_T offset, int len, int padlen);


    
    // THT Used in lieu of struct nand_ecc_ctrl ecc;
    brcmnand_ecc_level_t ecclevel;    // ECC scheme
    int        eccmode; // Always NAND_ECC_HW3_512
    int        eccsize; // Size of the ECC block, always 512 for Brcm Nand Controller
    int        eccbytes; // How many bytes are used for ECC per eccsize (3 for Brcm Nand Controller)
    int        eccsteps; // How many ECC block per page (4 for 2K page, 1 for 512B page
    int        eccOobSize; // # of oob byte per ECC steps, always 16
    

    int (*write_with_ecc)(struct mtd_info *mtd, loff_t to, size_t len,
                        size_t *retlen, const u_char *buf,
                        u_char *eccbuf, struct nand_oobinfo *oobsel);

    unsigned int        chipSize;
    unsigned int        numchips; // Always 1 in v0.0 and 0.1, up to 8 in v1.0+
    int                 directAccess;        // For v1,0+, use directAccess or EBI address    

    int                 CS[MAX_NAND_CS_PER_BRCMNAND_CHIP];    

    unsigned int        chip_shift; // How many bits shold be shifted.
    L_OFF_T            mtdSize;    // Total size of NAND flash, 64 bit integer for V1.0

    /* THT Added */
    unsigned int         busWidth, pageSize, blockSize; /* Actually page size from chip, as reported by the controller */

    unsigned int        erase_shift;
    unsigned int        page_shift;
    int                phys_erase_shift;    
    int                bbt_erase_shift;
    //unsigned int        ppb_shift;    /* Pages per block shift */
    unsigned int        page_mask;
    //int                subpagesize;
    uint8_t            cellinfo;

    u_char*    data_buf;
    u_char*    oob_buf;
    int        oobdirty;
    u_char        *data_poi;
    unsigned int    options;
    int        badblockpos;
    
    //unsigned long    chipsize;
    int        pagemask;
    int        pagebuf;
    struct nand_oobinfo    *ecclayout;
    struct page_layout_item *layout;
    int layout_allocated;

    uint8_t        *bbt;
    int (*isbad_bbt)(struct mtd_info *mtd, loff_t ofs, int allowbbt);
    struct nand_bbt_descr    *bbt_td;
    struct nand_bbt_descr    *bbt_md;
    struct nand_bbt_descr    *badblock_pattern;

    void                *priv;

    struct nand_statistics stats;
};




/*
 * Options bits
 */
#define BRCMNAND_CONT_LOCK        (0x0001)


extern void brcmnand_prepare_reboot(void);

/*
 * @ mtd        The MTD interface handle from opening "/dev/mtd<n>" or "/dev/mtdblock<n>"
 * @ buff        Buffer to hold the data read from the NOR flash, must be able to hold len bytes, and aligned on
 *            word boundary.
 * @ offset    Offset of the data from CS0 (on NOR flash), must be on word boundary.
 * @ len        Number of bytes to be read, must be even number.
 *
 * returns 0 on success, negative error codes on failure.
 *
 * The caller thread may block until access to the NOR flash can be granted.
 * Further accesses to the NAND flash (from other threads) will be blocked until this routine returns.
 * The routine performs the required swapping of CS0/CS1 under the hood.
 */
extern int brcmnand_readNorFlash(struct mtd_info *mtd, void* buff, unsigned int offset, int len);

#endif
