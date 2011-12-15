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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#ifdef USE_ZLIB
#include <zlib.h>
#endif

#include <directfb.h>
#include <directfb_strings.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <fusion/shmalloc.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/layers.h>
#include <core/layers_internal.h>
#include <core/palette.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/surfacemanager_internal.h>
#include <core/system.h>
#include <core/windows_internal.h>

#include <gfx/convert.h>
#include <gfx/util.h>

#include <byteswap.h>

D_DEBUG_DOMAIN( Core_Surface, "Core/Surface", "DirectFB Surface Core" );

static const __u8 lookup3to8[] = { 0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff };
static const __u8 lookup2to8[] = { 0x00, 0x55, 0xaa, 0xff };

#if D_DEBUG_ENABLED

static const DirectFBPixelFormatNames(format_names);

D_CONST_FUNC
static const char *
pixelformat_name( DFBSurfacePixelFormat format )
{
     int i;

     for (i=0; i<D_ARRAY_SIZE(format_names); i++) {
          if (format_names[i].format == format)
               return format_names[i].name;
     }

     return "INVALID";
}

#endif

/**************************************************************************************************/

static const ReactionFunc dfb_surface_globals[] = {
/* 0 */   _dfb_layer_region_surface_listener,
/* 1 */   _dfb_windowstack_background_image_listener,
/* 2 */   _dfb_window_surface_listener,
          NULL
};

static void surface_destructor( FusionObject *object, bool zombie )
{
     CoreSurface *surface = (CoreSurface*) object;

     D_HEAVYDEBUG( "Core/Surface: destroying %p (%dx%d%s)\n", surface,
                 surface->width, surface->height, zombie ? " ZOMBIE" : "");

     /* announce surface destruction */
     dfb_surface_notify_listeners( surface, CSNF_DESTROY );

     /* deallocate first buffer */
     dfb_surface_destroy_buffer( surface, surface->front_buffer );

     /* deallocate second buffer if it's another one */
     if (surface->back_buffer != surface->front_buffer)
          dfb_surface_destroy_buffer( surface, surface->back_buffer );

     /* deallocate third buffer if it's another one */
     if (surface->idle_buffer != surface->front_buffer &&
         surface->idle_buffer != surface->back_buffer)
          dfb_surface_destroy_buffer( surface, surface->idle_buffer );

     /* deallocate depth buffer if existent */
     if (surface->depth_buffer)
          dfb_surface_destroy_buffer( surface, surface->depth_buffer );

     /* unlink palette */
     if (surface->palette) {
          dfb_palette_detach_global( surface->palette,
                                     &surface->palette_reaction );
          dfb_palette_unlink( &surface->palette );
     }

     direct_serial_deinit( &surface->serial );

     fusion_object_destroy( object );
}

FusionObjectPool *dfb_surface_pool_create()
{
     FusionObjectPool *pool;

#ifdef DEBUG_ALLOCATION
     dfbbcm_show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

     pool = fusion_object_pool_create( "Surface Pool",
                                       sizeof(CoreSurface),
                                       sizeof(CoreSurfaceNotification),
                                       surface_destructor );

     return pool;
}

/**************************************************************************************************/

