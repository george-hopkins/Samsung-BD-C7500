/*
*/

#include <dfb_types.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <directfb.h>

#include <direct/debug.h>
#include <direct/util.h>

#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/screens.h>
#include <core/state.h>

#include <core/surfacemanager_internal.h>

#include <core/gfxcard_internal.h>

#include <core/surfaces.h>
#include <core/palette.h>

#include <gfx/convert.h>
#include <gfx/util.h>

#include <misc/conf.h>

/* driver_ exported symbols */
#include <core/graphics_driver.h>

/* for definition of BCMCoreContext */
#include <bcm/bcmcore.h>

DFB_GRAPHICS_DRIVER( bcmgfx )

#include "bcmgfx.h"
#include "bcmgfx_state.h"

#include "bdvd_bcc.h"

#if D_DEBUG_ENABLED
/* #define ENABLE_MORE_DEBUG */
/* #define ENABLE_MEMORY_DUMP */
/* #define DEBUG_ALLOCATION */
#endif

#define DEBUG_DEFRAG 0
#if DEBUG_DEFRAG
#define D_DEFRAG(x)    printf("%s: ", __FUNCTION__); printf x
#else
#define D_DEFRAG(x)
#endif

#define BCMGfxModuleVersionMajor 0
#define BCMGfxModuleVersionMinor 1

/* driver capability flags for indexed colors pixel formats */

/* Can't premultiply source, because the color is an index */
/* Can't blend on a LUT8 destination */

#define BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFLAGS \
               ( DSDRAW_NOFX | DSDRAW_SRC_PREMULTIPLY )

#define BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFLAGS \
               ( DSBLIT_SRC_COLORKEY | DSBLIT_SRC_PREMULTIPLY | DSBLIT_DEINTERLACE )

#define BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFUNCTIONS \
               ( DFXL_FILLRECTANGLE )

#define BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFUNCTIONS \
               ( DFXL_BLIT )

/* driver capability flags for direct colors pixel formats */

#define BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFLAGS \
               ( DSDRAW_BLEND | DSDRAW_SRC_PREMULTIPLY )

/* DSDRAW_DST_COLORKEY TBD */

#define BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFLAGS \
               ( DSBLIT_SRC_COLORKEY | DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTIPLY | DSBLIT_DEINTERLACE | DSBLIT_XOR )

/*DSBLIT_DST_COLORKEY*/

#define BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS \
               ( DFXL_FILLRECTANGLE )

#define BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS \
               ( DFXL_BLIT | DFXL_STRETCHBLIT )

#define UNUSED_PARAM(p) (void)p

static bool show_allocation;

/* This is temporary until I can figure out a way to pass handle
 * to BCMGfxAllocateSurfaceBuffer.
 */
BCMGfxDriverData *temp_bcmgfxdrv = NULL;

/* required implementations */

static void BCMGfxEngineReset( void *drv, void *dev )
{
    D_DEBUG("%s/BCMGfxEngineReset called\n",
            BCMGfxModuleName);
}

static void BCMGfxEngineSync( void *drv, void *dev )
{
    bresult err;
    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) drv;
    BCMGfxDeviceData *bcmgfxdev = ( BCMGfxDeviceData* ) dev;
    BCMCoreContext *ctx;
    bdvd_result_e rc;

	UNUSED_PARAM(dev);

    D_ASSERT( bcmgfxdrv != NULL );
    D_ASSERT( bcmgfxdev != NULL );

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxEngineSync called\n",
            BCMGfxModuleName);
#endif

    ctx = (BCMCoreContext *)dfb_system_data();

    rc = bdvd_bcc_sync(ctx->driver_bcc_pg);
    if (rc != bdvd_ok)
    {
        D_ERROR( "%s/BCMGfxEngineSync: error calling bdvd_bcc_sync\n", BCMGfxModuleName );
    }
    rc = bdvd_bcc_sync(ctx->driver_bcc_textst);
    if (rc != bdvd_ok)
    {
        D_ERROR( "%s/BCMGfxEngineSync: error calling bdvd_bcc_sync\n", BCMGfxModuleName );
    }
    rc = bdvd_bcc_sync(ctx->driver_bcc_ig);
    if (rc != bdvd_ok)
    {
        D_ERROR( "%s/BCMGfxEngineSync: error calling bdvd_bcc_sync\n", BCMGfxModuleName );
    }
    rc = bdvd_bcc_sync(ctx->driver_bcc_other);
    if (rc != bdvd_ok)
    {
        D_ERROR( "%s/BCMGfxEngineSync: error calling bdvd_bcc_sync\n", BCMGfxModuleName );
    }

    if (!(getenv("no_multi_gfxfeeder")))
    {
        ctx = (BCMCoreContext *)dfb_system_data();
        if ((err = bdvd_graphics_compositor_sync(ctx->mixer_compositor)) != b_ok) {
            D_ERROR( "%s/BCMGfxEngineSync: error calling bdvd_graphics_compositor_sync %d\n",
                      BCMGfxModuleName,
                      err );
        }
    }

    /* Don't forget the wait was modified in
     * /home/xmiville/workspace2/Andover/juju-20051026.97398/magnum/portinginterface/grc/7038/bgrc.c
     */
    if ((err = bdvd_graphics_compositor_sync(bcmgfxdrv->driver_compositor)) != b_ok) {
        D_ERROR( "%s/BCMGfxEngineSync: error calling bdvd_graphics_compositor_sync %d\n",
                  BCMGfxModuleName,
                  err );
    }

    if (bdvd_dtk_get_handle())
    {
        ctx = (BCMCoreContext *)dfb_system_data();
        bdvd_result_e err;

        while (bdvd_dtk_is_idle(ctx->driver_fgx) == false)
        {
            /* add something */
        }
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxEngineSync ended\n",
            BCMGfxModuleName);
#endif
}

static void BCMGfxCheckState( void *drv, void *dev,
                              CardState *state, DFBAccelerationMask accel )
{
	UNUSED_PARAM(drv);
	UNUSED_PARAM(dev);

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxCheckState called with:\n"
            "accel %d\n"
            "drawingflags %d\n"
            "blittingflags %d\n",
            BCMGfxModuleName,
            accel,
            state->drawingflags,
            state->blittingflags);
