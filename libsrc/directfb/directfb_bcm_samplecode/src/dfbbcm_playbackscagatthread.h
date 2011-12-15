
#include "dfbbcm_utils.h"

typedef enum {
    playback_window_settings_none     = 0x00000000,

    playback_window_settings_visible  = 0x00000001,
    playback_window_settings_zorder   = 0x00000002,
    playback_window_settings_luma     = 0x00000004,
    playback_window_settings_position = 0x00000008,
    playback_window_settings_cliprect = 0x00000010,
    playback_window_settings_all      = 0x0000001F
} playback_window_settings_flags_e;

typedef enum {
    playback_state_play,
    playback_state_pause,
    playback_state_frame_advance,
    playback_state_ff_ip,
    playback_state_ff_i,
    playback_state_sf_1_2,
    playback_state_sf_1_4,
    playback_state_sf_1_8,
    playback_state_sf_1_16
} playback_state_e;

void
playbackscagat_usage(void);

bdvd_result_e
playbackscagat_start(bdvd_display_h display,
                     wakeup_call_t * wakeup_call,
                     bdvd_decode_notify_cb_t pts_callback,
                     void * pts_callback_context,
                     int argc, char *argv[]);

bdvd_result_e
playbackscagat_start_with_settings(
                    bdvd_display_h display,
                    playback_window_settings_flags_e window_settings_flags,
                    bdvd_decode_window_settings_t * window_settings,
                    wakeup_call_t * wakeup_call,
                    bdvd_decode_notify_cb_t pts_callback,
                    void * pts_callback_context,
                    int argc, char *argv[]);

bdvd_result_e
playbackscagat_set_state(playback_state_e state);

bdvd_result_e
playbackscagat_window_settings_set(
                    playback_window_settings_flags_e window_settings_flags,
                    bdvd_decode_window_settings_t * window_settings);

bdvd_result_e
playbackscagat_decode_get_status(bdvd_decode_status_t * decode_status);

bdvd_result_e
playbackscagat_window_settings_get(
                    bdvd_decode_window_settings_t * window_settings);

bdvd_result_e
playbackscagat_stop(void);
