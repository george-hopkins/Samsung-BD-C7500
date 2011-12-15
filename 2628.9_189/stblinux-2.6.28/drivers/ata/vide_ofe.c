/*
 * Platform driver for Virtual IDE Optical Front End
 *
 * Copyright (C) 2009  Broadcom Corp
 *
 * Based on pata_pcmcia:
 *
 *   Copyright 2005-2006 Red Hat Inc <alan@redhat.com>, all rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <scsi/scsi_host.h>
#include <linux/ata.h>
#include <linux/libata.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/vide_ofe_sb.h>
#if defined VIDE_OFE_DEBUG
#include <linux/scatterlist.h>
#endif
#include "libata.h"
#include "vide_ofe.h"


/*
 * Externs
 */
extern struct mem_region *bmem_regions;

/*
 * Exported globals for mod params
 */
#ifdef VIDE_OFE_DEBUG
int ofe_debug		= 0;
EXPORT_SYMBOL(ofe_debug);
#endif

int ofe_debug_pio	= 0;
int ofe_show_cmds	= 0;
int ofe_boot_reset	= 0;
EXPORT_SYMBOL(ofe_debug_pio);
EXPORT_SYMBOL(ofe_show_cmds);
EXPORT_STMBOL(ofe_boot_reset);

#ifdef VIDE_OFE_DEBUG
#define VDPRINTK(fmt, args...) if (ofe_debug) printk(KERN_INFO "%s: " fmt, __func__, ## args)
#else
#define VDPRINTK(fmt, args...)
#endif

#define SPINDOWN_RETRIES	500


/*
 * Global variables
 */
static char my_device_name[]		= MODULE_NAME;
static int vide_ofe_open_count  	= 0;
static int dev_file_err			= -1;
static unsigned int reload_physaddr	= 0;
static unsigned int reload_windowsize	= 0;
static unsigned int reload_ru_alive	= 0;
static unsigned int scratch1		= 0;
static unsigned int fe_reset_requested	= 0;
static struct ata_host *gHost		= NULL;
static struct task_struct *rst_handler	= NULL;


/* For SB Protocol */
static int sb_in_progress		= 0;
static struct ata_host *vhost		= NULL;
static struct semaphore ioctl_sem;
static unsigned long vhost_lock_flags;


#ifdef CONFIG_PM
/*
 * VIDE_OFE Platform Device Power-Management
 */

/**
 * vide_ofe_dev_suspend -	Device-Level PM Suspend prep handler
 * @dev:			ptr to device
 * @mesg:			power mgmt message struct
 *
 **/
static int vide_ofe_dev_suspend(struct device *dev, pm_message_t mesg)
{
	struct ata_host *host        = NULL;
	struct ata_port *port        = NULL;
	struct ata_device *ata_dev   = NULL;
	int rc = 0;

	printk(KERN_DEBUG "%s: ENTER\n", __func__);

	if (dev) {
		host = (struct ata_host *)dev->driver_data;
		if (host) {
			port = host->ports[0];
			if (port)
				ata_dev = &port->link.device[0];
		}
	}
	else {
		printk(KERN_ERR "%s: NULL input device ptr, cannot perform power mgmt functions!\n",
		       __func__);
		return -ENODEV;
	}

	if (ata_dev == NULL) {
		printk(KERN_ERR "%s: NULL ata_dev ptr, cannot perform power mgmt functions!\n",
		       __func__);
		return -ENODEV;
	}
	else {
		struct ata_taskfile tf;
		u8 cdb[ATAPI_CDB_LEN];
		unsigned int err_mask;
		int retries;

		/*
		 * Close the tray.
		 * Do NOT issue in Immediate mode.
		 */
		retries = 6;
		do {
			memset(cdb, 0, sizeof(cdb));
			cdb[0] = START_STOP;
			cdb[4] = 0x3;

			ata_tf_init(ata_dev, &tf);
  
			tf.flags   |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE | ATA_TFLAG_POLLING;
			tf.command  = ATA_CMD_PACKET;
			tf.protocol = ATAPI_PROT_NODATA;

			err_mask = ata_exec_internal(ata_dev, &tf, cdb, DMA_NONE,
						     NULL, 0, 2000);
		} while (err_mask && --retries);

		if (!err_mask)
			printk(KERN_DEBUG "%s: Closed tray\n", __func__);

		/*
		 * Stop the disc
		 * Do NOT issue in Immediate mode.
		 * MUST BE DONE OR SLEEP COMMAND WILL FAIL !
		 */
		retries = SPINDOWN_RETRIES;
		do {
			memset(cdb, 0, sizeof(cdb));
			cdb[0] = START_STOP;

			ata_tf_init(ata_dev, &tf);
  
			tf.flags   |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE | ATA_TFLAG_POLLING;
			tf.command  = ATA_CMD_PACKET;
			tf.protocol = ATAPI_PROT_NODATA;

			err_mask = ata_exec_internal(ata_dev, &tf, cdb, DMA_NONE,
						     NULL, 0, 2000);

			if (err_mask) {

				u8 sense_buf[SCSI_SENSE_BUFFERSIZE];
				int ret;

				/*
				 * Figure out why the spin down failed.
				 * If No Media, then short-circuit out
				 * of this loop.
				 */
				memset(cdb, 0, sizeof(cdb));
				memset(sense_buf, 0, SCSI_SENSE_BUFFERSIZE);

				cdb[0] = REQUEST_SENSE;
				cdb[4] = SCSI_SENSE_BUFFERSIZE;

				/* initialize sense_buf with the error register,
				 * for the case where they are -not- overwritten
				 */
				ata_sff_tf_read(port, &tf);
				sense_buf[0] = 0x70;
				sense_buf[2] = tf.feature >> 4;;

				/* some devices time out if garbage left in tf */
				ata_tf_init(ata_dev, &tf);

				tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE | ATA_TFLAG_POLLING;
				tf.command = ATA_CMD_PACKET;

				tf.protocol = ATAPI_PROT_PIO;
				tf.lbam = SCSI_SENSE_BUFFERSIZE;
				tf.lbah = 0;

				ret = ata_exec_internal(ata_dev, &tf, cdb, DMA_FROM_DEVICE,
							sense_buf, SCSI_SENSE_BUFFERSIZE, 0);

				if (!ret) {
					/*
					 * Check raw sense data for
					 * "No Media" condition.
					 * No media - nothing to spin down.
					 */
					if ((sense_buf[2] & 0xf) == 0x02 &&
					    sense_buf[12] == 0x3a) {
						printk(KERN_DEBUG "%s: No media present\n", __func__);
						break;
					}
				}

				msleep(100);
			}
		} while (err_mask && --retries);

		if (!err_mask)
			printk(KERN_DEBUG "%s: Disc spun down in %d msec\n",
			       __func__, (SPINDOWN_RETRIES - retries) * 100);

		/*
		 * Gotta call ata_host_suspend() BEFORE putting firmware to sleep.
		 */
		rc = ata_host_suspend(host, mesg);
		if (!rc) {
			printk(KERN_DEBUG "%s: ata_host_suspend successful\n", __func__);

			/*
			 * Send ATA SLEEP command
			 *
			 *  - Can't go thru ata_exec_internal since we've
			 *    already called ata_host_suspend().
			 */
			ata_tf_init(ata_dev, &tf);

			tf.flags   |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE | ATA_TFLAG_POLLING;
			tf.command  = ATA_CMD_SLEEP;
			tf.protocol = ATA_PROT_NODATA;

			/*
			 * Must mask off OFE_IDE_CPU_INTR or else
			 * interrupts will spew after FE firmware
			 * executes the ATA SLEEP command.
			 */
			printk(KERN_DEBUG "%s: Disable OFE_IDE_CPU_INTR\n", __func__);

			*(volatile unsigned int *)REGADDR(BCHP_HIF_CPU_INTR1_INTR_W1_MASK_SET) = 
				BCHP_HIF_CPU_INTR1_INTR_W1_MASK_SET_OFE_IDE_CPU_INTR_MASK;

			printk(KERN_DEBUG "%s: Send SLEEP command to FE\n", __func__);

			port->ops->freeze(port);
			port->ops->sff_tf_load(port, &tf);
			port->ops->sff_exec_command(port, &tf);

			/*
			 * Wait for ARM "RUAlive" to go to zero
			 */
			retries = 50;
			while ((*(volatile int *)SB_ARM_SEMAPHORE != OFE_SB_INIT_STATE) &&
			       --retries)
				msleep(1);

			if (retries <= 0)
				printk(KERN_WARNING "%s: FAILED TO QUIESCE FE FIRMWARE !\n",
				       __func__);
			else
				printk(KERN_DEBUG "%s: FE firmware quiesced in %d msec\n",
				       __func__, (50 - retries));
		}
		else
			printk(KERN_ERR "\n%s: PM FAILURE - FAILED TO SUSPEND ATA HOST !\n\n",
			       __func__);
	}

	return (rc);
}


/**
 * vide_ofe_suspend -	Platform-level PM Suspend prep handler
 * @pdev:		ptr to platform device
 * @mesg:		power mgmt message struct
 *
 **/
static int vide_ofe_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct device *dev           = NULL;

	printk(KERN_DEBUG "%s: ENTER\n", __func__);

	if (pdev) {
		dev = &pdev->dev;
		return vide_ofe_dev_suspend(dev, mesg);
	}

	printk(KERN_ERR "%s: NULL input platform device ptr, cannot perform power mgmt functions!\n",
	       __func__);
	return -ENODEV;
}


/**
 * vide_ofe_suspend_late  -   PM Late Suspend handler
 * @pdev:		      ptr to platform device
 * @mesg:		      power mgmt message
 *
 **/
static int vide_ofe_suspend_late(struct platform_device *pdev, pm_message_t mesg)
{
	printk(KERN_DEBUG "%s: ENTER\n", __func__);
	return 0;
}


/**
 * vide_ofe_resume_early  -   PM Resume Early handler
 * @pdev:		      ptr to platform device
 *
 **/
static int vide_ofe_resume_early(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s: ENTER\n", __func__);
	return 0;
}

/**
 * vide_ofe_dev_resume - 	Device-level PM Resume handler
 * @pdev:			ptr to device
 *
 **/