DFBResult dfb_surface_create( CoreDFB *core,
                              SurfaceManager *manager,
                              int width, int height,
                              DFBSurfacePixelFormat format,
                              CoreSurfacePolicy policy,
                              DFBSurfaceCapabilities caps, CorePalette *palette,
                              CoreSurface **ret_surface )
{
     DFBResult    ret;
     CoreSurface *surface;

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_create( core %p, size %dx%d, format %s policy %d palette %p )\n",
                 core, width, height, pixelformat_name( format ), policy, palette );

     if (width * (long long) height > SURFACE_MAX_WIDTH*SURFACE_MAX_HEIGHT)
          return DFB_LIMITEXCEEDED;

     surface = dfb_core_create_surface( core );

     ret = dfb_surface_init( core, surface, manager, width, height, format, caps, palette );
     if (ret) {
          fusion_object_destroy( &surface->object );
          return ret;
     }

     switch (policy) {
          case CSP_SYSTEMONLY:
               surface->caps |= DSCAPS_SYSTEMONLY;
               break;
          case CSP_VIDEOONLY:
               surface->caps |= DSCAPS_VIDEOONLY;
               break;
          default:
               ;
     }

     /* Allocate front buffer. */
     ret = dfb_surface_allocate_buffer( surface, policy, format, &surface->front_buffer );
     if (ret) {
          fusion_object_destroy( &surface->object );
          return ret;
     }

     /* Allocate back buffer. */
     if (caps & DSCAPS_FLIPPING) {
          ret = dfb_surface_allocate_buffer( surface, policy, format, &surface->back_buffer );
          if (ret) {
               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }
     else
          surface->back_buffer = surface->front_buffer;

     /* Allocate extra back buffer. */
     if (caps & DSCAPS_TRIPLE) {
          ret = dfb_surface_allocate_buffer( surface, policy, format, &surface->idle_buffer );
          if (ret) {
               dfb_surface_destroy_buffer( surface, surface->back_buffer );
               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }
     else
          surface->idle_buffer = surface->front_buffer;

     /* Allocate depth buffer. */
     if (caps & DSCAPS_DEPTH) {
          ret = dfb_surface_allocate_buffer( surface, CSP_VIDEOONLY,  /* FIXME */
                                             DSPF_RGB16, &surface->depth_buffer );
          if (ret) {
               if (surface->idle_buffer != surface->front_buffer)
                    dfb_surface_destroy_buffer( surface, surface->idle_buffer );

               if (surface->back_buffer != surface->front_buffer)
                    dfb_surface_destroy_buffer( surface, surface->back_buffer );

               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }


     fusion_object_activate( &surface->object );

     *ret_surface = surface;

     return DFB_OK;
}

DFBResult dfb_surface_create_preallocated( CoreDFB *core,
                                           int width, int height,
                                           DFBSurfacePixelFormat format,
                                           CoreSurfacePolicy policy, DFBSurfaceCapabilities caps,
                                           CorePalette *palette,
                                           void *front_buffer, void *back_buffer,
                                           int front_pitch, int back_pitch,
                                           CoreSurface **surface )
{
     DFBResult    ret;
     CoreSurface *s;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_create_preallocated( core %p, size %dx%d, format %s )\n",
                 core, width, height, pixelformat_name( format ) );

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     if (policy == CSP_VIDEOONLY || (caps & (DSCAPS_DEPTH | DSCAPS_TRIPLE)))
          return DFB_UNSUPPORTED;

     s = dfb_core_create_surface( core );

     ret = dfb_surface_init( core, s, NULL, width, height, format, caps, palette );
     if (ret) {
          fusion_object_destroy( &s->object );
          return ret;
     }

     if (policy == CSP_SYSTEMONLY)
          s->caps |= DSCAPS_SYSTEMONLY;


     s->front_buffer = (SurfaceBuffer *) SHCALLOC( 1, sizeof(SurfaceBuffer) );

     s->front_buffer->flags   = SBF_FOREIGN_SYSTEM | SBF_WRITTEN;
     s->front_buffer->policy  = policy;
     s->front_buffer->format  = format;
     s->front_buffer->surface = s;

     s->front_buffer->system.health = CSH_STORED;
     s->front_buffer->system.pitch  = front_pitch;
     s->front_buffer->system.addr   = front_buffer;

     if (caps & DSCAPS_FLIPPING) {
          s->back_buffer = (SurfaceBuffer *) SHMALLOC( sizeof(SurfaceBuffer) );
          direct_memcpy( s->back_buffer, s->front_buffer, sizeof(SurfaceBuffer) );

          s->back_buffer->system.pitch  = back_pitch;
          s->back_buffer->system.addr   = back_buffer;
     }
     else
          s->back_buffer = s->front_buffer;

     /* No triple buffering */
     s->idle_buffer = s->front_buffer;

     fusion_object_activate( &s->object );

     *surface = s;

     return DFB_OK;
}

DirectResult
dfb_surface_notify_listeners( CoreSurface                  *surface,
                              CoreSurfaceNotificationFlags  flags)
{
     CoreSurfaceNotification notification;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_notify_listeners( %p, 0x%08x )\n", surface, flags );

     notification.flags   = flags;
     notification.surface = surface;

     direct_serial_increase( &surface->serial );

     return dfb_surface_dispatch( surface, &notification, dfb_surface_globals );
}

DFBResult dfb_surface_reformat( CoreDFB *core, CoreSurface *surface,
                                int width, int height,
                                DFBSurfacePixelFormat format )
{
     int old_width, old_height;
     DFBSurfacePixelFormat old_format;
     SurfaceBuffer	*front;
     SurfaceBuffer	*back;
     DFBResult ret;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_reformat( %p, %dx%d, %s )\n",
                 surface, width, height, dfb_pixelformat_name( format ) );

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     if (width * (long long) height > SURFACE_MAX_WIDTH*SURFACE_MAX_HEIGHT)
          return DFB_LIMITEXCEEDED;

     if (surface->front_buffer->flags & SBF_FOREIGN_SYSTEM ||
         surface->back_buffer->flags  & SBF_FOREIGN_SYSTEM)
     {
          return DFB_UNSUPPORTED;
     }

     dfb_surfacemanager_lock( surface->manager );

     front = surface->front_buffer;
     back = surface->back_buffer;

     if (front->system.locked || front->video.locked ||
         back->system.locked  || back->video.locked)
     {
          dfb_surfacemanager_unlock( surface->manager );
          return DFB_LOCKED;
     }

     old_width  = surface->width;
     old_height = surface->height;
     old_format = surface->format;

     surface->width  = width;
     surface->height = height;
     surface->format = format;

     if (width      <= surface->min_width &&
         old_width  <= surface->min_width &&
         height     <= surface->min_height &&
         old_height <= surface->min_height &&
         old_format == surface->format)
     {
          dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT );
          dfb_surfacemanager_unlock( surface->manager );
          return DFB_OK;
     }

     ret = dfb_surface_reallocate_buffer( surface, format, surface->front_buffer );
     if (ret) {
          surface->width  = old_width;
          surface->height = old_height;
          surface->format = old_format;

          dfb_surfacemanager_unlock( surface->manager );
          return ret;
     }

     if (surface->caps & DSCAPS_FLIPPING) {
          ret = dfb_surface_reallocate_buffer( surface, format, surface->back_buffer );
          if (ret) {
               surface->width  = old_width;
               surface->height = old_height;
               surface->format = old_format;

               dfb_surface_reallocate_buffer( surface, old_format, surface->front_buffer );

               dfb_surfacemanager_unlock( surface->manager );
               return ret;
          }
     }

     if (surface->caps & DSCAPS_TRIPLE) {
          ret = dfb_surface_reallocate_buffer( surface, format, surface->idle_buffer );
          if (ret) {
               surface->width  = old_width;
               surface->height = old_height;
               surface->format = old_format;
               dfb_surface_reallocate_buffer( surface, old_format, surface->back_buffer );
               dfb_surface_reallocate_buffer( surface, old_format, surface->front_buffer );

               dfb_surfacemanager_unlock( surface->manager );
               return ret;
          }
     }

     if (surface->caps & DSCAPS_DEPTH) {
          ret = dfb_surface_reallocate_buffer( surface, surface->depth_buffer->format,
                                               surface->depth_buffer );
          if (ret) {
               surface->width  = old_width;
               surface->height = old_height;
               surface->format = old_format;

               if (surface->caps & DSCAPS_FLIPPING) {
                    dfb_surface_reallocate_buffer( surface, old_format, surface->back_buffer );

                    if (surface->caps & DSCAPS_TRIPLE)
                         dfb_surface_reallocate_buffer( surface, old_format, surface->idle_buffer );
               }

               dfb_surface_reallocate_buffer( surface, old_format, surface->front_buffer );

               dfb_surfacemanager_unlock( surface->manager );
               return ret;
          }
     }

     if (DFB_PIXELFORMAT_IS_INDEXED( format ) && !surface->palette) {
          DFBResult    ret;
          CorePalette *palette;

          ret = dfb_palette_create( core, 1 << DFB_COLOR_BITS_PER_PIXEL( format ), &palette );
          if (ret) {
               D_DERROR( ret, "Core/Surface: Could not create a palette with %d entries!\n",
                         1 << DFB_COLOR_BITS_PER_PIXEL( format ) );
          }
          else {
               switch (format) {
                    case DSPF_LUT8:
                         dfb_palette_generate_rgb332_map( palette );
                         break;

                    case DSPF_ALUT44:
                         dfb_palette_generate_rgb121_map( palette );
                         break;

                    default:
                         D_WARN( "unknown indexed format" );
               }

               dfb_surface_set_palette( surface, palette );

               dfb_palette_unref( palette );
          }
     }

     dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT | CSNF_SYSTEM | CSNF_VIDEO );

     dfb_surfacemanager_unlock( surface->manager );

     return DFB_OK;
}

