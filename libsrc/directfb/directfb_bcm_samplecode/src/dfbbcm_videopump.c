
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <directfb.h>
#include <pthread.h>
#include <unistd.h>
#include <bdvd.h>

#include "dfbbcm_utils.h"

#define SIZEOF_ARRAY(a) (sizeof(a)/sizeof(a[0]))

#define PRINT(x...) { fprintf(stderr,x); }


typedef struct
{
    IDirectFBDataBuffer* buffer;
    FILE *fd;
    void* data;
    int data_size;
} cbctx_t;

typedef struct
{
    IDirectFBSurface* surface;
}dvcbctx_t;


unsigned int image_counter = 0;

static int dvcb(void*_ctx)
{
    dvcbctx_t* ctx=(dvcbctx_t*)_ctx;

    printf("decoded image %d\n", image_counter);

    image_counter++;

    ctx->surface->Flip(ctx->surface,NULL,DSFLIP_NONE);
    return 0;
}

static bool isVideoPumpThreadSpawned = false;
static bool isVideoProviderCreated = false;
static unsigned int repeat = 0;

void* pump_threadproc(void*ctx)
{
    DFBResult result=DFB_OK;
    cbctx_t* my_ctx=(cbctx_t*)ctx;
    int bytes = 0;;
    unsigned int *pheader = (unsigned int *)my_ctx->data;

    printf("pump_threadproc started\n");

    for(;;)
    {
        /*printf("read called %p %p %d\n", my_ctx, my_ctx->data, my_ctx->data_size);*/
        bytes = fread(my_ctx->data, sizeof(char), my_ctx->data_size, my_ctx->fd);
        /*printf("read done\n");*/

        /*printf("Read %d Header is 0x%x\n", bytes, *pheader);*/

        if(bytes==0) break;

        /*if (repeat == 2)*/
        {
        /*printf("put data called %p %p %p %d\n", my_ctx, my_ctx->buffer, my_ctx->data, bytes);*/
        result=my_ctx->buffer->PutData(my_ctx->buffer,my_ctx->data,bytes);
        if(result!=DFB_OK) break;
        /*printf("put data done\n");*/
        }
    }

    printf("pump_threadproc done\n");

    return NULL;
}


