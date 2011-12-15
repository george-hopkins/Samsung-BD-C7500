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

#include <ft2build.h>
#include FT_GLYPH_H

/*PR10573*/
#include FT_STROKER_H


#include <directfb.h>

#include <core/fonts.h>
#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <gfx/convert.h>

#include <media/idirectfbfont.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <misc/conf.h>
#include <misc/util.h>
#include <math.h>

static DFBResult
Probe( IDirectFBFont_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBFont      *thiz,
           CoreDFB            *core,
           const char         *filename,
           const void         *data,
           unsigned int        length,
           DFBFontDescription *desc );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBFont, FT2 )

static FT_Library      library           = NULL;
static int             library_ref_count = 0;
static pthread_mutex_t library_mutex     = DIRECT_UTIL_RECURSIVE_PTHREAD_MUTEX_INITIALIZER;

static FT_Matrix italic_transform_matrix =         { 1 * 0x10000L, (0x10000L >> 3),
                                                     0 * 0x10000L, 1 * 0x10000L};
static FT_Matrix reverse_italic_transform_matrix = { 1 * 0x10000L, -(0x10000L >> 3),
                                                     0 * 0x10000L,  1 * 0x10000L};
static FT_Matrix rotate_90_transform_matrix =      { 0 * 0x10000L,  1 * 0x10000L,
                                                    -1 * 0x10000L,  0 * 0x10000L};
static FT_Matrix rotate_180_transform_matrix =     {-1 * 0x10000L,  0 * 0x10000L,
                                                     0 * 0x10000L, -1 * 0x10000L};
static FT_Matrix rotate_270_transform_matrix =     { 0 * 0x10000L, -1 * 0x10000L,
                                                     1 * 0x10000L,  0 * 0x10000L};

#define KERNING_CACHE_MIN   32
#define KERNING_CACHE_MAX  127
#define KERNING_CACHE_SIZE (KERNING_CACHE_MAX - KERNING_CACHE_MIN + 1)

#define KERNING_DO_CACHE(a,b)      ((a) >= KERNING_CACHE_MIN && \
                                    (a) <= KERNING_CACHE_MAX && \
                                    (b) >= KERNING_CACHE_MIN && \
                                    (b) <= KERNING_CACHE_MAX)

#define KERNING_CACHE_ENTRY(a,b)   \
     (data->kerning[(a)-KERNING_CACHE_MIN][(b)-KERNING_CACHE_MIN])

#define FT_F26Dot6Ceil(x)           (((x) + (1 << 6) - 1) >> 6)
#define FT_XEMUnits2Pixels(x)       (((x) * face->size->metrics.x_ppem) / face->units_per_EM)
#define FT_YEMUnits2Pixels(x)       (((x) * face->size->metrics.y_ppem) / face->units_per_EM)


typedef struct {
     FT_Face      face;
     int          disable_charmap;
     int          fixed_advance;
     int          layout;
     bool         transform_enabled;

     /*PR10573*/     
     bool         stroking_enabled;
     int          stroking_radius;   
     FT_Stroker_LineCap stroking_lineCap;
     FT_Stroker_LineJoin stroking_lineJoin;
     unsigned char borderColorIndex;
     unsigned char backgroundColorIndex;
     unsigned char fontColorIndex;
} FT2ImplData;

typedef struct {
     char x;
     char y;
} KerningCacheEntry;

typedef struct {
     FT2ImplData base;

     KerningCacheEntry kerning[KERNING_CACHE_SIZE][KERNING_CACHE_SIZE];
} FT2ImplKerningData;


