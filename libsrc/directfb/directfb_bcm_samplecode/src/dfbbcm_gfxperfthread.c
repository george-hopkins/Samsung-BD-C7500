#include "bdvd.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "pthread.h"
#include "dfbbcm_utils.h"
#include "dfbbcm_gfxperfthread.h"

#define FB_WIDTH 960
#define FB_HEIGHT 1080

static char* argb_img_files[] = {
    DATADIR"/earth_256x256_argb.png",           //ig
    DATADIR"/babytux_128x256_argb.png",         //ig
    DATADIR"/nasa_shuttle0_128x256_argb.png",   //ig
    DATADIR"/brcm_256x512_argb.png",            //pg
    DATADIR"/pg_512x256_sm_argb.png"            //pg
};

static char* argb_rgb24_img_files[] = {
    DATADIR"/earth_256x256_rgb24.png",          //ig
    DATADIR"/babytux_128x256_argb.png",         //ig
    DATADIR"/nasa_shuttle0_128x256_rgb24.png",  //ig
    DATADIR"/brcm_256x512_rgb24.png",           //pg
    DATADIR"/pg_512x256_sm_argb.png"            //pg
};

#define MAX_IMGS 5

typedef struct 
{
    int x;
    int y;
    int xinc;
    int yinc;
} SPRITE_VEC;

struct GfxPerfThreadRoutineContext {
    bool                    running;
    IDirectFBDisplayLayer   *pg_layer;
    IDirectFBDisplayLayer   *ig_layer;
    IDirectFBSurface        *pg_surface;
    IDirectFBSurface        *ig_surface;
    DFBSurfaceDescription   dsc[MAX_IMGS];
    IDirectFBSurface        *img_surface[MAX_IMGS];
    SPRITE_VEC              vec[MAX_IMGS];
};

static IDirectFB * pDfb;
DFBResult err;

