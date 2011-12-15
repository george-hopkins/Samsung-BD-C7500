
#include <stdio.h>
#include <directfb.h>

#define BCMPLATFORM

#ifdef BCMPLATFORM
#include <bdvd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <direct/debug.h>
#include <direct/clock.h>
#include <directfb-internal/misc/util.h>

#ifdef BCMPLATFORM
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_utils.h"
#endif

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

/* This section should be kept as reference for implementation of
 * 1/2 and 1/4 rendering.
 */

/* BEGINNING OF RENDER/SCALE SECTION */

/* Only one instance of the intermediate surface should exist
 * and be used by all threads calling render_convert_image.
 */
static struct {
    pthread_mutex_t mutex;
    IDirectFBSurface * surface;
} intermediate_surface_context = {PTHREAD_MUTEX_INITIALIZER, NULL};

typedef enum {
    IMAGESCALING_NONE       = 0x00000000,
    IMAGESCALING_HALFWIDTH  = 0x00000001,
    IMAGESCALING_HALFHEIGHT = 0x00000002
} ImageScalingFlags;

/* If ImageScalingFlags are set to something else then IMAGESCALING_NONE
 * render_convert_image will create or reformat an intermediate surface
 * and use the StretchBlit call (uses the M2MC) to scale down the decoded
 * image into a final surface. The surface returned by ret_surface must be
 * released by the application like an surface that would be returned by
 * CreateSurface.
 */