static DFBResult
render_glyph( CoreFont      *thiz,
              unichar        glyph,
              CoreGlyphData *info,
              CoreSurface   *surface )
{
     FT_Error     err;
     FT_Face      face;
     FT_Int       load_flags;
     FT_UInt      index;
     __u8        *src;
     void        *dst;
     int          y;
     int          pitch;

     /*PR10573*/
     FT_Glyph     aglyph;
     FT_Stroker  astroker;
     FT_BitmapGlyph aglyphBitmap;
     
     
     __u8 *dstprev;
     int pitchprev;
     int ii;
     unsigned char threshold;
     int borderState = 0;
     int backgroundState = 0;

     threshold = 150;
      
      
     
     
     FT2ImplData *data = (FT2ImplData*) thiz->impl_data;

     pthread_mutex_lock ( &library_mutex );

     face = data->face;

     if (data->disable_charmap)
          index = glyph;
     else
          index = FT_Get_Char_Index( face, glyph );

     load_flags = (FT_Int) face->generic.data;
     
     /*PR10573*/     
     
/*Example values that were used for unit testing on MTD600
     data->stroking_enabled = true;
     data->stroking_radius = 64;
     data->stroking_lineCap = FT_STROKER_LINECAP_ROUND;
     data->stroking_lineJoin = FT_STROKER_LINEJOIN_ROUND;
     data->borderColorIndex = 1;  
     data->backgroundColorIndex = 2;
     thiz->state.color_index = 0;
*/   
    thiz->state.stroking_enabled = data->stroking_enabled;
    
    
     if (data->stroking_enabled)
     {
        load_flags |= FT_LOAD_DEFAULT;
     }
     else
     {
        load_flags |= FT_LOAD_RENDER;
     }  
     
     if ((err = FT_Load_Glyph( face, index, load_flags ))) {
          D_HEAVYDEBUG( "DirectFB/FontFT2: "
                         "Could not render glyph for character #%d!\n", glyph );
          pthread_mutex_unlock ( &library_mutex );
          return DFB_FAILURE;
     }

        if (data->stroking_enabled)
        {
            FT_Get_Glyph(face->glyph, &aglyph); 
            
     

     
            
            FT_Stroker_New(library, &astroker );
            
     
            
            FT_Stroker_Set(astroker, data->stroking_radius, data->stroking_lineCap, data->stroking_lineJoin, 0);
            
     
            FT_Glyph_Stroke(&aglyph, astroker, 0);
            
     
            
            FT_Glyph_To_Bitmap( &aglyph, FT_RENDER_MODE_NORMAL, 0, 0);
                
            
            
            aglyphBitmap = (FT_BitmapGlyph)aglyph;
     
            face->glyph->bitmap = aglyphBitmap->bitmap;
            face->glyph->bitmap_top = aglyphBitmap->top;
            face->glyph->bitmap_left = aglyphBitmap->left;
     
            FT_Done_Glyph(aglyph);
     
            FT_Stroker_Done( astroker );    
        }

     pthread_mutex_unlock ( &library_mutex );

     info->width = face->glyph->bitmap.width;
     info->height = face->glyph->bitmap.rows;
     switch(data->layout) {
     case DFFL_TOP_TO_BOTTOM:
          info->left = (face->glyph->metrics.vertBearingX >> 6);
          info->top  = (face->glyph->metrics.vertBearingY >> 6);
          info->advance_x = 0;
          info->advance_y = data->fixed_advance ? data->fixed_advance : (face->glyph->advance.y >> 6);
          break;
     case DFFL_BOTTOM_TO_TOP:
          info->left = (face->glyph->metrics.vertBearingX >> 6);
          info->top  = (face->glyph->metrics.vertBearingY >> 6);
          info->advance_x = 0;
          info->advance_y = data->fixed_advance ? data->fixed_advance : -(face->glyph->advance.y >> 6);
          break;
     case DFFL_RIGHT_TO_LEFT:
          info->left = face->glyph->bitmap_left;
          info->top  = (thiz->ascender - face->glyph->bitmap_top) - thiz->ascender;
          info->advance_x = data->fixed_advance ? data->fixed_advance : -(face->glyph->advance.x >> 6);
          info->advance_y = -(face->glyph->advance.y >> 6);
          break;
     default:
     case DFFL_LEFT_TO_RIGHT:
          info->left = face->glyph->bitmap_left;
          info->top  = (thiz->ascender - face->glyph->bitmap_top) - thiz->ascender;
          info->advance_x = data->fixed_advance ? data->fixed_advance : (face->glyph->advance.x >> 6);
          info->advance_y = -(face->glyph->advance.y >> 6);
          break;
     }

     if ((surface == NULL) ||
         (info->width + thiz->next_x > surface->width) ||
         (info->height > surface->height))
          return DFB_LIMITEXCEEDED;


     err = dfb_surface_soft_lock( surface, surface->back_buffer, DSLF_WRITE,
                                  &dst, &pitch );
     if (err) {
          D_ERROR( "DirectB/FontFT2: Unable to lock surface!\n" );
          return err;
     }

     src = face->glyph->bitmap.buffer;
     dst += DFB_BYTES_PER_LINE(surface->format, thiz->next_x);

/*PR10573*/
     dstprev = dst;
     pitchprev = pitch;

     

     for (y=0; y < info->height; y++) {
          int    i, j, n;
          __u8  *dst8  = dst;
          __u16 *dst16 = dst;
          __u32 *dst32 = dst;

          switch (face->glyph->bitmap.pixel_mode) {
               case ft_pixel_mode_grays:
                    switch (surface->format) {
                         case DSPF_ARGB:
                              for (i=0; i<info->width; i++)
                                   dst32[i] = (src[i] << 24) | 0xFFFFFF;
                              break;
                         case DSPF_AiRGB:
                              for (i=0; i<info->width; i++)
                                   dst32[i] = ((src[i] ^ 0xFF) << 24) | 0xFFFFFF;
                              break;
                         case DSPF_ARGB4444:
                              for (i=0; i<info->width; i++)
                                   dst16[i] = (src[i] << 8) | 0xFFF;
                              break;
                         case DSPF_ARGB2554:
                              for (i=0; i<info->width; i++)
                                   dst16[i] = (src[i] << 8) | 0x3FFF;
                              break;
                         case DSPF_ARGB1555:
                              for (i=0; i<info->width; i++)
                                   dst16[i] = (src[i] << 8) | 0x7FFF;
                              break;
                         case DSPF_A8:
                            /*PR10573*/
                            if (data->stroking_enabled)
                            {
                                for (i=0; i<info->width; i++)                           
                                   dst8[i] = (src[i] >= 180) ? data->borderColorIndex : data->backgroundColorIndex;
                            }
                            else
                              direct_memcpy( dst, src, info->width );
                              break;
                         case DSPF_A1:
                              for (i=0, j=0; i < info->width; ++j) {
                                   register __u8 p = 0;

                                   for (n=0; n<8 && i<info->width; ++i, ++n)
                                        p |= (src[i] & 0x80) >> n;

                                   dst8[j] = p;
                              }
                              break;
                         default:
                              break;
                    }
                    break;

               case ft_pixel_mode_mono:
                    switch (surface->format) {
                         case DSPF_ARGB:
                              for (i=0; i<info->width; i++)
                                   dst32[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                0xFF : 0x00) << 24) | 0xFFFFFF;
                              break;
                         case DSPF_AiRGB:
                              for (i=0; i<info->width; i++)
                                   dst32[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                0x00 : 0xFF) << 24) | 0xFFFFFF;
                              break;
                         case DSPF_ARGB4444:
                              for (i=0; i<info->width; i++)
                                   dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                0xF : 0x0) << 12) | 0xFFF;
                              break;
                         case DSPF_ARGB2554:
                              for (i=0; i<info->width; i++)
                                   dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                0x3 : 0x0) << 14) | 0x3FFF;
                              break;
                         case DSPF_ARGB1555:
                              for (i=0; i<info->width; i++)
                                   dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                0x1 : 0x0) << 15) | 0x7FFF;
                              break;
                         case DSPF_A8:
                              for (i=0; i<info->width; i++)
                                   dst8[i] = (src[i>>3] &
                                              (1<<(7-(i%8)))) ? 0xFF : 0x00;
                              break;
                         case DSPF_A1:
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(DSPF_A1, info->width) );
                              break;
                         default:
                              break;
                    }
                    break;

               default:
                    break;

          }

          src += face->glyph->bitmap.pitch;
          dst += pitch;
     }

     

    /*PR10573*/ 
    pthread_mutex_lock ( &library_mutex );

     if ((err = FT_Load_Glyph( face, index, FT_LOAD_RENDER))) {
          D_HEAVYDEBUG( "DirectFB/FontFT2: "
                         "Could not render glyph for character #%d!\n", glyph );
          pthread_mutex_unlock ( &library_mutex );
          return DFB_FAILURE;
     }

     pthread_mutex_unlock ( &library_mutex );

     src = face->glyph->bitmap.buffer;   

    
    
     for (y=0; y < info->height; y++)
     {
     borderState = 0;
     backgroundState = 0;
        for (ii=0; ii<info->width; ii++)
        {
            if (dstprev[ii] == data->borderColorIndex)
            {
                borderState += 1;   
                backgroundState = 0;
            }

            if (dstprev[ii] == data->backgroundColorIndex)
            {
                backgroundState += 1;
            }
            
            if ((y < face->glyph->bitmap.rows) && (ii < face->glyph->bitmap.width))
            {
                if ((src[ii] > threshold)&&(data->stroking_enabled)&&(dstprev[ii] != data->borderColorIndex)&&(backgroundState < 5)&&(borderState > 0))
                {
                    dstprev[ii] = data->fontColorIndex;             
                    backgroundState = 0;
                }
            }
        }
        dstprev+=pitchprev/DFB_BYTES_PER_PIXEL(surface->format);
        src+=face->glyph->bitmap.pitch;
        
     }
     

     return DFB_OK;
}