static DFBResult videopump(IDirectFB* dfb, IDirectFBDisplayLayer* layer, char* filename)
{
    DFBResult result=DFB_OK;
    IDirectFBSurface* surface=NULL;
    IDirectFBDataBuffer* buffer=NULL;
    IDirectFBVideoProvider* provider=NULL;
    DFBSurfaceDescription sdesc;
    void* data=NULL;
    int data_size=2*1024*1024;
    FILE *fd;
    cbctx_t ctx;
    dvcbctx_t dvctx;
    pthread_t pump_thread;

    PRINT("videopump test ENTER!\n");
    isVideoPumpThreadSpawned = true;

    fd=fopen(filename,"rb");

    data=malloc(data_size);
    if(data==NULL) result=DFB_NOSYSTEMMEMORY;
    *((unsigned int *)data) = 0xDEADBEEF;

    printf("Header is 0x%x\n", *((unsigned int *)data));

    if(result==DFB_OK)
        result=layer->GetSurface(layer,&surface);

    if(result==DFB_OK)
    {
        PRINT("creating streaming data buffer...\n");

        result=dfb->CreateDataBuffer(dfb,NULL,&buffer);

    }

    ctx.buffer=buffer;
    ctx.fd=fd;
    ctx.data=data;
    ctx.data_size=data_size;

    printf("pthread_create called\n");
    pthread_create(&pump_thread,NULL,pump_threadproc,((void *)&ctx));
    printf("pthread_create done\n");


    if(result==DFB_OK)
    {
        PRINT("creating video provider...\n");
        result=buffer->CreateVideoProvider(buffer,&provider);
        if (result)
            PRINT("CreateVideoProvider failed\n");

    }

    if(result==DFB_OK)
    {
        PRINT("getting image description...\n");
        result=provider->GetSurfaceDescription(provider,&sdesc);
        if (result)
            PRINT("GetSurfaceDescription failed\n");
    }

    printf("creating video provider done\n");

    printf("pthread_join called\n");

    //wait until the whole file is pushed in
    pthread_join(pump_thread,NULL);

    printf("pthread_join done\n");

    {
        unsigned int playTime = 0;
        bool playing = true;

        if(result==DFB_OK)
        {
            dvctx.surface=surface;

            PRINT("Calling PlayTo\n");
            result=provider->PlayTo(provider,surface,NULL,dvcb,&dvctx);
        }

        PRINT("whole file pushed in, now waiting for movie to complete...\n");

        //wait for movie to complete
        while(playing)
        {
            DFBVideoProviderStatus status;
            result=provider->GetStatus(provider,&status);
            if(result==DFB_UNSUPPORTED)
            {
                /* getting the decoder status is not supported with the current version of the video provider. */
                PRINT("GetStatus() is not supported, no way to tell when the movie is complete.\n");

                PRINT("Waiting 10 seconds and stopping.\n");
                mymsleep(10000);

                result=provider->Stop(provider);
                playing = false;
                break;
            }

            switch (status) {
                case DVSTATE_PLAY:
                    mymsleep(1000);
                    playTime++;
                    if (playTime == 10)
                    {
                        result=provider->Stop(provider);
                        playing = false;
                        image_counter = 0;
                    }
                break;
                case DVSTATE_FINISHED:
                    PRINT("DVSTATE_FINISHED -> movie playback complete!\n");
                    playing = false;
                break;
                case DVSTATE_STOP:
                    PRINT("DVSTATE_STOP -> movie playback stopped!\n");
                    playing = false;
                break;
                default:
                    PRINT("Invalid status -> stopping movie playback!\n");
                    result=provider->Stop(provider);
                break;
            }
        }

        if(provider)
            provider->Release(provider);

        if(buffer)
            buffer->Flush(buffer);

        if(buffer)
            buffer->Release(buffer);
    }

    if(surface)
        surface->Release(surface);

    if(data)
        free(data);

    if(fd)
        fclose(fd);

    PRINT("videopump test DONE!\n");

    return result;
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;
    DFBResult result=DFB_OK;
    IDirectFB* dfb=NULL;
    bdvd_dfb_ext_h hLayer=NULL;
    /* bdvd_init begin */
    bdvd_display_h bdisplay = NULL;
    bdvd_result_e retCode;
    bdvd_display_settings_t display_settings;
    bdvd_decode_window_h window, window0; /* use to display background images */
    bdvd_decode_window_settings_t decodeWindowSettings;
    DFBDisplayLayerConfig background_layer_config;

    if (bdvd_init(BDVD_VERSION) != b_ok) {
        fprintf(stderr, "Could not init bdvd\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
        fprintf(stderr, "bdvd_display_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* bdvd_init end */

    if(argc!=2)
    {
        fprintf(stderr, "usage: videopump FILENAME\n\n");
        return 1;
    }

#if 1
    bdvd_display_get(bdisplay, &display_settings);

    display_settings.format = bdvd_video_format_1080i;
    display_settings.svideo = NULL;
    display_settings.composite = NULL;
    display_settings.rf = NULL;


    if (bdvd_display_set(bdisplay, &display_settings) != bdvd_ok) {
        printf("bdvd_display_set failed\n");
        return bdvd_err_unspecified;
    }
#else
    display_settings.format = bdvd_video_format_ntsc;
#endif

#if 1
    window0 = bdvd_decode_window_open(B_ID(0), bdisplay);
        if (window0 == NULL) {
            printf("bdvd_decode_window_open failed\n");
            return bdvd_err_unspecified;
    }

  if (bdvd_decode_window_get(window0, &decodeWindowSettings) != bdvd_ok) {
    printf("bdvd_decode_window_get failed\n");
    return bdvd_err_unspecified;
  }

 /* make secondary B_ID(1) visible */

  decodeWindowSettings.zorder = 1;
  decodeWindowSettings.alpha_value = 0x00;


    if (bdvd_decode_window_set(window0, &decodeWindowSettings) != bdvd_ok) {
        printf("bdvd_decode_window_get failed\n");
        return bdvd_err_unspecified;
    }
    printf("open first display: z = %d\n", decodeWindowSettings.zorder);
#endif

    window = bdvd_decode_window_open(B_ID(1), bdisplay);
    if (window == NULL) {
        printf("bdvd_decode_window_open failed\n");
        return bdvd_err_unspecified;
    }


   if (bdvd_decode_window_get(window, &decodeWindowSettings) != bdvd_ok) {
     printf("bdvd_decode_window_get failed\n");
      return bdvd_err_unspecified;
   }

  decodeWindowSettings.zorder = 0;


    if (bdvd_decode_window_set(window, &decodeWindowSettings) != bdvd_ok) {
        printf("bdvd_decode_window_get failed\n");
        return bdvd_err_unspecified;
    }

    D_INFO("Press any key to setup background layer\n");
    getchar();

    //--- initialize DirectFB and display layer, then display the OSD ---
    if(result==DFB_OK)
    {
        IDirectFBDisplayLayer* layer=NULL;
        bdvd_dfb_ext_layer_settings_t layer_settings;
        dfb=bdvd_dfb_ext_open_with_params(6, &argc, &argv);
        if(dfb==NULL)
        {
            fprintf(stderr, "DirectFB initialization error!!\n");
            result=DFB_FAILURE;
        }

        if(result==DFB_OK)
        {
            hLayer=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_j_background_layer);
            result=bdvd_dfb_ext_layer_get(hLayer,&layer_settings);
        }

        if(result==DFB_OK)
            result=dfb->GetDisplayLayer(dfb,layer_settings.id,&layer);

        if(result==DFB_OK)
            result=layer->SetCooperativeLevel(layer,DLSCL_ADMINISTRATIVE);

        /* End of background layer setup */
        if (display_settings.format == bdvd_video_format_1080i)
        {
            /* Default pixel format is DSPF_UYVY */
            background_layer_config.flags = DLCONF_WIDTH | DLCONF_HEIGHT;
            background_layer_config.width = 720;
            background_layer_config.height = 480;

            D_INFO("SetConfiguration\n");
            if (layer->SetConfiguration(layer, &background_layer_config) != DFB_OK) {
                D_ERROR("SetConfiguration failed\n");
                ret = EXIT_FAILURE;
                goto out;
            }
        }

        layer->SetLevel(layer, 1);

        if(result==DFB_OK)
        {
#if 0
            for (repeat = 0; repeat < 1; repeat++)
            {
                printf("\n\nStarting Test %d\n\n", repeat);

                bdvd_dfb_ext_acquire_display_window();
                bdvd_dfb_ext_release_display_window();
            }
#endif
            for (repeat = 0; repeat < 1000; repeat++)
            {
                printf("\n\nStarting Test %d\n\n", repeat);

                retCode = bdvd_decode_set_still_display_mode(window, true);
                if (retCode)
                {
                    printf("bdvd_decode_set_still_display_mode returned with errors\n");
                }

                isVideoPumpThreadSpawned = false;
                result=videopump(dfb,layer,argv[1]);
                if (result)
                {
                    printf("\n\nTest %d failed\n\n", repeat);
                }

                while (isVideoPumpThreadSpawned == false)
                {
                    mymsleep(100);
                }

                retCode = bdvd_decode_set_still_display_mode(window, false);
                if (retCode)
                {
                    printf("bdvd_decode_set_still_display_mode returned with errors\n");
                }
            }
        }

        if(layer)
            layer->Release(layer);
    }

    //===== CLEANUP BDVD =====
    /*
    if(dfb)
        dfb->Release(dfb);
    */
    if(hLayer)
        bdvd_dfb_ext_layer_close(hLayer);

    bdvd_dfb_ext_close();

out:

    /* bdvd_uninit begin */
    if (bdisplay) bdvd_display_close(bdisplay);
    //bdvd_uninit();

    return 0;
}