#endif

    switch (state->destination->format) {
        case DSPF_ARGB:
        case DSPF_LUT8:
        case DSPF_RGB32:
        case DSPF_A8:
        case DSPF_A1:
        case DSPF_ARGB1555:
        case DSPF_ARGB4444:
        case DSPF_RGB16:
        case DSPF_YUY2:
        case DSPF_UYVY:
        case DSPF_AYUV:
        case DSPF_ALUT88:
        case DSPF_ABGR:
        case DSPF_LUT4:
        case DSPF_LUT2:
        case DSPF_LUT1:
        break;
        default:
        return;
    }

    if (DFB_PIXELFORMAT_IS_INDEXED(state->destination->format)) {
#ifdef ENABLE_MORE_DEBUG
        D_DEBUG("%s/BCMGfxCheckState indexed color pixel format functions and flags:\n"
                "Supported drawing fcts? %s\n"
                "Supported drawing flags? %s\n"
                "Supported blitting fcts? %s\n"
                "Supported blitting flags? %s\n"
                "Unsupported drawing fcts? %s\n"
                "Unsupported drawing flags? %s\n"
                "Unsupported blitting fcts? %s\n"
                "Unsupported blitting flags? %s\n",
                BCMGfxModuleName,
                accel & BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFUNCTIONS ? "yes" : "no",
                state->drawingflags & BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFLAGS ? "yes" : "no",
                accel & BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFUNCTIONS ? "yes" : "no",
                state->blittingflags & BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFLAGS ? "yes" : "no",
                accel & ~BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFUNCTIONS ? "yes" : "no",
                state->drawingflags & ~BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFLAGS ? "yes" : "no",
                accel & ~BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFUNCTIONS ? "yes" : "no",
                state->blittingflags & ~BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFLAGS ? "yes" : "no");
#endif

		if (state->source &&
		    !DFB_PIXELFORMAT_IS_INDEXED(state->source->format)) {
		    return;
	    }

        /* if there are no other drawing flags than the supported */
        if (!(accel & ~BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFUNCTIONS) &&
            !(state->drawingflags & ~BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFLAGS)) {
            state->accel |= BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFUNCTIONS;
        }

        /* What is the minimum size for a surface with the M2MC ???? */
        /* Also possible is that the before a certain minimum, it's not
         * worth doing hardware....
         */

        /* if there are no other blitting flags than the supported
           and the source has the minimum size */
        if (!(accel & ~BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFUNCTIONS) &&
            !(state->blittingflags & ~BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFLAGS) &&
            state->source)
        {
            switch (state->source->format) {
                case DSPF_ALUT88:
                case DSPF_LUT8:
                case DSPF_LUT4:
                case DSPF_LUT2:
                case DSPF_LUT1:
                    state->accel |= BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFUNCTIONS;
                break;
                default:
                    D_DEBUG("%s/BCMGfxCheckState unsupported source pixel format\n",
                            BCMGfxModuleName );
                    return;
            }
        }
    }
    else {
#ifdef ENABLE_MORE_DEBUG
        D_DEBUG("%s/BCMGfxCheckState direct color pixel format functions and flags:\n"
                "Supported drawing fcts? %s\n"
                "Supported drawing flags? %s\n"
                "Supported blitting fcts? %s\n"
                "Supported blitting flags? %s\n"
                "Unsupported drawing fcts? %s\n"
                "Unsupported drawing flags? %s\n"
                "Unsupported blitting fcts? %s\n"
                "Unsupported blitting flags? %s\n",
                BCMGfxModuleName,
                accel & BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS ? "yes" : "no",
                state->drawingflags & BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFLAGS ? "yes" : "no",
                accel & BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS ? "yes" : "no",
                state->blittingflags & BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFLAGS ? "yes" : "no",
                accel & ~BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS ? "yes" : "no",
                state->drawingflags & ~BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFLAGS ? "yes" : "no",
                accel & ~BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS ? "yes" : "no",
                state->blittingflags & ~BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFLAGS ? "yes" : "no");
#endif

        /* check for the special drawing function that does not support
            the usually supported drawingflags */

        /* if there are no other drawing flags than the supported */
        if (!(accel & ~BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS) &&
            !(state->drawingflags & ~BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFLAGS)) {
            if (!bcmgfx_check_drawingflags((BCMGfxDriverData *)drv, (BCMGfxDeviceData *)dev, state, false)) {
                D_DEBUG("%s/BCMGfxCheckState unsupported drawing flags\n",
                        BCMGfxModuleName );
                return;
            }
            state->accel |= BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS;
        }

        /* What is the minimum size for a surface with the M2MC ???? */
        /* Also possible is that the before a certain minimum, it's not
         * worth doing hardware....
         */

        /* if there are no other blitting flags than the supported
           and the source has the minimum size */
        if (!(accel & ~BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS) &&
            !(state->blittingflags & ~BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFLAGS) &&
            state->source)
        {
            if (!bcmgfx_check_blittingflags((BCMGfxDriverData *)drv, (BCMGfxDeviceData *)dev, state, false)) {
                D_DEBUG("%s/BCMGfxCheckState unsupported blitting flags\n",
                        BCMGfxModuleName );
                return;
            }
            switch (state->source->format) {
                case DSPF_ARGB:
                case DSPF_LUT8:
                case DSPF_RGB32:
                case DSPF_A8:
                case DSPF_A1:
                case DSPF_ARGB1555:
                case DSPF_ARGB4444:
                case DSPF_RGB16:
		        case DSPF_YUY2:
		        case DSPF_UYVY:
                case DSPF_AYUV:
                case DSPF_ALUT88:
                case DSPF_ABGR:
                case DSPF_LUT4:
                case DSPF_LUT2:
                case DSPF_LUT1:
                    state->accel |= BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS;
                break;
                default:
                    D_DEBUG("%s/BCMGfxCheckState unsupported source pixel format\n",
                            BCMGfxModuleName );
                return;
            }
        }
    }

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxCheckState exiting with accel %d\n",
            BCMGfxModuleName,
            state->accel);
#endif
}

static bool BCMGfxWeighArea( void *drv, void *dev,
                             CardState *state, DFBAccelerationMask accel,
                             const DFBRectangle *rects, int num )
{
	unsigned int area_hardware_threshold;

    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) drv;

    D_ASSERT( bcmgfxdrv != NULL );
    D_ASSERT( num != 0 );
    D_ASSERT( rects != NULL );

    area_hardware_threshold = bcmgfxdrv->area_hardware_threshold;

	if (DFB_BITS_PER_PIXEL(state->destination->format) != 32) {
		area_hardware_threshold = (area_hardware_threshold*80)/100;
	}

	if (accel &
	    (BCMGFX_INDEXEDPF_SUPPORTED_BLITTINGFUNCTIONS | BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS)) {
		switch (state->blittingflags & ~DSBLIT_SRC_PREMULTIPLY) {
            case DSBLIT_BLEND_COLORALPHA | DSBLIT_COLORIZE:
            case DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL:
            case DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE:
                area_hardware_threshold = (area_hardware_threshold*60)/100;
                goto quit;
            case DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE:
			    if (DFB_COLOR_BITS_PER_PIXEL(state->source->format)) {
	                area_hardware_threshold = (area_hardware_threshold*60)/100;
			    }
			    else {
   	                area_hardware_threshold = (area_hardware_threshold*80)/100;
			    }
                goto quit;
            case DSBLIT_SRC_COLORKEY:
            case DSBLIT_BLEND_ALPHACHANNEL:
                area_hardware_threshold = (area_hardware_threshold*80)/100;
                goto quit;
            default:
                goto quit;
		}
    }

	if (accel &
	    (BCMGFX_INDEXEDPF_SUPPORTED_DRAWINGFUNCTIONS | BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS)) {
		switch (state->drawingflags & ~DSDRAW_SRC_PREMULTIPLY) {
			case DSDRAW_BLEND:
				area_hardware_threshold = (area_hardware_threshold*80)/100;
                goto quit;
			default:
                goto quit;
        }
    }

quit:

    return (rects->w * rects->h) > area_hardware_threshold;
}

static void BCMGfxSetState( void *drv, void *dev,
                            GraphicsDeviceFuncs *funcs,
                            CardState *state, DFBAccelerationMask accel )
{
    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) drv;
    BCMGfxDeviceData *bcmgfxdev = ( BCMGfxDeviceData* ) dev;

    D_ASSERT( bcmgfxdrv != NULL );
    D_ASSERT( bcmgfxdev != NULL );
    D_ASSERT( state != NULL );

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxSetState called with:\n"
            "DFBAccelerationMask %d\n",
            BCMGfxModuleName,
            accel);