static DFBResult
get_kerning( CoreFont *thiz,
             unichar   prev,
             unichar   current,
             int      *kern_x,
             int      *kern_y)
{
     FT_Vector  vector;
     FT_UInt    prev_index;
     FT_UInt    current_index;

     FT2ImplKerningData *data = (FT2ImplKerningData*) thiz->impl_data;
     KerningCacheEntry *cache = NULL;

     D_ASSUME( (kern_x != NULL) || (kern_y != NULL) );

     /*
      * Use cached values if characters are in the
      * cachable range and the cache entry is already filled.
      */
     if (KERNING_DO_CACHE (prev, current)) {
          cache = &KERNING_CACHE_ENTRY (prev, current);

          if (kern_x)
               *kern_x = (int) cache->x;

          if (kern_y)
               *kern_y = (int) cache->y;

          return DFB_OK;
     }

     pthread_mutex_lock ( &library_mutex );

     /* Get the character indices for lookup. */
     prev_index    = FT_Get_Char_Index( data->base.face, prev );
     current_index = FT_Get_Char_Index( data->base.face, current );

     /* Lookup kerning values for the character pair. */
     FT_Get_Kerning( data->base.face,
                     prev_index, current_index, ft_kerning_default, &vector );

     pthread_mutex_unlock ( &library_mutex );

     /* Convert to integer. */
     if (kern_x)
          *kern_x = vector.x >> 6;

     if (kern_y)
          *kern_y = vector.y >> 6;

     return DFB_OK;
}

