/*
 * drivers/mtd/brcmnand/edu.c
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
 *
 * @file edu.c
 * @author Jean Roberge
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#include <linux/mtd/brcmnand.h> 

#include <asm/io.h>
#include <asm/bug.h>

#include "edu.h"


#include "brcmnand_priv.h"
/**************************** Public Prototypes ******************************/
void edu_init(struct mtd_info* mtd, edu_configuration_t eduConf);
void edu_exit(void);
void edu_reset_done(void);
uint32_t edu_get_error_status_register(void);
void edu_volatile_write(uint32_t, uint32_t);
uint32_t edu_volatile_read(uint32_t);


/**************************** Private Prototypes ******************************/

static void EDU_issue_command(uint32_t, uint32_t,uint8);

static int  edu_poll_write(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);
static int  edu_common_intr_tasklet_write(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);

static int  edu_poll_read(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);
static int  edu_intr_read(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);
static int  edu_tasklet_read(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address);

static void* edu_tasklet_read_get_data(uint32_t virtual_addr_buffer);

static uint32_t edu_wait_for_completion(void);
static uint32_t edu_poll(uint32_t address, uint32_t expect, uint32_t mask);


/**************************** Private Variables *******************************/
static void *EDU_dma_buf;
static eduData_t eduData;
/**************************** Public Functions ********************************/
/*******************************************************************************
*   Function: edu_init
*
*   Parameters: 
*       mtd:     IN:  mtd_info structure
*       eduConf: IN:    edu configuration enum
*
*   Description:
*       Depending on the edu configuration enum value, this function initializes
*       the EDU itself, as well as sets the function pointers that are to be
*       used by brcmnand_page to access the device.
*
* Returns: nothing
*******************************************************************************/
void edu_init(struct mtd_info* mtd, edu_configuration_t eduConf)
{

    struct brcmnand_chip *this = mtd->priv;

    eduData.mtd = mtd;

    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_CONFIG, EDU_CONFIG_VALUE);

    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_LENGTH, EDU_LENGTH_VALUE);
 
    // Writing to PCI control register (Init PCI Window is now Here)
    edu_volatile_write(0xb0000128, 0x00000001);
    edu_volatile_write(0xb040600c, 0x00010000);

    // Clear the interrupt for next time
    edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_CLEAR, HIF_INTR2_EDU_CLEAR_MASK|HIF_INTR2_CTRL_READY); 

    EDU_dma_buf = kmalloc (EDU_LENGTH_VALUE, GFP_KERNEL);
    if (!EDU_dma_buf) {
        panic("EDU_init: could not allocate buffer memory\n");
    }

    switch(eduConf)
    {
    case EDU_POLL:
            this->EDU_write                 =   edu_poll_write;    
            this->EDU_read                  =   edu_poll_read;
            this->EDU_wait_for_completion   =   edu_wait_for_completion;
            this->EDU_read_get_data         =   NULL;
        break;

    case EDU_INTR:
            this->EDU_write                 =   edu_common_intr_tasklet_write;    
            this->EDU_read                  =   edu_intr_read;
            this->EDU_wait_for_completion   =   ISR_wait_for_completion;
            this->EDU_read_get_data         =   NULL;
        break;
    case EDU_TASKLET:
            this->EDU_write                 =   edu_common_intr_tasklet_write;
            this->EDU_read                  =   edu_tasklet_read;
            this->EDU_wait_for_completion   =   NULL;
            this->EDU_read_get_data         =   edu_tasklet_read_get_data;
        break;
    default:
        break;
    }
    
    
    if ((eduConf == EDU_INTR)|| (eduConf == EDU_TASKLET))
    {
        ISR_init();
    }
}
/*******************************************************************************
*   Function: EDU_CLRI
*
*   Parameters: 
*       n/a
*
*   Description:
*       Clears all the EDU interrupts, as well as the NAND controller interrupts.
*
* Returns: nothing
*******************************************************************************/
void inline EDU_CLRI(void)
{
    struct brcmnand_chip *this = eduData.mtd->priv;

    //Wait for counter to get down to 0:
    edu_reset_done();
    
    //Clear NAND controller interrupts:
    this->ctrl_write(BCHP_NAND_ECC_CORR_EXT_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_CORR_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_EXT_ADDR, 0);
    this->ctrl_write(BCHP_NAND_ECC_UNC_ADDR, 0);
    
    //Clear EDU interrupts:
    edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_STATUS, 
                       HIF_INTR2_EDU_CLEAR_MASK);
    
    //Clear EDU_ERR_STATUS register:                                          
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_ERR_STATUS, 
                       0x00000000);
}

