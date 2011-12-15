
#include <directfb.h>

#include <direct/debug.h>

#include <directfb-internal/misc/util.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <bdvd.h>

#include "dfbbcm_utils.h"
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_gfxperfthread.h"

typedef struct {
    IDirectFBSurface * surface;
    bool cancelled;
} flip_routine_arguments_t;

/* When this thing dies, the fcts of this class should responds an error */
void * flip_thread_routine(void * arg) {

    flip_routine_arguments_t * routine_args = (flip_routine_arguments_t *)arg;
    DFBResult err;
    uint32_t i = 0;

    fprintf(stderr, "running flipping thread\n");

    while (!routine_args->cancelled) {
        DFBCHECK( routine_args->surface->Flip( routine_args->surface, NULL, 0 ) );
        i++;
    }

    fprintf(stderr, "exiting flipping thread, flipped %u times\n", i);

    return NULL;
}

typedef enum {
    CMD_PAUSE_ANIMATION = 0,
    CMD_MOVE_RIGHT_ANIMATION,
    CMD_MOVE_LEFT_ANIMATION
} animation_input_command;

typedef struct {
    IDirectFBSurface * surface;
    IDirectFBSurface * front_surface;
    IDirectFBFont ** fonts;
    size_t nb_fonts;
    char ** strings;
    int * x;
    int * y;
    int increment_value;
    size_t nb_strings;
    bool cancelled;
    bool clear;
    animation_input_command input_cmd;
} draw_string_routine_arguments_t;

void * draw_string_thread_routine(void * arg) {
    draw_string_routine_arguments_t * routine_args = (draw_string_routine_arguments_t *)arg;
    DFBResult err;
    uint32_t i = 0;
    int width;
    int height;
    uint32_t font_index = 0;
    
    int32_t move_inc = 0;
    
    uint32_t frame_count = 0;
    uint32_t currenttime;
    uint32_t previoustime = 0;

    fprintf(stderr, "running draw string thread\n");

    DFBCHECK( routine_args->surface->GetSize( routine_args->surface, &width, &height) );

   fprintf(stderr, "nb width  %d height %d\n", width, height);

    for (i = 0; i < routine_args->nb_strings; i++) {
        routine_args->x[i] = i*width/5;
        routine_args->y[i] = 4*height/5;
        /* Something weird here... */
        fprintf(stderr, "nb string %d %d\n", routine_args->x[i], routine_args->y[i]);        
    }

    DFBCHECK( routine_args->surface->SetDrawingFlags(routine_args->surface, DSDRAW_NOFX));
    DFBCHECK( routine_args->surface->SetBlittingFlags(routine_args->surface, DSBLIT_NOFX));

    while (!routine_args->cancelled) {
        switch (routine_args->input_cmd) {
            case CMD_MOVE_RIGHT_ANIMATION:
                move_inc = routine_args->increment_value*1;
            break;
            case CMD_PAUSE_ANIMATION:
                move_inc = 0;
            break;
            case CMD_MOVE_LEFT_ANIMATION:
                move_inc = routine_args->increment_value*-1;
            break;
            default:
            break;
        }
        if (routine_args->clear) {
            DFBCHECK( routine_args->surface->Clear( routine_args->surface, 0, 0, 0, 0 ) );
        }
        else {
            DFBCHECK( routine_args->surface->SetColor ( routine_args->surface,
                                                        0, 0, 0, 0));
            DFBCHECK( routine_args->surface->FillRectangle ( routine_args->surface,
                                                        0, 4*height/5, width, height/5));
        }
        for (i = 0; i < routine_args->nb_strings; i++) {
            DFBCHECK( routine_args->surface->SetColor ( routine_args->surface,
                                                        0xFF, 0xFF, 0xFF, 0xFF));
            font_index = routine_args->x[i]/width/5;
            DFBCHECK( routine_args->surface->SetFont ( routine_args->surface,
                                                       routine_args->fonts[font_index]));
            DFBCHECK( routine_args->surface->DrawString ( routine_args->surface,
                                                          routine_args->strings[i],
                                                          -1,
                                                          routine_args->x[i],
                                                          routine_args->y[i],
                                                          DSTF_TOP | DSTF_LEFT));
            if (move_inc < 0) {
                routine_args->x[i] = routine_args->x[i] + move_inc > 0 ? routine_args->x[i] + move_inc : width-1;
            }
            else if (move_inc > 0) {
                routine_args->x[i] = routine_args->x[i] + move_inc < width ? routine_args->x[i] + move_inc : 0;
            }
        }
        if (routine_args->front_surface) {
            DFBCHECK( routine_args->front_surface->Blit ( routine_args->front_surface,
                                                          routine_args->surface,
                                                          NULL,
                                                          0, 0));
            DFBCHECK( routine_args->front_surface->Flip( routine_args->front_surface, NULL, 0 ) );
        }
        else {
            DFBCHECK( routine_args->surface->Flip( routine_args->surface, NULL, 0 ) );
        }

        if (!(++frame_count % 30)) {
            currenttime = myclock();
            fprintf(stderr, "+++++++count %d, time elapsed %u+++++++\n", frame_count, currenttime - previoustime);
            previoustime = myclock();
            frame_count = 0;
        }
    }

    fprintf(stderr, "exiting draw string thread\n");

    return NULL;
}

