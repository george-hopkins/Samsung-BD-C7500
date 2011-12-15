/*
 * arch/mips/brcm/dbg_io.c
 *
 * Copyright (C) 2001 Broadcom Corporation
 *                    Steven J. Hill <shill@broadcom.com>
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
 * Remote debugging routines for Broadcom eval boards
 *
 * 10-29-2001   SJH    Created
 * 07-19-2005   Richard Y. Hsu		Major Revision
 * 10-07-2005   Richard Y. Hsu		Modified for new kgdb support
 */
#include <asm/io.h>
#include <linux/kgdb.h>


#include <asm/brcmstb/common/brcmstb.h>


static int	remoteDebugInitialized = 0;

void debugInit()
{
	uart_init(27000000);

#ifdef CONFIG_SINGLE_SERIAL_PORT
	uart_puts("KGDB on /dev/ttyS0\n");
#else
	uart_puts("KGDB on /dev/ttyS1\n");
#endif
}

unsigned char getDebugChar(void)
{
	unsigned char c;

	if( !remoteDebugInitialized ) {
		remoteDebugInitialized = 1;
		debugInit();
	}

#ifdef CONFIG_SINGLE_SERIAL_PORT
	return uart_getc();	/* 0 : receiver not ready */     
#else
	while((c=uartB_getc())==0);	/* 0 : receiver not ready */     
	return c;	
	/* return uartB_getc(); */	/* 0 : receiver not ready */     
#endif
}

int putDebugChar(char c)
{
	if( !remoteDebugInitialized ) {
		remoteDebugInitialized = 1;
		debugInit();
	}
	
#ifdef CONFIG_SINGLE_SERIAL_PORT
	uart_putc(c); 
#else
	uartB_putc(c); 
#endif
	return 1;
}


struct kgdb_io kgdb_io_ops = {
	.read_char = getDebugChar,
	.write_char = putDebugChar,
	.init = debugInit,
	.late_init = NULL,
	.pre_exception = NULL,
	.post_exception = NULL
};
