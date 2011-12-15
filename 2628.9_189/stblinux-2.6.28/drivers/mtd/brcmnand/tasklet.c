/*
 * drivers/mtd/brcmnand/tasklet.c
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
 * when		who		what
 * 20090331	mfi		Original coding
 */

#include <linux/interrupt.h>
#include <linux/mtd/brcmnand.h>

#include "brcmnand_priv.h"

/**************************Tasklet structure***********************************/



/**************************Private prototypes**********************************/

static int edu_tasklet_trig_read_intr(void);
static int edu_tasklet_trig_write_intr(void);
static int edu_tasklet_wait_for_completion(void);
static void edu_tasklet_edu_tasklet(unsigned long data);

/**************************Private variables **********************************/
//Declare wait queue:
static DECLARE_WAIT_QUEUE_HEAD(gEduTaskletWaitQ);


/**************************Public variables ***********************************/
eduTaskletData_t gEduTaskletData;


DECLARE_TASKLET(gEduTasklet, &edu_tasklet_edu_tasklet, (unsigned long)&gEduTaskletData);
/**************************Public, extern variables ***************************/
extern bool runningInA7601A;
extern char brcmNandMsg[50];
extern int gdebug;
extern uint32_t EDU_ldw;

/**************************Public functions************************************/
/*******************************************************************************
*   Function: edu_tasklet_trig_write
*
*   Parameters: 
*       mtd:        IN: Pointer to the general mtd structure
*       startslice: IN: Writing to the device is done in "" slices.  
*                       This indicates which of the slice is the start.
*       buffer:     IN: Data to write
*       oobarea:    IN: Out-Of-Bound area to write
*       offseet:    IN: Where to write?
*       padlen:     IN: Qty of bytes that need to be padded in the front of
*                       the buffer we will write
*   Description:
*       This trigs the write interrupt, puts the calling thread to sleep
*       and wakes up when the edu_tasklet_edu_tasklet is done.
*
*******************************************************************************/
int edu_tasklet_trig_write(struct mtd_info* mtd,
                      int startslice,  //column
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset,
                      int len,
                      int padlen)
{
    int ret = 0;
    struct brcmnand_chip *this = mtd->priv;  
    //Prepare the write interrupt:
    gEduTaskletData.cmd = EDU_WRITE;
    gEduTaskletData.pMtd = mtd;
    gEduTaskletData.pDataBuf = buffer;
    gEduTaskletData.pOobBuf = oobarea;
    gEduTaskletData.offset = offset;
    gEduTaskletData.len = len;
    gEduTaskletData.winslice = startslice;
    gEduTaskletData.retries = 0; //No retries on write for now - mfi 04/26/2009
    
    gEduTaskletData.padlen = padlen;
    gEduTaskletData.done = 0;
    
    //Initialize return values:
    gEduTaskletData.written = 0;
    gEduTaskletData.oobWritten = 0; 
    gEduTaskletData.opComplete = 0;
    gEduTaskletData.needBBT = false;

    gEduTaskletData.intr_status = 0;

    gEduTaskletData.error = 0;  

    edu_tasklet_trig_write_intr();
    
    ret = edu_tasklet_wait_for_completion();
    
    if (gEduTaskletData.needBBT == true)
    {
        ret = this->block_markbad(mtd, offset);
        if (ret == 0)
        {
            ret = -EBADBLK; //This is a write, so mark block as bad
        }
        else
        {
            ret = -EBADMSG; //Could not mark block as bad, so return same status as read
        }
        
    }
  
    return ret;
}

/* sort total number of ecc bits corrected in terms of 1 ,2, 3 bits error corrected  */
extern unsigned long g_numeccbits_corrected_cnt[BRCMNAND_ECC_BCH_8];
extern unsigned long g_total_ecc_bits_corrected_cnt;
extern unsigned long g_prev_total_bits_corrected_cnt;