DFBResult render_convert_image(
            IDirectFB * dfb,
            IDirectFBImageProvider * provider,
            ImageScalingFlags flags,
            DFBSurfacePixelFormat format,
            IDirectFBSurface ** ret_surface) {
    DFBResult ret = DFB_OK;
    int rc;
    IDirectFBSurface * surface = NULL;
    IDirectFBSurface * temp_surface = NULL;
    DFBSurfaceDescription image_surface_desc;
    DFBRectangle render_rect;

    if ((ret = provider->GetSurfaceDescription(provider, &image_surface_desc)) != DFB_OK) {
        D_DERROR(ret, "GetSurfaceDescription failed\n");
        return ret;
    }

    /* GetSurfaceDescription should at least have width, height and pixel format */
    if (!(image_surface_desc.flags & DSDESC_WIDTH) ||
        !(image_surface_desc.flags & DSDESC_WIDTH) ||
        ((format != DSPF_UNKNOWN) && !(image_surface_desc.flags & DSDESC_PIXELFORMAT))) {
        D_ERROR("missing DFBSurfaceDescriptionFlags\n");
        return DFB_FAILURE;
    }

    render_rect = (DFBRectangle){0,0,image_surface_desc.width,image_surface_desc.height};

    /* Always force surfaces in video memory to avoid cost of the initial
     * swapping from system to video or the use of the software rasterizer.
     */
    if (image_surface_desc.flags & DSDESC_CAPS) {
        image_surface_desc.caps |= (DFBSurfaceCapabilities)(DSCAPS_VIDEOONLY);
    }
    else {
        image_surface_desc.flags |= (DFBSurfaceDescriptionFlags)(DSDESC_CAPS);
        image_surface_desc.caps = DSCAPS_VIDEOONLY;
    }

    /* No goto before this */
    if ((rc = pthread_mutex_lock(&intermediate_surface_context.mutex)) != 0) {
        D_ERROR("pthread_mutex_lock failed    --> %s\n", strerror(rc));
        return DFB_FAILURE;
    }

    if (flags != IMAGESCALING_NONE) {
        static int max_width = 1920;
        static int max_height = 1080;

        /* This goes beyond the supported max size, let's create a new and disposable surface */
        if ((image_surface_desc.width > max_width) && (image_surface_desc.width > max_height)) {
            if ((ret = dfb->CreateSurface(dfb, &image_surface_desc, &temp_surface)) != DFB_OK) {
                D_DERROR(ret, "CreateSurface failed\n");
                goto out;
            }
        }
        else {
            /* The intermediate surface is set to larger size 1080i/p and reused,
             * this to avoid video memory fragmentation. If surfaces were allocated then
             * deallocated, this would create a lot of fragmentation. A 8MB buffer is kept and
             * released only when render_convert_release is called.
             */
            if (intermediate_surface_context.surface) {
                DFBSurfacePixelFormat pixelformat;

                if ((ret = intermediate_surface_context.surface->GetPixelFormat(intermediate_surface_context.surface, &pixelformat)) != DFB_OK) {
                    D_DERROR(ret, "GetPixelFormat failed\n");
                    goto out;
                }

                if (image_surface_desc.pixelformat != pixelformat) {
                    intermediate_surface_context.surface->Release(intermediate_surface_context.surface);
                    intermediate_surface_context.surface = NULL;
                }
            }

            if (!intermediate_surface_context.surface) {
                DFBSurfaceDescription temp_surface_desc = image_surface_desc;
                /* Create to a fixed byte size of 8MB to avoid creating to many holes
                 * in the video memory. The previously released surface should have been the
                 * same size, thus the location in memory will be reused.
                 */
                temp_surface_desc.width = max_width*4/DFB_BYTES_PER_PIXEL(image_surface_desc.pixelformat);
                temp_surface_desc.height = max_height;

                if ((ret = dfb->CreateSurface(dfb, &temp_surface_desc, &intermediate_surface_context.surface)) != DFB_OK) {
                    D_DERROR(ret, "CreateSurface failed\n");
                    goto out;
                }
            }
        }

        if ((ret = provider->RenderTo(provider, temp_surface ? temp_surface : intermediate_surface_context.surface, &render_rect)) != DFB_OK) {
            D_DERROR(ret, "RenderTo failed\n");
            goto out;
        }

        if (flags & IMAGESCALING_HALFWIDTH) {
            image_surface_desc.width /= 2;
        }
        if (flags & IMAGESCALING_HALFHEIGHT) {
            image_surface_desc.height /= 2;
        }

        image_surface_desc.pixelformat = (format == DSPF_UNKNOWN) ? image_surface_desc.pixelformat : format;

        if ((ret = dfb->CreateSurface(dfb, &image_surface_desc, &surface)) != DFB_OK) {
            D_DERROR(ret, "CreateSurface failed\n");
            goto out;
        }

        if ((ret = surface->StretchBlit(surface, temp_surface ? temp_surface : intermediate_surface_context.surface, &render_rect, NULL)) != DFB_OK) {
            D_DERROR(ret, "StretchBlit failed\n");
            goto out;
        }
    }
    else {
        /* Later add a colorspace conversion using a Blit */
        image_surface_desc.pixelformat = (format == DSPF_UNKNOWN) ? image_surface_desc.pixelformat : format;

        if ((ret = dfb->CreateSurface(dfb, &image_surface_desc, &surface)) != DFB_OK) {
            D_DERROR(ret, "CreateSurface failed\n");
            goto out;
        }

        if ((ret = provider->RenderTo(provider, surface, NULL)) != DFB_OK) {
            D_DERROR(ret, "RenderTo failed\n");
            goto out;
        }
    }

out:

    /* Unlock mutex */
    if ((rc = pthread_mutex_unlock(&intermediate_surface_context.mutex)) != 0) {
        D_ERROR("pthread_mutex_unlock failed    --> %s\n", strerror(rc));
        ret = DFB_FAILURE;
        /* Fall through */
    }

    if (temp_surface) {
        temp_surface->Release(temp_surface);
        temp_surface = NULL;
    }

    if (ret != DFB_OK) {
        if (surface) surface->Release(surface);
        *ret_surface = NULL;
    }
    else {
        *ret_surface = surface;
    }

    return ret;
}

/* The Render/Scale intermediate surface must be released at the end,
 * for example when the HD-DVD advanced navigator is exiting.
 */
