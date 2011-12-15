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
    uint32_t i;

    fprintf(stderr, "usage: %s\n", program_name);

    fprintf(stderr, "<-a>, set the HDMV Presentation layer pixelformat\n");
    fprintf(stderr, "      (defaults to LUT8)\n");
    fprintf(stderr, "<-b>, set the HDMV Interactive layer pixelformat\n");
    fprintf(stderr, "      (defaults to LUT8)\n");
    fprintf(stderr, "<-c>, set the OSD layer pixelformat\n");
    fprintf(stderr, "      (defaults to ARGB)\n");

    fprintf(stderr, "<-d>, set the HDMV Presentation layer width\n");
    fprintf(stderr, "      (defaults to current screen width)\n");
    fprintf(stderr, "<-e>, set the HDMV Interactive layer width\n");
    fprintf(stderr, "      (defaults to current screen width)\n");
    fprintf(stderr, "<-f>, set the OSD layer width\n");
    fprintf(stderr, "      (defaults to current screen width)\n");

    fprintf(stderr, "<-g>, set the HDMV Presentation layer height\n");
    fprintf(stderr, "      (defaults to current screen height)\n");
    fprintf(stderr, "<-h>, set the HDMV Interactive layer height\n");
    fprintf(stderr, "      (defaults to current screen height)\n");
    fprintf(stderr, "<-i>, set the OSD layer height\n");
    fprintf(stderr, "      (defaults to current screen height)\n");

    fprintf(stderr, "TODO DESCRIBE -m -n -p -o -t!!!\n");

    fprintf(stderr, "<-x>, set the HDMV Presentation layer level (z order)\n");
    fprintf(stderr, "      (defaults to BD-ROM Part 3 specification, figure 8-3 : 1)\n");
    fprintf(stderr, "<-y>, set the HDMV Interactive layer level (z order)\n");
    fprintf(stderr, "      (defaults to BD-ROM Part 3 specification, figure 8-3 : 2)\n");
    fprintf(stderr, "<-z>, set the OSD layer level (z order)\n");
    fprintf(stderr, "      (defaults to top most level : 3)\n");

    fprintf(stderr, "Supported pixelformat are:\n");
    for (i = 0; i < nb_format_strings(); i++) {
        fprintf(stderr, "%s, ", format_strings[i].string);
    }
    fprintf(stderr, "\n");

}

typedef enum {
    PG_LAYER_INDEX = 0,
    IG_LAYER_INDEX = 1,
    OSD_LAYER_INDEX = 2
} HDMVTestLayerIds;

#define NB_HDMV_LAYERS (OSD_LAYER_INDEX+1)

typedef enum {
    SOURCE_FORMAT_NTSC = 0,
    SOURCE_FORMAT_720p,
    SOURCE_FORMAT_1080i
} HDMVSourceFormats;

typedef enum {
    VIDEO_MODE_S_NTSC_O_NTSC = 0,
    VIDEO_MODE_S_720p_O_720p,
    VIDEO_MODE_S_1080i_O_1080i,
    VIDEO_MODE_S_1080i_O_NTSC
} HDMVVideoModes;

#define NB_HDMV_VIDEO_MODES (VIDEO_MODE_S_1080i_O_NTSC+1)

typedef enum {
    TESTPATTERN_RANDOM = 0,
    TESTPATTERN_E2E_CROSSHAIR,
    TESTPATTERN_E2E_SCANLINE
} HDMVTestPattern;

typedef enum {
    TEST_CASE_ALL = 0,
    TEST_CASE_GROUPA_ALL,
    TEST_CASE_GROUPA_VIDMODEA,
    TEST_CASE_GROUPA_VIDMODEB,
    TEST_CASE_GROUPA_VIDMODEC,
    TEST_CASE_GROUPA_VIDMODED,
    TEST_CASE_GROUPB_ALL,
    TEST_CASE_GROUPB_VIDMODEA,
    TEST_CASE_GROUPB_VIDMODEB,
    TEST_CASE_GROUPB_VIDMODEC,
    TEST_CASE_GROUPB_VIDMODED,
    TEST_CASE_GROUPC_ALL,
    TEST_CASE_GROUPC_VIDMODEA,
    TEST_CASE_GROUPC_VIDMODEB,
    TEST_CASE_GROUPC_VIDMODEC,
    TEST_CASE_GROUPC_VIDMODED
} HDMVTestCases;

#if NB_HDMV_LAYERS > (DFB_DISPLAYLAYER_IDS_MAX-1)
#error "exceeds the number of layers available"
#endif

typedef struct {
    IDirectFB * dfb;
    IDirectFBSurface * dest;
    IDirectFBSurface * source;
    DFBRectangle * source_rect;
    DFBRectangle * dest_rect;

    bool cancelled;

    uint32_t mdelay;
    bool partial_flip;
    HDMVTestPattern test_pattern;

    bool primary_double_buffered;

    /* Required for calculate_bandwidth */
    uint32_t start_time;

    /* Results */
    uint32_t nb_msecs;
    uint32_t nb_compositions;
    uint32_t compositions_sec;
    uint32_t mpixels_sec;
    
    wakeup_call_t * input_event;
} blit_routine_arguments_t;

typedef struct {
    IDirectFB * dfb;
    bdvd_display_h bdisplay;
    wakeup_call_t * input_event;
    bvideo_format video_format;
} display_set_routine_arguments_t;

/* When this thing dies, the fcts of this class should responds an error */
void * blit_thread_routine(void * arg) {

    blit_routine_arguments_t * blit_args = (blit_routine_arguments_t *)arg;

    DFBRegion       movement_region;
    DFBRegion       flip_region;
    int             dx = 0;
    int             dy = 0;
    DFBRectangle    current_rect;
    DFBRectangle    union_rect;

    DFBResult err;
    int rc;

    D_ASSERT(blit_args != NULL);

    movement_region.x1 = 0;
    movement_region.y1 = 0;
    movement_region.x2 = blit_args->dest_rect->w > blit_args->source_rect->w ? blit_args->dest_rect->w - blit_args->source_rect->w : 1;
    movement_region.y2 = blit_args->dest_rect->h > blit_args->source_rect->h ? blit_args->dest_rect->h - blit_args->source_rect->h : 1;

    blit_args->nb_compositions = 0;
    blit_args->mpixels_sec = 0;
    blit_args->nb_msecs = 0;

    /* Those two will never change */
    current_rect.w = blit_args->source_rect->w;
    current_rect.h = blit_args->source_rect->h;

    if ((rc = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL))) {
        D_ERROR("blit_thread_routine: error in pthread_setcanceltype %s\n", strerror(rc));
        return NULL;
    }

    if ((err = blit_args->dest->SetBlittingFlags(
                  blit_args->dest, 
                      DSBLIT_NOFX )) != DFB_OK ) {
            D_DERROR(err, "blit_thread_routine: error in SetBlittingFlags\n");
            return NULL;
        }
    
    if ((err = blit_args->dest->SetDrawingFlags(
                  blit_args->dest, 
                      DSDRAW_NOFX )) != DFB_OK ) {
            D_DERROR(err, "blit_thread_routine: error in SetDrawingFlags\n");
            return NULL;
        }
        
    if ((err = blit_args->dest->SetColor(blit_args->dest, 0, 0, 0, 0)) != DFB_OK ) {
            D_DERROR(err, "blit_thread_routine: error in SetColor\n");
            return NULL;
        }

    blit_args->start_time = myclock();

    while (!blit_args->cancelled) {

            union_rect.x = dx;
            union_rect.y = dy;
        union_rect.w = blit_args->source_rect->w;
        union_rect.h = blit_args->source_rect->h;

        if ((err = blit_args->dest->FillRectangle(
                        blit_args->dest,
                            union_rect.x, union_rect.y, union_rect.w, union_rect.h)) != DFB_OK ) {
                D_DERROR(err, "blit_thread_routine: error in Blit\n");
                return NULL;
            }

            switch (blit_args->test_pattern) {
                case TESTPATTERN_E2E_CROSSHAIR:
                    movement_edgetoedge_crosshair(
                        &dx, &dy,
                        &movement_region);
                break;
                default: /* TESTPATTERN_RANDOM */
                dx = myrand()%(movement_region.x2);
                dy = myrand()%(movement_region.y2);
                break;
            }

        current_rect.x = dx;
        current_rect.y = dy;

        if ((err = blit_args->dest->Blit(
                        blit_args->dest,
                        blit_args->source,
                        blit_args->source_rect,
                        current_rect.x, current_rect.y)) != DFB_OK ) {
            D_DERROR(err, "blit_thread_routine: error in Blit\n");
            return NULL;
        }

        if (blit_args->partial_flip) {
            dfb_rectangle_union(&union_rect, &current_rect);
    
            flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&union_rect);
        }

        if ((err = blit_args->dest->Flip( blit_args->dest, blit_args->partial_flip ? &flip_region : NULL, 0 )) != DFB_OK ) {
                D_DERROR(err, "blit_thread_routine: error in Flip\n");
                return NULL;
        }

        if (!blit_args->primary_double_buffered) {        
            blit_args->dfb->WaitIdle( blit_args->dfb );
        }

        blit_args->nb_compositions++;
        
        if (blit_args->mdelay) {
            mymsleep(blit_args->mdelay);
        }
        else {
            sched_yield();
        }
    }

    /* EngineSync does not work very well */
    blit_args->dfb->WaitIdle( blit_args->dfb );

    blit_args->nb_msecs = myclock() - blit_args->start_time;

    blit_args->compositions_sec = blit_args->nb_msecs ?
                                    blit_args->nb_compositions*1000/blit_args->nb_msecs : 0;

    blit_args->mpixels_sec = blit_args->nb_msecs ?
                                blit_args->source_rect->w*blit_args->source_rect->h*blit_args->nb_compositions*1000/blit_args->nb_msecs : 0;
    return NULL;
}