static int vide_ofe_dev_resume(struct device *dev)
{
	struct ata_host *host = NULL;
	struct ata_port *port = NULL;
	int retries;

	printk(KERN_DEBUG "%s: ENTER\n", __func__);

	if (dev) {
		host = (struct ata_host *)dev->driver_data;
		if (host)
			port = host->ports[0];
	}
	else {
		printk(KERN_ERR "%s: NULL input platform device ptr, cannot perform power mgmt functions!\n",
		       __func__);
		return -ENODEV;
	}

	if (host == NULL) {
		printk(KERN_ERR "%s: NULL host ptr, cannot perform power mgmt functions!\n",
		       __func__);
		return -ENODEV;
	}

	/*
	 * Bring port back to life with a hard reset,
	 * - Necessary after going into Standby via the
	 *   ATA SLEEP command.
	 */
	vide_ofe_hardreset();

	if (port) {
		/*
		 * Wait for some time to allow BSY to
		 * be deasserted.
		 */
		retries = 1000;
		while (retries-- &&
		       (ioread8(port->ioaddr.altstatus_addr) & ATA_BUSY))
			msleep(1);
		if (retries <= 0)
			printk(KERN_WARNING "%s: CONTINUE WITH ATA_BUSY ASSERTED\n", __func__);
		else
			VDPRINTK("ATA_BSY deasserted in %d msec\n", (1000 - retries));
	}

	/*
	 * Re-enable the OFE_IDE_CPU interrupts.
	 */
	printk(KERN_DEBUG "%s: Re-enable OFE_IDE_CPU_INTR\n", __func__);

	*(volatile unsigned int *)REGADDR(BCHP_HIF_CPU_INTR1_INTR_W1_MASK_CLEAR) = 
		BCHP_HIF_CPU_INTR1_INTR_W1_MASK_SET_OFE_IDE_CPU_INTR_MASK;

	printk(KERN_DEBUG "%s: Call ata_host_resume\n", __func__);

	/*
	 * ata_host_resume() will spin the disc back up
	 */
	ata_host_resume(host);

	return 0;
}

/**
 * vide_ofe_resume -	Platform-level PM Resume handler
 * @pdev:		ptr to platform device
 *
 **/
static int vide_ofe_resume(struct platform_device *pdev)
{
	struct device *dev = NULL;

	printk(KERN_DEBUG "%s: ENTER\n", __func__);

	if (pdev) {
		dev = &pdev->dev;
		return vide_ofe_dev_resume(dev);
	}
	else {
		printk(KERN_ERR "%s: NULL input platform device ptr, cannot perform power mgmt functions!\n",
		       __func__);
		return -ENODEV;
	}
}

#endif // CONFIG_PM


/*****************************************************************************
 **                                                                         **
 **                        LIBATA CODE SECTION                              **
 **                                                                         **
 ****************************************************************************/

#if defined VIDE_OFE_DEBUG
static struct ata_queued_cmd *active_qc = NULL;
static unsigned long lock_flags = 0;
#endif

static int vide_ofe_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val)
{
	if (sc_reg > SCR_CONTROL)
		return -EINVAL;
	*val = readl(link->ap->ioaddr.scr_addr + (sc_reg * 4));
	return 0;
}


static int vide_ofe_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val)
{
	if (sc_reg > SCR_CONTROL)
		return -EINVAL;
	writel(val, link->ap->ioaddr.scr_addr + (sc_reg * 4));
	return 0;
}

static int vide_ofe_check_atapi_dma(struct ata_queued_cmd *qc)
{
	u8 cmd = qc->cdb[0];

	if (qc) {
		if (qc->ap->flags & VIDE_OFE_FLAG_NO_ATAPI_DMA)
			return -1;	/* ATAPI DMA not supported */
		else {
			switch (cmd) {
			case READ_10:
			case READ_12:
			case READ_16:
			case READ_CD:
			case WRITE_10:
			case WRITE_12:
			case WRITE_16:
				return 0;
			default:
				return -1;
			}
		}
	}
	return -1;
}


/**
 *	vide_ofe_bmdma_setup - Set up BMDMA transaction
 *	@qc: Info associated with this ATA transaction.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host lock)
 */
static void vide_ofe_bmdma_setup(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	unsigned int rw = (qc->tf.flags & ATA_TFLAG_WRITE);
	u32 dmactl;
	u8 stat;

	stat = ioread8(ap->ioaddr.altstatus_addr);
	BUG_ON(stat & ATA_BUSY);

	/* load PRD table addr. */
	mb();	/* make sure PRD table writes are visible to controller */
	iowrite32(ap->prd_dma, ap->ioaddr.bmdma_addr + ATA_DMA_TABLE_OFS);

	/* specify data direction, triple-check start bit is clear */
	dmactl = ioread32(ap->ioaddr.bmdma_addr);
	dmactl &= ~(BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_MASK);

	BUG_ON(dmactl & ATA_DMA_START);
	BUG_ON((dmactl >> 8) & (ATA_DMA_INTR | ATA_DMA_ACTIVE));

	/* Turn on VIDE_OFE_DEV0_DMA_ENABLE */
	dmactl |= VIDE_OFE_DEV0_DMA_ENABLE;

	if (!rw)
		dmactl |= ATA_DMA_WR;

	iowrite32(dmactl, ap->ioaddr.bmdma_addr);

	/* issue r/w command */
	ap->ops->sff_exec_command(ap, &qc->tf);
}

/**
 *	vide_ofe_bmdma_start - Start a BMDMA transaction
 *	@qc: Info associated with this ATA transaction.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host lock)
 */
static void vide_ofe_bmdma_start(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	u32 dmactl;

#if defined VIDE_OFE_DEBUG
	active_qc = qc;
#endif

	/* start host DMA transaction */
	dmactl  = ioread32(ap->ioaddr.bmdma_addr);
	dmactl &= ~(BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_MASK);

	dmactl |= ATA_DMA_START;

	iowrite8((dmactl & 0xf), ap->ioaddr.bmdma_addr);
}

/**
 *	vide_ofe_bmdma_status - Read BMDMA status
 *	@ap: Port associated with this ATA transaction.
 *
 *	Read and return BMDMA status register.
 *
 *	May be used as the bmdma_status() entry in ata_port_operations.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host lock)
 */
static u8 vide_ofe_bmdma_status(struct ata_port *ap)
{
	u32 dmareg;

	dmareg = ioread32(ap->ioaddr.bmdma_addr);
	dmareg &= ~(BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_MASK);

	return ((dmareg >> 8) & 0xff);
}

/**
 *	vide_ofe_bmdma_stop - Stop BMDMA transfer
 *	@qc: Command we are ending DMA for
 *
 *	Clears the ATA_DMA_START flag in the dma control register
 *
 *	May be used as the bmdma_stop() entry in ata_port_operations.
 *
 *	CANNOT SLEEP IN THIS ROUTINE !
 *
 *	LOCKING:
 *	spin_lock_irqsave(host lock)
 */
static void vide_ofe_bmdma_stop(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	u32 dmactl;
	u8  stat;

	/*
	 * clear start/stop bit
	 */
	dmactl = ioread32(ap->ioaddr.bmdma_addr);
	dmactl &= ~(BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_MASK);

	if (dmactl & ATA_DMA_START) {

		dmactl &= ~ATA_DMA_START;

		iowrite8((dmactl & 0xff), ap->ioaddr.bmdma_addr);

		/* one-PIO-cycle guaranteed wait, per spec, for HDMA1:0 transition */
		stat = ioread8(ap->ioaddr.altstatus_addr);

		if (stat & ATA_BUSY)
			printk(KERN_WARNING "%s: ATA_BUSY asserted after DMA_Start cleared\n",
			       __func__);
	}
}


/**
 *	vide_ofe_bmdma_irq_clear - Clear BMDMA interrupt.
 *	@ap: Port associated with this ATA transaction.
 *
 *	Clear interrupt and error flags in DMA status register.
 *
 *	May be used as the irq_clear() entry in ata_port_operations.
 *
 *	CANNOT SLEEP IN THIS ROUTINE !
 *
 *	LOCKING:
 *	spin_lock_irqsave(host lock)
 */
static void vide_ofe_bmdma_irq_clear(struct ata_port *ap)
{
	struct ata_taskfile tf;
	int dumped = 0;
	u8  stat;
	u32 dmactl;

	/*
	 * Clear ATA_DMA_INTR and the
	 * Dev0DMA enable.
	 */
	dmactl = ioread32(ap->ioaddr.bmdma_addr);
	dmactl &= ~(BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved0_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved1_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved2_MASK |
		    BCHP_VIDE_BUS_MASTER_CMD_STATUS_reserved3_MASK);

	if (dmactl & (ATA_DMA_ACTIVE << 8)) {

		printk(KERN_WARNING "%s: ATA_DMA_ACTIVE ASSERTED BEFORE DMA IRQ CLEARED !\n",
		       __func__);

		/*
		 * Crucial point - read all taskfile registers.
		 * Works better with FE firmware recovery - could
		 * be timing, could be the ATA IRQ clearing action
		 * from reading the Status register.
		 */
		ata_sff_tf_read(ap, &tf);
		if (tf.command & ATA_BUSY) {
			vide_ofe_dump(ap, &tf);
			dumped = 1;
		}
	}

	dmactl |= (ATA_DMA_INTR << 8);
	dmactl &= ~VIDE_OFE_DEV0_DMA_ENABLE;

	iowrite32(dmactl, ap->ioaddr.bmdma_addr);

	stat = ioread8(ap->ioaddr.altstatus_addr);

	if (stat & ATA_BUSY) {
		printk(KERN_WARNING "%s: ATA_BUSY ASSERTED AFTER DMA IRQ CLEARED, stat = 0x%02x !\n",
		       __func__, stat);
		if (!dumped)
			vide_ofe_dump(ap, &tf);
	}

#if defined VIDE_OFE_DEBUG
	active_qc = NULL;
#endif
}


/**
 *	vide_ofe_do_reset - Put the ARM and OFE into reset
 */
