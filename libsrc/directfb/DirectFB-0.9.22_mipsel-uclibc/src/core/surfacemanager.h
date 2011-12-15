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

#ifndef __SURFACEMANAGER_H__
#define __SURFACEMANAGER_H__

#include <fusion/lock.h>

#include <directfb.h>
#include <core/core.h>

#include <core/gfxcard.h>

/*
 * These core flags must be kept consistent with the DFBSurfaceLockFlags enum 
 * as they are used together. 
 */
#define CSLF_FORCE              0x80000000

typedef struct {
    unsigned int offset;
    unsigned int length;
    unsigned int minimum_page_size;
} dfb_bcm_partition_info;

DFBEnumerationResult
surfacemanager_chunk_callback(
                           SurfaceBuffer *buffer,
                           int            offset,
                           int            length,
                           int            tolerations,
                           void          *ctx );

SurfaceManager *dfb_surfacemanager_create(
                           unsigned int            offset,
                           unsigned int            length,
                           unsigned int            minimum_page_size,
                           CardLimitations        *limits,
                           DFBSurfaceManagerTypes  type );

void dfb_surfacemanager_destroy( SurfaceManager *manager );

DFBResult dfb_surfacemanager_suspend( SurfaceManager *manager );
DFBResult dfb_surfacemanager_resume( SurfaceManager *manager );

/*
 * adjust the offset within the framebuffer for surface storage,
 * needs to be called after a resolution switch
 */
DFBResult dfb_surfacemanager_adjust_heap_offset( SurfaceManager *manager,
                                                 int             offset );

typedef DFBEnumerationResult (*SMChunkCallback)( SurfaceBuffer *buffer,
                                                 int            offset,
                                                 int            length,
                                                 int            tolerations,
                                                 void          *ctx );

void dfb_surfacemanager_enumerate_chunks( SurfaceManager  *manager,
                                          SMChunkCallback  callback,
                                          void            *ctx );

void dfb_surfacemanager_retrive_surface_info( SurfaceManager  *manager,
                                              SurfaceBuffer   *surface,
                                              int             *offset,
                                              int             *length,
                                              int             *tolerations);

/*
 * Lock/unlock the surfacemanager for usage of the functions below.
 */
void dfb_surfacemanager_lock( SurfaceManager *manager );
void dfb_surfacemanager_unlock( SurfaceManager *manager );

/*
 * lock a surface for access by software, returns a pointer to the
 * surface data and the line pitch a.k.a. rowstride
 */
DFBResult dfb_surface_software_lock( CoreSurface *surface, SurfaceBuffer *buffer, 
                                     unsigned int flags, void **data, int *pitch );

/*
 * lock a surface for access by hardware that enforces a video instance
 * an therefore the data and pitch can be looked up in the surface struct's
 * video struct, however this function will fail if the surfacemanager could
 * not assure a video memory instance
 */
DFBResult dfb_surface_hardware_lock( CoreSurface *surface,
                                     SurfaceBuffer *buffer, 
                                     unsigned int flags );


/*
 * finds and allocates one for the surface or fails,
 * after success the video health is CSH_RESTORE.
 * NOTE: this does not notify the listeners
 */
DFBResult dfb_surfacemanager_allocate( SurfaceManager *manager,
                                       SurfaceBuffer  *buffer );

/*
 * sets the video health to CSH_INVALID frees the chunk and
 * notifies the listeners
 */
DFBResult dfb_surfacemanager_deallocate( SurfaceManager *manager,
                                         SurfaceBuffer  *buffer );

/*
 * puts the surface into the video memory,
 * i.e. it initializees the video instance or fails
 */
DFBResult dfb_surfacemanager_assure_video( SurfaceManager *manager,
                                           SurfaceBuffer  *buffer );

/*
 * makes sure the system instance is not outdated,
 * it fails if the policy is CSP_VIDEOONLY
 */
DFBResult dfb_surfacemanager_assure_system( SurfaceManager *manager,
                                            SurfaceBuffer  *buffer );

SurfaceManager *dfb_surfacemanager_create_mp(
                           dfb_bcm_partition_info  partitions_list[],
                           unsigned char partitions_number,
                           CardLimitations *limits);

DFBResult dfb_surfacemanager_defrag(
     SurfaceManager *manager);

typedef struct {
    DFBResult (*AllocateSurfaceBuffer)
                               ( SurfaceManager             *manager,
                                 SurfaceBuffer              *buffer );

    DFBResult (*DeallocateSurfaceBuffer)
                               ( SurfaceManager             *manager,
                                 SurfaceBuffer              *buffer );

    DFBResult (*CopySurfaceBuffer)
                               ( SurfaceBuffer              *buffer,
                                 int                        offset );

    DFBResult (*UpdateSurfaceBuffer)
                               ( SurfaceBuffer              *buffer );

    DFBResult (*AllocatePaletteBuffer)
                               ( SurfaceManager             *manager,
                                 CorePalette                *palette );

    DFBResult (*DeallocatePaletteBuffer)
                               ( SurfaceManager             *manager,
                                 CorePalette                *palette );

} SurfaceManagerFuncs;

#endif
