#if 1

#include <directfb.h>

#include <direct/util.h>
#include <direct/debug.h>

#include <bdvd.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dfbbcm_utils.h"

static inline uint32_t gettimeus()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000000 + tv.tv_usec);
}

/* This is for the purpose of the test */
#define TEST_DISPLAYLAYER_IDS_MAX ((DFB_DISPLAYLAYER_IDS_MAX-1)*2)

int main( int argc, char *argv[] )
{
     int ret;
     DFBResult err;
     IDirectFB               *dfb;
     IDirectFBDisplayLayer   *primary_layer;
     IDirectFBSurface        *primary_layer_surface;
     IDirectFBDisplayLayer   *generic_layers[TEST_DISPLAYLAYER_IDS_MAX];
     IDirectFBSurface        *layer_surfaces[TEST_DISPLAYLAYER_IDS_MAX];

     bdvd_dfb_ext_h graphics_dfb_ext[TEST_DISPLAYLAYER_IDS_MAX];
     bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;

     DFBSurfaceDescription   dsc;
     IDirectFBSurface        *background_surface;
     DFBRectangle back_rect = {0,0,0,0};
     IDirectFBSurface        *yellow_surface;
     DFBRectangle yellow_rect = {0,0,0,0};
     IDirectFBSurface        *blue_surface;
     DFBRectangle blue_rect = {0,0,0,0};
     IDirectFBImageProvider  *provider;
     int xoff = 0;
     int yoff = 0;

     bool get_primary_layer = false;
     bool get_primary_layer_surface = false;
     bool set_primary_layer_config = false;
     DFBDisplayLayerConfig primary_layer_config;
     bool set_genericlayer_config = false;
     DFBDisplayLayerConfig genericlayer_config;

     bool show_background = true;

     char c;

     int width, height;

     DFBDisplayLayerID layer_index;
     char * genericlayers = NULL;
     uint32_t namedlayers = 0;
     char string_envvar[80];
     char string_param[80];

     bdvd_display_h bdisplay = NULL;
     bdvd_display_settings_t display_settings;

     uint32_t current_time;
     uint32_t previous_time;


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

     bdvd_display_get(bdisplay, &display_settings);

     display_settings.format = bdvd_video_format_1080i;
     display_settings.svideo = NULL;
     display_settings.composite = NULL;
     display_settings.rf = NULL;

     if (bdvd_display_set(bdisplay, &display_settings) != bdvd_ok) {
        D_ERROR("bdvd_display_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
     }

    dfb = bdvd_dfb_ext_open_with_params((DFB_DISPLAYLAYER_IDS_MAX+1), NULL, NULL);

    dfb->SetVideoMode(dfb, 1920, 1080, 32);

    graphics_dfb_ext[0] = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_j_graphics_layer);
    if (graphics_dfb_ext == NULL)
    {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (bdvd_dfb_ext_layer_get(graphics_dfb_ext[0], &graphics_dfb_ext_settings) != bdvd_ok)
    {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &generic_layers[0]) != DFB_OK)
    {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }


    DFBCHECK( generic_layers[0]->SetCooperativeLevel(generic_layers[0], DLSCL_ADMINISTRATIVE) );
    DFBCHECK( generic_layers[0]->SetBackgroundColor(generic_layers[0], 0, 0, 0, 0 ) );
    DFBCHECK( generic_layers[0]->SetBackgroundMode(generic_layers[0], DLBM_COLOR ) );

    {
        DFBDisplayLayerConfig LayerConfig;

        DFBCHECK( generic_layers[0]->GetConfiguration(generic_layers[0], &LayerConfig) );

        LayerConfig.flags = (DFBDisplayLayerConfigFlags)(DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT);
        LayerConfig.width = 1920;
        LayerConfig.height = 1080;
        LayerConfig.pixelformat = DSPF_ARGB;

        DFBCHECK( generic_layers[0]->SetConfiguration(generic_layers[0], &LayerConfig) );
    }

    DFBCHECK( generic_layers[0]->GetSurface(generic_layers[0], &layer_surfaces[0]) );

    DFBCHECK( layer_surfaces[0]->GetSize( layer_surfaces[0], &width, &height) );

    printf("Layer width = %d height = %d\n", width, height);

    DFBCHECK( generic_layers[0]->SetLevel(generic_layers[0], 1) );
/*

    DFBCHECK( layer_surfaces[0]->SetColor( layer_surfaces[0], 0x00, 0x00, 0x00,  0x00) );
    DFBCHECK( layer_surfaces[0]->FillRectangle( layer_surfaces[0], 0, 0, width, height) );
    DFBCHECK( layer_surfaces[0]->Flip(layer_surfaces[0], NULL, DSFLIP_NONE));           // Flip

    getchar();
    */
#if 0
    DFBCHECK( layer_surfaces[0]->SetColor( layer_surfaces[0], 0x00, 0x00, 0xFF,  0x00) );
    DFBCHECK( layer_surfaces[0]->FillRectangle( layer_surfaces[0], 0, 0, width/2, height/2) );
    DFBCHECK( layer_surfaces[0]->SetBlittingFlags( layer_surfaces[0], DSBLIT_XOR) );



    D_INFO("Starting Test\n");

    previous_time = gettimeus();
    {
        #define TEST_NUM_FRAME 1
        int i;

        for (i=0; i<TEST_NUM_FRAME; i++)
        {
            DFBCHECK( layer_surfaces[0]->Blit(
                        layer_surfaces[0],
                        background_surface,
                        &back_rect,//whole rect
                        xoff,yoff));

            DFBCHECK( layer_surfaces[0]->Flip( layer_surfaces[0], NULL, 0 ) );
        }
    }

    current_time = gettimeus();

    D_INFO("Test completed\n");
    D_INFO("- total time for %d (Blit+Flip) is %dms\n", TEST_NUM_FRAME, (current_time-previous_time)/1000);
    D_INFO("- avg time for each (Blit+Flip) is %dms\n", ((current_time-previous_time)/1000)/TEST_NUM_FRAME);

#else
#if 1
    printf("Setting xor operation\n");
    DFBCHECK( layer_surfaces[0]->SetPorterDuff(layer_surfaces[0], DSPD_XOR));           // Set XOR porterduff rule
    printf("fill one\n");
    DFBCHECK( layer_surfaces[0]->SetColor(layer_surfaces[0], 0x00, 0x00, 0xff, 0xff));  // Set Blue color
    DFBCHECK( layer_surfaces[0]->FillRectangle(layer_surfaces[0], 100, 100, 200, 200)); // FillRectangle
#if 1
    printf("fill two\n");
    DFBCHECK( layer_surfaces[0]->SetColor(layer_surfaces[0], 0xff, 0x00, 0x00, 0xff));  // Set red color
    DFBCHECK( layer_surfaces[0]->FillRectangle(layer_surfaces[0], 200, 200, 200, 200)); // FillRectangle
#endif

    DFBCHECK( layer_surfaces[0]->Flip(layer_surfaces[0], NULL, DSFLIP_NONE));           // Flip
    D_INFO("Test completed\n");
#endif
    getchar();
#endif



out:

    return ret;
}

