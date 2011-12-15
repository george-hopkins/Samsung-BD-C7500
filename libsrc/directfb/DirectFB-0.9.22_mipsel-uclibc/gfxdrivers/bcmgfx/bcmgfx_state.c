/**/

#include <directfb.h>

#include <direct/mem.h>
#include <direct/debug.h>

#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/state.h>
#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/palette.h>

#include <gfx/convert.h>

#include "bcmgfx_state.h"

#include <unistd.h>

#if D_DEBUG_ENABLED
/* #define ENABLE_MORE_DEBUG */
#endif

void bcmgfx_set_destination( BCMGfxDriverData *bcmgfxdrv,
                             BCMGfxDeviceData *bcmgfxdev,
                             CardState        *state )
{
    if ( bcmgfxdev->v_destination )
    return;

    switch (state->destination->format) {
        case DSPF_YUY2:
        case DSPF_UYVY:
            bcmgfxdev->destination_rgb = false;
        break;
        default:
            bcmgfxdev->destination_rgb = true;
        break;
    }

    bcmgfxdev->destination = state->destination;
    bcmgfxdev->v_destination = 1;
}

bool bcmfgx_is_destination_argb(CoreSurface *destination)
{
    bool is_rgb = false;

    if (destination)
    {
        switch (destination->format)
        {
            case DSPF_YUY2:
            case DSPF_UYVY:
                is_rgb = false;
            break;
            default:
                is_rgb = true;
            break;
        }
    }

    return is_rgb;
}

static void bcmgfx_load_palette( BCMGfxDriverData *bcmgfxdrv,
                                 BCMGfxDeviceData *bcmgfxdev,
                                 CoreSurface      *source ) {
    bresult err;
	uint32_t i;

	for (i = 0; i < 2; i++) {
		if (((bdvd_graphics_surface_h*)source->front_buffer->video.ctx)[i]) {
            if ((err = bdvd_graphics_surface_set_palette(
                    ((bdvd_graphics_surface_h*)source->front_buffer->video.ctx)[i],
                    (bdvd_graphics_palette_h)source->palette->ctx)) != b_ok) {
                D_ERROR( "bcmgfx_load_palette: error calling bdvd_graphics_surface_set_palette %d\n",
                         err );
                /* fall through */
            }
		}
	}
}

void bcmgfx_set_source( BCMGfxDriverData *bcmgfxdrv,
                        BCMGfxDeviceData *bcmgfxdev,
                        CardState        *state )
{
    if ( bcmgfxdev->v_source )
    return;

    /* Source is null for DFXL_FILLRECTANGLE */
    if (state->source) {
        switch (state->source->format) {
            case DSPF_AYUV:
            case DSPF_YUY2:
            case DSPF_UYVY:
                bcmgfxdev->source_rgb = false;
            break;
            case DSPF_ALUT88:
            case DSPF_LUT8:
            case DSPF_LUT4:
            case DSPF_LUT2:
            case DSPF_LUT1:
                /* TODO Must we really do this here? */
                bcmgfx_load_palette(bcmgfxdrv, bcmgfxdev, state->source);
                /* fall through */
            default:
                bcmgfxdev->source_rgb = true;
            break;
        }
    }

    bcmgfxdev->source = state->source;
    bcmgfxdev->v_source = 1;
}

bool bcmfgx_is_source_argb(CoreSurface *source)
{
    bool is_rgb = false;

    if (source)
    {
        switch (source->format) {
            case DSPF_AYUV:
            case DSPF_YUY2:
            case DSPF_UYVY:
                is_rgb = false;
            break;
            case DSPF_ALUT88:
            case DSPF_LUT8:
            case DSPF_LUT4:
            case DSPF_LUT2:
            case DSPF_LUT1:
            default:
                is_rgb = true;
            break;
        }
    }

    return is_rgb;
}

