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
#include <core/surfacemanager_firstfit.h>
#include <core/system.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>


D_DEBUG_DOMAIN( Core_SM, "Core/SurfaceMgrFirstFit", "DirectFB Surface Manager" );

#define DEBUG_DEFRAG 0
#if DEBUG_DEFRAG
#define D_DEFRAG(x)    printf("%s: ", __FUNCTION__); printf x
#else
#define D_DEFRAG(x)
#endif

/*
 * initially there is one big free chunk,
 * chunks are splitted into a free and an occupied chunk if memory is allocated,
 * two chunks are merged to one free chunk if memory is deallocated
 */
struct _Chunk {
     int             magic;

     int             offset;      /* offset in video memory,
                                     is greater or equal to the heap offset */
     int             length;      /* length of this chunk in bytes */

     SurfaceBuffer  *buffer;      /* pointer to surface buffer occupying
                                     this chunk, or NULL if chunk is free */

     Chunk          *prev;
     Chunk          *next;
};

static Chunk* split_chunk( Chunk *c, int length );
static Chunk* free_chunk( SurfaceManager *manager, Chunk *chunk );
static void occupy_chunk( SurfaceManager *manager, Chunk *chunk, SurfaceBuffer *buffer, int length );
static Chunk* join_chunks( Chunk *c1, Chunk *c2 );
static bool is_chunk_locked(Chunk *c);
static DFBResult lock_chunk_surfaces(Chunk *c);
static void unlock_chunk_surfaces(Chunk *c);

SurfaceManager *
dfb_surfacemanager_firstfit_create( unsigned int     offset,
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

     manager->type = DSMT_SURFACEMANAGER_FIRSTFIT;

     chunk = SHCALLOC( 1, sizeof(Chunk) );
     if (!chunk) {
          SHFREE( manager );
          return NULL;
     }

     chunk->offset = offset;
     chunk->length = length;

     manager->partition[dfb_system_get_partition_type( CSP_VIDEOONLY )].heap_offset = offset;
     manager->partition[dfb_system_get_partition_type( CSP_VIDEOONLY )].chunks      = chunk;
     manager->partition[dfb_system_get_partition_type( CSP_VIDEOONLY )].length      = length;
     manager->partition[dfb_system_get_partition_type( CSP_VIDEOONLY )].available   = length;
     manager->partitions_number = 1;
     manager->byteoffset_align = limits->surface_byteoffset_alignment;
     manager->pixelpitch_align = limits->surface_pixelpitch_alignment;
     manager->bytepitch_align  = limits->surface_bytepitch_alignment;
     manager->max_power_of_two_pixelpitch = limits->surface_max_power_of_two_pixelpitch;
     manager->max_power_of_two_bytepitch  = limits->surface_max_power_of_two_bytepitch;
     manager->max_power_of_two_height     = limits->surface_max_power_of_two_height;

     fusion_skirmish_init( &manager->lock, "Surface Manager" );

     D_MAGIC_SET( chunk, _Chunk_ );

     D_MAGIC_SET( manager, SurfaceManager );

     return manager;
}

void
dfb_surfacemanager_firstfit_destroy( SurfaceManager *manager )
{
     Chunk *chunk;
     unsigned char enum_partition;

     D_ASSERT( manager != NULL );


     D_MAGIC_ASSERT( manager, SurfaceManager );

     D_MAGIC_CLEAR( manager );

     for (enum_partition = 0; enum_partition < manager->partitions_number; enum_partition++)
     {
          /* Deallocate all chunks. */
         D_ASSERT( manager->partition[enum_partition].chunks != NULL );

         chunk = manager->partition[enum_partition].chunks;
         while (chunk) {
              Chunk *next = chunk->next;

              D_MAGIC_CLEAR( chunk );

              SHFREE( chunk );

              chunk = next;
         }
     }

     /* Destroy manager lock. */
     fusion_skirmish_destroy( &manager->lock );

     /* Deallocate manager struct. */
     SHFREE( manager );
}

