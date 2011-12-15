/* use software mpeg-2 decoder */
#define BRCM_SW_MPEG2

#ifdef BRCM_SW_MPEG2

/*
 * Using SW MPEG-2 Decoder
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef BDVD_PRCTL
#include <sys/prctl.h>
#endif

#include <directfb.h>

#include <media/idirectfbvideoprovider.h>
#include <media/idirectfbdatabuffer.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/state.h>
#include <core/gfxcard.h>
#include <core/layers.h>
#include <core/layer_control.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <display/idirectfbsurface.h>

#include <misc/gfx_util.h>

#include <misc/util.h>

#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/thread.h>
#include <direct/util.h>

#include <bcm/bcmcore.h>
#include <bdvd.h>
#include "bstd.h"
#include "bkni.h"
#include "bkni_multi.h"

#include "../IDirectFBImageProvider/mpeg2/mpeg2dec.h"

static int  mpeg2_read_func ( void *buf, int count, void *ctx );
static void mpeg2_write_func( int x, int y, __u32 argb, void *ctx );
static void mpeg2_scale_decode_image(void *ctx);


#define PRINT D_INFO
//#undef D_DEBUG
//#define D_DEBUG PRINT


#define MAX_READ         (8*1024)
#define BUFFER_SIZE      (128*1024)
#define NUM_DESCRIPTORS  (16)

//B_ID() values that this plugin uses
#define DECODER_ID    1
#define PLAYBACK_ID   1
#define WINDOW_ID     1
#define VGRAPHICS_ID  1

static DFBResult
Probe( IDirectFBVideoProvider_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBVideoProvider *thiz,
           IDirectFBDataBuffer    *buffer );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBVideoProvider, BCM )

#if 0
#define DEBUG_MSG(...) fprintf(stderr,__VA_ARGS__);
#else
#define DEBUG_MSG  do {} while (0)
#endif

static DFBResult IDirectFBVideoProvider_bcm_Stop(IDirectFBVideoProvider* thiz);

/*
 * private data struct of IDirectFBVideoProvider
 */
typedef struct {
    int                  ref;       /* reference counter */
    IDirectFBDataBuffer *buffer;

    //thread that pumps data
    pthread_t            video_decode_thread;

    DFBVideoProviderStatus status;

    //frame callback and its associated user context
    DVFrameCallback      callback;
    void                *ctx;

    //dimensions of decoded frames
    int                  width;
    int                  height;

    // software implementation
    MPEG2_Decoder         *dec;
    __u32                 *image;
    DFBSurfaceDescription  surface_desc;
    void                  *dst;
    int                    pitch;
    CoreSurface           *dst_surface;
    DFBRectangle           rect;
    DFBRectangle           *dstrect;
    IDirectFBSurface       *destination;
} IDirectFBVideoProvider_bcm_data;

static DFBResult IDirectFBVideoProvider_bcm_GetCapabilities (
                                                            IDirectFBVideoProvider       *thiz,
                                                            DFBVideoProviderCapabilities *caps )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (!caps)
          return DFB_INVARG;

     *caps = ( DVCAPS_BASIC      |
               DVCAPS_INTERLACED |
               DVCAPS_SCALE);

     return DFB_OK;
}

static DFBResult
IDirectFBVideoProvider_bcm_GetSurfaceDescription( IDirectFBVideoProvider *thiz,
                                                    DFBSurfaceDescription *dsc )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (data->surface_desc.flags == 0) {
         data->surface_desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
         data->surface_desc.width       = data->width;
         data->surface_desc.height      = data->height;
         data->surface_desc.pixelformat = dfb_primary_layer_pixelformat();
     }

     *dsc = data->surface_desc;

     return DFB_OK;
}

static inline uint32_t gettimeus()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000000 + tv.tv_usec);
}

unsigned int decode_and_scaling_start_time;
unsigned int decode_and_scaling_stop_time;

static void*
video_decode_thread(void* context)
{
    DFBResult result = DFB_OK;
    IDirectFBVideoProvider_bcm_data* data=(IDirectFBVideoProvider_bcm_data*)context;

    D_DEBUG("video_decode_thread started\n");

    decode_and_scaling_start_time = gettimeus();

    /* enter in play state */
    data->status = DVSTATE_PLAY;

    /* decode video sequece */
    if (MPEG2_DecodeAndScale( data->dec, mpeg2_write_func, data, mpeg2_scale_decode_image ))
    {
        D_INFO("MPEG2_Decode failed\n");
        return NULL;
    }

    /* sequence finished or issue stop command called */
    data->status = DVSTATE_FINISHED;

    D_DEBUG("video_decode_thread finished\n");

    return NULL;
}

