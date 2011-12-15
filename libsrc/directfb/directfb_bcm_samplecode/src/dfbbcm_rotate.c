/***************************************************************************
 *     Copyright (c) 2006, Broadcom Corporation
 *     All Rights Reserved;
 *     Confidential Property of Broadcom Corporation
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************/

#include <stdio.h>
#include <directfb.h>
#include <bdvd.h>
#include "dfbbcm_utils.h"

#define PRINT(x...) fprintf(stderr,x)

/**
 * Creates a new surface and set its content to an image, rotated by the 
 * specified number of degrees (which can be 90, 180, or 270 only).
 */
DFBResult RotateSurface(IDirectFB* dfb, IDirectFBSurface* surface, int degrees, IDirectFBSurface** rotated)
{
	DFBResult result=DFB_OK;
	DFBSurfaceDescription sdesc;
	int w,h;
	DFBSurfacePixelFormat format;
	void* sdata=NULL;
	int spitch=0;
	void* ddata=NULL;
	int dpitch=0;
	
	//we support only 90, 180 and 270
	if(degrees!=90 && degrees!=180 && degrees!=270)
		return DFB_INVARG;
	
	surface->GetPixelFormat(surface,&format);
	
	//we support only 32bits formats (but it could be extended easily)
	if(format!=DSPF_ARGB && format!=DSPF_RGB32)
		return DFB_UNSUPPORTED;
	
	//prepare the creation of the new surface
	surface->GetSize(surface,&w,&h);
	sdesc.flags=(DFBSurfaceDescriptionFlags)(DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT|DSDESC_CAPS);
	sdesc.pixelformat=format;
	sdesc.width=w;
	sdesc.height=h;
    sdesc.caps=DSCAPS_VIDEOONLY;
	
	if(degrees==90 || degrees==270)
	{
		//swap width and height
		sdesc.width=h;
		sdesc.height=w;
	}

	//create the new surface
	if(result==DFB_OK)
		result=dfb->CreateSurface(dfb,&sdesc,rotated);

	//lock source pixel data
	if(result==DFB_OK)
		result=surface->Lock(surface,DSLF_READ,&sdata,&spitch);

	//lock dest pixel data
	if(result==DFB_OK)
		result=(*rotated)->Lock(*rotated,DSLF_WRITE,&ddata,&dpitch);

	if(result==DFB_OK)
	{
		if(degrees==180)
		{
			int x,y;
			unsigned long * sline=(unsigned long *)((unsigned char *)sdata + spitch);
			unsigned long * dline=(unsigned long *)((unsigned char *)ddata + (h-1)*dpitch) + w-1;
			unsigned long * spixel;
			unsigned long * dpixel;
			
			for(y=0;y<h;y++)
			{
				spixel=sline;
				dpixel=dline;
				
				//copy one line in reverse order
				for(x=0;x<w;x++)
					*dpixel--=*spixel++;
				
				sline=(unsigned long *)((unsigned char *)sline + spitch);
				dline=(unsigned long *)((unsigned char *)dline - dpitch);
			}
		}
		else if(degrees==90)
		{
			int x,y;
			unsigned long * sline=(unsigned long *)((unsigned char *)sdata + spitch);
			unsigned long * dcol=(unsigned long *)((unsigned char *)ddata) + h-1;
			unsigned long * spixel;
			unsigned long * dpixel;

			for(y=0;y<h;y++)
			{
				spixel=sline;
				dpixel=dcol;
				
				for(x=0;x<w;x++)
				{
					*dpixel=*spixel;
					spixel++;
					dpixel=(unsigned long*)((unsigned char*)dpixel + dpitch);
				}
				
				sline=(unsigned long *)((unsigned char *)sline + spitch);
				dcol--;
			}
		}
		else if(degrees==270)
		{
			int x,y;
			unsigned long * sline=(unsigned long *)((unsigned char *)sdata + spitch);
			unsigned long * dcol=(unsigned long *)((unsigned char *)ddata + (w-1)*dpitch);
			unsigned long * spixel;
			unsigned long * dpixel;

			for(y=0;y<h;y++)
			{
				spixel=sline;
				dpixel=dcol;
				
				for(x=0;x<w;x++)
				{
					*dpixel=*spixel;
					spixel++;
					dpixel=(unsigned long*)((unsigned char*)dpixel - dpitch);
				}
				
				sline=(unsigned long *)((unsigned char *)sline + spitch);
				dcol++;
			}
		}

		//unlock both surfaces
        (*rotated)->Unlock(*rotated);
		surface->Unlock(surface);
	}

	return result;
}

