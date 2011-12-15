/*
 * drivers/mtd/brcmnand/brcmnand_base.c
 *
 *  Copyright (c) 2005-2009 Broadcom Corp.
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
 * 051011    tht    codings derived from onenand_base implementation.
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <linux/reboot.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <mtd/nand_utils.h>
#include <mtd/brcmnand_oob.h> /* BRCMNAND controller defined OOB */

#include <asm/errno.h>
#include <asm/io.h>
#include <asm/bug.h>

#include <asm/delay.h>


#include "brcmnand_priv.h"
#include "cfe_xiocb.h"

#define PRINTK(...)

#define my_be32_to_cpu(x) be32_to_cpu(x)

#include "edu.h"

// Prototypes
#include "eduproto.h"

#define        NAND_DESCRIPTION_STRING_LENGTH    512
char        str_nandinfo[NAND_DESCRIPTION_STRING_LENGTH];
int        geneccerror_test;
int        verbose_test = 0;


/* sort total number of ecc bits corrected in terms of 1 ,2, 3 bits error corrected  */
unsigned long g_numeccbits_corrected_cnt[BRCMNAND_ECC_BCH_8];
unsigned long g_total_ecc_bits_corrected_cnt;
unsigned long g_prev_total_bits_corrected_cnt;

extern void brcmnand_pageapi_init (struct mtd_info* mtd, edu_configuration_t eduConf);

extern int brcmnand_read_page(struct mtd_info* mtd, 
        L_OFF_T offset, u_char* data_buf, int len, u_char* oob_buf, 
        struct nand_oobinfo* oobsel, int* retlen, int* retooblen,
        bool bPerformRefresh);

extern int brcmnand_write_page (struct mtd_info *mtd, L_OFF_T offset, int len,
    u_char* data_buf, u_char* oob_buf);

extern int brcmnand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen);

extern eduTaskletData_t gEduTaskletData;


//Forward Declarations:
int brcmnand_get_statistics_bbt_code (struct mtd_info *mtd);
static uint32_t cnt_zeros(uint8_t* data, int dataSize);

static int brcmnand_reboot( struct notifier_block *nb, unsigned long val, void *v);

static int brcmnand_erase_block(struct mtd_info* mtd, L_OFF_T addr, int bDoNotMarkBad);

static u_char* brcmnand_prepare_oobbuf (
        struct mtd_info *mtd, const u_char *fsbuf, int fslen, u_char* oob_buf, struct nand_oobinfo *oobsel,
        int autoplace, int* retlen);

static int tr_write_oob (struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);
static int tr_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);

static uint32_t brcmnand_ctrl_read(uint32_t nandCtrlReg) ;
static void brcmnand_ctrl_write(uint32_t nandCtrlReg, uint32_t val);

static int brcmnand_set_bbt_code (struct mtd_info *mtd, loff_t ofs, loff_t abs_ofs, unsigned int code);
static int brcmnand_get_bbt_code (struct mtd_info *mtd, loff_t offs);
static int brcmnand_gen_ecc_error(struct mtd_info *mtd, struct nand_generate_ecc_error_command *command);
static int brcmnand_get_statistics (struct mtd_info *mtd, struct nand_statistics *stats);
static int brcmnand_gen_ecc_error_test_with_pattern(struct mtd_info *mtd, 
                                      struct nand_generate_ecc_error_command *command);
static int brcmnand_write_page_oob(struct mtd_info *mtd, 
               const uint8_t* inp_oob, uint32_t page);

static int brcmnand_allocate_block(struct mtd_info* mtd);
static void brcmnand_free_block(struct mtd_info* mtd);
static int brcmnand_read_block(struct mtd_info* mtd, L_OFF_T blockAddr);
static int brcmnand_write_block(struct mtd_info* mtd, L_OFF_T blockAddr);

static int brcmnand_save_block(struct mtd_info* mtd, loff_t ofs);
static int brcmnand_restore_block(struct mtd_info* mtd, loff_t ofs);
static int brcmnand_disable_ecc(struct mtd_info* mtd);
static int brcmnand_enable_ecc(struct mtd_info* mtd);


static bool g_bGenEccErrorTest = false;

struct brcmnand_driver thisDriver;

int gdebug=0;
bool runningInA7601A = false;
char brcmNandMsg[50];    //Buffer for [phys=00000000:XXXXXXXX, logic=00000000:XXXXXXXX] address format

// Whether we should clear the BBT to fix a previous error.
/* This will eventually be on the command line, to allow a user to 
 * clean the flash
 */
extern int gClearBBT;

/* Number of NAND chips, only applicable to v1.0+ NAND controller */
extern int gNumNand;

/* The Chip Select [0..7] for the NAND chips from gNumNand above, only applicable to v1.0+ NAND controller */
extern int* gNandCS;

#define DRIVER_NAME    "brcmnand"

#define HW_AUTOOOB_LAYOUT_SIZE        32 /* should be enough */

#define BRCMNAND_BOARD_7601A               0x74430000

uint32_t EDU_ldw;

static uint8_t** ppRefreshData = NULL;
static uint8_t** ppRefreshOob  = NULL;

#if defined(CONFIG_MTD_BRCMNAND_EDU_POLL) 
    #define EDU_CONFIGURATION EDU_POLL
#elif defined(CONFIG_MTD_BRCMNAND_EDU_INTR)
    #define EDU_CONFIGURATION EDU_INTR
#elif defined(CONFIG_MTD_BRCMNAND_EDU_TASKLET)
    #define EDU_CONFIGURATION EDU_TASKLET
#else
    #define EDU_CONFIGURATION EDU_INVALID
#endif

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0

#define BCHP_NAND_ACC_CONTROL_0_HAMMING        (BRCMNAND_ECC_HAMMING << BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_SHIFT)
#define BCHP_NAND_ACC_CONTROL_N_HAMMING        (BRCMNAND_ECC_HAMMING << BCHP_NAND_ACC_CONTROL_ECC_LEVEL_SHIFT)

#define BCHP_NAND_ACC_CONTROL_0_BCH_4        (BRCMNAND_ECC_BCH_4 << BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_SHIFT)
#define BCHP_NAND_ACC_CONTROL_N_BCH_4        (BRCMNAND_ECC_BCH_4 << BCHP_NAND_ACC_CONTROL_ECC_LEVEL_SHIFT)

#define BCHP_NAND_ACC_CONTROL_0_BCH_8        (BRCMNAND_ECC_BCH_8 << BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_SHIFT)
#define BCHP_NAND_ACC_CONTROL_N_BCH_8        (BRCMNAND_ECC_BCH_8 << BCHP_NAND_ACC_CONTROL_ECC_LEVEL_SHIFT)

#define BCHN_THRESHOLD(_n) (_n - 1) 


/*
 * ID options
 */
#define BRCMNAND_ID_HAS_BYTE3        0x00000001
#define BRCMNAND_ID_HAS_BYTE4        0x00000002
#define BRCMNAND_ID_HAS_BYTE5        0x00000004

#define BRCMNAND_ID_EXT_BYTES \
    (BRCMNAND_ID_HAS_BYTE3|BRCMNAND_ID_HAS_BYTE4|BRCMNAND_ID_HAS_BYTE5)

#endif //CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0


typedef struct brcmnand_chip_Id {
    uint8 mafId, chipId;
    const char* chipIdStr;
    uint32_t size;
    uint32 options;
    uint32_t idOptions;    // Whether chip has all 5 ID bytes
    uint32 timing1, timing2; // Specify a non-zero value to override the default timings.
} brcmnand_chip_Id;

/*
 * List of supported chip
 */
static brcmnand_chip_Id brcmnand_chips[] = {
    {    /* 0 */
        .chipId = SAMSUNG_K9F1G08U0A,
        .mafId = FLASHTYPE_SAMSUNG,
        .chipIdStr = "Samsung K9F1G08U0A/B/C",
        .options = NAND_USE_FLASH_BBT,         /* Use BBT on flash */
                //| NAND_COMPLEX_OOB_WRITE    /* Write data together with OOB for write_oob */
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x4353523B, //worst case is K9F1G08U0A
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x3242444f,
        .timing2 = 0x00000fc4,
#endif        
    },
    {    /* 1 */
        .chipId = NUMONYX_NAND01GW3B,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "ST NAND01GW3B2B/C",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x3353523B, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x33632226,
        .timing2 = 0x00000676,
#endif        
    },
    {    /* 2 */
        .chipId = NUMONYX_NAND512W3A,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "ST NUMONYX_NAND512W3A",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_64MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x43535234, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x32622226,
        .timing2 = 0x00000676,
#endif        
    },
    {    /* 3 */
        .chipId = NUMONYX_NAND256W3A,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "ST NUMONYX_NAND256W3A",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_32MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
        .timing1 = 0, //0x6474555f, 
        .timing2 = 0, //0x00000fc7,
    },
    {    /* 4 */
        .chipId = HYNIX_HY27UF081G2A,
        .mafId = FLASHTYPE_HYNIX,
        .chipIdStr = "Hynix HY27UF081G2A",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x4363523B, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0,
        .timing2 = 0,
#endif        

    },
    {    /* 5 ==>This device needs to be after the HYNIX_HY27UF081G2A 
                    in this table. */ 
        .chipId = HYNIX_H27U1G8F2B,
        .mafId = FLASHTYPE_HYNIX,
        .chipIdStr = "Hynix H27U1G8F2B",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0,
        .timing2 = 0,
#endif        

    },


    {    /* 6 */
        .chipId = HYNIX_HY27UF082G2B,
        .mafId = FLASHTYPE_HYNIX,
        .chipIdStr = "Hynix HY27UF082G2B",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_256MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
        .timing1 = 0x33535238, .timing2 = 0x80000B78,
    },
    {    /* 7 */
         .chipId = HYNIX_HY27UF084G2B, //HYNIX_HY27UF084G2M,         
        .mafId = FLASHTYPE_HYNIX,
         .chipIdStr = "Hynix HY27UF084G2B",
         .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_512MB,
         .options = NAND_USE_FLASH_BBT,
         .idOptions = 0,

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
         .timing1 = 0x33535238,
         .timing2 = 0x80000B98,
#else 
         .timing1 = 0,
         .timing2 = 0, 
#endif
    }, 

    {    /* 8 */
        .chipId = SAMSUNG_K9F2G08U0A,
        .mafId = FLASHTYPE_SAMSUNG,
        .chipIdStr = "Samsung K9F2G08U0A",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_256MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
        .timing1 = 0, .timing2 = 0,
    },

    {    /* 9 - 4K-page */
        .chipId = SAMSUNG_K9F8G08U0M,
        .mafId = FLASHTYPE_SAMSUNG,
        .chipIdStr = "Samsung K9F8G08U0M",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_1GB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
        .timing1 = 0x4353523B, 
        .timing2 = 0x80000B78,
    },

    {    /* 10 */
        .chipId = NUMONYX_NAND08GW3B,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "ST NAND08GW3B",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_1GB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x2252322b, 
        .timing2 = 0x00000675,
#endif        
    },
    {   /* 11 */
       .chipId = SAMSUNG_K9F4G08U0B,
       .mafId = FLASHTYPE_SAMSUNG,
       .chipIdStr = "Samsung K9F4G08U0B/M",
       .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_512MB,
       .options = NAND_USE_FLASH_BBT,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x22323228, 
       .timing2 = 0x00000674,
#endif        

              
    },
    {    /* 12 */
        .chipId = NUMONYX_NAND04GW3B,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "ST NAND04GW3B",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_512MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x22223228, 
        .timing2 = 0x00000674,
#endif        
    },
    {    /* 13 */
        .chipId = NUMONYX_NAND02GW3B,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "ST NAND02GW3B",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_256MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B78,
#else        
        .timing1 = 0x0, 
        .timing2 = 0x0,        
#endif
    },    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
 //MLC chips:         
    {    /* 14 */
        .chipId = SAMSUNG_K9G8G08U0B,
        .mafId = FLASHTYPE_SAMSUNG,
        .chipIdStr = "Samsung MLC K9G8G08U0A/B",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_1GB,
        .options = NAND_USE_FLASH_BBT,         /* Use BBT on flash */
        .idOptions = BRCMNAND_ID_EXT_BYTES,
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B98,
    },
    {    /* 15 */
        .chipId = NUMONYX_NAND08GW3C,
        .mafId = FLASHTYPE_NUMONYX,
        .chipIdStr = "NUMONYX MLC NAND08GW3C",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_1GB,
        .options = NAND_USE_FLASH_BBT,         /* Use BBT on flash */
        .idOptions = BRCMNAND_ID_EXT_BYTES,
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B98,
    },
    {    /* 16 */
        .chipId = SAMSUNG_K9GAG08U0M,
        .mafId = FLASHTYPE_SAMSUNG,
        .chipIdStr = "Samsung MLC K9GAG08U0M",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_2GB,
        .options = NAND_USE_FLASH_BBT,         /* Use BBT on flash */
        .idOptions = BRCMNAND_ID_EXT_BYTES,
        .timing1 = 0x33535238, 
        .timing2 = 0x80000B98,
    },
    {    /* 17 */
         /* using 108Mhz clock  timing */
        .chipId = TOSHIBA_TC58NVG1S3ETA00, 
        .mafId  = FLASHTYPE_TOSHIBA,
        .chipIdStr = "TOSHIBA SLC TC58NVG1S3ETA00",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_256MB,
        .options = NAND_USE_FLASH_BBT,         /* Use BBT on flash */
        .idOptions = 0,
        .timing1 = 0x33535233,
        .timing2 = 0x80000B98,
    },
    {    /* 18 */
         /* using 108Mhz clock  timing */
        .chipId = TOSHIBA_TC58NVG0S3ETA00, 
        .mafId  = FLASHTYPE_TOSHIBA,
        .chipIdStr = "TOSHIBA SLC TC58NVG0S3ETA00",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB,
        .options = NAND_USE_FLASH_BBT,         /* Use BBT on flash */
        .idOptions = 0,
        .timing1 = 0x33535233,
        .timing2 = 0x80000B98,
    },
    {    /* 19 */
         .chipId =  HYNIX_H27UAGT2M, 
        .mafId = FLASHTYPE_HYNIX,
         .chipIdStr = "Hynix MLC H27UAGT2M",
         .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_2GB,
         .options = NAND_USE_FLASH_BBT,
         .idOptions = BRCMNAND_ID_EXT_BYTES,
         .timing1 = 0x33535238,
         .timing2 = 0x80000B98,
    }, 

#endif    
    {    /* LAST DUMMY ENTRY */
        .chipId = 0,
        .mafId = 0,
        .chipIdStr = "UNSUPPORTED NAND CHIP",
        .size = BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB,
        .options = NAND_USE_FLASH_BBT,
        .idOptions = 0,
        .timing1 = 0, 
        .timing2 = 0,
    },


};


// Max chip account for the last dummy entry
#define BRCMNAND_MAX_CHIPS (ARRAY_SIZE(brcmnand_chips) - 1)



/*
 * This function return "true" if it is a 7601A and FALSE for all the other case!
 *
 */
static void is_this_a_7601A(void)
{
    int version;

    version = *(volatile int *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PROD_REVISION);

    printk(KERN_INFO "Product Revision 0x%08X ", version); 

    if(version == BRCMNAND_BOARD_7601A)
    {
        printk(KERN_INFO "This Board is a 7601A. The ECC fix will apply.\n");
            runningInA7601A = true;
    }
    else
    {
        runningInA7601A = false;
    }

}

/**
 * brcmnand_procfileread - Allow user to read the /proc/nandinfo file
 */
int brcmnand_procfileread(char *page, char **start, off_t off,
                                  int count, int *eof, void *data)
{
    int len = 0, i;
    struct brcmnand_chip* this = ((struct mtd_info*)data)->priv;
    uint32_t nand_timing1 = this->ctrl_read(this->nandTiming1Reg);
    uint32_t nand_timing2 = this->ctrl_read(this->nandTiming2Reg);

    if (off > 0)
    {
        *eof = 1;
        return 0;
    }
    
    len += sprintf(page, "%s", str_nandinfo);
    len += sprintf(page + len, "NAND ECClevel:%d CorrThres:%d CS0-T1:0x%x  CS0-T2:0x%x\n\n", 
                   this->ecclevel, BCHN_THRESHOLD(this->ecclevel),  nand_timing1,  nand_timing2);

    len += sprintf(page + len, "%s", "Nand stats on CS0: \n");
    brcmnand_get_statistics_bbt_code((struct mtd_info*)data);
    len += sprintf(page + len, "Bad Block due to wear   :%2d Bad Block due to wear upon reset   :%2d\n", this->stats.badBlocksDueToWearNow, this->stats.badBlocksDueToWearBefore);
    len += sprintf(page + len, "Factory Marked Bad Block:%2d Factory Marked Bad Block upon reset:%2d\n", this->stats.factoryMarkedBadBlocksNow, this->stats.factoryMarkedBadBlocksBefore);
    len += sprintf(page + len, "Reserved Block          :%2d Reserved Block upon reset          :%2d\n", this->stats.reservedBlocksNow, this->stats.reservedBlocksBefore);
    len += sprintf(page + len, "Good Block              :%2d Good Block upon reset              :%2d\n", this->stats.goodBlocksNow, this->stats.goodBlocksBefore);
    len += sprintf(page + len, "Total Number of blocks  :%2d\n", this->stats.goodBlocksNow + this->stats.badBlocksDueToWearNow + this->stats.factoryMarkedBadBlocksNow + this->stats.reservedBlocksNow);
    len += sprintf(page + len, "EccCorrErr since reset  :%2d\n", this->stats.eccCorrectableErrorsNow);
    len += sprintf(page + len, "Total EccCorrEcc bits   :%2lu\n" , g_total_ecc_bits_corrected_cnt);
    len += sprintf(page + len, "CorrEccBitErrStat       : ");
    for(i=0; i < BRCMNAND_ECC_BCH_8 ;i++)
    {
        if(g_numeccbits_corrected_cnt[i] != 0)
            len += sprintf(page + len, "%2dbit-%lu ", i + 1, g_numeccbits_corrected_cnt[i]);
    }
    len += sprintf(page + len, "\nEccUNCorrErr since reset:%2d\n", this->stats.eccUncorrectableErrorsNow);
    len += sprintf(page + len, "OOBEccErr since reset   :%2d\n", this->stats.eccBytesErrors);
    len += sprintf(page + len, "Block Refresh on read since reset:%2d\n", this->stats.blockRefreshed);

    return len;
}                                           

/**
 * brcmnand_createprocfile - Create a file with nand information in /proc/nandinfo
 */
static void brcmnand_createprocfile(struct mtd_info *mtd)
{
    static struct proc_dir_entry *proc_entry;
    proc_entry = create_proc_entry("nandinfo", 0644, NULL);
    if(proc_entry == NULL)
    {
        printk(KERN_INFO "Unable to create /proc/nandinfo\n");
    }
    else
    {
        proc_entry->read_proc = brcmnand_procfileread;
        proc_entry->write_proc = NULL;
        proc_entry->data = (void *) mtd;
        proc_entry->owner = THIS_MODULE;
    }
}


static void  brcmnand_hamming_code_fix_partial_page(
    struct mtd_info *mtd, 
    L_OFF_T offset, 
    int slice, 
    int raw)
{    
    struct brcmnand_chip* this = mtd->priv;
    uint32_t acc, acc0;
    int dataRead, oobRead;
    uint8_t* p8;
    uint8_t ecc0[HAMMING_CODE_LEN]; // Manually calculated
    
    uint8_t*   pTmpPage = NULL; 
        // Please note: 
        //  1- The Nand Controller has a bug, it is badly correcting the data part when the ecc correctable
        //     error is in the ECC bytes
        //  2- We MUST read again the Nand in order to get the RAW data. Instead of just reading the cache
        //     after having disabled the RD_EN_ECC! This is a another bug according to Irvine. 
        //  3- We found out that we must read again the flash because the oobarea variable is often NULL
        //     when this routine is called. This is the only way we can see if the bit error is located 
        //     in the ECC Hamming bits.
        //  
        // Here is the principles used in this fix:
        // When an ECC correctable occur, the data may be corrucpted or the ECC bytes.
        // If the data was corrupted, it was corrected correctly and the Software ECC calculated will match
        // the hardware ECC calculated.
        // If the ECC bytes has the bit error, the data will be wrongly corrected. Then, if we calculate the 
        // the ECC by software, there will be a mismatch on Software ECC calculated and Hardware ECC calculated.
        // This explains how we know if the bit error is in the ECC bytes or in the data.
        // If the Ecc correctable error was in the ECC bytes, we give back the RAW data to the calling application
        // If the Ecc coorectable error was in the data, we give back the corrected data as usual.
        //
        // You should also note that this function will be called recursively. This is why the refresh block
        // will be successful.
        
    //Re-read the oob & page:
    this->read_page(mtd, this->pLocalPage, this->pLocalOob, offset, mtd->writesize, &dataRead, &oobRead);

    // Calculate Hamming Codes
    this->hamming_calc_ecc(&this->pLocalPage[slice * mtd->eccsize], ecc0);    
        
    p8 = (uint8_t*) &this->pLocalOob[slice * this->eccOobSize];
    // Compare ecc0 against ECC from HW
    if (ecc0[0] != p8[6] || ecc0[1] != p8[7] || ecc0[2] != p8[8]) 
    {
        printk(KERN_DEBUG "SW ECC: 0x%02X 0x%02X 0x%02X HW ECC: 0x%02X 0x%02X 0x%02X\n", 
                         ecc0[0], ecc0[1], ecc0[2], p8[6], p8[7], p8[8]);
     

        printk(KERN_DEBUG "ECC Software Fix Running on 7601A\n");

        //Allocate temporary page:
        pTmpPage = kmalloc (this->pageSize, GFP_KERNEL);
        
        if(pTmpPage)
        {
            this->stats.eccBytesErrors++;
            // Error was in ECC bytes, correction made by HW is NOT good send uncorrected data.
            /* Disable ECC */

            acc = brcmnand_ctrl_read(this->nandAccControlReg);

            acc0 = acc & 
               ~(BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK | 
                     BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK);
                     
            brcmnand_ctrl_write(this->nandAccControlReg, acc0);

            // Reread the uncorrected buffer.
            this->read_page(mtd, pTmpPage, NULL, offset, mtd->writesize,
                    &dataRead, &oobRead);
            //Copy the data into this->pLocalPage:
            memcpy(&this->pLocalPage[slice * mtd->eccsize], 
                &pTmpPage[slice * mtd->eccsize],
                 mtd->eccsize);
                 
            // Calculate the good Hamming Codes
            this->hamming_calc_ecc (&pTmpPage[slice * mtd->eccsize], ecc0);             
            // Restore acc
            brcmnand_ctrl_write(this->nandAccControlReg, acc);    
            
            // Since error was in ECC send back the good ECC
            if (p8) 
            {
                p8[6] = ecc0[0];
                p8[7] = ecc0[1];
                p8[8] = ecc0[2];
            } 
            
            //free temporary page:
            kfree(pTmpPage);
            pTmpPage = NULL;
            
            printk(KERN_DEBUG "CORR error was handled properly by SW\n");
        }
        else
        {
            printk(KERN_ERR"Cannot allocate pTmpData\n");
        }
                  
        
    }
    else 
    { 
        // Error was in Data, send corrected data
        // buffer variable is already good
        printk(KERN_DEBUG "SW ECC: 0x%02X 0x%02X 0x%02X HW ECC: 0x%02X 0x%02X 0x%02X\n", 
                          ecc0[0], ecc0[1], ecc0[2], p8[6], p8[7], p8[8]);          
        printk(KERN_DEBUG "CORR error was handled by HW\n");
    }
}

static void  brcmnand_hamming_code_fix(struct mtd_info *mtd, L_OFF_T offset, int raw)
{
    int i;
    struct brcmnand_chip* this = mtd->priv;
    
    if(this->ecclevel == BRCMNAND_ECC_HAMMING && runningInA7601A == true)
    {
        for (i = 0; i < this->eccsteps; i++)
        {//run it for all ecc slices:
            brcmnand_hamming_code_fix_partial_page(mtd, offset, i, raw);
        }
    }
}

// Please note that this routine was change a bit. Byte we already swapped so no need to use be32_to_cpu() call
static void  brcmnand_hamming_calc_ecc(uint8_t *data, uint8_t *ecc_code)
{
    unsigned long i_din[128]; 
    uint32_t* p32 = (uint32_t*) data; 

    int i,j;
    uint8_t o_ecc[24],temp[10];
    unsigned long pre_ecc;

    for (i=0; i<128; i++) {
        i_din[i] = p32[i];
    }

    memset(o_ecc, 0, sizeof(o_ecc));

    for(i=0;i<128;i++){
        memset(temp, 0, sizeof(temp));
        
        for(j=0;j<32;j++){
            temp[0]^=((i_din[i]& 0x55555555)>>j)&0x1;
            temp[1]^=((i_din[i]& 0xAAAAAAAA)>>j)&0x1;
            temp[2]^=((i_din[i]& 0x33333333)>>j)&0x1;
            temp[3]^=((i_din[i]& 0xCCCCCCCC)>>j)&0x1;
            temp[4]^=((i_din[i]& 0x0F0F0F0F)>>j)&0x1;
            temp[5]^=((i_din[i]& 0xF0F0F0F0)>>j)&0x1;
            temp[6]^=((i_din[i]& 0x00FF00FF)>>j)&0x1;
            temp[7]^=((i_din[i]& 0xFF00FF00)>>j)&0x1;
            temp[8]^=((i_din[i]& 0x0000FFFF)>>j)&0x1;
            temp[9]^=((i_din[i]& 0xFFFF0000)>>j)&0x1;
        }

        for(j=0;j<10;j++)
            o_ecc[j]^=temp[j];
            
        //o_ecc[0]^=temp[0];//P1'
        //o_ecc[1]^=temp[1];//P1
        //o_ecc[2]^=temp[2];//P2'
        //o_ecc[3]^=temp[3];//P2
        //o_ecc[4]^=temp[4];//P4'
        //o_ecc[5]^=temp[5];//P4
        //o_ecc[6]^=temp[6];//P8'
        //o_ecc[7]^=temp[7];//P8
        //o_ecc[8]^=temp[8];//P16'
        //o_ecc[9]^=temp[9];//P16
               
        if(i%2){
            for(j=0;j<32;j++)
                o_ecc[11]^=(i_din[i]>>j)&0x1;//P32
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[10]^=(i_din[i]>>j)&0x1;//P32'
        }
                
        if((i&0x3)<2){
            for(j=0;j<32;j++)
                o_ecc[12]^=(i_din[i]>>j)&0x1;//P64'
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[13]^=(i_din[i]>>j)&0x1;//P64
        }
        
        if((i&0x7)<4){
            for(j=0;j<32;j++)
                o_ecc[14]^=(i_din[i]>>j)&0x1;//P128'
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[15]^=(i_din[i]>>j)&0x1;//P128
        }
        
        if((i&0xF)<8){
            for(j=0;j<32;j++)
                o_ecc[16]^=(i_din[i]>>j)&0x1;//P256'
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[17]^=(i_din[i]>>j)&0x1;//P256
        }
        
        if((i&0x1F)<16){
            for(j=0;j<32;j++)
                o_ecc[18]^=(i_din[i]>>j)&0x1;//P512'
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[19]^=(i_din[i]>>j)&0x1;//P512
        }
        
        if((i&0x3F)<32){
            for(j=0;j<32;j++)
                o_ecc[20]^=(i_din[i]>>j)&0x1;//P1024'
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[21]^=(i_din[i]>>j)&0x1;//P1024
        }
        
        if((i&0x7F)<64){
            for(j=0;j<32;j++)
                o_ecc[22]^=(i_din[i]>>j)&0x1;//P2048'
        }
        else{
            for(j=0;j<32;j++)
                o_ecc[23]^=(i_din[i]>>j)&0x1;//P2048
        }
        // print intermediate value
        pre_ecc = 0;
        for(j=23;j>=0;j--) {
            pre_ecc = (pre_ecc << 1) | (o_ecc[j] ? 1 : 0 ); 
        }

    }
   
    ecc_code[0] = (o_ecc[ 7]<<7 | o_ecc[ 6]<<6 | o_ecc[ 5]<<5 | o_ecc[ 4]<<4 | o_ecc[ 3]<<3 | o_ecc[ 2]<<2 | o_ecc[ 1]<<1 | o_ecc[ 0]);
    ecc_code[1] = (o_ecc[15]<<7 | o_ecc[14]<<6 | o_ecc[13]<<5 | o_ecc[12]<<4 | o_ecc[11]<<3 | o_ecc[10]<<2 | o_ecc[ 9]<<1 | o_ecc[ 8]);
    ecc_code[2] = (o_ecc[23]<<7 | o_ecc[22]<<6 | o_ecc[21]<<5 | o_ecc[20]<<4 | o_ecc[19]<<3 | o_ecc[18]<<2 | o_ecc[17]<<1 | o_ecc[16]);
  }


/*=============================================================================
Description: This function deduces the block offset from the offset that it
             receives
             
Parameters:  mtd:          IN:  the mtd_info structure that concerns this device
             offset:       IN:  offset
             pBlockOffset: OUT: block Offset
             
Returns:     nPageNumber:  The number of the page where the data is going

=============================================================================*/

static unsigned int brcmnand_deduce_blockOffset(
    struct mtd_info* mtd, 
    L_OFF_T offset, 
    L_OFF_T* pBlockOffset)
{
    unsigned int nPageNumber;

    struct brcmnand_chip *this = mtd->priv;

    unsigned int block_size = this->blockSize;
    unsigned int l_baseOffset = __ll_low(offset);
    unsigned int l_offset = l_baseOffset;
    
    *pBlockOffset = __ll_sub32(offset, l_offset);
    
    l_offset &= ~(block_size - 1);
    *pBlockOffset = __ll_add32(*pBlockOffset, l_offset);

    nPageNumber = l_baseOffset - l_offset;
    nPageNumber /= this->pageSize;

    return nPageNumber;
}

/*=============================================================================
Description: This function refreshes a block into flash.  It is called when
             we find an ECC correctable error.  
             
             a) read the entire block
             b) erase the entire block
             c) re-write the entire block
             
Parameters:  mtd:          IN:  the mtd_info structure that concerns this device
             blockOffset:  IN:  block offset

             
Returns:     Nothing


=============================================================================*/
static int brcmnand_refresh_block(
    struct mtd_info* mtd,
    const unsigned char* pData,
    const unsigned char* pOob,
    unsigned int nDataPageNumber, 
    L_OFF_T blockOffset,
    bool bReportErr)
{
    int ret;
    struct brcmnand_chip *this = mtd->priv;    
    
    ret = brcmnand_save_block(mtd, blockOffset);
    


    if ((ret != 0)&&(bReportErr))
    {
        printk(KERN_ERR"%s: Cannot save the block\n", __FUNCTION__);
        goto closeall;
    }
    else
    { 
        //Modify data (if need be):
        if (pData != NULL)
        {
            printk(KERN_DEBUG "Replacing data in ppRefreshData[%d]\n" ,nDataPageNumber);
            memcpy(ppRefreshData[nDataPageNumber], pData, this->pageSize);
        }
        
        if (pOob != NULL)
        {
            printk(KERN_DEBUG "Replacing oob in ppRefreshOob[%d]\n", nDataPageNumber);
            memcpy(ppRefreshOob[nDataPageNumber], pOob, mtd->oobsize);
        }        
    
        //brcmnand_restore_block erases then puts ppRefreshData & ppRefreshBlock into it        
        ret = brcmnand_restore_block(mtd, blockOffset);
        if ((ret != 0)&&(bReportErr))
        {
            printk(KERN_ERR"%s: Cannot restore the block\n", __FUNCTION__);
            goto closeall;
        }
    }

closeall:
    brcmnand_free_block(mtd);
    return ret;
}

static uint32_t brcmnand_ctrl_read(uint32_t nandCtrlReg) 
{
    volatile unsigned long* pReg = (volatile unsigned long*) (BRCMNAND_CTRL_REGS 
        + nandCtrlReg - BCHP_NAND_REVISION);

#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_BLK_WR_PROTECT ||
        (nandCtrlReg & 0x3) != 0) {
        printk(KERN_ERR "%s: Invalid register value %08x\n", __FUNCTION__, nandCtrlReg);
    }
#else
    if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_TIMING_2_CS3 ||
        (nandCtrlReg & 0x3) != 0) {
        printk(KERN_ERR "%s: Invalid register value %08x\n", __FUNCTION__, nandCtrlReg);
    }

#endif    
if (gdebug > 3) printk("%s: CMDREG=%08x val=%08x\n", __FUNCTION__, (unsigned int) nandCtrlReg, (unsigned int)*pReg);
    return (uint32_t) /*be32_to_cpu*/(*pReg);
}


static void brcmnand_ctrl_write(uint32_t nandCtrlReg, uint32_t val) 
{
    volatile unsigned long* pReg = (volatile unsigned long*) (BRCMNAND_CTRL_REGS 
        + nandCtrlReg - BCHP_NAND_REVISION);
        
#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_BLK_WR_PROTECT ||
        (nandCtrlReg & 0x3) != 0) {
        printk(KERN_ERR "%s: Invalid register value %08x\n", __FUNCTION__, nandCtrlReg);
    }
#else
    if (nandCtrlReg < BCHP_NAND_REVISION || nandCtrlReg > BCHP_NAND_TIMING_2_CS3 ||
        (nandCtrlReg & 0x3) != 0) {
        printk(KERN_ERR "%s: Invalid register value %08x\n", __FUNCTION__,  nandCtrlReg);
    }
#endif
    
    *pReg = (volatile unsigned long)/* cpu_to_be32*/(val);
if (gdebug > 3) printk("%s: CMDREG=%08x val=%08x\n", __FUNCTION__, nandCtrlReg, val);
}


/*
 * this: BRCM NAND handle
 * offset: offset from start of mtd
 * cmdEndAddr: 1 for CMD_END_ADDRESS, 0 for CMD_ADDRESS
 */
