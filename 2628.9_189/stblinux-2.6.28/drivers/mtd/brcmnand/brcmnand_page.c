/*
 * drivers/mtd/brcmnand/brcmnand_poll.c
 *
 *  Copyright (c) 2009 Broadcom Corp.
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
 * 051011    mfi    derived functions from brcmnand_base.c
 */
/*****************************************************************************/


#include <linux/mtd/brcmnand.h>
#include "brcmnand_priv.h"
#include "edu.h"
#include "eduproto.h"


/* sort total number of ecc bits corrected in terms of 1 ,2, 3 bits error corrected  */
extern unsigned long g_numeccbits_corrected_cnt[BRCMNAND_ECC_BCH_8];
extern unsigned long g_total_ecc_bits_corrected_cnt;
extern unsigned long g_prev_total_bits_corrected_cnt;

/************* External Variables (from brcmnand_base.c) *********************/
extern int gdebug;
extern char brcmNandMsg[];
extern uint32_t EDU_ldw;
extern bool runningInA7601A;

extern eduTaskletData_t gEduTaskletData;

extern struct brcmnand_driver thisDriver;
/******************** Public Function Prototypes *****************************/

int brcmnand_write_page (struct mtd_info *mtd, L_OFF_T offset, int len,
    u_char* data_buf, u_char* oob_buf);
int brcmnand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen);
int brcmnand_read_page(struct mtd_info* mtd, 
        L_OFF_T offset, u_char* data_buf, int len, u_char* oob_buf, 
        struct nand_oobinfo* oobsel, int* retlen, int* retooblen, 
        bool bPerformRefresh);

/******************** Private Function Prototypes *****************************/


//poll & intr-specific:

static int brcmnand_EDU_cache_is_valid(struct mtd_info* mtd,  int state, loff_t offset, int raw, uint32_t intr_status);

static int brcmnand_posted_read_cache(struct mtd_info* mtd, void* buffer, u_char* oobarea, L_OFF_T offset, int raw);
static int brcmnand_posted_write_cache(struct mtd_info *mtd, const void* buffer, const unsigned char* oobarea, L_OFF_T offset);

static int brcmnand_EDU_write_is_complete(struct mtd_info *mtd, int* outp_needBBT);

static int brcmnand_select_write_is_complete(struct mtd_info *mtd, int* outp_needBBT);

static int brcmnand_read_page_common_poll_isr(struct mtd_info* mtd,
                      void* buffer, 
                      uint8_t* pOobBuf, 
                      L_OFF_T offset, 
                      int len,
                      int* pDataRead,
                      int* pOobRead);


static int brcmnand_write_page_common_poll_isr(struct mtd_info* mtd,
                      int startslice,  //column
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset,
                      int len,
                      int padlen);
//edu_tasklet-specific:
static int brcmnand_edu_tasklet_select_write_is_complete(struct mtd_info *mtd, int* outp_needBBT);

static int brcmnand_edu_tasklet_read_cache(struct mtd_info* mtd,
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset, 
                      int ignoreBBT);

static int brcmnand_process_error(struct mtd_info* mtd,  uint8_t* buffer, 
        uint8_t* oobarea, int ecc);

static int brcmnand_process_read_isr_status(struct brcmnand_chip *this, 
                uint32_t intr_status);

static int brcmnand_process_write_isr_status(struct brcmnand_chip *this, 
                uint32_t intr_status, int* outp_needBBT);

//Others:
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0
static bool brcmnand_check_if_erased(struct mtd_info* mtd, uint8_t* oobarea);
#endif
/******************** Private Functions ***************************************/



static int brcmnand_edu_tasklet_read_cache(struct mtd_info* mtd,
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset, 
                      int ignoreBBT)
{
    int dataRead;
    int oobRead;

    int ret = edu_tasklet_trig_read(mtd, buffer, oobarea, offset, mtd->eccsize, &dataRead, &oobRead);

    return ret;
}

/* 
 * Returns: Good: >= 0
 *            Error:  < 0
 *
#define BRCMNAND_CORRECTABLE_ECC_ERROR       ( 2)
#define BRCMNAND_SUCCESS                     ( 1)

#define BRCMNAND_UNCORRECTABLE_ECC_ERROR     (-2)
#define BRCMNAND_TIMED_OUT                   (-3)

#define BRCMEDU_CORRECTABLE_ECC_ERROR        ( 4)
#define BRCMEDU_UNCORRECTABLE_ECC_ERROR      (-4)
#define BRCMEDU_MEM_BUS_ERROR                (-5)
 */
static int brcmnand_EDU_cache_is_valid(struct mtd_info* mtd,  int state, loff_t offset, int raw, uint32_t intr_status) 
{
    struct brcmnand_chip * this = mtd->priv;
    
    int error = this->process_read_isr_status(this, intr_status);

    return error;
}