int main( int argc, char *argv[] )
{
    DFBResult err;
    int rc;
    int ret = EXIT_SUCCESS;
    bdvd_result_e berr;
    IDirectFB * dfb = NULL;
    IDirectFBSurface * first_surface = NULL;
    IDirectFBSurface * second_surface = NULL;
    IDirectFBSurface * image_surface = NULL;
    IDirectFBDisplayLayer * first_layer = NULL;
    IDirectFBDisplayLayer * second_layer = NULL;
    bdvd_dfb_ext_h first_dfb_ext = NULL;
    bdvd_dfb_ext_h second_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t first_dfb_ext_settings;
    bdvd_dfb_ext_layer_settings_t second_dfb_ext_settings;
    char * decode_image_buffer;
    int surface_pitch;
    char * surface_buffer;

    IDirectFBImageProvider *provider;
    DFBSurfaceDescription surface_desc;
    DFBDisplayLayerConfig layer_config;
    
    bdvd_display_h bdisplay = NULL;
    bvideo_format video_format = bvideo_format_ntsc;
    char image_name[256];
    
    uint32_t step_duration[10];
    uint32_t step_index = 0;
    char c;
    uint32_t i;
    uint32_t total_time = 0;
    
    bool enable_flipping_thread = true;
    
    bool test_image_decode = false;
    bool test_font_render = false;
    
    bool stop_input_event = false;
    
    bool video_running = false;
    bool offscreen_surface = false;
    
    pthread_t flipping_thread;
    pthread_attr_t flipping_thread_attr;
    flip_routine_arguments_t flipping_thread_args;
    pthread_t draw_string_thread;
    pthread_attr_t draw_string_thread_attr;
    draw_string_routine_arguments_t draw_string_thread_args;

#define MAX_STRING_LENGTH 256

    char strings[][MAX_STRING_LENGTH] = { "CHAPTERS", "MOVIE", "SYNC", "SETTINGS", "ONLINE"};

    char font_name[256];    
    DFBFontDescription font_desc;
    
    int font_heights[] = { 17, 19, 21};

    memset(&layer_config, 0, sizeof(layer_config));

    memset(&flipping_thread_args, 0, sizeof(flipping_thread_args));
    
    memset(&draw_string_thread_args, 0, sizeof(draw_string_thread_args));

    draw_string_thread_args.increment_value = 10;

    while ((c = getopt (argc, argv, "eri:f:nc:o")) != -1) {
        switch (c) {
        case 'e':
            draw_string_thread_args.clear = true;
        break;
        case 'r':
            video_format = bvideo_format_1080i;
        break;
        case 'i':
            snprintf(image_name, sizeof(image_name)/sizeof(image_name[0]), "%s", optarg);
            test_image_decode = true;
        break;
        case 'f':
            snprintf(font_name, sizeof(font_name)/sizeof(font_name[0]), "%s", optarg);
            test_font_render = true;
        break;
        case 'n':
            enable_flipping_thread = false;
        break;
        case 'c':
            draw_string_thread_args.increment_value = strtoul(optarg, NULL, 10);
        break;
        case 'o':
            offscreen_surface = true;
        break;
        default:
        break;
        }
    }

    bdvd_init(BDVD_VERSION);
    
    bdisplay=bdvd_display_open(B_ID(0));
    
    dfb=bdvd_dfb_ext_open_with_params(2, &argc, &argv);

    DFBCHECK( display_set_video_format(bdisplay, video_format) );

    switch (video_format) {
        case bvideo_format_ntsc:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
           layer_config.flags |= DLCONF_WIDTH;
           layer_config.flags |= DLCONF_HEIGHT;
           layer_config.width = 720;
           layer_config.height = 480;
        break;
        case bvideo_format_1080i:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
           layer_config.flags |= DLCONF_WIDTH;
           layer_config.flags |= DLCONF_HEIGHT;
           layer_config.width = 1920;
           layer_config.height = 1080;
        break;
        default:
        break;
    }

    first_dfb_ext=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_presentation_layer);
    bdvd_dfb_ext_layer_get(first_dfb_ext, &first_dfb_ext_settings);
    DFBCHECK ( dfb->GetDisplayLayer(dfb,first_dfb_ext_settings.id,&first_layer) );
    DFBCHECK ( first_layer->SetCooperativeLevel(first_layer,DLSCL_ADMINISTRATIVE ));
    DFBCHECK ( first_layer->SetConfiguration(first_layer, &layer_config) );
    DFBCHECK ( first_layer->SetLevel(first_layer, 1));
    DFBCHECK ( first_layer->GetSurface(first_layer,&first_surface) );

    second_dfb_ext=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_interactive_layer);
    bdvd_dfb_ext_layer_get(second_dfb_ext, &second_dfb_ext_settings);
    DFBCHECK ( dfb->GetDisplayLayer(dfb,second_dfb_ext_settings.id,&second_layer) );
    DFBCHECK ( second_layer->SetCooperativeLevel(second_layer,DLSCL_ADMINISTRATIVE ));
    DFBCHECK ( second_layer->SetConfiguration(second_layer, &layer_config) );
    DFBCHECK ( second_layer->SetLevel(second_layer, 2));
    DFBCHECK ( second_layer->GetSurface(second_layer,&second_surface) );

    /* From this point, only the display_pts_callback should be allowed to operate on the osd surface */
    if (video_running) {
        fprintf(stderr, "Creating video stress thread\n");
        playbackscagat_start(bdisplay, NULL, NULL, NULL, argc, argv);
    }

    if (test_image_decode) {

        if (enable_flipping_thread) {
            flipping_thread_args.surface = first_surface;
        
            /* We might eventually want to set attributes ... */
            if ((rc = pthread_attr_init(&flipping_thread_attr))) {
                D_ERROR("Could not pthread_attr_init %s\n", strerror(rc));
                ret = EXIT_FAILURE;
                goto out;
            }
            
            if ((rc = pthread_create(&flipping_thread, &flipping_thread_attr, flip_thread_routine, &flipping_thread_args))) {
                D_ERROR("Could not pthread_create %s\n", strerror(rc));
                ret = EXIT_FAILURE;
                goto out;
            }
        }
    
        step_duration[step_index++] = myclock();
    
        DFBCHECK(dfb->CreateImageProvider( dfb, image_name, &provider ));
    
        DFBCHECK(provider->GetSurfaceDescription( provider, &surface_desc ));
        surface_desc.flags = DSDESC_CAPS;
        surface_desc.caps &= ~DSCAPS_SYSTEMONLY;
        surface_desc.caps |= DSCAPS_VIDEOONLY;
        DFBCHECK(dfb->CreateSurface( dfb, &surface_desc, &image_surface ));
    
        step_duration[step_index++] = myclock();
    
        DFBCHECK(provider->RenderTo( provider, image_surface, NULL ));
    
        step_duration[step_index++] = myclock();
    
        DFBCHECK(image_surface->Lock(image_surface, DSLF_READ, &surface_buffer, &surface_pitch));
        if ((decode_image_buffer = malloc(surface_pitch*surface_desc.height)) == NULL) {
            D_ERROR("Could not malloc %s\n", strerror(errno));
            ret = EXIT_FAILURE;
        }
        memcpy(decode_image_buffer, surface_buffer, surface_pitch*surface_desc.height);
        free(decode_image_buffer);
        DFBCHECK(image_surface->Unlock(image_surface));
    
        step_duration[step_index++] = myclock();
    
        DFBCHECK(second_surface->StretchBlit( second_surface, image_surface, NULL, NULL ));
    
        DFBCHECK(dfb->WaitIdle( dfb));
    
        step_duration[step_index++] = myclock();
    
        DFBCHECK(second_surface->Flip( second_surface, NULL, 0 ));
    
        step_duration[step_index++] = myclock();
    
        if (enable_flipping_thread) {
            flipping_thread_args.cancelled = true;
        
            if ((rc = pthread_join(flipping_thread, NULL))) {
                D_ERROR("Could not pthread_join %s\n", strerror(rc));
                ret = EXIT_FAILURE;
                goto out;
            }
        }
    
        for (i = 0; i < step_index; i++) {
            if (i) fprintf(stderr, "%u step took %u ms\n", i, step_duration[i]-step_duration[i-1]);
            if (i) total_time += step_duration[i]-step_duration[i-1];
        }
    
        fprintf(stderr, "total time was %u ms\n", total_time);
    }

    if (test_font_render) {
        if (offscreen_surface) {
            surface_desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
            surface_desc.pixelformat = DSPF_ARGB;
            DFBCHECK( second_surface->GetSize( second_surface, &surface_desc.width, &surface_desc.height) );
            DFBCHECK( dfb->CreateSurface( dfb, &surface_desc, &draw_string_thread_args.surface));
            draw_string_thread_args.front_surface = second_surface;
        }
        else {
            draw_string_thread_args.front_surface = NULL;
            draw_string_thread_args.surface = second_surface;
        }

        draw_string_thread_args.nb_fonts = sizeof(font_heights)/sizeof(font_heights[0]);

        if ((draw_string_thread_args.fonts = malloc(draw_string_thread_args.nb_fonts+2*sizeof(*draw_string_thread_args.fonts))) == NULL) {
            D_ERROR("Could not malloc %s\n", strerror(errno));
            ret = EXIT_FAILURE;
        }

        for (i = 0; i < draw_string_thread_args.nb_fonts; i++) {
            font_desc.flags = DFDESC_HEIGHT;
            font_desc.height = font_heights[i];
            DFBCHECK(dfb->CreateFont( dfb, font_name, &font_desc, &draw_string_thread_args.fonts[i]));
        }

        draw_string_thread_args.fonts[3] = draw_string_thread_args.fonts[1];
        draw_string_thread_args.fonts[4] = draw_string_thread_args.fonts[0];

        draw_string_thread_args.nb_strings = sizeof(strings)/sizeof(strings[0][0])/MAX_STRING_LENGTH;

        if ((draw_string_thread_args.strings = malloc(draw_string_thread_args.nb_strings*sizeof(*draw_string_thread_args.strings))) == NULL) {
            D_ERROR("Could not malloc %s\n", strerror(errno));
            ret = EXIT_FAILURE;
        }

        for (i = 0; i < draw_string_thread_args.nb_strings; i++) {
            draw_string_thread_args.strings[i] = &strings[i];
        }

        if ((draw_string_thread_args.x = malloc(draw_string_thread_args.nb_strings*sizeof(*draw_string_thread_args.x))) == NULL) {
            D_ERROR("Could not malloc %s\n", strerror(errno));
            ret = EXIT_FAILURE;
        }

        fprintf(stderr, "ccc%d\n", draw_string_thread_args.nb_strings*sizeof(*draw_string_thread_args.y));

        if ((draw_string_thread_args.y = malloc(draw_string_thread_args.nb_strings*sizeof(*draw_string_thread_args.y))) == NULL) {
            D_ERROR("Could not malloc %s\n", strerror(errno));
            ret = EXIT_FAILURE;
        }

       /* We might eventually want to set attributes ... */
        if ((rc = pthread_attr_init(&draw_string_thread_attr))) {
            D_ERROR("Could not pthread_attr_init %s\n", strerror(rc));
            ret = EXIT_FAILURE;
            goto out;
        }
        
        if ((rc = pthread_create(&draw_string_thread, &draw_string_thread_attr, draw_string_thread_routine, &draw_string_thread_args))) {
            D_ERROR("Could not pthread_create %s\n", strerror(rc));
            ret = EXIT_FAILURE;
            goto out;
        }

        D_INFO("-------- Press key:\n"
                "-------- q to quit\n"
                "-------- g to start animation\n"
                "-------- s to stop animation\n");

        while(!stop_input_event) {
            switch (getchar()) {
                case 'a':
                    draw_string_thread_args.input_cmd = CMD_MOVE_RIGHT_ANIMATION;
                break;
                case 'p':
                    draw_string_thread_args.input_cmd = CMD_PAUSE_ANIMATION;
                break;
                case 'd':
                    draw_string_thread_args.input_cmd = CMD_MOVE_LEFT_ANIMATION;
                break;
                case 's':
                    stop_input_event = true;
                break;
                default:
                    /* get new event */
                break;
            }
        }

        draw_string_thread_args.cancelled = true;
    
        if ((rc = pthread_join(draw_string_thread, NULL))) {
            D_ERROR("Could not pthread_join %s\n", strerror(rc));
            ret = EXIT_FAILURE;
            goto out;
        }

        for (i = 0; i < draw_string_thread_args.nb_fonts; i++) {
            DFBCHECK(draw_string_thread_args.fonts[i]->Release( draw_string_thread_args.fonts[i] ));
        }
        
        free(draw_string_thread_args.fonts);
        free(draw_string_thread_args.strings);
        free(draw_string_thread_args.x);
        free(draw_string_thread_args.y);
    }

    fprintf(stderr, "press any key to quit\n");

    getchar();

    if (video_running) {
        playbackscagat_stop();
    }

