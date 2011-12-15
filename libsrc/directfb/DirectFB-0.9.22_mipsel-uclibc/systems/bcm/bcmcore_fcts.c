//#define T_DEBUG(x...) printf(x)
#define T_DEBUG(x...)

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#include <directfb.h>
#include <directfb_strings.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <fusion/shmalloc.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/layers.h>
#include <core/layers_internal.h>
#include <core/palette.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/system.h>
#include <core/windows_internal.h>

#include <gfx/convert.h>
#include <gfx/util.h>
#include <gfx/clip.h>

#include "bcmlayer.h"
#include "bcmcore_fcts.h"

void dfb_surface_flip_buffers_nonotify( CoreSurface *surface )
{
    SurfaceBuffer * front;

    if (surface->caps & DSCAPS_TRIPLE) {
        front = surface->front_buffer;
        surface->front_buffer = surface->back_buffer;
        surface->back_buffer = surface->idle_buffer;
        surface->idle_buffer = front;
    } else {
        front = surface->front_buffer;
        surface->front_buffer = surface->back_buffer;
        surface->back_buffer = front;
        /* To avoid problems with buffer deallocation */
        surface->idle_buffer = surface->front_buffer;
    }
}

void dfb_surface_flip_index_nonotify( DFBSurfaceCapabilities caps, uint32_t * front_index, uint32_t * back_index, uint32_t * idle_index )
{
    uint32_t front;

    if (caps & DSCAPS_TRIPLE) {
        front = *front_index;
        *front_index = *back_index;
        *back_index = *idle_index;
        *idle_index = front;
    } else {
        front = *front_index;
        *front_index = *back_index;
        *back_index = front;
        /* To avoid problems with buffer deallocation */
        *idle_index = *front_index;
    }
}

DFBResult
dfb_surface_reformat_preallocated_video(
    CoreDFB *core,
    CoreSurface *surface,
    int width, int height,
    DFBSurfacePixelFormat format )
{
     int old_width, old_height;
     DFBSurfacePixelFormat old_format;

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     if (width * (long long) height > 4096*4096)
          return DFB_LIMITEXCEEDED;

     if (surface->front_buffer->flags & SBF_FOREIGN_SYSTEM ||
         surface->back_buffer->flags  & SBF_FOREIGN_SYSTEM)
     {
          return DFB_UNSUPPORTED;
     }

     old_width  = surface->width;
     old_height = surface->height;
     old_format = surface->format;

     surface->width  = width;
     surface->height = height;
     surface->format = format;

     if (width      <= surface->min_width &&
         old_width  <= surface->min_width &&
         height     <= surface->min_height &&
         old_height <= surface->min_height &&
         old_format == surface->format)
     {
          dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT );
          return DFB_OK;
     }

     if (surface->caps & DSCAPS_DEPTH) {
        /* Not supported */
        return DFB_UNSUPPORTED;
     }

     if (DFB_PIXELFORMAT_IS_INDEXED( format ) && !surface->palette) {
          DFBResult    ret;
          CorePalette *palette;

          ret = dfb_palette_create( core, 1 << DFB_COLOR_BITS_PER_PIXEL( format ), &palette );
          if (ret) {
               D_DERROR( ret, "Core/Surface: Could not create a palette with %d entries!\n",
                         1 << DFB_COLOR_BITS_PER_PIXEL( format ) );
          }
          else {
               switch (format) {
                    case DSPF_LUT8:
                         dfb_palette_generate_rgb332_map( palette );
                         break;

                    case DSPF_ALUT44:
                         dfb_palette_generate_rgb121_map( palette );
                         break;

                    default:
                         D_WARN( "unknown indexed format" );
               }

               dfb_surface_set_palette( surface, palette );

               dfb_palette_unref( palette );
          }
     }

     dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT | CSNF_SYSTEM | CSNF_VIDEO );

     return DFB_OK;
}

