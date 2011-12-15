/*     Copyright (c) 1999-2007, Broadcom Corporation
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
** File:		bcmintrnum.h
** Description: Linux system int number header file
**
** Created: 03/05/2002 by Qiang Ye
** Updated for SMP: 03/10/2006 by Troy Trammel
** Modifed for 7440B0: 01/15/07 by Troy Trammel
**
** REVISION:
**
** $Log: $
**
**
****************************************************************/
#ifndef BCMINTRNUM_H
#define BCMINTRNUM_H


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Following is the complete map of interrupts in the system. Please
 * keep this up to date and make sure you comment your changes in the
 * comment block above with the date, your initials and what you did.
 *
 * There are two different interrupts in the system. The first type
 * is an actual hardware interrupt. We have a total of eight MIPS
 * interrupts. Two of them are software interrupts and are ignored.
 * The remaining six interrupts are actually monitored and handled
 * by low-level interrupt code in 'int-handler.S' that call dispatch
 * functions in this file to call the IRQ descriptors in the Linux
 * kernel.
 * 
 * The second type is the Linux kernel system interrupt which is
 * a table of IRQ descriptors (see 'include/linux/irq.h' and
 * 'include/linux/interrupt.h') that relate the hardware interrupt
 * handler types to the software IRQ descriptors. This is where all
 * of the status information and function pointers are defined so
 * that registration, releasing, disabling and enabling of interrupts
 * can be performed. Multiple system interrupts can be tied to a
 * single hardware interrupt. Examining this file along with the
 * other three aforementioned files should solidify the concepts.
 *
 * The first table simply shows the MIPS IRQ mapping:
 *
 *   MIPS IRQ   Source
 *   --------   ------
 *       0      Software (Used for SMP IPC)
 *       1      Software (Used for SMP IPC)
 *       2      Hardware BRCMSTB chip Internal
 *       3      Hardware External *Unused*
 *       4      Hardware External *Unused*
 *       5      Hardware External *Unused*
 *       6      R4k timer 
 *       7      Performance Counters
 */


/* JPF Serial Code depends on a unique IRQ for each serial port */

/* JPF Each line in the INTC has an IRQ */

#define	BCM_LINUX_CPU_ENET_IRQ		(1+BCHP_HIF_CPU_INTR1_INTR_W0_STATUS_ENET_CPU_INTR_SHIFT)

#define BCM_LINUX_SATA_IRQ		(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_PCI_SATA_CPU_INTR_SHIFT)

#define	BCM_LINUX_IDE_IRQ		(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_IDE_CPU_INTR_SHIFT)

#define BCM_LINUX_1394_IRQ		(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_PCI_INTA_1_CPU_INTR_SHIFT)
#define BCM_LINUX_EXT_PCI_IRQ		(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_PCI_INTA_0_CPU_INTR_SHIFT)
#define BCM_LINUX_MINI_PCI_IRQ		(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_PCI_INTA_2_CPU_INTR_SHIFT)


#define BCM_LINUX_USB_EHCI_CPU_INTR	(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_USB_EHCI_CPU_INTR_SHIFT)

#define BCM_LINUX_USB_OHCI_0_CPU_INTR	(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_USB_OHCI_0_CPU_INTR_SHIFT)
//#define BCM_LINUX_USB_OHCI_1_CPU_INTR	(1+32+BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_USB_OHCI_1_CPU_INTR_SHIFT)

#define BCM_LINUX_SHARF_IRQ		(BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_FGX_CPU_INTR_SHIFT + 1 + 32) /* 50 */

/* Auxiliary interrupts */
 
/* Some of the codes relies on these 4 interrupts to be consecutive */
#define BCM_LINUX_UARTA_IRQ		(1+32+32)
#define BCM_LINUX_UARTB_IRQ		(BCM_LINUX_UARTA_IRQ+1)
#define BCM_LINUX_UARTC_IRQ		(BCM_LINUX_UARTA_IRQ+2)
#define BCM_LINUX_UARTD_IRQ		(BCM_LINUX_UARTA_IRQ+3)
#define BCM_LINUX_UARTx_IRQ		(BCM_LINUX_UARTA_IRQ+3)

#define BCM_LINUX_SYSTIMER_IRQ		(BCM_LINUX_UARTx_IRQ+1)
#define BCM_LINUX_SYSTIMER_1_IRQ 	(BCM_LINUX_UARTx_IRQ+2)

#define BCM_LINUX_IPC_0_IRQ		(BCM_LINUX_UARTx_IRQ+3)
#define BCM_LINUX_IPC_1_IRQ		(BCM_LINUX_UARTx_IRQ+4)

#define BCM_LINUX_PERFCOUNT_IRQ		(BCM_LINUX_UARTx_IRQ+5)

#define BCM_LINUX_PATA_IRQ		(BCM_LINUX_UARTx_IRQ+6)

#define BCM_LINUX_EDU_IRQ           	(BCM_LINUX_UARTx_IRQ+7)

#ifdef __cplusplus
}
#endif

#endif


