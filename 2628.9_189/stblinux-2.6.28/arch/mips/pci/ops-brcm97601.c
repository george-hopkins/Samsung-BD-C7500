/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Bcm97601 PCI COntroller specific pci setup.
 *
 * Copyright 2008 Broadcom Corp.
 *
 * This file was derived from the sample file pci_ops.c
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Revision Log
 * who when    what
 * tht  20041004 Adapted from sample codes from kernel tree
 * tht  20050603 Support for secondary PCI bus for 7038C0
 */
#include <linux/init.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/ioport.h>

#include <asm/delay.h>
//#include <asm/pci_channel.h>
#include <asm/io.h>
#include <asm/debug.h>

#include <asm/brcmstb/common/brcmstb.h>


/* Since the following is not defined in any of our header files. */
#define MIPS_PCI_XCFG_INDEX     0xf0600004
#define MIPS_PCI_XCFG_DATA      0xf0600008

#define MIPS_PCI_SATA_XCFG_INDEX     (0xb0000000+BCHP_PCIX_BRIDGE_SATA_CFG_INDEX)
#define MIPS_PCI_SATA_XCFG_DATA      (0xb0000000+BCHP_PCIX_BRIDGE_SATA_CFG_DATA)


#undef DEBUG

/* from PCI spec, Maybe we can put this in some include file. */
#define PCI_ENABLE              0x80000000
#define PCI_IDSEL(x)		(((x)&0x1f)<<11)
#define PCI_FNCSEL(x)		(((x)&0x7)<<8)

