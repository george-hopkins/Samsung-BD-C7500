#include <asm/mips-boards/prom.h>
int brcm_console_initialized=0;
int console_initialized=0;
int uart_puts(char *);
int get_RAM_size(void);

int uart_puts(char *s)
{
	prom_printf(s);
	return 1;
}

int get_RAM_size(void)
{
	return 32;
}

