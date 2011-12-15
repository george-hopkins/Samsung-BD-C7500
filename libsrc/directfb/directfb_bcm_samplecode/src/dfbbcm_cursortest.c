
#include <stdio.h>
#include <directfb.h>
#include <bdvd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <direct/debug.h>

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;

    IDirectFB *dfb = NULL;

    bdvd_dfb_ext_h cursor_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t cursor_dfb_ext_settings;
    IDirectFBDisplayLayer * cursor_layer = NULL;
    DFBDisplayLayerConfig cursor_layer_config;
    IDirectFBSurface * cursor_surface = NULL;

    bdvd_dfb_ext_h graphics_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;
    IDirectFBDisplayLayer * graphics_layer = NULL;
    IDirectFBSurface * graphics_surface = NULL;

    IDirectFBImageProvider * provider;
    IDirectFBSurface * image_surface = NULL;
    DFBSurfaceDescription image_surface_desc;

    uint32_t total_move = 0xFF;

    uint32_t current_time;
    uint32_t previous_time;

    char c;
    
    /* SetScreenLocation uses normalized values */
    /* Those dimensions do not reflect a requirement
     * from the spec, do not use as a reference.
     */
    float norm_width = 0.2;
    float norm_height = 0.2;
    float xpos = (1-norm_width)/2;
    float ypos = (1-norm_height)/2;

    char graphics_image_name[80];
    char cursor_image_name[80];
    
    bool stop_input_event = false;

    /* bdvd_init begin */
    bdvd_display_h bdisplay = NULL;
    if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
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

    if (argc < 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    memset(&cursor_layer_config, 0, sizeof(cursor_layer_config));
    memset(graphics_image_name, 0, sizeof(graphics_image_name));
    memset(cursor_image_name, 0, sizeof(cursor_image_name));

    while ((c = getopt (argc, argv, "g:c:")) != -1) {
        switch (c) {
        case 'g':
            snprintf(graphics_image_name, sizeof(graphics_image_name)/sizeof(graphics_image_name[0]), "%s", optarg);
        break;
        case 'c':
            snprintf(cursor_image_name, sizeof(cursor_image_name)/sizeof(cursor_image_name[0]), "%s", optarg);
        break;
        case '?':
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
            goto out;
        break;
        }
    }

    if (strlen(cursor_image_name) == 0) {
        D_ERROR("Cursor image file name must not be empty\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    dfb = bdvd_dfb_ext_open_with_params(2, &argc, &argv);
    if (dfb == NULL) {
        D_ERROR("bdvd_dfb_ext_open_with_params failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Graphics layer setup. Must be present for the cursor layer to operate */
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

    /* Cursor layer setup */
    cursor_dfb_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_hddvd_ac_cursor_layer);
    if (cursor_dfb_ext == NULL) {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (bdvd_dfb_ext_layer_get(cursor_dfb_ext, &cursor_dfb_ext_settings) != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (dfb->GetDisplayLayer(dfb, cursor_dfb_ext_settings.id, &cursor_layer) != DFB_OK) {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (cursor_layer->SetCooperativeLevel(cursor_layer, DLSCL_ADMINISTRATIVE) != DFB_OK) {
        D_ERROR("SetCooperativeLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (cursor_layer->SetLevel(cursor_layer, 2) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* End of cursor layer setup */

    /* HD DVD spec mentions the layer is 256x256. Default pixel format is ARGB */
    cursor_layer_config.flags = DLCONF_WIDTH | DLCONF_HEIGHT;
    cursor_layer_config.width = 256;
    cursor_layer_config.height = 256;

    if (cursor_layer->SetConfiguration(cursor_layer, &cursor_layer_config) != DFB_OK) {
        D_ERROR("SetConfiguration failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    
    /* Cursor image render */
    if (dfb->CreateImageProvider(dfb, cursor_image_name, &provider) != DFB_OK) {
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
    if (cursor_layer->GetSurface(cursor_layer, &cursor_surface) != DFB_OK) {
        D_ERROR("GetSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (cursor_surface->SetBlittingFlags(cursor_surface, DSBLIT_NOFX) != DFB_OK) {
        D_ERROR("SetBlittingFlags failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (cursor_surface->StretchBlit(cursor_surface, image_surface, NULL, NULL) != DFB_OK) {
        D_ERROR("StretchBlit failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (image_surface->Release(image_surface) != DFB_OK) {
        D_ERROR("Release failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    image_surface = NULL;
    /* End of cursor image render */

    if (cursor_layer->SetScreenLocation(cursor_layer, xpos, ypos, norm_width, norm_height) != DFB_OK) {
        D_ERROR("SetScreenLocation failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("Press q to quit\n"
           "Press a for left\n"
           "Press s for right\n"
           "Press w for up\n"
           "Press z for down\n"
           "getchar is used, press enter to apply key sequence\n"
           "i.e. Only when enter is pressed that the key action\n"
           "are applied, you can do multiple keys aaaaasssswwwzzz\n"
           "TODO do better termios with stdin\n");

    while(!stop_input_event) {
        switch (getchar()) {
            case 'q':
                stop_input_event = true;
            break;
            case 'a':
                xpos -= 0.1;
                xpos = xpos >= 0 ? xpos : 0;
                if (cursor_layer->SetScreenLocation(cursor_layer, xpos, ypos, norm_width, norm_height) != DFB_OK) {
                    D_ERROR("SetScreenLocation failed\n");
                    ret = EXIT_FAILURE;
                    goto out;
                }
            break;
            case 's':
                xpos += 0.1;
                xpos = xpos < 1 ? xpos : 1;
                if (cursor_layer->SetScreenLocation(cursor_layer, xpos, ypos, norm_width, norm_height) != DFB_OK) {
                    D_ERROR("SetScreenLocation failed\n");
                    ret = EXIT_FAILURE;
                    goto out;
                }
            break;
            case 'w':
                ypos -= 0.1;
                ypos = ypos >= 0 ? ypos : 0;
                if (cursor_layer->SetScreenLocation(cursor_layer, xpos, ypos, norm_width, norm_height) != DFB_OK) {
                    D_ERROR("SetScreenLocation failed\n");
                    ret = EXIT_FAILURE;
                    goto out;
                }
            break;
            case 'z':
                ypos += 0.1;
                ypos = ypos < 1 ? ypos : 1;
                if (cursor_layer->SetScreenLocation(cursor_layer, xpos, ypos, norm_width, norm_height) != DFB_OK) {
                    D_ERROR("SetScreenLocation failed\n");
                    ret = EXIT_FAILURE;
                    goto out;
                }
            break;
            default:
                /* get new event */
            break;
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
    if (cursor_layer) {
        if (cursor_layer->SetLevel(cursor_layer, -1) != DFB_OK) {
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
    if (cursor_surface) {
        if (cursor_surface->Release(cursor_surface) != DFB_OK) {
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
    if (cursor_layer) {
        if (cursor_layer->Release(cursor_layer) != DFB_OK) {
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
    if (cursor_dfb_ext) {
        if (bdvd_dfb_ext_layer_close(cursor_dfb_ext) != bdvd_ok) {
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

    /* bdvd_uninit begin */
    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();
    /* bdvd_uninit end */

    return EXIT_SUCCESS;
}