#else

#include <directfb.h>

#include <direct/util.h>
#include <direct/debug.h>

#include <bdvd.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dfbbcm_utils.h"

void print_usage(const char * program_name) {

    uint32_t i;

    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "Note: l is prioritized over n\n");
    fprintf(stderr, "<-l nblayers>, register nblayers additionnal generic layers to a maximum of %d\n", DFB_DISPLAYLAYER_IDS_MAX-1);
    fprintf(stderr, "<-n nblayers>, register nblayers additionnal named layers to a maximum of %d\n", DFB_DISPLAYLAYER_IDS_MAX-1);
    fprintf(stderr, "<-p>, get the primary layer\n");
    fprintf(stderr, "<-s>, get the primary layer surface\n");
    fprintf(stderr, "<-a>, set the primary layer pixelformat\n");
    fprintf(stderr, "<-b>, set the primary layer width\n");
    fprintf(stderr, "<-c>, set the primary layer height\n");
    fprintf(stderr, "<-d>, set the primary layer buffermode\n");
    fprintf(stderr, "<-e>, set the generic layer(s) pixelformat\n");
    fprintf(stderr, "<-f>, set the generic layer(s) width\n");
    fprintf(stderr, "<-g>, set the generic layer(s) height\n");
    fprintf(stderr, "<-h>, set the generic layer(s) buffermode\n");
    fprintf(stderr, "<-r>, hide background\n");

    fprintf(stderr, "Supported pixelformat are:\n");
    for (i = 0; i < nb_format_strings(); i++) {
        fprintf(stderr, "%s, ", format_strings[i].string);
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Supported buffermode are:\n");
    fprintf(stderr, "FRONTONLY, ");
    fprintf(stderr, "BACKVIDEO, ");
    fprintf(stderr, "BACKSYSTEM\n");

}

