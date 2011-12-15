/**
 * @date May 2006
 *
 * Copyright (c) 2005-2009, Broadcom Corporation
 *    All Rights Reserved
 *    Confidential Property of Broadcom Corporation
 *
 * THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 * AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 * EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 */
#ifndef __BCMLAYER_H__
#define __BCMLAYER_H__

#include <core/coretypes.h>
#include <core/surfaces.h>

#include <bdvd.h>

typedef struct {
    DFBDisplayLayerID        layer_id;
    void *                   layer_assigned;
    pthread_mutex_t          lock;
} BCMCoreVgraphicsWindowContext;

typedef struct {
    bdvd_display_h           display_handle;
    bdvd_graphics_window_h   window_handle;
    DFBDisplayLayerID        layer_id;
    void *                   layer_assigned;
    pthread_mutex_t          lock;
} BCMCoreGraphicsWindowContext;

/* don't use dfb_layer_id_translated, because DLID_PRIMARY will
 * always be layer_id 0 in our case
 */
/*DFBDisplayLayerID
dfb_layer_id( const CoreLayer *layer )
*/
typedef enum {
    BCMCORELAYER_INVALID = 0,
    BCMCORELAYER_OUTPUT,
    BCMCORELAYER_MIXER_FLIPBLIT,
    BCMCORELAYER_MIXER_FLIPTOGGLE,
    BCMCORELAYER_FAA,
    BCMCORELAYER_BACKGROUND
} BCMCoreLayerType;

typedef enum {
    BCMCORELAYER_HIDDEN = 0,
    BCMCORELAYER_VISIBLE
} BCMCoreLayerVisibility;

/* This is really the maximum of frame allowed by BD-J FAA, TBD */
#define BCMCORELAYER_MAXIMUM_SURFACES 256

typedef struct BCMCoreLayer *BCMCoreLayerHandle;

typedef struct {
    uint32_t next_front_surface_index;
    uint32_t repeat_sequence_index;
    uint32_t repeat_sequence_count;
    int increment_sign;
    int increment;
    bool frame_advance_expected;
} BCMCoreLayerAnimationContext;

typedef enum {
	BCMCORELAYER_UNLOCKED,
	BCMCORELAYER_LOCKED
} BCMCoreLayerLockState;

struct BCMCoreLayer {
    CoreLayer                *dfb_core_layer;
    char                      layer_name[DFB_DISPLAY_LAYER_DESC_NAME_LENGTH];
    DFBDisplayLayerID         layer_id;
    /* Cannot use dfb_layer_id because CoreLayer->shared->layer_id does
     * not exist until dfb_layers_initialize (invoked once at core creation
     * time by dfb_core_initialize) is called. This poses a problem for when
     * using BCMMixer fct in InitLayer for example, so duplicating id
     * here for internal use by BCMMixer fcts.
     */
    CoreSurface              *dfb_core_surface;
    SurfaceBuffer            *surface_buffers[BCMCORELAYER_MAXIMUM_SURFACES];
    DFBRectangle              dirty_rectangles[BCMCORELAYER_MAXIMUM_SURFACES];

    CoreSurface               frontsurface_clone;
    CoreSurface               backsurface_clone;

    uint32_t                  front_surface_index;
    uint32_t                  back_surface_index;
    uint32_t                  idle_surface_index;
    size_t                    nb_surfaces;
    bool                      premultiplied;

    DFBLocation               location;
    DFBRectangle              front_rect;
    DFBRectangle              back_rect;

    /* This is not stored anywhere else */
    int                       level; /* z position/order */
    /* This is the ressource wise type */
    BCMCoreLayerType          bcmcore_type;
    BCMCoreLayerVisibility    bcmcore_visibility;

    /* This is the extension type and settings */
    bdvd_dfb_ext_layer_type_e      dfb_ext_type;
    bdvd_dfb_ext_type_settings_t   dfb_ext_settings;
    bdvd_dfb_ext_animation_stats_t dfb_ext_animation_stats;
    bool                           dfb_ext_force_clean_on_stop;
    uint32_t                       frame_mixing_time;

    BCMCoreLayerAnimationContext  animation_context;

    /* This is for output layers */
    BCMCoreGraphicsWindowContext  *graphics_window;

    /* Sync stuff */
    DFBRegion                 dirty_region;

    pthread_mutex_t mutex;
    /* Must be recursive because all the functions are synchronised with this
     * mutex, we don't want the owning thread to deadlock itself
     */
    pthread_mutexattr_t mutex_attr;

    bdvd_dfb_ext_callback_t callback;
    void                    *callback_param1;
    int                      callback_param2;
    bool                    onsync_flip;

    DFBRectangle            src_rect;
    DFBRectangle            dst_rect;
	BCMCoreLayerLockState lock_state;

	bool                    is_layer_mixed;
};

#endif /* __BCMLAYER_H__ */