DFBResult dfb_surfacemanager_firstfit_suspend( SurfaceManager *manager )
{
     Chunk *c;
     unsigned char enum_partition;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     manager->suspended = true;

     for (enum_partition = 0; enum_partition < manager->partitions_number; enum_partition++)
     {
         c = manager->partition[enum_partition].chunks;
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
    }

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

void
test_chunks( SurfaceManager  *manager)
{
     Chunk *c;
     Chunk *p;
     unsigned int cum_length = 0;
     bool test_error = false;

     D_ASSERT( manager != NULL );


     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

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
                        D_INFO("FirstFit allocation error between addr 0x%x size %d and addr 0x%x size %d\n",
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

     dfb_surfacemanager_unlock( manager );
};

#if 0
void
dfb_surfacemanager_firstfit_enumerate_chunks( SurfaceManager  *manager,
                                              SMChunkCallback  callback,
                                              void            *ctx )
{
     Chunk *c;

     D_ASSERT( manager != NULL );
     D_ASSERT( callback != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     c = manager->chunks;
     while (c) {

          if (callback( c->buffer, c->offset,
                        c->length, 0, ctx) == DFENUM_CANCEL)
               break;

          c = c->next;
     }

     test_chunks(manager);

     dfb_surfacemanager_unlock( manager );
}
#else
void
dfb_surfacemanager_firstfit_enumerate_chunks( SurfaceManager  *manager,
                                              SMChunkCallback  callback,
                                              void            *ctx )
{
     Chunk *c;

     D_ASSERT( manager != NULL );
     D_ASSERT( callback != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

	 D_INFO("\n*** SHARED PARTITION ***\n");
     c = manager->partition[dfb_system_get_partition_type( CSP_VIDEOONLY )].chunks;
     while (c) {
          if (callback( c->buffer, c->offset,
                        c->length, 0, ctx) == DFENUM_CANCEL)
               break;

          c = c->next;
     }

     D_INFO("\n*** RESERVED PARTITION ***\n");
     c = manager->partition[dfb_system_get_partition_type( CSP_VIDEOONLY_RESERVED )].chunks;
     while (c) {
          if (callback( c->buffer, c->offset,
                        c->length, 0, ctx) == DFENUM_CANCEL)
               break;

          c = c->next;
     }
     D_INFO("\n");

     dfb_surfacemanager_unlock( manager );
}

#endif

void
dfb_surfacemanager_firstfit_retrive_surface_info(
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

     c = manager->chunks;
     while (c) {
         /* look for the surface */
         if( c->buffer == surface)
         {
             *offset      = c->offset;
             *length      = c->length;
             *tolerations = 0;
             break;
         }

         c = c->next;
     }

     dfb_surfacemanager_unlock( manager );
}

int align(int num, int alignment)
{
  int align = num & (alignment - 1);
  if (align) num += alignment - align;

  return num;
}

/** public functions NOT locking the surfacemanger theirself,
    to be called between lock/unlock of surfacemanager **/

DFBResult dfb_surfacemanager_firstfit_allocate( SurfaceManager *manager,
                                                SurfaceBuffer  *buffer )
{
     int pitch;
     int length;
     int width;
     int height;
     Chunk *c;
     Chunk *best_free = NULL;
     CoreSurface *surface = buffer->surface;
     unsigned int partition_type = dfb_system_get_partition_type(buffer->policy);

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (!manager->partition[partition_type].length || manager->suspended)
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
     if (length > manager->partition[partition_type].available/* - manager->partition[partition_type].heap_offset*/)
     {
		  D_INFO("Lenght is %d available is %d (%dx%d, %d)\n", length, manager->partition[partition_type].available, width, height, pitch);
          return DFB_NOVIDEOMEMORY;
	 }

     buffer->video.pitch = pitch;

     /* examine chunks */
     c = manager->partition[partition_type].chunks;
     while (c) {
          if (c->length >= length) {
               if (!c->buffer) {
                    /* found a nice place to chill */
                    if (!best_free  ||  best_free->length > c->length)
                         /* first found or better one? */
                         best_free = c;
               }
          }
          c = c->next;
     }

     /* if we found a place */
     if (best_free) {
          occupy_chunk( manager, best_free, buffer, length );
          if (manager->funcs->AllocateSurfaceBuffer) {
               if (manager->funcs->AllocateSurfaceBuffer(manager, buffer) != DFB_OK) {
                    dfb_surfacemanager_firstfit_deallocate( manager, buffer );
                    return DFB_NOVIDEOMEMORY;
               }
          }
          return DFB_OK;
     }

     D_DEBUG_AT( Core_SM, "Couldn't allocate enough heap space for video memory surface!\n" );

     /* no luck */
     return DFB_NOVIDEOMEMORY;
}

DFBResult dfb_surfacemanager_firstfit_deallocate( SurfaceManager *manager,
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

     buffer->video.health = CSH_INVALID;
     buffer->video.chunk = NULL;

     dfb_surface_notify_listeners( buffer->surface, CSNF_VIDEO );

     while (buffer->video.locked) {
          if (++loops > 1000)
               break;

          sched_yield();
     }

     if (buffer->video.locked)
          D_WARN( "Freeing chunk with a non-zero lock counter" );

     if (chunk)
          free_chunk( manager, chunk );

     return DFB_OK;
}

DFBResult dfb_surfacemanager_firstfit_assure_video( SurfaceManager *manager,
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
               ret = dfb_surfacemanager_firstfit_allocate( manager, buffer );
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
                    char *dst = dfb_system_video_memory_virtual( buffer->video.offset);

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

     D_BUG( "unknown video instance health" );
     return DFB_BUG;
}


/** internal functions NOT locking the surfacemanager **/

static bool is_chunk_locked(Chunk *c)
{
     Chunk *c_sub;
     int unreserved_partition;
     SurfaceManager *manager;

     D_MAGIC_ASSERT( c, _Chunk_ );
     D_ASSERT(c != NULL);
     D_ASSERT(c->buffer != NULL);
     D_ASSERT(c->buffer->surface != NULL);

     if (c->buffer->video.locked != 0)
     {
          return true;
     }

     if (c->buffer->surface->sub_manager != NULL)
     {
          if (c->buffer->surface->sub_manager->type != DSMT_SURFACEMANAGER_FIRSTFIT)
          {
               return true;
          }
          manager = c->buffer->surface->sub_manager;
          dfb_surfacemanager_lock(manager);
          unreserved_partition = dfb_system_get_partition_type(CSP_VIDEOONLY);
          c_sub = manager->partition[unreserved_partition].chunks;
          while (c_sub != NULL)
          {
               if (c_sub->buffer != NULL &&
                   c_sub->buffer->video.locked != 0)
               {
                    break;
               }
               c_sub = c_sub->next;
          }
          dfb_surfacemanager_unlock(manager);
          if (c_sub != NULL)
          {
               return true;
          }
     }

     return false;
}

static DFBResult lock_chunk_surfaces(Chunk *c)
{
     DFBResult ret = DFB_OK;
     Chunk *c_sub;
     int unreserved_partition;
     SurfaceManager *manager;
     DFBSurfaceLockFlags lock_flags;
     bool hw = false;
     void *data;
     int pitch;

     D_MAGIC_ASSERT( c, _Chunk_ );
     D_ASSERT(c != NULL);
     D_ASSERT(c->buffer != NULL);
     D_ASSERT(c->buffer->surface != NULL);

     hw = c->buffer->surface->manager->funcs->CopySurfaceBuffer != NULL;

     lock_flags = DSLF_READ | DSLF_WRITE;

     if (hw)
     {
          if (dfb_surface_hardware_lock(c->buffer->surface, c->buffer, lock_flags) != DFB_OK)
          {
               D_ERROR("%s: Failure locking buffer's (%p) surface!\n", __FUNCTION__,
                    c->buffer);
               return DFB_FAILURE;
          }
     }
     else
     {
          if (dfb_surface_software_lock(c->buffer->surface, c->buffer, lock_flags, &data, &pitch) != DFB_OK)
          {
               D_ERROR("%s: Failure locking buffer's (%p) surface!\n", __FUNCTION__,
                    c->buffer);
               return DFB_FAILURE;
          }
     }

     if (c->buffer->surface->sub_manager != NULL)
     {
          manager = c->buffer->surface->sub_manager;
          dfb_surfacemanager_lock(manager);
          unreserved_partition = dfb_system_get_partition_type(CSP_VIDEOONLY);
          c_sub = manager->partition[unreserved_partition].chunks;
          while (c_sub != NULL)
          {
               if (c_sub->buffer != NULL)
               {
                    if (hw)
                    {
                         if (dfb_surface_hardware_lock(c_sub->buffer->surface, c_sub->buffer, lock_flags)
                             != DFB_OK)
                         {
                              D_ERROR("%s: Failure locking buffer's (%p) surface!\n", __FUNCTION__,
                                   c_sub->buffer);
                              break;
                         }
                    }
                    else
                    {
                         if (dfb_surface_software_lock(c_sub->buffer->surface, c_sub->buffer, lock_flags, 
                             &data, &pitch) != DFB_OK)
                         {
                              D_ERROR("%s: Failure locking buffer's (%p) surface!\n", __FUNCTION__,
                                   c_sub->buffer);
                              break;
                         }
                    }
               }
               c_sub = c_sub->next;
          }
          if (c_sub != NULL)
          {
               c_sub = c_sub->prev;
               while (c_sub != NULL)
               {
                    dfb_surface_unlock(c_sub->buffer->surface, c_sub->buffer, lock_flags);
                    c_sub = c_sub->prev;
               }
               dfb_surface_unlock(c->buffer->surface, c->buffer, lock_flags);
               ret = DFB_FAILURE;
          }
          dfb_surfacemanager_unlock(manager);
     }

     return ret;
}

static void unlock_chunk_surfaces(Chunk *c)
{
     Chunk *c_sub;
     int unreserved_partition;
     SurfaceManager *manager;
     DFBSurfaceLockFlags lock_flags, sub_lock_flags;

     D_MAGIC_ASSERT( c, _Chunk_ );
     D_ASSERT(c != NULL);
     D_ASSERT(c->buffer != NULL);
     D_ASSERT(c->buffer->surface != NULL);

     lock_flags = DSLF_READ | DSLF_WRITE;
     dfb_surface_unlock(c->buffer->surface, c->buffer, lock_flags);

     if (c->buffer->surface->sub_manager != NULL)
     {
          manager = c->buffer->surface->sub_manager;
          dfb_surfacemanager_lock(manager);
          unreserved_partition = dfb_system_get_partition_type(CSP_VIDEOONLY);
          c_sub = manager->partition[unreserved_partition].chunks;
          while (c_sub != NULL)
          {
               if (c_sub->buffer != NULL)
               {
                    dfb_surface_unlock(c_sub->buffer->surface, c_sub->buffer, lock_flags);
               }
               c_sub = c_sub->next;
          }
          dfb_surfacemanager_unlock(manager);
     }
}

static Chunk* split_chunk( Chunk *c, int length )
{
     Chunk *newchunk;

     D_MAGIC_ASSERT( c, _Chunk_ );

     if (c->length == length)          /* does not need be splitted */
          return c;

     newchunk = (Chunk*) SHCALLOC( 1, sizeof(Chunk) );

     /* calculate offsets and lengths of resulting chunks */
     newchunk->offset = c->offset + c->length - length;
     newchunk->length = length;
     c->length -= newchunk->length;

     /* insert newchunk after chunk c */
     newchunk->prev = c;
     newchunk->next = c->next;
     if (c->next)
          c->next->prev = newchunk;
     c->next = newchunk;

     D_MAGIC_SET( newchunk, _Chunk_ );

     return newchunk;
}

static Chunk*
join_chunks( Chunk *c1, Chunk *c2 )
{
     Chunk *c;

     D_MAGIC_ASSERT( c1, _Chunk_ );
     D_MAGIC_ASSERT( c2, _Chunk_ );

     if (c2->next == c1)
     {
          D_DEFRAG(("Swapping chunks\n"));
          c = c2;
          c2 = c1;
          c1 = c;
     }

     c1->length += c2->length;
     c1->next = c2->next;
     if (c1->next != NULL)
     {
          c1->next->prev = c1;
     }
     D_MAGIC_CLEAR( c2 );
     SHFREE( c2 );

     return c1;
}

static Chunk*
free_chunk( SurfaceManager *manager, Chunk *chunk )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );
     D_MAGIC_ASSERT( chunk, _Chunk_ );
     unsigned int partition_type = dfb_system_get_partition_type(chunk->buffer->policy);

     if (!chunk->buffer) {
          D_BUG( "freeing free chunk" );
          return chunk;
     }
     else {
#if D_DEBUG_ENABLED
          D_DEBUG_AT( Core_SM,
#else
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> "
#endif
          "free_chunk: policy %d deallocating %d bytes at offset 0x%08X.\n", chunk->buffer->policy, chunk->length, (unsigned int)chunk->offset );
     }



     if (chunk->buffer->policy == CSP_VIDEOONLY) {
          manager->partition[partition_type].available += chunk->length;
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> free_chunk: policy %d now %d bytes available\n", chunk->buffer->policy, manager->partition[partition_type].available );
     }

     chunk->buffer = NULL;

     if (chunk->prev  &&  !chunk->prev->buffer) {
          Chunk *prev = chunk->prev;

          //D_DEBUG_AT( Core_SM, "  -> merging with previous chunk at %d\n", prev->offset );

          prev->length += chunk->length;

          prev->next = chunk->next;
          if (prev->next)
               prev->next->prev = prev;

          //D_DEBUG_AT( Core_SM, "  -> freeing %p (prev %p, next %p)\n", chunk, chunk->prev, chunk->next);

          D_MAGIC_CLEAR( chunk );

          SHFREE( chunk );
          chunk = prev;
     }

     if (chunk->next  &&  !chunk->next->buffer) {
          Chunk *next = chunk->next;

          //D_DEBUG_AT( Core_SM, "  -> merging with next chunk at %d\n", next->offset );

          chunk->length += next->length;

          chunk->next = next->next;
          if (chunk->next)
               chunk->next->prev = chunk;

          D_MAGIC_CLEAR( next );

          SHFREE( next );
     }

     return chunk;
}

static void
occupy_chunk( SurfaceManager *manager, Chunk *chunk, SurfaceBuffer *buffer, int length )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );
     D_MAGIC_ASSERT( chunk, _Chunk_ );
     unsigned int partition_type = dfb_system_get_partition_type(buffer->policy);

     if (buffer->policy == CSP_VIDEOONLY) {
          manager->partition[partition_type].available -= length;
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> occupy_chunk: policy %d now %d bytes available\n", buffer->policy, manager->partition[partition_type].available );
     }

     chunk = split_chunk( chunk, length );

#if D_DEBUG_ENABLED
          D_DEBUG_AT( Core_SM,
#else
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> "
#endif
          "occupy_chunk: policy %d allocating %d bytes at offset 0x%08X.\n", buffer->policy, chunk->length, (unsigned int)chunk->offset );

     buffer->video.health = CSH_RESTORE;
     buffer->video.offset = chunk->offset;
     buffer->video.chunk  = chunk;

     chunk->buffer = buffer;
}