/**
 * brcmnand_posted_read_cache - [BrcmNAND Interface] Read the 512B cache area
 * Assuming brcmnand_get_device() has been called to obtain exclusive lock
 * @param mtd        MTD data structure
 * @param oobarea    Spare area, pass NULL if not interested
 * @param buffer    the databuffer to put/get data, pass NULL if only spare area is wanted.
 * @param offset    offset to read from or write to, must be 512B aligned.
 * @param raw: Ignore BBT bytes when raw = 1
 *
 * Caller is responsible to pass a buffer that is
 * (1) large enough for 512B for data and optionally an oobarea large enough for 16B.
 * (2) 4-byte aligned.
 *
 * Read the cache area into buffer.  The size of the cache is mtd-->eccsize and is always 512B.
 */
static int brcmnand_posted_read_cache(struct mtd_info* mtd, 
        void* buffer, u_char* oobarea, L_OFF_T offset, int raw)
{

    int ecc, intr_status;

    struct brcmnand_chip* this = mtd->priv;
    L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
    int i, ret = 0, error = 0;
    int retries = 5, done = 0;
    uint32_t* p32 = (uint32_t*) oobarea;
    
if (gdebug > 3) {
printk("%s: offset=%s, oobarea=%p\n", __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val), oobarea);}

    intr_status = 0;
    
    while (retries > 0 && !done) 
    {
        if (unlikely(__ll_isub(offset, sliceOffset))) {
            printk(KERN_ERR "%s: offset %s is not cache aligned, sliceOffset=%s, CacheSize=%d\n", 
                __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val), __ll_sprintf(brcmNandMsg,sliceOffset, this->xor_invert_val), mtd->eccsize);
            return -EINVAL;
        }

        this->ctrl_writeAddr(this, sliceOffset, 0);
        PLATFORM_IOFLUSH_WAR(); 

        intr_status = this->EDU_read((uint32)buffer, (uint32) EDU_ldw);

        // Wait until cache is filled up
        ecc = brcmnand_EDU_cache_is_valid(mtd, FL_READING, offset, raw, intr_status);

        if (p32) 
        {
            PLATFORM_IOFLUSH_WAR(); 
            for (i = 0; i < 4; i++) 
            {
                p32[i] = be32_to_cpu (this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + i*4));
            }
            if (gdebug) {printk("%s: offset=%s, oob=\n", 
                                __FUNCTION__, __ll_sprintf(brcmNandMsg,sliceOffset, this->xor_invert_val)); 
                                print_oobbuf(oobarea, 16);}
        }
        error = this->process_error(mtd, buffer, oobarea, ecc);
        
        if (error == 0)
        {
            done = 1;
        }
        else if (error == -ETIMEDOUT)
        {
            retries--;
        }
        else if ((error == -EECCUNCOR) || (error == -EINVAL))
        {
            ret = -EBADMSG; 
            done = 1;
        }
        else if (error == -EECCCOR)
        {
            done = 1;
            if (ret != -EBADMSG)
            {
                ret = 0;    //Ecc correctable error fixed
            }
        }

    }//End of while()

    return ret;
}