static void vide_ofe_do_reset(void)
{
	int delay_cnt = OFE_RESET_MSEC;
	unsigned int regstat;

	/*
	 * Implement the following equivalent
	 * in code
	 *
	 * setenv -p F8 "e -w -p 0x10280004 0x401f2;
	 *               e -w -p 0x10280004 0x401f3;
	 */
	VDPRINTK("ENTER\n");
	VDPRINTK(" - Current RUAlive: 0x%d\n",
		 *(volatile int *)SB_RU_ALIVE_ARM);
	VDPRINTK(" - Starting ARM Bridge Status: 0x%x\n",
		 *(volatile unsigned int *)ARM_BRIDGE_STATUS);

	/*
	 * Save crucial registers.
	 * - scratch_1: CFE stores the chip rev in
	 *              Scratch1 register (checked
	 *              during probe).
	 * - dram_base: Need correct firmare window
	 * - dram_size: parameters to restore around
	 *              FE reset.
	 */
	VDPRINTK("Save CFE chip rev, dram_base and dram_size\n");
	scratch1          = *(volatile int *)SB_ARM_SCRATCH_1;	
	reload_physaddr   = *(volatile unsigned int *)ARM_BRIDGE_DRAM_BASE;
	reload_windowsize = *(volatile unsigned int *)ARM_BRIDGE_DRAM_SIZE;
	reload_ru_alive   = *(volatile unsigned int *)SB_RU_ALIVE_MIPS;

	/*
	 * Place ARM and OFE into reset
	 *
	 *   *** MUST BE DONE TOGETHER ! ***
	 */
	VDPRINTK("Put ARM and OFE into reset, delay 1 msec\n");
	*(volatile unsigned int *)ARM_BRIDGE_CONTROL |= (ARM_SOFT_RESET | OFE_SOFT_RESET);
	mdelay(1);

	/*
	 * Clear the Are You Alive ARM register
	 */
	VDPRINTK("Clear ARM RUAlive register\n");
	*(volatile int *)SB_RU_ALIVE_ARM = 0;	

	/*
	 * Wait for OFE_RESET_DONE to assert
	 */
	while (delay_cnt-- >= 0) {
		regstat = *(volatile unsigned int *)ARM_BRIDGE_STATUS;
		if ((regstat & FE_RESET_DONE) == FE_RESET_DONE)
			break;
		mdelay(1);
	}

	if (delay_cnt <= 0)
		printk(KERN_ERR "%s: FE FAILED TO QUIESCE, ARM_BRIDGE_STATUS = 0x%08x\n",
		       __func__, regstat);
	else {
		VDPRINTK("OFE_RESET_DONE asserted in %d msec\n", (OFE_RESET_MSEC - delay_cnt));

		/*
		 * Turn off 67.5 MHz clock to OFE
		 */
		VDPRINTK("Turn off 67.5 MHz clock to FE\n");
		*(volatile unsigned int *)PM_CTRL_2  |= BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_67M_CLK_MASK;
		udelay(10);
	}

	VDPRINTK("\n");
}


/**
 *	vide_ofe_clr_reset - Take the ARM and OFE out of reset
 */
static void 	vide_ofe_clr_reset(void)
{
	int delay_cnt = OFE_RESET_MSEC;
	unsigned int regstat;
	int retries;
	struct ata_port *ap = NULL;

	/*
	 * Implement the following in code:
	 *
	 *               e -w -p 0x10280004 0x401f2 0x181 0xfd00000 0x300000;
	 *               e -w -p 0x10280078 1;
	 *               e -w -p 0x10280004 0x401f0"
	 */
	VDPRINTK("ENTER\n");

	if (gHost)
		ap = gHost->ports[0];

	/*
	 * Take the OFE out of reset
	 */
	VDPRINTK("Take OFE out of reset\n");
	*(volatile unsigned int *)ARM_BRIDGE_CONTROL &= ~OFE_SOFT_RESET;

	/*
	 * Turn on 67.5 MHz clock to OFE
	 */
	VDPRINTK("Turn on 67.5 MHz clock to FE\n");
	udelay(10);
	*(volatile unsigned int *)PM_CTRL_2  &= ~BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_67M_CLK_MASK;

	/*
	 * Wait for OFE_RESET_DONE to deassert
	 */
	while (delay_cnt-- >= 0) {
		regstat = *(volatile unsigned int *)ARM_BRIDGE_STATUS;
		if (!(regstat & BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_STS_OFE_RESET_DONE_MASK))
			break;
		mdelay(1);
	}

	if (delay_cnt <= 0)
		printk(KERN_ERR "%s: FE FAILED TO ACTIVATE, ARM_BRIDGE_STATUS = 0x%08x\n",
		       __func__, regstat);
	else
		VDPRINTK("OFE_RESET_DONE deasserted in %d msec\n", (OFE_RESET_MSEC - delay_cnt));

	/*
	 * Enable the required ARM control bits
	 */
	*(volatile unsigned int *)ARM_BRIDGE_ARM_CONTROL |= ARM_CTL_BITS;
	
	/*
	 * Re-install the FE firmware base address,
	 * ARM window size and Product Revision loaded
	 * byt CFE at boot time.
	 */
	VDPRINTK("Restore CFE chip rev, MIPS RUAlive, dram_base and dram_size\n");
	*(volatile int *)SB_ARM_SCRATCH_1 = scratch1;
	*(volatile unsigned int *)SB_RU_ALIVE_MIPS     = reload_ru_alive;
	*(volatile unsigned int *)ARM_BRIDGE_DRAM_BASE = reload_physaddr;
	*(volatile unsigned int *)ARM_BRIDGE_DRAM_SIZE = reload_windowsize;

	/*
	 * Clear all of memory beyond the FE firmware image itself.
	 * 
	 *   Note that the FE FW image is currently 1 MB in size, and is
	 *   loaded into physical memory at an aligned MB boundary.
	 */
	{
		struct page *mem_page = NULL;
		void        *mem_ptr  = NULL;
		int i;

		BUG_ON((reload_physaddr & 0xfffff) || (FE_IMG_SIZE & 0xfffff));

#ifdef CONFIG_BRCM_AR_CHECKER_V2
		if (ar_enabled(ARC_FE) == 1) {
			VDPRINTK("Disable FE Address Range Checker\n");
			if (disable_ar_check(ARC_FE))
				VDPRINTK("Failed to disable FE Address Range Checker\n");
		}
#endif
		VDPRINTK("Clear FE memory buffer region\n");

		if ((reload_physaddr + FE_IMG_SIZE) & 0xe0000000)
			mem_page = pfn_to_page(((unsigned long)(reload_physaddr + FE_IMG_SIZE)) >> PAGE_SHIFT);
		else
			mem_ptr = (void *)BCM_PHYS_TO_K0(reload_physaddr + FE_IMG_SIZE);

		for (i = 0; i < ((reload_windowsize - FE_IMG_SIZE) >> PAGE_SHIFT); i++) {
			if (mem_page)
				mem_ptr = kmap(mem_page);

			memset(mem_ptr, 0, PAGE_SIZE);

			flush_data_cache_page((unsigned long)mem_ptr);

			if (mem_page)
				kunmap(mem_page++);
			else
				mem_ptr += PAGE_SIZE;
		}

#ifdef CONFIG_BRCM_AR_CHECKER_V2
		if (ar_enabled(ARC_FE) == 1) {
			VDPRINTK("Re-enable FE Address Range Checker\n");
			if (reenable_ar_check(ARC_FE, 0))
				VDPRINTK("Failed to re-enable FE Address Range Checker\n");
		}
#endif
	}

	/*
	 * Deassert ARM_HALT_N
	 */
	VDPRINTK("Deassert ARM_HALT_N, delay 1 msec\n");
	*(volatile unsigned int *)ARM_BRIDGE_ARM_HALT |= ARM_HALT_DEASSERT;
	mdelay(1);

	VDPRINTK("Take ARM out of reset\n");
	*(volatile unsigned int *)ARM_BRIDGE_CONTROL &= ~ARM_SOFT_RESET;

	
	/*
	 * Wait for Firmware "Are You Alive"
	 */
	delay_cnt = OFE_RESET_MSEC;
	while (delay_cnt-- >= 0) {
		regstat = *(volatile unsigned int *)SB_RU_ALIVE_ARM;
		if (regstat == OFE_SB_READY)
			break;
		mdelay(1);
	}

	if (delay_cnt <= 0)
		printk(KERN_ERR "%s: FE ALIVE INDICATOR TIMED OUT!\n",
		       __func__);
	else {
		VDPRINTK("FE ALIVE in %d msec\n", (OFE_RESET_MSEC - delay_cnt));

		if (ap) {
			/*
			 * Wait for some time to allow BSY to
			 * be deasserted.
			 */
			retries = OFE_RESET_MSEC;
			while (retries-- &&
			       (ioread8(ap->ioaddr.altstatus_addr) & ATA_BUSY))
				mdelay(1);
			if (retries <= 0)
				printk(KERN_WARNING "%s: ATA_BUSY ASSERTED AFTER HARD RESET!\n", __func__);
			else
				VDPRINTK("ATA_BUSY deasserted in %d msec\n", (OFE_RESET_MSEC - retries));
		}
	}

	VDPRINTK("Final FE Bridge Status: 0x%08x\n",
		 *(volatile unsigned int *)ARM_BRIDGE_STATUS);
	VDPRINTK("Final FE Bridge Control: 0x%08x\n",
		 *(volatile unsigned int *)ARM_BRIDGE_CONTROL);
	VDPRINTK("Final ARM RUAlive: 0x%d\n",
		 *(volatile int *)SB_RU_ALIVE_ARM);

	VDPRINTK("\n");
}


/**
 *	vide_ofe_hardreset - Perform required steps to hard reset
 *                           the OFE and ARM.
 */
void vide_ofe_hardreset(void)
{
#if !defined (CONFIG_MIPS_BCM7635)
	/*
	 * Requires 7635 FE firmware version
	 * to work.
	 */
	vide_ofe_do_reset();
	vide_ofe_clr_reset();
#endif
}



/**
 *	vide_ofe_dump - Error dump after BMDMA interrupt.
 *	@ap: Port associated with this ATA transaction.
 *
 *	LOCKING:
 *	spin_lock_irqsave(host lock)
 */
