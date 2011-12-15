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
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_gfxperfthread.h"

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

int main( int argc, char *argv[] )
{
     int ret = EXIT_SUCCESS;

     bdvd_decode_window_settings_t window_settings, backup_window_settings;
     playback_window_settings_flags_e window_settings_flags;

     bdvd_display_h bdisplay = NULL;

     bvideo_format video_format = bvideo_format_1080i;
     float normx, normy, normwidth, normheight;

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

     if (display_set_video_format(bdisplay, video_format) != DFB_OK) {
         D_ERROR("display_set_video_format failed\n");
         ret = EXIT_FAILURE;
         goto out;
     }

    if (playbackscagat_start_with_settings(bdisplay, playback_window_settings_none, &window_settings, NULL, NULL, NULL, argc, argv) != bdvd_ok) {
        D_ERROR("playbackscagat_start_with_settings failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (playbackscagat_window_settings_get(&backup_window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (playbackscagat_window_settings_get(&backup_window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    window_settings = backup_window_settings;

    printf("Press any key to continue\n");
    getchar();

    printf("1- Testing position\n");

    window_settings.position.x = window_settings.position.width/4;
    window_settings.position.y = window_settings.position.height/4;
    window_settings.position.width /= 2;
    window_settings.position.height /= 2;

    printf("Position will now be set at %d %d %u %u\n",
        window_settings.position.x, window_settings.position.y,
        window_settings.position.width, window_settings.position.height);

    if (playbackscagat_window_settings_set(playback_window_settings_position, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to continue\n");
    getchar();

    printf("2- Testing cliprect\n");

    window_settings.cliprect.x = window_settings.position.width/4;
    window_settings.cliprect.y = window_settings.position.height/4;
    window_settings.cliprect.width = window_settings.position.width/2;
    window_settings.cliprect.height = window_settings.position.height/2;

    printf("Cliprect will now be set at %d %d %u %u\n",
        window_settings.cliprect.x, window_settings.cliprect.y,
        window_settings.cliprect.width, window_settings.cliprect.height);

    if (playbackscagat_window_settings_set(playback_window_settings_cliprect, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to continue\n");
    getchar();

    printf("3a- Testing position\n");

   // float normx, normy, normwidth, normheight;

    normx = (float)window_settings.cliprect.x/(float)window_settings.position.width;
    normy = (float)window_settings.cliprect.y/(float)window_settings.position.height;
    normwidth = (float)window_settings.cliprect.width/(float)window_settings.position.width;
    normheight = (float)window_settings.cliprect.height/(float)window_settings.position.height;

    window_settings = backup_window_settings;

    window_settings.cliprect.x = normx*window_settings.position.width;
    window_settings.cliprect.y = normy*window_settings.position.height;
    window_settings.cliprect.width = normwidth*window_settings.position.width;
    window_settings.cliprect.height = normheight*window_settings.position.height;

    printf("Position will now be set at %d %d %u %u\n",
        window_settings.position.x, window_settings.position.y,
        window_settings.position.width, window_settings.position.height);

    if (playbackscagat_window_settings_set(playback_window_settings_position | playback_window_settings_cliprect, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to continue\n");
    getchar();

    printf("3b- Testing cliprect\n");

    window_settings.cliprect.x = 0;
    window_settings.cliprect.y = 0;
    window_settings.cliprect.width = window_settings.position.width;
    window_settings.cliprect.height = window_settings.position.height;

    printf("Cliprect will now be set at %d %d %u %u\n",
        window_settings.cliprect.x, window_settings.cliprect.y,
        window_settings.cliprect.width, window_settings.cliprect.height);

    if (playbackscagat_window_settings_set(playback_window_settings_cliprect, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to continue\n");
    getchar();

    printf("5- Testing cliprect\n");

    window_settings.cliprect.x = window_settings.position.width/4;
    window_settings.cliprect.y = window_settings.position.height/4;
    window_settings.cliprect.width = window_settings.position.width/2;
    window_settings.cliprect.height = window_settings.position.height/2;

    printf("Cliprect will now be set at %d %d %u %u\n",
        window_settings.cliprect.x, window_settings.cliprect.x,
        window_settings.cliprect.width, window_settings.cliprect.height);

    if (playbackscagat_window_settings_set(playback_window_settings_cliprect, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to continue\n");
    getchar();

    printf("6- Testing cliprect\n");

    window_settings.cliprect.x = window_settings.position.x;
    window_settings.cliprect.y = window_settings.position.y;
    window_settings.cliprect.width = window_settings.position.width;
    window_settings.cliprect.height = window_settings.position.height;

    printf("Cliprect will now be set at %d %d %u %u\n",
        window_settings.cliprect.x, window_settings.cliprect.y,
        window_settings.cliprect.width, window_settings.cliprect.height);

    if (playbackscagat_window_settings_set(playback_window_settings_cliprect, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to continue\n");
    getchar();

    printf("7- Testing position and cliprect\n");

    window_settings.position.x = window_settings.position.width/4;
    window_settings.position.y = window_settings.position.height/4;
    window_settings.position.width /= 2;
    window_settings.position.height /= 2;

    window_settings.cliprect.x = window_settings.position.width*0.25;
    window_settings.cliprect.y = window_settings.position.height*0.25;
    window_settings.cliprect.width = window_settings.position.width*0.5;
    window_settings.cliprect.height = window_settings.position.height*0.5;

    printf("Position will now be set at %d %d %u %u\n",
        window_settings.position.x, window_settings.position.y,
        window_settings.position.width, window_settings.position.height);

    printf("Cliprect will now be set at %d %d %u %u\n",
        window_settings.cliprect.x, window_settings.cliprect.y,
        window_settings.cliprect.width, window_settings.cliprect.height);

    if (playbackscagat_window_settings_set(playback_window_settings_position | playback_window_settings_cliprect, &window_settings) != bdvd_ok) {
        D_ERROR("playbackscagat_window_settings_set failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    printf("Press any key to stop\n");
    getchar();

    playbackscagat_stop();

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

    return ret;
}

    