/* When this thing dies, the fcts of this class should responds an error */
void * display_set_thread_routine(void * arg) {
    display_set_routine_arguments_t * display_args = (display_set_routine_arguments_t *)arg;

    DFBResult err;

    while (wait_wakeup_call(display_args->input_event) == DFB_OK) {
        /* Setting output format */
        DFBCHECK( display_set_video_format(display_args->bdisplay, display_args->video_format) );
        switch (display_args->video_format) {
            case bvideo_format_ntsc:
                D_INFO( "after display_set_video_format to bvideo_format_ntsc\n");
                DFBCHECK( display_args->dfb->SetVideoMode( display_args->dfb, 720, 480, 32 ) );
                D_INFO( "after SetVideoMode to 720, 480, 32\n");
            break;
            case bvideo_format_720p:
                D_INFO( "after display_set_video_format to bvideo_format_720p\n");
                DFBCHECK( display_args->dfb->SetVideoMode( display_args->dfb, 1280, 720, 32 ) );
                D_INFO( "after SetVideoMode to 1280, 720, 32\n");
            break;
            case bvideo_format_1080i:
                D_INFO( "after display_set_video_format to bvideo_format_1080i\n");
                DFBCHECK( display_args->dfb->SetVideoMode( display_args->dfb, 1920, 1080, 32 ) );
                D_INFO( "after SetVideoMode to 1920, 1080, 32\n");
            break;
            default:
            break;
        }
    }

    return NULL;
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
     bool primary_layer_config_flag = false;
     bool primary_double_buffered = true;

     /* This may be set as a parameter as well */
     uint32_t nb_hdmv_layers = NB_HDMV_LAYERS;

     IDirectFBDisplayLayer   *hdmv_layers[NB_HDMV_LAYERS];
     IDirectFBSurface        *hdmv_layer_surfaces[NB_HDMV_LAYERS];
     DFBRectangle            hdmv_layer_surface_rects[NB_HDMV_LAYERS];
     DFBDisplayLayerConfig   hdmv_layer_configs[NB_HDMV_LAYERS];
     DFBLocation             hdmv_layer_locations[NB_HDMV_LAYERS];
     int                     hdmv_layer_levels[NB_HDMV_LAYERS] = {PG_LAYER_INDEX+1, IG_LAYER_INDEX+1, OSD_LAYER_INDEX+1};
     const char              video_mode_string[][80] = {"ntsc->ntsc","720p->720p","1080i->1080i", "1080i->ntsc"};
     const char              source_format_string[][80] = {"ntsc","720p","1080i"};
     char                    *filename_source_format[NB_HDMV_LAYERS];
     char                    *filename_pixelformat[NB_HDMV_LAYERS];
     char                    *filename_suffix[NB_HDMV_LAYERS];

     HDMVTestPattern         hdmv_layer_testpatterns[NB_HDMV_LAYERS] = {TESTPATTERN_RANDOM, TESTPATTERN_E2E_CROSSHAIR, TESTPATTERN_RANDOM};
     pthread_t               hdmv_threads[NB_HDMV_LAYERS];
     pthread_attr_t          hdmv_threads_attr[NB_HDMV_LAYERS];

     blit_routine_arguments_t  blit_routine_arguments[NB_HDMV_LAYERS];

     pthread_t               display_set_thread;
     pthread_attr_t          display_set_thread_attr;

     display_set_routine_arguments_t display_set_thread_arguments;

     wakeup_call_t           input_event;

     uint32_t mdelay = 0;

     char file_path[256];

     /* Images ??? */
     DFBSurfaceDescription   dsc;
     IDirectFBSurface        *layer1_object;
     DFBRectangle            layer1_object_rect;
     IDirectFBSurface        *layer2_object;
     DFBRectangle            layer2_object_rect;
     IDirectFBSurface        *layer3_object;
     DFBRectangle            layer3_object_rect;

     IDirectFBPalette        *palette;
     IDirectFBPalette        *duppalette;

     IDirectFBSurface        *release_surface;
     DFBRectangle            release_surface_rect;
     
     IDirectFBImageProvider  *provider;

     bool flag_partial_flip = true;
     
     bool stop_input_event = false;

     /* TBD */

     char c;
     uint32_t i;
     int rc;
     
     DFBRectangle    current_rect;
     DFBRectangle    union_rect;
     int xpos, ypos;
     
     HDMVSourceFormats source_format;
     HDMVTestCases test_case = TEST_CASE_ALL;
     
     HDMVVideoModes output_video_mode;
     
     HDMVVideoModes start_output_video_mode;
     uint32_t max_output_video_mode;
     
     DFBRegion flip_region;
     
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

     if (argc < 1) {
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
        goto out;
     }

     /* setting all the flags to false and config to 0 */
     memset(&primary_layer_config, 0, sizeof(primary_layer_config));

     /* setting all the flags to false and config to 0 */
     memset(hdmv_layer_configs, 0, sizeof(hdmv_layer_configs));

     /* setting all location to 0, 0, 1, 1 */
     memset(hdmv_layer_locations, 0, sizeof(hdmv_layer_locations));
     
     for (i = 0; i < NB_HDMV_LAYERS; i++) {
        hdmv_layer_locations[i].w = 1;
        hdmv_layer_locations[i].h = 1;
     }

     while ((c = getopt (argc, argv, "0:1:2:3:4:5:6:7:8:9:q:r:a:b:c:d:e:f:g:h:i:jklm:n:opt:u:v:w:x:y:z:")) != -1) {
        switch (c) {
        case '0':
        case '1':
        case '2':
            hdmv_layer_locations[c-'0'].x = strtof(optarg, NULL);
        break;
        case '3':
        case '4':
        case '5':
            hdmv_layer_locations[c-'3'].y = strtof(optarg, NULL);
        break;
        case '6':
        case '7':
        case '8':
            hdmv_layer_locations[c-'6'].w = strtof(optarg, NULL);
        break;
        case '9':
            hdmv_layer_locations[0].h = strtof(optarg, NULL);
        break;
        case 'q':
        case 'r':
            hdmv_layer_locations[c-'q'+1].h = strtof(optarg, NULL);
        break;
        case 'a':
        case 'b':
        case 'c':
            hdmv_layer_configs[c-'a'].flags |= DLCONF_PIXELFORMAT;
            if ((hdmv_layer_configs[c-'a'].pixelformat = convert_string2dspf(optarg)) == DSPF_UNKNOWN) {
                D_ERROR("Unknown pixel format\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'd':
        case 'e':
        case 'f':
            hdmv_layer_configs[c-'d'].flags |= DLCONF_WIDTH;
            hdmv_layer_configs[c-'d'].width = strtoul(optarg, NULL, 10);
        break;
        case 'g':
        case 'h':
        case 'i':
            hdmv_layer_configs[c-'g'].flags |= DLCONF_HEIGHT;
            hdmv_layer_configs[c-'g'].height = strtoul(optarg, NULL, 10);
        break;
        case 'j':
        case 'k':
        case 'l':
            hdmv_layer_configs[c-'j'].flags |= DLCONF_BUFFERMODE;
            hdmv_layer_configs[c-'j'].buffermode = DLBM_FRONTONLY;
        break;
        case 'm':
            test_case = strtoul(optarg, NULL, 10);
        break;
        case 'n':
            nb_hdmv_layers = strtoul(optarg, NULL, 10);
        break;
        case 'o':
            flag_partial_flip = false;
        break;
        case 'p':
            primary_layer_config_flag = true;
            primary_layer_config.flags |= DLCONF_BUFFERMODE;
            primary_layer_config.buffermode = DLBM_FRONTONLY;
            primary_double_buffered = false;
        break;
        case 't':
            mdelay = strtoul(optarg, NULL, 10);
        break;
        case 'u':
        case 'v':
        case 'w':
            hdmv_layer_testpatterns[c-'u'] = strtoul(optarg, NULL, 10);
        case 'x':
        case 'y':
        case 'z':
            hdmv_layer_levels[c-'x'] = strtoul(optarg, NULL, 10);
        break;
        case '?':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        break;
        }
    }
    
    DFBCHECK( DirectFBInit(&argc, &argv) );

    if (setenv( "BCM_LAYER_1_NAME", "HDMV Presentation Graphics", 1 ) == -1) {
        D_PERROR("Could not set environment variable BCM_LAYER_1_NAME\n");
        exit(EXIT_FAILURE);
    }

    if (setenv( "BCM_LAYER_2_NAME", "HDMV Interactive Graphics", 1 ) == -1) {
        D_PERROR("Could not set environment variable BCM_LAYER_1_NAME\n");
        exit(EXIT_FAILURE);
    }

    if (setenv( "BCM_LAYER_3_NAME", "OSD", 1 ) == -1) {
        D_PERROR("Could not set environment variable BCM_LAYER_1_NAME\n");
        exit(EXIT_FAILURE);
    }

    DFBCHECK( DirectFBCreate(&dfb) );

#if 0
    /* This is needed to have access to SetVideoMode.
     * Bypassed that. */
    DFBCHECK( dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN) );
    
    Somehow puts the layer not REALIZED when this is done in addition to
    SetCooperativeLevel set at layer level.
#else
    /* It's necessary to obtain the primary layer, because of the
     * allocation of the layer context done beneath by directfb on GetLayer.
     */
    DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primary_layer) );
    DFBCHECK( primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) );
    if (primary_layer_config_flag) {
        DFBCHECK( primary_layer->SetConfiguration(primary_layer, &primary_layer_config) );
    }
    DFBCHECK( primary_layer->SetBackgroundColor(primary_layer, 0, 0, 0, 0 ) );
    DFBCHECK( primary_layer->SetBackgroundMode(primary_layer, DLBM_COLOR ) );