DFBResult dfb_surface_reconfig( CoreSurface       *surface,
                                CoreSurfacePolicy  front_policy,
                                CoreSurfacePolicy  back_policy )
{
     DFBResult      ret;
     SurfaceBuffer *front;
     SurfaceBuffer *back;
     SurfaceBuffer *idle;
     SurfaceBuffer *depth;
     SurfaceBuffer *new_front = NULL;
     SurfaceBuffer *new_back  = NULL;
     SurfaceBuffer *new_idle  = NULL;
     SurfaceBuffer *new_depth = NULL;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_reconfig( %p, %d, %d )\n", surface, front_policy, back_policy );

     D_ASSERT( surface != NULL );
     D_ASSERT( surface->front_buffer != NULL );

     front = surface->front_buffer;
     back  = surface->back_buffer;
     idle  = surface->idle_buffer;
     depth = surface->depth_buffer;

     if ((front->flags | back->flags) & SBF_FOREIGN_SYSTEM)
          return DFB_UNSUPPORTED;

     dfb_surfacemanager_lock( surface->manager );


     if (front->policy != front_policy) {
          ret = dfb_surface_allocate_buffer( surface, front_policy, surface->format, &new_front );
          if (ret) {
               dfb_surfacemanager_unlock( surface->manager );
               return ret;
          }
     }

     if (surface->caps & DSCAPS_FLIPPING) {
          ret = dfb_surface_allocate_buffer( surface, back_policy, surface->format, &new_back );
          if (ret)
               goto error;
     }

     if (surface->caps & DSCAPS_TRIPLE) {
          ret = dfb_surface_allocate_buffer( surface, back_policy, surface->format, &new_idle );
          if (ret)
               goto error;
     }

     if (surface->caps & DSCAPS_DEPTH) {
          ret = dfb_surface_allocate_buffer( surface, CSP_VIDEOONLY,  /* FIXME */
                                             DSPF_RGB16, &new_depth );
          if (ret)
               goto error;
     }

     if (new_front) {
          dfb_surface_destroy_buffer( surface, front );
          surface->front_buffer = new_front;
     }


     if (back != front)
          dfb_surface_destroy_buffer( surface, back );

     surface->back_buffer = new_back ? : surface->front_buffer;


     if (idle != front && idle != back)
          dfb_surface_destroy_buffer( surface, idle );

     surface->idle_buffer = new_idle ? : surface->front_buffer;


     if (depth)
          dfb_surface_destroy_buffer( surface, depth );

     surface->depth_buffer = new_depth;


     dfb_surfacemanager_unlock( surface->manager );

     dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT | CSNF_SYSTEM | CSNF_VIDEO );

     return DFB_OK;


error:
     if (new_depth)
          dfb_surface_destroy_buffer( surface, new_depth );

     if (new_idle)
          dfb_surface_destroy_buffer( surface, new_idle );

     if (new_back)
          dfb_surface_destroy_buffer( surface, new_back );

     if (new_front)
          dfb_surface_destroy_buffer( surface, new_front );

     dfb_surfacemanager_unlock( surface->manager );

     return ret;
}

DFBResult
dfb_surface_allocate_depth( CoreSurface *surface )
{
     DFBResult ret;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_allocate_depth( %p )\n", surface );

     D_ASSERT( surface != NULL );

     dfb_surfacemanager_lock( surface->manager );

     D_ASSUME( ! (surface->caps & DSCAPS_DEPTH) );

     if (surface->caps & DSCAPS_DEPTH) {
          dfb_surfacemanager_unlock( surface->manager );
          return DFB_OK;
     }

     ret = dfb_surface_allocate_buffer( surface, CSP_VIDEOONLY,  /* FIXME */
                                        DSPF_RGB16, &surface->depth_buffer );
     if (ret == DFB_OK)
          surface->caps |= DSCAPS_DEPTH;

     dfb_surfacemanager_unlock( surface->manager );

     return ret;
}

void
dfb_surface_deallocate_depth( CoreSurface *surface )
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_deallocate_depth( %p )\n", surface );

     D_ASSERT( surface != NULL );

     dfb_surfacemanager_lock( surface->manager );

     D_ASSUME( surface->caps & DSCAPS_DEPTH );

     if (! (surface->caps & DSCAPS_DEPTH)) {
          dfb_surfacemanager_unlock( surface->manager );
          return;
     }

     dfb_surface_destroy_buffer( surface, surface->depth_buffer );

     surface->depth_buffer  = NULL;
     surface->caps         &= ~DSCAPS_DEPTH;

     dfb_surfacemanager_unlock( surface->manager );
}

#define BYTESWAPPALETTE

DFBResult
dfb_surface_init_palette( CoreSurface *surface, const DFBSurfaceDescription *desc )
{
     int          num;
     CorePalette *palette = surface->palette;
#ifdef BYTESWAPPALETTE
     __u32        i;
#endif

     if (!palette || !(desc->flags & DSDESC_PALETTE))
          return DFB_OK;

     num = MIN( desc->palette.size, palette->num_entries );

#ifdef BYTESWAPPALETTE
     for (i = 0; i < num; i++) {
        ((__u32 *)palette->entries)[i] = bswap_32(((__u32 *)desc->palette.entries)[i]);
     }
#else
     direct_memcpy( palette->entries, desc->palette.entries, num * sizeof(DFBColor));
#endif

     dfb_palette_update( palette, 0, num - 1 );

     return DFB_OK;
}

DFBResult
dfb_surface_set_palette( CoreSurface *surface,
                         CorePalette *palette )
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_set_palette( %p, %p )\n", surface, palette );

     D_ASSERT( surface != NULL );

     if (surface->palette == palette)
          return DFB_OK;

     if (surface->palette) {
          dfb_palette_detach_global( surface->palette, &surface->palette_reaction );
          dfb_palette_unlink( &surface->palette );
     }

     if (palette) {
          dfb_palette_link( &surface->palette, palette );
          dfb_palette_attach_global( palette, DFB_SURFACE_PALETTE_LISTENER,
                                     surface, &surface->palette_reaction );
     }

     dfb_surface_notify_listeners( surface, CSNF_PALETTE_CHANGE );

     return DFB_OK;
}

void dfb_surface_flip_buffers( CoreSurface *surface, bool write_front )
{
     SurfaceBuffer *front;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_flip_buffers( %p, %s )\n", surface, write_front ? "true" : "false" );

     D_ASSERT( surface != NULL );

     D_ASSERT(surface->back_buffer->policy == surface->front_buffer->policy);

     dfb_surfacemanager_lock( surface->manager );

     if (surface->caps & DSCAPS_TRIPLE) {
          front = surface->front_buffer;
          surface->front_buffer = surface->back_buffer;

          if (write_front)
               surface->back_buffer = front;
          else {
               surface->back_buffer = surface->idle_buffer;
               surface->idle_buffer = front;
          }
     } else {
          front = surface->front_buffer;
          surface->front_buffer = surface->back_buffer;
          surface->back_buffer = front;

          /* To avoid problems with buffer deallocation */
          surface->idle_buffer = surface->front_buffer;
     }

     dfb_surfacemanager_unlock( surface->manager );

     dfb_surface_notify_listeners( surface, CSNF_FLIP );
}