void bcmgfx_set_clip( BCMGfxDriverData *bcmgfxdrv,
                      BCMGfxDeviceData *bcmgfxdev,
                      CardState        *state )
{
/*
    state->clip.y1
    state->clip.x1
    state->clip.y2
    state->clip.x2
*/
//    D_WARN("NOT SUPPORTED BY M2MC???\n");
}

void bcmgfx_set_color( BCMGfxDriverData *bcmgfxdrv,
                       BCMGfxDeviceData *bcmgfxdev,
                       CardState        *state )
{
    if ( bcmgfxdev->v_color )
    return;

    bcmgfxdev->color = state->color;

    switch ( state->destination->format ) {
    case DSPF_ARGB:
    case DSPF_ABGR:
    case DSPF_RGB32:
    case DSPF_A8:
    case DSPF_A1:
    case DSPF_ARGB1555:
    case DSPF_ARGB4444:
    case DSPF_RGB16:
    case DSPF_AYUV:
    case DSPF_YUY2:
    case DSPF_UYVY:
    /* The bdvd_graphics API handles the convertion */
        bcmgfxdev->fill_color = PIXEL_ARGB( state->color.a,
                                            state->color.r,
                                            state->color.g,
                                            state->color.b );
    break;
    case DSPF_ALUT88:
        bcmgfxdev->fill_color = (state->color.a << 8) | (state->color_index & 0xFF);
    break;
    case DSPF_LUT8:
    case DSPF_LUT4:
    case DSPF_LUT2:
    case DSPF_LUT1:
        bcmgfxdev->fill_color = state->color_index;
    break;
    default:
    D_BUG( "unexpected pixelformat!" );
    break;
    }

    bcmgfxdev->v_color = 1;
}

void bcmgfx_set_src_colorkey( BCMGfxDriverData *bcmgfxdrv,
                              BCMGfxDeviceData *bcmgfxdev,
                              CardState        *state )
{
    if ( bcmgfxdev->v_src_colorkey )
    return;

    if (DFB_PIXELFORMAT_IS_INDEXED(state->source->format) &&
        !DFB_PIXELFORMAT_IS_INDEXED(state->destination->format)) {
    	if (state->source->palette && state->src_colorkey < state->source->palette->num_entries) {
    		DFBColor * color = &state->source->palette->entries[state->src_colorkey];
    		bcmgfxdev->source_key_value =
    		    (bgraphics_pixel)PIXEL_ARGB(0, color->r, color->g, color->b);
    	}
    }
    else {
    	bcmgfxdev->source_key_value = state->src_colorkey;
    }

    bcmgfxdev->v_src_colorkey = 1;
}

void bcmgfx_set_dst_colorkey( BCMGfxDriverData *bcmgfxdrv,
                              BCMGfxDeviceData *bcmgfxdev,
                              CardState        *state )
{
    if ( bcmgfxdev->v_dst_colorkey )
    return;

    /*state->dst_colorkey*/
    D_WARN("NOT SUPPORTED FOR NOW\n");

    bcmgfxdev->v_dst_colorkey = 1;
}