#endif

    if ( state->modified & SMF_SOURCE )
    bcmgfxdev->v_source = 0;

    if ( state->modified & SMF_DESTINATION )
    bcmgfxdev->v_destination = bcmgfxdev->v_color = 0;

    if ( state->modified & SMF_COLOR )
    bcmgfxdev->v_color = 0;

    if ( state->modified & SMF_SRC_COLORKEY )
    bcmgfxdev->v_src_colorkey = 0;

    if ( state->modified & SMF_DST_COLORKEY )
    bcmgfxdev->v_dst_colorkey = 0;

    if ( state->modified & SMF_BLITTING_FLAGS )
    bcmgfxdev->v_blittingflags = 0;

    if ( state->modified & SMF_DRAWING_FLAGS )
    bcmgfxdev->v_drawingflags = 0;

    switch (accel) {
        case DFXL_FILLRECTANGLE:
            /* Setting the source, even if null is necessary */
            bcmgfx_set_source(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_destination(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_clip(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_drawingflags(bcmgfxdrv, bcmgfxdev, state);
#if 0
            bcmgfx_set_dst_colorkey(bcmgfxdrv, bcmgfxdev, state);
#endif
            /* We must set color after the blending flags,
             * as blend config can change the color */
            bcmgfx_set_color(bcmgfxdrv, bcmgfxdev, state);
            state->set |= DFXL_FILLRECTANGLE;
        break;
        case DFXL_STRETCHBLIT:
        case DFXL_BLIT:
            bcmgfx_set_source(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_destination(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_clip(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_blittingflags(bcmgfxdrv, bcmgfxdev, state);
            bcmgfx_set_src_colorkey(bcmgfxdrv, bcmgfxdev, state);
#if 0
            bcmgfx_set_dst_colorkey(bcmgfxdrv, bcmgfxdev, state);
#endif
            /* We must set color after the blending flags,
             * as blend config can change the color */
            bcmgfx_set_color(bcmgfxdrv, bcmgfxdev, state);
            state->set |= DFXL_BLIT | DFXL_STRETCHBLIT;
        break;
        default:
            D_BUG("unexpected drawing/blitting function");
        break;
    }

    state->modified = 0;
}

/* operations */

/* This is used by both BCMGfxBlit and BCMGfxFillRectangle (when DSDRAW_BLEND was requested) */
static inline bool BCMGfxUnifiedBlit(
                BCMGfxDriverData *bcmgfxdrv,
                BCMGfxDeviceData *bcmgfxdev,
                DFBRectangle *srect, DFBRectangle *drect,
                bdvd_graphics_polarity_e polarity,
                bdvd_graphics_blending_rule_e alpha_blending_rule,
                bdvd_graphics_blending_rule_e color_blending_rule,
                bool drawing) {
    bresult err;
    bdvd_result_e rc;
    BCMCoreContext *ctx;
    bdvd_bcc_rect_t bcc_srect, bcc_drect;
    bdvd_bcc_h bcc;

#ifdef ENABLE_MEMORY_DUMP
    uint8_t * surfacemem_cached;
    uint8_t * surfacemem_uncached;
#endif

    bdvd_graphics_surface_h source_surface;
    bdvd_graphics_surface_h destination_surface;
    bdvd_graphics_compositor_h compositor = bcmgfxdrv->driver_compositor;

    ctx = (BCMCoreContext *)dfb_system_data();

    /* use the mixer compositor if surface require faster blit */
    if (!(getenv("no_multi_gfxfeeder")) && ((bcmgfxdev->source!=NULL && bcmgfxdev->source->caps & DSCAPS_OPTI_BLIT) || (bcmgfxdev->destination && (bcmgfxdev->destination->caps & DSCAPS_OPTI_BLIT))))
    {
        compositor = ctx->mixer_compositor;
    }

#ifdef ENABLE_MORE_DEBUG
    uint32_t i;

    D_DEBUG("%s/BCMGfxUnifiedBlit called\n"
            "srect.x %d srect.y %d srect.w %d srect.h %d\n"
            "drect.x %d drect.y %d drect.w %d drect.h %d\n"
            "alpha_blending_rule %d color_blending_rule %d\n",
            BCMGfxModuleName,
            srect->x,
            srect->y,
            srect->w,
            srect->h,
            drect->x,
            drect->y,
            drect->w,
            drect->h,
            alpha_blending_rule,
            color_blending_rule);
#endif

    D_ASSERT(bcmgfxdev->source ? (bcmgfxdev->source->field == 0 || bcmgfxdev->source->field == 1) : true);

    if (bcmgfxdev->source) {
        switch (polarity) {
            case BGFX_POLARITY_FRAME:
                if (bcmgfxdev->source->front_buffer->flags & SBF_HIDDEN) {
                    source_surface = ((bdvd_graphics_surface_h*)bcmgfxdev->source->back_buffer->video.ctx)[bcmgfxdev->source->field];
                }
                else {
                    source_surface = ((bdvd_graphics_surface_h*)bcmgfxdev->source->front_buffer->video.ctx)[bcmgfxdev->source->field];
                }
                D_ASSERT(source_surface);
                D_ASSERT(bcmgfxdev->destination->field == 0 || bcmgfxdev->destination->field == 1);
            break;
            case BGFX_POLARITY_TOP:
                source_surface = ((bdvd_graphics_surface_h*)bcmgfxdev->source->front_buffer->video.ctx)[0];
                D_ASSERT(source_surface);
                D_ASSERT(bcmgfxdev->destination->field == 0);
            break;
            case BGFX_POLARITY_BOTTOM:
                source_surface = ((bdvd_graphics_surface_h*)bcmgfxdev->source->front_buffer->video.ctx)[1];
                D_ASSERT(source_surface);
                D_ASSERT(bcmgfxdev->destination->field == 0);
            break;
            default:
                D_ERROR( "%s/BCMGfxUnifiedBlit: bad polarity type %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
        }
    }
    else {
        source_surface = NULL;
    }
    destination_surface = ((bdvd_graphics_surface_h*)bcmgfxdev->destination->back_buffer->video.ctx)[bcmgfxdev->destination->field];
    D_ASSERT(destination_surface);

#ifdef ENABLE_MEMORY_DUMP
    /* A crude memory dump, I should do a functions better than that */
    surfacemem_cached = destination_surface->mem.buffer;
    surfacemem_uncached = b_mem2noncached(&b_board.sys.mem, surfacemem_cached);

    D_DEBUG("Pre blit cached destination dump:\n");
    for (i = 0; i < destination_surface->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_cached[i]);
    }

    D_DEBUG("\n");

    D_DEBUG("Pre blit uncached destination dump:\n");
    for (i = 0; i < destination_surface->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_uncached[i]);
    }

    D_DEBUG("\n");
#endif

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxUnifiedBlit: blitting from surface %p to surface %p\n",
            BCMGfxModuleName,
            source_surface,
            destination_surface);
    D_DEBUG("%s/BCMGfxUnifiedBlit: source (%s) width %d height %d destination width %d height %d\n",
            BCMGfxModuleName,
            source_surface ? "yes" : "no",
            source_surface ? bcmgfxdev->source->width : 0,
            source_surface ? bcmgfxdev->source->height : 0,
            bcmgfxdev->destination->width,
            bcmgfxdev->destination->height);
#endif

    switch (color_blending_rule) {
        case BGFX_RULE_PORTERDUFF_CLEAR:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_PORTERDUFF_CLEAR\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);

            if (bcmgfxdev->destination->caps & DSCAPS_BCC) {
                switch (bcmgfxdev->destination->caps & DSCAPS_BCC) {
                    case DSCAPS_COPY_BLIT_PG:
                        bcc = ctx->driver_bcc_pg;
                        break;
                    case DSCAPS_COPY_BLIT_IG:
                        bcc = ctx->driver_bcc_ig;
                        break;
                    case DSCAPS_COPY_BLIT_TEXTST:
                        bcc = ctx->driver_bcc_textst;
                        break;
                    case DSCAPS_COPY_BLIT_OTHR:
                        bcc = ctx->driver_bcc_other;
                        break;
                    default:
                        D_ERROR( "%s/BCMGfxUnifiedBlit: invalid DSCAPS_COPY_BLIT type.\n", BCMGfxModuleName );
                        return false;
                }
                bcc_drect.ui16Height    = drect->h;
                bcc_drect.ui16Width     = drect->w;
                bcc_drect.ui16X         = drect->x;
                bcc_drect.ui16Y         = drect->y;
                rc = bdvd_bcc_block_fill(bcc, destination_surface, &bcc_drect, 0);
                if (rc != bdvd_ok) {
                    D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_bcc_block_fill %d\n",
                             BCMGfxModuleName );
                    return false;
                }
            }
            else
            {
                if ((err = bdvd_graphics_surface_fill(                                             
                              compositor,                                                          
                              destination_surface,                                                 
                              (bsettop_rect *)drect,                                               
                              0)) != b_ok) {                                                       
                    D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_fill %d\n",
                             BCMGfxModuleName,                                                     
                             err );                                                                
                    return false;                                                                  
                }                                                                                  
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRC:
        /* This copies the source and sets the destinations alpha channel
           to opaque. */
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_PORTERDUFF_SRC\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);
            D_ASSERT(bcmgfxdev->source != NULL);
            D_ASSERT(source_surface != NULL);

            DFBSurfaceCapabilities scaps_bcc, dcaps_bcc;
            scaps_bcc = bcmgfxdev->source->caps & DSCAPS_BCC;
            dcaps_bcc = bcmgfxdev->destination->caps & DSCAPS_BCC;
            if (scaps_bcc && dcaps_bcc && (scaps_bcc == dcaps_bcc) &&
                (drect->w == srect->w) && (drect->h == srect->h)) {
                switch (dcaps_bcc) {
                    case DSCAPS_COPY_BLIT_PG:
                        bcc = ctx->driver_bcc_pg;
                        break;
                    case DSCAPS_COPY_BLIT_IG:
                        bcc = ctx->driver_bcc_ig;
                        break;
                    case DSCAPS_COPY_BLIT_TEXTST:
                        bcc = ctx->driver_bcc_textst;
                        break;
                    case DSCAPS_COPY_BLIT_OTHR:
                        bcc = ctx->driver_bcc_other;
                        break;
                    default:
                        D_ERROR( "%s/BCMGfxUnifiedBlit: invalid DSCAPS_COPY_BLIT type.\n", BCMGfxModuleName );
                        return false;
                }
                bcc_drect.ui16Height    = drect->h;
                bcc_drect.ui16Width     = drect->w;
                bcc_drect.ui16X         = drect->x;
                bcc_drect.ui16Y         = drect->y;
                bcc_srect.ui16Height    = srect->h;
                bcc_srect.ui16Width     = srect->w;
                bcc_srect.ui16X         = srect->x;
                bcc_srect.ui16Y         = srect->y;
                rc = bdvd_bcc_block_copy(bcc, source_surface, &bcc_srect,
                        destination_surface, &bcc_drect);
                if (rc != bdvd_ok) {
                    D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_bcc_block_copy %d\n",
                             BCMGfxModuleName );
                    return false;
                }
            }
            else
            {
                if ((err = bdvd_graphics_surface_blit(                                                                         
                                compositor,                                                                                    
                                destination_surface,                                                                           
                                (bsettop_rect *)drect,                                                                         
                                alpha_blending_rule,                                                                           
                                color_blending_rule,                                                                           
                                source_surface,                                                                                
                                (bsettop_rect *)srect,                                                                         
                                polarity,                                                                                      
                                (drawing ? false : bcmgfxdev->enable_source_key) ? destination_surface : NULL,                 
                                (bsettop_rect *)drect,                                                                         
                                bcmgfxdev->fill_color,                                                                         
                                bcmgfxdev->source_key_value,                                                                   
                                drawing ? false : bcmgfxdev->enable_fade_effect,                                               
                                drawing ? false : bcmgfxdev->enable_source_key,                                                
                                drawing ? false : bcmgfxdev->fade_effect_src_prem,
                                false, 0, 0, 0, 0)) != b_ok) {                                 
                    D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_PORTERDUFF_SRC %d\n",
                             BCMGfxModuleName,                                                                                 
                             err );                                                                                            
                    return false;                                                                                              
                }                                                                                                              
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRCOVER:
#ifdef ENABLE_MORE_DEBUG
           D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_PORTERDUFF_SRCOVER\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);
            D_ASSERT(bcmgfxdev->source != NULL);
            D_ASSERT(source_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            destination_surface,
                            (bsettop_rect *)drect,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_PORTERDUFF_SRCOVER %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_SRCALPHA_ZERO:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_SRCALPHA_ZERO\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);
            D_ASSERT(bcmgfxdev->source != NULL);
            D_ASSERT(source_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            (drawing ? false : bcmgfxdev->enable_source_key) ? destination_surface : NULL,
                            (bsettop_rect *)drect,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRCALPHA_ZERO %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_SRCALPHA_INVSRCALPHA:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_SRCALPHA_INVSRCALPHA\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);
            D_ASSERT(bcmgfxdev->source != NULL);
            D_ASSERT(source_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            destination_surface,
                            (bsettop_rect *)drect,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRCALPHA_INVSRCALPHA %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC\n",
                    BCMGfxModuleName );

            D_DEBUG( "%s/BCMGfxUnifiedBlit: fill color is 0x%08X\n",
                    BCMGfxModuleName,
                    bcmgfxdev->fill_color );