void
dfb_surface_set_field( CoreSurface *surface, int field )
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_set_field( %p, %d )\n", surface, field );

     D_ASSERT( surface != NULL );

     surface->field = field;

     dfb_surface_notify_listeners( surface, CSNF_FIELD );
}

void
dfb_surface_set_alpha_ramp( CoreSurface *surface,
                            __u8         a0,
                            __u8         a1,
                            __u8         a2,
                            __u8         a3 )
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_set_alpha_ramp( %p, %02x %02x %02x %02x )\n", surface, a0, a1, a2, a3 );

     D_ASSERT( surface != NULL );

     surface->alpha_ramp[0] = a0;
     surface->alpha_ramp[1] = a1;
     surface->alpha_ramp[2] = a2;
     surface->alpha_ramp[3] = a3;

     dfb_surface_notify_listeners( surface, CSNF_ALPHA_RAMP );
}

DFBResult dfb_surface_soft_lock(
     CoreSurface         *surface, 
     SurfaceBuffer       *buffer, 
     DFBSurfaceLockFlags flags,
     void                **data, 
     int                 *pitch )
{
     DFBResult ret;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_soft_lock( %p, %s%s%s )\n", surface,
                 (flags & DSLF_READ) ? "r" : "", (flags & DSLF_WRITE) ? "w" : "",
                 (flags & CSLF_FORCE) ? "F" : "");

     D_ASSERT( surface != NULL );
     D_ASSERT( data != NULL );
     D_ASSERT( pitch != NULL );

     dfb_surfacemanager_lock( surface->manager );
     ret = dfb_surface_software_lock( surface, buffer, flags, data, pitch );
     dfb_surfacemanager_unlock( surface->manager );

     return ret;
}

DFBResult dfb_surface_software_lock(
     CoreSurface         *surface, 
     SurfaceBuffer       *buffer,
     DFBSurfaceLockFlags flags,
     void                **data, 
     int                 *pitch)
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_software_lock( %p, %s%s%s )\n", surface,
                 (flags & DSLF_READ) ? "r" : "", (flags & DSLF_WRITE) ? "w" : "",
                 (flags & CSLF_FORCE) ? "F" : "");

     D_ASSERT( surface != NULL );
     D_ASSERT( buffer != NULL );
     D_ASSERT( flags != 0 );
     D_ASSERT( data != NULL );
     D_ASSERT( pitch != NULL );

     switch (buffer->policy) {
          case CSP_SYSTEMONLY:
               buffer->system.locked++;

               D_HEAVYDEBUG( "Core/Surface:   -> system only, counter: %d\n", buffer->system.locked );

               *data = buffer->system.addr;
               *pitch = buffer->system.pitch;

               break;
          case CSP_VIDEOLOW:
               /* no valid video instance
                  or read access and valid system? system lock! */
               if ((buffer->video.health != CSH_STORED ||
                    (flags & DSLF_READ && buffer->system.health == CSH_STORED))
                   && !buffer->video.locked)
               {
                    dfb_surfacemanager_assure_system( surface->manager, buffer );
                    buffer->system.locked++;

                    D_HEAVYDEBUG( "Core/Surface:   -> auto system, counter: %d\n", buffer->system.locked );

                    *data = buffer->system.addr;
                    *pitch = buffer->system.pitch;
                    if (flags & DSLF_WRITE &&
                        buffer->video.health == CSH_STORED)
                         buffer->video.health = CSH_RESTORE;
               }
               else {
                    /* ok, write only goes into video directly */
                    buffer->video.locked++;

                    D_HEAVYDEBUG( "Core/Surface:   -> auto video, counter: %d\n", buffer->video.locked );

                    *data = dfb_system_video_memory_virtual( buffer->video.offset);
                    *pitch = buffer->video.pitch;

                    if (flags & DSLF_WRITE)
                         buffer->system.health = CSH_RESTORE;

                    dfb_surface_video_access_by_software( buffer, flags );
               }
               break;
          case CSP_VIDEOHIGH:
               /* no video instance yet? system lock! */
               if (buffer->video.health != CSH_STORED) {
                    /* no video health, no fetch */
                    buffer->system.locked++;

                    D_HEAVYDEBUG( "Core/Surface:   -> auto system, counter: %d\n", buffer->system.locked );

                    *data = buffer->system.addr;
                    *pitch = buffer->system.pitch;
                    break;
               }

               /* video lock! write access? restore system! */
               if (flags & DSLF_WRITE)
                    buffer->system.health = CSH_RESTORE;
               /* FALL THROUGH, for the rest we have to do a video lock
                  as if it had the policy CSP_VIDEOONLY */

          case CSP_VIDEOONLY:
          case CSP_VIDEOONLY_RESERVED:
               if (dfb_surfacemanager_assure_video( surface->manager, buffer ))
                    D_ONCE( "accessing video memory during suspend" );

               buffer->video.locked++;

               if (buffer->policy == CSP_VIDEOONLY)
                    D_HEAVYDEBUG( "Core/Surface:   -> video only, counter: %d\n", buffer->video.locked );
               else
                    D_HEAVYDEBUG( "Core/Surface:   -> auto video, counter: %d\n", buffer->video.locked );

               *data = dfb_system_video_memory_virtual( buffer->video.offset );
               *pitch = buffer->video.pitch;

               dfb_surface_video_access_by_software( buffer, flags );
               break;

          default:
               D_BUG( "invalid surface policy" );
               return DFB_BUG;
     }

     if (flags & DSLF_WRITE)
          buffer->flags |= SBF_WRITTEN;

     D_HEAVYDEBUG( "Core/Surface:   -> %p [%d]\n", *data, *pitch );

     return DFB_OK;
}

