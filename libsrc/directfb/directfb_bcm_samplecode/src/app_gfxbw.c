/***************************************************************************
 *     Copyright (c) 2005-2006, Broadcom Corporation
 *     All Rights Reserved;
 *     Confidential Property of Broadcom Corporation
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************/

#include "bdvd.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
       
static void
print_usage(const char * program_name)
{
  printf("\nlaunch %s ...\n\n", program_name);
}

/* Timer control routines */
static inline uint32_t test_mssleep(uint32_t ms)
{
    struct timespec timeout;
    struct timespec timerem;
    
    timeout.tv_sec = ms / 1000ul;
    timeout.tv_nsec = (ms % 1000ul) * 1000000ul;
    
    do {
        if(nanosleep(&timeout, &timerem) == -1) {
            if (errno == EINTR) {
                memcpy(&timeout, &timerem, sizeof(timeout));
            }
            else {
                fprintf(stderr, "test_mssleep: %s", strerror(errno));
                return 0;
            }
        }
        else {
            return 0;
        }
    } while (1);
}

static inline uint32_t gettimeus()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000000 + tv.tv_usec);
}

static bdvd_result_e
display_get_video_format_dimensions(bvideo_format video_format, bgraphics_pixel_format pixel_format, int * width, int * height) {

    switch (video_format) {
        case bdvd_video_format_ntsc:
        case bdvd_video_format_480p:
            *width  = 720;
            *height = 480;
        break;            
        case bdvd_video_format_720p:
#ifdef OPTIMIZED_RTS_SETTINGS
            *width  = 1280;
#else
        /*PR 14281 - we would prefer width 720 to save
          memory and scale only once, but the scaling quality is bad.
          1280 allows 1080i to use 960, 720p to use 1280 and 480i/p to use 720.
          This appears best. However, 1280 on 720p with 32 bpp causes an RTS
          failure, so in that case, we have to accept the bad quality and go 960.*/
        /* [xm] Even with 960, there are still RTS failure in 720p, so let's drop
         * to 720. I think it's because the 398 as 32-bit wide DDR banks.
         * [xm] Because of horizontal scaling, we are loosing the original aspect ratio,
         * must find a fix for that.
         */
        if (pixel_format == bgraphics_pixel_format_a8_r8_g8_b8) {
            *width = 720; /* bad quality on 720p, but at least it works. */
        }
        else {
            *width  = 1280;
        }
#endif
            *height = 720;
        break;            
        case bdvd_video_format_1080i:
#ifdef OPTIMIZED_RTS_SETTINGS
            *width  = 1920;
#else
        /* See comments for 720p, the same applies here
         */
        if (pixel_format == bgraphics_pixel_format_a8_r8_g8_b8) 
            *width = 960; /* bad quality on 1080i, but at least it works. */
        else
            *width  = 1920;
#endif
            *height = 1080;
        break;
        default:
            fprintf(stderr, "display_get_video_format_dimensions: invalid video_format\n");
            return bdvd_err_unspecified;
    }   
    
    return bdvd_ok;
}

static bvideo_format
display_get_video_format(bdvd_display_h display) {
    bdvd_display_settings_t display_settings;
    
    bdvd_display_get(display, &display_settings);

    return display_settings.format;
}

