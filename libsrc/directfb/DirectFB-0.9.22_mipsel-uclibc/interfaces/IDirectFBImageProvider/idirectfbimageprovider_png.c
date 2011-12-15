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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <png.h>
#include <string.h>

#include <directfb.h>

#include <display/idirectfbsurface.h>

#include <media/idirectfbimageprovider.h>

#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/layers.h>
#include <core/palette.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <misc/gfx_util.h>

#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>
#include <misc/util.h>

#include <byteswap.h>
#define BYTESWAPPALETTE


#include <bcm/bcmcore.h>
#include <bmem.h>

#include "brcmpng.h"

extern IDirectFB *idirectfb_singleton;


static char *PNG_DECODER_LIBPNG_NAME = "libPNG";
static char *PNG_DECODER_BRCM_NAME = "BRCM PNG";
static char *PNG_DECODER_SID_NAME = "SID PNG";
static char *PNG_DECODER_UNKNOWN_NAME = "unknown PNG";

#define DECODER_TEST(result, test) \
    if (test) {\
        D_DEBUG("%s because (%s) is true\n", #result, #test); \
        snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s", #test); \
    } \
    result |= test;

extern const char *bsettop_get_config(const char *name);

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBImageProvider, PNG )


#define MAXCOLORMAPSIZE 256

enum {
     STAGE_ERROR = -1,
     STAGE_START =  0,
     STAGE_INFO,
     STAGE_IMAGE,
     STAGE_END
};

typedef enum png_decoder_e {
    PNG_DECODER_LIBPNG,
    PNG_DECODER_BRCM,
    PNG_DECODER_SID
} png_decoder;

/*
 * private data struct of IDirectFBImageProvider_PNG
 */
typedef struct {
     int                  ref;      /* reference counter */

     DFBSurfaceDescription surface_desc;

     IDirectFBDataBuffer *buffer;

     int                  stage;
     int                  rows;

     png_structp          png_ptr;
     png_infop            info_ptr;

     png_uint_32          width;
     png_uint_32          height;
     int                  bit_depth;
     int                  bits_per_pixel;
     int                  color_type;
     png_uint_32          color_key;
     bool                 color_keyed;
     png_decoder          decoder_type;
     char *               decoderReason[128];

     uint32_t             num_trans;
     uint8_t              trans[MAXCOLORMAPSIZE];

     __u32*               image;

     DIRenderCallback     render_callback;
     void*                render_callback_context;
     bool                 alloc_dinamic_input_buffer;
	 bool out_has_more;
} IDirectFBImageProvider_PNG_data;

static DFBResult
IDirectFBImageProvider_PNG_AddRef  ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_PNG_Release ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_PNG_RenderTo( IDirectFBImageProvider *thiz,
                                     IDirectFBSurface       *destination,
                                     const DFBRectangle     *destination_rect );

static DFBResult
IDirectFBImageProvider_PNG_SetRenderCallback( IDirectFBImageProvider *thiz,
                                              DIRenderCallback        callback,
                                              void                   *context );

static DFBResult
IDirectFBImageProvider_PNG_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                  DFBSurfaceDescription  *dsc );

static DFBResult
IDirectFBImageProvider_PNG_GetImageDescription( IDirectFBImageProvider *thiz,
                                                DFBImageDescription    *dsc );

int
LoadPNGPalette(CorePalette *dest_palette,
               IDirectFBImageProvider_PNG_data *data,
               int color_type);

/* Called at the start of the progressive load, once we have image info */
static void
png_info_callback (png_structp png_read_ptr,
                   png_infop   png_info_ptr);

/* Called for each row; note that you will get duplicate row numbers
   for interlaced PNGs */
static void
png_row_callback  (png_structp png_read_ptr,
                   png_bytep   new_row,
                   png_uint_32 row_num,
                   int         pass_num);

/* Called after reading the entire image */
static void
png_end_callback  (png_structp png_read_ptr,
                   png_infop   png_info_ptr);

/* Pipes data into libpng until stage is different from the one specified. */
static DFBResult
push_data_until_stage (IDirectFBImageProvider_PNG_data *data,
                       int                              stage,
                       int                              buffer_size);

static bresult
load_encoded_data(void *dst,
                    uint32_t dst_size,
                    uint32_t *length,
                    void *params);

static void png_get_IsAnimatedPNG(IDirectFBDataBuffer *thiz, unsigned int *isAnimated, unsigned int *num_frames);

