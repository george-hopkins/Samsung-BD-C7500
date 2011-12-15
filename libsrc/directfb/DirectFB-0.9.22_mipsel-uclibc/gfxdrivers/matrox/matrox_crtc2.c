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
#include <unistd.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <directfb.h>

#include <fusion/shmalloc.h>

#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/gfxcard.h>
#include <core/layers.h>
#include <core/screen.h>
#include <core/surfaces.h>
#include <core/windows.h>

#include <direct/mem.h>
#include <direct/messages.h>

#include <misc/conf.h>
#include <misc/util.h>

#include <fbdev/fbdev.h>

#include "regs.h"
#include "mmio.h"
#include "matrox.h"
#include "matrox_maven.h"

typedef struct {
     CoreLayerRegionConfig config;
     DFBColorAdjustment    adj;
     int                   field;

     /* Stored registers */
     struct {
          /* CRTC2 */
          __u32 c2CTL;
          __u32 c2DATACTL;
          __u32 c2MISC;
          __u32 c2OFFSET;

          __u32 c2HPARAM;
          __u32 c2VPARAM;

          __u32 c2STARTADD0;
          __u32 c2STARTADD1;
          __u32 c2PL2STARTADD0;
          __u32 c2PL2STARTADD1;
          __u32 c2PL3STARTADD0;
          __u32 c2PL3STARTADD1;
     } regs;

     MatroxMavenData mav;
} MatroxCrtc2LayerData;

static void crtc2_set_regs           ( MatroxDriverData      *mdrv,
                                       MatroxCrtc2LayerData  *mcrtc2 );

static void crtc2_calc_regs          ( MatroxDriverData      *mdrv,
                                       MatroxCrtc2LayerData  *mcrtc2,
                                       CoreLayerRegionConfig *config,
                                       CoreSurface           *surface );

static void crtc2_calc_buffer        ( MatroxDriverData      *mdrv,
                                       MatroxCrtc2LayerData  *mcrtc2,
                                       CoreSurface           *surface );

static void crtc2_set_buffer         ( MatroxDriverData      *mdrv,
                                       MatroxCrtc2LayerData  *mcrtc2 );

static DFBResult crtc2_disable_output( MatroxDriverData      *mdrv,
                                       MatroxCrtc2LayerData  *mcrtc2 );

static DFBResult crtc2_enable_output ( MatroxDriverData      *mdrv,
                                       MatroxCrtc2LayerData  *mcrtc2 );

#define CRTC2_SUPPORTED_OPTIONS   (DLOP_FIELD_PARITY | DLOP_FLICKER_FILTERING)

/**********************/

static int
crtc2LayerDataSize()
{
     return sizeof(MatroxCrtc2LayerData);
}

static DFBResult
crtc2InitLayer( CoreLayer                  *layer,
                void                       *driver_data,
                void                       *layer_data,
                DFBDisplayLayerDescription *description,
                DFBDisplayLayerConfig      *config,
                DFBColorAdjustment         *adjustment )
{
     MatroxDriverData     *mdrv   = (MatroxDriverData*) driver_data;
     MatroxCrtc2LayerData *mcrtc2 = (MatroxCrtc2LayerData*) layer_data;
     MatroxDeviceData     *mdev   = mdrv->device_data;
     MatroxMavenData      *mav    = &mcrtc2->mav;
     bool                  ntsc   = dfb_config->matrox_ntsc;
     DFBResult             res;

     if ((res = maven_init( mav, mdrv )) != DFB_OK)
          return res;


     /* set capabilities and type */
     description->caps = DLCAPS_SURFACE |
                         DLCAPS_FIELD_PARITY | DLCAPS_FLICKER_FILTERING |
                         DLCAPS_BRIGHTNESS | DLCAPS_CONTRAST |
                         DLCAPS_HUE | DLCAPS_SATURATION | DLCAPS_ALPHA_RAMP;
     description->type = DLTF_GRAPHICS | DLTF_VIDEO | DLTF_STILL_PICTURE;

     /* set name */
     snprintf( description->name,
               DFB_DISPLAY_LAYER_DESC_NAME_LENGTH, "Matrox CRTC2 Layer" );

     /* fill out the default configuration */
     config->flags        = DLCONF_WIDTH | DLCONF_HEIGHT |
                            DLCONF_PIXELFORMAT | DLCONF_BUFFERMODE |
                            DLCONF_OPTIONS | DLCONF_SURFACE_CAPS;
     config->width        = 720;
     config->height       = ntsc ? 480 : 576;
     config->pixelformat  = DSPF_YUY2;
     config->buffermode   = DLBM_FRONTONLY;
     config->options      = DLOP_NONE;
     config->surface_caps = DSCAPS_INTERLACED;

     /* fill out default color adjustment,
        only fields set in flags will be accepted from applications */
     adjustment->flags = DCAF_BRIGHTNESS | DCAF_CONTRAST |
                         DCAF_HUE | DCAF_SATURATION;
     if (mdev->g450_matrox) {
          adjustment->brightness = ntsc ? 0xAA00 : 0x9E00;
          adjustment->saturation = ntsc ? 0xAE00 : 0xBB00;
     } else {
          adjustment->brightness = ntsc ? 0xB500 : 0xA800;
          adjustment->saturation = ntsc ? 0x8E00 : 0x9500;
     }
     adjustment->contrast = ntsc ? 0xEA00 : 0xFF00;
     adjustment->hue      = 0x0000;

     /* remember color adjustment */
     mcrtc2->adj = *adjustment;

     return DFB_OK;
}

