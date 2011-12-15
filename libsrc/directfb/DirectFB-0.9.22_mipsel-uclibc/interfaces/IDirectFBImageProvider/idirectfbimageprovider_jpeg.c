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
#include <errno.h>
#include <string.h>


#include <directfb.h>

#include <display/idirectfbsurface.h>

#include <media/idirectfbimageprovider.h>
#include <media/idirectfbdatabuffer.h>

#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/layers.h>
#include <core/palette.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <misc/gfx_util.h>
#include <misc/util.h>
#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/messages.h>

#include <jpeglib.h>
#include <setjmp.h>
#include <math.h>
#include <bdvd.h>
#include <bdvd_ngraphics.h>

#include <bcm/bcmcore.h>

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBImageProvider, JPEG )

static char *JPG_DECODER_LIBJPG_NAME = "libJPG";
static char *JPG_DECODER_MVD_NAME = "MVD JPG";
static char *JPG_DECODER_SID_NAME = "SID JPG";
static char *JPG_DECODER_UNKNOWN_NAME = "unknown JPG";

#define DECODER_TEST(result, test) \
    if (test) {\
        D_DEBUG("%s because (%s) is true\n", #result, #test); \
        snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s", #test); \
    } \
    result |= test;

extern const char *bsettop_get_config(const char *name);
extern IDirectFB *idirectfb_singleton;

typedef enum jpeg_decoder_e {
    JPG_DECODER_LIBJPG,
    JPG_DECODER_MVD,
    JPG_DECODER_SID
} jpeg_decoder;

typedef enum jpeg_format_e {
    JPG_FORMAT_UNKNOWN,
    JPG_FORMAT_420,
    JPG_FORMAT_422,
    JPG_FORMAT_422r,
    JPG_FORMAT_444,
    JPG_FORMAT_GRAY
} jpeg_format;


struct my_error_mgr {
     struct jpeg_error_mgr pub;     /* "public" fields */
     jmp_buf  setjmp_buffer;          /* for return to caller */
};

/*
 * private data struct of IDirectFBImageProvider_JPEG
 */
typedef struct {
     int                  ref;      /* reference counter */

     DFBSurfaceDescription surface_desc;

     IDirectFBDataBuffer *buffer;
     struct jpeg_decompress_struct cinfo;
     struct my_error_mgr  jerr;

     jpeg_decoder         decoder_type;
     char                *decoderReason[128];
     jpeg_format           image_format;

     DIRenderCallback     render_callback;
     void                *render_callback_context;
     bool                 alloc_dinamic_input_buffer;
	 void 				 *dynamic_input_buffer;
} IDirectFBImageProvider_JPEG_data;

static DFBResult
IDirectFBImageProvider_JPEG_AddRef  ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_JPEG_Release ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_JPEG_RenderTo( IDirectFBImageProvider *thiz,
                                      IDirectFBSurface       *destination,
                                      const DFBRectangle     *destination_rect );

static DFBResult
IDirectFBImageProvider_JPEG_SetRenderCallback( IDirectFBImageProvider *thiz,
                                               DIRenderCallback        callback,
                                               void                   *context );

static DFBResult
IDirectFBImageProvider_JPEG_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                   DFBSurfaceDescription  *dsc);

static DFBResult
IDirectFBImageProvider_JPEG_GetImageDescription( IDirectFBImageProvider *thiz,
                                                 DFBImageDescription    *dsc );

static bresult
load_encoded_data(void *dst,
                    uint32_t dst_size,
                    uint32_t *length,
                    void *params);

#define JPEG_PROG_BUF_SIZE    0x10000

static void
SetJPEGDecoder(IDirectFBImageProvider_JPEG_data *data)
{
     static bool env_force_libjpeg = false;
     static bool env_force_no_sid = false;
     static bool env_vars_checked = false;
     bool sid_jpeg_cannot_decode = false;
     bool mvd_jpeg_cannot_decode = false;
     uint32_t jpeg_input_length;
     bdvd_graphics_decoder_limitations_t limitations;

     bdvd_graphics_get_image_limitations(&limitations);
     data->buffer->GetLength(data->buffer, &jpeg_input_length);

     if (!env_vars_checked)
     {
          env_vars_checked = true;
          if (bsettop_get_config("force_libjpeg") || bsettop_get_config("force_libjpg") || bsettop_get_config("force_sw_jpg_dec"))
          {
               D_INFO("force_libjpeg is set, using libJPEG for decode...\n");
               env_force_libjpeg = true;
          }

          if (bsettop_get_config("force_no_sid") || bsettop_get_config("no_sid"))
          {
               D_INFO("force_no_sid is set, using libJPEG and mini-titan ONLY...\n");
               env_force_no_sid = true;
          }
     }

     if (data->cinfo.jpeg_color_space == JCS_YCbCr)
     {
          if ((data->cinfo.comp_info[0].h_samp_factor == 1) && (data->cinfo.comp_info[0].v_samp_factor == 2))
               data->image_format =  JPG_FORMAT_422r;
          else if ((data->cinfo.comp_info[0].h_samp_factor == 2) && (data->cinfo.comp_info[0].v_samp_factor == 1))
               data->image_format =  JPG_FORMAT_422;
          else if ((data->cinfo.comp_info[0].h_samp_factor == 2) && (data->cinfo.comp_info[0].v_samp_factor == 2))
               data->image_format =  JPG_FORMAT_420;
          else if ((data->cinfo.comp_info[0].h_samp_factor == 1) && (data->cinfo.comp_info[0].v_samp_factor == 1))
               data->image_format =  JPG_FORMAT_444;
          else
              data->image_format =  JPG_FORMAT_UNKNOWN;
     }
     else if (data->cinfo.jpeg_color_space == JCS_GRAYSCALE)
          data->image_format =  JPG_FORMAT_GRAY;
     else
          data->image_format =  JPG_FORMAT_UNKNOWN;

     {
          bresult limitations_ret;
          bdvd_sid_handle hSid = ((BCMCoreContext *)dfb_system_data())->driver_sid;
          bdvd_graphics_sid_limitations sid_limitations;
          limitations_ret = bdvd_sid_get_limitations(hSid, &sid_limitations);
          DECODER_TEST(sid_jpeg_cannot_decode, limitations_ret != b_ok);
          DECODER_TEST(sid_jpeg_cannot_decode, env_force_no_sid);
          DECODER_TEST(sid_jpeg_cannot_decode, data->image_format == JPG_FORMAT_422r);
          DECODER_TEST(sid_jpeg_cannot_decode, data->image_format == JPG_FORMAT_UNKNOWN);
          DECODER_TEST(sid_jpeg_cannot_decode, data->cinfo.image_width > sid_limitations.max_jpeg_width);
          DECODER_TEST(sid_jpeg_cannot_decode, (data->cinfo.progressive_mode) && ((data->cinfo.image_width * data->cinfo.image_height * 4) > sid_limitations.max_jpeg_progressive_output_bytes));
          data->alloc_dinamic_input_buffer = jpeg_input_length > sid_limitations.max_input_bytes;
     }

     DECODER_TEST(mvd_jpeg_cannot_decode, env_force_libjpeg);
     DECODER_TEST(mvd_jpeg_cannot_decode, limitations.jpeg_decode_disabled);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->image_format == JPG_FORMAT_UNKNOWN);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->cinfo.progressive_mode);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->cinfo.image_width > limitations.max_jpeg_width);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->cinfo.image_height > limitations.max_jpeg_height);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->cinfo.image_width < limitations.min_jpeg_width);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->cinfo.image_height < limitations.min_jpeg_height);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->image_format == JPG_FORMAT_444);
     DECODER_TEST(mvd_jpeg_cannot_decode, !data->cinfo.saw_JFIF_marker);
     DECODER_TEST(mvd_jpeg_cannot_decode, data->cinfo.jpeg_color_space != JCS_YCbCr);


     if (sid_jpeg_cannot_decode && mvd_jpeg_cannot_decode)
          data->decoder_type = JPG_DECODER_LIBJPG;
     else if (sid_jpeg_cannot_decode)
          data->decoder_type = JPG_DECODER_MVD;
     else
          data->decoder_type = JPG_DECODER_SID;
}


