
#define BCMPLATFORM

#define BLEND_TEST 1

#include <stdio.h>
#include <directfb.h>
#ifdef BCMPLATFORM
#include <bdvd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <direct/debug.h>

#ifdef BCMPLATFORM
#include "dfbbcm_playbackscagatthread.h"
#endif

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;

    IDirectFB *dfb = NULL;

#ifdef BCMPLATFORM
    bdvd_dfb_ext_h graphics_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;
    IDirectFBDisplayLayer * graphics_layer = NULL;
#else
    DFBSurfaceDescription graphics_surface_desc;
#endif
    IDirectFBSurface * graphics_surface = NULL;

    IDirectFBImageProvider * provider;
    IDirectFBSurface * image_surface = NULL;
    DFBSurfaceDescription image_surface_desc;

	
	int loop_count = 10;
	IDirectFBSurface * image_surface_prev;

	int animation_count = 0;  
	
	int time_end;
	int flipTime_start;	
	int frameDelay;	   
	
	DFBRectangle     src_rect;
	DFBDisposeOp dispose_op = DI_DISPOSE_NONE; 
	time_end = 0;
	flipTime_start = myclock();	
	frameDelay = 0;	   
	
	

    char c;
    
    char graphics_image_name[80];

    int width;
    int height;
    
    uint32_t i;
    
