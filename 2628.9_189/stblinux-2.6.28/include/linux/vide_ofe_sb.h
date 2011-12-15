/*
 *  vide_ofe_sb.h - Header file for Broadcom VIDE OFE driver
 *                  Side Band Protocol
 *
 *  Copyright 2009 Broadcom Corp.  All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __VIDE_OFE_SB_H__
#define __VIDE_OFE_SB_H__

/*
 * The following define controls the reload model.
 * If defined, OFE FW is running SB Protocol from
 * TCM and therefore a call to SB_IOCTL_RELOAD
 * is not necessary. Otherwise, OFE FW reaload
 * and the ARM restart will be done under
 * SB_IOCTL_STATUS.
 */
//#define SB_FROM_TCM		1


/* "Are You Alive" State Codes */
enum {
	OFE_SB_INIT_STATE = 0,
	OFE_SB_READY,
	OFE_SB_BUSY,
	OFE_SB_IN_PROGRESS,
	OFE_SB_KERNEL_ALIVE,
};

/*
 * SB Error Codes
 */
#define SB_ERR_INVALID_COMMAND		0x80
#define SB_ERR_INVALID_OFFSET		0x81
#define SB_ERR_INVALID_SIZE		0x82
#define SB_ERR_INVALID_CHECKSUM		0x83
#define SB_ERR_FLASH_ERASE_FAIL		0x84
#define SB_ERR_FLASH_EXHAUSTED		0x85
#define SB_ERR_DATA_ERROR		0x86
#define SB_ERR_VAULT_FAILURE		0x87
#define SB_ERR_FLASH_READ_FAIL		0x88
#define SB_ERR_FLASH_BLANK		0x89	


/*
 * SB Protocol IOCTL Section
 */
typedef enum _tag_SB_Opcodes {
	SB_FLASH_ACCESS = 0xf1,
	SB_FE_RESET = 0x04,
} SB_Opcodes;

typedef enum _tag_SB_Flash_Commands {
	SB_READ = 1,
	SB_WRITE,
	SB_REBOOT,
} SB_Flash_Commands;

typedef enum _tag_SB_Flash_Index {
	SB_OFEFW_PART = 0,
	SB_OFEWS_PART,
} SB_Flash_Index;

/*
 * Structure to transmit Side Band command parameters
 * from the FE to the user-level code.
 */
typedef struct sb_command {
	SB_Flash_Commands cmd;		/* Read, Write, Reboot */
	SB_Flash_Index  idx;		/* ofefw, ofews */
	unsigned int flash_offset;	/* MBZ for Write, 256-byte aligned for Read */
	unsigned int data_size;		/* Write: 1M for ofefw, 128K for ofews. 256-multiple for read */
	unsigned int sdram_physaddr;	/* phys addr of buffer of size data_size */
	unsigned int checksum;		/* Data checksum for Write operations */
#if defined (SB_FROM_TCM)
	unsigned int reload_physaddr;	/* Physical address to reload OFE FW image to after flash update */
#endif
} sb_command_t;

typedef enum _tag_SB_Response_Code {
	SB_SUCCESS = 0x11,
	SB_FAIL = 0xdd,
} SB_Response_Code;

/*
 * Structure to transmit Side Band command response
 * from the user-level code to the FE.
 */
typedef struct sb_response {
	SB_Flash_Commands cmd;		/* Read, Write, Reboot */
	SB_Flash_Index  idx;		/* ofefw, ofews */
	SB_Response_Code status;	/* Success = 0x11, Fail = 0xdd */
	unsigned int errcode;		/* Valid if status = 0xdd */
	unsigned int checksum;		/* Calculated checksum of Read data */
} sb_response_t;

/*
 * Structure to transmit the SDRAM load address
 * to user-level code for the FE Reload/Reboot
 * stage.
 */
typedef struct fe_reboot {
	SB_Flash_Commands cmd;		/* Reboot */
	unsigned int physaddr;		/* Physical address to reload OFE FW image to after flash update */
} fe_reboot_t;


#define SB_IOC_MAGIC	0xed
#define SB_IOC_MAXNR	2

#define SB_IOCTL_GETCMD		_IOW(SB_IOC_MAGIC, 0, sb_command_t *)
#define SB_IOCTL_STATUS		_IOR(SB_IOC_MAGIC, 1, sb_response_t *)
#define SB_IOCTL_RELOAD		_IOW(SB_IOC_MAGIC, 2, fe_reboot_t *)

#endif /* __VIDE_OFE_SB_H__ */