static void vide_ofe_dump(struct ata_port *ap, struct ata_taskfile *tf)
{
#if defined VIDE_OFE_DEBUG
	struct ata_prd *prd;
	unsigned int idx, si;
	struct scatterlist *sg;
	char printbuf[128];
	char *printptr = &printbuf[0];
	int i;

	local_irq_save(lock_flags);

	/*
	 * Dump debug registers and ap->prd
	 */
	if (active_qc &&
	    active_qc->n_elem &&
	    (active_qc->flags & ATA_QCFLAG_ACTIVE)) {

		printk(KERN_INFO "\n Debug register dump:\n");
		printk(KERN_INFO "- BusMasterDbgPresAddr: 0x%08x\n", ioread32((void __iomem *)VIDE_BM_PRES_ADDR));
		printk(KERN_INFO "- BusMasterDbgXferSize: 0x%08x\n", ioread32((void __iomem *)VIDE_BM_XFER_SIZE));
		printk(KERN_INFO "- BusMasterDbgPrdPtr:   0x%08x\n", ioread32((void __iomem *)VIDE_BM_PRD_PTR));
		printk(KERN_INFO "- BusMasterDbgState:    0x%08x\n", ioread32((void __iomem *)VIDE_BM_STATE));

		/*
		 * Dump the ATA TaskFile Registers
		 */
		printk(KERN_INFO "\n TaskFile Registers dump:\n");
		printk(KERN_INFO " - status:      0x%02x\n", tf->command);
		printk(KERN_INFO " - error:       0x%02x\n", tf->feature);
		printk(KERN_INFO " - intr reason: 0x%02x\n", tf->nsect);
		printk(KERN_INFO " - lbal:        0x%02x\n", tf->lbal);
		printk(KERN_INFO " - lbam:        0x%02x\n", tf->lbam);
		printk(KERN_INFO " - lbah:        0x%02x\n", tf->lbah);
		printk(KERN_INFO " - device:      0x%02x\n", tf->device);
		printk(KERN_INFO " - control:     0x%02x\n", tf->ctl);

		printptr += sprintf(printptr, "\nCDB: 0x");
		for (i=0; i<12; i++)
			printptr += sprintf(printptr, "%02x ", active_qc->cdb[i]);
		sprintf(printptr, "\n");
		printk(KERN_INFO "%s", printbuf);

		/*
		 * Dump the SG list from the active qc
		 */
		printk(KERN_INFO "SG List from command:\n");

		idx = 0;
		for_each_sg(active_qc->sg, sg, active_qc->n_elem, si ) {
			u32 addr, offset;
			u32 sg_len, len;

			/* determine if physical DMA addr spans 64K boundary.
			 * Note h/w doesn't support 64-bit, so we unconditionally
			 * truncate dma_addr_t to u32.
			 */
			addr   = (u32) sg_dma_address(sg);
			sg_len = sg_dma_len(sg);

			while (sg_len) {
				offset = addr & 0xffff;
				len = sg_len;
				if ((offset + sg_len) > 0x10000)
					len = 0x10000 - offset;

				ap->prd[idx].addr = cpu_to_le32(addr);
				ap->prd[idx].flags_len = cpu_to_le32(len & 0xffff);
				printk(KERN_INFO "PRD[%u] = (0x%X, 0x%X)\n", idx, addr, len);

				idx++;
				sg_len -= len;
				addr += len;
			}
		}

		/*
		 * Dump the SG List that the Port HW sees
		 */
		if (idx) {
			prd = ap->prd;
			printk(KERN_INFO "--------------------------------\n");
			printk(KERN_INFO "PRD_DMA = 0x%08x\n", ap->prd_dma);
			printk(KERN_INFO "PRD List From Port @0x%p:\n", prd);

			do {
				printk(KERN_INFO "---> Addr     = 0x%08x\n", prd->addr);
				printk(KERN_INFO "---> Len/flag = 0x%08x\n", prd->flags_len);
			} while (--idx && prd++);
			printk(KERN_INFO "--------------------------------\n");
		}
	}
	local_irq_restore(lock_flags);
#else
	return;
#endif
}


static struct scsi_host_template vide_ofe_sht = {
	.module			= THIS_MODULE,
	.name			= DRIVER_NAME,
	.ioctl			= ata_scsi_ioctl,
	.queuecommand		= ata_scsi_queuecmd,
	.can_queue		= ATA_DEF_QUEUE,
	.this_id		= ATA_SHT_THIS_ID,
	.sg_tablesize		= LIBATA_MAX_PRD,
	.cmd_per_lun		= ATA_SHT_CMD_PER_LUN,
	.emulated		= ATA_SHT_EMULATED,
	.use_clustering		= ATA_SHT_USE_CLUSTERING,
	.proc_name		= DRIVER_NAME,
	.dma_boundary		= ATA_DMA_BOUNDARY,
	.slave_configure	= ata_scsi_slave_config,
	.slave_destroy		= ata_scsi_slave_destroy,
	.bios_param		= ata_std_bios_param,
};


static const struct ata_port_operations vide_ofe_port_ops = {
	.inherits		= &ata_bmdma_port_ops,

	.sff_irq_clear		= vide_ofe_bmdma_irq_clear,
	.check_atapi_dma	= vide_ofe_check_atapi_dma,
	.bmdma_setup		= vide_ofe_bmdma_setup,
	.bmdma_start		= vide_ofe_bmdma_start,
	.bmdma_stop		= vide_ofe_bmdma_stop,
	.bmdma_status		= vide_ofe_bmdma_status,
	.scr_read		= vide_ofe_scr_read,
	.scr_write		= vide_ofe_scr_write,
};

static void vide_ofe_setup_port(struct ata_ioports *ioaddr)
{
	ioaddr->data_addr	= ioaddr->cmd_addr + ATA_REG_DATA;
	ioaddr->error_addr	= ioaddr->cmd_addr + ATA_REG_ERR;
	ioaddr->feature_addr	= ioaddr->cmd_addr + ATA_REG_FEATURE;
	ioaddr->nsect_addr	= ioaddr->cmd_addr + ATA_REG_NSECT;
	ioaddr->lbal_addr	= ioaddr->cmd_addr + ATA_REG_LBAL;
	ioaddr->lbam_addr	= ioaddr->cmd_addr + ATA_REG_LBAM;
	ioaddr->lbah_addr	= ioaddr->cmd_addr + ATA_REG_LBAH;
	ioaddr->device_addr	= ioaddr->cmd_addr + ATA_REG_DEVICE;
	ioaddr->status_addr	= ioaddr->cmd_addr + ATA_REG_STATUS;
	ioaddr->command_addr	= ioaddr->cmd_addr + ATA_REG_CMD;
	ioaddr->altstatus_addr	= 
	ioaddr->ctl_addr	= ioaddr->cmd_addr + VIDE_OFE_ALTSTATUS_OFFSET;
	ioaddr->bmdma_addr	= ioaddr->cmd_addr + VIDE_OFE_BUS_MASTER_CMD_STATUS;
	ioaddr->scr_addr	= ioaddr->cmd_addr + VIDE_OFE_SCR_OFFSET;
	ioaddr->simr_addr	= (void __iomem *)NULL;
}


static const struct ata_port_info vide_ofe_port_info[] = {
	{
		.flags		= ATA_FLAG_SATA | ATA_FLAG_NO_LEGACY,
		.pio_mask	= 0x1f,
		.mwdma_mask	= 0x07,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &vide_ofe_port_ops,
	},
};


/*****************************************************************************
 **                                                                         **
 **        CHARACTER DEVICE CODE SECTION FOR SIDE BAND PROTOCOL             **
 **                                                                         **
 ****************************************************************************/

/*
 * The File Operations structure for the serial/ioctl interface of the driver
 */
static const struct file_operations vide_ofe_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= vide_ofe_sb_ioctl,
	.open		= vide_ofe_open,
	.release	= vide_ofe_close,
};

/*
 *
 * Device Open for SB Protocol
 *
 */
static int vide_ofe_open(struct inode *inode, struct file *filep)
{
	int ret = 0;

	/*
	 * Only allow superuser to access private ioctl interface
	 */
	if (!capable(CAP_SYS_ADMIN)) {
		ret = -EACCES;
		goto out;
	}

	/* only 1 open at a time */
	if (vide_ofe_open_count) {
		VDPRINTK("Already Open - count = %d\n", vide_ofe_open_count);
		ret = -EBUSY;
		goto out;
	}

	VDPRINTK("successful open\n");

	vide_ofe_open_count = 1;
      
out:
	return (ret);
}

/*
 *
 * Device driver close
 *
 */
static int vide_ofe_close(struct inode *inode, struct file *filp)
{
	if (vide_ofe_open_count == 0)
		VDPRINTK("Already closed ? Open count = %d\n", vide_ofe_open_count);

	/*
	 * Signal ARM that SB Protocol is no longer available
	 */
	do {
		*(volatile int *)SB_RU_ALIVE_MIPS = OFE_SB_INIT_STATE;
	} while (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_INIT_STATE);
	do {
		*(volatile int *)SB_RU_ALIVE_MIPS = OFE_SB_KERNEL_ALIVE;
	} while (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_KERNEL_ALIVE);
	
	/* Always successful */
	vide_ofe_open_count = 0;		/* only one open per device */

	VDPRINTK("successful close\n");

	return 0;
}


/**
 *	vide_ofe_sb_interrupt - Interrupt Handler for Side Band
 *                              Protocol.
 *
 *	@irq: irq line (unused)
 *	@dev_instance: pointer to our ata_host information structure
 *
 *	LOCKING:
 *	Obtains host lock during operation.
 *
 *	RETURNS:
 *	IRQ_NONE or IRQ_HANDLED.
 */
