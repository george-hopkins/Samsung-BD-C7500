/**
 * Copyright (c) 2005-2007, Broadcom Corporation
 *    All Rights Reserved
 *    Confidential Property of Broadcom Corporation
 *
 * THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 * AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 * EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 */
#ifndef __BCMMIXER_H__
#define __BCMMIXER_H__

#include <pthread.h>

#include "bcmlayer.h"
#include "bcmanimator.h"

#include <directfb.h>

#include <core/coredefs.h>
#include <core/layers.h>
#include <core/screens.h>

typedef enum {
    BCM_MIXER_0,
    BCM_MIXER_1,
    BCM_MIXER_2,
    BCM_MIXER_L
} BCMMixerId;

typedef struct BCMMixer *BCMMixerHandle;

typedef struct {
    BCMMixerHandle mixer;
    bool cancelled;
} mixer_thread_arguments_t;

struct BCMMixer {

    BCMMixerId mixerId;

    bdvd_graphics_compositor_h driver_compositor;
    bdvd_graphics_compositor_h mixer_compositor;

    uint32_t nb_compositors;

    bool mixerflipblit;

    /* CHANGE THIS COMMENT, for now use two arrays of pointer to
     * BCMCoreLayer with max size MAX_LAYERS.
     * The elements are dynamically allocated.
     */
    BCMCoreLayerHandle mixed_layers[MAX_LAYERS];
    BCMCoreLayerHandle mixed_layers_levelsorted[MAX_LAYERS];

    uint32_t max_layers;

    /* This variable is for internal use only, should
     * be put in a private structure, access must be
     * done with locking of the mutex.
     */
    uint32_t nb_layers_levelsorted;


    BCMCoreLayerHandle output_layer;

    BCMAnimatorHandle animator;

    /* Sync stuff */
    pthread_mutex_t mutex;
    /* Must be recursive because all the functions are synchronised with this
     * mutex, we don't want the owning thread to deadlock itself
     */
    pthread_mutexattr_t mutex_attr;

    pthread_t mixer_thread;
    pthread_attr_t mixer_thread_attr;
    mixer_thread_arguments_t mixer_thread_arguments;

    bool finished;

    pthread_mutex_t cond_mutex;
    pthread_mutexattr_t cond_mutex_attr;

    pthread_cond_t cond;
    pthread_condattr_t cond_attr;

    bool level_changed;

    /* Clearrect */
    bdvd_dfb_ext_clear_rect_t subpic_cr;
    bool                      subpic_cr_dirty;
    DFBRectangle              subpic_cr_union;
    bool                      subvid_cr_dirty;
    uint32_t                  subvid_cr_num;
    bdvd_rect_t               subvid_cr[DFB_EXT_MAX_CLEARRECTS];
    bool                      cr_notify;
};

DFBResult BCMMixerOpen(BCMMixerHandle mixer, BCMCoreLayerHandle output_layer);

DFBResult BCMMixerStart(BCMMixerHandle mixer);

DFBResult BCMMixerLayerRegister(BCMMixerHandle mixer,
                                BCMCoreLayerHandle ret_layer,
                                DFBDisplayLayerID *layer_id);

DFBResult BCMMixerOutputLayerAcquire(BCMMixerHandle mixer,
                                     BCMCoreLayerHandle * ret_layer);

DFBResult BCMMixerLayerAcquire(BCMMixerHandle mixer,
                               DFBDisplayLayerID layer_id,
                               BCMCoreLayerHandle * ret_layer);

DFBResult BCMMixerSetLayerLocation(BCMMixerHandle mixer,
                                   DFBDisplayLayerID layer_id,
                                   const DFBLocation * location,
                                   const DFBRectangle * destination,
                                   uint32_t screen_width,
                                   uint32_t screen_height,
                                   bool mixer_locked);

DFBResult BCMMixerSetDisplayDimensions(BCMMixerHandle mixer,
                                       BCMCoreLayerHandle output_layer,
                                       bool set_mixer);

DFBResult BCMMixerLayerSetLevel(BCMMixerHandle mixer,
                                DFBDisplayLayerID layer_id,
                                int level);

DFBResult BCMMixerLayerGetLevel(BCMMixerHandle mixer,
                                DFBDisplayLayerID layer_id,
                                int * level);

DFBResult BCMMixerLayerFlip(BCMMixerHandle mixer,
                            DFBDisplayLayerID layer_id,
                            const DFBRegion * update,
                            bool waitparam,
                            DFBSurfaceFlipFlags flags);

DFBResult BCMMixerLayerRelease(BCMCoreLayerHandle layer);

DFBResult BCMMixerLayerUnregister(BCMMixerHandle mixer,
                                  DFBDisplayLayerID layer_id,
                                  bool noerror_missing);

DFBResult BCMMixerStop(BCMMixerHandle mixer);

DFBResult BCMMixerClose(BCMMixerHandle mixer);

DFBResult BCMMixerAttachAnimator(BCMMixerHandle mixer);

DFBResult BCMMixerAnimatorSetVideoRate(BCMMixerHandle mixer,
                                       bdvd_dfb_ext_frame_rate_e framerate);

DFBResult BCMMixerAnimatorSetVideoStatus(BCMMixerHandle mixer,
                                         bool video_running);

DFBResult BCMMixerAnimatorUpdateCurrentPTS(BCMMixerHandle mixer,
                                           uint32_t current_pts);

DFBResult BCMMixerDetachAnimator(BCMMixerHandle mixer);

DFBResult BCMMixerSetOutputLayerSurfaces(BCMMixerHandle mixer,
                                         BCMCoreLayerHandle output_layer);

DFBResult BCMMixerWaitFinished(BCMMixerHandle mixer, BCMCoreLayerHandle layer);

DFBResult BCMMixerSignalFinished(BCMMixerHandle mixer);

DFBResult BCMMixerSetClearrect(BCMMixerHandle mixer, DFBDisplayLayerID layer_id, bdvd_dfb_ext_clear_rect_t *clearrect);


static inline bdvd_video_format_e
display_get_video_format(bdvd_display_h display) {
    bdvd_display_settings_t display_settings;

    bdvd_display_get(display, &display_settings);

    return display_settings.format;
}

#endif /* __BCMMIXER_H__ */
