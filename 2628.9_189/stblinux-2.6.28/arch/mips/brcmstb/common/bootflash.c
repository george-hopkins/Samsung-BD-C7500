/*
 * Flash Base address for BCM97xxx boards
 *
 * Copyright (C) 2002,2003,2004,2005 Broadcom Corporation
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
 * THT 10/22/2002
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <asm/delay.h>
#if defined (CONFIG_MIPS_BCM_NDVD) && !defined (CONFIG_MIPS_BCM7405)
#include <../arch/mips/brcmstb/common/cfe_xiocb.h>
#endif

/*
 * The way the file bcm97xxx.h are defined, it would require a compile
 * time switch in order to determine whether we boot from flash or ROM.
 * This simple QRY test can test for us, as we query the CFE boot address
 * to see if it responds to the CFI 'QRY' test.
 */
 
#ifndef CODE_IN_FLASH
#define CODE_IN_FLASH
#endif

#if defined(CONFIG_MIPS_BRCM76XX) || defined(CONFIG_MIPS_BCM7405)
#include <asm/brcmstb/common/brcmstb.h>
#define WINDOW_ADDR 0x1c000000		/* 2X 32MB flash */

#endif

static unsigned long FLASH_FLASH_BASE = WINDOW_ADDR;
extern const unsigned long ROM_FLASH_BASE;


static unsigned long RT_PHYS_FLASH_BASE = WINDOW_ADDR;

unsigned long 
getPhysFlashBase(void)
{
	return RT_PHYS_FLASH_BASE;
}
EXPORT_SYMBOL(getPhysFlashBase);

#define BOOT_LOADER_ENTRY 0xbfc00000



#if defined (CONFIG_MIPS_BRCM97XXX)
/*
 * Determine whether CFE was booted from Flash or ROM 
 */
void 
determineBootFromFlashOrRom(void)
{
#if defined( CONFIG_BRCM_PCI_SLAVE ) || (defined( CONFIG_MTD_BRCMNAND ) && !defined( CONFIG_MIPS_BCM7405 ))
	return;
#else
	char msg[128];
	
	extern int gFlashSize;
	extern int gFlashCode;
	unsigned short   query[3];
	//unsigned char cquery[3];
	volatile unsigned short * queryaddr;
	//volatile unsigned char * cqueryaddr;
	int bootFromFlash = 0;

	/* Reset for Spansion flash only */
	if (gFlashCode == 1) {
		*(volatile unsigned short *)(BOOT_LOADER_ENTRY | (0x55 << 1)) = 0xF0;
		udelay(10);
	}
	
	/* Enter query mode x16 */
	*(volatile unsigned short *)(BOOT_LOADER_ENTRY | (0x55 << 1)) = 0x98;
	//udelay(1000);
	
	queryaddr = (volatile unsigned short *)(BOOT_LOADER_ENTRY | (0x10 << 1));


	query[0] = *queryaddr++;
	query[1] = *queryaddr++;
	query[2] = *queryaddr;

	/* Go back to read-array mode */
	*(volatile unsigned short *)(BOOT_LOADER_ENTRY | (0x55 << 1)) = 0xFFFF;

#if (!defined( CONFIG_MIPS_BCM7110 ) || defined( CONFIG_MIPS_BCM7110_DSG))
	if( query[0] == 0x51 &&     /* Q */
	   	query[1] == 0x52 &&     /* R */
	   	query[2] == 0x59  )    /* Y */
	{
		bootFromFlash = 1;
#if defined (CONFIG_MIPS_BCM_NDVD) && !defined (CONFIG_MIPS_BCM7405)
		{
			/*
			** Determine flash size dynamically from flash partition map from CFE HW INFO
			*/
			xiocb_boardinfo_t *boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;
			cfe_xuint_t member, nElements;
			unsigned long baseAddr;
			int i, numPartitions;

			if (cfe_min_rev(boardinfo->bi_ver_magic)) {

				if (boardinfo->bi_flash_type == FLASH_XTYPE_NOR) {

					nElements     = boardinfo->bi_config_flash_numparts;
					numPartitions = (int)nElements;

					for (i = 0; i < numPartitions; i++) {
						member   = boardinfo->bi_flash_partmap[i].base;
						baseAddr = (unsigned long)member;

						if (baseAddr && (baseAddr < FLASH_FLASH_BASE)) {
							/* 
							** Adjust FLASH_FLASH_BASE to the lowest
							** base address reported by CFE.
							*/
							FLASH_FLASH_BASE = baseAddr;
						}
					}
				}
			}
		}
#endif // defined (CONFIG_MIPS_BCM_NDVD)
	}
  #if 0
/* THT: HW guarantees bus-width is always x16, but we may need this  for NAND flash */
	else {
		/* Enter query mode x8 */
		*(volatile unsigned char *)(BOOT_LOADER_ENTRY | (0x55 << 1)) = 0x98;
		cqueryaddr = (volatile unsigned char *)(BOOT_LOADER_ENTRY | (0x20));
	   	cquery[0] = *cqueryaddr; cqueryaddr += 2;
		cquery[1] = *cqueryaddr; cqueryaddr += 2;
		cquery[2] = *cqueryaddr;

		/* Go back to read-array mode */
		*(volatile unsigned short *)(BOOT_LOADER_ENTRY | (0x55 << 1)) = 0xFFFF;
sprintf(msg, "gFlashSize=%08x, cquery[0]=%04x, [1]=%04x, [2]=%04x\n", gFlashSize, cquery[0], cquery[1], cquery[2]);
uart_puts(msg);
		if ( cquery[0] == 0x51 &&     /* Q */
		   	cquery[1] == 0x52 &&     /* R */
		   	cquery[2] == 0x59  )    /* Y */	
		{
			bootFromFlash = 1;
		}
	}
  #endif /* x8 codes */
#else

	/*
	 * The 7110 (Not the 7110-DSG) has 2 8bit flash chips that are interleaved.  Rather than using the CFI_probe routine which
	 * does this test taking interleave into account, for all instances and purposes, this should
	 * be enough
	 */
	if( query[0] == 0x5151 &&     /* Q */
	   	query[1] == 0x5252 &&     /* R */
	   	query[2] == 0x5959  )    /* Y */
	{
	   	bootFromFlash = 1;
	}
#endif
	if (bootFromFlash) {	
        	if (!gFlashSize) {
		RT_PHYS_FLASH_BASE = FLASH_FLASH_BASE;
        	}
       	else {
            		RT_PHYS_FLASH_BASE = (0x20000000 - (gFlashSize << 20));
        	}
		sprintf(msg, "**********BOOTEDFROMFLASH, Base=%08lx\n", RT_PHYS_FLASH_BASE);
		uart_puts(msg);
	
	} else {

		
		RT_PHYS_FLASH_BASE = ROM_FLASH_BASE;
		sprintf(msg, "**********BOOTEDFROMROM, Base=%08lx\n", RT_PHYS_FLASH_BASE);
		uart_puts(msg);
	}
#endif // if PCI slave

}

#endif /* if BCM97xxx boards */
