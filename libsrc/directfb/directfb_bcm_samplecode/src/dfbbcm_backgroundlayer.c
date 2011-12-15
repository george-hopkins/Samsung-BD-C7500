
#include <stdio.h>
#include <directfb.h>
#include <bdvd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <direct/debug.h>

#include "dfbbcm_playbackscagatthread.h"

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;

    IDirectFB *dfb = NULL;

    bdvd_dfb_ext_h background_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t background_dfb_ext_settings;
    IDirectFBDisplayLayer * background_layer = NULL;
    DFBDisplayLayerConfig background_layer_config;
    IDirectFBSurface * background_surface = NULL;

    bdvd_dfb_ext_h graphics_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;
    IDirectFBDisplayLayer * graphics_layer = NULL;
    IDirectFBSurface * graphics_surface = NULL;

    IDirectFBImageProvider * provider;
    IDirectFBSurface * image_surface = NULL;
    DFBSurfaceDescription image_surface_desc;

    bdvd_decode_window_settings_t window_settings;
    playback_window_settings_flags_e window_settings_flags;

    uint32_t total_move = 0xFF;

    uint32_t current_time;
    uint32_t previous_time;

    char c;

    /* SetScreenLocation uses normalized values */
    float norm_width = 0.5;
    float norm_height = 0.5;
    float xpos = 0.5;
    float ypos = 0.1;

    char graphics_image_name[80];
    char background_image_name[80];

    bool stop_input_event = false;

    uint32_t backlevel = -1;
    bdvd_result_e retCode;

    bdvd_display_settings_t display_settings;

    bdvd_decode_window_h window, window1; /* use to display background images */
    bdvd_decode_window_settings_t decodeWindowSettings;

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
#if 1
    bdvd_display_get(bdisplay, &display_settings);

    display_settings.format = bdvd_video_format_1080i;
    display_settings.svideo = NULL;
    display_settings.composite = NULL;
    display_settings.rf = NULL;


    if (bdvd_display_set(bdisplay, &display_settings) != bdvd_ok) {
        printf("bdvd_display_set failed\n");
        return bdvd_err_unspecified;
    }
#else
    display_settings.format = bdvd_video_format_ntsc;
#endif

#if 1
    window1 = bdvd_decode_window_open(B_ID(0), bdisplay);
        if (window == NULL) {
            printf("bdvd_decode_window_open failed\n");
            return bdvd_err_unspecified;
    }

  if (bdvd_decode_window_get(window1, &decodeWindowSettings) != bdvd_ok) {
    printf("bdvd_decode_window_get failed\n");
    return bdvd_err_unspecified;
  }

  /* make secondary B_ID(1) visible */

  decodeWindowSettings.zorder = 1;
  decodeWindowSettings.alpha_value = 0x00;

decodeWindowSettings.deinterlacer = false;

    if (bdvd_decode_window_set(window1, &decodeWindowSettings) != bdvd_ok) {
        printf("bdvd_decode_window_get failed\n");
        return bdvd_err_unspecified;
    }
    printf("open first display: z = %d\n", decodeWindowSettings.zorder);
#endif
    window = bdvd_decode_window_open(B_ID(1), bdisplay);
    if (window == NULL) {
        printf("bdvd_decode_window_open failed\n");
        return bdvd_err_unspecified;
    }

  if (bdvd_decode_window_get(window, &decodeWindowSettings) != bdvd_ok) {
    printf("bdvd_decode_window_get failed\n");
    return bdvd_err_unspecified;
  }

  decodeWindowSettings.zorder = 0;
decodeWindowSettings.deinterlacer = true;


    if (bdvd_decode_window_set(window, &decodeWindowSettings) != bdvd_ok) {
        printf("bdvd_decode_window_get failed\n");
        return bdvd_err_unspecified;
    }

    printf("open second display: z = %d\n", decodeWindowSettings.zorder);

    {
        int i;

        for (i = 0; i < 0; i++)
        {
            printf("test number %d\n", i);
            retCode = bdvd_decode_set_still_display_mode(window, ((i&1) ? true : false));
            if (retCode)
            {
                printf("bdvd_decode_set_still_display_mode returned with errors\n");
            }
        }
    }

#if 1
    retCode = bdvd_decode_set_still_display_mode(window, true);
    if (retCode)
    {
        printf("bdvd_decode_set_still_display_mode returned with errors\n");
    }
