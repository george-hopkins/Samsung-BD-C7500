/*
 * drivers/mtd/brcmnand/brcmnand_isr.c
 *
 *  Copyright (c) 2005-2009 Broadcom Corp.
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
 * Data structures for Broadcom NAND controller
 * 
 * when		who		what
 * 20090318	tht		Original coding
 */


#include "brcmnand_priv.h"
#include "eduproto.h"  /* For Volatile Read/Write, for now, cuz ISR has nothing to do with EDU */


#define PRINTK(...)
//define PRINTK printk

#define BRCM_NAND_HIF_INTR2_CPU_STATUS_MASK (BCHP_HIF_INTR2_CPU_STATUS_EDU_DONE_INTR_MASK | \
                                             BCHP_HIF_INTR2_CPU_STATUS_EDU_ERR_INTR_MASK | \
                                             BCHP_HIF_INTR2_CPU_STATUS_NAND_CTLRDY_INTR_MASK)
 
// Wakes up the sleeping calling thread.
static DECLARE_WAIT_QUEUE_HEAD(gEduWaitQ);

#ifdef CONFIG_MTD_BRCMNAND_EDU_TASKLET
#include <linux/interrupt.h>
extern struct tasklet_struct gEduTasklet;
#endif //#ifndef CONFIG_MTD_BRCMNAND_EDU_TASKLET

eduIsrData_t gEduIsrData;

static irqreturn_t brcmnand_edu_intr(int irq, void *devid)
{
	uint32_t status, rd_data;
	uint32_t intrMask;  
	
	unsigned long flags;

	intrMask = edu_volatile_read(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_MASK_STATUS);
	rd_data = edu_volatile_read(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_STATUS);

	/*
	 * Not mine
	 */
	if (devid != (void*) &gEduIsrData || !(BRCM_NAND_HIF_INTR2_CPU_STATUS_MASK & rd_data & ~intrMask)) {
		return IRQ_NONE;
	}

    PRINTK("%s: Awaken rd_data=%08x, intrMask=%08x, cmd=%d, flashAddr=%08x\n", __FUNCTION__, 
           rd_data, intrMask, gEduIsrData.cmd, gEduIsrData.flashAddr);

	/*
	 * Remember the status, as there can be several L1 interrupts before completion
	 */
	spin_lock_irqsave(&gEduIsrData.lock, flags);

	gEduIsrData.status |= rd_data;
	status = gEduIsrData.status & gEduIsrData.mask;
	// Complete if done or error
	switch (gEduIsrData.cmd) {
	case EDU_READ:
	case NAND_CTRL_READY:
		gEduIsrData.opComplete = ((gEduIsrData.expect == (gEduIsrData.status & gEduIsrData.expect)) || 
								(gEduIsrData.status & gEduIsrData.error));
		break;
		
	case EDU_WRITE:
		/* 
		 * We wait for both DONE|ERR +CTRL_READY
		 */
		gEduIsrData.opComplete = ((gEduIsrData.expect == (gEduIsrData.status & gEduIsrData.expect) ||
									(gEduIsrData.status & gEduIsrData.error))
								&&
								(gEduIsrData.status & HIF_INTR2_CTRL_READY));
		break;							
	}
	if (gEduIsrData.opComplete) {
		ISR_disable_irq(gEduIsrData.intr);
		
#ifdef CONFIG_MTD_BRCMNAND_EDU_TASKLET
        if (gEduIsrData.bWait == true)
        {
            gEduIsrData.bWait = false;

     		wake_up_interruptible(&gEduWaitQ);
        }
        else
        {
            tasklet_schedule(&gEduTasklet);
        }

#else //#ifdef CONFIG_MTD_BRCMNAND_EDU_TASKLET
		wake_up_interruptible(&gEduWaitQ);
#endif //#ifdef CONFIG_MTD_BRCMNAND_EDU_TASKLET
		
	}
	else {
		/* Ack only the ones that show */
		uint32_t ack = gEduIsrData.status & gEduIsrData.intr;
		
printk("%s: opComp=0, intr=%08x, mask=%08x, expect=%08x, err=%08x, status=%08x, rd_data=%08x, intrMask=%08x, flashAddr=%08x, DRAM=%08x\n", __FUNCTION__, 
gEduIsrData.intr, gEduIsrData.mask, gEduIsrData.expect, gEduIsrData.error, gEduIsrData.status, rd_data, intrMask, gEduIsrData.flashAddr, gEduIsrData.dramAddr);

		// Just disable the ones that are triggered
		ISR_disable_irq(ack);
		gEduIsrData.intr &= ~ack;

		if (gEduIsrData.intr) {
			// Re-arm
			ISR_enable_irq();
	
		}
		else {
			printk(KERN_ERR "%s: Lost interrupt\n", __FUNCTION__);
			//BUG();
		}
	}

	spin_unlock_irqrestore(&gEduIsrData.lock, flags);

	return IRQ_HANDLED;
}


