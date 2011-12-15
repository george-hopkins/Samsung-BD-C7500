#include <directfb.h>

#include <direct/debug.h>

#include <directfb-internal/misc/util.h>

#include <bdvd.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "dfbbcm_utils.h"
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_gfxperfthread.h"

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

int main( int argc, char *argv[] )
{
     int ret = EXIT_SUCCESS;
     DFBResult err;
     IDirectFB               *dfb;
     /* only used for allocation of the layer context,
      * (done beneath by directfb on GetLayer).
      */
     IDirectFBDisplayLayer * primary_layer;
     IDirectFBSurface * primary_surface;
     
     IDirectFBDisplayLayer * work_layer;
     bdvd_dfb_ext_h work_layer_ext;
     bdvd_dfb_ext_layer_settings_t work_layer_ext_settings;
     IDirectFBSurface * work_surface;
     
     IDirectFBSurface * image_surface;
     DFBSurfaceDescription image_surface_desc;
     
     DFBDisplayLayerConfig primary_layer_config;
     DFBSurfaceDescription primary_desc;

     DFBRectangle rect;

     IDirectFBImageProvider * provider;

     bdvd_display_h bdisplay = NULL;
          
     bvideo_format video_format = bvideo_format_ntsc;
     
     bool graphics_running = true;
     bool video_running = true;
     
     char c;

     if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
         D_ERROR("Could not init bdvd\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
         D_ERROR("Could not open bdvd_display B_ID(0)\n");
         ret = EXIT_FAILURE;
         goto out;
     }

    DFBCHECK(DirectFBInit(&argc, &argv));

    dfb = bdvd_dfb_ext_open(2);

    DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primary_layer) );
    DFBCHECK( primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) );

    DFBCHECK( primary_layer->GetSurface(primary_layer, &primary_surface) );
    DFBCHECK( primary_surface->Clear(primary_surface, 0, 0, 0, 0) );

    if ((work_layer_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_osd_layer)) == NULL) {
        fprintf(stderr,"bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (bdvd_dfb_ext_layer_get(work_layer_ext, &work_layer_ext_settings) != bdvd_ok) {
        fprintf(stderr,"bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    DFBCHECK( dfb->GetDisplayLayer(dfb, work_layer_ext_settings.id, &work_layer) );
    DFBCHECK( work_layer->SetCooperativeLevel(work_layer, DLSCL_ADMINISTRATIVE) );
    DFBCHECK( work_layer->SetBackgroundColor(work_layer, 0, 0x00, 0x00, 0x00 ) );
    DFBCHECK( work_layer->SetBackgroundMode(work_layer, DLBM_COLOR ) );
    
    //DFBCHECK( work_layer->SetConfiguration(work_layer, &config) );
    DFBCHECK( work_layer->GetSurface(work_layer, &work_surface) );

    DFBCHECK( work_layer->SetLevel(work_layer, 1) );
    
    DFBCHECK( work_surface->SetColor(work_surface,
                                    0x00,     //r
                                    0x00,     //g
                                    0x00,     //b
                                    0x00) );  //a
    rect.w = rect.h = 0;
    DFBCHECK( work_surface->GetSize( work_surface, &rect.w, &rect.h) );
    DFBCHECK( work_surface->FillRectangle( work_surface, 0, 0, rect.w, rect.h) );
    DFBCHECK( work_surface->Flip( work_surface, NULL, (DFBSurfaceFlipFlags)0 ) );

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/1080i_fluorescence.png",
                                             &provider ));

    DFBCHECK(provider->GetSurfaceDescription( provider, &image_surface_desc ));
    image_surface_desc.flags = DSDESC_CAPS;
    image_surface_desc.caps &= ~DSCAPS_SYSTEMONLY;
    image_surface_desc.caps |= DSCAPS_VIDEOONLY;
    DFBCHECK(dfb->CreateSurface( dfb, &image_surface_desc, &image_surface ));
    
    DFBCHECK(provider->RenderTo( provider, image_surface, NULL ));

    DFBCHECK(work_surface->SetBlittingFlags( work_surface, DSBLIT_NOFX ));
    DFBCHECK(work_surface->StretchBlit( work_surface, image_surface, NULL, NULL ));
    DFBCHECK(work_surface->Flip( work_surface, NULL, 0 ));

    printf("Changing resolution to bvideo_format_1080i, press any key\n");
    getchar();

    DFBCHECK( display_set_video_format(bdisplay, bvideo_format_1080i) );
    DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );

    printf("Changed res, press any key to flip\n");
    getchar();

    DFBCHECK(work_surface->Flip( work_surface, NULL, 0 ));

#if 0
    printf("Changing resolution to bvideo_format_720p, press any key\n");
    getchar();

    DFBCHECK( display_set_video_format(bdisplay, bvideo_format_720p) );
    DFBCHECK( dfb->SetVideoMode( dfb, 1280/2, 720, 32 ) );

    printf("Changed res, press any key to flip\n");
    getchar();

    DFBCHECK(work_surface->Flip( work_surface, NULL, 0 ));
#endif

    printf("Changing resolution back to bvideo_format_ntsc, press any key\n");
    getchar();

    DFBCHECK( display_set_video_format(bdisplay, bvideo_format_ntsc) );
    DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );

    printf("Changed res, press any key to flip\n");
    getchar();

    DFBCHECK(work_surface->Flip( work_surface, NULL, 0 ));

    printf("Press any key to quit\n");
    getchar();

    DFBCHECK(work_surface->Release( work_surface ));
    DFBCHECK(work_layer->Release( work_layer ));
    DFBCHECK(primary_surface->Release( primary_surface ));
    DFBCHECK(primary_layer->Release( primary_layer ));
    DFBCHECK(dfb->Release( dfb ));

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

    return ret;
}
    
    