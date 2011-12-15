#include <directfb.h>
#include <direct/debug.h>
#include <sys/timeb.h>
#include <time.h>
#include <directfb-internal/misc/util.h>

#include <bsettop.h>
#include <bdvd.h>

#include "dfbbcm_utils.h"

IDirectFB *dfb = NULL;
static bdvd_dfb_ext_h	ihdLayerbdvdExt;
bdvd_dfb_ext_layer_settings_t	ihdLayerbdvdExtSettings;
bdvd_display_settings_t	display_settings;
DFBDisplayLayerConfig dfbLayerConfig;
bdvd_display_h bdisplay;
DFBDisplayLayerID bcm_layer_id = 0;
IDirectFBDisplayLayer* lite_layer=NULL;

int init_bdfb();
void uninit_bdfb();
void test1();
void test2();

int main(int argc, char* argv[])
{
/*--------------  Init --------------*/
	init_bdfb();
/*-----------------------------------*/
	test1();
	test2();

/*--------------  Release  --------------*/
	uninit_bdfb();
/*---------------------------------------*/

    return 0;
}

static int btn =  100;

void test2()
{
        DFBResult             err;
        DFBSurfaceDescription desc;
        IDirectFBSurface *surface = NULL;
        IDirectFBSurface *bidon[btn];
	DFBRectangle rr;
        struct timeb timebuffer2;
        char *timeline2;

	int iloop = 0, xl = 0, yl = 0;
	int w1 = 100, h1 =20;
	int xpos = 0, ypos = 0;

	rr.x = rr.y = 0;
	rr.w = w1, rr.h = h1;

        DFBCHECK ( lite_layer->GetSurface(lite_layer,&surface) );
        DFBCHECK ( surface->Clear(surface, 0, 0, 0, 0) );

	desc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
        desc.caps = DSCAPS_VIDEOONLY;
	desc.pixelformat = DSPF_ARGB;   

	desc.width = w1;
	desc.height = h1;
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before create surfaces  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

	for(iloop = 0; iloop < btn; iloop++)
	{
    	    DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon[iloop] ));
	}
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "After create surfaces  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	for(iloop = 0; iloop < btn; iloop++)
	{
            bidon[iloop]->SetColor(bidon[iloop], 0,255,255,125);
	    DFBCHECK(bidon[iloop]->FillRectangle(bidon[iloop], 0, 0, w1, h1));    
	}
  fprintf(stderr, "After setcolr & fill surfaces  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	
	surface->SetPorterDuff(surface, DSPD_SRC_OVER);  
	surface->SetBlittingFlags(surface, DSBLIT_BLEND_ALPHACHANNEL);
        surface->SetDrawingFlags(surface,DSDRAW_BLEND);
        surface->SetColor(surface,255,255,255,255);
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before blit..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        for(iloop = 0; iloop < btn; iloop++)
	{
	    xpos = 160 + (iloop % 10) * 110;
	    ypos = 100 + (iloop / 10) * 30;

    	    surface->Blit(surface, bidon[iloop], &rr, xpos, ypos );
	}
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "After blit..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        rr.w = 1920;
        rr.h = 1080;
		
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before Flip..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	
	
        surface->Flip(surface,&rr,DSFLIP_NONE);

  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "After Flip..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
        fprintf(stderr,"press enter...");
        getchar();



	for(iloop = 0; iloop < btn; iloop++)
	{
    	     bidon[iloop]->Release(bidon[iloop]);
	}
	surface->Release(surface);	
}

void test1()
{
        DFBResult             err;
        DFBSurfaceDescription desc;
        IDirectFBSurface *surface = NULL;
        IDirectFBSurface *bidon1 = NULL;
        IDirectFBSurface *bidon2 = NULL;
        IDirectFBSurface *bidon3 = NULL;
        IDirectFBSurface *bidon4 = NULL;
        IDirectFBSurface *bidon5 = NULL;
        IDirectFBSurface *bidon6 = NULL;
        IDirectFBSurface *bidon7 = NULL;
        IDirectFBSurface *bidon8 = NULL;

	DFBRectangle rr;

	int w = 1920, h =1080;

	int w1 = 200, h1 =200;
	int w2 = 1920, h2 =1080;
	int w3 = 1640, h3 =110, x3 = 140, y3 = 400;
	int w4 = 1640, h4 =110, x4 = 140, y4 = 600;
        struct timeb timebuffer2;
        char *timeline2;

        rr.x = 0;
        rr.y = 0;

        DFBCHECK ( lite_layer->GetSurface(lite_layer,&surface) );
        DFBCHECK ( surface->Clear(surface, 0, 0, 0, 0) );

	desc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
        desc.caps = DSCAPS_VIDEOONLY;
	desc.pixelformat = DSPF_ARGB;   

	desc.width = w1;
	desc.height = h1;
        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon1 ));

	desc.width = w;
	desc.height = h;

  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
        
	DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon5 ));
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "after  create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        bidon5->SetColor(bidon5, 255,255,255,150);
	DFBCHECK(bidon5->FillRectangle(bidon5, 0, 0, w, h));

  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	    
        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon6 ));
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "after  create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        bidon6->SetColor(bidon6, 0,150,150,125);
	DFBCHECK(bidon6->FillRectangle(bidon6, 0, 0, w, h)); 

  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	   
        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon7 ));
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "after  create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        bidon7->SetColor(bidon7,50,120,120,100);
	DFBCHECK(bidon7->FillRectangle(bidon7, 0, 0, w, h));

  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	    
        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon8 ));
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "after  create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        bidon8->SetColor(bidon8, 100,0,200,85);
	DFBCHECK(bidon8->FillRectangle(bidon8, 0, 0, w, h));    

	
	desc.width = w2;
	desc.height = h2;
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon2 ));
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "after  create surface (1920x1080)  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        bidon2->SetColor(bidon2, 0,0,0,75);
	DFBCHECK(bidon2->FillRectangle(bidon2, 0, 0, w2, h2));    

	desc.width = w3;
	desc.height = h3;
        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon3 ));
        bidon3->SetColor(bidon3, 0,255,0,125);
	DFBCHECK(bidon3->FillRectangle(bidon3, 0, 0, w3, h3));    
	
	desc.width = w4;
	desc.height = h4;
        DFBCHECK(dfb->CreateSurface( dfb, &desc, &bidon4 ));
        bidon4->SetColor(bidon4, 0, 0, 255, 75);
	DFBCHECK(bidon3->FillRectangle(bidon4, 0, 0, w4, h4));    

	surface->SetPorterDuff(surface, DSPD_SRC_OVER);  
	surface->SetBlittingFlags(surface, DSBLIT_BLEND_ALPHACHANNEL);