static void brcmnand_ctrl_writeAddr(struct brcmnand_chip* this, L_OFF_T offset, int cmdEndAddr) 
{
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    uint32_t pAddr = offset + this->pbase;

    this->ctrl_write(cmdEndAddr? BCHP_NAND_CMD_END_ADDRESS: BCHP_NAND_CMD_ADDRESS, pAddr);

#else
    uint32_t udw, ldw, cs;
    DIunion chipOffset;
    
    // cs is the index into this->CS[]
    cs = (uint32_t) __ll_RightShift(offset, this->chip_shift);

    // chipOffset is offset into the current CS
    chipOffset.ll = __ll_and32(offset, this->chipSize - 1);

    if (cs >= this->numchips) {
        printk(KERN_ERR "%s: Offset=%s outside of chip range cs=%d, this->CS[cs]=%d\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val), cs, this->CS[cs]);
        BUG();
        return;
    }

if (gdebug) printk("CS=%d, pthis->CS[cs]=%d\n", cs, this->CS[cs]);

    // ldw is lower 32 bit of chipOffset, need to add pbase when on CS0
    if (this->CS[cs] == 0) {
        ldw = chipOffset.s.low + this->pbase;
        ldw ^= this->xor_invert_val;
    }
    else {
        ldw = chipOffset.s.low;
    }

    EDU_ldw = ldw;
 
    udw = chipOffset.s.high | (this->CS[cs] << 16);

if (gdebug > 3) printk("%s: offset=%s  cs=%d ldw = %08x, udw = %08x\n", __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val), cs,  ldw, udw);
    this->ctrl_write(cmdEndAddr? BCHP_NAND_CMD_END_ADDRESS: BCHP_NAND_CMD_ADDRESS, ldw);
    this->ctrl_write(BCHP_NAND_CMD_EXT_ADDRESS, udw);
#endif
}


static void print_config_regs(struct mtd_info* mtd)
{
    struct brcmnand_chip* this = mtd->priv;

    unsigned long flash_id          = this->device_id;    
    
    unsigned long nand_acc_control  = brcmnand_ctrl_read(this->nandAccControlReg);
    unsigned long nand_config       = brcmnand_ctrl_read(this->nandConfigReg);
    unsigned long nand_timing1      = brcmnand_ctrl_read(this->nandTiming1Reg);
    unsigned long nand_timing2      = brcmnand_ctrl_read(this->nandTiming2Reg);
    
    printk(KERN_INFO "Found NAND: ACC=%08lx, cfg=%08lx, flashId=%08lx, tim1=%08lx, tim2=%08lx\n", 
        nand_acc_control, nand_config, flash_id, nand_timing1, nand_timing2);    
}


#if 1
void print_oobbuf(const unsigned char* buf, int len)
{
int i;


if (!buf) {printk("NULL"); return;}
 for (i=0; i<len; i++) {
  if (i % 16 == 0) printk("\n");
  else if (i % 4 == 0) printk(" ");
  printk("%02x", buf[i]);
 }
 printk("\n");
}

void print_databuf(const unsigned char* buf, int len)
{
int i;


 for (i=0; i<len; i++) {
  if (i % 32 == 0) printk("\n%04x: ", i);
  else if (i % 4 == 0) printk(" ");
  printk("%02x", buf[i]);
 }
 printk("\n");
}

#endif

/*
 * BRCMNAND controller always copies the data in 4 byte chunk, and in Big Endian mode
 * from and to the flash.
 * This routine assumes that dest and src are 4 byte aligned, and that len is a multiple of 4
 (Restriction removed)

 * TBD: 4/28/06: Remove restriction on count=512B, but do restrict the read from within a 512B section.
 * Change brcmnand_memcpy32 to be 2 functions, one to-flash, and one from-flash,
 * enforcing reading from/writing to flash on a 4B boundary, but relaxing on the buffer being on 4 byte boundary.
 */
 typedef union {
     uint32_t u;
     char c[4];
} u32_t;


static int brcmnand_to_flash_memcpy32(struct brcmnand_chip* this, L_OFF_T offset, const void* src, int len)
{
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    u_char* flash = this->vbase + offset;
#else
    u_char* flash = this->vbase;
#endif
    //int i;
    //uint32_t* pDest = (uint32_t*) flash;
    //uint32_t* pSrc = (uint32_t*) src;
    
    if (unlikely((unsigned long) flash & 0x3)) {
        printk(KERN_ERR "brcmnand_memcpy32 dest=%p not DW aligned\n", flash);
        return -EINVAL;
    }
    if (unlikely((unsigned long) src & 0x3)) {
        printk(KERN_ERR "brcmnand_memcpy32 src=%p not DW aligned\n", src);
        return -EINVAL;
    }
    if (unlikely(len & 0x3)) {
        printk(KERN_ERR "brcmnand_memcpy32 len=%d not DW aligned\n", len);
        return -EINVAL;
    }

#if 1
    // Use memcpy to take advantage of Prefetch
if (gdebug) printk("%s: flash=%p, len=%d\n", __FUNCTION__, flash, len);
    memcpy(flash, src, len);
#else
    for (i=0; i< (len>>2); i++) {
        pDest[i] = /* THT: 8/29/06 cpu_to_be32  */ (pSrc[i]);
    }
#endif

    return 0;
}

/*
 * Returns     0: No errors
 *             1: Correctable error
 *            -1: Uncorrectable error
 */
static int brcmnand_EDU_verify_ecc(struct brcmnand_chip* this, int state, uint32_t intr)
{
    int err = 1;       //  1 is no error, 2 is ECC correctable, 3 is EDU ECC correctable, -2 is ECC non-corr, -3 is EDU ECC non-corr
    //uint32_t intr;
    uint32_t status = 0;
    uint32_t addr, extAddr;
//static int prvErr = 1;
    /* Only make sense on read */
    if (state != FL_READING) 
        return 0;
        
    //Get EDU_ERR_STATUS:
    status = edu_volatile_read(EDU_BASE_ADDRESS + EDU_ERR_STATUS);

    if (intr & BCHP_HIF_INTR2_CPU_STATUS_EDU_ERR_INTR_MASK)
    {
        err = BRCMEDU_DMA_TRANSFER_ERROR;
    }
    
    if (intr & BCHP_HIF_INTR2_CPU_STATUS_NAND_CORR_INTR_MASK)
    {
        //Read where:
        addr = this->ctrl_read(BCHP_NAND_ECC_CORR_ADDR);
        extAddr = this->ctrl_read(BCHP_NAND_ECC_CORR_EXT_ADDR);

        if(status & EDU_ERR_STATUS_NandECCcor) 
        {
            printk(KERN_DEBUG"%s: NAND CORRECTABLE @%08X, tasklet offset: %08X\n", 
                 __FUNCTION__, addr, (int)gEduTaskletData.offset);
            err = BRCMEDU_CORRECTABLE_ECC_ERROR;
        }
        else
        {
            printk(KERN_DEBUG"%s: NAND CORRECTABLE @%08X (but no EDU error), tasklet offset: %08X\n",
                 __FUNCTION__, addr, (int)gEduTaskletData.offset);
            err = BRCMEDU_CORRECTABLE_ECC_ERROR;
        }
    
    }
    
    if (intr & BCHP_HIF_INTR2_CPU_STATUS_NAND_UNC_INTR_MASK)
    {
        //Read where:
        addr = this->ctrl_read(BCHP_NAND_ECC_UNC_ADDR);
        extAddr = this->ctrl_read(BCHP_NAND_ECC_UNC_EXT_ADDR);
        
        if(status & EDU_ERR_STATUS_NandECCuncor)
        {
#if 0  //No need to display this message if this is a false alarm (e.g. erased block).
       //brcmnand_process_error() takes care of displaying a message if there a true error.       
            printk(KERN_DEBUG"%s: NAND UNCORRECTABLE @%08X, tasklet offset: %08X\n", 
                __FUNCTION__, addr, (int)gEduTaskletData.offset);
#endif                
            err = BRCMEDU_UNCORRECTABLE_ECC_ERROR;
        }
        else
        {
            printk(KERN_DEBUG"%s:NAND UNCORRECTABLE @%08X (but no EDU error), tasklet offset: %08X\n",
                __FUNCTION__, addr, (int)gEduTaskletData.offset);
            err = BRCMEDU_UNCORRECTABLE_ECC_ERROR;
        }
    }

    if(status & EDU_ERR_STATUS_NandRBUS)
    {
        err = BRCMEDU_MEM_BUS_ERROR;
        printk(KERN_DEBUG"EDU_ERR_STATUS=%08x BUS ERROR\n", status);
    }
/*    
    if ((err == BRCMEDU_DMA_TRANSFER_ERROR)&&(prvErr == BRCMEDU_CORRECTABLE_ECC_ERROR))
    {
        err = BRCMNAND_TIMED_OUT;
    }
    else 
    {
*/    
        if(err == BRCMEDU_DMA_TRANSFER_ERROR)
        { 
           printk(KERN_DEBUG"%s: EDU_DMA_TRANSFER_ERROR tasklet offset: %08X\n", 
                 __FUNCTION__, (int)gEduTaskletData.offset);
        }
/*        
    }
    prvErr = err;
*/    
    return err;
}

/*
 * Returns     0: No errors
 *             1: Correctable error
 *            -1: Uncorrectable error
 */
static int brcmnand_ctrl_verify_ecc(struct brcmnand_chip* this, int state, uint32_t notUsed)
{
    int err = 1;        //1 is no error, 2 is ECC correctable, 3 is EDU ECC correctable, -2 is ECC non-corr, -3 is EDU ECC non-corr

    uint32_t addr;
    uint32_t extAddr = 0;
    DIunion chipOffset;

    /* Only make sense on read */
    if (state != FL_READING) 
        return 0;

    addr = this->ctrl_read(BCHP_NAND_ECC_CORR_ADDR);
    if (addr) {

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
        extAddr = this->ctrl_read(BCHP_NAND_ECC_CORR_EXT_ADDR);
        // Clear it
        this->ctrl_write(BCHP_NAND_ECC_CORR_EXT_ADDR, 0);
#endif
        // Clear it
        this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);

        chipOffset.s.high = extAddr;
        chipOffset.s.low = addr;

        printk(KERN_WARNING "%s: Correctable ECC error at %s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg, chipOffset.ll, this->xor_invert_val));
        err = BRCMNAND_CORRECTABLE_ECC_ERROR;
    }

    addr = this->ctrl_read(BCHP_NAND_ECC_UNC_ADDR);
    if (addr) {
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
        extAddr = this->ctrl_read(BCHP_NAND_ECC_UNC_EXT_ADDR);
        // Clear it
        this->ctrl_write(BCHP_NAND_ECC_UNC_EXT_ADDR, 0);
#endif
        // Clear it
        this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);

        /*
         * If the block was just erased, and have not yet been written to, this will be flagged,
         * so this could be a false alarm
         */

        err = BRCMNAND_UNCORRECTABLE_ECC_ERROR;
    }
    return err;
}




/**
 * brcmnand_wait - [DEFAULT] wait until the command is done
 * @param mtd        MTD device structure
 * @param state        state to select the max. timeout value
 *
 * Wait for command done. This applies to all BrcmNAND command
 * Read can take up to 53, erase up to ?s and program up to 30 clk cycle ()
 * according to general BrcmNAND specs
 */
static int brcmnand_wait(struct mtd_info *mtd, int state, uint32_t* pStatus)
{
    struct brcmnand_chip * this = mtd->priv;
    unsigned long timeout;
    uint32_t ready;

    /* The 20 msec is enough */
    timeout = jiffies + msecs_to_jiffies(3000); // THT: 3secs, for now
    while (time_before(jiffies, timeout)) {
        PLATFORM_IOFLUSH_WAR();
        ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

        if (ready & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {
            *pStatus = ready;
            return 0;
        }

        if (state != FL_READING)
            cond_resched();
        //touch_softlockup_watchdog();
    }

    /*
     * Get here on timeout
     */
    return -ETIMEDOUT;
}

#if 0
static int brcmnand_ctrl_ready(struct mtd_info *mtd, int state)
{
    uint32_t status;
    int err;

    err = brcmnand_wait(mtd, state, &status);
    if (!err) {
        return (status & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK);
    }
    return 0;
}
#endif






/* 
 * Returns      1: Success, no errors
 *              2: is ECC correctable
 *              3: is EDU ECC correctable
 *              0: Timeout
 *             -1: Errors
 *             -2: ECC non-corr
 *             -3: is EDU ECC non-corr 
 */
static int brcmnand_spare_is_valid(struct mtd_info* mtd,  int state, int raw) 
{
    struct brcmnand_chip * this = mtd->priv;
    unsigned long timeout;
    int ret = 0; // TIMEOUT

    uint32_t ready, intr_status;

    /* The 20 msec is enough */
    timeout = jiffies + msecs_to_jiffies(3000);  // 3 sec timeout for now
    while (time_before(jiffies, timeout)) {
        PLATFORM_IOFLUSH_WAR();

        ready = this->ctrl_read(BCHP_NAND_INTFC_STATUS);

        if (ready & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) {

            //Read intr status:
            intr_status = edu_volatile_read(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_STATUS);

            /* ECC status is returned only if not reading as raw OOB */
            ret = (raw ? 1 : brcmnand_ctrl_verify_ecc(this, state, intr_status));

            break;
        }

        if (state != FL_READING)
            cond_resched();
    }

    return ret; 
}


static int brcmnand_ctrl_write_is_complete(struct mtd_info *mtd, int* outp_needBBT)
{
    int err;
    uint32_t status;
    uint32_t flashStatus = 0;

    //Let's assume there's an error
    *outp_needBBT = 1;
    
    err = brcmnand_wait(mtd, FL_WRITING, &status);
    
    if (!err) 
    {
        if (status & BCHP_NAND_INTFC_STATUS_CTLR_READY_MASK) 
        {
            flashStatus = status & 0x01;

            *outp_needBBT = flashStatus; 
        }
        return 1; //brcmnand_wait succeeded!
    }
    return 0; //time out!!!
}





#if 0
/* Compare the OOB areas, skipping over the ECC, and BI bytes */

static int ecccmp(struct mtd_info* mtd, L_OFF_T offset, const unsigned char* left, const unsigned char* right, int len)
{
//    struct brcmnand_chip *this = mtd->priv;
    int i, ret=0;
    L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));  // Beginning of slice
    L_OFF_T pageOffset = __ll_and32(offset, ~ (mtd->writesize - 1)); // Beginning of page
    int sliceNum =  __ll_isub(sliceOffset , pageOffset) / mtd->eccsize; // Slice number 0-3
    int oobOffset = sliceNum * mtd->eccsize;

    /* Walk through the autoplace chunks */
    for (i = 0; i < len; i++) {
        if ((left[i] & this->pEccMask[i+oobOffset]) != (right[i] & this->pEccMask[i+oobOffset])) {
            ret = 1;
            break;
        }
    }
    
    return ret;
}
#endif


/**
 * brcmnand_convert_oobfree_to_fsbuf - [GENERIC] Read 1 page's oob buffer into fs buffer
 * @param mtd        MTD device structure
 * @oob_buf            IN: The raw OOB buffer read from the NAND flash
 * @param fsbuf        OUT: the converted file system oob buffer
 * @oob_buf            raw oob buffer read from flash
 * returns the number of bytes converted.
 */
static int brcmnand_convert_oobfree_to_fsbuf(struct mtd_info* mtd, 
    u_char* raw_oobbuf, u_char* out_fsbuf, int fslen, struct nand_oobinfo* oobsel, int autoplace) 
{
    int i;
    int len = 0;

    
    switch(oobsel->useecc) {
    case MTD_NANDECC_AUTOPLACE:
    case MTD_NANDECC_AUTOPL_USR:
        /* Walk through the autoplace chunks */
        for (i = 0; oobsel->oobfree[i][1] && i < ARRAY_SIZE(oobsel->oobfree); i++) {
            int from = oobsel->oobfree[i][0];
            int num = oobsel->oobfree[i][1];
            int clen;

            if (num == 0) break; // Reach end marker
            clen = min_t(int, num, fslen-len);            
            memcpy(&out_fsbuf[len], &raw_oobbuf[from], clen);
            len += clen;
        }
        break;
    case MTD_NANDECC_OFF:
    case MTD_NANDECC_PLACE:
        len = mtd->oobsize;
        memcpy(out_fsbuf, raw_oobbuf, len);
        break;
        
    default:
        printk(KERN_ERR "%s: Invalid useecc %d not supported\n",
            __FUNCTION__, oobsel->useecc);
        len = 0;
    }
    return len;

}

/**
 * brcmnand_prepare_oobbuf - [GENERIC] Prepare the out of band buffer
 * @mtd:    MTD device structure
 * @fsbuf:    buffer given by fs driver (input oob_buf)
 * @fslen:    size of buffer
 * @oobsel:    out of band selection structure
 * @oob_buf:    Output OOB buffer
 * @autoplace:    1 = place given buffer into the free oob bytes
 * @numpages:    number of pages to prepare. The Brcm version only handles 1 page at a time.
 * @retlen:    returned len, i.e. number of bytes used up from fsbuf.
 *
 * Return:
 * 1. Filesystem buffer available and autoplacement is off,
 *    return filesystem buffer
 * 2. No filesystem buffer or autoplace is off, return internal
 *    buffer
 * 3. Filesystem buffer is given and autoplace selected
 *    put data from fs buffer into internal buffer and
 *    retrun internal buffer
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
 * 
 *
*/
static u_char * 
brcmnand_prepare_oobbuf (
        struct mtd_info *mtd, const u_char *fsbuf, int fslen, u_char* oob_buf, 
        struct nand_oobinfo *oobsel, int autoplace, int* retlen)
{
    struct brcmnand_chip *this = mtd->priv;
    int i, len, ofs;

    *retlen = 0;

    /* Zero copy fs supplied buffer */
    if (fsbuf && !autoplace)
    {
        *retlen = fslen;
        return (u_char*) fsbuf;
    }

    /* Check, if the buffer must be filled with ff again */
    memset (oob_buf, 0xff, mtd->oobsize );
    this->oobdirty = 0;

    /* If we have no autoplacement or no fs buffer use the internal one */
    if (!autoplace || !fsbuf)
    {
        *retlen = 0;
        return oob_buf;
    }

    /* Walk through the pages and place the data */
    this->oobdirty = 1;
    ofs = 0;
    
    for (i = 0, len = 0; 
         len < mtd->oobavail && len < fslen && i < ARRAY_SIZE(oobsel->oobfree); 
         i++) 
    {
        int to = ofs + oobsel->oobfree[i][0];
        int num = oobsel->oobfree[i][1];

        if (num == 0) break; // End marker reached
#if 1
        if ((len + num) > fslen) 
        {
            memcpy(&oob_buf[to], &fsbuf[*retlen], fslen-len);
            *retlen += fslen-len;
            break;
        }
#endif            
        memcpy (&oob_buf[to], &fsbuf[*retlen], num);
        len += num;
        *retlen += num;
    }
    ofs += mtd->oobavail;
    
    return oob_buf;
}




/**
 * brcmnand_posted_read_oob - [BrcmNAND Interface] Read the spare area
 * @param mtd        MTD data structure
 * @param oobarea    Spare area, pass NULL if not interested
 * @param offset    offset to read from or write to
 *
 * This is a little bit faster than brcmnand_posted_read, making this command useful for improving
 * the performance of BBT management.
 * The 512B flash cache is invalidated.
 *
 * Read the cache area into buffer.  The size of the cache is mtd->writesize and is always 512B,
 * for this version of the BrcmNAND controller.
 */
static int brcmnand_posted_read_oob(struct mtd_info* mtd, 
        unsigned char* oobarea, L_OFF_T offset, int raw)
{
    struct brcmnand_chip* this = mtd->priv;
    L_OFF_T sliceOffset = __ll_and32(offset, ~(mtd->eccsize - 1));
    int ret = 0;
    int retries = 5, done = 0;
    int ecc, error = 0;
    int i;
    uint32_t* p32 = NULL;

// THT: 080608: SW workaround for NAND controller v3.0
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    uint32_t readCmd = OP_PAGE_READ; // Full page read instead of OOB read.
#else
    uint32_t readCmd = OP_SPARE_AREA_READ;
#endif
    

    while (retries > 0 && !done) 
    {
        if (unlikely(__ll_isub(sliceOffset, offset))) {
            printk(KERN_ERR "%s: offset %s is not cache aligned\n", 
                __FUNCTION__, __ll_sprintf(brcmNandMsg,offset, this->xor_invert_val));
            return -EINVAL;
        }
        //Clear all pending interrupts:
        EDU_CLRI();

        this->ctrl_writeAddr(this, sliceOffset, 0);
        this->ctrl_write(BCHP_NAND_CMD_START, readCmd);

        // Wait until spare area is filled up

        ecc  = brcmnand_spare_is_valid(mtd, FL_READING, raw);
        
        error = this->process_error(mtd, NULL, oobarea, ecc);
        
        if (error == 0)
        {
            done = 1;
        }
        else if (error == -ETIMEDOUT)
        {
            retries--;
        }
        else if ((error == -EECCUNCOR) || (error == -EINVAL))
        {
            ret = -EECCUNCOR;
            done = 1;
        }
        else if (error == -EECCCOR)
        {
            done = 1;
            if (ret != -EECCUNCOR)
            {
                ret = -EECCCOR;
            }
        }

        if (oobarea) {
            p32 = (uint32_t*) oobarea;

            for (i = 0; i < 4; i++) {
                p32[i] = /* THT 11-30-06 */ be32_to_cpu /**/(this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + (i<<2)));
           }
if (gdebug) {printk("%s: offset=%s, oob=\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,sliceOffset, this->xor_invert_val)); print_oobbuf(oobarea, 16);}
        }
     }//End of while

    return ret;
}




/**
 * brcmnand_posted_write_oob - [BrcmNAND Interface] Write the spare area
 * @param mtd        MTD data structure
 * @param oobarea    Spare area, pass NULL if not interested.  Must be able to 
 *                    hold mtd->oobsize (16) bytes.
 * @param offset    offset to write to, and must be 512B aligned
 *
 */
static int brcmnand_posted_write_oob(struct mtd_info *mtd,
        const unsigned char* oobarea, L_OFF_T offset)
{
    struct brcmnand_chip* this = mtd->priv;
    L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
    uint32_t* p32;
    int needBBT=0;
    int i;
    
    
 if (gdebug) printk( "%s: offset:%s \n", __FUNCTION__, __ll_sprintf(brcmNandMsg,offset, this->xor_invert_val));

    if (unlikely(__ll_isub(sliceOffset, offset))) {
        printk(KERN_ERR "%s: offset %s is not cache aligned\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg,offset, this->xor_invert_val));
    }
    
    // assert oobarea here
    BUG_ON(!oobarea);
    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
/*  Fix for PR12424:
    We need to write fake data (0xFF) & the oob when then PARTIAL_PAGE_EN bit is set 
    to 0. ==> This bit is always set to 0 for a 7601XX. 

    We cannot use the EDU to write the data: The EDU always forces ECC to be 
    written, so we must fill the NAND controller's FLASH_CACHE manually.
*/
    p32 = (uint32_t*)this->pLocalPage;
    
    this->ctrl_writeAddr(this, sliceOffset, 0);
    
    brcmnand_to_flash_memcpy32(this, sliceOffset, p32, NAND_CONTROLLER_DATA_BOUNDARY);
    
    p32 = (uint32_t*) oobarea;                                
    
// There's 4 32-bits registers to write to fill 16 oob bytes per 512 data bytes
    for (i = 0; i < 4; i++) {
        this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, /* THT 11-30-06 */ cpu_to_be32 (p32[i]));
    }  
    
    PLATFORM_IOFLUSH_WAR();

//    printk(KERN_DEBUG "****** Workaround, using OP_PROGRAM_PAGE instead of OP_PROGRAM_SPARE_AREA\n");
    this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_PAGE);

// Wait until flash is ready
    if (brcmnand_ctrl_write_is_complete(mtd, &needBBT)) 
    {
       // printk (KERN_DEBUG"%s: return success\n", __FUNCTION__);
        return 0;
    }  
    printk(KERN_ERR"%s: return error!\n", __FUNCTION__);                      
#else
    this->ctrl_writeAddr(this, sliceOffset, 0);

    /* Must write data when NAND_COMPLEX_OOB_WRITE option is set */
    if (this->options & NAND_COMPLEX_OOB_WRITE) {
        p32 = (uint32_t*)&this->pLocalOob[0];
        memset(p32, 0xFF, mtd->oobsize);
        brcmnand_to_flash_memcpy32(this, offset, p32, mtd->eccsize);
    }

    p32 = (uint32_t*) oobarea;                                
    
// There's 4 32-bits registers to write to fill 16 oob bytes per 512 data bytes
    for (i = 0; i < 4; i++) {
        this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, /* THT 11-30-06 */ cpu_to_be32 (p32[i]));
    }

    PLATFORM_IOFLUSH_WAR();
    if (this->options & NAND_COMPLEX_OOB_WRITE) {
        printk(KERN_INFO "****** Workaround, using OP_PROGRAM_PAGE instead of OP_PROGRAM_SPARE_AREA\n");
        this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_PAGE);
    }
    else {
        this->ctrl_write(BCHP_NAND_CMD_START, OP_PROGRAM_SPARE_AREA);
    }

    // Wait until flash is ready
    if (brcmnand_ctrl_write_is_complete(mtd, &needBBT)) 
    {
        printk (KERN_DEBUG "%s: brcmnand_write_is_complete returned success; needBBT=%d\n", __FUNCTION__, needBBT);
        return 0;
    }
    
    printk (KERN_ERR "%s: brcmnand_write_is_complete returned error; needBBT=%d\n", __FUNCTION__, needBBT);
#endif

    if (needBBT){
        int ret;
        //if (gdebug)        
            printk(KERN_WARNING "%s: Marking bad block @ %s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,offset, this->xor_invert_val));
        
        ret = this->block_markbad(mtd, offset);
        return -EINVAL;
    }

    return -ETIMEDOUT;
    
}



/**
 * brcmnand_get_device - [GENERIC] Get chip for selected access
 * @param mtd        MTD device structure
 * @param new_state    the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int brcmnand_get_device(struct mtd_info *mtd, int new_state)
{
//    struct brcmnand_chip *this = mtd->priv;
    DECLARE_WAITQUEUE(wait, current);

    /*
     * Grab the lock and see if the device is available
     */
    while (1) {
        spin_lock(&thisDriver.driver_lock);

        if (thisDriver.state == FL_READY) {
            thisDriver.state = new_state;
            spin_unlock(&thisDriver.driver_lock);
            break;
        }
        if (new_state == FL_PM_SUSPENDED) {
            spin_unlock(&thisDriver.driver_lock);
            return (thisDriver.state == FL_PM_SUSPENDED) ? 0 : -EAGAIN;
        }
        set_current_state(TASK_UNINTERRUPTIBLE);
        add_wait_queue(&thisDriver.wq, &wait);
        spin_unlock(&thisDriver.driver_lock);
        schedule();
        remove_wait_queue(&thisDriver.wq, &wait);
    }

    return 0;
}

static struct brcmnand_chip* 
brcmnand_get_device_exclusive(void)
{
    struct brcmnand_chip *this = (struct brcmnand_chip*) get_brcmnand_handle();
    struct mtd_info *mtd = (struct mtd_info*) this->priv;
    int ret;

    ret = brcmnand_get_device(mtd, BRCMNAND_FL_XIP);
    if (0 == ret)
    {
        printk(KERN_DEBUG"%s: Returning this = 0x%p\n", __FUNCTION__, this);
        return this;
    }
    else
    {
        printk(KERN_DEBUG"%s: Returning this = <NULL>\n",__FUNCTION__);
        return ((struct brcmnand_chip *) NULL);
    }
}




/**
 * brcmnand_release_device - [GENERIC] release chip
 * @param mtd        MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void brcmnand_release_device(struct mtd_info *mtd)
{
 //   struct brcmnand_chip *this = mtd->priv;

    /* Release the chip */
    spin_lock(&thisDriver.driver_lock);
    thisDriver.state = FL_READY;
    wake_up(&thisDriver.wq);
    spin_unlock(&thisDriver.driver_lock);
}

/*
 * Determine whether we should use autoplace
 */
static int 
brcmnand_autoplace(struct mtd_info *mtd, struct nand_oobinfo* oobsel)
{
    //struct brcmnand_chip *this = mtd->priv;
    int autoplace;

    /* if oobsel is NULL, use chip defaults */
    if (oobsel == NULL)
        oobsel = &mtd->oobinfo;

    switch(oobsel->useecc) {
        case MTD_NANDECC_AUTOPLACE:
        case MTD_NANDECC_AUTOPL_USR:
            autoplace = 1;
            break;
        default:
            autoplace = 0;
            break;
        }

    return autoplace;
}

/**
 * brcmnand_read_pageoob - [GENERIC] write to one page's OOB area
 * @mtd:            MTD device structure
 * @offset:         offset  from start of the chip, must be called with (offset & this->pagemask)
 * @len:             size of data buffer, len must be <= mtd->writesize, and can indicate partial page.
 * @data_buf:     data buffer.
 * @oob_buf:        out of band data buffer for 1 page (size = oobsize), must be prepared beforehand
 * @retlen:         Returned length
 * @convert2fs:     Convert oob_buf to fsbuf
 *
 * Nand_page_program function is used for write and writev !
 * This function will always program a full page of data
 * If you call it with a non page aligned buffer, you're lost :)
 *
 * Cached programming is not supported yet.
 */
static int brcmnand_read_pageoob (struct mtd_info *mtd, L_OFF_T offset,
    u_char* oob_buf, int* retooblen, struct nand_oobinfo *oobsel, int autoplace, int raw)
{
    struct brcmnand_chip *this = mtd->priv;
    int oobRead = 0;
    int winslice;
    int ret = 0, retFinal = 0;
    u_char* pOobbuf;

    int nPageNumber;
    L_OFF_T blockRefreshOffset;
    
    int sliceOobOffset;
    L_OFF_T sliceOffset;
    
    L_OFF_T eccCorOffset = offset;
    int eccCorWinslice = 0;

    /* If autoplace, use the scratch buffer */
    if (autoplace) {
        pOobbuf = this->pLocalOob;
    }
    else {
        pOobbuf = oob_buf;
    }

    /*
     * The 4 slice OOB areas may not have the same format, (e.g. for the 2K page, the first slice
     * lose 2 bytes.)
     * We have to prepare the OOB buffer for the entire page, then write it out one slice at a time.
     */
    for (winslice = 0; winslice < this->eccsteps && oobRead < mtd->oobsize; winslice++) {
        sliceOffset = __ll_add32(offset, this->eccsize*winslice);
        sliceOobOffset = this->eccOobSize*winslice;


if (gdebug) printk("winslice=%d, slice=%s, sliceOobOffset=%d\n", 
    winslice, __ll_sprintf(brcmNandMsg, sliceOffset, this->xor_invert_val), sliceOobOffset);

        ret = brcmnand_posted_read_oob(mtd, &pOobbuf[sliceOobOffset], sliceOffset, raw);

        if (ret == -EECCUNCOR) {
            retFinal = ret;
        } 
        else if (ret == -EECCCOR)
        {
            retFinal = ret;
            eccCorOffset = sliceOffset;
            eccCorWinslice = winslice;
            if (retFinal != -EECCUNCOR)
            {
                retFinal = ret;
            }
            ret = 0;
        }
        else if (ret < 0) {
            retFinal = ret;
             printk(KERN_ERR "%s: posted read oob failed at offset=%0llx, ret=%d\n", 
                __FUNCTION__, sliceOffset, ret);
            return ret;
        }
        oobRead += this->eccOobSize;
    }
    if (retFinal == -EECCCOR)
    {
//mfi: added
        L_OFF_T pageAddr = __ll_and32(offset, ~ (mtd->writesize - 1));
        if(this->ecclevel == BRCMNAND_ECC_HAMMING && runningInA7601A == true)
        {
            //Perform refresh block logic at page level:
            printk(KERN_DEBUG"%s: Calling hamming_code_fix\n", __FUNCTION__);
                    
            this->hamming_code_fix(mtd, offset, 0);

            printk(KERN_NOTICE"%s: Performing refresh block logic\n", __FUNCTION__);
                
            nPageNumber = this->deduce_blockOffset(mtd, pageAddr, &blockRefreshOffset);

            ret = this->refresh_block(mtd, this->pLocalPage, this->pLocalOob, nPageNumber, blockRefreshOffset, true);
        }
        else
        {    
            printk(KERN_DEBUG"%s: Performing refresh block logic\n", __FUNCTION__);
                
            nPageNumber = this->deduce_blockOffset(mtd, pageAddr, &blockRefreshOffset);

            ret = this->refresh_block(mtd, NULL, NULL, nPageNumber, blockRefreshOffset, true);
        }
//mfi: end of addition 
    }
 
    *retooblen = oobRead;

    // Convert the free bytes into fsbuffer
    if (autoplace) {

        *retooblen = brcmnand_convert_oobfree_to_fsbuf(mtd, pOobbuf, oob_buf, mtd->oobsize, oobsel, autoplace);
    }
    

    return retFinal;
}

/**
 * brcmnand_read_with_ecc - [MTD Interface] Read data with ECC
 * @param mtd        MTD device structure
 * @param from        offset to read from
 * @param len        number of bytes to read
 * @param retlen    pointer to variable to store the number of read bytes
 * @param buf        the databuffer to put data
 * @param oob_buf    filesystem supplied oob data buffer
 * @param oobsel    oob selection structure
 *
 * BrcmNAND read with ECC
 * returns 0 on success, -errno on error.
 */