static DFBResult
crtc2TestRegion( CoreLayer                  *layer,
                 void                       *driver_data,
                 void                       *layer_data,
                 CoreLayerRegionConfig      *config,
                 CoreLayerRegionConfigFlags *failed )
{
     CoreLayerRegionConfigFlags fail = 0;

     if (config->options & ~CRTC2_SUPPORTED_OPTIONS)
          fail |= CLRCF_OPTIONS;

     if (config->surface_caps & ~DSCAPS_INTERLACED)
          fail |= CLRCF_SURFACE_CAPS;

     switch (config->format) {
          case DSPF_ARGB:
          case DSPF_RGB32:
          case DSPF_ARGB1555:
          case DSPF_RGB16:
          case DSPF_YUY2:
          case DSPF_UYVY:
          case DSPF_I420:
          case DSPF_YV12:
               break;
          default:
               fail |= CLRCF_FORMAT;
     }

     if (config->width != 720)
          fail |= CLRCF_WIDTH;

     if (config->height != (dfb_config->matrox_ntsc ? 480 : 576))
          fail |= CLRCF_HEIGHT;

     if (failed)
          *failed = fail;

     if (fail)
          return DFB_UNSUPPORTED;

     return DFB_OK;
}

static DFBResult
crtc2AddRegion( CoreLayer             *layer,
                void                  *driver_data,
                void                  *layer_data,
                void                  *region_data,
                CoreLayerRegionConfig *config )
{
     return DFB_OK;
}

static DFBResult
crtc2SetRegion( CoreLayer                  *layer,
                void                       *driver_data,
                void                       *layer_data,
                void                       *region_data,
                CoreLayerRegionConfig      *config,
                CoreLayerRegionConfigFlags  updated,
                CoreSurface                *surface,
                CorePalette                *palette )
{
     DFBResult             ret;
     MatroxDriverData     *mdrv   = (MatroxDriverData*) driver_data;
     MatroxCrtc2LayerData *mcrtc2 = (MatroxCrtc2LayerData*) layer_data;

     /* remember configuration */
     mcrtc2->config = *config;

     mcrtc2->field = !config->parity;

     crtc2_calc_regs( mdrv, mcrtc2, config, surface );
     crtc2_calc_buffer( mdrv, mcrtc2, surface );

     ret = crtc2_enable_output( mdrv, mcrtc2 );
     if (ret)
          return ret;

     return DFB_OK;
}

static DFBResult
crtc2RemoveRegion( CoreLayer *layer,
                   void      *driver_data,
                   void      *layer_data,
                   void      *region_data )
{
     MatroxDriverData     *mdrv   = (MatroxDriverData*) driver_data;
     MatroxCrtc2LayerData *mcrtc2 = (MatroxCrtc2LayerData*) layer_data;

     crtc2_disable_output( mdrv, mcrtc2 );

     return DFB_OK;
}