static void
SetPNGDecoder(IDirectFBImageProvider_PNG_data *data)
{
     static bool env_force_libpng = false;
     static bool env_force_no_sid = false;
     static bool env_vars_checked = false;
     bool sid_png_cannot_decode = false;
     bool brcm_png_cannot_decode = false;
     uint32_t png_input_length;

     data->buffer->GetLength(data->buffer, &png_input_length);

     if (!env_vars_checked)
     {
          env_vars_checked = true;
          if (bsettop_get_config("force_libpng"))
          {
               D_INFO("force_libpng is set, using libPNG for decode...\n");
               env_force_libpng = true;
          }

          if (bsettop_get_config("force_no_sid") || bsettop_get_config("no_sid"))
          {
               D_INFO("force_no_sid is set, using libPNG and brcm PNG ONLY...\n");
               env_force_no_sid = true;
          }
     }

     {
          bresult limitations_ret;
          bdvd_sid_handle hSid = ((BCMCoreContext *)dfb_system_data())->driver_sid;
          bdvd_graphics_sid_limitations sid_limitations;
          limitations_ret = bdvd_sid_get_limitations(hSid, &sid_limitations);
          DECODER_TEST(sid_png_cannot_decode, limitations_ret != b_ok);
          DECODER_TEST(sid_png_cannot_decode, env_force_no_sid);
          DECODER_TEST(sid_png_cannot_decode, data->info_ptr->interlace_type == PNG_INTERLACE_ADAM7);
          DECODER_TEST(sid_png_cannot_decode, (data->color_type == PNG_COLOR_TYPE_PALETTE) && (data->width > sid_limitations.max_png_palette_width));
          DECODER_TEST(sid_png_cannot_decode, (data->color_type != PNG_COLOR_TYPE_PALETTE) && (data->width > sid_limitations.max_png_rgba_width));
          DECODER_TEST(sid_png_cannot_decode, (data->bit_depth >= 16) && (data->width > sid_limitations.max_png_16bpp_width));
          data->alloc_dinamic_input_buffer = png_input_length > sid_limitations.max_input_bytes;
    }

     DECODER_TEST(brcm_png_cannot_decode, env_force_libpng);
     DECODER_TEST(brcm_png_cannot_decode, data->bit_depth >= 16);

	 /*For now use libpng (instead of brcm_png) if SID cannot decode the PNG image*/
 	 brcm_png_cannot_decode = true;

     if (sid_png_cannot_decode && brcm_png_cannot_decode)
          data->decoder_type = PNG_DECODER_LIBPNG;
     else if (sid_png_cannot_decode)
          data->decoder_type = PNG_DECODER_BRCM;
     else
          data->decoder_type = PNG_DECODER_SID;

}

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx )
{
     if (png_check_sig( ctx->header, 8 ))
          return DFB_OK;

     return DFB_UNSUPPORTED;
}

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer )
{
     DFBResult ret = DFB_FAILURE;

     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBImageProvider_PNG)

     data->ref    = 1;
     data->buffer = buffer;
     data->num_trans = 0;


     /* Increase the data buffer reference counter. */
     buffer->AddRef( buffer );

     /* Create the PNG read handle. */
     data->png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
                                             NULL, NULL, NULL );
     if (!data->png_ptr)
          goto error;

     /* Create the PNG info handle. */
     data->info_ptr = png_create_info_struct( data->png_ptr );
     if (!data->info_ptr)
          goto error;

     /* Setup progressive image loading. */
     png_set_progressive_read_fn( data->png_ptr, data,
                                  png_info_callback,
                                  png_row_callback,
                                  png_end_callback );


     /* Read until info callback is called. */
     ret = push_data_until_stage( data, STAGE_INFO, 64 );
     if (ret)
          goto error;

     thiz->AddRef              = IDirectFBImageProvider_PNG_AddRef;
     thiz->Release             = IDirectFBImageProvider_PNG_Release;
     thiz->RenderToUnaltered   = IDirectFBImageProvider_PNG_RenderTo;
     thiz->SetRenderCallback   = IDirectFBImageProvider_PNG_SetRenderCallback;
     thiz->GetImageDescription = IDirectFBImageProvider_PNG_GetImageDescription;
     thiz->GetSurfaceDescriptionUnaltered = IDirectFBImageProvider_PNG_GetSurfaceDescription;

	 thiz->AnimationDescription.firstFrame = true;

     data->decoder_type = PNG_DECODER_LIBPNG;
     snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "best decoder available");
     return DFB_OK;

error:
     if (data->png_ptr)
          png_destroy_read_struct( &data->png_ptr, &data->info_ptr, NULL );

     buffer->Release( buffer );

     if (data->image)
          D_FREE( data->image );

     DIRECT_DEALLOCATE_INTERFACE(thiz);

     return ret;
}

static void
IDirectFBImageProvider_PNG_Destruct( IDirectFBImageProvider *thiz )
{
     IDirectFBImageProvider_PNG_data *data =
                              (IDirectFBImageProvider_PNG_data*)thiz->priv;

     png_destroy_read_struct( &data->png_ptr, &data->info_ptr, NULL );

     /* Decrease the data buffer reference counter. */
     data->buffer->Release( data->buffer );

     /* Deallocate image data. */
     if (data->image)
          D_FREE( data->image );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult
IDirectFBImageProvider_PNG_AddRef( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_PNG)

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_PNG_Release( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_PNG)

     if (--data->ref == 0) {
          IDirectFBImageProvider_PNG_Destruct( thiz );
     }

     return DFB_OK;
}


