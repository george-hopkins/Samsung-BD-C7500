/***************************************************************************
 *     Copyright (c) 2005-2007, Broadcom Corporation
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
#include "bstd.h"
#include "bkni.h"
#include "bkni_multi.h"
#include <fcntl.h>

#define MAX_READ         (32*1024)
#define BUFFER_SIZE      (256*1024)
#define NUM_DESCRIPTORS  (32)

#define VID_PID 0x1011
#define AUD_PID 0x1100
#define PKT_PID 0x1200

static int audio_pid  = AUD_PID;
static int video_pid  = VID_PID;
static int video_codec = bdvd_video_codec_mpeg2;

static int loop=0;
static int bluray=0;

#define MAX_FILES 10
static int file[MAX_FILES];
static struct
{
    char name[100];
    bool still;
} content_file[MAX_FILES];
static int num_files=0;
static int current_file=0;

static bdvd_decode_h decode = NULL;
static bdvd_decode_window_h window = NULL;
static bdvd_playback_h playback = NULL;

static pthread_t playback_thread = (pthread_t)NULL;

typedef struct {
    bool cancelled;
} playback_routine_arguments;

playback_routine_arguments playback_routine_args;

/* Private */

static void * playback_thread_routine(void * arg)
{
    playback_routine_arguments * playback_args = (playback_routine_arguments *)arg;

    int file_pos = 0;
    struct stat file_info;    
    uint8_t *buffer;
    size_t buffer_size;
    int n;
    
    /* 
     * Initially we want to fill up all of the descriptors available so
     * that each each get_buffer will give us one descriptor's worth of buffering
     * and we'll be able to keep the ring buffer close to # descriptors - 1
     * full.
     */
    stat(content_file[current_file].name, &file_info);
    if (bdvd_decode_start(decode, window) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_decode_start failed!\n");
        goto quit;
    }

    while (!playback_args->cancelled)
    {
        if (file_pos < file_info.st_size)
        {
            if (bdvd_playback_get_buffer(playback, &buffer, &buffer_size) == bdvd_ok)
            {
                /* check the buffer space available */
                if ( (buffer == NULL) || (buffer_size < MAX_READ/2) )
                {
                    sched_yield();
                    continue;
                }
                else
                {
                    buffer_size = (buffer_size > MAX_READ) ? MAX_READ : buffer_size;
                    n = read(file[current_file], buffer, buffer_size);
                    if (n > 0)
                    {
                        file_pos += n;
                        if (bdvd_playback_buffer_ready(playback, 0, n) != bdvd_ok)
                        {
                            fprintf(stderr, "bdvd_playback_buffer_ready failed\n");
                            goto quit;
                        }
                    }
                }
            }
        }
        else if (current_file < num_files-1)
        {
            if (content_file[current_file].still)
            {
                if (bdvd_decode_stop(decode, true) != bdvd_ok)
                {
                    fprintf(stderr, "bdvd_decode_stop failed\n");
                    goto quit;
                }
                if (bdvd_decode_start(decode, window) != bdvd_ok)
                {
                    fprintf(stderr, "bdvd_decode_start failed\n");
                    goto quit;
                }
            }
            sleep(1);
            fprintf(stderr, "Starting next stream...\n");
            current_file++;
            lseek(file[current_file], 0, 0);
            file_pos = 0;
            stat(content_file[current_file].name, &file_info);
        }
        else if (loop)
        {
            if (content_file[current_file].still)
            {
                if (bdvd_decode_stop(decode, true) != bdvd_ok)
                {
                    fprintf(stderr, "bdvd_decode_stop failed\n");
                    goto quit;
                }
                if (bdvd_decode_start(decode, window) != bdvd_ok)
                {
                    fprintf(stderr, "bdvd_decode_start failed\n");
                    goto quit;
                }
            }
            sleep(1);
            printf("Looping...\n");
            current_file = 0;
            lseek(file[current_file], 0, 0);
            file_pos = 0;
            stat(content_file[current_file].name, &file_info);
            if (loop != -1)
            {
                loop--;
            }
        }
        else
        {
            printf("End of stream(s).\n");
            if (content_file[current_file].still)
            {
                if (bdvd_decode_stop(decode, true) != bdvd_ok)
                {
                    fprintf(stderr, "bdvd_decode_stop failed\n");
                }
            }
            goto quit;
        }
    }
    
quit:
    return NULL;
    
}

