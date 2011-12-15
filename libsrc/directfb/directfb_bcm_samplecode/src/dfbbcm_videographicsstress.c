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
     IDirectFBDisplayLayer   *primary_layer;
     IDirectFBSurface * primary_surface;
     DFBDisplayLayerConfig   primary_layer_config;
     DFBSurfaceDescription primary_desc;

     GfxPerfThread_t gfxPerfThread;

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

     if (argc < 1) {
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
        goto out;
     }

    while ((c = getopt (argc, argv, "rgv")) != -1) {
        switch (c) {
        case 'r':
            video_format = bvideo_format_1080i;
        break;
        case 'g':
            graphics_running = false;
        break;
        case 'v':
            video_running = false;
        break;
        case '?':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        break;
        }
    }

    DFBCHECK(DirectFBInit(&argc, &argv));

    dfb = bdvd_dfb_ext_open(2);

    DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primary_layer) );
    DFBCHECK( primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) );
    DFBCHECK( display_set_video_format(bdisplay, video_format) );

    switch (video_format) {
        case bvideo_format_ntsc:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
        break;
        case bvideo_format_1080i:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
        break;
        default:;
    }

    DFBCHECK( primary_layer->GetSurface(primary_layer, &primary_surface) );
    DFBCHECK( primary_surface->Clear(primary_surface, 0, 0, 0, 0) );

    if (graphics_running) {
        fprintf(stderr, "Creating graphics performance thread\n");
        GfxPerfThreadCreate(dfb, &gfxPerfThread);
        fprintf(stderr, "Starting graphics performance thread\n");
        GfxPerfThreadStart(&gfxPerfThread);
    }

    if (video_running) {
        fprintf(stderr, "Creating video stress thread\n");
        playbackscagat_start(bdisplay, NULL, NULL, NULL, argc, argv);
    }

    getchar();

    if (video_running) {
        playbackscagat_stop();
    }

    if (graphics_running) {
        GfxPerfThreadStop(&gfxPerfThread);
        GfxPerfThreadDelete(&gfxPerfThread);
    }

    DFBCHECK(primary_surface->Release( primary_surface ));
    DFBCHECK(dfb->Release( dfb ));

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

    return ret;
}
    
    