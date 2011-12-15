/*
 * $Id: mtd-abi.h,v 1.13 2005/11/07 11:14:56 gleixner Exp $
 *
 * Portions of MTD ABI definition which are shared by kernel and user space
 */

#ifndef __MTD_ABI_H__
#define __MTD_ABI_H__

#define MTD_ABSENT		0
#define MTD_RAM			1
#define MTD_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_DATAFLASH		6
#define MTD_UBIVOLUME		7

#define MTD_ECC			128	// Device capable of automatic ECC

#define MTD_WRITEABLE		0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		0x1000	/* No erase necessary */
#define MTD_POWERUP_LOCK	0x2000	/* Always locked after reset */
#define MTD_OOB_WRITEABLE	0x4000	/* Use Out-Of-Band area */


// Some common devices / combinations of capabilities
#define MTD_CAP_ROM		0
#define MTD_CAP_RAM		(MTD_WRITEABLE | MTD_BIT_WRITEABLE | MTD_NO_ERASE)
#define MTD_CAP_NORFLASH	(MTD_WRITEABLE | MTD_BIT_WRITEABLE)
#define MTD_CAP_NANDFLASH	(MTD_WRITEABLE | MTD_OOB_WRITEABLE)
#define MTD_CAP_MLC_NANDFLASH (MTD_WRITEABLE)

// High level MLC test, compared to low level defined in brcmnand.h
#define MTD_IS_MLC(mtd) ((((mtd)->flags & MTD_CAP_MLC_NANDFLASH) == MTD_CAP_MLC_NANDFLASH) &&\
			(((mtd)->flags & MTD_OOB_WRITEABLE) != MTD_OOB_WRITEABLE))


// Types of automatic ECC/Checksum available
#define MTD_ECC_NONE		0 	// No automatic ECC available
#define MTD_ECC_RS_DiskOnChip	1	// Automatic ECC on DiskOnChip
#define MTD_ECC_SW		2	// SW ECC for Toshiba & Samsung devices

/* ECC byte placement */
#define MTD_NANDECC_OFF		0	// Switch off ECC (Not recommended)
#define MTD_NANDECC_PLACE	1	// Use the given placement in the structure (YAFFS1 legacy mode)
#define MTD_NANDECC_AUTOPLACE	2	// Use the default placement scheme
#define MTD_NANDECC_PLACEONLY	3	// Use the given placement in the structure (Do not store ecc result on read)
#define MTD_NANDECC_AUTOPL_USR 	4	// Use the given autoplacement scheme rather than using the default

/* OTP mode selection */
#define MTD_OTP_OFF		0
#define MTD_OTP_FACTORY		1
#define MTD_OTP_USER		2

struct erase_info_user {
	uint32_t start;
	uint32_t length;
};

struct mtd_oob_buf {
	uint32_t start;
	uint32_t length;
	unsigned char __user *ptr;
};


struct mtd_info_user {
	uint8_t type;
	uint32_t flags;
	uint32_t size;	 // Total size of the MTD
	uint32_t erasesize;
	uint32_t writesize;
	uint32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	uint32_t ecctype;
	uint32_t eccsize;
};

struct region_info_user {
	uint32_t offset;		/* At which this region starts,
					 * from the beginning of the MTD */
	uint32_t erasesize;		/* For this region */
	uint32_t numblocks;		/* Number of blocks in this region */
	uint32_t regionindex;
};

struct otp_info {
	uint32_t start;
	uint32_t length;
	uint32_t locked;
};
typedef struct mtd_info_user mtd_info_t;
typedef struct erase_info_user erase_info_t;
typedef struct region_info_user region_info_t;
typedef struct nand_oobinfo nand_oobinfo_t;
typedef struct nand_ecclayout nand_ecclayout_t;

#define MEMGETINFO		_IOR('M', 1, struct mtd_info_user)
#define MEMERASE		_IOW('M', 2, struct erase_info_user)
#define MEMWRITEOOB		_IOWR('M', 3, struct mtd_oob_buf)
#define MEMREADOOB		_IOWR('M', 4, struct mtd_oob_buf)
#define MEMLOCK			_IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK		_IOW('M', 6, struct erase_info_user)
#define MEMGETREGIONCOUNT	_IOR('M', 7, int)
#define MEMGETREGIONINFO	_IOWR('M', 8, struct region_info_user)
#define MEMSETOOBSEL		_IOW('M', 9, struct nand_oobinfo)
#define MEMGETOOBSEL		_IOR('M', 10, struct nand_oobinfo)
#define MEMGETBADBLOCK		_IOW('M', 11, loff_t)
#define MEMSETBADBLOCK		_IOW('M', 12, loff_t)
#define OTPSELECT		_IOR('M', 13, int)
#define OTPGETREGIONCOUNT	_IOW('M', 14, int)
#define OTPGETREGIONINFO	_IOW('M', 15, struct otp_info)
#define OTPLOCK			_IOR('M', 16, struct otp_info)
/* 2.6.12 ioctls 
#define MEMGETOOBAVAIL		_IOR('M', 17, uint32_t)
#define MEMWRITEOOBFREE         _IOWR('M', 18, struct mtd_oob_buf)
#define MEMREADOOBFREE          _IOWR('M', 19, struct mtd_oob_buf)
*/
/* 2.6.18.1 ioctls */
#define ECCGETLAYOUT		_IOR('M', 17, struct nand_ecclayout)
#define ECCGETSTATS		_IOR('M', 18, struct mtd_ecc_stats)
#define MTDFILEMODE		_IO('M', 19)