static DFBResult
IDirectFBImageProvider_PNG_RenderTo( IDirectFBImageProvider *thiz,
                                     IDirectFBSurface       *destination,
                                     const DFBRectangle     *dest_rect )
{
     DFBResult              ret;
     IDirectFBSurface_data *dst_data;
     CoreSurface           *dst_surface;
     DFBRectangle           rect = { 0, 0, 0, 0 };
     void                  *dst;
     int                    destPitch;
     DFBSurfacePixelFormat  format;

     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_PNG)

     dst_data = (IDirectFBSurface_data*) destination->priv;
     if (!dst_data)
          return DFB_DEAD;

     D_DEBUG("IDirectFBImageProvider_PNG_RenderTo(this=0x%08X, data=0x%08X, data->buffer=0x%08X, data->png_ptr=0x%08X, data->info_ptr=0x%08X, data->image=0x%08X\n", (int) thiz, (int) data, (int) data->buffer, (int) data->png_ptr, (int) data->info_ptr, (int) data->image);

     dst_surface = dst_data->surface;
     if (!dst_surface)
          return DFB_DESTROYED;

     ret = destination->GetSize( destination, &rect.w, &rect.h );
     if (ret)
          return ret;

     ret = destination->GetPixelFormat( destination, &format );
     if (ret)
          return ret;

     ret = destination->Lock( destination, DSLF_WRITE, &dst, &destPitch );
     if (ret)
          return ret;

     if ((data->decoder_type == PNG_DECODER_BRCM) || (data->decoder_type == PNG_DECODER_SID))
     {
          PngErrCode_t pngErr = 0;
          UInt8 *file_buffer = NULL;
          UInt8 *temp_decode_buffer = NULL;
          UInt8 *decode_buffer = NULL;
          bool decode_buffer_needed = false;
          UInt32 max_line_length =0;
          UInt32 bytesPerPixel = 4;

          if (dest_rect != NULL)
               dfb_rectangle_intersect ( &rect, dest_rect );

          if ((rect.w != data->width) || (rect.h != data->height) || (rect.x != 0) || (rect.y != 0))
          {
               snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s: Scaling needed", __FUNCTION__);
               data->decoder_type = PNG_DECODER_BRCM;
               decode_buffer_needed = true;
          }

          if (data->decoder_type == PNG_DECODER_SID)
          {
               ret = dfb_surface_hardware_lock( dst_surface, dst_surface->back_buffer, DSLF_WRITE );
               if (ret == DFB_OK)
               {
                    bresult bret;
                    bdvd_graphics_sid_decode_params dec_params;
                    bdvd_sid_handle hSid = ((BCMCoreContext *)dfb_system_data())->driver_sid;

                    IDirectFBSurface      *data_surface;

                    dec_params.image_format = BGFX_SID_IMAGE_FORMAT_PNG;
                    destination->GetBackBufferContext(destination, (void**)&dec_params.dst_surface);
                    dec_params.load_data = load_encoded_data;
                    dec_params.load_data_params = data;
                    dec_params.src = NULL;
                    dec_params.cb_func = NULL;
                    dec_params.ext_input_buffer = NULL;
					dec_params.firstImage = thiz->AnimationDescription.firstFrame;

					if (thiz->AnimationDescription.firstFrame == true)
					{			   		
						thiz->AnimationDescription.firstFrame = false;
					}

				    /* Create dinamic input buffer */
				    if (data->alloc_dinamic_input_buffer)
				    {
					    void *surface_addr;
					    int surface_pitch;
					    unsigned int png_input_length;
					    DFBSurfaceDescription  data_surface_desc;


					    data->buffer->GetLength(data->buffer, &png_input_length);
					    data_surface_desc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT);
					    data_surface_desc.caps = (DFBSurfaceCapabilities)(DSCAPS_VIDEOONLY);
					    data_surface_desc.pixelformat = DSPF_LUT8;
					    data_surface_desc.width = SURFACE_MAX_WIDTH;
					    data_surface_desc.height = (png_input_length + SURFACE_MAX_WIDTH)/SURFACE_MAX_WIDTH;

#if 0
					    D_INFO("Input size is %d, Used %dx%d surface\n",
					    png_input_length,
					    data_surface_desc.width,
					    data_surface_desc.height);
#endif
					    idirectfb_singleton->CreateSurface(idirectfb_singleton, &data_surface_desc, &data_surface);
					    data_surface->GetBackBufferContext(data_surface, (void**)&dec_params.ext_input_buffer);
				    }

                    bret = bdvd_sid_decode(hSid, &dec_params);

					

					bdvd_sid_populate_AnimatedImageInfo(hSid, &thiz->AnimationDescription.frameDelay, &data->out_has_more, &thiz->AnimationDescription.x_offset, &thiz->AnimationDescription.y_offset, &thiz->AnimationDescription.width, &thiz->AnimationDescription.height, &thiz->AnimationDescription.dispose_op, &thiz->AnimationDescription.blend_op, &thiz->AnimationDescription.isPartOfAnimation);

					if ((thiz->AnimationDescription.firstFrame == false)&&(data->out_has_more == false))
				    {			   		
						thiz->AnimationDescription.firstFrame = true;
				    }


      			    /* Release dinamic input buffer */
      			    if (data->alloc_dinamic_input_buffer)
				    {
					    data_surface->Release(data_surface);
				    }

                    if (bret != b_ok)
                    {
                         D_ERROR("Error in bdvd_sid_decode (%d)! Using brcm png\n", bret);
                         snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s: SID failed", __FUNCTION__);

                         data->decoder_type = PNG_DECODER_BRCM;
                    }
                    dfb_surface_unlock( dst_surface, dst_surface->back_buffer, 0 );


               }
               else
               {
                    D_ERROR("Unable to lock the surface for use with the SID! Using brcm png\n");
                    snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s: Unable to lock", __FUNCTION__);
                    data->decoder_type = PNG_DECODER_BRCM;
               }
          }

          if (data->decoder_type == PNG_DECODER_BRCM)
          {
               unsigned int file_length;
               unsigned int buffer_length;
               ImgInfo_t *pImgInfo;

               pImgInfo = (ImgInfo_t *)D_MALLOC(sizeof(ImgInfo_t));

               ret = data->buffer->SeekTo( data->buffer, 0 );
               ret = data->buffer->GetLength(data->buffer, &file_length);
               file_buffer = D_MALLOC(file_length);
               ret = data->buffer->GetData( data->buffer, file_length, file_buffer, &buffer_length );

               pImgInfo->out_buf = dst;
               pImgInfo->out_buf_pitch = destPitch;
               pImgInfo->in_buf = file_buffer;

               switch(format)
               {
               case DSPF_LUT8:
               case DSPF_LUT4:
               case DSPF_LUT2:
               case DSPF_LUT1:
                    if (data->color_type == PNG_COLOR_TYPE_PALETTE)
                    {
                      pImgInfo->out_fmt = OUT_PALETTE;
                      bytesPerPixel = 1;
                    }
                    if (data->color_type == PNG_COLOR_TYPE_GRAY)
                    {
                      pImgInfo->out_fmt = OUT_GREY;
                      bytesPerPixel = 1;
                    }
                    break;
               case DSPF_ALUT88:
                    if (data->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                    {
                      pImgInfo->out_fmt = OUT_GREY_ALPHA;
                      bytesPerPixel = 2;
                    }
                    break;
               case DSPF_ARGB:
                    bytesPerPixel = 4;
                    pImgInfo->out_fmt = OUT_ARGB;
                    break;
               case DSPF_ABGR:
                    bytesPerPixel = 4;
                    pImgInfo->out_fmt = OUT_ABGR;
                    break;
               default:
                    break;
               }

               if (decode_buffer_needed)
               {
                    // If we are going to scale, we need to allocate a temp decode buffer.  Allocate a max size (RGBA) buffer to decode into.
                    pImgInfo->out_buf_pitch = data->width*bytesPerPixel;
                    decode_buffer = D_MALLOC(pImgInfo->out_buf_pitch*data->height);
                    pImgInfo->out_buf = decode_buffer;
               }

               max_line_length = (data->width*bytesPerPixel) > destPitch ? (data->width*bytesPerPixel) : destPitch;
               temp_decode_buffer = D_MALLOC(max_line_length*2); // provide 2 lines of RGBA temporary storage to the png decoder.
               pImgInfo->temp_buff = temp_decode_buffer;

               pngErr = PNGDecode(pImgInfo);
               if (pngErr)
                    D_ERROR("brcm_png decoder returned error 0x%08x\n", pngErr);

               if (file_buffer)
                    D_FREE(file_buffer);

               if (temp_decode_buffer)
                    D_FREE(temp_decode_buffer);

               if (pImgInfo)
               	    D_FREE(pImgInfo);
          }

          if ((dst_surface->palette != NULL) && (dst_surface->palette->entries != NULL))
          {
               uint32_t palette_size;
               palette_size = LoadPNGPalette(dst_surface->palette, data, data->color_type);
               dfb_palette_update( dst_surface->palette, 0, palette_size - 1 );
          }

          if (decode_buffer_needed && decode_buffer)
          {
               UInt8 *temp_rgb_buff = D_MALLOC(data->width * data->height * 4);
               dfb_copy_to_argb( decode_buffer, data->width, data->height, data->width*bytesPerPixel, format, temp_rgb_buff, data->width * 4, dst_surface->palette );

               dfb_scale_linear_32( (__u32*)temp_rgb_buff, data->width, data->height,
                                   dst, destPitch, &rect, dst_surface );

               if (temp_rgb_buff)
                    D_FREE(temp_rgb_buff);
          }

          if (decode_buffer)
               D_FREE(decode_buffer);

     }
     else
     {
          /* Read until image is completely decoded. */
          ret = push_data_until_stage( data, STAGE_END, 16384 );
          if (ret) D_ERROR("lib_png decoder returned error 0x%08x\n", ret);

          /* actual rendering */
          if (dest_rect == NULL || dfb_rectangle_intersect ( &rect, dest_rect )) {
               dfb_scale_linear_32( data->image, data->width, data->height,
                                    dst, destPitch, &rect, dst_surface );
         }
     }
     destination->Unlock( destination );
     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_PNG_SetRenderCallback( IDirectFBImageProvider *thiz,
                                              DIRenderCallback        callback,
                                              void                   *context )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_PNG)

     data->render_callback         = callback;
     data->render_callback_context = context;

     return DFB_OK;
}