static DFBResult thread_context_open(GfxPerfThreadRoutineContext_t pThreadRoutineContext)
{
    DFBRectangle                  rect;
    bdvd_dfb_ext_layer_settings_t settings;
    DFBDisplayLayerConfig         config;
    DFBDisplayLayerID             pg_layer_id;
    DFBDisplayLayerID             ig_layer_id;
    int i;

    if (pThreadRoutineContext == NULL) {
        fprintf(stderr,"pThreadRoutineContext is NULL\n");
        return (DFB_FAILURE);
    }
    
    config.flags = DLCONF_WIDTH;
    config.width = FB_WIDTH;
    config.flags = (DFBDisplayLayerConfigFlags)((int)config.flags | (int)DLCONF_HEIGHT);
    config.height = FB_HEIGHT; 
    config.flags = (DFBDisplayLayerConfigFlags)((int)config.flags | (int)DLCONF_PIXELFORMAT);
    config.pixelformat = DSPF_ARGB;
    config.flags = (DFBDisplayLayerConfigFlags)((int)config.flags | (int)DLCONF_BUFFERMODE);
    config.buffermode = DLBM_FRONTONLY;

    //-------- Getting the IG layer surface --------
    pThreadRoutineContext->ig_layer = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_interactive_layer);
    if (pThreadRoutineContext->ig_layer == NULL) {
        fprintf(stderr,"failed\n");
        return (DFB_FAILURE);
	}
	if (bdvd_dfb_ext_layer_get(pThreadRoutineContext->ig_layer, &settings) != bdvd_ok) {
        fprintf(stderr,"failed\n");
        return (DFB_FAILURE);
	}

    ig_layer_id = (DFBDisplayLayerID)settings.id;
    DFBCHECK( pDfb->GetDisplayLayer(pDfb, ig_layer_id, &pThreadRoutineContext->ig_layer) );
    DFBCHECK( pThreadRoutineContext->ig_layer->SetCooperativeLevel(pThreadRoutineContext->ig_layer, DLSCL_ADMINISTRATIVE) );
    DFBCHECK( pThreadRoutineContext->ig_layer->SetBackgroundColor(pThreadRoutineContext->ig_layer, 0, 0x00, 0x00, 0x00 ) );
    DFBCHECK( pThreadRoutineContext->ig_layer->SetBackgroundMode(pThreadRoutineContext->ig_layer, DLBM_COLOR ) );
    DFBCHECK( pThreadRoutineContext->ig_layer->SetConfiguration(pThreadRoutineContext->ig_layer, &config) );
    DFBCHECK( pThreadRoutineContext->ig_layer->GetSurface(pThreadRoutineContext->ig_layer, &pThreadRoutineContext->ig_surface) );
    DFBCHECK( pThreadRoutineContext->ig_surface->SetColor(pThreadRoutineContext->ig_surface,
                                    0x00,     //r
                                    0x00,     //g
                                    0x00,     //b
                                    0x00) );  //a
    rect.w = rect.h = 0;
    DFBCHECK( pThreadRoutineContext->ig_surface->GetSize(pThreadRoutineContext->ig_surface, &rect.w, &rect.h) );
    DFBCHECK( pThreadRoutineContext->ig_surface->FillRectangle(pThreadRoutineContext->ig_surface, 0, 0, rect.w, rect.h) );
    DFBCHECK( pThreadRoutineContext->ig_surface->Flip( pThreadRoutineContext->ig_surface, NULL, (DFBSurfaceFlipFlags)0 ) );

    //-------- Getting the PG layer surface --------
    if ((pThreadRoutineContext->pg_layer = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_hdmv_presentation_layer)) == NULL) {
        fprintf(stderr,"failed\n");
        return (DFB_FAILURE);
	}

	if (bdvd_dfb_ext_layer_get(pThreadRoutineContext->pg_layer, &settings) != bdvd_ok) {
        fprintf(stderr,"failed\n");
        return (DFB_FAILURE);
	}

    pg_layer_id = (DFBDisplayLayerID)settings.id;
    DFBCHECK( pDfb->GetDisplayLayer(pDfb, pg_layer_id, &pThreadRoutineContext->pg_layer) );
    DFBCHECK( pThreadRoutineContext->pg_layer->SetCooperativeLevel(pThreadRoutineContext->pg_layer, DLSCL_ADMINISTRATIVE) );
    DFBCHECK( pThreadRoutineContext->pg_layer->SetBackgroundColor(pThreadRoutineContext->pg_layer, 0, 0x00, 0, 0x00 ) );
    DFBCHECK( pThreadRoutineContext->pg_layer->SetBackgroundMode(pThreadRoutineContext->pg_layer, DLBM_COLOR ) );
    DFBCHECK( pThreadRoutineContext->pg_layer->SetConfiguration(pThreadRoutineContext->pg_layer, &config) );
    DFBCHECK( pThreadRoutineContext->pg_layer->GetSurface(pThreadRoutineContext->pg_layer, &pThreadRoutineContext->pg_surface) );

    DFBCHECK( pThreadRoutineContext->pg_surface->SetColor(pThreadRoutineContext->pg_surface,
                                    0x00,     //r
                                    0x00,     //g
                                    0x00,     //b
                                    0x00) );  //a
    rect.w = rect.h = 0;
    DFBCHECK( pThreadRoutineContext->pg_surface->GetSize(pThreadRoutineContext->pg_surface, &rect.w, &rect.h) );
    DFBCHECK( pThreadRoutineContext->pg_surface->FillRectangle(pThreadRoutineContext->pg_surface, 0, 0, rect.w, rect.h) );
    DFBCHECK( pThreadRoutineContext->pg_surface->Flip( pThreadRoutineContext->pg_surface, NULL, (DFBSurfaceFlipFlags)0 ) );

    fprintf(stderr,"-------- Enumerating display layers --------\n");
    DFBCHECK( pDfb->EnumDisplayLayers( pDfb, enum_layers_callback, NULL ) );

    return (DFB_OK);
}

static DFBResult thread_context_close(GfxPerfThreadRoutineContext_t pThreadRoutineContext) {
    uint32_t i;
    
    for (i = 0; i < MAX_IMGS; i++) {
        pThreadRoutineContext->img_surface[i]->Release( pThreadRoutineContext->img_surface[i] );
    }
    
    pThreadRoutineContext->pg_surface->Release( pThreadRoutineContext->pg_surface );
    pThreadRoutineContext->ig_surface->Release( pThreadRoutineContext->ig_surface );
    pThreadRoutineContext->pg_layer->Release( pThreadRoutineContext->pg_layer );
    pThreadRoutineContext->ig_layer->Release( pThreadRoutineContext->ig_layer );

    return (DFB_OK);
}

/* Read bitmaps */