static int brcmnand_read_with_ecc(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf,
    u_char *oob_buf, struct nand_oobinfo *oobsel)
{
    int thislen;
    L_OFF_T pageAddr;
    int page;
    int retooblen, retdatalen;
    loff_t startPageOffset;

    struct brcmnand_chip *this = mtd->priv;
    int dataRead = 0, oobRead = 0;
    int ret = 0;
    int retFinal = 0;


    /*
     * this function must be single threaded. 
     * Can't have this large of a buffer on the stack
     */

    DEBUG(MTD_DEBUG_LEVEL3, "%s: from %s, len=%i, autoplace=%d\n", 
                        __FUNCTION__, __ll_sprintf(brcmNandMsg, from, this->xor_invert_val), (int) len, 
                        brcmnand_autoplace(mtd, oobsel));

    *retlen = 0;
    //oobarea = oob_buf;

    /* Do not allow reads past end of device */
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely(from + len) > mtd->size) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(from, len), this->mtdSize))) 
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_ecc: Attempt read beyond end of device\n");
        *retlen = 0;
        return -EINVAL;
    }

    /* Setup variables and oob buffer */
    // We know that pageSize=128k or pageShift = 17 bits for large page, so page fits inside an int
    page = (int) __ll_RightShift(from, this->page_shift);
    startPageOffset = __ll_and32(from, ~(this->pageSize - 1));
    
    while (dataRead < len) {
        u_char* oobbuf;
        L_OFF_T offset;
        int leadingBytes;
        
        thislen = min_t(int, mtd->writesize, len - dataRead);
        if (oob_buf) {
            oobbuf = &oob_buf[oobRead];
        }
        else {
            oobbuf = NULL;
        }

        /* page marks the start of the  block encompassing [from, thislen] */
        offset = __ll_add32(from, dataRead);
        pageAddr = __ll_and32(offset,  ~ (mtd->writesize - 1));

        /* If not at start of page, read up to the next start of page */
        leadingBytes = __ll_isub(offset, pageAddr);
        if (leadingBytes) {
            thislen = min_t(int, len - dataRead, mtd->writesize - leadingBytes);
            retooblen = retdatalen = 0;
            /* Read the entire page into the default buffer */
            ret = brcmnand_read_page(mtd, pageAddr, this->pLocalPage, mtd->writesize, 
                                        oobbuf, oobsel, &retdatalen, &retooblen, 
                                        true);

            if (ret != 0)
            {
                retFinal = ret;
            }
            

            /* Copy from default buffer into actual outbuf */
            memcpy(&buf[dataRead], &this->pLocalPage[leadingBytes], thislen);

        }
        else {
            /* Check thislen against the page size, if not equal, we will have to use the default buffer, again */
            if (thislen == mtd->writesize) 
            {
                /* Read straight into buffer */
                ret = brcmnand_read_page(mtd, pageAddr, &buf[dataRead], thislen, 
                                        oobbuf,  oobsel, &retdatalen, &retooblen,
                                        true);

                if (ret != 0)
                {
                    retFinal = ret;
                }
            }
            else 
            {
                /* thislen < mtd->writesize
                 * This is a partial read: Read into default buffer  
                 * then copy over to avoid writing beyond the end of the input buffer
                 */
                // Here we run the risk of overwriting the OOB input buffer, but the interface does not allow
                // us to know the size of the receiving OOB.
                if (gdebug) printk("%s: pageAddr %s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg, pageAddr, this->xor_invert_val));
                
                ret = brcmnand_read_page(mtd, pageAddr, this->pLocalPage, thislen, 
                                    oobbuf, oobsel, &retdatalen, &retooblen,
                                    true);

                if (ret != 0)
                {
                    retFinal = ret;
                }
                
                memcpy(&buf[dataRead], this->pLocalPage, thislen);
            }

        }
        dataRead += thislen;
        oobRead += retooblen;

        if (dataRead >= len)
            break;
    }



    /*
     * Return success, if no ECC failures, else -EBADMSG
     * fs driver will take care of that, because
     * retlen == desired len and result == -EBADMSG
     */
    *retlen = dataRead;


    return retFinal;
}

static int brcmnand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf,
    u_char *oob_buf, struct nand_oobinfo *oobsel)
{
    int status;
    
    
    /* Grab the lock and see if the device is available */
    /* THT: TBD: Move it inside while loop: It would be less efficient, but
     * would also give less response time to multiple threads
     */
    brcmnand_get_device(mtd, FL_READING);

    status = brcmnand_read_with_ecc(mtd, from, len, retlen, buf, oob_buf, oobsel);
    
    /* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);

    
    return status;
}

/**
 * brcmnand_read - [MTD Interface] MTD compability function for brcmnand_read_ecc
 * @param mtd        MTD device structure
 * @param from        offset to read from
 * @param len        number of bytes to read
 * @param retlen    pointer to variable to store the number of read bytes
 * @param buf        the databuffer to put data
 *
 * This function simply calls brcmnand_read_ecc with oob buffer and oobsel = NULL
*/
static int brcmnand_read(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf)
{
    DEBUG(MTD_DEBUG_LEVEL3, "-->%s from=%08x, len=%d\n", __FUNCTION__,
         (unsigned int) from, (int) len);
    return brcmnand_read_ecc(mtd, from, len, retlen, buf, NULL, NULL);
}





static int brcmnand_read_oob_int(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf, bool withoutEccVerification)
{
    struct brcmnand_chip *this = mtd->priv; // Needed by v1.0, warning unused by < v1.0
    int oobread = 0, thislen;
    //unsigned int page;
    L_OFF_T pageAddr;
    int ret = 0;
    //int raw;
    struct nand_oobinfo place_oob;

    // Read_oob always use place_oob.
    
    place_oob = mtd->oobinfo;
    place_oob.useecc = MTD_NANDECC_PLACE;
    
     DEBUG(MTD_DEBUG_LEVEL3, "%s: from = 0x%08x, len = %i\n", __FUNCTION__,
         (unsigned int) from, (int) len);

    /* Initialize return length value */
    *retlen = 0;

    /*
      * Well, we can't do this test any more since mtd->size may be larger than a 32bit integer can hold
      */
    /* Do not allow reads past end of device */
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely(from + len) > mtd->size) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(from, len), this->mtdSize)))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_oob: Attempt read beyond end of device\n");
        return -EINVAL;
    }

    /* page marks the start of the page block encompassing [from, thislen] */
    //page = ((unsigned long) from & ~ (mtd->writesize - 1)) >> this->page_shift;
    pageAddr = __ll_and32(from, ~ (mtd->writesize - 1));

    /* Make sure we don't go over bound */
    while (oobread < len) {
        
        ret = brcmnand_read_pageoob(mtd, pageAddr, &buf[oobread], &thislen, &place_oob, 0, (int)withoutEccVerification);

        if (ret) {
            printk(KERN_ERR "brcmnand_read_pageoob returns %d, thislen = %d\n", ret, thislen);
        }
        
        
        // Just copy up to thislen
        //memcpy(&buf[oobread], &this->oob_buf, thislen);
        oobread += thislen;
        if (oobread >= len) {
            break;
        }

        if (ret) {
            DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_read_oob: read failed = %d\n", ret);
            goto out;
        }

        //buf += thislen;        
        pageAddr = __ll_add32(pageAddr, mtd->writesize);
        
    }

out:

    *retlen = oobread;

    return ret;
}

/**
 * brcmnand_read_oob - [MTD Interface] BrcmNAND read out-of-band
 * @param mtd        MTD device structure
 * @param from        offset to read from
 * @param len        number of bytes to read
 * @param retlen    pointer to variable to store the number of read bytes
 * @param buf        the databuffer to put data
 *
 * BrcmNAND read out-of-band data from the spare area
 */
static int brcmnand_read_oob(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf)
{
    int ret = 0;

    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_READING);
    
    ret = brcmnand_read_oob_int(mtd, from, len, retlen, buf, true);

/* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);

    return ret;
}


/**
 * brcmnand_read_oobfree - [MTD Interface] NAND read free out-of-band
 * @mtd:    MTD device structure
 * @from:    offset to read from
 * @len:    number of bytes to read
 * @retlen:    pointer to variable to store the number of read bytes
 * @buf:    the databuffer to put data
 *
 * NAND read free out-of-band data from the spare area
 */
static int brcmnand_read_oobfree (struct mtd_info *mtd, 
            loff_t from, size_t len, size_t * retlen, u_char * out_fsbuf)
{

    struct brcmnand_chip *this = mtd->priv;
    int oobread = 0, thislen;
    //unsigned int page;
    L_OFF_T pageAddr;
    int ret = 0;
    int autoplace;


    DEBUG(MTD_DEBUG_LEVEL3, "-->%s from=%08x, len=%d\n", __FUNCTION__,
         (unsigned int) from, (int) len);


    *retlen=0;

    autoplace = 1;

    /* Do not allow reads past end of device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely(from + len) > mtd->size) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(from, len), this->mtdSize)))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt read beyond end of device\n", __FUNCTION__);
        return -EINVAL;
    }

    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_READING);

    /* page marks the start of the page block encompassing [from, thislen] */
    //page = ((unsigned long) from & ~ (mtd->writesize - 1)) >> this->page_shift;
    pageAddr = __ll_and32(from, ~ (mtd->writesize - 1));

    /* Make sure we don't go over bound */
    while (oobread < len) {
        
        ret = brcmnand_read_pageoob(mtd, pageAddr,  &out_fsbuf[*retlen], &thislen, this->ecclayout, autoplace, 1);
    
        if (ret) {
            printk(KERN_ERR "%s: read_oob returns %d\n", __FUNCTION__, ret);
            goto out;
        }

        // Convert to fsbuf, returning # bytes converted (== mtd->oobavail)
        // already done in readpageoob
        // ret = brcmnand_convert_oobfree_to_fsbuf(mtd, this->oob_buf, &fsbuf[oobread], thislen, this->ecclayout, autoplace);

        // Advance to next page
        pageAddr = __ll_add32(pageAddr, mtd->writesize);
        oobread += mtd->oobsize;
        *retlen += thislen;
        //from += mtd->writesize;
    }

    ret = 0;

out:
    brcmnand_release_device(mtd);

if (4 == gdebug) gdebug = 0;
    return ret;
}



#ifdef CONFIG_MTD_BRCMNAND_VERIFY_WRITE

/*
 * Returns 0 on success, 
 */
static int brcmnand_verify_pageoob_priv(struct mtd_info *mtd, loff_t offset, 
    const u_char* fsbuf, int fslen, u_char* oob_buf, int ooblen, struct nand_oobinfo* oobsel, 
    int autoplace, int raw)
{
    int ret = 0;
    int complen;

    if (autoplace) 
    {
        complen = min_t(int, ooblen, fslen);

        /* We may have read more from the OOB area, so just compare the min of the 2 */
        if (complen == fslen) 
        {
            ret = memcmp(fsbuf, oob_buf, complen);
            if (ret) 
            {
                goto comparison_failed;
            }
        }
        else 
        {
printk(KERN_ERR "%s: OOB comparison failed, ooblen=%d is less than fslen=%d\n", 
        __FUNCTION__, ooblen, fslen);
            return  -EBADMSG;
        }
    }
    else
    { // No autoplace.  Skip over non-freebytes

        /* 
         * THT:
         * WIth YAFFS1, the FS codes overwrite an already written chunks quite a lot
         * (without erasing it first, that is!!!!!)
         * For those write accesses, it does not make sense to check the write ops
         * because they are going to fail every time
         *
         * MFI:
         * We are not using YAFFS1 anymore, so we will more than likely uncomment this code,
         * but we need to test it first.
         */
#if 0
        int i, len; 
        
        for (i = 0; oobsel->oobfree[i][1] && i < ARRAY_SIZE(oobsel->oobfree); i++) 
        {
            int from = oobsel->oobfree[i][0];
            int num = oobsel->oobfree[i][1];
            int len1 = num;

            if (num == 0) break; // End of oobsel
            
            if ((from+num) > fslen) 
            {
                len1 = fslen-from;
            }
            ret = memcmp(&fsbuf[from], &oob_buf[from], len1);
            if (ret) 
            {
                printk(KERN_ERR "%s: comparison at offset=%08x, i=%d from=%d failed., num=%d\n", 
                    __FUNCTION__, i, __ll_low(offset), from, num); 
            
            
                if (gdebug > 3) 
                {
                    printk("No autoplace Comparison failed at %08x, ooblen=%d fslen=%d left=\n", 
                        __ll_low(offset), ooblen, fslen);
                    print_oobbuf(&fsbuf[0], fslen);
                    printk("\nRight=\n"); print_oobbuf(&oob_buf[0], ooblen);
                    dump_stack();
                }
                goto comparison_failed;
            }
            if ((from+num) >= fslen) break;
            len += num;
        }
#endif
    }
    return ret;


comparison_failed:
    {
//        printk(KERN_ERR "Comparison Failed\n");
//        print_diagnostics();
        return -EBADMSG;
    }
}


/**
 * brcmnand_verify_page - [GENERIC] verify the chip contents after a write
 * @param mtd        MTD device structure
 * @param dbuf        the databuffer to verify
 * @param dlen        the length of the data buffer, and should be less than mtd->writesize
 * @param fsbuf        the length of the file system OOB data and should be exactly
 *                             mtd-->oobavail (for autoplace) or mtd->oobsize otherise
  *                    bytes to verify.
 *
 * Assumes that lock on.  Munges the internal data and OOB buffers.
 */