#endif
            D_ASSERT(destination_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            NULL,
                            NULL,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC\n",
                    BCMGfxModuleName );

            D_DEBUG( "%s/BCMGfxUnifiedBlit: fill color is 0x%08X\n",
                    BCMGfxModuleName,
                    bcmgfxdev->fill_color );
#endif
            D_ASSERT(destination_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            destination_surface,
                            (bsettop_rect *)drect,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_SRCALPHAxCONSTANT_ZERO:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_SRCALPHAxCONSTANT_ZERO\n",
                    BCMGfxModuleName );

            D_DEBUG( "%s/BCMGfxUnifiedBlit: fill color is 0x%08X\n",
                    BCMGfxModuleName,
                    bcmgfxdev->fill_color );
#endif
            D_ASSERT(destination_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            NULL,
                            NULL,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRCALPHAxCONSTANT_ZERO %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA\n",
                    BCMGfxModuleName );

            D_DEBUG( "%s/BCMGfxUnifiedBlit: fill color is 0x%08X\n",
                    BCMGfxModuleName,
                    bcmgfxdev->fill_color );
#endif
            D_ASSERT(destination_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            destination_surface,
                            (bsettop_rect *)drect,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        case BGFX_RULE_SRC_XOR_DEST:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxUnifiedBlit: color blending rule is BGFX_RULE_SRC_XOR_DEST\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);
            D_ASSERT(bcmgfxdev->source != NULL);
            D_ASSERT(source_surface != NULL);

            if ((err = bdvd_graphics_surface_blit(
                            compositor,
                            destination_surface,
                            (bsettop_rect *)drect,
                            alpha_blending_rule,
                            color_blending_rule,
                            source_surface,
                            (bsettop_rect *)srect,
                            polarity,
                            destination_surface,
                            (bsettop_rect *)drect,
                            bcmgfxdev->fill_color,
                            bcmgfxdev->source_key_value,
                            drawing ? false : bcmgfxdev->enable_fade_effect,
                            drawing ? false : bcmgfxdev->enable_source_key,
                            drawing ? false : bcmgfxdev->fade_effect_src_prem,
                            false, 0, 0, 0, 0)) != b_ok) {
                D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_graphics_surface_blit in BGFX_RULE_SRC_XOR_DEST %d\n",
                         BCMGfxModuleName,
                         err );
                return false;
            }
        break;
        default:
            D_ERROR( "%s/BCMGfxUnifiedBlit: invalid blit operation %d\n",
                     BCMGfxModuleName,
                     color_blending_rule );
            return false;
        break;
    }

#ifdef ENABLE_MEMORY_DUMP
    /* A crude memory dump, I should do a functions better than that */
    surfacemem_cached = destination_surface->mem.buffer;
    surfacemem_uncached = b_mem2noncached(&b_board.sys.mem, surfacemem_cached);

    D_DEBUG("Post blit cached destination dump:\n");
    for (i = 0; i < destination_surface->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_cached[i]);
    }

    D_DEBUG("\n");

    D_DEBUG("Post blit uncached destination dump:\n");
    for (i = 0; i < destination_surface->mem.pitch && i < 10; i++) {
        fprintf(stderr, "0x%02X ", surfacemem_uncached[i]);
    }

    D_DEBUG("\n");
#endif

    return true;

}

static bool BCMGfxBlit( void *drv, void *dev,
                             DFBRectangle *rect, int dx, int dy )
{
    bool ret = true;
    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) drv;
    BCMGfxDeviceData *bcmgfxdev = ( BCMGfxDeviceData* ) dev;

    DFBRectangle srect;
    DFBRectangle drect;
    bdvd_graphics_polarity_e polarity;

#ifdef ENABLE_MORE_DEBUG
    uint32_t i;
    uint8_t * surfacemem_cached;
    uint8_t * surfacemem_uncached;
#endif

    D_ASSERT( bcmgfxdrv != NULL );
    D_ASSERT( bcmgfxdev != NULL );
    D_ASSERT( rect != NULL );

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxBlit called\n"
            "rect.x %d rect.y %d rect.w %d rect.h %d\n"
            "dx %d dy %d\n",
            BCMGfxModuleName,
            rect->x,
            rect->y,
            rect->w,
            rect->h,
            dx,
            dy);
#endif

    switch (bcmgfxdev->blit_alpha_blending_rule) {
        case BGFX_RULE_PORTERDUFF_CLEAR:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxBlit: alpha blending rule is BGFX_RULE_PORTERDUFF_CLEAR\n",
                    BCMGfxModuleName );
#endif
        break;
        case BGFX_RULE_PORTERDUFF_SRC:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxBlit: alpha blending rule is BGFX_RULE_PORTERDUFF_SRC\n",
                    BCMGfxModuleName );
#endif
        break;
        case BGFX_RULE_PORTERDUFF_SRCOVER:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxBlit: alpha blending rule is BGFX_RULE_PORTERDUFF_SRCOVER\n",
                    BCMGfxModuleName );
#endif
        break;
        case BGFX_RULE_SRC_XOR_DEST:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxBlit: alpha blending rule is BGFX_RULE_SRC_XOR_DEST\n",
                    BCMGfxModuleName );
#endif
        break;
        default:
            D_ERROR( "%s/BCMGfxBlit: invalid alpha blending rule %d\n",
                     BCMGfxModuleName,
                     bcmgfxdev->blit_alpha_blending_rule );
            return false;
        break;
    }

    drect.x = dx;
    drect.y = dy;
    drect.w = rect->w;

    srect = *rect;

    if (bcmgfxdev->source && bcmgfxdev->blit_deinterlace) {

        srect.h /= 2;
        srect.y /= 2;
        drect.y /= 2;
        drect.h = srect.h;

        if (true /*!(dy%2)*/) {
            polarity = BGFX_POLARITY_TOP; /* TOP FIRST */
            bcmgfxdev->source->field = 0;
        }
        else {
            polarity = BGFX_POLARITY_BOTTOM; /* BOTTOM FIRST */
            bcmgfxdev->source->field = 1;
        }

        if (bcmgfxdev->destination->field != 0) {
            D_ERROR( "%s/BCMGfxBlit: destination is not progressive\n",
                     BCMGfxModuleName );
            return false;
        }

        ret = BCMGfxUnifiedBlit( bcmgfxdrv, bcmgfxdev, &srect, &drect, polarity, bcmgfxdev->blit_alpha_blending_rule, bcmgfxdev->blit_color_blending_rule, false);
        polarity = (polarity == BGFX_POLARITY_TOP) ? BGFX_POLARITY_BOTTOM : BGFX_POLARITY_TOP;
        bcmgfxdev->source->field = (polarity == BGFX_POLARITY_TOP) ? 1 : 0;
        if (rect->h%2) {
            srect.h -= 1;
            drect.h = srect.h;
        }
    }
    else {
    drect.h = rect->h;
        polarity = BGFX_POLARITY_FRAME;
    }

    if (ret) ret = BCMGfxUnifiedBlit( bcmgfxdrv, bcmgfxdev, &srect, &drect, polarity, bcmgfxdev->blit_alpha_blending_rule, bcmgfxdev->blit_color_blending_rule, false);

    return ret;
}