static DFBResult thread_context_load_images(GfxPerfThreadRoutineContext_t pThreadRoutineContext)
{
    IDirectFBImageProvider  *provider;
    int offsetx, offsety, i;

    if (pThreadRoutineContext == NULL) {
        fprintf(stderr,"pThreadRoutineContext is NULL\n");
        return (DFB_FAILURE);
    }

    offsetx = 0;
    offsety = 0;
    for (i = 0; i < MAX_IMGS; i++)
    {
        DFBCHECK(pDfb->CreateImageProvider( pDfb, argb_img_files[i], &provider ));
        DFBCHECK(provider->GetSurfaceDescription (provider, &pThreadRoutineContext->dsc[i]));
        
        fprintf(stderr, "Image %s PixelFormat %s\n", argb_img_files[i], convert_dspf2string(pThreadRoutineContext->dsc[i].pixelformat));
        
        DFBCHECK(pDfb->CreateSurface( pDfb, &pThreadRoutineContext->dsc[i], &pThreadRoutineContext->img_surface[i] ));
        DFBCHECK(provider->RenderTo( provider, pThreadRoutineContext->img_surface[i], NULL ));
        provider->Release( provider );
        pThreadRoutineContext->vec[i].x = offsetx;
        pThreadRoutineContext->vec[i].y = offsety;
        pThreadRoutineContext->vec[i].xinc = 4;
        pThreadRoutineContext->vec[1].yinc = 4;
        offsetx +=80;
        offsety +=60;
    }
}

static void update_pos(SPRITE_VEC *vec, DFBSurfaceDescription *desc, int w, int h)
{
    int nextx, nexty;

    /* try updating X */
    nextx = vec->x + vec->xinc;
    nexty = vec->y + vec->yinc;

    if (nextx < 0)
    {
        vec->xinc = -vec->xinc;
        return;
    }

    if (nextx + desc->width > w)
    {
        vec->xinc = -vec->xinc;
        return;
    }

    if (nexty < 0)
    {
        vec->yinc = -vec->yinc;
        return;
    }

    if (nexty + desc->height > h)
    {
        vec->yinc = -vec->yinc;
        return;
    }

    vec->x = nextx;
    vec->y = nexty;
}

void move_sprites(GfxPerfThreadRoutineContext_t pThreadRoutineContext)
{
    int i;
    DFBRectangle            rect;

    DFBCHECK( pThreadRoutineContext->ig_surface->SetBlittingFlags(
                    pThreadRoutineContext->ig_surface, 
                    DSBLIT_SRC_PREMULTIPLY ) );

    DFBCHECK( pThreadRoutineContext->pg_surface->SetBlittingFlags(
                    pThreadRoutineContext->pg_surface, 
                    DSBLIT_SRC_PREMULTIPLY ) );

    DFBCHECK( pThreadRoutineContext->pg_surface->SetColor(pThreadRoutineContext->pg_surface,0x00,0x00,0x00,0x00) );
    DFBCHECK( pThreadRoutineContext->ig_surface->SetColor(pThreadRoutineContext->ig_surface,0x00,0x00,0x00,0x00) );

    for (i = 0; i < MAX_IMGS; i++)
    {
        rect.x = 0;
        rect.y = 0;
        rect.w = pThreadRoutineContext->dsc[i].width;
        rect.h = pThreadRoutineContext->dsc[i].height;

        // clear previous
        DFBCHECK( pThreadRoutineContext->pg_surface->FillRectangle(
                   i < 3 ? pThreadRoutineContext->ig_surface : pThreadRoutineContext->pg_surface,
                   pThreadRoutineContext->vec[i].x, pThreadRoutineContext->vec[i].y,
                   rect.w, rect.h) );

        // update position
        update_pos(&pThreadRoutineContext->vec[i], &pThreadRoutineContext->dsc[i], FB_WIDTH, FB_HEIGHT);

        // blit new sprite
        DFBCHECK( pThreadRoutineContext->pg_surface->Blit(
                    i < 3 ? pThreadRoutineContext->ig_surface : pThreadRoutineContext->pg_surface,
                    pThreadRoutineContext->img_surface[i],
                    &rect,//whole rect
                    pThreadRoutineContext->vec[i].x, pThreadRoutineContext->vec[i].y) );
    }
}