static DFBResult
crtc2FlipRegion( CoreLayer           *layer,
                 void                *driver_data,
                 void                *layer_data,
                 void                *region_data,
                 CoreSurface         *surface,
                 DFBSurfaceFlipFlags  flags )
{
     MatroxDriverData     *mdrv    = (MatroxDriverData*) driver_data;
     MatroxCrtc2LayerData *mcrtc2  = (MatroxCrtc2LayerData*) layer_data;
     volatile __u8        *mmio    = mdrv->mmio_base;

     dfb_surface_flip_buffers( surface, false );
     crtc2_calc_buffer( mdrv, mcrtc2, surface );

     if (mcrtc2->config.options & DLOP_FIELD_PARITY) {
          int field = (mga_in32( mmio, C2VCOUNT ) & C2FIELD) ? 1 : 0;

          while (field == mcrtc2->field) {
               dfb_screen_wait_vsync( mdrv->secondary );

               field = (mga_in32( mmio, C2VCOUNT ) & C2FIELD) ? 1 : 0;
          }
     }
     crtc2_set_buffer( mdrv, mcrtc2 );

     if (flags & DSFLIP_WAIT)
          dfb_screen_wait_vsync( mdrv->secondary );

     return DFB_OK;
}

static DFBResult
crtc2SetColorAdjustment( CoreLayer          *layer,
                         void               *driver_data,
                         void               *layer_data,
                         DFBColorAdjustment *adj )
{
     MatroxDriverData     *mdrv   = (MatroxDriverData*) driver_data;
     MatroxCrtc2LayerData *mcrtc2 = (MatroxCrtc2LayerData*) layer_data;
     MatroxMavenData      *mav    = &mcrtc2->mav;
     DFBResult             res;

     if ((res = maven_open( mav, mdrv )) != DFB_OK)
          return res;

     if (adj->flags & DCAF_HUE)
          maven_set_hue( mav, mdrv,
                         adj->hue >> 8 );
     if (adj->flags & DCAF_SATURATION)
          maven_set_saturation( mav, mdrv,
                                adj->saturation >> 8 );
     if (adj->flags & DCAF_BRIGHTNESS ||
         adj->flags & DCAF_CONTRAST)
          maven_set_bwlevel( mav, mdrv,
                             adj->brightness >> 8,
                             adj->contrast >> 8 );

     maven_close( mav, mdrv );

     /* remember color adjustment */
     mcrtc2->adj = *adj;

     return DFB_OK;
}

static DFBResult
crtc2GetCurrentOutputField( CoreLayer *layer,
                            void      *driver_data,
                            void      *layer_data,
                            int       *field )
{
     MatroxDriverData *mdrv = (MatroxDriverData*) driver_data;

     if (!field)
          return DFB_INVARG;

     *field = (mga_in32( mdrv->mmio_base, C2VCOUNT ) & C2FIELD) ? 1 : 0;

     return DFB_OK;
}

DisplayLayerFuncs matroxCrtc2Funcs = {
     LayerDataSize:         crtc2LayerDataSize,
     InitLayer:             crtc2InitLayer,

     TestRegion:            crtc2TestRegion,
     AddRegion:             crtc2AddRegion,
     SetRegion:             crtc2SetRegion,
     RemoveRegion:          crtc2RemoveRegion,
     FlipRegion:            crtc2FlipRegion,

     SetColorAdjustment:    crtc2SetColorAdjustment,
     GetCurrentOutputField: crtc2GetCurrentOutputField
};

/* internal */

static void crtc2_set_regs( MatroxDriverData     *mdrv,
                            MatroxCrtc2LayerData *mcrtc2 )
{
     volatile __u8 *mmio = mdrv->mmio_base;

     mga_out32( mmio, mcrtc2->regs.c2CTL,     C2CTL );
     mga_out32( mmio, mcrtc2->regs.c2DATACTL, C2DATACTL );
     mga_out32( mmio, mcrtc2->regs.c2HPARAM,  C2HPARAM );
     mga_out32( mmio, 0,                      C2HSYNC );
     mga_out32( mmio, mcrtc2->regs.c2VPARAM,  C2VPARAM );
     mga_out32( mmio, 0,                      C2VSYNC );
     mga_out32( mmio, mcrtc2->regs.c2OFFSET,  C2OFFSET );
     mga_out32( mmio, mcrtc2->regs.c2MISC,    C2MISC );
     mga_out32( mmio, 0,                      C2PRELOAD );
}