/*******************************************************************************
*   Function: edu_tasklet_processError
*
*   Parameters: 
*       ecc: IN:  Error returned by edu_tasklet_processIsrStatus()
*
*   Description:
*       Processes the error returned by edu_tasklet_processIsrStatus()
*
* Returns: 
*   0: no error
*   -EECCCOR:   Ecc correctable
*   -EECCUNCOR: Ecc uncorrectable
*******************************************************************************/
static int brcmnand_process_error(struct mtd_info* mtd,  uint8_t* buffer, 
        uint8_t* oobarea, int ecc)
{
    int ret = 0, bit_cnt;  //defaults to success!
    struct brcmnand_chip * this = mtd->priv;
    
#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0            
    bool bErased = false;
    uint32_t* p32 = NULL;
    uint8_t oobbuf[16];
    int i;

#else //#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0                

    /* bit level stats for ecc errors */
    g_total_ecc_bits_corrected_cnt = this->ctrl_read(BCHP_NAND_READ_ERROR_COUNT);
#endif

    switch (ecc) 
    {
        case BRCMNAND_SUCCESS: /* Success, no errors */
        {
            ret = 0;            // Success!
        }
        break;

        case BRCMNAND_CORRECTABLE_ECC_ERROR:
        case BRCMEDU_CORRECTABLE_ECC_ERROR:
        {
            // NAND CTRL BUG... EDU has stopped! Must copy buffer!!!
            if (buffer)
            {
                memcpy((void*)buffer, 
                    this->vbase, 
                    mtd->eccsize);
            }
            
            //printk(KERN_ERR "%s: CORRECTABLE ERROR.\n", __FUNCTION__);
            this->stats.eccCorrectableErrorsNow++;

            /* bit level stats for ecc errors */
            bit_cnt =  g_total_ecc_bits_corrected_cnt - g_prev_total_bits_corrected_cnt;
            
            if(bit_cnt && (bit_cnt <= BRCMNAND_ECC_BCH_8)) {
                g_numeccbits_corrected_cnt[bit_cnt - 1]++; 
            }
            
            g_prev_total_bits_corrected_cnt = g_total_ecc_bits_corrected_cnt;

            ret = -EECCCOR;
        }
        break;
        
        case BRCMEDU_DMA_TRANSFER_ERROR:
                    // NAND CTRL BUG... EDU has stopped! Must copy buffer!!!
            if (buffer)
            {
                memcpy((void*)buffer, 
                    this->vbase, 
                    mtd->eccsize);
            }
            ret = 0;
            /* just make counter current */
            g_prev_total_bits_corrected_cnt = g_total_ecc_bits_corrected_cnt;
        break;
        
        case BRCMNAND_UNCORRECTABLE_ECC_ERROR:
        case BRCMEDU_UNCORRECTABLE_ECC_ERROR:
        {
            // NAND CTRL BUG... EDU has stopped! Must copy buffer!!!
            if (buffer)
            {
                memcpy((void*)buffer, 
                    this->vbase, 
                    mtd->eccsize);
            }
               
#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0            
            printk(KERN_ERR"%s: NAND UNCORRECTABLE @tasklet offset: %08X\n", 
                    __FUNCTION__, (int)gEduTaskletData.offset);
                
            this->stats.eccUncorrectableErrorsNow++;
            ret = -EECCUNCOR;   //Ecc uncorrectable error!

#else //#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0            

            /* Flash chip returns errors 

            || There is a bug in the controller, where if one reads from an erased block that has NOT been written to,
            || this error is raised.  
            || (Writing to OOB area does not have any effect on this bug)
            || The workaround is to also look into the OOB area, to see if they are all 0xFF
        
            */
            
            if (!oobarea) 
            {
                oobarea = &oobbuf[0];
                p32 = (uint32_t*) oobarea;

                for (i = 0; i < 4; i++) 
                {   
                    p32[i] = be32_to_cpu (this->ctrl_read(BCHP_NAND_SPARE_AREA_READ_OFS_0 + i*4));
                }
            }
            
            bErased = brcmnand_check_if_erased(mtd, oobarea);
            if(bErased == true)
            {
                ret = 0;    //No error!
            }
            else
            {
                printk(KERN_ERR"%s: NAND UNCORRECTABLE @tasklet offset: %08X\n", 
                    __FUNCTION__, (int)gEduTaskletData.offset);
                
                this->stats.eccUncorrectableErrorsNow++;
                ret = -EECCUNCOR;   //Ecc uncorrectable error!
            }
#endif //#if CONFIG_MTD_BRCMNAND_VERSION >= CONFIG_MTD_BRCMNAND_VERS_3_0                        

            
            /* just make counter current */
            g_prev_total_bits_corrected_cnt = g_total_ecc_bits_corrected_cnt;

        }
        break;
            
        case BRCMNAND_TIMED_OUT:
        case BRCMEDU_MEM_BUS_ERROR:
        {
            //Read has timed out 
            printk(KERN_ERR "%s: Time Out.\n", __FUNCTION__);
            ret = -ETIMEDOUT;
            
        }
        /* just make counter current */
        g_prev_total_bits_corrected_cnt = g_total_ecc_bits_corrected_cnt;

        break;

        default:
        {
            /* Should never get here */
            ret = -EINVAL;
            BUG_ON(1);
        }
        break; 
    }//End of switch()
    return ret;
}








/**
 * brcmnand_posted_write - [BrcmNAND Interface] Write a buffer to the flash cache
 * Assuming brcmnand_get_device() has been called to obtain exclusive lock
 *
 * @param mtd        MTD data structure
 * @param buffer    the databuffer to put/get data
 * @param oobarea    Spare area, pass NULL if not interested
 * @param offset    offset to write to, and must be 512B aligned
 *
 * Write to the cache area TBD 4/26/06
 */
static int brcmnand_posted_write_cache(struct mtd_info *mtd,
        const void* buffer, const unsigned char* oobarea, L_OFF_T offset)
{
    uint32_t* p32;
    int i; 
    int ret;

    struct brcmnand_chip* this = mtd->priv;    
    int needBBT=0;
    L_OFF_T sliceOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));

    if (unlikely(__ll_isub(sliceOffset, offset))) 
    {
        printk(KERN_ERR "%s: offset %s is not cache aligned\n", 
            __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val));

        ret =  -EINVAL;
        goto out;
    }

    this->ctrl_writeAddr(this, sliceOffset, 0);

    if (oobarea) 
    {
        p32 = (uint32_t*) oobarea;
        if (gdebug) {printk("%s: oob=\n", __FUNCTION__); print_oobbuf(oobarea, 16);}
    }
    else 
    {
        // Fill with 0xFF if don't want to change OOB
        p32 = (uint32_t*)&this->pLocalOob[0];
        memset(p32, 0xFF, mtd->oobsize);
    }