static bool BCMGfxFillRectangle( void *drv, void *dev, DFBRectangle *rect )
{
    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) drv;
    BCMGfxDeviceData *bcmgfxdev = ( BCMGfxDeviceData* ) dev;
    bresult err;
    bdvd_graphics_surface_h destination_surface;
    bdvd_graphics_compositor_h compositor;
    bdvd_result_e rc;
    BCMCoreContext *ctx;
    bdvd_bcc_rect_t bcc_drect;
    bdvd_bcc_h bcc;
    uint32_t fill_value;

    D_ASSERT( bcmgfxdrv != NULL );
    D_ASSERT( bcmgfxdev != NULL );
    D_ASSERT( rect != NULL );

    compositor = bcmgfxdrv->driver_compositor;

    ctx = (BCMCoreContext *)dfb_system_data();

    /* use the mixer compositor if surface require faster blit */
    if (!(getenv("no_multi_gfxfeeder")) && ((bcmgfxdev->source!=NULL && bcmgfxdev->source->caps & DSCAPS_OPTI_BLIT) || (bcmgfxdev->destination && (bcmgfxdev->destination->caps & DSCAPS_OPTI_BLIT))))
    {
        compositor = ctx->mixer_compositor;
    }

    destination_surface = ((bdvd_graphics_surface_h*)bcmgfxdev->destination->back_buffer->video.ctx)[bcmgfxdev->destination->field];

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxFillRectangle called with:\n"
            "rect.x %d rect.y %d rect.w %d rect.h %d\n"
            "draw_alpha_blending_rule %d draw_color_blending_rule %d\n",
            BCMGfxModuleName,
            rect->x,
            rect->y,
            rect->w,
            rect->h,
            bcmgfxdev->draw_alpha_blending_rule,
            bcmgfxdev->draw_color_blending_rule);
#endif

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxFillRectangle: drawing to surface %p\n",
            BCMGfxModuleName,
            destination_surface);
#endif

    switch (bcmgfxdev->draw_alpha_blending_rule) {
        case BGFX_RULE_PORTERDUFF_CLEAR:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxFillRectangle: alpha blending rule is BGFX_RULE_PORTERDUFF_CLEAR\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);

            if (bcmgfxdev->draw_color_blending_rule == BGFX_RULE_PORTERDUFF_CLEAR) {
#ifdef ENABLE_MORE_DEBUG
                D_DEBUG( "%s/BCMGfxFillRectangle: color blending rule is BGFX_RULE_PORTERDUFF_CLEAR\n",
                        BCMGfxModuleName );
#endif
                if (bcmgfxdev->destination->caps & DSCAPS_BCC) {
                    switch (bcmgfxdev->destination->caps & DSCAPS_BCC) {
                        case DSCAPS_COPY_BLIT_PG:
                            bcc = ctx->driver_bcc_pg;
                            break;
                        case DSCAPS_COPY_BLIT_IG:
                            bcc = ctx->driver_bcc_ig;
                            break;
                        case DSCAPS_COPY_BLIT_TEXTST:
                            bcc = ctx->driver_bcc_textst;
                            break;
                        case DSCAPS_COPY_BLIT_OTHR:
                            bcc = ctx->driver_bcc_other;
                            break;
                        default:
                            D_ERROR( "%s/BCMGfxUnifiedBlit: invalid DSCAPS_COPY_BLIT type.\n", BCMGfxModuleName );
                            return false;
                    }
                    bcc_drect.ui16Height    = rect->h;
                    bcc_drect.ui16Width     = rect->w;
                    bcc_drect.ui16X         = rect->x;
                    bcc_drect.ui16Y         = rect->y;
                    rc = bdvd_bcc_block_fill(bcc, destination_surface, &bcc_drect, 0);
                    if (rc != bdvd_ok) {
                        D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_bcc_block_fill %d\n",
                                 BCMGfxModuleName );
                        return false;
                    }
                }
                else
                {
                    if ((err = bdvd_graphics_surface_fill(
                                  compositor,
                                  destination_surface,
                                  (bsettop_rect *)rect,
                                  0)) != b_ok) {
                        D_ERROR( "%s/BCMGfxFillRectangle: error calling bdvd_graphics_surface_fill %d\n",
                                 BCMGfxModuleName,
                                 err );
                        return false;
                    }
                }
                return true;
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxFillRectangle: alpha blending rule is BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC\n",
                    BCMGfxModuleName );
#endif
            D_ASSERT(destination_surface != NULL);

            if (bcmgfxdev->draw_color_blending_rule == BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC) {
#ifdef ENABLE_MORE_DEBUG
                D_DEBUG(4 "%s/BCMGfxFillRectangle: color blending rule is BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC\n",
                        BCMGfxModuleName );
#endif
                if (bcmgfxdev->destination->caps & DSCAPS_BCC) {
                    switch (bcmgfxdev->destination->caps & DSCAPS_BCC) {
                        case DSCAPS_COPY_BLIT_PG:
                            bcc = ctx->driver_bcc_pg;
                            break;
                        case DSCAPS_COPY_BLIT_IG:
                            bcc = ctx->driver_bcc_ig;
                            break;
                        case DSCAPS_COPY_BLIT_TEXTST:
                            bcc = ctx->driver_bcc_textst;
                            break;
                        case DSCAPS_COPY_BLIT_OTHR:
                            bcc = ctx->driver_bcc_other;
                            break;
                        default:
                            D_ERROR( "%s/BCMGfxUnifiedBlit: invalid DSCAPS_COPY_BLIT type.\n", BCMGfxModuleName );
                            return false;
                    }
                    bcc_drect.ui16Height    = rect->h;
                    bcc_drect.ui16Width     = rect->w;
                    bcc_drect.ui16X         = rect->x;
                    bcc_drect.ui16Y         = rect->y;
                    fill_value = (bcmgfxdev->premult_fill_color ? PIXEL_ARGB( bcmgfxdev->color.a,
                                    (bcmgfxdev->color.a*bcmgfxdev->color.r)/255,
                                    (bcmgfxdev->color.a*bcmgfxdev->color.g)/255,
                                    (bcmgfxdev->color.a*bcmgfxdev->color.b)/255)
                                    : bcmgfxdev->fill_color);
                    rc = bdvd_bcc_block_fill(bcc, destination_surface, &bcc_drect, fill_value);
                    if (rc != bdvd_ok) {
                        D_ERROR( "%s/BCMGfxUnifiedBlit: error calling bdvd_bcc_block_fill %d\n",
                                 BCMGfxModuleName );
                        return false;
                    }
                }
                else
                {
                    if ((err = bdvd_graphics_surface_fill(
                                  compositor,
                                  destination_surface,
                                  (bsettop_rect *)rect,
                                  (bdvd_graphics_pixel_t)
                                  (bcmgfxdev->premult_fill_color ? PIXEL_ARGB( bcmgfxdev->color.a,
                                                                    (bcmgfxdev->color.a*bcmgfxdev->color.r)/255,
                                                                    (bcmgfxdev->color.a*bcmgfxdev->color.g)/255,
                                                                    (bcmgfxdev->color.a*bcmgfxdev->color.b)/255)
                                                                    : bcmgfxdev->fill_color))) != b_ok) {
                        D_ERROR( "%s/BCMGfxFillRectangle: error calling bdvd_graphics_surface_fill %d\n",
                                 BCMGfxModuleName,
                                 err );
                        return false;
                    }
                }
                return true;
            }
        break;
        case BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxFillRectangle: alpha blending rule is BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC\n",
                    BCMGfxModuleName );
#endif
        break;
        case BGFX_RULE_SRC_XOR_DEST:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxFillRectangle: alpha blending rule is BGFX_RULE_SRC_XOR_DEST\n",
                    BCMGfxModuleName );