//=============================================================================

static DFBResult ShowSurface(IDirectFBSurface* dest, IDirectFBSurface* source)
{
	int dw,dh;
	int sw,sh;
	dest->GetSize(dest,&dw,&dh);
	source->GetSize(source,&sw,&sh);
	return dest->Blit(dest,source,NULL,dw/2-sw/2,dh/2-sh/2);
}


static DFBResult AdjustSurfaceDescription(DFBSurfaceDescription * sdesc) {
    static const int SURFACE_MAX_WIDTH = 3008;
    static const int SURFACE_MAX_HEIGHT = 2000;
    
    if (!sdesc) {
        return DFB_FAILURE;
    }
    
    /* Force pixelformat to ARGB for rotation feature */
    sdesc->flags|=(DFBSurfaceDescriptionFlags)(DSDESC_PIXELFORMAT);
    sdesc->pixelformat=DSPF_ARGB;
    /* Always force surfaces in video memory to avoid cost of the initial
     * swapping from system to video or the use of the software rasterizer.
     * Also there is more video memory than system memory.
     */
    if (sdesc->flags & DSDESC_CAPS) {
        sdesc->caps |= (DFBSurfaceCapabilities)(DSCAPS_VIDEOONLY);
    }
    else {
        sdesc->flags |= (DFBSurfaceDescriptionFlags)(DSDESC_CAPS);
        sdesc->caps = DSCAPS_VIDEOONLY;
    }
    
    /* Let's force software downscale, maximum will now be 3008 x 2000,
     * because more than 6MP is too expensive in terms of video memory.
     */
    if (sdesc->flags & DSDESC_WIDTH) {
        sdesc->height = sdesc->width > SURFACE_MAX_WIDTH ? (sdesc->height * (double)SURFACE_MAX_WIDTH/(double)sdesc->width) : sdesc->height;
        sdesc->width = sdesc->width > SURFACE_MAX_WIDTH ? SURFACE_MAX_WIDTH : sdesc->width;
    }
    if (sdesc->flags & DSDESC_HEIGHT) {
        sdesc->width = sdesc->height > SURFACE_MAX_HEIGHT ? (sdesc->width * (double)SURFACE_MAX_HEIGHT/(double)sdesc->height) : sdesc->width;
        sdesc->height = sdesc->height > SURFACE_MAX_HEIGHT ? SURFACE_MAX_HEIGHT : sdesc->height;
    }
    
    return DFB_OK;
}