//#define MYDEBUG
static int brcmnand_verify_page(struct mtd_info *mtd, loff_t addr, const u_char *dbuf, int dlen, 
        u_char* fsbuf, int fslen, struct nand_oobinfo* oobsel, int autoplace
        )
{
    struct brcmnand_chip *this = mtd->priv;
    /*
     * this function must be single threaded. 
     * Can't have this large of a buffer on the stack
     */
//    static u_char this->pLocalPage[MAX_PAGE_SIZE];
//    u_char this->pLocalOob [MAX_OOB_SIZE];
    int ret = 0;
    int ooblen=0, datalen=0;
    //int complen;
    u_char* oobbuf = (fsbuf && fslen > 0) ? this->pLocalOob : NULL;
    
if (gdebug) printk("%s: oobbuf=%p\n", __FUNCTION__, oobbuf);

    // Must read entire page
    ret = brcmnand_read_page(mtd, addr, this->pLocalPage, mtd->writesize, oobbuf, 
                                oobsel, &datalen, &ooblen, true);

    if (ret) {
        if (gdebug) printk(KERN_ERR "%s: brcmnand_read_page at %s failed ret=%d\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg,addr, this->xor_invert_val), ret);
        return ret;
    }
if (gdebug) printk("%s: fsdlen=%d, fslen=%d, datalen=%d, ooblen=%d\n", __FUNCTION__, 
    dlen, fslen, datalen, ooblen);

    /* 
     * If there are no Input filesytem buffer, there is nothing to verify, 
     * reading the page and checking the ECC status
     * flag should be enough
     */
    if (!fsbuf || fslen <= 0)
        return 0;
    
    // Verify the filesytem buffer:
    ret = brcmnand_verify_pageoob_priv(mtd, addr, fsbuf, fslen, this->pLocalOob, ooblen, oobsel, autoplace, 0);
    if (ret) {
    //Reread:
//        ret = brcmnand_read_page(mtd, addr, this->pLocalPage, mtd->writesize, oobbuf, oobsel, &datalen, &ooblen);
    //Reverify:
//        ret = brcmnand_verify_pageoob_priv(mtd, addr, fsbuf, fslen, this->pLocalOob, ooblen, oobsel, autoplace, 0);
//        if (ret) 
        {
            printk(KERN_ERR "Autoplace Comparison failed at %s, ooblen=%d fslen=%d Written=\n", 
                __ll_sprintf(brcmNandMsg, addr, this->xor_invert_val), ooblen, fslen);
            print_oobbuf(fsbuf, fslen);
    
            printk(KERN_ERR "\nRead=\n"); 
            print_oobbuf(oobbuf, ooblen);

            return ret;
        }
    }

    // Verify dlen bytes in dbuf:
    ret = memcmp(this->pLocalPage, dbuf, dlen);

    return ret;
}

/**
 * brcmnand_verify_pageoob - [GENERIC] verify the chip contents after a write
 * @param mtd        MTD device structure
 * @param dbuf        the databuffer to verify
 * @param dlen        the length of the data buffer, and should be less than mtd->writesize
 * @param fsbuf        the file system OOB data 
 * @param fslen        the length of the file system buffer
 * @param oobsel        Specify how to write the OOB data
 * @param autoplace    Specify how to write the OOB data
 * @param raw        Ignore the Bad Block Indicator when true
 *
 * Assumes that lock on.  Munges the OOB internal buffer.
 */
static int brcmnand_verify_pageoob(struct mtd_info *mtd, loff_t addr, const u_char* fsbuf, int fslen,
        struct nand_oobinfo *oobsel, int autoplace, int raw)
{
    struct brcmnand_chip *this = mtd->priv;
    //u_char* data_buf = this->data_buf;
//    static u_char this->pLocalOob[MAX_OOB_SIZE]; // = this->this->pLocalOob;
    int ret = 0;
    //int complen;
    //char tmpfsbuf[]; // Max oob size we support.
    int ooblen = 0;

if(gdebug) printk("-->%s addr=%s, fslen=%d, autoplace=%d, raw=%d\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,addr, this->xor_invert_val),
    fslen, autoplace, raw);

    // Must read entire page
    ret = brcmnand_read_pageoob(mtd, addr, this->pLocalOob, &ooblen, oobsel, autoplace, raw);

    if (ret) {
        printk(KERN_ERR "%s: brcmnand_read_page_oob at %s failed ret=%d\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg,addr, this->xor_invert_val), ret);
        return ret;
    }

if(gdebug) printk("%s: Calling verify_pageoob_priv(addr=%s, fslen=%d, ooblen=%d\n", 
    __FUNCTION__, __ll_sprintf(brcmNandMsg,addr, this->xor_invert_val), fslen, ooblen);
    ret = brcmnand_verify_pageoob_priv(mtd, addr, fsbuf, fslen, this->pLocalOob, ooblen, oobsel, autoplace, raw);

    return ret;
}



#else
#define brcmnand_verify_page(...)    (0)
#define brcmnand_verify_pageoob(...)        (0)
//#define brcmnand_verify_oob(...)        (0)
#endif

#define NOTALIGNED(x)    ((x & (mtd->writesize - 1)) != 0)






/**
 * brcmnand_write_pageoob - [GENERIC] write to one page's OOB area
 * @mtd:    MTD device structure
 * @offset:     offset  from start of the chip, must be called with (offset & this->pagemask)
 * @len: size of data buffer, len must be <= mtd->writesize, and can indicate partial page.
 * @data_buf: data buffer.
 * @oob_buf:    out of band data buffer for 1 page (size = oobsize), must be prepared beforehand
 *
 * Nand_page_program function is used for write and writev !
 * This function will always program a full page of data
 * If you call it with a non page aligned buffer, you're lost :)
 *
 * Cached programming is not supported yet.
 */

static int brcmnand_write_pageoob (struct mtd_info *mtd, L_OFF_T offset, int len,
    const u_char* inp_oob)
{
    struct brcmnand_chip *chip = (struct brcmnand_chip*) mtd->priv;
    int winslice;
    int oobWritten = 0;
    int ret = -EINVAL;

    bool bReportErr = true;

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    uint8_t* localOob;
    localOob = NULL;

    {
        //Read oob area: 
        bool bRefresh = false, bBadBlock = false, bEccEnabled = true;
        
        L_OFF_T blockOffset;
        int retlen;
        int i, y, z;
        unsigned int nPageNumber, acc_control;
        int result;

        localOob = kmalloc(mtd->oobsize, GFP_KERNEL);  

        if (localOob == NULL)
        {
            printk(KERN_ERR "%s: Out of memory for allocating localOob for readback",
                    __FUNCTION__);
            goto out;
        }
        memset(localOob, 0xFF, mtd->oobsize);
            
        bRefresh = false;

        //Check if WR_ECC_EN is set:
        acc_control = chip->ctrl_read(chip->nandAccControlReg);
        if ((acc_control & BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK) == 0)
        {
            bEccEnabled = false;
        } 
        result = brcmnand_read_oob_int(mtd, offset, mtd->oobsize, &retlen, localOob, true);
 
        //We don't care about the result of brcmnand_read_oob_int() if the
        //oob to be written has the bad block indicator:
        for (z = 0;  (mtd->ecclayout->bbindicator[z].length != 0); z++)
        {
            int zoffset = mtd->ecclayout->bbindicator[z].offset;
            int zlen = mtd->ecclayout->bbindicator[z].length;

            for (y = zoffset; y < zoffset + zlen; y++)
            {            
                if (inp_oob[y] != 0xFF)
                {
                    bBadBlock = true;
//                    printk("%s: We are marking a BB!\n", __FUNCTION__);
                }
            }
        }
    
        
        if ( (result == 0) || (bBadBlock ==  true))
        {
                //Check for ECC bytes != 0xFF:
                for (i = 0; i < mtd->ecclayout->eccbytes; i++)
                {
                    if(localOob[mtd->ecclayout->eccpos[i]] != 0xFF)
                    {
                        bRefresh = true;
                        printk(KERN_DEBUG"%s: We are refreshing the block!\n", __FUNCTION__);
                        break; //out of the 'for' loop
                    }
                }
                if ((bRefresh == true)&&(bEccEnabled == true))
                {
                    nPageNumber = chip->deduce_blockOffset(mtd, offset, &blockOffset);
    
                    if (inp_oob[0] != 0xFF)
                    {
                        printk(KERN_DEBUG"%s: We are refreshing the block and marking the bb as bad\n", __FUNCTION__);
                        /* Since we want to mard the block as bad, we do not 
                           want refresh_block to report error: */
                        bReportErr = false;
                    }
                    else
                    {
                        if (localOob[0] != 0xFF)
                        {
                            printk(KERN_DEBUG"%s: We are refreshing the block and marking the bb as good\n", __FUNCTION__);
                            /*Since we want to mark the block as good and it was 
                              marked bad, we do not want refresh_block to 
                              report error: */
                            bReportErr = false;
                        }
                    }
                    ret = chip->refresh_block(mtd, NULL, inp_oob, nPageNumber, blockOffset, bReportErr);

                    if (ret != 0)
                    {
                       printk (KERN_ERR "%s : failed to refresh block (ret=%d)\n", 
                            __FUNCTION__, ret);
                       goto out;
                       
                    }
                    /*Set return code to all ok: */
                    ret = 0;
                }
                else
                {
                    if (inp_oob[0] != 0xFF)
                    {
//                        printk("%s: We are writing the oob and marking the bb as bad\n", __FUNCTION__);
                    }
                    //Initialize return code:
                    ret = 0;

                    for (winslice = 0; winslice < chip->eccsteps && ret == 0; winslice++) 
                    {
                        ret = brcmnand_posted_write_oob(mtd,  &inp_oob[oobWritten] , 
                                            offset);
        
                        if (ret != 0) 
                        {
                            printk(KERN_ERR "%s: brcmnand_posted_write_oob failed at offset=%0llx, ret=%d\n", 
                                __FUNCTION__, offset, ret);
                            goto out;
                        }
                        offset = offset + chip->eccsize;
                        oobWritten += chip->eccOobSize;
                    }
                }

        }
    }    
    

out:
    if (localOob)
    {
        kfree(localOob);
    }
    
#else    
    for (winslice = 0; winslice < chip->eccsteps && ret == 0; winslice++) 
    {
        ret = brcmnand_posted_write_oob(mtd,  &inp_oob[oobWritten] , 
                    offset);
        
        if (ret < 0) {
            printk(KERN_ERR "%s: brcmnand_posted_write_oob failed at offset=%0llx, ret=%d\n", 
                __FUNCTION__, offset, ret);
            return ret;
        }
        offset = offset + chip->eccsize;
        oobWritten += chip->eccOobSize;
    }

//    ret = brcmnand_verify_pageoob();

#endif

    return ret;
}


/**
 * brcmnand_write_with_ecc - [MTD Interface] BrcmNAND write with ECC
 * @param mtd        MTD device structure
 * @param to        offset to write to
 * @param len        number of bytes to write
 * @param retlen    pointer to variable to store the number of written bytes
 * @param buf        the data to write
 * @param eccbuf    filesystem supplied oob data buffer
 * @param oobsel    oob selection structure
 *
 * BrcmNAND write with ECC
 */
static int brcmnand_write_with_ecc(struct mtd_info *mtd, loff_t to, size_t len,
    size_t *retlen, const u_char *buf,
    u_char *eccbuf, struct nand_oobinfo *oobsel)
{
    struct brcmnand_chip *this = mtd->priv;
    int page, ret = -EBADMSG, oob = 0, written = 0, eccWritten = 0;
    int autoplace = 0, totalpages;
    u_char *oobbuf;
    //int    ppblock = (1 << (this->phys_erase_shift - this->page_shift));
    loff_t startPageOffset; 
//    u_char this->pLocalOob[MAX_OOB_SIZE];
    int eccRetLen = 0;
    int acc_control = 0;

    DEBUG(MTD_DEBUG_LEVEL3, "%s: to %s, len=%i\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,to, this->xor_invert_val), (int) len);
    
if (gdebug > 3) printk("%s: to = 0x%s, len = %i\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,to, this->xor_invert_val), (int) len);
    autoplace = brcmnand_autoplace(mtd, oobsel);
if (gdebug > 3) printk("%s: autoplace=%d\n", __FUNCTION__, autoplace);

    /* Initialize retlen, in case of early exit */
    *retlen = 0;

    /* Do not allow writes past end of device */
#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely(to + len) > mtd->size) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(to, len), this->mtdSize)))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt write to past end of device\n",__FUNCTION__);
        return -EINVAL;
    }
    /* Reject writes, which are not page aligned */
        if (unlikely(NOTALIGNED(__ll_low(to))) || unlikely(NOTALIGNED(len))) {
                DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt to write not page aligned data\n", __FUNCTION__);
                return -EINVAL;
        }

    /* Setup variables and oob buffer */
    totalpages = len >> this->page_shift;
    page = (uint32_t) __ll_RightShift(to , this->page_shift);
    startPageOffset = __ll_and32(to, ~(this->pageSize-1)); // (loff_t) (page << this->page_shift);
    
    /* Invalidate the page cache, if we write to the cached page */
    //if (page <= this->pagebuf && this->pagebuf < (page + totalpages))

    eccWritten = 0;

    /*
     * Write the part before page boundary.
     */
    if (__ll_isub(startPageOffset,  to)) {
        // How many bytes will need to be written for partial page write.
        int preface = (int)(to - startPageOffset);
        int thislen = min_t(int, len - written, mtd->writesize - preface);
        /* 
         * this function must be single threaded. 
         * Can't have this large of a buffer on the stack
         */
//        static u_char this->pLocalPage[MAX_PAGE_SIZE];
        
        /* Make a page, fill it up with 0xFF up to where the actual buffer starts */
        memset(this->pLocalPage, 0xFF, preface);
        memcpy(&this->pLocalPage[preface], buf, thislen);
        this->data_poi = (u_char*) &this->pLocalPage[0];

        /* We dont write ECC to a partial page */
        if (eccbuf) {
            printk(KERN_INFO "%s: OOB buffer ignored in partial page write at offset %s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,to, this->xor_invert_val));
        }
        oobbuf = NULL;
        
        /* Write one page. If this is the last page to write
         * or the last page in this block, then use the
         * real pageprogram command, else select cached programming
         * if supported by the chip.
         */

if (gdebug) printk("%s: Calling brcmnand_write_page", __FUNCTION__);

        ret = brcmnand_write_page (mtd, startPageOffset, mtd->writesize, this->data_poi, oobbuf);
        if (ret) {
            printk ("nand_write with ecc: write_page failed (2) %d\n", ret);
            goto out;
        }

        /* We can't verify a partial page write.
         * ret = brcmnand_verify_page (mtd, (loff_t) (page << this->page_shift), 
                &buf[written], thislen, oobbuf, eccRetLen, oobsel, autoplace

         *        );
         */

        
        /* Update written bytes count */
        written = thislen;
        *retlen = written;
        page++;
        startPageOffset = __ll_add32(startPageOffset, mtd->writesize);
    }

    /* Loop until all data are written */
    while (written < len) {
        int thislen = min_t(int, len - written, mtd->writesize);

        eccRetLen = 0;

        this->data_poi = (u_char*) &buf[written];

        if (eccbuf) 
        {
            oobbuf = brcmnand_prepare_oobbuf (mtd, 
                        &eccbuf[eccWritten], mtd->oobsize, this->pLocalOob, oobsel, autoplace, &eccRetLen);

            oob = 0;
        }
        else 
        {
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
            //Read oob area: 
            bool bRefresh = false;
            bool bWrite = false;
                
            L_OFF_T blockOffset;
            int i;
            unsigned int nPageNumber, nPartialPageNumber;
            int result;
                
            int retooblen;
            int retdatalen;
            struct nand_oobinfo myoobsel;
            myoobsel = mtd->oobinfo;
            myoobsel.useecc = MTD_NANDECC_PLACE;

            /*
              If the block was erased prior to this function call,
                
              Read data & oob in order to check if we need to refresh the
              block.  ==>We do not need to refresh the block if the 
              data&oob are all 0xFF (erased).

              If data&oob are not all 0xFF, then we may face a false 
              UNCORRECTABLE error, so we need to verify how many bits are bad
              before declaring the block as bad

              Note: If the result is that the page is UNCORRECTABLE, we 
              need to verify 
            */
            result = brcmnand_read_page(mtd, startPageOffset, this->pLocalPage, mtd->writesize, 
                                        this->pLocalOob, &myoobsel, &retdatalen, &retooblen,
                                        false);
                
            if (result == 0) //No error reported by read
            {
                //Check if WR_ECC_EN is set:
                acc_control = this->ctrl_read(this->nandAccControlReg);
                    
                if ((acc_control & BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK) != 0)
                {
                    //Check for ECC bytes != 0xFF:
                    for (i = 0; i < mtd->ecclayout->eccbytes; i++)
                    {
                        if(this->pLocalOob[mtd->ecclayout->eccpos[i]] != 0xFF)
                        {
                            bRefresh = true;
                            break;
                        }
                    }
                    if (bRefresh == true)
                    {
                    
                        nPageNumber = this->deduce_blockOffset(mtd, startPageOffset, &blockOffset);
                    
                        printk(KERN_DEBUG "===>%s: ECC Byte %d != 0xFF,"
                               "refreshing page %08X in block %08X <===\n",
                               __FUNCTION__, i, nPageNumber, (int)blockOffset);
                    
                        ret = this->refresh_block(mtd, this->data_poi, NULL, nPageNumber, blockOffset, true);
                
                        if (ret != 0)
                        {
                            printk (KERN_ERR "%s : write_page failed with refresh block %d\n", __FUNCTION__, ret);
                            ret = -EBADMSG;
                            goto out;
                        }  
                    }
                    else
                    { //Is there data to be written?
                        for (i = 0; i < len; i++)
                        {
                            if (this->data_poi[i] != 0xFF)
                            {
                                bWrite = true;
                                break;
                            }
                        }
                    }
                    if (bWrite == false)
                    {
                        /*Set return code to all ok: */
                        ret = 0;
                        /* Update written bytes count */
                        goto next_oob_page;
                    }
                }
            }
            else
            {   
                /*
                  We have set the nand controller so as not to tell us we 
                  have ECC uncorrectable errors when the ecc bytes and the data 
                  are 0xFF (erased).  However, if the controller reads any 
                  value in data or oob which is not 0xFF, it will still 
                  interpret it as an uncorrectable error.
                    
                  ==>In that case, if there are less than or equal to 
                  BCHN_THRESHOLD errors in the data+oob, we have to say that
                  the page is in truth correctable.  
                    
                  ==>We do not refresh it because we are in the write function.
                  The following write/verify will take care of the "refresh".
                    
                  ==>In order to count the number of bits is error, we
                  count the number of bits@0 that are part of it. 
                */
                uint32_t nbZeros = 0;
                int sliceDataOffset, sliceOobOffset;
                
                for( nPartialPageNumber=0; nPartialPageNumber < this->eccsteps; nPartialPageNumber++)
                {
                    nbZeros = 0;
                    sliceDataOffset = this->eccsize * nPartialPageNumber;
                    sliceOobOffset = this->eccOobSize * nPartialPageNumber;
                    
                    nbZeros = cnt_zeros(&this->pLocalPage[sliceDataOffset], mtd->writesize / this->eccsteps);
                    nbZeros += cnt_zeros(&this->pLocalOob[sliceOobOffset], mtd->oobsize / this->eccsteps);
                    
                    if (nbZeros > BCHN_THRESHOLD(this->ecclevel))
                    {
                        printk(KERN_ERR "%s: Uncorrectable error "
                               "reading data+oob, nbError = %d!\n", 
                               __FUNCTION__, nbZeros);
                        goto out;
                    }
                    else
                    {
                        if (nbZeros > 0)
                        {
                            printk(KERN_WARNING "%s: Partial page # %d shows that an "
                                   "uncorrectable error is in truth "
                                   "a correctable error, "
                                   "nb of bits in error = %d!\n", 
                                   __FUNCTION__, nPartialPageNumber, nbZeros);
                        }
                    }
                }
            }
#endif  
        }

if (gdebug) printk("%s: 60, startPage=%s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg, startPageOffset, this->xor_invert_val));
        ret = brcmnand_write_page (mtd, startPageOffset, thislen, this->data_poi, oobbuf);
        if (ret) {
            printk ("nand_write with ecc : write_page failed (1) %d\n", ret);
            goto out;
        }

if (gdebug) printk("%s: Verify page, oobarea=%p, autoplace=%d\n", __FUNCTION__, oobbuf, autoplace);

        ret = brcmnand_verify_page (mtd, startPageOffset, 
                &buf[written], thislen, &eccbuf[eccWritten], eccRetLen, oobsel, autoplace
                );

        if (ret) {
            printk (KERN_ERR"%s: verify_pages failed %d @%08x, written=%d\n", __FUNCTION__, ret, (unsigned int) to, written);
            goto out;
        }

next_oob_page:
        /* Next oob page */
        oob += mtd->oobsize;
        /* Update written bytes count */
        written += mtd->writesize;
        // if autoplace, count only the free bytes, otherwise, must count the entire OOB size
        eccWritten += autoplace? eccRetLen :mtd->oobsize;

        *retlen = written;

        /* Increment page address */
        page++;
        startPageOffset = __ll_add32(startPageOffset, mtd->writesize);
    }
    /* Verify the remaining pages */

out:
    

    return ret;

}
static int brcmnand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len,
    size_t *retlen, const u_char *buf,
    u_char *eccbuf, struct nand_oobinfo *oobsel)
{
    int ret;
    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_WRITING);
    
    ret = brcmnand_write_with_ecc(mtd,  to,  len, retlen, buf, eccbuf, oobsel);
    
    /* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);
    
    return ret;
}
/**
 * brcmnand_write - [MTD Interface] compability function for brcmnand_write_ecc
 * @param mtd        MTD device structure
 * @param to        offset to write to
 * @param len        number of bytes to write
 * @param retlen    pointer to variable to store the number of written bytes
 * @param buf        the data to write
 *
 * This function simply calls brcmnand_write_ecc
 * with oob buffer and oobsel = NULL
 */
static int brcmnand_write(struct mtd_info *mtd, loff_t to, size_t len,
    size_t *retlen, const u_char *buf)
{
    DEBUG(MTD_DEBUG_LEVEL3, "-->%s to=%08x, len=%d\n", __FUNCTION__,
         (unsigned int) to, (int) len);
    return brcmnand_write_ecc(mtd, to, len, retlen, buf, NULL, NULL);
}


/*
 * Allow nested calls inside this module in order to avoid having to release-and-reacquire lock.  
 * Take same arguments as the public interface.
 * Used mostly by BBT routines.
 */
static int brcmnand_write_oob_int(struct mtd_info *mtd, loff_t to, size_t ooblen, size_t* retlen,
    const u_char *oob_buf)
{
    struct brcmnand_chip *this = mtd->priv;
    int column; 
    L_OFF_T pageAddr;
    int ret = -EIO, oob = 0, eccWritten = 0;
    int autoplace;
    //int chipnr;
    //u_char *oobbuf, *bufstart;
    struct nand_oobinfo oobsel;

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    uint32_t* p32;
#endif

    oobsel = mtd->oobinfo;
    oobsel.useecc = MTD_NANDECC_PLACE;

    //DEBUG(MTD_DEBUG_LEVEL3, "%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) ooblen);

    /* Shift to get page */
    //page = (int) __ll_RightShift(to, this->page_shift);
    pageAddr = __ll_and32(to, ~(mtd->writesize - 1));
    
    //chipnr = (int) (to >> this->chip_shift);

    /* Mask to get column */
    column = __ll_low(__ll_and32(to, (mtd->oobsize - 1)));

    /* Initialize return length value */
    *retlen = 0;

    if ((ooblen % mtd->oobsize) != 0) {
        printk(KERN_WARNING "%s called at offset %s with len(%d) not multiple of oobsize(%d)\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg,to, this->xor_invert_val), ooblen, mtd->oobsize);
    }

    /* Do not allow writes past end of device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely((to + ooblen) > (mtd->size+mtd->oobsize))) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(to, ooblen), __ll_add32(this->mtdSize, mtd->oobsize))))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt write to past end of device\n", __FUNCTION__);
        return -EINVAL;
    }

    /* Grab the lock and see if the device is available */
    // brcmnand_get_device(mtd, FL_WRITING);

    eccWritten = 0; // Dest offset
    oob = 0; // SOurce offset

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
/*
    Hamming: 
    We need to disable HW ECC generation/reading because we are writing fake 
    data (0xFF)into the data area.  
    
    If the page is blank, then the ECC is already set to 0xFF and
    doesn't need to be updated.
    
    If there is data already written in the page, it won't be 
    altered by this writing, so the ECC doesn't need to be updated.
    
    Note: ==>ECC bytes must be updated only once.  
    If we were to update the ECC here, this would generate a correctable
    or uncorrectable ECC error as soon as we would write then read the data, 
    ultimately leading to a block refresh or a block marked bad.
    
    BCH4: 
    We cannot disable HW ECC generation/reading because ECC bytes are 
    written when writing the OOB.  So, when we will later on write the data
    we will need to refresh the block.
*/
    // Set fake data (0xFF) into the local buffer:
    p32 = (uint32_t*)this->pLocalPage;
    memset(p32, 0xFF, mtd->writesize);
#endif

    /* Loop until all data are written */
    while (eccWritten < ooblen) {
        int thislen = min_t(int, ooblen - eccWritten, mtd->oobsize);

        /* Write one page. If this is the last page to write
         * or the last page in this block, then use the
         * real pageprogram command, else select cached programming
         * if supported by the chip.
         */
        // No autoplace when this routine is called.

        ret = brcmnand_write_pageoob (mtd, pageAddr, thislen, &oob_buf[oob]);
        
        if (ret) {
            DEBUG (MTD_DEBUG_LEVEL0, "%s: write_pageoob failed %d\n", __FUNCTION__, ret);
            goto out;
        }
        autoplace = 0;
        ret = brcmnand_verify_pageoob (mtd, pageAddr, &oob_buf[oob], thislen, &oobsel, autoplace, 1);

        if (ret) {
            DEBUG (MTD_DEBUG_LEVEL0, "%s: verify_pages failed %d\n", __FUNCTION__, ret);
            goto out;
        }

        /* Next oob page */
        oob += mtd->oobsize;
        /* Update written bytes count */
        eccWritten += mtd->oobsize;

        *retlen += mtd->oobsize;

        /* Increment page address */
        pageAddr = __ll_add32(pageAddr, mtd->writesize);
    }

out:

    /* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);

    return ret;

}

/**
 * brcmnand_write_oob - [MTD Interface] BrcmNAND write out-of-band
 * @param mtd        MTD device structure
 * @param to            offset to write to (of the page, not the oob).
 * @param ooblen        number of OOB bytes to write
 * @param oob_buf    the data to write must be formatted beforehand
 *
 * BrcmNAND write out-of-band
 */
static int brcmnand_write_oob(struct mtd_info *mtd, loff_t to, size_t ooblen, size_t* retlen,
    const u_char *oob_buf)
{
    int ret;

    DEBUG(MTD_DEBUG_LEVEL3, "%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) ooblen);

    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, BRCMNAND_FL_WRITING_OOB);

    ret = brcmnand_write_oob_int(mtd, to, ooblen, retlen, oob_buf);
    
    /* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);

    return ret;
}

/**
 * brcmnand_write_oobfree - [MTD Interface] NAND write free out-of-band
 * @mtd:    MTD device structure
 * @to:        offset to write to
 * @len:    number of bytes to write
 * @retlen:    pointer to variable to store the number of written bytes
 * @buf:    the data to write
 *
 * NAND write free out-of-band area
 */
static int brcmnand_write_oobfree (struct mtd_info *mtd, 
        loff_t to, size_t len, size_t * retlen, const u_char * oob_buf)
{
    struct brcmnand_chip *this = mtd->priv;
    int column; 
    int ret = -EIO, oob = 0, written = 0, eccWritten = 0;
    L_OFF_T pageAddr;
    int autoplace;
    u_char *oobbuf;
    struct nand_oobinfo* oobsel = this->ecclayout;
    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    uint32_t* p32;
#endif
    DEBUG(MTD_DEBUG_LEVEL3, "\n%s: to = 0x%08x, len = %i\n", __FUNCTION__, (unsigned int) to, (int) len);

    /* Shift to get page */
    //page = (int) (to >> this->page_shift);
    pageAddr = __ll_and32(to, ~(mtd->writesize-1));
    
    //chipnr = (int) (to >> this->chip_shift);

    /* Mask to get column */
    column = to & (mtd->oobsize - 1);

    /* Initialize return length value */
    *retlen = 0;

    /* Do not allow writes past end of device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely((to + len) > (mtd->size+mtd->oobsize))) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(to, len), __ll_add32(this->mtdSize, mtd->oobsize))))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "\n%s: Attempt write to past end of device, to=%08x, len=%d, size=%08x\n", 
            __FUNCTION__, (unsigned int) to, (unsigned int) len, mtd->size);
        return -EINVAL;
    }

    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, BRCMNAND_FL_WRITING_OOB);

    /* Invalidate the page cache, if we write to the cached page */
    //if (page == this->pagebuf)
    //    this->pagebuf = -1;

    eccWritten = 0;
    
    /* Loop until all data are written */
    while (eccWritten < len) {
        int eccRetLen = 0;
        int thislen = min_t(int, len - eccWritten, mtd->oobavail);
//        u_char this->pLocalOob[MAX_OOB_SIZE];


        oobbuf = brcmnand_prepare_oobbuf (mtd, &oob_buf[eccWritten], thislen, 
            this->pLocalOob, oobsel, 1, &eccRetLen);
        oob = 0;


        /* Write one page. If this is the last page to write
         * or the last page in this block, then use the
         * real pageprogram command, else select cached programming
         * if supported by the chip.
         */

        /* We write out the entire OOB buffer, regardless of thislen */
        if (gdebug > 0)
        {
            int low_offset = (int)pageAddr;
            printk(KERN_INFO "****** offset: %08X\n", low_offset);
            print_oobbuf(oobbuf, mtd->oobsize);
        }

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
/*
    Hamming: 
    We need to disable HW ECC generation/reading because we are writing fake 
    data (0xFF)into the data area.  
    
    If the page is blank, then the ECC is already set to 0xFF and
    doesn't need to be updated.
    
    If there is data already written in the page, it won't be 
    altered by this writing, so the ECC doesn't need to be updated.
    
    Note: ==>ECC bytes must be updated only once.  
    If we were to update the ECC here, this would generate a correctable
    or uncorrectable ECC error as soon as we would write then read the data, 
    ultimately leading to a block refresh or a block marked bad.
    
    BCH4: 
    We cannot disable HW ECC generation/reading because ECC bytes are 
    written when writing the OOB.  So, when we will later on write the data
    we will need to refresh the block.
*/

        // Set fake data (0xFF) into the local buffer:
        p32 = (uint32_t*)this->pLocalPage;
        memset(p32, 0xFF, mtd->writesize);
#endif
        ret = brcmnand_write_pageoob (mtd, pageAddr, mtd->oobsize, &oobbuf[oob]);

        if (ret) {
            DEBUG (MTD_DEBUG_LEVEL0, "%s: write_pageoob failed %d\n", __FUNCTION__, ret);
            goto out;
        }

        autoplace = 1;
        ret = brcmnand_verify_pageoob (mtd, pageAddr, &oob_buf[eccWritten], eccRetLen, 
            oobsel, autoplace, 0);

        if (ret) {
            DEBUG (MTD_DEBUG_LEVEL0, "%s: verify_pages failed %d\n", __FUNCTION__, ret);
            goto out;
        }
        eccWritten += eccRetLen;
        /* Next oob page */
        oob += mtd->oobavail;
        /* Update written bytes count */
        eccWritten += mtd->oobavail;
        
        *retlen = written;

        /* Increment page address */
        pageAddr = __ll_add32(pageAddr, mtd->writesize);
    }
    /* Verify the remaining pages TBD */
    ret = 0;

out:
    /* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);

    return ret;
}

/**
 * brcmnand_writev_ecc - [MTD Interface] write with iovec with ecc
 * @param mtd        MTD device structure
 * @param vecs        the iovectors to write
 * @param count        number of vectors
 * @param to        offset to write to
 * @param retlen    pointer to variable to store the number of written bytes
 * @param eccbuf    filesystem supplied oob data buffer 
 * @param oobsel    oob selection structure  
 *
 * BrcmNAND write with iovec with ecc
 */
static int brcmnand_writev_ecc(struct mtd_info *mtd, const struct kvec *vecs,
    unsigned long count, loff_t to, size_t *retlen,
    u_char *eccbuf, struct nand_oobinfo *oobsel)
{
    int i, len, total_len, ret = -EIO, written = 0, oobretlen, buflen;
    L_OFF_T pageAddr;
    int numpages, autoplace = 0, oobWritten = 0;
    struct brcmnand_chip *this = mtd->priv;
    //int    ppblock = (1 << (this->phys_erase_shift - this->page_shift));
    u_char *oobbuf, *bufstart = NULL;
//    u_char this->pLocalOob[MAX_OOB_SIZE];
    /*
     * this function must be single threaded. 
     * Can't have this large of a buffer on the stack
     */
//    static u_char this->pLocalPage[MAX_PAGE_SIZE];


    /* Preset written len for early exit */
    *retlen = 0;

    /* Calculate total length of data */
    total_len = 0;
    for (i = 0; i < count; i++)
        total_len += vecs[i].iov_len;

    DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_writev_ecc: to = 0x%08x, len = %i, count = %ld, eccbuf=%p, total_len=%d\n", 
    (unsigned int) to, (unsigned int) total_len, count, eccbuf, total_len);

    /* Do not allow write past end of the device */

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1
    if (unlikely(to + total_len) > mtd->size) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(to, total_len), this->mtdSize)))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_writev_ecc: Attempted write past end of device\n");
        return -EINVAL;
    }

    /* Reject writes, which are not page aligned */
        if (unlikely(NOTALIGNED(to)) || unlikely(NOTALIGNED(total_len))) {
                DEBUG(MTD_DEBUG_LEVEL0, "brcmnand_writev_ecc: Attempt to write not page aligned data\n");
                return -EINVAL;
        }

    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_WRITING);

    autoplace = brcmnand_autoplace(mtd, oobsel);

    /* Setup start page, we know that to is aligned on page boundary */
    pageAddr = to;

    /* Loop until all keve's data has been written */
    len = 0;         // How many data from current iovec has been written
    written = 0;    // How many bytes have been written so far in all
    oobWritten = 0; // How many oob bytes written in all
    buflen = 0;    // How many bytes from the buffer has been copied to.
    while (count) {
        /* If the given tuple is >= pagesize then
         * write it out from the iov
         */
        // THT: We must also account for the partial buffer left over from previous iovec
        if ((buflen + vecs->iov_len - len) >= mtd->writesize) {
            /* Calc number of pages we can write
             * out of this iov in one go */
            numpages = (buflen + vecs->iov_len - len) >> this->page_shift;


            //oob = 0;
            for (i = 0; i < numpages; i++) {
                if (0 == buflen) { // If we start a new page
                    bufstart = &((u_char *)vecs->iov_base)[len];
                }
                else { // Reuse existing partial buffer, partial refill to end of page
                    memcpy(&bufstart[buflen], &((u_char *)vecs->iov_base)[len], mtd->writesize - buflen);
                }

                /* Write one page. If this is the last page to write
                 * then use the real pageprogram command, else select
                 * cached programming if supported by the chip.
                 */
                oobbuf = brcmnand_prepare_oobbuf (mtd, &eccbuf[oobWritten], 
                    autoplace? mtd->oobsize: mtd->oobavail, 
                    this->pLocalOob, oobsel, autoplace, &oobretlen);

                ret = brcmnand_write_page (mtd, pageAddr, mtd->writesize, bufstart, oobbuf);
                bufstart = NULL;

                if (ret) {
                    printk(KERN_ERR "%s: brcmnand_write_page failed, ret=%d\n", __FUNCTION__, ret);
                    goto out;
                }
                len += mtd->writesize - buflen;
                buflen = 0;
                //oob += oobretlen;
                pageAddr = __ll_add32(pageAddr, mtd->writesize);
                written += mtd->writesize;
                oobWritten += oobretlen;
            }
            /* Check, if we have to switch to the next tuple */
            if (len >= (int) vecs->iov_len) {
                vecs++;
                len = 0;
                count--;
            }
        } else { // (vecs->iov_len - len) <  mtd->writesize)
            /*
             * We must use the internal buffer, read data out of each
             * tuple until we have a full page to write
             */
            


#if 0
            int cnt = 0;
            while (count && cnt < mtd->writesize) {
                if (vecs->iov_base != NULL && vecs->iov_len)
                    this->pLocalPage[cnt++] = ((u_char *) vecs->iov_base)[len++];
                /* Check, if we have to switch to the next tuple */
                if (len >= (int) vecs->iov_len) {
                    vecs++;
                    len = 0;
                    count--;
                }
#else
            /*
             * THT: Changed to use memcpy which is more efficient than byte copying, does not work yet
             *  Here we know that 0 < vecs->iov_len - len < mtd->writesize, and len is not necessarily 0
             */
            // While we have iovec to write and a partial buffer to fill
            while (count && (buflen < mtd->writesize)) {
                
                // Start new buffer?
                if (0 == buflen) {
                    bufstart = this->pLocalPage;
                }
                if (vecs->iov_base != NULL && (vecs->iov_len - len) > 0) {
                    // We fill up to the page
                    int fillLen = min_t(int, vecs->iov_len - len, mtd->writesize - buflen);
                    
                    memcpy(&this->pLocalPage[buflen], &((u_char*) vecs->iov_base)[len], fillLen);
                    buflen += fillLen;
                    len += fillLen;
                }
                /* Check, if we have to switch to the next tuple */
                if (len >= (int) vecs->iov_len) {
                    vecs++;
                    len = 0;
                    count--;
                }
#endif
            }
            // Write out a full page if we have enough, otherwise loop back to the top
            if (buflen == mtd->writesize) {
                numpages = 1;
                oobbuf = brcmnand_prepare_oobbuf (mtd, eccbuf, 
                    autoplace? mtd->oobsize: mtd->oobavail, 
                    this->pLocalOob, oobsel, autoplace, &oobretlen);

                ret = brcmnand_write_page (mtd, pageAddr, mtd->writesize, bufstart, oobbuf);
                if (ret) {
                    printk(KERN_ERR "%s: brcmnand_write_page failed, ret=%d\n", __FUNCTION__, ret);
                    goto out;
                }
                pageAddr = __ll_add32(pageAddr, mtd->writesize);
                written += mtd->writesize;
                oobWritten += oobretlen;
                bufstart = NULL;
                buflen = 0;
            }
        }

        /* All done ? */
        if (!count) {
            if (buflen) { // Partial page left un-written.  Imposible, as we check for totalLen being multiple of pageSize above.
                printk(KERN_ERR "%s: %d bytes left unwritten with writev_ecc at offset %s\n", 
                    __FUNCTION__, buflen, __ll_sprintf(brcmNandMsg,pageAddr, this->xor_invert_val));
                BUG();
            }
            break;
        }

#if 0
        /* Check, if we cross a chip boundary */
        if (!startpage) {
            chipnr++;
            this->select_chip(mtd, -1);
            this->select_chip(mtd, chipnr);
        }
#endif
    }
    ret = 0;
out:
    /* Deselect and wake up anyone waiting on the device */
    brcmnand_release_device(mtd);

    *retlen = written;
//if (*retlen <= 0) printk("%s returns retlen=%d, ret=%d, startAddr=%s\n", __FUNCTION__, *retlen, ret, __ll_sprintf(brcmNandMsg,startAddr, this->xor_invert_val));
    return ret;
}

/**
 * brcmnand_writev - [MTD Interface] compabilty function for brcmnand_writev_ecc
 * @param mtd        MTD device structure
 * @param vecs        the iovectors to write
 * @param count        number of vectors
 * @param to        offset to write to
 * @param retlen    pointer to variable to store the number of written bytes
 *
 * BrcmNAND write with kvec. This just calls the ecc function
 */
static int brcmnand_writev(struct mtd_info *mtd, const struct kvec *vecs,
    unsigned long count, loff_t to, size_t *retlen)
{
    DEBUG(MTD_DEBUG_LEVEL3, "-->%s to=%08x, count=%d\n", __FUNCTION__,
         (unsigned int) to, (int) count);
    return brcmnand_writev_ecc(mtd, vecs, count, to, retlen, NULL, NULL);
}

/**
 * brcmnand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @param mtd        MTD device structure
 * @param ofs        offset from device start
 * @param getchip    0, if the chip is already selected
 * @param allowbbt    1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int brcmnand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int getchip, int allowbbt)
{
    struct brcmnand_chip *this = mtd->priv;
    int res;

    if (getchip) {
        brcmnand_get_device(mtd, FL_READING);
    }
    
    /* Return info from the table */
    res = this->isbad_bbt(mtd, ofs, allowbbt);

    if (getchip) {
        brcmnand_release_device(mtd);
    }

    return res;
}


static int brcmnand_erase_block(struct mtd_info* mtd, L_OFF_T addr, int bDoNotMarkBad)
{
    int needBBT = 0;
    
    
    int ret = 0;
    struct brcmnand_chip *this = mtd->priv;
    
    this->ctrl_writeAddr(this, addr, 0);
    
    this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCK_ERASE);

    // Wait until flash is ready, without EDU:
    if (brcmnand_ctrl_write_is_complete(mtd, &needBBT))
    {
        ret = 0;
    }
    else
    {
        ret = -EBADMSG;
    }

        
    /* Check, if it is write protected: TBD */
    if (needBBT ) 
    {
        if (bDoNotMarkBad)
        {
            ret = -EBADMSG; //Error, but block not marked as bad
        }
        else
        {
            printk(KERN_ERR "%s: Failed erase, addr %s, flash status=%08x\n", 
                       __FUNCTION__, 
                   __ll_sprintf(brcmNandMsg,addr, this->xor_invert_val), 
                   needBBT);
               
            ret = -EBADBLK;// return bad block status to the caller

        }
      
    }

    if(ret != 0)
    {
        printk(KERN_DEBUG"%s : ret = %d\n", __FUNCTION__, ret);
    }
    return ret;
}

/**
 * brcmnand_erase_blocks - [Private] erase block(s)
 * @param mtd        MTD device structure
 * @param instr        erase instruction
 * @allowBBT            allow erase of BBT
 *
 * Erase one ore more blocks
 * ** FIXME ** This code does not work for multiple chips that span an address space > 4GB
 */
static int brcmnand_erase_blocks(struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
    struct brcmnand_chip *this = mtd->priv;
    unsigned int block_size;
    loff_t addr;
    int len;
    int ret = 0;

    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
    // For now ignore the high DW.
    L_OFF_T instr_addr = __ll_constructor(0, instr->addr);
    instr->addr_hi = 0;
#endif

    DEBUG(MTD_DEBUG_LEVEL3, "%s: start = 0x%08x, len = %08x\n", __FUNCTION__, (unsigned int) instr->addr, (unsigned int) instr->len);

    block_size = (1 << this->erase_shift);


    /* Start address must align on block boundary */
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
    if (unlikely(instr->addr & (block_size - 1))) 
#else
    if (unlikely(!__ll_is_zero(__ll_and32(instr_addr, (block_size - 1)))))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "%s: Unaligned address\n", __FUNCTION__);
        return -EINVAL;
    }

    /* Length must align on block boundary */
    if (unlikely(instr->len & (block_size - 1))) 
    {
        DEBUG(MTD_DEBUG_LEVEL0, 
"%s: Length not block aligned, len=%08x, blocksize=%08x, chip->blkSize=%08x, chip->erase=%d\n",
__FUNCTION__, instr->len, block_size, this->blockSize, this->erase_shift);
        return -EINVAL;
    }

    /* Do not allow erase past end of device */
    if (unlikely((instr->len + instr->addr) > mtd->size)) 
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
    if (unlikely((instr->len + instr->addr) > mtd->size)) 
#else
    if (unlikely(__ll_is_greater(__ll_add32(instr_addr, instr->len), this->mtdSize)))
#endif
    {
        DEBUG(MTD_DEBUG_LEVEL0, "%s: Erase past end of device\n", __FUNCTION__);
        return -EINVAL;
    }

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
    instr->fail_addr = 0xffffffff;
#else
    instr->fail_addr = instr->fail_addr_hi = 0xffffffff;
#endif


    /*
     * Clear ECC registers 
     */
    this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);
    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
    this->ctrl_write(BCHP_NAND_ECC_CORR_EXT_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_EXT_ADDR, 0);
#endif

    /* Loop through the pages */
    len = instr->len;

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
    addr = instr->addr;
#else
    addr = instr_addr;
#endif

    instr->state = MTD_ERASING;

    while (len) {


/* THT: We cannot call brcmnand_block_checkbad which just look at the BBT,
// since this code is also called when we create the BBT.
// We must look at the actual bits, or have a flag to tell the driver
// to read the BI directly from the OOB, bypassing the BBT
 */
        /* Check if we have a bad block, we do not erase bad blocks */
        if (brcmnand_block_checkbad(mtd, addr, 0, allowbbt)) {
            printk (KERN_ERR "%s: attempt to erase a bad block at addr %s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg,addr, this->xor_invert_val));
            instr->state = MTD_ERASE_FAILED;
            goto erase_one_block;
        }

        //this->command(mtd, ONENAND_CMD_ERASE, addr, block_size);
        
        ret = brcmnand_erase_block(mtd, addr, allowbbt);

        if (ret != 0)        
        {
                instr->state = MTD_ERASE_FAILED;

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
                instr->fail_addr = addr;
#else
                instr->fail_addr = __ll_low(addr);
                instr->fail_addr_hi = __ll_high(addr);
#endif
                break; //Out of the while loop!!
        }
        
erase_one_block:

        len -= block_size;
        addr = __ll_add32(addr, block_size);
    }

    instr->state = MTD_ERASE_DONE;

//erase_exit:

    if (ret != -EBADBLK){
        ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
    }
    

if (gdebug)    printk("%s OUT: ret = 0x%08X\n", __FUNCTION__, ret);    


    /* Do call back function */
    if (!ret) {
    mtd_erase_callback(instr);
    }
    
    if(ret == -EBADBLK)
        /* now mark the block bad */
        (void) this->block_markbad(mtd, addr);

    return ret;
}


/**
 * brcmnand_erase - [MTD Interface] erase block(s)
 * @param mtd        MTD device structure
 * @param instr        erase instruction
 *
 * Erase one ore more blocks
 */
static int brcmnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    int ret;
    DEBUG(MTD_DEBUG_LEVEL3, "-->%s addr=%08x, len=%d\n", __FUNCTION__,
         (unsigned int) instr->addr, instr->len);
//Take lock:
    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_ERASING);
      
    ret =  brcmnand_erase_blocks(mtd, instr, 0); // not allowed to access the bbt area
    
//Release lock    
    brcmnand_release_device(mtd);
    return ret;
}

/**
 * brcmnand_sync - [MTD Interface] sync
 * @param mtd        MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void brcmnand_sync(struct mtd_info *mtd)
{
    DEBUG(MTD_DEBUG_LEVEL3, "brcmnand_sync: called\n");

    /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_SYNCING);

    PLATFORM_IOFLUSH_WAR();

    /* Release it and go back */
    brcmnand_release_device(mtd);
}


/**
 * brcmnand_block_isbad - [MTD Interface] Check whether the block at the given offset is bad
 * @param mtd        MTD device structure
 * @param ofs        offset relative to mtd start
 *
 * Check whether the block is bad
 */
static int brcmnand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
    struct brcmnand_chip *this = mtd->priv;
    
    DEBUG(MTD_DEBUG_LEVEL3, "-->%s ofs=%08x\n", __FUNCTION__, __ll_low(ofs));
    /* Check for invalid offset */
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
    if (ofs > mtd->size)
#else
    if (__ll_is_greater(ofs, this->mtdSize))
#endif
    {
        return -EINVAL;
    }

    return brcmnand_block_checkbad(mtd, ofs, 1, 0);
}

/**
 *  - [DEFAULT] mark a block bad
 * @param mtd        MTD device structure
 * @param ofs        offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
 */
 
extern int brcmnand_update_bbt (struct mtd_info *mtd, loff_t offs);

static int brcmnand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    struct brcmnand_chip *this = mtd->priv;
    //struct bbm_info *bbm = this->bbm;
    // THT: 3/20/07: We restrict ourselves to only support x8
    u_char bbmarker[1] = {0x0};  // CFE and BBS uses 0x0F, Linux by default uses 0
                                //so we can use this to mark the difference
//    u_char this->pLocalOob[MAX_OOB_SIZE];

    uint32_t block, page;
    int dir;
    UL_OFF_T ulofs;
 
    int ret = 0;
    ulofs = (UL_OFF_T)__ll_and32(ofs, ~ this->page_mask);

    if (gdebug) printk("In brcmnand_default_block_markbad()\n");    

    /* Get block number.  Block is guaranteed to be < 2*32 */
    block = __ll_low(__ll_RightShift(ulofs, this->erase_shift));

    if (!NAND_IS_MLC(this)) { // SLC chip, mark first and 2nd page as bad.
        page = block << (this->erase_shift - this->page_shift);
        dir = 1;
    }
    else { // MLC chip, mark last and previous page as bad.
        page = ((block+1) << (this->erase_shift - this->page_shift)) - 1;
        dir = -1;
    }    
    
    memset(this->pLocalOob, 0xFF, mtd->oobsize);
    memcpy(&this->pLocalOob[this->badblockpos], bbmarker, sizeof(bbmarker));

    // Lock already held by caller, so cant call mtd->write_oob directly
    ret = brcmnand_write_page_oob(mtd, this->pLocalOob, page);
    if (ret) {
        printk(KERN_ERR "Mark bad page %d failed with retval=%d\n", page, ret);
    }

    //this->pLocalOob may have been used by brcmnand_write_page_oob
    memset(this->pLocalOob, 0xFF, mtd->oobsize);
    memcpy(&this->pLocalOob[this->badblockpos], bbmarker, sizeof(bbmarker));

    // Mark 2nd page as bad, ignoring last write
    page += dir;
    // Lock already held by caller, so cant call mtd->write_oob directly

    ret = brcmnand_write_page_oob(mtd, this->pLocalOob, page);
    if (ret) {
        printk(KERN_ERR "Mark bad page %d failed with retval=%d\n", page, ret);
    }

    if (this->bbt)
    {
        this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1); 
    }


    /*
     * According to the HW guy, even if the write fails, the controller have written 
     * a 0 pattern that certainly would have written a non 0xFF value into the BI marker.
     *
     * Ignoring ret.  Even if we fail to write the BI bytes, just ignore it, 
     * and mark the block as bad in the BBT
     */
    (void) brcmnand_update_bbt(mtd, ulofs);

    return ret;
}

/**
 * brcmnand_delete_bbt - [NAND Interface] Delete Bad Block Table
 * @mtd:    MTD device structure
 *
 * Each byte in the BBT contains 4 entries, 2 bits each per block.
 * So the entry for the block b is:
 * bbt[b >> 2] & (0x3 << ((b & 0x3) << 1)))
 *
*/

int brcmnand_delete_bbt (struct mtd_info *mtd)
{
    struct brcmnand_chip *this = mtd->priv;
    uint32_t bOffset, bOffsetStart=0, bOffsetEnd=0;
    int ret, needBBT;

//printk("%s: gClearBBT=%d, size=%08x, erasesize=%08x\n", __FUNCTION__, gClearBBT, mtd->size, mtd->erasesize);


    //bOffsetStart = 0; //mtd->size - (1<<20);  // BBT partition is 1MB
    
//    bOffsetStart = (uint32_t)(mtd->size - (1<<20));  // BBT partition is 1MB

    //BBT partition is 8 blocks at the end of the device:
    bOffsetStart = mtd->size - (8 * mtd->erasesize); 
    bOffsetEnd = (uint32_t) __ll_sub(this->mtdSize, mtd->erasesize);

    printk(KERN_INFO "Erasing flash from %08x to %08x\n", bOffsetStart, bOffsetEnd);

    /*
     * Clear ECC registers 
     */
    this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);

            
    for (bOffset=bOffsetStart; bOffset <= bOffsetEnd; bOffset += mtd->erasesize) {

        printk(KERN_INFO "%s: Erasing block at %08x\n", __FUNCTION__, bOffset);
        this->ctrl_writeAddr(this, bOffset, 0);

        this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCK_ERASE);
        // Wait until flash is ready
        ret = this->write_is_complete(mtd, &needBBT);
        if (needBBT) {
            printk(KERN_WARNING "%s: Erase failure, marking bad block @%08x\n", __FUNCTION__, bOffset);
            ret = this->block_markbad(mtd, bOffset);
        }
    }
    
    return 0;
    
}

/**
 * brcmnand_get_bbt_code - [NAND Interface] Return the block code
 * @mtd:    MTD device structure
 * @offs:    offset in the device
 * @allowbbt:    allow access to bad block table region
 *
 * Each byte in the BBT contains 4 entries, 2 bits each per block.
 * So the entry for the block b is:
 * bbt[b >> 2] & (0x3 << ((b & 0x3) << 1)))
 *
*/
static int brcmnand_get_bbt_code (struct mtd_info *mtd, loff_t offs)
{
    struct brcmnand_chip *this = mtd->priv;
    int block;
    uint8_t    res;

    //printk("In brcmnand_get_bbt_code() where offs is: %08x THIS value is: %08x\n", (unsigned int) offs, (unsigned int)this);

    block = __ll_low(__ll_RightShift(offs,  (this->bbt_erase_shift - 1)));

    //printk("Block Number is: %d\n", block);

    res = (this->bbt[block >> 3] >> (block & 0x06)) & 0x03;

    // printk("Table Content is: %d\n", res);

    DEBUG (MTD_DEBUG_LEVEL3, "brcmnand_get_bbt_code(): bbt info for offs 0x%08x: (block %d) 0x%02x\n",
        (unsigned int)offs, block >> 1, res);

    // PRINTK("%s: res=%x, allowbbt=%d at block %08x\n", __FUNCTION__, res, allowbbt, (unsigned int) offs);

    switch ((int)res) {
    case 0x00:    // Good block
        return 0;
    case 0x01:    // Marked bad due to wear
        return 1;
    case 0x02:    // Reserved blocks
        return 2;
    case 0x03:    // factory marked bad
        return 3; 
    }
    return 4; // Unknown kind of block
}

/**
 * brcmnand_get_statistics_bbt_code - [NAND Interface] Update BBT Statistics
 * @mtd:    MTD device structure
 *
 */