out:

    fprintf(stderr, "Releasing image_surface\n");
    if (image_surface) DFBCHECK(image_surface->Release( image_surface ));
    fprintf(stderr, "Releasing second_surface\n");
    if (second_surface) DFBCHECK(second_surface->Release( second_surface ));
    fprintf(stderr, "Releasing first_surface\n");
    if (first_surface) DFBCHECK(first_surface->Release( first_surface ));
    fprintf(stderr, "Releasing second_layer\n");
    if (second_layer) DFBCHECK(second_layer->Release( second_layer ));
    fprintf(stderr, "Releasing second_layer\n");
    if (first_layer) DFBCHECK(first_layer->Release( first_layer ));
    /* ignore berr for now??? */
    fprintf(stderr, "Releasing first_dfb_ext\n");
    if (first_dfb_ext) {
        if ((berr = bdvd_dfb_ext_layer_close(first_dfb_ext)) != bdvd_ok) {
            D_ERROR("Could not bdvd_dfb_ext_layer_close\n");
            ret = EXIT_FAILURE;
        }
    }
    fprintf(stderr, "Releasing second_dfb_ext\n");
    if (second_dfb_ext) {
        if ((berr = bdvd_dfb_ext_layer_close(second_dfb_ext)) != bdvd_ok) {
            D_ERROR("Could not bdvd_dfb_ext_layer_close\n");
            ret = EXIT_FAILURE;
        }
    }
    fprintf(stderr, "Releasing dfb\n");
    if (dfb) {
        if ((berr = bdvd_dfb_ext_close()) != bdvd_ok) {
                D_ERROR("Could not bdvd_dfb_ext_close\n");
                ret = EXIT_FAILURE;
        }

        fprintf(stderr, "Releasing the DirectFB context\n");
        DFBCHECK(dfb->Release( dfb ));
    }

    fprintf(stderr, "Releasing the display\n");
    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

    return ret;
}