// There's 4 32-bits registers to write to fill 16 oob bytes per 512 data bytes
    for (i = 0; i < 4; i++) 
    {
        this->ctrl_write(BCHP_NAND_SPARE_AREA_WRITE_OFS_0 + i*4, /* THT 11-30-06 */ cpu_to_be32 /* */(p32[i]));
    }

    PLATFORM_IOFLUSH_WAR(); 

    if(this->options & NAND_COMPLEX_OOB_WRITE)
    {
        uint32_t* p32 = (uint32_t*)&this->pLocalOob[0];
        memset(p32, 0xFF, mtd->oobsize);
        this->EDU_write((uint32)p32, (uint32) EDU_ldw);
    }
    else
    {
        this->EDU_write((uint32)buffer, (uint32) EDU_ldw);
    }

    // Wait until flash is ready
    ret = this->write_is_complete(mtd, &needBBT);

    if (ret != BRCMNAND_SUCCESS) 
    {
        if (!needBBT) 
        {
            ret = 0;
            goto out;
        }
        else
        { // Need BBT
            ret = this->block_markbad(mtd, offset);
            ret = -EBADBLK;
            //ret = -EINVAL;
            goto out;
        }
    }

    //Write has timed out or read found bad block. TBD: Find out which is which
    printk(KERN_INFO "%s: Timeout\n", __FUNCTION__);
    ret = -ETIMEDOUT;

out:

    return ret;
}


static int brcmnand_EDU_write_is_complete(struct mtd_info *mtd, int* outp_needBBT)
{
    uint32_t hif_err;
    struct brcmnand_chip *this = mtd->priv;
    int ret;
    
    /* For Erase op, use the controller version */
    if (thisDriver.state != FL_WRITING) {
        return this->ctrl_write_is_complete(mtd, outp_needBBT);  
    }
    
    hif_err = this->EDU_wait_for_completion();

    ret = this->process_write_isr_status(this, hif_err, outp_needBBT);

    return ret;
}

/******************** Public Functions ***************************************/
void brcmnand_pageapi_init(struct mtd_info* mtd, edu_configuration_t eduConf)
{
    struct brcmnand_chip *this = mtd->priv;

    this->process_error = brcmnand_process_error;
    this->process_read_isr_status = brcmnand_process_read_isr_status;
    this->process_write_isr_status = brcmnand_process_write_isr_status;
    
    switch(eduConf)
    {
    case EDU_POLL:
        this->read_page = brcmnand_read_page_common_poll_isr;
        this->read_raw_cache  = brcmnand_posted_read_cache;

        this->write_page = brcmnand_write_page_common_poll_isr;

        this->write_is_complete = brcmnand_select_write_is_complete;
        

        
        break;

    case EDU_INTR:
        this->read_page = brcmnand_read_page_common_poll_isr;
        this->read_raw_cache  = brcmnand_posted_read_cache;

        this->write_page = brcmnand_write_page_common_poll_isr;

        this->write_is_complete = brcmnand_select_write_is_complete;

        break;

    case EDU_TASKLET:
        this->read_page = edu_tasklet_trig_read;
        this->read_raw_cache  = brcmnand_edu_tasklet_read_cache;

        this->write_page = edu_tasklet_trig_write;

        this->write_is_complete = brcmnand_edu_tasklet_select_write_is_complete;
        break;

    default:
        printk(KERN_ERR"EDU configuration error\n");
        break;
    
    }
}


/**
 * brcmnand_read_page - [GENERIC] Read 1 page data and the page OOB into internal buffer.
 * @param mtd        MTD device structure
 * @param offset        offset from start of flash
 * @param data_buf    Data buffer to be read to, must be at least mtd->writesize
 * @param len        Length of data buffer, must be equal or less than page size
 * @param oob_buf    OOB buffer, must be at least mtd->oobsize
 * Assume that device lock has been held
 * returns0 on success
 */