uint32_t ISR_wait_for_completion(void)
{
	//uint32_t rd_data;
	int ret=1;
	unsigned long to_jiffies = 3*HZ; /* 3 secs */
	int cmd;

	unsigned long flags;

	
	gEduIsrData.bWait = true;
	
    ret = wait_event_interruptible_timeout(gEduWaitQ, gEduIsrData.opComplete, to_jiffies);


	spin_lock_irqsave(&gEduIsrData.lock, flags);

	cmd = gEduIsrData.cmd;
	gEduIsrData.cmd = -1;

	if (!gEduIsrData.opComplete && ret <= 0) {
		ISR_disable_irq(gEduIsrData.intr);
		if (ret == -ERESTARTSYS) {

			spin_unlock_irqrestore(&gEduIsrData.lock, flags);

			return (uint32_t) (ERESTARTSYS);  // Retry on Read
		}	
		else if (ret == 0) { 
			//gEduIsrData.opComplete = 1;
			printk("%s: DMA timedout\n", __FUNCTION__);
			spin_unlock_irqrestore(&gEduIsrData.lock, flags);
			return 0; // Timed Out
		}
	
		// DMA completes on Done or Error.
		//rd_data = edu_volatile_read(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_STATUS);
	
		printk("%s: EDU completes but Status is %08x\n", __FUNCTION__, gEduIsrData.status);
		//rd_data = 0; // Treat as a timeout
	}

	spin_unlock_irqrestore(&gEduIsrData.lock, flags);

	return gEduIsrData.status;
}

uint32_t ISR_cache_is_valid(uint32_t clearMask)
{
	uint32_t rd_data;
	unsigned long flags;


	// Clear existing interrupt
	edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_MASK_SET, clearMask);
	
	do {

		spin_lock_irqsave(&gEduIsrData.lock, flags);

        gEduIsrData.flashAddr = 0;
	 	gEduIsrData.dramAddr = 0;
		
		/*
		 * Enable L2 Interrupt
		 */
		gEduIsrData.cmd = NAND_CTRL_READY;
		gEduIsrData.opComplete = 0;
		gEduIsrData.status = 0;
		
		/* On write we wait for both DMA done|error and Flash Status */
		gEduIsrData.mask = HIF_INTR2_CTRL_READY;
		gEduIsrData.expect = HIF_INTR2_CTRL_READY;
		gEduIsrData.error = 0;
		gEduIsrData.intr = HIF_INTR2_CTRL_READY;

		spin_unlock_irqrestore(&gEduIsrData.lock, flags);

		ISR_enable_irq();
	
		rd_data = ISR_wait_for_completion();
	} while (rd_data == ERESTARTSYS);
	return rd_data;

}


uint32_t ISR_getStatus(void)
{
    if (gEduIsrData.status == 0)
    {
        printk("%s: ISR STATUS is 0\n", __FUNCTION__);
    }

    return gEduIsrData.status;
}


void ISR_init(void)
{
	int ret;
	uint32_t intrMask;

	spin_lock_init(&gEduIsrData.lock);

    //Initialize:
    gEduIsrData.bWait = false;

	// Mask NAND  L2 interrupts
    intrMask = (BCHP_HIF_INTR2_CPU_SET_EDU_DONE_INTR_MASK |     \
                BCHP_HIF_INTR2_CPU_SET_EDU_ERR_INTR_MASK |      \
                BCHP_HIF_INTR2_CPU_SET_NAND_CTLRDY_INTR_MASK);

	edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_MASK_SET, intrMask);
	mmiowb();

	ret = request_irq(BCM_LINUX_EDU_IRQ, brcmnand_edu_intr, IRQF_DISABLED | IRQF_SHARED, "brcmnand EDU", &gEduIsrData);
	if (ret) {
		printk(KERN_INFO "%s: request_irq(BCM_LINUX_CPU_INTR1_IRQ) failed ret=%d.  Someone not sharing?\n", 
			__FUNCTION__, ret);
	}
	
}


 
