/** @mainpage
 *  @date May 2006
 *
 *  Copyright (c) 2005-2010, Broadcom Corporation
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

//#define ENABLE_MIX_TRACES 1
//#define T_DEBUG(x...) printf(x)
#define T_DEBUG(x...)

#include <directfb.h>

#include <direct/debug.h>
#include <direct/mem.h>

#include <misc/util.h>

#include <core/layers.h>
#include <core/surfaces.h>

#include "bcmcore.h"
#include "bcmmixer.h"
#include "bcmcore_fcts.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifdef BDVD_PRCTL
#include <sys/prctl.h>
#endif

/* This must be done after including debug.h */
#if D_DEBUG_ENABLED
/* #define ENABLE_MORE_DEBUG */
#define REALTIME_DEBUG
/* #define ENABLE_MEMORY_DUMP */
#endif

/* Why the hell did I use a const char instead of a define....
 * Makes using debug macros complex for nothing... This is not C++, anyway too late.
 */

const char BCMMixerModuleName[] = "DirectFB/BCMMixer";

extern BCMCoreContext * dfb_bcmcore_context;

static DFBResult BCMMixerSyncFlip(BCMMixerHandle mixer, bool wait_vsync);

static void * mixer_thread_routine(void * arg);

//////////////////////////////// STATIC ////////////////////////////////////////

static int
BCMMixerLayerLevelCompare (const void *key,
                           const void *base)
{
    /* Let's place all the null elements of the array at the end */

    if (*(uintptr_t *)key == NULL && *(uintptr_t *)base == NULL) {
        return MAX_LAYERS;
    }
    else if (*(uintptr_t *)key == NULL) {
        return (MAX_LAYERS - ((const BCMCoreLayerHandle)(*(uintptr_t *)base))->level);
    }
    else if (*(uintptr_t *)base == NULL) {
        return (((const BCMCoreLayerHandle)(*(uintptr_t *)key))->level - MAX_LAYERS);
    }
    else {
        return (((const BCMCoreLayerHandle)(*(uintptr_t *)key))->level - ((const BCMCoreLayerHandle)(*(uintptr_t *)base))->level);
    }
}

/* When using a layer, please lock prior to doing the call.
 * For now, the mixer does not require to be locked, but this may
 * change in the future. If this would change, be aware that
 * a potential deadlock could occur in the mixer thread.
 */
static inline DFBResult
BCMMixerBlit(
    bdvd_graphics_compositor_h compositor,
    bdvd_graphics_surface_h dest,
    bdvd_graphics_surface_h source1,
    bdvd_graphics_surface_h source2,
    const DFBRectangle * dfb_dest_rect,
    const DFBRectangle * dfb_source_rect1,
    const DFBRectangle * dfb_source_rect2,
    bdvd_graphics_blending_rule_e blending_rule,
    bool use_fixed_scaling,
    double horiz_scale,
    double vert_scale,
    double horiz_phase,
    double vert_phase) {

    DFBResult ret = DFB_OK;
    bresult err;
/* Casting directly the DFBRectangle into bsettop_rect
 * while this is not safe, because of sign convertion,
 * I don't expect we go that far with width and height
 * anyway.
    bsettop_rect source_rect = {0,0,0,0};
    bsettop_rect dest_rect = {0,0,0,0};
 */

#ifdef ENABLE_MORE_DEBUG
    uint32_t i;
    uint8_t * surfacemem_cached;
    uint8_t * surfacemem_uncached;
#endif

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerBlit called with:\n"
            "bdvd_graphics_blending_rule_e %d\n",
            BCMMixerModuleName,
            blending_rule);
#endif

    D_ASSERT(dfb_source_rect1 != NULL);
    D_ASSERT(dfb_dest_rect != NULL);

    if (dest == NULL) {
        D_ERROR( "%s/BCMMixerBlit: dest surface cannot be NULL\n",
                 BCMMixerModuleName);
        ret = DFB_INVARG;
        goto error;
    }

    if (source1 == NULL) {
        D_ERROR( "%s/BCMMixerBlit: source1 surface cannot be NULL\n",
                 BCMMixerModuleName);
        ret = DFB_INVARG;
        goto error;
    }

    /* Maybe this check is not necessary, depends on the blit fct */
    if (dest == source1) {
        D_ERROR( "%s/BCMMixerBlit: dest and source surfaces are the same\n",
                 BCMMixerModuleName);
        ret = DFB_INVARG;
        goto error;
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerBlit source1 %p (x %d y %d width %u height %u)\n",
            BCMMixerModuleName,
            source1,
            dfb_source_rect1->x,
            dfb_source_rect1->y,
            dfb_source_rect1->w,
            dfb_source_rect1->h);

    D_DEBUG("%s/BCMMixerBlit source2 %p (x %d y %d width %u height %u)\n",
            BCMMixerModuleName,
            source2,
            dfb_source_rect2 ? dfb_source_rect2->x : 0,
            dfb_source_rect2 ? dfb_source_rect2->y : 0,
            dfb_source_rect2 ? dfb_source_rect2->w : 0,
            dfb_source_rect2 ? dfb_source_rect2->h : 0);

    D_DEBUG("%s/BCMMixerBlit dest %p (x %d y %d width %u height %u)\n",
            BCMMixerModuleName,
            dest,
            dfb_dest_rect->x,
            dfb_dest_rect->y,
            dfb_dest_rect->w,
            dfb_dest_rect->h);
#endif

#ifdef ENABLE_MEMORY_DUMP
    /* A crude memory dump, I should do a functions better than that */
    surfacemem_cached = dest->mem.buffer;
    surfacemem_uncached = b_mem2noncached(&b_board.sys.mem, surfacemem_cached);

    D_DEBUG("Cached destination dump:\n");
    for (i = 0; i < dest->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_cached[i]);
    }

    D_DEBUG("\n");

    D_DEBUG("Uncached destination dump:\n");
    for (i = 0; i < dest->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_uncached[i]);
    }

    D_DEBUG("\n");

    surfacemem_cached = source1->mem.buffer;
    surfacemem_uncached = b_mem2noncached(&b_board.sys.mem, surfacemem_cached);

    D_DEBUG("Cached source dump:\n");
    for (i = 0; i < source1->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_cached[i]);
    }

    D_DEBUG("\n");

    D_DEBUG("Uncached source dump:\n");
    for (i = 0; i < source1->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_uncached[i]);
    }

    D_DEBUG("\n");
#endif

    switch (blending_rule) {
        case BGFX_RULE_PORTERDUFF_SRC:
        /* This copies the source and sets the destinations alpha channel
           to opaque. */
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMMixerBlit: BGFX_RULE_PORTERDUFF_SRC\n",
                    BCMMixerModuleName );
#endif
            if ((err = bdvd_graphics_surface_blit(
                          compositor,
                          dest, (bsettop_rect *)dfb_dest_rect,
                          BGFX_RULE_PORTERDUFF_SRC,
                          BGFX_RULE_PORTERDUFF_SRC,
                          source1, (bsettop_rect *)dfb_source_rect1,
                          BGFX_POLARITY_FRAME,
                          NULL, NULL,
                          0,
                          0,
                          false,
                          false,
                          false,
                          use_fixed_scaling,
                          horiz_scale,
                          vert_scale,
                          horiz_phase,
                          vert_phase)) != b_ok) {
                D_ERROR( "%s/BCMMixerBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_PORTERDUFF_SRC %d\n",
                         BCMMixerModuleName,
                         err );
                ret = DFB_FAILURE;
                goto error;
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRCOVER:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMMixerBlit: BGFX_RULE_PORTERDUFF_SRCOVER\n",
                    BCMMixerModuleName );
#endif
            if ((err = bdvd_graphics_surface_blit(
                          compositor,
                          dest, (bsettop_rect *)dfb_dest_rect,
                          BGFX_RULE_PORTERDUFF_SRCOVER,
                          BGFX_RULE_PORTERDUFF_SRCOVER,
                          source1, (bsettop_rect *)dfb_source_rect1,
                          BGFX_POLARITY_FRAME,
                          source2 ? source2 : dest, source2 ? (bsettop_rect *)dfb_source_rect2 : (bsettop_rect *)dfb_dest_rect,
                          0,
                          0,
                          false,
                          false,
                          false,
                          use_fixed_scaling,
                          horiz_scale,
                          vert_scale,
                          horiz_phase,
                          vert_phase)) != b_ok) {
                D_ERROR( "%s/BCMMixerBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_PORTERDUFF_SRCOVER %d\n",
                         BCMMixerModuleName,
                         err );
                ret = DFB_FAILURE;
                goto error;
            }
        break;
        case BGFX_RULE_SRCALPHA_ZERO:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMMixerBlit: BGFX_RULE_SRCALPHA_ZERO\n",
                    BCMMixerModuleName );
#endif
            if ((err = bdvd_graphics_surface_blit(
                          compositor,
                          dest, (bsettop_rect *)dfb_dest_rect,
                          BGFX_RULE_PORTERDUFF_SRC,
                          BGFX_RULE_SRCALPHA_ZERO,
                          source1, (bsettop_rect *)dfb_source_rect1,
                          BGFX_POLARITY_FRAME,
                          NULL, NULL,
                          0,
                          0,
                          false,
                          false,
                          false,
                          use_fixed_scaling,
                          horiz_scale,
                          vert_scale,
                          horiz_phase,
                          vert_phase)) != b_ok) {
                D_ERROR( "%s/BCMMixerBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRCALPHA_ZERO %d\n",
                         BCMMixerModuleName,
                         err );
                ret = DFB_FAILURE;
                goto error;
            }
        break;
        case BGFX_RULE_SRCALPHA_INVSRCALPHA:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMMixerBlit: BGFX_RULE_SRCALPHA_INVSRCALPHA\n",
                    BCMMixerModuleName );
#endif
            if ((err = bdvd_graphics_surface_blit(
                          compositor,
                          dest, (bsettop_rect *)dfb_dest_rect,
                          BGFX_RULE_PORTERDUFF_SRCOVER,
                          BGFX_RULE_SRCALPHA_INVSRCALPHA,
                          source1, (bsettop_rect *)dfb_source_rect1,
                          BGFX_POLARITY_FRAME,
                          source2 ? source2 : dest, source2 ? (bsettop_rect *)dfb_source_rect2 : (bsettop_rect *)dfb_dest_rect,
                          0,
                          0,
                          false,
                          false,
                          false,
                          use_fixed_scaling,
                          horiz_scale,
                          vert_scale,
                          horiz_phase,
                          vert_phase)) != b_ok) {
                D_ERROR( "%s/BCMMixerBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRCALPHA_INVSRCALPHA %d\n",
                         BCMMixerModuleName,
                         err );
                ret = DFB_FAILURE;
                goto error;
            }
        break;
        default:
            D_ERROR( "%s/BCMMixerBlit: invalid blending_rule %d\n",
                     BCMMixerModuleName,
                     blending_rule );
            ret = DFB_INVARG;
            goto error;
        break;
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerBlit exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;

error:
    D_DEBUG("%s/BCMMixerBlit source1 %p (x %d y %d width %u height %u)\n",
            BCMMixerModuleName,
            source1,
            dfb_source_rect1->x,
            dfb_source_rect1->y,
            dfb_source_rect1->w,
            dfb_source_rect1->h);

    D_DEBUG("%s/BCMMixerBlit source2 %p (x %d y %d width %u height %u)\n",
            BCMMixerModuleName,
            source2,
            dfb_source_rect2 ? dfb_source_rect2->x : 0,
            dfb_source_rect2 ? dfb_source_rect2->y : 0,
            dfb_source_rect2 ? dfb_source_rect2->w : 0,
            dfb_source_rect2 ? dfb_source_rect2->h : 0);

    D_DEBUG("%s/BCMMixerBlit dest %p (x %d y %d width %u height %u)\n",
            BCMMixerModuleName,
            dest,
            dfb_dest_rect->x,
            dfb_dest_rect->y,
            dfb_dest_rect->w,
            dfb_dest_rect->h);

    return ret;

}