int brcmnand_read_page(struct mtd_info* mtd, 
        L_OFF_T offset, u_char* data_buf, int len, u_char* oob_buf, 
        struct nand_oobinfo* oobsel, int* retlen, int* retooblen,
        bool bPerformRefresh)
{
    struct brcmnand_chip *this = mtd->priv;
    int dataRead = 0, oobRead = 0;

    int oob = 0;
    int ret;
    int autoplace; 
    
    int nPageNumber;
    L_OFF_T blockRefreshOffset;
    
    u_char* pOobBuf = NULL;

    *retlen = 0;
    *retooblen = 0;

    if (oobsel == NULL)
    {
        oobsel = this->ecclayout;
    }

    switch(oobsel->useecc) {
        case MTD_NANDECC_AUTOPLACE:
        case MTD_NANDECC_AUTOPL_USR:
            autoplace = 1;
            /* Must read into temporary oob buffer, in order to convert it */
            pOobBuf = oob_buf ? this->pLocalOob : NULL;
            break;
        default:
            autoplace = 0;
            pOobBuf = oob_buf ? oob_buf : NULL;
            break;
        }
if (gdebug) printk("--> %s: offset %s oob_buf=%p, autoplace=%d\n", 
    __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val), pOobBuf, autoplace);

    if (len > mtd->writesize || len < 0) {
        printk(KERN_ERR "%s: Invalid length %d\n", __FUNCTION__, len);
        return -EINVAL;
    }
    //Make sure all interrupts are cleared:
    EDU_CLRI();   
    ret = this->read_page(mtd, data_buf, pOobBuf, offset, len, &dataRead, &oobRead);

    if ((ret == -EECCCOR)&&(bPerformRefresh))
    {   
        if(this->ecclevel == BRCMNAND_ECC_HAMMING && runningInA7601A == true)
        {
             //Perform refresh block logic at page level:
            printk(KERN_DEBUG"%s: Calling hamming_code_fix\n", __FUNCTION__);
                    
            this->hamming_code_fix(mtd, offset, 0);

            printk(KERN_NOTICE"%s: Performing refresh block logic\n", __FUNCTION__);
                
            nPageNumber = this->deduce_blockOffset(mtd, offset, &blockRefreshOffset);

            ret = this->refresh_block(mtd, this->pLocalPage, this->pLocalOob, nPageNumber, blockRefreshOffset, true);
            
            if(ret)
            {
                ret = -EECCUNCOR;
            }
            
        }
        else
        {  
#if 1
            printk(KERN_DEBUG"%s: Performing refresh block logic\n", __FUNCTION__);
                
            nPageNumber = this->deduce_blockOffset(mtd, offset, &blockRefreshOffset);

            ret = this->refresh_block(mtd, NULL, NULL, nPageNumber, blockRefreshOffset, true);
            
            if(ret)
            {
                ret = -EECCUNCOR;
            }

#endif
        }
    }
    
    


    // Convert the free bytes into fsbuffer
    if (!oob_buf) {
        oob = 0;
    }
    else if (autoplace && pOobBuf) {

        oob = this->convert_oobfree_to_fsbuf(mtd, pOobBuf, oob_buf, oobRead, oobsel, autoplace);
    }
    else {
        oob = oobRead;
    }
        
    *retlen = dataRead;
    *retooblen = oob;

if (gdebug) printk("<-- %s, retlen=%d, retooblen=%d\n", __FUNCTION__, *retlen, *retooblen);

     return RETURN_RD_ECC_STATE(ret);
}

static int brcmnand_read_page_common_poll_isr(struct mtd_info* mtd,
                      void* buffer, 
                      uint8_t* pOobBuf, 
                      L_OFF_T offset, 
                      int len,
                      int* pDataRead,
                      int* pOobRead)
{
    int winslice;
    int ret;
    unsigned char* oobbuf;
    int thislen;

    int retFinal = 0;
    int dataRead = 0;
    int oobRead = 0;
    struct brcmnand_chip *this = mtd->priv;
    uint8_t* data_buf = (uint8_t*)buffer;

    //Make sure all interrupts are cleared:
    EDU_CLRI();   
    for (winslice = 0; winslice < this->eccsteps && dataRead < len; winslice++) 
    {
        thislen = min_t(int, len-dataRead, mtd->eccsize);

        oobbuf = (pOobBuf ? &pOobBuf[oobRead] : NULL);

        // If partial read, must use internal buffer for data.  
        if (thislen != mtd->eccsize) 
        {
            ret = this->read_raw_cache(mtd, &this->data_buf[winslice*this->eccsteps], oobbuf, 
                    __ll_add32(offset, dataRead), 0);
            memcpy(&data_buf[dataRead], &this->data_buf[winslice*this->eccsteps], thislen);
        } 
        else 
        {
            ret = this->read_raw_cache(mtd, &data_buf[dataRead], oobbuf, 
                    __ll_add32(offset, dataRead),  0);
        }
        
        switch (ret)
        {
            case 0:                   //No Error, continue!
            break;

            case -EECCCOR:            //ECC correctable error !!!
                retFinal = -EBADMSG;        //Treat it as an ECC uncorrectable error !!!
                
                printk(KERN_DEBUG "%s: ECC correctable error @addr %s\n",
                    __FUNCTION__, __ll_sprintf(brcmNandMsg, __ll_add32(offset, dataRead), this->xor_invert_val));
                    
            break;
            case -EECCUNCOR:
                retFinal = -EBADMSG;        //ECC uncorrectable error !!!
                
                printk(KERN_ERR "%s: ECC uncorrectable error @addr %s\n", 
                    __FUNCTION__, __ll_sprintf(brcmNandMsg,__ll_add32(offset, dataRead), this->xor_invert_val));
            break;

            case -EBADBLK:
                retFinal = -EBADBLK;        //bad block error!!!
                printk(KERN_ERR "%s: Bad Block @addr %s, ret=%d\n", 
                    __FUNCTION__, __ll_sprintf(brcmNandMsg,__ll_add32(offset, dataRead), this->xor_invert_val), ret);
            break;
            
            default:
                retFinal = -EBADBLK;        //Should never happen...
                printk(KERN_ERR "%s: posted read cache failed at offset=%s, ret=%d, dumping stack\n", 
                __FUNCTION__, __ll_sprintf(brcmNandMsg, __ll_add32(offset, dataRead), this->xor_invert_val), ret);
                
                dump_stack();
                
                return retFinal;
            break;    
        }
        
       //Make sure all interrupts are cleared:
        EDU_CLRI();
         
        dataRead += thislen;
        oobRead += this->eccOobSize;
        
    }
    *pDataRead = dataRead;
    *pOobRead = oobRead;

    return retFinal;
}

