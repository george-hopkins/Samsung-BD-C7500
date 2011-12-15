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
#include "dfbbcm_playbackrinbufthread.h"
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_gfxperfthread.h"

void usage() {
    printf("\n\nUsage:\n\n");
    printf("dfbbcm_sync_faa -S stream -P pid -C codec [-t start:stop] [-f framerate]\n\n");
    printf("stream      stream file name\n");
    printf("pid         PID (hex)\n");
    printf("codec       codec (MPEG2=2, AVC=27, ...)\n");
    printf("start       PTS to start animation (hex)\n");
    printf("stop        PTS to stop animation (hex)\n");
    printf("framerate   bdvd_dfb_ext_frame_rate enum value\n");
    printf("\n");
}

#define NUM_BUFFERS 4

typedef struct {
    char * file_prefix;
    char * file_suffix;
    uint32_t nb_frames;
} faa_description_t;

faa_description_t image_faa_animations[] = {
    {"imagefaa_molecule_", ".gif", 12},
    {"imagefaa_numbers_", ".png", 10}
};

/* What is the limit on the number of elements
 * in the sequence in the BD-J spec?
 */

#define MAX_ELEM_REPEAT_SEQUENCE 8

/* Terminated by a -1 just for the purpose of our example, this
 * is NOT a requirement of the bdvd_dfb_ext API!!!.
 */
int image_faa_repeat_sequences[][MAX_ELEM_REPEAT_SEQUENCE] = {
    {1, -1},
    {2, -1},
    {1, 2, -1},
    {2, 1, -1},
    {1, 0, 2, -1},
    {2, 0, 1, -1},
    {1, 2, 3, -1},
    {1, 2, 1, -1},
    {2, 1, 2, -1}
};

IDirectFBDisplayLayer * osd_layer = NULL;
IDirectFBSurface * osd_surface = NULL;
DFBRectangle osd_surface_rect;
IDirectFBFont *font = NULL;
DFBFontDescription fontdesc;
char pts_desc[255];

#define STC_FREQ 45000

uint32_t currentPTS = 0;
bool videoRunning = false;

void bdvd_pts_callback(
    bdvd_decode_h           decode,         /* [handle] from bdvd_decode_open */
    bdvd_decode_event_e     event,          /* [in] event causing callback    */
    void                    *cookie,        /* ptr provided with pre_clip     */
    unsigned long           clip_id,        /* clip_id provided with pre_clip */
    bdvd_rect_t             displayRect,    /* Current display rectangle      */
    void                    *event_data,    /* [in] data for event            */
    void                    *notify_cb_ptr  /* [in] ptr provided at open      */
)
{
    bdvd_decode_timestamps_t * timestamp_data;

    switch (event) {
        case bdvd_decode_event_timestamps:
            if (event_data != NULL && videoRunning) {
                timestamp_data = (bdvd_decode_timestamps_t *)event_data;
                if (currentPTS == 0) {
                    if (bdvd_dfb_ext_set_video_status(true) != bdvd_ok) {
                        D_ERROR("pts_callback: bdvd_dfb_ext_set_video_status failed\n");
                    }
                }
                currentPTS = timestamp_data->video_pts;
                if (bdvd_dfb_ext_set_current_pts(currentPTS) != bdvd_ok) {
                    D_ERROR("pts_callback: bdvd_dfb_ext_set_current_pts failed\n");
                }
            }
            break;
            
        case bdvd_decode_event_video_presentation_start:
            videoRunning = true;
            break;

        case bdvd_decode_event_video_decode_done:
            videoRunning = false;
            if (bdvd_dfb_ext_set_video_status(false) != bdvd_ok) {
                D_ERROR("pts_callback: bdvd_dfb_ext_set_video_status failed\n");
            }
            break;
    }
}

typedef struct {
    bool cancelled;
} pts_display_thread_context_t;

