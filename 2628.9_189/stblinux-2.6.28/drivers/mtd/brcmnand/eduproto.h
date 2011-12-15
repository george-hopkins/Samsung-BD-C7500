/**
 * @par Copyright Information:
 *      Copyright (C) 2007, Broadcom Corporation.
 *      All Rights Reserved.
 *
 * @file eduproto.h
 * @author Jean Roberge
 *
 * @brief Prototypes for EDU Support Software
 */

#ifndef _EDUPROTO_H_
#define _EDUPROTO_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/jiffies.h>


#include <linux/timer.h>

#include <asm/brcmstb/brcm97440b0/bchp_nand.h>
#include <asm/brcmstb/brcm97440b0/bchp_common.h>
#include <asm/brcmstb/brcm97440b0/bchp_hif_intr2.h>

#include <asm/io.h>
#include <asm/bug.h>

#include <linux/mtd/brcmnand.h> 

/******************************************************************************
* Prototyping
*******************************************************************************/
extern uint32_t tb_r(uint32_t);
extern void tb_w(uint32_t, uint32_t);
extern uint32_t tb_poll(uint32_t, uint32_t, uint32_t);
extern int byteSmoosh(int);

extern void issue_command(uint32_t, uint32_t, uint8);
extern void get_edu_status(void);
extern void edu_check_data(uint32_t, uint32_t, uint32_t);

extern void edu_init(struct mtd_info* mtd, edu_configuration_t eduConf);
extern void edu_reset_done(void);
extern uint32_t edu_get_error_status_register(void);
extern uint32_t edu_volatile_read(uint32_t);
extern void edu_volatile_write(uint32_t, uint32_t);

extern void EDU_CLRI(void);

extern int edu_tasklet_trig_read(struct mtd_info* mtd,
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset, 
                      int len,
                      int* pDataRead,
                      int* pOobRead);

extern int edu_tasklet_trig_write(struct mtd_info* mtd,
                      int startslice,  //column
                      void* buffer, 
                      uint8_t* oobarea, 
                      L_OFF_T offset,
                      int len,
                      int padlen);
                      
extern int edu_tasklet_getWriteStatus(int *outp_needBBT);
                      
extern uint32_t ISR_getStatus(void);
extern uint32_t ISR_cache_is_valid(uint32_t clearMask);
extern uint32_t ISR_wait_for_completion(void);


#endif // _EDUPROTO_H_