/*******************************************************************************
*   Function: edu_tasklet_trig_read
*
*   Parameters: 
*       mtd:        IN: Pointer to the general mtd structure
*       buffer:     IN: Data to write
*       oobarea:    IN: Out-Of-Bound area to read
*       offset:     IN: Where to read?
*       len:        IN: How much bytes to read?
*       pDataRead:  OUT: The number of bytes read
*       pOobRead:   OUT: The number of bytes read out of the oob
*
*   Description:
*       This trigs the read interrupt, puts the calling thread to sleep
*       and wakes up when the edu_tasklet_edu_tasklet is done.
*
*******************************************************************************/
int edu_tasklet_trig_read(struct mtd_info* mtd,
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset, 
                      int len,
                      int* pDataRead,
                      int* pOobRead)
{
    int ret = 0; // no error

#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0                
    struct brcmnand_chip * this = mtd->priv;
    uint32_t bit_cnt;
    /* bit level stats for ecc errors */
    g_total_ecc_bits_corrected_cnt = this->ctrl_read(BCHP_NAND_READ_ERROR_COUNT);
    bit_cnt =  g_total_ecc_bits_corrected_cnt - g_prev_total_bits_corrected_cnt;
    
    if(bit_cnt && (bit_cnt <= BRCMNAND_ECC_BCH_8)) {
        g_numeccbits_corrected_cnt[bit_cnt - 1]++; 
    }
    
    g_prev_total_bits_corrected_cnt = g_total_ecc_bits_corrected_cnt;
#endif


    //Prepare the read interrupt:
    gEduTaskletData.cmd = EDU_READ;
    gEduTaskletData.pMtd = mtd;
    gEduTaskletData.pDataBuf = buffer;
    gEduTaskletData.pOobBuf = oobarea;
    gEduTaskletData.offset = offset;
    gEduTaskletData.len = len;
    gEduTaskletData.winslice = 0;
    gEduTaskletData.retries = 5;    //Up to 5 retries on read - mfi 04/26/2009
    //Initialize return values:
    gEduTaskletData.dataRead = 0;
    gEduTaskletData.oobRead = 0;
    gEduTaskletData.opComplete = 0;
    gEduTaskletData.needBBT = false; //not used for reads, but still needs to be initialized
    
    gEduTaskletData.intr_status = 0;

    gEduTaskletData.error = 0;    
    
    //
    
    gEduTaskletData.written = 0;
    gEduTaskletData.oobWritten = 0;
    gEduTaskletData.done    = 0;
    gEduTaskletData.padlen = 0;
    
    edu_tasklet_trig_read_intr();

    ret = edu_tasklet_wait_for_completion();
    
    *pDataRead = gEduTaskletData.dataRead;
    *pOobRead = gEduTaskletData.oobRead;
    
    return ret;        
}

int edu_tasklet_getWriteStatus(int *outp_needBBT)
{
    *outp_needBBT = (gEduTaskletData.needBBT == true)? 1 : 0;
       
    return gEduTaskletData.error;

}
/**************************Private functions***********************************/