DFBResult dfb_surface_hardware_lock(
     CoreSurface         *surface,
     SurfaceBuffer       *buffer,
     DFBSurfaceLockFlags flags)
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_hardware_lock( %p, %s%s%s )\n", surface,
                 (flags & DSLF_READ) ? "r" : "", (flags & DSLF_WRITE) ? "w" : "",
                 (flags & CSLF_FORCE) ? "F" : "");

     D_ASSERT( surface != NULL );
     D_ASSERT( buffer != NULL );
     D_ASSERT( flags != 0 );

     switch (buffer->policy) {
          case CSP_SYSTEMONLY:
               /* never ever! */
               break;

          case CSP_VIDEOHIGH:
          case CSP_VIDEOLOW:
               /* avoid inconsistency, could be optimized (read/write) */
               if (buffer->system.locked)
                    break;

               /* no reading? no force? no video instance? no success! ;-) */
               if (!(flags & (DSLF_READ|CSLF_FORCE)) && buffer->video.health != CSH_STORED)
                    break;

               if (dfb_surfacemanager_assure_video( surface->manager, buffer ))
                    break;

               if (flags & DSLF_WRITE)
                    buffer->system.health = CSH_RESTORE;
               /* fall through */

          case CSP_VIDEOONLY:
          case CSP_VIDEOONLY_RESERVED:
               if (dfb_surfacemanager_assure_video( surface->manager, buffer ))
                    break;

               buffer->video.locked++;

               if (buffer->policy == CSP_VIDEOONLY)
                    D_HEAVYDEBUG( "Core/Surface:   -> video only, counter: %d\n", buffer->video.locked );
               else
                    D_HEAVYDEBUG( "Core/Surface:   -> auto video, counter: %d\n", buffer->video.locked );

               dfb_surface_video_access_by_hardware( buffer, flags );

               if (flags & DSLF_WRITE)
                    buffer->flags |= SBF_WRITTEN;

               return DFB_OK;

          default:
               D_BUG( "invalid surface policy" );
               return DFB_BUG;
     }

     D_HEAVYDEBUG( "Core/Surface:   -> FAILED\n" );

     return DFB_FAILURE;
}

void dfb_surface_unlock(
     CoreSurface         *surface, 
     SurfaceBuffer       *buffer,
     DFBSurfaceLockFlags flags)
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_unlock( %p, %s%s%s )\n", surface,
                 (flags & DSLF_READ) ? "r" : "", (flags & DSLF_WRITE) ? "w" : "",
                 (flags & CSLF_FORCE) ? "F" : "");

     D_ASSERT( surface != NULL );
     D_ASSERT( buffer != NULL );

     dfb_surfacemanager_lock( surface->manager );

     D_HEAVYDEBUG( "Core/Surface:   -> system/video count: %d/%d before\n", buffer->system.locked, buffer->video.locked );

     D_ASSERT( buffer->system.locked == 0 || buffer->video.locked == 0 );

     if (buffer->system.locked)
          buffer->system.locked--;

     if (buffer->video.locked)
          buffer->video.locked--;

     D_HEAVYDEBUG( "Core/Surface:   -> system/video count: %d/%d after\n", buffer->system.locked, buffer->video.locked );

     dfb_surfacemanager_unlock( surface->manager );
}

DFBResult dfb_surface_init ( CoreDFB                *core,
                             CoreSurface            *surface,
                             SurfaceManager         *manager,
                             int                     width,
                             int                     height,
                             DFBSurfacePixelFormat   format,
                             DFBSurfaceCapabilities  caps,
                             CorePalette            *palette )
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_init( %p, %dx%d, %s, 0x%08x, %p )\n",
                 surface, width, height, dfb_pixelformat_name( format ), caps, palette );

     D_ASSUME( core != NULL );
     D_ASSERT( surface != NULL );
     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     switch (format) {
          case DSPF_A1:
          case DSPF_A8:
          case DSPF_ALUT44:
          case DSPF_ARGB:
          case DSPF_ARGB1555:
          case DSPF_ARGB2554:
          case DSPF_ARGB4444:
          case DSPF_AiRGB:
          case DSPF_I420:
          case DSPF_LUT8:
          case DSPF_NV12:
          case DSPF_NV21:
          case DSPF_NV16:
          case DSPF_RGB16:
          case DSPF_RGB24:
          case DSPF_RGB32:
          case DSPF_RGB332:
          case DSPF_UYVY:
          case DSPF_YUY2:
          case DSPF_YV12:
          case DSPF_AYUV:
          case DSPF_ALUT88:
          case DSPF_ABGR:
          case DSPF_LUT4:
          case DSPF_LUT2:
          case DSPF_LUT1:
               break;

          default:
               D_BUG( "unknown pixel format" );
               return DFB_BUG;
     }

     direct_serial_init( &surface->serial );

     surface->width  = width;
     surface->height = height;
     surface->format = format;
     surface->caps   = caps;

     surface->alpha_ramp[0] = 0x00;
     surface->alpha_ramp[1] = 0x55;
     surface->alpha_ramp[2] = 0xaa;
     surface->alpha_ramp[3] = 0xff;

     if (caps & DSCAPS_STATIC_ALLOC) {
          surface->min_width  = width;
          surface->min_height = height;
     }

     if (palette) {
          dfb_surface_set_palette( surface, palette );
     }
     else if (DFB_PIXELFORMAT_IS_INDEXED( format )) {
          DFBResult    ret;
          CorePalette *palette;

          ret = dfb_palette_create( core,
                                    1 << DFB_COLOR_BITS_PER_PIXEL( format ),
                                    &palette );
          if (ret)
               return ret;

          if (format == DSPF_LUT8)
               dfb_palette_generate_rgb332_map( palette );
          else if (format == DSPF_ALUT44)
               dfb_palette_generate_rgb121_map( palette );

          dfb_surface_set_palette( surface, palette );

          dfb_palette_unref( palette );
     }

     surface->manager = manager ? manager : dfb_gfxcard_surface_manager();

     /*
      * This surface doesn't have a manager allocating from it's buffer.
      */
     surface->sub_manager = NULL;

     return DFB_OK;
}

DFBResult dfb_surface_dump( CoreSurface *surface,
                            const char  *directory,
                            const char  *prefix )
{
    return dfb_surface_dump_buffer(surface, surface->front_buffer, directory, prefix, 0);
}

