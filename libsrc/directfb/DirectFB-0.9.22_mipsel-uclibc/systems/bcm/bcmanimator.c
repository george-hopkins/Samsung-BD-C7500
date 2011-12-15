/** @mainpage
 *  @date April 2006
 *
 *  Copyright (c) 2006-2009, Broadcom Corporation
 *     All Rights Reserved
 *     Confidential Property of Broadcom Corporation
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 *  This is the core system module for the Broadcom Settop API. This library is
 *  proprietary since the DirectFB project is LGPL and this module will be loaded
 *  at runtime.
 */

#include <directfb.h>
#include <direct/debug.h>

#include "bcmcore.h"
#include "bcmanimator.h"

#include <limits.h>
#include <string.h>

const char BCMAnimatorModuleName[] = "DirectFB/BCMAnimator";

extern BCMCoreContext * dfb_bcmcore_context;

#if D_DEBUG_ENABLED
/*#define ENABLE_MORE_DEBUG*/
#define REALTIME_DEBUG
#endif

#define STC_FREQ 45000

uint32_t pts_increments[] = {
    STC_FREQ*1000/29970, /* bdvd_dfb_ext_frame_rate_unknown */
    STC_FREQ*1000/23976, /* bdvd_dfb_ext_frame_rate_23_976 */
    STC_FREQ*1000/24000, /* bdvd_dfb_ext_frame_rate_24     */
    STC_FREQ*1000/25000, /* bdvd_dfb_ext_frame_rate_25     */
    STC_FREQ*1000/29970, /* bdvd_dfb_ext_frame_rate_29_97  */
    STC_FREQ*1000/50000, /* bdvd_dfb_ext_frame_rate_50     */
    STC_FREQ*1000/59940  /* bdvd_dfb_ext_frame_rate_59_94  */
};

uint32_t gfx_stc_incr[] = {
    STC_FREQ*1000/29970, /*bdvd_video_format_ntsc      */
    STC_FREQ*1000/29970, /*bdvd_video_format_ntsc_japan*/
    STC_FREQ*1000/25000, /*bdvd_video_format_pal       */
    STC_FREQ*1000/25000, /*bdvd_video_format_pal_m     */
    STC_FREQ*1000/25000, /*bdvd_video_format_pal_n     */
    STC_FREQ*1000/25000, /*bdvd_video_format_pal_nc    */
    STC_FREQ*1000/25000, /*bdvd_video_format_secam     */
    STC_FREQ*1000/59940, /*bdvd_video_format_480p      */
    STC_FREQ*1000/59940, /*bdvd_video_format_576p      */
    STC_FREQ*1000/29970, /*bdvd_video_format_1080i     */
    STC_FREQ*1000/50000, /*bdvd_video_format_1080i_50hz*/
    STC_FREQ*1000/59940, /*bdvd_video_format_1080p     */
    STC_FREQ*1000/24000, /*bdvd_video_format_1080p_24hz*/
    STC_FREQ*1000/29970, /*bdvd_video_format_1080p_30hz*/
    STC_FREQ*1000/59940, /*bdvd_video_format_720p      */
    STC_FREQ*1000/50000, /*bdvd_video_format_720p_50hz */
    STC_FREQ*1000/25000, /*bdvd_video_format_1080p_25hz*/
    STC_FREQ*1000/50000  /*bdvd_video_format_1080p_50hz*/
};

static struct {
    bool video_running; /* The video source decode handle */
    bdvd_dfb_ext_frame_rate_e framerate;
    uint32_t current_pts;
} video_info;

/* I don't want an additional mutex for the animator, so these functions should only be
 * used in BCMMixer when the mixer is locked.
 */

DFBResult
BCMAnimatorOpen(BCMAnimatorHandle animator) {
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMAnimatorOpen called\n",
            BCMAnimatorModuleName );

    D_ASSERT(animator != NULL);

    D_DEBUG("%s/BCMAnimatorOpen exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );

    return ret;
}