/**
 * brcmnand_read_raw - [PRIVATE] Read raw data including oob into buffer
 * @mtd:    MTD device structure
 * @buf:    temporary buffer
 * @from:    offset to read from
 * @len:    number of bytes to read
 * @ooblen:    number of oob data bytes to read
 *
 * Read raw data including oob into buffer.  Raw Reading ignores any BBT flag.
 */
int brcmnand_read_raw (struct mtd_info *mtd, uint8_t *buf, loff_t from, size_t len, size_t ooblen)
{
    struct brcmnand_chip *this = mtd->priv;
    int slice, ret=0, page, oobOffset;
    L_OFF_T offset, dataOffset, startPage;
    int ignoreBBT= 1;    

    if (from & (mtd->writesize - 1)) {
        printk(KERN_ERR "brcmnand_read_raw: from %s is not OOB sliceOffset aligned\n", 
            __ll_sprintf(brcmNandMsg,from, this->xor_invert_val));
        return -EINVAL;
    }
    /* len must be multiple of writesize */
    if (((len / mtd->writesize) * mtd->writesize) != len) {
        printk(KERN_ERR "%s: len %08x is not multiple of writesize %d\n", __FUNCTION__, len, mtd->writesize);
        return -EINVAL;
    }
    /* ooblen must be multiple of oobsize */
    if (((ooblen / mtd->oobsize) * mtd->oobsize) != ooblen) {
        printk(KERN_ERR "%s: ooblen %08x is not multiple of writesize %d\n", __FUNCTION__, ooblen, mtd->oobsize);
        return -EINVAL;
    }

 
    for (page=0; (page * mtd->writesize) < len; page++) {
 	    /* For each 2K page, the first 512B are for data , followed by 64B OOB data */
        
 	    startPage = page*(mtd->writesize+mtd->oobsize); // Address in buffer corresponding to start of this page
 	    for (slice = 0; slice < this->eccsteps; slice++) 
        {
 	        offset = from + (page * mtd->writesize) + slice*mtd->eccsize; // offset of data w.r.t. flash
 	        dataOffset = startPage + slice*mtd->eccsize; 	  	
 	        oobOffset = startPage + mtd->writesize + slice*this->eccOobSize; 	  	
     	
            ret = this->read_raw_cache(mtd, &buf[dataOffset], 
                &buf[oobOffset], offset, ignoreBBT); /* Ignore BBT */
/*
            if (ret) {
 	            printk(KERN_ERR "%s failed at offset = %s, dataOffset=%s, oobOffset=%d\n",
 	                __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val), __ll_sprintf(brcmNandMsg,dataOffset, this->xor_invert_val), oobOffset);
 	            return ret;
 	        }
*/ 	        
 	    } 
    }
    return ret;
}


/**
 * brcmnand_write_page - [GENERIC] write one page
 * @mtd:    MTD device structure
 * @offset:     offset  from start of the this, must be called with (offset & this->pagemask)
 * @len: size of data buffer, len must be <= mtd->writesize, and can indicate partial page.
 * @data_buf: data buffer.
 * @oob_buf:    out of band data buffer for 1 page (size = oobsize), must be prepared beforehand
 *
 * Nand_page_program function is used for write and writev !
 * This function will pad the buffer to a Cache boundary before writing it out.
 *
 * Cached programming is not supported yet.
 */

int brcmnand_write_page (struct mtd_info *mtd, L_OFF_T offset, int len,
    u_char* data_buf, u_char* oob_buf)
{

    struct brcmnand_chip *this = mtd->priv;
    int ret;
    L_OFF_T pageOffset, columnOffset;
    int column;
    int padlen = 0, trailer;

    /* Use the passed in buffer */
    this->data_poi = data_buf;
    /* page marks the start of the page encompassing [from, thislen] */
    pageOffset = __ll_and32( offset,  ~ (mtd->writesize - 1));
    /* column marks the start of the 512B cache slice encompassing [from, thislen] */
    columnOffset = __ll_and32(offset, ~ (mtd->eccsize - 1));
    // Make sure that we cast to int as mod op on long long yield unresolved symbols
    column = ((int) __ll_isub(columnOffset , pageOffset)) % mtd->eccsize;
    /* Pad to the front if not starting on cache slice boundary */
    padlen = __ll_isub(offset, columnOffset);
    if (padlen) {
        trailer = min_t(int, len, mtd->writesize - padlen);

        this->data_poi = this->data_buf;

        memset(this->pLocalOob,0xFF, mtd->oobsize);
        memcpy(this->data_buf, this->pLocalOob, padlen);
        // Make it simple stupid, just copy the remaining part of a page
        memcpy(&this->data_buf[padlen], data_buf, trailer);
        // Adjust offset accordingly
        // written = - padlen;
    }
    //Make sure all interrupts are cleared:
    EDU_CLRI(); 
    ret = this->write_page(mtd, column, this->data_poi, oob_buf, offset, len, padlen);

    return ret;
}