static void crtc2_calc_regs( MatroxDriverData      *mdrv,
                             MatroxCrtc2LayerData  *mcrtc2,
                             CoreLayerRegionConfig *config,
                             CoreSurface           *surface )
{
     MatroxDeviceData *mdev = mdrv->device_data;

     mcrtc2->regs.c2CTL = 0;

     /* Don't touch sub-picture bits. */
     mcrtc2->regs.c2DATACTL  = mga_in32( mdrv->mmio_base, C2DATACTL );
     mcrtc2->regs.c2DATACTL &= C2STATICKEY | C2OFFSETDIVEN | C2STATICKEYEN | C2SUBPICEN;

     if (mdev->g450_matrox)
          mcrtc2->regs.c2CTL |= C2PIXCLKSEL_CRISTAL;
     else
          mcrtc2->regs.c2CTL |= C2PIXCLKSEL_VDOCLK;

     /*
      * High priority request level.
      * According to G400 specs these values should
      * be fixed when CRTC2 is in YUV mode.
      */
     /* c2hiprilvl */
     mcrtc2->regs.c2CTL |= 2 << 4;
     /* c2maxhipri */
     mcrtc2->regs.c2CTL |= 1 << 8;

     mcrtc2->regs.c2CTL |= C2VIDRSTMOD_RISING | C2HPLOADEN | C2VPLOADEN;

     switch (surface->format) {
          case DSPF_ARGB1555:
          case DSPF_RGB16:
          case DSPF_RGB32:
          case DSPF_ARGB:
               mcrtc2->regs.c2DATACTL |= C2DITHEN | C2YFILTEN | C2CBCRFILTEN;
               break;
          default:
               break;
     }

     if (dfb_config->matrox_ntsc)
          mcrtc2->regs.c2DATACTL |= C2NTSCEN;

     /* pixel format settings */
     switch (surface->format) {
          case DSPF_I420:
          case DSPF_YV12:
               mcrtc2->regs.c2CTL |= C2DEPTH_YCBCR420;
               break;

          case DSPF_UYVY:
               mcrtc2->regs.c2DATACTL |= C2UYVYFMT;
               /* fall through */

          case DSPF_YUY2:
               mcrtc2->regs.c2CTL |= C2DEPTH_YCBCR422;
               break;

          case DSPF_ARGB1555:
               mcrtc2->regs.c2CTL |= C2DEPTH_15BPP;
               break;

          case DSPF_RGB16:
               mcrtc2->regs.c2CTL |= C2DEPTH_16BPP;
               break;

          case DSPF_RGB32:
          case DSPF_ARGB:
               mcrtc2->regs.c2CTL |= C2DEPTH_32BPP;
               break;

          default:
               D_BUG( "unexpected pixelformat" );
               return;
     }

     if (!(surface->caps & DSCAPS_INTERLACED))
          mcrtc2->regs.c2CTL |= C2VCBCRSINGLE;

     /* interleaved fields */
     mcrtc2->regs.c2OFFSET = surface->front_buffer->video.pitch * 2;

     {
          int hdisplay, htotal, vdisplay, vtotal;

          if (dfb_config->matrox_ntsc) {
               hdisplay = 720;
               htotal = 858;
               vdisplay = 480 / 2;
               vtotal = 525 / 2;
          } else {
               hdisplay = 720;
               htotal = 864;
               vdisplay = 576 / 2;
               vtotal = 625 / 2;
          }

          mcrtc2->regs.c2HPARAM = ((hdisplay - 8) << 16) | (htotal - 8);
          mcrtc2->regs.c2VPARAM = ((vdisplay - 1) << 16) | (vtotal - 1);

          mcrtc2->regs.c2MISC = 0;
          /* c2vlinecomp */
          mcrtc2->regs.c2MISC |= (vdisplay + 2) << 16;
     }

     /* c2bpp15halpha */
     mcrtc2->regs.c2DATACTL |= config->alpha_ramp[3] << 8;

     /* c2bpp15lalpha */
     mcrtc2->regs.c2DATACTL |= config->alpha_ramp[0] << 16;
}