#endif

    D_INFO("-------- Enumerating display layers --------\n");
    DFBCHECK( dfb->EnumDisplayLayers( dfb, enum_layers_callback, NULL ) );

    /* Let's set the layer configuration if required.
     */
    for (i = 0; i < nb_hdmv_layers; i++) {
        DFBCHECK( dfb->GetDisplayLayer(dfb, i+1, &hdmv_layers[i]) );
        DFBCHECK( hdmv_layers[i]->SetCooperativeLevel(hdmv_layers[i], DLSCL_ADMINISTRATIVE) );
        DFBCHECK( hdmv_layers[i]->SetBackgroundColor(hdmv_layers[i], 0, 0, 0, 0 ) );
        DFBCHECK( hdmv_layers[i]->SetBackgroundMode(hdmv_layers[i], DLBM_COLOR ) );
    }

    D_INFO("-------- Creating release surface --------\n");

    /* We don't want this rectangle to actually be blitted,
     * it's just to unref potential source surfaces from the
     * layers surfaces.
     */
    release_surface_rect.x = 0;
    release_surface_rect.y = 0;
    release_surface_rect.w = 10;
    release_surface_rect.h = 10;

    dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
    dsc.caps = 0;
    dsc.width = 10;
    dsc.height = 10;
    dsc.pixelformat = DSPF_ARGB;
    DFBCHECK( dfb->CreateSurface( dfb, &dsc, &release_surface ) );
        
    D_INFO("--- TEST GROUP A: LAYERS BLENDING ---\n");

    switch(test_case) {
        case TEST_CASE_ALL:
        case TEST_CASE_GROUPA_ALL:
            start_output_video_mode = 0;
            max_output_video_mode = NB_HDMV_VIDEO_MODES;
        break;
        case TEST_CASE_GROUPA_VIDMODEA:
            start_output_video_mode = VIDEO_MODE_S_NTSC_O_NTSC;
            max_output_video_mode = VIDEO_MODE_S_NTSC_O_NTSC+1;
        break;    
        case TEST_CASE_GROUPA_VIDMODEB:
            start_output_video_mode = VIDEO_MODE_S_720p_O_720p;
            max_output_video_mode = VIDEO_MODE_S_720p_O_720p+1;
        break;    
        case TEST_CASE_GROUPA_VIDMODEC:
            start_output_video_mode = VIDEO_MODE_S_1080i_O_1080i;
            max_output_video_mode = VIDEO_MODE_S_1080i_O_1080i+1;
        break;    
        case TEST_CASE_GROUPA_VIDMODED:
            start_output_video_mode = VIDEO_MODE_S_1080i_O_NTSC;
            max_output_video_mode = VIDEO_MODE_S_1080i_O_NTSC+1;
        break;
        default:
            start_output_video_mode = 0;
            max_output_video_mode = 0;
        break;
    }
    
    for (output_video_mode = start_output_video_mode; output_video_mode < max_output_video_mode; output_video_mode++) {
        D_INFO("--- %s video output ---\n", video_mode_string[output_video_mode]);
        
        switch (output_video_mode) {
            case VIDEO_MODE_S_NTSC_O_NTSC:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_ntsc) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_NTSC;
                
                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "argb";
                filename_suffix[PG_LAYER_INDEX] = "";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 720;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 480;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 720;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 480;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[OSD_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[OSD_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[OSD_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[OSD_LAYER_INDEX].width = 720;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].height = 480;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
            break;
            case VIDEO_MODE_S_720p_O_720p:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_720p) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 1280, 720, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_720p;
                
                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "argb";
                filename_suffix[PG_LAYER_INDEX] = "";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 1280;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 720;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 1280;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 720;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[OSD_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[OSD_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[OSD_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[OSD_LAYER_INDEX].width = 1280;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].height = 720;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
            break;
            case VIDEO_MODE_S_1080i_O_1080i:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_1080i) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_1080i;
                
                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "argb";
                filename_suffix[PG_LAYER_INDEX] = "";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 1920;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 1080;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {                
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 1920;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 1080;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[OSD_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[OSD_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[OSD_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[OSD_LAYER_INDEX].width = 1920;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].height = 1080;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
            break;
            case VIDEO_MODE_S_1080i_O_NTSC:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_1080i) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_NTSC;
                
                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "argb";
                filename_suffix[PG_LAYER_INDEX] = "";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 1920;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 1080;
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 1920;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 1080;
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
                filename_source_format[OSD_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[OSD_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[OSD_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[OSD_LAYER_INDEX].width = 1920;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].height = 1080;
                }
                if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[OSD_LAYER_INDEX].pixelformat = DSPF_ARGB;
                }
            break;
            default:
                D_DEBUG("this should not be happening");
                exit(EXIT_FAILURE);
            break;
        }

        D_INFO("------ Setting the layers configurations ------\n");

        /* Setting the layer configuration, and getting the layer surfaces
         */
        for (i = 0; i < nb_hdmv_layers; i++) {
            DFBCHECK( hdmv_layers[i]->SetConfiguration(hdmv_layers[i], &hdmv_layer_configs[i]) );

            DFBCHECK( hdmv_layers[i]->GetSurface(hdmv_layers[i], &hdmv_layer_surfaces[i]) );
            
            DFBCHECK( hdmv_layer_surfaces[i]->GetSize(hdmv_layer_surfaces[i], &hdmv_layer_surface_rects[i].w, &hdmv_layer_surface_rects[i].h) );
            hdmv_layer_surface_rects[i].x = hdmv_layer_surface_rects[i].y = 0;
            D_INFO( "hdmv_layer_surface_rects[%u]: x[%d], y[%d], w[%d], h[%d]\n",
                    i,
                    hdmv_layer_surface_rects[i].x,
                    hdmv_layer_surface_rects[i].y,
                    hdmv_layer_surface_rects[i].w,
                    hdmv_layer_surface_rects[i].h);
            DFBCHECK( hdmv_layers[i]->SetScreenLocation(hdmv_layers[i],
                        hdmv_layer_locations[i].x, hdmv_layer_locations[i].y,
                        hdmv_layer_locations[i].w, hdmv_layer_locations[i].h) );
            D_INFO( "hdmv_layer_locations[%u]: x[%f], y[%f], w[%f], h[%f]\n",
                    i,
                    hdmv_layer_locations[i].x,
                    hdmv_layer_locations[i].y,
                    hdmv_layer_locations[i].w,
                    hdmv_layer_locations[i].h);
        }

        D_INFO("------ loading and rendering images on intermediate surfaces ------\n");

        if (nb_hdmv_layers > PG_LAYER_INDEX) {

            snprintf(
                file_path,
                sizeof(file_path),
                DATADIR"/images/%s_layer1_%s%s.png",
                filename_source_format[PG_LAYER_INDEX],
                filename_pixelformat[PG_LAYER_INDEX],
                filename_suffix[PG_LAYER_INDEX]
                );
    
            D_INFO("-------- loading %s as layer1_object --------\n", file_path);
    
            DFBCHECK(dfb->CreateImageProvider( dfb, file_path, &provider ));
    
            DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
        
            layer1_object_rect.x = 0;
            layer1_object_rect.y = 0;
            layer1_object_rect.w = dsc.width;
            layer1_object_rect.h = dsc.height;
            D_INFO( "layer1_object_rect: x[%d], y[%d], w[%d], h[%d]\n",
                    layer1_object_rect.x,
                    layer1_object_rect.y,
                    layer1_object_rect.w,
                    layer1_object_rect.h);
        
            DFBCHECK(dfb->CreateSurface( dfb, &dsc, &layer1_object ));
        
            DFBCHECK(provider->RenderTo( provider, layer1_object, NULL ));
            provider->Release( provider );
        
        }

        if (nb_hdmv_layers > IG_LAYER_INDEX) {
    
            snprintf(
                file_path,
                sizeof(file_path),
                DATADIR"/imagesexamples/%s_layer2_%s%s.png",
                filename_source_format[IG_LAYER_INDEX],
                filename_pixelformat[IG_LAYER_INDEX],
                filename_suffix[IG_LAYER_INDEX]
                );
    
            D_INFO("-------- loading %s as layer2_object --------\n", file_path);
        
            DFBCHECK(dfb->CreateImageProvider( dfb, file_path, &provider ));
        
            DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
    
            layer2_object_rect.x = 0;
            layer2_object_rect.y = 0;
            layer2_object_rect.w = dsc.width;
            layer2_object_rect.h = dsc.height;
            D_INFO( "layer2_object_rect: x[%d], y[%d], w[%d], h[%d]\n",
                    layer2_object_rect.x,
                    layer2_object_rect.y,
                    layer2_object_rect.w,
                    layer2_object_rect.h );
        
            DFBCHECK(dfb->CreateSurface( dfb, &dsc, &layer2_object ));
    
            DFBCHECK(provider->RenderTo( provider, layer2_object, NULL ));
            provider->Release( provider );

        }
    
        if (nb_hdmv_layers > OSD_LAYER_INDEX) {
    
            snprintf(
                file_path,
                sizeof(file_path),
                DATADIR"/images/%s_layer3_%s%s.png",
                filename_source_format[OSD_LAYER_INDEX],
                filename_pixelformat[OSD_LAYER_INDEX],
                filename_suffix[OSD_LAYER_INDEX]
                );
        
            D_INFO("-------- loading %s as layer3_object --------\n", file_path);
        
            DFBCHECK(dfb->CreateImageProvider( dfb, file_path, &provider ));
        
            DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
            
            layer3_object_rect.x = 0;
            layer3_object_rect.y = 0;
            layer3_object_rect.w = dsc.width;
            layer3_object_rect.h = dsc.height;
            D_INFO( "layer3_object_rect: x[%d], y[%d], w[%d], h[%d]\n",
                    layer3_object_rect.x,
                    layer3_object_rect.y,
                    layer3_object_rect.w,
                    layer3_object_rect.h);
        
            DFBCHECK(dfb->CreateSurface( dfb, &dsc, &layer3_object ));
        
            DFBCHECK(provider->RenderTo( provider, layer3_object, NULL ));
            provider->Release( provider );

        }

        if (nb_hdmv_layers > PG_LAYER_INDEX) {

            D_INFO("-------- Blitting layer1_object onto HDMV Presentation layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[PG_LAYER_INDEX]->SetBlittingFlags(
                      hdmv_layer_surfaces[PG_LAYER_INDEX], 
                      DSBLIT_NOFX ) );
    
            DFBCHECK( hdmv_layer_surfaces[PG_LAYER_INDEX]->Blit(
                      hdmv_layer_surfaces[PG_LAYER_INDEX],
                      layer1_object,
                      &layer1_object_rect,
                      0,0) );
    
            D_INFO("-------- Setting level %d for HDMV Presentation layer --------\n", hdmv_layer_levels[PG_LAYER_INDEX]);
    
            DFBCHECK( hdmv_layers[PG_LAYER_INDEX]->SetLevel(hdmv_layers[PG_LAYER_INDEX], hdmv_layer_levels[PG_LAYER_INDEX]) );
    
            D_INFO("-------- Flipping from HDMV Presentation layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[PG_LAYER_INDEX]->Flip( hdmv_layer_surfaces[PG_LAYER_INDEX], NULL, 0 ) );
        
            getchar();

        }

        if (nb_hdmv_layers > IG_LAYER_INDEX) {
    
            D_INFO("-------- Blitting layer2_object onto HDMV Interactive layer surface --------\n");
        
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->SetBlittingFlags(
                                hdmv_layer_surfaces[IG_LAYER_INDEX],
                                DSBLIT_NOFX ) );
        
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Blit(
                                hdmv_layer_surfaces[IG_LAYER_INDEX],
                                layer2_object,
                                &layer2_object_rect,
                                0,0) );
        
            D_INFO("-------- Setting level %d for HDMV Interactive layer --------\n", hdmv_layer_levels[IG_LAYER_INDEX]);
    
            DFBCHECK( hdmv_layers[IG_LAYER_INDEX]->SetLevel( hdmv_layers[IG_LAYER_INDEX], hdmv_layer_levels[IG_LAYER_INDEX]) );
    
            D_INFO("-------- Flipping from HDMV Interactive layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Flip( hdmv_layer_surfaces[IG_LAYER_INDEX], NULL, 0 ) );
    
            getchar();

        }

        if (nb_hdmv_layers > OSD_LAYER_INDEX) {

            D_INFO("-------- Blitting layer3_object onto OSD layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[OSD_LAYER_INDEX]->SetBlittingFlags(
                                hdmv_layer_surfaces[OSD_LAYER_INDEX],
                                DSBLIT_NOFX ) );
    
            DFBCHECK( hdmv_layer_surfaces[OSD_LAYER_INDEX]->Blit(
                                hdmv_layer_surfaces[OSD_LAYER_INDEX],
                                layer3_object,
                                &layer3_object_rect,
                                0,0) );
        
            D_INFO("-------- Setting level %d for OSD layer --------\n", hdmv_layer_levels[OSD_LAYER_INDEX]);
    
            DFBCHECK( hdmv_layers[OSD_LAYER_INDEX]->SetLevel(hdmv_layers[OSD_LAYER_INDEX], hdmv_layer_levels[OSD_LAYER_INDEX]) );
    
            D_INFO("-------- Flipping from OSD layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[OSD_LAYER_INDEX]->Flip( hdmv_layer_surfaces[OSD_LAYER_INDEX], NULL, 0 ) );

            getchar();

        }

        /* Hide all the layers until next pass */
        for (i = 0; i < nb_hdmv_layers; i++) {
            /* setting all the flags to false and config to 0 */
            memset(&hdmv_layer_configs[i], 0, sizeof(hdmv_layer_configs[i]));

            DFBCHECK( hdmv_layers[i]->SetLevel(hdmv_layers[i], -1 ) );
            /* We must release the layer objects from the layer surface
             * state, this would unref the objects to permit deallocation.
             * Must find another way to do this!
             */
            DFBCHECK( hdmv_layer_surfaces[i]->Blit(
                      hdmv_layer_surfaces[i],
                      release_surface,
                      &release_surface_rect,
                      0,0) );
        }

        /* Destroy the composition objects surfaces */
        if (nb_hdmv_layers > PG_LAYER_INDEX)
            layer1_object->Release( layer1_object );

        if (nb_hdmv_layers > IG_LAYER_INDEX)
            layer2_object->Release( layer2_object );

        if (nb_hdmv_layers > OSD_LAYER_INDEX)
            layer3_object->Release( layer3_object );
    }

    D_INFO("--- TEST GROUP B: COMPOSITION OBJECTS TYPICAL ---\n");

    switch(test_case) {
        case TEST_CASE_ALL:
        case TEST_CASE_GROUPB_ALL:
            start_output_video_mode = 0;
            max_output_video_mode = NB_HDMV_VIDEO_MODES;
        break;
        case TEST_CASE_GROUPB_VIDMODEA:
            start_output_video_mode = VIDEO_MODE_S_NTSC_O_NTSC;
            max_output_video_mode = VIDEO_MODE_S_NTSC_O_NTSC+1;
        break;    
        case TEST_CASE_GROUPB_VIDMODEB:
            start_output_video_mode = VIDEO_MODE_S_720p_O_720p;
            max_output_video_mode = VIDEO_MODE_S_720p_O_720p+1;
        break;    
        case TEST_CASE_GROUPB_VIDMODEC:
            start_output_video_mode = VIDEO_MODE_S_1080i_O_1080i;
            max_output_video_mode = VIDEO_MODE_S_1080i_O_1080i+1;
        break;    
        case TEST_CASE_GROUPB_VIDMODED:
            start_output_video_mode = VIDEO_MODE_S_1080i_O_NTSC;
            max_output_video_mode = VIDEO_MODE_S_1080i_O_NTSC+1;
        break;
        default:
            start_output_video_mode = 0;
            max_output_video_mode = 0;
        break;
    }
    
    for (output_video_mode = start_output_video_mode; output_video_mode < max_output_video_mode; output_video_mode++) {
        D_INFO("--- %s video output ---\n", video_mode_string[output_video_mode]);
        
        switch (output_video_mode) {
            case VIDEO_MODE_S_NTSC_O_NTSC:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_ntsc) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_NTSC;
                
                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "lut8";
                filename_suffix[PG_LAYER_INDEX] = "_nodither";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 720; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 480; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 720; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 480; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
            break;
            case VIDEO_MODE_S_720p_O_720p:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_720p) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 1280, 720, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_720p;

                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "lut8";
                filename_suffix[PG_LAYER_INDEX] = "_half_nodither";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 640; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 360; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 640; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 360; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
            break;
            case VIDEO_MODE_S_1080i_O_1080i:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_1080i) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_1080i;

                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "lut8";
                filename_suffix[PG_LAYER_INDEX] = "_half_nodither";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 960; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 540; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 960; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 540; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
            break;
            case VIDEO_MODE_S_1080i_O_NTSC:
                /* Must set display format prior to calling SetVideoMode, else
                 * will fail.
                 */
                DFBCHECK( display_set_video_format(bdisplay, bvideo_format_1080i) );
                /* Setting output format */
                DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
                /* Setting source format */
                source_format = SOURCE_FORMAT_NTSC;
                
                filename_source_format[PG_LAYER_INDEX] = source_format_string[source_format];
                filename_pixelformat[PG_LAYER_INDEX] = "lut8";
                filename_suffix[PG_LAYER_INDEX] = "_half_nodither";
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[PG_LAYER_INDEX].width = 960; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[PG_LAYER_INDEX].height = 540; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[PG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[PG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[PG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
                filename_source_format[IG_LAYER_INDEX] = filename_source_format[PG_LAYER_INDEX];
                filename_pixelformat[IG_LAYER_INDEX] = filename_pixelformat[PG_LAYER_INDEX];
                filename_suffix[IG_LAYER_INDEX] = filename_suffix[PG_LAYER_INDEX];
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_WIDTH)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_WIDTH;
                    hdmv_layer_configs[IG_LAYER_INDEX].width = 960; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_HEIGHT;
                    hdmv_layer_configs[IG_LAYER_INDEX].height = 540; // As specified by Adam's spreadsheet.
                }
                if (!(hdmv_layer_configs[IG_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
                    hdmv_layer_configs[IG_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
                    hdmv_layer_configs[IG_LAYER_INDEX].pixelformat = DSPF_LUT8;
                }
            break;
            default:
                D_ERROR("this should not be happening");
                ret = EXIT_FAILURE;
                goto out;
            break;
        }

        /* For now, let's say the OSD is always ntsc in ARGB format */
        filename_source_format[OSD_LAYER_INDEX] = "ntsc";
        filename_pixelformat[OSD_LAYER_INDEX] = "argb";
        filename_suffix[OSD_LAYER_INDEX] = "";
        if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_WIDTH)) {
            hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_WIDTH;
            hdmv_layer_configs[OSD_LAYER_INDEX].width = 720;
        }
        if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_HEIGHT)) {
            hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_HEIGHT;
            hdmv_layer_configs[OSD_LAYER_INDEX].height = 480;
        }
        if (!(hdmv_layer_configs[OSD_LAYER_INDEX].flags & DLCONF_PIXELFORMAT)) {
            hdmv_layer_configs[OSD_LAYER_INDEX].flags |= DLCONF_PIXELFORMAT;
            hdmv_layer_configs[OSD_LAYER_INDEX].pixelformat = DSPF_ARGB;
        }

        D_INFO("------ Setting the layers configurations ------\n");

        /* Setting the layer configuration, and getting the layer surfaces
         */
        for (i = 0; i < nb_hdmv_layers; i++) {
            DFBCHECK( hdmv_layers[i]->SetConfiguration(hdmv_layers[i], &hdmv_layer_configs[i]) );

            DFBCHECK( hdmv_layers[i]->GetSurface(hdmv_layers[i], &hdmv_layer_surfaces[i]) );
            
            DFBCHECK( hdmv_layer_surfaces[i]->GetSize(hdmv_layer_surfaces[i], &hdmv_layer_surface_rects[i].w, &hdmv_layer_surface_rects[i].h) );
            hdmv_layer_surface_rects[i].x = hdmv_layer_surface_rects[i].y = 0;
            D_INFO( "hdmv_layer_surface_rects[%u]: x[%d], y[%d], w[%d], h[%d]\n",
                    i,
                    hdmv_layer_surface_rects[i].x,
                    hdmv_layer_surface_rects[i].y,
                    hdmv_layer_surface_rects[i].w,
                    hdmv_layer_surface_rects[i].h);
            DFBCHECK( hdmv_layers[i]->SetScreenLocation(hdmv_layers[i],
                        hdmv_layer_locations[i].x, hdmv_layer_locations[i].y,
                        hdmv_layer_locations[i].w, hdmv_layer_locations[i].h) );
            D_INFO( "hdmv_layer_locations[%u]: x[%f], y[%f], w[%f], h[%f]\n",
                    i,
                    hdmv_layer_locations[i].x,
                    hdmv_layer_locations[i].y,
                    hdmv_layer_locations[i].w,
                    hdmv_layer_locations[i].h);
        }

        D_INFO("------ loading and rendering images on intermediate surfaces ------\n");

        if (nb_hdmv_layers > PG_LAYER_INDEX) {

            snprintf(
                file_path,
                sizeof(file_path),
                DATADIR"/images/%s_antialiased_text_red_%s%s.png",
                filename_source_format[PG_LAYER_INDEX],
                filename_pixelformat[PG_LAYER_INDEX],
                filename_suffix[PG_LAYER_INDEX]
                );
    
            D_INFO("-------- loading %s as layer1_object --------\n", file_path);
    
            DFBCHECK(dfb->CreateImageProvider( dfb, file_path, &provider ));
    
            DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
        
            layer1_object_rect.x = 0;
            layer1_object_rect.y = 0;
            layer1_object_rect.w = dsc.width;
            layer1_object_rect.h = dsc.height;
            D_INFO( "layer1_object_rect: x[%d], y[%d], w[%d], h[%d]\n",
                    layer1_object_rect.x,
                    layer1_object_rect.y,
                    layer1_object_rect.w,
                    layer1_object_rect.h);
    
            /* Direct support for LUT8 in image providers seem to have been added in the new 0.9.24 release,
             * png are always ARGB format if an alpha channel, thus  for now as a work around
             * we let RenderTo do the actual convertion.
             */
            if (hdmv_layer_configs[PG_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                dsc.pixelformat = DSPF_LUT8;
            }
            else {
                dsc.pixelformat = DSPF_ARGB;
            }
        
            DFBCHECK(dfb->CreateSurface( dfb, &dsc, &layer1_object ));
    
            DFBCHECK(provider->RenderTo( provider, layer1_object, NULL ));
            provider->Release( provider );

            if (hdmv_layer_configs[PG_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                D_INFO("-------- Loading layer1_object palette onto HDMV Presentation layer surface --------\n");
                DFBCHECK(layer1_object->GetPalette( layer1_object, &palette ));
                DFBCHECK(palette->CreateCopy(palette, &duppalette));
                DFBCHECK(hdmv_layer_surfaces[PG_LAYER_INDEX]->SetPalette( hdmv_layer_surfaces[PG_LAYER_INDEX], duppalette ));
                duppalette->Release( duppalette );
                palette->Release( palette );
            }

        }

        if (nb_hdmv_layers > IG_LAYER_INDEX) {
    
            snprintf(
                file_path,
                sizeof(file_path),
                DATADIR"/images/%s_smiley_%s%s.png",
                filename_source_format[IG_LAYER_INDEX],
                filename_pixelformat[IG_LAYER_INDEX],
                filename_suffix[IG_LAYER_INDEX]
                );
        
            D_INFO("-------- loading %s as layer2_object --------\n", file_path);
        
            DFBCHECK(dfb->CreateImageProvider( dfb, file_path, &provider ));
        
            DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
    
            layer2_object_rect.x = 0;
            layer2_object_rect.y = 0;
            layer2_object_rect.w = dsc.width;
            layer2_object_rect.h = dsc.height;
            D_INFO( "layer2_object_rect: x[%d], y[%d], w[%d], h[%d]\n",
                    layer2_object_rect.x,
                    layer2_object_rect.y,
                    layer2_object_rect.w,
                    layer2_object_rect.h );
    
            /* Direct support for LUT8 in image providers seem to have been added in the new 0.9.24 release,
             * png are always ARGB format if an alpha channel, thus  for now as a work around
             * we let RenderTo do the actual convertion.
             */
            if (hdmv_layer_configs[IG_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                dsc.pixelformat = DSPF_LUT8;
            }
            else {
                dsc.pixelformat = DSPF_ARGB;
            }
        
            DFBCHECK(dfb->CreateSurface( dfb, &dsc, &layer2_object ));
    
            DFBCHECK(provider->RenderTo( provider, layer2_object, NULL ));
            provider->Release( provider );
            
            if (hdmv_layer_configs[IG_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                D_INFO("-------- Loading layer1_object palette onto HDMV Interactive layer surface --------\n");
                DFBCHECK(layer2_object->GetPalette( layer2_object, &palette ));
                DFBCHECK(palette->CreateCopy(palette, &duppalette));
                DFBCHECK(hdmv_layer_surfaces[IG_LAYER_INDEX]->SetPalette( hdmv_layer_surfaces[IG_LAYER_INDEX], duppalette ));
                duppalette->Release( duppalette );
                palette->Release( palette );
            }

        }
        
        if (nb_hdmv_layers > OSD_LAYER_INDEX) {
    
            snprintf(
                file_path,
                sizeof(file_path),
                DATADIR"/images/%s_bgmenu_%s%s.png",
                filename_source_format[OSD_LAYER_INDEX],
                filename_pixelformat[OSD_LAYER_INDEX],
                filename_suffix[OSD_LAYER_INDEX]
                );
        
            D_INFO("-------- loading %s as layer3_object --------\n", file_path);
        
            DFBCHECK(dfb->CreateImageProvider( dfb, file_path, &provider ));
        
            DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));
            
            layer3_object_rect.x = 0;
            layer3_object_rect.y = 0;
            layer3_object_rect.w = dsc.width;
            layer3_object_rect.h = dsc.height;
            D_INFO( "layer3_object_rect: x[%d], y[%d], w[%d], h[%d]\n",
                    layer3_object_rect.x,
                    layer3_object_rect.y,
                    layer3_object_rect.w,
                    layer3_object_rect.h);
    
            /* Direct support for LUT8 in image providers seem to have been added in the new 0.9.24 release,
             * png are always ARGB format if an alpha channel, thus  for now as a work around
             * we let RenderTo do the actual convertion.
             */
            if (hdmv_layer_configs[OSD_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                dsc.pixelformat = DSPF_LUT8;
            }
            else {
                dsc.pixelformat = DSPF_ARGB;
            }
        
            DFBCHECK(dfb->CreateSurface( dfb, &dsc, &layer3_object ));
        
            DFBCHECK(provider->RenderTo( provider, layer3_object, NULL ));
            provider->Release( provider );

        }

        if (nb_hdmv_layers > PG_LAYER_INDEX) {

            D_INFO("-------- Blitting layer1_object onto HDMV Presentation layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[PG_LAYER_INDEX]->SetBlittingFlags(
                      hdmv_layer_surfaces[PG_LAYER_INDEX], 
                      DSBLIT_NOFX ) );
    
            DFBCHECK( hdmv_layer_surfaces[PG_LAYER_INDEX]->Blit(
                      hdmv_layer_surfaces[PG_LAYER_INDEX],
                      layer1_object,
                      &layer1_object_rect,
                      0,0) );
    
            D_INFO("-------- Setting level %d for HDMV Presentation layer --------\n", hdmv_layer_levels[PG_LAYER_INDEX]);
    
            DFBCHECK( hdmv_layers[PG_LAYER_INDEX]->SetLevel(hdmv_layers[PG_LAYER_INDEX], hdmv_layer_levels[PG_LAYER_INDEX]) );
    
            D_INFO("-------- Flipping from HDMV Presentation layer surface --------\n");
    
            flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&layer1_object_rect);
    
            DFBCHECK( hdmv_layer_surfaces[PG_LAYER_INDEX]->Flip( hdmv_layer_surfaces[PG_LAYER_INDEX], &flip_region, 0 ) );
    
            getchar();

            if (hdmv_layer_configs[PG_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                D_INFO("-------- Changing palette entries to a blue gradient for Presentation layer surface --------\n");
                DFBCHECK(hdmv_layer_surfaces[PG_LAYER_INDEX]->GetPalette( hdmv_layer_surfaces[PG_LAYER_INDEX], &palette ));
                
                DFBCHECK(generate_palette_pattern_entries( palette, ePaletteGenType_blue_gradient ));
                
                DFBCHECK(hdmv_layer_surfaces[PG_LAYER_INDEX]->SetPalette( hdmv_layer_surfaces[PG_LAYER_INDEX], palette ));
                palette->Release( palette );

                getchar();

                D_INFO("-------- Restoring palette entries from layer1_object for Presentation layer surface --------\n");

                DFBCHECK(layer1_object->GetPalette( layer1_object, &palette ));
                DFBCHECK(palette->CreateCopy(palette, &duppalette));
                DFBCHECK(hdmv_layer_surfaces[PG_LAYER_INDEX]->SetPalette( hdmv_layer_surfaces[PG_LAYER_INDEX], duppalette ));
                duppalette->Release( duppalette );
                palette->Release( palette );
                
                getchar();
            }

        }

        if (nb_hdmv_layers > IG_LAYER_INDEX) {
    
            D_INFO("-------- Blitting layer2_object onto HDMV Interactive layer surface --------\n");
        
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->SetBlittingFlags(
                                hdmv_layer_surfaces[IG_LAYER_INDEX],
                                DSBLIT_NOFX ) );
        
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Blit(
                                hdmv_layer_surfaces[IG_LAYER_INDEX],
                                layer2_object,
                                &layer2_object_rect,
                                0,0) );
        
            D_INFO("-------- Setting level %d for HDMV Interactive layer --------\n", hdmv_layer_levels[IG_LAYER_INDEX]);
    
            DFBCHECK( hdmv_layers[IG_LAYER_INDEX]->SetLevel( hdmv_layers[IG_LAYER_INDEX], hdmv_layer_levels[IG_LAYER_INDEX]) );
    
            D_INFO("-------- Flipping from HDMV Interactive layer surface --------\n");
    
            flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&layer2_object_rect);
    
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Flip( hdmv_layer_surfaces[IG_LAYER_INDEX], &flip_region, 0 ) );
    
            getchar();

            if (hdmv_layer_configs[IG_LAYER_INDEX].pixelformat == DSPF_LUT8) {
                D_INFO("-------- Changing palette entries to a green gradient for Interactive layer surface --------\n");
                DFBCHECK(hdmv_layer_surfaces[IG_LAYER_INDEX]->GetPalette( hdmv_layer_surfaces[IG_LAYER_INDEX], &palette ));
                
                DFBCHECK(generate_palette_pattern_entries( palette, ePaletteGenType_green_gradient ));
                
                DFBCHECK(hdmv_layer_surfaces[IG_LAYER_INDEX]->SetPalette( hdmv_layer_surfaces[IG_LAYER_INDEX], palette ));
                palette->Release( palette );

                getchar();

                D_INFO("-------- Restoring palette entries from layer2_object for Interactive layer surface --------\n");

                DFBCHECK(layer2_object->GetPalette( layer2_object, &palette ));
                DFBCHECK(palette->CreateCopy(palette, &duppalette));
                DFBCHECK(hdmv_layer_surfaces[IG_LAYER_INDEX]->SetPalette( hdmv_layer_surfaces[IG_LAYER_INDEX], duppalette ));
                duppalette->Release( duppalette );
                palette->Release( palette );
                
                getchar();
            }

        }

        if (nb_hdmv_layers > OSD_LAYER_INDEX) {

            D_INFO("-------- Blitting layer3_object onto OSD layer surface --------\n");
    
            DFBCHECK( hdmv_layer_surfaces[OSD_LAYER_INDEX]->SetBlittingFlags(
                                hdmv_layer_surfaces[OSD_LAYER_INDEX],
                                DSBLIT_NOFX ) );
    
            DFBCHECK( hdmv_layer_surfaces[OSD_LAYER_INDEX]->Blit(
                                hdmv_layer_surfaces[OSD_LAYER_INDEX],
                                layer3_object,
                                &layer3_object_rect,
                                0,0) );
        
            D_INFO("-------- Setting level %d for OSD layer --------\n", hdmv_layer_levels[OSD_LAYER_INDEX]);
    
            DFBCHECK( hdmv_layers[OSD_LAYER_INDEX]->SetLevel(hdmv_layers[OSD_LAYER_INDEX], hdmv_layer_levels[OSD_LAYER_INDEX]) );
    
            D_INFO("-------- Flipping from OSD layer surface --------\n");
    
            flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&layer3_object_rect);
    
            DFBCHECK( hdmv_layer_surfaces[OSD_LAYER_INDEX]->Flip( hdmv_layer_surfaces[OSD_LAYER_INDEX], &flip_region, 0 ) );
    
            getchar();
            
        }

        if (nb_hdmv_layers > IG_LAYER_INDEX) {
        
            D_INFO("-------- Testing clipping of both Flip and Blit --------\n");
    
            xpos = 0;
            ypos = 0;
            
            current_rect.w = layer2_object_rect.w;
            current_rect.h = layer2_object_rect.h;
    
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->FillRectangle(
                                hdmv_layer_surfaces[IG_LAYER_INDEX],
                                0,
                                0,
                                current_rect.w,
                                current_rect.h) );
    
            for (i = 0; i < 4; i++) {
                union_rect.x = xpos;
                union_rect.y = ypos;
                union_rect.w = layer2_object_rect.w;
                union_rect.h = layer2_object_rect.h;
                switch (i) {
                    case 0:
                        D_INFO("-------- layer2_object Top Left Corner --------\n");
    
                        xpos = 0-layer2_object_rect.w/2;
                        ypos = 0-layer2_object_rect.h/2;
                    break;
                    case 1:
                        D_INFO("-------- layer2_object Top Right Corner  --------\n");
    
                        xpos = hdmv_layer_surface_rects[IG_LAYER_INDEX].w-layer2_object_rect.w/2;
                        ypos = 0-layer2_object_rect.h/2;
                    break;
                    case 2:
                        D_INFO("-------- layer2_object Bottom Left Corner --------\n");
    
                        xpos = 0-layer2_object_rect.w/2;
                        ypos = hdmv_layer_surface_rects[IG_LAYER_INDEX].h-layer2_object_rect.h/2;
                    break;
                    case 3:
                        D_INFO("-------- layer2_object Bottom Right Corner  --------\n");
    
                        xpos = hdmv_layer_surface_rects[IG_LAYER_INDEX].w-layer2_object_rect.w/2;
                        ypos = hdmv_layer_surface_rects[IG_LAYER_INDEX].h-layer2_object_rect.h/2;
                    break;
                    default:
                        D_ERROR("Should not be happening\n");
                        ret = EXIT_FAILURE;
                        goto out;
                    break;
                }
    
                current_rect.x = xpos;
                current_rect.y = ypos;
    
                DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Blit(
                                    hdmv_layer_surfaces[IG_LAYER_INDEX],
                                    layer2_object,
                                    &layer2_object_rect,
                                    current_rect.x,
                                    current_rect.y) );
    
                dfb_rectangle_union(&union_rect, &current_rect);
    
                flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&union_rect);
    
                DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Flip( hdmv_layer_surfaces[IG_LAYER_INDEX], &flip_region, 0 ) );
        
                getchar();
        
                DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->FillRectangle(
                                    hdmv_layer_surfaces[IG_LAYER_INDEX],
                                    current_rect.x,
                                    current_rect.y,
                                    current_rect.w,
                                    current_rect.h) );
            }
            
            flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&current_rect);
            
            DFBCHECK( hdmv_layer_surfaces[IG_LAYER_INDEX]->Flip( hdmv_layer_surfaces[IG_LAYER_INDEX], &flip_region, 0 ) );

        }

        D_INFO("-------- Starting multi-thread stress test --------\n");

        blit_routine_arguments[PG_LAYER_INDEX].source_rect = &layer1_object_rect;
        blit_routine_arguments[PG_LAYER_INDEX].source = layer1_object;
        blit_routine_arguments[IG_LAYER_INDEX].source_rect = &layer2_object_rect;
        blit_routine_arguments[IG_LAYER_INDEX].source = layer2_object;
        blit_routine_arguments[OSD_LAYER_INDEX].source_rect = &layer3_object_rect;
        blit_routine_arguments[OSD_LAYER_INDEX].source = layer3_object;

        DFBCHECK( init_wakeup_call(&input_event) );

        for (i = 0; i < nb_hdmv_layers; i++) {
            blit_routine_arguments[i].dest = hdmv_layer_surfaces[i];
            blit_routine_arguments[i].dest_rect = &hdmv_layer_surface_rects[i];
            blit_routine_arguments[i].cancelled = false;
            blit_routine_arguments[i].dfb = dfb;
            blit_routine_arguments[i].partial_flip = flag_partial_flip;
            blit_routine_arguments[i].mdelay = mdelay;
            blit_routine_arguments[i].test_pattern = hdmv_layer_testpatterns[i];
            blit_routine_arguments[i].primary_double_buffered = primary_double_buffered;
            blit_routine_arguments[i].input_event = &input_event;

            /* We might eventually want to set attributes ... */
            if ((rc = pthread_attr_init(&hdmv_threads_attr[i]))) {
                D_ERROR("Could not pthread_attr_init %s\n", strerror(rc));
                ret = EXIT_FAILURE;
                goto out;
            }
            
            if ((rc = pthread_create(&hdmv_threads[i], &hdmv_threads_attr[i], blit_thread_routine, &blit_routine_arguments[i]))) {
                D_ERROR("Could not pthread_create %s\n", strerror(rc));
                ret = EXIT_FAILURE;
                goto out;
            }
        }

        display_set_thread_arguments.dfb = dfb;
        display_set_thread_arguments.bdisplay = bdisplay;
        display_set_thread_arguments.input_event = &input_event;

        /* We might eventually want to set attributes ... */
        if ((rc = pthread_attr_init(&display_set_thread_attr))) {
            D_ERROR("Could not pthread_attr_init %s\n", strerror(rc));
            ret = EXIT_FAILURE;
            goto out;
        }

        if ((rc = pthread_create(&display_set_thread, &display_set_thread_attr, display_set_thread_routine, &display_set_thread_arguments))) {
            D_ERROR("Could not pthread_create %s\n", strerror(rc));
            ret = EXIT_FAILURE;
            goto out;
        }

        D_INFO("-------- Press key:\n"
               "-------- s to stop\n"
               "-------- n to change to ntsc\n"
               "-------- p to change to progressive\n"
               "-------- i to change to interlaced\n");

        while(!stop_input_event) {
            switch (getchar()) {
                case 's':
                    stop_input_event = true;
                break;
                case 'n':
                    display_set_thread_arguments.video_format = bvideo_format_ntsc;
                    DFBCHECK( broadcast_wakeup_call(&input_event) );
                break;
                case 'p':
                    display_set_thread_arguments.video_format = bvideo_format_720p;
                    DFBCHECK( broadcast_wakeup_call(&input_event) );
                break;
                case 'i':
                    display_set_thread_arguments.video_format = bvideo_format_1080i;
                    DFBCHECK( broadcast_wakeup_call(&input_event) );
                break;
                default:
                    /* get new event */
                break;
            }
        }
        
        stop_input_event = false;

        D_INFO("-------- Received stop command --------\n");

        for (i = 0; i < nb_hdmv_layers; i++) {
            blit_routine_arguments[i].cancelled = true;

            if ((rc = pthread_join(hdmv_threads[i], NULL))) {
                D_ERROR("Could not pthread_join %s\n", strerror(rc));
                ret = EXIT_FAILURE;
                goto out;
            }
        }

        for (i = 0; i < nb_hdmv_layers; i++) {
            printf("***************************\n"
                   "Layer %u:\n"
                   "Total time (secs) %u:\n"
                   "Total compositions %u:\n"
                   "Compositions/sec %u:\n"
                   "MPixels/sec (excluding mixing bandwidth) %u:\n"
                   "***************************\n",
                   i,
                   blit_routine_arguments[i].nb_msecs/1000,
                   blit_routine_arguments[i].nb_compositions,
                   blit_routine_arguments[i].compositions_sec,
                   blit_routine_arguments[i].mpixels_sec);
            fflush(stdout);
        }

        /* Hide all the layers until next pass */
        for (i = 0; i < nb_hdmv_layers; i++) {
            /* setting all the flags to false and config to 0 */
            memset(&hdmv_layer_configs[i], 0, sizeof(hdmv_layer_configs[i]));

            DFBCHECK( hdmv_layers[i]->SetLevel(hdmv_layers[i], -1 ) );
            /* We must release the layer objects from the layer surface
             * state, this would unref the objects to permit deallocation.
             * Must find another way to do this!
             */
            DFBCHECK( hdmv_layer_surfaces[i]->Blit(
                      hdmv_layer_surfaces[i],
                      release_surface,
                      &release_surface_rect,
                      0,0) );
        }

        /* Destroy the composition objects surfaces */
        if (nb_hdmv_layers > PG_LAYER_INDEX)
            layer1_object->Release( layer1_object );

        if (nb_hdmv_layers > IG_LAYER_INDEX)
            layer2_object->Release( layer2_object );

        if (nb_hdmv_layers > OSD_LAYER_INDEX)
            layer3_object->Release( layer3_object );
    }

    for (i = 0; i < nb_hdmv_layers; i++) {
        hdmv_layer_surfaces[i]->Release( hdmv_layer_surfaces[i] );
        hdmv_layers[i]->Release( hdmv_layers[i] );
    }

    release_surface->Release( release_surface );
    primary_layer->Release( primary_layer );
    dfb->Release( dfb );

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

    return ret;
}

