
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#include <directfb.h>

#include <display/idirectfbsurface.h>

#include <media/idirectfbimageprovider.h>

#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/layers.h>
#include <core/surfaces.h>

#include <misc/gfx_util.h>
#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <misc/util.h>


#pragma pack (push,1)

#define CVF_FILE_ID "HDDVDCIF"

typedef struct {
	char file_id[8];         // must be 'HDDVDCIF'
	unsigned char vern_reserved;
	unsigned char vern_bookpart;
	unsigned char enc_ty;    // must be 1
	unsigned short width;    // max:1920
	unsigned short height;   // max:1080
	unsigned char reserved[49];
} cvf_header_t;

#pragma pack (pop)

#define SWAP_SHORT(w) w=((w<<8) | (w>>8))

static void swap_cvf_header(cvf_header_t* header)
{
	SWAP_SHORT(header->width);
	SWAP_SHORT(header->height);
}

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer );


#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBImageProvider, CVF )

/*
 * private data struct of IDirectFBImageProvider_CVF
 */
typedef struct {
     int                  ref;      /* reference counter */

     DFBSurfaceDescription surface_desc;

     IDirectFBDataBuffer *buffer;

} IDirectFBImageProvider_CVF_data;

static DFBResult
IDirectFBImageProvider_CVF_AddRef  ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_CVF_Release ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_CVF_RenderTo( IDirectFBImageProvider *thiz,
                                     IDirectFBSurface       *destination,
                                     const DFBRectangle     *destination_rect );

static DFBResult
IDirectFBImageProvider_CVF_SetRenderCallback( IDirectFBImageProvider *thiz,
                                              DIRenderCallback        callback,
                                              void                   *context );

static DFBResult
IDirectFBImageProvider_CVF_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                  DFBSurfaceDescription  *dsc );

static DFBResult
IDirectFBImageProvider_CVF_GetImageDescription( IDirectFBImageProvider *thiz,
                                                DFBImageDescription    *dsc );

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx )
{
     if (strncmp (ctx->header, CVF_FILE_ID, strlen(CVF_FILE_ID)) == 0)
          return DFB_OK;

     return DFB_UNSUPPORTED;
}

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer )
{
     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBImageProvider_CVF)

     data->ref    = 1;
     data->buffer = buffer;

     buffer->AddRef( buffer );

     thiz->AddRef              = IDirectFBImageProvider_CVF_AddRef;
     thiz->Release             = IDirectFBImageProvider_CVF_Release;
     thiz->RenderToUnaltered   = IDirectFBImageProvider_CVF_RenderTo;
     thiz->SetRenderCallback   = IDirectFBImageProvider_CVF_SetRenderCallback;
     thiz->GetImageDescription = IDirectFBImageProvider_CVF_GetImageDescription;
     thiz->GetSurfaceDescriptionUnaltered =
                               IDirectFBImageProvider_CVF_GetSurfaceDescription;

     return DFB_OK;
}

static void
IDirectFBImageProvider_CVF_Destruct( IDirectFBImageProvider *thiz )
{
     IDirectFBImageProvider_CVF_data *data =
                                   (IDirectFBImageProvider_CVF_data*)thiz->priv;

     data->buffer->Release( data->buffer );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}