static irqreturn_t vide_ofe_sb_interrupt(int irq, void *dev_instance)
{
	unsigned int i;
	unsigned int handled = 1;
	unsigned int mbx;
	int valid;
#if defined VIDE_OFE_DEBUG
	static int spurious = 0;
#endif

	/*
	 * Clear interrupt status
	 */
	*(volatile unsigned int *)MB_INTR2_CPU_INTR_CLEAR = MIPS_MAILBOX_MASK;

	/*
	 * Interrupts from the FE while a previous SB
	 * command is in progress will be ignored. SB
	 * Protocol is single-threaded.
	 */
	if (sb_in_progress) {
		printk(KERN_WARNING "%s: FE INTERRUPT WHILE SB IN PROGRESS!\n",
		       __func__);
	}
	else {
		vhost = dev_instance;

		/* Lock out libata */
		spin_lock_irqsave(&vhost->lock, vhost_lock_flags);

		for (i = 0; i < vhost->n_ports; i++) {
			struct ata_port *ap;

			ap = vhost->ports[i];

			if (ap && !(ap->flags & ATA_FLAG_DISABLED)) {
				/*
				 * Check for SB command
				 */
				if (*(volatile int *)SB_ARM_SEMAPHORE != OFE_SB_IN_PROGRESS) {
					VDPRINTK("Port %d: SPURIOUS INTERRUPT (count = %d)\n",
					       i, ++spurious);
					continue;
				}

				if (*(volatile unsigned int *)SB_MIPS_SEMAPHORE != 0) {
					printk(KERN_WARNING "%s: SB PROTOCOL VIOLATION - Mips Semaphore Non-Zero (clearing)\n",
					       __func__);
					/*
					 * Unlock the semaphore
					 */
					*(volatile unsigned int *)SB_MIPS_SEMAPHORE = 0;
				}

				/*
				 * Peek at the mailbox register to see if this is
				 * a valid SB command. If this is a reset request
				 * directly from the FE, the SB Protocol is
				 * short-circuited.
				 */
				valid = 1;

				mbx = *(volatile unsigned int *)SB_MIPS_MAILBOX;

				switch((mbx >> 24 ) & 0xff) {
				default:
					printk(KERN_WARNING "%s: INVALID SB OPCODE 0x%x\n",
					       __func__, mbx >> 24);
					valid = 0;
				case SB_FLASH_ACCESS:
					break;
				case SB_FE_RESET:
					/* All other bytes MBZ. */
					if (mbx == (SB_FE_RESET << 24)) {
						VDPRINTK("***** Port %d: FE Reset requested *****\n\n", i);
						fe_reset_requested = 1;
						wake_up_process(rst_handler);
					}
					break;

				}

				if (!valid || fe_reset_requested)
					break;

				switch((mbx >> 16 ) & 0xff) {
				default:
					printk(KERN_WARNING "%s: INVALID SB FLASH OPERATION 0x%x\n",
					       __func__, mbx >> 16);
					valid = 0;
				case SB_READ:
				case SB_WRITE:
				case SB_REBOOT:
					break;
				}

				if (!valid)
					break;

				/*
				 * Signal that a SB command has been received
				 */
				VDPRINTK("***** Side Band interrupt received, set sb_in_progress *****\n\n");
				sb_in_progress = 1;
				up(&ioctl_sem);
				if (ar_enabled(ARC_FE) == 1) {
					VDPRINTK("Disable FE Address Range Checker\n");
					if (disable_ar_check(ARC_FE))
						VDPRINTK("Failed to disable FE Address Range Checker\n");
				}
			}
		}
		spin_unlock_irqrestore(&vhost->lock, vhost_lock_flags);
	}
	return IRQ_RETVAL(handled);
}


/**
 * vide_ofe_reset_handler - FE Reset Handler Thread
 */
static int vide_ofe_reset_handler(void *data)
{
	/*
	 * We use TASK_INTERRUPTIBLE so that the thread is not
	 * counted against the load average as a running process.
	 * We never actually get interrupted because kthread_run
	 * disables signal delivery for the created thread.
	 */
	set_current_state(TASK_INTERRUPTIBLE);

	while (!kthread_should_stop()) {

		if (!fe_reset_requested) {
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);
			continue;
		}

		/* FE Reset requested */
		__set_current_state(TASK_RUNNING);
		vide_ofe_hardreset();
		fe_reset_requested = 0;
		set_current_state(TASK_INTERRUPTIBLE);
	}

	__set_current_state(TASK_RUNNING);

	VDPRINTK("Reset Handler exiting\n");
	rst_handler = NULL;

	return 0;
}


/**
 *	vide_ofe_sb_ioctl - Ioctl interfaces for Side Band Protocol
 *
 *	@inode: Our device inode
 *	@filep: Unused
 *	@cmd:	Ioctl command
 *	@arg:	User Buffer
 *
 *	LOCKING:
 *	Releases host lock after call for SB_DONE command.
 *
 *	RETURNS: 0 on success, negative error code on error.
 */
