
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <directfb.h>
#include <bdvd.h>

#define SIZEOF_ARRAY(a) (sizeof(a)/sizeof(a[0]))

#define PRINT(x...) { fprintf(stderr,x); }


int main(int argc, char* argv[])
{
	DFBResult result=DFB_OK;
	IDirectFB* dfb=NULL;
	bdvd_dfb_ext_h hLayer=NULL;
	
	if(argc!=2)
	{
		PRINT("USAGE: dfbbcm_halffont FONTNAME\n\n");
		return 1;
	}
	
	bdvd_init(BDVD_VERSION);
	
	bdisplay_open(B_ID(0));
	
	//--- initialize DirectFB and display layer, then display the OSD ---
	if(result==DFB_OK)
	{
		IDirectFBDisplayLayer* layer=NULL;
		bdvd_dfb_ext_layer_settings_t layer_settings;
		dfb=bdvd_dfb_ext_open_with_params(6, &argc, &argv);
		if(dfb==NULL)
		{
			PRINT("DirectFB initialization error!!\n");
			result=DFB_FAILURE;
		}
		
		if(result==DFB_OK)
		{	
			PRINT("opening layer\n");
			hLayer=bdvd_dfb_ext_layer_open(
				//bdvd_dfb_ext_bd_j_background_layer);
				bdvd_dfb_ext_splashscreen_layer);
			
			if(hLayer)
				bdvd_dfb_ext_layer_get(hLayer,&layer_settings);
		}
		
		if(result==DFB_OK)
			result=dfb->GetDisplayLayer(dfb,layer_settings.id,&layer);
		if(result==DFB_OK)
			result=layer->SetCooperativeLevel(layer,DLSCL_ADMINISTRATIVE);
		if(result==DFB_OK)
			result=layer->SetLevel(layer,1);
			
		///########
		if(result==DFB_OK)
		{
			DFBFontDescription fdesc;
			IDirectFBSurface* surface=NULL;
			
			if(result==DFB_OK)
				result=layer->GetSurface(layer,&surface);
			
			fdesc.flags=DFDESC_HEIGHT;
			fdesc.height=48;
			
			IDirectFBFont* font=NULL;
			
			if(result==DFB_OK)
				result=dfb->CreateFont(dfb,argv[1],&fdesc,&font);
			
			if(result==DFB_OK)
				result=surface->SetFont(surface,font);
			if(result==DFB_OK)
				result=surface->SetColor(surface,0xFF,0xFF,0xFF,0xFF);
			if(result==DFB_OK)
				result=surface->DrawGlyph(surface,'5',200,200,DSTF_LEFT|DSTF_TOP);
			if(result==DFB_OK)
				result=surface->DrawGlyph(surface,'5',250,200,DSTF_LEFT|DSTF_TOP|DSTF_HALFWIDTH);
			if(result==DFB_OK)
				result=surface->DrawGlyph(surface,'5',300,200,DSTF_LEFT|DSTF_TOP|DSTF_HALFHEIGHT);
			if(result==DFB_OK)
				result=surface->DrawGlyph(surface,'5',350,200,DSTF_LEFT|DSTF_TOP|DSTF_HALFWIDTH|DSTF_HALFHEIGHT);
			
			if(result==DFB_OK)
				result=surface->DrawString(surface,"half",-1,200,300,DSTF_LEFT|DSTF_TOP);
			if(result==DFB_OK)
				result=surface->DrawString(surface,"half",-1,300,300,DSTF_LEFT|DSTF_TOP|DSTF_HALFWIDTH);
			if(result==DFB_OK)
				result=surface->DrawString(surface,"half",-1,400,300,DSTF_LEFT|DSTF_TOP|DSTF_HALFHEIGHT);
			if(result==DFB_OK)
				result=surface->DrawString(surface,"half",-1,500,300,DSTF_LEFT|DSTF_TOP|DSTF_HALFWIDTH|DSTF_HALFHEIGHT);
			
			if(result==DFB_OK)
				result=surface->Flip(surface,NULL,DSFLIP_BLIT);
			
			if(result!=DFB_OK)
				fprintf(stderr,"DFBerror! %d\n",result);
			
			getchar();
			
			font->Release(font);
			surface->Release(surface);
		}
		///########
		
		if(layer)
			layer->Release(layer);
	}
	
	//===== CLEANUP BDVD =====
	
	if(dfb)
		dfb->Release(dfb);
	
	if(hLayer)
		bdvd_dfb_ext_layer_close(hLayer);
	
	bdvd_dfb_ext_close();
	
	bdvd_uninit();
	
	return 0;
}
