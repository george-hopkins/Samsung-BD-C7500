/** 
 * Copyright (c) 2006, Broadcom Corporation
 *    All Rights Reserved
 *    Confidential Property of Broadcom Corporation
 *
 * THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 * AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 * EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 */
#ifndef __BCMANIMATOR_H__
#define __BCMANIMATOR_H__

#include <bdvd.h>

#include "bcmlayer.h"

typedef struct BCMAnimator *BCMAnimatorHandle;

struct BCMAnimator {
    uint32_t frozen_pts;
    uint32_t frozen_stc;
};

DFBResult BCMAnimatorOpen(BCMAnimatorHandle animator);
DFBResult BCMAnimatorSetVideoRate(BCMAnimatorHandle animator,
                                  bdvd_dfb_ext_frame_rate_e framerate);
DFBResult BCMAnimatorSetVideoStatus(BCMAnimatorHandle animator,
                                    bool video_running);
DFBResult BCMAnimatorLayerInitAnimation(BCMAnimatorHandle animator,
                                        BCMCoreLayerHandle layer);
DFBResult BCMAnimatorUpdateCurrentPTS(BCMAnimatorHandle animator,
                                      uint32_t current_pts);
DFBResult BCMAnimatorFreezeState(BCMAnimatorHandle animator);
DFBResult BCMAnimatorLayerAnimate(BCMAnimatorHandle animator,
                                  BCMCoreLayerHandle layer);
DFBResult BCMAnimatorThawState(BCMAnimatorHandle animator);
DFBResult BCMAnimatorClose(BCMAnimatorHandle animator);

#endif /* __BCMANIMATOR_H__ */
