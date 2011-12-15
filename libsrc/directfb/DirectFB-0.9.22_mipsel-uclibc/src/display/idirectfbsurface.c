/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>

#include <math.h>


#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/fonts.h>
#include <core/state.h>
#include <core/palette.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <media/idirectfbfont.h>

#include <display/idirectfbsurface.h>
#include <display/idirectfbpalette.h>

#include <misc/util.h>

#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <gfx/convert.h>
#include <gfx/util.h>


static ReactionResult
IDirectFBSurface_listener( const void *msg_data, void *ctx );


void
IDirectFBSurface_Destruct( IDirectFBSurface *thiz )
{
     IDirectFBSurface_data *data = (IDirectFBSurface_data*)thiz->priv;

     if (data->surface)
          dfb_surface_detach( data->surface, &data->reaction );

     dfb_state_set_destination( &data->state, NULL );
     dfb_state_set_source( &data->state, NULL );

     dfb_state_destroy( &data->state );

     if (data->font) {
          IDirectFBFont      *font      = data->font;
          IDirectFBFont_data *font_data = font->priv;

          if (font_data) {
               if (data->surface)
                    dfb_font_drop_destination( font_data->font, data->surface );

               font->Release( font );
          }
          else
               D_WARN( "font dead?" );
     }

     if (data->surface)
          dfb_surface_unref( data->surface );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult
IDirectFBSurface_AddRef( IDirectFBSurface *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Release( IDirectFBSurface *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (--data->ref == 0)
          IDirectFBSurface_Destruct( thiz );

     return DFB_OK;
}


static DFBResult
IDirectFBSurface_GetPixelFormat( IDirectFBSurface      *thiz,
                                 DFBSurfacePixelFormat *format )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!format)
          return DFB_INVARG;

     *format = data->surface->format;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_GetAccelerationMask( IDirectFBSurface    *thiz,
                                      IDirectFBSurface    *source,
                                      DFBAccelerationMask *mask )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!mask)
          return DFB_INVARG;

     dfb_gfxcard_state_check( &data->state, DFXL_FILLRECTANGLE );
     dfb_gfxcard_state_check( &data->state, DFXL_DRAWRECTANGLE );
     dfb_gfxcard_state_check( &data->state, DFXL_DRAWLINE );
     dfb_gfxcard_state_check( &data->state, DFXL_FILLTRIANGLE );

     if (source) {
          IDirectFBSurface_data *src_data = source->priv;

          dfb_state_set_source( &data->state, src_data->surface );

          dfb_gfxcard_state_check( &data->state, DFXL_BLIT );
          dfb_gfxcard_state_check( &data->state, DFXL_STRETCHBLIT );
          dfb_gfxcard_state_check( &data->state, DFXL_TEXTRIANGLES );
     }

     if (data->font) {
          IDirectFBFont_data *font_data = data->font->priv;

          dfb_gfxcard_drawstring_check_state( font_data->font, &data->state );
     }

     *mask = data->state.accel;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_GetSize( IDirectFBSurface *thiz,
                          int              *width,
                          int              *height )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!width && !height)
          return DFB_INVARG;

     if (width)
          *width = data->area.wanted.w;

     if (height)
          *height = data->area.wanted.h;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_GetVisibleRectangle( IDirectFBSurface *thiz,
                                      DFBRectangle     *rect )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!rect)
          return DFB_INVARG;

     rect->x = data->area.current.x - data->area.wanted.x;
     rect->y = data->area.current.y - data->area.wanted.y;
     rect->w = data->area.current.w;
     rect->h = data->area.current.h;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_GetCapabilities( IDirectFBSurface       *thiz,
                                  DFBSurfaceCapabilities *caps )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!caps)
          return DFB_INVARG;

     *caps = data->caps;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_GetPalette( IDirectFBSurface  *thiz,
                             IDirectFBPalette **interface )
{
     DFBResult         ret;
     CoreSurface      *surface;
     IDirectFBPalette *palette;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (!surface->palette)
          return DFB_UNSUPPORTED;

     if (!interface)
          return DFB_INVARG;

     DIRECT_ALLOCATE_INTERFACE( palette, IDirectFBPalette );

     ret = IDirectFBPalette_Construct( palette, surface->palette );
     if (ret)
          return ret;

     *interface = palette;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetPalette( IDirectFBSurface *thiz,
                             IDirectFBPalette *palette )
{
     CoreSurface           *surface;
     IDirectFBPalette_data *palette_data;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (!palette)
          return DFB_INVARG;

     if (! DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          return DFB_UNSUPPORTED;

     palette_data = (IDirectFBPalette_data*) palette->priv;
     if (!palette_data)
          return DFB_DEAD;

     if (!palette_data->palette)
          return DFB_DESTROYED;

     dfb_surface_set_palette( surface, palette_data->palette );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetAlphaRamp( IDirectFBSurface *thiz,
                               __u8              a0,
                               __u8              a1,
                               __u8              a2,
                               __u8              a3 )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     dfb_surface_set_alpha_ramp( data->surface, a0, a1, a2, a3 );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Lock( IDirectFBSurface *thiz,
                       DFBSurfaceLockFlags flags,
                       void **ret_ptr, int *ret_pitch )
{
     DFBResult  ret;
     int        front;
     int        pitch;
     void      *ptr;
     SurfaceBuffer *buffer;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!flags || !ret_ptr || !ret_pitch)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     buffer = (flags & DSLF_WRITE) ? data->surface->back_buffer : data->surface->front_buffer;

     ret = dfb_surface_soft_lock( data->surface, buffer, flags, &ptr, &pitch );
     if (ret)
          return ret;

     ptr += pitch * data->area.current.y +
            DFB_BYTES_PER_LINE( data->surface->format, data->area.current.x );

     data->locked = (buffer == data->surface->front_buffer ? 1 : 0) + 1;

     *ret_ptr   = ptr;
     *ret_pitch = pitch;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Unlock( IDirectFBSurface *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (data->locked)
          dfb_surface_unlock( data->surface, 
               data->locked - 1 ? data->surface->front_buffer : data->surface->back_buffer, 0);

     data->locked = 0;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Flip( IDirectFBSurface    *thiz,
                       const DFBRegion     *region,
                       DFBSurfaceFlipFlags  flags )
{
     DFBRegion    reg;
     CoreSurface *surface;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (data->locked)
          return DFB_LOCKED;

     if (!(surface->caps & DSCAPS_FLIPPING))
          return DFB_UNSUPPORTED;

     if (!data->area.current.w || !data->area.current.h ||
         (region && (region->x1 > region->x2 || region->y1 > region->y2)))
          return DFB_INVAREA;


     dfb_region_from_rectangle( &reg, &data->area.current );

     if (region) {
          DFBRegion clip = DFB_REGION_INIT_TRANSLATED( region,
                                                       data->area.wanted.x,
                                                       data->area.wanted.y );

          if (!dfb_region_region_intersect( &reg, &clip ))
               return DFB_INVAREA;
     }

     if (!(flags & DSFLIP_BLIT) && reg.x1 == 0 && reg.y1 == 0 &&
         reg.x2 == surface->width - 1 && reg.y2 == surface->height - 1)
          dfb_surface_flip_buffers( data->surface, false );
     else
          dfb_back_to_front_copy( data->surface, &reg );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetField( IDirectFBSurface    *thiz,
                           int                  field )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!(data->caps & DSCAPS_INTERLACED))
          return DFB_UNSUPPORTED;

     if (field < 0 || field > 1)
          return DFB_INVARG;

     dfb_surface_set_field( data->surface, field );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Clear( IDirectFBSurface *thiz,
                        __u8 r, __u8 g, __u8 b, __u8 a )
{
     DFBColor                old_color;
     unsigned int            old_index;
     DFBSurfaceDrawingFlags  old_flags;
     CoreSurface            *surface;
     DFBColor                color = { a, r, g, b };

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     /* save current color and drawing flags */
     old_color = data->state.color;
     old_index = data->state.color_index;
     old_flags = data->state.drawingflags;

     /* set drawing flags */
     dfb_state_set_drawing_flags( &data->state, DSDRAW_NOFX );

     /* set color */
     if (DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          dfb_state_set_color_index( &data->state,
                                     dfb_palette_search( surface->palette, r, g, b, a ) );
     else
          dfb_state_set_color( &data->state, &color );

     /* fill the visible rectangle */
     dfb_gfxcard_fillrectangles( &data->area.current, 1, &data->state );

     /* clear the depth buffer */
     if (data->caps & DSCAPS_DEPTH)
          dfb_clear_depth( data->surface, &data->state.clip );

     /* restore drawing flags */
     dfb_state_set_drawing_flags( &data->state, old_flags );

     /* restore color */
     if (DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          dfb_state_set_color_index( &data->state, old_index );
     else
          dfb_state_set_color( &data->state, &old_color );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetClip( IDirectFBSurface *thiz, const DFBRegion *clip )
{
     DFBRegion newclip;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (clip) {
          newclip = DFB_REGION_INIT_TRANSLATED( clip, data->area.wanted.x, data->area.wanted.y );

          if (!dfb_unsafe_region_rectangle_intersect( &newclip,
                                                      &data->area.wanted ))
               return DFB_INVARG;

          data->clip_set = true;
          data->clip_wanted = newclip;

          if (!dfb_region_rectangle_intersect( &newclip, &data->area.current ))
               return DFB_INVAREA;
     }
     else {
          dfb_region_from_rectangle( &newclip, &data->area.current );
          data->clip_set = false;
     }

     dfb_state_set_clip( &data->state, &newclip );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetColor( IDirectFBSurface *thiz,
                           __u8 r, __u8 g, __u8 b, __u8 a )
{
     CoreSurface *surface;
     DFBColor     color = { a, r, g, b };

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     dfb_state_set_color( &data->state, &color );

     if (DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          dfb_state_set_color_index( &data->state,
                                     dfb_palette_search( surface->palette, r, g, b, a ) );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetColorIndex( IDirectFBSurface *thiz,
                                unsigned int      index )
{
     CoreSurface *surface;
     CorePalette *palette;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (! DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          return DFB_UNSUPPORTED;

     palette = surface->palette;
     if (!palette)
          return DFB_UNSUPPORTED;

     if (index > palette->num_entries)
          return DFB_INVARG;

     dfb_state_set_color( &data->state, &palette->entries[index] );

     dfb_state_set_color_index( &data->state, index );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetSrcBlendFunction( IDirectFBSurface        *thiz,
                                      DFBSurfaceBlendFunction  src )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     switch (src) {
          case DSBF_ZERO:
          case DSBF_ONE:
          case DSBF_SRCCOLOR:
          case DSBF_INVSRCCOLOR:
          case DSBF_SRCALPHA:
          case DSBF_INVSRCALPHA:
          case DSBF_DESTALPHA:
          case DSBF_INVDESTALPHA:
          case DSBF_DESTCOLOR:
          case DSBF_INVDESTCOLOR:
          case DSBF_SRCALPHASAT:
               dfb_state_set_src_blend( &data->state, src );
               return DFB_OK;
     }

     return DFB_INVARG;
}

static DFBResult
IDirectFBSurface_SetDstBlendFunction( IDirectFBSurface        *thiz,
                                      DFBSurfaceBlendFunction  dst )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     switch (dst) {
          case DSBF_ZERO:
          case DSBF_ONE:
          case DSBF_SRCCOLOR:
          case DSBF_INVSRCCOLOR:
          case DSBF_SRCALPHA:
          case DSBF_INVSRCALPHA:
          case DSBF_DESTALPHA:
          case DSBF_INVDESTALPHA:
          case DSBF_DESTCOLOR:
          case DSBF_INVDESTCOLOR:
          case DSBF_SRCALPHASAT:
               dfb_state_set_dst_blend( &data->state, dst );
               return DFB_OK;
     }

     return DFB_INVARG;
}

static DFBResult
IDirectFBSurface_SetPorterDuff( IDirectFBSurface         *thiz,
                                DFBSurfacePorterDuffRule  rule )
{
     DFBSurfaceBlendFunction src;
     DFBSurfaceBlendFunction dst;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)


     switch (rule) {
          case DSPD_NONE:
               src = DSBF_SRCALPHA;
               dst = DSBF_INVSRCALPHA;
               break;
          case DSPD_CLEAR:
               src = DSBF_ZERO;
               dst = DSBF_ZERO;
               break;
          case DSPD_SRC:
               src = DSBF_ONE;
               dst = DSBF_ZERO;
               break;
          case DSPD_SRC_OVER:
               src = DSBF_ONE;
               dst = DSBF_INVSRCALPHA;
               break;
          case DSPD_DST_OVER:
               src = DSBF_INVDESTALPHA;
               dst = DSBF_ONE;
               break;
          case DSPD_SRC_IN:
               src = DSBF_DESTALPHA;
               dst = DSBF_ZERO;
               break;
          case DSPD_DST_IN:
               src = DSBF_ZERO;
               dst = DSBF_SRCALPHA;
               break;
          case DSPD_SRC_OUT:
               src = DSBF_INVDESTALPHA;
               dst = DSBF_ZERO;
               break;
          case DSPD_DST_OUT:
               src = DSBF_ZERO;
               dst = DSBF_INVSRCALPHA;
               break;
          case DSPD_XOR:
               src = DSBF_INVDESTALPHA;
               dst = DSBF_INVSRCALPHA;
               break;
          default:
               return DFB_INVARG;
     }

     dfb_state_set_src_blend( &data->state, src );
     dfb_state_set_dst_blend( &data->state, dst );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetSrcColorKey( IDirectFBSurface *thiz,
                                 __u8              r,
                                 __u8              g,
                                 __u8              b )
{
     CoreSurface *surface;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     data->src_key.r = r;
     data->src_key.g = g;
     data->src_key.b = b;

     if (DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          data->src_key.value = dfb_palette_search( surface->palette,
                                                    r, g, b, 0x80 );
     else
          data->src_key.value = dfb_color_to_pixel( surface->format, r, g, b );

     /* The new key won't be applied to this surface's state.
        The key will be taken by the destination surface to apply it
        to its state when source color keying is used. */

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetSrcColorKeyIndex( IDirectFBSurface *thiz,
                                      unsigned int      index )
{
     CoreSurface *surface;
     CorePalette *palette;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (! DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          return DFB_UNSUPPORTED;

     palette = surface->palette;
     if (!palette)
          return DFB_UNSUPPORTED;

     if (index > palette->num_entries)
          return DFB_INVARG;

     data->src_key.r = palette->entries[index].r;
     data->src_key.g = palette->entries[index].g;
     data->src_key.b = palette->entries[index].b;

     data->src_key.value = index;

     /* The new key won't be applied to this surface's state.
        The key will be taken by the destination surface to apply it
        to its state when source color keying is used. */

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetDstColorKey( IDirectFBSurface *thiz,
                                 __u8              r,
                                 __u8              g,
                                 __u8              b )
{
     CoreSurface *surface;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     data->dst_key.r = r;
     data->dst_key.g = g;
     data->dst_key.b = b;

     if (DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          data->dst_key.value = dfb_palette_search( surface->palette,
                                                    r, g, b, 0x80 );
     else
          data->dst_key.value = dfb_color_to_pixel( surface->format, r, g, b );

     dfb_state_set_dst_colorkey( &data->state, data->dst_key.value );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetDstColorKeyIndex( IDirectFBSurface *thiz,
                                      unsigned int      index )
{
     CoreSurface *surface;
     CorePalette *palette;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     if (! DFB_PIXELFORMAT_IS_INDEXED( surface->format ))
          return DFB_UNSUPPORTED;

     palette = surface->palette;
     if (!palette)
          return DFB_UNSUPPORTED;

     if (index > palette->num_entries)
          return DFB_INVARG;

     data->dst_key.r = palette->entries[index].r;
     data->dst_key.g = palette->entries[index].g;
     data->dst_key.b = palette->entries[index].b;

     data->dst_key.value = index;

     dfb_state_set_dst_colorkey( &data->state, data->dst_key.value );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetFont( IDirectFBSurface *thiz,
                          IDirectFBFont    *font )
{
     DFBResult ret;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (data->font != font) {
         if (font) {
              ret = font->AddRef( font );
              if (ret)
                   return ret;
         }

         if (data->font) {
              IDirectFBFont_data *old_data;
              IDirectFBFont      *old = data->font;

              DIRECT_INTERFACE_GET_DATA_FROM( old, old_data, IDirectFBFont );

              dfb_font_drop_destination( old_data->font, data->surface );

              old->Release( old );
         }

         data->font = font;
     }

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_GetFont( IDirectFBSurface  *thiz,
                          IDirectFBFont    **font )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!font)
          return DFB_INVARG;

     if (!data->font) {
	  *font = NULL;
          return DFB_MISSINGFONT;
     }

     data->font->AddRef (data->font);
     *font = data->font;

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetDrawingFlags( IDirectFBSurface       *thiz,
                                  DFBSurfaceDrawingFlags  flags )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     dfb_state_set_drawing_flags( &data->state, flags );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_FillRectangle( IDirectFBSurface *thiz,
                                int x, int y, int w, int h )
{
     DFBRectangle rect = { x, y, w, h };

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (w<=0 || h<=0)
          return DFB_INVARG;

     rect.x += data->area.wanted.x;
     rect.y += data->area.wanted.y;

     dfb_gfxcard_fillrectangles( &rect, 1, &data->state );

     return DFB_OK;
}


static DFBResult
IDirectFBSurface_DrawLine( IDirectFBSurface *thiz,
                           int x1, int y1, int x2, int y2 )
{
     DFBRegion line = { x1, y1, x2, y2 };

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     line.x1 += data->area.wanted.x;
     line.x2 += data->area.wanted.x;
     line.y1 += data->area.wanted.y;
     line.y2 += data->area.wanted.y;

     dfb_gfxcard_drawlines( &line, 1, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_DrawLines( IDirectFBSurface *thiz,
                            const DFBRegion  *lines,
                            unsigned int      num_lines )
{
     DFBRegion *local_lines = alloca(sizeof(DFBRegion) * num_lines);

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!lines || !num_lines)
          return DFB_INVARG;

     if (data->area.wanted.x || data->area.wanted.y) {
          unsigned int i;

          for (i=0; i<num_lines; i++) {
               local_lines[i].x1 = lines[i].x1 + data->area.wanted.x;
               local_lines[i].x2 = lines[i].x2 + data->area.wanted.x;
               local_lines[i].y1 = lines[i].y1 + data->area.wanted.y;
               local_lines[i].y2 = lines[i].y2 + data->area.wanted.y;
          }
     }
     else
          /* clipping may modify lines, so we copy them */
          direct_memcpy( local_lines, lines, sizeof(DFBRegion) * num_lines );

     dfb_gfxcard_drawlines( local_lines, num_lines, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_DrawRectangle( IDirectFBSurface *thiz,
                                int x, int y, int w, int h )
{
     DFBRectangle rect = { x, y, w, h };

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (w<=0 || h<=0)
          return DFB_INVARG;

     rect.x += data->area.wanted.x;
     rect.y += data->area.wanted.y;

     dfb_gfxcard_drawrectangle( &rect, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_FillTriangle( IDirectFBSurface *thiz,
                               int x1, int y1,
                               int x2, int y2,
                               int x3, int y3 )
{
     DFBTriangle tri = { x1, y1, x2, y2, x3, y3 };

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     tri.x1 += data->area.wanted.x;
     tri.y1 += data->area.wanted.y;
     tri.x2 += data->area.wanted.x;
     tri.y2 += data->area.wanted.y;
     tri.x3 += data->area.wanted.x;
     tri.y3 += data->area.wanted.y;

     dfb_gfxcard_filltriangle( &tri, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_FillRectangles( IDirectFBSurface   *thiz,
                                 const DFBRectangle *rects,
                                 unsigned int        num_rects )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!rects || !num_rects)
          return DFB_INVARG;

     if (data->area.wanted.x || data->area.wanted.y) {
          unsigned int  i;
          DFBRectangle *local_rects;
          bool          malloced = (num_rects > 256);

          if (malloced)
               local_rects = malloc( sizeof(DFBRectangle) * num_rects );
          else
               local_rects = alloca( sizeof(DFBRectangle) * num_rects );

          for (i=0; i<num_rects; i++) {
               local_rects[i].x = rects[i].x + data->area.wanted.x;
               local_rects[i].y = rects[i].y + data->area.wanted.y;
               local_rects[i].w = rects[i].w;
               local_rects[i].h = rects[i].h;
          }

          dfb_gfxcard_fillrectangles( local_rects, num_rects, &data->state );

          if (malloced)
               free( local_rects );
     }
     else
          dfb_gfxcard_fillrectangles( rects, num_rects, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_FillSpans( IDirectFBSurface *thiz,
                            int               y,
                            const DFBSpan    *spans,
                            unsigned int      num_spans )
{
     DFBSpan *local_spans = alloca(sizeof(DFBSpan) * num_spans);

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!spans || !num_spans)
          return DFB_INVARG;

     if (data->area.wanted.x || data->area.wanted.y) {
          unsigned int i;

          for (i=0; i<num_spans; i++) {
               local_spans[i].x = spans[i].x + data->area.wanted.x;
               local_spans[i].w = spans[i].w;
          }
     }
     else
          /* clipping may modify spans, so we copy them */
          direct_memcpy( local_spans, spans, sizeof(DFBSpan) * num_spans );

     dfb_gfxcard_fillspans( y + data->area.wanted.y, local_spans, num_spans, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_SetBlittingFlags( IDirectFBSurface        *thiz,
                                   DFBSurfaceBlittingFlags  flags )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     dfb_state_set_blitting_flags( &data->state, flags );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Blit( IDirectFBSurface   *thiz,
                       IDirectFBSurface   *source,
                       const DFBRectangle *sr,
                       int dx, int dy )
{
     DFBRectangle srect;
     IDirectFBSurface_data *src_data;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!source)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;


     src_data = (IDirectFBSurface_data*)source->priv;

     if (!src_data->area.current.w || !src_data->area.current.h)
          return DFB_INVAREA;


     if (sr) {
          if (sr->w < 1  ||  sr->h < 1)
               return DFB_OK;

          srect = *sr;

          srect.x += src_data->area.wanted.x;
          srect.y += src_data->area.wanted.y;

          if (!dfb_rectangle_intersect( &srect, &src_data->area.current ))
               return DFB_INVAREA;

          dx += srect.x - (sr->x + src_data->area.wanted.x);
          dy += srect.y - (sr->y + src_data->area.wanted.y);
     }
     else {
          srect = src_data->area.current;

          dx += srect.x - src_data->area.wanted.x;
          dy += srect.y - src_data->area.wanted.y;
     }

     dfb_state_set_source( &data->state, src_data->surface );

     /* fetch the source color key from the source if necessary */
     if (data->state.blittingflags & DSBLIT_SRC_COLORKEY)
          dfb_state_set_src_colorkey( &data->state, src_data->src_key.value );

     dfb_gfxcard_blit( &srect,
                       data->area.wanted.x + dx,
                       data->area.wanted.y + dy, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_TileBlit( IDirectFBSurface   *thiz,
                           IDirectFBSurface   *source,
                           const DFBRectangle *sr,
                           int dx, int dy )
{
     DFBRectangle srect;
     IDirectFBSurface_data *src_data;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!source)
          return DFB_INVARG;


     src_data = (IDirectFBSurface_data*)source->priv;

     if (!src_data->area.current.w || !src_data->area.current.h)
          return DFB_INVAREA;


     if (sr) {
          if (sr->w < 1  ||  sr->h < 1)
               return DFB_OK;

          srect = *sr;

          srect.x += src_data->area.wanted.x;
          srect.y += src_data->area.wanted.y;

          if (!dfb_rectangle_intersect( &srect, &src_data->area.current ))
               return DFB_INVAREA;

          dx += srect.x - (sr->x + src_data->area.wanted.x);
          dy += srect.y - (sr->y + src_data->area.wanted.y);
     }
     else {
          srect = src_data->area.current;

          dx += srect.x - src_data->area.wanted.x;
          dy += srect.y - src_data->area.wanted.y;
     }

     dfb_state_set_source( &data->state, src_data->surface );

     /* fetch the source color key from the source if necessary */
     if (data->state.blittingflags & DSBLIT_SRC_COLORKEY)
          dfb_state_set_src_colorkey( &data->state, src_data->src_key.value );

     dx %= srect.w;
     if (dx > 0)
          dx -= srect.w;

     dy %= srect.h;
     if (dy > 0)
          dy -= srect.h;

     dx += data->area.wanted.x;
     dy += data->area.wanted.y;

     dfb_gfxcard_tileblit( &srect, dx, dy,
                           dx + data->area.wanted.w + srect.w - 1,
                           dy + data->area.wanted.h + srect.h - 1, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_BatchBlit( IDirectFBSurface   *thiz,
                            IDirectFBSurface   *source,
                            const DFBRectangle *source_rects,
                            const DFBPoint     *dest_points,
                            int                 num )
{
     int                    i, dx, dy, sx, sy;
     DFBRectangle          *rects;
     DFBPoint              *points;
     IDirectFBSurface_data *src_data;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!source || !source_rects || !dest_points || num < 1)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;


     src_data = (IDirectFBSurface_data*)source->priv;

     if (!src_data->area.current.w || !src_data->area.current.h)
          return DFB_INVAREA;

     dx = data->area.wanted.x;
     dy = data->area.wanted.y;

     sx = src_data->area.wanted.x;
     sy = src_data->area.wanted.y;

     rects  = alloca( sizeof(DFBRectangle) * num );
     points = alloca( sizeof(DFBPoint) * num );

     direct_memcpy( rects, source_rects, sizeof(DFBRectangle) * num );
     direct_memcpy( points, dest_points, sizeof(DFBPoint) * num );

     for (i=0; i<num; i++) {
          rects[i].x += sx;
          rects[i].y += sy;

          points[i].x += dx;
          points[i].y += dy;

          if (!dfb_rectangle_intersect( &rects[i], &src_data->area.current ))
               rects[i].w = rects[i].h = 0;

          points[i].x += rects[i].x - (source_rects[i].x + sx);
          points[i].y += rects[i].y - (source_rects[i].y + sy);
     }

     dfb_state_set_source( &data->state, src_data->surface );

     /* fetch the source color key from the source if necessary */
     if (data->state.blittingflags & DSBLIT_SRC_COLORKEY)
          dfb_state_set_src_colorkey( &data->state, src_data->src_key.value );

     dfb_gfxcard_batchblit( rects, points, num, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_StretchBlit( IDirectFBSurface   *thiz,
                              IDirectFBSurface   *source,
                              const DFBRectangle *source_rect,
                              const DFBRectangle *destination_rect )
{
     DFBRectangle srect, drect;
     IDirectFBSurface_data *src_data;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!source)
          return DFB_INVARG;


     src_data = (IDirectFBSurface_data*)source->priv;

     if (!src_data->area.current.w || !src_data->area.current.h)
          return DFB_INVAREA;


     /* do destination rectangle */
     if (destination_rect) {
          if (destination_rect->w < 1  ||  destination_rect->h < 1)
               return DFB_INVARG;

          drect = *destination_rect;

          drect.x += data->area.wanted.x;
          drect.y += data->area.wanted.y;
     }
     else
          drect = data->area.wanted;

     /* do source rectangle */
     if (source_rect) {
          if (source_rect->w < 1  ||  source_rect->h < 1)
               return DFB_INVARG;

          srect = *source_rect;

          srect.x += src_data->area.wanted.x;
          srect.y += src_data->area.wanted.y;
     }
     else
          srect = src_data->area.wanted;


     /* clipping of the source rectangle must be applied to the destination */
     {
          DFBRectangle orig_src = srect;

          if (!dfb_rectangle_intersect( &srect, &src_data->area.current ))
               return DFB_INVAREA;

          if (srect.x != orig_src.x)
               drect.x += (int)( (srect.x - orig_src.x) *
                                 (drect.w / (float)orig_src.w) + 0.5f);

          if (srect.y != orig_src.y)
               drect.y += (int)( (srect.y - orig_src.y) *
                                 (drect.h / (float)orig_src.h) + 0.5f);

          if (srect.w != orig_src.w)
               drect.w = D_ICEIL(drect.w * (srect.w / (float)orig_src.w));
          if (srect.h != orig_src.h)
               drect.h = D_ICEIL(drect.h * (srect.h / (float)orig_src.h));
     }

     dfb_state_set_source( &data->state, src_data->surface );

     /* fetch the source color key from the source if necessary */
     if (data->state.blittingflags & DSBLIT_SRC_COLORKEY)
          dfb_state_set_src_colorkey( &data->state, src_data->src_key.value );

     dfb_gfxcard_stretchblit( &srect, &drect, &data->state );

     return DFB_OK;
}

#define SET_VERTEX(v,X,Y,Z,W,S,T)  \
     do {                          \
          (v)->x = X;              \
          (v)->y = Y;              \
          (v)->z = Z;              \
          (v)->w = W;              \
          (v)->s = S;              \
          (v)->t = T;              \
     } while (0)

static DFBResult
IDirectFBSurface_TextureTriangles( IDirectFBSurface     *thiz,
                                   IDirectFBSurface     *source,
                                   const DFBVertex      *vertices,
                                   const int            *indices,
                                   int                   num,
                                   DFBTriangleFormation  formation )
{
     int                    i;
     DFBVertex             *translated;
     IDirectFBSurface_data *src_data;
     bool                   src_sub;
     float                  x0 = 0;
     float                  y0 = 0;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;


     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!source || !vertices || num < 3)
          return DFB_INVARG;

     src_data = (IDirectFBSurface_data*)source->priv;

     if ((src_sub = (src_data->caps & DSCAPS_SUBSURFACE))) {
          D_ONCE( "sub surface texture not fully working with 'repeated' mapping" );

          x0 = data->area.wanted.x;
          y0 = data->area.wanted.y;
     }

     switch (formation) {
          case DTTF_LIST:
               if (num % 3)
                    return DFB_INVARG;
               break;

          case DTTF_STRIP:
          case DTTF_FAN:
               break;

          default:
               return DFB_INVARG;
     }

     translated = alloca( num * sizeof(DFBVertex) );
     if (!translated)
          return DFB_NOSYSTEMMEMORY;

     /* TODO: pass indices through to driver */
     if (src_sub) {
          float oowidth  = 1.0f / src_data->surface->width;
          float ooheight = 1.0f / src_data->surface->height;

          float s0 = src_data->area.wanted.x * oowidth;
          float t0 = src_data->area.wanted.y * ooheight;

          float fs = src_data->area.wanted.w * oowidth;
          float ft = src_data->area.wanted.h * ooheight;

          for (i=0; i<num; i++) {
               const DFBVertex *in  = &vertices[ indices ? indices[i] : i ];
               DFBVertex       *out = &translated[i];

               SET_VERTEX( out, x0 + in->x, y0 + in->y, in->z, in->w,
                           s0 + fs * in->s, t0 + ft * in->t );
          }
     }
     else {
          if (indices) {
               for (i=0; i<num; i++) {
                    const DFBVertex *in  = &vertices[ indices[i] ];
                    DFBVertex       *out = &translated[i];

                    SET_VERTEX( out, x0 + in->x, y0 + in->y, in->z, in->w, in->s, in->t );
               }
          }
          else {
               direct_memcpy( translated, vertices, num * sizeof(DFBVertex) );

               for (i=0; i<num; i++) {
                    translated[i].x += x0;
                    translated[i].y += y0;
               }
          }
     }

     dfb_state_set_source( &data->state, src_data->surface );

     /* fetch the source color key from the source if necessary */
     if (data->state.blittingflags & DSBLIT_SRC_COLORKEY)
          dfb_state_set_src_colorkey( &data->state, src_data->src_key.value );

     dfb_gfxcard_texture_triangles( translated, num, formation, &data->state );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_DrawString( IDirectFBSurface *thiz,
                             const char *text, int bytes,
                             int x, int y,
                             DFBSurfaceTextFlags flags )
{
     int offset = 0;
     IDirectFBFont_data *font_data;
     DFBFontLayout layout;
     DFBRectangle logical_rect;
     DFBRectangle ink_rect;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!text)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!data->font)
          return DFB_MISSINGFONT;


     if (bytes < 0)
          bytes = strlen (text);

     if (bytes == 0)
          return DFB_OK;

     data->font->GetTextLayout(data->font, &layout);
     data->font->GetStringExtents(data->font, text, bytes, &logical_rect, &ink_rect);


     if (layout & DFFL_VERTICAL) {
          if (flags & DSTF_LEFT) {
               x += ink_rect.w >> 1;
          }
          else if (flags & DSTF_RIGHT) {
               x -= ink_rect.w >> 1;
          }

          if (layout == DFFL_BOTTOM_TO_TOP) {
               if (flags & DSTF_BOTTOM)
                    y += ink_rect.h;
               else if (flags & DSTF_CENTER)
                    y += ink_rect.h >> 1;
          }
          else {
               if (flags & DSTF_BOTTOM)
                    y -= ink_rect.h;
               else if (flags & DSTF_CENTER)
                    y -= ink_rect.h >> 1;
          }
     }
     else {
          if (flags & DSTF_BOTTOM) {
               data->font->GetDescender (data->font, &offset);
               y += offset;
          }
          else if (flags & DSTF_TOP) {
               data->font->GetAscender (data->font, &offset);
               y += offset;
          }

          if (layout == DFFL_RIGHT_TO_LEFT) {
               if (flags & DSTF_RIGHT)
                    x += ink_rect.w;
               else if (flags & DSTF_CENTER)
                    x += ink_rect.w >> 1;
          }
          else {
               if (flags & DSTF_RIGHT)
                    x -= ink_rect.w;
               else if (flags & DSTF_CENTER)
                    x -= ink_rect.w >> 1;
          }
     }

     font_data = (IDirectFBFont_data *)data->font->priv;

     dfb_gfxcard_drawstring( (const unsigned char*) text, bytes,
                             data->area.wanted.x + x, data->area.wanted.y + y,
                             font_data->font, &data->state, flags );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_DrawGlyph( IDirectFBSurface *thiz,
                            unsigned int index, int x, int y,
                            DFBSurfaceTextFlags flags )
{
     int offset = 0;
     int advance = 0;
     DFBFontLayout layout;
     DFBRectangle ink_rect;

     IDirectFBFont_data *font_data;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!index)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->locked)
          return DFB_LOCKED;

     if (!data->font)
          return DFB_MISSINGFONT;

     data->font->GetTextLayout(data->font, &layout);
     data->font->GetGlyphExtents(data->font, index, &ink_rect, &advance);

     if (layout & DFFL_VERTICAL) {
          if (flags & DSTF_LEFT) {
               x += ink_rect.w >> 1;
          }
          else if (flags & DSTF_RIGHT) {
               x -= ink_rect.w >> 1;
          }

          if (layout == DFFL_BOTTOM_TO_TOP) {
               if (flags & DSTF_BOTTOM)
                    y += ink_rect.h;
               else if (flags & DSTF_CENTER)
                    y += ink_rect.h >> 1;
          }
          else {
               if (flags & DSTF_BOTTOM)
                    y -= ink_rect.h;
               else if (flags & DSTF_CENTER)
                    y -= ink_rect.h >> 1;
          }
     }
     else {
          if (flags & DSTF_BOTTOM) {
               data->font->GetDescender (data->font, &offset);
               y += offset;
          }
          else if (flags & DSTF_TOP) {
               data->font->GetAscender (data->font, &offset);
               y += offset;
          }

          if (layout == DFFL_RIGHT_TO_LEFT) {
               if (flags & DSTF_RIGHT)
                    x += ink_rect.w;
               else if (flags & DSTF_CENTER)
                    x += ink_rect.w >> 1;
          }
          else {
               if (flags & DSTF_RIGHT)
                    x -= ink_rect.w;
               else if (flags & DSTF_CENTER)
                    x -= ink_rect.w >> 1;
          }
     }

     font_data = (IDirectFBFont_data *)data->font->priv;

     dfb_gfxcard_drawglyph( index,
                            data->area.wanted.x + x, data->area.wanted.y + y,
                            font_data->font, &data->state, flags );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_CreatePPM( IDirectFBSurface* thiz,
                            unsigned int scale_numerator,
                            unsigned int scale_denominator,
                            unsigned int *out_buffer,
                            unsigned int out_buffer_size,
                            unsigned int *stream_length)
{
    DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

    DFBResult err = DFB_OK;
    bool scale_needed = false;
    bool format_conversion_needed = false;

    IDirectFBSurface *source_surface = thiz;
    DFBSurfacePixelFormat source_format = DSPF_UNKNOWN;
    unsigned int  source_width = 0;
    unsigned int  source_height = 0;
    unsigned int  source_buffer_pitch;
    void         *source_buffer_start = NULL;
    unsigned int *source_buffer_ptr = NULL;
    unsigned char  *out_argb_buffer_ptr = NULL;

    IDirectFBSurface *temp_surface = NULL;
    unsigned int temp_value = 0;

    unsigned int MAX_PPM_HEADER_LENGTH = 30;
    unsigned int x;
    unsigned int y;
    unsigned short widthSwap;
    unsigned short heightSwap;

    if (data->locked)
        return DFB_LOCKED;

    if (source_surface == NULL) {
        D_ERROR("%s: source is NULL!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    if ((scale_numerator < 1) || (scale_denominator < 1) ||
        (scale_numerator > 16) || (scale_denominator > 16) ||
        (scale_numerator > scale_denominator)) {
        D_ERROR("%s: invalid scale_numerator/scale_denominator!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    if (!data->area.current.w || !data->area.current.h) {
        D_ERROR("%s: invalid area!\n", __FUNCTION__);
        err = DFB_INVAREA;
        goto quit;
    }

    /* Get the source surface pixel format */
    err = source_surface->GetPixelFormat( source_surface, &source_format );
    if (err != DFB_OK) {
        D_ERROR("%s: GetPixelFormat failed!\n", __FUNCTION__);
        goto quit;
    }

    /* Determine if we need to convert the surface pixel format */
    if (source_format != DSPF_ARGB) {
        format_conversion_needed = true;
        source_format = DSPF_ARGB;
    }

    /* Get the source surface size */
    err = source_surface->GetSize( source_surface, &source_width, &source_height );
    if (err != DFB_OK) {
        D_ERROR("%s: GetSize failed!\n", __FUNCTION__);
        goto quit;
    }

    /* Determine if we need to scale the output image */
    if ((scale_denominator > 1) || (scale_numerator > 1)) {
        source_width = (source_width * scale_numerator) / scale_denominator;
        source_height = (source_height * scale_numerator) / scale_denominator;
        scale_needed = true;
    }

    *stream_length = MAX_PPM_HEADER_LENGTH + ((source_width * source_height) * 3);

    /* Ensure the buffer provided will be able to hold the entire PPM image */
    if (*stream_length > out_buffer_size){
        /* If the buffer size is 0, assume the caller is just looking to get the size before providing the buffer */
        if (out_buffer_size != 0)
            D_ERROR("%s: buffer provided is too small!\n", __FUNCTION__);
        err = DFB_LIMITEXCEEDED;
        goto quit;
    }

    if ((out_buffer == NULL) || (stream_length == NULL)) {
        D_ERROR("%s: output buffer must be provided!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    if ((source_width < 4) || (source_height < 8)) {
        D_ERROR("%s: cannot encode an image smaller than 4x8!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    /* If needed, blit to a temporary surface to get ready for PPM output */
    if (format_conversion_needed || scale_needed) {
        CoreSurface *surface;

        /* Allocate a temporary buffer to allow scaling, pixel format conversion */
        err = dfb_surface_create( NULL,
                                  NULL,
                                  source_width,
                                  source_height,
                                  source_format,
                                  CSP_VIDEOONLY, DSCAPS_NONE, NULL,
                                  &surface );

        if (err != DFB_OK) {
            D_ERROR("%s: unable to allocate temporary surface!\n", __FUNCTION__);
            goto quit;
        }

        /* Make a DFB interface for this surface to we can use standard function calls */
        DIRECT_ALLOCATE_INTERFACE( temp_surface, IDirectFBSurface );

        err = IDirectFBSurface_Construct( temp_surface, NULL, NULL, surface, DSCAPS_VIDEOONLY );
        if (err != DFB_OK) {
            D_ERROR("%s: unable to allocate temporary surface interface!\n", __FUNCTION__);
            goto quit;
        }

        dfb_surface_unref( surface );

        /* If we are scaling we need to call StretchBlit */
        if (scale_needed) {
            err = temp_surface->StretchBlit( temp_surface, source_surface, NULL, NULL );
            if (err != DFB_OK) {
                D_ERROR("%s: stretch blit failed!\n", __FUNCTION__);
                goto quit;
            }
        }
        else {
            err = temp_surface->Blit( temp_surface, source_surface, NULL, 0, 0);
            if (err != DFB_OK) {
                D_ERROR("%s: blit failed!\n", __FUNCTION__);
                goto quit;
            }
        }

        /* Set the source_surface to the newly created surface.*/
        source_surface = temp_surface;
    }

    /* Lock the surface to output the PPM data */
    err = source_surface->Lock( source_surface, DSLF_READ, &source_buffer_start, &source_buffer_pitch );
    if (err != DFB_OK) {
        D_ERROR("%s: Lock failed!\n", __FUNCTION__);
        goto quit;
    }

    /* Write header information */
    snprintf((char *)out_buffer, MAX_PPM_HEADER_LENGTH, "P6\n%d %d\n255\n", source_width, source_height);

    /* Write the PPM data */
    out_argb_buffer_ptr = (unsigned char *)out_buffer + MIN(strlen((char *)out_buffer), MAX_PPM_HEADER_LENGTH);
    for (y = 0; y < source_height; y++) {
        source_buffer_ptr = (unsigned int*)((unsigned char*)source_buffer_start + (y * source_buffer_pitch));
        for (x = 0; x < source_width; x++) {
            temp_value = *source_buffer_ptr++;
            *out_argb_buffer_ptr++ = (temp_value >> 16) & 0xFF;
            *out_argb_buffer_ptr++ = (temp_value >> 8) & 0xFF;
            *out_argb_buffer_ptr++ = (temp_value >> 0) & 0xFF;
            /**out_argb_buffer_ptr++ = (temp_value >> 24) & 0xFF;*/
        }
    }

    /* All done, unlock the surface */
    err = source_surface->Unlock( source_surface );
    if (err != DFB_OK) {
        D_ERROR("%s: Unlock failed!\n", __FUNCTION__);
        goto quit;
    }

quit:

    /* Release temporary surface if it was created */
    if (temp_surface)
        temp_surface->Release(temp_surface);

    return err;
}



static DFBResult
IDirectFBSurface_CreateCIF( IDirectFBSurface* thiz,
                            unsigned int scale_numerator,
                            unsigned int scale_denominator,
                            unsigned int *out_buffer,
                            unsigned int out_buffer_size,
                            unsigned int *stream_length)
{
    DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

    static const int FILE_ID_SIZE = 8;
    static const int FILE_ID_OFFSET = 0;
    static const int VERSION_SIZE = 1;
    static const int VERSION_OFFSET = 9;
    static const int ENCODE_TYPE_SIZE = 1;
    static const int ENCODE_TYPE_OFFSET = 10;
    static const int WIDTH_SIZE = 2;
    static const int WIDTH_OFFSET = 11;
    static const int HEIGHT_SIZE = 2;
    static const int HEIGHT_OFFSET = 13;
    static const int CIF_HEADER_SIZE = 64;
    static const int CIF_TYPE_UNKNOWN = 0x00;
    static const int CIF_TYPE_CVF = 0x01;
    static const int CIF_TYPE_CDF = 0x02;
    static const int CIF_VERSION = 0x10;

    DFBResult err = DFB_OK;
    bool scale_needed = false;
    bool format_conversion_needed = false;

    IDirectFBSurface *source_surface = thiz;
    DFBSurfacePixelFormat source_format = DSPF_UNKNOWN;
    unsigned int  source_width = 0;
    unsigned int  source_height = 0;
    unsigned int  source_buffer_pitch;
    void         *source_buffer_start = NULL;
    unsigned int *source_buffer_ptr = NULL;

    unsigned char  *out_header_start = (unsigned char *)out_buffer;
    unsigned int   *out_buffer_start = (unsigned int *)((unsigned char *)out_buffer + CIF_HEADER_SIZE);
    unsigned char  *out_argb_buffer_ptr = NULL;
    unsigned char  *out_y_buffer_ptr = NULL;
    unsigned char  *out_cb_buffer_ptr = NULL;
    unsigned char  *out_cr_buffer_ptr = NULL;

    unsigned char version = CIF_VERSION;
    unsigned char encoding_type = CIF_TYPE_UNKNOWN;

    IDirectFBSurface *temp_surface = NULL;
    unsigned int temp_value = 0;

    unsigned int x;
    unsigned int y;
    unsigned short widthSwap;
    unsigned short heightSwap;

    if (data->locked)
        return DFB_LOCKED;

    if (source_surface == NULL) {
        D_ERROR("%s: source is NULL!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    if ((scale_numerator < 1) || (scale_denominator < 1) ||
        (scale_numerator > 16) || (scale_denominator > 16) ||
        (scale_numerator > scale_denominator)) {
        D_ERROR("%s: invalid scale_numerator/scale_denominator!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    if (!data->area.current.w || !data->area.current.h) {
        D_ERROR("%s: invalid area!\n", __FUNCTION__);
        err = DFB_INVAREA;
        goto quit;
    }

    /* Get the source surface pixel format */
    err = source_surface->GetPixelFormat( source_surface, &source_format );
    if (err != DFB_OK) {
        D_ERROR("%s: GetPixelFormat failed!\n", __FUNCTION__);
        goto quit;
    }

    /* Determine if we need to convert the surface pixel format */
    if ((source_format != DSPF_ARGB) && (source_format != DSPF_UYVY)) {
        switch(source_format) {
            case DSPF_YUY2:
            case DSPF_UYVY:
            case DSPF_I420:
            case DSPF_YV12:
            case DSPF_NV12:
            case DSPF_NV16:
            case DSPF_NV21:
            case DSPF_AYUV:
                source_format = DSPF_UYVY;
                break;
            default:
                source_format = DSPF_ARGB;
                break;
        }
        format_conversion_needed = true;
    }

    /* Get the source surface size */
    err = source_surface->GetSize( source_surface, &source_width, &source_height );
    if (err != DFB_OK) {
        D_ERROR("%s: GetSize failed!\n", __FUNCTION__);
        goto quit;
    }

    /* Determine if we need to scale the output image */
    if ((scale_denominator > 1) || (scale_numerator > 1)) {
        source_width = (source_width * scale_numerator) / scale_denominator;
        source_height = (source_height * scale_numerator) / scale_denominator;
        scale_needed = true;
    }

    /* Determine the output CIF size and format type */
    if (source_format == DSPF_UYVY) {
        /* If we are using CVF, make sure we have an even width/height.  This simplifies things a lot */
        source_width = (source_width & ~0x1);
        source_height = (source_height & ~0x1);

        *stream_length = CIF_HEADER_SIZE + (source_width * source_height) * 2;
        encoding_type = CIF_TYPE_CVF;
    }
    else if (source_format == DSPF_ARGB) {
        *stream_length = CIF_HEADER_SIZE + (source_width * source_height) * 4;
        encoding_type = CIF_TYPE_CDF;
    }
    else {
        D_ERROR("%s: format error!\n", __FUNCTION__);
        err = DFB_FAILURE;
        goto quit;
    }

    /* Ensure the buffer provided will be able to hold the entire CIF image */
    if (*stream_length > out_buffer_size){
        /* If the buffer size is 0, assume the caller is just looking to get the size before providing the buffer */
        if (out_buffer_size != 0)
            D_ERROR("%s: buffer provided is too small!\n", __FUNCTION__);
        err = DFB_LIMITEXCEEDED;
        goto quit;
    }

    if ((out_buffer == NULL) || (stream_length == NULL)) {
        D_ERROR("%s: output buffer must be provided!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    if ((source_width < 4) || (source_height < 8)) {
        D_ERROR("%s: cannot encode an image smaller than 4x8!\n", __FUNCTION__);
        err = DFB_INVARG;
        goto quit;
    }

    /* If needed, blit to a temporary surface to get ready for CIF output */
    if (format_conversion_needed || scale_needed) {
        CoreSurface *surface;

        /* Allocate a temporary buffer to allow scaling, pixel format conversion */
        err = dfb_surface_create( NULL,
                                  NULL,
                                  source_width,
                                  source_height,
                                  source_format,
                                  CSP_VIDEOONLY, DSCAPS_NONE, NULL,
                                  &surface );

        if (err != DFB_OK) {
            D_ERROR("%s: unable to allocate temporary surface!\n", __FUNCTION__);
            goto quit;
        }

        /* Make a DFB interface for this surface to we can use standard function calls */
        DIRECT_ALLOCATE_INTERFACE( temp_surface, IDirectFBSurface );

        err = IDirectFBSurface_Construct( temp_surface, NULL, NULL, surface, DSCAPS_VIDEOONLY );
        if (err != DFB_OK) {
            D_ERROR("%s: unable to allocate temporary surface interface!\n", __FUNCTION__);
            goto quit;
        }

        dfb_surface_unref( surface );

        /* If we are scaling we need to call StretchBlit */
        if (scale_needed) {
            err = temp_surface->StretchBlit( temp_surface, source_surface, NULL, NULL );
            if (err != DFB_OK) {
                D_ERROR("%s: stretch blit failed!\n", __FUNCTION__);
                goto quit;
            }
        }
        else {
            err = temp_surface->Blit( temp_surface, source_surface, NULL, 0, 0);
            if (err != DFB_OK) {
                D_ERROR("%s: blit failed!\n", __FUNCTION__);
                goto quit;
            }
        }

        /* Set the source_surface to the newly created surface.  We will use this
           for CIF output */
        source_surface = temp_surface;
    }

    /* Lock the surface to output the CIF data */
    err = source_surface->Lock( source_surface, DSLF_READ, &source_buffer_start, &source_buffer_pitch );
    if (err != DFB_OK) {
        D_ERROR("%s: Lock failed!\n", __FUNCTION__);
        goto quit;
    }

    /* Write header information */
    widthSwap = (((source_width & 0xFF00) >> 8) | ((source_width & 0x00FF) << 8));
    heightSwap = (((source_height & 0xFF00) >> 8) | ((source_height & 0x00FF) << 8));
    memset(out_header_start, 0, CIF_HEADER_SIZE);
    memcpy(&out_header_start[FILE_ID_OFFSET], "HDDVDCIF", FILE_ID_SIZE);
    memcpy(&out_header_start[VERSION_OFFSET], &version, VERSION_SIZE);
    memcpy(&out_header_start[ENCODE_TYPE_OFFSET], &encoding_type, ENCODE_TYPE_SIZE);
    memcpy(&out_header_start[WIDTH_OFFSET], &widthSwap, WIDTH_SIZE);
    memcpy(&out_header_start[HEIGHT_OFFSET], &heightSwap, HEIGHT_SIZE);

    /* Write the CIF data */
    switch(source_format){
        case DSPF_ARGB:
            out_argb_buffer_ptr = (unsigned char *)out_buffer_start;
            for (y = 0; y < source_height; y++) {
                source_buffer_ptr = (unsigned int*)((unsigned char*)source_buffer_start + (y * source_buffer_pitch));
                for (x = 0; x < source_width; x++) {
                    temp_value = *source_buffer_ptr++;
                    *out_argb_buffer_ptr++ = (temp_value >> 0) & 0xFF;
                    *out_argb_buffer_ptr++ = (temp_value >> 8) & 0xFF;
                    *out_argb_buffer_ptr++ = (temp_value >> 16) & 0xFF;
                    *out_argb_buffer_ptr++ = (temp_value >> 24) & 0xFF;
                }
            }
            break;

        case DSPF_UYVY:
            out_y_buffer_ptr = (unsigned char *)out_buffer_start;
            out_cr_buffer_ptr = out_y_buffer_ptr + (source_width * source_height);
            out_cb_buffer_ptr = out_cr_buffer_ptr + (source_width/2 * source_height/2);
            for (y = 0; y < source_height; y++) {
                source_buffer_ptr = (unsigned int*)((unsigned char*)source_buffer_start + (y * source_buffer_pitch));
                if (y & 0x1) {
                    for (x = 0; x < source_width/2; x++) {
                        temp_value = *source_buffer_ptr++;
                        *out_y_buffer_ptr++ = (temp_value >> 24) & 0xFF;
                        *out_y_buffer_ptr++ = (temp_value >> 8) & 0xFF;
                    }
                }
                else {
                    for (x = 0; x < source_width/2; x++) {
                        temp_value = *source_buffer_ptr++;
                        *out_y_buffer_ptr++ = (temp_value >> 24) & 0xFF;
                        *out_y_buffer_ptr++ = (temp_value >> 8) & 0xFF;
                        *out_cb_buffer_ptr++ = (temp_value >> 0) & 0xFF;
                        *out_cr_buffer_ptr++ = (temp_value >> 16) & 0xFF;
                    }
                }
            }
            break;

        default:
            D_ERROR("%s: unknown pixel format!\n", __FUNCTION__);
            err = DFB_FAILURE;
            goto quit;
            break;
    }

    /* All done, unlock the surface */
    err = source_surface->Unlock( source_surface );
    if (err != DFB_OK) {
        D_ERROR("%s: Unlock failed!\n", __FUNCTION__);
        goto quit;
    }

quit:

    /* Release temporary surface if it was created */
    if (temp_surface)
        temp_surface->Release(temp_surface);

    return err;
}


static DFBResult
IDirectFBSurface_GetSubSurface( IDirectFBSurface    *thiz,
                                const DFBRectangle  *rect,
                                IDirectFBSurface   **surface )
{
     DFBRectangle wanted, granted;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     /* Check arguments */
     if (!data->surface)
          return DFB_DESTROYED;

     if (!surface)
          return DFB_INVARG;

     /* Compute wanted rectangle */
     if (rect) {
          wanted = *rect;

          wanted.x += data->area.wanted.x;
          wanted.y += data->area.wanted.y;

          if (wanted.w <= 0 || wanted.h <= 0) {
               wanted.w = 0;
               wanted.h = 0;
          }
     }
     else
          wanted = data->area.wanted;

     /* Compute granted rectangle */
     granted = wanted;

     dfb_rectangle_intersect( &granted, &data->area.granted );

     /* Allocate and construct */
     DIRECT_ALLOCATE_INTERFACE( *surface, IDirectFBSurface );

     return IDirectFBSurface_Construct( *surface, &wanted, &granted,
                                        data->surface,
                                        data->caps | DSCAPS_SUBSURFACE );
}

static DFBResult
IDirectFBSurface_GetGL( IDirectFBSurface   *thiz,
                        IDirectFBGL       **interface )
{
     DFBResult ret;
     DirectInterfaceFuncs *funcs = NULL;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_DESTROYED;

     if (!interface)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;


     ret = DirectGetInterface( &funcs, "IDirectFBGL", NULL, NULL, NULL );
     if (ret)
          return ret;

     ret = funcs->Allocate( (void**)interface );
     if (ret)
          return ret;

     ret = funcs->Construct( *interface, thiz );
     if (ret)
          *interface = NULL;

     return ret;
}

static DFBResult
IDirectFBSurface_Dump( IDirectFBSurface   *thiz,
                       const char         *directory,
                       const char         *prefix )
{
     CoreSurface *surface;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!directory || !prefix)
          return DFB_INVARG;

     if (!data->area.current.w || !data->area.current.h)
          return DFB_INVAREA;

     if (data->caps & DSCAPS_SUBSURFACE) {
          D_ONCE( "sub surface dumping not supported yet" );
          return DFB_UNSUPPORTED;
     }

     surface = data->surface;
     if (!surface)
          return DFB_DESTROYED;

     return dfb_surface_dump( surface, directory, prefix );
}


DFBResult
IDirectFBSurface_GetBackBufferContext( IDirectFBSurface *thiz, void **ctx )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_INVARG;

     dfb_gfxcard_surface_back_field_context(data->surface, ctx);

     return DFB_OK;
}


DFBResult
IDirectFBSurface_HardwareLock( IDirectFBSurface *thiz )
{
     DFBResult ret;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface)

     if (!data->surface)
          return DFB_INVARG;

     ret = dfb_surface_hardware_lock( data->surface, data->surface->back_buffer, DSLF_WRITE );
     if (ret == DFB_OK)
     {
          data->locked=1;
     }

     return ret;
}


DFBResult
IDirectFBSurface_ResetSource( IDirectFBSurface         *thiz )
{
     IDirectFBSurface_data *data = (IDirectFBSurface_data*)thiz->priv;
     dfb_state_set_source( &data->state, NULL );
     return DFB_OK;
}



/******/

DFBResult IDirectFBSurface_Construct( IDirectFBSurface       *thiz,
                                      DFBRectangle           *wanted,
                                      DFBRectangle           *granted,
                                      CoreSurface            *surface,
                                      DFBSurfaceCapabilities  caps )
{
     DFBRectangle rect = { 0, 0, surface->width, surface->height };

     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBSurface)

     data->ref = 1;
     data->caps = caps | surface->caps;

     if (dfb_surface_ref( surface )) {
          DIRECT_DEALLOCATE_INTERFACE(thiz);
          return DFB_FAILURE;
     }

     /* The area that was requested */
     if (wanted)
          data->area.wanted = *wanted;
     else
          data->area.wanted = rect;

     /* The area that will never be exceeded */
     if (granted)
          data->area.granted = *granted;
     else
          data->area.granted = data->area.wanted;

     /* The currently accessible rectangle */
     data->area.current = data->area.granted;
     dfb_rectangle_intersect( &data->area.current, &rect );

     data->surface = surface;

#ifdef DEBUG_ALLOCATION
     if (surface)
     {
        surface->dbg_ul_surface_handle = (void *)thiz;
     }
#endif

     dfb_state_init( &data->state );
     dfb_state_set_destination( &data->state, surface );

     data->state.clip.x1  = data->area.current.x;
     data->state.clip.y1  = data->area.current.y;
     data->state.clip.x2  = data->area.current.x + (data->area.current.w ? : 1) - 1;
     data->state.clip.y2  = data->area.current.y + (data->area.current.h ? : 1) - 1;

     data->state.modified = SMF_ALL;

     thiz->AddRef = IDirectFBSurface_AddRef;
     thiz->Release = IDirectFBSurface_Release;

     thiz->GetCapabilities = IDirectFBSurface_GetCapabilities;
     thiz->GetSize = IDirectFBSurface_GetSize;
     thiz->GetVisibleRectangle = IDirectFBSurface_GetVisibleRectangle;
     thiz->GetPixelFormat = IDirectFBSurface_GetPixelFormat;
     thiz->GetAccelerationMask = IDirectFBSurface_GetAccelerationMask;

     thiz->GetPalette = IDirectFBSurface_GetPalette;
     thiz->SetPalette = IDirectFBSurface_SetPalette;
     thiz->SetAlphaRamp = IDirectFBSurface_SetAlphaRamp;

     thiz->Lock = IDirectFBSurface_Lock;
     thiz->GetBackBufferContext = IDirectFBSurface_GetBackBufferContext;
     thiz->HardwareLock = IDirectFBSurface_HardwareLock;
     thiz->ResetSource = IDirectFBSurface_ResetSource;
     thiz->Unlock = IDirectFBSurface_Unlock;
     thiz->Flip = IDirectFBSurface_Flip;
     thiz->SetField = IDirectFBSurface_SetField;
     thiz->Clear = IDirectFBSurface_Clear;

     thiz->SetClip = IDirectFBSurface_SetClip;
     thiz->SetColor = IDirectFBSurface_SetColor;
     thiz->SetColorIndex = IDirectFBSurface_SetColorIndex;
     thiz->SetSrcBlendFunction = IDirectFBSurface_SetSrcBlendFunction;
     thiz->SetDstBlendFunction = IDirectFBSurface_SetDstBlendFunction;
     thiz->SetPorterDuff = IDirectFBSurface_SetPorterDuff;
     thiz->SetSrcColorKey = IDirectFBSurface_SetSrcColorKey;
     thiz->SetSrcColorKeyIndex = IDirectFBSurface_SetSrcColorKeyIndex;
     thiz->SetDstColorKey = IDirectFBSurface_SetDstColorKey;
     thiz->SetDstColorKeyIndex = IDirectFBSurface_SetDstColorKeyIndex;

     thiz->SetBlittingFlags = IDirectFBSurface_SetBlittingFlags;
     thiz->Blit = IDirectFBSurface_Blit;
     thiz->TileBlit = IDirectFBSurface_TileBlit;
     thiz->BatchBlit = IDirectFBSurface_BatchBlit;
     thiz->StretchBlit = IDirectFBSurface_StretchBlit;
     thiz->TextureTriangles = IDirectFBSurface_TextureTriangles;

     thiz->SetDrawingFlags = IDirectFBSurface_SetDrawingFlags;
     thiz->FillRectangle = IDirectFBSurface_FillRectangle;
     thiz->DrawLine = IDirectFBSurface_DrawLine;
     thiz->DrawLines = IDirectFBSurface_DrawLines;
     thiz->DrawRectangle = IDirectFBSurface_DrawRectangle;
     thiz->FillTriangle = IDirectFBSurface_FillTriangle;
     thiz->FillRectangles = IDirectFBSurface_FillRectangles;
     thiz->FillSpans = IDirectFBSurface_FillSpans;

     thiz->SetFont = IDirectFBSurface_SetFont;
     thiz->GetFont = IDirectFBSurface_GetFont;
     thiz->DrawString = IDirectFBSurface_DrawString;
     thiz->DrawGlyph = IDirectFBSurface_DrawGlyph;

     thiz->CreateCIF = IDirectFBSurface_CreateCIF;
     thiz->CreatePPM = IDirectFBSurface_CreatePPM;

     thiz->GetSubSurface = IDirectFBSurface_GetSubSurface;

     thiz->GetGL = IDirectFBSurface_GetGL;

     thiz->Dump = IDirectFBSurface_Dump;


     dfb_surface_attach( surface,
                         IDirectFBSurface_listener, thiz, &data->reaction );

     return DFB_OK;
}


/* internal */

static ReactionResult
IDirectFBSurface_listener( const void *msg_data, void *ctx )
{
     const CoreSurfaceNotification *notification = msg_data;
     IDirectFBSurface              *thiz         = ctx;
     IDirectFBSurface_data         *data         = thiz->priv;
     CoreSurface                   *surface      = data->surface;

     if (notification->flags & CSNF_DESTROY) {
          if (data->surface) {
               data->surface = NULL;
               dfb_surface_unref( surface );
          }
          return RS_REMOVE;
     }

     if (notification->flags & CSNF_SIZEFORMAT) {
          DFBRectangle rect = { 0, 0, surface->width, surface->height };

          if (data->caps & DSCAPS_SUBSURFACE) {
               data->area.current = data->area.granted;

               dfb_rectangle_intersect( &data->area.current, &rect );
          }
          else
               data->area.wanted = data->area.granted = data->area.current = rect;

          /* Reset clip to avoid crashes caused by drawing out of bounds. */
          if (data->clip_set)
               thiz->SetClip( thiz, &data->clip_wanted );
          else
               thiz->SetClip( thiz, NULL );
     }

     return RS_OK;
}

