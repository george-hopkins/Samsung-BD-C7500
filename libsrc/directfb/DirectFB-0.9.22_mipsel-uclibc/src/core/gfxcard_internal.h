
#ifndef __GFXCARD_INTERNAL_H__
#define __GFXCARD_INTERNAL_H__

#include <direct/types.h>

#include <fusion/property.h>

#include <core/gfxcard.h>

#include <core/surfacemanager.h>

/*
 * struct for graphics cards
 */
typedef struct {
     /* amount of usable video memory */
     unsigned int             videoram_length;

     char                    *module_name;

     GraphicsDriverInfo       driver_info;
     GraphicsDeviceInfo       device_info;
     void                    *device_data;

     FusionProperty           lock;
     GraphicsDeviceLockFlags  lock_flags;

     SurfaceManager          *surface_manager;

     /*
      * Points to the current state of the graphics card.
      */
     CardState               *state;
     int                      holder; /* Fusion ID of state owner. */
} GraphicsDeviceShared;

struct _GraphicsDevice {
     GraphicsDeviceShared      *shared;

     DirectModuleEntry         *module;
     const GraphicsDriverFuncs *driver_funcs;

     void                      *driver_data;
     void                      *device_data; /* copy of shared->device_data */

     CardCapabilities           caps;        /* local caps */

     CardLimitations            limits;

     GraphicsDeviceFuncs        funcs;

     /* Gfx driver surface management functions */
     SurfaceManagerFuncs        surfacemngmt_funcs;
};

#endif