static void
init_kerning_cache( FT2ImplKerningData *data )
{
     int a, b;

     for (a=KERNING_CACHE_MIN; a<=KERNING_CACHE_MAX; a++) {
          for (b=KERNING_CACHE_MIN; b<=KERNING_CACHE_MAX; b++) {
               FT_Vector          vector;
               FT_UInt            prev;
               FT_UInt            current;
               KerningCacheEntry *cache = &KERNING_CACHE_ENTRY( a, b );

               pthread_mutex_lock ( &library_mutex );

               /* Get the character indices for lookup. */
               prev    = FT_Get_Char_Index( data->base.face, a );
               current = FT_Get_Char_Index( data->base.face, b );

               /* Lookup kerning values for the character pair. */
               FT_Get_Kerning( data->base.face,
                               prev, current, ft_kerning_default, &vector );

               pthread_mutex_unlock ( &library_mutex );

               cache->x = (char) (vector.x >> 6);
               cache->y = (char) (vector.y >> 6);
          }
     }
}

static DFBResult
init_freetype( void )
{
     FT_Error err;

     pthread_mutex_lock ( &library_mutex );

     if (!library) {
          D_HEAVYDEBUG( "DirectFB/FontFT2: "
                         "Initializing the FreeType2 library.\n" );
          err = FT_Init_FreeType( &library );
          if (err) {
               D_ERROR( "DirectFB/FontFT2: "
                         "Initialization of the FreeType2 library failed!\n" );
               library = NULL;
               pthread_mutex_unlock( &library_mutex );
               return DFB_FAILURE;
          }
     }

     library_ref_count++;
     pthread_mutex_unlock( &library_mutex );

     return DFB_OK;
}


