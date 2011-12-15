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
 *
 *  Environment variables
 *
 *  In additional of normal usage through the DirectFB API, it's possible to
 *  export the following environment variables prior to running DirectFB
 *  applications to changes some of the settings:
 *
 *  BCM_DISPLAY defaults at 0, be can be set up to B_N_DISPLAYS
 *
 *  i.e. export BCM_DISPLAY=1
 *
 *  BCM_GRAPHICS_PIXELFORMAT defaults to DSPF_ARGB, but can be set to the
 *  following values:
 *  R5G6B5 -> DSPF_RGB16
 *  A1R5G5B5 -> DSPF_ARGB1555
 *
 *  i.e. export BCM_GRAPHICS_PIXELFORMAT=R5G6B5
 *
 *  Compilation defines
 *
 *  Define BCMUSESLINUXINPUTS if you plan to use the linux_input module
 *  with a keyboard with either console input or kernel event input.
 *
 *  Define OPTIMIZED_RTS_SETTINGS if you have settings for the
 *  7038 RTS modules for support necessary bandwidth of 1080i graphics
 *  surfaces at 32-bit per pixel.
 *
 *  - @ref StaticFunctions "Static Functions": Functions internal to bcmcore
 *  module.
 *  - @ref SystemCoreInterface "System core interface": Interface declared by <src/core/core_system.h>
 *  - @ref LayerInterface "Layer interface": Interface declared by <src/core/layers.h>
 *  - @ref ScreenInterface "Screen interface": Interface declared by <src/core/screens.h>
 */
#include <config.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#if defined(HAVE_SYSIO)
# include <sys/io.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/kd.h>

#include <pthread.h>

#include <fusion/shmalloc.h>
#include <fusion/reactor.h>
#include <fusion/arena.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/layer_control.h>
#include <core/layer_context.h>
#include <core/layers_internal.h>
#include <core/layers.h>
#include <core/palette.h>
#include <core/screen.h>
#include <core/screens.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/state.h>
#include <core/windows.h>

#include <gfx/convert.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/signals.h>
#include <direct/system.h>
#include <direct/util.h>

#include <misc/conf.h>
#include <misc/util.h>

#include "bcmlayer.h"
#include "bcmanimator.h"
#include "bcmmixer.h"
#include "bcmcore.h"
#include "bcmcore_fcts.h"

#include <core/core_system.h>

#include <assert.h>

// #ifdef NDEBUG
// #error 1
// #endif

#define UNUSED_PARAM(p) (void)p

/** @defgroup StaticFunctions Static Functions
 *  Functions internal to bcmcore
 */

/** @defgroup SystemCoreInterface System core interface
 *  Interface declared by <src/core/core_system.h>
 */

/** @defgroup LayerInterface Layer interface
 *  Interface declared by <src/core/layers.h>
 */

/** @defgroup ScreenInterface Screen interface
 *  Interface declared by <src/core/screens.h>
 */

DFB_CORE_SYSTEM( bcm )

const char BCMCoreModuleName[] = "DirectFB/BCMCore";

#define BCMCoreModuleVersionMajor 2
#define BCMCoreModuleVersionMinor 1

BCMCoreContext * dfb_bcmcore_context = NULL; /* !< Global variable used as a common context */

static DFBResult system_p_init(BCMCoreContext * ctx);

extern const char *bsettop_get_config(const char *name);

/* #define DEBUG_ALLOCATION */

#ifdef DEBUG_ALLOCATION
static bool show_allocation;
#endif

#ifdef BCMUSESLINUXINPUTS



/** @addtogroup StaticFunctions
 *  The following is a grouping of functions used when
 *  BCMUSESLINUXINPUTS is defined, that is when the linux_input
 *  is used
 *  @{
 */

/**
 *  This function opens the tty device later used by linux_input module
 *  for keyboard input events.
 *  @return
 *      DFBResult DFB_OK on success, error code otherwise.
 *  @see vt_kbd_input_close
 */
static DFBResult
vt_kbd_input_open()
{
     char buf[32];

     dfb_bcmcore_context->vt = D_CALLOC( 1, sizeof(VirtualTerminal) );

     /* tty0 is serial console, dummy console is 1 */
     dfb_bcmcore_context->vt->num = 1;

     D_INFO( "%s/vt_kbd_input_open: opening /dev/tty%d for keyboard input\n",
             BCMCoreModuleName,
             dfb_bcmcore_context->vt->num,
             dfb_bcmcore_context->vt->num );

     snprintf(buf, 32, "/dev/tty%d", dfb_bcmcore_context->vt->num);
     dfb_bcmcore_context->vt->fd = open( buf, O_RDWR );
     if (dfb_bcmcore_context->vt->fd < 0) {
          if (errno == ENOENT) {
               snprintf(buf, 32, "/dev/vc/%d", dfb_bcmcore_context->vt->num);
               dfb_bcmcore_context->vt->fd = open( buf, O_RDWR );
               if (dfb_bcmcore_context->vt->fd < 0) {
                    if (errno == ENOENT) {
                         D_PERROR( "%s/vt_kbd_input_open: couldn't open "
                                   "neither /dev/tty%d nor /dev/vc/%d!\n",
                                   BCMCoreModuleName,
                                   dfb_bcmcore_context->vt->num,
                                   dfb_bcmcore_context->vt->num );
                    }
                    else {
                         D_PERROR( "%s/vt_kbd_input_open: error opening %s!\n",
                                   BCMCoreModuleName,
                                   buf );
                    }

                    return errno2result( errno );
               }
          }
          else {
               D_PERROR( "%s/vt_kbd_input_open: Error opening %s!\n",
                         BCMCoreModuleName,
                         buf );
               return errno2result( errno );
          }
     }

     return DFB_OK;
}

/**
 *  This function closes the tty device later used by linux_input module
 *  for keyboard input events.
 *  @return
 *      DFBResult DFB_OK on success, error code otherwise.
 *  @see vt_kbd_input_open
 */
static DFBResult
vt_kbd_input_close() {
     close( dfb_bcmcore_context->vt->fd );
     D_FREE( dfb_bcmcore_context->vt );
     dfb_bcmcore_context->vt = NULL;
     return DFB_OK;
}

/** @} */

#endif

/**
 *  FormatString is a structure defining the string for the
 *  DFBSurfacePixelFormat types. This is used for debug traces
 *  and was taken out of src/misc/conf.c.
 */
typedef struct {
     const char            *string;
     DFBSurfacePixelFormat  format;
} FormatString;

/**
 *  format_strings is an array of FormatString used for debug traces.
 *  Was taken out of src/misc/conf.c and is qsort in system_initialize.
 */
static FormatString format_strings[] = {
     { "A8",       DSPF_A8       },
     { "ALUT44",   DSPF_ALUT44   },
     { "ARGB",     DSPF_ARGB     },
     { "ARGB1555", DSPF_ARGB1555 },
     { "ARGB2554", DSPF_ARGB2554 },
     { "ARGB4444", DSPF_ARGB4444 },
     { "AiRGB",    DSPF_AiRGB    },
     { "I420",     DSPF_I420     },
     { "LUT8",     DSPF_LUT8     },
     { "NV12",     DSPF_NV12     },
     { "NV21",     DSPF_NV21     },
     { "NV16",     DSPF_NV16     },
     { "RGB16",    DSPF_RGB16    },
     { "RGB24",    DSPF_RGB24    },
     { "RGB32",    DSPF_RGB32    },
     { "RGB332",   DSPF_RGB332   },
     { "UYVY",     DSPF_UYVY     },
     { "YUY2",     DSPF_YUY2     },
     { "YV12",     DSPF_YV12     }
};

#define NUM_FORMAT_STRINGS (sizeof(format_strings) / sizeof(FormatString)) //!< Used with bsearch and qsort call

/** @addtogroup StaticFunctions
 *  The following is a grouping of all functions used for conversion
 *  from DFBSurfacePixelFormat to string
 *  @{
 */

static int
pixelformat_compare (const void *key,
                     const void *base)
{
     return (((const FormatString *) key)->format - ((const FormatString *) base)->format);
}

/** @} */

/** @addtogroup StaticFunctions
 *  The following is a grouping of all functions used for conversion
 *  of bdvd_video_format_e
 *  @{
 */

/**
 *  This function creates the bgraphics frame buffer
 *  surface. the bdvd_video_format_e for display
 *  handle.
 *  @return
 *      bdvd_graphics_surface_h handle to the frame buffer surface
 */
