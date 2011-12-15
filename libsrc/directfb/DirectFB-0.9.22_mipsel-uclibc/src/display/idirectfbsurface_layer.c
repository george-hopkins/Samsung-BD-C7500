/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/state.h>
#include <core/layers.h>
#include <core/layer_region.h>
#include <core/surfaces.h>
#include <core/system.h>

#include <idirectfbsurface.h>
#include <idirectfbsurface_layer.h>

#include <misc/util.h>

#include <direct/interface.h>
#include <direct/mem.h>

#include <gfx/util.h>



/*
 * private data struct of IDirectFBSurface_Layer
 */
typedef struct {
     IDirectFBSurface_data  base;   /* base Surface implementation */

     CoreLayerRegion       *region; /* the region this surface belongs to */
} IDirectFBSurface_Layer_data;


static void
IDirectFBSurface_Layer_Destruct( IDirectFBSurface *thiz )
{
     IDirectFBSurface_Layer_data *data = (IDirectFBSurface_Layer_data*) thiz->priv;

     dfb_layer_region_unref( data->region );
     IDirectFBSurface_Destruct( thiz );
}

static DFBResult
IDirectFBSurface_Layer_Release( IDirectFBSurface *thiz )
{
     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface_Layer)

     if (--data->base.ref == 0)
          IDirectFBSurface_Layer_Destruct( thiz );

     return DFB_OK;
}

static DFBResult
IDirectFBSurface_Layer_Flip( IDirectFBSurface    *thiz,
                             const DFBRegion     *region,
                             DFBSurfaceFlipFlags  flags )
{
     DFBRegion reg;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface_Layer)

     if (!data->base.surface)
          return DFB_DESTROYED;

     if (data->base.locked)
          return DFB_LOCKED;

     if (!data->base.area.current.w || !data->base.area.current.h ||
         (region && (region->x1 > region->x2 || region->y1 > region->y2)))
          return DFB_INVAREA;


     dfb_region_from_rectangle( &reg, &data->base.area.current );

     if (region) {
          DFBRegion clip = DFB_REGION_INIT_TRANSLATED( region,
                                                       data->base.area.wanted.x,
                                                       data->base.area.wanted.y );

          if (!dfb_region_region_intersect( &reg, &clip ))
               return DFB_INVAREA;
     }

     return dfb_layer_region_flip_update( data->region, &reg, flags );
}

static DFBResult
IDirectFBSurface_Layer_GetSubSurface( IDirectFBSurface    *thiz,
                                      const DFBRectangle  *rect,
                                      IDirectFBSurface   **surface )
{
     DFBRectangle wanted, granted;

     DIRECT_INTERFACE_GET_DATA(IDirectFBSurface_Layer)

     /* Check arguments */
     if (!data->base.surface)
          return DFB_DESTROYED;

     if (!surface)
          return DFB_INVARG;

     /* Compute wanted rectangle */
     if (rect) {
          wanted = *rect;

          wanted.x += data->base.area.wanted.x;
          wanted.y += data->base.area.wanted.y;

          if (wanted.w <= 0 || wanted.h <= 0) {
               wanted.w = 0;
               wanted.h = 0;
          }
     }
     else
          wanted = data->base.area.wanted;

     /* Compute granted rectangle */
     granted = wanted;

     dfb_rectangle_intersect( &granted, &data->base.area.granted );

     /* Allocate and construct */
     DIRECT_ALLOCATE_INTERFACE( *surface, IDirectFBSurface );

     return IDirectFBSurface_Layer_Construct( *surface, &wanted, &granted,
                                              data->region, data->base.caps |
                                              DSCAPS_SUBSURFACE );
}

DFBResult
IDirectFBSurface_Layer_Construct( IDirectFBSurface       *thiz,
                                  DFBRectangle           *wanted,
                                  DFBRectangle           *granted,
                                  CoreLayerRegion        *region,
                                  DFBSurfaceCapabilities  caps )
{
     DFBResult    ret;
     CoreSurface *surface;

     DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBSurface_Layer);

     if (dfb_layer_region_ref( region ))
          return DFB_FUSION;

     ret = dfb_layer_region_get_surface( region, &surface );
     if (ret) {
          dfb_layer_region_unref( region );
          DIRECT_DEALLOCATE_INTERFACE(thiz);
          return ret;
     }

     ret = IDirectFBSurface_Construct( thiz, wanted, granted,
                                       surface, surface->caps | caps );
     if (ret) {
          dfb_surface_unref( surface );
          dfb_layer_region_unref( region );
          return ret;
     }

     dfb_surface_unref( surface );

     data->region = region;

     thiz->Release       = IDirectFBSurface_Layer_Release;
     thiz->Flip          = IDirectFBSurface_Layer_Flip;
     thiz->GetSubSurface = IDirectFBSurface_Layer_GetSubSurface;

     return DFB_OK;
}