SurfaceManager *
dfb_surfacemanager_firstfit_create_mp(
    dfb_bcm_partition_info  partitions_list[],
    unsigned char partitions_number,
    CardLimitations *limits )
{
     Chunk          *chunk;
     SurfaceManager *manager;
     unsigned char   enum_partition;

#ifdef DEBUG_ALLOCATION
     dfbbcm_show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

     manager = SHCALLOC( 1, sizeof(SurfaceManager) );
     if (!manager)
          return NULL;

     manager->type = DSMT_SURFACEMANAGER_FIRSTFIT;

     manager->partitions_number = partitions_number;
     for (enum_partition = 0; enum_partition < partitions_number; enum_partition++)
     {
          chunk = SHCALLOC( 1, sizeof(Chunk) );
          if (!chunk) {
             SHFREE( manager );
             return NULL;
          }

#if 1
          D_INFO("Partition[%d] offset = %d lenght = %d\n",
          enum_partition,
          partitions_list[enum_partition].offset,
          partitions_list[enum_partition].length);
#endif

          chunk->offset = partitions_list[enum_partition].offset;
          chunk->length = partitions_list[enum_partition].length;

          manager->partition[enum_partition].chunks      = chunk;
          manager->partition[enum_partition].heap_offset = partitions_list[enum_partition].offset;
          manager->partition[enum_partition].length      = partitions_list[enum_partition].length;
          manager->partition[enum_partition].available   = partitions_list[enum_partition].length;
     }

     manager->byteoffset_align = limits->surface_byteoffset_alignment;
     manager->pixelpitch_align = limits->surface_pixelpitch_alignment;
     manager->bytepitch_align  = limits->surface_bytepitch_alignment;
     manager->max_power_of_two_pixelpitch = limits->surface_max_power_of_two_pixelpitch;
     manager->max_power_of_two_bytepitch  = limits->surface_max_power_of_two_bytepitch;
     manager->max_power_of_two_height     = limits->surface_max_power_of_two_height;

     fusion_skirmish_init( &manager->lock, "Surface Manager" );

     D_MAGIC_SET( chunk, _Chunk_ );

     D_MAGIC_SET( manager, SurfaceManager );

     return manager;
}