void * pts_display_thread_routine(void * arg) {
    
    pts_display_thread_context_t * thread_routine_context = (pts_display_thread_context_t *)arg;
        
    uint32_t tempPTS;
    uint32_t hours;
    uint32_t mins;
    uint32_t secs;
    uint32_t secsdiv100;

    DFBRegion flip_region;
    DFBRectangle string_rect;
    
    string_rect = (DFBRectangle){osd_surface_rect.w/8,
                  osd_surface_rect.h/8,
                  osd_surface_rect.w/3,
                  fontdesc.height};
    
    flip_region = DFB_REGION_INIT_FROM_RECTANGLE(&string_rect);

    while (!thread_routine_context->cancelled) {
        tempPTS = currentPTS;
        secsdiv100 = (currentPTS%STC_FREQ)*100/STC_FREQ;
       
        hours = tempPTS/(60*60)/STC_FREQ;
        tempPTS -= hours*60*60*STC_FREQ;
        mins = tempPTS/60/STC_FREQ;
        tempPTS -= mins*60*STC_FREQ;
        secs = tempPTS/STC_FREQ;

        snprintf(pts_desc, sizeof(pts_desc)/sizeof(pts_desc[0]), "PTS:0x%08X Time:%02u:%02u:%02u:%02u", currentPTS, hours, mins, secs, secsdiv100);

        osd_surface->SetColor(osd_surface,
                              0x0,
                              0x0,
                              0x0,
                              0x0);
    
        osd_surface->FillRectangle(osd_surface,
                                   string_rect.x, string_rect.y,
                                   string_rect.w, string_rect.h);
    
        osd_surface->SetColor(osd_surface,
                              0xff,
                              0xff,
                              0xff,
                              0xff);

        osd_surface->DrawString(osd_surface,
                                pts_desc, -1,
                                string_rect.x, string_rect.y, DSTF_TOP | DSTF_LEFT);

        osd_surface->Flip(osd_surface, &flip_region, DSFLIP_NONE);
    }
    return NULL;
}

typedef enum {
    CMD_NONE = 0,
    CMD_START_ANIMATION,
    CMD_STOP_ANIMATION,
    CMD_RESET_ANIMATION_TIMER,
    CMD_RESUME_ANIMATION_TIMER
} animation_input_command;

animation_input_command input_cmd;

typedef struct {
    bool cancelled;
    IDirectFB * dfb;
    DFBLocation animation_location;
    faa_description_t * animation_image_desc;
    bdvd_dfb_ext_type_settings_t animation_settings;
    bdvd_decode_h decode;
    wakeup_call_t * input_event;
} faa_thread_context_t;



bool FinishDrawing(bdvd_dfb_ext_h layer_h, IDirectFBSurface * surface, uint32_t frameNumber)
{
    bdvd_dfb_ext_layer_settings_t layer_dfb_ext_settings;

    if (bdvd_dfb_ext_layer_get(layer_h, &layer_dfb_ext_settings) != bdvd_ok) {
        /* handle/print/throw error */
    }

    layer_dfb_ext_settings.type_settings.sync_faa.flags |= bdvd_dfb_ext_faa_flags_worknotfini;
    layer_dfb_ext_settings.type_settings.sync_faa.sync_faa_state.workNotFinished = false;
    if (bdvd_dfb_ext_layer_set(layer_h, &layer_dfb_ext_settings) != bdvd_ok) {
        /* handle/print/throw error */
        goto quit;
    }

    if (surface->Flip(surface, NULL, DSFLIP_NONE) != DFB_OK) {
        /* handle/print/throw error */
        goto quit;
    }

quit:
    return true;
}

