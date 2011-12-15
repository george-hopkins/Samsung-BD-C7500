/*  *********************************************************************
    *  Broadcom Common Firmware Environment (CFE)
    *  
    *  IOCB definitions				File: cfe_iocb.h
    *  
    *  This module describes CFE's IOCB structure, the main
    *  data structure used to communicate API requests with CFE.
    *  
    *  Author:  Mitch Lichtenberg (mpl@broadcom.com)
    *  
    *********************************************************************/ 
/*
 * Copyright (C) 2000-2005 Broadcom Corporation
 *
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
 *
 * [07/10/2007]
 *      mfi: corrected cfe_magic calculation and added cfe_min_rev_nand()
 ********************************************************************* */

#ifndef _CFE_XIOCB_H
#define _CFE_XIOCB_H

/*  *********************************************************************
    *  Constants
    ********************************************************************* */

#define CFE_CMD_FW_GETINFO	0
#define CFE_CMD_FW_RESTART	1
#define CFE_CMD_FW_BOOT		2
#define CFE_CMD_FW_CPUCTL	3
#define CFE_CMD_FW_GETTIME      4
#define CFE_CMD_FW_MEMENUM	5
#define CFE_CMD_FW_FLUSHCACHE	6

#define CFE_CMD_DEV_GETHANDLE	9
#define CFE_CMD_DEV_ENUM	10
#define CFE_CMD_DEV_OPEN	11
#define CFE_CMD_DEV_INPSTAT	12
#define CFE_CMD_DEV_READ	13
#define CFE_CMD_DEV_WRITE	14
#define CFE_CMD_DEV_IOCTL	15
#define CFE_CMD_DEV_CLOSE	16
#define CFE_CMD_DEV_GETINFO	17

#define CFE_CMD_ENV_ENUM	20
#define CFE_CMD_ENV_GET		22
#define CFE_CMD_ENV_SET		23
#define CFE_CMD_ENV_DEL		24
#define CFE_CMD_GET_BOARD_INFO  25


#define CFE_CMD_MAX		32

#define CFE_CMD_VENDOR_USE	0x8000	/* codes above this are for customer use */

#define CFE_MI_RESERVED	0		/* memory is reserved, do not use */
#define CFE_MI_AVAILABLE 1		/* memory is available */

#define CFE_FLG_WARMSTART     0x00000001
#define CFE_FLG_FULL_ARENA    0x00000001
#define CFE_FLG_ENV_PERMANENT 0x00000001

#define CFE_CPU_CMD_START 1
#define CFE_CPU_CMD_STOP 0

#define CFE_STDHANDLE_CONSOLE	0

#define CFE_DEV_NETWORK 	1
#define CFE_DEV_DISK		2
#define CFE_DEV_FLASH		3
#define CFE_DEV_SERIAL		4
#define CFE_DEV_CPU		5
#define CFE_DEV_NVRAM		6
#define CFE_DEV_CLOCK           7
#define CFE_DEV_OTHER		8
#define CFE_DEV_MASK		0x0F

#define CFE_CACHE_FLUSH_D	1
#define CFE_CACHE_INVAL_I	2
#define CFE_CACHE_INVAL_D	4
#define CFE_CACHE_INVAL_L2	8

/*  *********************************************************************
    *  Structures
    ********************************************************************* */

typedef unsigned long long cfe_xuint_t;
typedef long long cfe_xint_t;
typedef long long cfe_xptr_t;

typedef struct xiocb_buffer_s {
    cfe_xuint_t   buf_offset;		/* offset on device (bytes) */
    cfe_xptr_t 	  buf_ptr;		/* pointer to a buffer */
    cfe_xuint_t   buf_length;		/* length of this buffer */
    cfe_xuint_t   buf_retlen;		/* returned length (for read ops) */
    cfe_xuint_t   buf_ioctlcmd;		/* IOCTL command (used only for IOCTLs) */
} xiocb_buffer_t;

#define buf_devflags buf_ioctlcmd	/* returned device info flags */

typedef struct xiocb_inpstat_s {
    cfe_xuint_t inp_status;		/* 1 means input available */
} xiocb_inpstat_t;

typedef struct xiocb_envbuf_s {
    cfe_xint_t enum_idx;		/* 0-based enumeration index */
    cfe_xptr_t name_ptr;		/* name string buffer */
    cfe_xint_t name_length;		/* size of name buffer */
    cfe_xptr_t val_ptr;			/* value string buffer */
    cfe_xint_t val_length;		/* size of value string buffer */
} xiocb_envbuf_t;