static DFBResult IDirectFBVideoProvider_bcm_PlayTo(
                                                  IDirectFBVideoProvider *thiz,
                                                  IDirectFBSurface       *destination,
                                                  const DFBRectangle     *dstrect,
                                                  DVFrameCallback         callback,
                                                  void                   *ctx )
{
    DFBResult result = DFB_OK;

    DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

    /* make sure the parameters are good to proceed */
    if ((destination == NULL) ||
        (destination->priv == NULL) ||
        (((IDirectFBSurface_data*)destination->priv)->surface == NULL))
    {
        return DFB_INVARG;
    }

    D_DEBUG("IDirectFBVideoProvider_bcm_PlayTo enter\n");

    /* save parameters */
    data->destination = destination;
    data->dst_surface = ((IDirectFBSurface_data*)data->destination->priv)->surface;;
    data->dstrect     = dstrect;
    data->callback    = callback;
    data->ctx         = ctx;

    /* create thread */
    if (pthread_create(&data->video_decode_thread, NULL, video_decode_thread, data) != 0)
    {
        D_ERROR("cant spawn video_decode_threadproc\n");
        result=DFB_FAILURE;
    }

    /* wait unitl thread is spawned and decode state is play */
    while (data->status != DVSTATE_PLAY) {;
        BKNI_Sleep(1);
    }

    D_DEBUG("IDirectFBVideoProvider_bcm_PlayTo exit\n");

    return result;
}



static DFBResult IDirectFBVideoProvider_bcm_Stop(
                                                IDirectFBVideoProvider *thiz )
{
    DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

    D_DEBUG("IDirectFBVideoProvider_bcm_Stop started\n");

    /* stop state */
    data->status = DVSTATE_STOP;

    MPEG2_Stop(data->dec);

    /* wait unitl current image decoding is completed, state is finished */
    while (data->status != DVSTATE_FINISHED) {;
        BKNI_Sleep(10);
    }

    D_DEBUG("IDirectFBVideoProvider_bcm_Stop done\n");

    return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_GetStatus(
                                                     IDirectFBVideoProvider *thiz,
                                                     DFBVideoProviderStatus *status )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (!status)
          return DFB_INVARG;

     *status = data->status;

     return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_SeekTo(
                                                  IDirectFBVideoProvider *thiz,
                                                  double                  seconds )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNIMPLEMENTED;
}

static DFBResult IDirectFBVideoProvider_bcm_GetPos(
                                                  IDirectFBVideoProvider *thiz,
                                                  double                 *seconds )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_GetLength(
                                                     IDirectFBVideoProvider *thiz,
                                                     double                 *seconds )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_GetColorAdjustment(
                                                              IDirectFBVideoProvider *thiz,
                                                              DFBColorAdjustment     *adj )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_SetColorAdjustment( IDirectFBVideoProvider   *thiz,
                                                                const DFBColorAdjustment *adj )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_SendEvent( IDirectFBVideoProvider *thiz,
                                                       const DFBEvent         *evt )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

/* exported symbols */

static DFBResult
Probe( IDirectFBVideoProvider_ProbeContext *ctx )
{
    /* Look at the header of the mpeg file, let's validate it's indeed mpeg es */

    //check that the underlying driver is BCM.  Otherwise, it wont work.
    //check for sequence header or picture header, both are legal for video drip.

    if( dfb_system_type() == CORE_BCM &&
        ctx->header[0] == 0x00 &&
        ctx->header[1] == 0x00 &&
        ctx->header[2] == 0x01 &&
        ( (ctx->header[3] == 0xB3) || (ctx->header[3] == 0x00) ) )
    {
        D_DEBUG("MPEG-2 stream found\n");
        return DFB_OK;
    }

    return DFB_UNSUPPORTED;
}

