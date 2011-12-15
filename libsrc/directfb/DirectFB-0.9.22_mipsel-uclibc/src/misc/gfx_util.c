/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   Scaling routines ported from gdk_pixbuf by Sven Neumann
   <sven@convergence.de>.

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

#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/palette.h>
#include <core/surfaces.h>

#include <direct/memcpy.h>
#include <direct/mem.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <misc/util.h>
#include <misc/gfx_util.h>

#include <gfx/convert.h>


#define SUBSAMPLE_BITS 4
#define SUBSAMPLE (1 << SUBSAMPLE_BITS)
#define SUBSAMPLE_MASK ((1 << SUBSAMPLE_BITS)-1)
#define SCALE_SHIFT 16


typedef struct _PixopsFilter PixopsFilter;

struct _PixopsFilter {
     int *weights;
     int n_x;
     int n_y;
     float x_offset;
     float y_offset;
};


static void rgba_to_dst_format (__u8 *dst,
                                __u32 r, __u32 g, __u32 b, __u32 a,
                                DFBSurfacePixelFormat dst_format,
                                CorePalette *palette, int dx)
{
     __u32 value;

     switch (dst_format) {

          case DSPF_RGB332:
               *((__u8*)dst) = PIXEL_RGB332( r, g, b );
               break;

          case DSPF_A8:
               *((__u8*)dst) = a;
               break;

          case DSPF_ARGB:
               *((__u32*)dst) = PIXEL_ARGB( a, r, g, b );
               break;

          case DSPF_ABGR:
               *((__u32*)dst) = PIXEL_ARGB( a, b, g, r );
               break;

          case DSPF_ARGB1555:
               *((__u16*)dst) = PIXEL_ARGB1555( a, r, g, b );
               break;

          case DSPF_ARGB2554:
               *((__u16*)dst) = PIXEL_ARGB2554( a, r, g, b );
               break;

          case DSPF_ARGB4444:
               *((__u16*)dst) = PIXEL_ARGB4444( a, r, g, b );
               break;

          case DSPF_AiRGB:
               *((__u32*)dst) = PIXEL_AiRGB( a, r, g, b );
               break;

          case DSPF_RGB32:
               *((__u32*)dst) = PIXEL_RGB32( r, g, b );
               break;

          case DSPF_RGB16:
               *(__u16 *)dst = PIXEL_RGB16 (r, g, b);
               break;

          case DSPF_RGB24:
               *dst++ = b;
               *dst++ = g;
               *dst   = r;
               break;

          case DSPF_LUT1:
               if (palette)
               {
                    value = dfb_palette_search( palette, r, g, b, a );
                    if (!(dx & 7))
                         *dst = 0;
                    *dst |= (value & 1) << (7 - (dx & 7));
               }

               break;
          case DSPF_LUT2:
               if (palette)
               {
                    value = dfb_palette_search( palette, r, g, b, a );
                    if (!(dx & 3))
                         *dst = 0;
                    *dst |= (value & 3) << (6 - (2 * (dx & 3)));
               }

               break;
          case DSPF_LUT4:
               if (palette)
               {
                    value = dfb_palette_search( palette, r, g, b, a );
                    if (! (dx & 1))
                         *dst = 0;
                    *dst |= (value & 15) << (4 - (4 * (dx & 1)));
               }

               break;
          case DSPF_LUT8:
               if (palette)
                    *dst = dfb_palette_search( palette, r, g, b, a );
               break;

          case DSPF_ALUT44:
               if (palette)
                    *dst++ = (a & 0xF0) + dfb_palette_search( palette, r, g, b, 0x80 );
               break;

          case DSPF_YUY2:
               if (! (dx & 1)) {  /* HACK */
                    __u32 y, cb, cr;

                    RGB_TO_YCBCR( r, g, b, y, cb, cr );

                    *((__u32*)dst) = PIXEL_YUY2( y, cb, cr );
               }
               break;

          case DSPF_UYVY:
               if (! (dx & 1)) {  /* HACK */
                    __u32 y, cb, cr;

                    RGB_TO_YCBCR( r, g, b, y, cb, cr );

                    *((__u32*)dst) = PIXEL_UYVY( y, cb, cr );
               }
               break;

          case DSPF_YV12:
          case DSPF_I420:
          case DSPF_NV12:
          case DSPF_NV21:
          case DSPF_NV16:
               D_ONCE( "format not fully supported (only luma plane, yet)" );

               *((__u8*)dst) = ((r * 16829 + g *  33039 + b *  6416 + 0x8000) >> 16) + 16;
               break;

          default:
               D_ONCE( "unimplemented destination format (0x%08x)", dst_format );
               break;
     }
}

