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

#include <stdio.h>

#include <core/coredefs.h>
#include <core/layers.h>
#include <core/surfaces.h>

#include "neomagic.h"

typedef struct {
     CoreLayerRegionConfig config;

     /* overlay registers */
     struct {
          __u32 OFFSET;
          __u16 PITCH;
          __u16 X1;
          __u16 X2;
          __u16 Y1;
          __u16 Y2;
          __u16 HSCALE;
          __u16 VSCALE;
          __u8  CONTROL;
     } regs;
} NeoOverlayLayerData;

static void ovl_set_regs ( NeoDriverData         *ndrv,
                           NeoOverlayLayerData   *novl );
static void ovl_calc_regs( NeoDriverData         *ndrv,
                           NeoOverlayLayerData   *novl,
                           CoreLayerRegionConfig *config,
                           CoreSurface           *surface );

#define NEO_OVERLAY_SUPPORTED_OPTIONS   (DLOP_NONE)

/**********************/

static int
ovlLayerDataSize()
{
     return sizeof(NeoOverlayLayerData);
}

static DFBResult
ovlInitLayer( CoreLayer                  *layer,
              void                       *driver_data,
              void                       *layer_data,
              DFBDisplayLayerDescription *description,
              DFBDisplayLayerConfig      *default_config,
              DFBColorAdjustment         *default_adj )
{
     /* set capabilities and type */
     description->caps = DLCAPS_SCREEN_LOCATION | DLCAPS_SURFACE |
                         DLCAPS_BRIGHTNESS;
     description->type = DLTF_VIDEO | DLTF_STILL_PICTURE;

     /* set name */
     snprintf( description->name,
               DFB_DISPLAY_LAYER_DESC_NAME_LENGTH, "NeoMagic Overlay" );

     /* fill out the default configuration */
     default_config->flags       = DLCONF_WIDTH | DLCONF_HEIGHT |
                                   DLCONF_PIXELFORMAT | DLCONF_BUFFERMODE |
                                   DLCONF_OPTIONS;
     default_config->width       = 640;
     default_config->height      = 480;
     default_config->pixelformat = DSPF_YUY2;
     default_config->buffermode  = DLBM_FRONTONLY;
     default_config->options     = DLOP_NONE;

     /* fill out default color adjustment,
        only fields set in flags will be accepted from applications */
     default_adj->flags      = DCAF_BRIGHTNESS;
     default_adj->brightness = 0x8000;

     /* FIXME: use mmio */
     iopl(3);

     neo_unlock();

     /* reset overlay */
     OUTGR(0xb0, 0x00);

     /* reset brightness */
     OUTGR(0xc4, 0x00);

     /* disable capture */
     OUTGR(0x0a, 0x21);
     OUTSR(0x08, 0xa0);
     OUTGR(0x0a, 0x01);

     neo_lock();

     return DFB_OK;
}


static void
ovlOnOff( NeoDriverData       *ndrv,
          NeoOverlayLayerData *novl,
          int                  on )
{
     /* set/clear enable bit */
     if (on)
          novl->regs.CONTROL = 0x01;
     else
          novl->regs.CONTROL = 0x00;

     /* write back to card */
     neo_unlock();
     OUTGR(0xb0, novl->regs.CONTROL);
     neo_lock();
}

static DFBResult
ovlTestRegion( CoreLayer                  *layer,
               void                       *driver_data,
               void                       *layer_data,
               CoreLayerRegionConfig      *config,
               CoreLayerRegionConfigFlags *failed )
{
     CoreLayerRegionConfigFlags fail = 0;

     /* check for unsupported options */
     if (config->options & ~NEO_OVERLAY_SUPPORTED_OPTIONS)
          fail |= CLRCF_OPTIONS;

     /* check pixel format */
     switch (config->format) {
          case DSPF_YUY2:
               break;

          default:
               fail |= CLRCF_FORMAT;
     }

     /* check width */
     if (config->width > 1024 || config->width < 160)
          fail |= CLRCF_WIDTH;

     /* check height */
     if (config->height > 1024 || config->height < 1)
          fail |= CLRCF_HEIGHT;

     /* write back failing fields */
     if (failed)
          *failed = fail;

     /* return failure if any field failed */
     if (fail)
          return DFB_UNSUPPORTED;

     return DFB_OK;
}

static DFBResult
ovlSetRegion( CoreLayer                  *layer,
              void                       *driver_data,
              void                       *layer_data,
              void                       *region_data,
              CoreLayerRegionConfig      *config,
              CoreLayerRegionConfigFlags  updated,
              CoreSurface                *surface,
              CorePalette                *palette )
{
     NeoDriverData       *ndrv = (NeoDriverData*) driver_data;
     NeoOverlayLayerData *novl = (NeoOverlayLayerData*) layer_data;

     /* remember configuration */
     novl->config = *config;

     ovl_calc_regs( ndrv, novl, config, surface );
     ovl_set_regs( ndrv, novl );

     /* enable overlay */
     ovlOnOff( ndrv, novl, 1 );

     return DFB_OK;
}