void edu_exit(void)
{
    if (EDU_dma_buf)
        kfree(EDU_dma_buf);

}

static uint32_t edu_wait_for_completion(void)
{
    uint32_t hif_err = edu_poll(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_STATUS, 
        HIF_INTR2_EDU_DONE, HIF_INTR2_EDU_DONE_MASK);
    return hif_err;
}

// 32-bit register polling
// Poll a register until the reg has the expected value.
// a timeout read count. The value reflects how many reads
// the routine check the register before is gives up.

static uint32_t edu_poll(uint32_t address, uint32_t expect, uint32_t mask)
{
        uint32_t rd_data=0, i=0; 
        unsigned long timeout;
        
        __sync();
        rd_data = edu_volatile_read(address);

        timeout = jiffies + msecs_to_jiffies(3000); // 3 sec timeout for now (testing)
        while ((rd_data & mask) != (expect & mask)) /* && (i<cnt) */ 
        {
         
                __sync(); //PLATFORM_IOFLUSH_WAR();
                rd_data = edu_volatile_read(address);
                
                // JR+ 2008-02-01 Allow other tasks to run while waiting
                cond_resched();
                // JR- 2008-02-01 Allow other tasks to run while waiting
                
                i++;
                if(!time_before(jiffies, timeout))
                {
                   printk("edu_poll timeout at 3 SECONDS (just for testing) with i= 0x%.08x!\n", (int)i);
                   printk("DBG> edu_poll (0x%.08x, 0x%.08x, 0x%.08x);\n",address, expect, mask);
                   return(rd_data);
                }
        }
        return(rd_data);
}


static int edu_poll_write(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address)
{
    uint32_t  phys_mem;
    int ret = 0;

    if (KSEGX(virtual_addr_buffer) == KSEG0) {
        phys_mem = virt_to_phys((void *)virtual_addr_buffer);
        dma_cache_wback(virtual_addr_buffer, EDU_LENGTH_VALUE);
    } else {
        phys_mem = virt_to_phys((void *)EDU_dma_buf);
        memcpy(EDU_dma_buf, (void *)virtual_addr_buffer, EDU_LENGTH_VALUE);
        dma_cache_wback((unsigned long)EDU_dma_buf, EDU_LENGTH_VALUE);
    }

    edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_CLEAR, HIF_INTR2_EDU_CLEAR_MASK);

    edu_reset_done(); 
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_ERR_STATUS, 0x00000000); 
    
    EDU_issue_command(phys_mem, external_physical_device_address, EDU_WRITE); /* 1: Is a Read, 0 Is a Write */

//mfi: Do not wait for completion here;  This is done in brcmnand_base.c.

    return ret;
}

static int edu_common_intr_tasklet_write(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address)
{
    uint32_t  phys_mem;
    int ret = 0;

    unsigned long flags;


    if (KSEGX(virtual_addr_buffer) == KSEG0) {
        phys_mem = virt_to_phys((void *)virtual_addr_buffer);
        dma_cache_wback(virtual_addr_buffer, EDU_LENGTH_VALUE);
    } else {
        phys_mem = virt_to_phys((void *)EDU_dma_buf);
        memcpy(EDU_dma_buf, (void *)virtual_addr_buffer, EDU_LENGTH_VALUE);
        dma_cache_wback((unsigned long)EDU_dma_buf, EDU_LENGTH_VALUE);
    }

    spin_lock_irqsave(&gEduIsrData.lock, flags);
    gEduIsrData.flashAddr = external_physical_device_address;
    gEduIsrData.dramAddr = phys_mem;
    
    /*
     * Enable L2 Interrupt
     */
    gEduIsrData.cmd = EDU_WRITE;
    gEduIsrData.opComplete = 0;
    gEduIsrData.status = 0;
    
    /* On write we wait for both DMA done|error and Flash Status */
    gEduIsrData.mask = HIF_INTR2_EDU_CLEAR_MASK|HIF_INTR2_CTRL_READY;
    gEduIsrData.expect = HIF_INTR2_EDU_DONE|HIF_INTR2_CTRL_READY;
    gEduIsrData.error = HIF_INTR2_EDU_ERR;
    gEduIsrData.intr = HIF_INTR2_EDU_DONE;
    spin_unlock_irqrestore(&gEduIsrData.lock, flags);

    EDU_CLRI();
    ISR_enable_irq();

    
    EDU_issue_command(phys_mem, external_physical_device_address, EDU_WRITE); /* 1: Is a Read, 0 Is a Write */

//mfi: Do not wait for completion here;  This is done in brcmnand_base.c.

    return ret;
}