bool bcmgfx_check_drawingflags( BCMGfxDriverData *bcmgfxdrv,
                                BCMGfxDeviceData *bcmgfxdev,
                                CardState        *state,
                                bool apply )
{
    /* This should never happed because of the filtering done by
     * BCMGFX_INDEXEDPF_SUPPORTED macros.
     * Asserted only in debug
     */
    D_ASSERT ( !(DFB_PIXELFORMAT_IS_INDEXED(state->destination->format) && (state->drawingflags & ~(DSDRAW_DST_COLORKEY)) != DSDRAW_NOFX) );

    switch (state->drawingflags & ~(DSDRAW_DST_COLORKEY)) {
        case DSDRAW_NOFX:
            /* This should be checked when invoking blit instead, but it's not really possible because DSBLIT_NOFX is zero.
             * Thus the top application should check this condition and pass DSBLIT_SRC_PREMULTIPLY.
             * But this is to accomodate the existing examples.
             */
            if (!DFB_PIXELFORMAT_IS_INDEXED(state->destination->format) &&
                (state->destination->caps & DSCAPS_PREMULTIPLIED)) {
                /* We premult only the color itself, because blending is slower....
                 */
                if (apply)
                    bcmgfxdev->premult_fill_color = true;
            }
            if ((state->src_blend == DSBF_INVDESTALPHA) && (state->dst_blend == DSBF_INVSRCALPHA))
            {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                }
            }
            else
            {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                }
            }
            goto quit;
        break;
        case DSDRAW_BLEND | DSDRAW_SRC_PREMULTIPLY:
            /* Setting the color is useless, we are blending anyway */
            if (((state->src_blend == DSBF_SRCALPHA) || (state->src_blend == DSBF_ONE))
                && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA;
                }
                goto quit;
            }
            else if (((state->src_blend == DSBF_SRCALPHA) || (state->src_blend == DSBF_ONE))
                && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->premult_fill_color = true;
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_INVDESTALPHA) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                }
            }
            else {
                goto error;
            }
        break;
        case DSDRAW_BLEND:
            if ((state->src_blend == DSBF_SRCALPHA) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ONE) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_SRCALPHA) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_ZERO;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ONE) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_INVDESTALPHA) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                    bcmgfxdev->draw_color_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                }
            }
            else {
                goto error;
            }
        break;
        case DSDRAW_SRC_PREMULTIPLY:

            /* We premult only the color itself, because blending is slower....
             */
            if (apply) {
                bcmgfxdev->premult_fill_color = true;
                bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                bcmgfxdev->draw_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
            }
            goto quit;
        break;
        default:
            goto error;
        break;
    }

quit:

#ifdef ENABLE_MORE_DEBUG
    if (apply) {
        D_DEBUG("%s/check_drawingflags: draw_alpha_blending_rule %d draw_color_blending_rule %d\n",
                BCMGfxModuleName,
                bcmgfxdev->draw_alpha_blending_rule,
                bcmgfxdev->draw_color_blending_rule);
    }
#endif

    return true;

error:

    if (apply) {
        bcmgfxdev->draw_alpha_blending_rule = BGFX_RULE_INVALID;
        bcmgfxdev->draw_color_blending_rule = BGFX_RULE_INVALID;
    }

    return false;
}