static DFBResult
ovlRemoveRegion( CoreLayer *layer,
                 void      *driver_data,
                 void      *layer_data,
                 void      *region_data )
{
     NeoDriverData       *ndrv = (NeoDriverData*) driver_data;
     NeoOverlayLayerData *novl = (NeoOverlayLayerData*) layer_data;

     /* disable overlay */
     ovlOnOff( ndrv, novl, 0 );

     return DFB_OK;
}

static DFBResult
ovlFlipRegion(  CoreLayer           *layer,
                void                *driver_data,
                void                *layer_data,
                void                *region_data,
                CoreSurface         *surface,
                DFBSurfaceFlipFlags  flags )
{
     NeoDriverData       *ndrv = (NeoDriverData*) driver_data;
     NeoOverlayLayerData *novl = (NeoOverlayLayerData*) layer_data;
#if 0
     bool                 onsync  = (flags & DSFLIP_WAITFORSYNC);

     if (onsync)
          dfb_fbdev_wait_vsync();
#endif

     dfb_surface_flip_buffers( surface, false );

     ovl_calc_regs( ndrv, novl, &novl->config, surface );
     ovl_set_regs( ndrv, novl );

     return DFB_OK;
}

static DFBResult
ovlSetColorAdjustment( CoreLayer          *layer,
                       void               *driver_data,
                       void               *layer_data,
                       DFBColorAdjustment *adj )
{
     neo_unlock();
     OUTGR(0xc4, (signed char)((adj->brightness >> 8) -128));
     neo_lock();

     return DFB_OK;
}


DisplayLayerFuncs neoOverlayFuncs = {
     LayerDataSize:      ovlLayerDataSize,
     InitLayer:          ovlInitLayer,
     SetRegion:          ovlSetRegion,
     RemoveRegion:       ovlRemoveRegion,
     TestRegion:         ovlTestRegion,
     FlipRegion:         ovlFlipRegion,
     SetColorAdjustment: ovlSetColorAdjustment
};


/* internal */

static void ovl_set_regs( NeoDriverData *ndrv, NeoOverlayLayerData *novl )
{
     neo_unlock();

     OUTGR(0xb1, ((novl->regs.X2 >> 4) & 0xf0) | (novl->regs.X1 >> 8));
     OUTGR(0xb2, novl->regs.X1);
     OUTGR(0xb3, novl->regs.X2);
     OUTGR(0xb4, ((novl->regs.Y2 >> 4) & 0xf0) | (novl->regs.Y1 >> 8));
     OUTGR(0xb5, novl->regs.Y1);
     OUTGR(0xb6, novl->regs.Y2);
     OUTGR(0xb7, novl->regs.OFFSET >> 16);
     OUTGR(0xb8, novl->regs.OFFSET >>  8);
     OUTGR(0xb9, novl->regs.OFFSET);
     OUTGR(0xba, novl->regs.PITCH >> 8);
     OUTGR(0xbb, novl->regs.PITCH);
     OUTGR(0xbc, 0x2e);  /* Neo2160: 0x4f */
     OUTGR(0xbd, 0x02);
     OUTGR(0xbe, 0x00);
     OUTGR(0xbf, 0x02);

     OUTGR(0xc0, novl->regs.HSCALE >> 8);
     OUTGR(0xc1, novl->regs.HSCALE);
     OUTGR(0xc2, novl->regs.VSCALE >> 8);
     OUTGR(0xc3, novl->regs.VSCALE);

     neo_lock();
}

static void ovl_calc_regs( NeoDriverData         *ndrv,
                           NeoOverlayLayerData   *novl,
                           CoreLayerRegionConfig *config,
                           CoreSurface           *surface )
{
     SurfaceBuffer *front_buffer = surface->front_buffer;

     /* fill register struct */
     novl->regs.X1     = config->dest.x;
     novl->regs.X2     = config->dest.x + config->dest.w - 1;

     novl->regs.Y1     = config->dest.y;
     novl->regs.Y2     = config->dest.y + config->dest.h - 1;

     novl->regs.OFFSET = front_buffer->video.offset;
     novl->regs.PITCH  = front_buffer->video.pitch;

     novl->regs.HSCALE = (surface->width  << 12) / (config->dest.w + 1);
     novl->regs.VSCALE = (surface->height << 12) / (config->dest.h + 1);
}