DFBResult
dfb_surface_create_preallocated_video(
    CoreDFB *core,
    int width, int height,
    DFBSurfacePixelFormat format,
    DFBSurfaceCapabilities caps, CorePalette *palette,
    SurfaceBuffer * front_buffer, SurfaceBuffer * back_buffer, SurfaceBuffer * idle_buffer,
    CoreSurface **ret_surface )
{
     DFBResult    ret;
     CoreSurface *surface;

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     D_DEBUG( "dfb_surface_create_preallocated_video( core %p, size %dx%d, format %d palette %p )\n",
              core, width, height, format, palette );

     if (caps & DSCAPS_DEPTH)
          return DFB_UNSUPPORTED;

     if (width * (long long) height > 4096*4096)
          return DFB_LIMITEXCEEDED;

     surface = dfb_core_create_surface( core );

     ret = dfb_surface_init( core, surface, NULL, width, height, format, caps, palette );
     if (ret) {
          fusion_object_destroy( &surface->object );
          return ret;
     }

     surface->caps |= DSCAPS_VIDEOONLY;

     /* Allocate front buffer. */
     surface->front_buffer = front_buffer;
     if (!surface->front_buffer) {
          fusion_object_destroy( &surface->object );
          return ret;
     }

     /* Allocate back buffer. */
     if (caps & DSCAPS_FLIPPING) {
          surface->back_buffer = back_buffer;
          if (!surface->back_buffer) {
               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }
     else
          surface->back_buffer = surface->front_buffer;

     /* Allocate extra back buffer. */
     if (caps & DSCAPS_TRIPLE) {
          surface->idle_buffer = idle_buffer;
          if (!surface->idle_buffer) {
               dfb_surface_destroy_buffer( surface, surface->back_buffer );
               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }
     else
          surface->idle_buffer = surface->front_buffer;

     fusion_object_activate( &surface->object );

     *ret_surface = surface;

     return DFB_OK;
}

DFBResult
dfb_surface_reconfig_preallocated_video(
    CoreSurface       *surface,
    SurfaceBuffer     *front_buffer,
    SurfaceBuffer     *back_buffer,
    SurfaceBuffer     *idle_buffer )
{
     DFBResult      ret = DFB_FAILURE;
     SurfaceBuffer *front;
     SurfaceBuffer *back;
     SurfaceBuffer *idle;
     SurfaceBuffer *new_front = NULL;
     SurfaceBuffer *new_back  = NULL;
     SurfaceBuffer *new_idle  = NULL;

     D_ASSERT( surface != NULL );
     D_ASSERT( front_buffer != NULL );

     dfb_surfacemanager_lock( surface->manager );

     if (surface->caps & DSCAPS_DEPTH) {
        /* Not supported */
        ret = DFB_UNSUPPORTED;
        goto error;
     }

     if (surface->front_buffer || surface->back_buffer || surface->idle_buffer) {
         while ( surface->front_buffer != front_buffer ) {
             dfb_surface_flip_buffers_nonotify( surface );
         }
     }
     front = surface->front_buffer;
     back  = surface->back_buffer;
     idle  = surface->idle_buffer;

     if ((front_buffer->policy != CSP_VIDEOONLY) &&
     	 (front_buffer->policy != CSP_VIDEOONLY_RESERVED))
     {
        /* Not supported for preallocated_video */
        ret = DFB_UNSUPPORTED;
        goto error;
     }

     if ((front_buffer->flags | back_buffer->flags) & SBF_FOREIGN_SYSTEM) {
        ret = DFB_UNSUPPORTED;
        goto error;
     }

     if (front != front_buffer) {
         new_front = front_buffer;
         if (!new_front) {
             ret = DFB_UNSUPPORTED;
             goto error;
         }
         D_ASSERT(front == NULL);
         front = new_front;
     }

     if (surface->caps & DSCAPS_FLIPPING) {
          D_ASSERT(back_buffer != front_buffer);
          if (back != back_buffer) {
              new_back = back_buffer;
              if (!new_back) {
                   ret = DFB_FAILURE;
                   goto error;
              }
          }
     }
     else {
        if (back != front)
            new_back = front;
     }

     if (surface->caps & DSCAPS_TRIPLE) {
          D_ASSERT(idle_buffer != front_buffer);
          if (idle != idle_buffer) {
              new_idle = idle_buffer;
              if (!new_idle) {
                   ret = DFB_FAILURE;
                   goto error;
              }
          }
     }
     else {
        if (idle != front)
            new_idle = front;
     }

     if (new_front) {
         surface->front_buffer = new_front;
     }

     if (new_back) {
         D_ASSERT(back == NULL);
         surface->back_buffer = new_back;
     }

     if (new_idle) {
         D_ASSERT(idle == NULL);
         surface->idle_buffer = new_idle;
     }

     dfb_surfacemanager_unlock( surface->manager );

     dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT | CSNF_SYSTEM | CSNF_VIDEO );

     return DFB_OK;

error:

     if (new_idle)
          dfb_surface_destroy_buffer( surface, new_idle );

     if (new_back)
          dfb_surface_destroy_buffer( surface, new_back );

     if (new_front)
          dfb_surface_destroy_buffer( surface, new_front );

     dfb_surfacemanager_unlock( surface->manager );

     return ret;
}

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
    double *vert_phase)
{
    DFBRegion clip_rgn;
    DFBRegion src_rgn, dst_rgn;
    int i;

    *horiz_scale = (double)dst_layer_rect->w / (double)src_layer_rect->w;
    *vert_scale = (double)dst_layer_rect->h / (double)src_layer_rect->h;

    T_DEBUG("%s: src_layer_rect (%d, %d, %d, %d)\n", __FUNCTION__,
        src_layer_rect->x, src_layer_rect->y, src_layer_rect->w, src_layer_rect->h);
    T_DEBUG("%s: dst_layer_rect (%d, %d, %d, %d)\n", __FUNCTION__,
        dst_layer_rect->x, dst_layer_rect->y, dst_layer_rect->w, dst_layer_rect->h);
    T_DEBUG("%s: src_rect {%d, %d, %d, %d}\n", __FUNCTION__,
        src_rect->x, src_rect->y, src_rect->w, src_rect->h);
    T_DEBUG("%s: scale (h, v): %lf, %lf\n", __FUNCTION__,
        *horiz_scale, *vert_scale);

    dfb_region_from_rectangle(&src_rgn, src_rect);
    dst_rgn.x1 = dst_layer_rect->x + 
        (int)floor(*horiz_scale * (src_rgn.x1 - src_layer_rect->x));
    dst_rgn.y1 = dst_layer_rect->y + 
        (int)floor(*vert_scale * (src_rgn.y1 - src_layer_rect->y));
    dst_rgn.x2 = dst_layer_rect->x + 
        (int)ceil(*horiz_scale * (src_rgn.x2 - src_layer_rect->x));
    dst_rgn.y2 = dst_layer_rect->y + 
        (int)ceil(*vert_scale * (src_rgn.y2 - src_layer_rect->y));

    src_rgn.x1 = src_layer_rect->x + 
        (int)floor((dst_rgn.x1 - dst_layer_rect->x) / *horiz_scale);
    src_rgn.y1 = src_layer_rect->y + 
        (int)floor((dst_rgn.y1 - dst_layer_rect->y) / *vert_scale);
    src_rgn.x2 = src_layer_rect->x + 
        (int)ceil((dst_rgn.x2 - dst_layer_rect->x) / *horiz_scale);
    src_rgn.y2 = src_layer_rect->y + 
        (int)ceil((dst_rgn.y2 - dst_layer_rect->y) / *vert_scale);

    *horiz_phase =
        modf((dst_rgn.x1 - dst_layer_rect->x) / *horiz_scale, &i);
    *vert_phase =
        modf((dst_rgn.y1 - dst_layer_rect->y) / *vert_scale, &i);
    T_DEBUG("%s: phase (h, v): (%lf, %lf)\n", __FUNCTION__,
        *horiz_phase, *vert_phase);

    if (dst_fmt == DSPF_YUY2 || dst_fmt == DSPF_UYVY) {
        if (dst_rgn.x1 & 1) {
            dst_rgn.x1--;
        }
        if (dst_rgn.y1 & 1) {
            dst_rgn.y1--;
        }
        if (dst_rgn.x2 & 1) {
            dst_rgn.x2++;
        }
        if (dst_rgn.y2 & 1) {
            dst_rgn.y2++;
        }
    }

    if (src_fmt == DSPF_YUY2 || src_fmt == DSPF_UYVY) {
        if (src_rgn.x1 & 1) {
            src_rgn.x1--;
        }
        if (src_rgn.y1 & 1) {
            src_rgn.y1--;
        }
        if (src_rgn.x2 & 1) {
            src_rgn.x2++;
        }
        if (src_rgn.y2 & 1) {
            src_rgn.y2++;
        }
    }
    dfb_rectangle_from_region(src_rect, &src_rgn);
    dfb_rectangle_from_region(dst_rect, &dst_rgn);

    clip_rgn = DFB_REGION_INIT_FROM_RECTANGLE_VALS(dst_layer_rect->x, dst_layer_rect->y, 
        dst_layer_rect->w, dst_layer_rect->h);
    dfb_clip_rectangle(&clip_rgn, dst_rect);

    clip_rgn = DFB_REGION_INIT_FROM_RECTANGLE_VALS(src_layer_rect->x, src_layer_rect->y, 
        src_layer_rect->w, src_layer_rect->h);
    dfb_clip_rectangle(&clip_rgn, src_rect);

    T_DEBUG("%s: src_rect {%d, %d, %d, %d}\n", __FUNCTION__,
        src_rect->x, src_rect->y, src_rect->w, src_rect->h);
    T_DEBUG("%s: dst_rect {%d, %d, %d, %d}\n", __FUNCTION__,
        dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);
}

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
    double *vert_phase)
{
    DFBRegion clip_rgn;
    DFBRegion src_rgn, dst_rgn;
    int i;

    *horiz_scale = (double)dst_layer_rect->w / (double)src_layer_rect->w;
    *vert_scale = (double)dst_layer_rect->h / (double)src_layer_rect->h;

    T_DEBUG("%s: src_layer_rect (%d, %d, %d, %d)\n", __FUNCTION__,
        src_layer_rect->x, src_layer_rect->y, src_layer_rect->w, src_layer_rect->h);
    T_DEBUG("%s: dst_layer_rect (%d, %d, %d, %d)\n", __FUNCTION__,
        dst_layer_rect->x, dst_layer_rect->y, dst_layer_rect->w, dst_layer_rect->h);
    T_DEBUG("%s: dst_rect {%d, %d, %d, %d}\n", __FUNCTION__,
        dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);
    T_DEBUG("%s: scale (h, v): %lf, %lf\n", __FUNCTION__,
        *horiz_scale, *vert_scale);

    dfb_region_from_rectangle(&dst_rgn, dst_rect);
    src_rgn.x1 = src_layer_rect->x + 
        (int)floor((dst_rgn.x1 - dst_layer_rect->x) / *horiz_scale);
    src_rgn.y1 = src_layer_rect->y + 
        (int)floor((dst_rgn.y1 - dst_layer_rect->y) / *vert_scale);
    src_rgn.x2 = src_layer_rect->x + 
        (int)ceil((dst_rgn.x2 - dst_layer_rect->x) / *horiz_scale);
    src_rgn.y2 = src_layer_rect->y + 
        (int)ceil((dst_rgn.y2 - dst_layer_rect->y) / *vert_scale);

    *horiz_phase =
        modf((dst_rgn.x1 - dst_layer_rect->x) / *horiz_scale, &i);
    *vert_phase =
        modf((dst_rgn.y1 - dst_layer_rect->y) / *vert_scale, &i);
    T_DEBUG("%s: phase (h, v): (%lf, %lf)\n", __FUNCTION__,
        *horiz_phase, *vert_phase);

    if (dst_fmt == DSPF_YUY2 || dst_fmt == DSPF_UYVY) {
        if (dst_rgn.x1 & 1) {
            dst_rgn.x1--;
        }
        if (dst_rgn.y1 & 1) {
            dst_rgn.y1--;
        }
        if (dst_rgn.x2 & 1) {
            dst_rgn.x2++;
        }
        if (dst_rgn.y2 & 1) {
            dst_rgn.y2++;
        }
    }

    if (src_fmt == DSPF_YUY2 || src_fmt == DSPF_UYVY) {
        if (src_rgn.x1 & 1) {
            src_rgn.x1--;
        }
        if (src_rgn.y1 & 1) {
            src_rgn.y1--;
        }
        if (src_rgn.x2 & 1) {
            src_rgn.x2++;
        }
        if (src_rgn.y2 & 1) {
            src_rgn.y2++;
        }
    }
    dfb_rectangle_from_region(src_rect, &src_rgn);
    dfb_rectangle_from_region(dst_rect, &dst_rgn);

    clip_rgn = DFB_REGION_INIT_FROM_RECTANGLE_VALS(dst_layer_rect->x, dst_layer_rect->y, 
        dst_layer_rect->w, dst_layer_rect->h);
    dfb_clip_rectangle(&clip_rgn, dst_rect);

    clip_rgn = DFB_REGION_INIT_FROM_RECTANGLE_VALS(src_layer_rect->x, src_layer_rect->y, 
        src_layer_rect->w, src_layer_rect->h);
    dfb_clip_rectangle(&clip_rgn, src_rect);

    T_DEBUG("%s: dst_rect {%d, %d, %d, %d}\n", __FUNCTION__,
        dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);
    T_DEBUG("%s: src_rect {%d, %d, %d, %d}\n", __FUNCTION__,
        src_rect->x, src_rect->y, src_rect->w, src_rect->h);
}

