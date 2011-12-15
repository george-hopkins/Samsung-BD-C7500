
#ifndef __SURFACE_MANAGER_FIRSTFIT_H__
#define __SURFACE_MANAGER_FIRSTFIT_H__

SurfaceManager *dfb_surfacemanager_firstfit_create(
    unsigned int        offset,
    unsigned int        length,
    unsigned int        minimum_page_size,
    CardLimitations    *limits );

void dfb_surfacemanager_firstfit_destroy(
    SurfaceManager *manager );

DFBResult dfb_surfacemanager_firstfit_suspend( SurfaceManager *manager );

DFBResult dfb_surfacemanager_firstfit_adjust_heap_offset(
    SurfaceManager *manager,
    int             offset );

void dfb_surfacemanager_firstfit_enumerate_chunks(
    SurfaceManager  *manager,
    SMChunkCallback  callback,
    void            *ctx );

DFBResult dfb_surfacemanager_firstfit_allocate(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

DFBResult dfb_surfacemanager_firstfit_deallocate(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

DFBResult dfb_surfacemanager_firstfit_assure_video(
    SurfaceManager *manager,
    SurfaceBuffer  *buffer );

void dfb_surfacemanager_firstfit_retrive_surface_info(
    SurfaceManager *manager,
    SurfaceBuffer  *surface,
    int            *offset,
    int            *length,
    int            *tolerations
);

SurfaceManager *
dfb_surfacemanager_firstfit_create_mp(
    dfb_bcm_partition_info  partitions_list[],
    unsigned char partitions_number,
    CardLimitations *limits);

DFBResult dfb_surfacemanager_firstfit_defrag(
     SurfaceManager *manager);

#endif /* __SURFACE_MANAGER_FIRSTFIT_H__ */