/* Loading routines */

static DFBResult
IDirectFBImageProvider_PNG_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                  DFBSurfaceDescription *dsc )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_PNG)

     if (data->surface_desc.flags == 0)
     {
          data->surface_desc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
          data->surface_desc.caps  |= DSCAPS_VIDEOONLY;
          data->surface_desc.width  = data->width;
          data->surface_desc.height = data->height;
		  data->surface_desc.background_color.a = 0x0;
		  data->surface_desc.background_color.r = 0x0;
		  data->surface_desc.background_color.g = 0x0;
		  data->surface_desc.background_color.b = 0x0;

          SetPNGDecoder(data);

          if (data->decoder_type == PNG_DECODER_LIBPNG)
          {
               if (data->color_type & PNG_COLOR_MASK_ALPHA)
                    data->surface_desc.pixelformat = DSPF_ARGB;
               else
                    data->surface_desc.pixelformat = dfb_primary_layer_pixelformat();
          }
          else
          {
               switch(data->color_type) {
               case PNG_COLOR_TYPE_GRAY:
               case PNG_COLOR_TYPE_PALETTE:
                    if (data->bit_depth == 1)
                         data->surface_desc.pixelformat = DSPF_LUT1;
                    else if (data->bit_depth == 2)
                         data->surface_desc.pixelformat = DSPF_LUT2;
                    else if (data->bit_depth == 4)
                         data->surface_desc.pixelformat = DSPF_LUT4;
                    else
                         data->surface_desc.pixelformat = DSPF_LUT8;
                    break;
               case PNG_COLOR_TYPE_RGB:
               case PNG_COLOR_TYPE_RGB_ALPHA:
                    data->surface_desc.pixelformat = DSPF_ARGB;
                    break;
               case PNG_COLOR_TYPE_GRAY_ALPHA:
                    data->surface_desc.pixelformat = DSPF_ALUT88;
                    break;

               }
          }
     }

	 if (data->decoder_type != PNG_DECODER_SID)
	 {
		data->surface_desc.isAnimated = false;
		data->surface_desc.num_frames = 0;
	 }

     *dsc = data->surface_desc;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_PNG_GetImageDescription( IDirectFBImageProvider *thiz,
                                                DFBImageDescription    *dsc )
{
     char *decoderName;
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_PNG)

     if (!dsc)
          return DFB_INVARG;

     dsc->caps = DICAPS_NONE;


     if ((data->color_type == PNG_COLOR_TYPE_GRAY) && (data->num_trans == 1))
     {
          data->color_keyed = true;
          data->color_key = ((data->trans[0] << 16) | (data->trans[0] <<8) | (data->trans[0]));
     }
     else if ((data->color_type == PNG_COLOR_TYPE_PALETTE) && (data->num_trans != 0))
     {
          data->color_keyed = true;
     }

     if (data->color_type & PNG_COLOR_MASK_ALPHA)
          dsc->caps |= DICAPS_ALPHACHANNEL;

     if (data->color_keyed) {
          dsc->caps |= DICAPS_COLORKEY;

          dsc->colorkey_r = (data->color_key & 0xff0000) >> 16;
          dsc->colorkey_g = (data->color_key & 0x00ff00) >>  8;
          dsc->colorkey_b = (data->color_key & 0x0000ff);
     }

     switch (data->decoder_type){
     case PNG_DECODER_LIBPNG:
          decoderName = PNG_DECODER_LIBPNG_NAME;
          break;
     case PNG_DECODER_BRCM:
          decoderName = PNG_DECODER_BRCM_NAME;
          break;
     case PNG_DECODER_SID:
          decoderName = PNG_DECODER_SID_NAME;
          break;
     default:
          decoderName = PNG_DECODER_UNKNOWN_NAME;
          break;
     }
     snprintf(dsc->decoderName, sizeof(dsc->decoderName) - 1, "%s", decoderName);
     snprintf(dsc->decoderReason, sizeof(dsc->decoderReason) - 1, "%s", data->decoderReason);

     return DFB_OK;
}