static int brcmnand_write_page_common_poll_isr(struct mtd_info* mtd,
                      int column,  //startSlice
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset,
                      int len,
                      int padlen)
{
    int winslice;
    int done = 0;
    int ret;

    int written = 0, oobWritten = 0;
    struct brcmnand_chip *this = mtd->priv;


    if (gdebug) printk("%s: 30\noffset=%s\n", __FUNCTION__, __ll_sprintf(brcmNandMsg, offset, this->xor_invert_val));

    //Make sure all interrupts are cleared:
    EDU_CLRI(); 
    for (winslice = column; winslice < this->eccsteps && written < len && !done; winslice++) {
        int thislen = min_t(int, len-written, mtd->eccsize - padlen);
        //int i;

        // Pad at the end ?
        if (thislen != mtd->eccsize - padlen) {
            int fillupLen = ((padlen+len - written + mtd->eccsize) & ~(mtd->eccsize - 1)) - (padlen+len);
            done = 1; // Writing last slice
            //Pad with 0xff before writing
            memset(this->pLocalPage, 0xFF, fillupLen);
            memcpy(&this->data_poi[padlen+len], this->pLocalPage, fillupLen);
        }

        if (oobarea) {

            ret = brcmnand_posted_write_cache(mtd, &this->data_poi[written], &oobarea[oobWritten], 
                    __ll_add32(offset, written-padlen));
        }
        else {

            ret = brcmnand_posted_write_cache(mtd, &this->data_poi[written], NULL, 
                    __ll_add32(offset, written-padlen));
        }

        //Make sure all interrupts are cleared:
        EDU_CLRI();

        if (ret) {
            printk(/*KERN_ERR */ "%s: posted write cache failed at offset=%s, ret=%d\n", 
                __FUNCTION__, __ll_sprintf(brcmNandMsg,offset, this->xor_invert_val) + written, ret);
            return ret;
        }

        written += thislen;
        if (oobarea)
            oobWritten += this->eccOobSize;
        padlen = 0;

    }

    return 0;
}



int brcmnand_select_write_is_complete(struct mtd_info *mtd, int* outp_needBBT)
{
    int ret = 0;
    
    struct brcmnand_chip *this = mtd->priv;
    //Check the state of the current lock of the device: 
    if (thisDriver.state == FL_WRITING)
    {
        ret = brcmnand_EDU_write_is_complete(mtd, outp_needBBT);
    }
    else
    {
        //Get the flash_status value:
        ret = this->ctrl_write_is_complete(mtd, outp_needBBT);
    }
    
    return ret;
}

int brcmnand_edu_tasklet_select_write_is_complete(struct mtd_info *mtd, int* outp_needBBT)
{
    int ret = 0;
    struct brcmnand_chip *this = mtd->priv;

    if (thisDriver.state == FL_WRITING)
    {
    #if 0  // mfi : Not needed as this is done as part of edu_tasklet_trig_write:
        ret = edu_tasklet_getWriteStatus(outp_needBBT);
    #endif // end of mfi
    }
    else
    {
        //Get the flash_status value:
        ret = this->ctrl_write_is_complete(mtd, outp_needBBT);
    }

    return ret;
}

/*******************************************************************************
*   Function: brcmnand_process_read_isr_status
*
*   Parameters: 
*       intr_status: IN:  interrupt status
*
*   Description:
*       Processes the interrupt status and returns error or success
*
* Returns: Good: >= 0
*            Error:  < 0
*
* #define BRCMNAND_CORRECTABLE_ECC_ERROR       ( 2)
* #define BRCMNAND_SUCCESS                     ( 1)
*
* #define BRCMNAND_UNCORRECTABLE_ECC_ERROR     (-2)
* #define BRCMNAND_TIMED_OUT                   (-3)
* 
* #define BRCMEDU_CORRECTABLE_ECC_ERROR        ( 4)
* #define BRCMEDU_UNCORRECTABLE_ECC_ERROR      (-4)
* #define BRCMEDU_MEM_BUS_ERROR                (-5)
*******************************************************************************/
static int brcmnand_process_read_isr_status(struct brcmnand_chip *this, 
                uint32_t intr_status)
{
    int error = BRCMNAND_SUCCESS;
    //printk(KERN_DEBUG"%s: intr_status = %08x\n", __FUNCTION__, intr_status);

    if (intr_status == 0) 
    {
        /* EDU_read timed out */
        printk(KERN_DEBUG"%s: intr_status=0 TIMEOUT\n", __FUNCTION__);
        error = BRCMNAND_TIMED_OUT;
        goto out;
    }
    else
    {
        error = this->verify_ecc(this, FL_READING, intr_status);
    }

out:
        
    EDU_CLRI();
    return error;
}