/*
 * Defragments the manager's heap by moving all free chunks to the beginning 
 * of the heap and combining them into one free chunk. This is accomplished 
 * by starting with the free chunk nearest the end of the heap and bubbling 
 * it toward the top, combining it with any adjacent free chunks as it is 
 * bubbled. 
 */
DFBResult dfb_surfacemanager_firstfit_defrag(
     SurfaceManager *manager)
{
     DFBResult ret = DFB_OK;
     Chunk *c;
     Chunk *last_free;
     Chunk *c_free;
     Chunk *c_occupied;
     Chunk * c_bestfit;
     unsigned char *src, *dest, *end;
     int itemp;
     int unreserved_partition;
     void *data;
     int pitch;
     int length;
     char *str;

     D_MAGIC_ASSERT(manager, SurfaceManager);

     /*
      * Lock the surfacemanager to prevent anyone else from monkeying with 
      * it or any surfaces while we are using it. Locking the surfacemanager 
      * is done before any surface operation that manipulates the surface 
      * buffer (i.e. Blit, etc). 
      */
     dfb_surfacemanager_lock(manager);

     /*
      * Ensure we are the only ones using the GFX accelerator, all of the 
      * outstanding operations are complete, and the card state is propagated 
      * to the accelerator next time someone else uses it. 
      */
     if (manager->funcs->CopySurfaceBuffer)
     {
          if (dfb_gfxcard_lock(GDLF_SYNC | GDLF_INVALIDATE) != DFB_OK)
          {
               dfb_surfacemanager_unlock(manager);
               return DFB_FAILURE;
          }
     }

     /*
      * Find the free chunk nearest the end of the heap. 
      */
     D_DEFRAG(("\nHeap BEFORE defragging:\n"));
     unreserved_partition = dfb_system_get_partition_type(CSP_VIDEOONLY);
     c = manager->partition[unreserved_partition].chunks;
     last_free = NULL;
     while (c != NULL)
     {
          if (c->buffer == NULL)
          {
               str = "FREE";
               last_free = c;
          }
          else if (is_chunk_locked(c))
          {
               str = "LOCK";
          }
          else
          {
               str = "USED";
          }
          D_DEFRAG(("[%s]: %10d bytes @ 0x%08x\n", str, c->length, c->offset));
          c = c->next;
     }

     if (last_free == NULL)
     {
          D_INFO("No free chunks found!");
          D_DEFRAG(("No free chunks found, bailing...\n"));
          if (manager->funcs->CopySurfaceBuffer)
          {
               dfb_gfxcard_unlock();
          }
          dfb_surfacemanager_unlock(manager);
          return DFB_NOVIDEOMEMORY;
     }

     /*
      * Bubble free chunks to head of list combining them along the way. If 
      * we run into locked chunks we can't bubble the free chunk around it 
      * so we try to find chunks above it to fill it in as much as possible, 
      * thereby still moving the free space toward the top of the heap. 
      */
     c_free = last_free;
     while (c_free->prev != NULL)
     {
          /*
           * If the previous chunk is free, combine them.
           */
          if (c_free->prev->buffer == NULL)
          {
               D_DEFRAG(("Joining FREE chunks @ 0x%08x, 0x%08x\n", c_free->offset, c_free->prev->offset));
               c_free = join_chunks(c_free, c_free->prev);

               /*
                * Reiterate the algorithm with the new c_free
                */
               continue;
          }

          /*
           * If the previous chunk is locked, search for a chunk above the 
           * locked one that will best fill this free chunk. 
           */
          if (is_chunk_locked(c_free->prev))
          {
               D_DEFRAG(("Locked chunk @ 0x%08x encountered.\n", c_free->prev->offset));
               c = c_free->prev;
               c_bestfit = NULL;
               length = 0;
               while (c != NULL)
               {
                    /*
                     * It's the best one found so far if it's occupied, 
                     * not locked, will fit the free chunk, and is bigger 
                     * than any previously found.  
                     */
                    if (c->buffer != NULL &&
                        !is_chunk_locked(c) &&
                        c->length <= c_free->length &&
                        c->length > length)
                    {
                         c_bestfit = c;
                         length = c_bestfit->length;
                    }
                    c = c->prev;
               }

               /*
                * If we found a chunk that will fit, copy it into the free space.
                */
               if (c_bestfit != NULL)
               {
                    /*
                     * Split the free chunk to yield a new chunk equal in size to the 
                     * bestfit chunk. 
                     */
                    c = split_chunk(c_free, c_bestfit->length);

                    D_DEFRAG(("Moving best fit chunk @ 0x%08x into free chunk @ 0x%08x\n", 
                         c_bestfit->offset, c->offset));

                    /*
                     * Lock c_bestfit's surface to prevent anyone else from mucking with it 
                     * while we manipulate it's location, ensure all CPU data is 
                     * flushed from the cache, etc. 
                     */
                    if (lock_chunk_surfaces(c_bestfit) != DFB_OK)
                    {
                         ret = DFB_FAILURE;
                         goto done;
                    }

                    /*
                     * Use any GFX card acceleration if it is available.
                     */
                    if (manager->funcs->CopySurfaceBuffer != NULL)
                    {
                         ret = manager->funcs->CopySurfaceBuffer(c_bestfit->buffer, c->offset);
                    }
                    if (manager->funcs->CopySurfaceBuffer == NULL || ret != DFB_OK)
                    {
                         end = dfb_system_video_memory_virtual(c_bestfit->offset);
                         src = dfb_system_video_memory_virtual(c_bestfit->offset + c_bestfit->length - 1);
                         dest = dfb_system_video_memory_virtual(c->offset + c->length - 1);
                         while ((int)src >= (int)end)
                         {
                              *dest-- = *src--;
                         }
                    }

                    /*
                     * Point the new chunk to the bestfit chunk's SurfaceBuffer, update 
                     * the SurfaceBuffer to point to the new chunk, update the SurfaceBuffer's 
                     * offset, and mark the bestfit chunk free. 
                     */
                    c->buffer = c_bestfit->buffer;
                    c->buffer->video.chunk = c;
                    c->buffer->video.offset = c->offset;
                    c_bestfit->buffer = NULL;

                    /*
                     * Allow the GFX card to update any internally kept data based on the new 
                     * offset. 
                     */
                    if (manager->funcs->UpdateSurfaceBuffer)
                    {
                         manager->funcs->UpdateSurfaceBuffer(c->buffer);
                    }

                    /*
                     * See if this surface has a manager of its own. If it does we need to 
                     * update the offsets of the chunks it owns. 
                     */
                    if (c->buffer->surface->sub_manager != NULL)
                    {
                         dfb_surfacemanager_adjust_heap_offset(c->buffer->surface->sub_manager, 
                              c->buffer->video.offset);
                    }

                    /*
                     * Done manipulating this chunk's buffer, so wait for accelerator 
                     * operations to complete then unlock it.
                     */
                    if (manager->funcs->CopySurfaceBuffer)
                    {
                         dfb_gfxcard_sync();
                    }
                    unlock_chunk_surfaces(c);
               }
               else
               {
                    /*
                     * No more moveable chunks will fit into this free chunk, so 
                     * advance to the next free chunk above the locked one. 
                     */
                    c_free = c_free->prev;
                    while (c_free->prev != NULL &&
                           c_free->buffer != NULL)
                    {
                         c_free = c_free->prev;
                    }
               }

               /*
                * c_free is now either the smaller remaining free chunk or the 
                * next free chunk above the locked one. In either case we reiterate 
                * the algorithm. 
                */
               continue;
          }

          /*
           * If we got to here the previous chunk must be an occupied, unlocked 
           * chunk, so move it by the length of the free chunk, thereby bubbling 
           * the free chunk toward the top of the heap.  
           */
          c_occupied = c_free->prev;

          D_DEFRAG(("Swapping occupied chunk @ 0x%08x with free chunk @ 0x%08x\n", 
               c_occupied->offset, c_free->offset));

          /*
           * Lock c_occupied's surface to prevent anyone else from mucking with it 
           * while we manipulate it's location, ensure all CPU data is 
           * flushed from the cache, etc. 
           */
          if (lock_chunk_surfaces(c_occupied) != DFB_OK)
          {
               ret = DFB_FAILURE;
               goto done;
          }

          /*
           * Use any GFX card acceleration if it is available.
           */
          if (manager->funcs->CopySurfaceBuffer)
          {
               ret = manager->funcs->CopySurfaceBuffer(c_occupied->buffer, c_occupied->offset + c_free->length);
          }
          if (manager->funcs->CopySurfaceBuffer == NULL || ret != DFB_OK)
          {
               /*
                * If the occupied chunk is bigger than the free chunk then the destination 
                * overlaps the source. Since we know that it is moving toward the end of 
                * the heap we just copy it in reverse to avoid overwriting the source.
                */
               end = dfb_system_video_memory_virtual(c_occupied->offset);
               src = dfb_system_video_memory_virtual(c_occupied->offset + c_occupied->length - 1);
               dest = dfb_system_video_memory_virtual(c_free->offset + c_free->length - 1);
               while ((int)src >= (int)end)
               {
                    *dest-- = *src--;
               }
          }

          /*
           * Update the chunk offsets to their new (swapped) locations and swap the chunks' 
           * list order so that we keep the list in heap offset order. 
           */
          c_free->offset = c_occupied->offset;
          c_occupied->offset += c_free->length;
          c_occupied->buffer->video.offset = c_occupied->offset;

          c_occupied->next = c_free->next;
          if (c_occupied->next != NULL)
          {
               c_occupied->next->prev = c_occupied;
          }
          c_free->prev = c_occupied->prev;
          if (c_free->prev != NULL)
          {
               c_free->prev->next = c_free;
          }
          c_occupied->prev = c_free;
          c_free->next = c_occupied;

          /*
           * Allow the GFX card to update any internally kept data based on the new 
           * offset. 
           */
          if (manager->funcs->UpdateSurfaceBuffer)
          {
               manager->funcs->UpdateSurfaceBuffer(c_occupied->buffer);
          }

          /*
           * See if this surface has a manager of its own. If it does we need to 
           * update the offsets of the chunks it owns. 
           */
          if (c_occupied->buffer->surface->sub_manager != NULL)
          {
               dfb_surfacemanager_adjust_heap_offset(c_occupied->buffer->surface->sub_manager, 
                    c_occupied->buffer->video.offset);
          }

          /*
           * Done manipulating this chunk's buffer, so wait for accelerator 
           * operations to complete then unlock it.
           */
          if (manager->funcs->CopySurfaceBuffer)
          {
               dfb_gfxcard_sync();
          }
          unlock_chunk_surfaces(c_occupied);
     }

done:
     /*
      * We now need to ensure our GFX card operations are complete.
      */
     if (manager->funcs->CopySurfaceBuffer)
     {
          dfb_gfxcard_sync();
          dfb_gfxcard_unlock();
     }

#if DEBUG_DEFRAG
     D_DEFRAG(("\nHeap AFTER defragging:\n"));
     c = manager->partition[unreserved_partition].chunks;
     while (c != NULL)
     {
          if (c->buffer == NULL)
          {
               str = "FREE";
          }
          else if (is_chunk_locked(c))
          {
               str = "LOCK";
          }
          else
          {
               str = "USED";
          }
          D_DEFRAG(("[%s]: %10d bytes @ 0x%08x\n", str, c->length, c->offset));
          c = c->next;
     }
#endif

     dfb_surfacemanager_unlock(manager);
     return DFB_OK;
}

DFBResult dfb_surfacemanager_firstfit_adjust_heap_offset(
     SurfaceManager *manager,
     int            offset)
{
     int unreserved;
     Chunk *c;
     int distance;

     D_ASSERT(offset >= 0);
     D_MAGIC_ASSERT(manager, SurfaceManager);

     unreserved = dfb_system_get_partition_type(CSP_VIDEOONLY);
     if (manager->partition[unreserved].heap_offset == offset)
     {
          /*
           * Nothing to do.
           */
          return DFB_OK;
     }

     dfb_surfacemanager_lock(manager);

     c = manager->partition[unreserved].chunks;
     distance = offset - manager->partition[unreserved].heap_offset;
     while (c != NULL)
     {
          c->offset += distance;
          if (c->buffer != NULL)
          {
               c->buffer->video.offset = c->offset;
               if (manager->funcs->UpdateSurfaceBuffer)
               {
                    manager->funcs->UpdateSurfaceBuffer(c->buffer);
               }
          }
          c = c->next;
     }
     manager->partition[unreserved].heap_offset = offset;

     dfb_surfacemanager_unlock(manager);

     return DFB_OK;
}