static int SortColors (const void *a, const void *b)
{
     return (*((const __u8 *) a) - *((const __u8 *) b));
}

/*  looks for a color that is not in the colormap and ideally not
    even close to the colors used in the colormap  */
static __u32 FindColorKey( int n_colors, __u8 cmap[3][MAXCOLORMAPSIZE] )
{
     __u32 color = 0xFF000000;
     __u8  csort[MAXCOLORMAPSIZE];
     int   i, j, index, d;

     if (n_colors < 1)
          return color;

     D_ASSERT( n_colors <= MAXCOLORMAPSIZE );

     for (i = 0; i < 3; i++) {
          direct_memcpy( csort, cmap[i], n_colors );
          qsort( csort, n_colors, 1, SortColors );

          for (j = 1, index = 0, d = 0; j < n_colors; j++) {
               if (csort[j] - csort[j-1] > d) {
                    d = csort[j] - csort[j-1];
                    index = j;
               }
          }
          if ((csort[0] - 0x0) > d) {
               d = csort[0] - 0x0;
               index = n_colors;
          }
          if (0xFF - (csort[n_colors - 1]) > d) {
               index = n_colors + 1;
          }

          if (index < n_colors)
               csort[0] = csort[index] - (d/2);
          else if (index == n_colors)
               csort[0] = 0x0;
          else
               csort[0] = 0xFF;

          color |= (csort[0] << (8 * (2 - i)));
     }

     return color;
}