static void * thread_routine(void * arg)
{
    GfxPerfThreadRoutineContext_t pThreadRoutineContext = (GfxPerfThreadRoutineContext_t)arg;

    struct timeval  tv_start, tv_end;
    double t0, t1;
    uint32_t i;

    gettimeofday(&tv_start, NULL);
    for (i = 0; pThreadRoutineContext->running ; i++)
    {
        move_sprites(pThreadRoutineContext);
        DFBCHECK( pThreadRoutineContext->pg_surface->Flip( pThreadRoutineContext->pg_surface, NULL, (DFBSurfaceFlipFlags)DSFLIP_PIPELINE ) );
        DFBCHECK( pThreadRoutineContext->ig_surface->Flip( pThreadRoutineContext->ig_surface, NULL, (DFBSurfaceFlipFlags)0 ) );

        /* Display frame rate every 300 frames ~ 10 seconds */
        if ((i%300) == 0) {
            gettimeofday(&tv_end, NULL);

            t1 = (tv_end.tv_sec * 1000000) + tv_end.tv_usec;
            t0 = (tv_start.tv_sec * 1000000) + tv_start.tv_usec;

            fprintf(stderr, "frames per second %f\n",
                (float)(i * 1000000) / (t1 - t0));
    
            i = 0;
            gettimeofday(&tv_start, NULL);
        }
    }

    return NULL;
}

DFBResult GfxPerfThreadCreate(IDirectFB * dfb, GfxPerfThread_t * pGfxPerfThread)
{
    DFBResult ret;
    
    pDfb = dfb;

    if ((pGfxPerfThread->threadRoutineContext = (GfxPerfThreadRoutineContext_t)calloc(1, sizeof(*pGfxPerfThread->threadRoutineContext))) == NULL) {
        return ret;
    }

    if ((ret = thread_context_open(pGfxPerfThread->threadRoutineContext)) != DFB_OK) {
        return ret;
    }

    if ((ret = thread_context_load_images(pGfxPerfThread->threadRoutineContext)) != DFB_OK) {
        return ret;
    }

    return (DFB_OK);
}

DFBResult GfxPerfThreadStart(GfxPerfThread_t * pGfxPerfThread)
{
    int rc;

    DFBCHECK( pGfxPerfThread->threadRoutineContext->ig_layer->SetLevel(pGfxPerfThread->threadRoutineContext->ig_layer, 11) );
    DFBCHECK( pGfxPerfThread->threadRoutineContext->pg_layer->SetLevel(pGfxPerfThread->threadRoutineContext->pg_layer, 10) );

    pGfxPerfThread->threadRoutineContext->running = true;

    if ((rc = pthread_create( &pGfxPerfThread->thread, NULL, thread_routine, pGfxPerfThread->threadRoutineContext)) != 0) {
        fprintf(stderr, "Error creating perf thread %s\n", strerror(rc) );
        return (DFB_FAILURE);
    }

    return (DFB_OK);
}

DFBResult GfxPerfThreadStop(GfxPerfThread_t * pGfxPerfThread)
{
    int rc;
    
    pGfxPerfThread->threadRoutineContext->running = false;

    if ((rc = pthread_join( pGfxPerfThread->thread, NULL)) != 0) {
        fprintf(stderr, "Error joining perf thread %s\n", strerror(rc) );
        return (DFB_FAILURE);
    }
    
    DFBCHECK( pGfxPerfThread->threadRoutineContext->ig_surface->Clear(pGfxPerfThread->threadRoutineContext->ig_surface, 0, 0, 0, 0) );
    DFBCHECK( pGfxPerfThread->threadRoutineContext->ig_surface->Flip(pGfxPerfThread->threadRoutineContext->ig_surface, NULL, (DFBSurfaceFlipFlags)0) );
    DFBCHECK( pGfxPerfThread->threadRoutineContext->pg_surface->Clear(pGfxPerfThread->threadRoutineContext->pg_surface, 0, 0, 0, 0) );
    DFBCHECK( pGfxPerfThread->threadRoutineContext->pg_surface->Flip(pGfxPerfThread->threadRoutineContext->pg_surface, NULL, (DFBSurfaceFlipFlags)0) );

    DFBCHECK( pGfxPerfThread->threadRoutineContext->ig_layer->SetLevel(pGfxPerfThread->threadRoutineContext->ig_layer, -1) );
    DFBCHECK( pGfxPerfThread->threadRoutineContext->pg_layer->SetLevel(pGfxPerfThread->threadRoutineContext->pg_layer, -1) );

    return (DFB_OK);
}

DFBResult GfxPerfThreadDelete(GfxPerfThread_t * pGfxPerfThread)
{
    DFBResult ret;
    
    if ((ret = thread_context_close(pGfxPerfThread->threadRoutineContext)) != DFB_OK) {
        return ret;
    }

    free(pGfxPerfThread->threadRoutineContext);

    return (DFB_OK);
}
