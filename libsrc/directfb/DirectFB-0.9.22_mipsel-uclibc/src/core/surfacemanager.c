
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
#include <core/surfacemanager_internal.h>
#include <core/system.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <core/surfacemanager_buddy.h>
#include <core/surfacemanager_firstfit.h>
#include <core/surfacemanager_sequential.h>

/* Need to set fct pointers instead! */

DFBEnumerationResult
surfacemanager_chunk_callback(
                           SurfaceBuffer *buffer,
                           int            offset,
                           int            length,
                           int            tolerations,
                           void          *ctx ) {

    char *state = "FREE";

    if ((buffer) && (buffer->surface))
    {
        if (buffer->video.locked)
        {
            state = "LOCK";
        }
        else if (buffer->surface->sub_manager != NULL)
        {
            state = "SMGR";
        }
        else
        {
            state = "USED";
        }
    }

    printf("Chunk [%s]: size %9d bytes, addr 0x%08X (%9d), next Chunk Addr 0x%08x, toler %d, policy ", state, length, (unsigned int)offset, offset, (offset + length), tolerations);

    if (buffer) {
        switch (buffer->policy) {
            case CSP_VIDEOLOW:
            printf("CSP_VIDEOLOW");
            break;
            case CSP_VIDEOHIGH:
            printf("CSP_VIDEOHIGH");
            break;
            case CSP_VIDEOONLY:
            printf("CSP_VIDEOONLY");
            break;
            default:
            break;
        }
   }

   printf("\n");

   return DFENUM_OK;
}

SurfaceManager *
dfb_surfacemanager_create( unsigned int            offset,
                           unsigned int            length,
                           unsigned int            minimum_page_size,
                           CardLimitations        *limits,
                           DFBSurfaceManagerTypes  type )
{
    switch (type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_create(offset, length, minimum_page_size, limits);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return NULL;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return NULL;
        break;
        default:
            D_ERROR("Invalid type in dfb_surfacemanager_create.\n");
            abort();
        break;
    }
}

void dfb_surfacemanager_destroy( SurfaceManager *manager )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_destroy(manager);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return;
        break;
        default:
            D_ERROR("Invalid type in dfb_surfacemanager_destroy€\n");
            abort();
        break;
    }
}

DFBResult dfb_surfacemanager_suspend( SurfaceManager *manager )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_suspend(manager);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

DFBResult dfb_surfacemanager_defrag(
     SurfaceManager *manager)
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_defrag(manager);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

DFBResult dfb_surfacemanager_adjust_heap_offset( SurfaceManager *manager,
                                                 int             offset )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_adjust_heap_offset(manager, offset);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

void dfb_surfacemanager_enumerate_chunks( SurfaceManager  *manager,
                                          SMChunkCallback  callback,
                                          void            *ctx )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_enumerate_chunks(manager, callback, ctx);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

void dfb_surfacemanager_retrive_surface_info( SurfaceManager  *manager,
                                              SurfaceBuffer   *surface,
                                              int             *offset,
                                              int             *length,
                                              int             *tolerations)
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_retrive_surface_info(manager, surface, offset, length, tolerations);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}


DFBResult dfb_surfacemanager_allocate( SurfaceManager *manager,
                                       SurfaceBuffer  *buffer )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_allocate(manager, buffer);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

DFBResult dfb_surfacemanager_deallocate( SurfaceManager *manager,
                                         SurfaceBuffer  *buffer )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_deallocate(manager, buffer);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

DFBResult dfb_surfacemanager_assure_video( SurfaceManager *manager,
                                           SurfaceBuffer  *buffer )
{
    switch (manager->type) {
        case DSMT_SURFACEMANAGER_FIRSTFIT:
            return dfb_surfacemanager_firstfit_assure_video(manager, buffer);
        break;
        case DSMT_SURFACEMANAGER_BUDDY:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_BUDDY in "
                    "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        case DSMT_SURFACEMANAGER_SEQUENTIAL:
            D_ERROR("Unsupported type DSMT_SURFACEMANAGER_SEQUENTIAL in "
                "%s.\n", __FUNCTION__);
            return DFB_UNSUPPORTED;
        break;
        default:
            D_ERROR("Invalid type in "
                "%s.\n", __FUNCTION__);
            abort();
        break;
    }
}

DFBResult dfb_surfacemanager_resume( SurfaceManager *manager )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     manager->suspended = false;

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

void dfb_surfacemanager_lock( SurfaceManager *manager )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );

     fusion_skirmish_prevail( &manager->lock );
}

void dfb_surfacemanager_unlock( SurfaceManager *manager )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );

     fusion_skirmish_dismiss( &manager->lock );
}

DFBResult dfb_surfacemanager_assure_system( SurfaceManager *manager,
                                            SurfaceBuffer  *buffer )
{
     CoreSurface *surface = buffer->surface;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (buffer->policy == CSP_VIDEOONLY) {
          D_BUG( "surface_manager_assure_system() called on video only surface" );
          return DFB_BUG;
     }

     if (buffer->system.health == CSH_STORED)
          return DFB_OK;
     else if (buffer->video.health == CSH_STORED) {
          int   i;
          char *src = dfb_system_video_memory_virtual( buffer->video.offset);
          char *dst = buffer->system.addr;

          dfb_surface_video_access_by_software( buffer, DSLF_READ );

          for (i=0; i<surface->height; i++) {
               direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format, surface->width) );
               src += buffer->video.pitch;
               dst += buffer->system.pitch;
          }

          if (buffer->format == DSPF_YV12 || buffer->format == DSPF_I420) {
               for (i=0; i<surface->height; i++) {
                    direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                surface->width / 2) );
                    src += buffer->video.pitch  / 2;
                    dst += buffer->system.pitch / 2;
               }
          }
          else if (buffer->format == DSPF_NV12 || buffer->format == DSPF_NV21) {
               for (i=0; i<surface->height/2; i++) {
                    direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                surface->width) );
                    src += buffer->video.pitch;
                    dst += buffer->system.pitch;
               }
          }
          else if (buffer->format == DSPF_NV16) {
               for (i=0; i<surface->height; i++) {
                    direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                surface->width) );
                    src += buffer->video.pitch;
                    dst += buffer->system.pitch;
               }
          }

          buffer->system.health = CSH_STORED;

          dfb_surface_notify_listeners( surface, CSNF_SYSTEM );

          return DFB_OK;
     }

     D_BUG( "no valid surface instance" );
     return DFB_BUG;
}

SurfaceManager *
dfb_surfacemanager_create_mp( dfb_bcm_partition_info  partitions_list[],
                              unsigned char partitions_number,
                              CardLimitations *limits )
{
    return dfb_surfacemanager_firstfit_create_mp( partitions_list, partitions_number, limits );
}
