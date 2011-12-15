/*
 *  include/mtd/brcmnand_oob.h
 *
    Copyright (c) 2005-2007 Broadcom Corporation                 
    
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

    File: brcmnand_oob.c

    Description: 
    Export OOB structure to user space.

when	who what
-----	---	----
051011	tht	Moved OOB format here from brcmnand_base.c in order to share it with User space
 */

#ifndef __BRCMNAND_OOB_H
#define __BRCMNAND_OOB_H


/*
 * Assuming proper include that precede this has the typedefs for struct nand_oobinfo
 */

/**
 * brcmnand_oob oob info for 2K page
 */

static struct nand_oobinfo brcmnand_oob_64 = {
    .useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 12,
	.eccpos		= {
		6,7,8,
		22,23,24,
		38,39,40,
		54,55,56
		},
	.oobfree	= { /* 0-1 used for BBT and/or manufacturer bad block marker, 
	                    * first slice loses 2 bytes for BBT */
				{2, 4}, 
				{9,13}, 		/* First slice {9,7} 2nd slice {16,6}are combined */ 
									/* ST uses 6th byte (offset=5) as Bad Block Indicator, 
									  * in addition to the 1st byte, and will be adjusted at run time */
				{25, 13},				/* 2nd slice  */
				{41, 13},				/* 3rd slice */
				{57, 7},				/* 4th slice */
	            {0, 0}				/* End marker */
			},
    .bbindicator = {
                {0, 2},
                {0, 0}			/* End marker */
                }
};


/**
 * brcmnand_oob oob info for 512 page
 */
static struct nand_oobinfo brcmnand_oob_16 = {
    .useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 3,
	.eccpos		= {
		6,7,8
		},
	.oobfree	= { {0, 5}, 
				{9,7}, /* Byte 5 (6th byte) used for BI */
				{0, 0}		/* End marker */
			   },
			/* THT Bytes offset 4&5 are used by BBT.  Actually only byte 5 is used, but in order to accomodate
			 * for 16 bit bus width, byte 4 is also not used.  If we only use byte-width chip, (We did)
			 * then we can also use byte 4 as free bytes.
			 */
    .bbindicator = {
                {5, 1},
                {0, 0}			/* End marker */
                }

};



/*
 * 4K page MLC with BCH-4 ECC, uses 7 ECC bytes per 512B ECC step
 */
static struct nand_oobinfo brcmnand_oob_bch4_4k = {
    .useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 7*8,  /* 7*8 = 56 bytes */
	.eccpos		= { 
		9,10,11,12,13,14,15,
		25,26,27,28,29,30,31,
		41,42,43,44,45,46,47,
		57,58,59,60,61,62,63,
		73,74,75,76,77,78,79,
		89,90,91,92,93,94,95,
		105,106,107,108,109,110,111,
		121,122,123,124,125,126,127
		},
	.oobfree	= { /* 0  used for BBT and/or manufacturer bad block marker, 
	                    * first slice loses 1 byte for BBT */
				{1, 8}, 		/* 1st slice loses byte 0 */
				{16,9}, 		/* 2nd slice  */
				{32, 9},		/* 3rd slice  */
				{48, 9},		/* 4th slice */
				{64, 9},		/* 5th slice */
				{80, 9},		/* 6th slice */
				{96, 9},		/* 7th slice */
				{112, 9},		/* 8th slice */
	            		{0, 0}			/* End marker */
			},
    .bbindicator = {
                {0, 1},
                {0, 0}			/* End marker */
                }
};

/*
 * 2K page MLC with BCH-4 ECC, uses 7 ECC bytes per 512B ECC step
 */
static struct nand_oobinfo brcmnand_oob_bch4_2k = {
    .useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 7*4,  /* 7*4 = 28 bytes */
	.eccpos		= { 
		9,10,11,12,13,14,15,
		25,26,27,28,29,30,31,
		41,42,43,44,45,46,47,
		57,58,59,60,61,62,63
		},
	.oobfree	= { /* 0  used for BBT and/or manufacturer bad block marker, 
	                    * first slice loses 1 byte for BBT */
				{1, 8}, 		/* 1st slice loses byte 0 */
				{16,9}, 		/* 2nd slice  */
				{32, 9},		/* 3rd slice  */
				{48, 9},		/* 4th slice */
	            		{0, 0}			/* End marker */
			},
    .bbindicator = {
                {0, 1},
                {0, 0}			/* End marker */
                }
};


#if MTD_MAX_ECCPOS_ENTRIES >= 104 
/****************BCH8****************************************/

/*
 * 4K page MLC with BCH-8 ECC, uses 13 ECC bytes per 512B ECC step
 */
static struct nand_oobinfo brcmnand_oob_bch8_4k = {
    .useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 7*8,  /* 7*8 = 56 bytes */
	.eccpos		= { 
		3,4,5,6,7,8,9,10,11,12,13,14,15,
		19,20,21,22,23,24,25,26,27,28,29,30,31,
		35,36,37,38,39,40,41,42,43,44,45,46,47,
		51,52,53,54,55,56,57,58,59,60,61,62,63,
		67,68,69,70,71,72,73,74,75,76,77,78,79,
		83,84,85,86,87,88,89,90,91,92,93,94,95,
		99,100,101,102,103,104,105,106,107,108,109,110,111,
		115,116,117,118,119,120,121,122,123,124,125,126,127
		},
	.oobfree	= { /* 0  used for BBT and/or manufacturer bad block marker, 
	                    * first slice loses 1 byte for BBT */
				{1, 2}, 		/* 1st slice loses byte 0 */
				{16,3}, 		/* 2nd slice  */
				{32, 3},		/* 3rd slice  */
				{48, 3},		/* 4th slice */
				{64, 3},		/* 5th slice */
				{80, 3},		/* 6th slice */
				{96, 3},		/* 7th slice */
				{112, 3},		/* 8th slice */
	            {0, 0}			/* End marker */
			},
    .bbindicator = {
                {0, 1},
                {0, 0}			/* End marker */
                }
};

/*
 * 2K page SLC/MLC with BCH-8 ECC, uses 7 ECC bytes per 512B ECC step
 */
static struct nand_oobinfo brcmnand_oob_bch8_2k = {
    .useecc		= MTD_NANDECC_AUTOPLACE,
	.eccbytes	= 7*4,  /* 7*4 = 28 bytes */
	.eccpos		= { 
		3,4,5,6,7,8,9,10,11,12,13,14,15,
		19,20,21,22,23,24,25,26,27,28,29,30,31,
		35,36,37,38,39,40,41,42,43,44,45,46,47,
		51,52,53,54,55,56,57,58,59,60,61,62,63
		},
	.oobfree	= { /* 0  used for BBT and/or manufacturer bad block marker, 
	                    * first slice loses 1 byte for BBT */
				{1, 2}, 		/* 1st slice loses byte 0 */
				{16,3}, 		/* 2nd slice  */
				{32, 3},		/* 3rd slice  */
				{48, 3},		/* 4th slice */
	            		{0, 0}			/* End marker */
			},
    .bbindicator = {
                {0, 1},
                {0, 0}			/* End marker */
                }
};

#else

#endif

/***************END BCH8************************************/


#endif