typedef struct {
     struct jpeg_source_mgr  pub; /* public fields */

     JOCTET                 *data;       /* start of buffer */

     IDirectFBDataBuffer    *buffer;
} buffer_source_mgr;

typedef buffer_source_mgr * buffer_src_ptr;

static void
buffer_init_source (j_decompress_ptr cinfo)
{
     DFBResult            ret;
     buffer_src_ptr       src    = (buffer_src_ptr) cinfo->src;
     IDirectFBDataBuffer *buffer = src->buffer;

     /* FIXME: support streamed buffers */
     ret = buffer->SeekTo( buffer, 0 );
     if (ret)
          DirectFBError( "(DirectFB/ImageProvider_JPEG) Unable to seek", ret );
}
bool g_error = FALSE;
static boolean
buffer_fill_input_buffer (j_decompress_ptr cinfo)
{
     DFBResult            ret;
     unsigned int         nbytes;
     buffer_src_ptr       src    = (buffer_src_ptr) cinfo->src;
     IDirectFBDataBuffer *buffer = src->buffer;

     ret = buffer->GetData( buffer, JPEG_PROG_BUF_SIZE, src->data, &nbytes );
     g_error = FALSE;
     if (ret || nbytes <= 0) {
#if 0
          if (src->start_of_file)   /* Treat empty input file as fatal error */
               ERREXIT(cinfo, JERR_INPUT_EMPTY);
          WARNMS(cinfo, JWRN_JPEG_EOF);
#endif
          /* Insert a fake EOI marker */
          src->data[0] = (JOCTET) 0xFF;
          src->data[1] = (JOCTET) JPEG_EOI;
          nbytes = 2;
          g_error = TRUE;

     }

     src->pub.next_input_byte = src->data;
     src->pub.bytes_in_buffer = nbytes;

     return TRUE;
}