DFBResult render_convert_release() {
    DFBResult ret = DFB_OK;
    int rc;

    /* No goto before this */
    if ((rc = pthread_mutex_trylock(&intermediate_surface_context.mutex)) != 0) {
        D_ERROR("pthread_mutex_trylock failed    --> %s\n", strerror(rc));
        D_ERROR("Intermediate surface mutex was still locked!\n");
        return DFB_FAILURE;
    }

    if (intermediate_surface_context.surface) {
        intermediate_surface_context.surface->Release(intermediate_surface_context.surface);
        intermediate_surface_context.surface = NULL;
    }

    /* Unlock mutex */
    if ((rc = pthread_mutex_unlock(&intermediate_surface_context.mutex)) != 0) {
        D_ERROR("pthread_mutex_unlock failed    --> %s\n", strerror(rc));
        ret = DFB_FAILURE;
        /* Fall through */
    }

    return ret;
}

/* END OF RENDER/SCALE SECTION */

#define MAX_STRING_LENGTH 256

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;
     DFBResult err;

    IDirectFB *dfb = NULL;

    const char frames_filenames[][MAX_STRING_LENGTH] = {DATADIR"/images/frame_mainmenu_settings.png", DATADIR"/images/frame_settingsmenu.png"};

    char * printed_string = "DOING FULL RES";

#ifdef BCMPLATFORM
    bdvd_dfb_ext_h graphics_dfb_ext = NULL;
    bdvd_dfb_ext_clear_rect_t clear_rects;
    bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;
    DFBDisplayLayerConfig   graphics_layer_config;
#endif
    IDirectFBDisplayLayer * graphics_layer = NULL;
    IDirectFBSurface * layer_surface = NULL;

    IDirectFBImageProvider * provider;
    IDirectFBSurface * frame_surfaces[sizeof(frames_filenames)/sizeof(frames_filenames[0][0])/MAX_STRING_LENGTH];
#ifdef BCMPLATFORM
    bdvd_decode_window_settings_t window_settings;
    playback_window_settings_flags_e window_settings_flags;
#else
    DFBSurfaceDescription layer_surface_desc;
#endif
    DFBSurfaceTextFlags text_flags = DSTF_TOPLEFT;
    ImageScalingFlags render_flags = IMAGESCALING_NONE;
    DFBSurfacePixelFormat pixel_format = DSPF_ARGB;
    bool partial_flip = true;

    DFBSurfaceCapabilities surface_caps;

    uint32_t nb_frame_sec = 30;

    int32_t i;
    uint32_t nb_frames_rendered;
    int advance_sign;
    uint32_t step_size = 42;

    char c;

    float height_convertion;
    DFBRectangle layer_surface_rect = {0,0,0,0};
    DFBRectangle mainmenu_rect;
    DFBRectangle fill_rect;
    DFBRectangle previous_submenu_rect;
    DFBRectangle current_submenu_rect;
    DFBRegion flip_region;

    DFBFontDescription fontdesc;
    IDirectFBFont *font;

    uint32_t start_time;
    uint32_t current_time;
    uint32_t previous_time;
    uint32_t nb_jump;

    uint32_t blit_ops;
    uint32_t blend_ops;
    uint32_t clear_ops;
    uint32_t flipblit_ops;
    uint32_t queued_ops_time;
    uint32_t queued_ops_cumul_time;
    uint32_t flip_cumul_time;