int brcmnand_get_statistics_bbt_code (struct mtd_info *mtd)
{
    static int firstStats = 0;
    int code;
    struct brcmnand_chip *this = mtd->priv;

    loff_t endofs, ofs;
    unsigned int increment = (unsigned int)this->blockSize;

    ofs = 0;
    endofs = __ll_add(ofs, this->chipSize);

    this->stats.goodBlocksNow = 0;;    

    this->stats.badBlocksDueToWearNow = 0;

    this->stats.reservedBlocksNow = 0;

    this->stats.factoryMarkedBadBlocksNow = 0;
    
    while(1)
    {

        code = brcmnand_get_bbt_code (mtd, ofs);
            switch (code) 
            {
        case 0x00:    // Good block
            this->stats.goodBlocksNow++;    
            break;
        case 0x01:    // Marked bad due to wear
            this->stats.badBlocksDueToWearNow++;
            break;
        case 0x02:    // Reserved blocks
            this->stats.reservedBlocksNow++;
            break;
        case 0x03:    // factory marked bad
            this->stats.factoryMarkedBadBlocksNow++;
            break;
        default:
            printk(KERN_ERR "%s Unknown type of block in BBT\n", __FUNCTION__);
            return -1;
            break;
            }

        // Calculate next offset
        ofs = __ll_add32(ofs, increment);

        // Bail out if necessary
        if (__ll_isub(endofs, ofs) <=0 )
            break;
    }

     if(firstStats == 0)
    {
        this->stats.goodBlocksBefore = this->stats.goodBlocksNow;
        this->stats.badBlocksDueToWearBefore = this->stats.badBlocksDueToWearNow;
        this->stats.reservedBlocksBefore = this->stats.reservedBlocksNow;
        this->stats.factoryMarkedBadBlocksBefore = this->stats.factoryMarkedBadBlocksNow;
        firstStats = 1;
    }

    return 0;


}

/**
 *  - [DEFAULT] mark a block with a specific code
 * @param mtd        MTD device structure
 * @param ofs        offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
 */
 
static int brcmnand_set_bbt_code(struct mtd_info *mtd, loff_t ofs, loff_t abs_ofs, unsigned int code)
{
    uint32_t block, page;
    int dir;
    UL_OFF_T ulofs;

    struct brcmnand_chip *this = mtd->priv;
    
    /* 
        CFE and BBS uses 0x0F, Linux by default uses 0,
        but this is a test function.  So, let's write any value between 0 & 254: 
    */
    uint8_t bbmarker = jiffies & 0xFE;  
    
    uint8_t gbmarker = 0xff;  // CFE and BBS uses 0x0F, Linux by default uses 0
                                //so we can use this to mark the difference
    int ret = 0;

    if(abs_ofs == 0)
        ulofs = (UL_OFF_T)__ll_and32(ofs, ~ this->page_mask);
    else
        ulofs = (UL_OFF_T)__ll_and32(abs_ofs, ~ this->page_mask);

    printk(KERN_DEBUG"%s: Marking block %s as %s\n", __FUNCTION__, 
                        __ll_sprintf(brcmNandMsg, ulofs, this->xor_invert_val), 
                            (code == 0)? "GOOD":"BAD");  
    
    memset(this->pLocalOob, 0xFF, mtd->oobsize);

    if(code == 0) // We want to change that block as a good block!!!
        this->pLocalOob[this->badblockpos] = gbmarker;
    else{
        printk(KERN_DEBUG"bbmarker is 0x%02X", bbmarker);
        this->pLocalOob[this->badblockpos]=  bbmarker;
    }
        
/*******************/
    /* Get block number.  Block is guaranteed to be < 2*32 */
    block = __ll_low(__ll_RightShift(ulofs, this->erase_shift));

    /* Get page number. */
    if (!NAND_IS_MLC(this)) { // SLC chip, mark first and 2nd page as bad.
        page = block << (this->erase_shift - this->page_shift);
        dir = 1;
    }
    else { // MLC chip, mark last and previous page as bad.
        page = ((block+1) << (this->erase_shift - this->page_shift)) - 1;
        dir = -1;
    }    

    //Clear ram bbt:
    if (this->bbt)
    {
        // Get rid of what was there before in the 2 bits
        this->bbt[block >> 2] &= ~(0x3 << ((block & 0x03) << 1)); 
    }            

    // Lock already held by caller, so cant call mtd->write_oob directly
    ret = brcmnand_write_page_oob(mtd, this->pLocalOob, page);
    if (ret) {
        printk(KERN_ERR "Mark bad page %d failed with retval=%d\n", page, ret);
    }

    //this->pLocalOob may have been used by brcmnand_write_page_oob:
    memset(this->pLocalOob, 0xFF, mtd->oobsize);

    if(code == 0) // We want to change that block as a good block!!!
        this->pLocalOob[this->badblockpos] = gbmarker;
    else{
        printk(KERN_DEBUG"bbmarker is 0x%02X", bbmarker);
        this->pLocalOob[this->badblockpos]=  bbmarker;
    }

    // Mark 2nd page as bad, ignoring last write
    page += dir;
    // Lock already held by caller, so cant call mtd->write_oob directly

    ret = brcmnand_write_page_oob(mtd, this->pLocalOob, page);
    if (ret) {
        printk(KERN_ERR "Mark bad page %d failed with retval=%d\n", page, ret);
    }

    //Update ram bbt:
    if (this->bbt)
    {
        // Set new value:
        this->bbt[block >> 2] |= code << ((block & 0x03) << 1); 
    }            

    /*
     * According to the HW guys, even if the write fails, the controller have written 
     * a 0 pattern that certainly would have written a non 0xFF value into the BI marker.
     *
     * Ignoring ret.  Even if we fail to write the BI bytes, just ignore it, 
     * and mark the block as bad in the BBT
     */
    (void) brcmnand_update_bbt(mtd, ulofs);

    return 0; //success
}

/**
 * brcmnand_get_statistics - [NAND Interface] Return NAND Statistics
 * @mtd:    MTD device structure
 * @offs:    offset in the device (UNUSED)
 * @stats:    pointer on nand statistics structure
 *
 *
*/
static int brcmnand_get_statistics (struct mtd_info *mtd, struct nand_statistics *stats)
{
    struct brcmnand_chip *this = mtd->priv;

//    memcpy(stats->nandDescription, this->stats.nandDescription, NAND_DESCRIPTION_STRING_LENGTH);

    stats->eccCorrectableErrorsNow =    this->stats.eccCorrectableErrorsNow;
    stats->eccCorrectableErrorsBefore = this->stats.eccCorrectableErrorsBefore;
    
    stats->eccUncorrectableErrorsNow =    this->stats.eccUncorrectableErrorsNow;
    stats->eccUncorrectableErrorsBefore = this->stats.eccUncorrectableErrorsBefore;

    // For next time, keep actual errors

    this->stats.eccCorrectableErrorsBefore = this->stats.eccCorrectableErrorsNow;
    this->stats.eccUncorrectableErrorsBefore = this->stats.eccUncorrectableErrorsNow; 
    this->stats.unclassifiedCriticalErrorsBefore = this->stats.unclassifiedCriticalErrorsNow;


    // Read the Bad Block Table Statistics
    brcmnand_get_statistics_bbt_code(mtd);

    memcpy((void*)stats, (void*)&this->stats, sizeof (struct nand_statistics));

    return 0;

}

int flipbit(unsigned int bits, unsigned int *pattern, unsigned int width)
{
    unsigned int mask = 0x00000001;
    int i;
    int bitflipped = 0;
    unsigned int newpattern;

    // printk("\n\n\nPattern upon entering is 0x%08x\n\n\n", *pattern);

    for(i = 0; i < width; i++)
    {
        newpattern = pattern[i/32] & ~(mask << (i%32));

        //printk("\nIteration %d: newpattern = %08x\n", i, newpattern);

        if(newpattern != pattern[i/32])
        {
            bitflipped++;
            pattern[i/32] = newpattern;
        }
        if(bitflipped >= bits)
        {
            //printk("\nbitflipped = %d and bits = %d\n", bitflipped, bits);
            break;
        }
    }

    // printk("\n\n\nPattern upon exiting is 0x%08x\n\n\n", *pattern);

    return bitflipped;

}
static int brcmnand_gen_ecc_error_test_with_pattern(struct mtd_info *mtd, 
                               struct nand_generate_ecc_error_command *command)
{

    size_t retlen;
    uint32_t *p32, *p32ret;
    unsigned i;
    int bit_flipped;
    unsigned int pattern_with_bit_flipped;
    struct nand_oobinfo oobsel;
    struct erase_info instr;

    int ret = 0;
    int value = 0;
    u_char *buf = NULL, *retbuf = NULL;    
    struct brcmnand_chip *this = mtd->priv;


    unsigned int length     = mtd->writesize;   //block-based
    unsigned int pattern    = command->pattern;
    unsigned int bits       = command->bits;
    loff_t ofs              = command->offset;

    command->total_number_of_tests = 3;
    command->test_result[0] = GENECCERROR_UNKNOWN;
    command->test_result[1] = GENECCERROR_UNKNOWN;
    command->test_result[2] = GENECCERROR_UNKNOWN;

    printk("BCH Code Test with Erase First\n");

    pattern_with_bit_flipped = pattern;

    bit_flipped = flipbit(bits, &pattern_with_bit_flipped, 32);

    printk("Pattern: %08X, pattern_with_bit_flipped: %08X\n",
            pattern, pattern_with_bit_flipped);

    // Erase the portion of the NAND that we want to erase
    instr.mtd = mtd;
    instr.len = (uint32_t)(1 << this->erase_shift);
    instr.addr = (uint32_t)ofs;
    instr.callback = 0;

    g_bGenEccErrorTest = true;

    ret = brcmnand_save_block(mtd, ofs);
    
    if (ret == 0)
    { //The test really starts here:

        if(brcmnand_erase_blocks(mtd, &instr, 0) != 0) // not allowed to access the bbt area
        {
            printk(KERN_DEBUG"Unable to erase the block bailing out...\n");
            goto putback;
        }

        g_bGenEccErrorTest = false;

        // Allocate and fill the buffer

        buf = kmalloc(length, GFP_KERNEL);

        p32 = (uint32_t*) buf;

        for(i = 0;i < length/4; i++)
        {
            p32[i] = pattern;
        }

        oobsel.useecc = MTD_NANDECC_AUTOPLACE;

        // Read ACC_CONTROL register

        value = this->ctrl_read(this->nandAccControlReg);

        value &= ~BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK;

        this->ctrl_write(this->nandAccControlReg, value);

        // ######################################################################

        // TEST 1 Simple NAND Write/Read/Verify
        printk(KERN_DEBUG"Starting TEST 1: Simple NAND Write/Read/Verify after disabling RD ECC\n");
        brcmnand_write_with_ecc(mtd, ofs, (size_t)length, &retlen, buf, NULL, &oobsel);

        // Let's now read back!

        retbuf = kmalloc(length, GFP_KERNEL);

        brcmnand_read_with_ecc(mtd, ofs, (size_t)length, &retlen, retbuf, NULL, &oobsel);


        if( memcmp(buf, retbuf, length) != 0)
        {
            printk(KERN_DEBUG"TEST 1 FAILED\n");
            command->test_result[0] = GENECCERROR_FAILED;
            goto putback;
        }
        else
        {
            printk(KERN_DEBUG"TEST 1 SUCCESS\n");
            command->test_result[0] = GENECCERROR_SUCCESS;
        }

        // ######################################################################
    
        // TEST 2 Flip bits on the previous data disabling the ECC correction first

        if(bits == bit_flipped)
        {
            printk(KERN_DEBUG"Starting TEST 2: NAND Write/Read/Verify after disabling  WR ECC\n");
            // Unset the bits ECC_WR_EN / ECC_RD_EN
    
            value &= ~(BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK | BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK);

            this->ctrl_write(this->nandAccControlReg, value);

            // Now we flipped the bits in the original data... no error or correction has been done
            // since we disabled up to here the ECC correction scheme
    
            // Flip bits in the original packet... 

            p32[0] = pattern_with_bit_flipped;

            retlen = 0;

            brcmnand_write_with_ecc(mtd, ofs, (size_t)length, &retlen, buf, NULL, &oobsel);

            memset(retbuf, 0, length);


            // If we read the data back we should get no READ error and the flipped bit
            // should be seen!

            brcmnand_read_with_ecc(mtd, ofs, (size_t)length, &retlen, retbuf, NULL, &oobsel);

            p32ret = (uint32_t*)retbuf;
        
            printk(KERN_DEBUG"written buffer [0]:%08X, [1]:%08X, [2]:%08X, [3]:%08X\n",
                p32[0], p32[1], p32[2], p32[3]);
    
            printk(KERN_DEBUG"returned buffer [0]:%08X, [1]:%08X, [2]:%08X, [3]:%08X\n",
            p32ret[0], p32ret[1], p32ret[2], p32ret[3]);
            
            if( memcmp(buf, retbuf, length) != 0)
            {
                printk(KERN_DEBUG"TEST 2 FAILED\n");
                command->test_result[1] = GENECCERROR_FAILED;
                goto putback;
            }
            else
            {
                printk(KERN_DEBUG"BCH TEST 2 SUCCESS\n");
                command->test_result[1] = GENECCERROR_SUCCESS;
            }


            // ######################################################################
            // TEST 3.... Generate the ECC Correctable Error

            // TEST 3 Check that ECC has been caught
            // Put back Read ECC Enable

            printk(KERN_DEBUG"Starting TEST 3: NAND Read/verify after re-enabling RD & WR ECC\n");
            // Re-enable the ECC_WR_EN bit
            value |= BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK;
            this->ctrl_write(this->nandAccControlReg, value);
        
            // Re-enable the ECC_RD_EN bit
            value |= BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK;
            this->ctrl_write(this->nandAccControlReg, value);

            memset(retbuf, 0, length);

            brcmnand_read_with_ecc(mtd, ofs, (size_t)length, &retlen, retbuf, NULL, &oobsel);

            for(i = 0;i < length/4; i++)
            {
                p32[i] = pattern;
            }
            p32ret = (uint32_t*)retbuf;
        
            printk(KERN_DEBUG"written buffer [0]:%08X, [1]:%08X, [2]:%08X, [3]:%08X\n",
                p32[0], p32[1], p32[2], p32[3]);

            printk(KERN_DEBUG"returned buffer [0]:%08X, [1]:%08X, [2]:%08X, [3]:%08X\n",
                p32ret[0], p32ret[1], p32ret[2], p32ret[3]);
                
            if( memcmp(buf, retbuf, length) != 0)
            {
                printk(KERN_DEBUG"TEST 3 FAILED\n");
                command->test_result[2] = GENECCERROR_FAILED;
                goto putback;
            }
            else
            {
                printk(KERN_DEBUG"TEST 3 SUCCESS\n");
                command->test_result[2] = GENECCERROR_SUCCESS;
            }
        }    
        else
        {
            printk(KERN_DEBUG"Unable to flip the number of bits requested!\n");
        }
    }
putback:
    ret = brcmnand_restore_block(mtd, ofs);

    brcmnand_free_block(mtd);
//end of mfi:added
    if(buf != NULL)
        kfree(buf);

    if(retbuf != NULL)
        kfree(retbuf);

    return 0;

}

/*
    Function:brcmnand_gen_ecc_error_in_ecc_bytes
    Description:  
        1-Read page oob
        2-Disable WR/RD ECC
        3-Write page oob with 1 bit (or more) flipped
        4-Read page oob ==> No error ok, test 1 passed
        5-Enable RD ECC
        6-Read page oob ==> refresh_block takes place but we get an ecc read 
            error.
            (The ecc read will be wrong because it doesn't get corrected, 
            but the refresh will have corrected it.  We need a second read to 
            insure that the refresh has corrected it.)
        7-Second Read page oob ==> No error ok, test 2 passed.
    
*/
static int brcmnand_gen_ecc_error_in_ecc_bytes(
    struct mtd_info *mtd, 
    struct nand_generate_ecc_error_command *command)
{
    struct brcmnand_chip* this = mtd->priv;
    u_char    *orgbuf = NULL, *flipbuf = NULL, *retbuf = NULL;
    
    size_t     retlen;
    int        value;

    struct nand_oobinfo oobsel;
    int bit_flipped;
    unsigned int *ppattern = NULL;
    int i, j;
    int ret = 0;

    struct nand_oobinfo place_oob;
    
    unsigned int length = mtd->writesize; 
    
    //Align offset on page boundary:
    loff_t pageOfs = __ll_and32(command->offset, ~(this->pageSize-1));
    //Get the slice within this page (0-1-2-3-...):
    int slice = ((int)command->offset % mtd->writesize) / mtd->eccsize;
         
    command->total_number_of_tests = 2;
    command->test_result[0] = GENECCERROR_UNKNOWN;
    command->test_result[1] = GENECCERROR_UNKNOWN;

    oobsel.useecc = MTD_NANDECC_AUTOPLACE;

    length = mtd->oobsize; // Spare Area Size

    // ######################################################################
    // Read ACC_CONTROL register
    value = this->ctrl_read(this->nandAccControlReg);

    printk("Error in ECC Bytes of the OOB Area test\n");
    
    
    printk("Starting Test 1\n");


    // Simple NAND Read
  
    // Let's read the actual nand data!

    retbuf = kmalloc(mtd->oobsize, GFP_KERNEL); // buffer data read through nand controller

    orgbuf = kmalloc(mtd->oobsize, GFP_KERNEL); // Original data read

    flipbuf = kmalloc(mtd->oobsize, GFP_KERNEL); // Original data read but with one flipped bit.

    printk(KERN_DEBUG"Reading Original Data (%d Bytes)\n", length);

    place_oob = mtd->oobinfo;
    place_oob.useecc = MTD_NANDECC_PLACE;

    ret = brcmnand_read_pageoob(mtd, pageOfs, retbuf, &retlen, &place_oob, 0, 0);
//    brcmnand_read_oob(mtd, ofs, (size_t)mtd->oobsize, &retlen, retbuf);

    printk(KERN_DEBUG"\n OOB Area before = \n");
    // Print the content of the oob area

    for(i = 0, j = slice * this->eccOobSize; i < this->eccOobSize; i++) 
    {
        printk(KERN_DEBUG" %02X", retbuf[j++]);
    }
    printk(KERN_DEBUG"\n");    

    // The buffer just read is now in retbuf;

    // Keep a copy of the original data in buf

    memcpy(orgbuf, retbuf, length);

    printk(KERN_DEBUG"Flip available bit(s) in partial page\n");

//    if(this->ecclevel == BRCMNAND_ECC_BCH_4)
    
    ppattern = (unsigned int *)&retbuf[(slice * this->eccOobSize) + mtd->ecclayout->eccpos[0]];
   
    bit_flipped = flipbit(command->bits, ppattern, this->eccbytes * 8); //ecc correctable

    memcpy(flipbuf, retbuf, length);

    // Write the data back with a bit flipped
    if(bit_flipped == command->bits)
    {
        // Unset the bit WR_EN
        value = value & ~BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK;

        this->ctrl_write(this->nandAccControlReg, value);

        printk(KERN_DEBUG"Write ECC Disabled\n");

        // Unset the bit ECC_RD_EN

        value = value & ~BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK;

        this->ctrl_write(this->nandAccControlReg, value);

        printk("Read ECC Disabled\n");

        // Now we flipped the bits in the original data... no error or correction has been done
        // since we disabled up to here the ECC correction scheme

        // Make a copy of the buffer

        retlen = 0;

        printk(KERN_DEBUG"Write back the Data\n");

        brcmnand_write_oob(mtd, pageOfs, (size_t)length, &retlen, retbuf);

        // Re-enable the bit

        value = (value | BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK);
        this->ctrl_write(this->nandAccControlReg, value);

        printk(KERN_DEBUG"Write ECC Enabled\n");

        memset(retbuf, 0, length);


        // If we read the data back we should get no READ error and the flipped bit
        // should be seen!

        printk(KERN_DEBUG"Read Back the Data\n");

        ret = brcmnand_read_pageoob(mtd, pageOfs, retbuf, &retlen, &place_oob, 0, 0);
        //brcmnand_read_oob(mtd, ofs, (size_t)mtd->oobsize, &retlen, retbuf);

        printk(KERN_DEBUG"\n OOB Area after bit flipped = ");

        // Print the content of the oob area
        
        for(i = 0, j = slice * this->eccOobSize; i < this->eccOobSize; i++) 
        {
            printk(KERN_DEBUG" %02X", retbuf[j++]);
        }

        printk(KERN_DEBUG"\n");    

        if( memcmp(flipbuf, retbuf, length) != 0)
        {
            printk("Test 1 FAILED.\n");
            command->test_result[0] = GENECCERROR_FAILED;
            goto closeall;
        }
        else
        {
            printk("Test 1 SUCCESS.\n");
            command->test_result[0] = GENECCERROR_SUCCESS;
        }

        // TEST 2 Check that ECC has been caught
        // Put back Read ECC Enable

        printk(KERN_DEBUG"Starting Test 2\n");

        value = (value | BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK);
        this->ctrl_write(this->nandAccControlReg, value);

        printk(KERN_DEBUG"Read ECC Enabled\n");

        if (command->runAutomatedTest)
        {
            memset(retbuf, 0, length);
            printk(KERN_DEBUG"Reading data back a first time\n");

            /*
                This read will trig the refresh, but it will not return the correct 
                ecc bytes because the ecc bytes are not corrected.
            */
            ret = brcmnand_read_pageoob(mtd, pageOfs, retbuf, &retlen, &place_oob, 0, 0);

            for(i = 0; i < length; i++) 
            {
                if( orgbuf[i] != retbuf[i])
                {
                    printk(KERN_DEBUG"orgbuf[%d] = %02X retbuf[%d] = %02X\n", i, orgbuf[i], i, retbuf[i]);
                }
           }

            memset(retbuf, 0, length);
            /*
                This read will return the correct because the ecc bytes have been
                refreshed during the previous read:
            */

            printk(KERN_DEBUG"Reading data back a second time\n");

            ret = brcmnand_read_pageoob(mtd, pageOfs, retbuf, &retlen, &place_oob, 0, 0);

            for(i = 0; i < length; i++) 
            {
                if( orgbuf[i] != retbuf[i])
                {
                    printk(KERN_DEBUG"orgbuf[%d] = %02X retbuf[%d] = %02X\n", i, orgbuf[i], i, retbuf[i]);
                }
            }
            if( memcmp(orgbuf, retbuf, length) != 0)
            {
                printk(KERN_ERR"Test 2 FAILED\n");
                command->test_result[1] = GENECCERROR_FAILED;
                goto closeall;
            }
            else
            {
                printk(KERN_DEBUG"Test 2 SUCCESS\n");
                command->test_result[1] = GENECCERROR_SUCCESS;
            }
        }
    }
    else
    {
        printk(KERN_ERR"Unable to set a bit to zero in the ECC bytes!\n");
    }
closeall:

    // Put back Read ECC Enable
    value = (value | BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK);
    this->ctrl_write(this->nandAccControlReg, value);

    // Put back Write ECC Enable
    value = (value | BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK);
    this->ctrl_write(this->nandAccControlReg, value);

    if(retbuf != NULL)
        kfree(retbuf);

    if(orgbuf != NULL)
        kfree(orgbuf);

    if(flipbuf != NULL)
        kfree(flipbuf);


    return 0;
}

/*
    Function: brcmnand_gen_ecc_error_specific_location
    Description:  
        1-Read & save block
        2-Modify read data with bits flipped (data,bbi,ecc)
        3-Erase block
        4-Remove WR_ECC
        5-Write block with modified data
        6-Return
        
    Caution: This function alters the block and does not restore it afterwards.
*/
static int brcmnand_gen_ecc_error_specific_location(struct mtd_info *mtd, struct nand_generate_ecc_error_command *command)
{
    int ret;
    uint8_t save;

    struct brcmnand_chip* this = mtd->priv;
    int ofsWithinTheBlock = (int) __ll_and32(command->offset, this->blockSize - 1);
    loff_t blockOfs              = command->offset -  ofsWithinTheBlock;
    
    //determine page within the block:
    int pageWithinTheBlock = ofsWithinTheBlock / this->pageSize;
    int positionWithinTheBlock = ofsWithinTheBlock - (pageWithinTheBlock * this->pageSize);
    int partialPageWithinTheBlock = positionWithinTheBlock / this->eccsize;

//    printk("pageWithinTheBlock = %d, positionWithinTheBlock= %d, partialPageWithinTheBlock = %d, this->eccOobSize= %d\n", 
//        pageWithinTheBlock, positionWithinTheBlock, partialPageWithinTheBlock, this->eccOobSize);

    //Disable the ecc before saving the block, in order for multiple calls
    //to this function to always return ecc correctable errors
    brcmnand_disable_ecc(mtd);
    
    ret = brcmnand_save_block(mtd, blockOfs);
    if (ret != 0)
    {
        goto closeall;
    }

    if (command->bitmask != 0x00)
    {
        save = ppRefreshData[pageWithinTheBlock] [positionWithinTheBlock];
        //Modify data:
#if 1        
         ppRefreshData[pageWithinTheBlock] [positionWithinTheBlock] ^= (command->bitmask);
#else
        ppRefreshData[pageWithinTheBlock] [positionWithinTheBlock] &= ~(command->bitmask);
#endif         

        printk (KERN_DEBUG"Modified data byte @ page = %d, position = %d from %02X to %02X\n", 
            pageWithinTheBlock, positionWithinTheBlock, save, 
            ppRefreshData[pageWithinTheBlock] [positionWithinTheBlock]);
    }
    
    if (command->oob_bitmask != 0x00)
    {
        save = ppRefreshOob[pageWithinTheBlock] [(partialPageWithinTheBlock * this->eccOobSize) + command->oob_offset];
        //Modify oob:
#if 1        
        ppRefreshOob[pageWithinTheBlock] [(partialPageWithinTheBlock * this->eccOobSize) + command->oob_offset] ^= (command->oob_bitmask);
#else
        ppRefreshOob[pageWithinTheBlock] [(partialPageWithinTheBlock * this->eccOobSize) + command->oob_offset] &= ~(command->oob_bitmask);        
#endif
        printk (KERN_DEBUG"Modified oob byte @ page = %d, position = %d from %02X to %02X\n", 
            pageWithinTheBlock, (partialPageWithinTheBlock * this->eccOobSize) + command->oob_offset, save, 
            ppRefreshOob[pageWithinTheBlock] [(partialPageWithinTheBlock * this->eccOobSize) + command->oob_offset]);
    }
    
    
    //brcmnand_restore_block erases then puts ppRefreshData & ppRefreshBlock into it        
    ret = brcmnand_restore_block(mtd, blockOfs);
    if (ret != 0)
    {
        goto closeall;
    }

closeall:
    //Re-enable ECC:
    brcmnand_enable_ecc(mtd);

    brcmnand_free_block(mtd);
    return ret;

}

/*
    Function: brcmnand_gen_ecc_error_random_location
    Description:  
        1-Read & save block
        2-Modify read data with bits flipped (data,bbi,ecc)
        3-Erase block
        4-Remove WR_ECC
        5-Write block with modified data
        6-Return
        
    Caution: This function alters the block and does not restore it afterwards.
*/
static int brcmnand_gen_ecc_error_random_location(struct mtd_info *mtd, struct nand_generate_ecc_error_command *command)
{
    int i,j,bit_flipped,ret,acc,acc0;
   
    struct brcmnand_chip* this = mtd->priv;
    unsigned int bits       = command->bits;
    loff_t ofs              = command->offset;
    unsigned int nStartPage = command->partialPageX / this->eccsteps;
    unsigned int nEndPage   = command->partialPageY / this->eccsteps;
    
    unsigned int nPartialPage = command->partialPageX;
   
    //Read this->nandAccControlReg register
    acc = brcmnand_ctrl_read(this->nandAccControlReg); 

    ret = brcmnand_save_block(mtd, ofs);
    if (ret != 0)
    {
        goto closeall;
    }

    if (command->distribute)
    {
        command->ecc_error = 1;     //Set ecc_error so that we can generate errors in ecc bytes
    }

    //Distribute error in data:
    nPartialPage = command->partialPageX;

    printk(KERN_DEBUG "%s: distributing %d bit(s) in data\n",__FUNCTION__, bits);
        
    for (i = nStartPage; i <= nEndPage; i++)
    {
        for(j = (nPartialPage % this->eccsteps); 
            ((j < this->eccsteps) && (nPartialPage <= command->partialPageY));
            j++, nPartialPage++)
        {
            //flipbits:
            bit_flipped = flipbit(bits, (uint32_t*)(ppRefreshData[i]+(j*this->eccsize)), this->eccsize);
            if (bits != bit_flipped)
            {
                printk(KERN_WARNING"Can only flip %d bits in partial page %d", bit_flipped, nPartialPage);
            }
        }
    }

    if (command->distribute) //distribute in 1st byte of oob
    {
        nPartialPage = command->partialPageX;
        
        printk(KERN_DEBUG "%s: distributing %d bit(s) in 1st byte of oob\n",__FUNCTION__, bits);
        
        for (i = nStartPage; i <= nEndPage; i++)
        {

            for(j = (nPartialPage % this->eccsteps); 
                ((j < this->eccsteps) && (nPartialPage <= command->partialPageY));
                j++, nPartialPage++)
            {
                //flipbits in the first byte [bi]:
                bit_flipped = flipbit(bits, (uint32_t*)(ppRefreshOob[i]+(j*this->eccOobSize)), 8);
                if (bits != bit_flipped)
                {
                    printk(KERN_WARNING"Can only flip %d bits in partial oob (partial page = %d)", bit_flipped, nPartialPage);
                }
            }
        }
    }
    
    if (command->ecc_error)
    {            
        nPartialPage = command->partialPageX;

        printk(KERN_DEBUG"%s: distributing %d bit(s) in ecc bytes of oob\n",__FUNCTION__, bits);

        for (i = nStartPage; i <= nEndPage; i++)
        {
            for(j = (nPartialPage % this->eccsteps); 
                ((j < this->eccsteps) && (nPartialPage <= command->partialPageY));
                j++, nPartialPage++)
            {

                //flipbits in the ecc bytes [bi]:
                bit_flipped = flipbit(bits, 
                                      (uint32_t*)(ppRefreshOob[i]+(j*this->eccOobSize) + mtd->ecclayout->eccpos[0]), this->eccbytes * 8);
                if (bits != bit_flipped)
                {
                    printk(KERN_WARNING"Can only flip %d bits in partial ecc bytes (partial page= %d)", bit_flipped, nPartialPage);
                }
            }
        }     
    }
    //Disable WR_ECC:
    
    acc0 = acc & ~(BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK);
    brcmnand_ctrl_write(this->nandAccControlReg, acc0);
    
    //brcmnand_restore_block erases then puts ppRefreshData & ppRefreshBlock into it        
    ret = brcmnand_restore_block(mtd, ofs);
    if (ret != 0)
    {
        goto closeall;
    }

closeall:
    //Re-enable WR_ECC:
    brcmnand_ctrl_write(this->nandAccControlReg, acc);
    

    brcmnand_free_block(mtd);
    return ret;
}

/*******************************************************************************
*   Function: brcmnand_gen_ecc_error
*
*   Parameters: 
*       mtd:    IN: Pointer to the mtd_info structure
*       command:IN: Pointer to the nand_generate_ecc_error_command structure
*   Description:
*       This is the driver's entry point to dispatch gen_ecc_error
*
* Returns: 
*   TBD
*******************************************************************************/
static int brcmnand_gen_ecc_error(struct mtd_info *mtd, struct nand_generate_ecc_error_command *command)
{
    struct brcmnand_chip *this;

    int returnvalue = 0;
    g_bGenEccErrorTest = false;
    // Check parameters
    if((mtd == NULL)||(command == NULL))
    {
        printk(KERN_ERR"brcmnand_gen_ecc_error(): mtd is NULL\n");
        return -2;
    }
    
    this = mtd->priv;

     /* Grab the lock and see if the device is available */
    brcmnand_get_device(mtd, FL_WRITING);


    if (command->runSpecificLocationTest)
    {
        returnvalue = brcmnand_gen_ecc_error_specific_location(mtd, command);
        command->total_number_of_tests = 0;
    }
    else if (command->runAutomatedTest)
    {
        if(command->ecc_error)
        {
            returnvalue = brcmnand_gen_ecc_error_in_ecc_bytes(mtd, command);
        }
        else
        {
            returnvalue = brcmnand_gen_ecc_error_test_with_pattern(mtd,command);
        }
    }
    else    
    {
        returnvalue = brcmnand_gen_ecc_error_random_location(mtd, command);
        command->total_number_of_tests = 0;
    }
    /*Release the lock*/
    brcmnand_release_device(mtd);
    
    return returnvalue;
}

/**
 * brcmnand_block_markbad - [MTD Interface] Mark the block at the given offset as bad
 * @param mtd        MTD device structure
 * @param ofs        offset relative to mtd start
 *
 * Mark the block as bad
 */
static int brcmnand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    struct brcmnand_chip *this = mtd->priv;
    int ret;

    DEBUG(MTD_DEBUG_LEVEL3, "-->%s ofs=%08x\n", __FUNCTION__, (unsigned int) ofs);
    
    ret = brcmnand_block_isbad(mtd, ofs);
    if (ret) {
        /* If it was bad already, return success and do nothing */
        if (ret > 0)
            return 0;
        return ret;
    }

    return this->block_markbad(mtd, ofs);
}

/**
 * brcmnand_unlock - [MTD Interface] Unlock block(s)
 * @param mtd        MTD device structure
 * @param ofs        offset relative to mtd start
 * @param len        number of bytes to unlock
 *
 * Unlock one or more blocks
 */