static int  vide_ofe_sb_ioctl(struct inode *inode, struct file *filep,
			   unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int rc = -EINVAL, size;
	unsigned int sb_stat;
	static unsigned int scratch_1, scratch_2, scratch_3, scratch_4;
	static unsigned int sdram_physaddr;


	if (_IOC_TYPE(cmd) != SB_IOC_MAGIC) {
		printk(KERN_ERR "%s: IOC_TYPE MISMATCH (req = 0x%02x, SB IOC = 0x%02x\n",
		       my_device_name, _IOC_TYPE(cmd), SB_IOC_MAGIC);
		goto out;
	}
	if (_IOC_NR(cmd) > SB_IOC_MAXNR) {
		printk(KERN_ERR "%s: IOC_NR MISMATCH  (req = 0x%02x, SB IOC = 0x%02x\n",
		       my_device_name, _IOC_TYPE(cmd), SB_IOC_MAGIC);
		goto out;
	}

	size = _IOC_SIZE(cmd);
	rc = -EFAULT;
	VDPRINTK("iocdir=%.4x iocr=%.4x iocw=%.4x iocsize=%d cmd=%.4x\n",
	       _IOC_DIR(cmd), _IOC_READ, _IOC_WRITE, size, cmd);

	if (_IOC_DIR(cmd) & _IOC_READ) {
		if (!access_ok(VERIFY_WRITE, argp, size))
			goto out;
	}
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (!access_ok(VERIFY_READ, argp, size))
			goto out;
	}

	/* Init rc to success */
	rc = 0;

	switch (cmd) {
	case SB_IOCTL_GETCMD:
	{
		unsigned int sb_op;
		unsigned int sb_flash_idx;
		sb_command_t sb_cmd;
		unsigned int mbx;

		/*
		 * This ioctl is called by the user-level code
		 * to wait for a Side Band command from the
		 * ARM. The ioctl call is blocking, therefore
		 * we cannot actually return until we have
		 * received a SB command from the ARM. There
		 * are many cases where this will never happen.
		 * We will be signalled by vide_ofe_sb_interrupt()
		 * when the MIPS mailbox register is written by
		 * the FE.
		 */
		VDPRINTK("SB_IOCTL_GETCMD - Enter\n");

		if (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_READY) {
			/*
			 * Signal ARM that SB Protocol is Alive
			 */
			do {
				*(volatile int *)SB_RU_ALIVE_MIPS = OFE_SB_INIT_STATE;
			} while (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_INIT_STATE);
			do {
				*(volatile int *)SB_RU_ALIVE_MIPS = OFE_SB_READY;
			} while (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_READY);
		}

	snooze:
		VDPRINTK("SB_IOCTL_GETCMD - sleeping\n\n");
		if (down_interruptible(&ioctl_sem) < 0) {
			if (!sb_in_progress) {
				printk(KERN_WARNING "%s: SB_IOCTL_GETCMD - Interrupted!\n",
				       __func__);
			}
			rc = -EINTR;
			goto out;
		}

		VDPRINTK("SB_IOCTL_GETCMD - Awakened!\n");

		if (!sb_in_progress) {
			/*
			 * Huh ? Got an interrupt without a SB command ?
			 */
			printk(KERN_WARNING "%s: SB_IOCTL_GETCMD - Invalid SB interrupt\n",
			       __func__);
			rc = -EFAULT;
			goto out;
		}

		/*
		 * Lock out libata
		 */
		spin_lock_irqsave(&vhost->lock, vhost_lock_flags);

		/*
		 * Signal FE that the command is in progress.
		 */
		*(volatile unsigned int *)SB_MIPS_SEMAPHORE = 3;

		/*
		 * Side Band command received
		 */
		mbx       = *(volatile unsigned int *)SB_MIPS_MAILBOX;
		scratch_1 = *(volatile unsigned int *)SB_ARM_SCRATCH_1;
		scratch_2 = *(volatile unsigned int *)SB_ARM_SCRATCH_2;
		scratch_3 = *(volatile unsigned int *)SB_ARM_SCRATCH_3;
		scratch_4 = *(volatile unsigned int *)SB_ARM_SCRATCH_4;

		sb_op          = (mbx >> 16 ) & 0xff;
		sb_flash_idx   = mbx & 0xff;
		sdram_physaddr = scratch_3 + *(volatile unsigned int *)ARM_BRIDGE_DRAM_BASE;

		sb_cmd.cmd             = sb_op;
		sb_cmd.idx             = sb_flash_idx;
		sb_cmd.flash_offset    = scratch_1;
		sb_cmd.data_size       = scratch_2;
		sb_cmd.sdram_physaddr  = sdram_physaddr;
		sb_cmd.checksum        = scratch_4;

		VDPRINTK("SB Command:\n");
		VDPRINTK(" - Cmd Code:         %d\n", sb_cmd.cmd);
		VDPRINTK(" - Flash Idx:        %d\n", sb_cmd.idx);
		VDPRINTK(" - Flash Offset:     0x%08x\n", sb_cmd.flash_offset);
		VDPRINTK(" - Data Size:        0x%08x\n", sb_cmd.data_size);
		VDPRINTK(" - SDRAM Phys Addr:  0x%08x\n", sb_cmd.sdram_physaddr);
		VDPRINTK(" - Checksum:         0x%08x\n", sb_cmd.checksum);
#if defined (SB_FROM_TCM)
		VDPRINTK(" - Reload Phys Addr: 0x%08x\n", sb_cmd.reload_physaddr);
#endif

		/*
		 * Before sending back up to user level, perform a
		 * gross check on the command Dword. If things
		 * look out of whack, don't bother with passing
		 * the command up to user space.
		 */
		if ((sb_op != SB_READ && sb_op != SB_WRITE) ||
		    (sb_flash_idx != SB_OFEFW_PART && sb_flash_idx != SB_OFEWS_PART) ||
		    ((mbx >> 24) & 0xff) != 0xf1) {
			/*
			 * Invalid Command DWord.
			 */
			printk(KERN_WARNING "%s: SB_IOCTL_GETCMD - Invalid Command DWord 0x%08x\n",
			       __func__, mbx);
			*(volatile unsigned int *)SB_MIPS_SCRATCH_1 = scratch_1;
			*(volatile unsigned int *)SB_MIPS_SCRATCH_2 = scratch_2;
			*(volatile unsigned int *)SB_MIPS_SCRATCH_3 = scratch_3;
			*(volatile unsigned int *)SB_MIPS_SCRATCH_4 = 0;

			/* Set up the Status Dword */
			sb_stat = (SB_FAIL << 24 |
				   SB_ERR_INVALID_COMMAND << 16 |
				   4 << 8 |
				   sb_flash_idx);

			/* Clear ARM Semaphore Register 1 */
			*(volatile int *)SB_ARM_SEMAPHORE = 0;

			/*
			 * Write the SB Status Dword to the ARM Mailbox
			 * register. This write will interrupt the ARM.
			 * The ARM should clear the SB_MIPS_SEMAPHORE
			 * register to release it as an ack.
			 */
			*(volatile unsigned int *)SB_ARM_MAILBOX = sb_stat;

			/*
			 * Clear our in-progress flag.
			 */
			VDPRINTK("Clear sb_in_progress\n");
			sb_in_progress = 0;
			if (ar_enabled(ARC_FE) == 1) {
				VDPRINTK("Re-enable FE Address Range Checker\n");
				if (reenable_ar_check(ARC_FE, 0))
					VDPRINTK("Failed to re-enable FE Address Range Checker\n");
			}

			/*
			 * Release the host lock for libata.
			 */
			spin_unlock_irqrestore(&vhost->lock, vhost_lock_flags);

			goto snooze;
		}

		/*
		 * If the command is a SB write of the OFE FW image,
		 * record the ARM SDRAM Memory Window Base Address
		 * and size for the reload phase.
		 */
		if (sb_op == SB_WRITE && sb_flash_idx == SB_OFEFW_PART) {
			reload_physaddr   = *(volatile unsigned int *)ARM_BRIDGE_DRAM_BASE;
			reload_windowsize = *(volatile unsigned int *)ARM_BRIDGE_DRAM_SIZE;
		}

		/*
		 * Release the host lock for libata.
		 * Must release lock before copy_to_user (might sleep).
		 */
		spin_unlock_irqrestore(&vhost->lock, vhost_lock_flags);

		/*
		 * Copy the cmd & parameters to user space
		 * and return to user-level code.
		 */
		if (copy_to_user(argp, &sb_cmd, sizeof(sb_command_t))) {
			rc = -EFAULT;
			break;
		}

		/*
		 * Zero MIPS Scratch Registers
		 */
		*(volatile unsigned int *)SB_MIPS_SCRATCH_1 = 0;
		*(volatile unsigned int *)SB_MIPS_SCRATCH_2 = 0;
		*(volatile unsigned int *)SB_MIPS_SCRATCH_3 = 0;
		*(volatile unsigned int *)SB_MIPS_SCRATCH_4 = 0;

		VDPRINTK("SB_IOCTL_GETCMD - Exit\n\n");

		break;
	}
	case SB_IOCTL_STATUS:
	{
		/*
		 * This ioctl is called by user-level code when
		 * the requested flash operation has been
		 * completed. User-level code passes back
		 * status in the sb_response_t structure.
		 */
		sb_response_t sb_rsp;

		if (argp == NULL  ||
		    vhost == NULL ||
		    !sb_in_progress) {
			printk(KERN_WARNING "%s: SB_IOCTL_STATUS - Invalid response parameters\n",
			       __func__);
			rc = -EINVAL;
			break;
		}

		VDPRINTK("SB_IOCTL_STATUS - Enter\n");

		/*
		 * May block - cannot hold lock.
		 */
		if (copy_from_user(&sb_rsp, argp, sizeof(sb_response_t))) {
			rc = -EFAULT;
			break;
		}

		if (sb_rsp.cmd != SB_REBOOT) {

			int retries = 100;

			/*
			 * Lock out libata
			 */
			spin_lock_irqsave(&vhost->lock, vhost_lock_flags);

			/*
			 * Copy response arg's to ARM bridge as per
			 * Side Band protocol.
			 */
			*(volatile unsigned int *)SB_MIPS_SCRATCH_1 = scratch_1;
			*(volatile unsigned int *)SB_MIPS_SCRATCH_2 = scratch_2;
			*(volatile unsigned int *)SB_MIPS_SCRATCH_3 = scratch_3;

			if (sb_rsp.cmd == SB_READ)
				*(volatile unsigned int *)SB_MIPS_SCRATCH_4 = sb_rsp.checksum;
			else
				*(volatile unsigned int *)SB_MIPS_SCRATCH_4 = 0;

			/* Set up the Status Dword */
			sb_stat = sb_rsp.status << 24 |
				sb_rsp.errcode << 16 |
				4 << 8 |
				sb_rsp.idx;

			/* Clear ARM Semaphore Register 1 */
			*(volatile int *)SB_ARM_SEMAPHORE = 0;

			/*
			 * Write the SB Status Dword to the ARM Mailbox
			 * register. This write will interrupt the ARM.
			 * The ARM should clear the SB_MIPS_SEMAPHORE
			 * register to release it as an ack.
			 */
			VDPRINTK("SB_IOCTL_STATUS - Writing ARM Mailbox with 0x%08x\n", sb_stat);
			*(volatile unsigned int *)SB_ARM_MAILBOX = sb_stat;

			/*
			 * Wait for the MIPS Semaphore register to be
			 * cleared by the ARM, which acks the status
			 * receipt.
			 */
			while (--retries && *(volatile unsigned int *)SB_MIPS_SEMAPHORE != 0) {
				/* Can't sleep - we hold a lock */
				mdelay(10);
				VDPRINTK("SB_IOCTL_STATUS - Wait for ARM to clear MIPS semaphore\n");
			}

			if (!retries)
				printk(KERN_WARNING "%s: SB PROTOCOL VIOLATION - ARM never cleared MIPS Semaphore.\n",
				       __func__);

#if defined (SB_FROM_TCM)
			/*
			 * If Side Band Protocol code in the FE is running in the TCM,
			 * then user-level code will already have loaded the new
			 * firmware image into system memory. We simply have to reset
			 * the ARM and take it out of reset to restart the firmware.
			 */
			VDPRINTK("Reset the FE\n");
			vide_ofe_hard_reset();

			/*
			 * Clear our in-progress flag.
			 */
			VDPRINTK("Clear sb_in_progress\n");
			sb_in_progress = 0;
			if (ar_enabled(ARC_FE) == 1) {
				VDPRINTK("Re-enable FE Address Range Checker\n");
				if (reenable_ar_check(ARC_FE, 0))
					VDPRINTK("Failed to re-enable FE Address Range Checker\n");
			}
#else
			/*
			 * Clear our in-progress flag if we are done.
			 * If this was a Write to the OFEFW partition,
			 * we are not done until after the reload.
			 */
			if (sb_rsp.cmd != SB_WRITE ||
			    sb_rsp.idx != SB_OFEFW_PART) {
				VDPRINTK("Clear sb_in_progress\n");
				sb_in_progress = 0;
				if (ar_enabled(ARC_FE) == 1) {
					VDPRINTK("Re-enable FE Address Range Checker\n");
					if (reenable_ar_check(ARC_FE, 0))
						VDPRINTK("Failed to re-enable FE Address Range Checker\n");
				}
			}
#endif
			/*
			 * Release the host lock for libata.
			 */
			spin_unlock_irqrestore(&vhost->lock, vhost_lock_flags);

		}
		else {
			struct page *reload_page = NULL, *src_page = NULL;
			void        *reload_ptr  = NULL, *src_ptr  = NULL;
			int i, j;

			/*
			 * SB_IOCTL_STATUS after SB_REBOOT command
			 */
			if (sb_rsp.status == SB_SUCCESS) {

				int failed = 0;

				if (sdram_physaddr != reload_physaddr) {
					/*
					 * Sanity check src/reload images
					 */
					VDPRINTK("Checking relocated image against source data\n");

					if (sdram_physaddr & 0xe0000000)
						src_page = pfn_to_page((unsigned long)sdram_physaddr >> PAGE_SHIFT);
					else
						src_ptr = (void *)BCM_PHYS_TO_K0(sdram_physaddr);

					if (reload_physaddr & 0xe0000000)
						reload_page = pfn_to_page((unsigned long)reload_physaddr >> PAGE_SHIFT);
					else
						reload_ptr = (void *)BCM_PHYS_TO_K0(reload_physaddr);

					for (i = 0; i < (FE_IMG_SIZE >> PAGE_SHIFT); i++) {
						if (reload_page)
							reload_ptr = kmap(reload_page);
						if (src_page)
							src_ptr = kmap(src_page);

						for (j = 0; j < (PAGE_SIZE >> 2); j += sizeof(unsigned int)) {
							if (*(unsigned int *)(src_ptr + j) != *(unsigned int *)(reload_ptr + j)) {
								printk(KERN_WARNING "%s: IMAGE MISCOMPARE: Page %d, Offset %d, "
								       "SRC 0x%08x, RELOAD 0x%08x\n",
								       __func__, i, j * sizeof(unsigned int),
								       *(unsigned int *)(src_ptr + j),
								       *(unsigned int *)(reload_ptr + j));
								failed = 1;
							}
							if (i == ((FE_IMG_SIZE >> PAGE_SHIFT) - 1) &&
							    j == ((PAGE_SIZE - 16) / sizeof(unsigned int))) {
								char pidbuf[20];
								memcpy (pidbuf, reload_ptr + (j * sizeof(unsigned int)), 16);
								pidbuf[16] = '\0';

								VDPRINTK("PID = %s\n", pidbuf);
							}
						}
						if (src_page)
							kunmap(src_page++);
						else
							src_ptr += PAGE_SIZE;
						if (reload_page)
							kunmap(reload_page++);
						else
							reload_ptr += PAGE_SIZE;
					}
				}
				else {
					/*
					 * Be sure the dma cache is evicted
					 * before re-starting the Address Range
					 * Checker.
					 */
					if (reload_physaddr & 0xe0000000) {
						reload_page = pfn_to_page((unsigned long)reload_physaddr >> PAGE_SHIFT);
						reload_ptr  = kmap(reload_page);
					}
					else
						reload_ptr = (void *)BCM_PHYS_TO_K0(reload_physaddr);

					VDPRINTK("Flush cache at VA 0x%p Size 0x%08x\n", reload_ptr, FE_IMG_SIZE);

					dma_cache_wback_inv((unsigned long)reload_ptr, FE_IMG_SIZE);

					if (reload_page)
						kunmap(reload_page);
				}

				
				/*
				 * Successful reload/reboot - release the ARM.
				 */
				if (!failed) {
					VDPRINTK("Take ARM out of Soft Reset\n");
					*(volatile unsigned int *)ARM_BRIDGE_DRAM_BASE = reload_physaddr;
					*(volatile unsigned int *)ARM_BRIDGE_DRAM_SIZE = reload_windowsize;
					vide_ofe_clr_reset();
				}
				else
					printk(KERN_WARNING "%s: FE FIRMWARE RELOAD DATA CORRUPTION!\n",
					       __func__);
			}
			else
				printk(KERN_WARNING "%s: SB_IOCTL_STATUS - SB_REBOOT FAILURE!\n",
				       __func__);

			VDPRINTK("Clear sb_in_progress\n");
			sb_in_progress = 0;
			if (ar_enabled(ARC_FE) == 1) {
				VDPRINTK("Re-enable FE Address Range Checker\n");
				if (reenable_ar_check(ARC_FE, 0))
					VDPRINTK("Failed to re-enable FE Address Range Checker\n");
			}
		}

		VDPRINTK("SB_IOCTL_STATUS - Done\n\n");

		break;
	}
#if !defined (SB_FROM_TCM)
	case SB_IOCTL_RELOAD:
	{
		fe_reboot_t fe_reboot;

		VDPRINTK("SB_IOCTL_RELOAD - Enter\n");

		/*
		 * Lock out libata
		 */
		spin_lock_irqsave(&vhost->lock, vhost_lock_flags);

		/*
		 * First reset the ARM
		 */
		vide_ofe_do_reset();

		/*
		 * Load the re_reboot_t for the caller
		 */
		fe_reboot.cmd      = SB_REBOOT;
		fe_reboot.physaddr = reload_physaddr;

		/*
		 * Release the host lock for libata.
		 */
		spin_unlock_irqrestore(&vhost->lock, vhost_lock_flags);

		/*
		 * Copy the parameters to user space
		 * and return to user-level code.
		 */
		if (copy_to_user(argp, &fe_reboot, sizeof(fe_reboot_t))) {
			rc = -EFAULT;
			break;
		}

		VDPRINTK("SB_IOCTL_RELOAD - Exit\n\n");

		break;
	}
#endif
	default:
		printk(KERN_ERR "%s: UNSUPPORTED IOCTL CMD 0x%x\n",
		       my_device_name, cmd);
		rc = -ENOTTY;
		break;
	}