#endif
        break;
        default:
            D_ERROR( "%s/BCMGfxFillRectangle: invalid alpha blending rule %d\n",
                     BCMGfxModuleName,
                     bcmgfxdev->draw_alpha_blending_rule );
            return false;
        break;
    }

    return BCMGfxUnifiedBlit( bcmgfxdrv, bcmgfxdev, rect, rect, BGFX_POLARITY_FRAME, bcmgfxdev->draw_alpha_blending_rule, bcmgfxdev->draw_color_blending_rule, true);
}

static bool BCMGfxStretchBlit( void *drv, void *dev,
                                    DFBRectangle *srect, DFBRectangle *drect )
{
    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) drv;
    BCMGfxDeviceData *bcmgfxdev = ( BCMGfxDeviceData* ) dev;

    D_ASSERT( bcmgfxdrv != NULL );
    D_ASSERT( bcmgfxdev != NULL );
    D_ASSERT( srect != NULL );
    D_ASSERT( drect != NULL );

#ifdef ENABLE_MORE_DEBUG
    D_DEBUG("%s/BCMGfxStretchBlit called\n"
            "srect.x %d srect.y %d srect.w %d srect.h %d\n"
            "drect.x %d drect.y %d drect.w %d drect.h %d\n",
            BCMGfxModuleName,
            srect->x,
            srect->y,
            srect->w,
            srect->h,
            drect->x,
            drect->y,
            drect->w,
            drect->h);
#endif

	if( (srect->w > drect->w * bcmgfxdrv->max_horizontal_downscale) ||
		(srect->h > drect->h * bcmgfxdrv->max_vertical_downscale) ||
		(srect->w > bcmgfxdrv->max_image_size) ||
		(srect->h > bcmgfxdrv->max_image_size)) {
		return false;
	}

    switch (bcmgfxdev->blit_alpha_blending_rule) {
        case BGFX_RULE_PORTERDUFF_CLEAR:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxStretchBlit: alpha blending rule is BGFX_RULE_PORTERDUFF_CLEAR\n",
                    BCMGfxModuleName );
#endif
        break;
        case BGFX_RULE_PORTERDUFF_SRC:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxStretchBlit: alpha blending rule is BGFX_RULE_PORTERDUFF_SRC\n",
                    BCMGfxModuleName );
#endif
        break;
        case BGFX_RULE_PORTERDUFF_SRCOVER:
#ifdef ENABLE_MORE_DEBUG
            D_DEBUG( "%s/BCMGfxStretchBlit: alpha blending rule is BGFX_RULE_PORTERDUFF_SRCOVER\n",
                    BCMGfxModuleName );
#endif
        break;
        default:
            D_ERROR( "%s/BCMGfxStretchBlit: invalid alpha blending rule %d\n",
                     BCMGfxModuleName,
                     bcmgfxdev->blit_alpha_blending_rule );
            return false;
        break;
    }

    return BCMGfxUnifiedBlit( bcmgfxdrv, bcmgfxdev, srect, drect, BGFX_POLARITY_FRAME, bcmgfxdev->blit_alpha_blending_rule, bcmgfxdev->blit_color_blending_rule, false);
}

/*
 * after the video memory has been written to by the CPU (e.g. modification
 * of a texture) make sure the accelerator won't use cached texture data
 */
static void BCMGfxFlushSpecificTextureCache( void * drv, void * dev, SurfaceBuffer *buffer )
{
    bresult err;
    uint32_t i;

    if (buffer->video.ctx) {
        for (i = 0; i < 2; i++) {
            if (((bdvd_graphics_surface_h*)buffer->video.ctx)[i]) {
                if ((err = bdvd_graphics_surface_flush_cache(((bdvd_graphics_surface_h*)buffer->video.ctx)[i])) != b_ok) {
        D_ERROR( "%s/BCMGfxFlushTextureCache: error calling bdvd_graphics_surface_flush_cache %d\n",
                 BCMGfxModuleName,
                 err );
		/* cannot return error */
    }
}
        }
    }
}

static bool BCMGetBackFieldContext( void * drv, void * dev,
                                    CoreSurface *surface, void **ctx)
{
    DFBResult ret = DFB_OK;

    bdvd_graphics_surface_h *priv_surface;

    priv_surface = (bdvd_graphics_surface_h *)ctx;

    *priv_surface = ((bdvd_graphics_surface_h*)surface->back_buffer->video.ctx)[surface->field];

    return ret;
}

/* Surface management functions */

static DFBResult BCMGfxAllocateSurfaceBuffer( SurfaceManager             *manager,
                                              SurfaceBuffer              *buffer )
{
    DFBResult ret = DFB_OK;

    bdvd_graphics_surface_h surface = NULL;
    bdvd_graphics_surface_create_settings_t create_settings;
    bdvd_graphics_surface_memory_t memory;

    int offset;
    int nb_surface = 1;
    uint32_t i;

    bresult err;

    D_ASSERT(manager != NULL);
    D_ASSERT(buffer != NULL);

    D_DEBUG("%s/BCMGfxAllocateSurfaceBuffer called\n",
            BCMGfxModuleName);

    bdvd_graphics_surface_create_settings_init(&create_settings);

    /* Setting pixelformat */

    switch (buffer->format) {
        case DSPF_ARGB:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;
        break;
        case DSPF_LUT8:
            create_settings.pixel_format = bdvd_graphics_pixel_format_palette8;
        break;
        case DSPF_RGB32:
            create_settings.pixel_format = bdvd_graphics_pixel_format_x8_r8_g8_b8;
        break;
        case DSPF_A8:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a8;
        break;
        case DSPF_A1:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a1;
        break;
        case DSPF_ARGB1555:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a1_r5_g5_b5;
        break;
        case DSPF_ARGB4444:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a4_r4_g4_b4;
        break;
        case DSPF_RGB16:
            create_settings.pixel_format = bdvd_graphics_pixel_format_r5_g6_b5;
        break;
        case DSPF_YUY2:
            create_settings.pixel_format = bdvd_graphics_pixel_format_cr8_y18_cb8_y08;
        break;
        case DSPF_UYVY:
            create_settings.pixel_format = bdvd_graphics_pixel_format_y18_cr8_y08_cb8;
        break;
        case DSPF_AYUV:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a8_y8_cr8_cb8;
        break;
        case DSPF_ALUT88:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a8_palette8;
        break;
        case DSPF_ABGR:
            create_settings.pixel_format = bdvd_graphics_pixel_format_a8_b8_g8_r8;
        break;
        case DSPF_LUT4:
            create_settings.pixel_format = bdvd_graphics_pixel_format_palette4;
        break;
        case DSPF_LUT2:
            create_settings.pixel_format = bdvd_graphics_pixel_format_palette2;
        break;
        case DSPF_LUT1:
            create_settings.pixel_format = bdvd_graphics_pixel_format_palette1;
        break;
        default:
            ret = DFB_INVARG;
            goto quit;
    }

    create_settings.width = buffer->surface->width;         /* pixel width of the surface */
    create_settings.height = buffer->surface->height;        /* pixel height of the surface */

    D_DEBUG("%s/BCMGfxAllocateSurfaceBuffer: pixel format %d width %u height %u\n",
            BCMGfxModuleName,
            create_settings.pixel_format, create_settings.width, create_settings.height);

    buffer->video.ctx = calloc(2, sizeof(bdvd_graphics_surface_h));

    offset = buffer->video.offset;

    if ((buffer->surface->caps & DSCAPS_INTERLACED) &&
        (buffer->surface->caps & DSCAPS_SEPARATED)) {
        D_ASSERT(!(create_settings.height%2));
        nb_surface = 2;
    }

    for (i = 0; i < nb_surface; i++) {
        if ((i == 1) &&
            (buffer->surface->caps & DSCAPS_INTERLACED) &&
            (buffer->surface->caps & DSCAPS_SEPARATED)) {
            offset += create_settings.height*buffer->video.pitch/2;
        }

    if ((surface = bdvd_graphics_surface_create(&create_settings,
                        bdvd_graphics_uncached_virtual_addr_from_offset(offset),
                    buffer->video.pitch)) == NULL) {
        D_ERROR("%s/BCMGfxAllocateSurfaceBuffer: bdvd_graphics_surface_create failed\n",
                BCMGfxModuleName);
        ret = DFB_FAILURE;
        goto quit;
    }

#ifdef DEBUG_ALLOCATION
    if (show_allocation) {
        D_INFO("%s/BCMGfxAllocateSurfaceBuffer: post bdvd_graphics_surface_create buffer %p ctx %p)\n",
               BCMGfxModuleName,
               buffer, surface);
    }
#endif

    if (temp_bcmgfxdrv->clear_surface_memory) {
        if ((err = bdvd_graphics_surface_clear(temp_bcmgfxdrv->driver_compositor, surface)) != b_ok) {
            D_ERROR( "%s/BCMGfxAllocateSurfaceBuffer: error calling bdvd_graphics_surface_clear %d\n",
                 BCMGfxModuleName,
                 err );
            ret = DFB_FAILURE;
            goto quit;
        }
    }

        ((bdvd_graphics_surface_h*)buffer->video.ctx)[i] = surface;
    }

quit:

	if (ret != DFB_OK && surface) {
        for (i = 0; i < nb_surface; i++) {
    	    if ((err = bdvd_graphics_surface_destroy(((bdvd_graphics_surface_h*)buffer->video.ctx)[i])) != b_ok) {
	        D_ERROR( "%s/BCMGfxAllocateSurfaceBuffer: error calling bdvd_graphics_surface_destroy %d\n",
	                 BCMGfxModuleName,
	                 err );
			/* fall through */
	    }
	}
	}

    D_DEBUG("%s/BCMGfxAllocateSurfaceBuffer returned with %d\n",
            BCMGfxModuleName,
            ret);

    return ret;
}