static int brcmnand_unlock(struct mtd_info *mtd, loff_t llofs, size_t len)
{
    struct brcmnand_chip *this = mtd->priv;
    int status;
    UL_OFF_T blkAddr, ofs = (UL_OFF_T) llofs;

    DEBUG(MTD_DEBUG_LEVEL3, "-->%s llofs=%08x, len=%d\n", __FUNCTION__,
         (unsigned int) llofs, (int) len);



    /* Block lock scheme */
    for (blkAddr = ofs; __ll_is_less(blkAddr, __ll_add32(ofs, len)); blkAddr = __ll_add32(blkAddr, this->blockSize)) {

        /* The following 2 commands share the same CMD_EXT_ADDR, as the block never cross a CS boundary */
        this->ctrl_writeAddr(this, blkAddr, 0); 
        /* Set end block address */
        this->ctrl_writeAddr(this, __ll_add32(blkAddr, this->blockSize - 1), 1);
        /* Write unlock command */
        this->ctrl_write(BCHP_NAND_CMD_START, OP_BLOCKS_UNLOCK);


        /* There's no return value */
        this->wait(mtd, FL_UNLOCKING, &status);

        if (status & 0x0f)  
            printk(KERN_ERR "block address = %s, wp status = 0x%x\n", __ll_sprintf(brcmNandMsg, blkAddr, this->xor_invert_val), status);

        /* Sanity check */
#if 0
        while (this->read_word(this->base + ONENAND_REG_CTRL_STATUS)
            & ONENAND_CTRL_ONGO)
            continue;
#endif

        /* Check lock status */
        this->ctrl_writeAddr(this, blkAddr, 0); 
        this->ctrl_write(BCHP_NAND_CMD_START, OP_READ_BLOCKS_LOCK_STATUS);
        status = this->ctrl_read(BCHP_NAND_BLOCK_LOCK_STATUS);
        //status = this->read_word(this->base + ONENAND_REG_WP_STATUS);

    }

    return 0;
}


/**
 * brcmnand_print_device_info - Print device ID
 * @param device        device ID
 *
 * Print device ID
 */
static void brcmnand_print_device_info(struct mtd_info* mtd, brcmnand_chip_Id* chipId, unsigned long flashSize)
{
    char str[64];
    
    sprintf(str,"BrcmNAND mfg %x %x %s %dMB\n", chipId->mafId, chipId->chipId, chipId->chipIdStr, (int) (flashSize >>20));
    
    printk(str);
    strcat(str_nandinfo, str);
    
    printk(KERN_INFO "%s", str_nandinfo);
    
    print_config_regs(mtd);
}



/*
 * bit 31:     1 = OTP read-only
 * 30:         Page Size: 0 = PG_SIZE_512, 1 = PG_SIZE_2KB
 * 28-29:     Block size: 3=512K, 1 = 128K, 0 = 16K, 2 = 8K
 * 27:        Reserved
 * 24-26:    Device_Size
 *            0:    4MB
 *            1:    8MB
 *            2:     16MB
 *            3:    32MB
 *            4:    64MB
 *            5:    128MB
 *            6:     256MB
 * 23:        Dev_Width 0 = Byte8, 1 = Word16
 * 22-19:     Reserved
 * 18:16:    Full Address Bytes
 * 15        Reserved
 * 14:12    Col_Adr_Bytes
 * 11:        Reserved
 * 10-08    Blk_Adr_Bytes
 * 07-00    Reserved
 */
 

//Test bits are the same on all NAND controllers:
#define MSK_CONFIG_LOCK     0x80000000
#define MSK_DEV_SIZE        0x0F000000
#define MSK_DEV_WIDTH       0x00800000
#define MSK_FUL_ADR_BYTES   0x00070000
#define MSK_COL_ADR_BYTES   0x00007000
#define MSK_BLK_ADR_BYTES   0x00000700

#define SHT_CONFIG_LOCK     31
#define SHT_DEV_SIZE        24
#define SHT_DEV_WIDTH       23
#define SHT_FUL_ADR_BYTES   16
#define SHT_COL_ADR_BYTES   12
#define SHT_BLK_ADR_BYTES   8

#define SHT_BLK_SIZE        28    

//Test bits are different for NAND_VERS_1_0 and NAND_VERS_3_0
#if (CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_1_0)
    #define MSK_BLK_SIZE        0x30000000    
    #define MSK_PAGE_SIZE       0x40000000

    #define SHT_PAGE_SIZE       30
#else
    #define MSK_BLK_SIZE        0x70000000
    #define MSK_PAGE_SIZE       0x00300000

    #define SHT_PAGE_SIZE       20
#endif


void
brcmnand_decode_config(struct brcmnand_chip* chip, uint32_t nand_config)
{
    unsigned int chipSizeShift;
    
printk(KERN_INFO "MSK_BLK_SIZE: %08X, SHT_BLK_SIZE: %08X\n", MSK_BLK_SIZE, SHT_BLK_SIZE);

    switch ((nand_config & MSK_BLK_SIZE) >> SHT_BLK_SIZE) {
        case 4:
            chip->blockSize = 256 << 10;
            break;
        case 3:
            chip->blockSize = 512 << 10;
            break;
        case 2:
            chip->blockSize = 8 << 10;
            break;
        case 1:    
            chip->blockSize = 128 << 10;
            break;
        case 0:
            chip->blockSize = 16 << 10;
            break;
    }

    //Find first bit set in chip->blockSize and return its index (from 1):
    chip->erase_shift = ffs(chip->blockSize) - 1;

    switch((nand_config & MSK_PAGE_SIZE) >> SHT_PAGE_SIZE) {
        case 0:
            chip->pageSize = 512;
            break;
        case 1:
            chip->pageSize = 2048;
            break;
        case 2:
            chip->pageSize = 4096;
            break;
        default:
            printk(KERN_ERR "brcmnand_decode_config(): unknown pageSize\n");
            break;
    }
    //Find first bit set in chip->pageSize and return its index (from 1):
    chip->page_shift = ffs(chip->pageSize) - 1;
    chip->page_mask = (1 << chip->page_shift) - 1;

    chipSizeShift = (nand_config & MSK_DEV_SIZE) >> SHT_DEV_SIZE;
    chip->chipSize = (4 << 20) << chipSizeShift;

    chip->busWidth = 1 + ((nand_config & MSK_DEV_WIDTH) >> SHT_DEV_WIDTH);

    printk(KERN_INFO "NAND Config: Reg=%08x, chipSize=%dMB, blockSize=%dK, erase_shift=%x\n",
        nand_config, chip->chipSize >> 20, chip->blockSize >> 10, chip->erase_shift);
    printk(KERN_INFO "busWidth=%d, pageSize=%dB, page_shift=%d, page_mask=%08x\n", 
        chip->busWidth, chip->pageSize, chip->page_shift , chip->page_mask);

}


/*
 * Adjust timing pattern if specified in chip ID list
 * Also take dummy entries, but no adjustments will be made.
 */
static void brcmnand_adjust_timings(struct brcmnand_chip *this, brcmnand_chip_Id* chip)
{
    unsigned long nand_timing1, nand_timing1_b4, nand_timing2, nand_timing2_b4;

#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    this->nandTiming1Reg = BCHP_NAND_TIMING_1;
    this->nandTiming2Reg = BCHP_NAND_TIMING_2;
#else
    switch(this->CS[0])
    {
        case 0:
            this->nandTiming1Reg = BCHP_NAND_TIMING_1;
            this->nandTiming2Reg = BCHP_NAND_TIMING_2;
        break;
        
        case 1:
            this->nandTiming1Reg = BCHP_NAND_TIMING_1_CS1;
            this->nandTiming2Reg = BCHP_NAND_TIMING_2_CS1;
        break;
                
        case 2:
            this->nandTiming1Reg = BCHP_NAND_TIMING_1_CS2;
            this->nandTiming2Reg = BCHP_NAND_TIMING_2_CS2;
        break;
                
        case 3:
            this->nandTiming1Reg = BCHP_NAND_TIMING_1_CS3;
            this->nandTiming2Reg = BCHP_NAND_TIMING_2_CS3;
        break;  
              
       
        default:
            printk(KERN_ERR"%s: invalid cs (%d)", __FUNCTION__, this->CS[0]);
            this->nandTiming1Reg = BCHP_NAND_TIMING_1;
            this->nandTiming2Reg = BCHP_NAND_TIMING_2;
        break;        
    
    
    }
#endif    //#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    nand_timing1 = this->ctrl_read(this->nandTiming1Reg);
    nand_timing2 = this->ctrl_read(this->nandTiming2Reg);
                
    // Adjust NAND timings:
    if (chip->timing1) 
    {
        nand_timing1_b4 = nand_timing1;

        if (chip->timing1 & BCHP_NAND_TIMING_1_tWP_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tWP_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tWP_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tWH_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tWH_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tWH_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tRP_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tRP_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tRP_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tREH_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tREH_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tREH_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tCS_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tCS_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tCS_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tCLH_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tCLH_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tCLH_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tALH_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tALH_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tALH_MASK);  
        }
        if (chip->timing1 & BCHP_NAND_TIMING_1_tADL_MASK) {
            nand_timing1 &= ~BCHP_NAND_TIMING_1_tADL_MASK;
            nand_timing1 |= (chip->timing1 & BCHP_NAND_TIMING_1_tADL_MASK);  
        }

        this->ctrl_write(this->nandTiming1Reg, nand_timing1);
        printk(KERN_INFO "Adjust timing1: Was %08lx, changed to %08lx\n", nand_timing1_b4, nand_timing1);
    }
    else 
    {
        printk(KERN_NOTICE "timing1 not adjusted: %08lx\n", nand_timing1);
    }

    // Adjust NAND timings:
    if (chip->timing2) 
    {
        nand_timing2_b4 = nand_timing2;
        
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
        if (chip->timing2 & BCHP_NAND_TIMING_2_CLK_SELECT_MASK){
            nand_timing2 &= ~BCHP_NAND_TIMING_2_CLK_SELECT_MASK;
            nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_CLK_SELECT_MASK);  
        }
#endif        
        if (chip->timing2 & BCHP_NAND_TIMING_2_tWB_MASK) {
            nand_timing2 &= ~BCHP_NAND_TIMING_2_tWB_MASK;
            nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_tWB_MASK);  
        }
        if (chip->timing2 & BCHP_NAND_TIMING_2_tWHR_MASK) {
            nand_timing2 &= ~BCHP_NAND_TIMING_2_tWHR_MASK;
            nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_tWHR_MASK);  
        }
        if (chip->timing2 & BCHP_NAND_TIMING_2_tREAD_MASK) {
            nand_timing2 &= ~BCHP_NAND_TIMING_2_tREAD_MASK;
            nand_timing2 |= (chip->timing2 & BCHP_NAND_TIMING_2_tREAD_MASK);  
        }

        this->ctrl_write(this->nandTiming2Reg, nand_timing2);
        printk(KERN_INFO "Adjust timing2: Was %08lx, changed to %08lx\n", nand_timing2_b4, nand_timing2);
    }
    else 
    {
        printk(KERN_INFO "timing2 not adjusted: %08lx\n", nand_timing2);
    }
}

static void 
brcmnand_read_id(unsigned int i, unsigned long* dev_id)
{
    unsigned long timeout;
    
    uint32_t chipSelect = 0;

    if (i > 0)
    {
        chipSelect = 0x01 << (i-1);
    } 
    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
    /* Set correct chip Select */
    brcmnand_ctrl_write(BCHP_NAND_CMD_EXT_ADDRESS, chipSelect << 16);

    brcmnand_ctrl_write(BCHP_NAND_CMD_START, OP_DEVICE_ID_READ);
#endif
    /* 20 msec is enough */
    timeout = jiffies + msecs_to_jiffies(20); // mfi: 20 msecs, for now
    while (time_before(jiffies, timeout)) {
        ;
    }
    PLATFORM_IOFLUSH_WAR();
    /* Send the command for reading device ID from controller */
    *dev_id = brcmnand_ctrl_read(BCHP_NAND_FLASH_DEVICE_ID);

    if (*dev_id != 0) 
    {
        printk(KERN_INFO"%s: CS%1d: dev_id=%08x\n", __FUNCTION__, chipSelect, (unsigned int) *dev_id);
    }
    else
    {
        printk(KERN_INFO"%s: no multi-nand found %d\n", __FUNCTION__, i);
    }
}


/**
 * brcmnand_probe - [BrcmNAND Interface] Probe the BrcmNAND device
 * @param mtd        MTD device structure
 *
 * BrcmNAND detection method:
 *   Compare the the values from command with ones from register
 */
 #define MLC_DEVID_3RD_BYTE 0x14
 #define MLC_DEVID_4TH_BYTE 0xA5
 
static int brcmnand_probe(struct mtd_info *mtd, unsigned int chipSelect)
{
    struct brcmnand_chip *this = mtd->priv;
    unsigned char brcmnand_maf_id, brcmnand_dev_id;
    unsigned char devId3rdByte, devId4thByte;  
    uint32_t nand_config;
    int version_id;
    //int density;
    int i;

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    uint32_t pageSize = 0, pageSizeBits = 0;
    uint32_t blockSize = 0, blockSizeBits = 0;
#endif            

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3
    uint32_t oobSize = 0;
    uint32_t blkAdrBytes = 2, colAdrBytes = 2, fulAdrBytes = 4;
#endif
    /* Read manufacturer and device IDs from Controller */
    brcmnand_read_id(chipSelect, &this->device_id);

    brcmnand_maf_id = (this->device_id >> 24) & 0xff;
    brcmnand_dev_id = (this->device_id >> 16) & 0xff;
    devId3rdByte = (this->device_id >> 8) & 0xff;    
    devId4thByte = this->device_id & 0xff;
    /* Look up in our table for infos on device */
    for (i=0; i < BRCMNAND_MAX_CHIPS; i++) 
    {
        if (brcmnand_dev_id == brcmnand_chips[i].chipId 
            && brcmnand_maf_id == brcmnand_chips[i].mafId)
        {
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0        
            
            if ((devId3rdByte & SAMSUNG_3RDID_CELLTYPE_MASK) != 0)                 
            {
                if (brcmnand_chips[i].idOptions == BRCMNAND_ID_EXT_BYTES)
                {
                    printk(KERN_NOTICE "%s: Found MLC device %s\n", __FUNCTION__, 
                                brcmnand_chips[i].chipIdStr);
                    break;
                }
            }
            else
#endif            
            {
                if ( (brcmnand_chips[i].mafId == FLASHTYPE_HYNIX) && 
                     (brcmnand_chips[i].chipId == HYNIX_HY27UF081G2A) )
                {
                    if (devId3rdByte == HYNIX_3RD_ID_H27U1G8F2B)
                    {
                        i++; //Go to next device (H27U1G8F2B)
                    }
                }

                printk(KERN_NOTICE "%s: Found SLC device %s\n", __FUNCTION__, 
                                brcmnand_chips[i].chipIdStr);
                break; //break out of the 'for' loop
            }
        }
    }

    if (i >= BRCMNAND_MAX_CHIPS) {
#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_0
        printk(KERN_ERR "DevId %08x may not be supported\n", (unsigned int) this->device_id);
        /* Because of the bug in the controller in the first version,
         * if we can't identify the chip, we punt
         */
        return (-EINVAL);
#else
        printk(KERN_WARNING"DevId %08x is not supported. \n", (unsigned int) this->device_id);
        return (-EINVAL);
#endif
    }
    
    this->cellinfo = 0; // default to SLC, will read 3rd byte ID later for v3.0+ controller
    
#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    this->nandAccControlReg = BCHP_NAND_ACC_CONTROL;
    this->nandConfigReg = BCHP_NAND_CONFIG;
#else
    switch(this->CS[0])
    {
        case 0:
            this->nandConfigReg = BCHP_NAND_CONFIG;
            this->nandAccControlReg = BCHP_NAND_ACC_CONTROL;
        break;
        case 1:
            this->nandConfigReg = BCHP_NAND_CONFIG_CS1;
            this->nandAccControlReg = BCHP_NAND_ACC_CONTROL_CS1;
        break;
        case 2:
            this->nandConfigReg = BCHP_NAND_CONFIG_CS2;
            this->nandAccControlReg = BCHP_NAND_ACC_CONTROL_CS2;
        break;
        case 3:
            this->nandConfigReg = BCHP_NAND_CONFIG_CS3;
            this->nandAccControlReg = BCHP_NAND_ACC_CONTROL_CS3;
        break;
        default:
            this->nandConfigReg = BCHP_NAND_CONFIG;
            this->nandAccControlReg = BCHP_NAND_ACC_CONTROL;
        break;
    }
#endif //#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)

    nand_config = this->ctrl_read(this->nandConfigReg);

#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_0
    // Workaround for bug in 7400A0 returning invalid config
    switch(i) { 
    case 0: /* SamSung NAND 1Gbit */
    case 1: /* ST NAND 1Gbit */
    case 4:
    case 5:
        /* Large page, 128K erase block
        PAGE_SIZE = 0x1 = 1b = PG_SIZE_2KB
        BLOCK_SIZE = 0x1 = 01b = BK_SIZE_128KB
        DEVICE_SIZE = 0x5 = 101b = DVC_SIZE_128MB
        DEVICE_WIDTH = 0x0 = 0b = DVC_WIDTH_8
        FUL_ADR_BYTES = 5 = 101b
        COL_ADR_BYTES = 2 = 010b
        BLK_ADR_BYTES = 3 = 011b
        */
        nand_config &= ~0x30000000;
        nand_config |= 0x10000000; // bit 29:28 = 1 ===> 128K erase block
        //nand_config = 0x55042200; //128MB, 0x55052300  for 256MB
        this->ctrl_write(this->nandConfigReg, nand_config);

        break;

    case 2:
    case 3:
        /* Small page, 16K erase block
        PAGE_SIZE = 0x0 = 0b = PG_SIZE_512B
        BLOCK_SIZE = 0x0 = 0b = BK_SIZE_16KB
        DEVICE_SIZE = 0x5 = 101b = DVC_SIZE_128MB
        DEVICE_WIDTH = 0x0 = 0b = DVC_WIDTH_8
        FUL_ADR_BYTES = 5 = 101b
        COL_ADR_BYTES = 2 = 010b
        BLK_ADR_BYTES = 3 = 011b
        */
        nand_config &= ~0x70000000;
        this->ctrl_write(this->nandConfigReg, nand_config);

        break;
    
    default:
        printk(KERN_ERR "%s: DevId %08x not supported\n", __FUNCTION__, (unsigned int) this->device_id);
        BUG();
        break;
    }
#elif CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0

    if (brcmnand_chips[i].idOptions & BRCMNAND_ID_EXT_BYTES) 
    {
        unsigned char devId3rdByte =  (this->device_id >> 8) & 0xff;

        this->cellinfo = devId3rdByte & NAND_CI_CELLTYPE_MSK;


/* Read 5th ID byte if MLC type */

        if (this->cellinfo) {
            uint32_t devIdExt = this->ctrl_read(BCHP_NAND_FLASH_DEVICE_ID_EXT);
            uint8_t devId5thByte = (devIdExt & 0xff000000) >> 24;
            uint32_t nbrPlanes = 0;
            uint32_t planeSizeMB = 0, chipSizeMB = 0, nandConfigChipSize = 0;

/*---------------- 4th ID byte: page size, block size and OOB size ---------------- */
            switch(brcmnand_maf_id) {
            case FLASHTYPE_SAMSUNG:
            case FLASHTYPE_HYNIX:
            case FLASHTYPE_NUMONYX:
                pageSize = 1024 << (devId4thByte & SAMSUNG_4THID_PAGESIZE_MASK);
                blockSize = (64*1024) << ((devId4thByte & SAMSUNG_4THID_BLKSIZE_MASK) >> 4);
                //oobSize = devId4thByte & SAMSUNG_4THID_OOBSIZE_MASK ? 16 : 8;

                printk(KERN_NOTICE "%s() pageSize : %d, blockSize: %d", __FUNCTION__, pageSize, blockSize);         
                /* Update Config Register: Block Size */
                switch(blockSize) {
                case 512*1024:
                    blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_512KB;
                    break;
                case 128*1024:
                    blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_128KB;
                    break;
                case 16*1024:
                    blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_16KB;
                    break;
                case 256*1024:
                    blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_256KB;
                    break;
                }
                nand_config &= ~BCHP_NAND_CONFIG_BLOCK_SIZE_MASK;
                nand_config |= (blockSizeBits << BCHP_NAND_CONFIG_BLOCK_SIZE_SHIFT); 

                /* Update Config Register: Page Size */
                switch(pageSize) {
                case 512:
                    pageSizeBits = BCHP_NAND_CONFIG_PAGE_SIZE_PG_SIZE_512;
                    break;
                case 2048:
                    pageSizeBits = BCHP_NAND_CONFIG_PAGE_SIZE_PG_SIZE_2KB;
                    break;
                case 4096:
                    pageSizeBits = BCHP_NAND_CONFIG_PAGE_SIZE_PG_SIZE_4KB;
                    break;
                }
                nand_config &= ~BCHP_NAND_CONFIG_PAGE_SIZE_MASK ;
                nand_config |= (pageSizeBits << BCHP_NAND_CONFIG_PAGE_SIZE_SHIFT); 
                this->ctrl_write(this->nandConfigReg, nand_config);    
//                printk(KERN_INFO "Updating Config Reg: Block & Page Size: After: %08x\n", nand_config);
                break;
                
            default:
                printk(KERN_ERR "4th ID Byte: Device requiring Controller V3.0 in database, but not handled\n");
                //BUG();
            }
/*---------------- 5th ID byte ------------------------- */


            switch(brcmnand_maf_id) {
            case FLASHTYPE_SAMSUNG:
            case FLASHTYPE_HYNIX:
            case FLASHTYPE_NUMONYX:

printk(KERN_INFO "5th ID byte = %02x, extID = %08x\n", devId5thByte, devIdExt);

                switch(devId5thByte & SAMSUNG_5THID_NRPLANE_MASK) {
                case SAMSUNG_5THID_NRPLANE_1:
                    nbrPlanes = 1;
                    break;
                case SAMSUNG_5THID_NRPLANE_2:
                    nbrPlanes = 2;
                    break;
                case SAMSUNG_5THID_NRPLANE_4:
                    nbrPlanes = 4;
                    break;
                case SAMSUNG_5THID_NRPLANE_8:
                    nbrPlanes = 8;
                    break;
                }
printk(KERN_INFO "nbrPlanes = %d\n", nbrPlanes);

                switch(brcmnand_maf_id) {
                case FLASHTYPE_SAMSUNG:

                    /* Samsung Plane Size
                    #define SAMSUNG_5THID_PLANESZ_64Mb    0x00
                    #define SAMSUNG_5THID_PLANESZ_128Mb    0x10
                    #define SAMSUNG_5THID_PLANESZ_256Mb    0x20
                    #define SAMSUNG_5THID_PLANESZ_512Mb    0x30
                    #define SAMSUNG_5THID_PLANESZ_1Gb    0x40
                    #define SAMSUNG_5THID_PLANESZ_2Gb    0x50
                    #define SAMSUNG_5THID_PLANESZ_4Gb    0x60
                    #define SAMSUNG_5THID_PLANESZ_8Gb    0x70
                    */
                    // planeSize starts at (64Mb/8) = 8MB;
                    planeSizeMB = 8 << ((devId5thByte & SAMSUNG_5THID_PLANESZ_MASK) >> 4);
                    break;

                case FLASHTYPE_HYNIX:
                case FLASHTYPE_NUMONYX:
                    /* Hynix Plane Size 
                    #define HYNIX_5THID_PLANESZ_MASK    0x70
                    #define HYNIX_5THID_PLANESZ_512Mb    0x00
                    #define HYNIX_5THID_PLANESZ_1Gb        0x10
                    #define HYNIX_5THID_PLANESZ_2Gb        0x20
                    #define HYNIX_5THID_PLANESZ_4Gb        0x30
                    #define HYNIX_5THID_PLANESZ_8Gb        0x40
                    #define HYNIX_5THID_PLANESZ_RSVD1    0x50
                    #define HYNIX_5THID_PLANESZ_RSVD2    0x60
                    #define HYNIX_5THID_PLANESZ_RSVD3    0x70
                    */
                    // planeSize starts at (512Mb/8) = 64MB;
                    planeSizeMB = 64 << ((devId5thByte & SAMSUNG_5THID_PLANESZ_MASK) >> 4);
                    break;

                /* TBD Add other mfg ID here */

                }
                
                chipSizeMB = planeSizeMB*nbrPlanes;
                printk(KERN_INFO "planeSizeMB = %d, chipSizeMB=%d,0x%04x, planeSizeMask=%08x\n", planeSizeMB, chipSizeMB, chipSizeMB, devId5thByte & SAMSUNG_5THID_PLANESZ_MASK);

                /* NAND Config register starts at 4MB for chip size */
                nandConfigChipSize = ffs(chipSizeMB >> 2) - 1;

                printk(KERN_INFO "nandConfigChipSize = %04x\n", nandConfigChipSize);
                
                /* Correct chip Size accordingly, bit 24-27 */
                nand_config &= ~(BCHP_NAND_CONFIG_DEVICE_SIZE_MASK);
                nand_config |= (nandConfigChipSize << BCHP_NAND_CONFIG_DEVICE_SIZE_SHIFT); 

                this->ctrl_write(this->nandConfigReg, nand_config);
                                                
                break;

            default:
                printk(KERN_ERR "5th ID Byte: Device requiring Controller V3.0 in database, but not handled\n");
                //BUG();
            }
                
        }
    } //if (brcmnand_chips[i].idOptions & BRCMNAND_ID_EXT_BYTES) 
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3
    else
    {

        switch(brcmnand_maf_id) {
            case FLASHTYPE_HYNIX:
            case FLASHTYPE_SAMSUNG:
            case FLASHTYPE_NUMONYX:
            case FLASHTYPE_TOSHIBA:
                //Clear nand_config:
                nand_config &= ~(BCHP_NAND_CONFIG_BLOCK_SIZE_MASK | 
                                 BCHP_NAND_CONFIG_DEVICE_WIDTH_MASK |
                                 BCHP_NAND_CONFIG_PAGE_SIZE_MASK);
                                         
                /*---------------- 4th ID byte: page size, block size and OOB size ---------------- */                                             
                
                pageSize = 1024 << ((devId4thByte & SLC_4THID_PAGESIZE_MASK) >> SLC_4THID_PAGESIZE_SHIFT);
                blockSize = (64*1024) << ((devId4thByte & SLC_4THID_BLKSIZE_MASK) >> SLC_4THID_BLKSIZE_SHIFT);
                oobSize = ((devId4thByte & SLC_4THID_OOBSIZE_MASK)>> SLC_4THID_OOBSIZE_SHIFT) ? 16 : 8;

                printk(KERN_NOTICE "%s() oobSize: %d,  pageSize : %d, blockSize: %d", __FUNCTION__, oobSize, pageSize, blockSize);         
                /* Update Config Register: Block Size */
                switch(blockSize) {
                    case 512*1024:
                        blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_512KB;
                    break;
                    case 128*1024:
                        blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_128KB;
                    break;
                    case 16*1024:
                        blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_16KB;
                    break;
                    case 256*1024:
                        blockSizeBits = BCHP_NAND_CONFIG_BLOCK_SIZE_BK_SIZE_256KB;
                    break;
                    
                    default:
                        printk(KERN_ERR"Invalid blockSize %d", blockSize);
                        BUG();
                    break;
                }
                switch(pageSize){
                    case 512:
                        pageSizeBits = BCHP_NAND_CONFIG_PAGE_SIZE_PG_SIZE_512;
                    break;
                    case 2048:
                        pageSizeBits = BCHP_NAND_CONFIG_PAGE_SIZE_PG_SIZE_2KB;
                    break;
                    case 4096:
                        pageSizeBits = BCHP_NAND_CONFIG_PAGE_SIZE_PG_SIZE_4KB;
                    break;
                    default:
                        printk(KERN_ERR"Invalid pageSize %d", pageSize);
                        BUG();
                    break;
                }
                
                /*All devices connected to the nand controller can only be 8-bits wide*/
                nand_config |= (BCHP_NAND_CONFIG_DEVICE_WIDTH_DVC_WIDTH_8
                                     << BCHP_NAND_CONFIG_DEVICE_WIDTH_SHIFT);

                                    
                nand_config |= (pageSizeBits << BCHP_NAND_CONFIG_PAGE_SIZE_SHIFT);
                nand_config |= (blockSizeBits << BCHP_NAND_CONFIG_BLOCK_SIZE_SHIFT); 
            break;
            
            default:
                printk(KERN_ERR "4th ID Byte: Device requiring Controller V3.3 in database, but not handled\n");
            break;
        }  
    }
    
    /*---------------- Device size ----------------*/
    
    //Clear nand_config:
    nand_config &= ~(BCHP_NAND_CONFIG_DEVICE_SIZE_MASK |
                     BCHP_NAND_CONFIG_FUL_ADR_BYTES_MASK |
                     BCHP_NAND_CONFIG_COL_ADR_BYTES_MASK |
                     BCHP_NAND_CONFIG_BLK_ADR_BYTES_MASK);

    colAdrBytes = 2;    //Default for all devices
    
    if (brcmnand_chips[i].size <= BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_32MB)
    {
        blkAdrBytes = 1;
    }
    else if ((brcmnand_chips[i].size == BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_64MB) ||
             (brcmnand_chips[i].size == BCHP_NAND_CONFIG_DEVICE_SIZE_DVC_SIZE_128MB) )
    {
        blkAdrBytes = 2;
    }
    else //>= DVC_SIZE_256MB
    {
        blkAdrBytes = 3;
    }
                
    fulAdrBytes = colAdrBytes + blkAdrBytes;
                
    nand_config |= (brcmnand_chips[i].size << BCHP_NAND_CONFIG_DEVICE_SIZE_SHIFT);
    nand_config |= (colAdrBytes << BCHP_NAND_CONFIG_CS1_COL_ADR_BYTES_SHIFT);
    nand_config |= (blkAdrBytes << BCHP_NAND_CONFIG_CS1_BLK_ADR_BYTES_SHIFT);
    nand_config |= (fulAdrBytes << BCHP_NAND_CONFIG_CS1_FUL_ADR_BYTES_SHIFT);
                
    printk(KERN_DEBUG"Writing nand config register 0x%08X with 0x%08X\n", 
                        (int)this->nandConfigReg, 
                            nand_config);
                
    this->ctrl_write(this->nandConfigReg, nand_config);    
#endif   //#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3      
#endif   //#elif CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0

    brcmnand_decode_config(this, nand_config);

    // Also works for dummy entries, but no adjustments possible
    brcmnand_adjust_timings(this, &brcmnand_chips[i]);

    /* Flash device information */
    brcmnand_print_device_info(mtd, &brcmnand_chips[i], this->chipSize);
    this->options = brcmnand_chips[i].options;
        
    /* BrcmNAND page size & block size */    
    mtd->writesize = mtd->oobblock = this->pageSize; 
    
//The oobsize will need to be re-visited when we get devices that have larger oob's:
    mtd->oobsize = mtd->writesize >> 5; // tht - 16 byte OOB for 512B page, 64B for 2K page

    mtd->erasesize = this->blockSize;

    this->pLocalPage = kmalloc(mtd->writesize, GFP_KERNEL);
    if (this->pLocalPage == NULL)
    {
        printk(KERN_ERR "Out of memory for allocating this->pLocalPage\n");
        return (-EINVAL);
    }

    this->pLocalOob = kmalloc(mtd->oobsize, GFP_KERNEL);
    if (this->pLocalOob == NULL)
    {
        kfree(this->pLocalPage);
        printk(KERN_ERR "Out of memory for allocating this->pLocalOob");
        return (-EINVAL);
    }

    this->pEccMask = kmalloc(mtd->oobsize, GFP_KERNEL);
    if (this->pEccMask == NULL)
    {
        kfree(this->pLocalPage);
        kfree(this->pLocalOob);
        printk(KERN_ERR "Out of memory for allocating this->pEccMask");
        return (-EINVAL);
    }    

    /* Fix me: When we have both a NOR and NAND flash on board */
    /* For now, we will adjust the mtd->size for version 0.0 and 0.1 later in scan routine */
    this->mtdSize = __ll_mult32(this->chipSize, this->numchips);
    mtd->size = __ll_low(this->mtdSize);
    mtd->size_hi = __ll_high(this->mtdSize);

    /* Version ID */
    version_id = this->ctrl_read(BCHP_NAND_REVISION);
    printk(KERN_INFO "BrcmNAND version = 0x%04x %dMB @%p\n", 
        version_id, this->chipSize>>20, this->vbase);

    return 0;
}

/**
 * brcmnand_suspend - [MTD Interface] Suspend the BrcmNAND flash
 * @param mtd        MTD device structure
 */
static int brcmnand_suspend(struct mtd_info *mtd)
{
    DEBUG(MTD_DEBUG_LEVEL3, "-->%s  \n", __FUNCTION__);
    return brcmnand_get_device(mtd, FL_PM_SUSPENDED);
}

/**
 * brcmnand_resume - [MTD Interface] Resume the BrcmNAND flash
 * @param mtd        MTD device structure
 */
static void brcmnand_resume(struct mtd_info *mtd)
{
//    struct brcmnand_chip *this = mtd->priv;

    DEBUG(MTD_DEBUG_LEVEL3, "-->%s  \n", __FUNCTION__);
    if (thisDriver.state == FL_PM_SUSPENDED)
        brcmnand_release_device(mtd);
    else
        printk(KERN_ERR "resume() called for the chip which is not"
                "in suspended state\n");
}


/**
 * fill_autooob_layout - [NAND Interface] build the layout for hardware ECC case
 * @mtd:    MTD device structure
 * @eccbytes:    Number of ECC bytes per page
 *
 * Build the page_layout array for NAND page handling for hardware ECC
 * handling basing on the nand_oobinfo structure supplied for the chip
 */