#define LINE_PTR(dst,caps,y,h,pitch) \
     ((caps & DSCAPS_SEPARATED) \
          ? (((char*)dst) + y/2 * pitch + ((y%2) ? h/2 * pitch : 0)) \
          : (((char*)dst) + y * pitch))

void dfb_copy_buffer_32( __u32 *src,
                         void  *dst, int dpitch, DFBRectangle *drect,
                         CoreSurface *dst_surface )
{
     int x, y, bitsFilled = 0;
     __u32 a;
     int bytesPerPixel = DFB_BYTES_PER_PIXEL( dst_surface->format );
     int bitsPerPixel = DFB_BITS_PER_PIXEL( dst_surface->format );

     switch (dst_surface->format) {
          case DSPF_A8:
               for (y = drect->y; y < drect->y + drect->h; y++) {
                    __u8 *d = LINE_PTR( dst, dst_surface->caps,
                                        y, dst_surface->height, dpitch );

                    for (x = drect->x; x < drect->x + drect->w; x++)
                         d[x] = src[x] >> 24;

                    src += drect->w;
               }
               break;

          case DSPF_ARGB:
               for (y = drect->y; y < drect->y + drect->h; y++) {
                    void *d = LINE_PTR( dst, dst_surface->caps,
                                        y, dst_surface->height, dpitch );

                    direct_memcpy (d + drect->x * 4, src, drect->w * 4);

                    src += drect->w;
               }
               break;

          default:
               for (y = drect->y; y < drect->y + drect->h; y++) {
                    void *d = LINE_PTR( dst, dst_surface->caps,
                                        y, dst_surface->height, dpitch );
                    bitsFilled = bitsPerPixel * (drect->x & ((8 / bitsPerPixel) - 1));

                    for (x = drect->x; x < drect->x + drect->w; x++) {
                         a = *src >> 24;


                         rgba_to_dst_format ((__u8 *)d,
                                             (*src & 0x00FF0000) >> 16,
                                             (*src & 0x0000FF00) >> 8,
                                             (*src & 0x000000FF),
                                             a,
                                             dst_surface->format,
                                             dst_surface->palette,
                                             x);

                         if (bytesPerPixel > 0)
                              d += bytesPerPixel;
                         else
                         {
                              bitsFilled += bitsPerPixel;

                              if (bitsFilled >= 8)
                              {
                                   bitsFilled = 0;
                                   d++;
                              }
                         }

                         src++;
                    }
               }
               break;
     }
}