bool StartDrawing(bdvd_dfb_ext_h layer_h, uint32_t frameNumber)
{
    bdvd_result_e ret = bdvd_ok;
    bdvd_dfb_ext_layer_settings_t layer_dfb_ext_settings;
    bool draw_frame = true;
    bool done = false;
    bdvd_dfb_ext_animation_stats_t stats;

    if (bdvd_dfb_ext_layer_get_animation_stats(layer_h, &stats) != bdvd_ok) {
        printf("StartDrawing failed calling bdvd_dfb_ext_layer_get_animation_stats\n");
        draw_frame = false;
        goto quit;
    }
    if (!stats.isAnimated) {
        draw_frame = false;
        goto quit;
    }

    if (bdvd_dfb_ext_layer_get(layer_h, &layer_dfb_ext_settings) != bdvd_ok) {
        printf("StartDrawing failed calling bdvd_dfb_ext_layer_get\n");
        draw_frame = false;
        goto quit;
    }

    layer_dfb_ext_settings.type_settings.sync_faa.flags |= bdvd_dfb_ext_faa_flags_forceworking | bdvd_dfb_ext_faa_flags_worknotfini;
    layer_dfb_ext_settings.type_settings.sync_faa.sync_faa_state.workNotFinished = true;
    layer_dfb_ext_settings.type_settings.sync_faa.sync_faa_state.forceWorkingPosition = frameNumber;

    while (!done) {
        ret = bdvd_dfb_ext_layer_set(layer_h, &layer_dfb_ext_settings);
        switch (ret) {
        case bdvd_err_busy:
            /*
             * printf("No buffer available for drawing..sleeping\n");
             */
            /*usleep(10000);*/
            draw_frame = false;
            done = true;
            break;
        case bdvd_ok:
            /*
             * printf("Starting frame %d!\n", frameNumber);
             */
            draw_frame = true;
            done = true;
            break;
        case bdvd_err_invalid_parameter:
            /*
             * printf("DROPPING FRAME %d!\n", frameNumber);
             */
            draw_frame = false;
            done = true;
            break;
        default:
            /*
             * printf("Unknown error!!\n");
             */
            draw_frame = false;
            done = true;
            break;
        }
    }

quit:
    return draw_frame;
}