static int fill_autooob_layout(struct mtd_info *mtd)
{
    struct brcmnand_chip *this = mtd->priv;
    struct nand_oobinfo *oob = this->ecclayout;
    int oobfreesize = 0;
    int i, res = 0;
    int eccpos = 0, eccbytes = 0, cur = 0, oobcur = 0;

    this->layout = kmalloc(HW_AUTOOOB_LAYOUT_SIZE * sizeof(struct page_layout_item), GFP_KERNEL);

    if (this->layout == NULL) {
        printk(KERN_ERR "%s: kmalloc failed\n", __FUNCTION__);
        return -ENOMEM;
    }
    else
        this->layout_allocated = 1;

    memset(this->layout, 0, HW_AUTOOOB_LAYOUT_SIZE * sizeof (struct page_layout_item));


    this->layout[0].type = ITEM_TYPE_DATA;
    this->layout[0].length = mtd->writesize;
    DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: data type, length %d\n", this->layout[0].length);

    i = 1;
    // THT: Our layout is not uniform across eccsteps, so we must scan the entire layout,
    // and we cannot replicate it.
    while (i < HW_AUTOOOB_LAYOUT_SIZE && cur < mtd->oobsize) {
        if (oob->oobfree[oobcur][0] == cur) {
            int len = oob->oobfree[oobcur][1];
            oobfreesize += this->layout[i].length;
            oobcur++;
            if (i > 0 && this->layout[i-1].type == ITEM_TYPE_OOBFREE) {
                i--;
                cur -= this->layout[i].length;
                this->layout[i].length += len;
                DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oobfree concatenated, aggregate length %d\n", this->layout[i].length);
            } else {
                this->layout[i].type = ITEM_TYPE_OOBFREE;
                this->layout[i].length = len;
                DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oobfree type, length %d\n", this->layout[i].length);
            }
        } else if (oob->eccpos[eccpos] == cur) {
            int eccpos_cur = eccpos;
            do  {
                eccpos++;
                eccbytes++;
            } while (eccbytes < oob->eccbytes && oob->eccpos[eccpos] == oob->eccpos[eccpos+1] - 1);
            eccpos++;
            eccbytes++;
            this->layout[i].type = ITEM_TYPE_ECC;
            this->layout[i].length = eccpos - eccpos_cur;
            DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: ecc type, length %d\n", this->layout[i].length);
        } else {
            int len = min_t(int, oob->eccpos[eccpos], oob->oobfree[oobcur][0]);
            if (i > 0 && this->layout[i-1].type == ITEM_TYPE_OOB) {
                i--;
                cur -= this->layout[i].length;
                this->layout[i].length += len;
                DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oob concatenated, aggregate length %d\n", this->layout[i].length);
            } else {
                this->layout[i].type = ITEM_TYPE_OOB;
                this->layout[i].length = len;
                DEBUG (MTD_DEBUG_LEVEL3, "fill_autooob_layout: oob type, length %d\n", this->layout[i].length);
            }
        }
        cur += this->layout[i].length;
        i++;
    }

    return res;
}


static void fill_ecccmp_mask(struct mtd_info *mtd)
{
    struct brcmnand_chip *this = mtd->priv;
    int i, len;
    struct nand_oobinfo* oobsel = this->ecclayout;
    unsigned char* myEccMask = (unsigned char*) this->pEccMask; // Defeat const

    /* 
     * Should we rely on this->pEccMask being zeroed out
     */
    memset(myEccMask, 0, mtd->oobsize);
    
    /* Write 0xFF where there is a free byte */
    for (i = 0, len = 0; len < mtd->oobavail && len < mtd->oobsize && i < ARRAY_SIZE(oobsel->oobfree); i++) {
        int to = oobsel->oobfree[i][0];
        int num = oobsel->oobfree[i][1];

        if (num == 0) break; // End marker reached
        memset (&myEccMask[to], 0xFF, num);
        len += num;
    }
}

#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
/*
 * Sort Chip Select array into ascending sequence, and validate chip ID
 * We have to sort the CS in order not to use a wrong order when the user specify
 * a wrong order in the command line.
 */
static int
brcmnand_sort_chipSelects(struct mtd_info *mtd , int maxchips, int* argNandCS, int* chipSelect)
{
    int i;
    int cs[MAX_NAND_CS];
    struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
    

    this->numchips = 0;
    for (i=0; i<MAX_NAND_CS; i++) {
        cs[i] = -1;
    }
    for (i=0; i<maxchips; i++) {
        cs[argNandCS[i]] = argNandCS[i];
    }
    for (i=0; i<MAX_NAND_CS;i++) {
        if (cs[i] != -1) {
            this->CS[this->numchips++] = cs[i];
            printk(KERN_INFO "CS[%d] = %d\n", i, cs[i]);
        }
    }

    return 0;
}
#endif
/*
 * Make sure that all NAND chips have same ID
 */
static int
brcmnand_validate_cs(struct mtd_info *mtd )
{
    struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
    int i;
    unsigned long dev_id;
    
    // Now verify that a NAND chip is at the CS
    for (i=0; i<this->numchips; i++) {
        brcmnand_read_id(this->CS[i], &dev_id);

        if (dev_id != this->device_id) {
            printk(KERN_ERR "Device ID for CS[%1d] = %08lx, Device ID for CS[%1d] = %08lx\n",
                this->CS[0], this->device_id, this->CS[i], dev_id);
            return 1;
        }
    }
    return 0;
}

/*
 * Description: This function reads the oob area 
 *              and (if specified) 
 *              the data associated with it.
 *
 *  ==>The returned data can either include the ECC/bad block info or not
 *
 * Parameters:
 *  mtd:    IN: Memory Technology Device structure
 *  from:   IN: Data address where to get the oob from 
 *  ops:    IN: operation to perform (read data & oob or not)
 *
 *
 */
static int tr_read_oob(struct mtd_info *mtd, 
    loff_t from, struct mtd_oob_ops *ops)
{
    size_t len = ops->len;
    u_char *oobbuf = ops->oobbuf;
    u_char *datbuf = ops->datbuf;
    size_t retlen = 0;
    int result = 0;
    struct nand_oobinfo oobsel;
    struct brcmnand_chip *this = mtd->priv;

    ops->retlen = ops->len;
    ops->oobretlen = ops->ooblen;

    if (oobbuf != NULL && datbuf != NULL )
    {
        //Copy the autooob structure for free bytes vs other bytes placement:
        memcpy (&oobsel, this->ecclayout, sizeof(oobsel));

        switch(ops->mode)
        {
            case MTD_OOB_PLACE:
            case MTD_OOB_RAW:
                /* Setting oobsel.useecc to MTD_NANDECC_PLACE will permit
                   brcmnand_read_ecc() to return the raw oob:
                */
                oobsel.useecc = MTD_NANDECC_PLACE; 
            break;
                                
            case MTD_OOB_AUTO:
                oobsel.useecc = MTD_NANDECC_AUTOPLACE; 
            break;
                
            default:
                printk (KERN_ERR "%s: Invalid OOB mode (%d) while reading oob & data\n",
                        __FUNCTION__, ops->mode);
                return result;                
            break;
        }                           
                           
    
        if(ops->datbuf == NULL && ops->len < 512)
        {
            printk(KERN_INFO "Buffer smaller than 512 and ops->datbuf == NULL\n");
        }
        
        result = brcmnand_read_ecc(mtd, from, len, &retlen, datbuf, oobbuf, &oobsel);
    }
    else if(ops->oobbuf != NULL)
    {
        switch(ops->mode)
        {
            case MTD_OOB_AUTO:
                result = brcmnand_read_oobfree(mtd, from, ops->ooblen, &retlen, oobbuf);            
            break;
            
            case MTD_OOB_PLACE:
            case MTD_OOB_RAW:
                result = brcmnand_read_oob(mtd, from, ops->ooblen, &retlen, oobbuf);
            break;

            default:
                printk (KERN_ERR "%s: Invalid OOB mode (%d) while reading oob only\n",
                        __FUNCTION__, ops->mode);
                return result;
            break;
        } 
    }
    else
    {
        printk(KERN_ERR "%s: UNKNOWN CASE!!!!\n", __FUNCTION__);
    }
    
    return result;
}

static int tr_write_oob (struct mtd_info *mtd, loff_t to, 
    struct mtd_oob_ops *ops)
{
    u_char *oobbuf = ops->oobbuf;
    u_char *datbuf = ops->datbuf;
    size_t retlen;
    int result = 0;
    struct nand_oobinfo oobsel;
    struct brcmnand_chip *this = mtd->priv;

    ops->retlen = ops->len;
    ops->oobretlen = ops->ooblen;


    if (oobbuf != NULL && datbuf != NULL )
    {
        memcpy (&oobsel, this->ecclayout, sizeof(oobsel));
        
        switch(ops->mode)
        {
            case MTD_OOB_PLACE:
            case MTD_OOB_RAW:
                /* Setting oobsel.useecc to MTD_NANDECC_PLACE will permit
                   brcmnand_read_ecc() to return the raw oob:
                */
                oobsel.useecc = MTD_NANDECC_PLACE; 
            break;
                                
            case MTD_OOB_AUTO:
                oobsel.useecc = MTD_NANDECC_AUTOPLACE; 
            break;
                
            default:
                printk (KERN_ERR "%s: Invalid OOB mode (%d)\n",
                        __FUNCTION__, ops->mode);
                return result;                
            break;
        }                           
        
        result = brcmnand_write_ecc(mtd, to, ops->len, &retlen, datbuf, oobbuf, &oobsel);
    }
    else if(ops->oobbuf != NULL)
    {
        switch(ops->mode)
        {
            case MTD_OOB_AUTO:
                result = brcmnand_write_oobfree(mtd, to, ops->ooblen, &retlen, oobbuf);
            break;
            
            case MTD_OOB_RAW:
                result = brcmnand_write_oob(mtd, to, ops->ooblen, &retlen, oobbuf);
            break;

            default:
                printk (KERN_ERR "%s: Invalid OOB mode (%d)\n",
                        __FUNCTION__, ops->mode);
                return result;
            break;
        }                           
    }
    else
    {
        printk(KERN_ERR "%s: UNKNOWN CASE!!!!\n", __FUNCTION__);
    }
    
    return result;
}

/**
 * brcmnand_read_page_oob - {REPLACABLE] hardware ecc based page read function
 * @mtd:    mtd info structure
 * @chip:    nand chip info structure.  The OOB buf is stored in the oob_poi ptr on return
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
static int 
brcmnand_read_page_oob(struct mtd_info *mtd, 
                uint8_t* outp_oob, uint32_t  page)
{
    struct brcmnand_chip *chip = (struct brcmnand_chip*) mtd->priv;
    int winslice;
    int dataRead = 0;
    int oobRead = 0;
    int ret = 0, retFinal = 0;
    loff_t offset = (loff_t)page << chip->page_shift;

    int nPageNumber;
    L_OFF_T blockRefreshOffset;


    L_OFF_T eccCorOffset = offset;
    int eccCorWinslice = 0;
    
if (gdebug > 3 ) {
printk("-->%s, offset=%0llx\n", __FUNCTION__, offset);}

    chip->pagebuf = page;

    for (winslice = 0; winslice < chip->eccsteps && ret == 0; winslice++) {

        ret = brcmnand_posted_read_oob(mtd, &outp_oob[oobRead], 
                    offset + dataRead, 1);

        if (ret == -EECCUNCOR) {
            retFinal = ret;
            break; //out of the for loop
        } 
        else if (ret == -EECCCOR)
        {
            printk("ECC COR!\n"); 
            retFinal = ret;
            eccCorOffset = offset + dataRead;
            eccCorWinslice = winslice;
            
            if (retFinal == 0)
            {
                retFinal = ret;
            }
        }        
     
      
        dataRead += chip->eccsize;
        oobRead += chip->eccOobSize;
    }
    if (retFinal == -EECCCOR)
    {
//mfi: added
            if (chip->ecclevel == BRCMNAND_ECC_HAMMING)
            {
                struct nand_oobinfo place_oob;
                int retdatalen, retooblen;
                L_OFF_T pageAddr = __ll_and32(offset, ~ (mtd->writesize - 1));
                place_oob = mtd->oobinfo;
                place_oob.useecc = MTD_NANDECC_PLACE;
                
                /* Read the entire page into the default buffer */
                ret = brcmnand_read_page(mtd, pageAddr, chip->pLocalPage, 
                                            mtd->writesize, NULL, 
                                            &place_oob, &retdatalen, &retooblen,
                                            true);

                if (ret == -EECCCOR)
                {   
                    if(chip->ecclevel == BRCMNAND_ECC_HAMMING && runningInA7601A == true)
                    {
                        //Perform refresh block logic at page level:
                        printk(KERN_DEBUG"%s: Calling hamming_code_fix\n", __FUNCTION__);
                    
                        chip->hamming_code_fix(mtd, offset, 0);

                        printk(KERN_NOTICE"%s: Performing refresh block logic\n", __FUNCTION__);
                
                        nPageNumber = chip->deduce_blockOffset(mtd, pageAddr, &blockRefreshOffset);

                        ret = chip->refresh_block(mtd, chip->pLocalPage, chip->pLocalOob, nPageNumber, blockRefreshOffset, true);
                        /* this is read refresh update refresh stats counter */
                        chip->stats.blockRefreshed++;

                    }
                    else
                    {    
                        printk(KERN_DEBUG"%s: Performing refresh block logic\n", __FUNCTION__);
                
                        nPageNumber = chip->deduce_blockOffset(mtd, pageAddr, &blockRefreshOffset);

                        ret = chip->refresh_block(mtd, NULL, NULL, nPageNumber, blockRefreshOffset, true);
                        /* this is read refresh update refresh stats counter */
                        chip->stats.blockRefreshed++;
                        
                    }
                }
            }
            else
            {    
                printk(KERN_DEBUG"%s: Performing refresh block logic\n", __FUNCTION__);
                nPageNumber = chip->deduce_blockOffset(mtd, offset, &blockRefreshOffset);

                ret = chip->refresh_block(mtd, NULL, NULL, nPageNumber, blockRefreshOffset, true);
                /* this is read refresh update refresh stats counter */
                chip->stats.blockRefreshed++;

            }
//mfi: end of addition 
    }


if (gdebug > 3 ) {
printk("<--%s offset=%0llx\n", __FUNCTION__, offset);
print_oobbuf(outp_oob, mtd->oobsize); }
    return ret;
}
/**
 * brcmnand_write_page_oob - [INTERNAL] write one page
 * @mtd:    MTD device structure
 * @chip:    NAND chip descriptor.  The oob_poi pointer points to the OOB buffer.
 * @page:    page number to write
 */
static int brcmnand_write_page_oob(struct mtd_info *mtd, 
               const uint8_t* inp_oob, uint32_t page)
{
    struct brcmnand_chip *chip = (struct brcmnand_chip*) mtd->priv;
    loff_t offset = (loff_t)page << chip->page_shift;
        
    int ret = brcmnand_write_pageoob (mtd, offset, mtd->oobsize, inp_oob);

    return ret;
}

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3


int brcmnand_prescan(int cs, uint32_t* flash_id)
{
    int retVal = 0;
    volatile uint32_t csNandSelect = 0;
    volatile uint32_t csAddrXor = 0;

    //1st of all, initialize NAND_CS_NAND_SELECT so that this cs "USES NAND":
    csNandSelect = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
    
    csNandSelect |= (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_USES_NAND_MASK << cs);
    csNandSelect |= (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK << cs);
    
    brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, csNandSelect);
    
    //Read device ID:
    brcmnand_read_id(cs, (unsigned long*)flash_id);

    if (*flash_id == 0)  //device not present
    {
        //Mark not used:
        csNandSelect &= ~((BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_USES_NAND_MASK | 
                            BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK) << cs);
        brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, csNandSelect);
        //...and remove from extended address:
        brcmnand_ctrl_write(BCHP_NAND_CMD_EXT_ADDRESS, 0);
        retVal = -1;
    }
    else //device present
    {
        //Set this CS not so use XOR:
        csAddrXor = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_XOR);
        csAddrXor &= ~(BCHP_NAND_CS_NAND_XOR_EBI_CS_0_ADDR_1FC0_XOR_MASK << cs);

        printk(KERN_DEBUG"%s: Writing 0x%08X in BCHP_NAND_CS_NAND_XOR\n", __FUNCTION__, csAddrXor);
        brcmnand_ctrl_write(BCHP_NAND_CS_NAND_XOR, csAddrXor);
    }
    return retVal;
}
#else //CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3

int brcmnand_prescan(int cs, uint32_t* flash_id)
{
    //Read device ID:
    int retVal = -1;
    volatile uint32_t csAddrXor = 0;
     
    brcmnand_read_id(cs, (unsigned long*)flash_id);

    if (*flash_id != 0)
    {   
        retVal = 0;
        
#if (CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0)            
        //Set this CS not so use XOR:
        csAddrXor = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_XOR);
        printk(KERN_DEBUG"%s: BCHP_NAND_CS_NAND_XOR before: 0x%08X", __FUNCTION__, csAddrXor);
        csAddrXor &= ~(BCHP_NAND_CS_NAND_XOR_EBI_CS_0_ADDR_1FC0_XOR_MASK << cs);
        printk(KERN_DEBUG"%s: BCHP_NAND_CS_NAND_XOR after: 0x%08X", __FUNCTION__, csAddrXor);

        brcmnand_ctrl_write(BCHP_NAND_CS_NAND_XOR, csAddrXor);
#endif //#if (CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0)   
         
    }
    
    return retVal;
}
#endif //CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3
/**
 * brcmnand_scan - [BrcmNAND Interface] Scan for the BrcmNAND device
 * @param mtd        MTD device structure
 * @param maxchips    Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 *
 * THT: For now, maxchips should always be 1.
 */
int brcmnand_scan(struct mtd_info *mtd , int maxchips )
{
    struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
    unsigned char brcmnand_maf_id;
    int err, i;
    volatile unsigned long nand_select, cs;
    unsigned int version_id;
    unsigned int version_major;
    unsigned int version_minor;
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    volatile unsigned long acc_control, org_acc_control;
#endif //
    if (!this->ctrl_read)
        this->ctrl_read = brcmnand_ctrl_read;
    if (!this->ctrl_write)
        this->ctrl_write = brcmnand_ctrl_write;
    if (!this->ctrl_writeAddr)
        this->ctrl_writeAddr = brcmnand_ctrl_writeAddr;

    if (!this->read_raw)
        this->read_raw = brcmnand_read_raw;
    if (!this->read_pageoob)
        this->read_pageoob = brcmnand_read_pageoob;

    if (!this->read_page_oob)
        this->read_page_oob = brcmnand_read_page_oob;
    if (!this->write_page_oob)
        this->write_page_oob = brcmnand_write_page_oob;
    if (!this->read_oob)
        this->read_oob = tr_read_oob;
    if (!this->write_oob)
        this->write_oob = tr_write_oob;;

    if (!this->verify_ecc)
        this->verify_ecc =  brcmnand_EDU_verify_ecc;
        
    if (!this->deduce_blockOffset)
        this->deduce_blockOffset = brcmnand_deduce_blockOffset;
    
    if (!this->refresh_block)
        this->refresh_block = brcmnand_refresh_block;

    if (!this->hamming_calc_ecc)
        this->hamming_calc_ecc = brcmnand_hamming_calc_ecc;

    if (!this->hamming_code_fix)
        this->hamming_code_fix = brcmnand_hamming_code_fix;

    if (!this->ctrl_write_is_complete)
        this->ctrl_write_is_complete = brcmnand_ctrl_write_is_complete;

    if (!this->convert_oobfree_to_fsbuf)
        this->convert_oobfree_to_fsbuf = brcmnand_convert_oobfree_to_fsbuf;
    
    if (!this->wait)
        this->wait = brcmnand_wait;

    if (!this->block_markbad)
        this->block_markbad = brcmnand_default_block_markbad;
    if (!this->scan_bbt)
        this->scan_bbt = brcmnand_default_bbt;
    if (!this->erase_blocks)
        this->erase_blocks = brcmnand_erase_blocks;

#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_0
    cs = 0;
    this->CS[0] = 0;
    this->numchips = 1;

#elif CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_0_1
    /*
     * Read NAND strap option to see if this is on CS0 or CSn 
     */
    {
        int i;
        
        nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
        cs = 0;
        for (i=0; i<6; i++) {
            if (nand_select & (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK<<i)) {
                this->CS[0] = cs = i;
                break;  // Only 1 chip select is allowed
            }
        }
    }
    this->numchips = 1;

#elif (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    /*
      * For now, we can only support up to 1 chip using BCHP_NAND_CS_NAND_SELECT.  
      * We have to use kernel command line parameter 
      * to support more than one chip selects.  
      * May be future HW will allow us to read BCHP_NAND_CS_NAND_SELECT again.
      */
    /*
     * Read NAND strap option to see if this is on CS0 or CSn 
     */
    if (gNumNand == 0) {
        int i;
        
        nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
        cs = 0;
        // We start at 1 since 0 is used for Direct Addressing.
        // These bits are functional in v1.0 for backward compatibility, but we can only select 1 at a time.
        for (i=1; i<6; i++) {
            if (nand_select & (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK<<i)) {
                this->CS[0] = cs = i;
                break;  // Only 1 chip select is allowed
            }
        }
        this->numchips = 1;
    }
    else { // Chip Select via Kernel parameters, currently the only way to allow more than one CS to be set
        cs = this->numchips = gNumNand;
        if (brcmnand_sort_chipSelects(mtd, maxchips, gNandCS, this->CS))
            return (-EINVAL);
    }
#elif (CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3)
    // Only 1 chip select is allowed:
    cs = maxchips;
    this->CS[0] = cs;
    this->numchips = 1;
#endif


PRINTK("brcmnand_scan: Calling brcmnand_probe\n");
    if (brcmnand_probe(mtd, cs))
        return -ENXIO;

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
    if (this->numchips > 0) {
        if (brcmnand_validate_cs(mtd))
            return (-EINVAL);
    }
#endif

PRINTK("brcmnand_scan: Done brcmnand_probe\n");

#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_0_1    
    if (cs) {
        volatile unsigned long wr_protect;
        volatile unsigned long acc_control;

        
        if (this->chipSize >= (128 << 20)) {
            this->pbase = 0x11000000; /* Skip 16MB EBI Registers */
            mtd->size = this->chipSize - (16<<20); // Maximum size on a 128MB/256MB flash
            mtd->size_hi = 0;
            this->mtdSize = mtd->size;
        }
/*
        else if (this->chipSize == (256 << 20)) {
            this->pbase = 0x11000000; // Skip 16MB EBI Registers 
            mtd->size = 240<<20; // Maximum size on a 256MB flash, provided CS0/NOR is disabled
        }
 */
        else {
            this->pbase = 0x18000000 - this->chipSize;
            mtd->size = this->chipSize;
            mtd->size_hi = 0;
            this->mtdSize = mtd->size;
        }
        printk(KERN_NOTICE "Found NAND chip on Chip Select %d, chipSize=%dMB, usable size=%dMB, base=%08x\n", 
            cs, this->chipSize>>20, mtd->size>>20, this->pbase);

        /*
         * When NAND is on CS0, it reads the strap values and set up accordingly.
         * WHen on CS1, some configurations must be done by SW
         */

        // Set Write-Unprotect.  This register is sticky, so if someone already set it, we are out of luck
        wr_protect = brcmnand_ctrl_read(BCHP_NAND_BLK_WR_PROTECT);
        if (wr_protect) {
            printk(KERN_WARNING "Unprotect Register B4: %08x.  Please do a hard power recycle to reset\n", wr_protect);
            // THT: Actually we should punt here, as we cannot zero the register.
        } 
        brcmnand_ctrl_write(BCHP_NAND_BLK_WR_PROTECT, 0); // This will not work.
        if (wr_protect) {
            printk(KERN_DEBUG "Unprotect Register after: %08x\n", brcmnand_ctrl_read(BCHP_NAND_BLK_WR_PROTECT));
        }

        // Enable HW ECC.  This is another sticky register.
        acc_control = brcmnand_ctrl_read(this->nandAccControlReg);
        printk(KERN_DEBUG "ACC_CONTROL B4: %08x\n", acc_control);
         
        brcmnand_ctrl_write(this->nandAccControlReg, acc_control | BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK);
        if (!(acc_control & BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK)) {
            printk(KERN_DEBUG "ACC_CONTROL after: %08x\n", brcmnand_ctrl_read(this->nandAccControlReg));
        }
    }
    else {
        /* NAND chip on Chip Select 0 */
        this->CS[0] = 0;
    
        /* Set up base, based on flash size */
        if (this->chipSize >= (256 << 20)) {
            this->pbase = 0x12000000;
            mtd->size = 0x20000000 - this->pbase; // THT: This is different than this->chipSize
        } else {
            /* We know that flash endAddr is 0x2000_0000 */
            this->pbase = 0x20000000 - this->chipSize;
            mtd->size = this->chipSize;
        }
        mtd->size_hi = 0;
        this->mtdSize = mtd->size;

        printk(KERN_INFO "Found NAND chip on Chip Select 0, size=%dMB, base=%08x\n", mtd->size>>20, this->pbase);
    }
    this->vbase = (void*) KSEG1ADDR(this->pbase);
    
#else
    /*
     * v1.0 controller and after
     */

    //pbase is the "physical" start of flash. Physical means how Linux sees it,
    // The "physical" start of the flash is always at 0
    // this driver XORs the physical address with 0x1FC0_0000 in order to
    // move the address into the "logical" world (as seem by the bcm chip)
    this->pbase = 0; 
   
    // vbase is the address of the flash cache array
    this->vbase = (void*) (0xb0000000+BCHP_NAND_FLASH_CACHEi_ARRAY_BASE);  // Start of Buffer Cache
    // Already set in probe mtd->size = this->chipSize * this->numchips;
    // Make sure we use Buffer Array access, not direct access, Clear CS0
    nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
    printk(KERN_DEBUG "%s: B4 nand_select = %08x\n", __FUNCTION__, (uint32_t) nand_select);
    //this->directAccess = !(nand_select & BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK);
    // THT: Clear Direct Access bit
#if (CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_3)
    nand_select &= ~(BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK);
    this->directAccess = !(nand_select & BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK);
#elif (CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_3)
    nand_select &= ~(BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK << this->CS[0]);
    this->directAccess = !(nand_select & (BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK << this->CS[0]));
#endif    
    
    brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, nand_select);

    nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
    printk(KERN_DEBUG "%s: After nand_select = %08x\n", __FUNCTION__, (uint32_t)  nand_select);
    
#endif

#if CONFIG_MTD_BRCMNAND_VERSION == CONFIG_MTD_BRCMNAND_VERS_1_0
    switch (mtd->oobsize) 
    {
    case 64:
        printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_64\n",__FUNCTION__);
        this->ecclayout = &brcmnand_oob_64;
        
        /*
         * Adjust oobfree for ST chips, which also uses the 6th byte,
         * in addition to the first byte to mark a bad block
         */
        brcmnand_maf_id = (this->device_id >> 24) & 0xff;
        if (brcmnand_maf_id == FLASHTYPE_NUMONYX) {
            this->ecclayout->oobfree[0][1] = 3; // Exclude 6th byte from OOB free
        }
    break;
    case 16:
        printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_16\n",__FUNCTION__);
        this->ecclayout = &brcmnand_oob_16;
    break;
    default:
        /* To prevent kernel oops */
        printk(KERN_ERR "%s: Setting ecclayout to brcmnand_oob_16: Invalid oobsize %d\n",__FUNCTION__, mtd->oobsize);
        this->ecclayout = &brcmnand_oob_16;
    break;
    }


#elif CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0
    /*
     * We only support same ECC level for both block0 and other blocks
     */
    // Verify BCH-4 ECC: Handle CS0 block0;  
    // ==>We rely on this because it is a strapping option<==
    
    
    org_acc_control = acc_control = brcmnand_ctrl_read(this->nandAccControlReg);

    switch(acc_control & BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_MASK)
    {
    case BCHP_NAND_ACC_CONTROL_0_HAMMING:
    default:
        acc_control &= ~(BCHP_NAND_ACC_CONTROL_ECC_LEVEL_MASK);
        acc_control |= BCHP_NAND_ACC_CONTROL_N_HAMMING;
                    
        printk(KERN_INFO "Setting ECC to Hamming: ACC_CONTROL = %08lx from %08lx\n", acc_control, org_acc_control);
        brcmnand_ctrl_write(this->nandAccControlReg, acc_control );            
        
        this->ecclevel = BRCMNAND_ECC_HAMMING;

        switch (mtd->oobsize) 
        {
        case 64:
            printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_64\n",__FUNCTION__);
            this->ecclayout = &brcmnand_oob_64;
        /*
         * Adjust oobfree for ST chips, which also uses the 6th byte,
         * in addition to the first byte to mark a bad block
         */
            brcmnand_maf_id = (this->device_id >> 24) & 0xff;
            if (brcmnand_maf_id == FLASHTYPE_NUMONYX) {
                this->ecclayout->oobfree[0][1] = 3; // Exclude 6th byte from OOB free
            }
            
        break;
        case 16:
            printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_16\n",__FUNCTION__);
            this->ecclayout = &brcmnand_oob_16;
        break;
        default:
            /* To prevent kernel oops */
            printk(KERN_ERR "%s: Setting ecclayout to brcmnand_oob_16: Invalid oobsize %d\n",__FUNCTION__, mtd->oobsize);
            this->ecclayout = &brcmnand_oob_16;
        break;
        
        }
    break;

    case BCHP_NAND_ACC_CONTROL_0_BCH_4:
        acc_control &= ~(BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_MASK |
                    BCHP_NAND_ACC_CONTROL_ECC_LEVEL_MASK);
                    
        acc_control |= BCHP_NAND_ACC_CONTROL_0_BCH_4 | 
                    BCHP_NAND_ACC_CONTROL_N_BCH_4;

        printk(KERN_INFO "Setting ECC to BCH-4: ACC_CONTROL = %08lx from %08lx\n", acc_control, org_acc_control);                                                
        brcmnand_ctrl_write(this->nandAccControlReg, acc_control );

       
        this->ecclevel = BRCMNAND_ECC_BCH_4;

        /*Set BCH4 THRESHOLD */
        printk(KERN_INFO "Setting BCH4 THRESHOLD to %u\n", BCHN_THRESHOLD(this->ecclevel));
        this->ctrl_write(BCHP_NAND_CORR_STAT_THRESHOLD, BCHN_THRESHOLD(this->ecclevel));

        /* The number of bytes available for the filesystem to place fs dependend
        * oob data */
        printk(KERN_INFO "%s: mtd->oobsize = %d\n", __FUNCTION__, mtd->oobsize);

        switch (mtd->oobsize) 
        {
        case 128:
            switch (mtd->writesize) 
            {
            case 4096:
                printk(KERN_NOTICE "%s: Setting ecclayout to brcmnand_oob_bch4_4k\n", __FUNCTION__);
                this->ecclayout = &brcmnand_oob_bch4_4k;
            break;

            default:
                printk(KERN_ERR "Unsupported page size of %d\n", mtd->writesize);
                BUG();
            break;
            }
        break;

        case 64:
            switch (mtd->writesize) 
            {
            case 4096:
                printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_bch4_4k\n",__FUNCTION__);
                this->ecclayout = &brcmnand_oob_bch4_4k;
            break;
            case 2048:
                printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_bch4_2k\n",__FUNCTION__);
                this->ecclayout = &brcmnand_oob_bch4_2k;
            break;
            default:
                printk(KERN_ERR "Unsupported page size of %d\n", mtd->writesize);
                BUG();
            break;
            }
        break;
        
        default:
            /* To prevent kernel oops */
            printk(KERN_ERR "%s: Setting ecclayout to brcmnand_oob_bch4_2k: Invalid oobsize %d\n",__FUNCTION__, mtd->oobsize);
            this->ecclayout = &brcmnand_oob_bch4_2k;
        break;
        }
        
        break;

#ifdef BCHP_NAND_ACC_CONTROL_0_BCH_8
    case BCHP_NAND_ACC_CONTROL_0_BCH_8:
        acc_control &= ~(BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_MASK |
                    BCHP_NAND_ACC_CONTROL_ECC_LEVEL_MASK);
                    
        acc_control |= BCHP_NAND_ACC_CONTROL_0_BCH_8 | 
                    BCHP_NAND_ACC_CONTROL_N_BCH_8;

        printk(KERN_INFO "Setting ECC to BCH-8: ACC_CONTROL = %08lx from %08lx\n", acc_control, org_acc_control);                                                
        brcmnand_ctrl_write(this->nandAccControlReg, acc_control );

        this->ecclevel = BRCMNAND_ECC_BCH_8;

        /*Set BCH8 THRESHOLD */
        printk(KERN_INFO "Setting BCH8 THRESHOLD to %u\n", BCHN_THRESHOLD(this->ecclevel));
        this->ctrl_write(BCHP_NAND_CORR_STAT_THRESHOLD, BCHN_THRESHOLD(this->ecclevel));

        /* The number of bytes available for the filesystem to place fs dependend
        * oob data */
        printk(KERN_INFO "%s: mtd->oobsize = %d\n", __FUNCTION__, mtd->oobsize);

        switch (mtd->oobsize) 
        {
        case 128:
            switch (mtd->writesize) 
            {
            case 4096:
                printk(KERN_NOTICE "%s: Setting ecclayout to brcmnand_oob_bch8_4k\n", __FUNCTION__);
                this->ecclayout = &brcmnand_oob_bch8_4k;
            break;

            default:
                printk(KERN_ERR "Unsupported page size of %d\n", mtd->writesize);
                BUG();
            break;
            }
        break;

        case 64:
            switch (mtd->writesize) 
            {
            case 4096:
                printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_bch8_4k\n",__FUNCTION__);
                this->ecclayout = &brcmnand_oob_bch8_4k;
            break;
            case 2048:
                printk(KERN_INFO "%s: Setting ecclayout to brcmnand_oob_bch8_2k\n",__FUNCTION__);
                this->ecclayout = &brcmnand_oob_bch8_2k;
            break;
            default:
                printk(KERN_ERR "Unsupported page size of %d\n", mtd->writesize);
                BUG();
            break;
            }
        break;
        
        default:
            /* To prevent kernel oops */
            printk(KERN_ERR "%s: Setting ecclayout to brcmnand_oob_bch8_2k: Invalid oobsize %d\n",__FUNCTION__, mtd->oobsize);
            this->ecclayout = &brcmnand_oob_bch8_2k;
        break;
        }
     
        break;
#endif //BCHP_NAND_ACC_CONTROL_0_BCH_8 
    }

    /* Handle Partial Write Enable configuration for MLC
     * {FAST_PGM_RDIN, PARTIAL_PAGE_EN}
     * {0, 0} = 1 write per page, no partial page writes (required for MLC flash, suitable for SLC flash)
     * {1, 1} = 4 partial page writes per 2k page (SLC flash only)
     * {0, 1} = 8 partial page writes per 2k page (not recommended)
     * {1, 0} = RESERVED, DO NOT USE
     */
            
    if (NAND_IS_MLC(this)) 
    {
        /* Set FAST_PGM_RDIN, PARTIAL_PAGE_EN  to {0, 0} for NOP=1 */
        if (acc_control & BCHP_NAND_ACC_CONTROL_FAST_PGM_RDIN_MASK) 
        {
            acc_control &= ~(BCHP_NAND_ACC_CONTROL_FAST_PGM_RDIN_MASK);
            brcmnand_ctrl_write(this->nandAccControlReg, acc_control );
            printk(KERN_INFO "Corrected FAST_PGM_RDIN: ACC_CONTROL = %08lx\n", acc_control);
        }
        if (acc_control & BCHP_NAND_ACC_CONTROL_PARTIAL_PAGE_EN_MASK) 
        {
            acc_control &= ~(BCHP_NAND_ACC_CONTROL_PARTIAL_PAGE_EN_MASK);
            brcmnand_ctrl_write(this->nandAccControlReg, acc_control );
            printk(KERN_INFO "Corrected PARTIAL_PAGE_EN: ACC_CONTROL = %08lx\n", acc_control);
        }            
        /* THT Disable Optimization for 2K page */
        printk(KERN_INFO "Disabling WR_PREEMPT, but enabling PAGE_HIT_EN\n");
        
        acc_control &= ~(BCHP_NAND_ACC_CONTROL_WR_PREEMPT_EN_MASK);

        acc_control |= BCHP_NAND_ACC_CONTROL_PAGE_HIT_EN_MASK;                         
        
        brcmnand_ctrl_write(this->nandAccControlReg, acc_control );

        printk(KERN_INFO "ACC_CONTROL for MLC NAND: %08lx\n", acc_control);
    }
    else 
    {   
        volatile unsigned long acc_control;


        printk(KERN_INFO "Disabling PARTIAL_PAGE\n");
        printk(KERN_INFO "Enabling PAGE_HIT\n");
        printk(KERN_INFO "Enabling WR_PREEMPT\n");
        
        acc_control = brcmnand_ctrl_read(this->nandAccControlReg);
        
        acc_control &= ~(BCHP_NAND_ACC_CONTROL_PARTIAL_PAGE_EN_MASK);
        acc_control |= BCHP_NAND_ACC_CONTROL_PAGE_HIT_EN_MASK;
        acc_control |= BCHP_NAND_ACC_CONTROL_WR_PREEMPT_EN_MASK;
        
        this->ctrl_write(this->nandAccControlReg, acc_control);
                       
        printk(KERN_INFO "ACC_CONTROL for SLC NAND: %08lx\n", acc_control);
    }
