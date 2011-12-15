#ifndef __IDIRECTFBSURFACEMANAGER_H__
#define __IDIRECTFBSURFACEMANAGER_H__

#include <directfb.h>
#include <core/coretypes.h>

/*
 * initializes interface struct and private data
 */
DFBResult
IDirectFBSurfaceManager_Construct( IDirectFBSurfaceManager *thiz,
                                   CoreDFB                 *core,
                                   SurfaceManager          *manager,
                                   CoreSurface             *surface );

#endif