typedef struct xiocb_cpuctl_s {
    cfe_xuint_t  cpu_number;		/* cpu number to control */
    cfe_xuint_t  cpu_command;		/* command to issue to CPU */
    cfe_xuint_t  start_addr;		/* CPU start address */
    cfe_xuint_t  gp_val;		/* starting GP value */
    cfe_xuint_t  sp_val;		/* starting SP value */
    cfe_xuint_t  a1_val;		/* starting A1 value */
} xiocb_cpuctl_t;

typedef struct xiocb_time_s {
    cfe_xint_t ticks;			/* current time in ticks */
} xiocb_time_t;

typedef struct xiocb_exitstat_s {
    cfe_xint_t status;
} xiocb_exitstat_t;

typedef struct xiocb_meminfo_s {
    cfe_xint_t  mi_idx;			/* 0-based enumeration index */
    cfe_xint_t  mi_type;		/* type of memory block */
    cfe_xuint_t mi_addr;		/* physical start address */
    cfe_xuint_t mi_size;		/* block size */
} xiocb_meminfo_t;

#define CFE_FWI_64BIT		0x00000001
#define CFE_FWI_32BIT		0x00000002
#define CFE_FWI_RELOC		0x00000004
#define CFE_FWI_UNCACHED	0x00000008
#define CFE_FWI_MULTICPU	0x00000010
#define CFE_FWI_FUNCSIM		0x00000020
#define CFE_FWI_RTLSIM		0x00000040

typedef struct xiocb_fwinfo_s {
    cfe_xint_t fwi_version;		/* major, minor, eco version */
    cfe_xint_t fwi_totalmem;		/* total installed mem */
    cfe_xint_t fwi_flags;		/* various flags */
    cfe_xint_t fwi_boardid;		/* board ID */
    cfe_xint_t fwi_bootarea_va;		/* VA of boot area */
    cfe_xint_t fwi_bootarea_pa;		/* PA of boot area */
    cfe_xint_t fwi_bootarea_size;	/* size of boot area */
    cfe_xint_t fwi_reserved1;
    cfe_xint_t fwi_reserved2;
    cfe_xint_t fwi_reserved3;
} xiocb_fwinfo_t;

/*
** Hardware Info
*/
#define CFE_HWINFO_MIN_REV             15       /* Derived from magic number = version, hex format */
#define CFE_HWINFO_MIN_REV_NAND        16       /* Derived from magic number = version, hex format */ 

#define CFE_FLASH_PARTITION_NAME_SIZE  64
#define CFE_MAX_FLASH_PARTITIONS       32

#define CFE_NUM_DDR_CONTROLLERS         2
#define CFE_NUM_ENET_CONTROLLERS        2
#define CFE_NUM_UART_CONTROLLERS        8
#define CFE_MAC_ADDR_LENGTH             6

typedef enum cfe_xflash_types_s {
	FLASH_XTYPE_NOR,
	FLASH_XTYPE_NAND
} cfe_xflash_types_t;

typedef struct cfe_xflash_partition_map_s {
	char        part_name[CFE_FLASH_PARTITION_NAME_SIZE];
	cfe_xptr_t  base;
	cfe_xuint_t offset;
	cfe_xuint_t size;
} cfe_xflash_partition_map_t;

typedef struct cfe_xuart_layout_s {
	cfe_xint_t          channel;                                            /* uart number */
	cfe_xptr_t          base_address;                                       /* BRCM_SERIALn_BASE */
	cfe_xint_t          divisor;                                            /* UART_DLL setting */
	cfe_xuint_t         baud_rate;                                         /* absolute baudrate  */
	cfe_xuint_t         data_length;                                        /* data length in bits */
	cfe_xuint_t         stopbits_flag;                                     /* stop flag 0/1 */
} cfe_xuart_layout_t;