static void
buffer_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
     buffer_src_ptr src = (buffer_src_ptr) cinfo->src;

     if (num_bytes > 0) {
          while (num_bytes > (long) src->pub.bytes_in_buffer) {
               num_bytes -= (long) src->pub.bytes_in_buffer;
               (void)buffer_fill_input_buffer(cinfo);
          }
          src->pub.next_input_byte += (size_t) num_bytes;
          src->pub.bytes_in_buffer -= (size_t) num_bytes;
     }
}

static void
buffer_term_source (j_decompress_ptr cinfo)
{
}

static void
jpeg_buffer_src (j_decompress_ptr cinfo, IDirectFBDataBuffer *buffer)
{
     buffer_src_ptr src;

     cinfo->src = (struct jpeg_source_mgr *)
                  cinfo->mem->alloc_small ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                           sizeof (buffer_source_mgr));

     src = (buffer_src_ptr) cinfo->src;

     src->data = (JOCTET *)
                  cinfo->mem->alloc_small ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                           JPEG_PROG_BUF_SIZE * sizeof (JOCTET));

     src->buffer = buffer;

     src->pub.init_source       = buffer_init_source;
     src->pub.fill_input_buffer = buffer_fill_input_buffer;
     src->pub.skip_input_data   = buffer_skip_input_data;
     src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
     src->pub.term_source       = buffer_term_source;
     src->pub.bytes_in_buffer   = 0; /* forces fill_input_buffer on first read */
     src->pub.next_input_byte   = NULL; /* until buffer loaded */
}


static void
jpeglib_panic(j_common_ptr cinfo)
{
     struct my_error_mgr *myerr = (struct my_error_mgr*) cinfo->err;
     longjmp(myerr->setjmp_buffer, 1);
}


static void
copy_line32( __u32 *dst, __u8 *src, int width)
{
     __u32 r, g , b;
     while (width--) {
          r = (*src++) << 16;
          g = (*src++) << 8;
          b = (*src++);
          *dst++ = (0xFF000000 |r|g|b);
     }
}

static void
copy_line32_cmyk( __u32 *dst, __u8 *src, int width)
{
     __u32 r, g , b;
     while (width--) {
/*All 4 component images that we're required to support right now are in inverted YCCK format. So, CMYK o/p is actually [1-c, 1-m, 1-y, 1-k]*/	 	
          r = (*src++) << 16;
          g = (*src++) << 8;
          b = (*src++);
		  src++; /*Ignore 1-k and use constant alpha of 0xff instead*/
          *dst++ = (0xFF000000 |r|g|b);
     }
}


static void
copy_line32_uyvy( __u32 *dst, __u8 *src, int width, int pad)
{
     __u32 y1, y2, u , v;
     while (width > 0) {
          u = (*src++) << 8;
          y1 = (*src++);
          y2 = (*src++) << 16;
          v = (*src++) << 24;
          src+=2;
          width-=2;
         *dst++= (__u32)(v|y2|u|y1);
     }

     while (pad > 1) {
         *dst++ = (0x80008000);
      pad-=2;
    }
}


static void
copy_line32_yuy2( __u32 *dst, __u8 *src, int width, int pad)
{
     __u32 y1, y2, u , v;
     while (width > 0) {
          u = (*src++);
          y1 = (*src++) << 8;
          y2 = (*src++) << 24;
          v = (*src++) << 16;
          src+=2;
          width-=2;
         *dst++= (__u32)(v|y2|u|y1);
     }

     while (pad > 1) {
         *dst++ = (0x80008000);
      pad-=2;
    }
}

static void
copy_line24( __u8 *dst, __u8 *src, int width)
{
     while (width--) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[0];

          dst += 3;
          src += 3;
     }
}

static void
copy_line16( __u16 *dst, __u8 *src, int width)
{
     __u32 r, g , b;
     while (width--) {
          r = (*src++ >> 3) << 11;
          g = (*src++ >> 2) << 5;
          b = (*src++ >> 3);
          *dst++ = (r|g|b);
     }
}

