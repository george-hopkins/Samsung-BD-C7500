
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

#define CIF_FILE_ID "HDDVDCIF"

#define ENC_TY_CVF 0x1
#define ENC_TY_CDF 0x2

typedef struct {
	char file_id[8];         // must be 'HDDVDCIF'
	unsigned char vern_reserved;
	unsigned char vern_bookpart; //must be 0x10
	unsigned char enc_ty;    // must be 1 for CVF and 2 for CDF
	unsigned short width;    // max:1920
	unsigned short height;   // max:1080
	unsigned char reserved[49];
} cif_header_t;

#pragma pack (pop)

#define SWAP_SHORT(w) w=((w<<8) | (w>>8))

static void swap_cif_header(cif_header_t* header)
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

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBImageProvider, CIF )

/*
 * private data struct of IDirectFBImageProvider_CIF
 */
typedef struct {
     int                  ref;      /* reference counter */

     IDirectFBDataBuffer *buffer;

} IDirectFBImageProvider_CIF_data;

static DFBResult
IDirectFBImageProvider_CIF_AddRef  ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_CIF_Release ( IDirectFBImageProvider *thiz );

static DFBResult
IDirectFBImageProvider_CIF_RenderTo( IDirectFBImageProvider *thiz,
                                     IDirectFBSurface       *destination,
                                     const DFBRectangle     *destination_rect );

static DFBResult
IDirectFBImageProvider_CIF_SetRenderCallback( IDirectFBImageProvider *thiz,
                                              DIRenderCallback        callback,
                                              void                   *context );

static DFBResult
IDirectFBImageProvider_CIF_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                  DFBSurfaceDescription  *dsc );

static DFBResult
IDirectFBImageProvider_CIF_GetImageDescription( IDirectFBImageProvider *thiz,
                                                DFBImageDescription    *dsc );

static DFBResult
Probe( IDirectFBImageProvider_ProbeContext *ctx )
{
	cif_header_t* header=(cif_header_t*)ctx->header;

    if (memcmp (header->file_id, CIF_FILE_ID, sizeof(header->file_id)) != 0)
    	return DFB_UNSUPPORTED;

 	if(header->vern_bookpart != 0x10)
 		return DFB_UNSUPPORTED;

 	if(header->enc_ty != ENC_TY_CVF && header->enc_ty != ENC_TY_CDF)
 		return DFB_UNSUPPORTED;

 	//passed all checks: this is a CIF file!
    return DFB_OK;
}

static DFBResult
Construct( IDirectFBImageProvider *thiz,
           IDirectFBDataBuffer    *buffer )
{
     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBImageProvider_CIF)

     data->ref    = 1;
     data->buffer = buffer;

     buffer->AddRef( buffer );

     thiz->AddRef              = IDirectFBImageProvider_CIF_AddRef;
     thiz->Release             = IDirectFBImageProvider_CIF_Release;
     thiz->RenderToUnaltered   = IDirectFBImageProvider_CIF_RenderTo;
     thiz->SetRenderCallback   = IDirectFBImageProvider_CIF_SetRenderCallback;
     thiz->GetImageDescription = IDirectFBImageProvider_CIF_GetImageDescription;
     thiz->GetSurfaceDescriptionUnaltered =
                                 IDirectFBImageProvider_CIF_GetSurfaceDescription;

     return DFB_OK;
}

static void
IDirectFBImageProvider_CIF_Destruct( IDirectFBImageProvider *thiz )
{
     IDirectFBImageProvider_CIF_data *data =
                                   (IDirectFBImageProvider_CIF_data*)thiz->priv;

     data->buffer->Release( data->buffer );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}


static DFBResult
IDirectFBImageProvider_CIF_AddRef( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_CIF)

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_CIF_Release( IDirectFBImageProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_CIF)

     if (--data->ref == 0) {
          IDirectFBImageProvider_CIF_Destruct( thiz );
     }

     return DFB_OK;
}

static DFBResult
DecodeCDF(void* surfdata, int pitch, int w, int h, IDirectFBDataBuffer* buffer)
{
	DFBResult result=DFB_OK;
	unsigned int bytes_read;
	int y;

	//simply dump the file, line by line
	for(y=0;y<h;y++)
	{
		result=buffer->GetData(buffer,w*sizeof(unsigned long),surfdata,&bytes_read);
		if(result!=DFB_OK)
		{
			D_ERROR("failed (%s) reading data at line %d\n",DirectFBErrorString(result),y);
			break;
		}

		surfdata=(unsigned long*)((unsigned char*)surfdata+pitch);
	}

	return result;
}