static DFBResult
IDirectFBImageProvider_CVF_AddRef( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_CVF)

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_CVF_Release( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_CVF)

     if (--data->ref == 0) {
          IDirectFBImageProvider_CVF_Destruct( thiz );
     }

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_CVF_RenderTo( IDirectFBImageProvider *thiz,
                                     IDirectFBSurface       *destination,
                                     const DFBRectangle     *dest_rect )
{
	 DFBResult              result = DFB_OK;
     DFBRectangle           rect = { 0, 0, 0, 0 };
     DFBSurfacePixelFormat  format;
     cvf_header_t           header;
	 unsigned int           bytes_read;
     void*                  surfdata = NULL;
 	 int                    pitch = 0;

     DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_CVF)

	 result = destination->GetSize( destination, &rect.w, &rect.h );
	 if(result!=DFB_OK) return result;

     result = destination->GetPixelFormat( destination, &format );
     if(result!=DFB_OK) return result;

     if(format != DSPF_UYVY)
     {
     	D_ERROR("destination surface has the wrong pixel format\n");
     	return DFB_UNSUPPORTED;
     }

	//read the header
	result=data->buffer->GetData(data->buffer,sizeof(header),&header,&bytes_read);
	if(result==DFB_OK)
 		if(bytes_read!=sizeof(header))
 			result=DFB_IO;

	swap_cvf_header(&header);

 	//obtain a pointer to the surface data
 	if(result==DFB_OK)
 		result=destination->Lock(destination,DSLF_WRITE,&surfdata,&pitch);

 	if(result==DFB_OK)
	{
		int l,p;
 		unsigned char* src=NULL;

 		//allocate temporary buffer, enough for one full line of Y.
 		src=malloc(header.width);
 		if(src==NULL)
 			result=DFB_NOSYSTEMMEMORY;

        /* Pixel position is YCrYCb where Cb is LSB */

 		//Y PLANE
 		if(result==DFB_OK)
 		{
	 		for(l=0;l<header.height;l++)
	 		{
	 			unsigned long* dst_line=(char*)surfdata + pitch*l;
	 			unsigned char* src_line=src;

	 			//read source line
	 			result=data->buffer->GetData(data->buffer,header.width,src_line,&bytes_read);
	 			if(result!=DFB_OK) break;

	 			//write dest line
	 			for(p=0;p<header.width/2;p++)
	 			{
	 				*dst_line++ = (*src_line)<<8 | (*(src_line+1))<<24;
	 				src_line+=2;
	 			}
	 		}
 		}

 		//Cr PLANE
 		if(result==DFB_OK)
 		{
	 		for(l=0;l<header.height/2;l++)
	 		{
	 			unsigned long* first_line=(char*)surfdata + pitch*l*2;
	 			unsigned long* second_line=(char*)surfdata + pitch*(l*2+1);

	 			unsigned char* src_line=src;

	 			//read source line
	 			result=data->buffer->GetData(data->buffer,header.width/2,src_line,&bytes_read);
	 			if(result!=DFB_OK) break;

	 			//write dest line
	 			for(p=0;p<header.width/2;p++)
	 			{
	 				register int cr=(*src_line++)<<16;
	 				*first_line++  |= cr;
	 				*second_line++ |= cr;
	 			}
	 		}
 		}

 		//Cb PLANE
 		if(result==DFB_OK)
 		{
 			for(l=0;l<header.height/2;l++)
	 		{
	 			unsigned long* first_line=(char*)surfdata + pitch*l*2;
	 			unsigned long* second_line=(char*)surfdata + pitch*(l*2+1);

	 			unsigned char* src_line=src;

	 			//read source line
	 			result=data->buffer->GetData(data->buffer,header.width/2,src_line,&bytes_read);
	 			if(result!=DFB_OK) break;

	 			//write dest line
	 			for(p=0;p<header.width/2;p++)
	 			{
	 				register int cb=*src_line++;
	 				*first_line++  |= cb;
	 				*second_line++ |= cb;
	 			}
	 		}
 		}

 		destination->Unlock(destination);

		//free temporary buffer
		if(src)
			free(src);
 	}

	return result;
}

static DFBResult
IDirectFBImageProvider_CVF_SetRenderCallback( IDirectFBImageProvider *thiz,
                                              DIRenderCallback        callback,
                                              void                   *context )
{
     return DFB_UNIMPLEMENTED;
}

static DFBResult
IDirectFBImageProvider_CVF_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                  DFBSurfaceDescription  *dsc )
{
    DFBResult result=DFB_OK;
	cvf_header_t header;
	unsigned int bytes_read=0;

	DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_CVF)

    if (data->surface_desc.flags == 0) {
        if ((result = data->buffer->PeekData(data->buffer,sizeof(header),0,&header,&bytes_read)) != DFB_OK) {
            goto out;
        }

    	swap_cvf_header(&header);
        if(bytes_read != sizeof(header)) {
            result=DFB_FAILURE;
            goto out;
        }

		data->surface_desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
		data->surface_desc.width       = header.width;
		data->surface_desc.height      = header.height;
		data->surface_desc.pixelformat = DSPF_UYVY; //actually, the file is in DSPF_YV12 but we don't support that format...
    }

    *dsc = data->surface_desc;

out:

    if (result != DFB_OK) data->surface_desc.flags = 0;

	return result;
}

static DFBResult
IDirectFBImageProvider_CVF_GetImageDescription( IDirectFBImageProvider *thiz,
                                                DFBImageDescription    *dsc )
{
     //DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_CVF)

     dsc->caps = DICAPS_NONE;
     snprintf(dsc->decoderName, sizeof(dsc->decoderName) - 1, "software CVF");
     snprintf(dsc->decoderReason, sizeof(dsc->decoderReason) - 1, "best decoder available");

     return DFB_OK;
}