static void crtc2_calc_buffer( MatroxDriverData     *mdrv,
                               MatroxCrtc2LayerData *mcrtc2,
                               CoreSurface          *surface )
{
     SurfaceBuffer *front_buffer = surface->front_buffer;
     int            field_offset = front_buffer->video.pitch;

     mcrtc2->regs.c2STARTADD1 = front_buffer->video.offset;
     mcrtc2->regs.c2STARTADD0 = mcrtc2->regs.c2STARTADD1 + field_offset;

     if (surface->caps & DSCAPS_INTERLACED)
          field_offset /= 2;
     else
          field_offset = 0;

     switch (surface->format) {
          case DSPF_I420:
               mcrtc2->regs.c2PL2STARTADD1 = mcrtc2->regs.c2STARTADD1 + surface->height * front_buffer->video.pitch;
               mcrtc2->regs.c2PL2STARTADD0 = mcrtc2->regs.c2PL2STARTADD1 + field_offset;

               mcrtc2->regs.c2PL3STARTADD1 = mcrtc2->regs.c2PL2STARTADD1 + surface->height/2 * front_buffer->video.pitch/2;
               mcrtc2->regs.c2PL3STARTADD0 = mcrtc2->regs.c2PL3STARTADD1 + field_offset;
               break;
          case DSPF_YV12:
               mcrtc2->regs.c2PL3STARTADD1 = mcrtc2->regs.c2STARTADD1 + surface->height * front_buffer->video.pitch;
               mcrtc2->regs.c2PL3STARTADD0 = mcrtc2->regs.c2PL3STARTADD1 + field_offset;

               mcrtc2->regs.c2PL2STARTADD1 = mcrtc2->regs.c2PL3STARTADD1 + surface->height/2 *  front_buffer->video.pitch/2;
               mcrtc2->regs.c2PL2STARTADD0 = mcrtc2->regs.c2PL2STARTADD1 + field_offset;
               break;
          default:
               break;
     }
}

static void crtc2_set_buffer( MatroxDriverData     *mdrv,
                              MatroxCrtc2LayerData *mcrtc2 )
{
     volatile __u8 *mmio = mdrv->mmio_base;

     mga_out32( mmio, mcrtc2->regs.c2STARTADD0, C2STARTADD0 );
     mga_out32( mmio, mcrtc2->regs.c2STARTADD1, C2STARTADD1 );
     mga_out32( mmio, mcrtc2->regs.c2PL2STARTADD0, C2PL2STARTADD0 );
     mga_out32( mmio, mcrtc2->regs.c2PL2STARTADD1, C2PL2STARTADD1 );
     mga_out32( mmio, mcrtc2->regs.c2PL3STARTADD0, C2PL3STARTADD0 );
     mga_out32( mmio, mcrtc2->regs.c2PL3STARTADD1, C2PL3STARTADD1 );
}

static void
crtc2OnOff( MatroxDriverData     *mdrv,
            MatroxCrtc2LayerData *mcrtc2,
            int                   on )
{
     volatile __u8 *mmio = mdrv->mmio_base;

     if (on)
          mcrtc2->regs.c2CTL |= C2EN;
     else
          mcrtc2->regs.c2CTL &= ~C2EN;
     mga_out32( mmio, mcrtc2->regs.c2CTL, C2CTL );

     if (on)
          mcrtc2->regs.c2CTL &= ~C2PIXCLKDIS;
     else
          mcrtc2->regs.c2CTL |= C2PIXCLKDIS;
     mga_out32( mmio, mcrtc2->regs.c2CTL, C2CTL );

     if (!on) {
          mcrtc2->regs.c2CTL &= ~C2INTERLACE;
          mga_out32( mmio, mcrtc2->regs.c2CTL, C2CTL );
     }
}

static void crtc2_set_mafc( MatroxDriverData     *mdrv,
                            int                   on )
{
     volatile __u8 *mmio = mdrv->mmio_base;
     __u8           val;

     val = mga_in_dac( mmio, XMISCCTRL );
     if (on) {
          val &= ~(MFCSEL_MASK | VDOUTSEL_MASK);
          val |= MFCSEL_MAFC | VDOUTSEL_CRTC2656;
     } else {
          val &= ~MFCSEL_MASK;
          val |= MFCSEL_DIS;
     }
     mga_out_dac( mmio, XMISCCTRL, val );
}

