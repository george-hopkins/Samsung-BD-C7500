/**/

#ifndef ___BCMGFX_H__
#define ___BCMGFX_H__

#include <bdvd.h>

#include <dfb_types.h>
#include <core/coretypes.h>
#include <core/layers.h>

typedef struct {
    CoreSurface *source;
    bool source_rgb;
    CoreSurface *destination;
    bool destination_rgb;

    bool premult_fill_color;

    DFBSurfaceBlendFunction src_blend;
    DFBSurfaceBlendFunction dst_blend;

    DFBColor color;
    bdvd_graphics_pixel_t fill_color;
    bdvd_graphics_pixel_t source_key_value;

    bdvd_graphics_blending_rule_e draw_alpha_blending_rule;
    bdvd_graphics_blending_rule_e draw_color_blending_rule;

    bdvd_graphics_blending_rule_e blit_alpha_blending_rule;
    bdvd_graphics_blending_rule_e blit_color_blending_rule;

    bool enable_fade_effect;
    bool fade_effect_src_prem;
    bool enable_source_key;

    bool blit_deinterlace;

    /* state validation */
    int v_destination;
    int v_color;
    int v_source;
    int v_src_colorkey;
    int v_dst_colorkey;
    int v_drawingflags;
    int v_blittingflags;

} BCMGfxDeviceData;

typedef struct {
	bdvd_graphics_compositor_h driver_compositor;

	unsigned int max_horizontal_downscale;
	unsigned int max_vertical_downscale;
	unsigned int max_image_size;

	unsigned int area_hardware_threshold;

    bool clear_surface_memory;

    BCMGfxDeviceData *device_data;
} BCMGfxDriverData;

#define BCMGFX_COPY_WAIT_TIME   (1000)          /* 1ms   */
#define BCMGFX_COPY_TIMEOUT     (100 * 1000)    /* 100ms */

static const char BCMGfxModuleName[] = "DirectFB/BCMGfx";

#endif
