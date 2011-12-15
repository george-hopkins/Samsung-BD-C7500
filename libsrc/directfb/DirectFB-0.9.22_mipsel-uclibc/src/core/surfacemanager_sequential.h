
#ifndef __SURFACE_MANAGER_SEQUENTIAL_H__
#define __SURFACE_MANAGER_SEQUENTIAL_H__

SurfaceManager *dfb_surfacemanager_sequential_create(
    unsigned int        offset,
    unsigned int        length,
    unsigned int        minimum_page_size,
    CardLimitations    *limits );

void dfb_surfacemanager_sequential_destroy(
    SurfaceManager *manager );

DFBResult dfb_surfacemanager_sequential_suspend( SurfaceManager *manager );

DFBResult dfb_surfacemanager_sequential_adjust_heap_offset(
    SurfaceManager *manager,
    int             offset );

void dfb_surfacemanager_sequential_enumerate_chunks(
    SurfaceManager  *manager,
    SMChunkCallback  callback,
    void            *ctx );

DFBResult dfb_surfacemanager_sequential_allocate(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

DFBResult dfb_surfacemanager_sequential_deallocate(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

DFBResult dfb_surfacemanager_sequential_assure_video(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

void dfb_surfacemanager_sequential_retrive_surface_info(
    SurfaceManager *manager,
    SurfaceBuffer  *surface,
    int            *offset,
    int            *length,
    int            *tolerations
);

#endif /* __SURFACE_MANAGER_SEQUENTIAL_H__ */
