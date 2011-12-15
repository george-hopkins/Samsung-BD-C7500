#include <stdio.h>
#include <directfb.h>
#include <bdvd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dfbbcm_utils.h>

#define DFBCHECK(x...)                                         \
  {                                                            \
    DFBResult err = x;                                         \
                                                               \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        DirectFBErrorFatal( #x, err );                         \
      }                                                        \
  }

DFBSurfaceDescription dsc;
IDirectFB *dfb = NULL;
IDirectFBSurface *surface = NULL;
IDirectFBDisplayLayer* back_layer = NULL;
IDirectFBDisplayLayer* front_layer = NULL;

IDirectFBSurface* primarysurface = NULL;
IDirectFBDisplayLayer* primarylayer = NULL;

bdvd_dfb_ext_h dfb_ext_back;
bdvd_dfb_ext_h dfb_ext_front;
bdvd_dfb_ext_layer_settings_t dfb_ext_settings;
bdvd_display_h bdisplay = NULL;

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

void init_test(int         *argc,
               char       **argv[],
               DFBDisplayLayerConfig * layer_config,
               bool getprimary)
{
    DFBSurfaceCapabilities surface_caps;

	bdvd_init(BDVD_VERSION);

	bdisplay=bdvd_display_open(B_ID(0));

	dfb=bdvd_dfb_ext_open_with_params(2, argc, argv);

    if (getprimary) {
        DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primarylayer) );
        DFBCHECK( primarylayer->SetCooperativeLevel(primarylayer, DLSCL_ADMINISTRATIVE ));
        DFBCHECK( primarylayer->GetSurface(primarylayer, &primarysurface) );
    }
	
	dfb_ext_back=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_osd_layer);
	bdvd_dfb_ext_layer_get(dfb_ext_back, &dfb_ext_settings);
	DFBCHECK( dfb->GetDisplayLayer(dfb,dfb_ext_settings.id,&back_layer) );
	DFBCHECK( back_layer->SetCooperativeLevel(back_layer,DLSCL_ADMINISTRATIVE ));
	DFBCHECK( back_layer->SetLevel(back_layer, 1));

    DFBCHECK( back_layer->GetSurface(back_layer,&surface) );
    
    /* Doing a cyan line for the premultiply experiment */
    DFBCHECK(surface->SetBlittingFlags(surface, DSBLIT_NOFX));
    DFBCHECK(surface->SetColor(surface, 0, 0x80, 0x80, 0xFF));
    DFBCHECK(surface->FillRectangle(surface, 0, 120, 720, 26));    
    
    DFBCHECK(surface->Flip(surface,NULL,DSFLIP_NONE) );
    
    DFBCHECK(surface->Release(surface));

    dfb_ext_front=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_j_graphics_layer);
    bdvd_dfb_ext_layer_get(dfb_ext_front, &dfb_ext_settings);
    DFBCHECK( dfb->GetDisplayLayer(dfb,dfb_ext_settings.id,&front_layer) );
    DFBCHECK( front_layer->SetCooperativeLevel(front_layer,DLSCL_ADMINISTRATIVE ));
    DFBCHECK( front_layer->SetLevel(front_layer, 2));
    
    DFBCHECK( front_layer->SetConfiguration(front_layer, layer_config) );

	DFBCHECK( front_layer->GetSurface(front_layer,&surface) );
    
    DFBCHECK( surface->GetCapabilities(surface, &surface_caps));
    
    if (surface_caps & DSCAPS_PREMULTIPLIED) {
        fprintf(stderr, "Surface of layer %u is DSCAPS_PREMULTIPLIED\n", dfb_ext_settings.id);
    }
    
}