static int edu_poll_read(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address)
{
    uint32_t  phys_mem;
    int ret = 0;
    
    if (KSEGX(virtual_addr_buffer) == KSEG0) {
        dma_cache_inv(virtual_addr_buffer, EDU_LENGTH_VALUE);
        phys_mem = virt_to_phys((void *)virtual_addr_buffer);
    }
    else {
        dma_cache_inv((unsigned long)EDU_dma_buf, EDU_LENGTH_VALUE);
        phys_mem = virt_to_phys((void *)EDU_dma_buf);
    }

    edu_volatile_write(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_CLEAR, HIF_INTR2_EDU_CLEAR_MASK);

    edu_reset_done(); 
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_ERR_STATUS, 0x00000000); 
    
    EDU_issue_command(phys_mem, external_physical_device_address, EDU_READ); /* 1: Is a Read, 0 Is a Write */

    ret = edu_poll(EDU_BASE_ADDRESS  + BCHP_HIF_INTR2_CPU_STATUS, 
                    HIF_INTR2_EDU_DONE, HIF_INTR2_EDU_DONE);

    if (KSEGX(virtual_addr_buffer) != KSEG0) 
        memcpy((void *)virtual_addr_buffer, EDU_dma_buf, EDU_LENGTH_VALUE);

    return ret;
} 

static int edu_intr_read(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address)
{
    uint32_t  phys_mem;
    int ret = 0;

    unsigned long flags;

    int retries = 2;

    if (KSEGX(virtual_addr_buffer) == KSEG0) {
        dma_cache_inv(virtual_addr_buffer, EDU_LENGTH_VALUE);
        phys_mem = virt_to_phys((void *)virtual_addr_buffer);
    }
    else {
        dma_cache_inv((unsigned long)EDU_dma_buf, EDU_LENGTH_VALUE);
        phys_mem = virt_to_phys((void *)EDU_dma_buf);
    }

    spin_lock_irqsave(&gEduIsrData.lock, flags);

    gEduIsrData.flashAddr = external_physical_device_address;
    gEduIsrData.dramAddr = phys_mem;
    
    /*
     * Enable L2 Interrupt
     */
    gEduIsrData.cmd = EDU_READ;
    gEduIsrData.opComplete = 0;
    gEduIsrData.status = 0;

    // We must also wait for Ctlr_Ready, otherwise the OOB is not correct, since we read the OOB bytes off the controller
    gEduIsrData.mask = HIF_INTR2_EDU_CLEAR_MASK|HIF_INTR2_CTRL_READY;
    gEduIsrData.expect = HIF_INTR2_EDU_DONE|HIF_INTR2_CTRL_READY;
    // On error we also want Ctrl-Ready because for COR ERR, the Hamming WAR depends on the OOB bytes.
    gEduIsrData.error = BCHP_HIF_INTR2_CPU_STATUS_NAND_CORR_INTR_MASK | BCHP_HIF_INTR2_CPU_STATUS_NAND_UNC_INTR_MASK;
    gEduIsrData.intr =  HIF_INTR2_NAND_ERROR |
                        HIF_INTR2_CTRL_READY | 
                        HIF_INTR2_EDU_DONE;

    spin_unlock_irqrestore(&gEduIsrData.lock, flags);

    EDU_CLRI(); 
    ISR_enable_irq();
    
    do 
    {
        EDU_issue_command(phys_mem, external_physical_device_address, EDU_READ);
        
        ret = ISR_wait_for_completion();
        
    } while (ret == (uint32_t) (-ERESTARTSYS) && retries-- > 0);
    
    if (ret == (uint32_t) (-ERESTARTSYS)) 
    {
        ret = 0; /* Treat it as a timeout */
        show_stack(current,NULL);
        dump_stack();
    }

    if (KSEGX(virtual_addr_buffer) != KSEG0) 
        memcpy((void *)virtual_addr_buffer, EDU_dma_buf, EDU_LENGTH_VALUE);

    return ret;
} 