DFBResult dfb_surface_dump_buffer
                          ( CoreSurface *surface,
                            SurfaceBuffer *buffer,
                            const char  *directory,
                            const char  *prefix,
                            DFBSurfaceLockFlags flags)
{
     DFBResult          ret;
     int                num  = -1;
     int                fd_p = -1;
     int                fd_g = -1;
     int                i, n;
     int                len = strlen(directory) + strlen(prefix) + 40;
     char               filename[len];
     char               head[30];
     void              *data;
     int                pitch;
     bool               rgb   = false;
     bool               alpha = false;
#ifdef USE_ZLIB
     gzFile             gz_p = NULL, gz_g = NULL;
     static const char *gz_ext = ".gz";
#else
     static const char *gz_ext = "";
#endif
     CorePalette       *palette = NULL;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_dump( %p, %s, %s )\n", surface, directory, prefix );

     D_ASSERT( surface != NULL );
     D_ASSERT( directory != NULL );
     D_ASSERT( prefix != NULL );

     /* Check pixel format. */
     switch (surface->format) {
          case DSPF_LUT8:
               palette = surface->palette;

               if (!palette) {
                    D_BUG( "no palette" );
                    return DFB_BUG;
               }

               if (dfb_palette_ref( palette ))
                    return DFB_FUSION;

               rgb = true;

               /* fall through */

          case DSPF_A8:
               alpha = true;
               break;

          case DSPF_ARGB:
          case DSPF_ARGB1555:
          case DSPF_ARGB2554:
          case DSPF_ARGB4444:
          case DSPF_AiRGB:
               alpha = true;

               /* fall through */

          case DSPF_RGB332:
          case DSPF_RGB16:
          case DSPF_RGB24:
          case DSPF_RGB32:
          case DSPF_YUY2:
          case DSPF_UYVY:
               rgb   = true;
               break;

          default:
               D_ERROR( "DirectFB/core/surfaces: surface dump for format "
                         "'%s' is not implemented!\n",
                        dfb_pixelformat_name( surface->format ) );
               return DFB_UNSUPPORTED;
     }

     /* Lock the surface, get the data pointer and pitch. */
     ret = dfb_surface_soft_lock( surface, buffer, DSLF_READ | flags, &data, &pitch );
     if (ret) {
          if (palette)
               dfb_palette_unref( palette );
          return ret;
     }

     /* Find the lowest unused index. */
     while (++num < 10000) {
          snprintf( filename, len, "%s/%s_%04d.ppm%s",
                    directory, prefix, num, gz_ext );

          if (access( filename, F_OK ) != 0) {
               snprintf( filename, len, "%s/%s_%04d.pgm%s",
                         directory, prefix, num, gz_ext );

               if (access( filename, F_OK ) != 0)
                    break;
          }
     }

     if (num == 10000) {
          D_ERROR( "DirectFB/core/surfaces: "
                   "couldn't find an unused index for surface dump!\n" );
          dfb_surface_unlock( surface, buffer, DSLF_READ | flags );
          if (palette)
               dfb_palette_unref( palette );
          return DFB_FAILURE;
     }

     /* Create a file with the found index. */
     if (rgb) {
          snprintf( filename, len, "%s/%s_%04d.ppm%s",
                    directory, prefix, num, gz_ext );

          fd_p = open( filename, O_EXCL | O_CREAT | O_WRONLY, 0644 );
          if (fd_p < 0) {
               D_PERROR("DirectFB/core/surfaces: "
                        "could not open %s!\n", filename);

               dfb_surface_unlock( surface, buffer, DSLF_READ | flags );
               if (palette)
                    dfb_palette_unref( palette );
               return DFB_IO;
          }
     }

     /* Create a graymap for the alpha channel using the found index. */
     if (alpha) {
          snprintf( filename, len, "%s/%s_%04d.pgm%s",
                    directory, prefix, num, gz_ext );

          fd_g = open( filename, O_EXCL | O_CREAT | O_WRONLY, 0644 );
          if (fd_g < 0) {
               D_PERROR("DirectFB/core/surfaces: "
                         "could not open %s!\n", filename);

               dfb_surface_unlock( surface, buffer, DSLF_READ | flags );
               if (palette)
                    dfb_palette_unref( palette );

               if (rgb) {
                    close( fd_p );
                    snprintf( filename, len, "%s/%s_%04d.ppm%s",
                              directory, prefix, num, gz_ext );
                    unlink( filename );
               }

               return DFB_IO;
          }
     }

#ifdef USE_ZLIB
     if (rgb)
          gz_p = gzdopen( fd_p, "wb" );

     if (alpha)
          gz_g = gzdopen( fd_g, "wb" );
#endif

     if (rgb) {
          /* Write the pixmap header. */
          snprintf( head, 30,
                    "P6\n%d %d\n255\n", surface->width, surface->height );
#ifdef USE_ZLIB
          gzwrite( gz_p, head, strlen(head) );
#else
          write( fd_p, head, strlen(head) );
#endif
     }

     /* Write the graymap header. */
     if (alpha) {
          snprintf( head, 30,
                    "P5\n%d %d\n255\n", surface->width, surface->height );
#ifdef USE_ZLIB
          gzwrite( gz_g, head, strlen(head) );
#else
          write( fd_g, head, strlen(head) );
#endif
     }

     /* Write the pixmap (and graymap) data. */
     for (i=0; i<surface->height; i++) {
          int    n3;
          __u8  *data8;
          __u16 *data16;
          __u32 *data32;

          __u8 buf_p[surface->width * 3];
          __u8 buf_g[surface->width];

          /* Prepare one row. */
          data8  = dfb_surface_data_offset( surface, data, pitch, 0, i );
          data16 = (__u16*) data8;
          data32 = (__u32*) data8;

          switch (surface->format) {
               case DSPF_LUT8:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = palette->entries[data8[n]].r;
                         buf_p[n3+1] = palette->entries[data8[n]].g;
                         buf_p[n3+2] = palette->entries[data8[n]].b;

                         buf_g[n] = palette->entries[data8[n]].a;
                    }
                    break;
               case DSPF_A8:
                    direct_memcpy( &buf_g[0], data8, surface->width );
                    break;
               case DSPF_AiRGB:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data32[n] & 0xFF0000) >> 16;
                         buf_p[n3+1] = (data32[n] & 0x00FF00) >>  8;
                         buf_p[n3+2] = (data32[n] & 0x0000FF);

                         buf_g[n] = ~(data32[n] >> 24);
                    }
                    break;
               case DSPF_ARGB:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data32[n] & 0xFF0000) >> 16;
                         buf_p[n3+1] = (data32[n] & 0x00FF00) >>  8;
                         buf_p[n3+2] = (data32[n] & 0x0000FF);

                         buf_g[n] = data32[n] >> 24;
                    }
                    break;
               case DSPF_ARGB1555:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data16[n] & 0x7C00) >> 7;
                         buf_p[n3+1] = (data16[n] & 0x03E0) >> 2;
                         buf_p[n3+2] = (data16[n] & 0x001F) << 3;

                         buf_g[n] = (data16[n] & 0x8000) ? 0xff : 0x00;
                    }
                    break;
               case DSPF_ARGB2554:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data16[n] & 0x3E00) >> 6;
                         buf_p[n3+1] = (data16[n] & 0x01F0) >> 1;
                         buf_p[n3+2] = (data16[n] & 0x000F) << 4;

                         switch (data16[n] >> 14) {
                              case 0:
                                   buf_g[n] = 0x00;
                                   break;
                              case 1:
                                   buf_g[n] = 0x55;
                                   break;
                              case 2:
                                   buf_g[n] = 0xAA;
                                   break;
                              case 3:
                                   buf_g[n] = 0xFF;
                                   break;
                         }
                    }
                    break;
               case DSPF_ARGB4444:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data16[n] & 0x0F00) >> 4;
                         buf_p[n3+1] = (data16[n] & 0x00F0);
                         buf_p[n3+2] = (data16[n] & 0x000F) << 4;

                         buf_g[n]  = (data16[n] >> 12);
                         buf_g[n] |= buf_g[n] << 4;
                    }
                    break;
               case DSPF_RGB332:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = lookup3to8[ (data8[n] >> 5)        ];
                         buf_p[n3+1] = lookup3to8[ (data8[n] >> 2) & 0x07 ];
                         buf_p[n3+2] = lookup2to8[ (data8[n]     ) & 0x03 ];
                    }
                    break;
               case DSPF_RGB16:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data16[n] & 0xF800) >> 8;
                         buf_p[n3+1] = (data16[n] & 0x07E0) >> 3;
                         buf_p[n3+2] = (data16[n] & 0x001F) << 3;
                    }
                    break;
               case DSPF_RGB24:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