/*******************************************************************************
*   Function: edu_tasklet_edu_tasklet
*
*   Parameters: 
*       data: IN: A disguised pointer to an eduTaskletData_t* variable
*
*   Description:
*       This is the edu_tasklet that is scheduled from the Interrupt Service Routine
*
*******************************************************************************/
static void edu_tasklet_edu_tasklet(unsigned long data)
{
    int i, thislen;
    int ecc = 0;
    int error = 0;
    int wrStatus = 0;

    int outp_needBBT = 0;
    struct brcmnand_chip *this = gEduTaskletData.pMtd->priv;
    uint32_t* p32 = NULL;

    if ((eduTaskletData_t*)data != &gEduTaskletData)
    {
        printk(KERN_DEBUG"%s: data is not gEduTaskletData!\n", __FUNCTION__);
        return;
    }
    
    if (gEduTaskletData.cmd == EDU_READ)
    {
        if ((gEduTaskletData.winslice <= this->eccsteps) && 
            (gEduTaskletData.dataRead < gEduTaskletData.len))
        {
            //Get the intr_status:
            gEduTaskletData.intr_status = ISR_getStatus();

            ecc = this->process_read_isr_status(this, gEduTaskletData.intr_status);
            //save value:
            error = gEduTaskletData.error;
        
            //Fetch the data:
            this->EDU_read_get_data((uint32_t)&gEduTaskletData.pDataBuf[gEduTaskletData.dataRead]);

            gEduTaskletData.error = this->process_error(gEduTaskletData.pMtd,
                                        (uint8_t*)&gEduTaskletData.pDataBuf[gEduTaskletData.dataRead],
                                        &gEduTaskletData.pOobBuf[gEduTaskletData.oobRead],
                                        ecc);

            if ((gEduTaskletData.error == 0) && (error != 0))
            {//If we found an error before this pass:
                gEduTaskletData.error = error;  
            }
            else if (gEduTaskletData.error == -EECCCOR)
            {//If we found an error during this pass:
                if (error == -EECCUNCOR) //EECCUNCOR has higher priority than EECCCOR
                {
                    gEduTaskletData.error = -EECCUNCOR;
                }
            }
        

            if ((gEduTaskletData.error == -ETIMEDOUT) && (gEduTaskletData.retries > 0))
            {
                gEduTaskletData.retries--;
                printk(KERN_DEBUG"%s: Doing a read retry (status time out), retries= %d\n", 
                    __FUNCTION__, gEduTaskletData.retries);
                    
                gEduTaskletData.error = 0;     
            }
            else if (gEduTaskletData.error == -ETIMEDOUT)
            {
                printk(KERN_ERR"%s: too much retries, give up! \n", 
                    __FUNCTION__);

                 //More than "gEduTaskletData.retries" timeout:
                 gEduTaskletData.opComplete = 0;
                 
                 goto disable_and_wake_up;
            }
            else //all other cases
            {
                if (gEduTaskletData.pOobBuf)
                {
                    p32 = (uint32_t*) &gEduTaskletData.pOobBuf[gEduTaskletData.oobRead];

                    __sync(); //PLATFORM_IOFLUSH_WAR();
                    for (i = 0; i < 4; i++) 
                    {
                        p32[i] = be32_to_cpu (this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + i*4));
                    }
/*                
                printk("%s: offset=%s, oobRead= %d, oob=", 
                                __FUNCTION__, 
                                __ll_sprintf(brcmNandMsg,gEduTaskletData.offset, this->xor_invert_val), 
                                gEduTaskletData.oobRead);
                                
                print_oobbuf(&gEduTaskletData.pOobBuf[gEduTaskletData.oobRead], 16);
*/                
                    gEduTaskletData.oobRead += this->eccOobSize;
                }
            
                //Increment values for next call to this function:
                thislen = min_t(int, gEduTaskletData.len - gEduTaskletData.dataRead, gEduTaskletData.pMtd->eccsize);
    
                gEduTaskletData.dataRead += thislen;
                gEduTaskletData.offset += thislen;
                gEduTaskletData.winslice++;
                gEduTaskletData.retries = 5;    //Reset retries to 5 - mfi 04/26/2009
            }    
            //If dataRead is still lower than len:
            if (gEduTaskletData.dataRead < gEduTaskletData.len)
            {
                //Trig next read:
                edu_tasklet_trig_read_intr();
            }
            else
            {
                goto op_complete;
            }
        }
        else
        {
            goto op_complete;
        }
    }
    else if (gEduTaskletData.cmd == EDU_WRITE)
    {
        thislen = min_t(int, gEduTaskletData.len-gEduTaskletData.written, 
                        gEduTaskletData.pMtd->eccsize - gEduTaskletData.padlen);
        //One slice is already written, so increment:
        gEduTaskletData.winslice++;
        gEduTaskletData.written += thislen;
        if (gEduTaskletData.pOobBuf) 
        {
            gEduTaskletData.oobWritten += this->eccOobSize;
        }

        //Get the isr_status:
        gEduTaskletData.intr_status = ISR_getStatus();
        //Get the isr_status:
        wrStatus = this->process_write_isr_status(this, gEduTaskletData.intr_status, &outp_needBBT);

        if(wrStatus != BRCMNAND_SUCCESS)
        {//For now, treat all other return values as EECCUNCOR:
            gEduTaskletData.error = -EECCUNCOR;
            gEduTaskletData.opComplete = 0;
            if (outp_needBBT != 0)
            {
                gEduTaskletData.needBBT =  true;
            }            
            goto disable_and_wake_up;
        }
        
        
        if ((gEduTaskletData.winslice < this->eccsteps) && 
            (gEduTaskletData.written < gEduTaskletData.len) && 
            (!gEduTaskletData.done))
        {
            //trig next write:
            edu_tasklet_trig_write_intr();
        }
        else
        {
            goto op_complete;
        }
    }

    return; //If we're trigging the next read or write