#endif

    D_INFO("Press any key to setup background layer\n");
    getchar();


    /* bdvd_init end */

    if (argc < 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    memset(&background_layer_config, 0, sizeof(background_layer_config));
    memset(graphics_image_name, 0, sizeof(graphics_image_name));
    memset(background_image_name, 0, sizeof(background_image_name));

    while ((c = getopt (argc, argv, "g:b:s")) != -1) {
        switch (c) {
        case 'g':
            snprintf(graphics_image_name, sizeof(graphics_image_name)/sizeof(graphics_image_name[0]), "%s", optarg);
        break;
        case 'b':
            snprintf(background_image_name, sizeof(background_image_name)/sizeof(background_image_name[0]), "%s", optarg);
        break;
        case 's':
            backlevel = 1;
        break;
        case '?':
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
            goto out;
        break;
        }
    }

    if (strlen(background_image_name) == 0) {
        D_ERROR("Background image file name must not be empty\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    dfb = bdvd_dfb_ext_open_with_params(2, &argc, &argv);
    if (dfb == NULL) {
        D_ERROR("bdvd_dfb_ext_open_with_params failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (display_settings.format == bdvd_video_format_1080i)
        dfb->SetVideoMode(dfb, 1920, 1080, 32);

#if 0

    window_settings_flags = playback_window_settings_position | playback_window_settings_zorder;
    window_settings.position.x = 720/4;
    window_settings.position.y = 480/4;
    window_settings.position.width = 720/2;
    window_settings.position.height = 480/2;
    window_settings.zorder = 1;

    playbackscagat_start_with_settings(bdisplay, window_settings_flags, &window_settings, NULL, NULL, NULL, argc, argv);

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
    if (graphics_layer->SetLevel(graphics_layer, backlevel == -1 ? 1 : 2) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_layer->SetScreenLocation(graphics_layer, xpos, ypos, norm_width, norm_height) != DFB_OK) {
        D_ERROR("SetScreenLocation failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* End of graphics layer setup */

    /* Graphics image render (optionnal) */
    if (strlen(graphics_image_name) != 0) {
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
        if (provider->RenderTo(provider, image_surface, NULL) != DFB_OK) {
            D_ERROR("RenderTo failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        if (graphics_layer->GetSurface(graphics_layer, &graphics_surface) != DFB_OK) {
            D_ERROR("GetSurface failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        if (graphics_surface->SetBlittingFlags(graphics_surface, DSBLIT_NOFX) != DFB_OK) {
            D_ERROR("SetBlittingFlags failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        if (graphics_surface->StretchBlit(graphics_surface, image_surface, NULL, NULL) != DFB_OK) {
            D_ERROR("StretchBlit failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        if (graphics_surface->Flip(graphics_surface, NULL, DSFLIP_NONE) != DFB_OK) {
            D_ERROR("Flip failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        if (image_surface->Release(image_surface) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
        image_surface = NULL;
    }
    /* End of graphics image render */

    D_INFO("Press any key to setup background layer\n");
    getchar();
#endif

    D_INFO("bdvd_dfb_ext_layer_open\n");
    /* Background layer setup */
    background_dfb_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_j_background_layer);
    if (background_dfb_ext == NULL) {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("bdvd_dfb_ext_layer_get\n");
    if (bdvd_dfb_ext_layer_get(background_dfb_ext, &background_dfb_ext_settings) != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("GetDisplayLayer\n");
    if (dfb->GetDisplayLayer(dfb, background_dfb_ext_settings.id, &background_layer) != DFB_OK) {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("SetCooperativeLevel\n");
    if (background_layer->SetCooperativeLevel(background_layer, DLSCL_ADMINISTRATIVE) != DFB_OK) {
        D_ERROR("SetCooperativeLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* End of background layer setup */
    if (display_settings.format == bdvd_video_format_1080i)
    {
        /* Default pixel format is DSPF_UYVY */
        background_layer_config.flags = DLCONF_WIDTH | DLCONF_HEIGHT;
        background_layer_config.width = 720;
        background_layer_config.height = 480;

        D_INFO("SetConfiguration\n");
        if (background_layer->SetConfiguration(background_layer, &background_layer_config) != DFB_OK) {
            D_ERROR("SetConfiguration failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
    }

    D_INFO("CreateImageProvider\n");
    /* Background image render */
    if (dfb->CreateImageProvider(dfb, background_image_name, &provider) != DFB_OK) {
        D_ERROR("CreateImageProvider failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("GetSurfaceDescription\n");
    if (provider->GetSurfaceDescription(provider, &image_surface_desc) != DFB_OK) {
        D_ERROR("GetSurfaceDescription failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("CreateSurface\n");
    if (dfb->CreateSurface(dfb, &image_surface_desc, &image_surface) != DFB_OK) {
        D_ERROR("CreateSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("RenderTo\n");
    if (provider->RenderTo(provider, image_surface, NULL) != DFB_OK) {
        D_ERROR("RenderTo failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("GetSurface\n");
    if (background_layer->GetSurface(background_layer, &background_surface) != DFB_OK) {
        D_ERROR("GetSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    D_INFO("SetBlittingFlags\n");
    if (background_surface->SetBlittingFlags(background_surface, DSBLIT_NOFX) != DFB_OK) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("StretchBlit\n");
#if 1
    if (background_surface->StretchBlit(background_surface, image_surface, NULL, NULL) != DFB_OK) {
#else
    if (background_surface->Blit(background_surface, image_surface, NULL, 0, 0) != DFB_OK) {
#endif
        D_ERROR("StretchBlit failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("SetLevel\n");
    if (background_layer->SetLevel(background_layer, 1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (background_surface->Flip(background_surface, NULL, DSFLIP_NONE) != DFB_OK) {
        D_ERROR("Flip failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("Press any key to continue\n");
    getchar();

    if (background_layer->SetLevel(background_layer, -1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("Layer should now not be visible\n");
    getchar();

    if (background_layer->SetLevel(background_layer, 1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("Layer should now be visible\n");
    getchar();

    if (background_layer->SetLevel(background_layer, -1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }


    D_INFO("Layer should now not be visible\n");
    getchar();

    if (image_surface->Release(image_surface) != DFB_OK) {
        D_ERROR("Release failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    image_surface = NULL;
    /* End of background image render */

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
    if (background_layer) {
        if (background_layer->SetLevel(background_layer, -1) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (graphics_layer) {
        if (graphics_layer->SetLevel(graphics_layer, -1) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (background_surface) {
        if (background_surface->Release(background_surface) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (graphics_surface) {
        if (graphics_surface->Release(graphics_surface) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (background_layer) {
        if (background_layer->Release(background_layer) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (graphics_layer) {
        if (graphics_layer->Release(graphics_layer) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (background_dfb_ext) {
        if (bdvd_dfb_ext_layer_close(background_dfb_ext) != bdvd_ok) {
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

    return EXIT_SUCCESS;
}