static int edu_tasklet_read(uint32_t virtual_addr_buffer, uint32_t external_physical_device_address)
{
    uint32_t  phys_mem;
    int ret = 0;

    unsigned long flags;

    if (KSEGX(virtual_addr_buffer) == KSEG0) {
        dma_cache_inv(virtual_addr_buffer, EDU_LENGTH_VALUE);
        phys_mem = virt_to_phys((void *)virtual_addr_buffer);
    }
    else {
        dma_cache_inv((unsigned long)EDU_dma_buf, EDU_LENGTH_VALUE);
        phys_mem = virt_to_phys((void *)EDU_dma_buf);
    }

     spin_lock_irqsave(&gEduIsrData.lock, flags);

     gEduIsrData.flashAddr = external_physical_device_address;
     gEduIsrData.dramAddr = phys_mem;
    
    /*
     * Enable L2 Interrupt
     */
    gEduIsrData.cmd = EDU_READ;
    gEduIsrData.opComplete = 0;
    gEduIsrData.status = 0;

    // We must also wait for Ctlr_Ready, otherwise the OOB is not correct, since we read the OOB bytes off the controller

    gEduIsrData.mask = HIF_INTR2_EDU_CLEAR_MASK|HIF_INTR2_CTRL_READY;
    gEduIsrData.expect = HIF_INTR2_EDU_DONE|HIF_INTR2_CTRL_READY;
    // On error we also want Ctrlr-Ready because for COR ERR, the Hamming WAR depends on the OOB bytes.
    gEduIsrData.error = HIF_INTR2_EDU_ERR;
    gEduIsrData.intr = HIF_INTR2_EDU_DONE_MASK;

    spin_unlock_irqrestore(&gEduIsrData.lock, flags);

    EDU_CLRI(); 
    ISR_enable_irq();
    
    //issue command and return:  Interrupts will be handled from the tasklet
    EDU_issue_command(phys_mem, external_physical_device_address, EDU_READ);
        
    return ret;

} 


void* edu_tasklet_read_get_data(uint32_t virtual_addr_buffer)
{
    if (KSEGX(virtual_addr_buffer) != KSEG0) 
    {
        memcpy((void *)virtual_addr_buffer, EDU_dma_buf, EDU_LENGTH_VALUE);
    }
    //else the data is already in virtual_addr_buffer
    
    return (void *)virtual_addr_buffer;

}


// THT: Write until done clears
void edu_reset_done(void)
{
    volatile uint32_t rd_data;

    rd_data = edu_volatile_read(EDU_BASE_ADDRESS  + EDU_DONE);

    while (rd_data & 0x3) {        
        // Each Write decrement DONE by 1
       // printk("EDU_DONE: %d\n", rd_data);
        edu_volatile_write(EDU_BASE_ADDRESS  + EDU_DONE, 0);
        __sync();
        rd_data = edu_volatile_read(EDU_BASE_ADDRESS  + EDU_DONE);
    } 
}
uint32_t edu_get_error_status_register(void)
{
    uint32_t valueOfReg = edu_volatile_read(EDU_BASE_ADDRESS  + EDU_ERR_STATUS);  

    // Clear the error
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_ERR_STATUS, 0x00000000);   

    return(valueOfReg);
}

void edu_volatile_write(uint32_t addr, uint32_t data)
{
    volatile uint32_t* pAddr;

    pAddr = (volatile uint32_t *)addr;
    *pAddr = (volatile uint32_t)data;
}

uint32_t edu_volatile_read(uint32_t addr)
{
    volatile uint32_t* pAddr;
        
    pAddr = (volatile uint32_t *)addr;
        
    return *(uint32_t *)pAddr;
}


/**************************** Private Functions *******************************/

static void EDU_issue_command(uint32_t dram_addr, uint32_t ext_addr,uint8 cmd)
{
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_DRAM_ADDR, dram_addr);
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_EXT_ADDR, ext_addr);
    edu_volatile_write(EDU_BASE_ADDRESS  + EDU_CMD, cmd);
}
