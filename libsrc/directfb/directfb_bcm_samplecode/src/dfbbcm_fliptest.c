#include <directfb.h>

#include <direct/debug.h>

#include <directfb-internal/misc/util.h>

#include <bdvd.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "dfbbcm_utils.h"

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
     DFBDisplayLayerConfig   primary_layer_config;
     IDirectFBSurface * primary_surface;
     DFBSurfaceDescription primary_desc;
     
     IDirectFBImageProvider *provider;
     DFBSurfaceDescription other_desc;

     DFBFontDescription fontdesc;
     IDirectFBFont *font;
     int width, height;
     uint32_t i, j;
     
     DFBRegion       movement_region;
     
     char c;

     uint32_t nb_buffers = 1;
     uint32_t nb_iters = 1;

     bool flag_createsurface = false;
     
     bdvd_display_h bdisplay = NULL;

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

     if (argc < 1) {
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
        goto out;
     }

     while ((c = getopt (argc, argv, "cdtn:x")) != -1) {
        switch (c) {
        case 'c':
        break;
        case 'd':
            nb_buffers = 2;
        break;
        case 't':
            nb_buffers = 3;
        break;
        case 'n':
            nb_iters = strtoul(optarg, NULL, 10);
        break;
        case 'x':
            flag_createsurface = true;
        break;
        case '?':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        break;
        }
    }

    DFBCHECK(DirectFBCreate(&dfb));

    if (flag_createsurface) {

    DFBCHECK(dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN));

    primary_desc.flags = DSDESC_CAPS;
    primary_desc.caps = DSCAPS_PRIMARY;

    if (nb_buffers == 2) {
        primary_desc.caps |= DSCAPS_DOUBLE;
    }

    if (nb_buffers == 3) {
        primary_desc.caps |= DSCAPS_TRIPLE;
    }

    DFBCHECK(dfb->CreateSurface(dfb, &primary_desc, &primary_surface));

    }
    else {

    DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primary_layer) );
    DFBCHECK( primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) );

    primary_layer_config.flags = DLCONF_BUFFERMODE;

    switch (nb_buffers) {
        case 1:
            primary_layer_config.buffermode = DLBM_FRONTONLY;
        break;
        case 2:
            primary_layer_config.buffermode = DLBM_BACKVIDEO;
        break;
        case 3:
            primary_layer_config.buffermode = DLBM_TRIPLE;
        break;
    }

    DFBCHECK( primary_layer->SetConfiguration(primary_layer, &primary_layer_config) );

    DFBCHECK( display_set_video_format(bdisplay, bvideo_format_ntsc) );
    /* Setting output format */
    DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );

    DFBCHECK( primary_layer->GetSurface(primary_layer, &primary_surface) );

    }

    DFBCHECK(primary_surface->GetSize(primary_surface, &width, &height));

    fontdesc.flags = DFDESC_HEIGHT;
    fontdesc.height = 40;

    DFBCHECK(dfb->CreateFont (dfb, DATADIR"/fonts/msgothic.ttc", &fontdesc, &font));

    DFBCHECK(primary_surface->SetFont( primary_surface, font ));

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xFF, 0, 0, 0xFF));

    DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));

    /* flip display */
    DFBCHECK(primary_surface->Flip( primary_surface, NULL, 0 ));

    fprintf(stderr, "Flipping 0\n");

    getchar();

    if (nb_buffers > 1) {
        DFBCHECK(primary_surface->SetColor (primary_surface, 0, 0xFF, 0, 0xFF));
    
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));
    
        fprintf(stderr, "Flipping 1\n");
    
        /* flip display */
        DFBCHECK(primary_surface->Flip( primary_surface, NULL, 0 ));

        getchar();
    }

    if (nb_buffers > 2) {
        DFBCHECK(primary_surface->SetColor (primary_surface, 0, 0, 0xFF, 0xFF));
    
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));

        fprintf(stderr, "Flipping 2\n");
    
        /* flip display */
        DFBCHECK(primary_surface->Flip( primary_surface, NULL, 0 ));

        getchar();
    }

#if 0
    i = 0;
    
    while (1) {
        if (i == nb_buffers) {
            i = 0;
        }

        switch (i) {
            case 0:
            DFBCHECK(primary_surface->SetColor (primary_surface, 0xFF, 0, 0, 0xFF));
            break;
            case 1:
            DFBCHECK(primary_surface->SetColor (primary_surface, 0, 0xFF, 0, 0xFF));
            break;
            case 2:
            DFBCHECK(primary_surface->SetColor (primary_surface, 0, 0, 0xFF, 0xFF));
            break;
        }
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));

        /* flip display */
        DFBCHECK(primary_surface->Flip( primary_surface, NULL, 0 ));
        
        i++;
    }
#endif

    int x = 0;
    int y = 0;

    movement_region.x1 = 0;
    movement_region.y1 = 0;
    movement_region.x2 = width > 100 ? width - 100 : 1;
    movement_region.y2 = height > 100 ? height - 100 : 1;

    fprintf(stderr, "Steady state\n");

    i = 0;

    while (i++ < 300) {
        for (j = 0; j < nb_iters; j++) {
        DFBCHECK(primary_surface->SetColor (primary_surface, 0xC0, 0, 0, 0xFF));
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x40, 0, 0, 0xFF));
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));
        }

        movement_edgetoedge_crosshair(&x, &y, &movement_region);

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x80, 0, 0, 0xFF));
        DFBCHECK(primary_surface->FillRectangle( primary_surface, x, y, 100, 100 ));

        /* flip display */
        DFBCHECK(primary_surface->Flip( primary_surface, NULL, 0 ));
    }

    fprintf(stderr, "No flipping\n");

    while (1) {
        DFBCHECK(primary_surface->SetColor (primary_surface, 0xC0, 0, 0, 0xFF));
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x40, 0, 0, 0xFF));
        DFBCHECK(primary_surface->FillRectangle( primary_surface, 0, 0, width, height ));

        movement_edgetoedge_crosshair(&x, &y, &movement_region);

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x80, 0, 0, 0xFF));
        DFBCHECK(primary_surface->FillRectangle( primary_surface, x, y, 100, 100 ));
    }


    DFBCHECK(primary_surface->Release( primary_surface ));
    DFBCHECK(dfb->Release( dfb ));

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

    return ret;
}

