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


#include <pthread.h>

#include <direct/list.h>
#include <fusion/shmalloc.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/surfacemanager_internal.h>
#include <core/surfacemanager_sequential.h>
#include <core/system.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>


D_DEBUG_DOMAIN( Core_SM, "Core/SurfaceMgrSequential", "DirectFB Surface Manager" );

/*
 * initially there is one big free chunk,
 * chunks are splitted into a free and an occupied chunk if memory is allocated,
 * two chunks are merged to one free chunk if memory is deallocated
 */

SurfaceManager *
dfb_surfacemanager_sequential_create( unsigned int     offset,
                                    unsigned int     length,
                                    unsigned int     minimum_page_size,
                                    CardLimitations *limits )
{
     Chunk          *chunk;
     SurfaceManager *manager;

#ifdef DEBUG_ALLOCATION
     dfbbcm_show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

     manager = SHCALLOC( 1, sizeof(SurfaceManager) );
     if (!manager)
          return NULL;

     manager->type = DSMT_SURFACEMANAGER_SEQUENTIAL;

     manager->heap_offset      = offset;
     manager->chunks           = NULL;
     manager->length           = length;
     manager->available        = length;
     manager->byteoffset_align = limits->surface_byteoffset_alignment;
     manager->pixelpitch_align = limits->surface_pixelpitch_alignment;
     manager->bytepitch_align  = limits->surface_bytepitch_alignment;
     manager->max_power_of_two_pixelpitch = limits->surface_max_power_of_two_pixelpitch;
     manager->max_power_of_two_bytepitch  = limits->surface_max_power_of_two_bytepitch;
     manager->max_power_of_two_height     = limits->surface_max_power_of_two_height;
     manager->next_free_obj = 0;
     manager->cum_offset    = offset;

     {
         unsigned int obj_index;

         for (obj_index = 0; obj_index < MAX_NUM_OBJ; obj_index++)
         {
             manager->obj[obj_index].is_used = false;
         }
     }

     D_DEBUG("\n\nSequential Memory %d Lenght %d\n\n", offset, length);

#ifdef DEBUG_ALLOCATION
     D_INFO("\n\nSequential Memory %d Lenght %d\n\n", offset, length);
#endif

     fusion_skirmish_init( &manager->lock, "Surface Manager" );

     D_MAGIC_SET( manager, SurfaceManager );

     return manager;
}

