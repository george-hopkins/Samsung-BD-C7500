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

#ifndef __CORE__LAYER_CONTEXT_H__
#define __CORE__LAYER_CONTEXT_H__

#include <directfb.h>

#include <core/coretypes.h>
#include <fusion/object.h>

typedef enum {
     CLCNF_ACTIVATED   = 0x00000001,
     CLCNF_DEACTIVATED = 0x00000002
} CoreLayerContextNotificationFlags;

typedef struct {
     CoreLayerContextNotificationFlags  flags;
     CoreLayerContext                  *context;
} CoreLayerContextNotification;

/*
 * Creates a pool of layer context objects.
 */
FusionObjectPool *dfb_layer_context_pool_create();

/*
 * Generates dfb_layer_context_ref(), dfb_layer_context_attach() etc.
 */
FUSION_OBJECT_METHODS( CoreLayerContext, dfb_layer_context )


DFBResult dfb_layer_context_create( CoreLayer         *layer,
                                    CoreLayerContext **ret_context );

DFBResult dfb_layer_context_get_primary_region( CoreLayerContext  *context,
                                                bool               create,
                                                CoreLayerRegion  **ret_region );

/*
 * configuration testing/setting/getting
 */
DFBResult dfb_layer_context_test_configuration ( CoreLayerContext            *context,
                                                 const DFBDisplayLayerConfig *config,
                                                 DFBDisplayLayerConfigFlags  *ret_failed );

DFBResult dfb_layer_context_set_configuration  ( CoreLayerContext            *context,
                                                 const DFBDisplayLayerConfig *config );

DFBResult dfb_layer_context_get_configuration  ( CoreLayerContext            *context,
                                                 DFBDisplayLayerConfig       *ret_config );


/*
 * configuration details
 */
DFBResult dfb_layer_context_set_src_colorkey   ( CoreLayerContext            *context,
                                                 __u8                         r,
                                                 __u8                         g,
                                                 __u8                         b );

DFBResult dfb_layer_context_set_dst_colorkey   ( CoreLayerContext            *context,
                                                 __u8                         r,
                                                 __u8                         g,
                                                 __u8                         b );

DFBResult dfb_layer_context_set_sourcerectangle( CoreLayerContext            *context,
                                                 const DFBRectangle          *source );

DFBResult dfb_layer_context_set_screenlocation ( CoreLayerContext            *context,
                                                 const DFBLocation           *location,
                                                 bool                         context_locked );

DFBResult dfb_layer_context_set_screenrectangle( CoreLayerContext            *context,
                                                 const DFBRectangle          *rectangle );

DFBResult dfb_layer_context_set_screenposition ( CoreLayerContext            *context,
                                                 int                          x,
                                                 int                          y );

DFBResult dfb_layer_context_set_opacity        ( CoreLayerContext            *context,
                                                 __u8                         opacity);

DFBResult dfb_layer_context_set_coloradjustment( CoreLayerContext            *context,
                                                 const DFBColorAdjustment    *adjustment );

DFBResult dfb_layer_context_get_coloradjustment( CoreLayerContext            *context,
                                                 DFBColorAdjustment          *ret_adjustment );

DFBResult dfb_layer_context_set_field_parity   ( CoreLayerContext            *context,
                                                 int                          field );


/*
 * window control
 */
DFBResult dfb_layer_context_create_window( CoreLayerContext       *context,
                                           int                     x,
                                           int                     y,
                                           int                     width,
                                           int                     height,
                                           DFBWindowCapabilities   caps,
                                           DFBSurfaceCapabilities  surface_caps,
                                           DFBSurfacePixelFormat   pixelformat,
                                           CoreWindow            **window );

CoreWindow      *dfb_layer_context_find_window( CoreLayerContext       *context,
                                                DFBWindowID             id );

CoreWindowStack *dfb_layer_context_windowstack( const CoreLayerContext *context );

bool             dfb_layer_context_active     ( const CoreLayerContext *context );

/*
 * Locking
 */
DirectResult dfb_layer_context_lock  ( CoreLayerContext *context );
DirectResult dfb_layer_context_unlock( CoreLayerContext *context );

#endif
