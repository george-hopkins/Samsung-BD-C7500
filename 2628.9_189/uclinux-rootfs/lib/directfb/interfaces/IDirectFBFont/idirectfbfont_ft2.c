/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002       convergence GmbH.
   
   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de> and
              Sven Neumann <sven@convergence.de>.

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <malloc.h>

#include <freetype/ftglyph.h>

#include <directfb.h>

#include <core/fonts.h>
#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <gfx/convert.h>

#include <media/idirectfbfont.h>

#include <misc/tree.h>
#include <misc/mem.h>


static DFBResult
Probe( IDirectFBFont_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBFont      *thiz,
           const char         *filename,
           DFBFontDescription *desc );

#include <interface_implementation.h>

DFB_INTERFACE_IMPLEMENTATION( IDirectFBFont, FT2 )

static FT_Library      library           = NULL;
static int             library_ref_count = 0;
static pthread_mutex_t library_mutex     = PTHREAD_MUTEX_INITIALIZER;

#define KERNING_CACHE_MIN   32
#define KERNING_CACHE_MAX  127
#define KERNING_CACHE_SIZE (KERNING_CACHE_MAX - KERNING_CACHE_MIN + 1)

#define KERNING_DO_CACHE(a,b)      ((a) >= KERNING_CACHE_MIN && \
                                    (a) <= KERNING_CACHE_MAX && \
                                    (b) >= KERNING_CACHE_MIN && \
                                    (b) <= KERNING_CACHE_MAX)
                               
#define KERNING_CACHE_ENTRY(a,b)   \
     (data->kerning[(a)-KERNING_CACHE_MIN][(b)-KERNING_CACHE_MIN])


typedef struct {
     FT_Face  face;
     int disable_charmap;
} FT2ImplData;


typedef struct {
     FT_Face  face;
     int disable_charmap;

     struct {
          int  x   : 16;
          int  y   : 15;
          bool got : 1;
     } kerning[KERNING_CACHE_SIZE][KERNING_CACHE_SIZE];
} FT2ImplKerningData;