static int bilinear_make_fast_weights( PixopsFilter *filter, float x_scale, float y_scale )
{
     int i_offset, j_offset;
     float *x_weights, *y_weights;
     int n_x, n_y;

     if (x_scale > 1.0) {      /* Bilinear */
          n_x = 2;
          filter->x_offset = 0.5 * (1/x_scale - 1);
     }
     else {                    /* Tile */
          n_x = D_ICEIL (1.0 + 1.0 / x_scale);
          filter->x_offset = 0.0;
     }

     if (y_scale > 1.0) {      /* Bilinear */
          n_y = 2;
          filter->y_offset = 0.5 * (1/y_scale - 1);
     }
     else {                    /* Tile */
          n_y = D_ICEIL (1.0 + 1.0/y_scale);
          filter->y_offset = 0.0;
     }

     filter->n_y = n_y;
     filter->n_x = n_x;
     filter->weights = (int *) D_MALLOC( SUBSAMPLE * SUBSAMPLE * n_x * n_y *
                                         sizeof (int) );
     if (!filter->weights) {
          D_WARN ("couldn't allocate memory for scaling");
          return 0;
     }

     x_weights = (float *) alloca (n_x * sizeof (float));
     y_weights = (float *) alloca (n_y * sizeof (float));

     if (!x_weights || !y_weights) {
          D_FREE( filter->weights );

          D_WARN ("couldn't allocate memory for scaling");
          return 0;
     }

     for (i_offset = 0; i_offset < SUBSAMPLE; i_offset++)
          for (j_offset = 0; j_offset < SUBSAMPLE; j_offset++) {
               int *pixel_weights = filter->weights
                                    + ((i_offset * SUBSAMPLE) + j_offset)
                                    * n_x * n_y;

               float x = (float)j_offset / 16;
               float y = (float)i_offset / 16;
               int i,j;

               if (x_scale > 1.0) {     /* Bilinear */
                    for (i = 0; i < n_x; i++) {
                         x_weights[i] = ((i == 0) ? (1 - x) : x) / x_scale;
                    }
               }
               else {                   /* Tile */
                    for (i = 0; i < n_x; i++) {
                         if (i < x) {
                              if (i + 1 > x)
                                   x_weights[i] = MIN( i+ 1, x+ 1/x_scale ) -x;
                              else
                                   x_weights[i] = 0;
                         }
                         else {
                              if (x + 1/x_scale > i)
                                   x_weights[i] = MIN( i+ 1, x+ 1/x_scale ) -i;
                              else
                                   x_weights[i] = 0;
                         }
                    }
               }

               if (y_scale > 1.0) {     /* Bilinear */
                    for (i = 0; i < n_y; i++) {
                         y_weights[i] = ((i == 0) ? (1 - y) : y) / y_scale;
                    }
               }
               else {                   /* Tile */
                    for (i = 0; i < n_y; i++) {
                         if (i < y) {
                              if (i + 1 > y)
                                   y_weights[i] = MIN( i+ 1, y+ 1/y_scale ) -y;
                              else
                                   y_weights[i] = 0;
                         }
                         else {
                              if (y + 1/y_scale > i)
                                   y_weights[i] = MIN( i+ 1, y+ 1/y_scale ) -i;
                              else
                                   y_weights[i] = 0;
                         }
                    }
               }

               for (i = 0; i < n_y; i++) {
                    for (j = 0; j < n_x; j++) {
                         *(pixel_weights + n_x * i + j) =
                         65536 * x_weights[j] * x_scale
                         * y_weights[i] * y_scale;
                    }
               }
          }

     return 1;
}

static void scale_pixel( int *weights, int n_x, int n_y,
                         void *dst, __u32 **src,
                         int x, int sw, DFBSurfacePixelFormat dst_format,
                         CorePalette *palette, int dx )
{
     __u32 r = 0, g = 0, b = 0, a = 0;
     int i, j;

     for (i = 0; i < n_y; i++) {
          int *pixel_weights = weights + n_x * i;

          for (j = 0; j < n_x; j++) {
               __u32  ta;
               __u32 *q;

               if (x + j < 0)
                    q = src[i];
               else if (x + j < sw)
                    q = src[i] + x + j;
               else
                    q = src[i] + sw - 1;

               ta = ((*q & 0xFF000000) >> 24) * pixel_weights[j];

               b += ta * (((*q & 0xFF)) + 1);
               g += ta * (((*q & 0xFF00) >> 8) + 1);
               r += ta * (((*q & 0xFF0000) >> 16) + 1);
               a += ta;
          }
     }

     r = (r >> 24) == 0xFF ? 0xFF : (r + 0x800000) >> 24;
     g = (g >> 24) == 0xFF ? 0xFF : (g + 0x800000) >> 24;
     b = (b >> 24) == 0xFF ? 0xFF : (b + 0x800000) >> 24;
     a = (a >> 16) == 0xFF ? 0xFF : (a + 0x8000) >> 16;

     rgba_to_dst_format( dst, r, g, b, a, dst_format, palette, dx);
}