bool bcmgfx_check_blittingflags( BCMGfxDriverData *bcmgfxdrv,
                                 BCMGfxDeviceData *bcmgfxdev,
                                 CardState        *state,
                                 bool apply )
{
    /* This should never happed because of the filtering done by
     * BCMGFX_INDEXEDPF_SUPPORTED macros.
     * Asserted only in debug
     */
    D_ASSERT ( !(DFB_PIXELFORMAT_IS_INDEXED(state->destination->format) && (state->blittingflags & ~(DSBLIT_SRC_COLORKEY | DSBLIT_DST_COLORKEY | DSBLIT_DEINTERLACE)) != DSBLIT_NOFX) );

    if (apply) {
        bcmgfxdev->enable_fade_effect = false;
        bcmgfxdev->fade_effect_src_prem = false;
        bcmgfxdev->enable_source_key = (state->blittingflags & DSBLIT_SRC_COLORKEY) ? true : false;
        bcmgfxdev->blit_deinterlace = (state->blittingflags & DSBLIT_DEINTERLACE) ? true : false;
    }

    switch (state->blittingflags & ~(DSBLIT_SRC_COLORKEY | DSBLIT_DST_COLORKEY | DSBLIT_DEINTERLACE)) {
        case DSBLIT_NOFX:
            /* This should be checked when invoking blit instead, but it's not really possible because DSBLIT_NOFX is zero.
             * Thus the top application should check this condition and pass DSBLIT_SRC_PREMULTIPLY.
             * But this is to accomodate the existing examples.
             */
            if (!DFB_PIXELFORMAT_IS_INDEXED(state->destination->format) &&
                (state->destination->caps & DSCAPS_PREMULTIPLIED) &&
                !(state->source->caps & DSCAPS_PREMULTIPLIED)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHA_ZERO;
                }
                goto quit;
            }
            else {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                }
                goto quit;
            }
        break;
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTIPLY:
            /* We cannot allow this for now, because we are using a special
             * color matrix to do the fade effect, though we might able to
             * combine this with color space conversion matrixes in the
             * future.
             */
            /* Is checking the driver and not CardState a good idea ? */
            if(bcmgfxdev->source_rgb != bcmgfxdev->destination_rgb)
            {
                /* check card state because state_check happens before state_set and source_rgb and destination_rgb may not be reliable yet */
                if (bcmfgx_is_source_argb(state->source) != bcmfgx_is_destination_argb(state->destination))
                {
                    goto error;
                }
            }

            bcmgfxdev->enable_fade_effect = true;
            bcmgfxdev->fade_effect_src_prem = false;
            /* fall through */
        case DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTIPLY:
            if(!(state->source->format == DSPF_A8 || state->source->format == DSPF_A1)) {
                goto error;
            }

            /* TODO optimize ifs */
            if (((state->src_blend == DSBF_SRCALPHA) || (state->src_blend == DSBF_ONE))
                && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA;
                }
                goto quit;
            }
            else if (((state->src_blend == DSBF_SRCALPHA) || (state->src_blend == DSBF_ONE))
                && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_ZERO;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else {
                goto error;
            }
        break;
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE:
            /* Is checking the driver and not CardState a good idea ? */
            if(bcmgfxdev->source_rgb != bcmgfxdev->destination_rgb)
            {
                /* check card state because state_check happens before state_set and source_rgb and destination_rgb may not be reliable yet */
                if (bcmfgx_is_source_argb(state->source) != bcmfgx_is_destination_argb(state->destination))
                {
                    goto error;
                }
            }

            bcmgfxdev->enable_fade_effect = true;
            bcmgfxdev->fade_effect_src_prem = true;
            /* fall through */
        case DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE:
            if(!(state->source->format == DSPF_A8 || state->source->format == DSPF_A1)) {
                goto error;
            }

            /* TODO optimize ifs */
            if ((state->src_blend == DSBF_SRCALPHA) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_INVSRCALPHA;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ONE) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER_CONSTANTSRC;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ONE) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_SRCALPHA) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHAxCONSTANT_ZERO;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else {
                goto error;
            }
        break;
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_SRC_PREMULTIPLY:
            /* Is checking the driver and not CardState a good idea ? */
            if(bcmgfxdev->source_rgb != bcmgfxdev->destination_rgb)
            {
                /* check card state because state_check happens before state_set and source_rgb and destination_rgb may not be reliable yet */
                if (bcmfgx_is_source_argb(state->source) != bcmfgx_is_destination_argb(state->destination))
                {
                    goto error;
                }
            }

            bcmgfxdev->enable_fade_effect = true;
            bcmgfxdev->fade_effect_src_prem = false;
            /* fall through */
        case DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_SRC_PREMULTIPLY:
            /* TODO optimize ifs */
            if (((state->src_blend == DSBF_SRCALPHA) || (state->src_blend == DSBF_ONE))
                && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHA_INVSRCALPHA;
                }
                goto quit;
            }
            else if (((state->src_blend == DSBF_SRCALPHA) || (state->src_blend == DSBF_ONE))
                && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHA_ZERO;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else {
                goto error;
            }
        break;
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL:
            /* Is checking the driver and not CardState a good idea ? */
            if(bcmgfxdev->source_rgb != bcmgfxdev->destination_rgb)
            {
                /* check card state because state_check happens before state_set and source_rgb and destination_rgb may not be reliable yet */
                if (bcmfgx_is_source_argb(state->source) != bcmfgx_is_destination_argb(state->destination))
                {
                    goto error;
                }
            }

            bcmgfxdev->enable_fade_effect = true;
            bcmgfxdev->fade_effect_src_prem = true;
            /* fall through */
        case DSBLIT_BLEND_ALPHACHANNEL:
            /* TODO optimize ifs */
            if ((state->src_blend == DSBF_SRCALPHA) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHA_INVSRCALPHA;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ONE) && (state->dst_blend == DSBF_INVSRCALPHA)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_SRCOVER;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ONE) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_SRCALPHA) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHA_ZERO;
                }
                goto quit;
            }
            else if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else {
                goto error;
            }
        break;
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTIPLY:
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_COLORIZE:
        case DSBLIT_BLEND_COLORALPHA | DSBLIT_SRC_PREMULTIPLY:
        case DSBLIT_BLEND_COLORALPHA:
            if ((state->src_blend == DSBF_ZERO) && (state->dst_blend == DSBF_ZERO)) {
                if (apply) {
                    bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                    bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_CLEAR;
                }
                goto quit;
            }
            else {
                goto error;
            }
        break;
        case DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTIPLY:
            if(!(state->source->format == DSPF_A8 || state->source->format == DSPF_A1)) {
                goto error;
            }

            if (apply)
                bcmgfxdev->premult_fill_color = true;
        /* fall through */
        case DSBLIT_COLORIZE:
            if(!(state->source->format == DSPF_A8 || state->source->format == DSPF_A1)) {
                goto error;
            }

            if (apply) {
                bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                bcmgfxdev->blit_color_blending_rule = BGFX_RULE_PORTERDUFF_SRC_CONSTANTSRC;
            }
            goto quit;
        break;
        case DSBLIT_SRC_PREMULTIPLY:
            if (apply) {
                bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_PORTERDUFF_SRC;
                bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRCALPHA_ZERO;
            }
            goto quit;
        break;
        case DSBLIT_XOR:
            if (apply) {
                bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_SRC_XOR_DEST;
                bcmgfxdev->blit_color_blending_rule = BGFX_RULE_SRC_XOR_DEST;
            }
            goto quit;
        break;
        default:
            goto error;
        break;
    }

