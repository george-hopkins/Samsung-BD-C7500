
#ifndef ___BCMGFX_STATE_H__
#define ___BCMGFX_STATE_H__

#include "bcmgfx.h"

void bcmgfx_set_destination( BCMGfxDriverData *bcmgfxdrv,
                             BCMGfxDeviceData *bcmgfxdev,
                             CardState        *state );

void bcmgfx_set_source( BCMGfxDriverData *bcmgfxdrv,
                        BCMGfxDeviceData *bcmgfxdev,
                        CardState        *state );

void bcmgfx_set_clip( BCMGfxDriverData *bcmgfxdrv,
                      BCMGfxDeviceData *bcmgfxdev,
                      CardState        *state );

void bcmgfx_set_color( BCMGfxDriverData *bcmgfxdrv,
                       BCMGfxDeviceData *bcmgfxdev,
                       CardState        *state );

void bcmgfx_set_src_colorkey( BCMGfxDriverData *bcmgfxdrv,
                              BCMGfxDeviceData *bcmgfxdev,
                              CardState        *state );

void bcmgfx_set_dst_colorkey( BCMGfxDriverData *bcmgfxdrv,
                              BCMGfxDeviceData *bcmgfxdev,
                              CardState        *state );

bool bcmgfx_check_blittingflags( BCMGfxDriverData *bcmgfxdrv,
                                 BCMGfxDeviceData *bcmgfxdev,
                                 CardState        *state,
                                 bool apply );

void bcmgfx_set_blittingflags( BCMGfxDriverData *bcmgfxdrv,
                               BCMGfxDeviceData *bcmgfxdev,
                               CardState        *state );

bool bcmgfx_check_drawingflags( BCMGfxDriverData *bcmgfxdrv,
                                BCMGfxDeviceData *bcmgfxdev,
                                CardState        *state,
                                bool apply );

void bcmgfx_set_drawingflags( BCMGfxDriverData *bcmgfxdrv,
                              BCMGfxDeviceData *bcmgfxdev,
                              CardState        *state );

#endif