DFBResult
BCMAnimatorSetVideoRate(BCMAnimatorHandle animator,
                        bdvd_dfb_ext_frame_rate_e framerate) {
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMAnimatorSetVideoRate called\n",
            BCMAnimatorModuleName );

    /* We don't care about the animator */
    /*D_ASSERT(animator != NULL);*/

    if (framerate == bdvd_dfb_ext_frame_rate_unknown) {
        D_ERROR("%s/BCMAnimatorSetVideoRate: invalid frame rate\n",
                BCMAnimatorModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    video_info.framerate = framerate;

quit:

    D_DEBUG("%s/BCMAnimatorSetVideoRate exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );

    return ret;
}

DFBResult
BCMAnimatorSetVideoStatus(BCMAnimatorHandle animator,
                          bool video_running) {
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMAnimatorSetVideoStatus called\n",
            BCMAnimatorModuleName );

    /* We don't care about the animator */
    /*D_ASSERT(animator != NULL);*/

    video_info.video_running = video_running;

    D_DEBUG("%s/BCMAnimatorSetVideoStatus exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );

    return ret;
}

DFBResult
BCMAnimatorUpdateCurrentPTS(BCMAnimatorHandle animator, uint32_t current_pts) {
    DFBResult ret = DFB_OK;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorUpdateCurrentPTS called\n",
            BCMAnimatorModuleName );
#endif

    /* We don't care about the animator */
    /*D_ASSERT(animator != NULL);*/

    video_info.current_pts = current_pts;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorUpdateCurrentPTS: current pts is %u exiting with code %d\n",
            BCMAnimatorModuleName,
            video_info.current_pts,
            (int)ret );
#endif

    return ret;
}

DFBResult
BCMAnimatorFreezeState(BCMAnimatorHandle animator) {
    DFBResult ret = DFB_OK;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorFreezeState called\n",
            BCMAnimatorModuleName );
#endif

    D_ASSERT(animator != NULL);

    animator->frozen_pts = video_info.current_pts;

quit:

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorFreezeState exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );
#endif

    return ret;
}

DFBResult
BCMAnimatorThawState(BCMAnimatorHandle animator) {
    DFBResult ret = DFB_OK;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorThawState called\n",
            BCMAnimatorModuleName );
#endif

    D_ASSERT(animator != NULL);

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorThawState exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );
#endif

    return ret;
}

DFBResult
BCMAnimatorLayerInitAnimation(BCMAnimatorHandle animator,
                              BCMCoreLayerHandle layer) {
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMAnimatorLayerInitAnimation called\n",
            BCMAnimatorModuleName );

    D_ASSERT(animator != NULL);
    D_ASSERT(layer != NULL);

    D_DEBUG("%s/BCMAnimatorLayerInitAnimation exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );

    return ret;
}

static int default_repeat_sequence[] = {1};

DFBResult
BCMAnimatorLayerAnimate(BCMAnimatorHandle animator,
                        BCMCoreLayerHandle layer) {
    DFBResult ret = DFB_OK;

    bdvd_dfb_ext_faa_params_t * faa_params = NULL;
    bdvd_dfb_ext_faa_state_t * faa_state = NULL;

    int * repeat_sequence;
    size_t repeat_sequence_size;

    static uint32_t gfx_stc_pts_delta = 0;

    uint32_t gfx_frame_pts_incr;

    bool faa_timer_set;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorLayerAnimate called\n",
            BCMAnimatorModuleName );