op_complete:
    gEduTaskletData.opComplete = 1;

disable_and_wake_up:
    //Disable interrupt:
    ISR_disable_irq(gEduIsrData.intr);
                
    //Wake-up calling process:
    wake_up(&gEduTaskletWaitQ);
}




/*******************************************************************************
*   Function: edu_tasklet_trig_write_intr
*
*   Parameters: 
*       none
*
*   Description:
*       Trigs write interrupt 
*
*   Returns: 
*       0: no error
*******************************************************************************/
static int edu_tasklet_trig_write_intr(void)
{
//    struct mtd_info* mtd = gEduTaskletData.pMtd;
    struct brcmnand_chip *this = gEduTaskletData.pMtd->priv;
    
    uint32_t* p32;
    int i,thislen; 
//    int ret = 0;
    
    thislen = min_t(int, gEduTaskletData.len-gEduTaskletData.written, 
                    gEduTaskletData.pMtd->eccsize - gEduTaskletData.padlen);
                    
    //Set the offset correctly (removing the front padding if needed):
    if (gEduTaskletData.written == 0)
    {
        gEduTaskletData.offset = __ll_add32(gEduTaskletData.offset, gEduTaskletData.written-gEduTaskletData.padlen);
    }                                                        
    else
    {
        gEduTaskletData.offset = __ll_add32(gEduTaskletData.offset, thislen);
    }

    //  Pad at the end ?
    if (thislen != gEduTaskletData.pMtd->eccsize - gEduTaskletData.padlen) 
    {
        int fillupLen = ((gEduTaskletData.padlen+gEduTaskletData.len - 
                                    gEduTaskletData.written + 
                                        gEduTaskletData.pMtd->eccsize) & ~(gEduTaskletData.pMtd->eccsize - 1)) - 
                                        (gEduTaskletData.padlen+gEduTaskletData.len);
        gEduTaskletData.done = 1; // Writing last slice
        //Pad with 0xff before writing
        memset(this->pLocalPage, 0xFF, fillupLen);
        memcpy(&this->data_poi[gEduTaskletData.padlen+gEduTaskletData.len], this->pLocalPage, fillupLen);
    }
            
    this->ctrl_writeAddr(this, gEduTaskletData.offset, 0);

    if (gEduTaskletData.pOobBuf) 
    {
        p32 = (uint32_t*) &gEduTaskletData.pOobBuf[gEduTaskletData.oobWritten];
//        print_oobbuf(&gEduTaskletData.pOobBuf[gEduTaskletData.oobWritten], 16);
    }
    else 
    {
        // Fill with 0xFF if don't want to change OOB
        p32 = (uint32_t*)&this->pLocalOob[gEduTaskletData.oobWritten];
        memset(p32, 0xFF, this->eccOobSize);
//        printk("this->pLocalOob:");
//        print_oobbuf(&this->pLocalOob[gEduTaskletData.oobWritten], 16);
    }

// There's 4 32-bits registers to write to fill 16 oob bytes per 512 data bytes
    for (i = 0; i < 4; i++) 
    {
        this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, cpu_to_be32(p32[i]));
    }

    __sync(); //PLATFORM_IOFLUSH_WAR(); // Check if this line may be taken-out

    if(this->options & NAND_COMPLEX_OOB_WRITE)
    {
        p32 = (uint32_t*)&this->pLocalOob[0];
        memset(p32, 0xFF, gEduTaskletData.pMtd->oobsize);
        this->EDU_write((uint32)p32, (uint32) EDU_ldw);
    }
    else
    {
        this->EDU_write((uint32)&gEduTaskletData.pDataBuf[gEduTaskletData.written], (uint32) EDU_ldw);
    }

    //Reset padlen:
    gEduTaskletData.padlen = 0;    
    
    return 0;

}