static DFBResult
DecodeCVF(void* surfdata, int pitch, int w, int h, IDirectFBDataBuffer* buffer)
{
	DFBResult result=DFB_OK;
	int l,p;
	unsigned char* src=NULL;
	unsigned int bytes_read;

	//allocate temporary buffer, enough for one full line of Y.
	src=malloc(w);
	if(src==NULL)
		result=DFB_NOSYSTEMMEMORY;

    /* Pixel position is YCrYCb where Cb is LSB */

	//Y PLANE
	if(result==DFB_OK)
	{
 		for(l=0;l<h;l++)
 		{
 			unsigned long* dst_line=(unsigned long*)((char*)surfdata + pitch*l);
 			unsigned char* src_line=src;

 			//read source line
 			result=buffer->GetData(buffer,w,src_line,&bytes_read);
 			if(result!=DFB_OK) break;

 			//write dest line
 			for(p=0;p<w/2;p++)
 			{
 				*dst_line++ = (*src_line)<<8 | (*(src_line+1))<<24;
 				src_line+=2;
 			}
 		}
	}

	//Cr PLANE
	if(result==DFB_OK)
	{
 		for(l=0;l<h/2;l++)
 		{
 			unsigned long* first_line=(unsigned long*)((char*)surfdata + pitch*l*2);
 			unsigned long* second_line=(unsigned long*)((char*)surfdata + pitch*(l*2+1));

 			unsigned char* src_line=src;

 			//read source line
 			result=buffer->GetData(buffer,w/2,src_line,&bytes_read);
 			if(result!=DFB_OK) break;

 			//write dest line
 			for(p=0;p<w/2;p++)
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
		for(l=0;l<h/2;l++)
 		{
 			unsigned long* first_line=(unsigned long*)((char*)surfdata + pitch*l*2);
 			unsigned long* second_line=(unsigned long*)((char*)surfdata + pitch*(l*2+1));

 			unsigned char* src_line=src;

 			//read source line
 			result=buffer->GetData(buffer,w/2,src_line,&bytes_read);
 			if(result!=DFB_OK) break;

 			//write dest line
 			for(p=0;p<w/2;p++)
 			{
 				register int cb=*src_line++;
 				*first_line++  |= cb;
 				*second_line++ |= cb;
 			}
 		}
	}

	//free temporary buffer
	if(src)
		free(src);

	return result;
}


static DFBResult
IDirectFBImageProvider_CIF_RenderTo( IDirectFBImageProvider *thiz,
                                     IDirectFBSurface       *destination,
                                     const DFBRectangle     *dest_rect )
{
	DFBResult              result = DFB_OK;
	DFBRectangle           rect = { 0, 0, 0, 0 };
	DFBSurfacePixelFormat  format;
	cif_header_t           header;
	unsigned int           bytes_read;
	void*                  surfdata = NULL;
	int                    pitch = 0;

	D_DEBUG("IDirectFBImageProvider_CIF_RenderTo\n");

	DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_CIF)

	if(dest_rect==NULL) {
	    result = destination->GetSize( destination, &rect.w, &rect.h );
	    if(result!=DFB_OK) return result;

	    dest_rect=&rect;
	}

	result = destination->GetPixelFormat( destination, &format );
	if(result!=DFB_OK) return result;

	//read the header
	result=data->buffer->GetData(data->buffer,sizeof(header),&header,&bytes_read);
	if(result==DFB_OK)
 		if(bytes_read!=sizeof(header))
 			result=DFB_IO;

	swap_cif_header(&header);

	if((header.enc_ty == ENC_TY_CVF && format != DSPF_UYVY) || (header.enc_ty == ENC_TY_CDF && format != DSPF_ARGB))
	{
	    D_ERROR("destination surface has the wrong pixel format\n");
	    return DFB_INVARG;
	}

 	if(header.width != rect.w || header.height != rect.h)
	{
     	    D_ERROR("image file does not have the same size as the destination surface\n");
     	    return DFB_INVARG;
	}

	//obtain a pointer to the surface data
 	if(result==DFB_OK)
 		result=destination->Lock(destination,DSLF_WRITE,&surfdata,&pitch);

 	//jump to the beginning of the dest_rect
 	if(result==DFB_OK)
	 	surfdata=(unsigned char*)surfdata + dest_rect->y*pitch + dest_rect->x*sizeof(unsigned long);

	if(result==DFB_OK) {
    	switch(header.enc_ty)
    	{
    	case ENC_TY_CVF:
    		result=DecodeCVF(surfdata,pitch,header.width,header.height,data->buffer);
    		break;
    	case ENC_TY_CDF:
    		result=DecodeCDF(surfdata,pitch,header.width,header.height,data->buffer);
    		break;
    	default:
    		D_ERROR("invalid enc_ty");
    		result=DFB_INVARG;
    		break;
    	}
	}

	destination->Unlock(destination);

	return result;
}

static DFBResult
IDirectFBImageProvider_CIF_SetRenderCallback( IDirectFBImageProvider *thiz,
                                              DIRenderCallback        callback,
                                              void                   *context )
{
     return DFB_UNIMPLEMENTED;
}

static DFBResult
IDirectFBImageProvider_CIF_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                                  DFBSurfaceDescription  *dsc )
{
	DFBResult result=DFB_OK;
	cif_header_t header;
	unsigned int bytes_read=0;

	DIRECT_INTERFACE_GET_DATA (IDirectFBImageProvider_CIF)

	result=data->buffer->PeekData(data->buffer,sizeof(header),0,&header,&bytes_read);
	swap_cif_header(&header);

	if(result==DFB_OK)
	{
		if(bytes_read != sizeof(header))
			result=DFB_FAILURE;
	}

	if(result==DFB_OK)
	{
		dsc->flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
		dsc->width       = header.width;
		dsc->height      = header.height;

		if(header.enc_ty == ENC_TY_CVF)
			dsc->pixelformat = DSPF_UYVY; //actually, the file is in DSPF_YV12 but we don't support that format...
		else if(header.enc_ty == ENC_TY_CDF)
			dsc->pixelformat = DSPF_ARGB;
		else
			result=DFB_UNSUPPORTED;

	}

	return result;
}

static DFBResult
IDirectFBImageProvider_CIF_GetImageDescription( IDirectFBImageProvider *thiz,
                                                DFBImageDescription    *dsc )
{
     dsc->caps = DICAPS_NONE;

     snprintf(dsc->decoderName, sizeof(dsc->decoderName) - 1, "software CIF");
     snprintf(dsc->decoderReason, sizeof(dsc->decoderReason) - 1, "best decoder available");

     return DFB_OK;
}

