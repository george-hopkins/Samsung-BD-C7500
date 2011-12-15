/*---------------------------------------------------------------------------

    Copyright (c) 2001-2007 Broadcom Corporation                 /\
                                                          _     /  \     _
    _____________________________________________________/ \   /    \   / \_
                                                            \_/      \_/  
    
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    File: brcm-pm.h

    Description: 
    Power management for Broadcom STB/DTV peripherals

    when        who         what
    -----       ---         ----
    20071030    cernekee    initial version
 ------------------------------------------------------------------------- */

#ifndef _ASM_BRCMSTB_COMMON_BRCM_PM_H
#define _ASM_BRCMSTB_COMMON_BRCM_PM_H

#ifdef CONFIG_BRCM_PM

void brcm_pm_usb_add(void);
void brcm_pm_usb_remove(void);

void brcm_pm_enet_add(void);
void brcm_pm_enet_remove(void);

void brcm_pm_sata_add(void);
void brcm_pm_sata_remove(void);

void brcm_pm_register_sata(int (*off_cb)(void *), int (*on_cb)(void *),
	void *arg);
void brcm_pm_unregister_sata(void);

void brcm_pm_register_enet(int (*off_cb)(void *), int (*on_cb)(void *),
	void *arg);
void brcm_pm_unregister_enet(void);

void brcm_pm_register_ohci(int (*off_cb)(void *), int (*on_cb)(void *),
	void *arg);
void brcm_pm_unregister_ohci(void);

void brcm_pm_register_ehci(int (*off_cb)(void *), int (*on_cb)(void *),
	void *arg);
void brcm_pm_unregister_ehci(void);

#else /* CONFIG_BRCM_PM */

static inline void brcm_pm_usb_add(void) { }
static inline void brcm_pm_usb_remove(void) { }

static inline void brcm_pm_enet_add(void) { }
static inline void brcm_pm_enet_remove(void) { }

static inline void brcm_pm_sata_add(void) { }
static inline void brcm_pm_sata_remove(void) { }

#endif /* CONFIG_BRCM_PM */

#endif /* ! _ASM_BRCMSTB_COMMON_BRCM_PM_H */
