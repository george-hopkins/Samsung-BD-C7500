
#ifndef __BCMCORE_FCTS_H__
#define __BCMCORE_FCTS_H__

#include <directfb.h>

#include <direct/list.h>
#include <direct/serial.h>

#include <fusion/object.h>
#include <fusion/lock.h>
#include <fusion/reactor.h>

#include <core/coretypes.h>
#include <core/coredefs.h>

#include <core/gfxcard.h>

void dfb_surface_flip_buffers_nonotify( CoreSurface *surface );

void dfb_surface_flip_index_nonotify( DFBSurfaceCapabilities caps, uint32_t * front_index, uint32_t * back_index, uint32_t * idle_index );

DFBResult dfb_surface_create_preallocated_video( CoreDFB *core,
                                                 int width, int height,
                                                 DFBSurfacePixelFormat format,
                                                 DFBSurfaceCapabilities caps, CorePalette *palette,
                                                 SurfaceBuffer * front_buffer, SurfaceBuffer * back_buffer, SurfaceBuffer * idle_buffer,
                                                 CoreSurface **ret_surface );

DFBResult dfb_surface_reformat_preallocated_video( CoreDFB               *core,
                                                   CoreSurface           *surface,
                                                   int                    width,
                                                   int                    height,
                                                   DFBSurfacePixelFormat  format );

DFBResult dfb_surface_reconfig_preallocated_video( CoreSurface           *surface,
                                                   SurfaceBuffer         *front_buffer,
                                                   SurfaceBuffer         *back_buffer,
                                                   SurfaceBuffer         *idle_buffer );

void src2dst_xform(
    DFBRectangle *src_layer_rect, 
    DFBRectangle *dst_layer_rect,
    DFBSurfacePixelFormat src_fmt,
    DFBSurfacePixelFormat dst_fmt,
    DFBRectangle *src_rect, 
    DFBRectangle *dst_rect,
    double *horiz_scale,
    double *vert_scale,
    double *horiz_phase,
    double *vert_phase
);
void dst2src_xform(
    DFBRectangle *src_layer_rect, 
    DFBRectangle *dst_layer_rect,
    DFBSurfacePixelFormat src_fmt,
    DFBSurfacePixelFormat dst_fmt,
    DFBRectangle *src_rect, 
    DFBRectangle *dst_rect,
    double *horiz_scale,
    double *vert_scale,
    double *horiz_phase,
    double *vert_phase
);

void src2display_rect_transform(BCMCoreLayerHandle layer, DFBRectangle * src_rect, DFBRectangle * display_rect);

bdvd_graphics_pixel_format_e
dfb_surface_pixelformat_to_bdvd( DFBSurfacePixelFormat format );

#endif /* __BCMCORE_FCTS_H__ */
