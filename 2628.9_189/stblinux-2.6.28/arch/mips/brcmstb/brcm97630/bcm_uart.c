/*
 *
 *
 *  Copyright (c) 2001-2009 Broadcom Corporation 
 *                                              
 *                                            
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed with the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA

    File: bcm_uart.c

    Description: 
    Simple UART driver for 16550V2 style UART

when	who what
-----	---	----
051011	tht	Original coding
 */

#define DFLT_BAUDRATE   115200

#define SER_DIVISOR(_x, clk)		(((clk) + (_x) * 8) / ((_x) * 16))
#define DIVISOR_TO_BAUD(div, clk)	((clk) / 16 / (div))


#include <linux/autoconf.h>
#include <linux/types.h>
#include "asm/brcmstb/common/serial.h"
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <asm/serial.h>
#include <asm/io.h>
#include <linux/module.h>
#include <asm/brcmstb/common/brcmstb.h>
#include <../arch/mips/brcmstb/common/cfe_xiocb.h>

static int shift = 2;

/*
 * At this point, the serial driver is not initialized yet, but we rely on the
 * bootloader to have initialized UARTA
 */

#if 0
/* 
* InitSerial() - initialize UART-A to 115200, 8, N, 1
*/
asm void InitSerial(void)
{
	.set noreorder

	/* t3 contains the UART BAUDWORD */
	li		t3,0x0e
	li	t0, 0xb0400b00
	li	t1,0x83  // DLAB, 8bit
	sw	t1,0x0c(t0)
	sw	t3,0x00(t0)
	sw	zero,0x04(t0)
	li	t1,0x03
	sw	t1,0x0c(t0)
	sw	zero,0x08(t0) //disable FIFO.
	
	jr	ra
	nop
	
	.set reorder
}
#endif

/* 
 * UART GPIO pin assignments from the 7630 RDB SUN_TOP_CTRL_PIN_MUX_CTRL.
 *  bits field is 4 bits wide
 *


 *	UART	GPIO	MUX	bits	value
 *	______________________________________
 *	0 rx	00	7	03:00	1
 *	0 tx	01	7	07:04	1
 *	______________________________________
 *	1 rx	02	7	11:08:  1
 *	1 tx	03	7	15:12	1
 *	______________________________________
 *	2 rx	04	7	19:16	1
 *	2 tx	05	7	23:20	1
 *	______________________________________
 */
struct uart_pin_assignment {
	    int rx_mux;
	    int rx_pos;
	    int tx_mux;
	    int tx_pos;
    };

struct uart_pin_assignment brcm_7630_uart_pins[4] = {
	{ 7, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7_gpio_00_SHIFT, 7, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7_gpio_01_SHIFT},
	{ 7, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7_gpio_02_SHIFT, 7, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7_gpio_03_SHIFT},
	{ 7, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7_gpio_04_SHIFT, 7, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7_gpio_05_SHIFT},
    } ;

static cfe_xuart_layout_t  cfe_uarts[CFE_NUM_UART_CONTROLLERS];


static void
serial_set_pin_mux(int chan)
{
	int mux, mask;
	struct uart_pin_assignment *pins;
	volatile unsigned long *MuxCtrl;

	/* Pin mux control registers 0-10 are contiguous */
	 MuxCtrl = (volatile unsigned long*) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET
						+ BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_0);

	if (chan < 0 || chan > 2)
		return;

	pins = &brcm_7630_uart_pins[chan];
	
	/* tx */
	mux  = pins->tx_mux;
	mask = 0x0f <<  pins->tx_pos;
	MuxCtrl[mux] &= ~mask;
	MuxCtrl[mux] |= 1 << pins->tx_pos;
	
	/* rx */
	mux  = pins->rx_mux;
	mask = 0x0f <<  pins->rx_pos;
	MuxCtrl[mux] &= ~mask;
	MuxCtrl[mux] |= 1 << pins->rx_pos;

}

unsigned long serial_init(int chan, void *ignored)
{

#ifdef CONFIG_BOOT_RAW
	unsigned long uartBaseAddr = UARTA_ADR_BASE   + (0x40 * chan);
#else

	unsigned long uartBaseAddr = cfe_uarts[chan].base_address; 
#endif

	void uartB_puts(const char *s);

#define DIVISOR cfe_uarts[chan].divisor

	shift = 2;

#ifndef CONFIG_BOOT_RAW

	/* ttyS0 has already been initialized by the bootloader */
	if (chan > 0 )
#endif
	{
		// Write DLAB, and (8N1) = 0x83
		writel(UART_LCR_DLAB|UART_LCR_WLEN8, (void *)(uartBaseAddr + (UART_LCR << shift)));
		// Write DLL to 0xe
		writel(DIVISOR, (void *)(uartBaseAddr + (UART_DLL << shift)));
		writel(0, (void *)(uartBaseAddr + (UART_DLM << shift)));

		// Clear DLAB
		writel(UART_LCR_WLEN8, (void *)(uartBaseAddr + (UART_LCR << shift)));

		// Disable FIFO
		writel(0, (void *)(uartBaseAddr + (UART_FCR << shift)));

		if (chan == 1) {
			uartB_puts("Done initializing UARTB\n");
		}
	}
	return (uartBaseAddr);
}


void
serial_putc(unsigned long com_port, unsigned char c)
{
	while ((readl((void *)(com_port + (UART_LSR << shift))) & UART_LSR_THRE) == 0)
		;
	writel(c, (void *)com_port);
}

unsigned char
serial_getc(unsigned long com_port)
{
	while ((readl((void *)(com_port + (UART_LSR << shift))) & UART_LSR_DR) == 0)
		;
	return readl((void *)com_port);
}