/*******************************************************************************
*   Function: edu_tasklet_trig_read_intr
*
*   Parameters: 
*       none
*
*   Description:
*       Trigs read interrupt 
*
*   Returns: 
*       0: no error
*******************************************************************************/
static int edu_tasklet_trig_read_intr(void)
{
    struct brcmnand_chip *this = gEduTaskletData.pMtd->priv;
 
    /*   
    L_OFF_T sliceOffset = __ll_and32(gEduTaskletData.offset, ~ (gEduTaskletData.pMtd->eccsize - 1));
    
    if (unlikely(__ll_isub(gEduTaskletData.offset, sliceOffset))) {
        printk(KERN_ERR "%s: offset %s is not cache aligned, sliceOffset=%s, CacheSize=%d\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg, gEduTaskletData.offset, this->xor_invert_val), __ll_sprintf(brcmNandMsg,sliceOffset, this->xor_invert_val), gEduTaskletData.pMtd->eccsize);
        return -EINVAL;
    }
    */
    this->ctrl_writeAddr(this, gEduTaskletData.offset, 0);
    __sync(); //PLATFORM_IOFLUSH_WAR(); 

    //Start interrupt:
    this->EDU_read((uint32)&gEduTaskletData.pDataBuf[gEduTaskletData.dataRead], (uint32) EDU_ldw);
    
    return 0;        
}

/*******************************************************************************
*   Function: edu_tasklet_wait_for_completion
*
*   Parameters: 
*       none
*
*   Description:
*       Puts calling thread to sleep and returns error when it wakes up
*
*   Returns: 
*       0: no error
*       -EECCUNCOR: Ecc uncorrectable

*******************************************************************************/
static int edu_tasklet_wait_for_completion(void)
{
	int ret=1;
	unsigned long to_jiffies = 3*HZ; /* 3 secs */
	int cmd;
	
    ret = wait_event_timeout(gEduTaskletWaitQ, gEduTaskletData.opComplete, to_jiffies);

	cmd = gEduTaskletData.cmd;
	gEduTaskletData.cmd = -1;

	if (!gEduTaskletData.opComplete && ret <= 0) 
	{
		if (ret == -ERESTARTSYS) 
		{
			return ret;  // Retry on Read
		}	
		else if (ret == 0) 
		{ 
			//gEduTaskletData.opComplete = 1;
			printk("%s: timedout\n", __FUNCTION__);
			return 0; // Timed Out
		}
	
        printk(KERN_DEBUG"%s: EDU completes but intr_status is %08x and error is %d\n", 
            __FUNCTION__, 
                gEduTaskletData.intr_status,
                    gEduTaskletData.error );
	}
	
	
	return gEduTaskletData.error;
}
/*********************************EOF******************************************/
