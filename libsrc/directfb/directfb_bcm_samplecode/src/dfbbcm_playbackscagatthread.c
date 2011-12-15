
#include "bdvd.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "bstd.h"
#include "bkni.h"
#include "bkni_multi.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dfbbcm_playbackscagatthread.h"

/*
NOTE: The PAYLOAD_SIZE should be a multiple of 6(KB) to satisfy complete transport packets
      and sector transfers(2KB) for HDMV and DVD respectively.
*/

/* The player uses 48K of payloads */
#define PAYLOAD_SIZE            (48)
#define NAV_PAYLOAD_SIZE        (PAYLOAD_SIZE * 1024)
#define NAV_PAYLOAD_COUNT       (32)

int video_pid  = 0x1011;
int video_codec = bdvd_video_codec_mpeg2;

int audio_pid  = 0x1100;
int audio_codec = bdvd_audio_format_ac3;

int packet_pid = 0x1200;

void * payload_buffer = NULL;
uint32_t payload_buffer_offset;
uint8_t * current_payload;

uint32_t read_slot = 0;

uint32_t i;

volatile int playback_done;

static int ts_fd = -1;

static pthread_t playback_thread = (pthread_t)NULL;

typedef struct {
    bool show_progress;
    bool loop;
    bool cancelled;
} playback_routine_arguments;

static playback_routine_arguments playback_routine_args;

static bdvd_decode_h decode = NULL;
static bdvd_decode_window_h window = NULL;
static bdvd_playback_h playback = NULL;
static BKNI_EventHandle playback_event;

wakeup_call_t * input_event;

static playback_state_e temp_state = playback_state_play;

/* Private */