out:
	return rc;
}



/*****************************************************************************
 **                                                                         **
 **                 COMMON DRIVER INIT/PROBE/EXIT SECTIONS                  **
 **                                                                         **
 ****************************************************************************/

/**
 *	vide_ofe_probe		-	attach a platform interface
 *	@pdev: platform device
 *
 *	Register a platform bus VIDE interface.
 */
static int __devinit vide_ofe_probe(struct platform_device *pdev)
{
	const struct ata_port_info *ppi[] =
		{ &vide_ofe_port_info[0], NULL };
	struct ata_host *host;
	struct ata_port *ap;
	unsigned int chip_rev, arm_state, sb_alive;
	unsigned int load_addr, load_size, dst_addr;
	int i, rc, irq, n_ports = 1, last_idx = 0;
	int retries;
	int reloaded = 0;


	/*
	 * Better be a 763x
	 *
	 * - Note that first-pass Grain chip came out
	 *   as 0x76000000...bogus!
	 */
	chip_rev = *(volatile int *)SUN_TOP_PROD_REVISION;
	if ((chip_rev & 0xfff00000) != 0x76300000 &&
	    (chip_rev & 0xffff0000) != 0x76000000) {
		/*
		 * Fuggetaboudit.
		 */
		printk(KERN_WARNING "%s: CHIP REV 0x%08x, REQUIRES 0x763xxxxx\n",
		       __func__, chip_rev);
		return -ENODEV;
	}

	/*
	 * Check if FE firmware is loaded and running
	 */
	arm_state = *(volatile int *)ARM_BRIDGE_CONTROL;
	sb_alive  = *(volatile int *)SB_RU_ALIVE_ARM;
	load_addr = *(volatile int *)ARM_BRIDGE_DRAM_BASE;
	load_size = *(volatile int *)ARM_BRIDGE_DRAM_SIZE;

	if (!load_addr               ||
	    !load_size               ||
	    sb_alive != OFE_SB_READY ||
	    arm_state & ARM_SOFT_RESET) {
		/*
		 * No firmware loaded or active
		 */
		printk(KERN_WARNING "%s: No FE firmware loaded/active - failing probe\n",
		       __func__);
		return -ENODEV;
	}
	else if (*(volatile int *)SB_ARM_SCRATCH_1 != chip_rev)
		/*
		 * Check that CFE has properly loaded
		 * ARM Bridge Scratch Register 1 with
		 * SUN_TOP_CTRL_PROD_REVISION.
		 */
		printk(KERN_WARNING "%s: CFE UPGRADE REQUIRED - noncompliant with Side Band Protocol\n",
		       __func__);

	/*
	 * Save entry FE firmware base address & ARM window size.
	 */
	reload_physaddr   = load_addr;
	reload_windowsize = load_size;

	/*
	 * See if we need to relocate the firmware in memory.
	 * The last BMEM entry will be memory set
	 * aside for exclusive use of the FE.
	 * It ALWAYS resides at the top of physical memory.
	 */
	for (i = 0; i < MAX_BMEM_REGIONS; i++) {
		if (bmem_regions[i].size)
			last_idx = i;
	}

	if (bmem_regions[last_idx].size == FE_RSRC_SIZE) {
		/*
		 * Got the appropriate bmem entry.
		 */
		dst_addr = bmem_regions[last_idx].paddr;
	}
	else
		dst_addr = 0;
	
	if ((dst_addr && load_addr != dst_addr) ||
	    (load_size > FE_RSRC_SIZE)) {

		void *first_dst_ptr   = NULL;
		void *ioremapped_addr = NULL;
		struct page *src_page = NULL;
		struct page *dst_page = NULL;
		void *src_ptr         = NULL;
		void *dst_ptr         = NULL;
		unsigned int copy_len, bytes_left;

		VDPRINTK("Relocate FE firmware from PA 0x%08x to 0x%08x Size 0x%08x\n",
			 load_addr, dst_addr, load_size);

		reloaded = 1;

		/* Put the ARM into Soft Reset if it isn't already */
		if (!(arm_state & ARM_SOFT_RESET))
			vide_ofe_do_reset();

		/*
		 * Check that the ARM window is correctly sized.
		 * If not, fix it.
		 */
		if (load_size > FE_RSRC_SIZE) {
			printk(KERN_WARNING "%s: ARM WINDOW SIZE 0x%08x TOO LARGE, RESTRICT TO 0x%08x\n",
			       __func__, load_size, FE_RSRC_SIZE);
			load_size = FE_RSRC_SIZE;
		}

		/* Set up the source VA */
		if (load_addr & 0xe0000000) {
			/*
			 * Check if the source page is beyond where
			 * the memory configuration is limited. If
			 * load_addr is GT (dst_addr + load_size),
			 * then it is above "known" memory.
			 */
			if (load_addr >= (dst_addr + load_size)) {
				src_ptr = (void *)ioremap(load_addr, load_size);
				ioremapped_addr = src_ptr;
			}
			else
				src_page = pfn_to_page((unsigned long)load_addr >> PAGE_SHIFT);
		}
		else
			src_ptr = (void *)BCM_PHYS_TO_K0(load_addr);

		/*
		 * Set up the destination VA
		 * It had better be within the
		 * memory region known to the kernel !
		 */
		if (dst_addr & 0xe0000000)
			dst_page = pfn_to_page((unsigned long)dst_addr >> PAGE_SHIFT);
		else
			dst_ptr = first_dst_ptr = (void *)BCM_PHYS_TO_K0(dst_addr);

		if (ar_enabled(ARC_FE) == 1) {
			VDPRINTK("Disable FE Address Range Checker\n");
			if (disable_ar_check(ARC_FE))
				VDPRINTK("Failed to disable FE Address Range Checker\n");
		}

		copy_len   = PAGE_SIZE;
		bytes_left = load_size - copy_len;

		while (copy_len > 0) {

			if (src_page)
				src_ptr = kmap(src_page);
			if (dst_page) {
				dst_ptr = kmap(dst_page);
				if (bytes_left == (load_size - PAGE_SIZE))
					first_dst_ptr = dst_ptr;
			}

			if ( (load_size - bytes_left) <= FE_IMG_SIZE) {
				//VDPRINTK("Copy %d bytes from Source VA 0x%p to Dest VA 0x%p\n",
				// copy_len, src_ptr, dst_ptr);
				memcpy(dst_ptr, src_ptr, copy_len);
			}
			else {
				//VDPRINTK("Zero %d bytes from Source VA 0x%p to Dest VA 0x%p\n",
				// copy_len, src_ptr, dst_ptr);
				memset(dst_ptr, 0x0, copy_len);
			}

			if (src_page)
				kunmap(src_page++);
			else
				src_ptr += copy_len;
			if (dst_page)
				kunmap(dst_page++);
			else
				dst_ptr += copy_len;

			if (bytes_left >= PAGE_SIZE)
				bytes_left -= PAGE_SIZE;
			else if (bytes_left == 0) {
				copy_len = 0;
				break;
			}
			else {
				copy_len = bytes_left;
				bytes_left = 0;
			}
		}

		if (ioremapped_addr)
			iounmap(ioremapped_addr);

		VDPRINTK("Reset ARM Base Addr to 0x%08x, Window Size 0x%08x\n",
			 dst_addr, load_size);

		*(volatile int *)ARM_BRIDGE_DRAM_BASE = dst_addr;
		*(volatile int *)ARM_BRIDGE_DRAM_SIZE = load_size;

		/*
		 * Update global variables to reflect
		 * where firmware now resides.
		 */
		reload_physaddr   = dst_addr;
		reload_windowsize = load_size;

		if (first_dst_ptr)
			dma_cache_wback_inv((unsigned long)first_dst_ptr, load_size);

		if (ar_enabled(ARC_FE) == 1) {
			VDPRINTK("Re-enable FE Address Range Checker\n");
			if (reenable_ar_check(ARC_FE, dst_addr))
				VDPRINTK("Failed to re-enable FE Address Range Checker\n");
		}

		VDPRINTK("Take ARM out of Soft Reset\n");
		vide_ofe_clr_reset();

		/*
		 * Wait for SB Are You Alive
		 */
		retries = 100;
		while (retries-- &&
		       *(volatile int *)SB_RU_ALIVE_ARM != OFE_SB_READY)
			msleep(1);
		if (retries <= 0) {
			printk(KERN_WARNING "%s: FE Alive indicator timed out\n", __func__);
			return -ENODEV;
		}
		else
			VDPRINTK("FE firmware alive in %d msec\n", (100 - retries));
	}
	else
		VDPRINTK("FE firmware running at Phys Addr 0x%08x Size 0x%08x on entry\n",
			 load_addr, load_size);

	/*
	 * Get irq resource
	 */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0 || irq != BCM_LINUX_OFE_IRQ)
		irq = BCM_LINUX_OFE_IRQ;	/* set irq */

	/*
	 * Wire up the port..
	 */
	host = ata_host_alloc_pinfo(&pdev->dev, ppi, n_ports);
	if (!host)
		return -ENOMEM;

	gHost = host;
	ap = host->ports[0];
	ap->ioaddr.cmd_addr = (void __iomem *)VIDE_HOST_REG_BASE;

	vide_ofe_setup_port(&ap->ioaddr);

	/*
	 * Use polling mode if there's no IRQ
	 */
	if (!irq) {
		printk(KERN_WARNING "%s: No IRQ resource, using polling\n", __func__);
		/*
		 * Set port flags accordingly.
		 */
		ap->flags |= (ATA_FLAG_PIO_POLLING | VIDE_OFE_FLAG_NO_ATAPI_DMA);
	}

	/* Flag port type */
	ap->flags |= ATA_FLAG_VIDE_OFE;

	/*
	 * Link in the IRQ handler for Side Band Protocol
	 */
	rc = devm_request_irq(host->dev, BCM_LINUX_OFE_PROC_IRQ, vide_ofe_sb_interrupt,
			      IRQF_SHARED, my_device_name, host);
	if (rc)
		printk(KERN_ERR "%s: FAILED TO REGISTER IRQ FOR SB PROTOCOL!\n",
		       __func__);
	else {
		VDPRINTK("Registered IRQ Handler for Side Band Protocol\n");
		/*
		 * Disable the L2 interrupt controller mask for
		 * MIPS mailbox interrupts
		 */
		*(volatile unsigned int *)MB_INTR2_CPU_MASK_CLEAR = MIPS_MAILBOX_MASK;
	}

	/*
	 * Init the ioctl semaphore
	 */
	init_MUTEX_LOCKED(&ioctl_sem);

	/*
	 * Create the FE reset handler thread.
	 */
	rst_handler = kthread_run(vide_ofe_reset_handler, NULL, "ofe_rst");
	if (IS_ERR(rst_handler))
		printk(KERN_ERR "%s: FAILED TO CREATE FE RESET HANDLER !!\n", __func__);

	/*
	 * Note that vide_ofe_hardreset() will only wait for
	 * ATA_BUSY deassertion after ata_host_activate()
	 * has executed successfully.
	 */
	if (ofe_boot_reset ||
	    (!reloaded &&
	     (ioread8(ap->ioaddr.altstatus_addr) & ATA_BUSY))) {

		vide_ofe_hardreset();

		/*
		 * Double-check that BSY has been
		 * deasserted.
		 */
		retries = 1000;
		while (retries-- &&
		       (ioread8(ap->ioaddr.altstatus_addr) & ATA_BUSY))
			msleep(1);
		if (retries <= 0) {
			/*
			 * Firmware is still asserting ATA_BUSY.
			 * Port activation will not work.
			 * Kill the port and fail probe.
			 */
			printk(KERN_ERR "%s: ATA_BUSY NOT DEASSERTED, FIRMWARE HUNG, FAIL PROBE!\n",
			       __func__);
			ofe_standby(1);
			return -ENODEV;
		}
		else
			VDPRINTK("ATA_BSY deasserted in %d msec\n", (1000 - retries));
	}

	/*
	 * Activate the port under libata
	 */
	ata_port_desc(ap, "%s cmd 0x%lx",
		      "ioport",
		      (unsigned long)ap->ioaddr.cmd_addr);

	rc = ata_host_activate(host, irq, irq ? ata_sff_interrupt : NULL,
			       irq ? IRQF_SHARED : 0, &vide_ofe_sht);

	if (!rc) {
		/*
		 * Signal ARM that kernel is running.
		 */
		do {
			*(volatile int *)SB_RU_ALIVE_MIPS = OFE_SB_INIT_STATE;
		} while (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_INIT_STATE);
		do {
			*(volatile int *)SB_RU_ALIVE_MIPS = OFE_SB_KERNEL_ALIVE;
		} while (*(volatile int *)SB_RU_ALIVE_MIPS != OFE_SB_KERNEL_ALIVE);
	}

	return (rc);
}