int
serial_tstc(unsigned long com_port)
{
	return ((readl((void *)(com_port + (UART_LSR << shift))) & UART_LSR_DR) != 0);
}

/* Old interface, for compatibility */

extern int console_uart;

/* --------------------------------------------------------------------------
    Name: PutChar
 Purpose: Send a character to the UART
-------------------------------------------------------------------------- */
void 
uart_putc(char c)
{
	serial_putc(cfe_uarts[console_uart].base_address, c);
}

void 
uartB_putc(char c)
{
    serial_putc(cfe_uarts[1].base_address, c);
}
/* --------------------------------------------------------------------------
    Name: PutString
 Purpose: Send a string to the UART
-------------------------------------------------------------------------- */
void 
uart_puts(const char *s)
{
#ifndef CONFIG_BOOT_RAW
   /* disable except for raw/emulation */
   return;
#endif

    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
    	uart_putc(*s++);
    }
}

void 
uartB_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uartB_putc('\r');
        }
    	uartB_putc(*s++);
    }
}
/* --------------------------------------------------------------------------
    Name: GetChar
 Purpose: Get a character from the UART. Non-blocking
-------------------------------------------------------------------------- */
char 
uart_getc(void)
{
	return serial_getc(cfe_uarts[console_uart].base_address);
}

char
uartB_getc(void)
{
	return serial_getc(cfe_uarts[1].base_address);
}


int console_initialized;
int brcm_console_initialized(void)
{
	return console_initialized;
}
EXPORT_SYMBOL(brcm_console_initialized);

/* --------------------------------------------------------------------------
    Name: bcm71xx_uart_init
 Purpose: Initalize the UARTA abd UARTB
-------------------------------------------------------------------------- */
void 
uart_init(unsigned long ignored)
{
	xiocb_boardinfo_t *boardinfo = (xiocb_boardinfo_t *)&cfe_boardinfo.plist.xiocb_boardinfo;
	int i;

	/*
	** Initialize the hwinfo for 3 uarts
	*/
	if (cfe_min_rev(boardinfo->bi_ver_magic)) {

		/* Configure from CFE HWinfo */
		console_uart = (int)boardinfo->bi_uarts[0].channel;

		cfe_uarts[console_uart].channel       = boardinfo->bi_uarts[0].channel;
		cfe_uarts[console_uart].base_address  = boardinfo->bi_uarts[0].base_address;
		cfe_uarts[console_uart].divisor       = boardinfo->bi_uarts[0].divisor;
		cfe_uarts[console_uart].baud_rate     = boardinfo->bi_uarts[0].baud_rate;
		cfe_uarts[console_uart].data_length   = boardinfo->bi_uarts[0].data_length;
		cfe_uarts[console_uart].stopbits_flag = boardinfo->bi_uarts[0].stopbits_flag;

		/*
		** Fix up the rest of the uart data - use divisor, baud rate, data_length
		** and stopbits_flag from the console uart settings.
		*/
		for (i = 0; i <= 2; i++) {
			if (i == console_uart) continue;
			cfe_uarts[i].channel = i;
			cfe_uarts[i].base_address  = (cfe_uarts[console_uart].base_address & ~0xff) + (i * 0x40);
			cfe_uarts[i].divisor       = cfe_uarts[console_uart].divisor;
			cfe_uarts[i].baud_rate     = cfe_uarts[console_uart].baud_rate;
			cfe_uarts[i].data_length   = cfe_uarts[console_uart].data_length;
			cfe_uarts[i].stopbits_flag = cfe_uarts[console_uart].stopbits_flag;
		}
	}
	else {
		/*
		** Cannot obtain uart config info from CFE - configure
		** from hard-coded chip RDB data.
		*/
		for (i = 0; i <= 2; i++) {
			cfe_uarts[i].channel = i;
			cfe_uarts[i].base_address  = BRCM_SERIAL1_BASE + (i* 0x40);
#ifdef CONFIG_HW_EMULATOR_UART_DIVISOR
			/* baud rate for HW emulator */
			cfe_uarts[i].divisor       = CONFIG_HW_EMULATOR_UART_DIVISOR;
#else
			cfe_uarts[i].divisor       = 44;
#endif
			cfe_uarts[i].baud_rate     = 115200;
			cfe_uarts[i].data_length   = 8;
			cfe_uarts[i].stopbits_flag = 0;
		}
	}

	serial_init(0, NULL); 		/* UARTA */
	serial_init(1, NULL);		/* UARTB */
	serial_init(2, NULL);		/* UARTC */

	serial_set_pin_mux(0);
	serial_set_pin_mux(1);
	serial_set_pin_mux(2);

#ifdef CONFIG_BOOT_RAW
	/* 
	 * Need to actually enable the UARTS 
	 */
	{
	volatile unsigned long *csr;
	 csr = (volatile unsigned long*) BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET
						+ BCHP_SUN_TOP_CTRL_GENERAL_CTRL_5);

	*csr &=  ~(BCHP_SUN_TOP_CTRL_GENERAL_CTRL_5_en_debug_uart_0_MASK
		 | BCHP_SUN_TOP_CTRL_GENERAL_CTRL_5_en_debug_uart_1_MASK
		 | BCHP_SUN_TOP_CTRL_GENERAL_CTRL_5_en_debug_uart_2_MASK
		 | BCHP_SUN_TOP_CTRL_GENERAL_CTRL_5_en_debug_uart_3_MASK);
	}
#endif
		
	//console_initialized = 1;
}

