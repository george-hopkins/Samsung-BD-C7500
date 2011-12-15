/*
 * arch/mips/brcmstb/common/cfe_call.c
 *
 * Copyright (C) 2009 Broadcom Corporation
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
 * Broadcom specific proc fs commands
 *
 * 
 */

#if defined (CONFIG_MIPS_BCM_NDVD)

#include <linux/proc_fs.h>

#include <asm/brcmstb/common/brcmstb.h>
#include "cfe_xiocb.h"

extern int cfw_hwinfo_called;
extern cfe_xiocb_t cfe_boardinfo;
extern char cfe_boardname[];

static int proc_calc_metrics(char *page, char **start, off_t off,
				 int count, int *eof, int len)
{
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}

static int bcm_mem_regions_read_proc(char *page, char **start, off_t off,
                                int count, int *eof, void *data)
{
	int len;
	int i;

	len = 0;

	len += sprintf(page + len, "address         size\n");
	for (i = 0; i < MAX_MEM_REGIONS; i++) {
	    if (mem_regions[i].size)
		len += sprintf(page+len,
		    "0x%08lx    0x%08lx\n",
		    mem_regions[i].paddr,
		    mem_regions[i].size);
	}

	return proc_calc_metrics(page, start, off, count, eof, len);
}

static int bcm_bmem_read_proc(char *page, char **start, off_t off,
                                int count, int *eof, void *data)
{
	int len;
	int i;

	len = 0;

	for (i = 0; i < MAX_BMEM_REGIONS; i++) {
		len += sprintf(page+len,
		    "BmemAddress[%d]: 0x%08lx    BmemSize[%d]: 0x%08lx\n",
		    i, bmem_regions[i].paddr,
		    i, bmem_regions[i].size);
	}

	return proc_calc_metrics(page, start, off, count, eof, len);
}


