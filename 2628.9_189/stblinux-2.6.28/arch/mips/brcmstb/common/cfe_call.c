/*
 * arch/mips/brcmstb/common/cfe_call.c
 *
 * Copyright (C) 2001-2004 Broadcom Corporation
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
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/bootinfo.h>
#include "cfe_xiocb.h"
#include "asm/brcmstb/common/cfe_call.h"
#include <linux/module.h>

extern unsigned int cfe_seal;

#if defined (CONFIG_MIPS_BCM_NDVD)
/*
 * Board Name from CFE
 */
char cfe_boardname[CFE_BOARDNAME_MAX_LEN];

/*
 * Various NDVD environment variables provided to user
 * for tuning knobs.
 */
char tmp_envbuf[128];

int ATAPI_ERRINFO   = -1;
int NO_EARLY_SPINUP = -1;

#endif

#if defined (CONFIG_MIPS_BCM_NDVD) && !defined (CONFIG_MIPS_BCM7405)
/*
** Structure for return of CFE HW INFO
*/

cfe_xiocb_t cfe_boardinfo;
EXPORT_SYMBOL(cfe_boardinfo);

int cfe_hwinfo_called  = 0;
EXPORT_SYMBOL(cfe_hwinfo_called);

int cfe_hwinfo_stat    = -99;
EXPORT_SYMBOL(cfe_hwinfo_stat);

/*
 * Various platform-specific environment variables
 * provided to user for SATA PHY tuning knobs.
 */
int SATA_RX_LVL   = -1;
int SATA_TX_LVL   = -1;
int SATA_TX_PRE   = -1;

/*
 * Variable used to override the default platform
 * IPP state (asserted) in the USB_PWR_CTL mask define
 * in ./drivers/usb/host/brcmusb.h. Note that for
 * CONFIG_MIPS_BCM_NDVD the default state of
 * USB_PWR_CTL is (IPP | IOC | OC_DIS). Exporting
 * control of the IPP setting via the USB_IPP environment
 * variable gives the customer an easy way to
 * invert IPP state for the MIC 2026-1YM controller
 * (HiDef DVD Gnats PR 12869 / PR 9802). By defining
 * USP_IPP as 0 under CFE, the USB_PWR_CTL_MASK will
 * effectively become (IOC | OC_DIS) as required.
 */
int USB_IPP       = -1;		/* IPP bit state in USB_PWR_CTL mask (see brcmusb.h) */

#endif

#define ETH_HWADDR_LEN 18	/* 12:45:78:ab:de:01\0 */

/*
 * Convert ch from a hex digit to an int
 */
static inline int hex(unsigned char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

static int
bcm_atoi(const char *s)
{
	int n;
	int neg = 0;

	n = 0;

	if (*s == '-') {
		neg = 1;
		s++;
	}

	while (isdigit(*s))
		n = (n * 10) + *s++ - '0';

	if (neg)
		n = 0 - n;

	return (n);
}


static int get_cfe_env_variable(cfe_xiocb_t *cfeparam,
				void * name_ptr, int name_length,
				void * val_ptr,  int val_length)
{
	int res = 0;

	cfeparam->xiocb_fcode  = CFE_CMD_ENV_GET;
	cfeparam->xiocb_status = 0;
	cfeparam->xiocb_handle = 0;
	cfeparam->xiocb_flags  = 0;
	cfeparam->xiocb_psize  = sizeof(xiocb_envbuf_t);
	cfeparam->plist.xiocb_envbuf.name_ptr    = (unsigned int)name_ptr;
	cfeparam->plist.xiocb_envbuf.name_length = name_length;
	cfeparam->plist.xiocb_envbuf.val_ptr     = (unsigned int)val_ptr;
	cfeparam->plist.xiocb_envbuf.val_length  = val_length;

	if (cfe_seal == CFE_SEAL) {
		res = cfe_call(cfeparam);
	}
	else
		res = -1;

	return (res);
}


/*
 * ethHwAddrs is an array of 16 uchar arrays, each of length 6, allocated by the caller
 * numAddrs are the actual number of HW addresses used by CFE.
 * For now we only use 1 MAC address for eth0
 */
int get_cfe_boot_parms( char bootParms[CFE_CMDLINE_BUFLEN], int* numAddrs, unsigned char* ethHwAddrs[] )
{
	/*
	 * This string can be whatever you want, as long
	 * as it is * consistant both within CFE and here.
	 */
	const char* cfe_env = "BOOT_FLAGS";
	const char* eth0HwAddr_env = "ETH0_HWADDR";
	cfe_xiocb_t cfeparam;
	int res;

	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,   strlen(cfe_env),
				   (void *)bootParms, CFE_CMDLINE_BUFLEN);

	if (res)
		res = -1;
	else {
		/* The kernel only takes 256 bytes, but CFE buffer can get up to 1024 bytes */
		if (strlen(bootParms) >= COMMAND_LINE_SIZE) {
			int i;
			for (i = COMMAND_LINE_SIZE-1; i >= 0; i--) {
				if (isspace(bootParms[i])) {
					bootParms[i] = '\0';
					break;
				}
			}
		}	
	}

#ifdef CONFIG_MTD_BRCMNAND
	if (ethHwAddrs != NULL) {
		unsigned char eth0HwAddr[ETH_HWADDR_LEN];
		int i, j, k;

		*numAddrs = 1;

		res = get_cfe_env_variable(&cfeparam,
					   (void *)eth0HwAddr_env, strlen(eth0HwAddr_env),
					   (void *)eth0HwAddr,     ETH_HWADDR_LEN*(*numAddrs));
		if (res)
			res = -2;
		else {
			if (strlen(eth0HwAddr) >= ETH_HWADDR_LEN*(*numAddrs))
				eth0HwAddr[ETH_HWADDR_LEN-1] = '\0';

			/*
			 * Convert to binary format
			 */
			for (k = 0; k < *numAddrs; k++) {
				unsigned char* hwAddr = ethHwAddrs[k];
				int done = 0;
				for (i = 0,j = 0; i < ETH_HWADDR_LEN && !done; ) {
					switch (eth0HwAddr[i]) {
					case ':':
						i++;
						continue;
				
					case '\0':
						done = 1;
						break;

					default:
						hwAddr[j] = (unsigned char) ((hex(eth0HwAddr[i]) << 4) | hex(eth0HwAddr[i+1]));
						j++;
						i +=2;
					}
				}
			}
			res = 0;
		}
	}
#endif

#if defined (CONFIG_MIPS_BCM_NDVD)
	/*
	 * Kernel tunables exported to the user
	 * via CFE Environment variables:
	 *
	 *  int ATAPI_ERRINFO   = -1;   {0, 1}
	 *  int NO_EARLY_SPINUP = -1;   {0, 1)
	 */
	cfe_env = "ATAPI_ERRINFO";
	tmp_envbuf[0] = 0;
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,    strlen(cfe_env),
				   (void *)tmp_envbuf, sizeof(tmp_envbuf));
	if (res == 0 && tmp_envbuf[0])
		ATAPI_ERRINFO = bcm_atoi(tmp_envbuf);
	else if (res)
		res = -3;

	cfe_env = "NO_EARLY_SPINUP";
	tmp_envbuf[0] = 0;
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,    strlen(cfe_env),
				   (void *)tmp_envbuf, sizeof(tmp_envbuf));
	if (res == 0 && tmp_envbuf[0])
		NO_EARLY_SPINUP = bcm_atoi(tmp_envbuf);
	else if (res)
		res = -3;