static DFBResult
create_graphics_framebuffers(BCMMixerHandle mixer, BCMCoreLayerHandle layer, CoreSurface * surface, CoreSurfacePolicy surface_policy )
{
    DFBResult ret = DFB_OK;
    bresult err;
    uint32_t i;

    D_ASSERT( layer != NULL );
    D_ASSERT( surface != NULL );

    if (surface->format != DSPF_ARGB) {
        D_WARN( "%s/create_graphics_framebuffers: bgraphics framebuffer should really be\n"
                "DSPF_ARGB format to keep alphachannel\n",
                BCMCoreModuleName );
    }

    /* [xm] Should always put maximum (bdvd_video_format_1080i),
     * because there is no way to reallocate
     * the frame buffer when changing the output resolution. Instead just
     * resize the region.
     */
    if (bdvd_graphics_window_get_video_format_dimensions(
            bdvd_video_format_1080i,
            dfb_surface_pixelformat_to_bdvd(surface->format),
            &surface->width, &surface->height) != b_ok) {
        D_ERROR("%s/create_graphics_framebuffers: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                BCMCoreModuleName);
        ret = DFB_FAILURE;
        goto error;
    }

    for (i = 0; i < layer->nb_surfaces; i++) {
        if (dfb_surface_allocate_buffer( surface,
                                         surface_policy,
                                         surface->format,
                                         &layer->surface_buffers[i] ) != DFB_OK) {
            D_ERROR("%s/create_graphics_framebuffers: error creating graphics framebuffer surface\n",
                     BCMCoreModuleName );
            ret = DFB_FAILURE;
            goto error;
        }

        if ((err = bdvd_graphics_surface_clear(dfb_bcmcore_context->driver_compositor, ((bdvd_graphics_surface_h*)layer->surface_buffers[i]->video.ctx)[0])) != b_ok) {
            D_ERROR("%s/create_graphics_framebuffers: error calling bdvd_graphics_surface_clear %d\n",
                     BCMCoreModuleName,
                     err );
            ret = DFB_FAILURE;
            goto error;
        }

        D_DEBUG("%s/create_graphics_framebuffers: Allocated bgraphics framebuffer [%p]:\n"
                "format: %d\n"
                "width: %d\n"
                "height: %d\n",
                BCMCoreModuleName,
                ((bdvd_graphics_surface_h*)layer->surface_buffers[i]->video.ctx)[0],
                surface->format,
                surface->width,
                surface->height);
    }

    if ((ret = BCMMixerSetOutputLayerSurfaces(mixer, layer)) != DFB_OK) {
        goto error;
    }

error:

    return ret;
}

static DFBResult BCMCore_load_palette( CoreSurface *surface,
                               CorePalette *palette ) {
    DFBResult ret = DFB_OK;
    bresult err;

    D_ASSERT( surface != NULL );
    D_ASSERT( palette != NULL );

    if ((err = bdvd_graphics_surface_set_palette(
            ((bdvd_graphics_surface_h*)surface->back_buffer->video.ctx)[0],
            (bdvd_graphics_palette_h)palette->ctx)) != b_ok) {
        D_ERROR("%s/BCMCore_load_palette: error calling bdvd_graphics_surface_set_palette %d\n", BCMCoreModuleName, err );
        ret = DFB_FAILURE;
    }

    return ret;
}

/** @} */

/** @addtogroup ExportedFunctions
 *  @{
 */

/* These will propagate the settings for the bdvd_dfb_ext to the core module */
DFBResult
BCMCoreLayerRegister(bdvd_dfb_ext_layer_type_e layer_type,
                     DFBDisplayLayerID * layer_id) {
    int rc = 0;
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMMixerHandle     bcm_mixer = NULL;
    bool attach_animator = false;

    bool flipmod = false;
    char * getenv_string;

    DFBDisplayLayerID layer_index;

    if ((rc = pthread_mutex_lock(&dfb_bcmcore_context->bcm_layers.mutex))) {
        D_ERROR("%s/BCMCoreLayerRegister: error locking mixer mutex    --> %s\n",
            BCMCoreModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

    /* sanity check: check if a layer of the same type is already open */
    for (layer_index = 0; layer_index< MAX_LAYERS; layer_index++)
    {
        if (dfb_bcmcore_context->bcm_layers.layer[layer_index].handle->dfb_ext_type == layer_type)
        {
            D_ERROR("%s/BCMCoreLayerRegister: ERROR ERROR ERROR Layer %d is already open!!! You MUST close it before open it again\n", BCMCoreModuleName, layer_type);
        }
    }

    /* find the first available bcm layer */
    for (layer_index = 0; layer_index< MAX_LAYERS; layer_index++)
    {
        if (dfb_bcmcore_context->bcm_layers.layer[layer_index].handle->bcmcore_type == BCMCORELAYER_INVALID)
        {
            bcm_layer = dfb_bcmcore_context->bcm_layers.layer[layer_index].handle;
            *layer_id = layer_index;
            break;
        }
    }

    if ((rc = pthread_mutex_unlock(&dfb_bcmcore_context->bcm_layers.mutex))) {
        D_ERROR("%s/BCMCoreLayerRegister: error unlocking mixer mutex    --> %s\n",
                BCMCoreModuleName,
                strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

    if (bcm_layer == NULL) {
        D_ERROR("%s/BCMCoreLayerRegister: no available layer found\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

#if 1 /* LGEBDJMOD */
    getenv_string = getenv( "dfbbcm_bdj_flipmod" );
    if (getenv_string) {
        if (!strncmp(getenv_string, "y", 1)) {
            flipmod = true;
        }
    }
#endif

    /* register layer type */
    bcm_layer->dfb_ext_type = layer_type;

    /* zeroed layer data structure */
    memset(&bcm_layer->dfb_ext_settings, 0x00, sizeof(bcm_layer->dfb_ext_settings));

    switch (layer_type) {
        case bdvd_dfb_ext_bd_j_background_layer:
            if (getenv("disable_bd_j_background_layer"))
            {
                ret = DFB_FAILURE;
                goto quit;
            }

            if (!dfb_bcmcore_context->vgraphics_window.layer_assigned)
            {
                dfb_bcmcore_context->vgraphics_window.layer_id = *layer_id;
                dfb_bcmcore_context->vgraphics_window.layer_assigned = bcm_layer;
                bcm_layer->bcmcore_type = BCMCORELAYER_BACKGROUND;
            }
            else {
                D_ERROR("%s/BCMCoreLayerRegister: background layer already registered\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_1];
        break;
        case bdvd_dfb_ext_osd_secondary_layer:
            if (!dfb_bcmcore_context->secondary_display_handle) {
                D_ERROR("%s/BCMCoreLayerRegister: there is no secondary display\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
            if (!dfb_bcmcore_context->secondary_graphics_window.layer_assigned) {
                dfb_bcmcore_context->secondary_graphics_window.display_handle = dfb_bcmcore_context->secondary_display_handle;
                if ((dfb_bcmcore_context->secondary_graphics_window.window_handle = bdvd_graphics_window_open(B_ID(dfb_bcmcore_context->graphics_constraints.nb_gfxWindows[0]), dfb_bcmcore_context->secondary_graphics_window.display_handle)) == NULL) {
                    D_ERROR("%s/BCMCoreLayerRegister: bdvd_graphics_window_open failed\n",
                            BCMCoreModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
                }
                dfb_bcmcore_context->secondary_graphics_window.layer_id = *layer_id;
                dfb_bcmcore_context->secondary_graphics_window.layer_assigned = bcm_layer;
                bcm_layer->graphics_window = &dfb_bcmcore_context->secondary_graphics_window;
                bcm_layer->bcmcore_type = BCMCORELAYER_OUTPUT;
                bcm_layer->premultiplied = true;
            }
            else {
                D_ERROR("%s/BCMCoreLayerRegister: secondary osd layer already registered\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_1];
        break;
        case bdvd_dfb_ext_bd_j_graphics_layer:
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_1];
        if (flipmod) {
            bcm_layer->bcmcore_type = BCMCORELAYER_MIXER_FLIPTOGGLE;
            /* Always visible, but will only be mixed when
             * registered with SetLevel.
             */
            bcm_layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
        break;
        }
        /* fall though */
        case bdvd_dfb_ext_hddvd_ac_cursor_layer:
        case bdvd_dfb_ext_hddvd_ac_graphics_layer:
        case bdvd_dfb_ext_osd_layer:
        case bdvd_dfb_ext_splashscreen_layer:
        case bdvd_dfb_ext_bd_hdmv_interactive_layer:
            bcm_layer->bcmcore_type = BCMCORELAYER_MIXER_FLIPBLIT;
            /* Always visible, but will only be mixed when
             * registered with SetLevel.
             */
            bcm_layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_1];
        break;
        case bdvd_dfb_ext_hddvd_subpicture_layer:
        case bdvd_dfb_ext_bd_hdmv_presentation_layer:

            bcm_layer->bcmcore_type = BCMCORELAYER_MIXER_FLIPBLIT;
            /* Always visible, but will only be mixed when
             * registered with SetLevel.
             */
            bcm_layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_0];
        break;
        case bdvd_dfb_ext_bd_j_sync_faa_layer:
            bcm_layer->bcmcore_type = BCMCORELAYER_FAA;
            memset(&bcm_layer->animation_context, 0, sizeof(bcm_layer->animation_context));
            memset(&bcm_layer->dfb_ext_animation_stats, 0, sizeof(bcm_layer->dfb_ext_animation_stats));
            bcm_layer->frame_mixing_time = 0;
            bcm_layer->dfb_ext_animation_stats.currentWorkingFrame = -1;
            bcm_layer->idle_surface_index = bcm_layer->front_surface_index = 0;
            bcm_layer->back_surface_index = 1;
            /* The animator decides if the layer is visible or
             * not, but will only be mixed when registered with
             * SetLevel.
             */
            bcm_layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
            attach_animator = true;
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_1];
        break;

        case bdvd_dfb_ext_bd_j_image_faa_layer:
            bcm_layer->bcmcore_type = BCMCORELAYER_FAA;
            /* The animator decides if the layer is visible or
             * not, but will only be mixed when registered with
             * SetLevel.
             */
            bcm_layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
            attach_animator = true;
            bcm_mixer = dfb_bcmcore_context->bcm_layers.layer[layer_index].mixer = dfb_bcmcore_context->mixers[BCM_MIXER_1];
        break;
        default:
            D_ERROR("%s/BCMCoreLayerRegister: unknown bdvd_dfb_ext_layer_type_e\n",
                    BCMCoreModuleName );
            ret = DFB_FAILURE;
            goto quit;
        break;
    }

    /* initialize onsync layer variable */
    bcm_layer->callback        = NULL;
    bcm_layer->callback_param1 = NULL;
    bcm_layer->callback_param2 = 0;
    bcm_layer->onsync_flip     = false;


    ret = BCMMixerLayerRegister(bcm_mixer, bcm_layer, &dfb_bcmcore_context->bcm_layers.layer[layer_index].mixed_layer_id);
    if (ret != DFB_OK)
    {
        D_ERROR("%s/BCMCoreLayerRegister: BCMMixerLayerRegister failed\n", BCMCoreModuleName );
        goto quit;
    }



    D_DEBUG("%sBCMCoreLayerRegister: bcm/dfb layer %d mixer layer %d\n", BCMCoreModuleName, *layer_id, dfb_bcmcore_context->bcm_layers.layer[layer_index].mixed_layer_id);

quit:

    if (ret == DFB_OK && attach_animator) {
        if ((ret = BCMMixerAttachAnimator(bcm_mixer)) != DFB_OK) {
            goto quit;
        }
    }

    D_DEBUG("%s/BCMCoreLayerRegister exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}


DFBResult
BCMCoreLayerSet(DFBDisplayLayerID layer_id,
                        bdvd_dfb_ext_layer_settings_t * settings) {
    bresult err;
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

    /**/
    bdvd_dfb_ext_faa_flags_e flags;
    bdvd_dfb_ext_faa_params_t * faa_params = NULL;
    bdvd_dfb_ext_faa_state_t * faa_state = NULL;


    D_DEBUG("%s: layer_id: 0x%08x\n", __FUNCTION__, layer_id);
    D_DEBUG("%s: settings->id: 0x%08x\n", __FUNCTION__, settings->id);
    D_DEBUG("%s: settings->type: 0x%08x\n", __FUNCTION__, settings->type);
    if (settings->type == bdvd_dfb_ext_bd_j_image_faa_layer) {
        D_DEBUG("Image FAA: \n");
        D_DEBUG("%s: settings->type_settings.image_faa.flags: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.flags);
        D_DEBUG("%s: settings->type_settings.image_faa.faa_state.default_framerate: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.faa_state.default_framerate);
        D_DEBUG("%s: settings->type_settings.image_faa.faa_state.trigger: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.faa_state.trigger);
        D_DEBUG("%s: settings->type_settings.image_faa.faa_state.threadPriority: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.faa_state.threadPriority);
        D_DEBUG("%s: settings->type_settings.image_faa.faa_state.startPTS: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.faa_state.startPTS);
        D_DEBUG("%s: settings->type_settings.image_faa.faa_state.stopPTS: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.faa_state.stopPTS);
        D_DEBUG("%s: settings->type_settings.image_faa.image_faa_params.lockedToVideo: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.image_faa_params.lockedToVideo);
        D_DEBUG("%s: settings->type_settings.image_faa_state.forceDisplayPosition: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.image_faa_state.forceDisplayPosition);
        D_DEBUG("%s: settings->type_settings.image_faa_state.playmode: 0x%08x\n", __FUNCTION__, settings->type_settings.image_faa.image_faa_state.playmode);
    }

    if (settings->type == bdvd_dfb_ext_bd_j_sync_faa_layer) {
        D_DEBUG("Sync FAA: \n");
        D_DEBUG("%s: settings->type_settings.sync_faa.flags: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.flags);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_state.default_framerate: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_state.default_framerate);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_state.trigger: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_state.trigger);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_state.threadPriority: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_state.threadPriority);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_state.startPTS: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_state.startPTS);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_state.stopPTS: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_state.stopPTS);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_params.numBuffers: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_params.numBuffers);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_params.repeatCount: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_params.repeatCount);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_params.repeatCountSize: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_params.repeatCountSize);
        D_DEBUG("%s: settings->type_settings.sync_faa.faa_params.scaleFactor: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.faa_params.scaleFactor);
        D_DEBUG("%s: settings->type_settings.sync_faa.sync_faa_state.forceWorkingPosition: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.sync_faa_state.forceWorkingPosition);
        D_DEBUG("%s: settings->type_settings.sync_faa.sync_faa_state.workNotFinished: 0x%08x\n", __FUNCTION__, settings->type_settings.sync_faa.sync_faa_state.workNotFinished);
    }

    uint32_t i;

    D_DEBUG("%s/BCMCoreLayerSet called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    if (layer_id < dfb_bcmcore_context->graphics_constraints.nb_gfxWindows[0])
    {
        D_ERROR("%s/BCMCoreLayerSet: set of primary layer is not possible\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if (bcm_layer->bcmcore_type == BCMCORELAYER_INVALID) {
        D_ERROR("%s/BCMCoreLayerSet: invalid layer type\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    if (settings->type != bcm_layer->dfb_ext_type) {
        D_ERROR("%s/BCMCoreLayerSet: layer type differ from registration\n",
                BCMCoreModuleName );
        D_ERROR("%s/BCMCoreLayerSet: settings->type: %d\n",
                BCMCoreModuleName, settings->type );
        D_ERROR("%s/BCMCoreLayerSet: bcm_layer->dfb_ext_type: %d\n",
                BCMCoreModuleName, bcm_layer->dfb_ext_type );
        ret = DFB_FAILURE;
        goto quit;
    }

    if (bcm_layer->bcmcore_type == BCMCORELAYER_FAA) {
        switch (bcm_layer->dfb_ext_type) {
            case bdvd_dfb_ext_bd_j_image_faa_layer:
                D_DEBUG("%s/BCMCoreLayerSet: Layer type ImageFAA\n", BCMCoreModuleName);
                flags = settings->type_settings.image_faa.flags;
                faa_params = &settings->type_settings.image_faa.faa_params;
                faa_state = &settings->type_settings.image_faa.faa_state;
            break;
            case bdvd_dfb_ext_bd_j_sync_faa_layer:
                D_DEBUG("%s/BCMCoreLayerSet: Layer type SyncFAA\n", BCMCoreModuleName);
                flags = settings->type_settings.sync_faa.flags;
                faa_params = &settings->type_settings.sync_faa.faa_params;
                faa_state = &settings->type_settings.sync_faa.faa_state;
            break;
            default:
                D_ERROR("%s/BCMCoreLayerSet: invalid layer type\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
        }

        D_DEBUG("%s/BCMCoreLayerSet: Received flags=0x%08x\n", BCMCoreModuleName, flags);
        if (flags & bdvd_dfb_ext_faa_flags_params) {
            D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_params is set in flags\n", BCMCoreModuleName);
            D_DEBUG("%s/BCMCoreLayerSet: faa_params->repeatCountSize=%d\n", BCMCoreModuleName, faa_params->repeatCountSize);

            for (i = 0; i < faa_params->repeatCountSize; i++) {
                D_DEBUG("%s/BCMCoreLayerSet: faa_params->repeatCount[%d] = %d\n", BCMCoreModuleName, i, faa_params->repeatCount[i]);
                if (faa_params->repeatCount[i] == 0) {
                    D_ERROR("%s/BCMCoreLayerSet: cannot have a zero value in the repeatCount[]\n",
                            BCMCoreModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
                }
            }

            /* The first surface was already allocated when the DisplayLayer
             * surface was obtained. It cannot be null.
             */
            if (bcm_layer->nb_surfaces != 1) {
                D_ERROR("%s/BCMCoreLayerSet: layer must have one and only one surface\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
            if (bcm_layer->surface_buffers[0] == NULL) {
                D_ERROR("%s/BCMCoreLayerSet: layer unique surface is NULL\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }

            for (i = faa_params->numBuffers; i < bcm_layer->nb_surfaces; i++) {
                D_DEBUG("%s/BCMCoreLayerSet: Deallocate &backsurface_clone=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName,  &bcm_layer->backsurface_clone, i, bcm_layer->surface_buffers[i]);
                dfb_surface_destroy_buffer( &bcm_layer->backsurface_clone,
                                            bcm_layer->surface_buffers[i] );
                bcm_layer->surface_buffers[i] = NULL;
            }

            for (i = bcm_layer->nb_surfaces; i < faa_params->numBuffers; i++) {
                if (dfb_surface_allocate_buffer( &bcm_layer->backsurface_clone,
                                                 CSP_VIDEOONLY,
                                                  bcm_layer->backsurface_clone.format,
                                                 &bcm_layer->surface_buffers[i] ) != DFB_OK) {
                    D_ERROR("%s/BCMCoreLayerSet: error creating layer %u buffer %d\n",
                             BCMCoreModuleName,
                             bcm_layer->layer_id,
                             i );
                    ret = DFB_FAILURE;
                    goto quit;
                }

                D_DEBUG("%s/BCMCoreLayerSet: Allocate &bcm_layer->backsurface_clone=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, &bcm_layer->backsurface_clone, i, bcm_layer->surface_buffers[i]);

                if ((err = bdvd_graphics_surface_clear(dfb_bcmcore_context->driver_compositor, ((bdvd_graphics_surface_h*)bcm_layer->surface_buffers[i]->video.ctx)[0])) != b_ok) {
                    D_ERROR("%s/BCMCoreLayerSet: error calling bdvd_graphics_surface_clear %d\n",
                             BCMCoreModuleName,
                             err );
                    ret = DFB_FAILURE;
                    goto quit;
                }
            }

            bcm_layer->nb_surfaces = faa_params->numBuffers;

            switch (bcm_layer->dfb_ext_type) {
                case bdvd_dfb_ext_bd_j_image_faa_layer:
                    D_DEBUG("%s/BCMCoreLayerSet: Setting image FAA params\n", BCMCoreModuleName);

                    bcm_layer->dfb_ext_settings.image_faa.faa_params = *faa_params;
                    bcm_layer->dfb_ext_settings.image_faa.image_faa_params = settings->type_settings.image_faa.image_faa_params;
                break;
                case bdvd_dfb_ext_bd_j_sync_faa_layer:
                    D_DEBUG("%s/BCMCoreLayerSet: Setting sync FAA params\n", BCMCoreModuleName);
                    bcm_layer->dfb_ext_settings.sync_faa.faa_params = *faa_params;
                break;
                default:
                    D_ERROR("%s/BCMCoreLayerSet: invalid layer type\n",
                            BCMCoreModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
            }
        }

        if (flags & bdvd_dfb_ext_faa_flags_framerate) {
            D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_framerate is set in flags\n", BCMCoreModuleName);
            D_DEBUG("%s/BCMCoreLayerSet: default_framerate: %d\n", BCMCoreModuleName, faa_state->default_framerate);
            switch (bcm_layer->dfb_ext_type) {
                case bdvd_dfb_ext_bd_j_image_faa_layer:
                    bcm_layer->dfb_ext_settings.image_faa.faa_state.default_framerate = faa_state->default_framerate;
                break;
                case bdvd_dfb_ext_bd_j_sync_faa_layer:
                    bcm_layer->dfb_ext_settings.sync_faa.faa_state.default_framerate = faa_state->default_framerate;
                break;
                default:
                    D_ERROR("%s/BCMCoreLayerSet: invalid layer type\n",
                            BCMCoreModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
            }
        }

        if (bcm_layer->dfb_ext_type == bdvd_dfb_ext_bd_j_image_faa_layer) {
            if (flags & bdvd_dfb_ext_faa_flags_playmode) {
                D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_playmode is set in flags for ImageFAA\n", BCMCoreModuleName);

                switch (settings->type_settings.image_faa.image_faa_state.playmode) {
                    case bdvd_dfb_ext_image_faa_play_repeating:
                    case bdvd_dfb_ext_image_faa_play_once:
                    case bdvd_dfb_ext_image_faa_play_alternating:
                        bcm_layer->dfb_ext_settings.image_faa.image_faa_state.playmode =
                            settings->type_settings.image_faa.image_faa_state.playmode;
                    break;
                    default:
                        D_ERROR("%s/BCMCoreLayerSet: invalid play mode\n",
                                BCMCoreModuleName );
                        ret = DFB_FAILURE;
                        goto quit;
                }
            }
        }
        if (flags & bdvd_dfb_ext_faa_flags_trigger) {
#if 0 /* need to rethink this, this is not mandatory or passed by constructor */
            /* When trigger is set, the default framerate is not allowed
             * to be unknown.
             */
            if (faa_state->default_framerate == bdvd_dfb_ext_frame_rate_unknown) {
                D_ERROR("%s/BCMCoreLayerSet: invalid frame rate\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
#endif
            D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_trigger is set in flags\n", BCMCoreModuleName);

            switch (bcm_layer->dfb_ext_type) {
                case bdvd_dfb_ext_bd_j_image_faa_layer:
                    /* When trigger is set, the play mode is not allowed
                     * to be unknown.
                     */
                    if (bcm_layer->dfb_ext_settings.image_faa.image_faa_state.playmode == bdvd_dfb_ext_image_faa_play_unknown) {
                        D_ERROR("%s/BCMCoreLayerSet: invalid play mode\n",
                                BCMCoreModuleName );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                    switch (bcm_layer->dfb_ext_settings.image_faa.faa_state.trigger = faa_state->trigger) {
                        case bdvd_dfb_ext_faa_trigger_time:
                            if (flags & bdvd_dfb_ext_faa_flags_time) {
                                D_DEBUG("%s/BCMCoreLayerSet: Setting image FAA trigger time\n", BCMCoreModuleName);
                                bcm_layer->dfb_ext_settings.image_faa.faa_state.startPTS = faa_state->startPTS;
                                bcm_layer->dfb_ext_settings.image_faa.faa_state.stopPTS = faa_state->stopPTS;
                                memset(&bcm_layer->animation_context, 0, sizeof(bcm_layer->animation_context));
                                memset(&bcm_layer->dfb_ext_animation_stats, 0, sizeof(bcm_layer->dfb_ext_animation_stats));
                                bcm_layer->animation_context.increment_sign = 1;
                                bcm_layer->idle_surface_index = bcm_layer->front_surface_index = 0;
                                if ((bcm_layer->dfb_core_surface->front_buffer = bcm_layer->surface_buffers[bcm_layer->front_surface_index]) == NULL) {
                                    D_ERROR("%s/BCMCoreLayerSet: surface buffer is NULL\n",
                                            BCMCoreModuleName);
                                    ret = DFB_FAILURE;
                                    goto quit;
                                }
                                bcm_layer->dfb_core_surface->idle_buffer = bcm_layer->dfb_core_surface->front_buffer;
                                bcm_layer->dfb_ext_force_clean_on_stop = false;
                            }
                        break;
                        case bdvd_dfb_ext_faa_trigger_forced_start:
                            D_DEBUG("%s/BCMCoreLayerSet: Setting image FAA force start\n", BCMCoreModuleName);
                            if (!bcm_layer->dfb_ext_animation_stats.isAnimated) {
                                memset(&bcm_layer->animation_context, 0, sizeof(bcm_layer->animation_context));
                                memset(&bcm_layer->dfb_ext_animation_stats, 0, sizeof(bcm_layer->dfb_ext_animation_stats));
                                bcm_layer->animation_context.increment_sign = 1;
                                bcm_layer->idle_surface_index = bcm_layer->front_surface_index = 0;
                                if ((bcm_layer->dfb_core_surface->front_buffer = bcm_layer->surface_buffers[bcm_layer->front_surface_index]) == NULL) {
                                    D_ERROR("%s/BCMCoreLayerSet: surface buffer is NULL\n",
                                            BCMCoreModuleName);
                                    ret = DFB_FAILURE;
                                    goto quit;
                                }
                                bcm_layer->dfb_core_surface->idle_buffer = bcm_layer->dfb_core_surface->front_buffer;
                                bcm_layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
                                bcm_layer->dfb_ext_force_clean_on_stop = false;
                            }
                        break;
                        case bdvd_dfb_ext_faa_trigger_none:
                            D_DEBUG("%s/BCMCoreLayerSet: Setting image FAA trigger none\n", BCMCoreModuleName);
                            bcm_layer->dfb_ext_settings.image_faa.faa_state.startPTS = 0;
                            bcm_layer->dfb_ext_settings.image_faa.faa_state.stopPTS = 0;
                        break;
                        case bdvd_dfb_ext_faa_trigger_forced_stop:
                            D_DEBUG("%s/BCMCoreLayerSet: Setting image FAA trigger forced stop\n", BCMCoreModuleName);
                            bcm_layer->dfb_ext_animation_stats.isAnimated = false;
                            bcm_layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
                            bcm_layer->dfb_ext_force_clean_on_stop = true;
                        break;
                        default:
                            D_ERROR("%s/BCMCoreLayerSet: invalid image FAA trigger\n",
                                    BCMCoreModuleName );
                            ret = DFB_FAILURE;
                            goto quit;
                    }
                break;
                case bdvd_dfb_ext_bd_j_sync_faa_layer:
                    switch (bcm_layer->dfb_ext_settings.sync_faa.faa_state.trigger = faa_state->trigger) {
                        case bdvd_dfb_ext_faa_trigger_time:
                            if (flags & bdvd_dfb_ext_faa_flags_time) {
                                D_DEBUG("%s/BCMCoreLayerSet: Setting sync FAA trigger time\n", BCMCoreModuleName);
                                bcm_layer->dfb_ext_settings.sync_faa.faa_state.startPTS = faa_state->startPTS;
                                bcm_layer->dfb_ext_settings.sync_faa.faa_state.stopPTS = faa_state->stopPTS;
                            }
                        case bdvd_dfb_ext_faa_trigger_forced_start:
                            D_DEBUG("%s/BCMCoreLayerSet: Setting sync FAA start\n", BCMCoreModuleName);
                            if (!bcm_layer->dfb_ext_animation_stats.isAnimated) {
                                if ((bcm_layer->dfb_core_surface->front_buffer = bcm_layer->surface_buffers[bcm_layer->front_surface_index]) == NULL) {
                                    D_ERROR("%s/BCMCoreLayerSet: surface buffer is NULL\n",
                                            BCMCoreModuleName);
                                    ret = DFB_FAILURE;
                                    goto quit;
                                }
                                bcm_layer->dfb_core_surface->idle_buffer = bcm_layer->dfb_core_surface->front_buffer;

                                if ((bcm_layer->dfb_core_surface->back_buffer = bcm_layer->surface_buffers[bcm_layer->back_surface_index]) == NULL) {
                                    D_ERROR("%s/BCMCoreLayerSet: surface buffer is NULL\n",
                                            BCMCoreModuleName);
                                    ret = DFB_FAILURE;
                                    goto quit;
                                }

								/* pr9290: at the time force_start is called the layer is set visible. This triggers
								   the animator to jump to frame one right away instead of allowing a frame  time to
								   to draw frame zero. SFAA-50 was failing on this. Fixed initial value to overcome
								   this issue. Also masked expectedOutputFrame value in BCMCoreLayerStatsGet in case
								   the expectedOutputFrame is read before the animator start working on it. */
								bcm_layer->dfb_ext_animation_stats.expectedOutputFrame = -1;
                                bcm_layer->bcmcore_visibility = BCMCORELAYER_VISIBLE;
                                bcm_layer->dfb_ext_force_clean_on_stop = false;
                            }
                        break;
                        case bdvd_dfb_ext_faa_trigger_none:
                            D_DEBUG("%s/BCMCoreLayerSet: Setting sync FAA trigger none\n", BCMCoreModuleName);
                            bcm_layer->dfb_ext_settings.image_faa.faa_state.startPTS = 0;
                            bcm_layer->dfb_ext_settings.image_faa.faa_state.stopPTS = 0;
                        break;
                        case bdvd_dfb_ext_faa_trigger_forced_stop:
                            D_DEBUG("%s/BCMCoreLayerSet: Setting sync FAA force stop\n", BCMCoreModuleName);
                            bcm_layer->dfb_ext_animation_stats.isAnimated = false;
                            bcm_layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
                            bcm_layer->dfb_ext_force_clean_on_stop = true;
                        break;
                        default:
                            D_ERROR("%s/BCMCoreLayerSet: invalid sync FAA trigger\n",
                                    BCMCoreModuleName );
                            ret = DFB_FAILURE;
                            goto quit;
                    }
                break;
                default:
                    D_ERROR("%s/BCMCoreLayerSet: invalid layer type\n",
                            BCMCoreModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
            }
        }


        switch (bcm_layer->dfb_ext_type) {
            case bdvd_dfb_ext_bd_j_image_faa_layer:
                if (flags & bdvd_dfb_ext_faa_flags_playmode) {
                    D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_playmode is set in flags for image FAA\n", BCMCoreModuleName);
                    switch (settings->type_settings.image_faa.image_faa_state.playmode) {
                        case bdvd_dfb_ext_image_faa_play_repeating:
                        case bdvd_dfb_ext_image_faa_play_once:
                        case bdvd_dfb_ext_image_faa_play_alternating:
                            bcm_layer->dfb_ext_settings.image_faa.image_faa_state.playmode =
                                settings->type_settings.image_faa.image_faa_state.playmode;
                        break;
                        default:
                            D_ERROR("%s/BCMCoreLayerSet: invalid play mode\n",
                                    BCMCoreModuleName );
                            ret = DFB_FAILURE;
                            goto quit;
                    }
                }

                if (flags & bdvd_dfb_ext_faa_flags_forcedisplay) {
                    D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_forcedisplay is set in flags for image FAA\n", BCMCoreModuleName);

                    if (settings->type_settings.image_faa.image_faa_state.forceDisplayPosition >=
                        bcm_layer->nb_surfaces) {
                        D_ERROR("%s/BCMCoreLayerSet: invalid display position\n",
                                BCMCoreModuleName );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                    bcm_layer->dfb_ext_animation_stats.displayPosition =
                    bcm_layer->animation_context.next_front_surface_index =
                        settings->type_settings.image_faa.image_faa_state.forceDisplayPosition;
                }
            break;
            case bdvd_dfb_ext_bd_j_sync_faa_layer:
                if (flags & bdvd_dfb_ext_faa_flags_forceworking) {

                    D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_forceworking is set in flags for sync FAA.  Setting  forceWorkingPosition to %d\n", BCMCoreModuleName, settings->type_settings.sync_faa.sync_faa_state.forceWorkingPosition);

                    bcm_layer->dfb_ext_animation_stats.currentWorkingFrame =
                    settings->type_settings.sync_faa.sync_faa_state.forceWorkingPosition;
                }

                if (flags & bdvd_dfb_ext_faa_flags_worknotfini) {
                    D_DEBUG("%s/BCMCoreLayerSet: bdvd_dfb_ext_faa_flags_worknotfini is set in flags for sync FAA\n", BCMCoreModuleName);

                    if (settings->type_settings.sync_faa.sync_faa_state.workNotFinished) {
                        D_DEBUG("%s/BCMCoreLayerSet: Trying to start frame %d\n", BCMCoreModuleName, bcm_layer->dfb_ext_animation_stats.currentWorkingFrame);

                        if (bcm_layer->dfb_ext_animation_stats.currentWorkingFrame == -1) {
                            D_ERROR("%s/BCMCoreLayerSet: Working position must be set before starting to draw!\n", BCMCoreModuleName );
                            ret = DFB_FAILURE;
                            goto quit;
                        }
                        if (bcm_layer->dfb_ext_animation_stats.currentWorkingFrame <
                            bcm_layer->dfb_ext_animation_stats.expectedOutputFrame) {
                            D_DEBUG("%s/BCMCoreLayerSet: invalid frame position.  This frame should be dropped\n",
                                    BCMCoreModuleName);
                            ret = DFB_ACCESSDENIED;
                            goto quit;
                        }
                        else if (bcm_layer->dfb_ext_animation_stats.currentWorkingFrame <
                            bcm_layer->dfb_ext_animation_stats.totalFinishedFrames) {
                            D_ERROR("%s/BCMCoreLayerSet: invalid frame position.  This frame has already been completed\n",
                                    BCMCoreModuleName );
                            ret = DFB_ACCESSDENIED;
                            goto quit;
                        }
                        else if (bcm_layer->dfb_ext_animation_stats.totalFinishedFrames - bcm_layer->dfb_ext_animation_stats.displayedFrameCount >= (bcm_layer->nb_surfaces - 1)) {
                            D_DEBUG("%s/BCMCoreLayerSet: No buffer available for this frame\n",
                                    BCMCoreModuleName );
                            ret = DFB_TEMPUNAVAIL;
                            goto quit;
                        }
                    }
                    else {
                        D_DEBUG("%s/BCMCoreLayerSet: Sync FAA frame complete\n", BCMCoreModuleName);
                        bcm_layer->dfb_ext_animation_stats.totalFinishedFrames++;
                        bcm_layer->dfb_ext_animation_stats.currentWorkingFrame = -1;
                    }
                }
            break;
            default:
                /* Do nothing */
            break;
        }
    }

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMCoreLayerSet exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreLayerGet(DFBDisplayLayerID layer_id,
                        bdvd_dfb_ext_layer_settings_t * settings) {
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

    D_DEBUG("%s/BCMCoreLayerGet called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    if (layer_id < dfb_bcmcore_context->graphics_constraints.nb_gfxWindows[0])
    {
        D_ERROR("%s/BCMCoreLayerGet: get of primary layer is not possible\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if (bcm_layer->bcmcore_type == BCMCORELAYER_INVALID) {
        D_ERROR("%s/BCMCoreLayerGet: invalid layer type\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    /* We make a copy of the setting, mainly because we don't want to
     * lock the layer context outside of this API.
     */
    settings->type_settings = bcm_layer->dfb_ext_settings;

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMCoreLayerGet exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreLayerStatsGet(DFBDisplayLayerID layer_id,
                     bdvd_dfb_ext_animation_stats_t * dfb_ext_animation_stats) {
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

    D_DEBUG("%s/BCMCoreLayerStatsGet called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if (bcm_layer->bcmcore_type != BCMCORELAYER_FAA) {
        D_ERROR("%s/BCMCoreLayerStatsGet: invalid layer type\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    /* We make a copy of the animation stats, mainly because we don't want to
     * lock the layer context outside of this API.
     */
    *dfb_ext_animation_stats = bcm_layer->dfb_ext_animation_stats;
    /* pr9290: see comment above */
    dfb_ext_animation_stats->expectedOutputFrame = (bcm_layer->dfb_ext_animation_stats.expectedOutputFrame == -1) ? 0 : bcm_layer->dfb_ext_animation_stats.expectedOutputFrame;

    D_DEBUG("%s/BCMCoreLayerStatsGet: isAnimated =%d\n", BCMCoreModuleName, bcm_layer->dfb_ext_animation_stats.isAnimated);
    D_DEBUG("%s/BCMCoreLayerStatsGet: displayPosition =%d\n", BCMCoreModuleName,  bcm_layer->dfb_ext_animation_stats.displayPosition);
    D_DEBUG("%s/BCMCoreLayerStatsGet: displayedFrameCount =%d\n", BCMCoreModuleName, bcm_layer->dfb_ext_animation_stats.displayedFrameCount);
    D_DEBUG("%s/BCMCoreLayerStatsGet: totalFinishedFrames =%d\n", BCMCoreModuleName, bcm_layer->dfb_ext_animation_stats.totalFinishedFrames);
    D_DEBUG("%s/BCMCoreLayerStatsGet: expectedOutputFrame =%d\n", BCMCoreModuleName, bcm_layer->dfb_ext_animation_stats.expectedOutputFrame);
    D_DEBUG("%s/BCMCoreLayerStatsGet: currentPTS =%d\n", BCMCoreModuleName, bcm_layer->dfb_ext_animation_stats.currentPTS);
quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMCoreLayerStatsGet exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreLayerUnregister(DFBDisplayLayerID layer_id) {
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;
    bool detach_animator = false;
    uint32_t i;

    D_DEBUG("%s/BCMCoreLayerUnregister called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    if (layer_id < dfb_bcmcore_context->graphics_constraints.nb_gfxWindows[0])
    {
        D_ERROR("%s/BCMCoreLayerUnregister: unregister of primary layer is not possible\n",
                BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if (bcm_layer->surface_buffers[bcm_layer->front_surface_index] != NULL ||
        bcm_layer->surface_buffers[bcm_layer->back_surface_index] != NULL ||
        bcm_layer->surface_buffers[bcm_layer->idle_surface_index] != NULL) {
        D_WARN("%s/BCMCoreLayerUnregister: called without proper release of layer\n",
               BCMCoreModuleName );
    }

    switch (bcm_layer->bcmcore_type) {
        case BCMCORELAYER_FAA:
            {
                /* Setting back to mixer type, until we can really unregister */
                CoreSurface * surface = dfb_core_create_surface( dfb_bcmcore_context->core );
                direct_serial_init( &surface->serial );
                surface->manager = dfb_gfxcard_surface_manager();
                for (i = 0; i < bcm_layer->nb_surfaces; i++) {
                    if (bcm_layer->surface_buffers[i] != NULL) {
                        /* Deallocation is done in BCMGfxDeallocate for CoreBuffer assigned,
                         * surface handles should be cleared in RemoveRegion.
                         * It is possible that BCMCoreLayerUnregister and RemoveRegion may
                         * be called out of order.
                         */
                        if ((i != bcm_layer->front_surface_index) &&
                            (i != bcm_layer->back_surface_index) &&
                            (i != bcm_layer->idle_surface_index)) {
                            bcm_layer->surface_buffers[i]->surface = surface;
                            D_DEBUG("%s/BCMCoreLayerUnregister: Deallocate &surface=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, surface, i, bcm_layer->surface_buffers[i]);

                            dfb_surface_destroy_buffer( surface,
                                                        bcm_layer->surface_buffers[i] );
                            bcm_layer->surface_buffers[i] = NULL;
                        }
                    }
                }
                direct_serial_deinit( &surface->serial );
                fusion_object_destroy( &surface->object );
            }

            detach_animator = true;
            break;
        case BCMCORELAYER_MIXER_FLIPBLIT:
        case BCMCORELAYER_MIXER_FLIPTOGGLE:
        break;
        case BCMCORELAYER_BACKGROUND:
            if (layer_id != dfb_bcmcore_context->vgraphics_window.layer_id) {
                D_ERROR("%s/BCMCoreLayerUnregister: layer ressource assignement invalid\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
            if (dfb_bcmcore_context->vgraphics_window.layer_assigned) {
                dfb_bcmcore_context->vgraphics_window.layer_id = 0;
                dfb_bcmcore_context->vgraphics_window.layer_assigned = NULL;
            }
            else {
                D_ERROR("%s/BCMCoreLayerUnregister: vgraphics is not assigned\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
            }
        break;
        case BCMCORELAYER_OUTPUT:
            switch (bcm_layer->dfb_ext_type) {
                case bdvd_dfb_ext_osd_secondary_layer:
                    if (layer_id != dfb_bcmcore_context->secondary_graphics_window.layer_id) {
                        D_ERROR("%s/BCMCoreLayerUnregister: layer ressource assignement invalid\n",
                                BCMCoreModuleName );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                    if (dfb_bcmcore_context->secondary_graphics_window.layer_assigned) {
                        if (bdvd_graphics_window_close(dfb_bcmcore_context->secondary_graphics_window.window_handle) != b_ok) {
                            D_ERROR("%s/BCMCoreLayerUnregister: bdvd_graphics_window_close failed\n",
                                    BCMCoreModuleName );
                            ret = DFB_FAILURE;
                            goto quit;
                        }
                        dfb_bcmcore_context->secondary_graphics_window.layer_id = 0;
                        dfb_bcmcore_context->secondary_graphics_window.layer_assigned = NULL;
                        dfb_bcmcore_context->secondary_graphics_window.window_handle = NULL;
                        dfb_bcmcore_context->secondary_graphics_window.display_handle = NULL;
                        bcm_layer->graphics_window = NULL;
                        bcm_layer->premultiplied = false;
                    }
                    else {
                        D_ERROR("%s/BCMCoreLayerUnregister: secondary graphics is not assigned\n",
                                BCMCoreModuleName );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                break;
                default:
                    D_ERROR("%s/BCMCoreLayerUnregister: layer type mismatch\n",
                            BCMCoreModuleName );
                    ret = DFB_FAILURE;
                    goto quit;
            }
        break;
        default:
            D_ERROR("%s/BCMCoreLayerUnregister: invalid layer type\n",
                    BCMCoreModuleName );
            ret = DFB_FAILURE;
            goto quit;
    }

    bcm_layer->bcmcore_type = BCMCORELAYER_INVALID;
    bcm_layer->bcmcore_visibility = BCMCORELAYER_HIDDEN;
    bcm_layer->dfb_ext_type = bdvd_dfb_ext_unknown;
    memset(&bcm_layer->dfb_ext_settings, 0, sizeof(bcm_layer->dfb_ext_settings));
    bcm_layer->nb_surfaces = 0;

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    if (ret == DFB_OK && detach_animator) {
        if ((ret = BCMMixerDetachAnimator(layer_ctx->mixer)) != DFB_OK) {
            goto quit;
        }
    }

    ret = BCMMixerLayerUnregister(layer_ctx->mixer, layer_ctx->mixed_layer_id, false);
    if (ret != DFB_OK)
    {
        D_ERROR("%s/BCMCoreLayerUnRegister: BCMMixerLayerUnRegister failed\n", BCMCoreModuleName );
    }

    D_DEBUG("%s/BCMCoreLayerUnregister exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreSetVideoRate(bdvd_dfb_ext_frame_rate_e framerate)
{
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMCoreSetVideoRate called:\n",
            BCMCoreModuleName );

    D_DEBUG("%s/BCMCoreSetVideoRate: framerate: %d\n", BCMCoreModuleName, framerate);
    if ((ret = BCMMixerAnimatorSetVideoRate(dfb_bcmcore_context->mixers[BCM_MIXER_1], framerate)) != DFB_OK) {
        goto quit;
    }

quit:

    D_DEBUG("%s/BCMCoreSetVideoRate exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreSetVideoStatus(bool video_running)
{
    DFBResult ret = DFB_OK;

    D_DEBUG("%s/BCMCoreSetVideoStatus: video_running: %d\n", BCMCoreModuleName, video_running);
    if ((ret = BCMMixerAnimatorSetVideoStatus(dfb_bcmcore_context->mixers[BCM_MIXER_1], video_running)) != DFB_OK) {
        goto quit;
    }

quit:

    D_DEBUG("%s/BCMCoreSetVideoStatus exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreSetCurrentPTS(uint32_t current_pts)
{
    DFBResult ret = DFB_OK;

//#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMCoreSetCurrentPTS called: pts: 0x%08x\n",
            BCMCoreModuleName, current_pts );
//#endif

    /* FOR NOW THIS IS NO LOCKING ANYTHING */
    if ((ret = BCMMixerAnimatorUpdateCurrentPTS(dfb_bcmcore_context->mixers[BCM_MIXER_1], current_pts)) != DFB_OK) {
        goto quit;
    }

quit:

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMCoreSetCurrentPTS exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);
#endif

    return ret;
}

/** @} */

static DFBResult BCMCoreInitScreen( CoreScreen           *screen,
                                    GraphicsDevice       *device,
                                    void                 *driver_data,
                                    void                 *screen_data,
                                    DFBScreenDescription *description );

static DFBResult BCMCoreGetScreenSize( CoreScreen           *screen,
                                       void                 *driver_data,
                                       void                 *screen_data,
                                       int                  *ret_width,
                                       int                  *ret_height );

static DFBResult BCMCoreInitLayer( CoreLayer                  *layer,
                                   void                       *driver_data,
                                   void                       *layer_data,
                                   DFBDisplayLayerDescription *description,
                                   DFBDisplayLayerConfig      *config,
                                   DFBColorAdjustment         *adjustment );

static DFBResult BCMCoreGetLevel( CoreLayer              *layer,
                                  void                   *driver_data,
                                  void                   *layer_data,
                                  int                    *level );

static DFBResult BCMCoreSetLevel( CoreLayer              *layer,
                                  void                   *driver_data,
                                  void                   *layer_data,
                                  int                     level );

static DFBResult BCMCoreTestRegion( CoreLayer                  *layer,
                                    void                       *driver_data,
                                    void                       *layer_data,
                                    CoreLayerRegionConfig      *config,
                                    CoreLayerRegionConfigFlags *failed );

static DFBResult BCMCoreAddRegion( CoreLayer                  *layer,
                                   void                       *driver_data,
                                   void                       *layer_data,
                                   void                       *region_data,
                                   CoreLayerRegionConfig      *config );

static DFBResult BCMCoreSetRegion( CoreLayer                  *layer,
                                   void                       *driver_data,
                                   void                       *layer_data,
                                   void                       *region_data,
                                   CoreLayerRegionConfig      *config,
                                   CoreLayerRegionConfigFlags  updated,
                                   CoreSurface                *surface,
                                   CorePalette                *palette );

static DFBResult BCMCoreRemoveRegion( CoreLayer                  *layer,
                                      void                       *driver_data,
                                      void                       *layer_data,
                                      void                       *region_data );

static DFBResult BCMCoreFlipRegion( CoreLayer                  *layer,
                                    void                       *driver_data,
                                    void                       *layer_data,
                                    void                       *region_data,
                                    CoreSurface                *surface,
                                    DFBSurfaceFlipFlags         flags );

static DFBResult BCMCoreUpdateRegion( CoreLayer                  *layer,
                                      void                       *driver_data,
                                      void                       *layer_data,
                                      void                       *region_data,
                                      CoreSurface                *surface,
                                      const DFBRegion            *update );

static DFBResult BCMCoreUpdateRegionFlags( CoreLayer             *layer,
                                      void                       *driver_data,
                                      void                       *layer_data,
                                      void                       *region_data,
                                      CoreSurface                *surface,
                                      const DFBRegion            *update,
                                      DFBSurfaceFlipFlags         flags);

static DFBResult BCMCoreAllocateSurface_wrapper( CoreLayer                  *layer,
                                                 void                       *driver_data,
                                                 void                       *layer_data,
                                                 void                       *region_data,
                                                 CoreLayerRegionConfig      *config,
                                                 CoreSurface               **ret_surface );

static DFBResult BCMCoreReallocateSurface( CoreLayer             *layer,
                                           void                  *driver_data,
                                           void                  *layer_data,
                                           void                  *region_data,
                                           CoreLayerRegionConfig *config,
                                           CoreSurface           *surface );

static DFBResult BCMCoreDeallocateSurface_wrapper( CoreLayer              *layer,
                                                   void                   *driver_data,
                                                   void                   *layer_data,
                                                   void                   *region_data,
                                                   CoreSurface            *surface );

/**
 *  bcmcore_screen_funcs is a structure that will be registered in
 *  system_initialize with dfb_screens_register. ScreenFuncs
 *  defined in <src/core/screens.h> is assigned here.
 */
static ScreenFuncs bcmcore_screen_funcs = {
    .InitScreen    = BCMCoreInitScreen,
    .GetScreenSize = BCMCoreGetScreenSize,
};

/**
 *  bcmcore_layer_funcs is a structure that will be registered in
 *  system_initialize with dfb_layers_register to primary screen.
 *  DisplayLayerFuncs defined in <src/core/layers.h> is assigned here.
 */
static DisplayLayerFuncs bcmcore_layer_funcs = {
    .InitLayer         = BCMCoreInitLayer,
    .GetLevel          = BCMCoreGetLevel,
    .SetLevel          = BCMCoreSetLevel,
    .TestRegion        = BCMCoreTestRegion,
    .AddRegion         = BCMCoreAddRegion,
    .SetRegion         = BCMCoreSetRegion,
    .RemoveRegion      = BCMCoreRemoveRegion,
    .FlipRegion        = BCMCoreFlipRegion,
    .UpdateRegion      = BCMCoreUpdateRegion,
    .UpdateRegionFlags = BCMCoreUpdateRegionFlags,
    .AllocateSurface   = BCMCoreAllocateSurface_wrapper,
    .ReallocateSurface = BCMCoreReallocateSurface,
    .DeallocateSurface = BCMCoreDeallocateSurface_wrapper
};

/**
 *  The following is a enum typedef for RPC types handled by
 *  BCMCoreCallHandler. Multi Application support not
 *  implemented yet.
 */
typedef enum {
     BCMCORE_NOT_SUPPORTED_YET
} DFB_BCMCORE_RPC;

static int BCMCoreCallHandler( int   caller,
                                      int   call_arg,
                                      void *call_ptr,
                                      void *ctx );

static DFBResult BCMCoreExampleHandler( CoreLayerRegionConfig *config );

static DFBResult
BCMCoreLayerCreate(BCMCoreContextLayer *ret_layer, DFBDisplayLayerID layer_id, BCMCoreLayerCreateSettings *layer_settings)
{
    DFBResult          ret = DFB_OK; /* return code for this function */
    int                rc  = 0;      /* return code from pthread functions */
    BCMCoreLayerHandle layer = NULL;


    D_DEBUG("%s/BCMCoreLayerCreate: enter with layer id %d\n",
            BCMCoreModuleName, layer_id );

    /*
     * allocate layer data structire
     */
    layer = D_CALLOC(1, sizeof(*layer));
    if (layer == NULL)
    {
        D_ERROR("BCMCoreLayerCreate: layer allocation failed\n");
        ret = DFB_INIT;
        goto error;
    }

    /*
     * register attribute for layer mutex sem
     */
    pthread_mutexattr_init(&layer->mutex_attr);

    /*
     * set attribute of layer mutex sem to be PTHREAD_MUTEX_RECURSIVE_NP
     */
    if ((rc = pthread_mutexattr_settype(&layer->mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP)))
    {
        D_ERROR("BCMCoreLayerCreate: error setting mutex type\n");
        ret = DFB_INIT;
        goto error;
    }

    /*
     * specialize layer mutex sem to have mutex_attr attributes
     */
    pthread_mutex_init(&layer->mutex, &layer->mutex_attr);

    /*
     * init layer structure fields
     */

    /* layer id */
    layer->layer_id = layer_id;
    /* default layer level: -1 is not visible */
    layer->level = -1;
    /* set bcm layer type according with input settings */
    layer->bcmcore_type  = layer_settings->bcmcore_type;
    /* set premultiply property */
    layer->premultiplied = layer_settings->premultiplied;
    /* set layer name */
    snprintf(layer->layer_name, DFB_DISPLAY_LAYER_DESC_NAME_LENGTH, "%s", layer_settings->layer_name);
    /* register layer to mixed_layer data structure */
    ret_layer->handle = layer;
    ret_layer->mixer = layer_settings->bcmcore_mixer;

error:

    D_DEBUG("%s/BCMCoreLayerCreate: exited with %d\n", ret);

    return ret;
}

static DFBResult
BCMCoreLayersCreate(CoreScreen *screen, BCMCoreContext *dfb_bcmcore_context, unsigned int num_layers)
{
    DFBResult                  ret = DFB_OK;
    int                        rc = 0;
    char                       string_param1[80];
    DFBDisplayLayerID          layer_index;
    BCMCoreLayerCreateSettings layer_settings;

    D_DEBUG("%s/BCMCoreLayersCreate: entered", BCMCoreModuleName);

    /*
     * register attribute for bcm layers mutex sem
     */
    pthread_mutexattr_init(&dfb_bcmcore_context->bcm_layers.mutex_attr);

    /*
     * set attribute of layers mutex sem to be PTHREAD_MUTEX_RECURSIVE_NP
     */
    if ((rc = pthread_mutexattr_settype(&dfb_bcmcore_context->bcm_layers.mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP)))
    {
        D_ERROR("BCMCoreLayerCreate: error setting mutex type\n");
        ret = DFB_INIT;
        goto error;
    }

    /*
     * specialize layers mutex sem to have mutex_attr attributes
     */
    pthread_mutex_init(&dfb_bcmcore_context->bcm_layers.mutex, &dfb_bcmcore_context->bcm_layers.mutex_attr);

    /* set layer defaults */
    layer_settings.bcmcore_type = BCMCORELAYER_INVALID;
    layer_settings.bcmcore_mixer = NULL;
    layer_settings.premultiplied = false;

    /* create bcm layers and register layer and function to access them in directfb */
    for (layer_index = 0; layer_index < num_layers; layer_index++)
    {
        /* set layer name */
        snprintf(string_param1, sizeof(string_param1), "generic %u", layer_index);
        layer_settings.layer_name = string_param1;

        /* create bcm layer */
        if ((ret = BCMCoreLayerCreate(&dfb_bcmcore_context->bcm_layers.layer[layer_index], layer_index, &layer_settings)) != DFB_OK) {
            D_ERROR("%s/system_initialize: BCMCoreLayerCreate failed for layer_id %u\n",
                    BCMCoreModuleName,
                    layer_index);
            ret = DFB_FAILURE;
            goto error;
        }

        /* register bcm layer and the api to handle it in the directfb world */
        if (dfb_layers_register(screen, NULL, &bcmcore_layer_funcs) == NULL) {
            D_ERROR("%s/system_initialize: dfb_layers_register failed for layer_id %u\n",
                    BCMCoreModuleName,
                    layer_index);
            ret = DFB_FAILURE;
            goto error;
        }
    }

error:

    D_DEBUG("%s/BCMCoreLayersCreate: exited with %d\n", ret);

    return ret;
}

/** @addtogroup SystemCoreInterface
 *  The following is a grouping of static functions
 *  implementing the <src/core/core_system.h> system core
 *  module interface, refer to this file for function
 *  descriptions
 *  @{
 */

static void
system_get_info( CoreSystemInfo *info )
{
    D_DEBUG("%s/system_get_info called\n",
            BCMCoreModuleName );

    info->type = CORE_BCM;

    snprintf( info->name, DFB_CORE_SYSTEM_INFO_NAME_LENGTH, BCMCoreModuleName );
    snprintf( info->vendor, DFB_CORE_SYSTEM_INFO_VENDOR_LENGTH, "Broadcom Corporation" );
    snprintf( info->url, DFB_CORE_SYSTEM_INFO_URL_LENGTH, "http://www.broadcom.com/" );
    snprintf( info->license, DFB_CORE_SYSTEM_INFO_LICENSE_LENGTH, "proprietary" );

    info->version.major = BCMCoreModuleVersionMajor;
    info->version.minor = BCMCoreModuleVersionMinor;
}

static DFBResult
system_initialize( CoreDFB *core, void **data )
{
    DFBResult ret;
    CoreScreen * screen;
    char string_param1[80];
    unsigned int window_id;

    DFBDisplayLayerID layer_index;

    BCMCoreLayerCreateSettings register_settings;

    D_ASSERT( dfb_bcmcore_context == NULL );

    D_DEBUG("%s/system_initialize called\n",
            BCMCoreModuleName );

    /* We cannot let this go undefined */
    if (DirectFBSetOption("desktop-buffer-mode", "triple") != DFB_OK) {
        D_WARN( "%s/system_initialize: could not set desktop-buffer-mode to triple\n",
                BCMCoreModuleName );
    }

    D_DEBUG("%s/system_initialize returned from DirectFBSetOption\n",
            BCMCoreModuleName );

    dfb_bcmcore_context = D_CALLOC( 1, sizeof(BCMCoreContext) );

    dfb_bcmcore_context->shared = (BCMCoreContextShared *) SHCALLOC( 1, sizeof(BCMCoreContextShared) );

    fusion_arena_add_shared_field( dfb_core_arena( core ),
                                   BCMCoreModuleName, dfb_bcmcore_context->shared );

    D_DEBUG("%s/system_initialize returned from fusion_arena_add_shared_field\n",
            BCMCoreModuleName );

    dfb_bcmcore_context->core = core;

#ifdef BCMUSESLINUXINPUTS
    if (vt_kbd_input_open() != DFB_OK) {
    D_WARN( "%s/system_initialize: Could not open tty1, meaning there is no dummy console, linux input will not be supported\n",
            BCMCoreModuleName );
    }
#endif

    fusion_skirmish_init( &dfb_bcmcore_context->shared->lock, BCMCoreModuleName );

    D_DEBUG("%s/system_initialize returned from fusion_skirmish_init\n",
            BCMCoreModuleName );

    fusion_call_init( &dfb_bcmcore_context->shared->call,
                       BCMCoreCallHandler, NULL );

    D_DEBUG("%s/system_initialize returned from fusion_call_init\n",
            BCMCoreModuleName );

    qsort(format_strings, NUM_FORMAT_STRINGS, sizeof(FormatString), pixelformat_compare);

    D_DEBUG("%s/system_initialize returned from qsort\n",
            BCMCoreModuleName );

    /* Register primary screen functions */
    screen = dfb_screens_register( NULL, NULL, &bcmcore_screen_funcs );

    D_DEBUG("%s/system_initialize returned from dfb_screens_register\n",
            BCMCoreModuleName );

    /* create bcm layers and register them in directfb */
    if (BCMCoreLayersCreate(screen, dfb_bcmcore_context, MAX_LAYERS) != DFB_OK)
    {
         D_ERROR("%s/system_initialize: BCMCoreLayersCreate failed\n",
                 BCMCoreModuleName);
         ret = DFB_INIT;
         goto error;
    }

    D_DEBUG("%s/system_initialize returned from BCMCoreLayersCreate\n",
            BCMCoreModuleName );

    if ((ret = system_p_init(dfb_bcmcore_context)) != DFB_OK) {
        D_ERROR("%s/system_initialize: system_p_init failed\n",
            BCMCoreModuleName);
        goto error;
    }

    D_DEBUG("%s/system_initialize returned from system_p_init\n",
            BCMCoreModuleName );

    dfb_bcmcore_context->hdi_mixer_issue_clearrect = false;

    *data = dfb_bcmcore_context;

    D_DEBUG("%s/system initialize exit\n", BCMCoreModuleName);

    return DFB_OK;

error:

    D_DEBUG("%s/system_initialize exited with error code %d\n",
            BCMCoreModuleName,
            ret);

    return ret;

}

static DFBResult system_p_init(BCMCoreContext * ctx) {
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle output_layer = NULL;
    char * getenv_string;
    bdvd_video_format_e video_format;
    BCMMixerId mixer_id;
    bool invert_compositors;
    unsigned int window_id;
    bool mixerflipblit;
    bdvd_result_e rc;
    bdvd_bcc_cfg_t bcc_cfg;

    if (bdvd_graphics_constraints_get(&ctx->graphics_constraints) != b_ok) {
        D_ERROR("%s/system_p_init: failure to destroy surface\n",
                BCMCoreModuleName);
        ret = DFB_FAILURE;
        goto out;
    }

    D_INFO("%s/system_p_init: video memory size is %u\n",
        BCMCoreModuleName,
        ctx->graphics_constraints.videomemory_length);

    getenv_string = getenv( "dfbbcm_cached_video_memory" );
    if (getenv_string && !strncmp(getenv_string, "n", 1)) {
        D_INFO("%s/system_p_init: using uncached memory for surfaces\n",
               BCMCoreModuleName);
        /* Not necessary to flush cache since we are uncached */
        ctx->graphics_constraints.requires_cache_flush = false;
        ctx->system_video_memory_virtual = bdvd_graphics_uncached_virtual_addr_from_offset;
    }
    else {
        D_INFO("%s/system_p_init: using cached memory for surfaces, cache flush is %srequired\n",
               BCMCoreModuleName,
               ctx->graphics_constraints.requires_cache_flush ? "" : "not ");
        ctx->system_video_memory_virtual = bdvd_graphics_cached_virtual_addr_from_offset;
    }

#ifdef DEBUG_ALLOCATION
    show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

    /* B_ID(0) */
    ctx->primary_display_handle = ctx->graphics_constraints.displays[0];

    if (ctx->graphics_constraints.nb_displays > 1) {
        /* B_ID(1) */
        ctx->secondary_display_handle = ctx->graphics_constraints.displays[1];
    }

    /* This should eventually be done in bdvd_init */
    bdvd_graphics_window_p_init();

    for (window_id = 0; window_id < ctx->graphics_constraints.nb_gfxWindows[0]; window_id++)
    {
        if (!ctx->primary_graphics_window[window_id].layer_assigned) {
            ctx->primary_graphics_window[window_id].display_handle = ctx->primary_display_handle;
            if ((ctx->primary_graphics_window[window_id].window_handle = bdvd_graphics_window_open(B_ID(window_id), ctx->primary_graphics_window[window_id].display_handle)) == NULL) {
            D_ERROR("%s/system_p_init: bdvd_graphics_window_open failed\n",
                    BCMCoreModuleName );
            ret = DFB_INIT;
            goto out;
        }

            ctx->primary_graphics_window[window_id].layer_id = window_id;
        /* TODO Change this not clean */
            ctx->primary_graphics_window[window_id].layer_assigned = (void *)(0x00000001);
    }
    else {
        D_ERROR("%s/system_p_init: primary graphics window already registered\n",
             BCMCoreModuleName);
        ret = DFB_INIT;
        goto out;
    }
    }

    video_format = display_get_video_format(ctx->primary_display_handle);
    D_INFO( "%s/system_p_init: display is set to video_format %d\n",
        BCMCoreModuleName,
        video_format );

    if (ctx->graphics_constraints.nb_compositors == 2) {
        getenv_string = getenv( "dfbbcm_invert_compositors" );
        if (getenv_string) {
            unsetenv( "dfbbcm_invert_compositors" );
            if (!strncmp(getenv_string, "y", 1)) {
                invert_compositors = true;
            }
            else {
                invert_compositors = false;
            }
        }
        else {
             /* New default for now is true */
            invert_compositors = true;
        }
    }
    else {
        invert_compositors = false;
    }

    if ((ctx->mixer_compositor = bdvd_graphics_compositor_open(invert_compositors ? B_ID(1) : B_ID(0))) == NULL) {
         D_ERROR("%s/system_p_init: could not open bdvd_graphics_compositor B_ID(0)\n",
                 BCMCoreModuleName);
         ret = DFB_INIT;
         goto out;
    }

    bcc_cfg.client = bdvd_bcc_client_dfb_pg;
    rc = bdvd_bcc_open(&bcc_cfg, &ctx->driver_bcc_pg);
    if (rc != bdvd_ok) {
        D_ERROR("%s/system_p_init: could not open driver_bcc_pg.\n", BCMCoreModuleName);
        ret = DFB_INIT;
        goto out;
    }
    bcc_cfg.client = bdvd_bcc_client_dfb_textst;
    rc = bdvd_bcc_open(&bcc_cfg, &ctx->driver_bcc_textst);
    if (rc != bdvd_ok) {
        D_ERROR("%s/system_p_init: could not open driver_bcc_textst.\n", BCMCoreModuleName);
        ret = DFB_INIT;
        goto out;
    }
    bcc_cfg.client = bdvd_bcc_client_dfb_ig;
    rc = bdvd_bcc_open(&bcc_cfg, &ctx->driver_bcc_ig);
    if (rc != bdvd_ok) {
        D_ERROR("%s/system_p_init: could not open driver_bcc_ig.\n", BCMCoreModuleName);
        ret = DFB_INIT;
        goto out;
    }
    bcc_cfg.client = bdvd_bcc_client_dfb_other;
    rc = bdvd_bcc_open(&bcc_cfg, &ctx->driver_bcc_other);
    if (rc != bdvd_ok) {
        D_ERROR("%s/system_p_init: could not open driver_bcc_other.\n", BCMCoreModuleName);
        ret = DFB_INIT;
        goto out;
    }

    getenv_string = getenv( "dfbbcm_mixerflipblit" );
    if (getenv_string) {
        unsetenv( "dfbbcm_mixerflipblit" );
        if (!strncmp(getenv_string, "y", 1)) {
            mixerflipblit = true;
        }
        else {
            mixerflipblit = false;
        }
    }
    else {
         /* New default for now is true */
        mixerflipblit = true;
    }

    D_INFO("%s/system_p_init: using %s compositor for flip-blit\n",
           BCMCoreModuleName,
           mixerflipblit ? "mixer" : "driver");

    getenv_string = getenv( "dfbbcm_waitforflip" );
    if (!mixerflipblit && getenv_string) {
        unsetenv( "dfbbcm_waitforflip" );
        if (!strncmp(getenv_string, "y", 1)) {
            ctx->waitforflip = true;
        }
        else {
            ctx->waitforflip = false;
        }
    }
    else {
         /* New default for now is true */
        ctx->waitforflip = true;
    }

    D_INFO("%s/system_p_init: Flip with %sdo a forcefull DSFLIP_WAITFORSYNC\n",
           BCMCoreModuleName,
           ctx->waitforflip ? "" : "not ");

    if (ctx->graphics_constraints.nb_compositors == 2) {
        if ((ctx->driver_compositor = bdvd_graphics_compositor_open(invert_compositors ? B_ID(0) : B_ID(1))) == NULL) {
             D_ERROR("%s/system_p_init: could not open bdvd_graphics_compositor B_ID(1)\n",
                     BCMCoreModuleName);
             ret = DFB_INIT;
             goto out;
        }
        D_INFO("%s/system_p_init: using compositor M2MC%s for mixer and M2MC%s for driver\n",
               BCMCoreModuleName,
               invert_compositors ? "_1" : "",
               invert_compositors ? "" : "_1");
    }
    else {
        ctx->driver_compositor = ctx->mixer_compositor;
        D_INFO("%s/system_p_init: using compositor M2MC%s for both mixer and driver\n",
               BCMCoreModuleName,
               invert_compositors ? "_1" : "");
    }

    /* create mixer/s */
    for (mixer_id = BCM_MIXER_0; mixer_id < ctx->graphics_constraints.nb_gfxWindows[0]; mixer_id++)
    {
        /* allocate mixer data structure */
        ctx->mixers[mixer_id] = D_CALLOC(1, sizeof(*ctx->mixers[mixer_id]));
        if (ctx->mixers[mixer_id] == NULL) {
            D_ERROR("%s/system_initialize: could not allocate mixer data structure\n",
                    BCMCoreModuleName);
            ret = DFB_INIT;
            goto out;
        }

        ctx->mixers[mixer_id]->mixerId = mixer_id;
        ctx->mixers[mixer_id]->mixerflipblit = mixerflipblit;
        ctx->mixers[mixer_id]->nb_compositors = getenv("dfbbcm_one_compositor") ? 1 : ctx->graphics_constraints.nb_compositors;
        ctx->mixers[mixer_id]->driver_compositor = ctx->driver_compositor;
        if (mixer_id > BCM_MIXER_0)
        {
            ctx->mixers[mixer_id]->mixer_compositor = ctx->driver_compositor;
        }
        else
        {
            ctx->mixers[mixer_id]->mixer_compositor = ctx->mixer_compositor;
        }

        /* specialize the layer to assign to the mixer/s */
        {
            BCMCoreLayerHandle layer = ctx->bcm_layers.layer[mixer_id].handle;
            layer->layer_id = 0;
            layer->level = -1;
            layer->premultiplied = true;
            layer->bcmcore_type = BCMCORELAYER_OUTPUT;
            layer->graphics_window = &ctx->primary_graphics_window[mixer_id];
            snprintf(layer->layer_name, (DFB_DISPLAY_LAYER_DESC_NAME_LENGTH * sizeof(char)), "primary_%d", mixer_id);

            /* opening mixer and assign primary layer for it */
            if ((ret = BCMMixerOpen(ctx->mixers[mixer_id], layer)) != DFB_OK) {
                goto out;
            }

            /* register that layer belong to this mixer */
            ctx->bcm_layers.layer[mixer_id].mixer = ctx->mixers[mixer_id];
            ctx->bcm_layers.layer[mixer_id].mixed_layer_id = 0;
        }
    }

    /* backward compatibility :set other mixers to default BCM_MIXER_0 */
    for (mixer_id = ctx->graphics_constraints.nb_gfxWindows[0]; mixer_id < BCM_MIXER_L; mixer_id ++)
    {
        ctx->mixers[mixer_id] = ctx->mixers[BCM_MIXER_0];
    }

    if (!(bsettop_get_config("no_sid")))
    {
        bdvd_sid_open(B_ID(0), &ctx->driver_sid);
        D_ASSERT( ctx->driver_sid != NULL );
    }
    else
    {
        ctx->driver_sid = NULL;
    }

    if (getenv("enable_fgx"))
    {
        ctx->driver_fgx = bdvd_dtk_create();
        D_ASSERT( ctx->driver_fgx != NULL );
        D_INFO("%s/system_initialize: fgx opened\n", BCMCoreModuleName);
    }
    else
    {
        ctx->driver_fgx = NULL;
    }

    ctx->MemoryMode256MB = (getenv("MemoryMode256MB") != NULL);
    ctx->DualHeap 		 = (getenv("dfb_dual_heap") != NULL);

    if (ctx->MemoryMode256MB == true)
    {
		D_INFO("256MB Memory Architecture, %s Heap Mode\n", (ctx->DualHeap == true) ? "Dual" : "Single");
	}

out:

    if (rc != bdvd_ok) {
        if (ctx->driver_bcc_pg != NULL) {
            bdvd_bcc_close(ctx->driver_bcc_pg);
        }
        if (ctx->driver_bcc_textst != NULL) {
            bdvd_bcc_close(ctx->driver_bcc_textst);
        }
        if (ctx->driver_bcc_ig != NULL) {
            bdvd_bcc_close(ctx->driver_bcc_ig);
        }
        if (ctx->driver_bcc_other != NULL) {
            bdvd_bcc_close(ctx->driver_bcc_other);
        }
    }

    return ret;
}

static DFBResult
system_shutdown( bool emergency )
{
    DFBResult ret = DFB_OK;
    DFBDisplayLayerID layer_index;
    BCMCoreLayerHandle output_layer = NULL;
    BCMMixerId mixer_id;
    uint32_t i;
    uint32_t nb_surfaces;
    unsigned int window_id;

    UNUSED_PARAM(emergency);

    D_ASSERT( dfb_bcmcore_context != NULL );

    D_DEBUG("%s/system_shutdown called\n",
            BCMCoreModuleName );

    for (mixer_id = BCM_MIXER_0; mixer_id < dfb_bcmcore_context->graphics_constraints.nb_gfxWindows[0]; mixer_id++)
    {
        if ((ret = BCMMixerStop(dfb_bcmcore_context->mixers[mixer_id])) != DFB_OK) {
            goto quit;
        }

        if ((ret = BCMMixerOutputLayerAcquire(dfb_bcmcore_context->mixers[mixer_id], &output_layer)) != DFB_OK) {
        goto quit;
    }

    nb_surfaces = output_layer->nb_surfaces;
    output_layer->nb_surfaces = 0;

        if ((ret = BCMMixerSetOutputLayerSurfaces(dfb_bcmcore_context->mixers[mixer_id], output_layer)) != DFB_OK) {
        goto quit;
    }

    for (i = 0; i < nb_surfaces; i++) {
        if (output_layer->surface_buffers[i] != NULL) {
            D_DEBUG("%s/system_shutdown: Deallocate &backsurface_clone=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, &output_layer->backsurface_clone, i, output_layer->surface_buffers[i]);

            dfb_surface_destroy_buffer( &output_layer->backsurface_clone,
                                        output_layer->surface_buffers[i] );
            output_layer->surface_buffers[i] = NULL;
        }
    }

    output_layer->graphics_window = NULL;

    if ((ret = BCMMixerLayerRelease(output_layer)) != DFB_OK) {
        goto quit;
    }

    /* Supposes that layers have sequential numbering !!!!.
     * Also, this could be done in MixerClose.
     */
        for (layer_index = 0; layer_index < dfb_bcmcore_context->mixers[mixer_id]->max_layers; layer_index++) {
        /* Will unlock and release the layer.
         * With a true to noerror_missing, will no generate an error message or code if the
         * layer is not registered.
         */
            if ((ret = BCMMixerLayerUnregister(dfb_bcmcore_context->mixers[mixer_id], layer_index, true)) != DFB_OK) {
            goto quit;
        }
    }

        if ((ret = BCMMixerClose(dfb_bcmcore_context->mixers[mixer_id])) != DFB_OK) {
        goto quit;
    }
    }


    if (dfb_bcmcore_context->driver_compositor) {
        if (bdvd_graphics_compositor_close(dfb_bcmcore_context->driver_compositor) != b_ok) {
             D_ERROR("%s/system_shutdown: could not close driver_compositor\n",
                     BCMCoreModuleName);
             ret = DFB_INIT;
             goto quit;
        }
    }

    if (dfb_bcmcore_context->mixer_compositor) {
        if (bdvd_graphics_compositor_close(dfb_bcmcore_context->mixer_compositor) != b_ok) {
             D_ERROR("%s/system_shutdown: could not close mixer_compositor\n",
                     BCMCoreModuleName);
             ret = DFB_INIT;
             goto quit;
        }
    }

    for (window_id = 0; window_id < (dfb_bcmcore_context->graphics_constraints.nb_gfxWindows[0]); window_id++)
    {
        if (dfb_bcmcore_context->primary_graphics_window[window_id].window_handle) {
            if (bdvd_graphics_window_close(dfb_bcmcore_context->primary_graphics_window[window_id].window_handle) != b_ok) {
             D_ERROR("%s/system_shutdown: could not close graphics_window\n",
                     BCMCoreModuleName);
             ret = DFB_INIT;
             goto quit;
        }
            dfb_bcmcore_context->primary_graphics_window[window_id].layer_id = 0;
            dfb_bcmcore_context->primary_graphics_window[window_id].layer_assigned = NULL;
            dfb_bcmcore_context->primary_graphics_window[window_id].window_handle = NULL;
            dfb_bcmcore_context->primary_graphics_window[window_id].display_handle = NULL;
    }
    }

    if (!(bsettop_get_config("no_sid")))
    {
        bdvd_sid_close(dfb_bcmcore_context->driver_sid);
    }
    dfb_bcmcore_context->driver_sid = NULL;

    if (getenv("enable_fgx"))
    {
        bdvd_dtk_destroy(dfb_bcmcore_context->driver_fgx);
    }
    dfb_bcmcore_context->driver_fgx = NULL;

/* Well close the whole thing anyway, if something went wrong, there is nothing to do */
quit:

#ifdef BCMUSESLINUXINPUTS
     vt_kbd_input_close();
#endif

     fusion_call_destroy( &dfb_bcmcore_context->shared->call );

     fusion_skirmish_prevail( &dfb_bcmcore_context->shared->lock );

     /* Do some bsettop related cleanup here */

     fusion_skirmish_destroy( &dfb_bcmcore_context->shared->lock );

     SHFREE( dfb_bcmcore_context->shared );

    for (mixer_id = BCM_MIXER_0; mixer_id < BCM_MIXER_L; mixer_id++)
    {
        D_FREE( dfb_bcmcore_context->mixers[mixer_id]);
    }

     D_FREE( dfb_bcmcore_context );
     dfb_bcmcore_context = NULL;

     D_DEBUG("%s/system_shutdown called\n", BCMCoreModuleName);

     return ret;
}

/* Multi Application support not implemented yet */
static DFBResult
system_join( CoreDFB *core, void **data )
{
    UNUSED_PARAM(core);
    UNUSED_PARAM(data);

    D_DEBUG("%s/system_join called\n",
            BCMCoreModuleName );

    return DFB_UNIMPLEMENTED;
}

/* Multi Application support not implemented yet */
static DFBResult
system_leave( bool emergency )
{
    UNUSED_PARAM(emergency);

    D_DEBUG("%s/system_leave called\n",
            BCMCoreModuleName );

    return DFB_UNIMPLEMENTED;
}

/* Power management support not implemented yet */
static DFBResult
system_suspend()
{
    D_DEBUG("%s/system_suspend called\n",
            BCMCoreModuleName );

    return DFB_UNIMPLEMENTED;
}

/* Power management support not implemented yet */
static DFBResult
system_resume()
{
    D_DEBUG("%s/system_resume called\n",
            BCMCoreModuleName );

    return DFB_UNIMPLEMENTED;
}

static volatile void *
system_map_mmio( unsigned int    offset,
                 int             length )
{
    UNUSED_PARAM(offset);
    UNUSED_PARAM(length);

    D_DEBUG("%s/system_map_mmio called\n",
            BCMCoreModuleName );

    return NULL;
}

static void
system_unmap_mmio( volatile void  *addr,
                   int             length )
{
    UNUSED_PARAM(addr);
    UNUSED_PARAM(length);

    D_DEBUG("%s/system_unmap_mmio called\n",
            BCMCoreModuleName );
}

static int
system_get_accelerator()
{
    D_DEBUG("%s/system_get_accelerator called\n",
            BCMCoreModuleName );

    return dfb_bcmcore_context->graphics_constraints.chip_id;
}

static VideoMode *
system_get_modes()
{
    D_DEBUG("%s/system_get_modes called\n",
            BCMCoreModuleName );

    return NULL;
}

static VideoMode *
system_get_current_mode()
{
    D_DEBUG("%s/system_get_current_mode called\n",
            BCMCoreModuleName );

    return NULL;
}

static DFBResult
system_thread_init()
{
    D_DEBUG("%s/system_thread_init called\n",
            BCMCoreModuleName );

    return DFB_OK;
}

static bool
system_input_filter( CoreInputDevice *device,
                     DFBInputEvent   *event )
{
    UNUSED_PARAM(device);
    UNUSED_PARAM(event);

    D_DEBUG("%s/system_input_filter called\n",
            BCMCoreModuleName );

    return false;
}

static unsigned long
system_video_memory_physical( unsigned int offset )
{
    UNUSED_PARAM(offset);

    /* [xm] Not really used */
    return 0;
}

static void *
system_video_memory_virtual( unsigned int offset )
{
    return dfb_bcmcore_context->system_video_memory_virtual(offset);
}

static unsigned int
system_videoram_length(unsigned int partition)
{

	if (dfb_bcmcore_context->DualHeap == false)
	{
		switch (partition)
		{
			case 0:
				return dfb_bcmcore_context->graphics_constraints.videomemory_length;
			break;

			default:
				return 0;
			break;

		}
	}
	else
	{
		unsigned int reserved_partition_size =
			(3 *  960 * 1080 * 4) + /* Primary Layers: half res    */
			(    1920 * 1080 * 4) + /* OSD Layer: front bufferonly */
			(    1920 * 1080 * 1) + /* PG Laye r: front only       */
			(     960 * 1080 * 4) + /* BD-J Layer: front buffer    */
			(    1920 * 1080 * 4) + /* BD-J Layer: back buffer     */
			(     720 *  480 * 2) + /* BD-J BG Layer               */
			                   4;   /* not really necessary but just in case */

		switch (partition)
		{
			case 0:
				return dfb_bcmcore_context->graphics_constraints.videomemory_length - reserved_partition_size;
			break;

			default:
				return reserved_partition_size;
			break;

		}
	}
}

static unsigned int
system_get_partition_type(CoreSurfacePolicy policy)
{
    switch (policy)
    {
        case CSP_VIDEOONLY_RESERVED:
            return 0;
        break;
        default:
            return 1;
        break;
    }
}

static unsigned int
system_get_num_of_partitions( )
{
	return (dfb_bcmcore_context->DualHeap == true) ? 2 : 1;
}


/** @} */

/** @addtogroup LayerInterface
 *  The following is a grouping of static functions implementing
 *  the <src/core/layers.h> interface, refer to this file for function
 *  descriptions
 *  @{
 */

/**
 *  This function inits the DFBDisplayLayerDescription and
 *  DFBDisplayLayerConfig.
 *  <pre>
 *  For now DFBDisplayLayerCapabilities are set only to DLCAPS_SURFACE for
 *  the primary layer, which is of DFBDisplayLayerTypeFlags DLTF_GRAPHICS.
 *  But there are a few other flags that could be useful for this layer
 *  mapped to bgraphics and eventually others bdecode_window.
 *
 *  bgraphics frame buffer
 *
 *  DLCAPS_SURFACE           -> The layer has a surface that can be drawn to.
 *                              bdecode_window cannot have this flag define,
 *                              because we don't have access to its surface,
 *                              this is managed by the mpeg decoder source.
 *  DLCAPS_OPACITY           -> The layer supports blending with layer(s) below
 *                              based on a global alpha factor.
 *                              For the frame buffer surface, this is controlled
 *                              with the alpha field.
 *  DLCAPS_ALPHACHANNEL      -> The layer supports blending with layer(s) below
 *                              based on each pixel's alpha value.
 *                              For the frame buffer surface, this is handled
 *                              on a pixel-per-pixel basis when the
 *                              alpha field is set to 0xFF
 *  DLCAPS_FLICKER_FILTERING -> Flicker filtering can be enabled for smooth output
 *                              on interlaced display devices. A shame, but not
 *                              available for graphics on the 7038, no LPF.
 *  DLCAPS_SRC_COLORKEY      -> A specific color can be declared as transparent.
 *                              This can be set with the
 *                              graphics_settings.lower_chromakey,
 *                              graphics_settings.upper_chromakey,
 *                              graphics_settings.chromakey_enabled fields.
 *
 *  bdecode_window
 *
 *  DLCAPS_SCREEN_LOCATION   -> The layer location on the screen can be changed,
 *                              this includes position and size as normalized
 *                              values. The default is 0.0f, 0.0f, 1.0f, 1.0f.
 *                              bdecode_window could have this flag, controlled
 *                              by the bdecode_window_settings.position field.
 *  DLCAPS_DEINTERLACING     -> The layer provides optional deinterlacing for
 *                              displaying interlaced video data on progressive
 *                              display devices, true for bdecode_window and
 *                              controlled by the bdecode_window_settings.deinterlacer
 *                              field.
 *  DLCAPS_BRIGHTNESS        -> Adjustment of brightness is supported.
 *                              Done through the bdecode_window_settings.brightness
 *                              field.
 *  DLCAPS_CONTRAST          -> Adjustment of contrast is supported.
 *                              Done through the bdecode_window_settings.contrast
 *                              field.
 *  DLCAPS_HUE               -> Adjustment of hue is supported.
 *                              Done through the bdecode_window_settings.hue
 *                              field.
 *  DLCAPS_SATURATION        -> Adjustment of saturation is supported.
 *                              Done through the bdecode_window_settings.saturation
 *                              field.
 *  DLCAPS_LEVELS            -> Adjustment of the layer's level
 *                              (z position) is supported.
 *                              Done through the bdecode_window_settings.zorder
 *                              field.
 *
 *  The DFBDisplayLayerBufferMode default to DLBM_BACKVIDEO, so we can support
 *  double buffering, and this flag is required if we want BCMCoreFlipRegion to be
 *  used.
 *  DLBM_BACKSYSTEM would do this in software with dfb_back_to_front_copy,
 *  if required, we support that in BCMCoreTestRegion and
 *  primaryAllocateSurface.
 *  DLBM_FRONTONLY is supported in BCMCoreTestRegion and
 *  primaryAllocateSurface.
 *
 *  The DFBSurfacePixelFormat defaults to DSPF_ARGB, but this can be changed
 *  with the help of the BCM_GRAPHICS_PIXELFORMAT env variable.
 *  This is because we want to create the framebuffer surface as a ARGB,
 *  this is required for transparency of video overlays for support of
 *  alpha channel blending with other layers (bdecode_windows). But
 *  as mentionned previous, you will have to change the RTS settings
 *  to permit a 1920 width to give enough memory bandwidth, else it
 *  falls back to 720 width.
 *
 *  </pre>
 *  @par description
 *       DFBDisplayLayerDescription * containing primary layer description
 *  @par config
 *       DFBDisplayLayerConfig * containing primary layer configuration
 *  @return
 *      DFBResult DFB_OK, else error code
 */
static DFBResult
BCMCoreInitLayer( CoreLayer                  *layer,
                  void                       *driver_data,
                  void                       *layer_data,
                  DFBDisplayLayerDescription *description,
                  DFBDisplayLayerConfig      *config,
                  DFBColorAdjustment         *adjustment )
{
    DFBResult ret = DFB_OK;
    char * getenv_string;

    /* Note that layer->shared is not yet assigned so no layer_id.
     * Also InitLayer is called for all preregistered layers
     * from dfb_layers_initialize (invoked once at core creation
     * time by dfb_core_initialize), so looping the same way.
     */
    static DFBDisplayLayerID current_layer_index = 0;
    BCMCoreLayerHandle bcm_layer = dfb_bcmcore_context->bcm_layers.layer[current_layer_index].handle;

    FormatString format_string;

    UNUSED_PARAM(layer);
    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(adjustment);

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMCoreInitLayer called\n",
            BCMCoreModuleName );
#endif

    /* set capabilities and type */
    description->caps = DLCAPS_SURFACE;

    description->type = DLTF_GRAPHICS;

    /* Since this function is called once by dfb_core_initialize,
     * we need to set all support caps for all types of layer.
     * The actual check with be done in SetRegion.
     */
    {
        /* Mixed layers can have their level set */
        description->caps |= DLCAPS_LEVELS;

        /* Mixed layers can be position on screen, this will set mode CLLM_LOCATION */
        description->caps |= DLCAPS_SCREEN_LOCATION;

         /* This must be set with DSCAPS_PREMULTIPLIED through the surface_caps
          * of the layer config. Here we only say this is supported, setting the surface caps.
          */
        description->caps |= DLCAPS_PREMULTIPLIED;
    }

    /* set name */
    snprintf( description->name,
              DFB_DISPLAY_LAYER_DESC_NAME_LENGTH, "%s", bcm_layer->layer_name );

    /* fill out the default configuration, it's really suggestions and the
     * application can override that */
    config->flags       = DLCONF_WIDTH       | DLCONF_HEIGHT |
                          DLCONF_PIXELFORMAT | DLCONF_BUFFERMODE;

    /* The mixer ouput surfaces should be triple buffered because of the
     * flip callback being called at RUL building instead of after the actual
     * VSYNC.
     * This is really for statically assigned layers, such as the primary
     * frame buffer.
     */
    if (bcm_layer->bcmcore_type == BCMCORELAYER_OUTPUT) {
        const char * primary_mode_string = getenv( "dfbbcm_primary_buffer_mode" ) ? : "triple";

        if (!strncmp(primary_mode_string, "single", sizeof("single"))) {
            config->buffermode = DLBM_FRONTONLY;
        }
        else if (!strncmp(primary_mode_string, "double", sizeof("double"))) {
            config->buffermode = DLBM_BACKVIDEO;
        }
        else {
            config->buffermode = DLBM_TRIPLE;
        }
        D_INFO( "%s/BCMCoreInitLayer: output layer buffer mode is %s\n",
                BCMCoreModuleName,
                primary_mode_string );
    }
    else {
        config->buffermode = DLBM_BACKVIDEO;
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG( "%s/BCMCoreInitLayer: Default layer buffer mode is %d\n",
            BCMCoreModuleName,
            config->buffermode );
#endif

    /* Default pixel format for layers is set to DSPF_ARGB.
     * This can be changed through reallocation.
     */
    config->pixelformat = DSPF_ARGB;

    /* This is for debug purposes, a way to override this default setting without
     * changing surface description in examples with:
     * desc.flags = DSDESC_PIXELFORMAT and desc.pixelformat set to desired value
     */
    getenv_string = getenv( "dfbbcm_graphics_pixelformat" );
    if (getenv_string) {
        if (!strcmp(getenv_string, "R5G6B5")) {
            config->pixelformat = DSPF_RGB16;
        }
        else if (!strcmp(getenv_string, "A1R5G5B5")) {
            config->pixelformat = DSPF_ARGB1555;
        }
        unsetenv( "dfbbcm_graphics_pixelformat" );
    }

    format_string.format = config->pixelformat;

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMCoreInitLayer: Default layer surface pixel format is %s\n",
            BCMCoreModuleName,
            ((const FormatString *)bsearch( &format_string, format_strings,
                                   NUM_FORMAT_STRINGS, sizeof(FormatString),
                                   pixelformat_compare ))->string );
#endif

    if (bdvd_graphics_window_get_video_format_dimensions(
            display_get_video_format(dfb_bcmcore_context->primary_display_handle),
            dfb_surface_pixelformat_to_bdvd(config->pixelformat),
            (unsigned int *)&config->width, (unsigned int *)&config->height) != b_ok) {
        D_ERROR("%s/BCMCoreInitLayer: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                BCMCoreModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

    bcm_layer->front_rect.x = 0;
    bcm_layer->front_rect.y = 0;
    bcm_layer->front_rect.w = config->width;
    bcm_layer->front_rect.h = config->height;

    bcm_layer->location.x = 0;
    bcm_layer->location.y = 0;
    bcm_layer->location.w = 1;
    bcm_layer->location.h = 1;

    current_layer_index++;

quit:

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMCoreInitLayer exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);
#endif

    return ret;
}

static DFBResult BCMCoreGetLevel( CoreLayer              *layer,
                                  void                   *driver_data,
                                  void                   *layer_data,
                                  int                    *level ) {

    DFBResult ret = DFB_OK;
    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);

    D_DEBUG("%s/BCMCoreGetLevel called with:\n"
            "dfb_layer_id: %u\n",
            BCMCoreModuleName,
            dfb_layer_id(layer));

    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    ret = BCMMixerLayerGetLevel(layer_ctx->mixer,
                                layer_ctx->mixed_layer_id,
                                level);

    D_DEBUG("%s/BCMCoreGetLevel exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

static DFBResult BCMCoreSetLevel( CoreLayer              *layer,
                                  void                   *driver_data,
                                  void                   *layer_data,
                                  int                     level ) {

    DFBResult ret = DFB_OK;
    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);

    D_DEBUG("%s/BCMCoreSetLevel called with:\n"
            "dfb_layer_id: %u\n"
            "level: %d\n",
            BCMCoreModuleName,
            dfb_layer_id(layer),
            level);

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    ret = BCMMixerLayerSetLevel(layer_ctx->mixer,
                                layer_ctx->mixed_layer_id,
                                level);

    D_DEBUG("%s/BCMCoreSetLevel exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

static DFBResult
BCMCoreTestRegion( CoreLayer                  *layer,
                   void                       *driver_data,
                   void                       *layer_data,
                   CoreLayerRegionConfig      *config,
                   CoreLayerRegionConfigFlags *failed )
{

    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle bcm_layer = NULL;

    CoreLayerRegionConfigFlags fail = 0;

#if D_DEBUG_ENABLED
     FormatString format_string;

     format_string.format = config->format;
#endif // D_DEBUG_ENABLED

    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(layer);
    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);

    D_DEBUG("%s/BCMCoreTestRegion called with:\n"
            "dfb_layer_id: %u\n"
            "CoreLayerRegionConfig width: %d\n"
            "CoreLayerRegionConfig height: %d\n"
            "CoreLayerRegionConfig format: %s\n"
            "CoreLayerRegionConfig surface_caps: 0x%08x\n"
            "CoreLayerRegionConfig buffermode: 0x%08x\n"
            "CoreLayerRegionConfig options: 0x%08x\n"
            "CoreLayerRegionConfigFlags pointer : %p\n"
            "CoreLayerRegionConfigFlags value : 0x%08x\n",
            BCMCoreModuleName,
            dfb_layer_id(layer),
            config->width,
            config->height,
            ((const FormatString *)bsearch( &format_string, format_strings,
                                   NUM_FORMAT_STRINGS, sizeof(FormatString),
                                   pixelformat_compare ))->string,
            config->surface_caps,
            config->buffermode,
            config->options,
            failed,
            failed ? *failed : 0);

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if (bcm_layer->bcmcore_type == BCMCORELAYER_INVALID) {
        D_ERROR("%s/BCMCoreTestRegion: layer is invalid\n",
                BCMCoreModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

    if (bcm_layer->layer_id == DLID_PRIMARY) {
        const char * primary_mode_string = getenv( "dfbbcm_primary_buffer_mode" ) ? : "triple";

        if (!strncmp(primary_mode_string, "single", sizeof("single"))) {
            config->buffermode = DLBM_FRONTONLY;
        }
        else if (!strncmp(primary_mode_string, "double", sizeof("double"))) {
            config->buffermode = DLBM_BACKVIDEO;
        }
    }

    switch (config->buffermode) {
        /* no backbuffer */
        case DLBM_FRONTONLY:
        /* If we do support double buffering,
         * the buffer would be in a heap managed by
         * the BMEM memory manager, let's call that
         * video memory, also, required for system core
         * BCMCoreFlipRegion
         * (required if DSCAPS_VIDEOONLY is set before
         * invoking CreateSurface for primary)
         */
#if 0
            /* Forcing DLBM_BACKVIDEO for flip-blit layers */
            if (bcm_layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT) {
                config->buffermode = DLBM_BACKVIDEO;
            }
#endif

        break;
        case DLBM_BACKVIDEO:
            /* Forcing DLBM_FRONTONLY for faa layers */
            if (bcm_layer->bcmcore_type == BCMCORELAYER_FAA) {
                config->buffermode = DLBM_FRONTONLY;
            }
        break;
        /* Only output layers for now */
        case DLBM_TRIPLE:
            if (bcm_layer->bcmcore_type != BCMCORELAYER_OUTPUT) {
                fail |= CLRCF_BUFFERMODE;
            }
        break;
        /* we don't support other modes */
        default:
            fail |= CLRCF_BUFFERMODE;
            break;
    }

    if (config->options)
        /* maybe for DLOP_ALPHACHANNEL
         *           DLOP_SRC_COLORKEY
         *           DLOP_OPACITY see comments for BCMCoreInitLayer
         */
        fail |= CLRCF_OPTIONS;

    if (failed)
        *failed = fail;

    if (fail)
        ret = DFB_UNSUPPORTED;

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMCoreTestRegion exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

/**
 *  This function flips the primary layer surface.
 *  <pre>
 *  Invoked only if buffermode is DLBM_BACKVIDEO.
 *  </pre>
 *  @par surface
 *       CoreSurface * containing layer surface
 *  @return
 *      DFBResult DFB_OK always returned.
 */
static DFBResult BCMCoreFlipRegion    ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        void                       *region_data,
                                        CoreSurface                *surface,
                                        DFBSurfaceFlipFlags         flags ) {
    DFBResult ret = DFB_OK;
    SurfaceBuffer * buffer;

#ifdef DUMP_SURFACE
    const char * dump_layer_id_string = getenv("dfbbcm_dump_layerid");
    uint32_t dump_layer_id;
#endif

    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);

    D_ASSERT(surface != NULL);
    buffer = surface->back_buffer;

    D_DEBUG("%s/BCMCoreFlipRegion called with:\n"
             "dfb_layer_id: %u\n",
             BCMCoreModuleName,
             dfb_layer_id(layer) );

    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    if (dfb_bcmcore_context->graphics_constraints.requires_cache_flush) {
        if (buffer->video.access & VAF_SOFTWARE_WRITE) {
            dfb_gfxcard_flush_texture_cache(buffer);
            buffer->video.access &= ~VAF_SOFTWARE_WRITE;
        }
    }

#if 0
    if (layer_ctx->mixer->mixerflipblit && layer_ctx->mixer->nb_compositors == 2)
    {
        if (buffer->video.access & VAF_HARDWARE_READ) {
            dfb_gfxcard_sync();
            buffer->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
        }

        if (buffer->video.access & VAF_HARDWARE_WRITE) {
            dfb_gfxcard_wait_serial( &buffer->video.serial );
            buffer->video.access &= ~VAF_HARDWARE_WRITE;
        }
    }
#endif

#ifdef DUMP_SURFACE
    if (dump_layer_id_string) {
        dump_layer_id = strtoul(dump_layer_id_string, NULL, 10);
        if (mixed_layer_id == dump_layer_id) {
            if (ret = dfb_surface_dump_buffer( surface, surface->back_buffer, ".", dump_layer_id_string, 0) != DFB_OK) {
                goto quit;
            }
        }
    }
#endif

    if ((ret = BCMMixerLayerFlip(layer_ctx->mixer,
                                 layer_ctx->mixed_layer_id,
                                 NULL,
                                 dfb_bcmcore_context->waitforflip,
                                 flags)) != DFB_OK) {
        goto quit;
    }

quit:

    D_DEBUG("%s/BCMCoreFlipRegion exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

static DFBResult BCMCoreUpdateRegionFlags( CoreLayer                  *layer,
                                           void                       *driver_data,
                                           void                       *layer_data,
                                           void                       *region_data,
                                           CoreSurface                *surface,
                                           const DFBRegion            *update,
                                           DFBSurfaceFlipFlags         flags)
{
    DFBResult ret = DFB_OK;
    SurfaceBuffer * buffer;

#ifdef DUMP_SURFACE
    const char * dump_layer_id_string = getenv("dfbbcm_dump_layerid");
    uint32_t dump_layer_id;
    bool dump_surface = false;
#endif

    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);

    D_ASSERT(surface != NULL);
    buffer = surface->back_buffer;

    D_DEBUG("%s/BCMCoreUpdateRegion called with:\n"
             "dfb_layer_id: %u\n",
             BCMCoreModuleName,
             dfb_layer_id(layer) );

    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    if (dfb_bcmcore_context->graphics_constraints.requires_cache_flush) {
        if (buffer->video.access & VAF_SOFTWARE_WRITE) {
            dfb_gfxcard_flush_texture_cache(buffer);
            buffer->video.access &= ~VAF_SOFTWARE_WRITE;
        }
    }

#if 0
    if (layer_ctx->mixer->mixerflipblit && layer_ctx->mixer->nb_compositors == 2)
    {
        if (buffer->video.access & VAF_HARDWARE_READ) {
            dfb_gfxcard_sync();
            buffer->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
        }

        if (buffer->video.access & VAF_HARDWARE_WRITE) {
            dfb_gfxcard_wait_serial( &buffer->video.serial );
            buffer->video.access &= ~VAF_HARDWARE_WRITE;
        }
    }
#endif

#ifdef DUMP_SURFACE
    if (dump_layer_id_string) {
        dump_layer_id = strtoul(dump_layer_id_string, NULL, 10);
        if (mixed_layer_id == dump_layer_id) {
            if (ret = dfb_surface_dump_buffer( surface, surface->back_buffer, ".", 
                dump_layer_id_string, 0) != DFB_OK) {
                goto quit;
            }
        }
    }
#endif

    if (update &&
        update->x2 &&
        update->y2) {
        D_DEBUG("%s/BCMCoreUpdateRegion called with:\n"
                 "x1 %d y1 %d x2 %d y2 %d\n",
                 BCMCoreModuleName,
                 update->x1,
                 update->y1,
                 update->x2,
                 update->y2 );

        if ((ret = BCMMixerLayerFlip(layer_ctx->mixer,
                                     layer_ctx->mixed_layer_id,
                                     update,
                                     dfb_bcmcore_context->waitforflip,
                                     flags)) != DFB_OK) {
            goto quit;
        }
    }
    else {
        D_DEBUG("%s/BCMCoreUpdateRegion called without a region\n",
                 BCMCoreModuleName );

        if ((ret = BCMMixerLayerFlip(layer_ctx->mixer,
                                     layer_ctx->mixed_layer_id,
                                     NULL,
                                     dfb_bcmcore_context->waitforflip,
                                     flags)) != DFB_OK) {
            goto quit;
        }
    }

quit:

    D_DEBUG("%s/BCMCoreUpdateRegion exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

static DFBResult BCMCoreUpdateRegion( CoreLayer                  *layer,
                                      void                       *driver_data,
                                      void                       *layer_data,
                                      void                       *region_data,
                                      CoreSurface                *surface,
                                      const DFBRegion            *update) {

    return BCMCoreUpdateRegionFlags(layer, driver_data, layer_data, region_data, surface, update, DSFLIP_NONE);
}

/**
 *  TO UPDATE!
 *  This function allocates the layer surface(s).
 *  <pre>
 *  The DFBSurfaceCapabilities for the primary layer, primary surface are the
 *  following. Note that we are neglecting config->surface_caps and passing
 *  our own capabilities.
 *
 *  DSCAPS_PRIMARY       -> This is the primary surface.
 *  DSCAPS_VIDEOONLY     -> Surface data is permanently stored in video memory.
 *                          There's no system memory allocation/storage.
 *                          Eventually maybe if we have our own surfacemanager.
 *  DSCAPS_DOUBLE        -> Surface is double buffered is set depending on
 *                          config->buffermode.
 *  </pre>
 *  @par ret_width
 *       CoreLayerRegionConfig * containing desired config
 *  @par ret_surface
 *       CoreSurface ** returning primary surface
 *  @return
 *      DFBResult DFB_OK on success, error code otherwise.
 */
static inline DFBResult
BCMCoreAllocateSurface( BCMMixerHandle         mixer,
                        BCMCoreLayerHandle     bcm_layer,
                        CoreLayerRegionConfig  *config,
                        CoreSurface            **ret_surface )
{

    DFBResult              ret = DFB_OK;

    bresult                err;
    CoreSurface            *surface;

    /* It is now preferable to use our own surface manager, so let it know
     * that video memory should be used.
     */
    DFBSurfaceCapabilities caps = DSCAPS_VIDEOONLY;

    bdvd_video_format_e video_format;

    int32_t i;

    DFBRectangle clip_rect;

    CoreSurfacePolicy surface_policy = CSP_VIDEOONLY;

#if D_DEBUG_ENABLED
     FormatString format_string;

     D_ASSERT(config != NULL);

     format_string.format = config->format;
#endif // D_DEBUG_ENABLED

    D_ASSERT(bcm_layer != NULL);

    D_DEBUG("%s/BCMCoreAllocateSurface called with:\n"
            "layer_id: %u\n"
            "CoreLayerRegionConfig width: %d\n"
            "CoreLayerRegionConfig height: %d\n"
            "CoreLayerRegionConfig format: %s\n"
            "CoreLayerRegionConfig surface_caps: 0x%08x\n"
            "CoreLayerRegionConfig buffermode: 0x%08x\n",
            BCMCoreModuleName,
            bcm_layer->layer_id,
            config->width,
            config->height,
            ((const FormatString *)bsearch( &format_string, format_strings,
                                   NUM_FORMAT_STRINGS, sizeof(FormatString),
                                   pixelformat_compare ))->string,
            config->surface_caps,
            config->buffermode);

    /* TODO CHECK TYPE
     * if FAA and no number assigned by the set functions,
     * then default to 1? Else if number reassign number and
     * don't care about buffer mode. Always say it's front only.
     */
    if (bcm_layer->bcmcore_type == BCMCORELAYER_OUTPUT) {
        /* Mixer ouput layers surfaces are always premultiplied, we must implement
         * CLRCF_SURFACE_CAPS in SetRegion for this type.
         */
        caps |= DSCAPS_PREMULTIPLIED;
    }

    D_ASSERT(bcm_layer->layer_id == DLID_PRIMARY ? bcm_layer->bcmcore_type == BCMCORELAYER_OUTPUT : true);

    if (bcm_layer->layer_id == DLID_PRIMARY)
    {
        caps |= DSCAPS_PRIMARY;
    }

    /* First step is initialization of the framebuffer_create_settings structure
     */
    if (config->width == 0 || config->height == 0) {
        D_ERROR("%s/BCMCoreAllocateSurface: invalid width %u or height %u\n",
                BCMCoreModuleName,
                config->width,
                config->height);
        ret = DFB_INVARG;
        goto quit;
    }

    if (bcm_layer->bcmcore_type == BCMCORELAYER_BACKGROUND) {
        switch (config->format) {
            case DSPF_YUY2:
            case DSPF_UYVY:
                bcm_layer->backsurface_clone.format = config->format;
            break;
            default:
                bcm_layer->backsurface_clone.format = config->format = DSPF_UYVY;
            break;
        }
    }
    else {
        /* Then set pixel format, for now we only support:
         * DSPF_ARGB (recommended in BCMCoreInitLayer for pixel basis blending with other layers)
         * DSPF_RGB16
         * DSPF_ARGB1555
         * DSPF_LUT8
         */
        switch (config->format) {
            /* The two following could happen after alloc/dealloc of
             * a background layer.
             */
            case DSPF_YUY2:
            case DSPF_UYVY:
            case DSPF_RGB32:
                bcm_layer->backsurface_clone.format = config->format = DSPF_ARGB;
            break;
            case DSPF_ARGB:
            case DSPF_ABGR:
            case DSPF_RGB16:
            case DSPF_ARGB1555:
            case DSPF_ARGB4444:
            case DSPF_LUT8:
                bcm_layer->backsurface_clone.format = config->format;
            break;
            default:
                D_ERROR("%s/BCMCoreAllocateSurface: config->format is not supported\n",
                         BCMCoreModuleName );
                ret = DFB_UNSUPPORTED;
                goto quit;
        }
    }

	/* In Dual Heap Configuration Layers are allocated from reserved partition.*/
	if (dfb_bcmcore_context->DualHeap == true)
	{
		switch (bcm_layer->bcmcore_type)
		{
			case BCMCORELAYER_OUTPUT:
			case BCMCORELAYER_MIXER_FLIPBLIT:
			case BCMCORELAYER_BACKGROUND:
				surface_policy = CSP_VIDEOONLY_RESERVED;

			break;
			default:
				surface_policy = CSP_VIDEOONLY;
			break;
		}
	}

    bcm_layer->backsurface_clone.manager = dfb_gfxcard_surface_manager();
    /* Apply params to front as well, will be changed next */
    bcm_layer->frontsurface_clone = bcm_layer->backsurface_clone;

    video_format = display_get_video_format(dfb_bcmcore_context->primary_display_handle);

    /* Forcing DLBM_FRONTONLY for faa layers */
    if (bcm_layer->bcmcore_type == BCMCORELAYER_FAA) {
        config->buffermode = DLBM_FRONTONLY;
    }

    /* Forcing DLBM_FRONTONLY for bg layer */
    if (bcm_layer->bcmcore_type == BCMCORELAYER_BACKGROUND) {
        config->buffermode = DLBM_FRONTONLY;
    }

    /* If BCMCoreRemoveRegion has been called, the layer must have been BCMCoreLayerUnregister afterwards */
    /* D_ASSERT( *ret_surface == NULL ? bcm_layer->nb_surfaces == 0 : true ); */
    /* Failing with Sonic code right now */

    /* Vice versa, if BCMCoreLayerUnregister has been called, BCMCoreRemoveRegion should have been called beforehand */
    D_ASSERT( *ret_surface != NULL ? bcm_layer->nb_surfaces != 0 : true );

    /* That way we will be able to comply with the application desires on buffermode,
     * for the one we support at least
     */
    switch (config->buffermode) {
        case DLBM_FRONTONLY:
            D_DEBUG("%s/BCMCoreAllocateSurface: config->buffermode=DLBM_FRONTONLY\n",
                BCMCoreModuleName);

            caps &= ~DSCAPS_FLIPPING;

            switch (bcm_layer->bcmcore_type) {
                /* Those layers can be scaled in the mixing or video composition */
                case BCMCORELAYER_BACKGROUND:
                    bcm_layer->nb_surfaces = 1;
                    bcm_layer->backsurface_clone = bcm_layer->frontsurface_clone;

 					/* re-enforce size limitation on background layer size */
                    config->width = 720;
                    config->height = 480;
                case BCMCORELAYER_FAA:
                    bcm_layer->frontsurface_clone.width = config->width;
                    bcm_layer->frontsurface_clone.height = config->height;

                    /* For FAA you want to leave the value set by BCMCoreLayerSet */
                    bcm_layer->nb_surfaces = bcm_layer->nb_surfaces ? bcm_layer->nb_surfaces : 1;
                    bcm_layer->backsurface_clone = bcm_layer->frontsurface_clone;
                break;
                case BCMCORELAYER_OUTPUT:
                case BCMCORELAYER_MIXER_FLIPTOGGLE:
                case BCMCORELAYER_MIXER_FLIPBLIT:
                    /* First pass it to adjust the surface dimensions to the current maximum
                     * allowed for the display format.
                     */
					bcm_layer->frontsurface_clone.width = config->width;
					bcm_layer->frontsurface_clone.height = config->height;

					/* allow front only surfaces to be resized to any geometry */
					bcm_layer->frontsurface_clone.min_width = config->width;
					bcm_layer->frontsurface_clone.min_height = config->height;

					bcm_layer->backsurface_clone = bcm_layer->frontsurface_clone;

                    /*
                     * Virtual front buffer (primary layer) is always ARGB.
                     */
                    bcm_layer->frontsurface_clone.format = DSPF_ARGB;

					if (bdvd_graphics_window_get_video_format_dimensions(
							bdvd_video_format_1080i,
							dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
							&bcm_layer->frontsurface_clone.width, &bcm_layer->frontsurface_clone.height) != b_ok) {
						D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
								BCMCoreModuleName);
						ret = DFB_FAILURE;
						goto quit;
					}

                    bcm_layer->nb_surfaces = 1;

                    D_DEBUG("After Config %dx%d BackSurf %dx%d BackRect %dx%d FrontSurf %dx%d FrontRect %dx%d\n",
                    config->width,
                    config->height,
                    bcm_layer->backsurface_clone.width,
                    bcm_layer->backsurface_clone.height,
                    bcm_layer->back_rect.w,
                    bcm_layer->back_rect.h,
                    bcm_layer->frontsurface_clone.width,
                    bcm_layer->frontsurface_clone.height,
                    bcm_layer->front_rect.w,
                    bcm_layer->front_rect.h);

                break;
                default:
                    D_ERROR("%s/BCMCoreAllocateSurface: invalid BCMCoreLayerType for DLBM_FRONTONLY\n",
                            BCMCoreModuleName);
                    ret = DFB_FAILURE;
                    goto quit;
            }

            /* In case of DLBM_FRONTONLY, both back and front
             * surface_create_settings are the same. In fact, back should
             * not be used.
             */


            bcm_layer->back_surface_index = 0;
            bcm_layer->idle_surface_index = 0;
            bcm_layer->front_surface_index = 0;
        break;
        case DLBM_BACKVIDEO:
            D_DEBUG("%s/BCMCoreAllocateSurface: config->buffermode=DLBM_BACKVIDEO\n",
                BCMCoreModuleName);

            caps &= ~DSCAPS_TRIPLE;
            caps |=  DSCAPS_DOUBLE;

            switch (bcm_layer->bcmcore_type) {
                case BCMCORELAYER_OUTPUT:
                case BCMCORELAYER_MIXER_FLIPTOGGLE:
                    /* All the surfaces for the mixer ouput layers are the same size */

                    /* First pass it to adjust the surface dimensions to the current maximum
                     * allowed for the display format.
                     */
                    if (bdvd_graphics_window_get_video_format_dimensions(
                            video_format,
                            dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
                            &bcm_layer->frontsurface_clone.width, &bcm_layer->frontsurface_clone.height) != b_ok) {
                        D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                                BCMCoreModuleName);
                        ret = DFB_FAILURE;
                        goto quit;
                    }

                    config->width = config->width > (int)bcm_layer->frontsurface_clone.width ? (int)bcm_layer->frontsurface_clone.width : config->width;
                    config->height = config->height > (int)bcm_layer->frontsurface_clone.height ? (int)bcm_layer->frontsurface_clone.height : config->height;

                    /* Second pass will set the dimensions used for surface creation the
                     * maximum allowed for all display format. This does not affect
                     * config->width and height.
                     */
                    if (bdvd_graphics_window_get_video_format_dimensions(
                            bdvd_video_format_1080i,
                            dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
                            &bcm_layer->frontsurface_clone.width, &bcm_layer->frontsurface_clone.height) != b_ok) {
                        D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                                BCMCoreModuleName);
                        ret = DFB_FAILURE;
                        goto quit;
                    }

                    bcm_layer->frontsurface_clone.min_width = bcm_layer->frontsurface_clone.width;
                    bcm_layer->frontsurface_clone.min_height = bcm_layer->frontsurface_clone.height;

                    /* For the output layer both back and front
                     * surface_create_settings are the same.
                     */
                    bcm_layer->backsurface_clone = bcm_layer->frontsurface_clone;
                break;
                case BCMCORELAYER_MIXER_FLIPBLIT:
                    /* Special case for the other layers, if double buffered will always flip blit,
                     * and the front buffer is always in ARGB to avoid pixel format conversion at sync
                     * time of layers.
                     */
                    bcm_layer->frontsurface_clone.format = DSPF_ARGB;

                    /* On other layers the back buffer can have the dimensions specified by the layer config, this would be
                     * scaled when doing a flip blit
                     */
                    bcm_layer->backsurface_clone.width = config->width;
                    bcm_layer->backsurface_clone.height = config->height;

                    /* Second pass will set the dimensions used for surface creation the
                     * maximum allowed for all display format. This does not affect
                     * config->width and height.
                     */
                    if (bdvd_graphics_window_get_video_format_dimensions(
                            bdvd_video_format_1080i,
                            dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
                            &bcm_layer->frontsurface_clone.width, &bcm_layer->frontsurface_clone.height) != b_ok) {
                        D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                                BCMCoreModuleName);
                        ret = DFB_FAILURE;
                        goto quit;
                    }

                    bcm_layer->frontsurface_clone.min_width = bcm_layer->frontsurface_clone.width;
                    bcm_layer->frontsurface_clone.min_height = bcm_layer->frontsurface_clone.height;


                    if ((bcm_layer->dfb_ext_type == bdvd_dfb_ext_hddvd_subpicture_layer) ||
                        (bcm_layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_presentation_layer) ||
                        (bcm_layer->dfb_ext_type == bdvd_dfb_ext_bd_hdmv_interactive_layer))
                    {
                        /* limit to max width for paletizzed 8 format */
                        bcm_layer->backsurface_clone.min_width = 1920;
                    }
                    else
                    {
#ifndef DFB_LAYER_DISABLE_WORST_CASE_ALLOC
                        /* Largest possible */
                        bcm_layer->backsurface_clone.min_width = 1920*4/DFB_BYTES_PER_PIXEL(bcm_layer->backsurface_clone.format);
#else
                        /* Alloc as needed */
                        bcm_layer->backsurface_clone.min_width = config->width*4/DFB_BYTES_PER_PIXEL(bcm_layer->backsurface_clone.format);
#endif
                    }

#ifndef DFB_LAYER_DISABLE_WORST_CASE_ALLOC
                    bcm_layer->backsurface_clone.min_height = 1080;
#else
                    /* Alloc as needed */
                    bcm_layer->backsurface_clone.min_height = config->height;
#endif
                break;
                case BCMCORELAYER_BACKGROUND:
                    bcm_layer->frontsurface_clone.width = config->width;
                    bcm_layer->frontsurface_clone.height = config->height;
                    bcm_layer->frontsurface_clone.min_width = bcm_layer->frontsurface_clone.width;
                    bcm_layer->frontsurface_clone.min_height = bcm_layer->frontsurface_clone.height;

                    /* For the background layer both back and front
                     * surface_create_settings are the same.
                     */
                    bcm_layer->backsurface_clone = bcm_layer->frontsurface_clone;
                break;
                default:
                    D_ERROR("%s/BCMCoreAllocateSurface: invalid BCMCoreLayerType for DLBM_BACKVIDEO\n",
                            BCMCoreModuleName);
                    ret = DFB_FAILURE;
                    goto quit;
            }

            bcm_layer->back_surface_index = 0;
            bcm_layer->idle_surface_index = 1;
            bcm_layer->front_surface_index = 1;
            bcm_layer->nb_surfaces = 2;
        break;
        case DLBM_TRIPLE:
            D_DEBUG("%s/BCMCoreAllocateSurface: config->buffermode=DLBM_TRIPLE\n",
                BCMCoreModuleName);

            caps &= ~DSCAPS_DOUBLE;
            caps |=  DSCAPS_TRIPLE;

            switch (bcm_layer->bcmcore_type) {
                case BCMCORELAYER_OUTPUT:
                    /* First pass it to adjust the surface dimensions to the current maximum
                     * allowed for the display format.
                     */
                    if (bdvd_graphics_window_get_video_format_dimensions(
                            video_format,
                            dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
                            &bcm_layer->frontsurface_clone.width, &bcm_layer->frontsurface_clone.height) != b_ok) {
                        D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                                BCMCoreModuleName);
                        ret = DFB_FAILURE;
                        goto quit;
                    }

                    config->width = config->width > (int)bcm_layer->frontsurface_clone.width ? (int)bcm_layer->frontsurface_clone.width : config->width;
                    config->height = config->height > (int)bcm_layer->frontsurface_clone.height ? (int)bcm_layer->frontsurface_clone.height : config->height;

                    /* Second pass will set the dimensions used for surface creation the
                     * maximum allowed for all display format. This does not affect
                     * config->width and height.
                     */
                    if (bdvd_graphics_window_get_video_format_dimensions(
                            bdvd_video_format_1080i,
                            dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
                            &bcm_layer->frontsurface_clone.width, &bcm_layer->frontsurface_clone.height) != b_ok) {
                        D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                                BCMCoreModuleName);
                        ret = DFB_FAILURE;
                        goto quit;
                    }

                    bcm_layer->frontsurface_clone.min_width = bcm_layer->frontsurface_clone.width;
                    bcm_layer->frontsurface_clone.min_height = bcm_layer->frontsurface_clone.height;
                break;
                case BCMCORELAYER_BACKGROUND:
                    bcm_layer->frontsurface_clone.width = config->width;
                    bcm_layer->frontsurface_clone.height = config->height;
                    bcm_layer->frontsurface_clone.min_width = bcm_layer->frontsurface_clone.width;
                    bcm_layer->frontsurface_clone.min_height = bcm_layer->frontsurface_clone.height;
                break;
                default:
                    D_ERROR("%s/BCMCoreAllocateSurface: invalid BCMCoreLayerType for DLBM_TRIPLE\n",
                            BCMCoreModuleName);
                    ret = DFB_FAILURE;
                    goto quit;
            }

            /* For the output layers, both back and front create_settings are the same as
             * there is no flip-blit.
             */
            bcm_layer->backsurface_clone = bcm_layer->frontsurface_clone;

            bcm_layer->back_surface_index = 0;
            bcm_layer->idle_surface_index = 1;
            bcm_layer->front_surface_index = 2;
            bcm_layer->nb_surfaces = 3;
        break;
        default:
            D_ERROR("%s/BCMCoreAllocateSurface: config->buffermode is not supported\n",
                     BCMCoreModuleName );
            ret = DFB_UNSUPPORTED;
            goto quit;
    }

    D_DEBUG("%s/BCMCoreAllocateSurface: creating surfaces for layer %u\n",
             BCMCoreModuleName,
             bcm_layer->layer_id );

    switch (bcm_layer->bcmcore_type) {
        case BCMCORELAYER_OUTPUT:
            D_DEBUG("%s/BCMCoreAllocateSurface: bcm_layer->bcmcore_type=BCMCORELAYER_OUTPUT\n",
                BCMCoreModuleName);

            /* If the ret_surface is null, everything should be deallocated by now */
            if (*ret_surface == NULL) {
                /* Deallocation is done in BCMGfxDeallocate, but we need to clear the surface handles */
                memset(bcm_layer->surface_buffers, 0, sizeof(bcm_layer->surface_buffers));
                if ( (ret = create_graphics_framebuffers(mixer, bcm_layer, &bcm_layer->frontsurface_clone, surface_policy)) != DFB_OK) {
                    goto quit;
                }
            }
            /* We can never reallocate the mixer ouput layers buffers */
            else {
                /* This is a hack to support examples, the default is always the max available
                 * surfaces for the mixer ouput layers, so you can only change the buffermode
                 * to something less. This is to restart the work and display indexes.
                 * Unset then set.
                 */
                if ((ret = BCMMixerSetOutputLayerSurfaces(mixer, bcm_layer)) != DFB_OK) {
                    goto quit;
                }
            }
        break;
        case BCMCORELAYER_MIXER_FLIPBLIT:
        {
            D_ASSERT(bcm_layer->nb_surfaces < 3);

            D_DEBUG("%s/BCMCoreAllocateSurface: bcm_layer->bcmcore_type=%d\n",
                BCMCoreModuleName,
                bcm_layer->bcmcore_type);

            if (*ret_surface == NULL) {
                /* Deallocation is done in BCMGfxDeallocate, but we need to clear the surface handles */
                memset(bcm_layer->surface_buffers, 0, sizeof(bcm_layer->surface_buffers));
            }
            /* Already done for the mixer ouput layers, also those surfaces on the primary layer
             * cannot be deallocated or reallocated. Also the reduce memory fragmentation,
             * we should really verify the need to reallocate the surfaces!!!!. TODO Really only
             * changing the pixel format should require reallocation, even the back buffer should
             * always be the same size but clipping adjusted accordingly.
             */
            D_DEBUG("%s/BCMCoreAllocateSurface: Allocate %d surfaces!\n",
                BCMCoreModuleName,
                bcm_layer->nb_surfaces);

			if (bcm_layer->nb_surfaces != 1)
            {
				for (i = (bcm_layer->nb_surfaces - 1); i >= 0; i--) {
					if (bcm_layer->surface_buffers[i] == NULL) {
						/* The front surface is always the size of the display */
						CoreSurface * surface = (i == bcm_layer->nb_surfaces-1) ? &bcm_layer->frontsurface_clone : &bcm_layer->backsurface_clone;
						if (dfb_surface_allocate_buffer( surface,
														 surface_policy,
														 surface->format,
														 &bcm_layer->surface_buffers[i] ) != DFB_OK) {
							D_ERROR("%s/BCMCoreAllocateSurface: error creating layer %u buffer %d\n",
									 BCMCoreModuleName,
									 bcm_layer->layer_id,
									 i );
							ret = DFB_FAILURE;
							goto quit;
						}
						else{
							D_DEBUG("%s/BCMCoreAllocateSurface: Allocate surface=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, surface, i, bcm_layer->surface_buffers[i]);
						}

						if ((err = bdvd_graphics_surface_clear(dfb_bcmcore_context->driver_compositor, ((bdvd_graphics_surface_h*)bcm_layer->surface_buffers[i]->video.ctx)[0])) != b_ok) {
							D_ERROR("%s/BCMCoreAllocateSurface: error calling bdvd_graphics_surface_clear %d\n",
									 BCMCoreModuleName,
									 err );
							ret = DFB_FAILURE;
							goto quit;
						}
					}
					else{
						D_DEBUG("%s/BCMCoreAllocateSurface: No need to allocate surface %d.  It already exists. bcm_layer->surface_buffers[%d]=0x%08x\n",
							BCMCoreModuleName,
							i,i, bcm_layer->surface_buffers[i]);
					}
				}
			}
			else
			{
				i = 0;
				if (bcm_layer->surface_buffers[i] == NULL) {
					/* The front surface is always the size of the display */
					CoreSurface * surface = &bcm_layer->backsurface_clone;
					if (dfb_surface_allocate_buffer( surface,
													 surface_policy,
													 surface->format,
													 &bcm_layer->surface_buffers[i] ) != DFB_OK) {
						D_ERROR("%s/BCMCoreAllocateSurface: error creating layer %u buffer %d\n",
								 BCMCoreModuleName,
								 bcm_layer->layer_id,
								 i );
						ret = DFB_FAILURE;
						goto quit;
					}
					else{
						D_DEBUG("%s/BCMCoreAllocateSurface: Allocate surface=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, surface, i, bcm_layer->surface_buffers[i]);
					}

					if ((err = bdvd_graphics_surface_clear(dfb_bcmcore_context->driver_compositor, ((bdvd_graphics_surface_h*)bcm_layer->surface_buffers[i]->video.ctx)[0])) != b_ok) {
						D_ERROR("%s/BCMCoreAllocateSurface: error calling bdvd_graphics_surface_clear %d\n",
								 BCMCoreModuleName,
								 err );
						ret = DFB_FAILURE;
						goto quit;
					}
				}
				else{
					D_DEBUG("%s/BCMCoreAllocateSurface: No need to allocate surface %d.  It already exists. bcm_layer->surface_buffers[%d]=0x%08x\n",
						BCMCoreModuleName,
						i,i, bcm_layer->surface_buffers[i]);
				}
			}
        }
        break;
        case BCMCORELAYER_MIXER_FLIPTOGGLE:
            D_ASSERT(bcm_layer->nb_surfaces < 3);
        case BCMCORELAYER_BACKGROUND:
            D_ASSERT(bcm_layer->nb_surfaces < 4);
        case BCMCORELAYER_FAA:

            D_DEBUG("%s/BCMCoreAllocateSurface: bcm_layer->bcmcore_type=%d\n",
                BCMCoreModuleName,
                bcm_layer->bcmcore_type);

            if (*ret_surface == NULL) {
                /* Deallocation is done in BCMGfxDeallocate, but we need to clear the surface handles */
                memset(bcm_layer->surface_buffers, 0, sizeof(bcm_layer->surface_buffers));
            }
            /* Already done for the mixer ouput layers, also those surfaces on the primary layer
             * cannot be deallocated or reallocated. Also the reduce memory fragmentation,
             * we should really verify the need to reallocate the surfaces!!!!. TODO Really only
             * changing the pixel format should require reallocation, even the back buffer should
             * always be the same size but clipping adjusted accordingly.
             */
            D_DEBUG("%s/BCMCoreAllocateSurface: Allocate %d surfaces!\n",
                BCMCoreModuleName,
                bcm_layer->nb_surfaces);

            for (i = 0; i < bcm_layer->nb_surfaces; i++) {
                if (bcm_layer->surface_buffers[i] == NULL) {
                    /* The front surface is always the size of the display */
                    CoreSurface * surface = (i == bcm_layer->nb_surfaces-1) ? &bcm_layer->frontsurface_clone : &bcm_layer->backsurface_clone;
                    if (dfb_surface_allocate_buffer( surface,
                                                     surface_policy,
                                                     surface->format,
                                                     &bcm_layer->surface_buffers[i] ) != DFB_OK) {
                        D_ERROR("%s/BCMCoreAllocateSurface: error creating layer %u buffer %d\n",
                                 BCMCoreModuleName,
                                 bcm_layer->layer_id,
                                 i );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                    else{
                        D_DEBUG("%s/BCMCoreAllocateSurface: Allocate surface=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, surface, i, bcm_layer->surface_buffers[i]);
                    }

                    if ((err = bdvd_graphics_surface_clear(dfb_bcmcore_context->driver_compositor, ((bdvd_graphics_surface_h*)bcm_layer->surface_buffers[i]->video.ctx)[0])) != b_ok) {
                        D_ERROR("%s/BCMCoreAllocateSurface: error calling bdvd_graphics_surface_clear %d\n",
                                 BCMCoreModuleName,
                                 err );
                        ret = DFB_FAILURE;
                        goto quit;
                    }
                }
                else{
                    D_DEBUG("%s/BCMCoreAllocateSurface: No need to allocate surface %d.  It already exists. bcm_layer->surface_buffers[%d]=0x%08x\n",
                        BCMCoreModuleName,
                        i,i, bcm_layer->surface_buffers[i]);
                }
            }
        break;
        default:
            D_ERROR("%s/BCMCoreAllocateSurface: invalid BCMCoreLayerType\n",
                     BCMCoreModuleName );
            ret = DFB_UNSUPPORTED;
            goto quit;
    }

    /* This is if the *ret_surface is not NULL, meaning this
     * function was called from BCMCoreReallocateSurface
     */
    if (*ret_surface != NULL) {
        uint32_t nb_surfaces;

        D_DEBUG("%s/BCMCoreAllocateSurface: reconfig and reformat of surface %p\n",
                 BCMCoreModuleName,
                 *ret_surface );

        surface = *ret_surface;

        surface->caps = caps;

        if ((bcm_layer->bcmcore_type == BCMCORELAYER_FAA)) {

            /* We've released all buffers in Deallocate, and we need to assign at least the front buffer now */
            if ((bcm_layer->dfb_core_surface->front_buffer = bcm_layer->surface_buffers[bcm_layer->front_surface_index]) == NULL) {
                D_ERROR("%s/BCMCoreAllocateSurface: surface buffer is NULL\n",
                        BCMCoreModuleName);
                ret = DFB_FAILURE;
                goto quit;
            }

            nb_surfaces = 1;
        }
        else {
            nb_surfaces = bcm_layer->nb_surfaces;
        }

#if 0
			printf("Index f %d (%p) b %d (%p) i %d (%p)\n",
			bcm_layer->front_surface_index,
			bcm_layer->surface_buffers[bcm_layer->front_surface_index],
			bcm_layer->back_surface_index,
			bcm_layer->surface_buffers[bcm_layer->back_surface_index],
			bcm_layer->idle_surface_index,
			bcm_layer->surface_buffers[bcm_layer->idle_surface_index]
			);
#endif

        /* Maybe we should let the gfxdrv memory allocator do this.... */
        /* Will clean this up in a later version */
        if ((ret = dfb_surface_reconfig_preallocated_video(
                    surface,
                    bcm_layer->surface_buffers[bcm_layer->front_surface_index],
                    (nb_surfaces > 1) ? bcm_layer->surface_buffers[bcm_layer->back_surface_index] : bcm_layer->surface_buffers[bcm_layer->front_surface_index],
                    (nb_surfaces > 2) ? bcm_layer->surface_buffers[bcm_layer->idle_surface_index] : bcm_layer->surface_buffers[bcm_layer->front_surface_index])) != DFB_OK) {
            goto quit;
        }

        if ((ret = dfb_surface_reformat_preallocated_video(
                    dfb_bcmcore_context->core,
                    surface,
                    config->width,
                    config->height,
                    config->format )) != DFB_OK) {
            goto quit;
        }
    }
    else {
        /* Is this true if you flip? */
        /* For the framebuffer surface, we need to find the uncached address
         * (the mmmap heap is both opened with and without O_SYNC, we want with)
         * This will avoid the tearing effect. For secondary surfaces this is not
         * necessary if we call msync before calling for a blit RPC.
         */

        D_DEBUG("%s/BCMCoreAllocateSurface: creating new surface\n",
                 BCMCoreModuleName);

        /* No palette */
        if ((ret = dfb_surface_create_preallocated_video(
                    /* CoreDFB *core */
                    dfb_bcmcore_context->core,
                    /* int width */
                    config->width,
                    /* int height */
                    config->height,
                    /* DFBSurfacePixelFormat format */
                    config->format,
                    /* DFBSurfaceCapabilities caps */
                    caps,
                    /* CorePalette *palette */
                    NULL,
                    bcm_layer->surface_buffers[bcm_layer->front_surface_index],
                    (bcm_layer->nb_surfaces > 1) ? bcm_layer->surface_buffers[bcm_layer->back_surface_index] : NULL,
                    (bcm_layer->nb_surfaces > 2) ? bcm_layer->surface_buffers[bcm_layer->idle_surface_index] : NULL,
                    /* CoreSurface **surface */
                    &surface)) != DFB_OK) {
            goto quit;
        }

        if (bcm_layer->bcmcore_type == BCMCORELAYER_MIXER_FLIPBLIT) {
            bcm_layer->surface_buffers[bcm_layer->front_surface_index]->flags |= SBF_HIDDEN;
        }
    }

    bcm_layer->dfb_core_surface = surface;

    bcm_layer->frontsurface_clone.object = bcm_layer->backsurface_clone.object = bcm_layer->dfb_core_surface->object;
    bcm_layer->frontsurface_clone.serial = bcm_layer->backsurface_clone.serial = bcm_layer->dfb_core_surface->serial;

    bcm_layer->back_rect.x = 0;
    bcm_layer->back_rect.y = 0;
    bcm_layer->back_rect.w = bcm_layer->dfb_core_surface->width;
    bcm_layer->back_rect.h = bcm_layer->dfb_core_surface->height;

    D_DEBUG("%s/BCMCoreAllocateSurface: %d %d\n",
             BCMCoreModuleName,
             bcm_layer->dfb_core_surface->width,
             bcm_layer->dfb_core_surface->height);

    clip_rect.x = clip_rect.y = 0;
    if ((bcm_layer->bcmcore_type == BCMCORELAYER_FAA) || (config->buffermode == DLBM_FRONTONLY)) {
        /* FAA layers are scaled on the output layer surface.
         * So taking max for now.
         */
        if (bdvd_graphics_window_get_video_format_dimensions(
                bdvd_video_format_1080i,
                dfb_surface_pixelformat_to_bdvd(DSPF_ARGB),
                (unsigned int *)&clip_rect.w, (unsigned int *)&clip_rect.h) != b_ok) {
            D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                    BCMCoreModuleName);
            ret = DFB_FAILURE;
            goto quit;
        }
    }
    else {
        clip_rect.w = bcm_layer->frontsurface_clone.width;
        clip_rect.h = bcm_layer->frontsurface_clone.height;
    }

    if (!dfb_rectangle_intersect( &bcm_layer->front_rect,
                                  &clip_rect )) {
        D_ERROR("%s/BCMCoreAllocateSurface: invalid front rectangle\n",
             BCMCoreModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    D_DEBUG("%s/BCMCoreAllocateSurface: front rect is %d %d\n",
             BCMCoreModuleName,
             bcm_layer->front_rect.w,
             bcm_layer->front_rect.h);

    switch (bcm_layer->dfb_ext_type) {
        case bdvd_dfb_ext_bd_hdmv_presentation_layer:
            surface->caps &= ~DSCAPS_BCC;
            surface->caps |= DSCAPS_COPY_BLIT_PG;
            break;

        case bdvd_dfb_ext_bd_hdmv_interactive_layer:
            surface->caps &= ~DSCAPS_BCC;
            surface->caps |= DSCAPS_COPY_BLIT_IG;
            break;

        default:
            break;
    }

    *ret_surface = surface;

quit:

    D_DEBUG("%s/BCMCoreAllocateSurface exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}


static DFBResult
BCMCoreAllocateSurface_wrapper( CoreLayer              *layer,
                                void                   *driver_data,
                                void                   *layer_data,
                                void                   *region_data,
                                CoreLayerRegionConfig  *config,
                                CoreSurface           **ret_surface ) {

    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    /* Find the bcm_layer handle to work with it. Use the layer parameter. */
    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if ((ret = BCMCoreAllocateSurface(layer_ctx->mixer, bcm_layer, config, ret_surface)) != DFB_OK) {
        goto quit;
    }

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

/* TODO It's not safe that BCMCoreDeallocateSurface get's called
 * while the layer is still registered on a mixer. Only
 * when invoked from Reallocate it is safe. Must check if
 * this ever pose a problem.
 */
static inline DFBResult
BCMCoreDeallocateSurface( BCMCoreLayerHandle     bcm_layer,
                          CoreLayerRegionConfig  *config,
                          CoreSurface            *surface ) {

    DFBResult ret = DFB_OK;

    uint32_t i;

    uint32_t offset = 0;
    size_t nb_surfaces = 0;

    D_ASSERT(bcm_layer != NULL);
    D_ASSERT(surface != NULL);

    D_DEBUG("%s/BCMCoreDeallocateSurface called with:\n"
        "layer_id: %u\n"
        "bcm_layer->bcmcore_type=%d\n",
        BCMCoreModuleName,
        bcm_layer->layer_id,
        bcm_layer->bcmcore_type);

    D_ASSERT(bcm_layer->dfb_core_surface == surface);

    /* We are reallocating */
    if (config) {
        switch (bcm_layer->bcmcore_type) {
            case BCMCORELAYER_OUTPUT:
            case BCMCORELAYER_MIXER_FLIPTOGGLE:
                /* We never reallocate the mixer output surfaces, as they are the
                 * maximum size already and pixel format should always be ARGB,
                 * and this should never be allowed to change.
                 * This is to avoid fragmentation of the video memory.
                 * Only if the buffermode is different, then we deallocate.
                 */
                switch (config->buffermode) {
                    case DLBM_FRONTONLY:
                        nb_surfaces = 3;
                        offset = 1;
                    break;
                    case DLBM_BACKVIDEO:
                        nb_surfaces = 3;
                        offset = 2;
                    break;
                    case DLBM_TRIPLE:
                    default:
                        nb_surfaces = 0;
                    break;
                }

                nb_surfaces = bcm_layer->nb_surfaces < nb_surfaces ? bcm_layer->nb_surfaces : nb_surfaces;
            break;
            case BCMCORELAYER_MIXER_FLIPBLIT:
                /* If double buffered, we don't reallocate the mixed layer front
                 * surface, because it should never change.
                 * This is to avoid fragmentation of the video memory.
                 */
                nb_surfaces = bcm_layer->nb_surfaces == 2 ? bcm_layer->nb_surfaces-1 : bcm_layer->nb_surfaces;

                if (config->buffermode == DLBM_FRONTONLY)
                {
					D_DEBUG("\n\n\nTwo surface to release\n\n\n");
                    nb_surfaces = 2;
				}
            break;
            case BCMCORELAYER_FAA:
                nb_surfaces = bcm_layer->nb_surfaces;
            break;
            case BCMCORELAYER_BACKGROUND:
                nb_surfaces = bcm_layer->nb_surfaces;
            break;
            default:
                D_ERROR("%s/BCMCoreDeallocateSurface: unknown BCMCoreLayerType\n",
                        BCMCoreModuleName );
                ret = DFB_FAILURE;
                goto quit;
        }
    }
    else {
        D_ERROR("%s/BCMCoreDeallocateSurface: deallocation is not implemented and should not be required\n",
                BCMCoreModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

    D_DEBUG("%s/BCMCoreDeallocateSurface: Need to deallocate surfaces from %d to %d\n",
        BCMCoreModuleName, offset, nb_surfaces);
    for (i = offset; i < nb_surfaces; i++) {
        D_ASSERT(bcm_layer->surface_buffers[i] != NULL);
        if (i == bcm_layer->front_surface_index) {
            D_ASSERT(surface->front_buffer == bcm_layer->surface_buffers[i]);
            surface->front_buffer = NULL;
        }
        if (i == bcm_layer->back_surface_index) {
            D_ASSERT(surface->back_buffer == bcm_layer->surface_buffers[i]);
            surface->back_buffer = NULL;
        }
        if (i == bcm_layer->idle_surface_index) {
            D_ASSERT(surface->idle_buffer == bcm_layer->surface_buffers[i]);
            surface->idle_buffer = NULL;
        }

        D_DEBUG("%s/BCMCoreDeallocateSurface: Deallocate &backsurface_clone=0x%08x, i=%d surface_buffers[i]=0x%08x\n", BCMCoreModuleName, &bcm_layer->backsurface_clone, i, bcm_layer->surface_buffers[i]);
        if ((bcm_layer) && (bcm_layer->surface_buffers[i]))
        	dfb_surface_destroy_buffer( &bcm_layer->backsurface_clone, bcm_layer->surface_buffers[i] );
        bcm_layer->surface_buffers[i] = NULL;
    }

    /* Shouldn't we release also something in CoreSurface ? */

quit:

    D_DEBUG("%s/BCMCoreDeallocateSurface exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

static DFBResult
BCMCoreReallocateSurface( CoreLayer             *layer,
                          void                  *driver_data,
                          void                  *layer_data,
                          void                  *region_data,
                          CoreLayerRegionConfig *config,
                          CoreSurface           *surface ) {

    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

#if D_DEBUG_ENABLED
     FormatString format_string;

     D_ASSERT(layer != NULL);
     D_ASSERT(config != NULL);

     format_string.format = config->format;
#endif // D_DEBUG_ENABLED

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);
    UNUSED_PARAM(surface);

    D_DEBUG("%s/BCMCoreReallocateSurface called with:\n"
        "dfb_layer_id: %u\n"
        "CoreLayerRegionConfig width: %d\n"
        "CoreLayerRegionConfig height: %d\n"
        "CoreLayerRegionConfig format: %s\n"
        "CoreLayerRegionConfig surface_caps: 0x%08x\n"
        "CoreLayerRegionConfig buffermode: 0x%08x\n",
        BCMCoreModuleName,
        dfb_layer_id(layer),
        config->width,
        config->height,
        ((const FormatString *)bsearch( &format_string, format_strings,
                               NUM_FORMAT_STRINGS, sizeof(FormatString),
                               pixelformat_compare ))->string,
        config->surface_caps,
        config->buffermode);

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    /* Find the bcm_layer handle to work with it. Use the layer parameter. */
    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    /* The mixer is tied to the primary layer */
    if (bcm_layer->layer_id == DLID_PRIMARY) {
        /* This is necessary so we don't deadlock with the mixer thread */
        if ((ret = BCMMixerLayerRelease(bcm_layer)) != DFB_OK) {
            bcm_layer = NULL;
            goto quit;
        }

        bcm_layer = NULL;

        /* The mixer thread must be stopped, because the layer's surfaces are going to
         * change. The mixer thread will be reenabled in the BCMCoreSetRegion */
        if ((ret = BCMMixerStop(layer_ctx->mixer)) != DFB_OK) {
            goto quit;
        }

        if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
            goto quit;
        }
    }

    D_ASSERT(bcm_layer->dfb_core_surface == surface);

    /* TODO we should really verify the need to reallocate the surfaces!!!!. TODO Really only
     * changing the pixel format should require reallocation, even the back buffer should
     * always be the same size but clipping adjusted accordingly.
     * BCMCoreSetRegion or BCMCoreTestRegion should have been called before, giving the updates.
     */
    if ((ret = BCMCoreDeallocateSurface(
                bcm_layer,
                config,
                surface )) != DFB_OK) {
        goto quit;
    }

    if ((ret = BCMCoreAllocateSurface(
                layer_ctx->mixer,
                bcm_layer,
                config,
                &surface )) != DFB_OK) {
        goto quit;
    }

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMCoreReallocateSurface exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

static DFBResult
BCMCoreDeallocateSurface_wrapper( CoreLayer              *layer,
                                  void                   *driver_data,
                                  void                   *layer_data,
                                  void                   *region_data,
                                  CoreSurface            *surface ) {

    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);
    UNUSED_PARAM(surface);

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    /* Find the bcm_layer handle to work with it. Use the layer parameter. */
    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if ((ret = BCMCoreDeallocateSurface(bcm_layer, NULL, surface)) != DFB_OK) {
        goto quit;
    }

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    return ret;
}

static DFBResult
BCMCoreAddRegion( CoreLayer                  *layer,
                  void                       *driver_data,
                  void                       *layer_data,
                  void                       *region_data,
                  CoreLayerRegionConfig      *config )
{
    /* Should register layer with the mixer here!!! */
    return DFB_OK;
}

static DFBResult
BCMCoreSetRegion( CoreLayer                  *layer,
                  void                       *driver_data,
                  void                       *layer_data,
                  void                       *region_data,
                  CoreLayerRegionConfig      *config,
                  CoreLayerRegionConfigFlags  updated,
                  CoreSurface                *surface,
                  CorePalette                *palette )
{
    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle bcm_layer = NULL;

    bdvd_video_format_e video_format;

    bool do_set_display_dimensions = false;

    bool implicit_flip = false;

    CoreLayerContext * layer_context = NULL;

#if D_DEBUG_ENABLED
    FormatString format_string;

    format_string.format = config->format;
#endif // D_DEBUG_ENABLED

    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);

    D_DEBUG("%s/BCMCoreSetRegion called with:\n"
            "dfb_layer_id: %u\n"
            "updated: 0x%08x\n"
            "CoreLayerRegionConfig width: %d\n"
            "CoreLayerRegionConfig height: %d\n"
            "CoreLayerRegionConfig format: %s\n"
            "CoreLayerRegionConfig surface_caps: 0x%08x\n"
            "CoreLayerRegionConfig buffermode: 0x%08x\n"
            "CoreSurface ? %s\n"
            "CorePalette ? %s\n",
            BCMCoreModuleName,
            dfb_layer_id(layer),
            updated,
            config->width,
            config->height,
            ((const FormatString *)bsearch( &format_string, format_strings,
                                   NUM_FORMAT_STRINGS, sizeof(FormatString),
                                   pixelformat_compare ))->string,
            config->surface_caps,
            config->buffermode,
            surface ? "yes" : "no",
            palette ? "yes" : "no");

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    /* Find the bcm_layer handle to work with it. Use the layer parameter. */
    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    if ((ret = dfb_layer_get_primary_context(layer, false, &layer_context)) != DFB_OK) {
        goto quit;
    }

    D_ASSERT(bcm_layer->dfb_core_surface == surface);

    if ((updated & (CLRCF_SURFACE)) && (bcm_layer->dfb_core_surface != surface)) {
        D_ERROR("%s/BCMCoreSetRegion: impossible to update surface\n",
                BCMCoreModuleName);
        ret = DFB_INVARG;
        goto quit;
    }

    if (bcm_layer->layer_id == DLID_PRIMARY) {
        /* This happens when SetVideoMode is called, I'm not sure it is when SetConfiguration is.
         * This will change the active region of the layers front buffers. Eventually this should be
         * done on all output layers, when there will be multiple feeders.
         */
        if (updated & (CLRCF_WIDTH | CLRCF_HEIGHT)) {
            if (((updated & CLRCF_WIDTH) && !(updated & CLRCF_HEIGHT)) ||
                 (!(updated & CLRCF_WIDTH) && (updated & CLRCF_HEIGHT))) {
                D_ERROR("%s/BCMCoreSetRegion: cannot set width and height appart\n",
                        BCMCoreModuleName);
                ret = DFB_INVARG;
                goto quit;
            }

            video_format = display_get_video_format(dfb_bcmcore_context->primary_display_handle);
            switch (video_format) {
                case bdvd_video_format_ntsc:
                case bdvd_video_format_ntsc_japan:
                case bdvd_video_format_pal_m:
                case bdvd_video_format_480p:
                    if (config->width == 720 && config->height == 480) {
                        do_set_display_dimensions = true;
                    }
                    else {
                        D_ERROR("%s/BCMCoreSetRegion: invalid width or height for display video format\n",
                                BCMCoreModuleName);
                        D_ERROR("%s/BCMCoreSetRegion: width: %d height: %d\n", BCMCoreModuleName, config->width, config->height);
                        ret = DFB_INVARG;
                        goto quit;
                    }
                break;
                case bdvd_video_format_pal:
                case bdvd_video_format_pal_n:
                case bdvd_video_format_pal_nc:
                case bdvd_video_format_576p:
                    if (config->width == 720 && config->height == 576) {
                        do_set_display_dimensions = true;
                    }
                    else {
                        D_ERROR("%s/BCMCoreSetRegion: invalid width or height for display video format\n",
                                BCMCoreModuleName);
                        ret = DFB_INVARG;
                        goto quit;
                    }
                break;
                case bdvd_video_format_720p:
                case bdvd_video_format_720p_50hz:
                    if ((config->width == 1280 || config->width == 720 || config->width == 640) && config->height == 720) {
                        do_set_display_dimensions = true;
                    }
                    else {
                        D_ERROR("%s/BCMCoreSetRegion: invalid width or height for display video format\n",
                                BCMCoreModuleName);
                        ret = DFB_INVARG;
                        goto quit;
                    }
                break;
                case bdvd_video_format_1080i:
                case bdvd_video_format_1080i_50hz:
                case bdvd_video_format_1080p:
                case bdvd_video_format_1080p_24hz:
                case bdvd_video_format_1080p_30hz:
                case bdvd_video_format_1080p_50hz:
                case bdvd_video_format_1080p_25hz:
                    if ((config->width == 1920 || config->width == 960) && config->height == 1080) {
                        do_set_display_dimensions = true;
                    }
                    else {
                        D_ERROR("%s/BCMCoreSetRegion: invalid width or height for display video format\n",
                                BCMCoreModuleName);
                        ret = DFB_INVARG;
                        goto quit;
                    }
                break;
                default:
                    D_ERROR("%s/BCMCoreSetRegion: display is currently set to an invalid video format\n",
                            BCMCoreModuleName);
                    ret = DFB_INVARG;
                    goto quit;
                break;
            }

            if (do_set_display_dimensions) {
                bdvd_graphics_window_settings_t graphics_settings;

                /* We must make sure the thread is dead by now, because the bdvd_display_set
                 * called before SetVideoMode must have disabled the graphics and
                 * there is no more flip callback. We will restart once the graphics
                 * is reenabled.
                 */

                /* This is necessary so we don't deadlock with the mixer thread */
                if ((ret = BCMMixerLayerRelease(bcm_layer)) != DFB_OK) {
                    goto quit;
                }

                /* Clear handle */
                bcm_layer = NULL;

                /* The mixer thread must be stopped, because the layer's surfaces are going to
                 * change. The mixer thread will be reenabled by the
                 * BCMMixerSetDisplayDimensions call
                 */
                if ((ret = BCMMixerStop(layer_ctx->mixer)) != DFB_OK) {
                    goto quit;
                }

                if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
                    goto quit;
                }

                    bdvd_graphics_window_get(bcm_layer->graphics_window->window_handle, &graphics_settings);
                    if (!graphics_settings.enabled || (dfb_bcmcore_context->video_format != video_format)) {
                        if ((ret = BCMMixerSetDisplayDimensions(layer_ctx->mixer,
                                                                bcm_layer,
                                                                true)) != DFB_OK) {
                            goto quit;
                        }
                    }

                    /* TODO Need to lock this layer */
                    if (dfb_bcmcore_context->secondary_graphics_window.layer_assigned) {
                        bdvd_graphics_window_get(((BCMCoreLayerHandle)dfb_bcmcore_context->secondary_graphics_window.layer_assigned)->graphics_window->window_handle, &graphics_settings);
                        if (!graphics_settings.enabled || (dfb_bcmcore_context->video_format != video_format)) {
                            if ((ret = BCMMixerSetDisplayDimensions(layer_ctx->mixer,
                                                                    dfb_bcmcore_context->secondary_graphics_window.layer_assigned,
                                                                    false)) != DFB_OK) {
                                goto quit;
                            }
                            ((BCMCoreLayerHandle)dfb_bcmcore_context->secondary_graphics_window.layer_assigned)->back_rect = bcm_layer->back_rect;
                        }
                    }

                dfb_bcmcore_context->video_format = video_format;

                /* We need to trigger an implicit refresh of the display when changing the resolution.
                 */
                implicit_flip = true;
            }
        }

        if ((updated & CLRCF_SOURCE)) {
            D_DEBUG("%s/BCMCoreSetRegion: setting source rect x %d y %d w %d h %d\n",
                    BCMCoreModuleName,
                    config->source.x, config->source.y, config->source.w, config->source.h);
        }

        if ((updated & CLRCF_DEST)) {
            D_DEBUG("%s/BCMCoreSetRegion: setting destination rect x %d y %d w %d h %d\n",
                    BCMCoreModuleName,
                    config->dest.x, config->dest.y, config->dest.w, config->dest.h);
        }

/* Those need to be fixed */
#if 0
        /* Not supported for now, we also need to set the graphics feeder
         * accordingly.
         */
        /* Do we want to set the premult cap of the gfxfeeder here?
         * CLRCF_SURFACE_CAPS
         */
        if ((updated & CLRCF_SURFACE_CAPS)) {
            if (config->surface_caps & DSCAPS_PREMULTIPLIED) {
                bcm_layer->premultiplied = true;
            }
            else {
                bcm_layer->premultiplied = false;
            }
        }

        /* This has already been done in the Reallocate fct.. */
        if (updated & (CLRCF_BUFFERMODE)) {
            D_ERROR("%s/BCMCoreSetRegion: impossible to update config\n",
                    BCMCoreModuleName);
            ret = DFB_INVARG;
            goto quit;
        }
#endif
    }
    else if (bcm_layer->bcmcore_type == BCMCORELAYER_OUTPUT) {
        if (bcm_layer->dfb_ext_type == bdvd_dfb_ext_osd_secondary_layer) {
            /* This is just to adjust the graphics feeder params */
            if (bdvd_graphics_window_set_video_format_dimensions(bcm_layer->graphics_window->window_handle, bcm_layer->graphics_window->display_handle) != b_ok) {
                D_ERROR("%s/BCMCoreSetRegion: failure in bdvd_graphics_window_set_video_format_dimensions\n",
                        BCMCoreModuleName);
                ret = DFB_FAILURE;
                goto quit;
            }
        }
    }
    else if (bcm_layer->bcmcore_type == BCMCORELAYER_BACKGROUND) {
        /* TODO */
    }
    else {
        /* TODO, apparently for those, TestRegion and Reallocate are called instead,
         * so I have to see if necessary to implement.
         */
        /* CLRCF_WIDTH | CLRCF_HEIGHT | CLRCF_FORMAT are supported, changes
         * would have been done in the Reallocate fct.
         * CLRCF_SURFACE_CAPS might be usefull for the premultiplied flag.
         */
        if ((updated & CLRCF_SURFACE_CAPS)) {
            if (config->surface_caps & DSCAPS_PREMULTIPLIED) {
                D_DEBUG("%s/BCMCoreSetRegion: layer %u surface_caps DSCAPS_PREMULTIPLIED is set\n",
                        BCMCoreModuleName,
                        dfb_layer_id(layer));
                bcm_layer->premultiplied = true;
                bcm_layer->dfb_core_surface->caps |= DSCAPS_PREMULTIPLIED;
            }
            else {
                D_DEBUG("%s/BCMCoreSetRegion: layer %u surface_caps DSCAPS_PREMULTIPLIED is not set\n",
                        BCMCoreModuleName,
                        dfb_layer_id(layer));
                bcm_layer->premultiplied = false;
                bcm_layer->dfb_core_surface->caps &= ~(DSCAPS_PREMULTIPLIED);
            }
        }

        if (config->surface_caps & DSCAPS_OPTI_BLIT)
        {
            bcm_layer->dfb_core_surface->caps |= DSCAPS_OPTI_BLIT;
        }
        else
        {
            bcm_layer->dfb_core_surface->caps &= ~(DSCAPS_OPTI_BLIT);
        }

        {
            const char * force_alphapremult_string = getenv("dfbbcm_force_alphapremult_layerid");
            uint32_t force_alphapremult_id;
            if (force_alphapremult_string) {
                force_alphapremult_id = strtoul(force_alphapremult_string, NULL, 10);
                if (layer_ctx->mixed_layer_id == force_alphapremult_id) {
                    D_DEBUG("%s/BCMCoreSetRegion: forcing layer %u surface_caps DSCAPS_PREMULTIPLIED to be set\n",
                           BCMCoreModuleName,
                           force_alphapremult_id);
                    bcm_layer->premultiplied = true;
                    bcm_layer->dfb_core_surface->caps |= DSCAPS_PREMULTIPLIED;
                }
            }
        }

        if ((updated & CLRCF_SOURCE)) {
            D_DEBUG("%s/BCMCoreSetRegion: setting source rect x %d y %d w %d h %d\n",
                    BCMCoreModuleName,
                    config->source.x, config->source.y, config->source.w, config->source.h);

            /* this updates the width and height of the surface, so disabling for now */
            /* Should maybe clip??? */
            bcm_layer->back_rect = config->source;
        }

        if ((updated & CLRCF_DEST)) {
            uint32_t width;
            uint32_t height;

            D_DEBUG("%s/BCMCoreSetRegion: setting destination rect x %d y %d w %d h %d\n",
                    BCMCoreModuleName,
                    config->dest.x, config->dest.y, config->dest.w, config->dest.h);

            D_DEBUG("%s/BCMCoreSetRegion: setting location x %f y %f w %f h %f\n",
                    BCMCoreModuleName,
                    layer_context->screen.location.x, layer_context->screen.location.y, layer_context->screen.location.w, layer_context->screen.location.h);

            /* Front rectangle will be updated later */
            if (bdvd_graphics_window_get_video_format_dimensions(
                    display_get_video_format(dfb_bcmcore_context->primary_display_handle),
                    dfb_surface_pixelformat_to_bdvd(bcm_layer->frontsurface_clone.format),
                    &width, &height) != b_ok) {
                    D_ERROR("%s/BCMCoreSetRegion: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                            BCMCoreModuleName);
                    ret = DFB_FAILURE;
                    goto quit;
                }

            /* This is necessary so we don't deadlock with the mixer thread */
            if ((ret = BCMMixerLayerRelease(bcm_layer)) != DFB_OK) {
                goto quit;
            }

            /* Clear handle */
            bcm_layer = NULL;

            if ((ret = BCMMixerStop(layer_ctx->mixer)) != DFB_OK) {
                goto quit;
            }

            if ((ret = BCMMixerSetLayerLocation(layer_ctx->mixer,
                                                layer_ctx->mixed_layer_id,
                                                &layer_context->screen.location,
                                                &config->dest,
                                                width, height, false)) != DFB_OK) {
                goto quit;
            }

            if ((ret = BCMMixerStart(layer_ctx->mixer)) != DFB_OK) {
                goto quit;
            }

            if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
                goto quit;
            }

            switch (bcm_layer->bcmcore_type) {
                case BCMCORELAYER_MIXER_FLIPBLIT:
                /* We need to trigger an implicit refresh of the front surface. If not,
                 * the current front surface content could be mixed, and we don't want
                 * that.
                 */
                implicit_flip = true;
                break;
                case BCMCORELAYER_FAA:
                    /* Force a update of the FAA a next mix (if visible) */
                    bcm_layer->dirty_region = DFB_REGION_INIT_FROM_RECTANGLE(&bcm_layer->front_rect);
                break;
                default:
                    D_ERROR("%s/BCMCoreSetRegion: unknown bcmcore_type\n",
                            BCMCoreModuleName);
                    ret = DFB_FAILURE;
                    goto quit;
            }
        }

#if 0 /* This has already been dealt with in the Reallocate fct.. */
        if (updated & (CLRCF_BUFFERMODE)) {
            D_ERROR("%s/BCMCoreSetRegion: impossible to update config\n",
                    BCMCoreModuleName);
            ret = DFB_INVARG;
            goto quit;
        }
#endif
    }

    if ((updated & CLRCF_FORMAT)) {
        if (config->format == DSPF_RGB32)
            config->format = DSPF_ARGB;
    }

    if ((updated & CLRCF_PALETTE) && palette) {
        if ((ret = BCMCore_load_palette( bcm_layer->dfb_core_surface, palette )) != DFB_OK) {
            goto quit;
        }
    }

quit:

    if (layer_context) dfb_layer_context_unref( layer_context );

    if (bcm_layer) {
        /* unlock the output layer if still locked */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;

        if (ret != DFB_OK) {
            implicit_flip = false;
        }
        /* fall through */
    }

    if (implicit_flip) {
        if ((ret = BCMCoreFlipRegion(
                    layer,
                    driver_data,
                    layer_data,
                    region_data,
                    surface,
                    0)) != DFB_OK) {
            /* fall through */
        }
    }

    D_DEBUG("%s/BCMCoreSetRegion exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret );

    return ret;
}

static DFBResult
BCMCoreRemoveRegion( CoreLayer                  *layer,
                     void                       *driver_data,
                     void                       *layer_data,
                     void                       *region_data )
{
    DFBResult ret = DFB_OK;

    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer * layer_ctx = NULL;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(layer_data);
    UNUSED_PARAM(region_data);

    D_DEBUG("%s/BCMCoreRemoveRegion called with:\n"
            "dfb_layer_id: %u\n",
            BCMCoreModuleName,
            dfb_layer_id(layer));

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[dfb_layer_id(layer)];

    /* Remove layer from mixing if not already done */
    if ((ret = BCMMixerLayerSetLevel(layer_ctx->mixer,
                                     layer_ctx->mixed_layer_id, -1)) != DFB_OK) {
        goto quit;
    }

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    bcm_layer->surface_buffers[bcm_layer->front_surface_index] = NULL;
    bcm_layer->surface_buffers[bcm_layer->back_surface_index] = NULL;
    bcm_layer->surface_buffers[bcm_layer->idle_surface_index] = NULL;

quit:

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

    D_DEBUG("%s/BCMCoreRemoveRegion exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

/** @} */

/** @addtogroup ScreenInterface
 *  The following is a grouping of static functions implementing
 *  the <src/core/screens.h> interface, refer to this file for function
 *  descriptions
 *  @{
 */

/**
 *  This function initialise the DFBScreenDescription.
 *  <pre>
 *  Set the screen capabilities to DSCCAPS_NONE, but there could
 *  be capabilities useable to expose some of the bsettop
 *  functionnality such as:
 *  DSCCAPS_VSYNC -> Synchronization with the vertical retrace
 *  DSCCAPS_POWER_MANAGEMENT -> Power management if supported
 *  DSCCAPS_MIXERS -> Has mixers (composition)
 *  DSCCAPS_ENCODERS -> Has encoders
 *  DSCCAPS_OUTPUTS -> Has outputs
 *  </pre>
 *  @return
 *      DFBResult DFB_OK on success, error code otherwise.
 */
static DFBResult
BCMCoreInitScreen( CoreScreen           *screen,
                   GraphicsDevice       *device,
                   void                 *driver_data,
                   void                 *screen_data,
                   DFBScreenDescription *description )
{
    description->caps = DSCCAPS_NONE;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(screen_data);

    /* Set the screen name. */
    snprintf( description->name,
              DFB_SCREEN_DESC_NAME_LENGTH, "%s Primary Screen", BCMCoreModuleName );

    return DFB_OK;
}

/**
 *  This function obtains the screen (bdvd_display_h) size.
 *  @par ret_width
 *       int * returning width
 *  @par ret_height
 *       int * returning height
 *  @return
 *      DFBResult DFB_OK on success, error code otherwise.
 */
static DFBResult
BCMCoreGetScreenSize( CoreScreen *screen,
                      void       *driver_data,
                      void       *screen_data,
                      int        *ret_width,
                      int        *ret_height )
{
    DFBResult ret = DFB_OK;

    UNUSED_PARAM(driver_data);
    UNUSED_PARAM(screen_data);

    D_ASSERT( dfb_bcmcore_context != NULL );

// This causes a compilation error -- please fix.
//    D_ASSERT( dfb_bcmcore_context->primary_graphics_window.handle != NULL );

    D_ASSERT( dfb_bcmcore_context->primary_display_handle != NULL );

    D_DEBUG("%s/BCMCoreGetScreenSize called\n",
             BCMCoreModuleName );

    /* We assume that all the gfxfeeder are taking ARGB surfaces as input. */
    if (bdvd_graphics_window_get_video_format_dimensions(
            display_get_video_format(dfb_bcmcore_context->primary_display_handle),
            dfb_surface_pixelformat_to_bdvd(DSPF_ARGB),
            (unsigned int *)ret_width, (unsigned int *)ret_height) != b_ok) {
        D_ERROR("%s/BCMCoreAllocateSurface: failure in bdvd_graphics_window_get_video_format_dimensions\n",
                BCMCoreModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

quit:

    return ret;
}

/** @} */

/** @addtogroup StaticFunctions
 *  The following is a grouping of static functions used
 *  for multi-application support, not fully implemented yet,
 *  but this is how the remote calls would be done with fusion
 *  @{
 */

static DFBResult
BCMCoreExampleHandler( CoreLayerRegionConfig * config )
{
     fusion_skirmish_prevail( &dfb_bcmcore_context->shared->lock );

     fusion_skirmish_dismiss( &dfb_bcmcore_context->shared->lock );

     return DFB_UNIMPLEMENTED;
}

static int
BCMCoreCallHandler( int   caller,
                           int   call_arg,
                           void *call_ptr,
                           void *ctx )
{
     switch (call_arg) {
          case BCMCORE_NOT_SUPPORTED_YET:
               return BCMCoreExampleHandler( call_ptr );
          default:
               D_BUG( "unknown call" );
               break;
     }

     return 0;
}

DFBResult
BCMCoreLayerSetClearrect(DFBDisplayLayerID layer_id,
                         bdvd_dfb_ext_clear_rect_t *clear_rects)
{
    DFBResult ret = DFB_OK;
    BCMCoreContextLayer *layer_ctx = NULL;

    D_DEBUG("%s/BCMCoreLayerSetClearrect called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

    if ((ret = BCMMixerSetClearrect(layer_ctx->mixer, layer_ctx->mixed_layer_id, clear_rects)) != DFB_OK) {
        goto quit;
    }

quit:

    D_DEBUG("%s/BCMCoreLayerSetClearrect exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

#define CONVERTER_BUFFER_SIZE (1920*1088*4)
extern IDirectFB *idirectfb_singleton;
extern SID_SurfaceOutputBufferInfo sid_info;

DFBResult
BCMCoreCreateStaticBuffers()
{
    DFBResult ret = DFB_OK;
    IDirectFBSurfaceManager *surface_manager;

    D_DEBUG("%s/BCMCoreCreateStaticBuffers called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            0 );

	/*PR12701*/
	if (getenv("sid_use_intermediate_buffer"))
	{
    sid_info.buffer_size = CONVERTER_BUFFER_SIZE;
	}
	else
	{
		sid_info.buffer_size = 0;
		sid_info.surface_manager = NULL; /*optional*/
		sid_info.enabled = false;
	}

    /* allocate SID Output Surface Manager */
    if ((sid_info.enabled == true) &&
        (sid_info.surface_manager == NULL))
    {
        if ((ret = idirectfb_singleton->CreateSurfaceManager(
               idirectfb_singleton,
               sid_info.buffer_size,
               0,
               DSMT_SURFACEMANAGER_FIRSTFIT,
               &surface_manager)) != DFB_OK)
        {
            D_ERROR("%s/BCMCoreCreateStaticBuffers failed\n",
            BCMCoreModuleName);
            goto quit;
        }

		/*PR12701*/
		D_DEBUG("\nAllocating 8 MB intermediate buffer for SID\n");

        sid_info.surface_manager = surface_manager;
    }

quit:

    D_DEBUG("%s/BCMCoreCreateStaticBuffers exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult
BCMCoreDestroyStaticBuffers()
{
    DFBResult ret = DFB_OK;
    IDirectFBSurfaceManager *surface_manager = (IDirectFBSurfaceManager *)sid_info.surface_manager;

    D_DEBUG("%s/BCMCoreDestroyStaticBuffers called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            0 );

    if (surface_manager)
    {
        surface_manager->Release(surface_manager);
    }

    D_DEBUG("%s/BCMCoreDestroyStaticBuffers exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult BCMCoreAttachCallback(
    DFBDisplayLayerID layer_id,
    bdvd_dfb_ext_callback_t *callback,
    void *param1,
    int param2)
{
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer *layer_ctx = NULL;

    D_DEBUG("%s/BCMCoreAttachCallback called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &bcm_layer)) != DFB_OK) {
        goto quit;
    }

    /* save callback information to layer structure */
    bcm_layer->callback        = callback;
    bcm_layer->callback_param1 = param1;
    bcm_layer->callback_param2 = param2;

    if (bcm_layer) {
        /* unlock a still locked layer handle */
        ret = (BCMMixerLayerRelease(bcm_layer) == DFB_OK) ? ret : DFB_FAILURE;
        /* fall through */
    }

quit:

    D_DEBUG("%s/BCMCoreAttachCallback exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult BCMCoreDetachCallback(DFBDisplayLayerID layer_id)
{
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer *layer_ctx = NULL;

    D_DEBUG("%s/BCMCoreDetachCallback called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    ret = BCMCoreAttachCallback(layer_id, NULL, NULL, 0);

quit:

    D_DEBUG("%s/BCMCoreDetachCallback exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

#ifdef DEBUG_ALLOCATION
#include <direct/interface.h>
#include <display/idirectfbsurface.h>
#include <core/surfacemanager_internal.h>
#endif

DFBResult BCMCoreShowMemory()
{
    DFBResult ret = DFB_OK;

    IDirectFBSurfaceManager *surface_manager = dfb_gfxcard_surface_manager();

    D_DEBUG("%s/BCMCoreShowMemory called: %p\n",
            BCMCoreModuleName,
            surface_manager);

    dfb_surfacemanager_enumerate_chunks( surface_manager, surfacemanager_chunk_callback, NULL );


quit:

    D_DEBUG("%s/BCMCoreShowMemory exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);
    return ret;
}



DFBResult BCMCoreGetSurfaceInfo(IDirectFBSurface *thiz, bdvd_dfb_ext_surface_info_t * surface_info)
{
    DFBResult ret = DFB_OK;
#ifdef DEBUG_ALLOCATION

    DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

    D_DEBUG("%s/BCMCoreGetSurfaceInfo called: %p %p\n",
            BCMCoreModuleName,
            thiz,
            surface_info);

    surface_info->app_num_ref = data->ref;

    if (data->surface)
    {
        surface_info->core_surface = data->surface->dbg_ul_surface_handle;
        surface_info->core_surface_ref_int_ref = data->surface->object.ref.fake.refs;
    }
    else
    {
        surface_info->core_surface = NULL;
        surface_info->core_surface_ref_int_ref = 0x0;
    }

    if (data->state.source)
    {
        surface_info->src_surface = data->state.source->dbg_ul_surface_handle;
        surface_info->src_surface_ref_int_ref = data->state.source->object.ref.fake.refs;
    }
    else
    {
        surface_info->src_surface = NULL;
        surface_info->src_surface_ref_int_ref = 0x0;
    }

    if (data->state.destination)
    {
        surface_info->dst_surface = data->state.destination->dbg_ul_surface_handle;
        surface_info->dst_surface_ref_int_ref = data->state.destination->object.ref.fake.refs;
    }
    else
    {
        surface_info->dst_surface = NULL;
        surface_info->dst_surface_ref_int_ref = 0x0;
    }

    if (data->surface->manager)
    {
        surface_info->surface_manager_type = data->surface->manager->type;
    }
    else
    {
        surface_info->surface_manager_type = 0x3; /* invalid */
    }

    surface_info->surface_caps = data->caps;

    if (((surface_info->surface_caps & DSCAPS_FLIPPING) == DSCAPS_FLIPPING) || (surface_info->surface_caps & DSCAPS_TRIPLE))
    {
        surface_info->buffering_type = bdvd_dfb_ext_surface_buffering_triple;
    }
    else if (surface_info->surface_caps & DSCAPS_DOUBLE)
    {
        surface_info->buffering_type = bdvd_dfb_ext_surface_buffering_double;

    }
    else
    {
        surface_info->buffering_type = bdvd_dfb_ext_surface_buffering_single;
    }

    if (data->surface->front_buffer)
    {
        unsigned int toleration;
        dfb_surfacemanager_retrive_surface_info(
            data->surface->manager,
            data->surface->front_buffer,
            &surface_info->front_buffer_offset,
            &surface_info->front_buffer_lenght,
            &toleration);
    }
    else
    {
        surface_info->front_buffer_offset = 0x0;
        surface_info->front_buffer_lenght = 0x0;
    }

    if (data->surface->back_buffer)
    {
        unsigned int toleration;
        dfb_surfacemanager_retrive_surface_info(
            data->surface->manager,
            data->surface->back_buffer,
            &surface_info->back_buffer_offset,
            &surface_info->back_buffer_lenght,
            &toleration);
    }
    else
    {
        surface_info->back_buffer_offset = 0x0;
        surface_info->back_buffer_lenght = 0x0;
    }

    if (data->surface->idle_buffer)
    {
        unsigned int toleration;
        dfb_surfacemanager_retrive_surface_info(
            data->surface->manager,
            data->surface->idle_buffer,
            &surface_info->idle_buffer_offset,
            &surface_info->idle_buffer_lenght,
            &toleration);
    }
    else
    {
        surface_info->idle_buffer_offset = 0x0;
        surface_info->idle_buffer_lenght = 0x0;
    }

quit:

    D_DEBUG("%s/BCMCoreGetSurfaceInfo exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);
#endif

    return ret;
}

DFBResult BCMCoreLayerLock(
    DFBDisplayLayerID layer_id,
    void *param1,
    int param2)
{
    DFBResult ret = DFB_OK;
    BCMCoreLayerHandle bcm_layer = NULL;
    BCMCoreContextLayer *layer_ctx = NULL;

    D_DEBUG("%s/BCMCoreLayerLock called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

    /* look up the mixer to use and its layer_id in its mixed_layer structure */
    layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

	/* lock the layer from the mixer */
    if ((ret = BCMMixerLayerAcquire(layer_ctx->mixer, layer_ctx->mixed_layer_id, &layer_ctx->layer_locked_ext)) != DFB_OK) {
        goto quit;
    }

	D_DEBUG("Layer Locked externally\n");

quit:

    D_DEBUG("%s/BCMCoreLayerLock exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult BCMCoreLayerUnlock(
    DFBDisplayLayerID layer_id)
{
    DFBResult ret = DFB_OK;
    BCMCoreContextLayer *layer_ctx = NULL;
    BCMCoreLayerHandle bcm_layer = NULL;

    D_DEBUG("%s/BCMCoreLayerUnlock called:\n"
            "layer_id %u\n",
            BCMCoreModuleName,
            layer_id );

	layer_ctx = &dfb_bcmcore_context->bcm_layers.layer[layer_id];

	if (layer_ctx->layer_locked_ext != NULL)
	{
		ret = BCMMixerLayerRelease(layer_ctx->layer_locked_ext);
        if (ret)
        {
            D_DEBUG("%s/BCMCoreLayerUnlock: BCMMixerLayerRelease failed with code %d\n",
                    BCMCoreModuleName,
                    (int)ret);
        }
		layer_ctx->layer_locked_ext = NULL;
	}

quit:

    D_DEBUG("%s/BCMCoreLayerUnlock exiting with code %d\n",
            BCMCoreModuleName,
            (int)ret);

    return ret;
}

DFBResult BCMCoreDefrag(IDirectFBSurfaceManager *thiz)
{
    DFBResult ret = DFB_OK;
    IDirectFBSurfaceManager *mgr;
    int mixer_id;
    BCMCoreContext *ctx;

    D_DEBUG("%s/BCMCoreDefrag called.\n",
            BCMCoreModuleName);

    ctx = (BCMCoreContext *)dfb_system_data();

    mgr = thiz;
    if (mgr == NULL) {
        mgr = dfb_gfxcard_surface_manager();
    }

    /*
     * Have to disable the mixer for defrag. This is because the mixer 
     * doesn't pay attention to the DFB gfxcard, surface, or surfacemanager 
     * locks. Without this we run into coherency problems between the 
     * mixer M2MC & the defrag's BCC operations. 
     */
    for (mixer_id = BCM_MIXER_0; mixer_id < ctx->graphics_constraints.nb_gfxWindows[0]; mixer_id++) {
        if (BCMMixerStop(dfb_bcmcore_context->mixers[mixer_id]) != DFB_OK)
        {
            D_ERROR("%s: Failure stopping mixer!\n", __FUNCTION__);
            return DFB_FAILURE;
        }
    }

    ret = dfb_surfacemanager_defrag(mgr);

    for (mixer_id = BCM_MIXER_0; mixer_id < ctx->graphics_constraints.nb_gfxWindows[0]; mixer_id++) {
        if (BCMMixerStart(dfb_bcmcore_context->mixers[mixer_id]) != DFB_OK)
        {
            D_ERROR("%s: Failure restarting mixer!\n", __FUNCTION__);
            return DFB_FAILURE;
        }
    }

    D_DEBUG("%s/BCMCoreDefrag exiting with code %d\n",
            BCMCoreModuleName, (int)ret);

    return ret;
}
