
void playbackrinbuf_usage(void);

bdvd_result_e playbackrinbuf_start(bdvd_display_h display,
    bdvd_decode_notify_cb_t pts_callback,
    void * pts_callback_context,
    int argc, char *argv[]);

void playbackrinbuf_stop(void);