/**
 *	vide_ofe_remove	-	unplug a platform interface
 *	@pdev: platform device
 *
 *	A platform bus ATA device has been unplugged. Perform the needed
 *	cleanup. Also called on module unload for any active devices.
 */
static int __devexit vide_ofe_remove(struct platform_device *pdev)
{
	struct device *dev    = &pdev->dev;
	struct ata_host *host = dev_get_drvdata(dev);

	ata_host_detach(host);

	return 0;
}

static struct platform_driver vide_ofe_platform_driver = {
	.probe		= vide_ofe_probe,
	.remove		= __devexit_p(vide_ofe_remove),
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
#ifdef CONFIG_PM
	.suspend	= vide_ofe_suspend,
	.suspend_late	= vide_ofe_suspend_late,
	.resume_early	= vide_ofe_resume_early,
	.resume		= vide_ofe_resume,
#endif
};


static struct platform_device *vide_ofe_platform_device = NULL;

static ssize_t vide_ofe_show_rev(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return (sprintf(buf, "%d.%d\n",
			ioread8((void __iomem *)VIDE_CTL_VERSION_MAJOR),
			ioread8((void __iomem *)VIDE_CTL_VERSION_MINOR)));
}
DEVICE_ATTR(vide_ofe, S_IRUGO, vide_ofe_show_rev, NULL);


static int __init vide_ofe_init(void)
{
	int ret;
	struct device *cdev;

	ret = platform_driver_register(&vide_ofe_platform_driver);
	if (ret) {
		printk(KERN_ERR "%s: Unable to register platform driver\n", DRIVER_NAME);
		goto bad;
	}

	vide_ofe_platform_device = platform_device_alloc(DRIVER_NAME, -1);
	if (! vide_ofe_platform_device) {
		printk(KERN_ERR "%s: Unable to allocate platform device\n", DRIVER_NAME);
		ret = -ENODEV;
		goto bad2;
	}

	ret = platform_device_add(vide_ofe_platform_device);
	if (ret || vide_ofe_platform_device->dev.driver == NULL) {
		printk(KERN_ERR "%s: unable to add platform device\n", DRIVER_NAME);
		if (!ret)
			ret = -ENODEV;
		goto bad3;
	}

#ifdef CONFIG_PM
	vide_ofe_platform_device->dev.driver->suspend = vide_ofe_dev_suspend;
	vide_ofe_platform_device->dev.driver->resume  = vide_ofe_dev_resume;
#endif

	cdev = &vide_ofe_platform_device->dev;
	dev_file_err  = device_create_file(cdev, &dev_attr_vide_ofe);

	/*
	 * Register the character device for SB Protocol.
	 */
	if (register_chrdev(VIDE_OFE_MAJOR, my_device_name, &vide_ofe_fops))
		printk(KERN_WARNING "%s: Failed to register char device for SB Protocol\n", my_device_name);

	return(0);

bad3:
	platform_device_put(vide_ofe_platform_device);
bad2:
	platform_driver_unregister(&vide_ofe_platform_driver);
bad:
	return(ret);
}

static void __exit vide_ofe_exit(void)
{
	struct device *cdev = &vide_ofe_platform_device->dev;

	if (cdev && !dev_file_err)
		device_remove_file(cdev, &dev_attr_vide_ofe);

	if (vide_ofe_platform_device) {
		platform_device_unregister(vide_ofe_platform_device);
		vide_ofe_platform_device = NULL;
	}

	platform_driver_unregister(&vide_ofe_platform_driver);

	/*
	 * Unregister the character device for SB Protocol.
	 */
	unregister_chrdev(VIDE_OFE_MAJOR, my_device_name);
}

module_init(vide_ofe_init);
module_exit(vide_ofe_exit);

#if defined VIDE_OFE_DEBUG
module_param(ofe_debug, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ofe_debug, "enable overall debug prints for BRCM OFE operation");
#endif
module_param(ofe_debug_pio, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ofe_debug_pio, "enable debug prints for BRCM OFE PIO mode transfers");
module_param(ofe_show_cmds, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ofe_show_cmds, "enable debug prints for ATAPI commands sent via libata");
module_param(ofe_boot_reset, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ofe_boot_reset, "force FE hard reset at boot/probe time");