/* This is for the purpose of the test */
#define TEST_DISPLAYLAYER_IDS_MAX ((DFB_DISPLAYLAYER_IDS_MAX-1)*2)

int main( int argc, char *argv[] )
{
     int ret;
     DFBResult err;
     IDirectFB               *dfb;
     IDirectFBDisplayLayer   *primary_layer;
     IDirectFBSurface        *primary_layer_surface;
     IDirectFBDisplayLayer   *generic_layers[TEST_DISPLAYLAYER_IDS_MAX];
     IDirectFBSurface        *layer_surfaces[TEST_DISPLAYLAYER_IDS_MAX];

     bdvd_dfb_ext_h graphics_dfb_ext[TEST_DISPLAYLAYER_IDS_MAX];
     bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;

     DFBSurfaceDescription   dsc;
     IDirectFBSurface        *background_surface;
     DFBRectangle back_rect = {0,0,0,0};
     IDirectFBSurface        *yellow_surface;
     DFBRectangle yellow_rect = {0,0,0,0};
     IDirectFBSurface        *blue_surface;
     DFBRectangle blue_rect = {0,0,0,0};
     IDirectFBImageProvider  *provider;
     int xoff = 0;
     int yoff = 0;

     bool get_primary_layer = false;
     bool get_primary_layer_surface = false;
     bool set_primary_layer_config = false;
     DFBDisplayLayerConfig primary_layer_config;
     bool set_genericlayer_config = false;
     DFBDisplayLayerConfig genericlayer_config;

     bool show_background = true;

     char c;

     int width, height;

     DFBDisplayLayerID layer_index;
     char * genericlayers = NULL;
     uint32_t namedlayers = 0;
     char string_envvar[80];
     char string_param[80];

     bdvd_display_h bdisplay = NULL;
     bdvd_display_settings_t display_settings;

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

     bdvd_display_get(bdisplay, &display_settings);

     display_settings.format = bdvd_video_format_1080i;
     display_settings.svideo = NULL;
     display_settings.composite = NULL;
     display_settings.rf = NULL;

     if (bdvd_display_set(bdisplay, &display_settings) != bdvd_ok) {
        D_ERROR("bdvd_display_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
     }


/*
     DFBCHECK( DirectFBInit(NULL, NULL) );
     */

     if (argc < 1) {
        print_usage(argv[0]);
        return -1;
     }

     while ((c = getopt (argc, argv, "l:n:psa:b:c:d:e:f:g:h:r")) != -1) {
        switch (c) {
        case 'l':
            /* No need to check the value to the system maximum, this is done at a lower level,
             * plus we want to test the error handling. We do have a max in this test,
             * but it's far more than the system's.
             */
            if (strtoul(optarg, NULL, 10) > TEST_DISPLAYLAYER_IDS_MAX) {
                D_ERROR("maximum generic layer reached\n");
                exit(EXIT_FAILURE);
            }
            else {
                genericlayers = optarg;
            }
        break;
        case 'n':
            /* Same as l
             */
            if ((namedlayers = strtoul(optarg, NULL, 10)) > TEST_DISPLAYLAYER_IDS_MAX) {
                D_ERROR("maximum generic layer reached\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'p':
            get_primary_layer = true;
        break;
        case 's':
            get_primary_layer_surface = true;
        break;
        case 'a':
            set_primary_layer_config = true;
            primary_layer_config.flags |= DLCONF_PIXELFORMAT;
            if ((primary_layer_config.pixelformat = convert_string2dspf(optarg)) == DSPF_UNKNOWN) {
                D_ERROR("Unknown pixel format\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'b':
            set_primary_layer_config = true;
            primary_layer_config.flags |= DLCONF_WIDTH;
            primary_layer_config.width = strtoul(optarg, NULL, 10);
        break;
        case 'c':
            set_primary_layer_config = true;
            primary_layer_config.flags |= DLCONF_HEIGHT;
            primary_layer_config.height = strtoul(optarg, NULL, 10);
        break;
        case 'd':
            set_primary_layer_config = true;
            primary_layer_config.flags = DLCONF_BUFFERMODE;
            if (!strcmp(optarg, "FRONTONLY")) {
                primary_layer_config.buffermode = DLBM_FRONTONLY;
            }
            else if (!strcmp(optarg, "BACKVIDEO")) {
                primary_layer_config.buffermode = DLBM_BACKVIDEO;
            }
            else if (!strcmp(optarg, "BACKSYSTEM")) {
                primary_layer_config.buffermode = DLBM_BACKSYSTEM;
            }
            else {
                D_ERROR("Unknown buffer mode\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'e':
            set_genericlayer_config = true;
            genericlayer_config.flags |= DLCONF_PIXELFORMAT;
            if ((genericlayer_config.pixelformat = convert_string2dspf(optarg)) == DSPF_UNKNOWN) {
                D_ERROR("Unknown pixel format\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'f':
            set_genericlayer_config = true;
            genericlayer_config.flags |= DLCONF_WIDTH;
            genericlayer_config.width = strtoul(optarg, NULL, 10);
        break;
        case 'g':
            set_genericlayer_config = true;
            genericlayer_config.flags |= DLCONF_HEIGHT;
            genericlayer_config.height = strtoul(optarg, NULL, 10);
        break;
        case 'h':
            set_genericlayer_config = true;
            genericlayer_config.flags = DLCONF_BUFFERMODE;
            if (!strcmp(optarg, "FRONTONLY")) {
                genericlayer_config.buffermode = DLBM_FRONTONLY;
            }
            else if (!strcmp(optarg, "BACKVIDEO")) {
                genericlayer_config.buffermode = DLBM_BACKVIDEO;
            }
            else if (!strcmp(optarg, "BACKSYSTEM")) {
                genericlayer_config.buffermode = DLBM_BACKSYSTEM;
            }
            else {
                D_ERROR("Unknown buffer mode\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'r':
            show_background = false;
        break;
        case '?':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        break;
        }
    }

    if (genericlayers) {
        if (setenv( "BCM_NB_GENERIC_LAYERS", genericlayers, 1 ) == -1) {
            D_PERROR("Could not set environment variable\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Don't exclude, see what happens if genericlayers was set */
    if (namedlayers) {
        for (layer_index = 1; (layer_index-1) < namedlayers; layer_index++) {
            snprintf(string_envvar, sizeof(string_envvar), "BCM_LAYER_%u_NAME", layer_index);
            snprintf(string_param, sizeof(string_param), "Test %u", layer_index);
            if (setenv( string_envvar, string_param, 1 ) == -1) {
                D_PERROR("Could not set environment variable\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    D_INFO("-------- bdvd_dfb_ext_open_with_params --------\n");
    D_INFO("-------- bdvd_dfb_ext_open_with_params --------\n");
    D_INFO("-------- bdvd_dfb_ext_open_with_params --------\n");

    dfb = bdvd_dfb_ext_open_with_params((DFB_DISPLAYLAYER_IDS_MAX+1), NULL /*&argc*/, NULL /*&argv*/);

    D_INFO("-------- end bdvd_dfb_ext_open_with_params --------\n");
    D_INFO("-------- end bdvd_dfb_ext_open_with_params --------\n");
    D_INFO("-------- end bdvd_dfb_ext_open_with_params --------\n");

    D_INFO("-------- set video mode 0 --------\n");

    dfb->SetVideoMode(dfb, 1920, 1080, 32);

    D_INFO("-------- set video mode 1 --------\n");

#if 0
    if (get_primary_layer) {

        graphics_dfb_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_presentation_layer);
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
        if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &primary_layer) != DFB_OK) {
            D_ERROR("GetDisplayLayer failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        if (primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) != DFB_OK) {
            D_ERROR("SetCooperativeLevel failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }

        /*DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primary_layer) );*/
        DFBCHECK( primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) );

        DFBCHECK( primary_layer->SetBackgroundColor(primary_layer, 0, 0, 0, 0 ) );
        DFBCHECK( primary_layer->SetBackgroundMode(primary_layer, DLBM_COLOR ) );

        if (get_primary_layer_surface) {
            DFBCHECK( primary_layer->GetSurface(primary_layer, &primary_layer_surface) );
        }
    }

    if (set_primary_layer_config) {
        DFBCHECK( primary_layer->SetConfiguration(primary_layer, &primary_layer_config) );
    }


    D_DEBUG("-------- Enumerating display layers --------\n");
    DFBCHECK( dfb->EnumDisplayLayers( dfb, enum_layers_callback, NULL ) );
#endif
    /* loading images into their respective surfaces  DATADIR"/images/1080i_layer1_argb.png" */

    D_INFO("-------- loading image on background_surface --------\n");

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/images/background.jpg",
                                        &provider ));

    DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));

    back_rect.w = dsc.width;
    back_rect.h = dsc.height;
    fprintf(stderr, "back_rect, x[%d], y[%d], w[%d], h[%d]", back_rect.x, back_rect.y, back_rect.w, back_rect.h);

    DFBCHECK(dfb->CreateSurface( dfb, &dsc, &background_surface ));

    DFBCHECK(provider->RenderTo( provider, background_surface, NULL ));
    provider->Release( provider );

    D_INFO("-------- loading image on yellow_surface --------\n");

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/images/yellow.png",
                                        &provider ));

    DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));

    yellow_rect.w = dsc.width;
    yellow_rect.h = dsc.height;
    fprintf(stderr, "yellow_rect, x[%d], y[%d], w[%d], h[%d]", yellow_rect.x, yellow_rect.y, yellow_rect.w, yellow_rect.h);

    DFBCHECK(dfb->CreateSurface( dfb, &dsc, &yellow_surface ));

    DFBCHECK(provider->RenderTo( provider, yellow_surface, NULL ));
    provider->Release( provider );

    D_INFO("-------- loading image on blue_surface --------\n");

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/images/blue.png",
                                       &provider ));

    DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
    blue_rect.w = dsc.width;
    blue_rect.h = dsc.height;
    fprintf(stderr, "blue_rect, x[%d], y[%d], w[%d], h[%d]", blue_rect.x, blue_rect.y, blue_rect.w, blue_rect.h);

    DFBCHECK(dfb->CreateSurface( dfb, &dsc, &blue_surface ));

    DFBCHECK(provider->RenderTo( provider, blue_surface, NULL ));
    provider->Release( provider );

    /* Getting 3 layers and their surfaces */
    D_INFO("-------- bdvd_dfb_ext_layer_open --------\n");
    D_INFO("-------- bdvd_dfb_ext_layer_open --------\n");
    D_INFO("-------- bdvd_dfb_ext_layer_open --------\n");

    graphics_dfb_ext[0] = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_presentation_layer);
    if (graphics_dfb_ext == NULL)
    {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("-------- bdvd_dfb_ext_layer_get --------\n");
    D_INFO("-------- bdvd_dfb_ext_layer_get --------\n");
    D_INFO("-------- bdvd_dfb_ext_layer_get --------\n");

    if (bdvd_dfb_ext_layer_get(graphics_dfb_ext[0], &graphics_dfb_ext_settings) != bdvd_ok)
    {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("-------- GetDisplayLayer --------\n");
    D_INFO("-------- GetDisplayLayer --------\n");
    D_INFO("-------- GetDisplayLayer --------\n");

    if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &generic_layers[0]) != DFB_OK)
    {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("-------- SetCooperativeLevel --------\n");
    D_INFO("-------- SetCooperativeLevel --------\n");
    D_INFO("-------- SetCooperativeLevel --------\n");


    DFBCHECK( generic_layers[0]->SetCooperativeLevel(generic_layers[0], DLSCL_ADMINISTRATIVE) );

    D_INFO("-------- SetBackgroundColor --------\n");
    D_INFO("-------- SetBackgroundColor --------\n");
    D_INFO("-------- SetBackgroundColor --------\n");

    DFBCHECK( generic_layers[0]->SetBackgroundColor(generic_layers[0], 0, 0, 0, 0 ) );

    D_INFO("-------- SetBackgroundMode --------\n");
    D_INFO("-------- SetBackgroundMode --------\n");
    D_INFO("-------- SetBackgroundMode --------\n");

    DFBCHECK( generic_layers[0]->SetBackgroundMode(generic_layers[0], DLBM_COLOR ) );

    D_INFO("-------- GetSurface --------\n");
    D_INFO("-------- GetSurface --------\n");
    D_INFO("-------- GetSurface --------\n");

    DFBCHECK( generic_layers[0]->GetSurface(generic_layers[0], &layer_surfaces[0]) );

    /* Clear does also a flip, and we don't want that at this point */
    /*  DFBCHECK( layer_surfaces[0]->Clear(layer_surfaces[0], 0, 0, 0, 0) ); */

    D_INFO("-------- GetSize --------\n");
    D_INFO("-------- GetSize --------\n");
    D_INFO("-------- GetSize --------\n");

    DFBCHECK( layer_surfaces[0]->GetSize( layer_surfaces[0], &width, &height) );
    D_INFO("layer_surface w=%d, h=%d\n", width, height);
    D_INFO("-------- SetColor --------\n");
    D_INFO("-------- SetColor --------\n");
    D_INFO("-------- SetColor --------\n");

    DFBCHECK( layer_surfaces[0]->SetColor( layer_surfaces[0], 0, 0, 0, 0) );

    D_INFO("-------- FillRectangle --------\n");
    D_INFO("-------- FillRectangle --------\n");
    D_INFO("-------- FillRectangle --------\n");

    DFBCHECK( layer_surfaces[0]->FillRectangle( layer_surfaces[0], 0, 0, width, height) );


    D_INFO("-------- Getting layer 2 --------\n");

    graphics_dfb_ext[1] = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_interactive_layer);
    if (graphics_dfb_ext == NULL)
    {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (bdvd_dfb_ext_layer_get(graphics_dfb_ext[1], &graphics_dfb_ext_settings) != bdvd_ok)
    {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("On layer 1 the id is %d\n", graphics_dfb_ext_settings.id);

    if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &generic_layers[1]) != DFB_OK)
    {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_DEBUG("-------- Setting cooperative level for layer 2 --------\n");

    DFBCHECK( generic_layers[1]->SetCooperativeLevel(generic_layers[1], DLSCL_ADMINISTRATIVE) );

    D_DEBUG("-------- Setting background color for layer 2 --------\n");

    DFBCHECK( generic_layers[1]->SetBackgroundColor(generic_layers[1], 0, 0, 0, 0 ) );
    DFBCHECK( generic_layers[1]->SetBackgroundMode(generic_layers[1], DLBM_COLOR ) );

    D_DEBUG("-------- Getting layer 2 surface --------\n");

    DFBCHECK( generic_layers[1]->GetSurface(generic_layers[1], &layer_surfaces[1]) );

    D_DEBUG("-------- Filling layer 2 surface with black --------\n");

    DFBCHECK( layer_surfaces[1]->GetSize( layer_surfaces[1], &width, &height) );
    DFBCHECK( layer_surfaces[1]->SetColor( layer_surfaces[1], 0, 0, 0, 0) );
    DFBCHECK( layer_surfaces[1]->FillRectangle( layer_surfaces[1], 0, 0, width, height) );

    D_INFO("-------- Getting layer 3 --------\n");

    graphics_dfb_ext[2] = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_osd_layer);
    if (graphics_dfb_ext == NULL)
    {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (bdvd_dfb_ext_layer_get(graphics_dfb_ext[2], &graphics_dfb_ext_settings) != bdvd_ok)
    {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("On layer 1 the id is %d\n", graphics_dfb_ext_settings.id);

    if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &generic_layers[2]) != DFB_OK)
    {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_DEBUG("-------- Setting cooperative level for layer 3 --------\n");

    DFBCHECK( generic_layers[2]->SetCooperativeLevel(generic_layers[2], DLSCL_ADMINISTRATIVE) );

    D_DEBUG("-------- Setting background color for layer 3 --------\n");

    DFBCHECK( generic_layers[2]->SetBackgroundColor(generic_layers[2], 0, 0, 0, 0 ) );
    DFBCHECK( generic_layers[2]->SetBackgroundMode(generic_layers[2], DLBM_COLOR ) );

    D_DEBUG("-------- Getting layer 3 surface --------\n");

    DFBCHECK( generic_layers[2]->GetSurface(generic_layers[2], &layer_surfaces[2]) );

    D_DEBUG("-------- Filling layer 3 surface with black --------\n");

    DFBCHECK( layer_surfaces[2]->GetSize( layer_surfaces[2], &width, &height) );
    DFBCHECK( layer_surfaces[2]->SetColor( layer_surfaces[2], 0, 0, 0, 0) );
    DFBCHECK( layer_surfaces[2]->FillRectangle( layer_surfaces[2], 0, 0, width, height) );

    xoff = 0;
    yoff = 0;

    D_DEBUG("-------- Blitting background_surface onto layer 1 surface --------\n");

    if (show_background) {

        D_INFO("-------- SetBlittingFlags --------\n");
        D_INFO("-------- SetBlittingFlags --------\n");
        D_INFO("-------- SetBlittingFlags --------\n");

        DFBCHECK( layer_surfaces[0]->SetBlittingFlags(
                        layer_surfaces[0],
                        DSBLIT_NOFX ) );


        D_INFO("-------- Blit --------\n");
        D_INFO("-------- Blit --------\n");
        D_INFO("-------- Blit --------\n");

        DFBCHECK( layer_surfaces[0]->Blit(
                        layer_surfaces[0],
                        background_surface,
                        &back_rect,//whole rect
                        xoff,yoff) );

        D_INFO("-------- SetLevel --------\n");
        D_INFO("-------- SetLevel --------\n");
        D_INFO("-------- SetLevel --------\n");

        DFBCHECK( generic_layers[0]->SetLevel(generic_layers[0], 1) );
    }

    D_INFO("-------- Flip --------\n");
    D_INFO("-------- Flip --------\n");
    D_INFO("-------- Flip --------\n");

    DFBCHECK( layer_surfaces[0]->Flip( layer_surfaces[0], NULL, 0 ) );

    getchar();

    D_INFO("-------- Blitting yellow_surface onto layer 2 surface --------\n");

    DFBCHECK( layer_surfaces[1]->SetBlittingFlags(
                        layer_surfaces[1],
                        DSBLIT_NOFX ) );

    DFBCHECK( layer_surfaces[1]->Blit(
                        layer_surfaces[1],
                        yellow_surface,
                        &yellow_rect,//whole rect
                        xoff,yoff) );

    D_INFO("-------- Setting level for layer 2 --------\n");

    DFBCHECK( generic_layers[1]->SetLevel(generic_layers[1], 2) );

    D_INFO("-------- Flipping from layer 2 surface --------\n");

    DFBCHECK( layer_surfaces[1]->Flip( layer_surfaces[1], NULL, 0 ) );

    getchar();

    D_INFO("-------- Blitting blue_surface onto layer 3 surface --------\n");

    DFBCHECK( layer_surfaces[2]->SetBlittingFlags(
                        layer_surfaces[2],
                        DSBLIT_NOFX ) );

    DFBCHECK( layer_surfaces[2]->Blit(
                        layer_surfaces[2],
                        blue_surface,
                        &blue_rect,//whole rect
                        xoff,yoff) );

    D_DEBUG("-------- Setting level for layer 3 --------\n");

    DFBCHECK( generic_layers[2]->SetLevel(generic_layers[2], 3) );

    D_DEBUG("-------- Flipping from layer 3 surface --------\n");

    DFBCHECK( layer_surfaces[2]->Flip( layer_surfaces[2], NULL, 0 ) );

    getchar();

    printf("Getting out\n");

    {
        background_surface->Release(background_surface);
        yellow_surface->Release(yellow_surface);
        blue_surface->Release(blue_surface);
        generic_layers[0]->SetLevel(generic_layers[0], -1);
        generic_layers[1]->SetLevel(generic_layers[1], -1);
        generic_layers[2]->SetLevel(generic_layers[2], -1);
        layer_surfaces[0]->Release(layer_surfaces[0]);
        layer_surfaces[1]->Release(layer_surfaces[1]);
        layer_surfaces[2]->Release(layer_surfaces[2]);
        generic_layers[0]->Release(generic_layers[0]);
        generic_layers[1]->Release(generic_layers[1]);
        generic_layers[2]->Release(generic_layers[2]);
    }

    if (graphics_dfb_ext[0]) {
        if (bdvd_dfb_ext_layer_close(graphics_dfb_ext[0]) != bdvd_ok) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }

    if (graphics_dfb_ext[1]) {
        if (bdvd_dfb_ext_layer_close(graphics_dfb_ext[1]) != bdvd_ok) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }

    if (graphics_dfb_ext[2]) {
        if (bdvd_dfb_ext_layer_close(graphics_dfb_ext[2]) != bdvd_ok) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }

    printf("calling bdvd_dfb_ext\n");

    if (bdvd_dfb_ext_close() != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_close failed\n");
        ret = EXIT_FAILURE;
    }

    printf("bdvd_dfb_ext done\n");

    if (bdisplay) bdvd_display_close(bdisplay);
    printf("bdvd_display_close done\n");
    bdvd_uninit();

    printf("bdvd_uninit done\n");

out:

    return ret;
}

#endif