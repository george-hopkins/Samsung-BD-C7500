/*
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <directfb.h>

#include <core/surfaces.h>
#include <core/palette.h>
#include <core/surfacemanager.h>

#include <display/idirectfbpalette.h>
#include <display/idirectfbsurface.h>

#include <direct/interface.h>

#include "idirectfbsurfacemanager.h"

/*
 * private data struct of IDirectFBSurfaceManager
 */
typedef struct {
     int                              ref;     /* reference counter */

     CoreDFB                         *core;
     SurfaceManager                  *manager;
     CoreSurface                     *surface;

     int                              id;
     DFBSurfaceManagerTypes           type;
} IDirectFBSurfaceManager_data;

/******************************************************************************/

static void
IDirectFBSurfaceManager_Destruct( IDirectFBSurfaceManager *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurfaceManager)

     dfb_surfacemanager_destroy( data->manager );

     dfb_surface_destroy_buffer( data->surface,
                                 data->surface->front_buffer );
     direct_serial_deinit( &data->surface->serial );
     fusion_object_destroy( &data->surface->object );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult
IDirectFBSurfaceManager_AddRef( IDirectFBSurfaceManager *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurfaceManager)

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBSurfaceManager_Release( IDirectFBSurfaceManager *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurfaceManager)

     if (--data->ref == 0)
          IDirectFBSurfaceManager_Destruct( thiz );

     return DFB_OK;
}

static DFBResult
IDirectFBSurfaceManager_CreateSurface( IDirectFBSurfaceManager      *thiz,
                                       const DFBSurfaceDescription  *desc,
                                       IDirectFBSurface            **interface )
{

     DFBResult ret;
     int width = 256;
     int height = 256;
     int policy = CSP_VIDEOONLY;
     DFBSurfacePixelFormat format = DSPF_ARGB;
     DFBSurfaceCapabilities caps = 0;
     CoreSurface *surface = NULL;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurfaceManager)

     if (!desc || !interface)
          return DFB_INVARG;

     if (desc->flags & DSDESC_WIDTH) {
          width = desc->width;
          if (width < 1)
               return DFB_INVARG;
     }
     if (desc->flags & DSDESC_HEIGHT) {
          height = desc->height;
          if (height < 1)
               return DFB_INVARG;
     }

     if (desc->flags & DSDESC_PALETTE &&
         desc->flags & DSDESC_DFBPALETTEHANDLE)
         return DFB_INVARG;

     if (desc->flags & DSDESC_PALETTE)
          if (!desc->palette.entries || !desc->palette.size)
               return DFB_INVARG;

     if (desc->flags & DSDESC_DFBPALETTEHANDLE)
          if (!desc->dfbpalette)
               return DFB_INVARG;

     if (desc->flags & DSDESC_CAPS)
          caps = desc->caps;

     if (desc->flags & DSDESC_PIXELFORMAT)
          format = desc->pixelformat;

     switch (format) {
          case DSPF_A1:
          case DSPF_A8:
          case DSPF_ARGB:
          case DSPF_ARGB1555:
          case DSPF_ARGB2554:
          case DSPF_ARGB4444:
          case DSPF_AiRGB:
          case DSPF_I420:
          case DSPF_LUT8:
          case DSPF_ALUT44:
          case DSPF_RGB16:
          case DSPF_RGB24:
          case DSPF_RGB32:
          case DSPF_RGB332:
          case DSPF_UYVY:
          case DSPF_YUY2:
          case DSPF_YV12:
          case DSPF_NV12:
          case DSPF_NV21:
          case DSPF_NV16:
          case DSPF_AYUV:
          case DSPF_ALUT88:
          case DSPF_ABGR:
          case DSPF_LUT4:
          case DSPF_LUT2:
          case DSPF_LUT1:
               break;

          default:
               return DFB_INVARG;
     }

     /* Source compatibility with older programs */
     if ((caps & DSCAPS_FLIPPING) == DSCAPS_FLIPPING)
          caps &= ~DSCAPS_TRIPLE;

     if (caps & DSCAPS_TRIPLE)
          return DFB_UNSUPPORTED;

     if (desc->flags & DSDESC_PREALLOCATED) {
          return DFB_UNSUPPORTED;
     }

     if (((caps & DSCAPS_VIDEOONLY) != DSCAPS_VIDEOONLY) || caps & DSCAPS_SYSTEMONLY) {
          return DFB_UNSUPPORTED;
     }

     {
          CorePalette *palette = NULL;

          if (desc->flags & DSDESC_DFBPALETTEHANDLE) {
               IDirectFBPalette_data *palette_data;

               if (!DFB_PIXELFORMAT_IS_INDEXED( format ))
                    return DFB_UNSUPPORTED;

               palette_data = (IDirectFBPalette_data*) desc->dfbpalette->priv;
               if (!palette_data)
                    return DFB_INVARG;

               palette = palette_data->palette;
               if (!palette)
                    return DFB_INVARG;
          }

          ret = dfb_surface_create( data->core, data->manager,
                                    width, height, format,
                                    policy, caps,
                                    palette,
                                    &surface );
          if (ret)
               return ret;
     }


     dfb_surface_init_palette( surface, desc );

     DIRECT_ALLOCATE_INTERFACE( *interface, IDirectFBSurface );

     ret = IDirectFBSurface_Construct( *interface, NULL, NULL, surface, caps );

     dfb_surface_unref( surface );

     return ret;
}


/******************************************************************************/

DFBResult
IDirectFBSurfaceManager_Construct( IDirectFBSurfaceManager *thiz,
                                   CoreDFB                 *core,
                                   SurfaceManager          *manager,
                                   CoreSurface             *surface )
{
     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBSurfaceManager)

     data->ref     = 1;
     data->core    = core;
     data->manager = manager;
     data->surface = surface;

     thiz->AddRef                   = IDirectFBSurfaceManager_AddRef;
     thiz->Release                  = IDirectFBSurfaceManager_Release;
     thiz->CreateSurface            = IDirectFBSurfaceManager_CreateSurface;

     return DFB_OK;
}