#ifdef BCMPLATFORM
     bvideo_format video_format = bvideo_format_1080i;

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
#endif

    if (argc < 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    memset(&frame_surfaces, 0, sizeof(frame_surfaces));

    while ((c = getopt (argc, argv, "hq2np")) != -1) {
        switch (c) {
        case 'h':
            text_flags |= (DFBSurfaceTextFlags)DSTF_HALFWIDTH;
            render_flags = IMAGESCALING_HALFWIDTH;
            printed_string = "DOING 1/2 RES";
        break;
        case 'q':
            text_flags |= (DFBSurfaceTextFlags)(DSTF_HALFWIDTH | DSTF_HALFHEIGHT);
            render_flags = (ImageScalingFlags)(IMAGESCALING_HALFWIDTH | IMAGESCALING_HALFHEIGHT);
            printed_string = "DOING 1/4 RES";
        break;
        case '2':
            pixel_format = DSPF_ARGB4444;
        break;
        case 'n':
            video_format = bvideo_format_ntsc;
        break;
        case 'p':
            partial_flip = false;
        break;
        case '?':
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
            goto out;
        break;
        }
    }

    window_settings_flags = playback_window_settings_none;

    playbackscagat_start_with_settings(bdisplay, window_settings_flags, &window_settings, NULL, NULL, NULL, argc, argv);

#ifdef BCMPLATFORM
    dfb = bdvd_dfb_ext_open_with_params(2, NULL /*&argc*/, NULL /*&argv*/);
    if (dfb == NULL) {
        D_ERROR("bdvd_dfb_ext_open_with_params failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (display_set_video_format(bdisplay, video_format) != DFB_OK) {
        D_ERROR("display_set_video_format failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    switch (video_format) {
        case bvideo_format_ntsc:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
        break;
        case bvideo_format_1080i:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
        break;
        default:
        break;
    }

    graphics_dfb_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_hddvd_subpicture_layer);
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

    /* Layer configuration */
    graphics_layer_config.flags = (DFBDisplayLayerConfigFlags)(DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT | DLCONF_SURFACE_CAPS);
    graphics_layer_config.width = 1920;
    graphics_layer_config.height = 1080;
    /* If we are doing either half or quarter width, must set the
     * layer dimensions accordingly.
     */
    if (render_flags & IMAGESCALING_HALFWIDTH) {
        graphics_layer_config.width /= 2;
        step_size /= 2;
    }
    if (render_flags & IMAGESCALING_HALFHEIGHT) {
        graphics_layer_config.height /= 2;
    }
    graphics_layer_config.pixelformat = pixel_format;
    graphics_layer_config.surface_caps = DSCAPS_PREMULTIPLIED;

    if (graphics_layer->SetConfiguration(graphics_layer, &graphics_layer_config) != DFB_OK) {
        D_ERROR("SetConfiguration failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->SetLevel(graphics_layer, 1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->GetSurface(graphics_layer, &layer_surface) != DFB_OK) {
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
    layer_surface_desc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;
    layer_surface_desc.caps = DSCAPS_PRIMARY | DSCAPS_PREMULTIPLIED;
    layer_surface_desc.width = 960;
    layer_surface_desc.height = 540;

    if (dfb->CreateSurface( dfb, &layer_surface_desc, &layer_surface ) != DFB_OK) {
        D_ERROR("CreateSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#endif
    /* End of graphics layer setup */
    if (layer_surface->GetSize(layer_surface, &layer_surface_rect.w, &layer_surface_rect.h) != DFB_OK) {
        D_ERROR("GetSize failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Image are predecoded for later composition, decoded images must be kept and
     * composited several time for the scale down of image to be efficient.
     */
    {
        uint32_t render_time;

        previous_time = myclock();

        for (i = 0; i < sizeof(frames_filenames)/sizeof(frames_filenames[0][0])/MAX_STRING_LENGTH; i++) {
            if (dfb->CreateImageProvider(dfb, frames_filenames[i], &provider) != DFB_OK) {
                D_ERROR("CreateImageProvider failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }
                if (render_convert_image(dfb, provider, render_flags, pixel_format, &frame_surfaces[i]) != DFB_OK) {
                    D_ERROR("render_convert_image failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }
                if (provider->Release(provider) != DFB_OK) {
                    D_ERROR("Release failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }
        }

        /* THIS IS NOT A NECESSARY STEP.
         * This is only to get an accurate reading of the time taken by the StretchBlit.
         */
        if (dfb->WaitIdle(dfb) != DFB_OK) {
            D_ERROR("WaitIdle failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }

        render_time = myclock() - previous_time;

        fprintf(stderr, "Total time spend decoding and converting size and/or pixel format: %u ms\n", render_time);
    }

    fontdesc.flags = DFDESC_HEIGHT;
    fontdesc.height = 44;

    if (dfb->CreateFont (dfb, "/usr/local/share/player/fonts/Helvetica.ttf", &fontdesc, &font) != DFB_OK) {
        D_WARN("MuktiNarrowBold.ttf not found, switching to msgothic.ttc\n");
        if (dfb->CreateFont (dfb, DATADIR"/fonts/msgothic.ttc", &fontdesc, &font) != DFB_OK) {
            D_ERROR("CreateFont failed\n");
            ret = DFB_FAILURE;
            goto out;
        }
    }

    {
        DFBRectangle surface_rect = {0,0,0,0};

        if (frame_surfaces[0]->GetSize( frame_surfaces[0], &surface_rect.w, &surface_rect.h) != DFB_OK) {
        D_ERROR("GetSize failed\n");
            ret = DFB_FAILURE;
        goto out;
    }

#ifdef BCMPLATFORM
        height_convertion = 1;
#else
        height_convertion = (float)height/(float)surface_rect.height;
#endif
        D_INFO("height_convertion %f\n", height_convertion);

        mainmenu_rect = (DFBRectangle){0, 0, height_convertion*surface_rect.w, height_convertion*surface_rect.h};

        if (frame_surfaces[1]->GetSize( frame_surfaces[1], &surface_rect.w, &surface_rect.h) != DFB_OK) {
            D_ERROR("GetSize failed\n");
            ret = DFB_FAILURE;
            goto out;
        }

        previous_submenu_rect = (DFBRectangle){0, 0, height_convertion*surface_rect.w, height_convertion*surface_rect.h};
        current_submenu_rect = previous_submenu_rect;
        fill_rect = previous_submenu_rect;
        dfb_rectangle_union ( &fill_rect, &mainmenu_rect );

        D_INFO("mainmenu_rect.x %d mainmenu_rect.y %d mainmenu_rect.w %d mainmenu_rect.h %d\n", mainmenu_rect.x, mainmenu_rect.y, mainmenu_rect.w, mainmenu_rect.h );
    }

    flip_region = (DFBRegion){mainmenu_rect.x, mainmenu_rect.y, mainmenu_rect.x+mainmenu_rect.w-1, mainmenu_rect.y+mainmenu_rect.h-1};

    /* End of graphics layer setup */
    if (layer_surface->StretchBlit(layer_surface, frame_surfaces[0], NULL, &mainmenu_rect) != DFB_OK) {
        D_ERROR("StretchBlit failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (layer_surface->Flip(layer_surface, partial_flip ? &flip_region : NULL, DSFLIP_NONE) != DFB_OK) {
        D_ERROR("Flip failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (layer_surface->SetPorterDuff(layer_surface, DSPD_SRC_OVER)) {
        D_ERROR("SetPorterDuff failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (layer_surface->SetFont(layer_surface, font)) {
        D_ERROR("SetFont failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    c = 'm';

    clear_rects.rects[0].rect.x =100;
    clear_rects.rects[0].rect.y =100;
    clear_rects.rects[0].rect.w =200;
    clear_rects.rects[0].rect.h =200;

    while (c != 's') {
        c = fgetc(stdin);
        switch (c) {
            case 'a':
                printf("scroll menu right\n");
                advance_sign = -1;
                break;
            case 'd':
                    printf("scroll menu left\n");
                    advance_sign = 1;
                    break;
            case 'c':
                printf("enable clear rect\n");
                clear_rects.number = 1;
                clear_rects.rects[0].rect.x+=10;
                clear_rects.rects[0].rect.y+=10;
                bdvd_dfb_ext_set_clear_rects(graphics_dfb_ext, &clear_rects);
                continue;
            case 'r':
                printf("remove clear rect\n");
                clear_rects.number = 0;
                bdvd_dfb_ext_set_clear_rects(graphics_dfb_ext, &clear_rects);
                continue;
            default:
            continue;
        }

        current_time = previous_time = start_time = myclock();
        nb_frames_rendered = 0;

        blit_ops = 0;
        blend_ops = 0;
        clear_ops = 0;
        flipblit_ops = 0;
        queued_ops_time = 0;
        queued_ops_cumul_time = 0;
        flip_cumul_time = 0;

        i = (advance_sign == 1 ? 0 : nb_frame_sec-1);

        while (advance_sign == 1 ? i < nb_frame_sec : i >= 0) {
            fill_rect.w = mainmenu_rect.w < previous_submenu_rect.x+previous_submenu_rect.w ? previous_submenu_rect.x+previous_submenu_rect.w : mainmenu_rect.w;
            current_submenu_rect.x = (current_submenu_rect.x != advance_sign*((nb_frame_sec-1)*step_size*height_convertion-current_submenu_rect.w) ? i*step_size : (nb_frame_sec-1)*step_size)*height_convertion-current_submenu_rect.w;
            dfb_rectangle_union ( &previous_submenu_rect, &current_submenu_rect );
            dfb_rectangle_intersect ( &previous_submenu_rect, &layer_surface_rect );
            flip_region = (DFBRegion){previous_submenu_rect.x, previous_submenu_rect.y, previous_submenu_rect.x+(previous_submenu_rect.w ? previous_submenu_rect.w-1 : 0), previous_submenu_rect.y+(previous_submenu_rect.h ? previous_submenu_rect.h-1 : 0)};

            /* For better cleaning of the blended result */
            flip_region.x1 = 0;
            flip_region.x2 = mainmenu_rect.w-1 < flip_region.x2 ? flip_region.x2 : mainmenu_rect.w-1;

            previous_submenu_rect = current_submenu_rect;

            if (layer_surface->SetColor(layer_surface, 0, 0, 0, 0) != DFB_OK) {
                D_ERROR("SetColor failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            clear_ops += fill_rect.w*fill_rect.h;
            if (layer_surface->FillRectangles(layer_surface, &fill_rect, 1) != DFB_OK) {
                D_ERROR("StretchBlit failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            if (layer_surface->GetCapabilities(frame_surfaces[1], &surface_caps) != DFB_OK) {
                D_ERROR("GetCapabilities failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            if (layer_surface->SetBlittingFlags(layer_surface, ((int)surface_caps & DSCAPS_PREMULTIPLIED ? 0 : DSBLIT_SRC_PREMULTIPLY)) != DFB_OK) {
                D_ERROR("SetBlittingFlags failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            blit_ops += current_submenu_rect.w*current_submenu_rect.h;
#if 0
            if (layer_surface->StretchBlit(layer_surface, frame_surfaces[1], NULL, &current_submenu_rect) != DFB_OK) {
#else
            if (layer_surface->Blit(layer_surface, frame_surfaces[1], NULL, current_submenu_rect.x, current_submenu_rect.y) != DFB_OK) {
#endif
                D_ERROR("StretchBlit failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            if (layer_surface->SetColor (layer_surface, 0xff, 0, 0, 0xff) != DFB_OK) {
                D_ERROR("SetColor failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            if ((current_submenu_rect.x+current_submenu_rect.w/2) > 0) {
                if (layer_surface->DrawString (layer_surface, printed_string, -1, current_submenu_rect.x+current_submenu_rect.w/2, current_submenu_rect.h/5, text_flags) != DFB_OK) {
                    D_ERROR("DrawString failed\n");
                    ret = EXIT_FAILURE;
                    goto out;
                }
            }

            if (layer_surface->GetCapabilities(frame_surfaces[0], &surface_caps) != DFB_OK) {
                D_ERROR("GetCapabilities failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            if (layer_surface->SetBlittingFlags(layer_surface, DSBLIT_BLEND_ALPHACHANNEL | ((int)surface_caps & DSCAPS_PREMULTIPLIED ? 0 : DSBLIT_SRC_PREMULTIPLY))) {
                D_ERROR("SetBlittingFlags failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            blend_ops += mainmenu_rect.w*mainmenu_rect.h;
#if 0
            if (layer_surface->StretchBlit(layer_surface, frame_surfaces[0], NULL, &mainmenu_rect) != DFB_OK) {
#else
            if (layer_surface->Blit(layer_surface, frame_surfaces[0], NULL, mainmenu_rect.x, mainmenu_rect.y) != DFB_OK) {
#endif
                D_ERROR("StretchBlit failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            /* THIS IS NOT A NECESSARY STEP.
             * This is only to get an accurate reading of the time taken by queued operations.
             * In a typical application, WaitIdle is already performed in the Flip call.
             */
            if (dfb->WaitIdle(dfb) != DFB_OK) {
                D_ERROR("WaitIdle failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            queued_ops_time = myclock();
            queued_ops_cumul_time += queued_ops_time - previous_time;

            if (partial_flip) {
                flipblit_ops += (flip_region.x2-flip_region.x1+1)*(flip_region.y2-flip_region.y1+1);
            }
            else {
                flipblit_ops += layer_surface_rect.w*layer_surface_rect.h;
            }

            /* DSFLIP_NONE here is really (DSFLIP_BLIT | DSFLIP_WAITFORSYNC) */
            if (layer_surface->Flip(layer_surface, partial_flip ? &flip_region : NULL, DSFLIP_NONE) != DFB_OK) {
                D_ERROR("Flip failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }

            current_time = myclock();
            flip_cumul_time += current_time - queued_ops_time;

            nb_jump = (current_time - previous_time)/((float)1000/(float)nb_frame_sec);
            previous_time = current_time;
            nb_jump = (advance_sign == 1 ? i < nb_frame_sec-nb_jump : i >= nb_jump) ? nb_jump : 1;
            i += advance_sign*(nb_jump ? nb_jump : 1);
            nb_frames_rendered++;
        }

        previous_time = current_time - start_time;

        fprintf(stderr, "Total frame rendered %u in %u ms\n", nb_frames_rendered, previous_time);
        fprintf(stderr, "Queued graphics operations took an average %f ms per frame\n", (float)queued_ops_cumul_time/(float)nb_frames_rendered);
        fprintf(stderr, "Flip-blit and time spent waiting for vsync took an average %f ms per frame\n", (float)flip_cumul_time/(float)nb_frames_rendered);
        fprintf(stderr, "Total waiting period for Flip operation would have been %f ms per frame\n", (float)(queued_ops_cumul_time+flip_cumul_time)/(float)nb_frames_rendered);
        fprintf(stderr, "Total rates for the sequence is:\n"
                        "clear %f Mpxls/sec\n"
                        "blit %f Mpxls/sec\n"
                        "blend %f Mpxls/sec\n"
                        "flip-blit %f Mpxls/sec\n",
                        blit_ops/(float)1000000,
                        blend_ops/(float)1000000,
                        clear_ops/(float)1000000,
                        flipblit_ops/(float)1000000);

        D_INFO("Press a key, then enter key to scroll left,\n"
               "    a then enter to scroll right, s then enter to leave\n");
    }

out:

    /* Cleanup */
    for (i = 0; i < sizeof(frame_surfaces)/sizeof(frame_surfaces[0]); i++) {
        if (frame_surfaces[i]) {
            if (frame_surfaces[i]->Release(frame_surfaces[i]) != DFB_OK) {
                D_ERROR("Release failed\n");
                ret = EXIT_FAILURE;
            }
        }
    }
    if (graphics_layer) {
        if (graphics_layer->SetLevel(graphics_layer, -1) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (layer_surface) {
        if (layer_surface->Release(layer_surface) != DFB_OK) {
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

    /* The Render/Scale intermediate surface must be released at the end */
    if (render_convert_release() != DFB_OK) {
        D_ERROR("render_convert_release failed\n");
        ret = EXIT_FAILURE;
    }

#ifdef BCMPLATFORM
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