static void standard_callback(void *event)
{
    BKNI_SetEvent((BKNI_EventHandle)event);
}

static int open_files(void)
{
    int i;
    int rc = bdvd_ok;

    if (num_files == 0) {
        return bdvd_err_unspecified;
    }

    for (i=0; i<num_files; i++)
    {
        file[i] = open(content_file[i].name, O_RDONLY | O_LARGEFILE, 0444);
        if (file[i] < 0)
        {
            fprintf(stderr, "open failed on file %s\n",content_file[i].name);
            rc = bdvd_err_unspecified;
            break;
        }
    }

    return rc;
}

static int close_files(void)
{
    int i;

    for (i=0; i<num_files; i++)
    {
        if (file[i] != -1)
        {
            close(file[i]);
        }
    }

    return bdvd_ok;
}

/* Public */

void playbackrinbuf_usage(void)
{
    fprintf(stderr, "\nDVD API HDMV Play Application\n");
    fprintf(stderr, "\nUsage:");
    fprintf(stderr, "\nplayback <args>\n");
    fprintf(stderr, "\nArgument:              Description:");
    fprintf(stderr, "\n-file <val>            Source content file");
    fprintf(stderr, "\n-video_pid <val>       Video packet ID (default is %d)", VID_PID);
    fprintf(stderr, "\n-audio_pid <val>       Audio packet ID (default is %d)", AUD_PID);
    fprintf(stderr, "\n-video_codec <val>     MPEG2=2, AVC=27 (default is 2)");
    fprintf(stderr, "\n-loop [<val>]          Loop playback indefinitely (or optional <val> times)");
    fprintf(stderr, "\n-stillfile             Still image file");
    fprintf(stderr, "\n-bluray                Files are all HDMV format (default is program stream)");
    fprintf(stderr, "\n\nExample:\n");
    fprintf(stderr, "\-file stream.m2ts -stillfile still.m2ts -bluray -loop\n\n");
}