static DFBResult
crtc2_disable_output( MatroxDriverData     *mdrv,
                      MatroxCrtc2LayerData *mcrtc2 )
{
     MatroxDeviceData *mdev = mdrv->device_data;
     MatroxMavenData  *mav  = &mcrtc2->mav;
     DFBResult         res;

     if ((res = maven_open( mav, mdrv )) != DFB_OK)
          return res;

     maven_disable( mav, mdrv );
     if (!mdev->g450_matrox)
          crtc2_set_mafc( mdrv, 0 );
     crtc2OnOff( mdrv, mcrtc2, 0 );

     maven_close( mav, mdrv );

     if (mdev->g450_matrox) {
          volatile __u8 *mmio = mdrv->mmio_base;
          __u8           val;

          /* Set Rset to 0.7 V */
          val = mga_in_dac( mmio, XGENIOCTRL );
          val &= ~0x40;
          mga_out_dac( mmio, XGENIOCTRL, val );
          val = mga_in_dac( mmio, XGENIODATA );
          val &= ~0x40;
          mga_out_dac( mmio, XGENIODATA, val );

          val = mga_in_dac( mmio, XPWRCTRL );
          val &= ~(DAC2PDN | CFIFOPDN);
          mga_out_dac( mmio, XPWRCTRL, val );

          val = mga_in_dac( mmio, XDISPCTRL );
          val &= ~DAC2OUTSEL_MASK;
          val |= DAC2OUTSEL_DIS;
          mga_out_dac( mmio, XDISPCTRL, val );
     }

     return DFB_OK;
}

static DFBResult
crtc2_enable_output( MatroxDriverData      *mdrv,
                     MatroxCrtc2LayerData  *mcrtc2 )
{
     MatroxDeviceData *mdev = mdrv->device_data;
     MatroxMavenData  *mav  = &mcrtc2->mav;
     volatile __u8    *mmio = mdrv->mmio_base;
     DFBResult         res;

     if ((res = maven_open( mav, mdrv )) != DFB_OK)
          return res;

     if (mdev->g450_matrox) {
          volatile __u8 *mmio = mdrv->mmio_base;
          __u8           val;

          /* Set Rset to 1.0 V */
          val = mga_in_dac( mmio, XGENIOCTRL );
          val |= 0x40;
          mga_out_dac( mmio, XGENIOCTRL, val );
          val = mga_in_dac( mmio, XGENIODATA );
          val &= ~0x40;
          mga_out_dac( mmio, XGENIODATA, val );

          val = mga_in_dac( mmio, XPWRCTRL );
          val |= DAC2PDN | CFIFOPDN;
          mga_out_dac( mmio, XPWRCTRL, val );

          val = mga_in_dac( mmio, XDISPCTRL );
          val &= ~DAC2OUTSEL_MASK;
          val |= DAC2OUTSEL_TVE;
          mga_out_dac( mmio, XDISPCTRL, val );

          if (dfb_config->matrox_cable == 1) {
               val = mga_in_dac( mmio, XSYNCCTRL );
               val &= ~(DAC2HSOFF | DAC2VSOFF | DAC2HSPOL | DAC2VSPOL);
               mga_out_dac( mmio, XSYNCCTRL, val );
          }
     }

     maven_disable( mav, mdrv );
     if (!mdev->g450_matrox)
          crtc2_set_mafc( mdrv, 0 );
     crtc2OnOff( mdrv, mcrtc2, 0 );

     crtc2_set_regs( mdrv, mcrtc2 );
     crtc2_set_buffer( mdrv, mcrtc2 );

     if (!mdev->g450_matrox)
          crtc2_set_mafc( mdrv, 1 );
     crtc2OnOff( mdrv, mcrtc2, 1 );

     maven_set_regs( mav, mdrv, &mcrtc2->config, &mcrtc2->adj );

     mcrtc2->regs.c2CTL |= C2INTERLACE;
     if (mdev->g450_matrox)
          mcrtc2->regs.c2CTL |= 0x1000; /* Undocumented bit */
     while ((mga_in32( mmio, C2VCOUNT ) & 0x00000FFF) != 1)
          ;
     while ((mga_in32( mmio, C2VCOUNT ) & 0x00000FFF) != 0)
          ;
     mga_out32( mmio, mcrtc2->regs.c2CTL, C2CTL );

     maven_enable( mav, mdrv );

     if (!mdev->g450_matrox) {
          while ((mga_in32( mmio, C2VCOUNT ) & 0x00000FFF) != 1)
               ;
          while ((mga_in32( mmio, C2VCOUNT ) & 0x00000FFF) != 0)
               ;
          maven_sync( mav, mdrv );
     }

     maven_close( mav, mdrv );

     return DFB_OK;
}