#endif

    D_ASSERT(animator != NULL);
    D_ASSERT(layer != NULL);

    switch (layer->dfb_ext_type) {
        case bdvd_dfb_ext_bd_j_image_faa_layer:
            faa_params = &layer->dfb_ext_settings.image_faa.faa_params;
            faa_state = &layer->dfb_ext_settings.image_faa.faa_state;
        break;
        case bdvd_dfb_ext_bd_j_sync_faa_layer:
            faa_params = &layer->dfb_ext_settings.sync_faa.faa_params;
            faa_state = &layer->dfb_ext_settings.sync_faa.faa_state;
        break;
        default:
            D_ERROR("%s/BCMAnimatorLayerAnimate: invalid layer type\n",
                    BCMAnimatorModuleName );
            ret = DFB_FAILURE;
            goto quit;
    }

    switch (faa_state->trigger) {
        case bdvd_dfb_ext_faa_trigger_none:
        break;
        case bdvd_dfb_ext_faa_trigger_time:
            /* Shouldn't we put some kind of threshold? */
            D_DEBUG("%s/BCMAnimatorLayerAnimate: Got trigger time message\n", BCMAnimatorModuleName);
            D_DEBUG("%s/BCMAnimatorLayerAnimate: video_running: %d\n", BCMAnimatorModuleName, video_info.video_running);
            D_DEBUG("%s/BCMAnimatorLayerAnimate: animator->frozen_pts: %d\n", BCMAnimatorModuleName, animator->frozen_pts);
            D_DEBUG("%s/BCMAnimatorLayerAnimate: faa_state->startPTS: %d\n", BCMAnimatorModuleName, faa_state->startPTS);
            D_DEBUG("%s/BCMAnimatorLayerAnimate: faa_state->stopPTS: %d\n", BCMAnimatorModuleName, faa_state->stopPTS);
            if ((!video_info.video_running) ||
                (animator->frozen_pts < faa_state->startPTS) ||
                (animator->frozen_pts >= faa_state->stopPTS)) {
                D_DEBUG("%s/BCMAnimatorLayerAnimate: Hiding layer\n", BCMAnimatorModuleName);
                if ((animator->frozen_pts >= faa_state->stopPTS) && 
                    (layer->bcmcore_visibility == BCMCORELAYER_VISIBLE)) {
                    layer->dfb_ext_force_clean_on_stop = true;
                }
                layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
                goto quit;
            }
            else {
                D_DEBUG("%s/BCMAnimatorLayerAnimate: showing layer\n", BCMAnimatorModuleName);
                layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
            }
        break;
        case bdvd_dfb_ext_faa_trigger_forced_start:
            /* MAY CHANGE */
            D_DEBUG("%s/BCMAnimatorLayerAnimate: Got force start message\n", BCMAnimatorModuleName);
            faa_timer_set = !(faa_state->startPTS == 0 && faa_state->stopPTS == 0);
            if (faa_timer_set) {
                if ((!video_info.video_running) ||
                    (animator->frozen_pts < faa_state->startPTS) ||
                    (animator->frozen_pts >= faa_state->stopPTS)) {
                    D_DEBUG("%s/BCMAnimatorLayerAnimate: Hiding layer\n", BCMAnimatorModuleName);
                    if ((animator->frozen_pts >= faa_state->stopPTS) && 
                        (layer->bcmcore_visibility == BCMCORELAYER_VISIBLE)) {
                        layer->dfb_ext_force_clean_on_stop = true;
                    }
                    layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
                    goto quit;
                }
                else {
                    D_DEBUG("%s/BCMAnimatorLayerAnimate: showing layer\n", BCMAnimatorModuleName);
                    layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
                }
            }
            else {
                D_DEBUG("%s/BCMAnimatorLayerAnimate: showing layer\n", BCMAnimatorModuleName);
                layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
            }
        break;
        case bdvd_dfb_ext_faa_trigger_forced_stop:
            /* MAY CHANGE */
            D_DEBUG("%s/BCMAnimatorLayerAnimate: Got force stop message\n", BCMAnimatorModuleName);
            D_DEBUG("%s/BCMAnimatorLayerAnimate: Hiding layer\n", BCMAnimatorModuleName);
            layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
        break;
        default:
            D_ERROR("%s/BCMAnimatorLayerAnimate: invalid trigger type\n",
                    BCMAnimatorModuleName );
            ret = DFB_FAILURE;
            goto quit;
    }

    if ((layer->dfb_ext_type == bdvd_dfb_ext_bd_j_image_faa_layer) &&
         layer->dfb_ext_settings.image_faa.image_faa_params.lockedToVideo &&
         !video_info.video_running) {
        D_DEBUG("%s/BCMAnimatorLayerAnimate: Hiding image FAA layer\n", BCMAnimatorModuleName);
        if (layer->bcmcore_visibility == BCMCORELAYER_VISIBLE) {
            layer->dfb_ext_force_clean_on_stop = true;
        }
        layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
        goto quit;
    }

    if (layer->bcmcore_visibility == BCMCORELAYER_VISIBLE) {
        /*
         * Graphics animations run at the video frame rate if video exists, or if no video 
         * exists they run at a default frame rate specified when the animation is created.  
         * Normal A/V TSM determines when to advance the frame by comparing the PTS of the 
         * next frame to a free-runing system STC. For graphics we don't have an STC if 
         * video isn't running, so in our case we use either the video PTS as the STC or 
         * we generate a virtual STC by incrementing a counter by the amount of 45kHz ticks 
         * in a vsync period. Since we really only care about the STC to PTS delta our 
         * counter only tallies that count.  
         */
        gfx_stc_pts_delta = video_info.video_running ? 
            animator->frozen_pts-layer->dfb_ext_animation_stats.currentPTS :
            gfx_stc_pts_delta + gfx_stc_incr[dfb_bcmcore_context->video_format];
        D_DEBUG("%s/BCMAnimatorLayerAnimate: gfx_stc_pts_delta: 0x%08x\n", BCMAnimatorModuleName, gfx_stc_pts_delta);

        gfx_frame_pts_incr = video_info.video_running ? pts_increments[video_info.framerate] : pts_increments[faa_state->default_framerate ? faa_state->default_framerate : bdvd_dfb_ext_frame_rate_unknown];
        D_DEBUG("%s/BCMAnimatorLayerAnimate: gfx_frame_pts_incr: 0x%08x\n", BCMAnimatorModuleName, gfx_frame_pts_incr);

        /* If IFAA and locked to video */
        if ((layer->dfb_ext_type == bdvd_dfb_ext_bd_j_image_faa_layer) &&
            layer->dfb_ext_settings.image_faa.image_faa_params.lockedToVideo) {
            if (animator->frozen_pts < layer->dfb_ext_animation_stats.currentPTS) {
                /* reset everything */
                D_DEBUG("%s/BCMAnimatorLayerAnimate: resetting image FAA\n", BCMAnimatorModuleName);
                memset(&layer->animation_context, 0, sizeof(layer->animation_context));
                memset(&layer->dfb_ext_animation_stats, 0, sizeof(layer->dfb_ext_animation_stats));
                layer->animation_context.increment_sign = 1;
            }

            if (layer->dfb_ext_animation_stats.isAnimated) {
                D_DEBUG("%s/BCMAnimatorLayerAnimate: isAnimated\n", BCMAnimatorModuleName);
                gfx_stc_pts_delta = animator->frozen_pts-layer->dfb_ext_animation_stats.currentPTS;
                D_DEBUG("%s/BCMAnimatorLayerAnimate: gfx_stc_pts_delta: 0x%08x\n", BCMAnimatorModuleName, gfx_stc_pts_delta);
                D_DEBUG("%s/BCMAnimatorLayerAnimate: frozen_pts: 0x%08x\n", BCMAnimatorModuleName, animator->frozen_pts);
                D_DEBUG("%s/BCMAnimatorLayerAnimate: currentPTS: 0x%08x\n", BCMAnimatorModuleName, layer->dfb_ext_animation_stats.currentPTS);
            }
            else {
                /* From the start */
                D_DEBUG("%s/BCMAnimatorLayerAnimate: From the start\n", BCMAnimatorModuleName);
                gfx_stc_pts_delta = animator->frozen_pts-faa_state->startPTS;
                D_DEBUG("%s/BCMAnimatorLayerAnimate: gfx_stc_pts_delta: 0x%08x\n", BCMAnimatorModuleName, gfx_stc_pts_delta);
                D_DEBUG("%s/BCMAnimatorLayerAnimate: frozen_pts: 0x%08x\n", BCMAnimatorModuleName, animator->frozen_pts);
                D_DEBUG("%s/BCMAnimatorLayerAnimate: startPTS: 0x%08x\n", BCMAnimatorModuleName, faa_state->startPTS);
            }
        }

        while (gfx_stc_pts_delta >= gfx_frame_pts_incr) {
            D_DEBUG("gfx_stc_pts_delta %u gfx_frame_pts_incr %u\n", gfx_stc_pts_delta, gfx_frame_pts_incr);
            switch (layer->dfb_ext_type) {
                case bdvd_dfb_ext_bd_j_image_faa_layer:
                    D_DEBUG("%s/BCMAnimatorLayerAnimate: Running image FAA\n", BCMAnimatorModuleName);

                    if (((layer->animation_context.increment_sign < 0) &&
                        (layer->animation_context.next_front_surface_index == 0)) ||
                        ((layer->animation_context.increment_sign > 0) &&
                        (layer->animation_context.next_front_surface_index == layer->nb_surfaces))) {

                         switch (layer->dfb_ext_settings.image_faa.image_faa_state.playmode) {
                             case bdvd_dfb_ext_image_faa_play_once:
                                layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
                                goto quit;
                            break;
                            case bdvd_dfb_ext_image_faa_play_alternating:
                                layer->animation_context.increment_sign *= -1;
                                if (layer->animation_context.increment_sign < 0) {
                                    layer->animation_context.next_front_surface_index -= 2;
                                    if (layer->animation_context.next_front_surface_index < 0) {
                                        layer->animation_context.next_front_surface_index = 0;
                                    }
                                }
                                else {
                                    layer->animation_context.next_front_surface_index = 0;
                                }
                                layer->animation_context.increment = 0;
                            break;
                            case bdvd_dfb_ext_image_faa_play_repeating:
                                layer->animation_context.next_front_surface_index = 0;
                                layer->animation_context.increment = 0;
                            break;
                            default:
                                D_ERROR("%s/BCMAnimatorLayerAnimate: invalid play mode\n",
                                        BCMAnimatorModuleName );
                                ret = DFB_FAILURE;
                                goto quit;
                        }
                    }

                    layer->idle_surface_index = layer->front_surface_index = layer->animation_context.next_front_surface_index;
                    if ((layer->dfb_core_surface->front_buffer = layer->surface_buffers[layer->front_surface_index]) == NULL) {
                        D_ERROR("%s/BCMAnimatorLayerAnimate: surface buffer is NULL\n",
                                BCMAnimatorModuleName);
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                    layer->dfb_core_surface->idle_buffer = layer->dfb_core_surface->front_buffer;

                    if ((layer->bcmcore_visibility == BCMCORELAYER_VISIBLE)) {
                        /* do repeat sequence stuff */
                        repeat_sequence = faa_params->repeatCount && faa_params->repeatCountSize ? faa_params->repeatCount : default_repeat_sequence;
                        repeat_sequence_size = faa_params->repeatCount && faa_params->repeatCountSize ? faa_params->repeatCountSize : sizeof(default_repeat_sequence)/sizeof(int);
                        layer->animation_context.increment =
                            (layer->animation_context.repeat_sequence_count+1 <
                            repeat_sequence[layer->animation_context.repeat_sequence_index])
                            ? 0 : 1;

                        /*layer->animation_context.increment  = 1;*/
                        /* This should have been checked in BCMCoreLayerSet */
                        D_ASSERT(repeat_sequence[layer->animation_context.repeat_sequence_index] != 0);

                        if (layer->animation_context.increment) {
                            if (layer->animation_context.repeat_sequence_index+1 < repeat_sequence_size) {
                                layer->animation_context.repeat_sequence_index++;
                            }
                            else {
                                layer->animation_context.repeat_sequence_index = 0;
                            }
                            layer->animation_context.repeat_sequence_count = 0;
                            layer->animation_context.next_front_surface_index +=
                                layer->animation_context.increment*layer->animation_context.increment_sign;
                            layer->dfb_ext_animation_stats.displayedFrameCount++;
                        }
                        else {
                            layer->animation_context.repeat_sequence_count++;
                            if (layer->dfb_ext_animation_stats.isAnimated) {
                                /* 
                                 * For now there is no need to update the animation unless 
                                 * we were previously not showing the animation, then we want to 
                                 * show whatever we are on. 
                                 */
                                ret = DFB_INVAREA;
                            }
                        }

                        layer->dfb_ext_animation_stats.displayPosition = layer->front_surface_index;
                    }
                break;
                case bdvd_dfb_ext_bd_j_sync_faa_layer:
                    D_DEBUG("%s/BCMAnimatorLayerAnimate: Running sync FAA\n", BCMAnimatorModuleName);
                    if ((layer->bcmcore_visibility == BCMCORELAYER_VISIBLE)) {
                        /* Check if there is repeat information and we have displayed a frame */
                        if (faa_params->repeatCount && faa_params->repeatCountSize) {
                            /*D_ASSERT(repeat_sequence[layer->animation_context.repeat_sequence_index] != 0);*/
                            layer->frame_mixing_time++;
                            if (++layer->animation_context.repeat_sequence_count >=
                                faa_params->repeatCount[layer->animation_context.repeat_sequence_index]) {
                                layer->animation_context.repeat_sequence_index = (layer->animation_context.repeat_sequence_index + 1) % faa_params->repeatCountSize;
                                layer->animation_context.repeat_sequence_count = 0;
                                layer->dfb_ext_animation_stats.expectedOutputFrame++;
                                layer->animation_context.frame_advance_expected = true;
                            }
                        }
                        else {
                            layer->frame_mixing_time++;
                            layer->dfb_ext_animation_stats.expectedOutputFrame++;
                            layer->animation_context.frame_advance_expected = true;
                        }

                        /* Check if we should advance a frame and if there is an available frame */
                        if (layer->animation_context.frame_advance_expected) {
                            if (layer->dfb_ext_animation_stats.totalFinishedFrames >
                                layer->dfb_ext_animation_stats.displayedFrameCount) {
                                /* Advance the frame */
								uint32_t new_front_index = layer->front_surface_index + 1;
								if (new_front_index >= layer->nb_surfaces)
									new_front_index = 0;

								if (new_front_index != layer->back_surface_index) {

									layer->idle_surface_index = layer->front_surface_index = new_front_index;
									if ((layer->dfb_core_surface->front_buffer = layer->surface_buffers[layer->front_surface_index]) == NULL) {
										D_ERROR("%s/BCMAnimatorLayerAnimate: surface buffer is NULL\n",
												BCMAnimatorModuleName);
										ret = DFB_FAILURE;
										goto quit;
									}

									layer->dfb_core_surface->idle_buffer = layer->dfb_core_surface->front_buffer;
									layer->dfb_ext_animation_stats.displayPosition = layer->front_surface_index;
									layer->dfb_ext_animation_stats.displayedFrameCount++;
									layer->animation_context.frame_advance_expected = false;
								}
								else {
									D_ERROR("%s/BCMAnimatorLayerAnimate: SyncFAA reached back surface...not receiving frames fast enough!!\n", BCMAnimatorModuleName);
								}
                            }
							else {
								D_ERROR("%s/BCMAnimatorLayerAnimate: SyncFAA not enough finished frames!!\n", BCMAnimatorModuleName);
							}
                        }
                    }
                break;
                default:
                    D_ERROR("%s/BCMAnimatorLayerAnimate: invalid layer type\n",
                            BCMAnimatorModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
            }
            gfx_stc_pts_delta -= gfx_frame_pts_incr;
        }
    }

quit:

    layer->dfb_ext_animation_stats.currentPTS = animator->frozen_pts-gfx_stc_pts_delta;

    if (ret == DFB_OK) {
        if (layer->bcmcore_visibility == BCMCORELAYER_HIDDEN) {
            /* for now there is no animation */
            ret = DFB_INVAREA;
            layer->dfb_ext_animation_stats.isAnimated = false;
            layer->animation_context.increment = 0;
        }
        else {
            layer->dfb_ext_animation_stats.isAnimated = true;
        }
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMAnimatorLayerAnimate exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );
#endif

    return ret;
}

DFBResult
BCMAnimatorClose(BCMAnimatorHandle animator) {
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMAnimatorClose called\n",
            BCMAnimatorModuleName );

    D_ASSERT(animator != NULL);

    /* */
    video_info.video_running = false;

    D_DEBUG("%s/BCMAnimatorClose exiting with code %d\n",
            BCMAnimatorModuleName,
            (int)ret );

    return ret;
}