#ifdef BCMPLATFORM
    DFBDisplayLayerConfig layer_config;
    
    /* bdvd_init begin */
    bdvd_display_h bdisplay = NULL;
    if (bdvd_init(BDVD_VERSION) != b_ok) {
        D_ERROR("Could not init bdvd\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
        printf("bdvd_display_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* bdvd_init end */

    layer_config.flags = DLCONF_SURFACE_CAPS;
    layer_config.surface_caps = DSCAPS_PREMULTIPLIED;
#endif

    if (argc < 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    memset(graphics_image_name, 0, sizeof(graphics_image_name));

    while ((c = getopt (argc, argv, "i:")) != -1) {
        switch (c) {
        case 'i':
            snprintf(graphics_image_name, sizeof(graphics_image_name)/sizeof(graphics_image_name[0]), "%s", optarg);
        break;
        case '?':
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
            goto out;
        break;
        }
    }

#ifdef BCMPLATFORM
    dfb = bdvd_dfb_ext_open_with_params(2, &argc, &argv);
    if (dfb == NULL) {
        D_ERROR("bdvd_dfb_ext_open_with_params failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    graphics_dfb_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_hddvd_ac_graphics_layer);
    if (graphics_dfb_ext == NULL) {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (bdvd_dfb_ext_layer_get(graphics_dfb_ext, &graphics_dfb_ext_settings) != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &graphics_layer) != DFB_OK) {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->SetCooperativeLevel(graphics_layer, DLSCL_ADMINISTRATIVE) != DFB_OK) {
        D_ERROR("SetCooperativeLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->SetLevel(graphics_layer, 1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->GetSurface(graphics_layer, &graphics_surface) != DFB_OK) {
        D_ERROR("GetSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#else
    if (DirectFBInit( &argc, &argv ) != DFB_OK) {
        D_ERROR("DirectFBInit failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
     /* create the super interface */
    if (DirectFBCreate( &dfb ) != DFB_OK) {
        D_ERROR("DirectFBCreate failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer. */
    if (dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ) != DFB_OK) {
        D_ERROR("SetCooperativeLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Get the primary surface, i.e. the surface of the primary layer. */
    graphics_surface_desc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;
    graphics_surface_desc.caps = DSCAPS_PRIMARY | DSCAPS_PREMULTIPLIED;
    graphics_surface_desc.width = 960;
    graphics_surface_desc.height = 540;

    if (dfb->CreateSurface( dfb, &graphics_surface_desc, &graphics_surface ) != DFB_OK) {
        D_ERROR("CreateSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#endif

    if (graphics_surface->Clear( graphics_surface, 0x55, 0x55, 0x55, 0xFF ) != DFB_OK) {
        D_ERROR("Clear failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_surface->GetSize(graphics_surface, &width, &height) != DFB_OK) {
        D_ERROR("GetSize failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* End of graphics layer setup */

    if (dfb->CreateImageProvider(dfb, graphics_image_name, &provider) != DFB_OK) {
        D_ERROR("CreateImageProvider failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (provider->GetSurfaceDescription(provider, &image_surface_desc) != DFB_OK) {
        D_ERROR("GetSurfaceDescription failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }	
	
	
    if (dfb->CreateSurface(dfb, &image_surface_desc, &image_surface) != DFB_OK) {
        D_ERROR("CreateSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
	
	if (image_surface_desc.isAnimated)
	{
	if (dfb->CreateSurface(dfb, &image_surface_desc, &image_surface_prev) != DFB_OK) {
        D_ERROR("CreateSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

	if (image_surface_prev->Clear( image_surface_prev, image_surface_desc.background_color.r, image_surface_desc.background_color.g, image_surface_desc.background_color.b, image_surface_desc.background_color.a ) != DFB_OK) {
        D_ERROR("Clear failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

	printf("\nimage_surface_desc.isAnimated = %d, image_surface_desc.num_frames = %d\n", image_surface_desc.isAnimated, image_surface_desc.num_frames);

		while(loop_count)
	{

	if (image_surface->Clear( image_surface, 0x0, 0x0, 0x0, 0x0 ) != DFB_OK) {
        D_ERROR("Clear failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (provider->RenderTo(provider, image_surface, NULL) != DFB_OK) {
        D_ERROR("RenderTo failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
	
		

		if (!provider->AnimationDescription.isPartOfAnimation)
		{
				 if (provider->RenderTo(provider, image_surface, NULL) != DFB_OK) {
		        D_ERROR("RenderTo failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }
		}
		
    /* End of graphics image render */
	
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = provider->AnimationDescription.width;
	src_rect.h = provider->AnimationDescription.height;

	if (animation_count == 0)
	{
			dispose_op = DI_DISPOSE_NONE;
			if (image_surface_prev->Clear( image_surface_prev, image_surface_desc.background_color.r, image_surface_desc.background_color.g, image_surface_desc.background_color.b, image_surface_desc.background_color.a) != DFB_OK) {
	        D_ERROR("Clear failed\n");
	        ret = EXIT_FAILURE;
	        goto out;
	    }
			
		}	
		

		if (provider->AnimationDescription.blend_op == DI_BLEND_OVER_APNG)
		{
			if (image_surface_prev->SetPorterDuff(image_surface_prev, DSPD_NONE)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

			

			if (image_surface_prev->SetBlittingFlags(image_surface_prev, DSBLIT_BLEND_ALPHACHANNEL | (image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? DSBLIT_NOFX : DSBLIT_SRC_PREMULTIPLY))) {
		        D_ERROR("SetBlittingFlags failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }
		}
		else
		if (provider->AnimationDescription.blend_op == DI_BLEND_SOURCE_APNG)	
		{
			if (image_surface_prev->SetPorterDuff(image_surface_prev, DSPD_SRC)) {
		        D_ERROR("SetBlittingFlags failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }

			if (image_surface_prev->SetBlittingFlags(image_surface_prev, (image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? DSBLIT_NOFX : DSBLIT_SRC_PREMULTIPLY))) {
		        D_ERROR("SetBlittingFlags failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }
		}
		else /*GIF style*/
		{
			if (image_surface_prev->SetPorterDuff(image_surface_prev, DSPD_NONE)) {
		        D_ERROR("SetBlittingFlags failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }

			if (image_surface_prev->SetBlittingFlags(image_surface_prev, DSBLIT_BLEND_ALPHACHANNEL | (image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? DSBLIT_NOFX : DSBLIT_SRC_PREMULTIPLY))) {
		        D_ERROR("SetBlittingFlags failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }
		}

		

	if (dispose_op > DI_DISPOSE_RESTORE_PREV)
	{
		dispose_op = DI_DISPOSE_NONE;
	}

	if (dispose_op == DI_DISPOSE_RESTORE_PREV)
	{
		printf("\n\n\n\nDI_DISPOSE_RESTORE_PREV!!!!\n\n\n\n");		
	}

	if ((dispose_op == DI_DISPOSE_NONE) || (provider->AnimationDescription.dispose_op == DI_DISPOSE_KEEP))
	{
	
	    if (image_surface_prev->Blit(image_surface_prev, image_surface, &src_rect, provider->AnimationDescription.x_offset, provider->AnimationDescription.y_offset)) {
	        D_ERROR("SetBlittingFlags failed\n");
	        ret = EXIT_FAILURE;
	        goto out;
	    }
	}
	else 
	if (dispose_op == DI_DISPOSE_RESTORE_BG)
	{
			 



			 if (image_surface_prev->Clear( image_surface_prev, image_surface_desc.background_color.r, image_surface_desc.background_color.g, image_surface_desc.background_color.b, image_surface_desc.background_color.a ) != DFB_OK) {
	        D_ERROR("SetBlittingFlags failed\n");
	        ret = EXIT_FAILURE;
	        goto out;
	    }


		if (image_surface_prev->Blit(image_surface_prev, image_surface, &src_rect, provider->AnimationDescription.x_offset, provider->AnimationDescription.y_offset)) {
		        D_ERROR("SetBlittingFlags failed\n");
		        ret = EXIT_FAILURE;
		        goto out;
		    }
	}
	
	
	dispose_op = provider->AnimationDescription.dispose_op;

	
    /* Blit test */
    if (graphics_surface->SetBlittingFlags(graphics_surface, image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? DSBLIT_NOFX : DSBLIT_SRC_PREMULTIPLY)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
	
	

    if (graphics_surface->Blit(graphics_surface, image_surface_prev, NULL, 0, 0)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    
	time_end = myclock();
	

	if (((frameDelay)*10000) >= ((time_end - flipTime_start)*1000))
	{
		usleep(((frameDelay)*10000) - ((time_end - flipTime_start)*1000));
	}	
	
    
	flipTime_start = myclock();

    if (graphics_surface->Flip(graphics_surface, NULL, DSFLIP_NONE) != DFB_OK) {
        D_ERROR("Flip failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

/*We should ideally have this for accuracy of animation timing. But the call is intensive -- so, disabling it unless animation timing accuracy becomes an issue*/	
#if 0	
	
	if (dfb->WaitIdle(dfb) != DFB_OK) {
        D_ERROR("WaitIdle failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#endif	
	

	animation_count = (animation_count + 1) % image_surface_desc.num_frames;

		if (animation_count == 0)
		{
			loop_count--;
		}

	frameDelay = provider->AnimationDescription.frameDelay;
	
	
	}
	}
	else
	{

	if (provider->RenderTo(provider, image_surface, NULL) != DFB_OK) {
        D_ERROR("RenderTo failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* End of graphics image render */

    /* Blit test */
    if (graphics_surface->SetBlittingFlags(graphics_surface, image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? DSBLIT_NOFX : DSBLIT_SRC_PREMULTIPLY)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_surface->Blit(graphics_surface, image_surface, NULL, 0, 0)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
	
#if BLEND_TEST	
	if (!image_surface_desc.isAnimated)
	{
    /* Blend test */
   if (graphics_surface->SetPorterDuff(graphics_surface, DSPD_SRC_OVER)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_surface->SetBlittingFlags(graphics_surface, DSBLIT_BLEND_ALPHACHANNEL | (image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? 0 : DSBLIT_SRC_PREMULTIPLY))) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_surface->Blit(graphics_surface, image_surface, NULL, width/6, 0)) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Fade test */

    if (graphics_surface->SetBlittingFlags(graphics_surface, DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL | (image_surface_desc.caps & DSCAPS_PREMULTIPLIED ? 0 : DSBLIT_SRC_PREMULTIPLY))) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    for (i = 0; i < 5; i++) {
        if (graphics_surface->SetColor(graphics_surface, 0, 0, 0, 255/4*i)) {
            D_ERROR("SetBlittingFlags failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        
        /* graphics_surface is the destination, image_surface is the source */
        
        if (graphics_surface->Blit(graphics_surface, image_surface, NULL, width/6*i, height/2)) {
            D_ERROR("SetBlittingFlags failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
    }
	}
#endif	

    if (graphics_surface->Flip(graphics_surface, NULL, DSFLIP_NONE) != DFB_OK) {
        D_ERROR("Flip failed\n");
        ret = EXIT_FAILURE;
        goto out;
	    }

    }
    

    D_INFO("Press any key to continue\n");
    getchar();

out:

    /* Cleanup */
    if (image_surface) {
        if (image_surface->Release(image_surface) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }

	if (image_surface_prev) {
        if (image_surface_prev->Release(image_surface_prev) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }

	
	
#ifdef BCMPLATFORM
    if (graphics_layer) {
        if (graphics_layer->SetLevel(graphics_layer, -1) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
#endif
    if (graphics_surface) {
        if (graphics_surface->Release(graphics_surface) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
#ifdef BCMPLATFORM
    if (graphics_layer) {
        if (graphics_layer->Release(graphics_layer) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (graphics_dfb_ext) {
        if (bdvd_dfb_ext_layer_close(graphics_dfb_ext) != bdvd_ok) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (bdvd_dfb_ext_close() != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_close failed\n");
        ret = EXIT_FAILURE;
    }
    /* End of cleanup */

    playbackscagat_stop();

    /* bdvd_uninit begin */
    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();
    /* bdvd_uninit end */
#else
    dfb->Release( dfb );
#endif

    return EXIT_SUCCESS;
}
