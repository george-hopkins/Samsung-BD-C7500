/*
 * arch/mips/brcmstb/common/cfe_call.c
 *
 * Copyright (C) 2001 Broadcom Corporation
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
 * Interface between CFE boot loader and Linux Kernel.
 *
 * 
 */
#ifndef _CFE_CALL_H

#define CFE_SEAL	0x43464531


#ifdef DUMP_TRACE

#define CFE_TRACE(x)	printk x
#else
#define CFE_TRACE(x)
#endif

#define CFE_CMDLINE_BUFLEN 1024
#define CFE_BOARDNAME_MAX_LEN  256


typedef struct cmdEntry_t {
 	struct cmdEntry_t* next;
 	char* entry;
 	char* device;
 	unsigned int startAddr;
 	unsigned int size;
 } cmdEntry_t;

/*
 * Retrieve the CFE BOOT_PARMS environment variable
 * THe last 2 parameters return the MAC addresses of the internal MAC, and is only
 * meaningful for NAND flash, under CONFIG_MTD_BRCMNAND
 */
int get_cfe_boot_parms(char bootParms[], int* numAddrs, unsigned char* ethHwAddrs[]);

/*
 * Parse the CFE command line and transform it into appropriate
 * kernel boot parameter command line
 */
int parse_CFE_cmdline(cmdEntry_t* rootEntry, char cfeCmdLine[]);

int cfe_call(void* cfeHandle);
void uart_puts(const char* msg);

#endif /* _CFE_CALL_H */