static void IDirectFBVideoProvider_bcm_Destruct( IDirectFBVideoProvider *thiz )
{
    IDirectFBVideoProvider_bcm_data *data = (IDirectFBVideoProvider_bcm_data*)thiz->priv;

    /* stop the plugin if it was running */
    if(data->status == DVSTATE_PLAY)
    {
        thiz->Stop(thiz);
    }

    /* Decrease the data buffer reference counter. */
    if (data->buffer)
    {
        data->buffer->Release( data->buffer );
    }

    /* Deallocate image data. */
    if (data->image)
    {
        D_FREE( data->image );
    }

    DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult IDirectFBVideoProvider_bcm_AddRef( IDirectFBVideoProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     data->ref++;

     return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_Release( IDirectFBVideoProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (--data->ref == 0)
     {
          IDirectFBVideoProvider_bcm_Destruct( thiz );
     }

     return DFB_OK;
}

static DFBResult
Construct( IDirectFBVideoProvider *thiz, IDirectFBDataBuffer *buffer )
{
    DFBResult ret = DFB_OK;

    D_DEBUG("Enter BRCM SW MPEG-2 Decoder Construct\n");

    /* allocate data structure */
    DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBVideoProvider_bcm)

    /* set decoder state */
    data->status = DVSTATE_UNKNOWN;

    /* set data buffer reference counter */
    data->ref = 1;
    data->buffer = buffer;

    /* Increase the data buffer reference counter. */
    buffer->AddRef( buffer );

    D_DEBUG("Constructing MPEG-2 decoder %p %p\n", mpeg2_read_func, buffer);

    /* Initialize mpeg2 decoding. */
    data->dec = MPEG2_Init( mpeg2_read_func, buffer, &data->width, &data->height );
    if (!data->dec)
    {
        D_ERROR("MPEG2_Init failed\n");
        goto error;
    }

    /* Allocate image data. */
    data->image = D_MALLOC( data->width * data->height * 4 );
    if (!data->image)
    {
        D_ERROR("Image buffer allocation failed\n");
        goto error;
    }

    /* set class methods */
    thiz->AddRef    = IDirectFBVideoProvider_bcm_AddRef;
    thiz->Release   = IDirectFBVideoProvider_bcm_Release;
    thiz->GetCapabilities = IDirectFBVideoProvider_bcm_GetCapabilities;
    thiz->GetSurfaceDescription = IDirectFBVideoProvider_bcm_GetSurfaceDescription;
    thiz->PlayTo    = IDirectFBVideoProvider_bcm_PlayTo;
    thiz->Stop      = IDirectFBVideoProvider_bcm_Stop;
    thiz->GetStatus = IDirectFBVideoProvider_bcm_GetStatus;
    thiz->SeekTo    = IDirectFBVideoProvider_bcm_SeekTo;
    thiz->GetPos    = IDirectFBVideoProvider_bcm_GetPos;
    thiz->GetLength = IDirectFBVideoProvider_bcm_GetLength;
    thiz->GetColorAdjustment = IDirectFBVideoProvider_bcm_GetColorAdjustment;
    thiz->SetColorAdjustment = IDirectFBVideoProvider_bcm_SetColorAdjustment;
    thiz->SendEvent = IDirectFBVideoProvider_bcm_SendEvent;

    D_DEBUG("Exit BRCM SW MPEG-2 Decoder Construct\n");

    return DFB_OK;

error:

    /* uninit decoder */
    if (data->dec)
    {
        MPEG2_Close(data->dec);
    }

    /* free output image buffer */
    if (data->image)
    {
        D_FREE(data->image);
    }

    /* release buffer */
    buffer->Release( buffer );

    DIRECT_DEALLOCATE_INTERFACE(thiz);

    return ret;
}

int
mpeg2_read_func( void *buf, int count, void *ctx )
{
     unsigned int         len;
     DFBResult            ret;
     IDirectFBDataBuffer *buffer = (IDirectFBDataBuffer*) ctx;

     buffer->WaitForData( buffer, 1 );

     ret = buffer->GetData( buffer, count, buf, &len );
     if (ret)
          return 0;

     return len;
}

void
mpeg2_write_func( int x, int y, __u32 argb, void *ctx )
{
     IDirectFBVideoProvider_bcm_data *data =
                                       (IDirectFBVideoProvider_bcm_data*) ctx;

     data->image[ data->width * y + x ] = argb;
}

void
mpeg2_scale_decode_image(void *ctx)
{
    IDirectFBVideoProvider_bcm_data *data =
                                       (IDirectFBVideoProvider_bcm_data*) ctx;

    data->rect.x=0;
    data->rect.y=0;
    data->rect.w=0;
    data->rect.h=0;

    if (data->destination->Lock( data->destination, DSLF_WRITE, &data->dst, &data->pitch ))
    {
        D_ERROR("mpeg2_scale_decode_image Lock failed\n");
        return NULL;
    }

    if (data->destination->GetSize(data->destination, &data->rect.w, &data->rect.h ))
    {
        D_ERROR("mpeg2_scale_decode_image GetSize failed\n");
        return NULL;
    }

    if ((data->dstrect == NULL) || dfb_rectangle_intersect ( &data->rect, data->dstrect ))
    {
        /* scale and post the image on the screen */
        dfb_scale_linear_32( data->image, data->width, data->height, data->dst, data->pitch, &data->rect, data->dst_surface );
    }

    if (data->destination->Unlock(data->destination))
    {
        D_ERROR("mpeg2_scale_decode_image Unlock failed\n");
        return NULL;
    }

    decode_and_scaling_stop_time = (gettimeus() - decode_and_scaling_start_time)/1000;

    D_DEBUG("Decode and Scale time is %dms\n", decode_and_scaling_stop_time);

    if (data->callback)
    {
        data->callback(data->ctx);
    }

    decode_and_scaling_start_time = gettimeus();
}

#else

/*
 * Using BRCM HW MPEG-2 Decoder
 */

/* */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef BDVD_PRCTL
#include <sys/prctl.h>
#endif

#include <directfb.h>

#include <media/idirectfbvideoprovider.h>
#include <media/idirectfbdatabuffer.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/state.h>
#include <core/gfxcard.h>
#include <core/layers.h>
#include <core/layer_control.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <display/idirectfbsurface.h>

#include <misc/gfx_util.h>

#include <misc/util.h>

#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/thread.h>
#include <direct/util.h>

#include <bcm/bcmcore.h>
#include <bdvd.h>
#include "bstd.h"
#include "bkni.h"
#include "bkni_multi.h"

#define PRINT D_INFO
//#undef D_DEBUG
//#define D_DEBUG PRINT


#define MAX_READ         (8*1024)
#define BUFFER_SIZE      (128*1024)
#define NUM_DESCRIPTORS  (16)

//B_ID() values that this plugin uses
#define DECODER_ID    1
#define PLAYBACK_ID   1
#define WINDOW_ID     1
#define VGRAPHICS_ID  1

static DFBResult
Probe( IDirectFBVideoProvider_ProbeContext *ctx );

static DFBResult
Construct( IDirectFBVideoProvider *thiz,
           IDirectFBDataBuffer    *buffer );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBVideoProvider, BCM )