static bdvd_result_e decode_set_state(bdvd_decode_h decode, playback_state_e state) {
    bdvd_result_e ret = bdvd_ok;

    bdvd_decode_video_state_t decoder_state;

    decoder_state.length = sizeof(bdvd_decode_video_state_t);

    switch (state) {
        case playback_state_play:
            fprintf(stderr, "Play\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_all;
            decoder_state.persistence = BDVD_PERSISTENCE_PLAY;
            decoder_state.display_skip = 0;
        break;
        case playback_state_pause:
            fprintf(stderr, "Pause\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_all;
            decoder_state.persistence = BDVD_PERSISTENCE_PAUSE;
            decoder_state.display_skip = 0;
        break;
        case playback_state_ff_ip:
            fprintf(stderr, "Fast Foward IP\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_IP;
            decoder_state.persistence = 1;
            decoder_state.display_skip = 0;
        break;
        case playback_state_ff_i:
            fprintf(stderr, "Fast Foward I\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_I;
            decoder_state.persistence = 1;
            decoder_state.display_skip = 0;
        break;
        case playback_state_sf_1_2:
            fprintf(stderr, "1/16 Slow Forward\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_all;
            decoder_state.persistence = 16;
            decoder_state.display_skip = 0;
        break;
        case playback_state_sf_1_4:
            fprintf(stderr, "1/8 Slow Forward\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_all;
            decoder_state.persistence = 8;
            decoder_state.display_skip = 0;
        break;
        case playback_state_sf_1_8:
            fprintf(stderr, "1/4 Slow Forward\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_all;
            decoder_state.persistence = 4;
            decoder_state.display_skip = 0;
        break;
        case playback_state_frame_advance:
            fprintf(stderr, "1/2 Slow Forward\n");
            decoder_state.discontiguous = false;
            decoder_state.direction = bdvd_video_dir_forward;
            decoder_state.mode = bdvd_video_decode_all;
            decoder_state.persistence = 2;
            decoder_state.display_skip = 0;
        break;
            fprintf(stderr, "Frame Advance\n");
            if (bdvd_decode_video_frame_advance(decode) != bdvd_ok)
            {
                fprintf(stderr, "bdvd_decode_video_frame_advance failed!\n");
                ret = bdvd_err_unspecified;
            }
            goto quit;
        default:
            fprintf(stderr, "1/2 Slow Forward\n");
            ret = bdvd_err_unspecified;
            goto quit;
    }

    if (bdvd_decode_set_video_state(decode, &decoder_state) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_decode_set_video_state failed!\n");
        ret = bdvd_err_unspecified;
    }

quit:

    return ret;
}

static void * playback_thread_routine(void * arg)
{
    playback_routine_arguments * playback_args = (playback_routine_arguments *)arg;

    uint32_t done_count = 0;
    uint32_t pushed_total = 0;
    int buffer_added = 0;
    int n;
    unsigned next_buffer = 0;
    unsigned busy_buffers = 0;
    bdvd_decode_clip_info_t clip;

    playback_state_e state = playback_state_play;

    int progress=0;
    
    /* 
     * Initially we want to fill up all of the descriptors available so
     * that each each get_buffer will give us one descriptor's worth of buffering
     * and we'll be able to keep the ring buffer close to # descriptors - 1
     * full.
     */
    if (bdvd_decode_start(decode, window) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_decode_start failed!\n");
        goto quit;
    }

    clip.info.time.start = BDVD_IGNORE_START_TIME;
    clip.info.time.stop = BDVD_IGNORE_STOP_TIME;
    clip.type = bdvd_decode_video_pre_clip_info;
    if (bdvd_decode_clip_info(decode, &clip) != bdvd_ok) {
      printf("bdvd_decode_clip_info (pre) failed\n");
      goto quit;
    }

    while (!playback_args->cancelled)
    {
        if (busy_buffers == NAV_PAYLOAD_COUNT) {
          BKNI_WaitForEvent(playback_event, 250);

          if (bdvd_playback_check_buffers(playback, &done_count) != bdvd_ok) {
            printf("bdvd_playback_check_buffers failed\n");
            goto quit;
          }

          busy_buffers -= done_count;
          continue;
        } else {
          current_payload = (uint8_t *)((uint32_t)payload_buffer + next_buffer*NAV_PAYLOAD_SIZE);
        }

        n = read(ts_fd, current_payload, NAV_PAYLOAD_SIZE);

        if (n > 0) {
          int buffer_added = 0;

          while (!buffer_added) {
            switch (bdvd_playback_add_buffer(playback, current_payload, n)) {
            case bdvd_ok:
              next_buffer++;
              if (next_buffer == NAV_PAYLOAD_COUNT) {
                next_buffer = 0;
              }
              busy_buffers++;
              buffer_added=1;
              break;
            case bdvd_err_busy:
            case bdvd_err_no_descriptor:
              BKNI_WaitForEvent(playback_event, 250);
              continue;
            default:
              printf("bdvd_playback_add_buffer failed\n");
              goto quit;
            }
          }

          if (playback_args->show_progress) {
            progress += n;
            if (progress > (1024*1024)) {
              progress = 0; putchar('+'); fflush(stdout);
            }
          }
        } else {
          /*
           * Wait for the playback buffer to drain; should just take
           * a few milliseconds.
           */
          while (busy_buffers) {
            BKNI_WaitForEvent(playback_event, 250);

            if (bdvd_playback_check_buffers(playback, &done_count) != bdvd_ok) {
              printf("bdvd_playback_check_buffers failed\n");
              goto quit;
            }
            busy_buffers -= done_count;
          }



            if (playback_args->loop) {
                fprintf(stderr, "\nLooping...\n");
                if (lseek(ts_fd, 0, SEEK_SET) == -1) {
                    fprintf(stderr, "Error seeking fd %d: %s\n", ts_fd, strerror(errno));
                    goto quit;
                }
            } else {
                fprintf(stderr, "Error seeking fd %d: %s\n", ts_fd, strerror(errno));
                break;
            }
        }

        if (temp_state != state) {
            if (decode_set_state(decode, temp_state) != bdvd_ok)
            {
                fprintf(stderr, "bdvd_decode_stop failed!\n");
                goto quit;
            }

            switch (temp_state) {
                case playback_state_frame_advance:
                    temp_state = playback_state_pause;
                /* fall through */
                case playback_state_pause:
                    while (temp_state == playback_state_pause) {
                        if (wait_wakeup_call(input_event) != DFB_OK) {
                            fprintf(stderr, "wait_wakeup_call failed\n");
                            goto quit;
                        }
                    }

                    if (decode_set_state(decode, temp_state) != bdvd_ok)
                    {
                        fprintf(stderr, "bdvd_decode_stop failed!\n");
                        goto quit;
                    }
                break;
                default:
                /* do nothing */
                break;
            }

            state = temp_state;
        }
    }

quit:

    if (bdvd_decode_stop(decode, false) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_decode_stop failed!\n");
        goto quit;
    }

    return NULL;
}

static void standard_callback(void *event)
{
    BKNI_SetEvent((BKNI_EventHandle)event);
}


/* Public */

void playbackscagat_usage(void)
{
  fprintf(stderr, "\nBluray Stream Playback Application\n");
  fprintf(stderr, "\nUsage:");
  fprintf(stderr, "\nlaunch app_bda <args>\n");
  fprintf(stderr, "\nArgument:              Description:");
  fprintf(stderr, "\n-file <val>            Source content file");
  fprintf(stderr, "\n-video_pid <val>       Video Stream PID [%d]", video_pid);
  fprintf(stderr, "\n-audio_pid <val>       Audio Stream PID [%d]", audio_pid);
  fprintf(stderr, "\n-packet_pid <val>      Packet Stream PID [%d]", packet_pid);
  fprintf(stderr, "\n-video_codec <val>     MPEG2=2, AVC=27 [%d]", video_codec);
  fprintf(stderr, "\n-1080i                 1080i Output");
  fprintf(stderr, "\n-progress              Progress indicator");
  fprintf(stderr, "\n-loop                  Loop playback\n");
  fprintf(stderr, "\nExample:\n");
  fprintf(stderr, "\nlaunch app_bda -file stream.m2ts -loop\n\n");
}

bdvd_result_e
playbackscagat_window_settings_set(
                    playback_window_settings_flags_e window_settings_flags,
                    bdvd_decode_window_settings_t * window_settings)
{
    bdvd_result_e ret = bdvd_ok;
    bdvd_decode_window_settings_t temp_window_settings;

    if (window_settings_flags != playback_window_settings_none) {
        if (bdvd_decode_window_get(window, &temp_window_settings) != bdvd_ok) {
            fprintf(stderr, "bdvd_decode_window_get failed\n");
            ret = bdvd_err_unspecified;
            goto out;
        }
    }

    if (window_settings) {
        if (window_settings_flags & playback_window_settings_visible) {
            temp_window_settings.visible = window_settings->visible;
        }
    
        if (window_settings_flags & playback_window_settings_zorder) {
            temp_window_settings.zorder = window_settings->zorder;
        }
    
        if (window_settings_flags & playback_window_settings_luma) {
            temp_window_settings.lumakey_en = window_settings->lumakey_en;
            temp_window_settings.lumakey_lower = window_settings->lumakey_lower;
            temp_window_settings.lumakey_upper = window_settings->lumakey_upper;
            temp_window_settings.lumakey_mask = window_settings->lumakey_mask;
        }
    
        if (window_settings_flags & playback_window_settings_position) {
            temp_window_settings.position.x = window_settings->position.x;
            temp_window_settings.position.y = window_settings->position.y;
            temp_window_settings.position.width = window_settings->position.width;
            temp_window_settings.position.height = window_settings->position.height;
        }

        if (window_settings_flags & playback_window_settings_cliprect) {
            temp_window_settings.cliprect.x = window_settings->cliprect.x;
            temp_window_settings.cliprect.y = window_settings->cliprect.y;
            temp_window_settings.cliprect.width = window_settings->cliprect.width;
            temp_window_settings.cliprect.height = window_settings->cliprect.height;
        }
    }

    if (window_settings_flags != playback_window_settings_none) {
        if (bdvd_decode_window_set(window, &temp_window_settings) != bdvd_ok) {
            fprintf(stderr, "bdvd_decode_window_set failed\n");
            ret = bdvd_err_unspecified;
            goto out;
        }
    }

out:
    return ret;
}

bdvd_result_e
playbackscagat_window_settings_get(
                    bdvd_decode_window_settings_t * window_settings)
{
    bdvd_result_e ret = bdvd_ok;
    
    if (bdvd_decode_window_get(window, window_settings) != bdvd_ok) {
        fprintf(stderr, "bdvd_decode_window_get failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

out:
    return ret;
}

bdvd_result_e
playbackscagat_decode_get_status(bdvd_decode_status_t * decode_status)
{
    bdvd_result_e ret = bdvd_ok;
    
    if (bdvd_decode_get_video_state(decode, decode_status) != bdvd_ok) {
        fprintf(stderr, "bdvd_decode_get_video_state failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

out:
    return ret;
}

bdvd_result_e
playbackscagat_start(bdvd_display_h display,
                     wakeup_call_t * wakeup_call,
                     bdvd_decode_notify_cb_t pts_callback,
                     void * pts_callback_context,
                     int argc, char *argv[])
{
    return playbackscagat_start_with_settings(
        display,
        playback_window_settings_none,
        NULL,
        wakeup_call,
        pts_callback,
        pts_callback_context,
        argc, argv);
}

bdvd_result_e playbackscagat_start_with_settings(
    bdvd_display_h display,
    playback_window_settings_flags_e window_settings_flags,
    bdvd_decode_window_settings_t * window_settings,
    wakeup_call_t * wakeup_call,
    bdvd_decode_notify_cb_t pts_callback,
    void * pts_callback_context,
    int argc, char *argv[])
{
    bdvd_result_e ret = bdvd_ok;
    char content_file[100] = "";
    int i;
    bdvd_display_settings_t display_settings;

    bdvd_playback_open_params_t playback_params;
    bdvd_playback_start_params_t start_params;
    bdvd_decode_audio_params_t audio_params;
    bdvd_decode_video_params_t video_params;
    
    bdvd_decode_window_settings_t temp_window_settings;

    input_event = wakeup_call;

    memset(&playback_routine_args, 0, sizeof(playback_routine_args));

    for (i = 1; i < argc; i++) {
        if ( !strcmp(argv[i], "-file") ) {
            strncpy(content_file, argv[++i],sizeof(content_file)/sizeof(content_file[0]));
        } else if ( !strcmp(argv[i], "-video_pid") ) {
            video_pid = atoi(argv[++i]);
            if ( !video_pid ) {
                playbackscagat_usage();
                ret = bdvd_err_unspecified;
                goto out;
            }
        } else if ( !strcmp(argv[i], "-audio_pid") ) {
            audio_pid = atoi(argv[++i]);
            if ( !audio_pid ) {
                playbackscagat_usage();
                ret = bdvd_err_unspecified;
                goto out;
            }
        } else if ( !strcmp(argv[i], "-video_codec") ) {
            video_codec = atoi(argv[++i]);
            if ( !video_codec ) {
                playbackscagat_usage();
                ret = bdvd_err_unspecified;
                goto out;
            }
        } else if ( !strcmp(argv[i], "-progress") ) {
            playback_routine_args.show_progress = 1;
        } else if ( !strcmp(argv[i], "-loop") ) {
            playback_routine_args.loop = 1;
        }
    }

    if ((ts_fd = open(content_file, O_RDONLY | O_LARGEFILE, 0444)) == -1) {;
        fprintf(stderr, "open failed on file %s\n", content_file);
        ret = bdvd_err_unspecified;
        goto out;
    }

    playback_params.stream_type = bdvd_playback_stream_type_bd;
    playback_params.buffer_size = 0;
    playback_params.alignment = 0;
    playback_params.num_descriptors = NAV_PAYLOAD_COUNT;

    payload_buffer = bdvd_playback_alloc_buffer((NAV_PAYLOAD_COUNT * NAV_PAYLOAD_SIZE), NAV_PAYLOAD_SIZE);
    if (payload_buffer == NULL) {
        fprintf(stderr, "bdvd_playback_alloc_buffer failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    playback = bdvd_playback_open(B_ID(0), &playback_params);
    if (playback == NULL) {
        fprintf(stderr, "bdvd_playback_open failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    if (BKNI_CreateEvent(&playback_event) != BERR_SUCCESS) {
        fprintf(stderr, "BKNI_CreateEvent failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    start_params.buffer_available_cb = standard_callback;
    start_params.callback_context = playback_event;
  
    if (bdvd_playback_start(playback, &start_params) != bdvd_ok) {
        fprintf(stderr, "bdvd_playback_start failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    window = bdvd_decode_window_open(B_ID(0), display);
    if (window == NULL) {
        fprintf(stderr, "bdvd_decode_window_open failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    if (bdvd_decode_window_get(window, &temp_window_settings) != bdvd_ok) {
        fprintf(stderr, "bdvd_decode_window_get failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    if (window_settings) {
        if (window_settings_flags & playback_window_settings_visible) {
            temp_window_settings.visible = window_settings->visible;
        }
    
        if (window_settings_flags & playback_window_settings_zorder) {
            temp_window_settings.zorder = window_settings->zorder;
        }
    
        if (window_settings_flags & playback_window_settings_luma) {
            temp_window_settings.lumakey_en = window_settings->lumakey_en;
            temp_window_settings.lumakey_lower = window_settings->lumakey_lower;
            temp_window_settings.lumakey_upper = window_settings->lumakey_upper;
            temp_window_settings.lumakey_mask = window_settings->lumakey_mask;
        }
    
        if (window_settings_flags & playback_window_settings_position) {
            temp_window_settings.position.x = window_settings->position.x;
            temp_window_settings.position.y = window_settings->position.y;
            temp_window_settings.position.width = window_settings->position.width;
            temp_window_settings.position.height = window_settings->position.height;
        }
    }

    if (window_settings_flags == playback_window_settings_none) {
        temp_window_settings.position.x = 0;
        temp_window_settings.position.y = 0;
        temp_window_settings.position.width = 0;
        temp_window_settings.position.height = 0;
    }

    if (bdvd_decode_window_set(window, &temp_window_settings) != bdvd_ok) {
        fprintf(stderr, "bdvd_decode_window_set failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    decode = bdvd_decode_open(B_ID(0), pts_callback, pts_callback ? pts_callback_context : NULL);
    if (decode == NULL) {
        fprintf(stderr, "bdvd_decode_open failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }
    
    if (pts_callback) {
        if (bdvd_decode_notify_wait(decode, bdvd_decode_event_timestamps, bdvd_decode_notify_always, 0) != bdvd_ok) {
            fprintf(stderr, "bdvd_decode_notify_wait failed\n");
            ret = bdvd_err_unspecified;
            goto out;
        }
        if (bdvd_decode_notify_wait(decode, bdvd_decode_event_video_presentation_start, bdvd_decode_notify_always, 0) != bdvd_ok) {
            fprintf(stderr, "bdvd_decode_notify_wait failed\n");
            ret = bdvd_err_unspecified;
            goto out;
        }
        if (bdvd_decode_notify_wait(decode, bdvd_decode_event_video_decode_done, bdvd_decode_notify_always, 0) != bdvd_ok) {
            fprintf(stderr, "bdvd_decode_notify_wait failed\n");
            ret = bdvd_err_unspecified;
            goto out;
        }
    }

    video_params.pid          = video_pid;
    video_params.stream_id    = 0x00;
    video_params.substream_id = 0x00;
    video_params.format       = video_codec;
    video_params.playback_id  = B_ID(0);

    if (bdvd_decode_select_video_stream(decode, &video_params) != bdvd_ok) {
        fprintf(stderr, "bdvd_decode_select_video_stream failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    audio_params.pid          = audio_pid;
    audio_params.stream_id    = 0x00;
    audio_params.substream_id = 0x00;
    audio_params.format       = audio_codec;
    audio_params.playback_id  = B_ID(0);

    if (bdvd_decode_select_audio_stream(decode, &audio_params) != bdvd_ok) {
        fprintf(stderr, "bdvd_decode_select_audio_stream failed\n");
        ret = bdvd_err_unspecified;
        goto out;
    }

    if (pthread_create(&playback_thread, NULL, playback_thread_routine, &playback_routine_args) < 0)
    {
        fprintf(stderr, "pthread_create failed\n");
        ret = bdvd_err_unspecified;
    }

out:

    return ret;
}

bdvd_result_e playbackscagat_set_state(playback_state_e state) {
    bdvd_result_e ret = bdvd_ok;

    temp_state = state;

    if (broadcast_wakeup_call(input_event) != DFB_OK) {
        fprintf(stderr, "broadcast_wakeup_call failed\n");
        ret = bdvd_err_unspecified;
    }

    return ret;
}

bdvd_result_e playbackscagat_stop(void)
{
    bdvd_result_e ret = bdvd_ok;

    if (playback_thread)
    {
        bdvd_playback_stop(playback);

        playback_routine_args.cancelled = true;
        
        pthread_join(playback_thread, NULL);
    }

    if (playback)
    {
        if (bdvd_playback_close(playback) != bdvd_ok)
        {
            fprintf(stderr, "bdvd_playback_close failed!\n");
            ret = bdvd_err_unspecified;
            /* fall through */
        }
    }

    if (decode)
    {
        if (bdvd_decode_close(decode) != bdvd_ok)
        {
            fprintf(stderr, "bdvd_decode_close failed!\n");
            ret = bdvd_err_unspecified;
            /* fall through */
        }
    }

    if (window)
    {
        if (bdvd_decode_window_close(window) != bdvd_ok)
        {
            fprintf(stderr, "bdvd_decode_window_close failed!\n");
            ret = bdvd_err_unspecified;
            /* fall through */
        }
    }

    if (payload_buffer) {
        if (bdvd_playback_free_buffer(payload_buffer) != bdvd_ok) {
            fprintf(stderr, "*** bdvd_playback_free_buffer error\n");
            ret = bdvd_err_unspecified;
            /* fall through */
        }
    }

    if (ts_fd != -1) {
        if (close(ts_fd) == -1) {
            fprintf(stderr, "Error closing fd %d: %s\n", ts_fd, strerror(errno));
            ret = bdvd_err_unspecified;
            /* fall through */
        }
    }
    
    return ret;
}