static void
release_freetype( void )
{
     pthread_mutex_lock( &library_mutex );

     if (library && --library_ref_count == 0) {
          D_HEAVYDEBUG( "DirectFB/FontFT2: "
                         "Releasing the FreeType2 library.\n" );
          FT_Done_FreeType( library );
          library = NULL;
     }

     pthread_mutex_unlock( &library_mutex );
}


static void
IDirectFBFont_FT2_Destruct( IDirectFBFont *thiz )
{
     IDirectFBFont_data *data = (IDirectFBFont_data*)thiz->priv;

     if (data->font->impl_data) {
          FT2ImplData *impl_data = (FT2ImplData*) data->font->impl_data;

          pthread_mutex_lock ( &library_mutex );
          FT_Done_Face( impl_data->face );
          pthread_mutex_unlock ( &library_mutex );

          D_FREE( impl_data );

          data->font->impl_data = NULL;
     }

     IDirectFBFont_Destruct( thiz );

     release_freetype();
}


static DFBResult
IDirectFBFont_FT2_Release( IDirectFBFont *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBFont)

     if (--data->ref == 0) {
          IDirectFBFont_FT2_Destruct( thiz );
     }

     return DFB_OK;
}


static DFBResult
Probe( IDirectFBFont_ProbeContext *ctx )
{
     FT_Error err;
     FT_Face  face;

     if (!ctx->filename && !ctx->data)
          return DFB_UNSUPPORTED;

     if (ctx->filename)
          D_HEAVYDEBUG( "DirectFB/FontFT2: Probe font `%s'.\n", ctx->filename );
     else
          D_HEAVYDEBUG( "DirectFB/FontFT2: Probe font `%p'.\n", ctx->data );

     if (init_freetype() != DFB_OK) {
          return DFB_FAILURE;
     }

     pthread_mutex_lock ( &library_mutex );
#if 0
    /* This does not work with ttcollections, must find why.
     * Also since there is a library mutex, should also try to
     * compile freetype without reentrancy and see if there is a speed up
     */
     err = FT_New_Face( library, ctx->filename, -1, &face );
#endif
     pthread_mutex_unlock ( &library_mutex );

     release_freetype();

     return /*err ? DFB_UNSUPPORTED :*/ DFB_OK;
}