static void *scale_line( int *weights, int n_x, int n_y, void *dst,
                         void *dst_end, __u32 **src, int x, int x_step, int sw,
                         DFBSurfacePixelFormat dst_format, CorePalette *palette, int *dx )
{
     int i, j;
     int *pixel_weights;
     __u32 *q;
     __u32 r, g, b, a;
     int  x_scaled;
     int *line_weights;
     int bytesPerPixel = DFB_BYTES_PER_PIXEL( dst_format );
     int bitsPerPixel = DFB_BITS_PER_PIXEL( dst_format );
     __u8 bitsFilled = bitsPerPixel * ((*dx) & ((8 / bitsPerPixel) - 1));

     
     
     while (dst < dst_end) {
          r = g = b = a = 0;
          x_scaled = x >> SCALE_SHIFT;

          pixel_weights = weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS))
                                     & SUBSAMPLE_MASK) * n_x * n_y;

          for (i = 0; i < n_y; i++) {
               line_weights = pixel_weights + n_x * i;

               q = src[i] + x_scaled;

               for (j = 0; j < n_x; j++) {
                    __u32 ta;

                    ta = ((*q & 0xFF000000) >> 24) * line_weights[j];

                    b += ta * (((*q & 0xFF)) + 1);
                    g += ta * (((*q & 0xFF00) >> 8) + 1);
                    r += ta * (((*q & 0xFF0000) >> 16) + 1);
                    a += ta;

                    q++;
               }
          }

          r = (r >> 24) == 0xFF ? 0xFF : (r + 0x800000) >> 24;
          g = (g >> 24) == 0xFF ? 0xFF : (g + 0x800000) >> 24;
          b = (b >> 24) == 0xFF ? 0xFF : (b + 0x800000) >> 24;
          a = (a >> 16) == 0xFF ? 0xFF : (a + 0x8000) >> 16;


          rgba_to_dst_format( dst, r, g, b, a, dst_format, palette, (*dx)++);


          if (bytesPerPixel > 0)
               dst += bytesPerPixel;
          else
          {
               bitsFilled += bitsPerPixel;

               if (bitsFilled >= 8)
               {
                    bitsFilled = 0;
                    dst++;
               }
          }

          x += x_step;
     }

     return dst;
}

