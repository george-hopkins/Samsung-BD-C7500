/*
 *  vide_ofe.h - Header file for Broadcom VIDE OFE driver
 *
 *  Copyright 2009 Broadcom Corp.  All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __VIDE_OFE_H__
#define __VIDE_OFE_H__

#if defined (CONFIG_BCM_VIDE_OFE)

#include <asm/brcmstb/common/brcmstb.h>

#undef DRIVER_NAME
#undef DRIVER_VERSION
#define DRIVER_NAME "vide_ofe"
#define DRIVER_VERSION "1.0"

/* Module name for SB Protocol Driver side */
#define MODULE_NAME "vide_ofe_sb"

#undef REGADDR
#define REGADDR(offset)		BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + offset)


/* Probe defines */
#define ARM_SOFT_RESET		BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL_arm_soft_rst_MASK
#define OFE_SOFT_RESET		BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL_ofe_soft_rst_MASK
#define ARM_HALT_DEASSERT	BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_HALT_ARM_HALT_N_MASK
#define FE_RSRC_SIZE		3 * 1024 * 1024
#define FE_IMG_SIZE		1 * 1024 * 1024
#define OFE_RESET_MSEC		50

/* Reset defines */
#define FE_RESET_DONE		BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_STS_OFE_RESET_DONE_MASK
#define ARM_CTL_BITS		(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL_INITRAMA_MASK | \
				 BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL_CR4_DBGEN_MASK | \
				 BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL_DAP_DBGEN_MASK)

/* DEBUG */
#define VIDE_OFE_DEBUG

/* Register defines */
#define VIDE_HOST_REG_BASE	REGADDR(BCHP_VIDE_HOST_REG_REG_START)
#define VIDE_CTL_VERSION_MAJOR	REGADDR(BCHP_VIDE_CTL_VERSION + 2)
#define VIDE_CTL_VERSION_MINOR	REGADDR(BCHP_VIDE_CTL_VERSION + 1)

#define VIDE_BM_PRES_ADDR	REGADDR(BCHP_VIDE_BUS_MASTER_DBG_PRES_ADDR)
#define VIDE_BM_XFER_SIZE	REGADDR(BCHP_VIDE_BUS_MASTER_DBG_XFER_SIZE)
#define VIDE_BM_PRD_PTR		REGADDR(BCHP_VIDE_BUS_MASTER_DBG_PRD_PTR)
#define VIDE_BM_STATE		REGADDR(BCHP_VIDE_BUS_MASTER_DBG_STATE)

/* Register offsets off VIDE_HOST_REG_BASE */
#define VIDE_OFE_ALTSTATUS_OFFSET	BCHP_VIDE_HOST_REG_REG_2 + 2 - BCHP_VIDE_HOST_REG_REG_0
#define VIDE_OFE_BUS_MASTER_CMD_STATUS	BCHP_VIDE_BUS_MASTER_CMD_STATUS - BCHP_VIDE_HOST_REG_REG_START
#define VIDE_OFE_SCR_OFFSET		BCHP_VIDE_SCR_REG_START - BCHP_VIDE_HOST_REG_REG_START
#define VIDE_OFE_DMA_STATUS		0x01	/* off of VIDE_OFE_BUS_MASTER_CMD_STATUS */

enum {
	/* Unique register bits */
	VIDE_OFE_DEV0_DMA_ENABLE	= BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dev0DMA_MASK,
	VIDE_OFE_DMA_DIR		= BCHP_VIDE_BUS_MASTER_CMD_STATUS_Dir_MASK,
};

/* Register defines for Side Band Protocol */
#define SB_RU_ALIVE_ARM		REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SEMAPHORE_2)
#define SB_RU_ALIVE_MIPS	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SEMAPHORE_4)

#define SB_MIPS_MAILBOX		REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_MBOX_MIPS)
#define SB_ARM_MAILBOX		REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_MBOX_ARM)

#define SB_ARM_SEMAPHORE	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SEMAPHORE_1)
#define SB_MIPS_SEMAPHORE	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SEMAPHORE_3)

#define SB_ARM_SCRATCH_1	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_1)
#define SB_ARM_SCRATCH_2	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_2)
#define SB_ARM_SCRATCH_3	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_3)
#define SB_ARM_SCRATCH_4	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_4)