//          (DFBSurfaceBlittingFlags)(DSBLIT_SRC_PREMULTIPLY | DSBLIT_BLEND_ALPHACHANNEL  | DSBLIT_BLEND_COLORALPHA));
        surface->SetDrawingFlags(surface,DSDRAW_BLEND);
        surface->SetColor(surface,255,255,255,125);
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before blit..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        surface->Blit(surface, bidon5, NULL, 0, 0 );
        surface->Blit(surface, bidon6, NULL, 0, 0 );
        surface->Blit(surface, bidon7, NULL, 0, 0 );
        surface->Blit(surface, bidon8, NULL, 0, 0 );


        surface->Blit(surface, bidon2, NULL, 0, 0 );

        rr.w = w3;
        rr.h = h3;
        surface->Blit(surface, bidon3, &rr, x3, y3 );

        rr.w = w4;
        rr.h = h4;
        surface->Blit(surface, bidon4, &rr, x4, y4 );
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "After blit..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);

        rr.w = w;
        rr.h = h;	
  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "Before Flip..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
	
	
        surface->Flip(surface,/*&rr*/NULL,DSFLIP_NONE);

  ftime(&timebuffer2);
  timeline2 = ctime(&(timebuffer2.time));
  fprintf(stderr, "After Flip..  %.19s.%hu %s\n",timeline2, timebuffer2.millitm, &timeline2[20]);
        
        fprintf(stderr,"press enter...");
        getchar();

	bidon1->Release(bidon1);
	bidon2->Release(bidon2);
	bidon3->Release(bidon3);
	bidon4->Release(bidon4);
	bidon5->Release(bidon5);
	bidon6->Release(bidon6);
	bidon7->Release(bidon7);
	bidon8->Release(bidon8);

	surface->Release(surface);


}

int init_bdfb()
{

	DFBResult ret;


	if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
       	DirectFBError("initBCM: bdvd_init", DFB_FAILURE);
         	return DFB_FAILURE;
     	}

	if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
		DirectFBError("initBCM: bdisplay_open", DFB_FAILURE);
	       return DFB_FAILURE;
	}
		
dfb = bdvd_dfb_ext_open(4);
	bdvd_display_get(bdisplay, &display_settings);
	display_settings.format = bvideo_format_1080i;
	bdvd_display_set(bdisplay, &display_settings);
	
        if (dfb->SetVideoMode(dfb, 1920, 1080, 32) != DFB_OK) {
           printf("SetVideoMode Get error!\n");
        }

	ihdLayerbdvdExt = bdvd_dfb_ext_layer_open(2); /*PKG30 case equ. to bdvd_dfb_ext_sync_faa_layer*/
	printf("covadis>>>>>>> layer open\n");
		
	if( ihdLayerbdvdExt == NULL ){
		printf("ihdLayerbdvdExt error!\n");
	}

	if( bdvd_dfb_ext_layer_get(ihdLayerbdvdExt, &ihdLayerbdvdExtSettings) != bdvd_ok ){
		printf("ihdLayerbdvdExt Get error!\n");
	}

	bcm_layer_id = 2; // just for test by covadis
		
	dfb->GetDisplayLayer(dfb, bcm_layer_id, &lite_layer);

	lite_layer->SetCooperativeLevel(lite_layer,DLSCL_ADMINISTRATIVE);
	lite_layer->SetBackgroundColor(lite_layer,0,0,0,0);
	lite_layer->SetBackgroundMode(lite_layer,DLBM_COLOR);

	/* DirectFB Layer Config Setting */
	dfbLayerConfig.flags = (DFBDisplayLayerConfigFlags)(DLCONF_WIDTH | DLCONF_HEIGHT);
	dfbLayerConfig.width = 1920;
	dfbLayerConfig.height = 1080;

	lite_layer->SetConfiguration(lite_layer, &dfbLayerConfig);
	lite_layer->SetCursorOpacity(lite_layer, 0);

	lite_layer->SetLevel(lite_layer,DFB_DISPLAYLAYER_IDS_MAX-2);  /*Under OSD/GUI Layer*/

	printf("SUCCESS: iHD LAYER CREATION! w: = %d, h: %d\n", dfbLayerConfig.width, dfbLayerConfig.height);
	//------------------------------------------------------------------------		  


    return 0;
}

void uninit_bdfb()
{
	lite_layer->Release(lite_layer);
	lite_layer = NULL;
	printf("Layer Release\n");
			
	bdvd_dfb_ext_layer_close(ihdLayerbdvdExt);			   
	printf("ext layer close\n");
			
	dfb->Release(dfb);
	dfb   = NULL;
	printf("Release dfb done\n");

     if (bdisplay) bdisplay_close(bdisplay);
     bsettop_uninit();

}