#if 0
#define DEBUG_MSG(...) fprintf(stderr,__VA_ARGS__);
#else
#define DEBUG_MSG  do {} while (0)
#endif

static DFBResult IDirectFBVideoProvider_bcm_Stop(IDirectFBVideoProvider* thiz);

/*
 * private data struct of IDirectFBVideoProvider
 */
typedef struct {
    int                  ref;       /* reference counter */
    IDirectFBDataBuffer *buffer;

    //context of the bcm core
    BCMCoreContext* bcmcore_context;

    //handles to bdvd
    bdvd_decode_h decode;
    bdvd_decode_window_h window;
    bdvd_playback_h playback;
    BKNI_EventHandle playback_event;

    //thread that pumps data
    pthread_t            pump_thread;

    //whether the thread should continue pumping
    bool                 pumping;

    DFBVideoProviderStatus status;

    //frame callback and its associated user context
    DVFrameCallback      callback;
    void                *ctx;

    //dimensions of decoded frames
    int                  width;
    int                  height;

    pthread_mutex_t      lock;

} IDirectFBVideoProvider_bcm_data;

static void IDirectFBVideoProvider_bcm_Destruct( IDirectFBVideoProvider *thiz )
{
     IDirectFBVideoProvider_bcm_data *data =
     (IDirectFBVideoProvider_bcm_data*)thiz->priv;

    //stop the plugin if it was running
    if(data->status == DVSTATE_PLAY)
        thiz->Stop(thiz);

     pthread_mutex_destroy( &data->lock );

     /* Decrease the data buffer reference counter. */
     data->buffer->Release( data->buffer );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult IDirectFBVideoProvider_bcm_AddRef( IDirectFBVideoProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     data->ref++;

     return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_Release( IDirectFBVideoProvider *thiz )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (--data->ref == 0) {
          IDirectFBVideoProvider_bcm_Destruct( thiz );
     }

     return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_GetCapabilities (
                                                            IDirectFBVideoProvider       *thiz,
                                                            DFBVideoProviderCapabilities *caps )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (!caps)
          return DFB_INVARG;

     *caps = ( DVCAPS_BASIC      |
               DVCAPS_INTERLACED |
               DVCAPS_SCALE);

     return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_GetSurfaceDescription(
                                                                 IDirectFBVideoProvider *thiz,
                                                                 DFBSurfaceDescription  *desc )
{
    DFBResult result=DFB_OK;
    DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm);

    if (!thiz || !desc)
          return DFB_INVARG;


    unsigned char sequence_header[12];
    int bytes_read=0;

    result=data->buffer->PeekData(data->buffer,sizeof(sequence_header),0,sequence_header,&bytes_read);

    if(result==DFB_OK)
    {
        if( sequence_header[0] == 0x00 &&
            sequence_header[1] == 0x00 &&
            sequence_header[2] == 0x01 &&
            sequence_header[3] == 0xB3)
        {
            //STREAM BEGINS WITH SEQUENCE HEADER
            data->width=(sequence_header[4]<<4) | ((sequence_header[5]&0xF0)>>4);
            data->height=((sequence_header[5]&0x0F) <<8) | sequence_header[6];
        }
        else if( sequence_header[0] == 0x00 &&
                 sequence_header[1] == 0x00 &&
                 sequence_header[2] == 0x01 &&
                 sequence_header[3] == 0x00)
        {
            //STREAM BEGINS WITH PICTURE HEADER
            //there's no sequence header, we cant tell what the frame format is.  report SD resolution?
            data->width  = 720;
            data->height = 480;
        }
        else
            result=DFB_INVARG;
    }

    if(result==DFB_OK)
    {

        desc->flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
        desc->pixelformat = DSPF_UYVY;
        desc->width       = data->width;
        desc->height      = data->height;

        D_DEBUG("reporting resolution %d %d\n",desc->width,desc->height);
    }

    return result;
}

static void
standard_callback(void *event)
{
  BKNI_SetEvent((BKNI_EventHandle)event);
}

static void*
video_pump_threadproc(void* context)
{
    DFBResult result=DFB_OK;
    IDirectFBVideoProvider_bcm_data* data=(IDirectFBVideoProvider_bcm_data*)context;
    bdvd_decode_clip_info_t clip;

    D_DEBUG("video_pump_threadproc\n");

    D_DEBUG("bdvd_decode_notify_wait...\n");
#ifdef BDVD_PRCTL
        prctl(PR_SET_NAME, "video_pump", 0, 0.,0);
#endif
    if( bdvd_decode_notify_wait(
        data->decode,
        bdvd_decode_event_video_decode_done,
        bdvd_decode_notify_always,
        0) != bdvd_ok)
    {
        D_ERROR("bdvd_decode_notify_wait failed\n");
        result=DFB_FAILURE;
    }

    D_DEBUG("bdvd_decode_clip_info (pre)...\n");
    clip.info.time.start = BDVD_IGNORE_START_TIME;
    clip.info.time.stop = BDVD_IGNORE_STOP_TIME;
    clip.type = bdvd_decode_audio_pre_clip_info;
      bdvd_decode_clip_info(data->decode, &clip);
    clip.type = bdvd_decode_video_pre_clip_info;
      bdvd_decode_clip_info(data->decode, &clip);

    for(;;)
    {
        uint8_t *buffer;
        size_t buffer_size;
        unsigned int bytes_read;

        if(data->pumping==false || result!=DFB_OK)
            break;

        switch(bdvd_playback_get_buffer(data->playback, &buffer, &buffer_size))
        {
        case bdvd_ok:
        case bdvd_err_not_available:
            break;
        default:
            D_ERROR("bdvd_playback_get_buffer failed\n");
            return NULL;
        }

        if (buffer_size == 0) {
            BKNI_WaitForEvent(data->playback_event, BKNI_INFINITE);
            continue;
        }

        if (buffer_size > MAX_READ) {
            buffer_size = MAX_READ;
        }

        if(result==DFB_OK)
            result=data->buffer->WaitForDataWithTimeout(data->buffer,buffer_size,0,500);

        if(result==DFB_TIMEOUT)
        {
            D_DEBUG("TIMEOUT!\n");

            if(data->pumping==false)
                break;

            result=DFB_OK;
        }

        if(result==DFB_OK)
        {
            bytes_read=0;
            result=data->buffer->GetData(data->buffer,buffer_size,buffer,&bytes_read);

            if(result==DFB_BUFFEREMPTY)
            {
                D_DEBUG("videopump: end of file\n");
                //End of file!
                //dont signal an error, just leave
                result=DFB_OK;
                break;
            }
        }

        if(result==DFB_OK)
        {
          if (bytes_read > 0) {
            int busy = true;

            while (busy) {
              switch (bdvd_playback_buffer_ready(data->playback, 0, bytes_read)) {
              case bdvd_ok:
                busy = false;
                break;
              case bdvd_err_busy:
                BKNI_WaitForEvent(data->playback_event, BKNI_INFINITE);
                continue;
              default:
                D_ERROR("bdvd_playback_buffer_ready failed\n");
                return NULL;
              }
            }
          } else {
            D_ERROR("Unexpected end-of-file\n");
            break;
          }
        }
    }

    D_DEBUG("bdvd_decode_clip_info (post)...\n");
  clip.info.connection = bdvd_decode_connection_non_seamless;
  clip.type = bdvd_decode_audio_post_clip_info;
    bdvd_decode_clip_info(data->decode, &clip);
  clip.type = bdvd_decode_video_post_clip_info;
    bdvd_decode_clip_info(data->decode, &clip);

    if(result!=DFB_OK)
    {
        D_ERROR("in videoProviderBCM's pump thread, DirectFB error: %d\n",result);
    }

    data->pumping=false;

    return NULL;
}


static void decode_notify(
    bdvd_decode_h           decode,         /* [handle] from bdvd_decode_open */
    bdvd_decode_event_e     event,          /* [in] event causing callback    */
    void                    *event_data,    /* [in] data for event            */
    void                    *ctx            /* [in] ptr provided at open      */
)
{
    IDirectFBVideoProvider* thiz=(IDirectFBVideoProvider*)ctx;
    IDirectFBVideoProvider_bcm_data* data=(IDirectFBVideoProvider_bcm_data*)thiz->priv;

    D_DEBUG("decode_notify was called\n");

    if(event==bdvd_decode_event_video_decode_done)
    {
        D_DEBUG("DECODE REACHED END\n");
        data->status=DVSTATE_FINISHED;
    }
}

static DFBResult IDirectFBVideoProvider_bcm_PlayTo(
                                                  IDirectFBVideoProvider *thiz,
                                                  IDirectFBSurface       *destination,
                                                  const DFBRectangle     *dstrect,
                                                  DVFrameCallback         callback,
                                                  void                   *ctx )
{
    DFBResult result=DFB_OK;
    IDirectFBSurface_data *dst_data;
    bdvd_playback_start_params_t start_params;

    DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

    D_DEBUG("IDirectFBVideoProvider_bcm_PlayTo\n");

    if(data->pumping==true)
    {
        D_ERROR("already running, stop first!\n");
        return DFB_TEMPUNAVAIL;
    }

    if (destination==NULL)
        return DFB_INVARG;

    dst_data = (IDirectFBSurface_data*)destination->priv;
    if (dst_data==NULL)
        return DFB_DEAD;

    pthread_mutex_lock( &data->lock );

    //this was checked already in Probe()
    D_ASSERT(dfb_system_type() == CORE_BCM);

    data->bcmcore_context = (BCMCoreContext*)dfb_system_data();
    data->callback        = callback;
    data->ctx             = ctx;

    if(result==DFB_OK && data->bcmcore_context==NULL) {
        D_ERROR("cant acquire bcmcore context\n");
        result=DFB_FAILURE;
    }

    // if the vgraphics window was open, we have to close it
    if(result==DFB_OK && data->bcmcore_context->vgraphics_window.window_handle)
        bdvd_vgraphics_window_close(data->bcmcore_context->vgraphics_window.window_handle);

    D_DEBUG("opening playback...\n");
    if(result==DFB_OK)
    {
        bdvd_playback_open_params_t playback_params;

        playback_params.stream_type = bdvd_playback_stream_type_bd;
        playback_params.buffer_size = BUFFER_SIZE;
        playback_params.alignment = 0;
        playback_params.num_descriptors = NUM_DESCRIPTORS;

        data->playback = bdvd_playback_open(B_ID(PLAYBACK_ID), &playback_params);
        if (data->playback == NULL) {
          D_ERROR("bdvd_playback_open failed\n");
          result=DFB_FAILURE;
        }
    }

    D_DEBUG("creating playback event...\n");
    if(result==DFB_OK)
    {
      if (BKNI_CreateEvent(&data->playback_event) != BERR_SUCCESS) {
        D_ERROR("BKNI_CreateEvent failed\n");
        result=DFB_FAILURE;
      }
    }

    // start playback
    if(result==DFB_OK)
    {
      start_params.buffer_available_cb = standard_callback;
      start_params.callback_context = data->playback_event;

      if (bdvd_playback_start(data->playback, &start_params) != bdvd_ok) {
        D_ERROR("bdvd_playback_start failed\n");
        result=DFB_FAILURE;
      }
    }

    //open decode window
    if(result==DFB_OK)
    {
        data->window = bdvd_decode_window_open(B_ID(WINDOW_ID), (bdvd_display_h)data->bcmcore_context->primary_display_handle);
        if (data->window == NULL) {
            D_ERROR("bdvd_decode_window_open failed\n");
            result=DFB_FAILURE;
        }
    }

    //set decode window location
    if(result==DFB_OK)
    {
        bdvd_decode_window_settings_t settings;
        if( bdvd_decode_window_get(data->window,&settings) != bdvd_ok ) {
            D_ERROR("bdvd_decode_window_get failed\n");
            result=DFB_FAILURE;
        }

        if(dstrect)
        {
            //use the specified coordinates
            settings.position.x      = dstrect->x;
            settings.position.y      = dstrect->y;
            settings.position.width  = dstrect->w;
            settings.position.height = dstrect->h;
        }
        else
        {
            //if not specified, use full screen

            settings.position.x = 0;
            settings.position.y = 0;

            bdvd_display_settings_t dsettings;
            bdvd_display_get((bdvd_display_h)data->bcmcore_context->primary_display_handle,&dsettings);
            switch(dsettings.format)
            {
            case bdvd_video_format_ntsc:          /* NTSC Interlaced                       */
            case bdvd_video_format_ntsc_japan:    /* Japan NTSC, no pedestal level         */
            case bdvd_video_format_pal_m:         /* PAL Brazil                            */
            case bdvd_video_format_480p:          /* NTSC Progressive (27Mhz)              */
                settings.position.width  =  720;
                settings.position.height =  480;
                break;
            case bdvd_video_format_pal:           /* PAL Europe                            */
            case bdvd_video_format_pal_n:         /* PAL_N                                 */
            case bdvd_video_format_pal_nc:        /* PAL_N, Argentina                      */
            case bdvd_video_format_secam:         /* SECAM III B6                          */
            case bdvd_video_format_576p:          /* HD PAL Progressive 50Hz for Australia */
                settings.position.width  =  720;
                settings.position.height =  576;
                break;

            case bdvd_video_format_1080i:         /* HD 1080 Interlaced                    */
            case bdvd_video_format_1080i_50hz:    /* European 50Hz HD 1080                 */
                settings.position.width  = 1920;
                settings.position.height = 1080;
                break;
            case bdvd_video_format_720p:          /* HD 720 Progressive                    */
            case bdvd_video_format_720p_50hz:     /* HD 720p 50Hz for Australia            */
                settings.position.width  = 1280;
                settings.position.height =  720;
                break;
            default:
                D_ERROR("current video format of the display is unrecognized\n");
                result=DFB_INVARG;
                break;
            }
        }

        if( bdvd_decode_window_set(data->window,&settings) != bdvd_ok ) {
            D_ERROR("bdvd_decode_window_set failed\n");
            result=DFB_FAILURE;
        }
    }

    //opening decode
    if(result==DFB_OK)
    {
        data->decode = bdvd_decode_open(B_ID(DECODER_ID), decode_notify, thiz);
        if (data->decode == NULL) {
        D_ERROR("bdvd_decode_open failed\n");
        result=DFB_FAILURE;
        }
    }

    //set video streams
    if(result==DFB_OK)
    {
        bdvd_decode_video_params_t video_params;

        video_params.stream_id    = 0;
        video_params.substream_id = 0;
        video_params.format       = bdvd_video_codec_mpeg2;
        video_params.playback_id  = B_ID(PLAYBACK_ID);
        video_params.stc_offset   = 0;

        if (bdvd_decode_select_video_stream(data->decode, &video_params) != bdvd_ok) {
            D_ERROR("bdvd_decode_select_video_stream failed\n");
            result=DFB_FAILURE;
        }
    }

    /*D_DEBUG("disabling timestamp management...\n");
    if(result==DFB_OK)
    {
        if (bdvd_decode_set_tsm_mode(data->decode, false, false,
                                   bdvd_video_discnt_mode_normal,
                                   bdvd_audio_discnt_mode_normal) != bdvd_ok) {
            D_ERROR("bdvd_decode_set_tsm_mode failed!\n");
            result=DFB_FAILURE;
        }
    }*/

    D_DEBUG("setting decoder to play...\n");
    if(result==DFB_OK)
    {
        bdvd_decode_video_state_t decoder_state;

        decoder_state.length        = sizeof(bdvd_decode_video_state_t);
        decoder_state.discontiguous = true;
        decoder_state.direction     = bdvd_video_dir_forward;
        decoder_state.mode          = bdvd_video_decode_all;
        decoder_state.persistence   = BDVD_PERSISTENCE_PLAY;
        decoder_state.display_skip  = 0;

        if (bdvd_decode_set_video_state(data->decode, &decoder_state) != bdvd_ok) {
          D_ERROR("bdvd_decode_set_video_state failed!\n");
          result=DFB_FAILURE;
        }
    }

    //starting decoder
    if(result==DFB_OK)
    {
        if (bdvd_decode_start(data->decode, data->window) != bdvd_ok) {
          D_ERROR("bdvd_decode_start failed\n");
          result=DFB_FAILURE;
        }
    }

    if(result==DFB_OK)
    {
        data->pumping=true;
        data->status=DVSTATE_PLAY;

        //start a thread so we can return control to the app
        if( pthread_create(&data->pump_thread,NULL,video_pump_threadproc,data) != 0 )
        {
            D_ERROR("cant spawn thread\n");
            result=DFB_FAILURE;
        }
    }

    if(result!=DFB_OK)
    {
        D_ERROR("DirectFB error: %d! will try to clean up\n",result);
        if(data)
        {
            if(data->window)
            {
                bdvd_decode_window_close(data->window);
                data->window=NULL;
            }
            if(data->playback)
            {
                bdvd_playback_close(data->playback);
                data->playback=NULL;
            }
            if(data->decode)
            {
                bdvd_decode_close(data->decode);
                data->decode=NULL;
            }
            BKNI_DestroyEvent(data->playback_event);
        }
    }

    pthread_mutex_unlock( &data->lock );

    return result;
}

static DFBResult IDirectFBVideoProvider_bcm_Stop(
                                                IDirectFBVideoProvider *thiz )
{
    DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

    D_DEBUG("bcm_Stop!!\n");

    pthread_mutex_lock( &data->lock );

    if(data->pump_thread)
    {
        //signal the thread that it can terminate
        data->pumping=false;

        //wait for the thread to complete
        pthread_join(data->pump_thread,NULL);
        data->pump_thread=NULL;

        //cleanup bdvd
        bdvd_decode_stop(data->decode,true);
        bdvd_playback_stop(data->playback);

        bdvd_decode_window_close(data->window);
        data->window=NULL;
        bdvd_decode_close(data->decode);
        data->decode=NULL;
        bdvd_playback_close(data->playback);
        data->playback=NULL;

        BKNI_DestroyEvent(data->playback_event);

        data->bcmcore_context->vgraphics_window.window_handle =
            bdvd_vgraphics_window_open (
                B_ID(VGRAPHICS_ID),
                data->bcmcore_context->primary_display_handle
            );

        data->status=DVSTATE_STOP;
    }

    pthread_mutex_unlock( &data->lock );

    return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_GetStatus(
                                                     IDirectFBVideoProvider *thiz,
                                                     DFBVideoProviderStatus *status )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     if (!status)
          return DFB_INVARG;

     *status = data->status;

     return DFB_OK;
}

static DFBResult IDirectFBVideoProvider_bcm_SeekTo(
                                                  IDirectFBVideoProvider *thiz,
                                                  double                  seconds )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNIMPLEMENTED;
}