#define SB_MIPS_SCRATCH_1	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_5)
#define SB_MIPS_SCRATCH_2	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_6)
#define SB_MIPS_SCRATCH_3	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_7)
#define SB_MIPS_SCRATCH_4	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_8)

/* For MIPS Mailbox interrupts */
#define MIPS_MAILBOX_MASK	BCHP_OFE_DATAPATH_INTR2_CPU_STATUS_MIPS_MAILBOX_MASK
#define MB_INTR2_CPU_INTR_CLEAR	REGADDR(BCHP_OFE_DATAPATH_INTR2_CPU_CLEAR)
#define MB_INTR2_CPU_MASK_CLEAR	REGADDR(BCHP_OFE_DATAPATH_INTR2_CPU_MASK_CLEAR)

/* ARM Bridge Registers for Hard Reset and Firmware reload */
#define ARM_BRIDGE_CONTROL	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL)
#define ARM_BRIDGE_ARM_CONTROL	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL)
#define ARM_BRIDGE_STATUS	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_STS)
#define ARM_BRIDGE_ARM_HALT	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_HALT)
#define ARM_BRIDGE_DRAM_BASE	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_DRAM_OFFSET)
#define ARM_BRIDGE_DRAM_SIZE	REGADDR(BCHP_OFE_ARMCR4_BRIDGE_REG_DRAM_RANGE)

#define PM_CTRL			REGADDR(BCHP_CLK_PM_CTRL)
#define PM_CTRL_2		REGADDR(BCHP_CLK_PM_CTRL_2)
#define PM_CTRL_216		REGADDR(BCHP_CLK_PM_CTRL_216)

#define SUN_TOP_PROD_REVISION	REGADDR(BCHP_SUN_TOP_CTRL_PROD_REVISION)
#define SUN_TOP_CTRL_SW_RESET	REGADDR(BCHP_SUN_TOP_CTRL_SW_RESET)

/* External Functions */
extern u8          ata_altstatus(struct ata_port *ap);
extern void        ata_sff_tf_read(struct ata_port *ap, struct ata_taskfile *tf);
extern irqreturn_t ata_sff_interrupt(int irq, void *dev_instance);
extern void        ofe_standby(int do_magic);

/* Static/Libata Functions */
#ifdef CONFIG_PM
static int  vide_ofe_dev_suspend(struct device *dev, pm_message_t mesg);
static int  vide_ofe_suspend(struct platform_device *dev, pm_message_t mesg);
static int  vide_ofe_suspend_late(struct platform_device *dev, pm_message_t mesg);
static int  vide_ofe_resume_early(struct platform_device *dev);
static int  vide_ofe_dev_resume(struct device *dev);
static int  vide_ofe_resume(struct platform_device *dev);
#endif // CONFIG_PM
static int  vide_ofe_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val);
static int  vide_ofe_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val);
static int  vide_ofe_check_atapi_dma(struct ata_queued_cmd *qc);
static void vide_ofe_bmdma_setup(struct ata_queued_cmd *qc);
static void vide_ofe_bmdma_start(struct ata_queued_cmd *qc);
static u8   vide_ofe_bmdma_status(struct ata_port *ap);
static void vide_ofe_bmdma_stop(struct ata_queued_cmd *qc);
static void vide_ofe_bmdma_irq_clear(struct ata_port *ap);
static void vide_ofe_do_reset(void);
static void vide_ofe_clr_reset(void);
static void vide_ofe_dump(struct ata_port *ap, struct ata_taskfile *tf);

/* Character Device Functions */
static int  vide_ofe_open(struct inode *inode, struct file *filep);
static int  vide_ofe_close(struct inode *inode, struct file *filp);
static int  vide_ofe_sb_ioctl(struct inode *inode, struct file *filep, unsigned int cmd,
			      unsigned long arg);

static irqreturn_t vide_ofe_sb_interrupt(int irq, void *dev_instance);
static int         vide_ofe_reset_handler(void *data);

#endif /* CONFIG_BCM_VIDE_OFE */

#endif /* __VIDE_OFE_H__ */