#define MEMGETOOBAVAIL		_IOR('M', 20, uint32_t)
#define MEMWRITEOOBFREE         _IOWR('M', 21, struct mtd_oob_buf)
#define MEMREADOOBFREE          _IOWR('M', 22, struct mtd_oob_buf)



struct nand_bbt_command {
	loff_t	     offset;
	loff_t       absolute_offset;
	unsigned int code;
};
                                                
#define MAX_NUMBER_OF_TESTS_FOR_GENECCERROR		6
#define GENECCERROR_FAILED			           -1
#define GENECCERROR_SUCCESS				        0
#define GENECCERROR_UNKNOWN			           -2


struct nand_generate_ecc_error_command {
    unsigned int bits;
    loff_t	     offset;

    uint8_t bitmask;
    uint8_t oob_offset;
    uint8_t oob_bitmask;

    unsigned int partialPageX;
    unsigned int partialPageY;
    
    unsigned int distribute; 
    unsigned int ecc_error;
    unsigned int runAutomatedTest;
    unsigned int runSpecificLocationTest;
	unsigned int pattern;
	

	unsigned int total_number_of_tests;
	unsigned int test_result[MAX_NUMBER_OF_TESTS_FOR_GENECCERROR]; // Sucess:0 Failure: -1 All Other: unknown
};

struct nand_statistics {

	unsigned int versionNumber;
	
	unsigned char nandDescription[512];

	unsigned int eccCorrectableErrorsNow;
	unsigned int eccCorrectableErrorsBefore;

	unsigned int eccUncorrectableErrorsNow;
	unsigned int eccUncorrectableErrorsBefore;

 	unsigned int goodBlocksNow;
 	unsigned int goodBlocksBefore;

	unsigned int badBlocksDueToWearNow; 
	unsigned int badBlocksDueToWearBefore;

	unsigned int factoryMarkedBadBlocksNow;
	unsigned int factoryMarkedBadBlocksBefore;

	unsigned int reservedBlocksNow;
	unsigned int reservedBlocksBefore;

	unsigned int unclassifiedCriticalErrorsNow;
	unsigned int unclassifiedCriticalErrorsBefore;

	unsigned int eccBytesErrors;
	unsigned int blockRefreshed;
};


#define MTD_BRCMNAND_SCANBBT          _IOWR('M', 44, loff_t)
#define MTD_BRCMNAND_NANDSTAT         _IOWR('M', 45, struct nand_statistics)
#define MTD_BRCMNAND_GENECCERROR      _IOWR('M', 46, struct nand_generate_ecc_error_command)
#define MTD_BRCMNAND_DELETEBBT        _IOWR('M', 47, loff_t)
#define MTD_BRCMNAND_READBBTCODE      _IOWR('M', 48, loff_t)
#define MTD_BRCMNAND_WRITEBBTCODE     _IOWR('M', 49, struct nand_bbt_command)



#define MTD_BRCMNAND_READNORFLASH _IOWR('M', 50, struct brcmnand_readNorFlash)
struct brcmnand_readNorFlash {
	void* buff;
	unsigned int offset;
	int len;
	int retVal;
};


#define MTD_MAX_OOBFREE_ENTRIES	9
#define MTD_MAX_ECCPOS_ENTRIES	104	// for BCH8 support
#define MTD_MAX_BBINDICATOR_ENTRIES 3 

struct nand_oobinfo {
	uint32_t useecc;
	uint32_t eccbytes;
	uint32_t oobfree[MTD_MAX_OOBFREE_ENTRIES][2];
	uint32_t eccpos[MTD_MAX_ECCPOS_ENTRIES];
    uint32_t bbindicator[MTD_MAX_BBINDICATOR_ENTRIES][2];
};

struct nand_oobfree {
	uint32_t offset;
	uint32_t length;
};

//Where the bad block indicators are located:
struct nand_bbindicator {
	uint32_t offset;
	uint32_t length;
};

/*
 * ECC layout control structure. Exported to userspace for
 * diagnosis and to allow creation of raw images
 */
struct nand_ecclayout {
	uint32_t eccbytes;
	uint32_t eccpos[MTD_MAX_ECCPOS_ENTRIES];
	uint32_t oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
    struct nand_bbindicator bbindicator[MTD_MAX_BBINDICATOR_ENTRIES];
    
};

/**
 * struct mtd_ecc_stats - error correction stats
 *
 * @corrected:	number of corrected bits
 * @failed:	number of uncorrectable errors
 * @badblocks:	number of bad blocks in this partition
 * @bbtblocks:	number of blocks reserved for bad block tables
 */
struct mtd_ecc_stats {
	uint32_t corrected;
	uint32_t failed;
	uint32_t badblocks;
	uint32_t bbtblocks;
};

/*
 * Read/write file modes for access to MTD
 */
enum mtd_file_modes {
	MTD_MODE_NORMAL = MTD_OTP_OFF,
	MTD_MODE_OTP_FACTORY = MTD_OTP_FACTORY,
	MTD_MODE_OTP_USER = MTD_OTP_USER,
	MTD_MODE_RAW,
};

#endif /* __MTD_ABI_H__ */