static bdvd_result_e
display_set_video_format(bdvd_display_h display,
                         bgraphics_t graphics,
                         bvideo_format video_format) {
    static bdvd_output_svideo_h svideo=NULL;
    static bdvd_output_composite_h composite=NULL;
    static bdvd_output_rf_h rf=NULL;

    bdvd_display_settings_t display_settings;
    bgraphics_settings graphics_settings;
    bdvd_video_format_settings_t display_video_format_settings;

    assert( display != NULL );
    assert( graphics != NULL );
        printf("AAAAAAff\n");
    bdvd_display_get(display, &display_settings);
        printf("AAAAAAccff\n");
    if (display_settings.format != video_format) {
        //save handles
        if(display_settings.svideo) svideo = display_settings.svideo;
        if(display_settings.composite) composite = display_settings.composite;
        if(display_settings.rf) rf = display_settings.rf;
        
        display_settings.format = video_format;
        
        switch (display_settings.format) {
            case bdvd_video_format_ntsc:
                fprintf(stdout, "display_set_video_format: setting to ntsc\n");
                //restore handles if they have been deactivated
                if((display_settings.rf == NULL) && rf)
                    display_settings.rf = rf;
                if((display_settings.svideo == NULL) && svideo)
                    display_settings.svideo = svideo;
                if((display_settings.composite == NULL) && composite)
                    display_settings.composite = composite;
            break;
            case bdvd_video_format_480p:
                fprintf(stdout, "display_set_video_format: setting to 480p\n");
                display_settings.composite = NULL;
                display_settings.svideo = NULL;
                display_settings.rf = NULL;
            break;     
            case bdvd_video_format_720p:
                fprintf(stdout, "display_set_video_format: setting to 720p\n");
                display_settings.composite = NULL;
                display_settings.svideo = NULL;
                display_settings.rf = NULL;
            break;            
            case bdvd_video_format_1080i:
                fprintf(stdout, "display_set_video_format: setting to 1080i\n");
                display_settings.composite = NULL;
                display_settings.svideo = NULL;
                display_settings.rf = NULL;
            break;
            default:
                fprintf(stderr, "display_set_video_format: video format does not exist\n");
                return DFB_INVARG;
        }

        if (bdvd_display_set(display, &display_settings) != bdvd_ok) {
            fprintf(stderr, "display_set_video_format: could not set display settings\n");
            return bdvd_err_unspecified;
        }
    }

    bdvd_display_get(display, &display_settings);
    /* We need to add this functions to bdvd */
    bdvd_video_get_format_settings(display_settings.format, &display_video_format_settings);

    bdvd_graphics_get(graphics, &graphics_settings);

    if (graphics_settings.nb_framebuffers != 0 &&
        graphics_settings.framebuffers != NULL) {
        graphics_settings.enabled = true;
        graphics_settings.chromakey_enabled = false;

        display_get_video_format_dimensions(video_format,
                                            graphics_settings.framebuffers[0]->create_settings.pixel_format,
                                            &graphics_settings.source_width,
                                            &graphics_settings.source_height);
        fprintf(stdout, "display_set_video_format: now set with:\n"
                "graphics_settings.source_width: %d\n"
                "graphics_settings.source_height: %d\n",
                graphics_settings.source_width,
                graphics_settings.source_height);
        graphics_settings.destination_width = display_video_format_settings.width;
        graphics_settings.destination_height = display_video_format_settings.height;
        fprintf(stdout, "display_set_video_format: now set with:\n"
                "display_video_format_settings.width: %d\n"
                "display_video_format_settings.height: %d\n",
                display_video_format_settings.width,
                display_video_format_settings.height);
        
        if (bdvd_graphics_set(graphics, &graphics_settings) != bdvd_ok) {
            fprintf(stderr, "display_set_video_format: could not set graphics settings\n");
            return bdvd_err_unspecified;
        }
    }

    return bdvd_ok;
}

static bdvd_result_e
create_graphics_framebuffers(bgraphics_t graphics,
                             size_t nb_framebuffers,
                             bsurface_t * framebuffers,
                             bsurface_create_settings * create_settings )
{
    bdvd_result_e ret;
    bresult err;
    bgraphics_settings graphics_settings;
    uint32_t i;

    bsettop_rect fill_rect;

    assert( graphics != NULL );
    assert( create_settings != NULL );
    if (create_settings->pixel_format != bgraphics_pixel_format_a8_r8_g8_b8) {
        fprintf(stdout, "create_graphics_framebuffers: bgraphics framebuffer should really be\n"
                "a8_r8_g8_b8 format to keep alphachannel\n" );
    }

    memset(&fill_rect, 0, sizeof(fill_rect));
    fill_rect.width = create_settings->width;
    fill_rect.height = create_settings->height;
    for (i = 0; i < nb_framebuffers; i++) { 
        /* Don't use bgraphics double buffering */
        if ((framebuffers[i] = bdvd_surface_create(create_settings)) == NULL) {
            fprintf(stderr, "create_graphics_framebuffers: error creating graphics framebuffer surface\n");
            ret = bdvd_err_unspecified;
            goto error;
        }

        if ((err = bdvd_surface_fill(
              framebuffers[i],
              &fill_rect,
              (bgraphics_pixel)0)) != bdvd_ok) {
            fprintf(stderr, "create_graphics_framebuffers: error calling bdvd_surface_fill %d\n",
                 err );
            ret = bdvd_err_unspecified;
            goto error;
        }
        fprintf(stdout, "create_graphics_framebuffers: Created bgraphics framebuffer [%p]:\n"
                "pixel_format: %d\n"
                "width: %d\n"
                "height: %d\n",
                framebuffers[i],
                create_settings->pixel_format,
                create_settings->width,
                create_settings->height);
    }
    bdvd_graphics_get(graphics, &graphics_settings);
    graphics_settings.framebuffers = framebuffers;
    graphics_settings.nb_framebuffers = nb_framebuffers;
    
    if (bdvd_graphics_set(graphics, &graphics_settings) != bdvd_ok) {
        fprintf(stderr, "create_graphics_framebuffers: error setting graphic framebuffer surface\n");
        ret = bdvd_err_unspecified;
        goto error;
    }
 
    return bdvd_ok;

error:

    return ret;
}