/* Called at the start of the progressive load, once we have image info */
static void
png_info_callback   (png_structp png_read_ptr,
                     png_infop   png_info_ptr)
{
     IDirectFBImageProvider_PNG_data *data;

     data = png_get_progressive_ptr( png_read_ptr );

     /* error stage? */
     if (data->stage < 0)
          return;

     /* set info stage */
     data->stage = STAGE_INFO;

     png_get_IHDR( data->png_ptr, data->info_ptr,
                   &data->width, &data->height, &data->bit_depth, &data->color_type,
                   NULL, NULL, NULL );

	 png_get_IsAnimatedPNG(data->buffer, &data->surface_desc.isAnimated, &data->surface_desc.num_frames);

     data->bits_per_pixel = data->bit_depth;
     switch(data->color_type) {
       case PNG_COLOR_TYPE_RGB:        data->bits_per_pixel*=3; break;
       case PNG_COLOR_TYPE_GRAY_ALPHA: data->bits_per_pixel*=2; break;
       case PNG_COLOR_TYPE_RGB_ALPHA:  data->bits_per_pixel*=4; break;
     }

     if (data->color_type == PNG_COLOR_TYPE_PALETTE)
          png_set_palette_to_rgb( data->png_ptr );

     if (data->color_type == PNG_COLOR_TYPE_GRAY
         || data->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb( data->png_ptr );

     if (png_get_valid( data->png_ptr, data->info_ptr, PNG_INFO_tRNS )) {

          /* generate color key based on palette... */
          if (data->color_type == PNG_COLOR_TYPE_PALETTE) {
               int        i;
               __u32      key;
               png_colorp palette    = data->info_ptr->palette;
               png_bytep  trans      = data->info_ptr->trans;
               int        num_colors = MIN( MAXCOLORMAPSIZE,
                                            data->info_ptr->num_palette );
               __u8       cmap[3][num_colors];

               for (i=0; i<num_colors; i++) {
                    cmap[0][i] = palette[i].red;
                    cmap[1][i] = palette[i].green;
                    cmap[2][i] = palette[i].blue;
               }

               key = FindColorKey( num_colors, cmap );

               for (i=0; i<data->info_ptr->num_trans; i++) {
                    if (!trans[i]) {
                         //trans[i] = 0;

                         palette[i].red   = (key & 0xff0000) >> 16;
                         palette[i].green = (key & 0x00ff00) >>  8;
                         palette[i].blue  = (key & 0x0000ff);
                    }
                    data->trans[i] = data->info_ptr->trans[i];
               }
               data->num_trans = data->info_ptr->num_trans;

               data->color_key = key;
          }
          else if (data->color_type == PNG_COLOR_TYPE_GRAY)
          {
               data->num_trans = 1;
               data->trans[0] = (uint8_t)(data->info_ptr->trans_values.gray  & 0xff);
          }
          else {
               uint8_t r, g, b;
               /* ...or based on trans rgb value */
               data->color_keyed = true;

               png_color_16p trans = &data->info_ptr->trans_values;
               r = (uint8_t)(trans->red & 0xff);
               g = (uint8_t)(trans->green & 0xff);
               b = (uint8_t)(trans->blue & 0xff);

               data->color_key = ((r << 16) | (g << 8) | (b << 0));
          }
     }

     if (data->bit_depth == 16)
          png_set_strip_16( data->png_ptr );

#ifdef WORDS_BIGENDIAN
     if (!(data->color_type & PNG_COLOR_MASK_ALPHA))
          png_set_filler( data->png_ptr, 0xFF, PNG_FILLER_BEFORE );

     png_set_swap_alpha( data->png_ptr );
#else
     if (!(data->color_type & PNG_COLOR_MASK_ALPHA))
          png_set_filler( data->png_ptr, 0xFF, PNG_FILLER_AFTER );

     png_set_bgr( data->png_ptr );
#endif

     png_set_interlace_handling( data->png_ptr );

     /* Update the info to reflect our transformations */
     png_read_update_info( data->png_ptr, data->info_ptr );
}

/* Called for each row; note that you will get duplicate row numbers
   for interlaced PNGs */
static void
png_row_callback   (png_structp png_read_ptr,
                    png_bytep   new_row,
                    png_uint_32 row_num,
                    int         pass_num)
{
     IDirectFBImageProvider_PNG_data *data;

     data = png_get_progressive_ptr( png_read_ptr );

     /* error stage? */
     if (data->stage < 0)
          return;

     /* set image decoding stage */
     data->stage = STAGE_IMAGE;

     /* check image data pointer */
     if (!data->image) {
          // FIXME: allocates four additional bytes because the scaling functions
          //        in src/misc/gfx_util.c have an off-by-one bug which causes
          //        segfaults on darwin/osx (not on linux)
          int size = data->width * data->height * 4 + 4 ;

          /* allocate image data */
          data->image = D_MALLOC( size );
          if (!data->image) {
               D_ERROR("DirectFB/ImageProvider_PNG: Could not "
                        "allocate %d bytes of system memory!\n", size);

               /* set error stage */
               data->stage = STAGE_ERROR;

               return;
          }
     }

     /* write to image data */
     png_progressive_combine_row( data->png_ptr, (png_bytep) (data->image +
                                  row_num * data->width), new_row );

     /* increase row counter, FIXME: interlaced? */
     data->rows++;

     if (data->render_callback) {
          DFBRectangle rect = { 0, row_num, data->width, 1 };

          data->render_callback( &rect, data->render_callback_context );
     }
}

/* Called after reading the entire image */
static void
png_end_callback   (png_structp png_read_ptr,
                    png_infop   png_info_ptr)
{
     IDirectFBImageProvider_PNG_data *data;

     data = png_get_progressive_ptr( png_read_ptr );

     /* error stage? */
     if (data->stage < 0)
          return;

     /* set end stage */
     data->stage = STAGE_END;
}

/* Pipes data into libpng until stage is different from the one specified. */
static DFBResult
push_data_until_stage (IDirectFBImageProvider_PNG_data *data,
                       int                              stage,
                       int                              buffer_size)
{
     DFBResult            ret;
     IDirectFBDataBuffer *buffer = data->buffer;

     while (data->stage < stage) {
          unsigned int  len;
          unsigned char buf[buffer_size];

          if (data->stage < 0)
               return DFB_FAILURE;

          while (buffer->HasData( buffer ) == DFB_OK) {
               D_DEBUG( "ImageProvider/PNG: Retrieving data (up to %d bytes)...\n", buffer_size );

               ret = buffer->GetData( buffer, buffer_size, buf, &len );
               if (ret)
                    return ret;

               D_DEBUG( "ImageProvider/PNG: Got %d bytes...\n", len );

               png_process_data( data->png_ptr, data->info_ptr, buf, len );

               D_DEBUG( "ImageProvider/PNG: ...processed %d bytes.\n", len );

               /* are we there yet? */
               if (data->stage < 0 || data->stage >= stage)
                    return DFB_OK;
          }

          D_DEBUG( "ImageProvider/PNG: Waiting for data...\n" );

          if (buffer->WaitForData( buffer, 1 ) == DFB_EOF)
               return DFB_FAILURE;
     }

     return DFB_OK;
}


int LoadPNGPalette(CorePalette *dest_palette, IDirectFBImageProvider_PNG_data *data, int color_type)
{
     uint32_t i;
     uint32_t palette_size = 0;

     if ((dest_palette != NULL) || (dest_palette->entries != NULL))
     {
          if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
          {
               uint32_t val = 0;
               palette_size = 1 << MIN( data->bit_depth, 8);
               uint32_t step = 0xff / (palette_size - 1);

               for(i = 0; i < palette_size; i++)
               {
#ifdef BYTESWAPPALETTE
                    dest_palette->entries[i].b = 0xff;
                    dest_palette->entries[i].g = val;
                    dest_palette->entries[i].r = val;
                    dest_palette->entries[i].a = val;
#else
                    dest_palette->entries[i].a = 0xff;
                    dest_palette->entries[i].r = val;
                    dest_palette->entries[i].g = val;
                    dest_palette->entries[i].b = val;
#endif
                    val += step;
               }

               if ((color_type == PNG_COLOR_TYPE_GRAY) && (data->num_trans == 1) && (data->trans[0] < palette_size))
               {
#ifdef BYTESWAPPALETTE
                    dest_palette->entries[data->trans[0]].b = 0;
#else
                    dest_palette->entries[data->trans[0]].a = 0;
#endif
               }
          }
          else if (color_type == PNG_COLOR_TYPE_PALETTE)
          {
               if ((data != NULL) || (data->info_ptr != NULL) || (data->info_ptr->palette != NULL))
               {
                    palette_size = MIN( MAXCOLORMAPSIZE, data->info_ptr->num_palette );
                    for(i = 0; i < palette_size; i++)
                    {
#ifdef BYTESWAPPALETTE
                         dest_palette->entries[i].b = 0xff;
                         dest_palette->entries[i].g = data->info_ptr->palette[i].red;
                         dest_palette->entries[i].r = data->info_ptr->palette[i].green;
                         dest_palette->entries[i].a = data->info_ptr->palette[i].blue;
#else
                         dest_palette->entries[i].a = 0xff;
                         dest_palette->entries[i].r = data->info_ptr->palette[i].red;
                         dest_palette->entries[i].g = data->info_ptr->palette[i].green;
                         dest_palette->entries[i].b = data->info_ptr->palette[i].blue;
#endif
                    }
               }

               for (i=0; i<data->num_trans; i++)
               {
                    if (i < palette_size)
                    {
#ifdef BYTESWAPPALETTE
                         dest_palette->entries[i].b = data->trans[i];
#else
                         dest_palette->entries[i].a = data->trans[i];
#endif
                    }
               }

          }
     }

     return palette_size;
}


static bresult load_encoded_data(void *dst, uint32_t dst_size, uint32_t *length, void *params)
{
     bresult bret = b_ok;
     DFBResult ret;
     uint32_t file_length;
     IDirectFBImageProvider_PNG_data *data = (IDirectFBImageProvider_PNG_data *)params;

     ret = data->buffer->SeekTo( data->buffer, 0 );
     if (ret != DFB_OK)
     {
          D_ERROR("Error in load_encoded_data: data->buffer->SeekTo() Failed!\n");
          bret = berr_external_error;
     }

     ret = data->buffer->GetLength(data->buffer, &file_length);
     if (ret != DFB_OK)
     {
          D_ERROR("Error in load_encoded_data: data->buffer->GetLength() Failed!\n");
          bret = berr_external_error;
     }

     if (dst_size < file_length)
     {
          D_ERROR("Error in load_encoded_data: file_length(%d) is greater than dst_size(%d)!\n", file_length, dst_size);
          bret = berr_external_error;
     }

     ret = data->buffer->GetData( data->buffer, file_length, dst, length );
     if (ret != DFB_OK)
     {
          D_ERROR("Error in load_encoded_data: data->buffer->GetData() Failed!\n");
          bret = berr_external_error;
     }

     return bret;
}


static void png_get_IsAnimatedPNG(IDirectFBDataBuffer *thiz, unsigned int *isAnimated, unsigned int *num_frames)
{
	unsigned int bytes_read;
	unsigned int chunk_type;
	unsigned int pos;
	unsigned int current_pos;
	unsigned int chunk_len;
	unsigned int length;
	
	chunk_type = 0;
	*isAnimated = 0;
	*num_frames = 0;
	
	thiz->GetPosition(thiz, &pos);
	thiz->SeekTo(thiz, 8);

	thiz->GetPosition(thiz, &current_pos);
	thiz->GetLength(thiz, &length);
	
	
	while (current_pos < length)
	{
		thiz->GetData(thiz, 4, &chunk_len, &bytes_read);
		chunk_len = ((((chunk_len) & 0x000000ff) << 24) | (((chunk_len) & 0x0000ff00) << 8) | (((chunk_len) & 0x00ff0000) >> 8) | (((chunk_len) & 0xff000000) >> 24));
		
		thiz->GetData(thiz, 4, &chunk_type, &bytes_read);
		
		

		if (chunk_type == (('L' << 24) | ('T' << 16) | ('c' << 8) | 'a'))
		{
			*isAnimated = 1;
			thiz->GetData(thiz, 4, num_frames, &bytes_read);
			*num_frames = ((((*num_frames) & 0x000000ff) << 24) | (((*num_frames) & 0x0000ff00) << 8) | (((*num_frames) & 0x00ff0000) >> 8) | (((*num_frames) & 0xff000000) >> 24));
			break;
		}
		else
		if (chunk_type == (('T' << 24) | ('A' << 16) | ('D' << 8) | 'I'))	
		{
			break;
		}
		else
		{		
			thiz->GetPosition(thiz, &current_pos);
			thiz->SeekTo(thiz, current_pos + chunk_len + 4);
		}		
			
	}	

	thiz->SeekTo(thiz, pos);
	
}