void dfb_scale_linear_32( __u32 *src, int sw, int sh,
                          void  *dst, int dpitch, DFBRectangle *drect,
                          CoreSurface *dst_surface )
{
     float scale_x, scale_y;
     int i, j;
     int sx, sy;
     int x_step, y_step;
     int scaled_x_offset;
     PixopsFilter filter;
     int bytesPerPixel = DFB_BYTES_PER_PIXEL( dst_surface->format );
     int bitsPerPixel = DFB_BITS_PER_PIXEL( dst_surface->format );
     __u8 bitsFilled = 0;


     if (sw < 1 || sh < 1 || drect->w < 1 || drect->h < 1)
          return;

     if (drect->w == sw && drect->h == sh) {
          dfb_copy_buffer_32( src, dst, dpitch, drect, dst_surface );
          return;
     }

     D_WARN("Source and Dest rect do not match. USING SOFTWARE SCALING!");

     scale_x = (float)drect->w / sw;
     scale_y = (float)drect->h / sh;

     x_step = (1 << SCALE_SHIFT) / scale_x;
     y_step = (1 << SCALE_SHIFT) / scale_y;

     if (! bilinear_make_fast_weights( &filter, scale_x, scale_y ))
          return;

     scaled_x_offset = D_IFLOOR( filter.x_offset * (1 << SCALE_SHIFT) );
     sy = D_IFLOOR( filter.y_offset * (1 << SCALE_SHIFT) );

     for (i = drect->y; i < drect->y + drect->h; i++) {
          int x_start;
          int y_start;
          int dest_x;
          int *run_weights;
          void *outbuf;
          void *outbuf_end;
          void *new_outbuf;
          __u32 **line_bufs;

          y_start = sy >> SCALE_SHIFT;

          run_weights = filter.weights + ((sy >> (SCALE_SHIFT - SUBSAMPLE_BITS))
                                          & SUBSAMPLE_MASK) * filter.n_x * filter.n_y * SUBSAMPLE;

          line_bufs = (__u32 **) alloca( filter.n_y * sizeof (void *) );

          for (j = 0; j < filter.n_y; j++) {
               if (y_start <  0)
                    line_bufs[j] = src;
               else if (y_start < sh)
                    line_bufs[j] = src + sw * y_start;
               else
                    line_bufs[j] = src + sw * (sh - 1);

               y_start++;
          }

          outbuf = (LINE_PTR( dst, dst_surface->caps,
                              i, dst_surface->height, dpitch ) +
                    DFB_BYTES_PER_LINE( dst_surface->format, drect->x ));

          outbuf_end = outbuf + DFB_BYTES_PER_LINE( dst_surface->format, drect->w );

          sx = scaled_x_offset;
          x_start = sx >> SCALE_SHIFT;
          dest_x = 0;

          /**((__u8*)outbuf) = 0;*/
          bitsFilled = bitsPerPixel * (dest_x & ((8 / bitsPerPixel) - 1));
          while (x_start < 0 && outbuf < outbuf_end) {
               scale_pixel( run_weights + ((sx >> (SCALE_SHIFT - SUBSAMPLE_BITS))
                                           & SUBSAMPLE_MASK) * (filter.n_x * filter.n_y),
                            filter.n_x, filter.n_y, outbuf, line_bufs,
                            sx >> SCALE_SHIFT, sw, dst_surface->format,
                            dst_surface->palette, dest_x );


               if (bytesPerPixel > 0)
               {
                    outbuf += bytesPerPixel;
               }
               else
               {
                    bitsFilled += bitsPerPixel;

                    if (bitsFilled >= 8)
                    {
                         bitsFilled = 0;
                         outbuf++;
                    }
               }

               x_start = sx >> SCALE_SHIFT;
               sx += x_step;
               dest_x++;
          }

          new_outbuf = scale_line (run_weights, filter.n_x, filter.n_y, outbuf,
                                   outbuf_end, line_bufs, sx >> SCALE_SHIFT,
                                   x_step, sw, dst_surface->format,
                                   dst_surface->palette, &dest_x);
          
          sx = dest_x * x_step + scaled_x_offset;
          outbuf = new_outbuf;

          /**((__u8*)outbuf) = 0;*/
          bitsFilled = bitsPerPixel * (dest_x & ((8 / bitsPerPixel) - 1));
          while (outbuf < outbuf_end) {
               scale_pixel( run_weights + ((sx >> (SCALE_SHIFT - SUBSAMPLE_BITS))
                                           & SUBSAMPLE_MASK) * (filter.n_x * filter.n_y),
                            filter.n_x, filter.n_y, outbuf, line_bufs,
                            sx >> SCALE_SHIFT, sw, dst_surface->format,
                            dst_surface->palette, dest_x);

               if (bytesPerPixel > 0)
               {
                    outbuf += bytesPerPixel;
                    sx += x_step;
               }
               else
               {
                    bitsFilled += bitsPerPixel;

                    if (bitsFilled >= 8)
                    {
                         bitsFilled = 0;
                         outbuf++;
                    }
               }
               sx += x_step;
          }

          sy += y_step;
     }

     D_FREE(filter.weights);
}