static DFBResult IDirectFBVideoProvider_bcm_GetPos(
                                                  IDirectFBVideoProvider *thiz,
                                                  double                 *seconds )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_GetLength(
                                                     IDirectFBVideoProvider *thiz,
                                                     double                 *seconds )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_GetColorAdjustment(
                                                              IDirectFBVideoProvider *thiz,
                                                              DFBColorAdjustment     *adj )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_SetColorAdjustment( IDirectFBVideoProvider   *thiz,
                                                                const DFBColorAdjustment *adj )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

static DFBResult IDirectFBVideoProvider_bcm_SendEvent( IDirectFBVideoProvider *thiz,
                                                       const DFBEvent         *evt )
{
     DIRECT_INTERFACE_GET_DATA (IDirectFBVideoProvider_bcm)

     return DFB_UNSUPPORTED;
}

/* exported symbols */

static DFBResult
Probe( IDirectFBVideoProvider_ProbeContext *ctx )
{
    /* Look at the header of the mpeg file, let's validate it's indeed mpeg es */

    //check that the underlying driver is BCM.  Otherwise, it wont work.
    //check for sequence header or picture header, both are legal for video drip.
    if( dfb_system_type() == CORE_BCM &&
        ctx->header[0] == 0x00 &&
        ctx->header[1] == 0x00 &&
        ctx->header[2] == 0x01 &&
        ( (ctx->header[3] == 0xB3) || (ctx->header[3] == 0x00) ) )
    {
        return DFB_OK;
    }

    return DFB_UNSUPPORTED;
}