void
dfb_surfacemanager_sequential_destroy( SurfaceManager *manager )
{
     Chunk *chunk;

     D_ASSERT( manager != NULL );
     D_ASSERT( manager->chunks != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     D_MAGIC_CLEAR( manager );

     /* Destroy manager lock. */
     fusion_skirmish_destroy( &manager->lock );

     /* Deallocate manager struct. */
     SHFREE( manager );
}

DFBResult dfb_surfacemanager_sequential_suspend( SurfaceManager *manager )
{
     Chunk *c;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     manager->suspended = true;

#if 0
     c = &manager->obj[0];
     while (c) {
          if (c->buffer &&
              c->buffer->policy != CSP_VIDEOONLY &&
              c->buffer->video.health == CSH_STORED)
          {
               dfb_surfacemanager_assure_system( manager, c->buffer );
               c->buffer->video.health = CSH_RESTORE;
          }

          c = c->next;
     }

#endif

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

DFBResult dfb_surfacemanager_sequential_adjust_heap_offset( SurfaceManager *manager,
                                                          int             offset )
{
     D_ASSERT( offset >= 0 );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

#if 0

     if (manager->byteoffset_align > 1) {
          offset += manager->byteoffset_align - 1;
          offset -= offset % manager->byteoffset_align;
     }

     if (manager->chunks->buffer == NULL) {
          /* first chunk is free */
          if (offset <= manager->chunks->offset + manager->chunks->length) {
               /* ok, just recalculate offset and length */
               manager->chunks->length = manager->chunks->offset + manager->chunks->length - offset;
               manager->chunks->offset = offset;
          }
          else {
               D_WARN("unable to adjust heap offset");
               /* more space needed than free at the beginning */
               /* TODO: move/destroy instances */
          }
     }
     else {
          D_WARN("unable to adjust heap offset");
          /* very rare case that the first chunk is occupied */
          /* TODO: move/destroy instances */
     }

     manager->heap_offset = offset;
#endif

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

static void
test_chunks( SurfaceManager  *manager)
{
     Chunk *c;
     Chunk *p;
     unsigned int cum_length = 0;
     bool test_error = false;

     D_ASSERT( manager != NULL );


     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

#if 0

     c = manager->chunks;

     if (c)
     {
          if (c->next)
          {
                p = c;
                c = c->next;

                while (c)
                {
                    if ((p->offset + p->length) != c->offset)
                    {
                        D_INFO("Sequential allocation error between addr 0x%x size %d and addr 0x%x size %d\n",
                        p->offset,
                        p->length,
                        c->offset,
                        c->length);
                        test_error = true;
                        break;
                    }
                    else
                    {
                        cum_length += p->length;

                    }

                    p = p->next;
                    c = c->next;
                }
                cum_length += p->length;
          }
     }

     if (cum_length != manager->length)
     {
          D_INFO("Cumulative chunk size %d does not match heap size %d\n", cum_length, manager->length);
     }

#endif

     dfb_surfacemanager_unlock( manager );
};

void
dfb_surfacemanager_sequential_enumerate_chunks( SurfaceManager  *manager,
                                              SMChunkCallback  callback,
                                              void            *ctx )
{
     Chunk *c;
     unsigned int obj_pos = 0;

     D_ASSERT( manager != NULL );
     D_ASSERT( callback != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     for (obj_pos = 0; obj_pos < manager->next_free_obj; obj_pos++)
     {
          if (callback( manager->obj[obj_pos].buffer, manager->obj[obj_pos].offset, manager->obj[obj_pos].length, 0, ctx) == DFENUM_CANCEL)
               break;

          obj_pos++;
     }

     test_chunks(manager);

     dfb_surfacemanager_unlock( manager );
}




void
dfb_surfacemanager_sequential_retrive_surface_info(
    SurfaceManager *manager,
    SurfaceBuffer  *surface,
    int            *offset,
    int            *length,
    int            *tolerations
)
{
     Chunk *c;

     D_ASSERT( manager != NULL );


     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     *offset      = manager->obj[surface->obj_index].offset;
     *length      = manager->obj[surface->obj_index].length;
     *tolerations = 0;

     dfb_surfacemanager_unlock( manager );
}


static int align(int num, int alignment)
{
  int align = num & (alignment - 1);
  if (align) num += alignment - align;

  return num;
}

/** public functions NOT locking the surfacemanger theirself,
    to be called between lock/unlock of surfacemanager **/

DFBResult dfb_surfacemanager_sequential_allocate( SurfaceManager *manager,
                                                SurfaceBuffer  *buffer )
{
     int pitch;
     int length;
     int width;
     int height;
     Chunk *c;

     Chunk *best_free = NULL;
     Chunk *best_occupied = NULL;

     CoreSurface *surface = buffer->surface;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (!manager->length || manager->suspended)
          return DFB_NOVIDEOMEMORY;

     width = surface->width;
     height = surface->height;

     /* Align width and height before malloc */
     if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_4)
          width = (width + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_8)
          width = (width + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_16)
          width = (width + 15) & ~15;

     if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_4)
          height = (height + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_8)
          height = (height + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_16)
          height = (height + 15) & ~15;

     /* calculate the required length depending on limitations */
     pitch = MAX( width, surface->min_width );

     if (pitch < manager->max_power_of_two_pixelpitch &&
         height < manager->max_power_of_two_height)
          pitch = 1 << direct_log2( pitch );

     if (manager->pixelpitch_align > 1) {
          pitch += manager->pixelpitch_align - 1;
          pitch -= pitch % manager->pixelpitch_align;
     }

     pitch = DFB_BYTES_PER_LINE( buffer->format, pitch );

     if (pitch < manager->max_power_of_two_bytepitch &&
         height < manager->max_power_of_two_height)
          pitch = 1 << direct_log2( pitch );

     if (manager->bytepitch_align > 1) {
          pitch += manager->bytepitch_align - 1;
          pitch -= pitch % manager->bytepitch_align;
     }

     length = DFB_PLANE_MULTIPLY( buffer->format,
                                  MAX( height, surface->min_height ) * pitch );

     if (manager->byteoffset_align > 1) {
          length += manager->byteoffset_align - 1;
          length -= length % manager->byteoffset_align;
     }

     /* Do a pre check before iterating through all chunks. */
     if (length > manager->available/* - manager->heap_offset*/)
          return DFB_NOVIDEOMEMORY;

     buffer->video.pitch = pitch;

     /* round up buffer lenght */
     length = (length + 3) & ~3;

     /* register buffer */
     buffer->type      = manager->type;
     buffer->obj_index = manager->next_free_obj;
     manager->obj[manager->next_free_obj].length = length;
     manager->obj[manager->next_free_obj].offset = manager->cum_offset;
     manager->obj[manager->next_free_obj].buffer = buffer;
     manager->obj[manager->next_free_obj].is_used = true;
     buffer->video.health = CSH_RESTORE;
     buffer->video.offset = manager->obj[manager->next_free_obj].offset;
     buffer->video.chunk = NULL;

     /* create a lower level surface pointing to this area of memory */
     if (manager->funcs->AllocateSurfaceBuffer) {
          if (manager->funcs->AllocateSurfaceBuffer(manager, buffer) != DFB_OK) {
                return DFB_NOVIDEOMEMORY;
          }
     }
/*
     D_INFO("Alloc[%d][%d, %d, %d]\n",
     manager->next_free_obj,
     manager->cum_offset,
     length,
     manager->cum_offset - length);
*/
#if D_DEBUG_ENABLED
          D_DEBUG_AT( Core_SM,
#else
          D_ALLOCATION( "Core/SurfaceMgrSequential -> "
#endif
          "occupy_chunk: allocating %d bytes at offset 0x%08X.\n", length, manager->cum_offset );

     /* get ready for next object */
     manager->cum_offset += length;
     manager->available  -= length;
     manager->next_free_obj++;

    return DFB_OK;
}

DFBResult dfb_surfacemanager_sequential_deallocate( SurfaceManager *manager,
                                                  SurfaceBuffer  *buffer )
{
     int    loops = 0;
     Chunk *chunk = buffer->video.chunk;

     D_ASSERT( buffer->surface );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->funcs->DeallocateSurfaceBuffer) {
         /* Should check DFBResult */
         manager->funcs->DeallocateSurfaceBuffer(manager, buffer);
     }

     if (buffer->video.health == CSH_INVALID)
          return DFB_OK;

     if (buffer->video.access & VAF_SOFTWARE_WRITE) {
          dfb_gfxcard_flush_texture_cache(buffer);
          buffer->video.access &= ~VAF_SOFTWARE_WRITE;
     }

     if (buffer->video.access & VAF_HARDWARE_READ) {
          dfb_gfxcard_sync();
          buffer->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
     }

     if (buffer->video.access & VAF_HARDWARE_WRITE) {
          dfb_gfxcard_wait_serial( &buffer->video.serial );
          buffer->video.access &= ~VAF_HARDWARE_WRITE;
     }

     /* un-register element */
     manager->available += manager->obj[buffer->obj_index].length;
     manager->cum_offset -= manager->obj[buffer->obj_index].length;
     manager->obj[buffer->obj_index].length = 0;
     manager->obj[buffer->obj_index].offset = 0;
     manager->obj[buffer->obj_index].buffer = NULL;
     manager->obj[buffer->obj_index].is_used = false;
     buffer->video.chunk = NULL;
     buffer->video.health = CSH_INVALID;
     buffer->video.chunk = NULL;
     buffer->obj_index = -1;

     dfb_surface_notify_listeners( buffer->surface, CSNF_VIDEO );

     while (buffer->video.locked) {
          if (++loops > 1000)
               break;

          sched_yield();
     }

     if (buffer->video.locked)
          D_WARN( "Freeing chunk with a non-zero lock counter" );

     manager->next_free_obj--;

/*
     D_INFO("Free[%d] @ %d (left %d)",
     buffer->obj_index,
     manager->cum_offset,
     manager->next_free_obj);
*/
     return DFB_OK;
}

/*
 * wipe out all the surfaces that have been allocated so far.
 */
DFBResult dfb_surfacemanager_reset( SurfaceManager *manager)
{
    /* call dfb_surfacemanager_sequential_deallocate on all the surfaces allocated so far */
}

DFBResult dfb_surfacemanager_sequential_assure_video( SurfaceManager *manager,
                                                    SurfaceBuffer  *buffer )
{
     DFBResult    ret;
     CoreSurface *surface = buffer->surface;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->suspended)
          return DFB_NOVIDEOMEMORY;

     switch (buffer->video.health) {
          case CSH_STORED:
               return DFB_OK;

          case CSH_INVALID:

               ret = dfb_surfacemanager_sequential_allocate( manager, buffer );
               if (ret == DFB_NOVIDEOMEMORY) {
                    D_WARN( "ran out of video memory" );
#ifdef DEBUG_ALLOCATION
                    dfb_surfacemanager_enumerate_chunks( manager,
                    surfacemanager_chunk_callback,
                    NULL );
#endif
               }
               if (ret)
                    return ret;

               /* FALL THROUGH, because after successful allocation
                  the surface health is CSH_RESTORE */

          case CSH_RESTORE:
               if (buffer->system.health != CSH_STORED)
                    D_BUG( "system/video instances both not stored!" );

               if (buffer->flags & SBF_WRITTEN) {
                    int   i;
                    char *src = buffer->system.addr;
                    char *dst = dfb_system_video_memory_virtual( buffer->video.offset );

                    dfb_surface_video_access_by_software( buffer, DSLF_WRITE );

                    for (i=0; i<surface->height; i++) {
                         direct_memcpy( dst, src,
                                        DFB_BYTES_PER_LINE(buffer->format, surface->width) );
                         src += buffer->system.pitch;
                         dst += buffer->video.pitch;
                    }

                    if (buffer->format == DSPF_YV12 || buffer->format == DSPF_I420) {
                         for (i=0; i<surface->height; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width / 2) );
                              src += buffer->system.pitch / 2;
                              dst += buffer->video.pitch  / 2;
                         }
                    }
                    else if (buffer->format == DSPF_NV12 || buffer->format == DSPF_NV21) {
                         for (i=0; i<surface->height/2; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width) );
                              src += buffer->system.pitch;
                              dst += buffer->video.pitch;
                         }
                    }
                    else if (buffer->format == DSPF_NV16) {
                         for (i=0; i<surface->height; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width) );
                              src += buffer->system.pitch;
                              dst += buffer->video.pitch;
                         }
                    }
               }

               buffer->video.health             = CSH_STORED;

               dfb_surface_notify_listeners( surface, CSNF_VIDEO );

               return DFB_OK;

          default:
               break;
     }

     return DFB_BUG;
}