#endif

#if defined (CONFIG_MIPS_BCM_NDVD) && !defined (CONFIG_MIPS_BCM7405)
	/*
	 * Kernel tunables exported to the user
	 * via CFE Environment variables:
	 *
	 *  int SATA_RX_LVL   = -1;   {0..3}
	 *  int SATA_TX_LVL   = -1;   {0..15}
	 *  int SATA_TX_PRE   = -1;   {0..7}
	 *
	 *  int USB_IPP       = -1;   {0, 1}
	 */
	cfe_env = "SATA_RX_LVL";
	tmp_envbuf[0] = 0;
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,    strlen(cfe_env),
				   (void *)tmp_envbuf, sizeof(tmp_envbuf));
	if (res == 0 && tmp_envbuf[0])
		SATA_RX_LVL = bcm_atoi(tmp_envbuf);
	else if (res)
		res = -3;

	cfe_env = "SATA_TX_LVL";
	tmp_envbuf[0] = 0;
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,    strlen(cfe_env),
				   (void *)tmp_envbuf, sizeof(tmp_envbuf));
	if (res == 0 && tmp_envbuf[0])
		SATA_TX_LVL = bcm_atoi(tmp_envbuf);
	else if (res)
		res = -3;

	cfe_env = "SATA_TX_PRE";
	tmp_envbuf[0] = 0;
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,    strlen(cfe_env),
				   (void *)tmp_envbuf, sizeof(tmp_envbuf));
	if (res == 0 && tmp_envbuf[0])
		SATA_TX_PRE = bcm_atoi(tmp_envbuf);
	else if (res)
		res = -3;

	cfe_env = "USB_IPP";
	tmp_envbuf[0] = 0;
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,    strlen(cfe_env),
				   (void *)tmp_envbuf, sizeof(tmp_envbuf));
	if (res == 0 && tmp_envbuf[0])
		USB_IPP = bcm_atoi(tmp_envbuf);
	else if (res)
		res = -3;

	/*
	** Get CFE HW INFO
	*/
	cfeparam.xiocb_fcode  = CFE_CMD_GET_BOARD_INFO;
	cfeparam.xiocb_status = 0;
	cfeparam.xiocb_handle = 0;
	cfeparam.xiocb_flags  = 0;
	cfeparam.xiocb_psize  = sizeof(xiocb_boardinfo_t);

	if (cfe_seal == CFE_SEAL) {
		cfe_hwinfo_called = 1;
		cfe_hwinfo_stat = cfe_call(&cfeparam);

		/* Copy the returned xiocb structure to a global for later access */
		cfe_boardinfo = cfeparam;
	}

#endif

#if defined (CONFIG_MIPS_BCM_NDVD)

	/*
	 * Get the board name string.
	 */
	cfe_env = "CFE_BOARDNAME";
	res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,       strlen(cfe_env),
				   (void *)cfe_boardname, CFE_BOARDNAME_MAX_LEN);
	if (res)
		res = -4;
#endif

	return res;
}