typedef struct xiocb_boardinfo_s {
	cfe_xuint_t                 bi_ver_magic;
	cfe_xuint_t                 bi_chip_version;                            /* from SUN_TOP_CTRL_PROD_REVISION register */
	cfe_xflash_types_t          bi_flash_type;                              /* NOR, NAND */
	cfe_xuint_t                 bi_config_flash_numparts;              /* Actual number of partitions */
	cfe_xflash_partition_map_t  bi_flash_partmap[CFE_MAX_FLASH_PARTITIONS]; /* flash partition layout */
	cfe_xuint_t                 bi_config_numuarts;                         /* number of uarts confugered */ 
	cfe_xuart_layout_t          bi_uarts[CFE_NUM_UART_CONTROLLERS];         /* console uart details */
	cfe_xuint_t                 bi_ddr_bank_size[CFE_NUM_DDR_CONTROLLERS];  /* memory bank size, in MB, by controller */
	cfe_xuint_t                 bi_config_numenetctrls;                     /* Actual number of enet controllers */
	unsigned char               bi_mac_addr[CFE_NUM_ENET_CONTROLLERS][CFE_MAC_ADDR_LENGTH];   /* ethernet mac address per controller */
} xiocb_boardinfo_t;

typedef struct cfe_xiocb_s {
	cfe_xuint_t xiocb_fcode;		/* IOCB function code */
	cfe_xint_t  xiocb_status;		/* return status */
	cfe_xint_t  xiocb_handle;		/* file/device handle */
	cfe_xuint_t xiocb_flags;		/* flags for this IOCB */
	cfe_xuint_t xiocb_psize;		/* size of parameter list */
	union {
	xiocb_buffer_t  xiocb_buffer;		/* buffer parameters */
	xiocb_inpstat_t xiocb_inpstat;		/* input status parameters */
	xiocb_envbuf_t  xiocb_envbuf;		/* environment function parameters */
	xiocb_cpuctl_t  xiocb_cpuctl;		/* CPU control parameters */
	xiocb_time_t    xiocb_time;		/* timer parameters */
	xiocb_meminfo_t xiocb_meminfo;		/* memory arena info parameters */
	xiocb_fwinfo_t  xiocb_fwinfo;		/* firmware information */
	xiocb_exitstat_t xiocb_exitstat;	/* Exit Status */
	xiocb_boardinfo_t xiocb_boardinfo; 	/* Board information */
	} plist;
} cfe_xiocb_t;

/*
** Standardize check for minimum CFE Rev for hwinfo support
*/
extern cfe_xiocb_t cfe_boardinfo;
extern int cfe_hwinfo_called;
extern int cfe_hwinfo_stat;


static inline int cfe_min_rev(cfe_xuint_t magic_number) {
	int cfe_magic;

/*
    If version is 1.5.7, magic_number is 0x10507, 
    We shift it rigth 16,                       0x10507>>16         = 0x1
    We multiply the value by 10 (upper digit)   0x1 * 10            = 0xA
    We shift it rigth 8,                        0x10507>>8          = 0x105 
    We remove the upper digit                   0x105 - (0x1 << 8)  = 0x5
    We add the upper and lower digits           0xA+0x5             = 0xF (15). 
*/
	if (cfe_hwinfo_called && cfe_hwinfo_stat == 0) 
    {
        //printk("CFE magic_number: 0x%08X\n",(int)magic_number);

        cfe_magic = (magic_number >> 16) * 10;
        cfe_magic += (magic_number >> 8) - ((magic_number >> 16) << 8);

        //printk("cfe_magic: 0x%08X\n", cfe_magic);
		if (cfe_magic >= CFE_HWINFO_MIN_REV)
            return 1;
    }
    return 0;
}

static inline int cfe_min_rev_nand(cfe_xuint_t magic_number) {
	int cfe_magic;

/*
    If version is 1.5.7, magic_number is 0x10507, 
    We shift it rigth 16,                       0x10507>>16         = 0x1
    We multiply the value by 10 (upper digit)   0x1 * 10            = 0xA
    We shift it rigth 8,                        0x10507>>8          = 0x105 
    We remove the upper digit                   0x105 - (0x1 << 8)  = 0x5
    We add the upper and lower digits           0xA+0x5             = 0xF (15). 
*/
	if (cfe_hwinfo_called && cfe_hwinfo_stat == 0) 
    {
        //printk("CFE magic_number: 0x%08X\n",(int)magic_number);

        cfe_magic = (magic_number >> 16) * 10;
        cfe_magic += (magic_number >> 8) - ((magic_number >> 16) << 8);

        //printk("cfe_magic: 0x%08X\n", cfe_magic);
		if (cfe_magic >= CFE_HWINFO_MIN_REV_NAND)
            return 1;
    }
    return 0;
}

#endif