static DFBResult
Construct( IDirectFBFont      *thiz,
           CoreDFB            *core,
           const char         *filename,
           const void         *data,
           unsigned int        length,
           DFBFontDescription *desc )
{
     CoreFont              *font;
     FT_Face                face;
     FT_Error               err;
     FT_Int                 load_flags = FT_LOAD_DEFAULT;
     FT2ImplData           *impl_data;
     bool                   disable_charmap = false;
     bool                   disable_kerning = false;
     bool                   load_mono = false;
     DFBFontLayout          layout = DFFL_HORIZONTAL;
     FT_Matrix              rotate_special_matrix;
     FT_Matrix             *transform_matrix = NULL;

     if (filename) {
     D_HEAVYDEBUG( "DirectFB/FontFT2: "
                    "Construct font from file `%s' (index %d) at pixel size %d x %d.\n",
                    filename,
                    (desc->flags & DFDESC_INDEX)  ? desc->index  : 0,
                    (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                    (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
     }
     else {
         D_HEAVYDEBUG( "DirectFB/FontFT2: "
                        "Construct font from buffer `%p' (index %d) at pixel size %d x %d.\n",
                        data,
                        (desc->flags & DFDESC_INDEX)  ? desc->index  : 0,
                        (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                        (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
     }

     if (init_freetype() != DFB_OK) {
          DIRECT_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }

     pthread_mutex_lock ( &library_mutex );
     if (filename) {
     err = FT_New_Face( library, filename,
                        (desc->flags & DFDESC_INDEX) ? desc->index : 0,
                        &face );
     }
     else {
          err = FT_New_Memory_Face( library,
                                    data, length,
                                    (desc->flags & DFDESC_INDEX) ? desc->index : 0,
                                    &face );
     }
     pthread_mutex_unlock ( &library_mutex );
     if (err) {
          switch (err) {
               case FT_Err_Unknown_File_Format:
                    D_ERROR( "DirectFB/FontFT2: "
                              "Unsupported font format!\n" );
                    break;
               default:
                    D_ERROR( "DirectFB/FontFT2: "
                              "Failed loading face %d!\n",
                              (desc->flags & DFDESC_INDEX) ? desc->index : 0 );
                    break;
          }
          DIRECT_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }

     D_DEBUG( "DirectFB/FontFT2: [xm] face->num_faces is %d\n", face->num_faces );
     D_DEBUG( "DirectFB/FontFT2: [xm] face->num_glyphs is %d\n", face->num_glyphs );
     D_DEBUG( "DirectFB/FontFT2: [xm] face->family_name is %s\n", face->family_name );
     D_DEBUG( "DirectFB/FontFT2: [xm] face->style_name is %s\n", face->style_name );

     if (dfb_config->font_format == DSPF_A1 || dfb_config->font_format == DSPF_ARGB1555)
          load_mono = true;

     if (desc->flags & DFDESC_ATTRIBUTES) {
          if (desc->attributes & DFFA_NOHINTING)
               load_flags |= FT_LOAD_NO_HINTING;
          if (desc->attributes & DFFA_NOCHARMAP)
               disable_charmap = true;
          if (desc->attributes & DFFA_NOKERNING)
               disable_kerning = true;
          if (desc->attributes & DFFA_MONOCHROME)
               load_mono = true;

          if (desc->attributes & DFFA_ITALIC) {
               if (!(face->style_flags & FT_STYLE_FLAG_ITALIC))
                    transform_matrix = &italic_transform_matrix;
          }
          else if (desc->attributes & DFFA_REVERSE_ITALIC) {
               transform_matrix = &reverse_italic_transform_matrix;
          }
          else if (desc->attributes & DFFA_ROTATE_90)
               transform_matrix = &rotate_90_transform_matrix;
          else if (desc->attributes & DFFA_ROTATE_180)
               transform_matrix = &rotate_180_transform_matrix;
          else if (desc->attributes & DFFA_ROTATE_270)
               transform_matrix = &rotate_270_transform_matrix;
          else if (desc->attributes & DFFA_ROTATE_X) {
               double angle = desc->rotate_degrees * 3.141593 / 180.0;
               rotate_special_matrix.xx =  cos(angle) * 0x10000L;
               rotate_special_matrix.xy =  sin(angle) * 0x10000L;
               rotate_special_matrix.yx =  -rotate_special_matrix.xy;
               rotate_special_matrix.yy =  rotate_special_matrix.xx;
               transform_matrix = &rotate_special_matrix;
          }
     }

     if (desc->flags & DFDESC_LAYOUT)
          layout = desc->layout;
     else
          layout = DFFL_HORIZONTAL;

     if (layout & DFFL_VERTICAL)
     {
          if (FT_HAS_VERTICAL(face))
               load_flags |= FT_LOAD_VERTICAL_LAYOUT;
          else
               layout = DFFL_HORIZONTAL;
     }

     if (layout == DFFL_HORIZONTAL)
          layout = DFFL_LEFT_TO_RIGHT; /*default to a more specific layout */
     if (layout == DFFL_VERTICAL)
          layout = DFFL_TOP_TO_BOTTOM; /*default to a more specific layout */

     if ((layout != DFFL_RIGHT_TO_LEFT) &&
         (layout != DFFL_LEFT_TO_RIGHT) &&
         (layout != DFFL_BOTTOM_TO_TOP) &&
         (layout != DFFL_TOP_TO_BOTTOM))
        layout = DFFL_LEFT_TO_RIGHT;

     if ((layout != DFFL_LEFT_TO_RIGHT) &&
         (transform_matrix != &italic_transform_matrix) &&
         (transform_matrix != &reverse_italic_transform_matrix))
          transform_matrix = NULL;

     if ((layout != DFFL_LEFT_TO_RIGHT) &&
         (layout != DFFL_TOP_TO_BOTTOM))
        disable_kerning = true;

     if (load_mono) {
#ifdef FT_LOAD_TARGET_MONO  /* added in FreeType-2.1.3 */
          load_flags |= FT_LOAD_TARGET_MONO;
#else
          load_flags |= FT_LOAD_MONOCHROME;
#endif
     }

     if (transform_matrix) {
          pthread_mutex_lock ( &library_mutex );
          FT_Set_Transform( face, transform_matrix, NULL );
          pthread_mutex_unlock ( &library_mutex );
     }

     if (!disable_charmap) {
          pthread_mutex_lock ( &library_mutex );
          err = FT_Select_Charmap( face, ft_encoding_unicode );
          pthread_mutex_unlock ( &library_mutex );

#if FREETYPE_MINOR > 0

          /* ft_encoding_latin_1 has been introduced in freetype-2.1 */
          if (err) {
               D_HEAVYDEBUG( "DirectFB/FontFT2: "
                              "Couldn't select Unicode encoding, "
                              "falling back to Latin1.\n");
               pthread_mutex_lock ( &library_mutex );
               err = FT_Select_Charmap( face, ft_encoding_latin_1 );
               pthread_mutex_unlock ( &library_mutex );
          }
#endif
     }

#if 0
     if (err) {
          D_ERROR( "DirectFB/FontFT2: "
                   "Couldn't select a suitable encoding for face %d!\n", (desc->flags & DFDESC_INDEX) ? desc->index : 0 );
          pthread_mutex_lock ( &library_mutex );
          FT_Done_Face( face );
          pthread_mutex_unlock ( &library_mutex );
          DIRECT_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }
#endif

     if (desc->flags & (DFDESC_HEIGHT | DFDESC_WIDTH)) {
          pthread_mutex_lock ( &library_mutex );
          err = FT_Set_Pixel_Sizes( face,
                                    (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                                    (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
          pthread_mutex_unlock ( &library_mutex );
          if (err) {
               D_ERROR( "DirectB/FontFT2: "
                         "Could not set pixel size to %d x %d!\n",
                         (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                         (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
               pthread_mutex_lock ( &library_mutex );
               FT_Done_Face( face );
               pthread_mutex_unlock ( &library_mutex );
               DIRECT_DEALLOCATE_INTERFACE( thiz );
               return DFB_FAILURE;
          }
     }

     face->generic.data = (void *) load_flags;
     face->generic.finalizer = NULL;

     font = dfb_font_create( core );

     D_ASSERT( font->pixel_format == DSPF_ARGB ||
               font->pixel_format == DSPF_AiRGB ||
               font->pixel_format == DSPF_ARGB4444 ||
               font->pixel_format == DSPF_ARGB2554 ||
               font->pixel_format == DSPF_ARGB1555 ||
               font->pixel_format == DSPF_A8 ||
               font->pixel_format == DSPF_A1 );

     font->width        = (desc->flags & DFDESC_WIDTH) ? desc->width : 0;
     font->height       = (desc->flags & DFDESC_HEIGHT) ? desc->height : 0;
     font->maxadvance   = (desc->flags & DFDESC_FIXEDADVANCE) ? desc->fixed_advance : 
          FT_F26Dot6Ceil(face->size->metrics.max_advance);
     font->ascender     = FT_F26Dot6Ceil(face->size->metrics.ascender);
     font->descender    = FT_F26Dot6Ceil(face->size->metrics.descender);
     if (font->descender > 0)
     {
         font->descender *= -1;
     }
     font->line_height  = FT_F26Dot6Ceil(face->size->metrics.height);
     font->bbox.xMin    = FT_XEMUnits2Pixels(face->bbox.xMin); 
     font->bbox.yMin    = FT_YEMUnits2Pixels(face->bbox.yMin); 
     font->bbox.xMax    = FT_XEMUnits2Pixels(face->bbox.xMax); 
     font->bbox.yMax    = FT_YEMUnits2Pixels(face->bbox.yMax); 
     font->layout       = layout;

     D_DEBUG( "DirectFB/FontFT2: font is %s.\n", FT_IS_SCALABLE(face) ? "scalable" : "fixed" );
     D_DEBUG( "DirectFB/FontFT2: font->width (pixels)                   = %d\n", font->width );
     D_DEBUG( "DirectFB/FontFT2: font->height (pixels)                  = %d\n", font->height );
     D_DEBUG( "DirectFB/FontFT2: font->ascender (pixels)                = %d\n", font->ascender );
     D_DEBUG( "DirectFB/FontFT2: font->descender (pixels)               = %d\n", font->descender );
     D_DEBUG( "DirectFB/FontFT2: font->maxadvance (pixels)              = %d\n", font->maxadvance );
     D_DEBUG( "DirectFB/FontFT2: font->line_height (pixels)             = %d\n", font->line_height );
     D_DEBUG( "DirectFB/FontFT2: font->bbox                             = (%d, %d, %d, %d)\n", 
         font->bbox.xMin, font->bbox.yMin, font->bbox.xMax, font->bbox.yMax);
     D_DEBUG( "DirectFB/FontFT2: face->height (EM units)                = %d\n", face->height );
     D_DEBUG( "DirectFB/FontFT2: face->units_per_EM (EM units)          = %d\n", face->units_per_EM );
     D_DEBUG( "DirectFB/FontFT2: face->size->metrics.y_ppem (pixels)    = %d\n", face->size->metrics.y_ppem );
     D_DEBUG( "DirectFB/FontFT2: face->size->metrics.->y_scale          = %f\n", 
         (float)face->size->metrics.y_scale / (1 << 16) );
     D_DEBUG( "DirectFB/FontFT2: face->size->metrics.height (pixels)    = %f\n", 
         (float)face->size->metrics.height / (1 << 6) );

     font->RenderGlyph  = render_glyph;

     if (FT_HAS_KERNING(face) && !disable_kerning) {
          font->GetKerning = get_kerning;
          impl_data = D_CALLOC( 1, sizeof(FT2ImplKerningData) );
     }
     else
          impl_data = D_CALLOC( 1, sizeof(FT2ImplData) );

     impl_data->face              = face;
     impl_data->disable_charmap   = disable_charmap;
     impl_data->layout            = layout;
     impl_data->transform_enabled = (transform_matrix != NULL);
     impl_data->stroking_enabled = (desc->flags & DFDESC_ATTRIBUTES) ? (desc->attributes & DFFA_STROKING) : false;
     impl_data->stroking_lineCap = (FT_Stroker_LineCap) desc->stroking_lineCap;
     impl_data->stroking_lineJoin = (FT_Stroker_LineJoin) desc->stroking_lineJoin;
     impl_data->stroking_radius = desc->stroking_radius;
     impl_data->borderColorIndex = desc->borderColorIndex;
     impl_data->backgroundColorIndex = desc->backgroundColorIndex;
     impl_data->fontColorIndex = desc->fontColorIndex;
     impl_data->fixed_advance = (desc->flags & DFDESC_FIXEDADVANCE) ? desc->fixed_advance : 0;

     /*sanity check*/
     if (impl_data->stroking_enabled)
     {
        if ((desc->stroking_lineCap > FT_STROKER_LINECAP_SQUARE) || (desc->stroking_lineJoin > DFB_STROKER_LINEJOIN_MITER))
        {
            impl_data->stroking_enabled = 0;
        }
     }
 

     if (FT_HAS_KERNING(face) && !disable_kerning)
          init_kerning_cache( (FT2ImplKerningData*) impl_data );

     font->impl_data = impl_data;

     IDirectFBFont_Construct( thiz, font );

     thiz->Release = IDirectFBFont_FT2_Release;

     return DFB_OK;
}