static void
copy_line15( __u16 *dst, __u8 *src, int width)
{
     __u32 r, g , b;
     while (width--) {
          r = (*src++ >> 3) << 10;
          g = (*src++ >> 3) << 5;
          b = (*src++ >> 3);
          *dst++ = (0x8000|r|g|b);
     }
}

static void
copy_line8( __u8 *dst, __u8 *src, int width)
{
     __u32 r, g , b;
     while (width--) {
          r = (*src++ >> 5) << 5;
          g = (*src++ >> 5) << 2;
          b = (*src++ >> 6);
          *dst++ = (r|g|b);
     }
}

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx )
{

     /* Look of the Jpeg SOI marker */
     if (ctx->header[0] == 0xff && ctx->header[1] == 0xd8) {
          /* Look for JFIF or Exif strings, also could look at header[3:2] for APP0(0xFFE0),
           * APP1(0xFFE1) or even other APPx markers.
           */
          if (strncmp ((char*) ctx->header + 6, "JFIF", 4) == 0 ||
              strncmp ((char*) ctx->header + 6, "Exif", 4) == 0 ||
              strncmp ((char*) ctx->header + 6, "VVL", 3) == 0 ||
              strncmp ((char*) ctx->header + 6, "WANG", 4) == 0)
               return DFB_OK;

          /* Else look for Quantization table marker or Define Huffman table marker,
           * useful for EXIF thumbnails that have no APPx markers.
           */
          if (ctx->header[2] == 0xff && (ctx->header[3] == 0xdb || ctx->header[3] == 0xc4))
          return DFB_OK;

          if (ctx->filename && strchr (ctx->filename, '.' ) &&
             (strcasecmp ( strchr (ctx->filename, '.' ), ".jpg" ) == 0 ||
              strcasecmp ( strchr (ctx->filename, '.' ), ".jpeg") == 0))
               return DFB_OK;
     }

     return DFB_UNSUPPORTED;
}

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer )
{
     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBImageProvider_JPEG)

     data->ref    = 1;
     data->buffer = buffer;

     buffer->AddRef( buffer );

     thiz->AddRef              = IDirectFBImageProvider_JPEG_AddRef;
     thiz->Release             = IDirectFBImageProvider_JPEG_Release;
     thiz->RenderToUnaltered   = IDirectFBImageProvider_JPEG_RenderTo;
     thiz->SetRenderCallback   = IDirectFBImageProvider_JPEG_SetRenderCallback;
     thiz->GetImageDescription = IDirectFBImageProvider_JPEG_GetImageDescription;
     thiz->GetSurfaceDescriptionUnaltered = IDirectFBImageProvider_JPEG_GetSurfaceDescription;

	 thiz->PipelineImageDecodingAndScaling.imageFormat = (ImageFormat)unknown;
	 thiz->PipelineImageDecodingAndScaling.first_segment = true;
	 thiz->PipelineImageDecodingAndScaling.segment_height = 0;
	 thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn = false;

	 thiz->AnimationDescription.firstFrame = true;

     data->cinfo.err = jpeg_std_error(&data->jerr.pub);
     data->jerr.pub.error_exit = jpeglib_panic;
     jpeg_create_decompress(&data->cinfo);

     snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "best decoder available");
     return DFB_OK;
}

static void
IDirectFBImageProvider_JPEG_Destruct( IDirectFBImageProvider *thiz )
{
     IDirectFBImageProvider_JPEG_data *data =
                              (IDirectFBImageProvider_JPEG_data*)thiz->priv;

     jpeg_destroy_decompress(&data->cinfo);
     data->buffer->Release( data->buffer );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult
IDirectFBImageProvider_JPEG_AddRef( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_JPEG)

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_JPEG_Release( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_JPEG)

     if (--data->ref == 0) {
          IDirectFBImageProvider_JPEG_Destruct( thiz );
     }

     return DFB_OK;
}


static void buffer_read_dword(j_decompress_ptr cinfo, uint32_t *data, uint32_t *numBytesReturned)
/* Read next byte */
{
     volatile struct jpeg_source_mgr * datasrc = cinfo->src;

     if (datasrc->bytes_in_buffer >= 4)
     {
          uint32_t val;
          val = (uint32_t)(*((uint32_t*)(datasrc->next_input_byte)));
          *data = (val << 24) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) | ((val >> 24) & 0xFF);
          *numBytesReturned=4;

          datasrc->next_input_byte+=4;
          datasrc->bytes_in_buffer-=4;
     }
     else
     {
          uint32_t i;
          *data = 0;
          *numBytesReturned = 0;
          for (i = 0; i < 4; i++)
          {
               if (datasrc->bytes_in_buffer == 0)
               {
                    if (! (*cinfo->src->fill_input_buffer) (cinfo))
                    {
                         break;
                    }
               }

               if ((cinfo->src->bytes_in_buffer == 0) || (g_error))
                    break;

               *data |= (GETJOCTET(*(datasrc->next_input_byte)) << (24-(i*8)));
               (*numBytesReturned)++;
               datasrc->next_input_byte++;
               datasrc->bytes_in_buffer--;
          }
     }
}