static DFBResult
render_glyph( CoreFont      *thiz,
              unichar        glyph,
              CoreGlyphData *info,
              CoreSurface   *surface )
{
     FT_Error err;
     FT_Face  face;
     FT_Int   load_flags;
     FT_UInt  index;
     __u8 *src, *dst;
     int y;
     int pitch;
     FT2ImplData *data = (FT2ImplData*) thiz->impl_data;

     face = data->face;

     if (data->disable_charmap)
          index = glyph;
     else
          index = FT_Get_Char_Index( face, glyph );

     load_flags = (FT_Int) face->generic.data;
     load_flags |= FT_LOAD_RENDER;

     if ((err = FT_Load_Glyph( face, index, load_flags ))) {
          HEAVYDEBUGMSG( "DirectFB/FontFT2: "
                         "Could not render glyph for character #%d!\n", glyph );
          return DFB_FAILURE;
     }

     err = dfb_surface_soft_lock( surface, DSLF_WRITE,
                                  (void**)&dst, &pitch, 0 );
     if (err) {
          ERRORMSG( "DirectB/FontFT2: Unable to lock surface!\n" );
          return err;
     }

     info->width = face->glyph->bitmap.width;
     if (info->width + thiz->next_x > surface->width)
          info->width = surface->width - thiz->next_x;

     info->height = face->glyph->bitmap.rows;
     if (info->height > surface->height)
          info->height = surface->height;

     info->left = face->glyph->bitmap_left;
     info->top  = thiz->ascender - face->glyph->bitmap_top;

     src = face->glyph->bitmap.buffer;
     dst += thiz->next_x * DFB_BYTES_PER_PIXEL(surface->format);

     for (y=0; y < info->height; y++) {

          switch (face->glyph->bitmap.pixel_mode) {
          case ft_pixel_mode_grays:
               switch (surface->format) {
                    case DSPF_ARGB:
                         span_a8_to_argb( src, (__u32*) dst, info->width );
                         break;
                    case DSPF_A8:
                         memcpy( dst, src, info->width );
                         break;
                    default:
                         break;
               }
               break;

          case ft_pixel_mode_mono:
               switch (surface->format) {
                    case DSPF_ARGB:
                         span_a1_to_argb( src, (__u32*) dst, info->width );
                         break;
                    case DSPF_A8:
                         span_a1_to_a8( src, dst, info->width );
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

     dfb_surface_unlock( surface, 0 );

     return DFB_OK;
}


static DFBResult
get_glyph_info( CoreFont      *thiz,
                unichar        glyph,
                CoreGlyphData *info )
{
     FT_Error err;
     FT_Face  face;
     FT_Int   load_flags;
     FT_UInt  index;
     FT2ImplData *data = (FT2ImplData*) thiz->impl_data;

     face = data->face;

     if (data->disable_charmap)
          index = glyph;
     else
          index = FT_Get_Char_Index( face, glyph );

     load_flags = (FT_Int) face->generic.data;

     if ((err = FT_Load_Glyph( face, index, load_flags ))) {
          HEAVYDEBUGMSG( "DirectB/FontFT2: "
                         "Could not load glyph for character #%d!\n", glyph );
          return DFB_FAILURE;
     }

     if (face->glyph->format != ft_glyph_format_bitmap) {
          err = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
          if (err) {
               ERRORMSG( "DirectFB/FontFT2: Could not "
                         "render glyph for character #%d!\n", glyph );

               return DFB_FAILURE;
          }
     }

     info->width   = face->glyph->bitmap.width;
     info->height  = face->glyph->bitmap.rows;
     info->advance = face->glyph->advance.x >> 6;

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

     /*
      * Use cached values if characters are in the
      * cachable range and the cache entry is already filled.
      */
     if (KERNING_DO_CACHE (prev, current) &&
         KERNING_CACHE_ENTRY (prev, current).got)
     {
          *kern_x = KERNING_CACHE_ENTRY (prev, current).x;
          *kern_y = KERNING_CACHE_ENTRY (prev, current).y;

          return DFB_OK;
     }

     /* Get the character indices for lookup. */
     prev_index    = FT_Get_Char_Index( data->face, prev );
     current_index = FT_Get_Char_Index( data->face, current );

     /* Lookup kerning values for the character pair. */
     FT_Get_Kerning( data->face,
                     prev_index, current_index, ft_kerning_default, &vector );

     /* Convert to integer. */
     *kern_x = vector.x >> 6;
     *kern_y = vector.y >> 6;

     /*
      * Fill cache entry if characters are in the cachable range.
      */
     if (KERNING_DO_CACHE (prev, current)) {
          KERNING_CACHE_ENTRY (prev, current).x   = *kern_x;
          KERNING_CACHE_ENTRY (prev, current).y   = *kern_y;
          KERNING_CACHE_ENTRY (prev, current).got = true;
     }
     
     return DFB_OK;
}


static DFBResult
init_freetype( void )
{
     FT_Error err;
     
     pthread_mutex_lock ( &library_mutex );

     if (!library) {
          HEAVYDEBUGMSG( "DirectFB/FontFT2: "
                         "Initializing the FreeType2 library.\n" );
          err = FT_Init_FreeType( &library );
          if (err) {
               ERRORMSG( "DirectFB/FontFT2: "
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
          HEAVYDEBUGMSG( "DirectFB/FontFT2: "
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

          FT_Done_Face( impl_data->face );

          DFBFREE( impl_data );
          
          data->font->impl_data = NULL;
     }

     IDirectFBFont_Destruct( thiz );

     release_freetype();
}


static DFBResult
IDirectFBFont_FT2_Release( IDirectFBFont *thiz )
{
     INTERFACE_GET_DATA(IDirectFBFont)

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

     HEAVYDEBUGMSG( "DirectFB/FontFT2: Probe font `%s'.\n", ctx->filename );

     if (!ctx->filename)
          return DFB_UNSUPPORTED;

     if (init_freetype() != DFB_OK) {
          return DFB_FAILURE;
     }

     err = FT_New_Face( library, ctx->filename, -1, &face );

     release_freetype();
     
     return err ? DFB_UNSUPPORTED : DFB_OK;
}


static DFBResult
Construct( IDirectFBFont      *thiz,
           const char         *filename,
           DFBFontDescription *desc )
{
     CoreFont    *font;
     FT_Face      face;
     FT_Error     err;
     FT_Int       load_flags = FT_LOAD_DEFAULT;
     FT2ImplData *data;
     bool         disable_charmap = false;
     bool         disable_kerning = false;

     HEAVYDEBUGMSG( "DirectFB/FontFT2: "
                    "Construct font from file `%s' (index %d) at pixel size %d x %d.\n",
                    filename,
                    (desc->flags & DFDESC_INDEX)  ? desc->index  : 0,
                    (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                    (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );

     if (init_freetype() != DFB_OK) {
          DFB_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }

     err = FT_New_Face( library, filename,
                        (desc->flags & DFDESC_INDEX) ? desc->index : 0,
                        &face );
     if (err) {
          switch (err) {
          case FT_Err_Unknown_File_Format:
               ERRORMSG( "DirectFB/FontFT2: "
                         "Unsupported font format in file `%s'!\n", filename );
               break;
          default:
               ERRORMSG( "DirectFB/FontFT2: "
                         "Failed loading face %d from font file `%s'!\n",
                         (desc->flags & DFDESC_INDEX) ? desc->index : 0,
                         filename );
               break;
          }
          DFB_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }

     if (desc->flags & DFDESC_ATTRIBUTES) {
          if (desc->attributes & DFFA_NOHINTING)
               load_flags |= FT_LOAD_NO_HINTING;
          if (desc->attributes & DFFA_MONOCHROME)
               load_flags |= FT_LOAD_MONOCHROME;
          if (desc->attributes & DFFA_NOCHARMAP)
               disable_charmap = true;
          if (desc->attributes & DFFA_NOKERNING)
               disable_kerning = true;
     }

     if (!disable_charmap) {
          err = FT_Select_Charmap( face, ft_encoding_unicode );

#if FREETYPE_MINOR > 0
          /* ft_encoding_latin_1 has been introduced in freetype-2.1 */
          if (err) {
               HEAVYDEBUGMSG( "DirectFB/FontFT2: "
                              "Couldn't select Unicode encoding, "
			      "falling back to Latin1.\n");
               err = FT_Select_Charmap( face, ft_encoding_latin_1 );
          }
#endif
     }

#if 0
     if (err) {
          ERRORMSG( "DirectFB/FontFT2: "
                    "Couldn't select a suitable encoding for face %d from font file `%s'!\n", (desc->flags & DFDESC_INDEX) ? desc->index : 0, filename );
          FT_Done_Face( face );
          DFB_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }
#endif

     if (desc->flags & (DFDESC_HEIGHT | DFDESC_WIDTH)) {
          err = FT_Set_Pixel_Sizes( face,
                            (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                            (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
          if (err) {
               ERRORMSG( "DirectB/FontFT2: "
                         "Could not set pixel size to %d x %d!\n",
                         (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                         (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
               FT_Done_Face( face );
               DFB_DEALLOCATE_INTERFACE( thiz );
               return DFB_FAILURE;
          }
     }

     face->generic.data = (void *) load_flags;
     face->generic.finalizer = NULL;

     font = dfb_font_create();

     DFB_ASSERT( font->pixel_format == DSPF_ARGB || 
                 font->pixel_format == DSPF_A8 );

     font->ascender   = face->size->metrics.ascender >> 6;
     font->descender  = face->size->metrics.descender >> 6;
     font->height     = face->size->metrics.height >> 6;
     font->maxadvance = face->size->metrics.max_advance >> 6;

     HEAVYDEBUGMSG( "DirectFB/FontFT2: font->height = %d\n", font->height );
     HEAVYDEBUGMSG( "DirectFB/FontFT2: font->ascender = %d\n", font->ascender );
     HEAVYDEBUGMSG( "DirectFB/FontFT2: font->descender = %d\n",font->descender );

     font->GetGlyphInfo = get_glyph_info;
     font->RenderGlyph  = render_glyph;

     if (FT_HAS_KERNING(face) && !disable_kerning) {
          font->GetKerning = get_kerning;
          data = DFBCALLOC( 1, sizeof(FT2ImplKerningData) );
     } else
          data = DFBCALLOC( 1, sizeof(FT2ImplData) );

     data->face = face;
     data->disable_charmap = disable_charmap;
     
     font->impl_data = data;

     IDirectFBFont_Construct( thiz, font );

     thiz->Release = IDirectFBFont_FT2_Release;

     return DFB_OK;
}