#ifdef WORDS_BIGENDIAN
                         buf_p[n3+0] = data8[n3+0];
                         buf_p[n3+1] = data8[n3+1];
                         buf_p[n3+2] = data8[n3+2];
#else
                         buf_p[n3+0] = data8[n3+2];
                         buf_p[n3+1] = data8[n3+1];
                         buf_p[n3+2] = data8[n3+0];
#endif
                    }
                    break;
               case DSPF_RGB32:
                    for (n=0, n3=0; n<surface->width; n++, n3+=3) {
                         buf_p[n3+0] = (data32[n] & 0xFF0000) >> 16;
                         buf_p[n3+1] = (data32[n] & 0x00FF00) >>  8;
                         buf_p[n3+2] = (data32[n] & 0x0000FF);
                    }
                    break;
               case DSPF_YUY2:
                    for (n=0, n3=0; n<surface->width/2; n++, n3+=6) {
                         register __u32 y0, cb, y1, cr;
                         y0 = (data32[n] & 0x000000FF);
                         cb = (data32[n] & 0x0000FF00) >>  8;
                         y1 = (data32[n] & 0x00FF0000) >> 16;
                         cr = (data32[n] & 0xFF000000) >> 24;
                         YCBCR_TO_RGB( y0, cb, cr,
                                       buf_p[n3+0], buf_p[n3+1], buf_p[n3+2] );
                         YCBCR_TO_RGB( y1, cb, cr,
                                       buf_p[n3+3], buf_p[n3+4], buf_p[n3+5] );
                    }
                    break;
               case DSPF_UYVY:
                    for (n=0, n3=0; n<surface->width/2; n++, n3+=6) {
                         register __u32 y0, cb, y1, cr;
                         cb = (data32[n] & 0x000000FF);
                         y0 = (data32[n] & 0x0000FF00) >>  8;
                         cr = (data32[n] & 0x00FF0000) >> 16;
                         y1 = (data32[n] & 0xFF000000) >> 24;
                         YCBCR_TO_RGB( y0, cb, cr,
                                       buf_p[n3+0], buf_p[n3+1], buf_p[n3+2] );
                         YCBCR_TO_RGB( y1, cb, cr,
                                       buf_p[n3+3], buf_p[n3+4], buf_p[n3+5] );
                    }
                    break;
               default:
                    D_BUG( "unexpected pixelformat" );
                    break;
          }

          /* Write color buffer to pixmap file. */
          if (rgb)
#ifdef USE_ZLIB
               gzwrite( gz_p, buf_p, surface->width * 3 );
#else
               write( fd_p, buf_p, surface->width * 3 );
#endif

          /* Write alpha buffer to graymap file. */
          if (alpha)
#ifdef USE_ZLIB
               gzwrite( gz_g, buf_g, surface->width );
#else
               write( fd_g, buf_g, surface->width );
#endif
     }

     /* Unlock the surface. */
     dfb_surface_unlock( surface, buffer, DSLF_READ | flags );

     /* Release the palette. */
     if (palette)
          dfb_palette_unref( palette );

#ifdef USE_ZLIB
     if (rgb)
          gzclose( gz_p );

     if (alpha)
          gzclose( gz_g );
#endif

     /* Close pixmap file. */
     if (rgb)
          close( fd_p );

     /* Close graymap file. */
     if (alpha)
          close( fd_g );

     return DFB_OK;
}

/**************************************************************************************************/

#ifdef DEBUG_ALLOCATION
DFBResult dfb_surface_allocate_buffer_debug (
                                       const char * file,
                                       int line,
#else
DFBResult dfb_surface_allocate_buffer(
#endif
                                       CoreSurface            *surface,
                                       CoreSurfacePolicy       policy,
                                       DFBSurfacePixelFormat   format,
                                       SurfaceBuffer         **ret_buffer )
{
     SurfaceBuffer *buffer;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_allocate_buffer( %p, %d, %s )\n",
                 surface, policy, dfb_pixelformat_name( format ) );

     D_ASSERT( surface != NULL );
     D_ASSERT( ret_buffer != NULL );

     /* Allocate buffer structure. */
     buffer = SHCALLOC( 1, sizeof(SurfaceBuffer) );

     buffer->policy  = policy;
     buffer->surface = surface;
     buffer->format  = format;

     switch (policy) {
          case CSP_SYSTEMONLY:
          case CSP_VIDEOLOW:
          case CSP_VIDEOHIGH: {
               int   pitch;
               int   size;
               void *data;

               /* Calculate pitch. */
               pitch = DFB_BYTES_PER_LINE( buffer->format,
                                           MAX( surface->width, surface->min_width ) );

               /* Align pitch. */
               pitch = (pitch + 3) & ~3;

               /* Calculate amount of data to allocate. */
               size = DFB_PLANE_MULTIPLY( buffer->format,
                                          MAX( surface->height, surface->min_height ) * pitch );

               /* Allocate shared memory. */
               data = SHMALLOC( size );
               if (!data) {
                    SHFREE( buffer );
                    return DFB_NOSYSTEMMEMORY;
               }

               /* Write back values. */
               buffer->system.health = CSH_STORED;
               buffer->system.pitch  = pitch;
               buffer->system.addr   = data;

               break;
          }

          case CSP_VIDEOONLY:
          case CSP_VIDEOONLY_RESERVED:
          {
               DFBResult ret;

               /* Lock surface manager. */
               dfb_surfacemanager_lock( surface->manager );

               /* Allocate buffer in video memory. */
               ret = dfb_surfacemanager_allocate( surface->manager, buffer );
               if (ret == DFB_NOVIDEOMEMORY) {
                    D_WARN( "ran out of video memory" );
#ifdef DEBUG_ALLOCATION
                    dfb_surfacemanager_enumerate_chunks( surface->manager,
                    surfacemanager_chunk_callback,
                    NULL );
               }
               else {
                   D_ALLOCATION( "Core/Surface: policy %d allocated buffer 0x%08X from %s %d\n", buffer->policy, (unsigned int)buffer->video.offset, file, line);
#endif
               }

               /* Unlock surface manager. */
               dfb_surfacemanager_unlock( surface->manager );

               /* Check for successful allocation. */
               if (ret) {
                    SHFREE( buffer );
                    return ret;
               }

               /* Set from 'to be restored' to 'is stored'. */
               buffer->video.health = CSH_STORED;

               break;
          }
     }

     /* Return the new buffer. */
     *ret_buffer = buffer;

     return DFB_OK;
}