static DFBResult
IDirectFBImageProvider_JPEG_RenderTo( IDirectFBImageProvider *thiz,
                                      IDirectFBSurface       *destination,
                                      const DFBRectangle     *dest_rect )
{
     int                    err;
     void                  *dst;
     int                    pitch;
     bool                   direct = false;
     int                    yuv_pad = 0;
     DFBRectangle           rect = { 0, 0, 0, 0};
     DFBSurfacePixelFormat  format;
     IDirectFBSurface_data *dst_data;
     CoreSurface           *dst_surface;
     JSAMPARRAY buffer;      /* Output row buffer */
     int row_stride;         /* physical row width in output buffer */
     void *image_data;
     void *row_ptr;
     bdvd_graphics_image_decode_t dec;
     bresult rc = b_ok;
     bdvd_graphics_sid_decode_params dec_params;

     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_JPEG)

     dst_data = (IDirectFBSurface_data*) destination->priv;
     if (!dst_data)
          return DFB_DEAD;

     dst_surface = dst_data->surface;
     if (!dst_surface)
          return DFB_DESTROYED;

     err = destination->GetPixelFormat( destination, &format );
     if (err)
          return err;

     err = destination->GetSize( destination, &rect.w, &rect.h );
     if (err)
          return err;

     if (dest_rect && !dfb_rectangle_intersect( &rect, dest_rect ))
          return DFB_OK;

     err = destination->Lock( destination, DSLF_WRITE, &dst, &pitch );
     if (err)
          return err;

     if (setjmp(data->jerr.setjmp_buffer)) {
          destination->Unlock( destination );
          return DFB_FAILURE;
     }

     switch (format) {
     case DSPF_RGB332:
     case DSPF_ARGB1555:
     case DSPF_RGB16:
     case DSPF_RGB24:
     case DSPF_RGB32:
     case DSPF_ARGB:
         dec.pixel_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;
		/*Support for YCCK images added for ARGB surfaces*/
         if (data->cinfo.jpeg_color_space != JCS_YCCK)
		 {
         	data->cinfo.out_color_space = JCS_RGB;
         	data->cinfo.output_components = 3;
		 }
		 else
		 {		 
		 	data->cinfo.out_color_space = JCS_CMYK;
         	data->cinfo.output_components = 4;
		 }
         break;
     case DSPF_UYVY:
         dec.pixel_format = bdvd_graphics_pixel_format_y18_cr8_y08_cb8;
         data->cinfo.out_color_space = JCS_YCbCr;
         data->cinfo.output_components = 3;
         break;
     case DSPF_YUY2:
         dec.pixel_format = bdvd_graphics_pixel_format_cr8_y18_cb8_y08;     
		 data->cinfo.out_color_space = JCS_YCbCr;
         data->cinfo.output_components = 3;		 		 
         break;
     case DSPF_LUT8:
     default:
         data->cinfo.out_color_space = JCS_RGB;
         data->cinfo.output_components = 3;
         direct = false;
         break;
     }


	 


     if (((rect.w == (int)data->cinfo.image_width && rect.h == (int)data->cinfo.image_height) &&
         (! (dst_surface->caps & DSCAPS_SEPARATED))) || (thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn))
     {
         direct = true;
     }
     else
     {
         snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s: Scaling needed", __FUNCTION__);
         data->decoder_type = JPG_DECODER_LIBJPG;
         data->cinfo.out_color_space = JCS_RGB;
         if ((format == DSPF_UYVY) || (format == DSPF_YUY2))
         {
             dec.pixel_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;
         }
     }

     

     if (data->decoder_type == JPG_DECODER_SID)
     {
          uint32_t pos;
          DFBResult ret;

          data->buffer->GetPosition( data->buffer, &pos );
          pos -= data->cinfo.src->bytes_in_buffer;

          ret = dfb_surface_hardware_lock( dst_surface, dst_surface->back_buffer, DSLF_WRITE );
          if (ret == DFB_OK)
          {
               bdvd_sid_handle hSid = ((BCMCoreContext *)dfb_system_data())->driver_sid;
               bresult bret;

               IDirectFBSurface      *data_surface;

               dec_params.image_format = BGFX_SID_IMAGE_FORMAT_JPEG;

			   destination->GetBackBufferContext(destination, (void**)&dec_params.dst_surface);
     			if (format == DSPF_YUY2 || format == DSPF_UYVY || format == DSPF_I420 ||
         			format == DSPF_YV12 || format == DSPF_NV12 || format == DSPF_NV16 ||
         			format == DSPF_NV21 || format == DSPF_AYUV)
     			{
          			bdvd_graphics_surface_set_color_space(
               			dec_params.dst_surface, bdvd_graphics_color_space_xvycc_601);
     			}	
			   
               dec_params.load_data = load_encoded_data;
               dec_params.load_data_params = data;
               dec_params.src = NULL;
               dec_params.cb_func = NULL;
               dec_params.ext_input_buffer = NULL;

			   dec_params.firstSegment = thiz->PipelineImageDecodingAndScaling.first_segment;


			   
			   
			   dec_params.segment_height = thiz->PipelineImageDecodingAndScaling.segment_height;


			   dec_params.segmentedDecodingOn = thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn;

			   /* Create dinamic input buffer */
			   if (data->alloc_dinamic_input_buffer)
               {
					if ((thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn) && (!thiz->PipelineImageDecodingAndScaling.first_segment))
                	{
                		dec_params.ext_input_buffer = data->dynamic_input_buffer;
               		}
 					else
 					{	
				   void *surface_addr;
				   int surface_pitch;
				   unsigned int jpeg_input_length;
				   DFBSurfaceDescription  data_surface_desc;


				   data->buffer->GetLength(data->buffer, &jpeg_input_length);
				   data_surface_desc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT);
				   data_surface_desc.caps = (DFBSurfaceCapabilities)(DSCAPS_VIDEOONLY);
				   data_surface_desc.pixelformat = DSPF_LUT8;
				   data_surface_desc.width = SURFACE_MAX_WIDTH;
				   data_surface_desc.height = (jpeg_input_length + SURFACE_MAX_WIDTH)/SURFACE_MAX_WIDTH;

#if 0
				   D_INFO("Input size is %d, Used %dx%d surface\n",
				   jpeg_input_length,
				   data_surface_desc.width,
				   data_surface_desc.height);
#endif
				   ret = idirectfb_singleton->CreateSurface(idirectfb_singleton, &data_surface_desc, &data_surface);
				   if (ret != DFB_OK)
				   {
					   dfb_surface_unlock( dst_surface, dst_surface->back_buffer, 0 );
					   return DFB_FAILURE;
				   }
				   data_surface->GetBackBufferContext(data_surface, (void**)&dec_params.ext_input_buffer);
					   data->dynamic_input_buffer = dec_params.ext_input_buffer;
 			   		}
			   }

               bret = bdvd_sid_decode(hSid, &dec_params);

			   /* Release dinamic input buffer */
			
			   if ((data->alloc_dinamic_input_buffer)&&((!thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn) || (thiz->PipelineImageDecodingAndScaling.first_segment)))			
               {
	               data_surface->Release(data_surface);
			}

               dfb_surface_unlock( dst_surface, dst_surface->back_buffer, 0 );
               if (bret != b_ok)
               {
                    D_ERROR("Error in bdvd_sid_decode (%d)! Using libjpg\n", bret);
                    data->buffer->SeekTo( data->buffer, pos );
                    data->cinfo.src->bytes_in_buffer = 0;

                    data->decoder_type = JPG_DECODER_LIBJPG;

					if (thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn)
					{						
                    	return DFB_FAILURE;
					}
               }

               if ((dst_surface->palette != NULL) && (dst_surface->palette->entries != NULL) && (format == DSPF_LUT8) && (data->image_format == JPG_FORMAT_GRAY))
               {
                    uint32_t palette_size = 256;
                    uint32_t i;

                    for(i = 0; i < palette_size; i++)
                    {
                         dst_surface->palette->entries[i].a = i;
                         dst_surface->palette->entries[i].r = i;
                         dst_surface->palette->entries[i].g = i;
                         dst_surface->palette->entries[i].b = 0xff;
                    }
                    dfb_palette_update( dst_surface->palette, 0, palette_size - 1 );
               }
          }
          else
          {
               D_ERROR("Unable to lock the surface for use with the SID! Using libjpg\n");
               snprintf(data->decoderReason, sizeof(data->decoderReason) - 1, "%s: Unable to lock", __FUNCTION__);
               data->decoder_type = JPG_DECODER_LIBJPG;
          }
     }

     if (data->decoder_type == JPG_DECODER_LIBJPG)
     {
          jpeg_start_decompress(&data->cinfo);
          row_stride = data->cinfo.output_width * data->cinfo.output_components;

          buffer = (*data->cinfo.mem->alloc_sarray)((j_common_ptr) &data->cinfo,
                                         JPOOL_IMAGE, row_stride, 1);

          if (direct)
          {
               int y = 0;

               /* image must not be scaled */
               row_ptr = dst;

               dst += rect.x * DFB_BYTES_PER_PIXEL(format) + rect.y * pitch;

               while (data->cinfo.output_scanline < data->cinfo.output_height) {
                    jpeg_read_scanlines(&data->cinfo, buffer, 1);
                    switch (format) {
                         case DSPF_RGB332:
                              copy_line8( (__u8*)row_ptr, *buffer,
                                          data->cinfo.output_width);
                              break;
                         case DSPF_RGB16:
                              copy_line16( (__u16*)row_ptr, *buffer,
                                           data->cinfo.output_width);
                              break;
                         case DSPF_ARGB1555:
                              copy_line15( (__u16*)row_ptr, *buffer,
                                           data->cinfo.output_width);
                              break;
                         case DSPF_RGB24:
                              copy_line24( row_ptr, *buffer,
                                           data->cinfo.output_width);
                              break;
                         case DSPF_ARGB:
                         case DSPF_RGB32:
						 	/*Support for YCCK images added for ARGB surfaces*/	 
         					if (data->cinfo.jpeg_color_space != JCS_YCCK)
                              copy_line32( (__u32*)row_ptr, *buffer,
                                         data->cinfo.output_width);
							else
								copy_line32_cmyk( (__u32*)row_ptr, *buffer,
                                         data->cinfo.output_width);
                              break;
                         case DSPF_UYVY:
                              copy_line32_uyvy( (__u32*)row_ptr, *buffer,
                                          data->cinfo.output_width, yuv_pad);
                              break;
                         case DSPF_YUY2:
                              copy_line32_yuy2( (__u32*)row_ptr, *buffer,
                                          data->cinfo.output_width, yuv_pad);
                              break;
                         default:
                              D_BUG("unsupported format not filtered before");
                              return DFB_BUG;
                    }
                    row_ptr += pitch;
                    y++;

                    if (data->render_callback) {
                         DFBRectangle rect = { 0, y, data->cinfo.output_width, 1 };

                         data->render_callback( &rect, data->render_callback_context );
                    }
               }
          }
          else {     /* image must be scaled */
               int y = 0;

               image_data = malloc(data->cinfo.output_width * data->cinfo.output_height*4);
               row_ptr = image_data;

               while (data->cinfo.output_scanline < data->cinfo.output_height) {
                    jpeg_read_scanlines(&data->cinfo, buffer, 1);
					
					/*Support for YCCK images added for ARGB surfaces*/	 
         			if (data->cinfo.jpeg_color_space != JCS_YCCK)
                    	copy_line32( (__u32*)row_ptr, *buffer, data->cinfo.output_width);
					else
						copy_line32_cmyk( (__u32*)row_ptr, *buffer, data->cinfo.output_width);
					
                    row_ptr += data->cinfo.output_width * 4;

                    y++;

                    if (data->render_callback) {
                         DFBRectangle rect = { 0, y, data->cinfo.output_width, 1 };

                         data->render_callback( &rect, data->render_callback_context );
                    }
               }

               dfb_scale_linear_32( image_data,
                                    data->cinfo.output_width, data->cinfo.output_height,
                                    dst, pitch, &rect, dst_surface );

               free( image_data );
          }

          jpeg_finish_decompress(&data->cinfo);
     }

     err = destination->Unlock( destination );
     if (err)
          return err;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_JPEG_SetRenderCallback( IDirectFBImageProvider *thiz,
                                               DIRenderCallback        callback,
                                               void                   *context )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_JPEG)

     data->render_callback         = callback;
     data->render_callback_context = context;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_JPEG_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                   DFBSurfaceDescription  *dsc )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_JPEG)

     if (data->surface_desc.flags == 0)
     {
          if (setjmp(data->jerr.setjmp_buffer)) {
               return DFB_FAILURE;
          }

          jpeg_buffer_src(&data->cinfo, data->buffer);
          jpeg_read_header(&data->cinfo, TRUE);

          data->surface_desc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
          data->surface_desc.caps  |= DSCAPS_VIDEOONLY;
          data->surface_desc.width = data->cinfo.image_width;
          data->surface_desc.height = data->cinfo.image_height;

          SetJPEGDecoder(data);

		  if ((data->decoder_type == JPG_DECODER_SID)&&(! (getenv("no_segmented_decoding"))))
		  {
			thiz->PipelineImageDecodingAndScaling.imageFormat = (ImageFormat)JPEG;
		  }
		  

          if (data->decoder_type == JPG_DECODER_SID)
          {
               switch (data->image_format)
               {
               case JPG_FORMAT_422r:
                    data->surface_desc.flags |= DSDESC_CAPS;
                    data->surface_desc.caps |= DSCAPS_ALLOC_ALIGNED_WIDTH_8 | DSCAPS_ALLOC_ALIGNED_HEIGHT_16;
                    data->surface_desc.pixelformat = DSPF_UYVY;
                    break;
               case JPG_FORMAT_422:
                    data->surface_desc.flags |= DSDESC_CAPS;
                    data->surface_desc.caps |= DSCAPS_ALLOC_ALIGNED_WIDTH_16 | DSCAPS_ALLOC_ALIGNED_HEIGHT_8;
                    data->surface_desc.pixelformat = DSPF_UYVY;
                    break;
               case JPG_FORMAT_420:
                    data->surface_desc.flags |= DSDESC_CAPS;
                    data->surface_desc.caps |= DSCAPS_ALLOC_ALIGNED_WIDTH_16 | DSCAPS_ALLOC_ALIGNED_HEIGHT_16;
                    data->surface_desc.pixelformat = DSPF_UYVY;
                    break;
               case JPG_FORMAT_444:
                    data->surface_desc.flags |= DSDESC_CAPS;
                    data->surface_desc.caps |= DSCAPS_ALLOC_ALIGNED_WIDTH_8 | DSCAPS_ALLOC_ALIGNED_HEIGHT_8;
                    data->surface_desc.pixelformat = DSPF_AYUV;
                    break;
               case JPG_FORMAT_GRAY:
                    data->surface_desc.flags |= DSDESC_CAPS;
                    data->surface_desc.caps |= DSCAPS_ALLOC_ALIGNED_WIDTH_8 | DSCAPS_ALLOC_ALIGNED_HEIGHT_8;
                    data->surface_desc.pixelformat = DSPF_LUT8;
                    break;
               case JPG_FORMAT_UNKNOWN:
               default:
                    break;
               }
          }
          else
          {
               switch (data->cinfo.jpeg_color_space) {
               case JCS_YCCK:      /* Y/Cb/Cr/K */
			   	/*Support for YCCK images added for ARGB surfaces*/
			   	data->surface_desc.pixelformat = DSPF_ARGB;
				break;               
               case JCS_YCbCr:     /* Y/Cb/Cr (also known as YUV) */
                    data->surface_desc.pixelformat = DSPF_YUY2;
               break;
               case JCS_CMYK:      /* C/M/Y/K */
               /* Probably unsupported if cannot have RGB out_color_space */
               case JCS_GRAYSCALE: /* monochrome */
               case JCS_RGB:       /* red/green/blue */
               default:
                    data->surface_desc.pixelformat = DSPF_ARGB;
               break;
               }

               /* If the output will be in YUV format, we need to align the surface */
               if (data->surface_desc.pixelformat == DSPF_UYVY)
               {
                    data->surface_desc.flags |= DSDESC_CAPS;
                    data->surface_desc.caps |= DSCAPS_ALLOC_ALIGNED_WIDTH_4;
               }

          }
     }

     *dsc = data->surface_desc;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_JPEG_GetImageDescription( IDirectFBImageProvider *thiz,
                                                 DFBImageDescription    *dsc )
{
     char *decoderName;
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_JPEG)

     if (!dsc)
          return DFB_INVARG;

     dsc->caps = DICAPS_NONE;

     switch (data->decoder_type){
     case JPG_DECODER_LIBJPG:
          decoderName = JPG_DECODER_LIBJPG_NAME;
          break;
     case JPG_DECODER_MVD:
          decoderName = JPG_DECODER_MVD_NAME;
          break;
     case JPG_DECODER_SID:
          decoderName = JPG_DECODER_SID_NAME;
          break;
     default:
          decoderName = JPG_DECODER_UNKNOWN_NAME;
          break;
     }
     snprintf(dsc->decoderName, sizeof(dsc->decoderName) - 1, "%s", decoderName);
     snprintf(dsc->decoderReason, sizeof(dsc->decoderReason) - 1, "%s", data->decoderReason);

     return DFB_OK;
}


static bresult load_encoded_data(void *dst, uint32_t dst_size, uint32_t *length, void *params)
{
     bresult bret = b_ok;
     DFBResult ret;
     uint32_t file_length;
     IDirectFBImageProvider_JPEG_data *data = (IDirectFBImageProvider_JPEG_data *)params;

     ret = data->buffer->SeekTo( data->buffer, 0 );
     if (ret != DFB_OK)
     {
          D_ERROR("Error in load_encoded_data: data->buffer->SeekTo() Failed!\n");
          bret = berr_external_error;
     }

     data->cinfo.src->bytes_in_buffer = 0;

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