static DFBResult BCMGfxCopySurfaceBuffer(
    SurfaceBuffer   *buffer,
    int             offset)
{
    bdvd_result_e rc;
    int length;
    int remaining;
    unsigned char *src;
    unsigned char *dest;
    unsigned char *end;
    BCMCoreContext *ctx;
    int block;
    int max_block;
    int wait_time;

    D_DEFRAG(("buffer: 0x%08x\n", buffer));
    D_DEFRAG(("offset: 0x%08x\n", offset));

    /*
     * Since the BCC can't handle overlapping copies we have to break 
     * the copy up into multiple copies of non-overlapping blocks. 
     */
    src = bdvd_graphics_uncached_virtual_addr_from_offset(buffer->video.offset);
    dest = bdvd_graphics_uncached_virtual_addr_from_offset(offset);
    if (src == dest)
    {
        /*
         * Nothing to do.
         */
        return DFB_OK;
    }

    if (src < dest)
    {
        /*
         * If the new offset is > the current offset we need to copy in 
         * blocks from the end of the surface buffer toward the beginning 
         * of the surface buffer to prevent overwriting our source data. 
         *  
         */
        length = buffer->video.pitch * buffer->surface->height;
        src += length;
        dest += length;
    }

    ctx = (BCMCoreContext *)dfb_system_data();
    max_block = (int)(src < dest ? dest - src : src - dest);
    remaining = length;
    do
    {
        block = remaining >= max_block ? max_block : remaining;
        if (src < dest)
        {
            src -= block;
            dest -= block;
            wait_time = 0;
            do
            {
                rc = bdvd_bcc_mem_copy(ctx->driver_bcc_other, dest, src, block);
                if (rc == bdvd_err_busy)
                {
                    D_ERROR("%s: BCC busy; retrying.\n", __FUNCTION__);
                    usleep(BCMGFX_COPY_WAIT_TIME);
                    wait_time += BCMGFX_COPY_WAIT_TIME;
                }
            } while (rc == bdvd_err_busy && wait_time < BCMGFX_COPY_TIMEOUT);
            if (rc != bdvd_ok)
            {
                D_ERROR("%s: bdvd_bcc_mem_copy failed or timeout occurred waiting for BCC "
                        "availability!\n", __FUNCTION__);
                D_ERROR("%s: Attempting to restore buffers to pre-copy state.\n", __FUNCTION__);
                end = bdvd_graphics_uncached_virtual_addr_from_offset(offset) + length;
                src += block;
                dest += block;
                while ((int)dest < (int)end)
                {
                    *src++ = *dest++;
                }
                return DFB_FAILURE;
            }
        }
        else
        {
            wait_time = 0;
            do
            {
                rc = bdvd_bcc_mem_copy(ctx->driver_bcc_other, dest, src, block);
                if (rc == bdvd_err_busy)
                {
                    D_ERROR("%s: BCC busy; retrying.\n", __FUNCTION__);
                    usleep(BCMGFX_COPY_WAIT_TIME);
                    wait_time += BCMGFX_COPY_WAIT_TIME;
                }
            } while (rc == bdvd_err_busy && wait_time < BCMGFX_COPY_TIMEOUT);
            if (rc != bdvd_ok)
            {
                D_ERROR("%s: bdvd_bcc_mem_copy failed or timeout occurred waiting for BCC "
                        "availability!\n", __FUNCTION__);
                D_ERROR("%s: Attempting to restore buffers to pre-copy state.\n", __FUNCTION__);
                end = bdvd_graphics_uncached_virtual_addr_from_offset(offset);
                src--;
                dest--;
                while ((int)dest >= (int)end)
                {
                     *src-- = *dest--;
                }
                return DFB_FAILURE;
            }
            src += block;
            dest += block;
        }
        remaining -= block;
    } while (remaining > 0);

    return DFB_OK;
}

static DFBResult BCMGfxUpdateSurfaceBuffer(
    SurfaceBuffer   *buffer)
{
    bdvd_graphics_surface_h bdvd_surface;

    D_ASSERT(buffer != NULL);

    bdvd_surface = ((bdvd_graphics_surface_h *)buffer->video.ctx)[0];

    bdvd_graphics_surface_update_buffer(bdvd_surface, 
        bdvd_graphics_uncached_virtual_addr_from_offset(buffer->video.offset));

    return DFB_OK;

}

static DFBResult BCMGfxDeallocateSurfaceBuffer( SurfaceManager             *manager,
                                   SurfaceBuffer              *buffer )
{
    DFBResult ret = DFB_OK;
    bresult err;

    D_ASSERT(manager != NULL);
    D_ASSERT(buffer != NULL);

    D_DEBUG("%s/BCMGfxDeallocateSurfaceBuffer called\n",
            BCMGfxModuleName);

    if (buffer->video.ctx != NULL) {
        if (((bdvd_graphics_surface_h*)buffer->video.ctx)[0] != NULL) {
            if ((err = bdvd_graphics_surface_destroy(((bdvd_graphics_surface_h*)buffer->video.ctx)[0])) != b_ok) {
                D_ERROR( "%s/BCMGfxDeallocateSurfaceBuffer: error calling bdvd_graphics_surface_destroy %d\n",
                         BCMGfxModuleName,
                         err );
                ret = DFB_FAILURE;
                goto quit;
            }
        }
    
        if (((bdvd_graphics_surface_h*)buffer->video.ctx)[1] != NULL) {
            if ((err = bdvd_graphics_surface_destroy(((bdvd_graphics_surface_h*)buffer->video.ctx)[1])) != b_ok) {
                D_ERROR( "%s/BCMGfxDeallocateSurfaceBuffer: error calling bdvd_graphics_surface_destroy %d\n",
                         BCMGfxModuleName,
                         err );
                ret = DFB_FAILURE;
                goto quit;
            }
        }

#ifdef DEBUG_ALLOCATION
        if (show_allocation) {
            D_INFO("%s/BCMGfxDeallocate: post bdvd_graphics_surface_destroy buffer %p ctx %p\n",
                   BCMGfxModuleName,
                   buffer, buffer->video.ctx);
        }
#endif

        free(buffer->video.ctx);
    
        buffer->video.ctx = NULL;
    }

quit:

    D_DEBUG("%s/BCMGfxDeallocateSurfaceBuffer returned with %d\n",
            BCMGfxModuleName,
            ret);

    return ret;
}

static DFBResult BCMGfxAllocatePaletteBuffer( SurfaceManager             *manager,
                                              CorePalette                *palette )
{
    DFBResult ret = DFB_OK;
    bresult err;
    bdvd_graphics_palette_h temp_palette = NULL;

    D_ASSERT(manager != NULL);
    D_ASSERT(palette != NULL);

    D_DEBUG("%s/BCMGfxAllocatePaletteBuffer called\n",
            BCMGfxModuleName);

    if (palette->num_entries == 0) {
        D_ERROR( "%s/BCMGfxAllocatePaletteBuffer: error num_entries must be greater than zero\n",
                 BCMGfxModuleName );
        ret = DFB_FAILURE;
        goto quit;
}

    if ((temp_palette = bdvd_graphics_palette_create(palette->num_entries)) == NULL) {
        D_ERROR( "%s/BCMGfxAllocatePaletteBuffer: error calling bdvd_graphics_palette_create\n",
                 BCMGfxModuleName );
        ret = DFB_FAILURE;
        goto quit;
    }

    palette->ctx = temp_palette;
    palette->entries = (DFBColor *)temp_palette->uncached_addr;

quit:

    if (ret != DFB_OK && temp_palette) {
        if ((err = bdvd_graphics_palette_destroy(temp_palette)) != b_ok) {
            D_ERROR( "%s/BCMGfxAllocatePaletteBuffer: error calling bdvd_graphics_palette_destroy %d\n",
                     BCMGfxModuleName,
                     err );
            /* fall through */
        }
    }

    D_DEBUG("%s/BCMGfxAllocatePaletteBuffer returned with %d\n",
            BCMGfxModuleName,
            ret);

    return ret;
}