#ifdef DEBUG_ALLOCATION
DFBResult dfb_surface_reallocate_buffer_debug (
                                         const char * file,
                                         int line,
#else
DFBResult dfb_surface_reallocate_buffer(
#endif
                                         CoreSurface           *surface,
                                         DFBSurfacePixelFormat  format,
                                         SurfaceBuffer         *buffer )
{
     DFBResult    ret;

     D_HEAVYDEBUG( "Core/Surface: dfb_surface_reallocate_buffer( %p, %s, %p )\n",
                 surface, dfb_pixelformat_name( format ), buffer );

     if (buffer->flags & SBF_FOREIGN_SYSTEM)
          return DFB_UNSUPPORTED;

     if (buffer->system.health) {
          int   pitch;
          int   size;
          void *data;

          /* Calculate pitch. */
          pitch = DFB_BYTES_PER_LINE( format, MAX( surface->width, surface->min_width ) );
          if (pitch & 3)
               pitch += 4 - (pitch & 3);

          /* Calculate amount of data to allocate. */
          size = DFB_PLANE_MULTIPLY( format, MAX( surface->height, surface->min_height ) * pitch );

          /* Allocate shared memory. */
          data = SHMALLOC( size );
          if (!data)
               return DFB_NOSYSTEMMEMORY;

          /* Free old memory. */
          SHFREE( buffer->system.addr );

          /* Write back new values. */
          buffer->system.health = CSH_STORED;
          buffer->system.pitch  = pitch;
          buffer->system.addr   = data;
     }

     buffer->format = format;

     if (buffer->video.health) {

          /* FIXME: better support video instance reallocation */
          dfb_surfacemanager_deallocate( surface->manager, buffer );

          ret = dfb_surfacemanager_allocate( surface->manager, buffer );
          if (ret == DFB_NOVIDEOMEMORY) {
               D_WARN( "ran out of video memory" );
#ifdef DEBUG_ALLOCATION
               dfb_surfacemanager_enumerate_chunks( surface->manager,
               surfacemanager_chunk_callback,
               NULL );
          }
          else {
               D_ALLOCATION( "Core/Surface: policy %d allocated buffer 0x%08X from %s %d\n", buffer->policy, (unsigned int)buffer->video.offset, file, line);
#endif
          }

          if (ret) {
               if (!buffer->system.health)
                    D_WARN( "reallocation of video instance failed" );
               else {
                    buffer->system.health = CSH_STORED;
                    return DFB_OK;
               }

               return ret;
          }

          buffer->video.health = CSH_STORED;
     }

     return DFB_OK;
}

#ifdef DEBUG_ALLOCATION
void dfb_surface_destroy_buffer_debug (
                                 const char * file,
                                 int line,
#else
void dfb_surface_destroy_buffer(
#endif
                                 CoreSurface   *surface,
                                 SurfaceBuffer *buffer )
{
     D_HEAVYDEBUG( "Core/Surface: dfb_surface_destroy_buffer( %p, %p )\n", surface, buffer );

     D_ASSERT( surface != NULL );
     D_ASSERT( buffer != NULL );

     dfb_surfacemanager_lock( surface->manager );

     if (buffer->system.health && !(buffer->flags & SBF_FOREIGN_SYSTEM)) {
          SHFREE( buffer->system.addr );

          buffer->system.addr   = NULL;
          buffer->system.health = CSH_INVALID;
     }

     if (buffer->video.health && !(buffer->flags & SBF_FOREIGN_VIDEO)) {
          D_ALLOCATION( "Core/Surface: policy %d deallocating buffer 0x%08X from %s %d\n", buffer->policy, (unsigned int)buffer->video.offset, file, line);
          dfb_surfacemanager_deallocate( surface->manager, buffer );
     }

     dfb_surfacemanager_unlock( surface->manager );

     SHFREE( buffer );
}

ReactionResult
_dfb_surface_palette_listener( const void *msg_data,
                               void       *ctx )
{
     const CorePaletteNotification *notification = msg_data;
     CoreSurface                   *surface      = ctx;

     D_HEAVYDEBUG( "Core/Surface: _dfb_surface_palette_listener( %p, %p )\n", msg_data, ctx );

     if (notification->flags & CPNF_DESTROY)
          return RS_REMOVE;

     if (notification->flags & CPNF_ENTRIES)
          dfb_surface_notify_listeners( surface, CSNF_PALETTE_UPDATE );

     return RS_OK;
}

/* internal functions needed to avoid side effects */

void dfb_surface_video_access_by_hardware( SurfaceBuffer       *buffer,
                                           DFBSurfaceLockFlags  flags )
{
     if (buffer->video.access & VAF_SOFTWARE_WRITE) {
          dfb_gfxcard_flush_texture_cache(buffer);
          buffer->video.access &= ~VAF_SOFTWARE_WRITE;
     }

     if (flags & DSLF_READ)
          buffer->video.access |= VAF_HARDWARE_READ;

     if (flags & DSLF_WRITE)
          buffer->video.access |= VAF_HARDWARE_WRITE;
}

void dfb_surface_video_access_by_software( SurfaceBuffer       *buffer,
                                           DFBSurfaceLockFlags  flags )
{
     if (flags & DSLF_WRITE) {
          if (buffer->video.access & VAF_HARDWARE_READ) {
               dfb_gfxcard_sync();
               buffer->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
          }
     }

     if (buffer->video.access & VAF_HARDWARE_WRITE) {
          dfb_gfxcard_wait_serial( &buffer->video.serial );
          buffer->video.access &= ~VAF_HARDWARE_WRITE;
     }

     if (flags & DSLF_READ)
          buffer->video.access |= VAF_SOFTWARE_READ;

     if (flags & DSLF_WRITE)
          buffer->video.access |= VAF_SOFTWARE_WRITE;
}