void cleanup_test()
{
	if(surface)
		surface->Release(surface);

	if(front_layer)
		front_layer->Release(front_layer);

    if(back_layer)
        front_layer->Release(back_layer);

    if(primarysurface)
        primarysurface->Release(primarysurface);

    if(primarylayer)
        primarylayer->Release(primarylayer);
    
    if(dfb)
		dfb->Release(dfb);

    bdvd_dfb_ext_layer_close(dfb_ext_back);
	bdvd_dfb_ext_layer_close(dfb_ext_front);
	bdvd_dfb_ext_close();

	bdvd_display_close(bdisplay);
	bdvd_uninit();
}
  
  
int main(int argc, char* argv[])
{
    IDirectFBSurface * image_surface;
    DFBSurfaceDescription image_surface_desc;

    DFBDisplayLayerConfig layer_config;

    IDirectFBWindow * window[3];
    size_t nwindows = sizeof(window)/sizeof(window[0]);
    
    DFBWindowDescription wdesc;
    
    IDirectFBImageProvider* provider;

    DFBSurfaceCapabilities surface_caps;

    uint32_t total_move = 0xFF;

    uint32_t current_time;
    uint32_t previous_time;

    bool window_fade = false;
    bool show_background = false;
    bool dump_frame_buffer = false;
    
    char c;
    
    uint32_t i, j;
    
    char image_name[80];
    char dump_name[80];
    char dump_dir[80];

    if (argc < 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    snprintf(image_name, sizeof(image_name)/sizeof(image_name[0]), "icons/clock.png");
    snprintf(dump_dir, sizeof(dump_dir)/sizeof(dump_dir[0]), "/mnt/nfs");

    memset(&layer_config, 0, sizeof(layer_config));

    while ((c = getopt (argc, argv, "abdfi:p")) != -1) {
        switch (c) {
        case 'b':
            show_background = true;
        break;
        case 'd':
            dump_frame_buffer = true;
            if (optarg) snprintf(dump_dir, sizeof(dump_dir)/sizeof(dump_dir[0]), "%s", optarg);
        break;
        case 'f':
            window_fade = true;
        break;
        case 'i':
            snprintf(image_name, sizeof(image_name)/sizeof(image_name[0]), "%s", optarg);
        break;
        case 'p':
            layer_config.flags |= DLCONF_SURFACE_CAPS;
            layer_config.surface_caps |= DSCAPS_PREMULTIPLIED;
        break;
        case '?':
            print_usage(argv[0]);
            return EXIT_FAILURE;
        break;
        }
    }

	init_test(&argc, &argv, &layer_config, dump_frame_buffer);

	wdesc.flags=DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_POSX | DWDESC_POSY | DWDESC_CAPS;
	wdesc.caps=DWCAPS_ALPHACHANNEL;
	wdesc.width=200;
	wdesc.height=200;
	wdesc.posx=100;
	wdesc.posy=100;
	DFBCHECK ( front_layer->CreateWindow(front_layer,&wdesc,&window[0]) );
	wdesc.posx=150;
	wdesc.posy=150;
	DFBCHECK ( front_layer->CreateWindow(front_layer,&wdesc,&window[1]) );
	wdesc.caps=DWCAPS_NONE;
	wdesc.width=720;
	wdesc.height=480;
	wdesc.posx=0;
	wdesc.posy=0;
	DFBCHECK ( front_layer->CreateWindow(front_layer,&wdesc,&window[2]) );
	
	//send layer 2 to the background
	window[2]->LowerToBottom(window[2]);
	
	DFBCHECK(dfb->CreateImageProvider(dfb,image_name,&provider));
	
    /* If you create an intermediate surface, better have it created with the description
     * of the image provider. If you define dimensions that are not those of the image,
     * RenderTo will do the scaling in software!
     */
    DFBCHECK(provider->GetSurfaceDescription(provider, &image_surface_desc));

    DFBCHECK(dfb->CreateSurface(dfb, &image_surface_desc, &image_surface));

    DFBCHECK(provider->RenderTo(provider, image_surface, NULL));
    
	for(i=0; i<nwindows; i++)
	{
		IDirectFBSurface* ws=NULL;
		
		DFBCHECK(window[i]->GetSurface(window[i],&ws));

        if (i<nwindows-1 || show_background) {
            DFBCHECK(ws->StretchBlit(ws, image_surface, NULL, NULL));
    
            /* This also does an implicit flip (commit) to screen */        
            DFBCHECK(window[i]->SetOpacity(window[i],0xFF));
        }
        else {
            DFBCHECK(ws->Flip(ws, NULL, 0));
        }

		DFBCHECK(ws->Release(ws));
	}
    
    DFBCHECK(image_surface->Release(image_surface));

    previous_time = myclock();

    for(j=0; j<1; j+= dump_frame_buffer ? 1 : 0)
	{
		for(i=0;i<total_move;i++) {
			window[0]->Move(window[0],1,0);
            if (window_fade) DFBCHECK(window[0]->SetOpacity(window[0], i));
        }
        current_time = myclock();
        printf("In step 1 Window 0 moved %u times in %u mseconds\n", total_move, current_time-previous_time);
        previous_time = current_time;
		for(i=0;i<total_move;i++) {
			window[0]->Move(window[0],-1,0);
            if (window_fade) DFBCHECK(window[0]->SetOpacity(window[0], total_move-i));
        }
        current_time = myclock();
        printf("In step 2 Window 0 moved %u times in %u mseconds\n", total_move, current_time-previous_time);
        previous_time = current_time;
	}

    if (dump_frame_buffer) {
        /* We want to see the result of a half opaque window */
        if (window_fade) window[0]->SetOpacity(window[0], 0x80);
        
        DFBCHECK( primarysurface->GetCapabilities(primarysurface, &surface_caps));
        if (surface_caps & DSCAPS_TRIPLE) {
            j = 3;
        }
        else if (surface_caps & DSCAPS_DOUBLE) {
            j = 2;
        }
        else {
            j = 1;
        }
        for (i=0;i<j;i++) {	
            snprintf(dump_name, sizeof(dump_name)/sizeof(dump_name[0]),
                    "compobuffer_%u_dump%s%s%s",
                    i,
                    layer_config.surface_caps & DSCAPS_PREMULTIPLIED ? "_premult" : "",
                    window_fade ? "_fade" : "",
                    show_background ? "_wback" : "");
            DFBCHECK (primarysurface->Dump( primarysurface, "/mnt/nfs", dump_name ));
            DFBCHECK (primarysurface->Flip( primarysurface, NULL, 0 ));
        }
    }
    
	for(i=0;i<nwindows;i++)
	{
		if(window[i])
			DFBCHECK (window[i]->Release(window[i]));
	}

	cleanup_test();
	
    return EXIT_SUCCESS;
}