#endif // Version 3.0+


///////////////        
    


    //Find first bit set in mtd->erasesize and return its index (from 1):
    this->bbt_erase_shift =  ffs(mtd->erasesize) - 1;

    /* Calculate the address shift from the page size */    
    //Find first bit set in mtd->writesize and return its index (from 1):
    this->page_shift = ffs(mtd->writesize) - 1;
    this->bbt_erase_shift = this->phys_erase_shift = ffs(mtd->erasesize) - 1;
    
    //Find first bit set in this->chipSize and return its index (from 1):
    this->chip_shift = ffs(this->chipSize) - 1;

    printk(KERN_INFO "page_shift=%d, bbt_erase_shift=%d, chip_shift=%d, phys_erase_shift=%d\n",
        this->page_shift, this->bbt_erase_shift , this->chip_shift, this->phys_erase_shift);

    /* Set the bad block position */
    this->badblockpos = mtd->writesize > 512 ? 
        NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;


    /* Allocate buffers, if neccecary */
    if (!this->oob_buf) {
        size_t len;

        printk (KERN_INFO "mtd->oobsize = %d,  this->phys_erase_shift = %d, this->page_shift = %d\n",
            mtd->oobsize, this->phys_erase_shift, this->page_shift   );

        len = mtd->oobsize << (this->phys_erase_shift - this->page_shift);
        this->oob_buf = kmalloc (len, GFP_KERNEL);
        if (!this->oob_buf) {
            printk (KERN_ERR "brcmnand_scan(): Cannot allocate oob_buf %d bytes\n", len);
            return -ENOMEM;
        }
        this->options |= NAND_OOBBUF_ALLOC;
    }

    if (!this->data_buf) {
        size_t len;
        len = mtd->writesize + mtd->oobsize;
        this->data_buf = kmalloc (len, GFP_KERNEL);
        if (!this->data_buf) {
            if (this->options & NAND_OOBBUF_ALLOC)
                kfree (this->oob_buf);
            printk (KERN_ERR "brcmnand_scan(): Cannot allocate data_buf %d bytes\n", len);
            return -ENOMEM;
        }
        this->options |= NAND_DATABUF_ALLOC;
    }
    
    if (this->CS[0] == 0)
    { //Init only once
        thisDriver.state = FL_READY;
        init_waitqueue_head(&thisDriver.wq);
        spin_lock_init(&thisDriver.driver_lock);
    }

    memcpy(&mtd->oobinfo, this->ecclayout, sizeof(struct nand_oobinfo));
    
    mtd->oobavail = 0;
    for (i = 0; this->ecclayout->oobfree[i][1] && i < 8; i++) {
        PRINTK("i=%d, oobfree.length=%d\n", i, this->ecclayout->oobfree[i][1]);
        mtd->oobavail += this->ecclayout->oobfree[i][1];
    }

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0

    switch(acc_control & BCHP_NAND_ACC_CONTROL_ECC_LEVEL_0_MASK)
    {
        case BCHP_NAND_ACC_CONTROL_0_BCH_4:
            //BCH 4 bits (7 bytes):
            this->eccmode = NAND_ECC_HW7_512;
            this->eccbytes = 7;
            this->eccsize = NAND_CONTROLLER_DATA_BOUNDARY;    
            break;
#ifdef BCHP_NAND_ACC_CONTROL_0_BCH_8
        case BCHP_NAND_ACC_CONTROL_0_BCH_8:
            //BCH 8 bits (13 bytes):
            this->eccmode = NAND_ECC_HW13_512;
            this->eccbytes = 13;
            this->eccsize = NAND_CONTROLLER_DATA_BOUNDARY;    
            break;
#endif
        case BCHP_NAND_ACC_CONTROL_0_HAMMING:
        default:
            // Hamming code 1 bit (3 bytes)
            this->eccmode = NAND_ECC_HW3_512;
            this->eccbytes = 3;
            this->eccsize = NAND_CONTROLLER_DATA_BOUNDARY;    
            break;
    }
#endif

    mtd->eccsize = NAND_CONTROLLER_DATA_BOUNDARY;  // Fixed for Broadcom controller
    this->eccOobSize = NAND_CONTROLLER_OOB_BOUNDARY;

    printk(KERN_DEBUG "mtd->eccsize=%d, eccbytes=%d\n", mtd->eccsize, this->eccbytes);

    /* For 2K page, eccsteps is 4 for the 4 slides that make up a page */
    this->eccsteps = mtd->writesize / this->eccsize;
    /* We consider only layout allocation performed in nand_base */
    this->layout_allocated = 0;
    if (!this->layout && this->ecclayout)
        if (fill_autooob_layout(mtd) < 0)
            BUG();

    /* Version ID */
    version_id = this->ctrl_read(BCHP_NAND_REVISION);
    version_major = (version_id & 0xff00) >> 8;
    version_minor = (version_id & 0xff);


    printk(KERN_INFO "Brcm NAND controller version (EDU Enable) = %x.%x NAND flash size %dMB @%08x\n", 
        version_major, version_minor, this->chipSize>>20, (uint32_t) this->pbase);

    printk(KERN_INFO "Block Scrubbing is Enable\n");

    is_this_a_7601A();

    /*
     * Initialize the this->pEccMask array for ease of verifying OOB area.
     */
    fill_ecccmp_mask(mtd);
    
    this->write_with_ecc = brcmnand_write_with_ecc;
    /* Fill in remaining MTD driver data */
    mtd->type = MTD_NANDFLASH;
    mtd->flags = NAND_IS_MLC(this) ? MTD_CAP_MLC_NANDFLASH : MTD_CAP_NANDFLASH;
    mtd->ecctype = MTD_NANDECC_AUTOPLACE;    /* ECC byte placement */
    mtd->erase = brcmnand_erase;
    mtd->point = NULL;
    mtd->unpoint = NULL;
    mtd->read = brcmnand_read;
    mtd->write = brcmnand_write;
    mtd->read_ecc = brcmnand_read_ecc;
    mtd->write_ecc = brcmnand_write_ecc;

    mtd->read_oob = tr_read_oob;
    mtd->write_oob = tr_write_oob;

    mtd->read_oobfree = brcmnand_read_oobfree;
    mtd->write_oobfree = brcmnand_write_oobfree;
    
    mtd->ecclayout = kmalloc(sizeof(struct nand_ecclayout), GFP_KERNEL);
    if(mtd->ecclayout != NULL)
    {
        // mtd->ecclayout->useecc = MTD_NANDECC_AUTOPLACE;
        memcpy(mtd->ecclayout->eccpos,mtd->oobinfo.eccpos,  sizeof(mtd->ecclayout->eccpos));
        memcpy(mtd->ecclayout->oobfree,mtd->oobinfo.oobfree, sizeof(mtd->ecclayout->oobfree));
        memcpy(mtd->ecclayout->bbindicator,mtd->oobinfo.bbindicator, sizeof(mtd->ecclayout->bbindicator));
        mtd->ecclayout->eccbytes = mtd->oobinfo.eccbytes;
        mtd->ecclayout->oobavail = mtd->oobavail;

    }
    else
    {
         printk (KERN_ERR "brcmnand_scan(): Cannot allocate ecclayout %d bytes\n", sizeof(struct nand_ecclayout));
         return -ENOMEM;
    }

    
    mtd->readv = NULL;
    mtd->readv_ecc = NULL;
    mtd->writev = brcmnand_writev;
    mtd->writev_ecc = brcmnand_writev_ecc;
    mtd->sync = brcmnand_sync;
    mtd->lock = NULL;
    mtd->unlock = brcmnand_unlock;
    mtd->suspend = brcmnand_suspend;
    mtd->resume = brcmnand_resume;
    mtd->block_isbad = brcmnand_block_isbad;
    mtd->block_markbad = brcmnand_block_markbad;
    mtd->owner = THIS_MODULE;
    mtd->reboot_notifier.notifier_call = brcmnand_reboot;

    /*
     * Clear ECC registers 
     */
    this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);
  
    this->ctrl_write(BCHP_NAND_ECC_CORR_EXT_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_EXT_ADDR, 0);
    
    brcmnand_pageapi_init(mtd, EDU_CONFIGURATION);
    edu_init(mtd, EDU_CONFIGURATION);

    if (this->xor_invert_val == 0x1FC00000)
    {
        printk(KERN_NOTICE "%s : This driver is performing physical to logical mapping [XOR with 0x1FC00000]\n", __FUNCTION__);
    }
    else
    {
        printk(KERN_NOTICE "%s : This driver does not perform physical to logical maping [XOR with 0x00000000]\n", __FUNCTION__);    
    }
    /* Unlock whole block */
PRINTK("Calling mtd->unlock(ofs=0, MTD Size=%08x\n", mtd->size);
    mtd->unlock(mtd, 0x0, mtd->size);
    
    register_reboot_notifier(&mtd->reboot_notifier);

    mtd->get_bbt_code = brcmnand_get_bbt_code;
    mtd->set_bbt_code = brcmnand_set_bbt_code;
    mtd->delete_bbt_table = brcmnand_delete_bbt;
    mtd->scan_bbt_table = brcmnand_default_bbt;
    mtd->gen_ecc_error = brcmnand_gen_ecc_error;
    mtd->get_statistics = brcmnand_get_statistics;
    mtd->enable_ecc = brcmnand_enable_ecc;
    mtd->disable_ecc = brcmnand_disable_ecc;
    
    this->bbt = NULL;
    err =  this->scan_bbt(mtd);
    
    this->stats.versionNumber = 0;

    memcpy(this->stats.nandDescription, str_nandinfo, strlen(str_nandinfo));

    this->stats.eccCorrectableErrorsNow = 0;
    this->stats.eccCorrectableErrorsBefore = 0;

    this->stats.eccUncorrectableErrorsNow = 0;
    this->stats.eccUncorrectableErrorsBefore = 0;

    this->stats.goodBlocksNow = 0;
    this->stats.goodBlocksBefore = 0;

    this->stats.badBlocksDueToWearNow = 0; 
    this->stats.badBlocksDueToWearBefore = 0;

    this->stats.factoryMarkedBadBlocksNow = 0;
    this->stats.factoryMarkedBadBlocksBefore = 0;

    this->stats.reservedBlocksNow = 0;
    this->stats.reservedBlocksBefore = 0;

    this->stats.unclassifiedCriticalErrorsNow = 0;
    this->stats.unclassifiedCriticalErrorsBefore = 0;

    this->stats.eccBytesErrors = 0;
    this->stats.blockRefreshed = 0;

    for(i=0; i < BRCMNAND_ECC_BCH_8;i++)
        g_numeccbits_corrected_cnt[i] = 0;
    
    g_total_ecc_bits_corrected_cnt = 0;
    g_prev_total_bits_corrected_cnt = 0;


    //Create proc entry only once (for device 0)
    if (this->CS[0] == 0)
    {
        brcmnand_createprocfile(mtd);
    }
    return err;
}


    /* 
     * Must set NAND back to Direct Access mode for reboot, but only if NAND is on CS0
     */

     
     
void brcmnand_prepare_reboot(void)
{

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0
    volatile uint32_t nand_select;
    volatile uint32_t csAddrXor;
    struct brcmnand_chip *this = NULL;
    struct mtd_info *mtd = NULL;
    
    static int passCnt = 0;
      
    /*
        ==> We need to do this only for the first call to this function.  
            Otherwise, the lock will already be held
    */
    if (passCnt == 0)   
    {
        /*
         * Prevent further access to the NAND flash, we are rebooting 
         */
        this = brcmnand_get_device_exclusive();

        if (this != NULL)
        {
            mtd = this->priv;
            
            printk(KERN_DEBUG"%s: this->CS[0] = %d\n", __FUNCTION__, this->CS[0]);
        
            if (this->CS[0] == 0) 
            { // Only if on CS0
                nand_select = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);
        
                //Set Direct Access bit:
                nand_select |= BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK;
                
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0                
                //Use_nand bit:
                nand_select &= ~(BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_1_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_2_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_3_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_4_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_5_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_6_USES_NAND_MASK | 
                                 BCHP_NAND_CS_NAND_SELECT_EBI_CS_7_USES_NAND_MASK );
                                 
                nand_select |= BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_USES_NAND_MASK;
            
                printk(KERN_DEBUG"%s: Writing 0x%08X in BCHP_NAND_CS_NAND_SELECT\n", __FUNCTION__, nand_select);
                brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, nand_select);

                //Set this CS to use XOR:
                csAddrXor = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_XOR);
        
                csAddrXor |= BCHP_NAND_CS_NAND_XOR_EBI_CS_0_ADDR_1FC0_XOR_MASK;
                printk(KERN_DEBUG"%s: Writing 0x%08X in BCHP_NAND_CS_NAND_XOR\n", __FUNCTION__, csAddrXor);

                brcmnand_ctrl_write(BCHP_NAND_CS_NAND_XOR, csAddrXor);
#endif   //CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0         
            }
            else
            {
                printk(KERN_ERR"%s: Something is wrong: this->CS[0] != 0!!!\n", __FUNCTION__);
                
                //Release device:
                brcmnand_release_device(mtd);
                passCnt = 0;
                return;
            }
        }
    }
    
    passCnt++;
#endif //CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_1_0

    return;
}

EXPORT_SYMBOL(brcmnand_prepare_reboot);

static int brcmnand_reboot( struct notifier_block *nb,
                unsigned long val,
                void *v)
{
    brcmnand_prepare_reboot();
    
    return NOTIFY_DONE;
}



#if defined( CONFIG_MIPS_BCM7401C0 ) || defined( CONFIG_MIPS_BCM7118A0 )  || defined( CONFIG_MIPS_BCM7403A0 ) 
extern int bcm7401Cx_rev;
extern int bcm7118Ax_rev;
extern int bcm7403Ax_rev; 
static DEFINE_SPINLOCK(bcm9XXXX_lock);
static unsigned long misb_war_flags;

static inline void
HANDLE_MISB_WAR_BEGIN(void)
{
    /* if it is 7401C0, then we need this workaround */
    if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 
                                 || bcm7403Ax_rev == 0x20)
    {    
        spin_lock_irqsave(&bcm9XXXX_lock, misb_war_flags);
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
        *(volatile unsigned long*)0xb0400b1c = 0xFFFF;
    }
}

static inline void
HANDLE_MISB_WAR_END(void)
{
    if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 
                                 || bcm7403Ax_rev == 0x20)
    {    
        spin_unlock_irqrestore(&bcm9XXXX_lock, misb_war_flags);
    }
}

#else
#define HANDLE_MISB_WAR_BEGIN()
#define HANDLE_MISB_WAR_END()
#endif


#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
/*
 * CS0 reset values are gone by now, since the bootloader disabled CS0 before booting Linux
 * in order to give the EBI address space to NAND.
 * We will need to read strap_ebi_rom_size in order to reconstruct the CS0 values
 * This will not be a problem, since in order to boot with NAND on CSn (n != 0), the board
 * must be strapped for NOR.
 */
static unsigned int 
get_rom_size(unsigned long* outp_cs0Base)
{
    volatile unsigned long strap_ebi_rom_size, sun_top_ctrl_strap_value;
    uint32_t romSize = 0;

    sun_top_ctrl_strap_value = *(volatile unsigned long*) (0xb0000000|BCHP_SUN_TOP_CTRL_STRAP_VALUE);
    strap_ebi_rom_size = sun_top_ctrl_strap_value & BCHP_SUN_TOP_CTRL_STRAP_VALUE_strap_ebi_rom_size_MASK;
    strap_ebi_rom_size >>= BCHP_SUN_TOP_CTRL_STRAP_VALUE_strap_ebi_rom_size_SHIFT;

    // Here we expect these values to remain the same across platforms.
    // Some customers want to have a 2MB NOR flash, but I don't see how that is possible.
    switch(strap_ebi_rom_size) {
    case 0:
        romSize = 64<<20;
        *outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_64MB;
        break;
    case 1:
        romSize = 16<<20;
        *outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_16MB;
        break;
    case 2:
        romSize = 8<<20;
        *outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_8MB;
        break;
    case 3:
        romSize = 4<<20;
        *outp_cs0Base = (0x20000000 - romSize) | BCHP_EBI_CS_BASE_0_size_SIZE_4MB;
        break;
    default:
        printk(KERN_ERR "%s: Impossible Strap Value %08lx for BCHP_SUN_TOP_CTRL_STRAP_VALUE\n", 
            __FUNCTION__, sun_top_ctrl_strap_value);
        BUG();
    }


    return romSize;
}

/*
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
int brcmnand_readNorFlash(struct mtd_info *mtd, void* buff, unsigned int offset, int len)
{
    struct brcmnand_chip* this = (struct brcmnand_chip*) mtd->priv;
    int ret = -EFAULT;
    int i; 
    int csNand; // Which CS is NAND
    unsigned long cs0Base = 0;
    volatile unsigned long cs0Cnfg, cs0BaseAddr, csNandSelect;
    volatile unsigned long csNandBase[MAX_NAND_CS], csNandCnfg[MAX_NAND_CS];
    unsigned int romSize;
    volatile uint16_t* pui16 = (volatile uint16_t*) buff;
    volatile uint16_t* fp;

    if (!this) { // When booting from CRAMFS/SQUASHFS using romblock dev
        this = brcmnand_get_device_exclusive();
        mtd = (struct mtd_info*) this->priv;
    }
    else if (brcmnand_get_device(mtd, BRCMNAND_FL_EXCLUSIVE))
        return ret;

    romSize = get_rom_size(&cs0Base);
    
    cs0BaseAddr = cs0Base & BCHP_EBI_CS_BASE_0_base_addr_MASK;

    if ((len + offset) > romSize) {
        printk(KERN_ERR "%s; Attempt to read past end of CS0, (len+offset)=%08x, romSize=%dMB\n",
            __FUNCTION__, len + offset, romSize>>20);
        ret = (-EINVAL);
        goto release_device_and_out;
    }

    cs0Cnfg = *(volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_CONFIG_0);

    // Turn off NAND CS
    for (i=0; i < this->numchips; i++) {
        csNand = this->CS[i];

        if (csNand == 0) {
            printk(KERN_INFO "%s: Call this routine only if NAND is not on CS0\n", __FUNCTION__);
            ret = (-EINVAL);
            goto release_device_and_out;
        }

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
        BUG_ON(csNand > 5);
#else
        BUG_ON(csNand > 8);
#endif
        csNandBase[i] = *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_BASE_0 + 8*csNand);
        csNandCnfg[i] = *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_CONFIG_0 + 8*csNand);

        // Turn off NAND, must turn off both NAND_CS_NAND_SELECT and CONFIG.
        // We turn off the CS_CONFIG here, and will turn off NAND_CS_NAND_SELECT for all CS at once,
        // outside the loop.
        *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_CONFIG_0 + 8*csNand) = 
            csNandCnfg[i] & (~BCHP_EBI_CS_CONFIG_0_enable_MASK);

    }
    
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_0_1
    csNandSelect = brcmnand_ctrl_read(BCHP_NAND_CS_NAND_SELECT);

    brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, csNandSelect & 
        ~(
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
            BCHP_NAND_CS_NAND_SELECT_EBI_CS_5_SEL_MASK
            | BCHP_NAND_CS_NAND_SELECT_EBI_CS_4_SEL_MASK
            | BCHP_NAND_CS_NAND_SELECT_EBI_CS_3_SEL_MASK
            | BCHP_NAND_CS_NAND_SELECT_EBI_CS_2_SEL_MASK
            | BCHP_NAND_CS_NAND_SELECT_EBI_CS_1_SEL_MASK
            | BCHP_NAND_CS_NAND_SELECT_EBI_CS_0_SEL_MASK
#else
            0x0000003E    /* Not documented on V1.0+ */
#endif
        ));
#endif

    // Turn on NOR on CS0
    *(volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_CONFIG_0) = 
        cs0Cnfg | BCHP_EBI_CS_CONFIG_0_enable_MASK;

    // Take care of MISB Bridge bug on 7401c0/7403a0/7118a0
    HANDLE_MISB_WAR_BEGIN();

    // Read NOR, 16 bits at a time
    fp = (volatile uint16_t*) (KSEG1ADDR(cs0BaseAddr + offset));
    for (i=0; i < (len>>1); i++) {
        pui16[i] = fp[i];
    }

    HANDLE_MISB_WAR_END();

    // Turn Off NOR
    *(volatile unsigned long*) (0xb0000000|BCHP_EBI_CS_CONFIG_0) = 
        cs0Cnfg & (~BCHP_EBI_CS_CONFIG_0_enable_MASK);

    // Turn NAND back on
    for (i=0; i < this->numchips; i++) {
        csNand = this->CS[i];
        if (csNand == 0) {
            printk(KERN_INFO "%s: Call this routine only if NAND is not on CS0\n", __FUNCTION__);
            ret = (-EINVAL);
            goto release_device_and_out;
        }
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_1_0
        BUG_ON(csNand > 5);
#else
        BUG_ON(csNand > 8);
#endif
        *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_BASE_0 + 8*csNand) = csNandBase[i] ;
        *(volatile unsigned long*) (0xb0000000 + BCHP_EBI_CS_CONFIG_0 + 8*csNand) = csNandCnfg[i];
    }

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_0_1
    // Restore NAND_CS_SELECT
    brcmnand_ctrl_write(BCHP_NAND_CS_NAND_SELECT, csNandSelect);
#endif
    udelay(10000); // Wait for ID Configuration to stabilize
    
release_device_and_out:
    brcmnand_release_device(mtd);


    return ret;
}
EXPORT_SYMBOL(brcmnand_readNorFlash);

#endif //#if CONFIG_MTD_BRCMNAND_VERSION <= CONFIG_MTD_BRCMNAND_VERS_1_0


/**
 * brcmnand_release - [BrcmNAND Interface] Free resources held by the BrcmNAND device
 * @param mtd        MTD device structure
 */
void brcmnand_release(struct mtd_info *mtd)
{
    struct brcmnand_chip *this = mtd->priv;

    printk(KERN_DEBUG"In %s\n", __FUNCTION__);
    brcmnand_prepare_reboot();
    unregister_reboot_notifier(&mtd->reboot_notifier);

#ifdef CONFIG_MTD_PARTITIONS
    /* Deregister partitions */
    del_mtd_partitions (mtd);
#endif
    /* Deregister the device */
    del_mtd_device (mtd);

    /* Buffer allocated by brcmnand_scan */
    if ((this->options & NAND_DATABUF_ALLOC)&&(this->data_buf))
    {
        kfree(this->data_buf);
        this->data_buf = NULL;
    }

    /* Buffer allocated by brcmnand_scan */
    if ((this->options & NAND_OOBBUF_ALLOC)&&(this->oob_buf))
    {
        kfree(this->oob_buf);
        this->oob_buf = NULL;
    }

    kfree(this->pLocalPage);
    this->pLocalPage = NULL;


    kfree(this->pLocalOob);
    this->pLocalOob = NULL;

    kfree(this->pEccMask);
    this->pEccMask  = NULL;
}

static int brcmnand_allocate_block(struct mtd_info* mtd)
{
    int i;
    struct brcmnand_chip* this = mtd->priv;
    unsigned int nPages = this->blockSize / this->pageSize;
    
    if ((ppRefreshData == NULL) && (ppRefreshOob == NULL))
    {
        ppRefreshData= kmalloc(nPages * sizeof(uint8_t*), GFP_KERNEL);
        ppRefreshOob = kmalloc(nPages * sizeof(uint8_t*), GFP_KERNEL);

        for (i = 0; i < nPages; i++)
        { //to be on the safe side:
            ppRefreshData[i] = NULL;
            ppRefreshOob[i] = NULL;
        }
        
        for (i = 0; i < nPages; i++)
        {
            ppRefreshData[i] = kmalloc(this->pageSize, GFP_KERNEL);
            
            if (ppRefreshData[i] == NULL)
            {
                printk (KERN_ERR "%s: Out of memory for allocating ppRefreshData[%d]\n", 
                    __FUNCTION__, i);
    
                brcmnand_free_block(mtd);
                return -1;    //error
            }
            memset( ppRefreshData[i], 0, this->pageSize);
            
            ppRefreshOob[i] = kmalloc(mtd->oobsize, GFP_KERNEL);
            
            if (ppRefreshOob[i] == NULL)
            {
                printk (KERN_ERR "%s: Out of memory for allocating ppRefreshOob[%d]\n", 
                    __FUNCTION__, i);
    
                brcmnand_free_block(mtd);
                return -1;    //error
            }
            memset (ppRefreshOob[i],  0, mtd->oobsize);
        }
        return 0; //success
    }
    else
    {//This should not happen:
        printk(KERN_ERR"%s: ppRefreshData or ppRefreshOob alreay allcated!\n",
                __FUNCTION__);
        return -1; //error                
    }        
}

static void brcmnand_free_block(struct mtd_info* mtd)
{
    int j;
    struct brcmnand_chip* this = mtd->priv;
    unsigned int nPages = this->blockSize / this->pageSize;
    
    for (j = 0; j < nPages; j++)
    {
        if (ppRefreshData)
        {
            if(ppRefreshData[j] != NULL) kfree(ppRefreshData[j]);
        }
        if(ppRefreshOob)
        {
            if(ppRefreshOob[j] != NULL) kfree(ppRefreshOob[j]);
        }
    }
    if (ppRefreshData)
        kfree(ppRefreshData);
    if (ppRefreshOob)
        kfree(ppRefreshOob);
    
    ppRefreshData = NULL;
    ppRefreshOob = NULL;
}

static int brcmnand_read_block(struct mtd_info* mtd, L_OFF_T blockAddr)
{
    int i;
    int ret = 0, retFinal = 0;
    struct brcmnand_chip* this = mtd->priv;    
    unsigned int nPages = this->blockSize / this->pageSize;
    L_OFF_T pageAddr = blockAddr;
    
    int retdatalen, retooblen;
    struct nand_oobinfo oobsel;
    /* Setting oobsel.useecc to MTD_NANDECC_PLACE will permit
        brcmnand_read_ecc() to return the raw oob:
    */
    oobsel.useecc = MTD_NANDECC_PLACE; 

    
    if ((ppRefreshData != NULL) && (ppRefreshOob != NULL))
    {
        for (i = 0; i < nPages; i++)
        {
            ret = brcmnand_read_page(mtd, pageAddr, ppRefreshData[i], this->pageSize, 
                                    ppRefreshOob[i],&oobsel, &retdatalen, &retooblen,
                                    false);
            if(ret != 0)
            {
                retFinal = ret;
            }                                    
            pageAddr = __ll_add32(pageAddr, this->pageSize);
        }
        return retFinal;
    }
    return -1;
}

static int brcmnand_write_block(struct mtd_info* mtd, L_OFF_T blockAddr)
{
    int i,j;
    int ret, retFinal = 0;
    int allFF;
    uint32_t* p32;
    struct brcmnand_chip* this = mtd->priv;
    unsigned int nPages = this->blockSize / this->pageSize;
    L_OFF_T pageAddr = blockAddr;
    
    if ((ppRefreshData != NULL) && (ppRefreshOob != NULL))
    {
        for (i = 0; i < nPages; i++)
        {
            //Check if ppRefreshData is all 0xff... If it is the case that block was all FF... Then do not prepare oobbuf
            // Assume first that we have all FF
                
            allFF = 1;
                
            for (j = 0; j < (this->pageSize / 4); j++)
            {
                p32 = (uint32_t*)ppRefreshData[i];
                  
                if ( *(p32 + j) != 0xFFFFFFFF )
                {
                    allFF = 0;
                    break;
                     
                }
            } 
            if (allFF == 1)
            {
             //Check if ppRefreshOob free is all 0xff... If it is the case that block was all FF... Then do not prepare oobbuf
             // Assume first that we have all FF
                for (j = 0; j < mtd->oobsize; j++)
                {
                    if (ppRefreshOob[i][j] != 0xFF)
                    {
                        allFF = 0;
                        break;
                    }
                }
            }                          
            if (allFF == 0)
            {
//            printk("Writing page %d\n",i);

                ret = brcmnand_write_page (mtd, pageAddr, this->pageSize,
                                ppRefreshData[i], ppRefreshOob[i]);
                if(ret != 0)
                {
                    retFinal = ret;
                }
            }    
            
            pageAddr = __ll_add32(pageAddr, this->pageSize);
        }
        return retFinal; 
    }
    return -1; //error
}

static int brcmnand_save_block(struct mtd_info* mtd, loff_t ofs)
{
    int nPageNumber,ret;
    L_OFF_T blockAddr;
    struct brcmnand_chip* this = mtd->priv;
    
    nPageNumber = brcmnand_deduce_blockOffset(mtd, ofs, &blockAddr);
    printk(KERN_DEBUG"%s: Saving block at addr %s\n", __FUNCTION__, 
                __ll_sprintf(brcmNandMsg, blockAddr, this->xor_invert_val));

    ret = brcmnand_allocate_block(mtd);
    if (ret == 0)
    {
        ret = brcmnand_read_block(mtd, blockAddr);
    }
    else
    {
        goto closeall;
    }
    
    return ret;
    
closeall:
    brcmnand_free_block(mtd);
    return ret;    
}

static int brcmnand_restore_block(struct mtd_info* mtd, loff_t ofs)
{
    int nPageNumber,ret;
    L_OFF_T blockAddr;
    struct erase_info instr;
    struct brcmnand_chip* this = mtd->priv;
        
    instr.mtd = mtd;
    instr.len = (uint32_t)(1 << this->erase_shift);
    instr.addr = (uint32_t)ofs;
    instr.callback = 0;
    
    nPageNumber = brcmnand_deduce_blockOffset(mtd, ofs, &blockAddr);
    printk(KERN_DEBUG"%s: Restoring block at addr %s\n", __FUNCTION__, 
                __ll_sprintf(brcmNandMsg, blockAddr, this->xor_invert_val));

    ret = brcmnand_erase_blocks(mtd, &instr, 0); // not allowed to access the bbt area
    if(ret != 0)
    {
       printk(KERN_DEBUG"Unable to erase the block bailing out...\n");
       goto closeall;
    }
    else
    {
        ret = brcmnand_write_block(mtd, blockAddr);
    }

closeall:
    brcmnand_free_block(mtd);
    return ret;    
}   

static int brcmnand_disable_ecc(struct mtd_info* mtd)
{
    uint32_t acc, acc0;
    struct brcmnand_chip* this = mtd->priv;
    
    acc = brcmnand_ctrl_read(this->nandAccControlReg);

    acc0 = acc & 
               ~(BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK | 
                 BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK |
                 BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK );

    brcmnand_ctrl_write(this->nandAccControlReg, acc0);
    
    return 0;
}

static int brcmnand_enable_ecc(struct mtd_info* mtd)
{
    uint32_t acc, acc0;
    struct brcmnand_chip* this = mtd->priv;
    
    acc = brcmnand_ctrl_read(this->nandAccControlReg);

    acc0 = acc | 
                (BCHP_NAND_ACC_CONTROL_RD_ECC_EN_MASK | 
                 BCHP_NAND_ACC_CONTROL_RD_ECC_BLK0_EN_MASK |
                 BCHP_NAND_ACC_CONTROL_WR_ECC_EN_MASK );


    brcmnand_ctrl_write(this->nandAccControlReg, acc0);
    
    return 0;
}



static uint32_t cnt_zeros(uint8_t* data, int dataSize)
{
    int i,j;
    uint32_t nbBits = 0;
    uint32_t* p = (uint32_t*)data;
    
    for (i = 0; i < dataSize; i+=4)
    {
        for (j = 0; j < 32; j++)
        {
            if((*p & (0x01 << j))== 0)
                nbBits++;
        }
        p++;    //move to next 32-bits
    }
    return nbBits;
}