static DFBResult BCMGfxDeallocatePaletteBuffer( SurfaceManager             *manager,
                                                CorePalette                *palette )
{
    DFBResult ret = DFB_OK;
    bresult err;

    D_ASSERT(manager != NULL);
    D_ASSERT(palette != NULL);

    D_DEBUG("%s/BCMGfxDeallocatePaletteBuffer called\n",
            BCMGfxModuleName);

    if ((err = bdvd_graphics_palette_destroy((bdvd_graphics_palette_h)palette->ctx)) != b_ok) {
        D_ERROR( "%s/BCMGfxDeallocatePaletteBuffer: error calling bdvd_graphics_surface_destroy %d\n",
                 BCMGfxModuleName,
                 err );
        ret = DFB_FAILURE;
    }

    D_DEBUG("%s/BCMGfxDeallocatePaletteBuffer returned with %d\n",
            BCMGfxModuleName,
            ret);

    return ret;
}

/* exported symbols */

static int
driver_probe( GraphicsDevice *device ) {
    D_DEBUG("%s/driver_probe called\n",
            BCMGfxModuleName);

    switch (dfb_gfxcard_get_accelerator( device )) {
        case 7038:          /* 7038 */
            return 7038;
        case 7438:          /* 7438 */
            return 7438;
    }

    return 7438;
}

static void
driver_get_info( GraphicsDevice     *device,
                 GraphicsDriverInfo *info ) {
    D_DEBUG("%s/driver_get_info called\n",
            BCMGfxModuleName);

    snprintf( info->name, DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH, BCMGfxModuleName );
    snprintf( info->vendor, DFB_GRAPHICS_DRIVER_INFO_VENDOR_LENGTH, "Broadcom Corporation" );
    snprintf( info->url, DFB_GRAPHICS_DRIVER_INFO_URL_LENGTH, "http://www.broadcom.com/" );
    snprintf( info->license, DFB_GRAPHICS_DRIVER_INFO_LICENSE_LENGTH, "proprietary" );

    info->version.major = BCMGfxModuleVersionMajor;
    info->version.minor = BCMGfxModuleVersionMinor;

    info->driver_data_size = sizeof (BCMGfxDriverData);
    info->device_data_size = sizeof (BCMGfxDeviceData);
}

static DFBResult
driver_init_driver( GraphicsDevice      *device,
                    GraphicsDeviceFuncs *funcs,
                    void                *driver_data,
                    void                *device_data )
{
    DFBResult ret = DFB_OK;

    BCMGfxDriverData *bcmgfxdrv = ( BCMGfxDriverData* ) driver_data;

    BCMCoreContext * dfb_bcmcore_context;

    char * getenv_string;

	UNUSED_PARAM(device_data);

    D_ASSERT( bcmgfxdrv != NULL );

    D_DEBUG("%s/driver_init_driver called\n",
            BCMGfxModuleName);

	dfb_bcmcore_context = (BCMCoreContext *)dfb_system_data();

	bcmgfxdrv->driver_compositor = dfb_bcmcore_context->driver_compositor;
	bcmgfxdrv->max_horizontal_downscale = dfb_bcmcore_context->graphics_constraints.max_horizontal_downscale;
	bcmgfxdrv->max_vertical_downscale = dfb_bcmcore_context->graphics_constraints.max_vertical_downscale;
	
	bcmgfxdrv->max_image_size = dfb_bcmcore_context->graphics_constraints.max_M2MC_width;

    getenv_string = getenv( "dfbbcm_driver_compositor_area_threshold" );
    if (getenv_string) {
        bcmgfxdrv->area_hardware_threshold = strtol(getenv_string, NULL, 10);
        unsetenv( "dfbbcm_driver_compositor_area_threshold" );
    }
    else {
		bcmgfxdrv->area_hardware_threshold = 0;
	}

    D_INFO( "%s/driver_init_driver: driver compositor area threshold is %u\n",
            BCMGfxModuleName,
            bcmgfxdrv->area_hardware_threshold );

    getenv_string = getenv( "dfbbcm_clear_surface_memory" );
    if (getenv_string) {
        unsetenv( "dfbbcm_clear_surface_memory" );
        if (!strncmp(getenv_string, "y", 1)) {
            bcmgfxdrv->clear_surface_memory = true;
        }
        else {
            bcmgfxdrv->clear_surface_memory = false;
        }
    }

    D_INFO( "%s/driver_init_driver: surface memory will %sbe cleared\n",
            BCMGfxModuleName,
            bcmgfxdrv->clear_surface_memory ? "" : "not " );

    funcs->EngineReset   = BCMGfxEngineReset;
    funcs->CheckState    = BCMGfxCheckState;
    funcs->WeighArea     = NULL; /* BCMGfxWeighArea */
    funcs->SetState      = BCMGfxSetState;
    funcs->EngineSync    = BCMGfxEngineSync;
    funcs->FillRectangle = BCMGfxFillRectangle;
    funcs->Blit          = BCMGfxBlit;
    funcs->StretchBlit   = BCMGfxStretchBlit;
    funcs->GetBackFieldContext = BCMGetBackFieldContext;

	if (dfb_bcmcore_context->graphics_constraints.requires_cache_flush) {
		funcs->FlushSpecificTextureCache = BCMGfxFlushSpecificTextureCache;
	}

    device->surfacemngmt_funcs.AllocateSurfaceBuffer = BCMGfxAllocateSurfaceBuffer;
    device->surfacemngmt_funcs.DeallocateSurfaceBuffer = BCMGfxDeallocateSurfaceBuffer;
    device->surfacemngmt_funcs.CopySurfaceBuffer = BCMGfxCopySurfaceBuffer;
    device->surfacemngmt_funcs.UpdateSurfaceBuffer = BCMGfxUpdateSurfaceBuffer;
    device->surfacemngmt_funcs.AllocatePaletteBuffer = BCMGfxAllocatePaletteBuffer;
    device->surfacemngmt_funcs.DeallocatePaletteBuffer = BCMGfxDeallocatePaletteBuffer;

	/* temp for BCMGfxAllocateSurfaceBuffer */
	temp_bcmgfxdrv = bcmgfxdrv;

	if (bcmgfxdrv->driver_compositor == NULL) {
        D_ERROR( "%s/driver_init_driver: driver graphics compositor cannot be null\n",
                 BCMGfxModuleName);
        ret = DFB_FAILURE;
        goto quit;
	}

quit:

    return ret;
}

static DFBResult
driver_init_device( GraphicsDevice     *device,
                    GraphicsDeviceInfo *device_info,
                    void               *driver_data,
                    void               *device_data )
{
    D_DEBUG("%s/driver_init_device called\n",
            BCMGfxModuleName);

    /* fill device info */
    snprintf( device_info->name,
              DFB_GRAPHICS_DEVICE_INFO_NAME_LENGTH, "BCM7%s38/xxxAPI", dfb_gfxcard_get_accelerator( device ) == 7038 ? "0" : "4" );

    snprintf( device_info->vendor,
              DFB_GRAPHICS_DEVICE_INFO_VENDOR_LENGTH, "Broadcom Corporation" );

#ifdef DEBUG_ALLOCATION
    show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

    device_info->caps.flags    = CCF_NOTRIEMU;
    /* The underling PI API does not handle clipping so no CCF_CLIPPING */
    /* CCF_NOTRIEMU must be put if we notice that span rasterizing is
     * too slow. It is really slow with async blits, so let's forget about that for now.
     */

    device_info->caps.accel    = BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFUNCTIONS | BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFUNCTIONS; /* For now */
    device_info->caps.drawing  = BCMGFX_DIRECTPF_SUPPORTED_DRAWINGFLAGS; /* For now */
    device_info->caps.blitting = BCMGFX_DIRECTPF_SUPPORTED_BLITTINGFLAGS; /* For now */

    device_info->limits.surface_byteoffset_alignment = 4; /* Maximum byte size is 4 for 32 bpp */
    device_info->limits.surface_pixelpitch_alignment = 0; /* 1 pixel alignement is required */
    device_info->limits.surface_bytepitch_alignment  = 4; /* Don't use */

    return DFB_OK;
}

static void
driver_close_device( GraphicsDevice *device,
                     void           *driver_data,
                     void           *device_data )
{
    D_DEBUG("%s/driver_close_device called\n",
            BCMGfxModuleName);
}

static void
driver_close_driver( GraphicsDevice *device,
                     void           *driver_data )
{
    D_DEBUG("%s/driver_close_driver called\n",
            BCMGfxModuleName);
}