static int brcmnand_process_write_isr_status(struct brcmnand_chip *this, 
                uint32_t intr_status, int* outp_needBBT)
{
    int ret = BRCMNAND_SUCCESS;   //success
    int flashStatus;
    int edu_err; 
     
    *outp_needBBT = 0;

    if(intr_status != 0)//No timeout 
    {
        flashStatus = this->ctrl_read(BCHP_NAND_INTFC_STATUS);
        
        //Get status:  should we check HIF_INTR2_ERR?
        edu_err = edu_get_error_status_register();

        /* sanity check on last cmd status */
        if ((edu_err & EDU_ERR_STATUS_NandWrite) && !(flashStatus & 0x1)) {
            int cmd = this->ctrl_read(BCHP_NAND_CMD_START);
            printk(KERN_ERR"%s: false EDU write error status (edu_err: 0x%08X, flashStatus: 0x%08X) for NAND CMD %x  \n", 
                   __FUNCTION__, edu_err, flashStatus, cmd);
        }

        /* we primarily rely on NAND controller FLASH_STATUS bit 0, since EDU error may not be cleared yet */
        if ((edu_err & EDU_ERR_STATUS_NandWrite) && (flashStatus & 0x01)) {
            /* Write did not complete, flash error, will mark block bad */
            *outp_needBBT = 1;
            printk(KERN_ERR"%s: flash write error (edu_err: 0x%08X, flashStatus: 0x%08X), \n", 
                __FUNCTION__, edu_err, flashStatus);
            ret = BRCMEDU_UNCORRECTABLE_ECC_ERROR;
            goto out;
            
        }
        else if (edu_err & EDU_ERR_STATUS_NandRBUS) {
            /* Write did not complete, bus error, will mark block bad  */
            *outp_needBBT = 1;
            printk(KERN_ERR"%s: bus error (0x%08X)\n", __FUNCTION__, edu_err);
            ret = BRCMEDU_MEM_BUS_ERROR;
            goto out;
            
        }
/*We are checking for a write command result, so we do not need 
            to check for ECC errors (they come only when we do a read)*/
#if 0            
            if (*outp_needBBT & EDU_ERR_STATUS_NandECCcor)
            {
                printk(KERN_ERR "%s(): Nand ECC Correctable Error\n", __FUNCTION__);
            }
            
            if (*outp_needBBT & EDU_ERR_STATUS_NandECCuncor)
            {
                printk(KERN_ERR "%s(): Nand ECC Uncorrectable Error\n", __FUNCTION__);
            }
#endif          
        ret = BRCMNAND_SUCCESS;
    }
    else
    {// Write timeout
        printk(KERN_ERR"%s: Write has timed out\n", __FUNCTION__);
        //*outp_needBBT = 1;
        ret = BRCMNAND_TIMED_OUT;
    }
out:
    EDU_CLRI();
    return ret; 
}

#if CONFIG_MTD_BRCMNAND_VERSION < CONFIG_MTD_BRCMNAND_VERS_3_0
static bool brcmnand_check_if_erased(struct mtd_info* mtd, uint8_t* oobarea)
{
    int eccbyte;
    
    bool bRet = false;
    int all00 = 1;
    int allFF = 1;
   
    struct brcmnand_chip * this = mtd->priv;
    
    //mfi: Use ecclayout because the ecc bytes aren't the same for hammin vs bch4.
    for ( eccbyte = 0; 
          eccbyte < (this->ecclayout->eccbytes / this->eccsteps); 
          eccbyte++)
    {   
        all00 = ((oobarea[this->ecclayout->eccpos[eccbyte]] == 0xff)&& (all00)) ? 1 : 0;
        allFF = ((oobarea[this->ecclayout->eccpos[eccbyte]] == 0x00)&& (allFF)) ? 1 : 0;
    }
    if ( all00 || allFF) {
        /* 
         * For the first case, the slice is an erased block, and the ECC bytes are all 0xFF,
         * for the 2nd, all bytes are 0xFF, so the Hamming Codes for it are all zeroes.
         * The current version of the BrcmNAND controller treats these as un-correctable errors.
         * For either case, return success. 
         * (The data buffer should already be filled with 0x00 or 0xff.)  
         * Note: The error has already been cleared inside brcmnand_verify_ecc.
         * Both cases will be handled correctly by the BrcmNand controller in later releases.
         */
        bRet = true;  //Erased.
    }
    else 
    {
        /* Real error: Disturb read returns uncorrectable errors */

        if (gdebug) printk("%s: ECC uncorrectable error\n",
            __FUNCTION__);
    }
    return bRet;
}
#endif