/* You can ignore the cleanup return value */
static DFBResult BCMMixerCleanup(BCMMixerHandle mixer) {

    int rc = 0;
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMMixerCleanup called\n",
            BCMMixerModuleName);

    D_ASSERT( mixer != NULL );

    /* pthread_mutex_destroy  actually does nothing except checking that the
     * mutex is unlocked.
     */
    if ((rc = pthread_mutex_destroy(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerCleanup: error detroying mutex    --> %s",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* continue */
    }

    /* pthread_mutexattr_destroy does nothing in the LinuxThreads implementation,
     * but we never know, could be used in NPTL
     */
    if ((rc = pthread_mutexattr_destroy(&mixer->mutex_attr))) {
        D_ERROR("%s/BCMMixerCleanup: error detroying mutex_attr    --> %s",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

/* When this thing dies, the fcts of this class should responds an error */
static void * mixer_thread_routine(void * arg) {

    mixer_thread_arguments_t * mixer_thread_arguments = (mixer_thread_arguments_t *)arg;

    DFBResult ret = DFB_OK;

    /*D_INFO("mixer_thread_routine_p started: id = %d %d\n", mixer_thread_arguments->mixer->mixerId, mixer_thread_arguments->cancelled);*/

#ifdef REALTIME_DEBUG
    int i = 0;
    struct timeval now;
    uint32_t currenttime;
    uint32_t previoustime;
    uint32_t sync_count = 0;
#endif

    D_ASSERT( mixer_thread_arguments != NULL );
#ifdef BDVD_PRCTL
    prctl(PR_SET_NAME, "mixer_thread", 0, 0.,0);
#endif

    while (!mixer_thread_arguments->cancelled) {
        ret = BCMMixerSyncFlip(mixer_thread_arguments->mixer, true);
        switch (ret) {
            case DFB_OK:
#ifdef REALTIME_DEBUG
                sync_count++;
#endif
            break;
            case DFB_INVAREA:
            break;
            default:
                goto quit;
        }
#ifdef REALTIME_DEBUG
        if (!(++i % 30)) {
            gettimeofday(&now, NULL);
            currenttime = now.tv_sec * 1000 + now.tv_usec / 1000;
            fprintf(stderr, "+++++++thread count %d sync count %d current time %u, time elapsed %u+++++++\n", i, sync_count, currenttime, currenttime - previoustime);
            previoustime = currenttime;
            i = 0;
            sync_count = 0;
        }
#endif
    }

quit:

    /*D_INFO("%s/mixer_thread_routine_p exiting %d\n",
            BCMMixerModuleName, mixer_thread_arguments->mixer->mixerId);*/

    return NULL;
}

//////////////////////////// EXTERN /////////////////////////////////////

inline DFBResult BCMMixerOutputLayerAcquire(BCMMixerHandle mixer, BCMCoreLayerHandle * ret_layer) {
    int rc = 0;
    DFBResult ret = DFB_OK;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerOutputLayerAcquire called\n",
            BCMMixerModuleName );
#endif

    D_ASSERT( mixer != NULL );

    /* in case of failure */
    *ret_layer = NULL;

    /* Output layers cannot be changed, thus no need the lock the mixer */

    if ((*ret_layer = mixer->output_layer) == NULL) {
        D_ERROR("%s/BCMMixerOutputLayerAcquire: mixer output_layer is NULL    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

    /* lock the layer */
    if ((rc = pthread_mutex_lock(&(*ret_layer)->mutex))) {
        D_ERROR("%s/BCMMixerOutputLayerAcquire: error locking output layer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

quit:

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerOutputLayerAcquire exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called recursively (while owning the layer mutex).
 */
inline DFBResult BCMMixerLayerAcquire(BCMMixerHandle mixer, DFBDisplayLayerID layer_id, BCMCoreLayerHandle * ret_layer) {

    int rc = 0;
    DFBResult ret = DFB_OK;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerAcquire called with:\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            layer_id );
#endif

    D_ASSERT( mixer != NULL );

    /* Look at see if it's the output layer, this is the same as doing BCMMixerOutputLayerAcquire */
    if (layer_id == mixer->output_layer->layer_id) {
        return BCMMixerOutputLayerAcquire(mixer, ret_layer);
    }

    /* Else it may be a mixed layer */

    /* in case of failure */
    *ret_layer = NULL;

    /* No goto error before this */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerAcquire: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if (layer_id >= mixer->max_layers) {
        ret = DFB_INVARG;
        /*D_INFO("\n\nBCMMixerLayerAcquire: layer_id > mixer->nax_layer\n\n");*/
        goto quit;
    }

    /* get the layer */
    if ((*ret_layer = mixer->mixed_layers[layer_id]) == NULL) {
        D_ERROR("%s/BCMMixerLayerAcquire: layer_id %u is not a registered layer\n",
                BCMMixerModuleName,
                layer_id);
        ret = DFB_INVARG;
        goto quit;
    }

    /* lock the layer */
    if ((rc = pthread_mutex_lock(&(*ret_layer)->mutex))) {
        D_ERROR("%s/BCMMixerLayerAcquire: error locking layer_id %u mutex    --> %s\n",
                BCMMixerModuleName,
                layer_id,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

quit:

    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerAcquire: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerAcquire exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called recursively (while owning the layer mutex).
 */
inline DFBResult BCMMixerLayerAcquireMixerLocked(BCMMixerHandle mixer, DFBDisplayLayerID layer_id, BCMCoreLayerHandle * ret_layer) {

    int rc = 0;
    DFBResult ret = DFB_OK;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerAcquire called with:\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            layer_id );
#endif

    D_ASSERT( mixer != NULL );

    /* Look at see if it's the output layer, this is the same as doing BCMMixerOutputLayerAcquire */
    if (layer_id == mixer->output_layer->layer_id) {
        return BCMMixerOutputLayerAcquire(mixer, ret_layer);
    }

    /* Else it may be a mixed layer */

    /* in case of failure */
    *ret_layer = NULL;

    if (layer_id >= mixer->max_layers) {
        ret = DFB_INVARG;
        /*D_INFO("\n\nBCMMixerLayerAcquire: layer_id > mixer->nax_layer\n\n");*/
        goto quit;
    }

    /* get the layer */
    if ((*ret_layer = mixer->mixed_layers[layer_id]) == NULL) {
        D_ERROR("%s/BCMMixerLayerAcquire: layer_id %u is not a registered layer\n",
                BCMMixerModuleName,
                layer_id);
        ret = DFB_INVARG;
        goto quit;
    }

    /* lock the layer */
    if ((rc = pthread_mutex_lock(&(*ret_layer)->mutex))) {
        D_ERROR("%s/BCMMixerLayerAcquire: error locking layer_id %u mutex    --> %s\n",
                BCMMixerModuleName,
                layer_id,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

quit:

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerAcquire exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;
}

inline DFBResult BCMMixerLayerRelease(BCMCoreLayerHandle layer) {
    int rc = 0;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerRelease called\n",
            BCMMixerModuleName);
#endif

    D_ASSERT( layer != NULL );

    /* unlock the layer */
    if ((rc = pthread_mutex_unlock(&layer->mutex))) {
        D_ERROR("%s/BCMMixerLayerRelease: error unlocking layer_id %u mutex    --> %s\n",
                BCMMixerModuleName,
                layer->layer_id,
                strerror(rc) );
        return DFB_FAILURE;
    }

    return DFB_OK;
}

DFBResult BCMMixerLayerRegister(BCMMixerHandle mixer,
                                BCMCoreLayerHandle mixed_layer,
                                DFBDisplayLayerID *layer_id)
{
    int rc = 0;
    DFBResult ret = DFB_OK;
    DFBDisplayLayerID layer_index;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerRegister called with:\n"
            "mixer: %u\n"
            "mixed_layer: %u\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            mixer,
            mixed_layer,
            layer_id);
#endif

    D_ASSERT( mixer != NULL );
    D_ASSERT( mixed_layer != NULL );

    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerRegister: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    /* find the first mixed layer available in the mixer */
    for (layer_index = 0; layer_index < MAX_LAYERS; layer_index++)
    {
        if (mixer->mixed_layers[layer_index] == NULL)
        {
            mixer->mixed_layers[layer_index] = mixed_layer;
            *layer_id = layer_index;
            break;
        }
    }

    D_ASSERT( layer_index != MAX_LAYERS );

    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerRegister: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
    }

error:

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerLayerRegister exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;
}

/* If level already set, crap out */
DFBResult BCMMixerLayerUnregister(BCMMixerHandle mixer, DFBDisplayLayerID layer_id, bool noerror_missing) {
    int rc = 0;
    DFBResult ret = DFB_OK;
    int i;

    BCMCoreLayerHandle layer;

    D_DEBUG("%s/BCMMixerLayerUnregister called with:\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            layer_id );

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    /* lock the mixer */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerUnregister: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if (layer_id >= mixer->max_layers) {
        D_ERROR("%s/BCMMixerLayerUnregister: layer_id >= mixer->max_layers\n",
                BCMMixerModuleName);
        ret = DFB_INVARG;
        goto quit;
    }

    /* not using BCMMixerLayerAcquire because of the trylock */
    /* get the layer */
    if ((layer = mixer->mixed_layers[layer_id]) == NULL) {
        if (noerror_missing) {
            D_DEBUG("%s/BCMMixerLayerUnregister: noerror_missing\n", BCMMixerModuleName);
            ret = DFB_OK;
        }
        else {
            D_ERROR("%s/BCMMixerLayerUnregister: layer_id %u is not a registered layer\n",
                    BCMMixerModuleName,
                    layer_id);
            ret = DFB_INVARG;
        }
        goto quit;
    }

    /* lock the layer with trylock, if busy then crap out, because
     * you should not try to unregister the layer which is still in use!!!!
     */
    rc = pthread_mutex_trylock(&layer->mutex);

    switch (rc) {
        case 0:
            D_DEBUG( "%s/BCMMixerLayerUnregister: The slot will be forever lost in DirectFB because there is no dfb_layers_unregister\n",
                    BCMMixerModuleName );

            /* Release slot in the id array */
            mixer->mixed_layers[layer_id] = NULL;

            /* Release slot in the level sorted array */
            i = 0;
            while (mixer->mixed_layers_levelsorted[i] != NULL) {
                if (mixer->mixed_layers_levelsorted[i] == layer) {
                    mixer->mixed_layers_levelsorted[i] = NULL;
                    break;
                }
                i++;
            }
            /* Sort the level array */
            qsort(mixer->mixed_layers_levelsorted, sizeof(mixer->mixed_layers_levelsorted)/sizeof(mixer->mixed_layers_levelsorted[0]), sizeof(BCMCoreLayerHandle), BCMMixerLayerLevelCompare);
            /* Could not take the result of the previous loop because it might have
             * been interrupted if layer was found
             */
            for (mixer->nb_layers_levelsorted = 0; mixer->nb_layers_levelsorted < (mixer->max_layers-1); mixer->nb_layers_levelsorted++) {
                if (mixer->mixed_layers_levelsorted[mixer->nb_layers_levelsorted] == NULL) {
                    break;
                }
            }

            /* No need to deallocate surfaces as this is done in remove region.
             */

            /* Unlock */
            if ((rc = pthread_mutex_unlock(&layer->mutex))) {
                D_ERROR("%s/BCMMixerLayerUnregister: error unlocking layer_id %u mutex    --> %s\n",
                        BCMMixerModuleName,
                        layer_id,
                        strerror(rc));
                ret = DFB_FAILURE;
                goto quit;
            }

#if 0
            /* pthread_mutex_destroy  actually does nothing except checking that the
             * mutex is unlocked.
             */
            if ((rc = pthread_mutex_destroy(&layer->mutex))) {
                D_ERROR("%s/BCMMixerLayerUnregister: error detroying mutex    --> %s",
                        BCMMixerModuleName,
                        strerror(rc));
                ret = DFB_FAILURE;
                /* continue */
            }

            /* pthread_mutexattr_destroy does nothing in the LinuxThreads implementation,
             * but we never know, could be used in NPTL
             */
            if ((rc = pthread_mutexattr_destroy(&layer->mutex_attr))) {
                D_ERROR("%s/BCMMixerLayerUnregister: error detroying mutex_attr    --> %s",
                        BCMMixerModuleName,
                        strerror(rc));
                ret = DFB_FAILURE;
                /* continue */
            }

            /* free */
            D_FREE(layer);
#endif
        break;
        case EBUSY:
            /* This can happen if the directfb flip function is invoked,
             * because only sync is locks the mixer as well. If this case happen,
             * it means the application is trying to unregister the layer while
             * still using it. */
            D_DEBUG("%s/BCMMixerLayerUnregister: error locking the layer_id %u mutex, already locked by another thread\n",
                    BCMMixerModuleName,
                    layer_id);
            ret = DFB_FAILURE;
            goto quit;
        break;
        default:
            D_ERROR("%s/BCMMixerLayerUnregister: error locking the layer_id %u mutex    --> %s\n",
                    BCMMixerModuleName,
                    layer_id,
                    strerror(rc));
            ret = DFB_FAILURE;
            goto quit;
        break;
    }

quit:

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerUnregister: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMMixerLayerUnregister exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called while owning one of the mixer's layer mutex.
 *
 * If level already set, crap out.
 * Also a -1 level will remove the layer from the list.
 * 0 level is reserved to the primary layer.
 */
DFBResult BCMMixerLayerSetLevel(BCMMixerHandle mixer,
                                DFBDisplayLayerID layer_id,
                                int level) {
    int rc = 0;
    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle layer;
    uint32_t i = 0;

    D_DEBUG("%s/BCMMixerLayerSetLevel called with:\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            layer_id );

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    /* lock the mixer, because we are reordering it's lists */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerLayerSetLevel: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if (level < -1 || level == 0 || level >= (int32_t)mixer->max_layers) {
        D_ERROR("%s/BCMMixerLayerSetLevel: invalid level %d\n"
                "must be -1 for invisible or must be greater than 0 (0 is for primary)\n"
                "and must be less than %d to be visible\n",
                BCMMixerModuleName,
                level,
                mixer->max_layers);
        ret = DFB_INVARG;
        goto quit;
    }

    /* No goto error starting here */
    if ((ret = BCMMixerLayerAcquire(mixer, layer_id, &layer)) != DFB_OK) {
        goto quit;
    }

    /* BCMMixerLayerAcquire should have returned an error code if this is
     * the case.
     */
    D_ASSERT(layer != NULL);

    switch (layer->bcmcore_type) {
        case BCMCORELAYER_MIXER_FLIPBLIT:
        case BCMCORELAYER_MIXER_FLIPTOGGLE:
        case BCMCORELAYER_FAA:
        case BCMCORELAYER_BACKGROUND:
            /* Check if level is not already used */
            /* set the new level, if already set to another layer, crap out */
            /* If not reassign */
            layer->level = level;

        break;
        case BCMCORELAYER_OUTPUT:
#if 0 /* This cause failures: "cPEConsumer_DVD::Run() - bdvd_decode_start FAILED! (0)", must fix */
            if (level == -1) {
                /* Let's disable the window */
                bdvd_graphics_window_settings_t graphics_settings;
                bdvd_graphics_window_get(layer->graphics_window->window_handle, &graphics_settings);
                graphics_settings.enabled = false;
                if (bdvd_graphics_window_set(layer->graphics_window->window_handle, &graphics_settings) != b_ok) {
                    D_ERROR( "%s/BCMMixerLayerSetLevel: error setting graphic framebuffer surface\n",
                             BCMMixerModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
                }
            }
            else {
                /* Let's reenable the window */
                bdvd_graphics_window_settings_t graphics_settings;
                bdvd_graphics_window_get(layer->graphics_window->window_handle, &graphics_settings);
                graphics_settings.enabled = true;
                if (bdvd_graphics_window_set(layer->graphics_window->window_handle, &graphics_settings) != b_ok) {
                    D_ERROR( "%s/BCMMixerLayerSetLevel: error setting graphic framebuffer surface\n",
                             BCMMixerModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
                }
            }
#endif
            /* Don't not assign in the mixer */
            level = -1;
        break;
        default:
            /* Ignore for now */
            level = -1;
        break;
    }

    /* No goto error ending here */
    if ((ret = BCMMixerLayerRelease(layer)) != DFB_OK) {
        goto quit;
    }

    /* don't trigger the mixer since not used for background layer */
    if (layer->bcmcore_type == BCMCORELAYER_BACKGROUND)
    {
        pthread_mutex_unlock(&mixer->mutex);
        ret = BCMMixerLayerFlip(mixer, layer_id, NULL, false, 0);
        return ret;
    }

    /* Assign the same slot if already there, or a free slot if not in the level array */
    while (mixer->mixed_layers_levelsorted[i] != NULL) {
        if (mixer->mixed_layers_levelsorted[i] == layer) {
            /* If level is -1, remove from level sorted layer list, else reuse the same slot */
            if (level == -1) {
                mixer->mixed_layers_levelsorted[i] = NULL;
            }
            break;
        }
        else if (mixer->mixed_layers_levelsorted[i]->level == level) {
            D_ERROR("%s/BCMMixerLayerSetLevel: level %d is already assigned to layer_id %u\n",
                    BCMMixerModuleName,
                    level,
                    mixer->mixed_layers_levelsorted[i]->layer_id);
            ret = DFB_INVARG;
            goto quit;
        }
        i++;
    }



    if (level == -1) {
        /* Trigger a new mixer to apply removal of layer */
        mixer->level_changed = true;
    }
    else {
        mixer->mixed_layers_levelsorted[i] = layer;
    }

    /* Sort the level array */
    qsort(mixer->mixed_layers_levelsorted, sizeof(mixer->mixed_layers_levelsorted)/sizeof(mixer->mixed_layers_levelsorted[0]), sizeof(BCMCoreLayerHandle), BCMMixerLayerLevelCompare);

    /* Could not take the result of the previous loop because it might have
     * been interrupted if layer was found
     */
    for (mixer->nb_layers_levelsorted = 0; mixer->nb_layers_levelsorted < (mixer->max_layers-1); mixer->nb_layers_levelsorted++) {
        if (mixer->mixed_layers_levelsorted[mixer->nb_layers_levelsorted] == NULL) {
            break;
        }
    }

quit:

    /* unlock the mixer */
        if ((rc = pthread_mutex_unlock(&mixer->mutex)))
        {
            D_ERROR("%s/BCMMixerLayerSetLevel: error unlocking mixer mutex    --> %s\n",
                    BCMMixerModuleName,
                    strerror(rc));
            ret = DFB_FAILURE;
            /* fall through */
        }

    D_DEBUG("%s/BCMMixerLayerSetLevel exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

DFBResult BCMMixerLayerGetLevel(BCMMixerHandle mixer,
                                DFBDisplayLayerID layer_id,
                                int * level) {

    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle layer;

    D_DEBUG("%s/BCMMixerLayerGetLevel called with:\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            layer_id );

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    if ((ret = BCMMixerLayerAcquire(mixer, layer_id, &layer)) != DFB_OK) {
        *level = -1;
        return ret;
    }

    /* BCMMixerLayerAcquire should have returned an error code if this is
     * the case.
     */
    D_ASSERT(layer != NULL);

    /* get the level */
    *level = layer->level;

    if ((ret = BCMMixerLayerRelease(layer)) != DFB_OK) {
        *level = -1;
        /* fall through */
    }

    D_DEBUG("%s/BCMMixerLayerGetLevel exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

DFBResult BCMMixerSetLayerLocation(BCMMixerHandle mixer,
                                   DFBDisplayLayerID layer_id,
                                   const DFBLocation * location,
                                   const DFBRectangle * destination,
                                   uint32_t screen_width,
                                   uint32_t screen_height,
                                   bool mixer_locked) {
    int rc = 0;
    DFBResult ret = DFB_OK;
    bresult err;

    DFBRectangle clip_rect;
    DFBRectangle new_front_rect;
    DFBLocation new_location;

    BCMCoreLayerHandle layer = NULL;
    BCMCoreLayerHandle other_layer = NULL;

    bool use_internal_lock = false;

    uint32_t i;

    D_DEBUG("%s/BCMMixerSetLayerLocation called\n",
            BCMMixerModuleName);

    D_ASSERT( mixer != NULL );

    if (!mixer_locked)
    {
        /* we are coming from SetRegion rather than SetDisplayDimension so */
        /* lock the mutex to make sure we not in the middle of SetLevel */
        if ((rc = pthread_mutex_lock(&mixer->mutex))) {
            D_ERROR("%s/BCMMixerStop: error locking mixer mutex    --> %s\n",
                    BCMMixerModuleName,
                    strerror(rc));
            return DFB_FAILURE;
        }

        use_internal_lock = true;
    }

    if (use_internal_lock)
    {
        if ((ret = BCMMixerLayerAcquireMixerLocked(mixer, layer_id, &layer)) != DFB_OK) {
            goto quit;
        }
    }
    else
    {
        if ((ret = BCMMixerLayerAcquire(mixer, layer_id, &layer)) != DFB_OK) {
            goto quit;
        }
    }

    /* the layer may have been released, don't proceed */
    if (layer->dfb_ext_type == bdvd_dfb_ext_unknown)
    {
        goto quit;
    }

    /* default to the original rectangle */
    new_front_rect = layer->front_rect;

    if (!layer->surface_buffers[layer->front_surface_index] || !((bdvd_graphics_surface_h*)layer->surface_buffers[layer->front_surface_index]->video.ctx)[0]) {
        goto quit;
    }

    new_location = location ? *location : layer->location;

    switch (layer->dfb_ext_type) {
        case bdvd_dfb_ext_bd_j_image_faa_layer:
        case bdvd_dfb_ext_bd_j_sync_faa_layer:

            for (i = 0; i < mixer->max_layers; i++) {
                if (mixer->mixed_layers[i] == NULL || mixer->mixed_layers[i] == layer) {
                    continue;
                }

                /* don't lock not bd-j graphics layers */
                if (mixer->mixed_layers[i]->dfb_ext_type != bdvd_dfb_ext_bd_j_graphics_layer)
                    continue;

                if (use_internal_lock)
                {
                    if ((ret = BCMMixerLayerAcquireMixerLocked(mixer, i, &other_layer)) != DFB_OK) {
                        goto quit;
                    }
                }
                else
                {
                    if ((ret = BCMMixerLayerAcquire(mixer, i, &other_layer)) != DFB_OK) {
                        goto quit;
                    }
                }

                if (other_layer->dfb_ext_type == bdvd_dfb_ext_bd_j_graphics_layer) {
                    if (destination) {
                        new_location.x = (float)destination->x/(float)other_layer->backsurface_clone.width;
                        new_location.y = (float)destination->y/(float)other_layer->backsurface_clone.height;
                        new_location.w = (float)destination->w/(float)other_layer->backsurface_clone.width;
                        new_location.h = (float)destination->h/(float)other_layer->backsurface_clone.height;
                    }

                    new_front_rect.x = other_layer->front_rect.x + (int)((new_location.x * (float)other_layer->front_rect.w) + 1);
                    new_front_rect.y = other_layer->front_rect.y + (int)((new_location.y * (float)other_layer->front_rect.h) + 1);
                    new_front_rect.w = (int)((new_location.w * (float)other_layer->front_rect.w) + 1);
                    new_front_rect.h = (int)((new_location.h * (float)other_layer->front_rect.h) + 1);

                    if (!dfb_rectangle_intersect( &new_front_rect,
                                                  &other_layer->front_rect )) {
                        D_ERROR( "%s/BCMMixerSetLayerLocation: invalid front rectangle\n",
                             BCMMixerModuleName );
                        ret = DFB_FAILURE;
                        goto quit;
                    }

                    /* break */
                    i = mixer->max_layers;
                }

                if ((ret = BCMMixerLayerRelease(other_layer)) != DFB_OK) {
                    goto quit;
                }

                other_layer = NULL;
            }

            layer->front_rect = new_front_rect;
        break;
        case bdvd_dfb_ext_bd_j_background_layer:
        /* TODO or is it really? */
        default:

        	D_DEBUG("front rect %d %d %d %d\n", layer->front_rect.x, layer->front_rect.y, layer->front_rect.w, layer->front_rect.h);

            new_front_rect.x = new_location.x * screen_width;
            new_front_rect.y = new_location.y * screen_height;
            new_front_rect.w = new_location.w * screen_width;
            new_front_rect.h = new_location.h * screen_height;

                clip_rect.x = clip_rect.y = 0;
                    clip_rect.w = layer->frontsurface_clone.width;
                    clip_rect.h = layer->frontsurface_clone.height;

            if (!dfb_rectangle_intersect( &new_front_rect,
                                              &clip_rect )) {
                D_ERROR( "%s/BCMMixerSetLayerLocation: invalid front rectangle\n",
                         BCMMixerModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
                }

            if (layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT &&
                layer->nb_surfaces == 2) {
                /* TODO when two compositors, should be handled by the mixer compositor */
                if ((err = bdvd_graphics_surface_fill(
                      dfb_bcmcore_context->driver_compositor,
                      ((bdvd_graphics_surface_h*)layer->surface_buffers[layer->front_surface_index]->video.ctx)[0],
                      (bsettop_rect *)&layer->front_rect,
                      (bdvd_graphics_pixel_t)0)) != b_ok) {
                    D_ERROR( "%s/BCMMixerSetLayerLocation: error calling bdvd_graphics_surface_fill %d\n",
                         BCMMixerModuleName,
                         err );
                    ret = DFB_FAILURE;
                    goto quit;
                }

                if ((err = bdvd_graphics_compositor_sync(dfb_bcmcore_context->driver_compositor)) != b_ok) {
                    D_ERROR( "%s/BCMMixerSetLayerLocation: error calling bdvd_graphics_compositor_sync %d\n",
                         BCMMixerModuleName,
                         err );
                    ret = DFB_FAILURE;
                    goto quit;
                }
            }

            layer->front_rect = new_front_rect;

            D_DEBUG("front rect %d %d %d %d\n", layer->front_rect.x, layer->front_rect.y, layer->front_rect.w, layer->front_rect.h);
        break;
    }

    layer->location = new_location;

    if (layer->dfb_ext_type == bdvd_dfb_ext_bd_j_graphics_layer) {

        for (i = 0; i < mixer->max_layers; i++) {
            if (mixer->mixed_layers[i] == NULL || mixer->mixed_layers[i] == layer) {
                continue;
            }

            /* don't lock not faa layers, they are the only one we care */
            if ((mixer->mixed_layers[i]->dfb_ext_type != bdvd_dfb_ext_bd_j_image_faa_layer ||
                mixer->mixed_layers[i]->dfb_ext_type != bdvd_dfb_ext_bd_j_sync_faa_layer))
                continue;

            if (use_internal_lock)
            {
                if ((ret = BCMMixerLayerAcquireMixerLocked(mixer, i, &other_layer)) != DFB_OK) {
                    goto quit;
                }
            }
            else
            {
                if ((ret = BCMMixerLayerAcquire(mixer, i, &other_layer)) != DFB_OK) {
                    goto quit;
                }
            }

            if (other_layer->bcmcore_type == BCMCORELAYER_FAA) {
                other_layer->front_rect.x = layer->front_rect.x + (other_layer->location.x * layer->front_rect.w);
                other_layer->front_rect.y = layer->front_rect.y + (other_layer->location.y * layer->front_rect.h);
                other_layer->front_rect.w = other_layer->location.w * layer->front_rect.w;
                other_layer->front_rect.h = other_layer->location.h * layer->front_rect.h;

                if (!dfb_rectangle_intersect( &other_layer->front_rect,
                                              &layer->front_rect )) {
                    D_ERROR( "%s/BCMMixerSetLayerLocation: invalid front rectangle\n",
                         BCMMixerModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
                }
            }

            if ((ret = BCMMixerLayerRelease(other_layer)) != DFB_OK) {
                goto quit;
            }

            other_layer = NULL;
        }
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerSetLayerLocation: back rect %d %d %d %d\n",
            BCMMixerModuleName,
            layer->back_rect.x,
            layer->back_rect.y,
            layer->back_rect.w,
            layer->back_rect.h);

    D_DEBUG("%s/BCMMixerSetLayerLocation: front rect %d %d %d %d\n",
            BCMMixerModuleName,
            layer->front_rect.x,
            layer->front_rect.y,
            layer->front_rect.w,
            layer->front_rect.h);

    D_DEBUG("%s/BCMMixerSetLayerLocation: location %f %f %f %f\n",
            BCMMixerModuleName,
            layer->location.x,
            layer->location.y,
            layer->location.w,
            layer->location.h);

    D_DEBUG("%s/BCMMixerSetLayerLocation: back to front ratio h %f v %f\n",
                    BCMMixerModuleName,
                    layer->horizontal_back2front_scalefactor,
                    layer->vertical_back2front_scalefactor);
#endif

quit:

    if (other_layer) {
        ret = (BCMMixerLayerRelease(other_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    if (layer) {
        ret = (BCMMixerLayerRelease(layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    if (!mixer_locked)
    {
        /* unlock the mixer */
        if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
            D_ERROR("%s/BCMMixerSetDisplayDimensions: error unlocking mixer mutex    --> %s\n",
                    BCMMixerModuleName,
                    strerror(rc));
            ret = DFB_FAILURE;
            goto quit;
        }
    }

    D_DEBUG("%s/BCMMixerSetLayerLocation exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

/* WARNING: the display handle is not to be used by another thread,
 * because it is not locked. As only the thread that invokes bdisplay_set should
 * be allowed to call the IDirectFB SetVideoMode function. This function gets should
 * only be called from a SetVideoMode function.
 */
DFBResult BCMMixerSetDisplayDimensions(BCMMixerHandle mixer,
                                       BCMCoreLayerHandle output_layer,
                                       bool set_mixer) {
    int rc = 0;
    DFBResult ret = DFB_OK;
    bresult err;

    uint32_t width;
    uint32_t height;

    uint32_t i = 0;

    DFBRectangle clip_rect;

    D_DEBUG("%s/BCMMixerSetDisplayDimensions called\n",
            BCMMixerModuleName );

    D_ASSERT( mixer != NULL );

    /* No goto quit before this */
    /* lock the mixer, because we are reordering it's lists */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSetDisplayDimensions: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    /* The layers front buffer should always be ARGB. Also the layer front
     * buffer must match the resolution of that of the output layer, because
     * there might be resolution restrictions on the feeder, and no scaling is
     * allowed in the sync step.
     */
    if (bdvd_graphics_window_get_video_format_dimensions(
                display_get_video_format(output_layer->graphics_window->display_handle),
                bdvd_graphics_pixel_format_a8_r8_g8_b8,
                &width, &height) != b_ok) {
        D_ERROR("%s/BCMMixerSetDisplayDimensions: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                BCMMixerModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

    output_layer->front_rect.x = 0;
    output_layer->front_rect.y = 0;
    output_layer->front_rect.w = width;
    output_layer->front_rect.h = height;

    for (i = 0; i < output_layer->nb_surfaces; i++) {
        /* TODO this should never be null */
        if (output_layer->surface_buffers[i] && ((bdvd_graphics_surface_h*)output_layer->surface_buffers[i]->video.ctx)[0]) {
            if ((err = bdvd_graphics_surface_fill(
                  mixer->driver_compositor,
                  ((bdvd_graphics_surface_h*)output_layer->surface_buffers[i]->video.ctx)[0],
                  (bsettop_rect *)&output_layer->front_rect,
                  (bgraphics_pixel)0)) != b_ok) {
                D_ERROR( "%s/BCMMixerSetDisplayDimensions: error calling bdvd_graphics_surface_fill %d\n",
                     BCMMixerModuleName,
                     err );
                ret = DFB_FAILURE;
                goto quit;
            }
        }

        output_layer->dirty_rectangles[i] = output_layer->front_rect;
    }

    D_DEBUG("%s/BCMMixerSetDisplayDimensions: back rect %d %d %d %d\n",
            BCMMixerModuleName,
            output_layer->back_rect.x,
            output_layer->back_rect.y,
            output_layer->back_rect.w,
            output_layer->back_rect.h);

    D_DEBUG("%s/BCMMixerSetDisplayDimensions: front rect %d %d %d %d\n",
            BCMMixerModuleName,
            output_layer->front_rect.x,
            output_layer->front_rect.y,
            output_layer->front_rect.w,
            output_layer->front_rect.h);

    D_DEBUG("%s/BCMMixerSetDisplayDimensions: back to front ratio h %f v %f\n",
            BCMMixerModuleName,
            output_layer->horizontal_back2front_scalefactor,
            output_layer->vertical_back2front_scalefactor);

    /* This is just to adjust the graphics feeder params */
    if (bdvd_graphics_window_set_video_format_dimensions(output_layer->graphics_window->window_handle, output_layer->graphics_window->display_handle) != b_ok) {
        D_ERROR("%s/BCMMixerSetDisplayDimensions: failure in bdvd_graphics_window_set_video_format_dimensions\n",
                BCMMixerModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

    if (set_mixer) {
        for (i = 0; i < mixer->max_layers; i++) {

            if (mixer->mixed_layers[i] == NULL) {
                continue;
            }

            if ((ret = BCMMixerSetLayerLocation(mixer,
                                                i,
                                                NULL,
                                                NULL,
                                                width,
                                                height,
                                                true)) != DFB_OK) {
                goto quit;
            }
        }

        /* The thread should be cancelled if we've gone that far */
        D_ASSERT(mixer->mixer_thread_arguments.cancelled);

        /* The mixer thread must be restarted now that the graphics is reenabled */
        if ((ret = BCMMixerStart(mixer)) != DFB_OK) {
            goto quit;
        }
    }

quit:

    if (set_mixer) {
        if ((err = bdvd_graphics_compositor_sync(mixer->driver_compositor)) != b_ok) {
            D_ERROR( "%s/BCMMixerSetDisplayDimensions: error calling bdvd_graphics_compositor_sync %d\n",
                     BCMMixerModuleName,
                     err );
            ret = DFB_FAILURE;
            /* fall through */
        }
    }

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSetDisplayDimensions: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMMixerSetDisplayDimensions exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

/* BCMMixerOpen requires to have pre-allocated the mixer context...
 * This should change
 */
DFBResult BCMMixerOpen(BCMMixerHandle mixer, BCMCoreLayerHandle output_layer) {

    int rc = 0;
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMMixerOpen called\n", BCMMixerModuleName );

    D_ASSERT( mixer != NULL );
    D_ASSERT( output_layer != NULL );

    mixer->max_layers = sizeof(mixer->mixed_layers)/sizeof(mixer->mixed_layers[0]);
    memset(mixer->mixed_layers, 0x00, sizeof(mixer->mixed_layers));

    /* Always returns 0 */
    pthread_mutexattr_init(&mixer->mutex_attr);

    /* mixer mutex should be init with PTHREAD_MUTEX_RECURSIVE_NP */
    if ((rc = pthread_mutexattr_settype(&mixer->mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP))) {
        D_ERROR("%s/BCMMixerOpen: error setting mutex type    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_INIT;
        goto error;
    }

    /* Always returns 0 */
    pthread_mutex_init(&mixer->mutex, &mixer->mutex_attr);

    /* Always returns 0 */
    pthread_mutexattr_init(&mixer->cond_mutex_attr);

#ifdef D_DEBUG_ENABLED
    if ((rc = pthread_mutexattr_settype(&mixer->cond_mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP))) {
#else
    if ((rc = pthread_mutexattr_settype(&mixer->cond_mutex_attr, PTHREAD_MUTEX_FAST_NP))) {
#endif
        D_ERROR("%s/BCMMixerOpen: error setting mutex type    --> %s\n",
        BCMMixerModuleName,
        strerror(rc));
        ret = DFB_INIT;
        goto error;
    }

    /* Always returns 0 */
    pthread_mutex_init(&mixer->cond_mutex, &mixer->cond_mutex_attr);

    /* This does really nothing in LinuxThread */
    if ((rc = pthread_condattr_init(&mixer->cond_attr))) {
        D_ERROR("%s/BCMMixerOpen: Could not pthread_condattr_init %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_INIT;
        goto error;
    }

    if ((rc = pthread_cond_init(&mixer->cond, &mixer->cond_attr))) {
        D_ERROR("%s/BCMMixerOpen: Could not pthread_cond_init %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_INIT;
        goto error;
    }

    /* We might eventually want to set attributes ... */
    if ((rc = pthread_attr_init(&mixer->mixer_thread_attr))) {
        D_ERROR("%s/BCMMixerOpen: Could not pthread_attr_init %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_INIT;
        goto error;
    }

    memset(&mixer->mixer_thread_arguments, 0, sizeof(mixer->mixer_thread_arguments));
    mixer->mixer_thread_arguments.mixer = mixer;
    mixer->mixer_thread_arguments.cancelled = true;

    /* register primary output layer */
    mixer->output_layer = mixer->mixed_layers[0] = output_layer;

    /* Always returns 0 */
    pthread_mutexattr_init(&mixer->output_layer->mutex_attr);

    /* mixer mutex should be init with PTHREAD_MUTEX_RECURSIVE_NP */
    if ((rc = pthread_mutexattr_settype(&mixer->output_layer->mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP))) {
        D_ERROR("%s/BCMMixerOpen: error setting mutex type    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_INIT;
        goto error;
    }

    /* Always returns 0 */
    pthread_mutex_init(&mixer->output_layer->mutex, &mixer->output_layer->mutex_attr);

    /* Initialize Clearrect data structures */
    mixer->subpic_cr_dirty = false;
    mixer->subpic_cr.number = 0;
    memset(mixer->subpic_cr.rects, 0, sizeof(mixer->subpic_cr.rects));
    memset(&mixer->subpic_cr_union, 0x00, sizeof(DFBRectangle));
    mixer->subvid_cr_dirty = false;
    mixer->subvid_cr_num = 0;

    D_DEBUG("%s/BCMMixerOpen exiting with code %d\n", BCMMixerModuleName,(int)ret );

    return ret;

error:

    BCMMixerCleanup(mixer);

    return ret;
}

DFBResult BCMMixerStart(BCMMixerHandle mixer) {
    DFBResult   ret = DFB_OK;
    int         rc = 0;

    BCMCoreLayerHandle output_layer = NULL;
    struct sched_param param;


    D_DEBUG("%s/BCMMixerStart called\n",
            BCMMixerModuleName );

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    /* lock the mixer, because we are reordering it's lists */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerStart: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    /* Acquire primary layer, this is necessary whenever we
     * use the bdvd_graphics_window_set_xxx functions
     */
    if ((ret = BCMMixerOutputLayerAcquire(mixer, &output_layer)) != DFB_OK) {
        goto quit;
    }

    mixer->mixer_thread_arguments.cancelled = false;

    D_DEBUG("%s/BCMMixerStart starting mixer %d\n", BCMMixerModuleName, mixer->mixerId);

    if ((rc = pthread_create(&mixer->mixer_thread, &mixer->mixer_thread_attr, mixer_thread_routine, &mixer->mixer_thread_arguments))) {
        D_ERROR("%s/BCMMixerStart: Could not pthread_create %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

    param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 19;
    pthread_setschedparam(mixer->mixer_thread, SCHED_FIFO,  &param);


#if 0
    /* TODO IS NOT NECESSARY, plus colides on the trigger...!!!! */
    if ((err = bdvd_graphics_window_set_vsync_trigger(output_layer->graphics_window->window_handle, true)) != b_ok) {
        D_ERROR("%s/BCMMixerStart: failure in bdvd_graphics_window_set_vsync_trigger %d\n",
                BCMMixerModuleName,
                err );
        ret = DFB_FAILURE;
        goto quit;
    }
#endif

quit:

    if (output_layer) {
        /* unlock the output layer if still locked */
        ret = (BCMMixerLayerRelease(output_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerStart: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMMixerStart exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

DFBResult BCMMixerStop(BCMMixerHandle mixer) {

    int rc = 0;
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMMixerStop called\n",
            BCMMixerModuleName);

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    /* lock the mixer */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerStop: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if (mixer->mixer_thread_arguments.cancelled == false) {
        mixer->mixer_thread_arguments.cancelled = true;

        /* unlock the mixer */
        if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
            D_ERROR("%s/BCMMixerStop: error unlocking mixer mutex    --> %s\n",
                    BCMMixerModuleName,
                    strerror(rc));
            /* continue */
        }

        D_DEBUG("%s/BCMMixerStop joining\n",
                BCMMixerModuleName);

        if ((rc = pthread_join(mixer->mixer_thread, NULL))) {
            D_ERROR("%s/BCMMixerStop: could not pthread_join %s\n",
                    BCMMixerModuleName,
                    strerror(rc));
            ret = DFB_FAILURE;
            goto error;
        }

        D_DEBUG("%s/BCMMixerStop joined\n",
                BCMMixerModuleName);
    }
    else {
        /* unlock the mixer */
        if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
            D_ERROR("%s/BCMMixerStop: error unlocking mixer mutex    --> %s\n",
                    BCMMixerModuleName,
                    strerror(rc));
            /* continue */
        }
    }

    D_DEBUG("%s/BCMMixerStop exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;

error:

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerStop: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
    }

    D_DEBUG("%s/BCMMixerStop exiting with code %d\n",
            BCMMixerModuleName,
           (int)ret );

    return ret;
}


DFBResult BCMMixerClose(BCMMixerHandle mixer) {

    int rc = 0;
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMMixerClose called\n",
            BCMMixerModuleName);

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    /* lock the mixer */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerClose: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    /* Nothing to do really, BCMMixerStop must have been called, should check that. */

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerClose: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMMixerClose exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret == DFB_OK ? BCMMixerCleanup(mixer) : ret;
}


DFBResult BCMMixerSetClearrect(
    BCMMixerHandle mixer,
    DFBDisplayLayerID layer_id,
    bdvd_dfb_ext_clear_rect_t *clearrect)
{

    int rc = 0;
    DFBResult ret = DFB_OK;
    int i;
    DFBRectangle display_rect;

    D_DEBUG("%s/BCMMixerSetClearrect called\n",
            BCMMixerModuleName);

    D_ASSERT( mixer != NULL );

    /* lock the mixer */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSetClearrect: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    mixer->subpic_cr.clear_rect_done_cb = clearrect->clear_rect_done_cb;
    mixer->subpic_cr.cb_context = clearrect->cb_context;

    mixer->subpic_cr.number = 0;
    for (i = 0; i < clearrect->number; i++)
    {
        /* copy individual rectangle to list */
        memcpy((void *)&mixer->subpic_cr.rects[mixer->subpic_cr.number], (void *)&clearrect->rects[i], sizeof(bdvd_dfb_ext_rect_t));
        if (dfb_rectangle_intersect(&mixer->subpic_cr.rects[mixer->subpic_cr.number].rect, &mixer->mixed_layers[layer_id]->back_rect))
        {
            if ((mixer->subpic_cr.rects[mixer->subpic_cr.number].rect.w!=0)&&
                (mixer->subpic_cr.rects[mixer->subpic_cr.number].rect.h!=0))
            {
                mixer->subpic_cr.number++;
            }
        }
        else
        {
            if (!((mixer->subpic_cr.rects[mixer->subpic_cr.number].rect.w==0) &&
                (mixer->subpic_cr.rects[mixer->subpic_cr.number].rect.h==0)))
            D_ERROR("%s/BCMMixerSetClearrect: rectangle[%d] out of layer\n", BCMMixerModuleName, i);
        }
    }

    if (clearrect->wait_mode == bdvd_dfb_ext_clear_rect_wait_until_vsync)
    {
        /* list of rectangles make sense, copy the data structure for execution at the next vsync */
        mixer->subpic_cr_dirty = true;
        dfb_bcmcore_context->hdi_mixer_issue_clearrect = false;
    }
    else
    {
        /* list of rectangles make sense, copy the data structure for execution but execute when hdi layer is flipped */
        mixer->subpic_cr_dirty = false;
        dfb_bcmcore_context->hdi_mixer_issue_clearrect = true;
    }

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSetClearrect: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;

    }

    D_DEBUG("%s/BCMMixerSetClearrect exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}


void video_src2display_rect_transform(
    BCMCoreLayerHandle layer,
    bdvd_graphics_window_h window_handle,
    DFBRectangle * src_rect,
    DFBRectangle * display_rect)
{
    bdvd_graphics_window_settings_t settings;
    double horizontal_back2front_scalefactor = 0;
    double vertical_back2front_scalefactor = 0;

    bdvd_graphics_window_get(window_handle, &settings);

    horizontal_back2front_scalefactor = (double)settings.destination_width/(double)layer->back_rect.w;
    vertical_back2front_scalefactor = (double)settings.destination_height/(double)layer->back_rect.h;

    D_DEBUG("gfx window w=%d h=%d\n", settings.destination_width, settings.destination_height);

    display_rect->x = layer->front_rect.x + (src_rect->x * horizontal_back2front_scalefactor + 0.5f);
    display_rect->y = layer->front_rect.y + (src_rect->y * vertical_back2front_scalefactor + 0.5f);
    display_rect->w = src_rect->w * horizontal_back2front_scalefactor + 0.5f;
    display_rect->h = src_rect->h * vertical_back2front_scalefactor + 0.5f;
}

static inline DFBResult
BCMMixerApplyClearrect(BCMMixerHandle mixer,
               const BCMCoreLayerHandle ihd_layer,
               const BCMCoreLayerHandle output_layer) {

    DFBResult ret = DFB_OK;
    bresult err;
    DFBRectangle video_rect;
    DFBRectangle display_rect;
    int i;
    int index = 0;
    double horiz_scale, vert_scale;
    double horiz_phase, vert_phase;

    D_DEBUG("%s/BCMMixerApplyClearrect called\n",
            BCMMixerModuleName);

    if (0 == mixer->subpic_cr.number)
    return ret;

    if (mixer->subpic_cr.number > 32)
    {
    D_ERROR( "BCMMixerApplyClearrect: error clearrect number >32 (%d)\n",mixer->subpic_cr.number);
    return DFB_FAILURE;
    }

    D_DEBUG("%s/BCMMixerApplyClearrect called\n",
            BCMMixerModuleName);

    mixer->subvid_cr_num = 0;

    D_DEBUG("front rect %d %d %d %d\n",
            output_layer->front_rect.x,
            output_layer->front_rect.y,
            output_layer->front_rect.w,
            output_layer->front_rect.h);

    D_DEBUG("back rect %d %d %d %d\n",
            ihd_layer->back_rect.x,
            ihd_layer->back_rect.y,
            ihd_layer->back_rect.w,
            ihd_layer->back_rect.h);

    mixer->cr_notify = (mixer->subpic_cr.clear_rect_done_cb) ? true : false;
    mixer->subvid_cr_dirty = true;

    for (i = 0; i < mixer->subpic_cr.number; i++)
    {
        uint32_t layer_right_margin;
        uint32_t layer_bottom_margin;
        uint32_t crect_right_margin;
        uint32_t crect_bottom_margin;

        D_DEBUG("src rect: x=%d y=%d w=%d h=%d\n",
        mixer->subpic_cr.rects[i].rect.x,
        mixer->subpic_cr.rects[i].rect.y,
        mixer->subpic_cr.rects[i].rect.w,
        mixer->subpic_cr.rects[i].rect.h);

        /* convert from ihd coordiates and size to display coordinates and size */
        src2dst_xform(
            &ihd_layer->front_rect, 
            &output_layer->back_rect,
            ihd_layer->frontsurface_clone.front_buffer->format, 
            output_layer->backsurface_clone.back_buffer->format,
            &mixer->subpic_cr.rects[i].rect, 
            &display_rect,
            &horiz_scale,
            &vert_scale,
            &horiz_phase,
            &vert_phase);

        /* crop to front rect size if display_rect exceeds layer front rect */
        layer_right_margin = ihd_layer->front_rect.x + ihd_layer->front_rect.w;
        crect_right_margin = display_rect.x + display_rect.w;
        display_rect.w = (crect_right_margin > layer_right_margin) ? display_rect.w - (crect_right_margin - layer_right_margin) : display_rect.w;
        layer_bottom_margin = ihd_layer->front_rect.y + ihd_layer->front_rect.h;
        crect_bottom_margin = display_rect.y + display_rect.h;
        display_rect.h = (crect_bottom_margin > layer_bottom_margin) ? display_rect.h - (crect_bottom_margin - layer_bottom_margin) : display_rect.h;

        dfb_rectangle_union ( &mixer->subpic_cr_union, &display_rect);

        D_DEBUG("gfx rect: x=%d y=%d w=%d h=%d\n",
        display_rect.x,
        display_rect.y,
        display_rect.w,
        display_rect.h);

        if (!((display_rect.w==0)||(display_rect.h==0)))
        {
            if (mixer->subpic_cr.rects[i].target_plane == bdvd_dfb_ext_clear_rect_target_plane_main_video)
            {
                video_src2display_rect_transform(
                    ihd_layer,
                    output_layer->graphics_window->window_handle,
                    &mixer->subpic_cr.rects[i].rect,
                    &video_rect);

                mixer->subvid_cr[index].x = video_rect.x;
                mixer->subvid_cr[index].y = video_rect.y;
                mixer->subvid_cr[index].width = video_rect.w;
                mixer->subvid_cr[index].height = video_rect.h;

                D_DEBUG("dis rect: x=%d y=%d w=%d h=%d\n",
                mixer->subvid_cr[index].x,
                mixer->subvid_cr[index].y,
                mixer->subvid_cr[index].width,
                mixer->subvid_cr[index].height);

                index++;
                mixer->subvid_cr_num++;
            }

            /* clear the rectangle */
            if ((err = bdvd_graphics_surface_fill(
                 mixer->mixer_compositor,
                 ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                 (bsettop_rect *)&display_rect,
                 (bgraphics_pixel)0)) != b_ok)
            {
                D_ERROR( "BCMMixerApplyClearrect: error calling bdvd_graphics_surface_fill %d\n",err );
                return DFB_FAILURE;
            }
        }
    }

    D_DEBUG("%s/BCMMixerApplyClearrect exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret);

    return ret;
}

static inline DFBResult
BCMMixerSync(BCMMixerHandle mixer,
             BCMCoreLayerHandle output_layer) {

    int rc = 0;
    bresult err;
    DFBResult ret = DFB_OK;

    uint32_t i = 0;

    /* If there is multiple visible layers,
     * the first blit will blend the two first layers together onto
     * the frame buffer surface. If there is only one visible layer,
     * only a regular blit is done to clear frame
     * buffer surface.
     */
    bool first_blit = true;

    uint32_t nb_locked_layers = 0;
    bool animator_frozen = false;

    bdvd_graphics_surface_h first_surface = NULL;
    BCMCoreLayerHandle      first_surface_layer = NULL;

    BCMCoreLayerHandle current_layer;

    DFBRectangle temp_rectangle;
    DFBRectangle dirty_rectangle;

    double horiz_scale, vert_scale;
    double horiz_phase, vert_phase;

    D_ASSERT( mixer != NULL );

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerSync entering\n",
            BCMMixerModuleName );
#endif

    /* No goto error before this */
    /* lock the mixer */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSync: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    /* Ok the dfb_source_rect.x and .y position should be scaled to
     * src/dest ratio
     */
    memset(&dirty_rectangle, 0, sizeof(dirty_rectangle));

    /* We must go through the visible layer list three time because we need to
     * know if it's necessary to launch the mixing, the second time because the
     * first blit does two layers in one shoot, thus we cannot release one of them
     * before this operation is finished (could be preempted before the state is
     * issued).
     */
    if (!mixer->nb_layers_levelsorted) {
        ret = DFB_INVAREA;
        goto quit;
    }

    if (mixer->animator) {
        if ((ret = BCMAnimatorFreezeState(mixer->animator)) != DFB_OK) {
            goto quit;
        }
        animator_frozen = true;
    }

    /* Go through list, and if visible, blend.
     * For now the visibility is set looking at the level.
     */
    for (nb_locked_layers = 0; nb_locked_layers < (mixer->max_layers-1); nb_locked_layers++) {
        if (mixer->mixed_layers_levelsorted[nb_locked_layers] == NULL) {
            if (nb_locked_layers == (mixer->nb_layers_levelsorted-1)) {
                D_ERROR("%s/BCMMixerSync: nb layer acquired does not match that of level sorted layers %d\n",
                        BCMMixerModuleName,
                        nb_locked_layers);
                goto quit;
            }
            break;
        }
        else {
            current_layer = mixer->mixed_layers_levelsorted[nb_locked_layers];
        }

        /* Go through list and union all dirty rectangles. */
        if ((rc = pthread_mutex_lock(&current_layer->mutex))) {
            D_ERROR("%s/BCMMixerSync: error locking mixer(%d), current_layer(%d)->layer_id %u mutex    --> %s\n",
                    BCMMixerModuleName,
                    mixer,
                    current_layer,
                    current_layer->layer_id,
                    strerror(rc));
            goto quit;
        }

        switch (current_layer->bcmcore_type) {
            case BCMCORELAYER_MIXER_FLIPBLIT:
            case BCMCORELAYER_MIXER_FLIPTOGGLE:
                if (current_layer->dirty_region.x2 && current_layer->dirty_region.y2) {
                    temp_rectangle = DFB_RECTANGLE_INIT_FROM_REGION(&current_layer->dirty_region);
                    dfb_rectangle_union ( &dirty_rectangle, &temp_rectangle );
                    /* notify that the mixing of this layer is about to begin */
                    if ((current_layer->onsync_flip == true) && (current_layer->callback != NULL))
                    {
                        (current_layer->callback)(
                            current_layer->callback_param1,
                            current_layer->callback_param2,
                            bdvd_dfb_ext_event_layer_mixing_started
                            );
                    }
                }
            break;
            case BCMCORELAYER_FAA:
                if (!mixer->animator) {
                    D_ERROR("%s/BCMMixerSync: animator is missing, should not happen\n",
                            BCMMixerModuleName);
                    ret = DFB_FAILURE;
                    nb_locked_layers++;
                    goto quit;
                }
                ret = BCMAnimatorLayerAnimate(mixer->animator, current_layer);
                switch (ret) {
                    /* using invarea to signal no update, may find another
                     * return code for that later.
                     */
                    case DFB_INVAREA:
                        if (current_layer->dfb_ext_force_clean_on_stop)
                        {
                            /* clean by update dirty region on all layers but faa which is invisible */
                            dfb_rectangle_union ( &dirty_rectangle, &output_layer->front_rect );
                            current_layer->dfb_ext_force_clean_on_stop = false;
                        }

                        /* The FAA location might have been updated */
                        if (!current_layer->dirty_region.x2 && !current_layer->dirty_region.y2) {
                            break;
                        }
                    /* ok signals the need for an update, let's update the full thing */
                    case DFB_OK:
                        dfb_rectangle_union( &dirty_rectangle, &current_layer->front_rect );
                    break;
                    default:
                        D_ERROR("%s/BCMMixerSync: failure in BCMAnimatorLayerAnimate\n",
                                BCMMixerModuleName);
                        ret = DFB_FAILURE;
                        nb_locked_layers++;
                        goto quit;
                }
            break;
            default:
                D_ERROR("%s/BCMMixerSync: unknown bcmcore_type\n",
                        BCMMixerModuleName);
                ret = DFB_FAILURE;
                nb_locked_layers++;
                goto quit;
        }
    }

    if (mixer->subpic_cr_dirty)
    {
        D_DEBUG("Clear Rect has been updated\n");

        if ((mixer->subpic_cr_union.x == 0) &&
            (mixer->subpic_cr_union.y == 0) &&
            (mixer->subpic_cr_union.w == 0) &&
            (mixer->subpic_cr_union.h == 0))
        {
            dirty_rectangle.w = dirty_rectangle.h = 1;
        }
        else
        {
            dfb_rectangle_union ( &dirty_rectangle, &mixer->subpic_cr_union);
        }

        D_DEBUG("CurReq x=%d y=%d w=%d h=%d\n PreReq x=%d y=%d w=%d h=%d\n",
        mixer->subpic_cr.rects[0].rect.x,
        mixer->subpic_cr.rects[0].rect.y,
        mixer->subpic_cr.rects[0].rect.w,
        mixer->subpic_cr.rects[0].rect.h,
        mixer->subpic_cr_union.x,
        mixer->subpic_cr_union.y,
        mixer->subpic_cr_union.w,
        mixer->subpic_cr_union.h);

        if (mixer->subpic_cr.number == 0)
        {
            memset(&mixer->subpic_cr_union, 0x00, sizeof(DFBRectangle));
            mixer->subvid_cr_dirty = true;
            mixer->subvid_cr_num = 0;
            mixer->cr_notify = (mixer->subpic_cr.clear_rect_done_cb) ? true : false;
        }

        D_DEBUG("DirtRec x=%d y=%d w=%d h=%d\n",
        dirty_rectangle.x,
        dirty_rectangle.y,
        dirty_rectangle.w,
        dirty_rectangle.h);

        mixer->subpic_cr_dirty = false;
    }

    /* If ClearRect has been updated ensure the sync in applied.
     * The dirty rectangle is used as a flag,  the dimensions are not
     *  used.
     */
    if (mixer->level_changed)
    {
        dfb_rectangle_union ( &dirty_rectangle, &output_layer->front_rect );
        mixer->level_changed = false;
    }

    if ((dirty_rectangle.w != 0) && (dirty_rectangle.h != 0))
    {
        D_DEBUG("DirtRec x=%d y=%d w=%d h=%d\n",
        dirty_rectangle.x,
        dirty_rectangle.y,
        dirty_rectangle.w,
        dirty_rectangle.h);

        if (!mixer->nb_layers_levelsorted) {

            dfb_rectangle_union ( &output_layer->dirty_rectangles[output_layer->front_surface_index], &output_layer->front_rect );
            dfb_rectangle_union ( &output_layer->dirty_rectangles[output_layer->back_surface_index], &output_layer->front_rect );
            dfb_rectangle_union ( &output_layer->dirty_rectangles[output_layer->idle_surface_index], &output_layer->front_rect );

            D_DEBUG("DirtRecOutput x=%d y=%d w=%d h=%d\n",
            output_layer->dirty_rectangles[output_layer->back_surface_index].x,
            output_layer->dirty_rectangles[output_layer->back_surface_index].y,
            output_layer->dirty_rectangles[output_layer->back_surface_index].w,
            output_layer->dirty_rectangles[output_layer->back_surface_index].h);

            if ((err = bdvd_graphics_surface_fill(
                  mixer->driver_compositor,
                  ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                  (bsettop_rect *)&output_layer->front_rect,
                  (bgraphics_pixel)0)) != b_ok) {
                D_ERROR( "%s/BCMMixerSync: error calling bdvd_graphics_surface_fill %d\n",
                     BCMMixerModuleName,
                     err );
                ret = DFB_FAILURE;
                goto quit;
            }
        }
        else
        {
            DFBRectangle src_dirty_rect;
            DFBRectangle output_dirty_rect;

            dfb_rectangle_union ( &output_layer->dirty_rectangles[output_layer->front_surface_index], &dirty_rectangle );
            dfb_rectangle_union ( &output_layer->dirty_rectangles[output_layer->back_surface_index], &dirty_rectangle );
            dfb_rectangle_union ( &output_layer->dirty_rectangles[output_layer->idle_surface_index], &dirty_rectangle );

            D_DEBUG("DirtRecOutput x=%d y=%d w=%d h=%d\n",
            output_layer->dirty_rectangles[output_layer->back_surface_index].x,
            output_layer->dirty_rectangles[output_layer->back_surface_index].y,
            output_layer->dirty_rectangles[output_layer->back_surface_index].w,
            output_layer->dirty_rectangles[output_layer->back_surface_index].h);

            for (i = 0; i < mixer->nb_layers_levelsorted; i++)
            {
            /* In sync, take the primary front dimensions only,
             * the current layer and the primary need to match anyway.
             */
            /* WARNING ABOUT THE BACK BUFFER OF THE OUTPUT LAYER,
             * it's should never be taken in account in sync, it's only for legacy
             * examples support
             */
            current_layer = mixer->mixed_layers_levelsorted[i];

            /* NO SCALING IS TAKING PLACE HERE, it's rather if src1 different from
             * output rectangle, it's going to clip. In the case of first pass
             * the src2 surface will be scanned completly, in the other cases
             * both rectangles are set to src1.
             */
            if (mixer->nb_layers_levelsorted > 1) {
                if (first_blit) {
                    if (current_layer->bcmcore_type == BCMCORELAYER_FAA) {
                        D_ERROR("%s/BCMMixerSync: error FAA layers cannot be at the bottom\n",
                                BCMMixerModuleName);
                        ret = DFB_FAILURE;
                    }

                    first_blit = false;
                    first_surface = ((bdvd_graphics_surface_h*)current_layer->surface_buffers[current_layer->front_surface_index]->video.ctx)[0];
                    first_surface_layer = current_layer;
                }
                else {
                    /* The M2MC won't allow a destination rectangle different from the
                     * output rectangle. It is also not possible for the source rectangle
                     * to be different from the output rectangle without scaling. Attempts
                     * to disable the checks in BGRC_IssueState resulted in a stuck M2MC.
                     */
                    if (first_surface) {
                        if (current_layer->bcmcore_type == BCMCORELAYER_FAA) {
                            T_DEBUG("+9\n");
                            horiz_scale = vert_scale = 1.0;
                            horiz_phase = vert_phase = 0.0;
                            if ((ret = BCMMixerBlit(
                                    mixer->mixer_compositor,
                                    ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                    first_surface,
                                    NULL,
                                    &output_layer->dirty_rectangles[output_layer->back_surface_index],
                                    &output_layer->dirty_rectangles[output_layer->back_surface_index],
                                    NULL,
                                    BGFX_RULE_PORTERDUFF_SRC,
                                    true,
                                    horiz_scale,
                                    vert_scale,
                                    horiz_phase,
                                    vert_phase)) != DFB_OK) {
                                goto quit;
                            }
                            T_DEBUG("-9\n");

                            if (current_layer->bcmcore_visibility == BCMCORELAYER_VISIBLE) {
                                /* When the blit is going to be moved to the graphics plane, then
                                 * we are not going to blend anymore.
                                 */
                                T_DEBUG("+10\n");
                                src_dirty_rect = current_layer->back_rect;
                                output_dirty_rect = current_layer->front_rect;
                                src2dst_xform(
                                    &current_layer->back_rect,
                                    &current_layer->front_rect,
                                    current_layer->surface_buffers[current_layer->front_surface_index]->format,
                                    output_layer->surface_buffers[output_layer->back_surface_index]->format,
                                    &src_dirty_rect,
                                    &output_dirty_rect,
                                    &horiz_scale,
                                    &vert_scale,
                                    &horiz_phase,
                                    &vert_phase);
                                if ((ret = BCMMixerBlit(
                                        mixer->mixer_compositor,
                                        ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                        ((bdvd_graphics_surface_h*)current_layer->surface_buffers[current_layer->front_surface_index]->video.ctx)[0],
                                        NULL,
                                        &output_dirty_rect,
                                        &src_dirty_rect,
                                        NULL,
                                        current_layer->premultiplied ? BGFX_RULE_PORTERDUFF_SRCOVER : BGFX_RULE_SRCALPHA_INVSRCALPHA,
                                        true,
                                        horiz_scale,
                                        vert_scale,
                                        horiz_phase,
                                        vert_phase)) != DFB_OK) {
                                    goto quit;
                                }
                                T_DEBUG("-10\n");
#ifdef ENABLE_MIX_TRACES
                            D_INFO("%s Blit 0.1 i (%d %d %d %d) o (%d %d %d %d)\n",
                            "FAA",
                            current_layer->back_rect.x,
                            current_layer->back_rect.y,
                            current_layer->back_rect.w,
                            current_layer->back_rect.h,
                            current_layer->front_rect.x,
                            current_layer->front_rect.y,
                            current_layer->front_rect.w,
                            current_layer->front_rect.h);
#endif
                            }
                        }
                        else
                        {
                            output_dirty_rect = output_layer->dirty_rectangles[output_layer->back_surface_index];

                            if ((first_surface_layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT) && (first_surface_layer->nb_surfaces == 1))
                            {
                                dfb_rectangle_intersect( &output_dirty_rect, &first_surface_layer->front_rect);

                                if ((output_dirty_rect.x != output_layer->dirty_rectangles[output_layer->back_surface_index].x) ||
                                    (output_dirty_rect.y != output_layer->dirty_rectangles[output_layer->back_surface_index].y) ||
                                    (output_dirty_rect.w != output_layer->dirty_rectangles[output_layer->back_surface_index].w) ||
                                    (output_dirty_rect.h != output_layer->dirty_rectangles[output_layer->back_surface_index].h))
                                {
                                    if ((err = bdvd_graphics_surface_fill(
                                            mixer->driver_compositor,
                                            ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                            (bsettop_rect *)&output_layer->dirty_rectangles[output_layer->back_surface_index],
                                            (bgraphics_pixel)0)) != b_ok) {
                                        D_ERROR( "%s/BCMMixerSync: error calling bdvd_graphics_surface_fill %d\n",
                                             BCMMixerModuleName,
                                             err );
                                        ret = DFB_FAILURE;
                                        goto quit;
                                    }
                                }
                                dst2src_xform(
                                    &first_surface_layer->back_rect,
                                    &first_surface_layer->front_rect,
                                    first_surface_layer->surface_buffers[first_surface_layer->front_surface_index]->format,
                                    output_layer->surface_buffers[output_layer->back_surface_index]->format,
                                    &src_dirty_rect,
                                    &output_dirty_rect,
                                    &horiz_scale,
                                    &vert_scale,
                                    &horiz_phase,
                                    &vert_phase);
                            }
                            else
                            {
                                src_dirty_rect = output_dirty_rect;
                                horiz_scale = vert_scale = 1.0;
                                horiz_phase = vert_phase = 0.0;
                            }

                            T_DEBUG("+1\n");
                            if ((ret = BCMMixerBlit(
                                    mixer->mixer_compositor,
                                    ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                    first_surface,
                                    NULL,
                                    &output_dirty_rect,
                                    &src_dirty_rect,
                                    NULL,
                                    BGFX_RULE_PORTERDUFF_SRC,
                                    true,
                                    horiz_scale,
                                    vert_scale,
                                    horiz_phase,
                                    vert_phase)) != DFB_OK) {
                                D_ERROR("Blit error1 %d\n", (first_surface_layer->nb_surfaces == 1));
                                goto quit;
                            }
                            T_DEBUG("-1\n");

#ifdef ENABLE_MIX_TRACES
                            D_INFO("%s Blit 1.1 i (%d %d %d %d) o (%d %d %d %d)\n",
                            (first_surface_layer->dfb_ext_type == bdvd_dfb_ext_osd_layer) ? "OSD" : ((first_surface_layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) ? "PG" : "IG"),
                            src_dirty_rect.x,
                            src_dirty_rect.y,
                            src_dirty_rect.w,
                            src_dirty_rect.h,
                            output_dirty_rect.x,
                            output_dirty_rect.y,
                            output_dirty_rect.w,
                            output_dirty_rect.h);
#endif

                            output_dirty_rect = output_layer->dirty_rectangles[output_layer->back_surface_index];

                            if ((current_layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT) && (current_layer->nb_surfaces == 1))
                            {
                                dfb_rectangle_intersect( &output_dirty_rect, &current_layer->front_rect);
                                dst2src_xform(
                                    &current_layer->back_rect,
                                    &current_layer->front_rect,
                                    current_layer->surface_buffers[current_layer->front_surface_index]->format,
                                    output_layer->surface_buffers[output_layer->back_surface_index]->format,
                                    &src_dirty_rect,
                                    &output_dirty_rect,
                                    &horiz_scale,
                                    &vert_scale,
                                    &horiz_phase,
                                    &vert_phase);
                            }
                            else
                            {
                                src_dirty_rect = output_dirty_rect;
                                horiz_scale = vert_scale = 1.0;
                                horiz_phase = vert_phase = 0.0;
                            }

                            T_DEBUG("+2\n");
                            if ((ret = BCMMixerBlit(
                                    mixer->mixer_compositor,
                                    ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                    ((bdvd_graphics_surface_h*)current_layer->surface_buffers[current_layer->front_surface_index]->video.ctx)[0],
                                    NULL,
                                    &output_dirty_rect,
                                    &src_dirty_rect,
                                    NULL,
                                    BGFX_RULE_PORTERDUFF_SRCOVER,
                                    true,
                                    horiz_scale,
                                    vert_scale,
                                    horiz_phase,
                                    vert_phase)) != DFB_OK) {
                                D_ERROR("Blit error2\n");
                                goto quit;
                            }
                            T_DEBUG("-2\n");

#ifdef ENABLE_MIX_TRACES
                            D_INFO("%s Blit 1.2 i (%d %d %d %d) o (%d %d %d %d)\n",
                            (current_layer->dfb_ext_type == bdvd_dfb_ext_osd_layer) ? "OSD" : ((current_layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) ? "PG" : "IG"),
                            src_dirty_rect.x,
                            src_dirty_rect.y,
                            src_dirty_rect.w,
                            src_dirty_rect.h,
                            output_dirty_rect.x,
                            output_dirty_rect.y,
                            output_dirty_rect.w,
                            output_dirty_rect.h);
#endif
                        }
                        first_surface = NULL;
                        first_surface_layer = NULL;
                    }
                    else {
                        if (current_layer->bcmcore_type == BCMCORELAYER_FAA) {
                            if (current_layer->bcmcore_visibility == BCMCORELAYER_VISIBLE) {
                            /* When the blit is going to be moved to the graphics plane, then
                             * we are not going to blend anymore.
                             */
                                T_DEBUG("+11\n");
                                src_dirty_rect = current_layer->back_rect;
                                output_dirty_rect = current_layer->front_rect;
                                src2dst_xform(
                                    &current_layer->back_rect,
                                    &current_layer->front_rect,
                                    current_layer->surface_buffers[current_layer->front_surface_index]->format,
                                    output_layer->surface_buffers[output_layer->back_surface_index]->format,
                                    &src_dirty_rect,
                                    &output_dirty_rect,
                                    &horiz_scale,
                                    &vert_scale,
                                    &horiz_phase,
                                    &vert_phase);
                                if ((ret = BCMMixerBlit(
                                        mixer->mixer_compositor,
                                        ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                        ((bdvd_graphics_surface_h*)current_layer->surface_buffers[current_layer->front_surface_index]->video.ctx)[0],
                                        NULL,
                                        &output_dirty_rect,
                                        &src_dirty_rect,
                                        NULL,
                                        current_layer->premultiplied ? BGFX_RULE_PORTERDUFF_SRCOVER : BGFX_RULE_SRCALPHA_INVSRCALPHA,
                                        true,
                                        horiz_scale,
                                        vert_scale,
                                        horiz_phase,
                                        vert_phase)) != DFB_OK) {
                                    goto quit;
                                }
                                T_DEBUG("-11\n");
#ifdef ENABLE_MIX_TRACES
                            D_INFO("%s Blit 0.2 i (%d %d %d %d) o (%d %d %d %d)\n",
                            "FAA",
                            current_layer->back_rect.x,
                            current_layer->back_rect.y,
                            current_layer->back_rect.w,
                            current_layer->back_rect.h,
                            current_layer->front_rect.x,
                            current_layer->front_rect.y,
                            current_layer->front_rect.w,
                            current_layer->front_rect.h);
#endif
                            }
                        }
                        else
                        {
                            output_dirty_rect = output_layer->dirty_rectangles[output_layer->back_surface_index];

                            if ((current_layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT) && (current_layer->nb_surfaces == 1))
                            {
                                dfb_rectangle_intersect( &output_dirty_rect, &current_layer->front_rect);
                                dst2src_xform(
                                    &current_layer->back_rect,
                                    &current_layer->front_rect,
                                    current_layer->surface_buffers[current_layer->front_surface_index]->format,
                                    output_layer->surface_buffers[output_layer->back_surface_index]->format,
                                    &src_dirty_rect,
                                    &output_dirty_rect,
                                    &horiz_scale,
                                    &vert_scale,
                                    &horiz_phase,
                                    &vert_phase);
                            }
                            else
                            {
                                src_dirty_rect = output_dirty_rect;
                                horiz_scale = vert_scale = 1.0;
                                horiz_phase = vert_phase = 0.0;
                            }

                            T_DEBUG("+3\n");
                            if ((ret = BCMMixerBlit(
                                    mixer->mixer_compositor,
                                    ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                                    ((bdvd_graphics_surface_h*)current_layer->surface_buffers[current_layer->front_surface_index]->video.ctx)[0],
                                    NULL,
                                    &output_dirty_rect,
                                    &src_dirty_rect,
                                    NULL,
                                    BGFX_RULE_PORTERDUFF_SRCOVER,
                                    true,
                                    horiz_scale,
                                    vert_scale,
                                    horiz_phase,
                                    vert_phase)) != DFB_OK) {
                                D_ERROR("Blit error3\n");
                                goto quit;
                            }
                            T_DEBUG("-3\n");

#ifdef ENABLE_MIX_TRACES
                            D_INFO("%s Blit 2 i (%d %d %d %d) o (%d %d %d %d)\n",
                            (current_layer->dfb_ext_type == bdvd_dfb_ext_osd_layer) ? "OSD" : ((current_layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) ? "PG" : "IG"),
                            src_dirty_rect.x,
                            src_dirty_rect.y,
                            src_dirty_rect.w,
                            src_dirty_rect.h,
                            output_dirty_rect.x,
                            output_dirty_rect.y,
                            output_dirty_rect.w,
                            output_dirty_rect.h);
#endif
                        }
                    }
                }
            }
            else
            {
                if (current_layer->bcmcore_type == BCMCORELAYER_FAA) {
                    D_ERROR("%s/BCMMixerSync: error must be at least one mixed layer in addition to FAA\n",
                            BCMMixerModuleName);
                    ret = DFB_FAILURE;
                }

                output_dirty_rect = output_layer->dirty_rectangles[output_layer->back_surface_index];

                if ((current_layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT) && (current_layer->nb_surfaces == 1))
                {
                    dfb_rectangle_intersect( &output_dirty_rect, &current_layer->front_rect);
                    dst2src_xform(
                        &current_layer->back_rect,
                        &current_layer->front_rect,
                        current_layer->surface_buffers[current_layer->front_surface_index]->format,
                        output_layer->surface_buffers[output_layer->back_surface_index]->format,
                        &src_dirty_rect,
                        &output_dirty_rect,
                        &horiz_scale,
                        &vert_scale,
                        &horiz_phase,
                        &vert_phase);
                }
                else
                {
                    src_dirty_rect = output_dirty_rect;
                    horiz_scale = vert_scale = 1.0;
                    horiz_phase = vert_phase = 0.0;
                }

#ifdef ENABLE_MIX_TRACES
                D_INFO("src %d %d %d %d dst %d %d %d %d, src_dirty %d %d %d %d dst_dirty %d %d %d %d\n",
                current_layer->src_rect.x,
                current_layer->src_rect.y,
                current_layer->src_rect.w,
                current_layer->src_rect.h,
                current_layer->dst_rect.x,
                current_layer->dst_rect.y,
                current_layer->dst_rect.w,
                current_layer->dst_rect.h,
                src_dirty_rect.x,
                src_dirty_rect.y,
                src_dirty_rect.w,
                src_dirty_rect.h,
                output_dirty_rect.x,
                output_dirty_rect.y,
                output_dirty_rect.w,
                output_dirty_rect.h);
#endif

                T_DEBUG("+4\n");
                T_DEBUG("horiz_scale: %f\n", horiz_scale);
                T_DEBUG("vert_scale: %f\n", vert_scale);
                T_DEBUG("horiz_phase: %f\n", horiz_phase);
                T_DEBUG("vert_phase: %f\n", vert_phase);
                if ((ret = BCMMixerBlit(
                        mixer->mixer_compositor,
                        ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                        ((bdvd_graphics_surface_h*)current_layer->surface_buffers[current_layer->front_surface_index]->video.ctx)[0],
                        NULL,
                        &output_dirty_rect,
                        &src_dirty_rect,
                        NULL,
                        BGFX_RULE_PORTERDUFF_SRC,
                        true,
                        horiz_scale,
                        vert_scale,
                        horiz_phase,
                        vert_phase)) != DFB_OK) {
                    D_ERROR("Blit error4\n");
                    goto quit;
                }
                T_DEBUG("-4\n");

#ifdef ENABLE_MIX_TRACES
                D_INFO("%s Blit 3 i (%d %d %d %d) o (%d %d %d %d)\n",
                (current_layer->dfb_ext_type == bdvd_dfb_ext_osd_layer) ? "OSD" : ((current_layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) ? "PG" : "IG"),
                src_dirty_rect.x,
                src_dirty_rect.y,
                src_dirty_rect.w,
                src_dirty_rect.h,
                output_dirty_rect.x,
                output_dirty_rect.y,
                output_dirty_rect.w,
                output_dirty_rect.h);
#endif

                /*
                 *   Apply clearrect to subpicture layer.
                 */
                if (bdvd_dfb_ext_hddvd_subpicture_layer == current_layer->dfb_ext_type)
                {
                    BCMMixerApplyClearrect(mixer, current_layer, output_layer);
                }
            }



        } /* end for */
        }

        if ((err = bdvd_graphics_compositor_sync(mixer->mixer_compositor)) != b_ok) {
            D_ERROR( "%s/BCMMixerSync: error calling bdvd_graphics_compositor_sync %d\n",
                     BCMMixerModuleName,
                     err );
            ret = DFB_FAILURE;
            goto quit;
        }

        if (dfb_bcmcore_context->secondary_graphics_window.layer_assigned) {
            DFBRectangle dfb_source_rect = dirty_rectangle;
            DFBRectangle dfb_dest_rect;
            BCMCoreLayerHandle secondary_graphics_layer = 
                (BCMCoreLayerHandle)(dfb_bcmcore_context->secondary_graphics_window.layer_assigned);

            src2dst_xform(
                &secondary_graphics_layer->front_rect,
                &output_layer->back_rect,
                secondary_graphics_layer->surface_buffers[secondary_graphics_layer->front_surface_index]->format,
                output_layer->surface_buffers[output_layer->back_surface_index]->format,
                &dfb_source_rect, 
                &dfb_dest_rect,
                &horiz_scale,
                &vert_scale,
                &horiz_phase,
                &vert_phase);

            T_DEBUG("+5\n");
            if ((ret = BCMMixerBlit(
                          mixer->mixer_compositor,
                          ((bdvd_graphics_surface_h*)secondary_graphics_layer->surface_buffers[((BCMCoreLayerHandle)(dfb_bcmcore_context->secondary_graphics_window.layer_assigned))->front_surface_index]->video.ctx)[0],
                          ((bdvd_graphics_surface_h*)output_layer->surface_buffers[output_layer->back_surface_index]->video.ctx)[0],
                          NULL,
                          &dfb_dest_rect,
                          &dfb_source_rect,
                          NULL,
                          BGFX_RULE_PORTERDUFF_SRC,
                          true,
                          horiz_scale,
                          vert_scale,
                          horiz_phase,
                          vert_phase)) != DFB_OK) {
                goto quit;
            }
            T_DEBUG("-5\n");
        }

        memset(&output_layer->dirty_rectangles[output_layer->back_surface_index], 0, sizeof(output_layer->dirty_rectangles[output_layer->back_surface_index]));
    }

quit:

    for (i = 0; i < nb_locked_layers; i++) {
        /* if something on this layer was mixed notify that the mixing was done */
        if ((mixer->mixed_layers_levelsorted[i]->dirty_region.x2 && mixer->mixed_layers_levelsorted[i]->dirty_region.y2) &&
            (mixer->mixed_layers_levelsorted[i]->onsync_flip == true && mixer->mixed_layers_levelsorted[i]->callback != NULL))
        {
            (mixer->mixed_layers_levelsorted[i]->callback)(
                mixer->mixed_layers_levelsorted[i]->callback_param1,
                mixer->mixed_layers_levelsorted[i]->callback_param2,
                bdvd_dfb_ext_event_layer_mixing_done);
        }

        /* reset the dirty regione for the layer */
        memset(&mixer->mixed_layers_levelsorted[i]->dirty_region, 0, sizeof(mixer->mixed_layers_levelsorted[i]->dirty_region));

        /* release the layer */
        if ((ret = BCMMixerLayerRelease(mixer->mixed_layers_levelsorted[i])) != DFB_OK) {
            /* continue */
        }
    }

    if (animator_frozen) {
        if ((ret = BCMAnimatorThawState(mixer->animator)) != DFB_OK) {
            /* continue */
        }
    }

    /* If there is no update, signal an invalid area to avoid
     * BCMMixerSignalFinished being called for nothing.
     */
    if (dirty_rectangle.w && dirty_rectangle.h) {
        ret = DFB_OK;
    }
    else {
        ret = DFB_INVAREA;
    }

    /* unlock the mixer */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSync: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        /* continue */
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerSync exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;
}

/* Sync does now the whole surface and does not account the rect
 * parameter, because the framebuffer surface will not be a
 * flip blit, but rather a real flip.
 * CANNOT LOCK THE MUTEX HERE, aside when flipping because
 * the mixer thread will go sleep with the mixer mutex.
 */
static DFBResult BCMMixerSyncFlip(BCMMixerHandle mixer, bool wait_vsync) {
    DFBResult       ret = DFB_OK;
    bresult         err;

    BCMCoreLayerHandle output_layer = NULL;

    bool            flipping = false;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerSyncFlip called\n",
            BCMMixerModuleName);
#endif

    D_ASSERT( mixer != NULL );

    /* Acquire primary layer */
    if ((ret = BCMMixerOutputLayerAcquire(mixer, &output_layer)) != DFB_OK) {
        goto quit;
    }

    if (output_layer->nb_surfaces == 0) {
    /* The mixer thread depends on the flipping callback, which
     * is not installed when there is only one buffer. This could be changed
     * though, but the implementation is done this way for now.
     * This never should happen really. (see bdvd_graphics_window_set fct)
     */
        D_ERROR("%s/BCMMixerSyncFlip: must at least have one surface\n",
                BCMMixerModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

    /* TODO THIS IS TEMP UNTIL SOLUTION !!!! */
    if ((ret = BCMMixerLayerRelease(output_layer)) != DFB_OK) {
        goto quit;
    }

    if (wait_vsync) err = bdvd_graphics_window_wait_flip(output_layer->graphics_window->window_handle);

    /* Acquire primary layer */
    if ((ret = BCMMixerOutputLayerAcquire(mixer, &output_layer)) != DFB_OK) {
        goto quit;
    }
#if 0
    TODO TOFIX missing initial trigger!!!!!
    switch (err) {
        case b_ok:
        break;
        /* Handling of timeout is only allowed for the mixer thread,
         * this should never be allowed for async operation
         */
        case berr_busy:
            D_DEBUG("%s/BCMMixerSyncFlip: timeout in bdvd_graphics_window_wait_flip\n",
                    BCMMixerModuleName);
            ret = DFB_TIMEOUT;
            goto quit;
        default:
            D_ERROR("%s/BCMMixerSyncFlip: failure in bdvd_graphics_window_wait_flip\n",
                    BCMMixerModuleName);
            ret = DFB_FAILURE;
            goto quit;
    }
#endif

#if 0
    if (mixer->nb_compositors == 2)
    {
        if (mixer->mixerflipblit) {
            if ((err = bdvd_graphics_compositor_sync(mixer->mixer_compositor)) != b_ok) {
                D_ERROR( "%s/BCMMixerSync: error calling bdvd_graphics_compositor_sync %d\n",
                         BCMMixerModuleName,
                         err );
                ret = DFB_FAILURE;
                goto quit;
            }
        }
        else {
            if ((err = bdvd_graphics_compositor_sync(mixer->driver_compositor)) != b_ok) {
                D_ERROR( "%s/BCMMixerSyncFlip: error calling bdvd_graphics_compositor_sync %d\n",
                         BCMMixerModuleName,
                         err );
                ret = DFB_FAILURE;
                goto quit;
            }
        }

        if ((ret = BCMMixerSignalFinished(mixer)) != DFB_OK) {
            goto quit;
        }
    }
#else
	if ((ret = BCMMixerSignalFinished(mixer)) != DFB_OK) {
		goto quit;
	}
#endif

    /* If the output layer is marked as dirty this means flip was
     * called from this layer, in that case skip the sync,
     * but do the flip and wake up waiting threads.
     */
    if (output_layer->dirty_region.x2 && output_layer->dirty_region.y2) {
        memset(&output_layer->dirty_region, 0, sizeof(output_layer->dirty_region));
        flipping = true;
    }
    else {
        ret = BCMMixerSync(mixer,
                           output_layer);
        switch (ret) {
            case DFB_OK:
                flipping = true;
            break;
            /* Invalid area is used as a code when there is nothing to sync,
             * this is used because we still need to trigger a signal on next
             * vsync. If the mixer thread is not used, then this is treated
             * as an error.
             */
            case DFB_INVAREA:
                if (wait_vsync) {
                    if ((err = bdvd_graphics_window_set_vsync_trigger(output_layer->graphics_window->window_handle, true)) != b_ok) {
                        D_ERROR("%s/BCMMixerSyncFlip: failure in bdvd_graphics_window_set_vsync_trigger %d\n",
                                BCMMixerModuleName,
                                err );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                }
                /* we are not flipping */
            break;
            default:
                goto quit;
        }
    }

    if (flipping) {
        uint32_t temp_index = output_layer->back_surface_index;

#ifdef DUMP_SURFACE
        const char * dump_layer_id_string = getenv("dfbbcm_dump_layerid");
#endif
        if (wait_vsync && output_layer->nb_surfaces == 1) {
            if ((err = bdvd_graphics_window_set_vsync_trigger(output_layer->graphics_window->window_handle, true)) != b_ok) {
                   D_ERROR("%s/BCMMixerSyncFlip: failure in bdvd_graphics_window_set_vsync_trigger %d\n",
                            BCMMixerModuleName,
                            err );
                    ret = DFB_FAILURE;
                    goto quit;
            }
        }
        else {
            if((err = bdvd_graphics_window_trigger_flip(output_layer->graphics_window->window_handle, &temp_index)) != b_ok) {
                D_ERROR("%s/BCMMixerSyncFlip: failure in bdvd_graphics_window_trigger_flip %d\n",
                        BCMMixerModuleName,
                        err );
                ret = DFB_FAILURE;
                goto quit;
            }

            /* I have a vsync time to setup clearrect on subvid: only bdvd_dfb_ext_hddvd_subpicture_layer) can turn it on and off */
            if (mixer->subvid_cr_dirty)
            {
                bdvd_display_window_clear_rect(1, mixer->subvid_cr_num, mixer->subvid_cr);
                mixer->subvid_cr_dirty = false;
            }

            if (mixer->cr_notify)
            {
                (*mixer->subpic_cr.clear_rect_done_cb)(mixer->subpic_cr.cb_context);
                mixer->cr_notify = false;
            }
        }

#ifdef DUMP_SURFACE
        if (dump_layer_id_string) {
            if (ret = dfb_surface_dump_buffer( output_layer->dfb_core_surface, 
                output_layer->dfb_core_surface->back_buffer, ".", "framebuffer", 0) != DFB_OK) {
                goto quit;
            }
        }
#endif

        /* The notify in dfb_surface_flip_buffers is causing a deadlock, must find out why.... */
        dfb_surface_flip_buffers_nonotify( output_layer->dfb_core_surface );
        dfb_surface_flip_index_nonotify(
            output_layer->dfb_core_surface->caps,
            &output_layer->front_surface_index,
            &output_layer->back_surface_index,
            &output_layer->idle_surface_index);

        D_ASSERT(output_layer->back_surface_index == temp_index);

#if 0
		if (mixer->nb_compositors == 1) {
            if ((ret = BCMMixerSignalFinished(mixer)) != DFB_OK) {
                goto quit;
            }
        }
#endif
    }

quit:

    if (output_layer) {
        /* unlock the output layer if still locked */
        ret = (BCMMixerLayerRelease(output_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerSyncFlip exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );
#endif

    return ret;
}

/* TEMP MUST MOVE! */
extern unsigned int
system_video_memory_offset_from_virtual( void * addr );

#include "osapi.h"
extern unsigned long g_bdvdMutex; /* This mutex comes from PE. */

/*
 * bgraphics_flip does to much, such as call sync, clear and copy of the
 * backbuffer to front buffer. That's too much for us, let's just do a
 * simple copy instead, plus do that for the region defined and not
 * the whole framebuffer surface which may be up to 1920x1080.
 *
 * Here we decide between flip blit and flip.
 */
DFBResult BCMMixerLayerFlip(BCMMixerHandle mixer,
                            DFBDisplayLayerID layer_id,
                            const DFBRegion * update,
                            bool waitparam,
                            DFBSurfaceFlipFlags flags) {
/* add a wait parameter */
    DFBResult ret = DFB_OK;
    bresult         err;

    DFBRectangle dfb_source_rect;
    DFBRectangle dfb_dest_rect;
    BCMCoreLayerHandle layer = NULL;
    BCMCoreLayerHandle output_layer = NULL;

    bool waitformixer = false;
    bool explicitsync = false;
    bool skipcopy = false;
    bool onsync = false;
    double horiz_scale, vert_scale;
    double horiz_phase, vert_phase;

    bdvd_graphics_surface_memory_t surface_memory;

    BCMCoreContextLayer *layer_ctx = NULL;
    BCMCoreContext *ctx = (BCMCoreContext *)dfb_system_data();

    D_DEBUG("%s/BCMMixerLayerFlip called with %d:\n"
            "layer_id: %u\n",
            BCMMixerModuleName,
            layer_id,
            getpid());

    D_ASSERT( mixer != NULL );

#if 0
    if ((ret = BCMMixerOutputLayerAcquire(mixer, &output_layer)) != DFB_OK) {
        goto quit;
    }

    if (output_layer->nb_surfaces < 2) explicitsync = true;

    if ((ret = BCMMixerLayerRelease(output_layer)) != DFB_OK) {
        goto quit;
    }
#endif

    output_layer = NULL;

    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

	if (layer_ctx->layer_locked_ext == NULL)
	{
		if ((ret = BCMMixerLayerAcquire(mixer, layer_id, &layer)) != DFB_OK) {

			//D_ERROR("Error acquiring the mutex\n");
			goto quit;
		}
	}
	else
	{
		D_DEBUG("Layer has been locked externally\n");
		layer = layer_ctx->layer_locked_ext;
	}

    if ((layer->dfb_ext_type == bdvd_dfb_ext_bd_j_graphics_layer) ||
        (layer->dfb_ext_type == bdvd_dfb_ext_hddvd_ac_graphics_layer))
    {
        layer->onsync_flip = onsync = (flags == DSFLIP_ONSYNC);
    }
    else
    {
        layer->onsync_flip = false;
    }

    switch (layer->bcmcore_type) {
        case BCMCORELAYER_MIXER_FLIPBLIT:
#if 0 /* to accomodate osd and pg when configured as DLBM_FRONTONLY */
            if (layer->nb_surfaces < 2) {
                D_INFO( "%s/BCMMixerLayerFlip: layer must be at least double_buffered, skipping the blit\n",
                         BCMMixerModuleName);
                goto quit;
            }
#endif
            /* A triple buffered mixer layer is not possible, safe checking here */
            D_ASSERT(layer->nb_surfaces == 2);

            if (update) {
                DFBRectangle temp_rectangle;

                dfb_source_rect = layer->back_rect;

                if (!dfb_rectangle_intersect_by_region(&dfb_source_rect, update)) {
                    D_DEBUG("%s/BCMMixerLayerFlip: no intersection between update region and back buffer rectangle, skipping\n",
                            BCMMixerModuleName);
                    goto quit;
                }

                src2dst_xform(
                    &layer->back_rect,
                    &layer->front_rect,
                    layer->surface_buffers[layer->back_surface_index]->format,
                    layer->surface_buffers[layer->front_surface_index]->format,
                    &dfb_source_rect, 
                    &dfb_dest_rect,
                    &horiz_scale,
                    &vert_scale,
                    &horiz_phase,
                    &vert_phase);

                T_DEBUG("%s/BCMMixerLayerFlip: (from update) source rect %d %d %d %d\n",
                        BCMMixerModuleName,
                        dfb_source_rect.x,
                        dfb_source_rect.y,
                        dfb_source_rect.w,
                        dfb_source_rect.h);

                T_DEBUG("%s/BCMMixerLayerFlip: (update region) dest rect %d %d %d %d\n",
                        BCMMixerModuleName,
                        dfb_dest_rect.x,
                        dfb_dest_rect.y,
                        dfb_dest_rect.w,
                        dfb_dest_rect.h);

                temp_rectangle = DFB_RECTANGLE_INIT_FROM_REGION(&layer->dirty_region);
                dfb_rectangle_union ( &temp_rectangle, &dfb_dest_rect );
                layer->dirty_region = DFB_REGION_INIT_FROM_RECTANGLE(&temp_rectangle);

                T_DEBUG("%s/BCMMixerLayerFlip: (update region) dirty region %d %d %d %d\n",
                        BCMMixerModuleName,
                        layer->dirty_region.x1,
                        layer->dirty_region.y1,
                        layer->dirty_region.x2,
                        layer->dirty_region.y2);

                T_DEBUG("%s/BCMMixerLayerFlip: (update region) dirty rec %d %d %d %d\n",
                        BCMMixerModuleName,
                        temp_rectangle.x,
                        temp_rectangle.y,
                        temp_rectangle.w,
                        temp_rectangle.h);
            }
            else {
                dfb_source_rect = layer->back_rect;
                dfb_dest_rect = layer->front_rect;
                src2dst_xform(
                    &layer->back_rect,
                    &layer->front_rect,
                    layer->surface_buffers[layer->back_surface_index]->format,
                    layer->surface_buffers[layer->front_surface_index]->format,
                    &dfb_source_rect, 
                    &dfb_dest_rect,
                    &horiz_scale,
                    &vert_scale,
                    &horiz_phase,
                    &vert_phase);

                T_DEBUG("%s/BCMMixerLayerFlip: (from layer settings) source rect %d %d %d %d\n",
                        BCMMixerModuleName,
                        layer->back_rect.x,
                        layer->back_rect.y,
                        layer->back_rect.w,
                        layer->back_rect.h);

                T_DEBUG("%s/BCMMixerLayerFlip: (from layer settings) dest rect %d %d %d %d\n",
                        BCMMixerModuleName,
                        layer->front_rect.x,
                        layer->front_rect.y,
                        layer->front_rect.w,
                        layer->front_rect.h);

                layer->dirty_region = DFB_REGION_INIT_FROM_RECTANGLE(&layer->front_rect);

                T_DEBUG("%s/BCMMixerLayerFlip: (update layer) region %d %d %d %d\n",
                        BCMMixerModuleName,
                        layer->dirty_region.x1,
                        layer->dirty_region.y1,
                        layer->dirty_region.x2,
                        layer->dirty_region.y2);
            }

            /* hdi layer update: check if we need to flag a clearrect to the subtitle mixer */
            if ((layer->dfb_ext_type == bdvd_dfb_ext_hddvd_ac_graphics_layer) && (dfb_bcmcore_context->hdi_mixer_issue_clearrect == true))
            {
                dfb_bcmcore_context->mixers[BCM_MIXER_0]->subpic_cr_dirty = true;
                dfb_bcmcore_context->hdi_mixer_issue_clearrect = false;
            }

			if (layer->nb_surfaces == 1)
			{
				skipcopy = true; /* no front buffer available, back buffer will be copied directly to primary layers at mixing time */

				memcpy(&layer->src_rect, &dfb_source_rect, sizeof(DFBRectangle));
				memcpy(&layer->dst_rect, &dfb_dest_rect, sizeof(DFBRectangle));

#if 0
				D_DEBUG("%s FLIP (%s) src_rect %d %d %d %d dst_rect %d %d %d %d\n",
					(layer->dfb_ext_type == bdvd_dfb_ext_osd_layer) ? "OSD" : "PG",
					(update) ? "Region" : "Full",
					layer->src_rect.x,
					layer->src_rect.y,
					layer->src_rect.w,
					layer->src_rect.h,
					layer->dst_rect.x,
					layer->dst_rect.y,
					layer->dst_rect.w,
					layer->dst_rect.h);
#endif
			}

            if (!skipcopy)
            {
                if (layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_interactive_layer ||
                    layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) {
                    bdvd_bcc_sync(ctx->driver_bcc_pg);
                    bdvd_bcc_sync(ctx->driver_bcc_ig);
                }

                if (layer->premultiplied) {

                    T_DEBUG("+7\n");
                    T_DEBUG("scale (h, v): %2.4f, %2.4f\n", horiz_scale, vert_scale);
                    T_DEBUG("phase (h, v): %2.4f, %2.4f\n", horiz_phase, vert_phase);
                    if ((ret = BCMMixerBlit(
                                  mixer->mixerflipblit ? mixer->mixer_compositor : mixer->driver_compositor,
                                  ((bdvd_graphics_surface_h*)layer->surface_buffers[layer->front_surface_index]->video.ctx)[0],
                                  ((bdvd_graphics_surface_h*)layer->surface_buffers[layer->back_surface_index]->video.ctx)[0],
                                  NULL,
                                  &dfb_dest_rect,
                                  &dfb_source_rect,
                                  NULL,
                                  BGFX_RULE_PORTERDUFF_SRC,
                                  true,
                                  horiz_scale,
                                  vert_scale,
                                  horiz_phase, 
                                  vert_phase
                                  )) != DFB_OK) {
                        goto quit;
                    }
                    T_DEBUG("-7\n");
                }
                else {
                    T_DEBUG("+8\n");
                    if ((ret = BCMMixerBlit(
                                  mixer->mixerflipblit ? mixer->mixer_compositor : mixer->driver_compositor,
                                  ((bdvd_graphics_surface_h*)layer->surface_buffers[layer->front_surface_index]->video.ctx)[0],
                                  ((bdvd_graphics_surface_h*)layer->surface_buffers[layer->back_surface_index]->video.ctx)[0],
                                  NULL,
                                  &dfb_dest_rect,
                                  &dfb_source_rect,
                                  NULL,
                                  BGFX_RULE_SRCALPHA_ZERO,
                                  true,
                                  horiz_scale,
                                  vert_scale,
                                  horiz_phase, 
                                  vert_phase
                                  )) != DFB_OK) {
                        goto quit;
                    }
                    T_DEBUG("-8\n");
                }

                if (layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_interactive_layer ||
                    layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) {
                    bdvd_graphics_compositor_post_sync(mixer->mixer_compositor);
                    bdvd_graphics_compositor_wait_sync_done(mixer->mixer_compositor);
                }
            }

            if (!onsync)
            {
                if (layer->level != -1) waitformixer = waitparam;
            }
        break;
        case BCMCORELAYER_FAA:
            /* Simply advance the back pointer until the end
             */
            if (++layer->back_surface_index >= layer->nb_surfaces)
                layer->back_surface_index = 0;

            /* Reassign the surfaces manually here! The buffer never move and anyway
             * front is back is idle because it's front only.
             */
            if ((layer->dfb_core_surface->back_buffer = layer->surface_buffers[layer->back_surface_index]) == NULL) {
                D_ERROR("%s/BCMMixerLayerFlip: surface buffer is NULL\n",
                        BCMMixerModuleName);
                ret = DFB_FAILURE;
                goto quit;
            }

            memset(&layer->dirty_region, 0, sizeof(layer->dirty_region));
        break;
        case BCMCORELAYER_MIXER_FLIPTOGGLE:
        case BCMCORELAYER_OUTPUT:
            if (layer->nb_surfaces < 2) {
                D_DEBUG( "%s/BCMMixerLayerFlip: layer must be at least double_buffered, skipping\n",
                         BCMMixerModuleName);
                goto quit;
            }

            if (layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPTOGGLE) {
                /* TOCHECK The notify in dfb_surface_flip_buffers is causing
                 * a deadlock, must find out why, so using temp function instead...
                 */
                dfb_surface_flip_buffers_nonotify( layer->dfb_core_surface );
                dfb_surface_flip_index_nonotify(
                    layer->dfb_core_surface->caps,
                    &layer->front_surface_index,
                    &layer->back_surface_index,
                    &layer->idle_surface_index);
            }

            /* Trigger a flip, but do not mix! do we use the dirty region? */
            /* The flip blit is necessary for examples.
             * Sync is really specific to multilayer implementation, in which
             * case the primary surface should never be used directly. Should
             * never be called on a surface that is not double buffered, but
             * this would generate an error anyway.
             */
            /* Issue a warning? */
            layer->dirty_region = update ? *update : DFB_REGION_INIT_FROM_RECTANGLE(&layer->front_rect);

            waitformixer = true;
        break;
        case BCMCORELAYER_BACKGROUND:
        {
            bdvd_still_picture_t temp;
            bdvd_decode_window_settings_t window_settings;
            bool is_window_set = false;
            bdvd_result_e bdvd_err = bdvd_ok;

#if 0       /* using M2MC to take care of this copy */
			/*PR13209*/
			bdvd_graphics_compositor_post_sync(mixer->mixer_compositor);
			bdvd_graphics_compositor_wait_sync_done(mixer->mixer_compositor);
#endif

            if (layer->level > 0)
            {
				uint32_t width, height;
                temp.bMute                      = false;
                temp.ulDisplayHorizontalSize    = 0;
                temp.ulDisplayVerticalSize      = 0;
                temp.i32_HorizontalPanScan      = 0;
                temp.i32_VerticalPanScan        = 0;
                temp.eAspectRatio               = bdvd_still_display_aspect_ratio_unknown;
                temp.eFrameRateCode             = bdvd_still_display_frame_rate_unknown;
                temp.hSurface                   = ((bdvd_graphics_surface_h*)layer->surface_buffers[layer->back_surface_index]->video.ctx)[0];
                temp.hCompositor                = mixer->driver_compositor;

				/* This Mutex is shared with PE in accessing the UOD api */
				if (g_bdvdMutex)
				{
					OS_SemTake(g_bdvdMutex, OS_WAIT_FOREVER);
				}

                bdvd_err = bdvd_decode_window_get(bdvd_decode_get_window(B_ID(1)), &window_settings);
                if (bdvd_err != bdvd_ok)
                {
                    D_ERROR("%s/BCMMixerLayerFlip:: bdvd_decode_window_get failed with err=0x%x, flip called with background window was closed!!!\n",
                            BCMMixerModuleName, bdvd_err);
                    OS_SemGive(g_bdvdMutex);
                    ret = DFB_FAILURE;
                    goto quit;
                }

                if (bdvd_graphics_window_get_video_format_dimensions(
                    display_get_video_format(dfb_bcmcore_context->primary_display_handle),
                    dfb_surface_pixelformat_to_bdvd(layer->backsurface_clone.format),
                    &width, &height) != b_ok) {
                    D_ERROR("%s/BCMMixerLayerFlip:: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                            BCMMixerModuleName);
                    ret = DFB_FAILURE;

					/* This Mutex is shared with PE */
					if (g_bdvdMutex)
					{
						OS_SemGive(g_bdvdMutex);
					}

                    goto quit;
                }

				is_window_set = (window_settings.position.x == 0) &&
								(window_settings.position.y == 0) &&
								(window_settings.position.width == width) &&
								(window_settings.position.height == height) &&
								(window_settings.cliprect.x == 0) &&
								(window_settings.cliprect.y == 0) &&
								(window_settings.cliprect.width == 0) &&
								(window_settings.cliprect.height == 0) &&
								(window_settings.visible == true) &&
								(window_settings.zorder == 0) &&
								(window_settings.override_display_content_mode == true) &&
								(window_settings.content_mode == bdvd_display_content_mode_full) &&
								(window_settings.alpha_value == 0xFF) &&
								(window_settings.deinterlacer == false);

                if ((bdvd_decode_is_still_display_on(bdvd_decode_get_window(B_ID(1))) == true) && (is_window_set == false))
                {
					window_settings.position.x = 0;
					window_settings.position.y = 0;
					window_settings.position.width = width;
					window_settings.position.height = height;
					window_settings.cliprect.x = 0;
					window_settings.cliprect.y = 0;
					window_settings.cliprect.width = 0;
					window_settings.cliprect.height = 0;
					window_settings.visible = true;
					window_settings.zorder = 0;
					window_settings.override_display_content_mode = true;
					window_settings.content_mode = bdvd_display_content_mode_full;
					window_settings.alpha_value = 0xFF;
					window_settings.deinterlacer = false;

                	bdvd_decode_window_set(bdvd_decode_get_window(B_ID(1)), &window_settings);
				}

				/* This Mutex is shared with PE */
				if (g_bdvdMutex)
				{
					OS_SemGive(g_bdvdMutex);
				}

                if (bdvd_decode_window_set_picture(bdvd_decode_get_window(B_ID(1)), &temp) != b_ok)
                {
                    D_ERROR("%s/BCMMixerLayerFlip:: bdvd_decode_window_set_picture failed\n",
                             BCMMixerModuleName );
                    ret = DFB_FAILURE;

                    goto quit;
                }

                /* This is temporary and to make sure the picture was set (wait for flip) */
                if (bdvd_decode_window_get_picture(bdvd_decode_get_window(B_ID(1)), &temp) != b_ok) {
                    D_ERROR("%s/BCMMixerLayerFlip:: bdvd_decode_window_get_picture failed\n",
                             BCMMixerModuleName );
                    ret = DFB_FAILURE;

                    goto quit;
                }
            }
            else
            {

				/* This Mutex is shared with PE in accessing the UOD api */
				if (g_bdvdMutex)
				{
					OS_SemTake(g_bdvdMutex, OS_WAIT_FOREVER);
				}

                bdvd_err = bdvd_decode_window_get(bdvd_decode_get_window(B_ID(1)), &window_settings);
                if (bdvd_err != bdvd_ok)
                {
                    D_ERROR("%s/BCMMixerLayerFlip:: bdvd_decode_window_get failed with err=0x%x, flip called with background window was closed!!!\n",
                            BCMMixerModuleName, bdvd_err);
                    OS_SemGive(g_bdvdMutex);
                    ret = DFB_FAILURE;
                    goto quit;
                }

				is_window_set = (window_settings.visible == false) &&
								(window_settings.deinterlacer == false);
                if ((bdvd_decode_is_still_display_on(bdvd_decode_get_window(B_ID(1))) == true) && (is_window_set == false))
                {
					window_settings.visible = false;
					window_settings.deinterlacer = false;
                	bdvd_decode_window_set(bdvd_decode_get_window(B_ID(1)), &window_settings);
				}

				/* This Mutex is shared with PE */
				if (g_bdvdMutex)
				{
					OS_SemGive(g_bdvdMutex);
				}
            }
        }
        break;
        default:
        /* TODO */
        break;
    }

quit:

    if (output_layer) {
        ret = (BCMMixerLayerRelease(output_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    if (layer) {
        int rc;
        switch (layer->dfb_ext_type) {
            case bdvd_dfb_ext_bd_j_background_layer:
                if (layer) {
                    /* unlock the layer if still locked */
                    ret = (BCMMixerLayerRelease(layer) == DFB_OK) ? ret : DFB_FAILURE;
                }
            break;
            case bdvd_dfb_ext_bd_j_graphics_layer:
            /*case bdvd_dfb_ext_hddvd_ac_graphics_layer:*/
#if 0
                /* unlock the layer if still locked */
                ret = (BCMMixerLayerRelease(layer) == DFB_OK) ? ret : DFB_FAILURE;
                /* fall through */

                layer = NULL;

                if ((rc = pthread_mutex_lock(&mixer->mutex))) {
                    D_ERROR("%s/BCMMixerLayerFlip: error locking mixer mutex    --> %s\n",
                            BCMMixerModuleName,
                            strerror(rc));
                    /* fall through */
                }

                if (mixer->nb_layers_levelsorted < 3) {
                    explicitsync = true;
                }

                /* unlock the mixer */
                if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
                    D_ERROR("%s/BCMMixerLayerFlip: error unlocking mixer mutex    --> %s\n",
                            BCMMixerModuleName,
                            strerror(rc));
                    /* continue */
                }

                if (!explicitsync) {
                    if ((ret = BCMMixerLayerAcquire(mixer, layer_id, &layer)) != DFB_OK) {
                        /* fall through */
                    }
                }
                /* continue */
#else
#if 0
                waitformixer = false;
#endif
#endif

            default:
            	if (!skipcopy)
            	{
					if (waitformixer && !explicitsync) {
						if ((ret = BCMMixerWaitFinished(mixer, layer)) != DFB_OK) {
							/* fall through */
						}
					}
					else {
						if (!onsync &&
							!explicitsync &&
							mixer->mixerflipblit /*&&
							mixer->nb_compositors == 2*/){
							if ((err = bdvd_graphics_compositor_sync(mixer->mixer_compositor)) != b_ok) {
								D_ERROR( "%s/BCMMixerLayerFlip: error calling bdvd_graphics_compositor_sync %d\n",
										 BCMMixerModuleName,
										 err );
								ret = DFB_FAILURE;
								/* fall through */
							}
						}

						if (layer) {
							/* unlock the layer if still locked */
							ret = (BCMMixerLayerRelease(layer) == DFB_OK) ? ret : DFB_FAILURE;
							/* fall through */
						}

						if (explicitsync) {
							if ((ret = BCMMixerSyncFlip(mixer, false)) != DFB_OK) {
								ret = (ret == DFB_INVAREA) ? DFB_OK : ret;
								/* fall through */
							}
						}
					}
				}
				else
				{
					if (layer) {

						ret = BCMMixerLayerRelease(layer);
                        if (ret)
                        {
                            D_DEBUG("%s/BCMMixerLayerFlip: BCMMixerLayerRelease failed with code %d\n",
                                    BCMMixerModuleName,
                                    (int)ret);
                        }
						layer_ctx->layer_locked_ext = NULL;
					}
				}
            break;
        }
    }

    D_DEBUG("%s/BCMMixerLayerFlip exiting with code %d\n",
            BCMMixerModuleName,
            (int)ret );

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called while owning one of the mixer's layer mutex.
 */
DFBResult
BCMMixerAttachAnimator(BCMMixerHandle mixer) {

    DFBResult ret = DFB_OK;
    int rc;

    D_ASSERT( mixer != NULL );

    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAttachAnimator: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if (mixer->animator == NULL) {
        mixer->animator = D_CALLOC( 1, sizeof(*mixer->animator) );
        if ((ret = BCMAnimatorOpen(mixer->animator)) != DFB_OK) {
            goto quit;
        }
    }
    else {
        D_DEBUG("%s/BCMMixerAttachAnimator: mixer already has an animator\n",
                BCMMixerModuleName);
    }

quit:

    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAttachAnimator: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called while owning one of the mixer's layer mutex.
 */
DFBResult
BCMMixerAnimatorSetVideoRate(BCMMixerHandle mixer,
                             bdvd_dfb_ext_frame_rate_e framerate) {
    DFBResult ret = DFB_OK;
    int rc;

    D_ASSERT( mixer != NULL );

    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAnimatorSetVideoRate: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if ((ret = BCMAnimatorSetVideoRate(mixer->animator, framerate)) != DFB_OK) {
        goto quit;
    }

quit:

    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAnimatorSetVideoRate: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called while owning one of the mixer's layer mutex.
 */
DFBResult
BCMMixerAnimatorSetVideoStatus(BCMMixerHandle mixer,
                               bool video_running) {
    DFBResult ret = DFB_OK;
    int rc;

    D_ASSERT( mixer != NULL );

#if 0
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAnimatorSetVideoStatus: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }
#endif

    if ((ret = BCMAnimatorSetVideoStatus(mixer->animator, video_running)) != DFB_OK) {
        goto quit;
    }



quit:

#if 0
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAnimatorSetVideoStatus: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }
#endif

    return ret;
}

DFBResult
BCMMixerAnimatorUpdateCurrentPTS(BCMMixerHandle mixer,
                                 uint32_t current_pts) {
    DFBResult ret = DFB_OK;

    D_ASSERT( mixer != NULL );

#if 0 /* Not locking the mixer for now, we are in a callback */
    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAnimatorUpdateCurrentPTS: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }
#endif

    if ((ret = BCMAnimatorUpdateCurrentPTS(mixer->animator, current_pts)) != DFB_OK) {
        goto quit;
    }

quit:

#if 0 /* Not locking the mixer for now, we are in a callback */
    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerAnimatorUpdateCurrentPTS: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }
#endif

    return ret;
}

/* WARNING:
 * Potential deadlock with the mixer thread. This function should never be
 * called while owning one of the mixer's layer mutex.
 */
DFBResult
BCMMixerDetachAnimator(BCMMixerHandle mixer) {

    DFBResult ret = DFB_OK;
    int rc;
    bool still_faa = false;
    uint32_t i;

    D_ASSERT( mixer != NULL );

    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerDetachAnimator: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    /* For now look if there is any more FAA types */
    for (i = 0; i < (mixer->max_layers-1); i++) {
        if (mixer->mixed_layers[i]) {
            if (mixer->mixed_layers[i]->bcmcore_type == BCMCORELAYER_FAA) {
                still_faa = true;
                break;
            }
        }
    }

    if (!still_faa) {
        D_ASSERT(mixer->animator);

        if ((ret = BCMAnimatorClose(mixer->animator)) != DFB_OK) {
            goto quit;
        }
        D_FREE(mixer->animator);
        mixer->animator = NULL;
    }
    else {
        D_DEBUG("%s/BCMMixerAttachAnimator: mixer already has an animator\n",
                BCMMixerModuleName);
    }

quit:

    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerDetachAnimator: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

DFBResult
BCMMixerSetOutputLayerSurfaces(BCMMixerHandle mixer, BCMCoreLayerHandle output_layer) {
    DFBResult ret = DFB_OK;
    int rc;

    bdvd_graphics_window_settings_t graphics_settings;

    D_ASSERT( mixer != NULL );
    D_ASSERT( output_layer != NULL );

    if ((rc = pthread_mutex_lock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSetOutputLayerSurfaces: error locking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    bdvd_graphics_window_get(output_layer->graphics_window->window_handle, &graphics_settings);
    if (graphics_settings.framebuffers) {
        D_FREE(graphics_settings.framebuffers);
        graphics_settings.framebuffers = NULL;
    }
    graphics_settings.nb_framebuffers = 0;
    graphics_settings.enabled = false;
    if (bdvd_graphics_window_set(output_layer->graphics_window->window_handle, &graphics_settings) != b_ok) {
        D_ERROR( "%s/BCMMixerSetOutputLayerSurfaces: error setting graphic framebuffer surface\n",
                 BCMMixerModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    if (output_layer->nb_surfaces) {
        int i;

        bdvd_graphics_window_get(output_layer->graphics_window->window_handle, &graphics_settings);
        graphics_settings.nb_framebuffers = output_layer->nb_surfaces;
        if ((graphics_settings.framebuffers = D_CALLOC( graphics_settings.nb_framebuffers, sizeof(bdvd_graphics_surface_h) )) == NULL) {
            D_ERROR( "%s/BCMMixerSetOutputLayerSurfaces: failed to create graphics_settings.framebuffers\n",
                     BCMMixerModuleName );
            ret = DFB_FAILURE;
            goto quit;
        }
        for (i = 0; i < graphics_settings.nb_framebuffers; i++) {
            graphics_settings.framebuffers[i] = ((bdvd_graphics_surface_h*)output_layer->surface_buffers[i]->video.ctx)[0];
        }

        /* TODO set alpha_premultiplied depending on output_layer->premultiplied */
        /* We are always in a explicit wait flip mode when using the
         * mixer thread.
         */
        graphics_settings.explicit_wait_flip = true;

        if (bdvd_graphics_window_set(output_layer->graphics_window->window_handle, &graphics_settings) != b_ok) {
            D_ERROR( "%s/BCMMixerSetOutputLayerSurfaces: error setting graphic framebuffer surface\n",
                     BCMMixerModuleName );
            ret = DFB_FAILURE;
            goto quit;
        }
    }

quit:

    if ((rc = pthread_mutex_unlock(&mixer->mutex))) {
        D_ERROR("%s/BCMMixerSetOutputLayerSurfaces: error unlocking mixer mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

DFBResult
BCMMixerWaitFinished(BCMMixerHandle mixer, BCMCoreLayerHandle layer) {

    DFBResult ret = DFB_OK;
    int rc;

    D_ASSERT( mixer != NULL );
    D_ASSERT( layer != NULL );

    /* No goto error before this */
    if ((rc = pthread_mutex_lock(&mixer->cond_mutex))) {
        D_ERROR("%s/BCMMixerWaitFinished: error locking condition mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

    if ((ret = BCMMixerLayerRelease(layer)) != DFB_OK) {
        goto quit;
    }

    D_DEBUG("%s/BCMMixerWaitFinished waiting\n"
            "layer id: %u\n",
            BCMMixerModuleName,
            layer->layer_id);

    if ((rc = pthread_cond_wait(&mixer->cond, &mixer->cond_mutex))) {
        D_ERROR("%s/BCMMixerWaitFinished: error waiting for condition    --> %s",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* continue */
    }

    D_DEBUG("%s/BCMMixerWaitFinished done\n"
            "layer id: %u\n",
            BCMMixerModuleName,
            layer->layer_id);

quit:

    if ((rc = pthread_mutex_unlock(&mixer->cond_mutex))) {
        D_ERROR("%s/BCMMixerWaitFinished: error unlocking condition mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
    }

    return ret;
}

DFBResult BCMMixerSignalFinished(BCMMixerHandle mixer) {

    DFBResult ret = DFB_OK;
    int rc;

    D_ASSERT( mixer != NULL );

    /* No goto error before this */
    if ((rc = pthread_mutex_lock(&mixer->cond_mutex))) {
        D_ERROR("%s/BCMMixerSignalFinished: error locking condition mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        return DFB_FAILURE;
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMMixerSignalFinished broadcasting\n",
            BCMMixerModuleName);
#endif

    if ((rc = pthread_cond_broadcast(&mixer->cond))) {
        D_ERROR("%s/BCMMixerSignalFinished: error broadcasting condition    --> %s",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        /* continue */
    }

    if ((rc = pthread_mutex_unlock(&mixer->cond_mutex))) {
        D_ERROR("%s/BCMMixerLayerAcquire: error unlocking condition mutex    --> %s\n",
                BCMMixerModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
    }

    return ret;
}