int playbackrinbuf_start(bdvd_display_h display, bdvd_decode_notify_cb_t pts_callback, void * pts_callback_context, int argc, char *argv[])
{
    int i;
    BKNI_EventHandle playback_event;
    bdvd_playback_open_params_t playback_params;
    bdvd_playback_start_params_t start_params;
    bdvd_decode_audio_params_t audio_params;
    bdvd_decode_video_params_t video_params;

    memset(&playback_routine_args, 0, sizeof(playback_routine_args));
    
    for (i = 0; i < MAX_FILES; i++)
    {
        file[i] = -1;
    }
    
    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-file"))
        {
            strcpy(content_file[num_files].name, argv[++i]);
            content_file[num_files++].still = false;
            if (num_files >= MAX_FILES)
            {
                close_files();
                return bdvd_err_unspecified;
            }
        }
        else if (!strcmp(argv[i], "-video_pid"))
        {
            video_pid = atoi(argv[++i]);
            if (!video_pid)
            {
                playbackrinbuf_usage();
                return bdvd_err_unspecified;
            }
        }
        else if (!strcmp(argv[i], "-audio_pid"))
        {
            audio_pid = atoi(argv[++i]);
            if (!audio_pid)
            {
                playbackrinbuf_usage();
                return bdvd_err_unspecified;
            }
        }
        else if (!strcmp(argv[i], "-video_codec"))
        {
            video_codec = atoi(argv[++i]);
            if (!video_codec)
            {
                playbackrinbuf_usage();
                return bdvd_err_unspecified;
            }
        }
        else if (!strcmp(argv[i], "-loop"))
        {
            if (i < argc - 1)
            {
                loop = atoi(argv[++i]);
                if (!loop)
                {
                    loop = -1;
                    i--;
                }
            }
            else
            {
                loop = -1;
            }
        }
        else if (!strcmp(argv[i], "-stillfile"))
        {
            strcpy(content_file[num_files].name, argv[++i]);
            content_file[num_files++].still = true;
            if (num_files >= MAX_FILES)
            {
                close_files();
                return bdvd_err_unspecified;
            }
        }
        else if (!strcmp(argv[i], "-bluray"))
        {
            bluray = 1;
        }
    }

    if (open_files() != bdvd_ok)
    {
        return bdvd_err_unspecified;
    }

    if (bluray)
      playback_params.stream_type = bdvd_playback_stream_type_bd;
    else
      playback_params.stream_type = bdvd_playback_stream_type_ps;

    playback_params.buffer_size = BUFFER_SIZE;
    playback_params.alignment = 0;
    playback_params.num_descriptors = NUM_DESCRIPTORS;

    playback = bdvd_playback_open(B_ID(0), &playback_params);
    if (playback == NULL)
    {
        fprintf(stderr, "bdvd_playback_open failed\n");
        return bdvd_err_unspecified;
    }

    if (BKNI_CreateEvent(&playback_event) != BERR_SUCCESS)
    {
        fprintf(stderr, "BKNI_CreateEvent failed\n");
        return bdvd_err_unspecified;
    }

    start_params.buffer_available_cb = standard_callback;
    start_params.callback_context = playback_event;

    if (bdvd_playback_start(playback, &start_params) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_playback_start failed\n");
        return bdvd_err_unspecified;
    }

    window = bdvd_decode_window_open(B_ID(0), display);
    if (window == NULL)
    {
        fprintf(stderr, "bdvd_decode_window_open failed\n");
        return bdvd_err_unspecified;
    }

    decode = bdvd_decode_open(B_ID(0), pts_callback, pts_callback ? pts_callback_context : NULL);
    if (decode == NULL)
    {
        fprintf(stderr, "bdvd_decode_open failed\n");
        return bdvd_err_unspecified;
    }

    if (bluray)
    {
        video_params.pid = video_pid;
    }
    else
    {
        video_params.stream_id = 0xE0;
    }

    video_params.format = video_codec;
    video_params.playback_id = B_ID(0);

    if (bdvd_decode_select_video_stream(decode, &video_params) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_decode_select_video_stream failed\n");
        return bdvd_err_unspecified;
    }

    if (bluray)
    {
        audio_params.pid = audio_pid;
    }
    else
    {
        audio_params.stream_id    = 0xBD;
        audio_params.substream_id = 0x80;
    }

    audio_params.format = bdvd_audio_format_ac3;
    audio_params.playback_id = B_ID(0);

    if (bdvd_decode_select_audio_stream(decode, &audio_params) != bdvd_ok)
    {
        fprintf(stderr, "bdvd_decode_select_audio_stream failed\n");
        return bdvd_err_unspecified;
    }

    if (pthread_create(&playback_thread, NULL, playback_thread_routine, &playback_routine_args) < 0)
    {
        fprintf(stderr, "pthread_create failed\n");
        return bdvd_err_unspecified;
    }

    return bdvd_ok;
}

void playbackrinbuf_stop(void)
{
    playback_routine_args.cancelled = true;
    
    if (playback_thread)
    {
        pthread_join(playback_thread, NULL);
    }
    
    if (playback)
    {
        if (bdvd_playback_close(playback) != bdvd_ok)
        {
            fprintf(stderr, "bdvd_playback_close failed!\n");
        }
    }

    if (decode)
    {
        if (bdvd_decode_close(decode) != bdvd_ok)
        {
            fprintf(stderr, "bdvd_decode_close failed!\n");
        }
    }

    if (window)
    {
        if (bdvd_decode_window_close(window) != bdvd_ok)
        {
            fprintf(stderr, "bdvd_decode_window_close failed!\n");
        }
    }
    
    close_files();
}