static int brcm_pci_sata_read_config_dword(struct pci_bus *bus, unsigned int devfn,
				unsigned int where, u32 *val)
{
    int slot_num, func_num;
    u32 cfc,cf8;

    db_assert((where & 3) == 0);
    db_assert(where < (1 << 8));
	
    /* SATA is the only device on the bus, with devfn == 0 */
    if (devfn != 0) 
    {
	    return PCIBIOS_FUNC_NOT_SUPPORTED;
    }

    slot_num = PCI_SLOT(devfn);
    func_num = PCI_FUNC(devfn);
    cf8 = PCI_ENABLE + PCI_IDSEL(slot_num) + PCI_FNCSEL(func_num) + where;

    *(volatile u32*)(MIPS_PCI_SATA_XCFG_INDEX) = cf8;
    cfc = *(volatile u32*)(MIPS_PCI_SATA_XCFG_DATA);
    *val = cfc;
    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_sata_write_config_dword(struct pci_bus *bus, unsigned int devfn, unsigned int where,
					u32 val)
{
    int slot_num, func_num;
    u32 cfc;

    db_assert((where & 3) == 0);
    db_assert(where < (1 << 8));
	
		
    /* SATA is the only device on the bus, with devfn == 0 */
    if (devfn != 0) 
    {
	    return PCIBIOS_FUNC_NOT_SUPPORTED;
    }

    slot_num = PCI_SLOT(devfn);
    func_num = PCI_FUNC(devfn);

	cfc = PCI_ENABLE + PCI_IDSEL(slot_num) + PCI_FNCSEL(func_num) + where;
	*(volatile u32*)(MIPS_PCI_SATA_XCFG_INDEX) = cfc;
	*(volatile u32*)(MIPS_PCI_SATA_XCFG_DATA) = val;

    return PCIBIOS_SUCCESSFUL;
}


static int brcm_pci_read_config_dword(struct pci_bus *bus, unsigned int devfn,
				unsigned int where, u32 *val)
{
    int slot_num, func_num;
    u32 cfc,cf8;

    db_assert((where & 3) == 0);
    db_assert(where < (1 << 8));
	


    /* SATA PCI bus?  */
    switch (bus->number) {
    case 1: /*slot_num == PCI_DEV_NUM_SATA*/
        return brcm_pci_sata_read_config_dword(bus, devfn, where, val);
		
    case 0:
        break;
    default:

     /*
      *   configuration cycle 0,1 only (primary and sata bus only)
      */
         printk(KERN_ERR "brcm_pci_write_config_dword: Invalid bus number\n");
	     return PCIBIOS_FUNC_NOT_SUPPORTED;
    }

    slot_num = PCI_SLOT(devfn);
    func_num = PCI_FUNC(devfn);
    cf8 = PCI_ENABLE + PCI_IDSEL(slot_num) + PCI_FNCSEL(func_num) + where;

    *(volatile u32*)(MIPS_PCI_XCFG_INDEX) = cf8;
    cfc = *(volatile u32*)(MIPS_PCI_XCFG_DATA);
    *val = cfc;
    return PCIBIOS_SUCCESSFUL;
}




static int brcm_pci_write_config_dword(struct pci_bus *bus, unsigned int devfn, unsigned int where,
					u32 val)
{
    int slot_num, func_num;
    u32 cfc;

    db_assert((where & 3) == 0);
    db_assert(where < (1 << 8));
	
    /*
     *  For starters let's do configuration cycle 0 and 1 only
     */
    switch (bus->number) {
    case 1: /*slot_num == PCI_DEV_NUM_SATA*/
        return brcm_pci_sata_write_config_dword(bus, devfn, where, val);
		
    case 0:
        break;
    default:
      printk(KERN_ERR "brcm_pci_write_config_dword: Invalid bus number\n");
	    return PCIBIOS_FUNC_NOT_SUPPORTED;
    }

    slot_num = PCI_SLOT(devfn);
    func_num = PCI_FUNC(devfn);

	cfc = PCI_ENABLE + PCI_IDSEL(slot_num) + PCI_FNCSEL(func_num) + where;
	*(volatile u32*)(MIPS_PCI_XCFG_INDEX) = cfc;
	*(volatile u32*)(MIPS_PCI_XCFG_DATA) = val;

    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_read_config_word(struct pci_bus *bus, unsigned int devfn, unsigned int where, u16 *val)
{
    int status;
    u32 result;
    
    db_assert((where & 1) == 0);

    status = brcm_pci_read_config_dword(bus, devfn, where & ~3, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 2)
	result >>= 16;
    *val = result & 0xffff;
    return PCIBIOS_SUCCESSFUL;
}

static int brcm_pci_read_config_byte(struct pci_bus *bus, unsigned int devfn, unsigned int where, u8 *val)
{
    int status;
    u32 result;
	
    status = brcm_pci_read_config_dword(bus, devfn, where & ~3, &result);

#ifdef DEBUG
    printk("brcm_pci_read_config_byte res32[%x]=%08x\n", where, result);
#endif

    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 1)
	result >>= 8;
    if (where & 2)
	result >>= 16;

#ifdef DEBUG
    printk("after shift, res=%08x\n", result);
#endif

    *val = result & 0xff;

#ifdef DEBUG
    printk("returning, res=%0x\n", *val);
#endif

    return PCIBIOS_SUCCESSFUL;
}



static int brcm_pci_write_config_word(struct pci_bus *bus, unsigned int devfn, unsigned int where, u16 val)
{
    int status, shift = 0;
    u32 result;

    db_assert((where & 1) == 0);

    status = brcm_pci_read_config_dword(bus, devfn, where & ~3, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 2)
	shift += 16;
    result &= ~(0xffff << shift);
    result |= val << shift;
    return brcm_pci_write_config_dword(bus, devfn, where  & ~3, result);
}

static int brcm_pci_write_config_byte(struct pci_bus *bus, unsigned int devfn, unsigned int where, u8 val)
{
    int status, shift = 0;
    u32 result;

    status = brcm_pci_read_config_dword(bus, devfn, where & ~3, &result);
    if (status != PCIBIOS_SUCCESSFUL)
	return status;
    if (where & 2)
	shift += 16;
    if (where & 1)
	shift += 8;
    result &= ~(0xff << shift);
    result |= val << shift;
    return brcm_pci_write_config_dword(bus, devfn, where & ~3, result);
}

static int brcm_pci_read_config(struct pci_bus *bus, unsigned int devfn, int where,
		    int size, u32* val)
{
	int status;
	
	switch (size) {
	case 1: 
	{
		u8 val8;
		status = brcm_pci_read_config_byte(bus, devfn, (unsigned int) where, &val8);
		*val = (u32) val8;
		return status;
	}
	case 2:
	{
		u16 val16;
		status =  brcm_pci_read_config_word(bus, devfn, (unsigned int) where, (u16*) &val16);
		*val = (u32) val16;
		return status;
	}
	case 4:
		return brcm_pci_read_config_dword(bus, devfn, (unsigned int) where, val);
	default:
		printk("brcm_pci_read_config: Invalid size of %d\n", size);
	}
	return -1;
}


static int brcm_pci_write_config(struct pci_bus *bus, unsigned int devfn, int where,
		    int size, u32 val)
{
	switch (size) {
	case 1:
		return brcm_pci_write_config_byte(bus, devfn, (unsigned int) where, val);

	case 2:
		return brcm_pci_write_config_word(bus, devfn, (unsigned int) where, val);

	case 4:
		return brcm_pci_write_config_dword(bus, devfn, (unsigned int) where, val);
	default:
		printk("brcm_pci_write_config: Invalid size of %d\n", size);
	}
	return -1;
}

struct pci_ops bcm7601_pci_ops = {
	.read = brcm_pci_read_config,
	.write = brcm_pci_write_config,
};