void dfb_copy_to_argb( __u8 *src, int w, int h, int spitch, DFBSurfacePixelFormat src_format, __u8  *dst, int dpitch, CorePalette *palette )
{
    int err;
    __u32 x,y;
    __u32 val;
    __u8 index;
    __u32 pitch;
    DFBRectangle rect = { 0, 0, 0, 0};
    __u8 *pIn8;
    __u32* pIn32;
    __u32 pValue, bits_per_pix=0;
    __u8 value;
    int bit_pos;
    __u32 pix_mask;
    __u8 y1, y2, cr, cb;
    __u32 argb1, argb2;
    __u32 a, r, g, b, r2, g2, b2;
    __u32 *pOut32;

     switch(src_format)
     {
     case DSPF_LUT1:
     case DSPF_LUT2:
     case DSPF_LUT4:
     case DSPF_LUT8:
          if (src_format == DSPF_LUT1)      bits_per_pix = 1;
          else if (src_format == DSPF_LUT2) bits_per_pix = 2;
          else if (src_format == DSPF_LUT4) bits_per_pix = 4;
          else if (src_format == DSPF_LUT8) bits_per_pix = 8;

          pix_mask = ((1 << bits_per_pix) - 1);

          for (y = 0; y < h; y++)
          {
               pIn8 = src + (y * spitch);
               pOut32 = (__u32 *)(dst + (y * dpitch));

               for (x = 0; x < w;) 
               {
                    value = *pIn8++;
                    for (bit_pos = 8-bits_per_pix; bit_pos >= 0 && x < w; bit_pos-=bits_per_pix)
                    {
                         index = ((value >> bit_pos) & pix_mask);
                         pValue = *((__u32*)&palette->entries[index]);
                         a = (pValue >> 24) & 0xFF;
                         r = (pValue >> 16) & 0xFF;
                         g = (pValue >> 8) & 0xFF;
                         b = (pValue) & 0xFF;
                         *pOut32++ = PIXEL_ARGB( a, r, g, b );
                         x++;
                    }
               }
          }
          break;
     case DSPF_ARGB:
          for (y = 0; y < h; y++)
          {
               pIn8 = src + (y * spitch);
               pIn32 = (__u32*)pIn8;
               pOut32 = (__u32 *)(dst + (y * dpitch));

               for (x = 0; x < w; x++)
               {
                    val = *pIn32++;
                    a = (val >> 24) & 0xFF;
                    r = (val >> 16) & 0xFF;
                    g = (val >> 8) & 0xFF;
                    b = (val) & 0xFF;
                    *pOut32++ = PIXEL_ARGB( a, b, g, r );
               }
          }         
          break;
     case DSPF_ABGR:
          for (y = 0; y < h; y++)
          {
               pIn8 = src + (y * spitch);
               pIn32 = (__u32*)pIn8;
               pOut32 = (__u32 *)(dst + (y * dpitch));

               for (x = 0; x < w; x++)
               {
                    val = *pIn32++;
                    a = (val >> 24) & 0xFF;
                    r = (val) & 0xFF;
                    g = (val >> 8) & 0xFF;
                    b = (val >> 16) & 0xFF;
                    *pOut32++ = PIXEL_ARGB( a, b, g, r );
               }
          }         
          break;
     case DSPF_UYVY:

          for (y = 0; y < h; y++)
          {
               pIn8 = src + (y * spitch);
               pIn32 = (__u32*)pIn8;
               pOut32 = (__u32 *)(dst + (y * dpitch));

               for (x = 0; x < w; x+=2)
               {
                    val = *pIn32++;
                    y1 = (val >> 24) & 0xFF;
                    y2 = (val >> 8) & 0xFF;
                    cb = (val >> 16) & 0xFF;
                    cr = (val >> 0) & 0xFF;
                    a = 0xff;
                    YCBCR_TO_RGB(y1, cb, cr, r, g, b);
                    *pOut32++ = PIXEL_ARGB( a, b, g, r );
                    YCBCR_TO_RGB(y2, cb, cr, r, g, b);
                    *pOut32++ = PIXEL_ARGB( a, b, g, r );
               }
          } 
          break;
     default:
          printf( "unimplemented source format (0x%08x)\n", src_format );
     break;
     }

}