quit:

#ifdef ENABLE_MORE_DEBUG
    if (apply) {
        D_DEBUG("%s/check_blittingflags: blit_alpha_blending_rule %d blit_color_blending_rule %d\n",
                BCMGfxModuleName,
                bcmgfxdev->blit_alpha_blending_rule,
                bcmgfxdev->blit_color_blending_rule);
    }
#endif

    return true;

error:

    if (apply) {
        bcmgfxdev->blit_alpha_blending_rule = BGFX_RULE_INVALID;
        bcmgfxdev->blit_color_blending_rule = BGFX_RULE_INVALID;
    }

    return false;
}

/* Used for the DSDRAW_BLEND */
void bcmgfx_set_drawingflags( BCMGfxDriverData *bcmgfxdrv,
                              BCMGfxDeviceData *bcmgfxdev,
                              CardState        *state )
{
    if ( bcmgfxdev->v_drawingflags )
    return;

    bool old_premult_fill_color = bcmgfxdev->premult_fill_color;

    bcmgfxdev->premult_fill_color = false;

    /* No need to check return value at this point, this should not happen and the Blit op will return false
     */
    bcmgfx_check_drawingflags(bcmgfxdrv, bcmgfxdev, state, true);

    if (old_premult_fill_color != bcmgfxdev->premult_fill_color) {
        bcmgfxdev->v_color = 0;
    }

    bcmgfxdev->v_drawingflags = 1;
}

/* Used for the DSBLIT_BLEND_ALPHACHANNEL, DSBLIT_BLEND_COLORALPHA and DSBLIT_COLORIZE blitting flags. */
void bcmgfx_set_blittingflags( BCMGfxDriverData *bcmgfxdrv,
                               BCMGfxDeviceData *bcmgfxdev,
                               CardState        *state )
{
    if ( bcmgfxdev->v_blittingflags )
    return;

    /* No need to check return value at this point, this should not happen and the Blit op will return false
     */
    bcmgfx_check_blittingflags(bcmgfxdrv, bcmgfxdev, state, true);

    bcmgfxdev->v_blittingflags = 1;
}