typedef struct {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ColorArray_t;

#define NUM_COLORS 7
ColorArray_t colorArray[NUM_COLORS] = {
{0xff, 0xFF, 0xFF, 0xFF}, 
{0xff, 0xFF, 0x00, 0x00}, 
{0xff, 0x00, 0xFF, 0x00},
{0xff, 0x00, 0x00, 0xFF}, 
{0xff, 0xFF, 0xFF, 0x00}, 
{0xff, 0x00, 0xFF, 0xFF},
{0xff, 0xFF, 0x00, 0xFF}
};

int x, y, w, h, deltax, deltay;
int sur_w, sur_h;
uint32_t c;

void DrawSurface(bdvd_dfb_ext_h layer, IDirectFBSurface * surface, uint32_t frame)
{
    surface->SetDrawingFlags(surface, DSDRAW_BLEND);
    surface->Clear(surface, 0, 0, 0, 0);
    if (x+w >= sur_w) {
        deltax *= -1;
        c++;
    }
    if (y+h >= sur_h) {
        deltay *= -1;
        c++;
    }
    if (x <= 0) {
        deltax *= -1;
        c++;
    }
    if (y <= 0) {
        deltay *= -1;
        c++;
    }
    x += deltax;
    y += deltay;
    c %= NUM_COLORS;

    surface->SetColor(surface, 
                      colorArray[c].r,
                      colorArray[c].g,
                      colorArray[c].b,
                      colorArray[c].a);
    surface->FillRectangle(surface, x, y, w, h);
}

void sync_faa_thread_routine(void * arg) {

    faa_thread_context_t * thread_routine_context = (faa_thread_context_t *)arg;
    int i;
    IDirectFB* dfb;

    DFBResult err;
    IDirectFBDisplayLayer * layer = NULL;
    DFBDisplayLayerConfig layer_config;
    IDirectFBSurface * surface = NULL;
    DFBSurfaceDescription image_desc;
    DFBRectangle surface_rect;
    bdvd_dfb_ext_h layer_h = NULL;
    bdvd_dfb_ext_layer_type_e layer_type;
    bdvd_dfb_ext_layer_settings_t dfb_ext_layer_settings;

    memset(&layer_config, 0, sizeof(DFBDisplayLayerConfig));

    if (thread_routine_context == NULL) {
        D_ERROR("sync_faa_thread_routine: context is NULL\n");
        goto quit;
    }

    if ((dfb = thread_routine_context->dfb) == NULL) {
        D_ERROR("sync_faa_thread_routine: dfb is NULL\n");
        goto quit;
    }

    layer_h=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_j_sync_faa_layer);
    bdvd_dfb_ext_layer_get(layer_h, &dfb_ext_layer_settings);

    /* Let's set the layer configuration if required.    */
    DFBCHECK( dfb->GetDisplayLayer(dfb, dfb_ext_layer_settings.id, &layer) );
    DFBCHECK( layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE) );

    DFBCHECK( layer->SetBackgroundColor(layer, 0, 0, 0, 0 ) );
    DFBCHECK( layer->SetBackgroundMode(layer, DLBM_COLOR ) );
    layer_config.flags |= (DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT | DLCONF_BUFFERMODE);
    layer_config.width = 1920;
    layer_config.height = 1080;
    layer_config.pixelformat = DSPF_ARGB;
    layer_config.buffermode = DLBM_FRONTONLY;
    DFBCHECK( layer->SetConfiguration(layer, &layer_config) );
    layer->SetLevel(layer, 10);
    layer->GetSurface(layer, &surface);
    surface->SetFont(surface, font);

    dfb_ext_layer_settings.type_settings.sync_faa.flags = 
        thread_routine_context->animation_settings.sync_faa.flags;

    dfb_ext_layer_settings.type_settings.sync_faa.faa_params =
        thread_routine_context->animation_settings.sync_faa.faa_params;

    dfb_ext_layer_settings.type_settings.sync_faa.faa_state =
        thread_routine_context->animation_settings.sync_faa.faa_state;

    if (bdvd_dfb_ext_layer_set(layer_h, &dfb_ext_layer_settings) != bdvd_ok) {
        D_ERROR("sync_faa_thread_routine: bdvd_dfb_ext_layer_set failed\n");
        goto quit;
    }

    surface->GetSize(surface, &sur_w, &sur_h);
    x = sur_w / 2;
    y = sur_h / 2;
    w = sur_w / 16;
    h = sur_h / 16;
    deltax = sur_w / 256;
    deltay = deltax;
    c = 0;

    for (i = 0; !thread_routine_context->cancelled; i++)
    {
        if (StartDrawing(layer_h, i)) {
            DrawSurface(layer_h, surface, i);
            FinishDrawing(layer_h, surface, i);
        }
    }

quit:

    DFBCHECK( layer->SetLevel(layer, -1) );

    if (surface) surface->Release(surface);
    if (layer) layer->Release(layer);
    if (layer_h) bdvd_dfb_ext_layer_close(layer_h);
}

