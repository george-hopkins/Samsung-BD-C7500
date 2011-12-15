#include <directfb.h>

#include <direct/util.h>
#include <direct/debug.h>

#include <bdvd.h>

#include <stdlib.h>

#include "dfbbcm_utils.h"

#if 0
                              /* pixel = (source * fs + destination * fd),
                                 sa = source alpha,
                                 da = destination alpha */
     DSPD_NONE           = 0, /* fs: sa      fd: 1.0-sa (defaults) */
     DSPD_CLEAR          = 1, /* fs: 0.0     fd: 0.0    */
     DSPD_SRC            = 2, /* fs: 1.0     fd: 0.0    */
     DSPD_SRC_OVER       = 3, /* fs: 1.0     fd: 1.0-sa */
     DSPD_DST_OVER       = 4, /* fs: 1.0-da  fd: 1.0    */
     DSPD_SRC_IN         = 5, /* fs: da      fd: 0.0    */
     DSPD_DST_IN         = 6, /* fs: 0.0     fd: sa     */
     DSPD_SRC_OUT        = 7, /* fs: 1.0-da  fd: 0.0    */
     DSPD_DST_OUT        = 8  /* fs: 0.0     fd: 1.0-sa */
#endif

     
int main( int argc, char *argv[] )
{
#if 0    
     int ret;
     DFBResult err;
     IDirectFB               *dfb;
     IDirectFBDisplayLayer   *layer;
     IDirectFBSurface        *primary;
     IDirectFBSurface        *background;
     DFBRectangle back_rect = {0,0,0,0};
     DFBSurfaceDescription   dsc;
     IDirectFBSurface        *yellow;
     DFBRectangle yellow_rect = {0,0,0,0};
     IDirectFBSurface        *blue;
     DFBRectangle blue_rect = {0,0,0,0};
     IDirectFBImageProvider  *provider;
     int xoff = 0;
     int yoff = 0;
     unsigned long param = 0;
     DFBSurfacePorterDuffRule pdrule = 0;

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

     DFBCHECK( DirectFBInit(NULL, NULL) );
     DFBCHECK( DirectFBCreate(&dfb) );
     DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer) );
     DFBCHECK( layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE) );
 
     DFBCHECK( layer->SetBackgroundColor(layer, 0, 0, 0, 0 ) );
     DFBCHECK( layer->SetBackgroundMode(layer, DLBM_COLOR ) );
 
     DFBCHECK( layer->GetSurface(layer, &primary) );

     if (argc < 1) {
        print_usage(argv[0]);
        return -1;
     }

     while ((c = getopt (argc, argv, "l:n:psa:b:c:d:e:f:g:h:r")) != -1) {
        switch (c) {
        case 'l':
            /* No need to check the value to the system maximum, this is done at a lower level,
             * plus we want to test the error handling. We do have a max in this test,
             * but it's far more than the system's.
             */
            if (strtoul(optarg, NULL, 10) > TEST_DISPLAYLAYER_IDS_MAX) {
                D_ERROR("maximum generic layer reached\n");
                exit(EXIT_FAILURE);
            }
            else {
                genericlayers = optarg;
            }
        break;
        case 'n':
            /* Same as l
             */
            if ((namedlayers = strtoul(optarg, NULL, 10)) > TEST_DISPLAYLAYER_IDS_MAX) {
                D_ERROR("maximum generic layer reached\n");
                exit(EXIT_FAILURE);
            }
        break;
        case 'p':
            get_primary_layer = true;
        break;
        case 's':
            get_primary_layer_surface = true;
        break;
        }
     }

     switch (param) {
        case 0:
             pdrule = DSPD_NONE;  /* fs: sa      fd: 1.0-sa (defaults) */
        break;
        case 1:
             pdrule = DSPD_CLEAR; /* fs: 0.0     fd: 0.0    */
        break;
        case 2:
             pdrule = DSPD_SRC;   /* fs: 1.0     fd: 0.0    */
        break;
        case 3:
             pdrule = DSPD_SRC_OVER; /* fs: 1.0     fd: 1.0-sa */
        break;
        case 4:
             pdrule = DSPD_DST_OVER; /* fs: 1.0-da  fd: 1.0    */
        break;
        case 5:
             pdrule = DSPD_SRC_IN; /* fs: da      fd: 0.0    */
        break;
        case 6:
             pdrule = DSPD_DST_IN; /* fs: 0.0     fd: sa     */
        break;
        case 7:
             pdrule = DSPD_SRC_OUT; /* fs: 1.0-da  fd: 0.0    */
        break;
        case 8:
             pdrule = DSPD_DST_OUT; /* fs: 0.0     fd: 1.0-sa */
        break;
     }

     if (param < 9) {
         fprintf(stderr, "---------------PorterDuff %d-------------------\n", pdrule);
         DFBCHECK( primary->SetPorterDuff( primary, pdrule ) );
     }
     else {
         fprintf(stderr, "---------------Other rules %d-------------------\n", param);
        /* other rules */
        switch (param) {
            case 9:
                fprintf(stderr, "---------------a atop b-------------------\n");
                DFBCHECK( primary->SetSrcBlendFunction( primary, DSBF_DESTALPHA ) );
                DFBCHECK( primary->SetDstBlendFunction( primary, DSBF_INVSRCALPHA ) );
            break;
            case 10:
                fprintf(stderr, "---------------b atop a-------------------\n");
                DFBCHECK( primary->SetSrcBlendFunction( primary, DSBF_INVDESTALPHA ) );
                DFBCHECK( primary->SetDstBlendFunction( primary, DSBF_SRCALPHA ) );
            break;
            case 11:
                fprintf(stderr, "---------------A xor B-------------------\n");
                DFBCHECK( primary->SetSrcBlendFunction( primary, DSBF_INVDESTALPHA ) );
                DFBCHECK( primary->SetDstBlendFunction( primary, DSBF_INVSRCALPHA ) );
            break;
            case 12:
                fprintf(stderr, "---------------DSBF_SRCALPHA DSBF_DESTALPHA-------------------\n");
                DFBCHECK( primary->SetSrcBlendFunction( primary, DSBF_SRCALPHA ) );
                DFBCHECK( primary->SetDstBlendFunction( primary, DSBF_DESTALPHA ) );
            break;
            case 13:
                fprintf(stderr, "---------------DSBF_SRCALPHA DSBF_DESTCOLOR-------------------\n");
                DFBCHECK( primary->SetSrcBlendFunction( primary, DSBF_SRCALPHA ) );
                DFBCHECK( primary->SetDstBlendFunction( primary, DSBF_DESTCOLOR ) );
                DFBCHECK( primary->SetColor( primary, 0, 0, 0, 0xFF ) );
            break;


#if 0            
     DSBF_ZERO               = 1,  /* */
     DSBF_ONE                = 2,  /* */
     DSBF_SRCCOLOR           = 3,  /* */
     DSBF_INVSRCCOLOR        = 4,  /* */
     DSBF_SRCALPHA           = 5,  /* */
     DSBF_INVSRCALPHA        = 6,  /* */
     DSBF_DESTALPHA          = 7,  /* */
     DSBF_INVDESTALPHA       = 8,  /* */
     DSBF_DESTCOLOR          = 9,  /* */
     DSBF_INVDESTCOLOR       = 10, /* */
     DSBF_SRCALPHASAT        = 11  /* */
#endif            
    } }
     
     /* load back */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/images/background.jpg",
                                        &provider ));

     DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));

     back_rect.w = dsc.width;
     back_rect.h = dsc.height;
     fprintf(stderr, "back_rect, x[%d], y[%d], w[%d], h[%d]", back_rect.x, back_rect.y, back_rect.w, back_rect.h);

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &background ));

     DFBCHECK(provider->RenderTo( provider, background, NULL ));
     provider->Release( provider );

     /* load the yellow and blue images */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/images/yellow.png",
                                        &provider ));

     DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));

     yellow_rect.w = dsc.width;
     yellow_rect.h = dsc.height;
     fprintf(stderr, "yellow_rect, x[%d], y[%d], w[%d], h[%d]", yellow_rect.x, yellow_rect.y, yellow_rect.w, yellow_rect.h);

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &yellow ));

     DFBCHECK(provider->RenderTo( provider, yellow, NULL ));
     provider->Release( provider );

     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/images/blue.png",
                                        &provider ));

     DFBCHECK(provider->GetSurfaceDescription (provider, &dsc));

     blue_rect.w = dsc.width;
     blue_rect.h = dsc.height;
     fprintf(stderr, "blue_rect, x[%d], y[%d], w[%d], h[%d]", blue_rect.x, blue_rect.y, blue_rect.w, blue_rect.h);

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &blue ));

     DFBCHECK(provider->RenderTo( provider, blue, NULL ));
     provider->Release( provider );

     xoff = 0;
     yoff = 0;

     param = 0;

     if (argc == 3) {
         param = strtoul(argv[2], NULL, 10);
     }

     switch (param) {
        case 0:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0,
                    0,
                    0) );
        break;
        case 1:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0,
                    0,
                    0xFF) );
        break;
        case 2:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0xFF,
                    0,
                    0) );
        break;
        case 3:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0xFF,
                    0,
                    0xFF) );
        break;
        case 9:
        /* back */
        DFBCHECK( primary->SetBlittingFlags(
                        primary, 
                        DSBLIT_NOFX ) );

        DFBCHECK( primary->Blit(
                    primary,
                    background,
                    &back_rect,//whole rect
                    0,0) );
        break;
     }
        
     
     /*| DSBLIT_SRC_PREMULTIPLY | DSBLIT_DST_PREMULTIPLY | DSBLIT_DEMULTIPLY*/
     
     /* Yellow bottom */
          
     DFBCHECK( primary->SetBlittingFlags(
                        primary, 
                        DSBLIT_BLEND_ALPHACHANNEL ) );

     DFBCHECK( primary->Blit(
                    primary,
                    yellow,
                    &yellow_rect,//whole rect
                    xoff,yoff) );

     //blend according to alpha channel
     DFBCHECK( primary->SetBlittingFlags(
                        primary, 
                        DSBLIT_BLEND_ALPHACHANNEL ) );

     DFBCHECK( primary->Blit(
                    primary,
                    blue,
                    &blue_rect,//whole rect
                    xoff,yoff) );

     DFBCHECK( primary->Flip( primary, NULL, 0 ));

     getchar();

     xoff = 0;
     yoff = 0;

     /* back */
     switch (param) {
        case 0:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0,
                    0,
                    0) );
        break;
        case 1:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0,
                    0,
                    0xFF) );
        break;
        case 2:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0xFF,
                    0,
                    0) );
        break;
        case 3:
          DFBCHECK( primary->Clear(
                    primary,
                    0,
                    0xFF,
                    0,
                    0xFF) );
        break;
        case 9:
        /* back */
        DFBCHECK( primary->SetBlittingFlags(
                        primary, 
                        DSBLIT_NOFX ) );

        DFBCHECK( primary->Blit(
                    primary,
                    background,
                    &back_rect,//whole rect
                    0,0) );
        break;
     }

     /* Blue bottom */

     DFBCHECK( primary->SetBlittingFlags(
                        primary, 
                        DSBLIT_BLEND_ALPHACHANNEL ) );

     DFBCHECK( primary->Blit(
                    primary,
                    blue,
                    &blue_rect,//whole rect
                    xoff,yoff) );
                  
     //blend according to alpha channel
     DFBCHECK( primary->SetBlittingFlags(
                        primary, 
                        DSBLIT_BLEND_ALPHACHANNEL ) );

     DFBCHECK( primary->Blit(
                    primary,
                    yellow,
                    &yellow_rect,//whole rect
                    xoff,yoff) );

     DFBCHECK( primary->Flip( primary, NULL, 0 ));

     getchar();

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

     return ret;
#endif     
}
     