int main(int argc, char* argv[])
{
	DFBResult result=DFB_OK;
	IDirectFB* dfb=NULL;
	IDirectFBSurface* fb=NULL;
	IDirectFBSurface* imagesurface=NULL;
	IDirectFBSurface* rotated=NULL;
	IDirectFBImageProvider* provider=NULL;
	IDirectFBDisplayLayer* layer=NULL;
	bdvd_display_h display;
	bdvd_dfb_ext_h hLayer;
	bdvd_dfb_ext_layer_settings_t lsettings;
	DFBSurfaceDescription sdesc;
    bdvd_video_format_e video_format = bdvd_video_format_ntsc;
	
	if(argc<2)
	{
		fprintf(stderr,"USAGE: dfbbcm_rotate PICTUREFILE [DFBFLAGS]\n\n");
		return 1;
	}
	
	bdvd_init(BDVD_VERSION);
	display=bdvd_display_open(B_ID(0));

    if ((result=display_set_video_format(display, video_format)) != DFB_OK) {
        D_ERROR("display_set_video_format failed\n");
        result=DFB_FAILURE;
    }
    
    if(result==DFB_OK)
        dfb=bdvd_dfb_ext_open(2);
    
    if(result==DFB_OK) {
        switch (video_format) {
            case bdvd_video_format_ntsc:
                /* Setting output format */
               result=dfb->SetVideoMode( dfb, 720, 480, 32 );
            break;
            case bdvd_video_format_1080i:
                /* Setting output format */
               result=dfb->SetVideoMode( dfb, 1920, 1080, 32 );
            break;
            default:
            break;
        }
    }
	
	if(result==DFB_OK)
	{	
		PRINT("opening layer\n");
		hLayer=bdvd_dfb_ext_layer_open(
			bdvd_dfb_ext_osd_layer);
		
		if(hLayer)
			bdvd_dfb_ext_layer_get(hLayer,&lsettings);
	}
	
	if(result==DFB_OK)
		result=dfb->GetDisplayLayer(dfb,lsettings.id,&layer);
	if(result==DFB_OK)
		result=layer->SetCooperativeLevel(layer,DLSCL_ADMINISTRATIVE);
	if(result==DFB_OK)
	{
		DFBDisplayLayerConfig conf;
		conf.flags=DLCONF_WIDTH|DLCONF_HEIGHT;
		conf.width=1920;
		conf.height=1080;
		
		layer->SetConfiguration(layer,&conf);
	}
	if(result==DFB_OK)
		result=layer->SetLevel(layer,1);
	
	if(result==DFB_OK)
	{
		PRINT("layersurface\n");
		result=layer->GetSurface(layer,&fb);
	}
	
	if(result==DFB_OK)
	{
		PRINT("image provider\n");
		result=dfb->CreateImageProvider(dfb,argv[1],&provider);
	}

    /* IMAGE DECODE PROCEDURE */
	if(result==DFB_OK) {
        PRINT("GetSurfaceDescription\n");
		result=provider->GetSurfaceDescription(provider,&sdesc);
    }
    
    if(result==DFB_OK) {
        PRINT("AdjustSurfaceDescription\n");
        result=AdjustSurfaceDescription(&sdesc);
    }
    
	if(result==DFB_OK) {
        PRINT("CreateSurface\n");
		result=dfb->CreateSurface(dfb,&sdesc,&imagesurface);
    }
    /* END OF IMAGE DECODE PROCEDURE */
    
	if(result==DFB_OK)
	{
		PRINT("renderto\n");
		result=provider->RenderTo(provider,imagesurface,NULL);
	}
	if(provider)
		provider->Release(provider);
	
	if(result==DFB_OK)
		result=fb->Clear(fb,0,0,0,0);
	
	if(result==DFB_OK)
		result=ShowSurface(fb,imagesurface);

	if(result==DFB_OK)
	{
		PRINT("flipping\n");
		result=fb->Flip(fb,NULL,DSFLIP_BLIT);
	}	
	if(result==DFB_OK)
		getchar();
	
	//====== rotate 90
	if(result==DFB_OK)
		result=RotateSurface(dfb,imagesurface,90,&rotated);
	if(result==DFB_OK)
		result=fb->Clear(fb,0,0,0,0);
	if(result==DFB_OK)
		result=ShowSurface(fb,rotated);
	if(rotated)
		rotated->Release(rotated);
		
	if(result==DFB_OK)
		result=fb->Flip(fb,NULL,DSFLIP_BLIT);
	
	if(result==DFB_OK)
		getchar();
	
	//====== rotate 180
	if(result==DFB_OK)
		result=RotateSurface(dfb,imagesurface,180,&rotated);
	if(result==DFB_OK)
		result=fb->Clear(fb,0,0,0,0);
	if(result==DFB_OK)
		result=ShowSurface(fb,rotated);
	if(rotated)
		rotated->Release(rotated);
	
	if(result==DFB_OK)
		result=fb->Flip(fb,NULL,DSFLIP_BLIT);
		
	if(result==DFB_OK)
		getchar();
	
	//====== rotate 270
	if(result==DFB_OK)
		result=RotateSurface(dfb,imagesurface,270,&rotated);
	if(result==DFB_OK)
		result=fb->Clear(fb,0,0,0,0);
	if(result==DFB_OK)
		result=ShowSurface(fb,rotated);
	if(rotated)
		rotated->Release(rotated);
	
	if(result==DFB_OK)
		result=fb->Flip(fb,NULL,DSFLIP_BLIT);
		
	if(result==DFB_OK)
		getchar();
	
	//=================
	
	if(result!=DFB_OK)
	{
		fprintf(stderr,DirectFBErrorString(result));
	}
	
	if(imagesurface)
		imagesurface->Release(imagesurface);
	if(fb)
		fb->Release(fb);
	if(layer)
		layer->Release(layer);
	if(dfb)
		dfb->Release(dfb);
	
	return 0;
}