static int bcm_hwinfo_proc(char *page, char **start, off_t off,
                                int count, int *eof, void *data)
{
	int i, j;
	int len = 0;
	int size;
	unsigned int xlat[8];
	xiocb_boardinfo_t *boardinfo;
	char *partition_strings;
	char *uart_strings;
	char *macaddr_strings;
	char *pPtr;

	boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;

	if (cfe_min_rev(boardinfo->bi_ver_magic)) {

		/*
		 * Create strings for flash partitions
		 */
		size = 256 * boardinfo->bi_config_flash_numparts;
		partition_strings = (char *)kmalloc(size, GFP_KERNEL);

		if (partition_strings == (char *)NULL) {
			return -ENOMEM;
		}

		memset(partition_strings, 0, size);

		pPtr = partition_strings;

		for (i = 0; i < boardinfo->bi_config_flash_numparts; i++) {
			xlat[0] = (unsigned int)boardinfo->bi_flash_partmap[i].base;
			xlat[1] = (unsigned int)boardinfo->bi_flash_partmap[i].offset;
			xlat[2] = (unsigned int)boardinfo->bi_flash_partmap[i].size;
			pPtr += sprintf(pPtr, "-   Partition %d: Name: %s, Base 0x%08x, Offset 0x%08x, Size 0x%08x\n",
					i,
					boardinfo->bi_flash_partmap[i].part_name,
					xlat[0],
					xlat[1],
					xlat[2]);
		}

		/*
		 * Create strings for uart info
		 */
		size = 256 * boardinfo->bi_config_numuarts;
		uart_strings = (char *)kmalloc(size, GFP_KERNEL);

		if (uart_strings == (char *)NULL) {
			kfree (partition_strings);
			return -ENOMEM;
		}

		memset(uart_strings, 0, size);

		pPtr = uart_strings;

		for (i = 0; i < boardinfo->bi_config_numuarts; i++) {
			xlat[0] = (unsigned int)boardinfo->bi_uarts[i].channel;
			xlat[1] = (unsigned int)boardinfo->bi_uarts[i].base_address;
			xlat[2] = (unsigned int)boardinfo->bi_uarts[i].divisor;
			xlat[3] = (unsigned int)boardinfo->bi_uarts[i].baud_rate;
			xlat[4] = (unsigned int)boardinfo->bi_uarts[i].data_length;
			xlat[5] = (unsigned int)boardinfo->bi_uarts[i].stopbits_flag;
			pPtr += sprintf(pPtr, "-   UART %d: Channel %d, Base_Address 0x%08x, Divisor 0x%08x, Baud_Rate 0x%08x, Data_Length %d, StopBits_Flag %d\n",
					i,
					xlat[0],
					xlat[1],
					xlat[2],
					xlat[3],
					xlat[4],
					xlat[5]);
		}

		/*
		 * Create strings for macaddr info
		 */
		size = 256 * boardinfo->bi_config_numenetctrls;
		macaddr_strings = (char *)kmalloc(size, GFP_KERNEL);

		if (macaddr_strings == (char *)NULL) {
			kfree (partition_strings);
			kfree (uart_strings);
			return -ENOMEM;
		}

		memset(macaddr_strings, 0, size);

		pPtr = macaddr_strings;

		for (i = 0; i < boardinfo->bi_config_numenetctrls; i++) {
			pPtr += sprintf(pPtr, "-   Enet Controller %d: MAC Address 0x", i);
			for (j = 0; j < CFE_MAC_ADDR_LENGTH; j++) {
				pPtr += sprintf(pPtr, "%02x ", boardinfo->bi_mac_addr[i][j]);
			}
			pPtr += sprintf(pPtr, "\n");
		}

		/*
		 * Necessary conversions from CFE datatypes to printable datatypes.
		 */
		xlat[0] = (unsigned int)boardinfo->bi_ver_magic;
		xlat[1] = (unsigned int)boardinfo->bi_chip_version;
		xlat[2] = (unsigned int)boardinfo->bi_config_flash_numparts;
		xlat[3] = (unsigned int)boardinfo->bi_config_numuarts;
		xlat[4] = (unsigned int)boardinfo->bi_ddr_bank_size[0];
		xlat[5] = (unsigned int)boardinfo->bi_ddr_bank_size[1];
		xlat[6] = (unsigned int)boardinfo->bi_config_numenetctrls;

		len = sprintf(page,
			      "CFE_BOARDNAME:                  %s\n"
			      "CFE Version Magic Number:       0x%08x\n"
			      "CFE SUN_TOP_CTRL_PROD_REVISION: 0x%08x\n"
			      "Flash Type:                     %s\n"
			      "Number of Flash Partitions:     %d\n"
			      "%s\n"
			      "Number of UARTS:                %d\n"
			      "%s"
			      "DDR Bank 0 Size (MB):           %d\n"
			      "DDR Bank 1 Size (MB):           %d\n"
			      "Number of Ethernet controllers: %d\n"
			      "%s",
			      cfe_boardname,
			      xlat[0],
			      xlat[1],
			      boardinfo->bi_flash_type == FLASH_XTYPE_NOR ? "NOR" : "NAND",
			      xlat[2],
			      partition_strings,
			      xlat[3],
			      uart_strings,
			      xlat[4],
			      xlat[5],
			      xlat[6],
			      macaddr_strings);

		kfree(partition_strings);
		kfree(uart_strings);
		kfree(macaddr_strings);
	}
	else {
		xlat[0] = (unsigned int)boardinfo->bi_ver_magic;
		len = sprintf(page,
			      "CFE Version Magic Number: 0x%08x does not support hwinfo\n",
			      xlat[0]);
	}

	return proc_calc_metrics(page, start, off, count, eof, len);
}


static int __init proc_misc_init(void)
{
	static struct {
		char *name;
		int (*read_proc)(char*,char**,off_t,int,int*,void*);
	} *p, simple_ones[] = {
    {"bmem", bcm_bmem_read_proc},
    {"mem_regions", bcm_mem_regions_read_proc},
    {"hwinfo", bcm_hwinfo_proc},
		{NULL,}
	};
	for (p = simple_ones; p->name; p++)
		create_proc_read_entry(p->name, 0, NULL, p->read_proc, NULL);
  return 0;
}

module_init(proc_misc_init);

#endif /* CONFIG_MIPS_BCM_NDVD */
