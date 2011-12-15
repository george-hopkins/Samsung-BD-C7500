
#ifndef __SURFACE_MANAGER_BUDDY_H__
#define __SURFACE_MANAGER_BUDDY_H__

SurfaceManager *dfb_surfacemanager_buddy_create(
    unsigned int        offset,
    unsigned int        length,
    unsigned int        minimum_page_size,
    CardLimitations    *limits );

void dfb_surfacemanager_buddy_destroy(
    SurfaceManager *manager );

DFBResult dfb_surfacemanager_buddy_adjust_heap_offset(
    SurfaceManager *manager,
    int             offset );

void dfb_surfacemanager_buddy_enumerate_chunks(
    SurfaceManager  *manager,
    SMChunkCallback  callback,
    void            *ctx );

DFBResult dfb_surfacemanager_buddy_allocate(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

DFBResult dfb_surfacemanager_buddy_deallocate(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

DFBResult dfb_surfacemanager_buddy_assure_video(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

#endif /* __SURFACE_MANAGER_BUDDY_H__ */