int
main(int argc, const char *argv[])
{
    int ret = EXIT_SUCCESS;

    bresult err;
    
    bool render_offscreen = true;
    
    bool fb_are_max_size = false;
    
    bool do_alpha_premult = false;
    
    bool start_graphics = false;
    
    bool wait_for_op = true;

    uint32_t current_time;
    uint32_t previous_time;
    
    char c;

    bvideo_format video_format = bdvd_video_format_ntsc;
    uint32_t width = 4096;
    uint32_t height = 4096;
    
    bdvd_display_h display = NULL;
    bgraphics_t graphics = NULL;

    bsurface_t framebuffer = NULL;

    bsurface_t test_surface = NULL;
    bsurface_t offscreen_surface = NULL;    

    bsurface_create_settings framebuffer_settings;

    bsurface_create_settings create_settings;

    bsettop_rect surface_rect;

    framebuffer_settings.pixel_format = bgraphics_pixel_format_a8_r8_g8_b8;

    if (argc < 1) {
        print_usage(argv[0]);
        return -1;
    }

    while ((c = getopt (argc, argv, "lbgrs:h:w:p")) != -1) {
        switch (c) {
        case 'l':
            wait_for_op = false;
        break;
        case 'f':
            render_offscreen = false;
        break;
        case 'r':
            video_format = bdvd_video_format_1080i;
        break;
        case 'g':
            start_graphics = true;
        break;
        case 'w':
            width = strtoul(optarg, NULL, 10);
        break;
        case 'h':
            height = strtoul(optarg, NULL, 10);
        break;
        case 'p':
            do_alpha_premult = true;
        break;
        case '?':
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
            goto out;
        break;
        }
    }

    if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
        fprintf(stderr, "bdvd_init failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if ((display = bdvd_display_open(B_ID(0))) == NULL) {
        printf("bdvd_display_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if ((graphics = bgraphics_open(B_ID(0),display)) == NULL) {
        printf("bgraphics_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (display_set_video_format(display, graphics, video_format) != bdvd_ok) {
        fprintf(stderr, "display_set_video_format failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (fb_are_max_size) {
        /* [xm] Should always put maximum (bdvd_video_format_1080i),
         * because there is no way to reallocate
         * the frame buffer when changing the output resolution. Instead just
         * resize the region.
         */
        if ((ret = display_get_video_format_dimensions(
            bdvd_video_format_1080i,
            framebuffer_settings.pixel_format,
            &framebuffer_settings.width, &framebuffer_settings.height)) != bdvd_ok) {
            fprintf(stderr, "display_get_video_format_dimensions failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
    }
    else {
        if ((ret = display_get_video_format_dimensions(
            video_format,
            framebuffer_settings.pixel_format,
            &framebuffer_settings.width, &framebuffer_settings.height)) != bdvd_ok) {
            fprintf(stderr, "display_get_video_format_dimensions failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
    }

    if (start_graphics) {
        if ((ret = create_graphics_framebuffers(graphics,
                    1,
                    &framebuffer,
                    &framebuffer_settings )) != bdvd_ok) {
            fprintf(stderr, "display_get_video_format_dimensions failed\n");
            ret = EXIT_FAILURE;
            goto out;
        }
    }

    create_settings.pixel_format = bgraphics_pixel_format_a8_r8_g8_b8;
    create_settings.width = width;
    create_settings.height = height;

    if ((test_surface = bdvd_surface_create(&create_settings)) == NULL) {
        fprintf(stderr, "bdvd_surface_create failed for test_surface\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Measuring the fill\n");
    
    memset(&surface_rect, 0, sizeof(surface_rect));
    surface_rect.width = create_settings.width;
    surface_rect.height = create_settings.height;

    printf("Before BGRC_ListEngineBusy %d BGRC_ListEngineFinished %d BGRC_BlitEngineRunning %d\n",
           BGRC_ListEngineBusy(b_root.video.grc),
           BGRC_ListEngineFinished(b_root.video.grc),
           BGRC_BlitEngineRunning(b_root.video.grc));

    previous_time = gettimeus();

    if ((err = bdvd_surface_fill(
        test_surface,
        &surface_rect,
        (bgraphics_pixel)0)) != bdvd_ok) {
        fprintf(stderr, "error calling bdvd_surface_fill %d\n",
             err );
        ret = EXIT_FAILURE;
        goto out;
    }

    if (!wait_for_op) {
        while (BGRC_BlitEngineBusy(b_root.video.grc));
    }
    
    current_time = gettimeus();

    printf("After BGRC_ListEngineBusy %d BGRC_ListEngineFinished %d BGRC_BlitEngineRunning %d\n",
           BGRC_ListEngineBusy(b_root.video.grc),
           BGRC_ListEngineFinished(b_root.video.grc),
           BGRC_BlitEngineRunning(b_root.video.grc));
    
    printf("Previous time %u (us), current time %u (us), total time is %u (us)\n",
           previous_time, current_time, current_time-previous_time);

    current_time -= previous_time;
    current_time = current_time/1000;

    printf("Pixel size %ux%u, total bandwidth %u\n",
           surface_rect.width, surface_rect.height,
           current_time ? surface_rect.width*surface_rect.height/(current_time) : 0);

    getchar();
    
    printf("Measuring the blit\n");

    if ((offscreen_surface = bdvd_surface_create(&create_settings)) == NULL) {
        fprintf(stderr, "bdvd_surface_create failed for offscreen_surface\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Before BGRC_ListEngineBusy %d BGRC_ListEngineFinished %d BGRC_BlitEngineRunning %d\n",
           BGRC_ListEngineBusy(b_root.video.grc),
           BGRC_ListEngineFinished(b_root.video.grc),
           BGRC_BlitEngineRunning(b_root.video.grc));

    previous_time = gettimeus();

    if ((err = bdvd_surface_blit(offscreen_surface, &surface_rect,
                             BGFX_RULE_PORTERDUFF_SRC,
                             do_alpha_premult ? BGFX_RULE_SRCALPHA_ZERO : BGFX_RULE_PORTERDUFF_SRC,
                             test_surface, &surface_rect,
                             NULL, NULL,
                             (bgraphics_pixel)0,
                             false,
                             wait_for_op)) != bdvd_ok) {
        fprintf(stderr, "error calling bdvd_surface_simpleblit in BGFX_RULE_PORTERDUFF_SRCOVER %d\n",
                 err );
        ret = EXIT_FAILURE;
        goto out;
    }

    if (!wait_for_op) {
        while (BGRC_BlitEngineRunning(b_root.video.grc));
    }

    current_time = gettimeus();

    printf("After BGRC_ListEngineBusy %d BGRC_ListEngineFinished %d BGRC_BlitEngineRunning %d\n",
           BGRC_ListEngineBusy(b_root.video.grc),
           BGRC_ListEngineFinished(b_root.video.grc),
           BGRC_BlitEngineRunning(b_root.video.grc));
    
    printf("Previous time %u (us), current time %u (us), total time is %u (us)\n",
           previous_time, current_time, current_time-previous_time);

    current_time -= previous_time;
    current_time = current_time/1000;

    printf("Pixel size %ux%u, total bandwidth %u\n",
           surface_rect.width, surface_rect.height,
           current_time ? surface_rect.width*surface_rect.height/(current_time) : 0);

    getchar();
    
    printf("Measuring the blend\n");

    printf("Before BGRC_ListEngineBusy %d BGRC_ListEngineFinished %d BGRC_BlitEngineRunning %d\n",
           BGRC_ListEngineBusy(b_root.video.grc),
           BGRC_ListEngineFinished(b_root.video.grc),
           BGRC_BlitEngineRunning(b_root.video.grc));

    previous_time = gettimeus();

    if ((err = bdvd_surface_blit(offscreen_surface, &surface_rect,
                             BGFX_RULE_PORTERDUFF_SRCOVER,
                             do_alpha_premult ? BGFX_RULE_SRCALPHA_INVSRCALPHA : BGFX_RULE_PORTERDUFF_SRCOVER,
                             test_surface, &surface_rect,
                             offscreen_surface, &surface_rect,
                             (bgraphics_pixel)0,
                             false,
                             wait_for_op)) != bdvd_ok) {
        fprintf(stderr, "error calling bdvd_surface_simpleblit in BGFX_RULE_PORTERDUFF_SRCOVER %d\n",
                 err );
        ret = EXIT_FAILURE;
        goto out;
    }
    
    if (!wait_for_op) {
        while (BGRC_BlitEngineRunning(b_root.video.grc));
    }

    current_time = gettimeus();

    printf("After BGRC_ListEngineBusy %d BGRC_ListEngineFinished %d BGRC_BlitEngineRunning %d\n",
           BGRC_ListEngineBusy(b_root.video.grc),
           BGRC_ListEngineFinished(b_root.video.grc),
           BGRC_BlitEngineRunning(b_root.video.grc));
    
    printf("Previous time %u (us), current time %u (us), total time is %u (us)\n",
           previous_time, current_time, current_time-previous_time);

    current_time -= previous_time;
    current_time = current_time/1000;

    printf("Pixel size %ux%u, total bandwidth %u\n",
           surface_rect.width, surface_rect.height,
           current_time ? surface_rect.width*surface_rect.height/(current_time) : 0);

out:

    printf("getting out\n");

    if (graphics) bgraphics_close(graphics);
    if (display) bdvd_display_close(display);
    bdvd_uninit();

    return ret;
}