int main( int argc, char *argv[] )
{
     int        ret = EXIT_SUCCESS;
     DFBResult  err;
     IDirectFB  *dfb;

     int        vidargc = 0;
     char       *vidargv[10];
     
     int rc;
     /* only used for allocation of the layer context,
      * (done beneath by directfb on GetLayer).
      */
     bdvd_dfb_ext_h osd_layer_ext = NULL;
     bdvd_dfb_ext_layer_settings_t osd_layer_ext_settings;

     char font_name[80];

     bdvd_display_h bdisplay = NULL;

     uint32_t animation_index = 1; /* numbers */
     uint32_t sequence_index = 0; /* 1 */
     uint32_t sequence_multiplicator = 1;

     pthread_t faa_thread;
     faa_thread_context_t faa_thread_context;

     pthread_t pts_display_thread;
     pts_display_thread_context_t pts_display_thread_context;

     char c;
     uint32_t i;
     char *nextarg;

     wakeup_call_t input_event;
     bool stop_input_event = false;

     playback_state_e state = playback_state_play;

     if (argc < 1) {
        usage();
        ret = EXIT_FAILURE;
        goto quit;
     }

    memset(&faa_thread_context, 0, sizeof(faa_thread_context_t));
    memset(&pts_display_thread_context, 0, sizeof(pts_display_thread_context_t));

    snprintf(font_name, sizeof(font_name)/sizeof(font_name[0]), DATADIR"/fonts/msgothic.ttc");
    fontdesc.flags = DFDESC_HEIGHT | DFDESC_ATTRIBUTES;
    fontdesc.attributes = DFFA_NOKERNING;
    fontdesc.height = 14;

    //faa_thread_context.animation_image_desc = &image_faa_animations[animation_index];
    faa_thread_context.animation_location.x = 0.3;
    faa_thread_context.animation_location.y = 0.3;
    faa_thread_context.animation_location.w = 0.4;
    faa_thread_context.animation_location.h = 0.4;
    faa_thread_context.animation_settings.sync_faa.flags = bdvd_dfb_ext_faa_flags_trigger | 
                                                           bdvd_dfb_ext_faa_flags_params |
                                                           bdvd_dfb_ext_faa_flags_framerate;
    faa_thread_context.animation_settings.sync_faa.faa_state.trigger = bdvd_dfb_ext_faa_trigger_forced_start;
    faa_thread_context.animation_settings.sync_faa.faa_state.default_framerate = bdvd_dfb_ext_frame_rate_24;
    faa_thread_context.animation_settings.sync_faa.faa_params.numBuffers = NUM_BUFFERS;
    faa_thread_context.animation_settings.sync_faa.faa_params.repeatCountSize = 0;
    faa_thread_context.animation_settings.sync_faa.faa_params.repeatCount = NULL;
    faa_thread_context.animation_settings.sync_faa.faa_params.scaleFactor = 1;

    while ((c = getopt (argc, argv, "f:t:S:C:P:")) != -1) {
        switch (c) {
        case 'S':
            vidargv[1] = "-file";
            vidargv[2] = optarg;
            vidargc += 2;
        break;
        case 'C':
            vidargv[3] = "-video_codec";
            vidargv[4] = optarg;
            vidargc += 2;
        break;
        case 'P':
            vidargv[5] = "-video_pid";
            vidargv[6] = optarg;
            vidargc += 2;
        break;
        case 'f':
            faa_thread_context.animation_settings.sync_faa.faa_state.default_framerate = (bdvd_dfb_ext_frame_rate_e)strtoul(optarg, NULL, 10);
        break;
        case 't':
            faa_thread_context.animation_settings.sync_faa.flags |= bdvd_dfb_ext_faa_flags_trigger | bdvd_dfb_ext_faa_flags_time;
            faa_thread_context.animation_settings.sync_faa.faa_state.trigger = bdvd_dfb_ext_faa_trigger_time;
            faa_thread_context.animation_settings.sync_faa.faa_state.startPTS = strtoul(optarg, &nextarg, 16);
            faa_thread_context.animation_settings.sync_faa.faa_state.stopPTS = strtoul(++nextarg, NULL, 16);
            /* For now the handle itself is not used, it must only be non-null */
            faa_thread_context.decode = (bdvd_decode_h)1;
        break;
        case '?':
            usage();
            ret = EXIT_FAILURE;
            goto quit;
        break;
        }
    }

    if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
        D_ERROR("Could not init bdvd\n");
        ret = EXIT_FAILURE;
        goto quit;
    }
    if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
        D_ERROR("Could not open bdvd_display B_ID(0)\n");
        ret = EXIT_FAILURE;
        goto quit;
    }

    DFBCHECK( display_set_video_format(bdisplay, bdvd_video_format_1080i) );

    dfb = bdvd_dfb_ext_open(3);

    if ((osd_layer_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_osd_layer)) == NULL) {
        fprintf(stderr,"bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto quit;
    }
    if (bdvd_dfb_ext_layer_get(osd_layer_ext, &osd_layer_ext_settings) != bdvd_ok) {
        fprintf(stderr,"bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto quit;
    }

    DFBCHECK( dfb->GetDisplayLayer(dfb, osd_layer_ext_settings.id, &osd_layer) );
    DFBCHECK( osd_layer->SetCooperativeLevel(osd_layer, DLSCL_ADMINISTRATIVE) );
    /* We have to put at least one regular layer beneath the FAA layer, and this
     * layer must always be visible */
    DFBCHECK( osd_layer->SetLevel(osd_layer, 1) );
    DFBCHECK( osd_layer->GetSurface(osd_layer, &osd_surface) );

    osd_surface_rect.x = osd_surface_rect.y = 0;
    DFBCHECK( osd_surface->GetSize( osd_surface, &osd_surface_rect.w, &osd_surface_rect.h) );

    DFBCHECK(dfb->CreateFont (dfb, font_name, &fontdesc, &font));

    DFBCHECK(osd_surface->SetFont( osd_surface, font ));
    DFBCHECK( init_wakeup_call(&input_event) );

    /* From this point, only the display_pts_callback should be allowed to operate on the osd surface */

    if (playbackscagat_start(bdisplay,
                             &input_event,
                             bdvd_pts_callback,
                             NULL, 
                             vidargc, 
                             vidargv) != bdvd_ok) {
        D_ERROR("playbackscagat_start failed\n");
        ret = EXIT_FAILURE;
        goto quit;
    }

    if ((rc = pthread_create(&pts_display_thread, NULL, pts_display_thread_routine, &pts_display_thread_context)) < 0)
    {
        D_ERROR("pthread_create failed: %s\n", strerror(rc));
        ret = EXIT_FAILURE;
        goto quit;
    }

    faa_thread_context.dfb = dfb;
    faa_thread_context.input_event = &input_event;
    if ((rc = pthread_create(&faa_thread, NULL, sync_faa_thread_routine, &faa_thread_context)) < 0)
    {
        D_ERROR("pthread_create failed: %s\n", strerror(rc));
        ret = EXIT_FAILURE;
        goto quit;
    }

    D_INFO("-------- Press key:\n"
           "-------- q to quit\n"
           "-------- t to reset animation timer params\n");

    while(!stop_input_event) {
        switch (getchar()) {
            case 'q':
                stop_input_event = true;
            break;
            case 't':
                input_cmd = CMD_RESET_ANIMATION_TIMER;
                DFBCHECK( broadcast_wakeup_call(&input_event) );
            break;
            default:
                /* get new event */
            break;
        }
    }

    faa_thread_context.cancelled = true;
    DFBCHECK( broadcast_wakeup_call(&input_event) );

    if ((rc = pthread_join(faa_thread, NULL)) < 0)
    {
        D_ERROR("pthread_join failed: %s\n", strerror(rc));
        ret = EXIT_FAILURE;
        goto quit;
    }

    pts_display_thread_context.cancelled = true;

    if ((rc = pthread_join(pts_display_thread,NULL)) < 0)
    {
        D_ERROR("pthread_join failed: %s\n", strerror(rc));
        ret = EXIT_FAILURE;
        goto quit;
    }

    playbackscagat_stop();

    DFBCHECK( osd_layer->SetLevel(osd_layer, -1) );

    if (font) font->Release(font);
    if (osd_surface) osd_surface->Release(osd_surface);
    if (osd_layer) osd_layer->Release(osd_layer);
    if (osd_layer_ext) bdvd_dfb_ext_layer_close(osd_layer_ext);

    DFBCHECK(dfb->Release( dfb ));

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

quit:

    return ret;
}


    