static DFBResult
Construct( IDirectFBVideoProvider *thiz, IDirectFBDataBuffer *buffer )
{
    DFBResult ret = DFB_OK;

    DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBVideoProvider_bcm)

    data->ref = 1;
    data->buffer = buffer;

    /* Increase the data buffer reference counter. */
    buffer->AddRef( buffer );

    direct_util_recursive_pthread_mutex_init( &data->lock );

    thiz->AddRef    = IDirectFBVideoProvider_bcm_AddRef;
    thiz->Release   = IDirectFBVideoProvider_bcm_Release;
    thiz->GetCapabilities = IDirectFBVideoProvider_bcm_GetCapabilities;
    thiz->GetSurfaceDescription = IDirectFBVideoProvider_bcm_GetSurfaceDescription;
    thiz->PlayTo    = IDirectFBVideoProvider_bcm_PlayTo;
    thiz->Stop      = IDirectFBVideoProvider_bcm_Stop;
    thiz->GetStatus = IDirectFBVideoProvider_bcm_GetStatus;
    thiz->SeekTo    = IDirectFBVideoProvider_bcm_SeekTo;
    thiz->GetPos    = IDirectFBVideoProvider_bcm_GetPos;
    thiz->GetLength = IDirectFBVideoProvider_bcm_GetLength;
    thiz->GetColorAdjustment = IDirectFBVideoProvider_bcm_GetColorAdjustment;
    thiz->SetColorAdjustment = IDirectFBVideoProvider_bcm_SetColorAdjustment;
    thiz->SendEvent = IDirectFBVideoProvider_bcm_SendEvent;

    //dont know yet
    data->width=0;
    data->height=0;

    return DFB_OK;

error:

    buffer->Release( buffer );

    DIRECT_DEALLOCATE_INTERFACE(thiz);

    return ret;
}

#endif