bdvd_graphics_pixel_format_e
dfb_surface_pixelformat_to_bdvd( DFBSurfacePixelFormat format )
{
    bdvd_graphics_pixel_format_e returned_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;

    switch (format) {
        case DSPF_YUY2:
            returned_format = bdvd_graphics_pixel_format_cr8_y18_cb8_y08;
        break;
        case DSPF_UYVY:
            returned_format = bdvd_graphics_pixel_format_y18_cr8_y08_cb8;
        break;
        case DSPF_RGB32:
            returned_format = bdvd_graphics_pixel_format_x8_r8_g8_b8;
        break;
        case DSPF_ARGB:
            returned_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;
        break;
        case DSPF_RGB16:
            returned_format = bdvd_graphics_pixel_format_r5_g6_b5;
        break;
        case DSPF_ARGB1555:
            returned_format = bdvd_graphics_pixel_format_a1_r5_g5_b5;
        break;
        case DSPF_ARGB4444:
            returned_format = bdvd_graphics_pixel_format_a4_r4_g4_b4;
        break;
        case DSPF_LUT8:
            returned_format = bdvd_graphics_pixel_format_palette8;
        break;
        default:
            D_ERROR( "dfb_surface_pixelformat_to_bdvd: format is not supported\n" );
    }

    return returned_format;
}

