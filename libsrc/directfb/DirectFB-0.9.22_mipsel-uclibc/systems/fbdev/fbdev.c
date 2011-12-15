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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#if defined(HAVE_SYSIO)
# include <sys/io.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/kd.h>

#include <linux/fb.h>

#include <pthread.h>

#include <fusion/shmalloc.h>
#include <fusion/reactor.h>
#include <fusion/arena.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/layer_control.h>
#include <core/layers.h>
#include <core/gfxcard.h>
#include <core/palette.h>
#include <core/screen.h>
#include <core/screens.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/state.h>
#include <core/windows.h>

#include <gfx/convert.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/signals.h>
#include <direct/system.h>
#include <direct/util.h>

#include <misc/conf.h>
#include <misc/util.h>

#include "fbdev.h"
#include "vt.h"

#include <core/core_system.h>

DFB_CORE_SYSTEM( fbdev )


static int fbdev_ioctl_call_handler( int   caller,
                                     int   call_arg,
                                     void *call_ptr,
                                     void *ctx );

static int fbdev_ioctl( int request, void *arg, int arg_size );

#define FBDEV_IOCTL(request,arg)   fbdev_ioctl( request, arg, sizeof(*(arg)) )

FBDev *dfb_fbdev = NULL;

/******************************************************************************/

static int       primaryLayerDataSize ();

static int       primaryRegionDataSize();

static DFBResult primaryInitLayer     ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        DFBDisplayLayerDescription *description,
                                        DFBDisplayLayerConfig      *config,
                                        DFBColorAdjustment         *adjustment );

static DFBResult primarySetColorAdjustment( CoreLayer              *layer,
                                            void                   *driver_data,
                                            void                   *layer_data,
                                            DFBColorAdjustment     *adjustment );

static DFBResult primaryTestRegion    ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        CoreLayerRegionConfig      *config,
                                        CoreLayerRegionConfigFlags *failed );

static DFBResult primaryAddRegion     ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        void                       *region_data,
                                        CoreLayerRegionConfig      *config );

static DFBResult primarySetRegion     ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        void                       *region_data,
                                        CoreLayerRegionConfig      *config,
                                        CoreLayerRegionConfigFlags  updated,
                                        CoreSurface                *surface,
                                        CorePalette                *palette );

static DFBResult primaryRemoveRegion  ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        void                       *region_data );

static DFBResult primaryFlipRegion    ( CoreLayer                  *layer,
                                        void                       *driver_data,
                                        void                       *layer_data,
                                        void                       *region_data,
                                        CoreSurface                *surface,
                                        DFBSurfaceFlipFlags         flags );


static DFBResult primaryAllocateSurface  ( CoreLayer                  *layer,
                                           void                       *driver_data,
                                           void                       *layer_data,
                                           void                       *region_data,
                                           CoreLayerRegionConfig      *config,
                                           CoreSurface               **ret_surface );

static DFBResult primaryReallocateSurface( CoreLayer                  *layer,
                                           void                       *driver_data,
                                           void                       *layer_data,
                                           void                       *region_data,
                                           CoreLayerRegionConfig      *config,
                                           CoreSurface                *surface );

static DisplayLayerFuncs primaryLayerFuncs = {
     LayerDataSize:      primaryLayerDataSize,
     RegionDataSize:     primaryRegionDataSize,
     InitLayer:          primaryInitLayer,

     SetColorAdjustment: primarySetColorAdjustment,

     TestRegion:         primaryTestRegion,
     AddRegion:          primaryAddRegion,
     SetRegion:          primarySetRegion,
     RemoveRegion:       primaryRemoveRegion,
     FlipRegion:         primaryFlipRegion,

     AllocateSurface:    primaryAllocateSurface,
     ReallocateSurface:  primaryReallocateSurface,
     /* default DeallocateSurface copes with our chunkless video buffers */
};

/******************************************************************************/

static DFBResult primaryInitScreen  ( CoreScreen           *screen,
                                      GraphicsDevice       *device,
                                      void                 *driver_data,
                                      void                 *screen_data,
                                      DFBScreenDescription *description );

static DFBResult primarySetPowerMode( CoreScreen           *screen,
                                      void                 *driver_data,
                                      void                 *screen_data,
                                      DFBScreenPowerMode    mode );

static DFBResult primaryWaitVSync   ( CoreScreen           *screen,
                                      void                 *driver_data,
                                      void                 *layer_data );

static DFBResult primaryGetScreenSize( CoreScreen           *screen,
                                       void                 *driver_data,
                                       void                 *screen_data,
                                       int                  *ret_width,
                                       int                  *ret_height );

static ScreenFuncs primaryScreenFuncs = {
     .InitScreen    = primaryInitScreen,
     .SetPowerMode  = primarySetPowerMode,
     .WaitVSync     = primaryWaitVSync,
     .GetScreenSize = primaryGetScreenSize
};

/******************************************************************************/

static DFBResult dfb_fbdev_read_modes();
static DFBResult dfb_fbdev_set_gamma_ramp( DFBSurfacePixelFormat format );
static DFBResult dfb_fbdev_set_palette( CorePalette *palette );
static DFBResult dfb_fbdev_set_rgb332_palette();
static DFBResult dfb_fbdev_pan( int offset, bool onsync );
static DFBResult dfb_fbdev_blank( int level );
static DFBResult dfb_fbdev_set_mode( CoreSurface           *surface,
                                     VideoMode             *mode,
                                     CoreLayerRegionConfig *config );

/******************************************************************************/

static inline
void waitretrace (void)
{
#if defined(HAVE_INB_OUTB_IOPL)
     if (iopl(3))
          return;

     if (!(inb (0x3cc) & 1)) {
          while ((inb (0x3ba) & 0x8))
               ;

          while (!(inb (0x3ba) & 0x8))
               ;
     }
     else {
          while ((inb (0x3da) & 0x8))
               ;

          while (!(inb (0x3da) & 0x8))
               ;
     }
#endif
}

/******************************************************************************/

static DFBResult dfb_fbdev_open()
{
     if (dfb_config->fb_device) {
          dfb_fbdev->fd = open( dfb_config->fb_device, O_RDWR );
          if (dfb_fbdev->fd < 0) {
               D_PERROR( "DirectFB/FBDev: Error opening `%s'!\n",
                         dfb_config->fb_device);

               return errno2result( errno );
          }
     }
     else if (getenv( "FRAMEBUFFER" ) && *getenv( "FRAMEBUFFER" ) != '\0') {
          dfb_fbdev->fd = open( getenv ("FRAMEBUFFER"), O_RDWR );
          if (dfb_fbdev->fd < 0) {
               D_PERROR( "DirectFB/FBDev: Error opening `%s'!\n",
                          getenv ("FRAMEBUFFER"));

               return errno2result( errno );
          }
     }
     else {
          dfb_fbdev->fd = open( "/dev/fb0", O_RDWR );
          if (dfb_fbdev->fd < 0) {
               if (errno == ENOENT) {
                    dfb_fbdev->fd = open( "/dev/fb/0", O_RDWR );
                    if (dfb_fbdev->fd < 0) {
                         if (errno == ENOENT) {
                              D_PERROR( "DirectFB/FBDev: Couldn't open "
                                         "neither `/dev/fb0' nor `/dev/fb/0'!\n" );
                         }
                         else {
                              D_PERROR( "DirectFB/FBDev: "
                                         "Error opening `/dev/fb/0'!\n" );
                         }

                         return errno2result( errno );
                    }
               }
               else {
                    D_PERROR( "DirectFB/FBDev: Error opening `/dev/fb0'!\n");

                    return errno2result( errno );
               }
          }
     }

     return DFB_OK;
}

/** public **/

static void
system_get_info( CoreSystemInfo *info )
{
     info->type = CORE_FBDEV;

     snprintf( info->name, DFB_CORE_SYSTEM_INFO_NAME_LENGTH, "FBDev" );
}

static DFBResult
system_initialize( CoreDFB *core, void **data )
{
     DFBResult   ret;
     CoreScreen *screen;
     long        page_size;

     D_ASSERT( dfb_fbdev == NULL );

     dfb_fbdev = D_CALLOC( 1, sizeof(FBDev) );

     dfb_fbdev->shared = (FBDevShared*) SHCALLOC( 1, sizeof(FBDevShared) );

     fusion_arena_add_shared_field( dfb_core_arena( core ),
                                    "fbdev", dfb_fbdev->shared );

     dfb_fbdev->core = core;

     page_size = direct_pagesize();
     dfb_fbdev->shared->page_mask = page_size < 0 ? 0 : (page_size - 1);

     ret = dfb_fbdev_open();
     if (ret) {
          SHFREE( dfb_fbdev->shared );
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;

          return ret;
     }

     if (dfb_config->vt) {
          ret = dfb_vt_initialize();
          if (ret) {
               SHFREE( dfb_fbdev->shared );
               D_FREE( dfb_fbdev );
               dfb_fbdev = NULL;

               return ret;
          }
     }

     /* Retrieve fixed informations like video ram size */
     if (ioctl( dfb_fbdev->fd, FBIOGET_FSCREENINFO, &dfb_fbdev->shared->fix ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                    "Could not get fixed screen information!\n" );
          SHFREE( dfb_fbdev->shared );
          close( dfb_fbdev->fd );
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;

          return DFB_INIT;
     }

     /* Map the framebuffer */
     dfb_fbdev->framebuffer_base = mmap( NULL, dfb_fbdev->shared->fix.smem_len,
                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                         dfb_fbdev->fd, 0 );
     if ((int)(dfb_fbdev->framebuffer_base) == -1) {
          D_PERROR( "DirectFB/FBDev: "
                    "Could not mmap the framebuffer!\n");
          SHFREE( dfb_fbdev->shared );
          close( dfb_fbdev->fd );
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;

          return DFB_INIT;
     }

     if (ioctl( dfb_fbdev->fd, FBIOGET_VSCREENINFO, &dfb_fbdev->shared->orig_var ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                    "Could not get variable screen information!\n" );
          SHFREE( dfb_fbdev->shared );
          munmap( dfb_fbdev->framebuffer_base, dfb_fbdev->shared->fix.smem_len );
          close( dfb_fbdev->fd );
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;

          return DFB_INIT;
     }

     dfb_fbdev->shared->current_var = dfb_fbdev->shared->orig_var;
     dfb_fbdev->shared->current_var.accel_flags = 0;

     if (ioctl( dfb_fbdev->fd, FBIOPUT_VSCREENINFO, &dfb_fbdev->shared->current_var ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                    "Could not disable console acceleration!\n" );
          SHFREE( dfb_fbdev->shared );
          munmap( dfb_fbdev->framebuffer_base, dfb_fbdev->shared->fix.smem_len );
          close( dfb_fbdev->fd );
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;

          return DFB_INIT;
     }

     dfb_fbdev->shared->orig_cmap.start  = 0;
     dfb_fbdev->shared->orig_cmap.len    = 256;
     dfb_fbdev->shared->orig_cmap.red    = (__u16*)SHMALLOC( 2 * 256 );
     dfb_fbdev->shared->orig_cmap.green  = (__u16*)SHMALLOC( 2 * 256 );
     dfb_fbdev->shared->orig_cmap.blue   = (__u16*)SHMALLOC( 2 * 256 );
     dfb_fbdev->shared->orig_cmap.transp = (__u16*)SHMALLOC( 2 * 256 );

     if (ioctl( dfb_fbdev->fd, FBIOGETCMAP, &dfb_fbdev->shared->orig_cmap ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                    "Could not retrieve palette for backup!\n" );
          SHFREE( dfb_fbdev->shared->orig_cmap.red );
          SHFREE( dfb_fbdev->shared->orig_cmap.green );
          SHFREE( dfb_fbdev->shared->orig_cmap.blue );
          SHFREE( dfb_fbdev->shared->orig_cmap.transp );
          dfb_fbdev->shared->orig_cmap.len = 0;
     }

     dfb_fbdev->shared->temp_cmap.len    = 256;
     dfb_fbdev->shared->temp_cmap.red    = SHCALLOC( 256, 2 );
     dfb_fbdev->shared->temp_cmap.green  = SHCALLOC( 256, 2 );
     dfb_fbdev->shared->temp_cmap.blue   = SHCALLOC( 256, 2 );
     dfb_fbdev->shared->temp_cmap.transp = SHCALLOC( 256, 2 );

     dfb_fbdev->shared->current_cmap.len    = 256;
     dfb_fbdev->shared->current_cmap.red    = SHCALLOC( 256, 2 );
     dfb_fbdev->shared->current_cmap.green  = SHCALLOC( 256, 2 );
     dfb_fbdev->shared->current_cmap.blue   = SHCALLOC( 256, 2 );
     dfb_fbdev->shared->current_cmap.transp = SHCALLOC( 256, 2 );

     fusion_call_init( &dfb_fbdev->shared->fbdev_ioctl,
                       fbdev_ioctl_call_handler, NULL );

     /* Register primary screen functions */
     screen = dfb_screens_register( NULL, NULL, &primaryScreenFuncs );

     /* Register primary layer functions */
     dfb_layers_register( screen, NULL, &primaryLayerFuncs );

     *data = dfb_fbdev;

     return DFB_OK;
}

static DFBResult
system_join( CoreDFB *core, void **data )
{
     DFBResult   ret;
     CoreScreen *screen;

     D_ASSERT( dfb_fbdev == NULL );

     if (dfb_config->vt) {
          ret = dfb_vt_join();
          if (ret)
               return ret;
     }

     dfb_fbdev = D_CALLOC( 1, sizeof(FBDev) );

     fusion_arena_get_shared_field( dfb_core_arena( core ),
                                    "fbdev", (void**) &dfb_fbdev->shared );

     dfb_fbdev->core = core;

     /* Open framebuffer device */
     ret = dfb_fbdev_open();
     if (ret) {
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;
          return ret;
     }

     /* Map the framebuffer */
     dfb_fbdev->framebuffer_base = mmap( NULL, dfb_fbdev->shared->fix.smem_len,
                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                         dfb_fbdev->fd, 0 );
     if ((int)(dfb_fbdev->framebuffer_base) == -1) {
          D_PERROR( "DirectFB/FBDev: "
                    "Could not mmap the framebuffer!\n");
          close( dfb_fbdev->fd );
          D_FREE( dfb_fbdev );
          dfb_fbdev = NULL;

          return DFB_INIT;
     }

     /* Register primary screen functions */
     screen = dfb_screens_register( NULL, NULL, &primaryScreenFuncs );

     /* Register primary layer functions */
     dfb_layers_register( screen, NULL, &primaryLayerFuncs );

     *data = dfb_fbdev;

     return DFB_OK;
}

static DFBResult
system_shutdown( bool emergency )
{
     DFBResult  ret;
     VideoMode *m;

     D_ASSERT( dfb_fbdev != NULL );

     m = dfb_fbdev->shared->modes;
     while (m) {
          VideoMode *next = m->next;
          SHFREE( m );
          m = next;
     }

     if (ioctl( dfb_fbdev->fd, FBIOPUT_VSCREENINFO, &dfb_fbdev->shared->orig_var ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                     "Could not restore variable screen information!\n" );
     }

     if (dfb_fbdev->shared->orig_cmap.len) {
          if (ioctl( dfb_fbdev->fd, FBIOPUTCMAP, &dfb_fbdev->shared->orig_cmap ) < 0)
               D_PERROR( "DirectFB/FBDev: "
                          "Could not restore palette!\n" );

          SHFREE( dfb_fbdev->shared->orig_cmap.red );
          SHFREE( dfb_fbdev->shared->orig_cmap.green );
          SHFREE( dfb_fbdev->shared->orig_cmap.blue );
          SHFREE( dfb_fbdev->shared->orig_cmap.transp );
     }

     SHFREE( dfb_fbdev->shared->temp_cmap.red );
     SHFREE( dfb_fbdev->shared->temp_cmap.green );
     SHFREE( dfb_fbdev->shared->temp_cmap.blue );
     SHFREE( dfb_fbdev->shared->temp_cmap.transp );

     SHFREE( dfb_fbdev->shared->current_cmap.red );
     SHFREE( dfb_fbdev->shared->current_cmap.green );
     SHFREE( dfb_fbdev->shared->current_cmap.blue );
     SHFREE( dfb_fbdev->shared->current_cmap.transp );

     fusion_call_destroy( &dfb_fbdev->shared->fbdev_ioctl );

     munmap( dfb_fbdev->framebuffer_base, dfb_fbdev->shared->fix.smem_len );

     if (dfb_config->vt) {
          ret = dfb_vt_shutdown( emergency );
          if (ret)
               return ret;
     }

     close( dfb_fbdev->fd );

     SHFREE( dfb_fbdev->shared );
     D_FREE( dfb_fbdev );
     dfb_fbdev = NULL;

     return DFB_OK;
}

static DFBResult
system_leave( bool emergency )
{
     DFBResult ret;

     D_ASSERT( dfb_fbdev != NULL );

     munmap( dfb_fbdev->framebuffer_base,
             dfb_fbdev->shared->fix.smem_len );

     if (dfb_config->vt) {
          ret = dfb_vt_leave( emergency );
          if (ret)
               return ret;
     }

     close( dfb_fbdev->fd );

     D_FREE( dfb_fbdev );
     dfb_fbdev = NULL;

     return DFB_OK;
}

static DFBResult
system_suspend()
{
     return DFB_OK;
}

static DFBResult
system_resume()
{
     return DFB_OK;
}

/******************************************************************************/

static volatile void *
system_map_mmio( unsigned int    offset,
                 int             length )
{
     void *addr;

     if (length <= 0)
          length = dfb_fbdev->shared->fix.mmio_len;

     addr = mmap( NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED,
                  dfb_fbdev->fd, dfb_fbdev->shared->fix.smem_len + offset );
     if ((int)(addr) == -1) {
          D_PERROR( "DirectFB/FBDev: Could not mmap MMIO region "
                     "(offset %d, length %d)!\n", offset, length );
          return NULL;
     }

     return(volatile void*) ((__u8*) addr + (dfb_fbdev->shared->fix.mmio_start &
                                             dfb_fbdev->shared->page_mask));
}

static void
system_unmap_mmio( volatile void  *addr,
                   int             length )
{
     if (length <= 0)
          length = dfb_fbdev->shared->fix.mmio_len;

     if (munmap( (void*) ((__u8*) addr - (dfb_fbdev->shared->fix.mmio_start &
                                          dfb_fbdev->shared->page_mask)), length ) < 0)
          D_PERROR( "DirectFB/FBDev: Could not unmap MMIO region "
                     "at %p (length %d)!\n", addr, length );
}

static int
system_get_accelerator()
{
#ifdef FB_ACCEL_MATROX_MGAG400
     if (!strcmp( dfb_fbdev->shared->fix.id, "MATROX DH" ))
          return FB_ACCEL_MATROX_MGAG400;
#endif
     return dfb_fbdev->shared->fix.accel;
}

static VideoMode *
system_get_modes()
{
     return dfb_fbdev->shared->modes;
}

static VideoMode *
system_get_current_mode()
{
     return dfb_fbdev->shared->current_mode;
}

static DFBResult
system_thread_init()
{
     if (dfb_config->block_all_signals)
          direct_signals_block_all();

     if (dfb_config->vt)
          return dfb_vt_detach( false );

     return DFB_OK;
}

static bool
system_input_filter( CoreInputDevice *device,
                     DFBInputEvent   *event )
{
     if (dfb_config->vt && dfb_config->vt_switching) {
          switch (event->type) {
               case DIET_KEYPRESS:
                    if (DFB_KEY_TYPE(event->key_symbol) == DIKT_FUNCTION &&
                        event->modifiers == (DIMM_CONTROL | DIMM_ALT))
                         return dfb_vt_switch( event->key_symbol - DIKS_F1 + 1 );

                    break;

               case DIET_KEYRELEASE:
                    if (DFB_KEY_TYPE(event->key_symbol) == DIKT_FUNCTION &&
                        event->modifiers == (DIMM_CONTROL | DIMM_ALT))
                         return true;

                    break;

               default:
                    break;
          }
     }

     return false;
}

static unsigned long
system_video_memory_physical( unsigned int offset )
{
     return dfb_fbdev->shared->fix.smem_start + offset;
}

static void *
system_video_memory_virtual( unsigned int offset )
{
     return(void*)((__u8*)(dfb_fbdev->framebuffer_base) + offset);
}

static unsigned int
system_videoram_length()
{
     return dfb_fbdev->shared->fix.smem_len;
}

/******************************************************************************/

static DFBResult
init_modes()
{
     dfb_fbdev_read_modes();

     if (!dfb_fbdev->shared->modes) {
          /* try to use current mode*/
          dfb_fbdev->shared->modes = (VideoMode*) SHCALLOC( 1, sizeof(VideoMode) );

          dfb_fbdev->shared->modes->xres = dfb_fbdev->shared->orig_var.xres;
          dfb_fbdev->shared->modes->yres = dfb_fbdev->shared->orig_var.yres;
          dfb_fbdev->shared->modes->bpp  = dfb_fbdev->shared->orig_var.bits_per_pixel;
          dfb_fbdev->shared->modes->hsync_len = dfb_fbdev->shared->orig_var.hsync_len;
          dfb_fbdev->shared->modes->vsync_len = dfb_fbdev->shared->orig_var.vsync_len;
          dfb_fbdev->shared->modes->left_margin = dfb_fbdev->shared->orig_var.left_margin;
          dfb_fbdev->shared->modes->right_margin = dfb_fbdev->shared->orig_var.right_margin;
          dfb_fbdev->shared->modes->upper_margin = dfb_fbdev->shared->orig_var.upper_margin;
          dfb_fbdev->shared->modes->lower_margin = dfb_fbdev->shared->orig_var.lower_margin;
          dfb_fbdev->shared->modes->pixclock = dfb_fbdev->shared->orig_var.pixclock;


          if (dfb_fbdev->shared->orig_var.sync & FB_SYNC_HOR_HIGH_ACT)
               dfb_fbdev->shared->modes->hsync_high = 1;
          if (dfb_fbdev->shared->orig_var.sync & FB_SYNC_VERT_HIGH_ACT)
               dfb_fbdev->shared->modes->vsync_high = 1;

          if (dfb_fbdev->shared->orig_var.vmode & FB_VMODE_INTERLACED)
               dfb_fbdev->shared->modes->laced = 1;
          if (dfb_fbdev->shared->orig_var.vmode & FB_VMODE_DOUBLE)
               dfb_fbdev->shared->modes->doubled = 1;

          if (dfb_fbdev_set_mode(NULL, dfb_fbdev->shared->modes, NULL)) {
               D_ERROR("DirectFB/FBDev: "
                        "No supported modes found in /etc/fb.modes and "
                        "current mode not supported!\n");

               D_ERROR( "DirectFB/FBDev: Current mode's pixelformat: "
                         "rgba %d/%d, %d/%d, %d/%d, %d/%d (%dbit)\n",
                         dfb_fbdev->shared->orig_var.red.length,
                         dfb_fbdev->shared->orig_var.red.offset,
                         dfb_fbdev->shared->orig_var.green.length,
                         dfb_fbdev->shared->orig_var.green.offset,
                         dfb_fbdev->shared->orig_var.blue.length,
                         dfb_fbdev->shared->orig_var.blue.offset,
                         dfb_fbdev->shared->orig_var.transp.length,
                         dfb_fbdev->shared->orig_var.transp.offset,
                         dfb_fbdev->shared->orig_var.bits_per_pixel );

               return DFB_INIT;
          }
     }

     return DFB_OK;
}

/******************************************************************************/

static DFBResult
primaryInitScreen( CoreScreen           *screen,
                   GraphicsDevice       *device,
                   void                 *driver_data,
                   void                 *screen_data,
                   DFBScreenDescription *description )
{
     /* Set the screen capabilities. */
     description->caps = DSCCAPS_VSYNC | DSCCAPS_POWER_MANAGEMENT;

     /* Set the screen name. */
     snprintf( description->name,
               DFB_SCREEN_DESC_NAME_LENGTH, "FBDev Primary Screen" );

     return DFB_OK;
}

static DFBResult
primarySetPowerMode( CoreScreen         *screen,
                     void               *driver_data,
                     void               *screen_data,
                     DFBScreenPowerMode  mode )
{
     int level;

     switch (mode) {
          case DSPM_OFF:
               level = 4;
               break;
          case DSPM_SUSPEND:
               level = 3;
               break;
          case DSPM_STANDBY:
               level = 2;
               break;
          case DSPM_ON:
               level = 0;
               break;
          default:
               return DFB_INVARG;
     }

     return dfb_fbdev_blank( level );
}

static DFBResult
primaryWaitVSync( CoreScreen *screen,
                  void       *driver_data,
                  void       *screen_data )
{
     static const int zero = 0;

     if (dfb_config->pollvsync_none)
          return DFB_OK;

     if (ioctl( dfb_fbdev->fd, FBIO_WAITFORVSYNC, &zero ))
          waitretrace();

     return DFB_OK;
}

static DFBResult
primaryGetScreenSize( CoreScreen *screen,
                      void       *driver_data,
                      void       *screen_data,
                      int        *ret_width,
                      int        *ret_height )
{
     FBDevShared *shared;

     D_ASSERT( dfb_fbdev != NULL );
     D_ASSERT( dfb_fbdev->shared != NULL );

     shared = dfb_fbdev->shared;

     if (shared->current_mode) {
          *ret_width  = shared->current_mode->xres;
          *ret_height = shared->current_mode->yres;
     }
     else if (shared->modes) {
          *ret_width  = shared->modes->xres;
          *ret_height = shared->modes->yres;
     }
     else {
          D_WARN( "no current and no default mode" );
          return DFB_UNSUPPORTED;
     }

     return DFB_OK;
}

/******************************************************************************/

static int
primaryLayerDataSize()
{
     return 0;
}

static int
primaryRegionDataSize()
{
     return 0;
}

static DFBResult
primaryInitLayer( CoreLayer                  *layer,
                  void                       *driver_data,
                  void                       *layer_data,
                  DFBDisplayLayerDescription *description,
                  DFBDisplayLayerConfig      *config,
                  DFBColorAdjustment         *adjustment )
{
     DFBResult  ret;
     VideoMode *default_mode;

     /* initialize mode table */
     ret = init_modes();
     if (ret)
          return ret;

     default_mode = dfb_fbdev->shared->modes;

     /* set capabilities and type */
     description->caps = DLCAPS_SURFACE    | DLCAPS_CONTRAST |
                         DLCAPS_SATURATION | DLCAPS_BRIGHTNESS;
     description->type = DLTF_GRAPHICS;

     /* set name */
     snprintf( description->name,
               DFB_DISPLAY_LAYER_DESC_NAME_LENGTH, "FBDev Primary Layer" );

     /* fill out default color adjustment */
     adjustment->flags      = DCAF_BRIGHTNESS | DCAF_CONTRAST | DCAF_SATURATION;
     adjustment->brightness = 0x8000;
     adjustment->contrast   = 0x8000;
     adjustment->saturation = 0x8000;

     /* fill out the default configuration */
     config->flags      = DLCONF_WIDTH       | DLCONF_HEIGHT |
                          DLCONF_PIXELFORMAT | DLCONF_BUFFERMODE;
     config->buffermode = DLBM_FRONTONLY;
     config->width      = default_mode->xres;
     config->height     = default_mode->yres;

     if (dfb_config->mode.width || dfb_config->mode.height) {
          VideoMode *videomode = dfb_fbdev->shared->modes;

          while (videomode) {
               if (videomode->xres == dfb_config->mode.width &&
                   videomode->yres == dfb_config->mode.height)
               {
                    config->width  = dfb_config->mode.width;
                    config->height = dfb_config->mode.height;

                    break;
               }

               videomode = videomode->next;
          }

          if (!videomode) {
               D_ERROR( "DirectFB/FBDev: Specified mode (%dx%d) not supported or not defined in '/etc/fb.modes'!\n",
                        dfb_config->mode.width, dfb_config->mode.height );
               D_ERROR( "DirectFB/FBDev: Using default mode (%dx%d) instead!\n", config->width, config->height );
          }
     }

     if (dfb_config->mode.format != DSPF_UNKNOWN) {
          config->pixelformat = dfb_config->mode.format;
     }
     else if (dfb_config->mode.depth > 0) {
          DFBSurfacePixelFormat format = dfb_pixelformat_for_depth( dfb_config->mode.depth );

          if (format != DSPF_UNKNOWN)
               config->pixelformat = format;
          else
               D_ERROR("DirectFB/FBDev: Unknown depth (%d) specified!\n", dfb_config->mode.depth);
     }

     if (config->pixelformat == DSPF_UNKNOWN) {
          CoreLayerRegionConfig tmp;

          tmp.format     = DSPF_RGB16;
          tmp.buffermode = DLBM_FRONTONLY;

          if (dfb_fbdev_set_mode( NULL, NULL, &tmp ))
               config->pixelformat = dfb_pixelformat_for_depth( dfb_fbdev->shared->orig_var.bits_per_pixel );
          else
               config->pixelformat = DSPF_RGB16;
     }

     return DFB_OK;
}

static DFBResult
primarySetColorAdjustment( CoreLayer          *layer,
                           void               *driver_data,
                           void               *layer_data,
                           DFBColorAdjustment *adjustment )
{
     struct fb_cmap *cmap       = &dfb_fbdev->shared->current_cmap;
     struct fb_cmap *temp       = &dfb_fbdev->shared->temp_cmap;
     int             contrast   = adjustment->contrast >> 8;
     int             brightness = (adjustment->brightness >> 8) - 128;
     int             saturation = adjustment->saturation >> 8;
     int             r, g, b, i;

     if (dfb_fbdev->shared->fix.visual != FB_VISUAL_DIRECTCOLOR)
          return DFB_UNIMPLEMENTED;

     /* Use gamma ramp to set color attributes */
     for (i = 0; i < (int)cmap->len; i++) {
          r = cmap->red[i];
          g = cmap->green[i];
          b = cmap->blue[i];
          r >>= 8;
          g >>= 8;
          b >>= 8;

          /*
        * Brightness Adjustment: Increase/Decrease each color channels
        * by a constant amount as specified by value of brightness.
        */
          if (adjustment->flags & DCAF_BRIGHTNESS) {
               r += brightness;
               g += brightness;
               b += brightness;

               r = (r < 0) ? 0 : r;
               g = (g < 0) ? 0 : g;
               b = (b < 0) ? 0 : b;

               r = (r > 255) ? 255 : r;
               g = (g > 255) ? 255 : g;
               b = (b > 255) ? 255 : b;
          }

          /*
           * Contrast Adjustment:  We increase/decrease the "separation"
           * between colors in proportion to the value specified by the
           * contrast control. Decreasing the contrast has a side effect
           * of decreasing the brightness.
           */

          if (adjustment->flags & DCAF_CONTRAST) {
               /* Increase contrast */
               if (contrast > 128) {
                    int c = contrast - 128;

                    r = ((r + c/2)/c) * c;
                    g = ((g + c/2)/c) * c;
                    b = ((b + c/2)/c) * c;
               }
               /* Decrease contrast */
               else if (contrast < 127) {
                    float c = (float)contrast/128.0;

                    r = (int)((float)r * c);
                    g = (int)((float)g * c);
                    b = (int)((float)b * c);
               }
               r = (r < 0) ? 0 : r;
               g = (g < 0) ? 0 : g;
               b = (b < 0) ? 0 : b;

               r = (r > 255) ? 255 : r;
               g = (g > 255) ? 255 : g;
               b = (b > 255) ? 255 : b;
          }

          /*
           * Saturation Adjustment:  This is is a better implementation.
           * Saturation is implemented by "mixing" a proportion of medium
           * gray to the color value.  On the other side, "removing"
           * a proportion of medium gray oversaturates the color.
           */
          if (adjustment->flags & DCAF_SATURATION) {
               if (saturation > 128) {
                    float gray = ((float)saturation - 128.0)/128.0;
                    float color = 1.0 - gray;

                    r = (int)(((float)r - 128.0 * gray)/color);
                    g = (int)(((float)g - 128.0 * gray)/color);
                    b = (int)(((float)b - 128.0 * gray)/color);
               }
               else if (saturation < 128) {
                    float color = (float)saturation/128.0;
                    float gray = 1.0 - color;

                    r = (int)(((float) r * color) + (128.0 * gray));
                    g = (int)(((float) g * color) + (128.0 * gray));
                    b = (int)(((float) b * color) + (128.0 * gray));
               }

               r = (r < 0) ? 0 : r;
               g = (g < 0) ? 0 : g;
               b = (b < 0) ? 0 : b;

               r = (r > 255) ? 255 : r;
               g = (g > 255) ? 255 : g;
               b = (b > 255) ? 255 : b;
          }
          r |= r << 8;
          g |= g << 8;
          b |= b << 8;

          temp->red[i]   =  (unsigned short)r;
          temp->green[i] =  (unsigned short)g;
          temp->blue[i]  =  (unsigned short)b;
     }

     temp->len = cmap->len;
     temp->start = cmap->start;
     if (FBDEV_IOCTL( FBIOPUTCMAP, temp ) < 0) {
          D_PERROR( "DirectFB/FBDev: Could not set the palette!\n" );

          return errno2result(errno);
     }

     return DFB_OK;
}

static DFBResult
primaryTestRegion( CoreLayer                  *layer,
                   void                       *driver_data,
                   void                       *layer_data,
                   CoreLayerRegionConfig      *config,
                   CoreLayerRegionConfigFlags *failed )
{
     VideoMode                  *videomode = NULL;
     CoreLayerRegionConfigFlags  fail = 0;

     videomode = dfb_fbdev->shared->modes;
     while (videomode) {
          if (videomode->xres == config->width  &&
              videomode->yres == config->height)
               break;

          videomode = videomode->next;
     }

     if (!videomode || dfb_fbdev_set_mode( NULL, videomode, config ))
          fail |= CLRCF_WIDTH | CLRCF_HEIGHT | CLRCF_FORMAT | CLRCF_BUFFERMODE;

     if (config->options)
          fail |= CLRCF_OPTIONS;

     if (failed)
          *failed = fail;

     if (fail)
          return DFB_UNSUPPORTED;

     return DFB_OK;
}

static DFBResult
primaryAddRegion( CoreLayer             *layer,
                  void                  *driver_data,
                  void                  *layer_data,
                  void                  *region_data,
                  CoreLayerRegionConfig *config )
{
     return DFB_OK;
}

static DFBResult
primarySetRegion( CoreLayer                  *layer,
                  void                       *driver_data,
                  void                       *layer_data,
                  void                       *region_data,
                  CoreLayerRegionConfig      *config,
                  CoreLayerRegionConfigFlags  updated,
                  CoreSurface                *surface,
                  CorePalette                *palette )
{
     DFBResult  ret;
     VideoMode *videomode;

     videomode = dfb_fbdev->shared->modes;
     while (videomode) {
          if (videomode->xres == config->width  &&
              videomode->yres == config->height)
               break;

          videomode = videomode->next;
     }

     if (!videomode)
          return DFB_UNSUPPORTED;

     if (updated & (CLRCF_BUFFERMODE | CLRCF_FORMAT | CLRCF_HEIGHT | CLRCF_SURFACE | CLRCF_WIDTH)) {
          ret = dfb_fbdev_set_mode( surface, videomode, config );
          if (ret)
               return ret;
     }

     if ((updated & CLRCF_PALETTE) && palette)
          dfb_fbdev_set_palette( palette );

     return DFB_OK;
}

static DFBResult
primaryRemoveRegion( CoreLayer             *layer,
                     void                  *driver_data,
                     void                  *layer_data,
                     void                  *region_data )
{
     return DFB_OK;
}

static DFBResult
primaryFlipRegion( CoreLayer           *layer,
                   void                *driver_data,
                   void                *layer_data,
                   void                *region_data,
                   CoreSurface         *surface,
                   DFBSurfaceFlipFlags  flags )
{
     DFBResult ret;

     if (((flags & DSFLIP_WAITFORSYNC) == DSFLIP_WAITFORSYNC) &&
         !dfb_config->pollvsync_after)
          dfb_screen_wait_vsync( dfb_screens_at(DSCID_PRIMARY) );

     ret = dfb_fbdev_pan( surface->back_buffer->video.offset /
                          surface->back_buffer->video.pitch,
                          (flags & DSFLIP_WAITFORSYNC) == DSFLIP_ONSYNC );
     if (ret)
          return ret;

     if ((flags & DSFLIP_WAIT) &&
         (dfb_config->pollvsync_after || !(flags & DSFLIP_ONSYNC)))
          dfb_screen_wait_vsync( dfb_screens_at(DSCID_PRIMARY) );

     dfb_surface_flip_buffers( surface, false );

     return DFB_OK;
}

static DFBResult
primaryAllocateSurface( CoreLayer              *layer,
                        void                   *driver_data,
                        void                   *layer_data,
                        void                   *region_data,
                        CoreLayerRegionConfig  *config,
                        CoreSurface           **ret_surface )
{
     DFBResult               ret;
     CoreSurface            *surface;
     DFBSurfaceCapabilities  caps = DSCAPS_VIDEOONLY;

     /* determine further capabilities */
     if (config->buffermode == DLBM_TRIPLE)
          caps |= DSCAPS_TRIPLE;
     else if (config->buffermode != DLBM_FRONTONLY)
          caps |= DSCAPS_DOUBLE;

     /* allocate surface object */
     surface = dfb_core_create_surface( dfb_fbdev->core );
     if (!surface)
          return DFB_FAILURE;

     /* reallocation just needs an allocated buffer structure */
     surface->idle_buffer = surface->back_buffer =
                            surface->front_buffer = SHCALLOC( 1, sizeof(SurfaceBuffer) );

     if (!surface->front_buffer) {
          fusion_object_destroy( &surface->object );
          return DFB_NOSYSTEMMEMORY;
     }

     /* initialize surface structure */
     ret = dfb_surface_init( dfb_fbdev->core, surface,
                             config->width, config->height,
                             config->format, caps, NULL );
     if (ret) {
          SHFREE( surface->front_buffer );
          fusion_object_destroy( &surface->object );
          return ret;
     }

     /* activate object */
     fusion_object_activate( &surface->object );

     /* return surface */
     *ret_surface = surface;

     return DFB_OK;
}

static DFBResult
primaryReallocateSurface( CoreLayer             *layer,
                          void                  *driver_data,
                          void                  *layer_data,
                          void                  *region_data,
                          CoreLayerRegionConfig *config,
                          CoreSurface           *surface )
{
     /* reallocation is done during SetConfiguration,
        because the pitch can only be determined AFTER setting the mode */
     if (DFB_PIXELFORMAT_IS_INDEXED(config->format) && !surface->palette) {
          DFBResult    ret;
          CorePalette *palette;

          ret = dfb_palette_create( dfb_fbdev->core,
                                    1 << DFB_COLOR_BITS_PER_PIXEL( config->format ),
                                    &palette );
          if (ret)
               return ret;

          if (config->format == DSPF_LUT8)
               dfb_palette_generate_rgb332_map( palette );

          dfb_surface_set_palette( surface, palette );

          dfb_palette_unref( palette );
     }

     return DFB_OK;
}


/** fbdev internal **/

static int dfb_fbdev_compatible_format( struct fb_var_screeninfo *var,
                                        int al, int rl, int gl, int bl,
                                        int ao, int ro, int go, int bo )
{
     int ah, rh, gh, bh;
     int vah, vrh, vgh, vbh;

     ah = al + ao - 1;
     rh = rl + ro - 1;
     gh = gl + go - 1;
     bh = bl + bo - 1;

     vah = var->transp.length + var->transp.offset - 1;
     vrh = var->red.length + var->red.offset - 1;
     vgh = var->green.length + var->green.offset - 1;
     vbh = var->blue.length + var->blue.offset - 1;

     if (ah == vah && al >= (int)var->transp.length &&
         rh == vrh && rl >= (int)var->red.length &&
         gh == vgh && gl >= (int)var->green.length &&
         bh == vbh && bl >= (int)var->blue.length)
          return 1;

     return 0;
}

static DFBSurfacePixelFormat dfb_fbdev_get_pixelformat( struct fb_var_screeninfo *var )
{
     switch (var->bits_per_pixel) {

          case 8:
/*
               This check is omitted, since we want to use RGB332 even if the
               hardware uses a palette (in that case we initialize a calculated
               one to have correct colors)

               if (fbdev_compatible_format( var, 0, 3, 3, 2, 0, 5, 2, 0 ))*/

               return DSPF_RGB332;

          case 15:
               if (dfb_fbdev_compatible_format( var, 0, 5, 5, 5, 0, 10, 5, 0 ) |
                   dfb_fbdev_compatible_format( var, 1, 5, 5, 5,15, 10, 5, 0 ) )
                    return DSPF_ARGB1555;

               break;

          case 16:
               if (dfb_fbdev_compatible_format( var, 0, 5, 5, 5, 0, 10, 5, 0 ) |
                   dfb_fbdev_compatible_format( var, 1, 5, 5, 5,15, 10, 5, 0 ) )
                    return DSPF_ARGB1555;

               if (dfb_fbdev_compatible_format( var, 0, 5, 6, 5, 0, 11, 5, 0 ))
                    return DSPF_RGB16;

               break;

          case 24:
               if (dfb_fbdev_compatible_format( var, 0, 8, 8, 8, 0, 16, 8, 0 ))
                    return DSPF_RGB24;

               break;

          case 32:
               if (dfb_fbdev_compatible_format( var, 0, 8, 8, 8, 0, 16, 8, 0 ))
                    return DSPF_RGB32;

               if (dfb_fbdev_compatible_format( var, 8, 8, 8, 8, 24, 16, 8, 0 ))
                    return DSPF_ARGB;

               break;
     }

     D_ERROR( "DirectFB/FBDev: Unsupported pixelformat: "
               "rgba %d/%d, %d/%d, %d/%d, %d/%d (%dbit)\n",
               var->red.length,    var->red.offset,
               var->green.length,  var->green.offset,
               var->blue.length,   var->blue.offset,
               var->transp.length, var->transp.offset,
               var->bits_per_pixel );

     return DSPF_UNKNOWN;
}

/*
 * pans display (flips buffer) using fbdev ioctl
 */
static DFBResult dfb_fbdev_pan( int offset, bool onsync )
{
     struct fb_var_screeninfo *var;

     var = &dfb_fbdev->shared->current_var;

     if (var->yres_virtual < offset + var->yres) {
          D_ERROR( "DirectFB/FBDev: yres %d, vyres %d, offset %d\n",
                    var->yres, var->yres_virtual, offset );
          D_BUG( "panning buffer out of range" );
          return DFB_BUG;
     }

     var->xoffset = 0;
     var->yoffset = offset;
     var->activate = onsync ? FB_ACTIVATE_VBL : FB_ACTIVATE_NOW;

     dfb_gfxcard_sync();

     if (FBDEV_IOCTL( FBIOPAN_DISPLAY, var ) < 0) {
          int erno = errno;

          D_PERROR( "DirectFB/FBDev: Panning display failed!\n" );

          return errno2result( erno );
     }

     return DFB_OK;
}

/*
 * blanks display using fbdev ioctl
 */
static DFBResult dfb_fbdev_blank( int level )
{
     if (ioctl( dfb_fbdev->fd, FBIOBLANK, level ) < 0) {
          D_PERROR( "DirectFB/FBDev: Display blanking failed!\n" );

          return errno2result( errno );
     }

     return DFB_OK;
}

/*
 * sets (if surface != NULL) or tests (if surface == NULL) video mode,
 * sets virtual y-resolution according to buffermode
 */
static DFBResult dfb_fbdev_set_mode( CoreSurface           *surface,
                                     VideoMode             *mode,
                                     CoreLayerRegionConfig *config )
{
     unsigned int              vyres;
     struct fb_var_screeninfo  var;
     FBDevShared              *shared = dfb_fbdev->shared;

     D_DEBUG("DirectFB/FBDev: dfb_fbdev_set_mode (surface: %p, "
              "mode: %p, buffermode: %d)\n", surface, mode,
              config ? config->buffermode : DLBM_FRONTONLY);

     if (!mode)
          mode = shared->current_mode ? shared->current_mode : shared->modes;

     vyres = mode->yres;

     var = shared->current_var;

     var.xoffset = 0;
     var.yoffset = 0;

     if (config) {
          switch (config->buffermode) {
               case DLBM_TRIPLE:
                    vyres *= 3;
                    break;

               case DLBM_BACKVIDEO:
                    vyres *= 2;
                    break;

               case DLBM_BACKSYSTEM:
               case DLBM_FRONTONLY:
                    break;

               default:
                    return DFB_UNSUPPORTED;
          }

          var.bits_per_pixel = DFB_BYTES_PER_PIXEL(config->format) * 8;

          var.transp.length = var.transp.offset = 0;

          switch (config->format) {
               case DSPF_ARGB1555:
                    var.transp.length = 1;
                    var.red.length    = 5;
                    var.green.length  = 5;
                    var.blue.length   = 5;
                    var.transp.offset = 15;
                    var.red.offset    = 10;
                    var.green.offset  = 5;
                    var.blue.offset   = 0;
                    break;

               case DSPF_RGB16:
                    var.red.length    = 5;
                    var.green.length  = 6;
                    var.blue.length   = 5;
                    var.red.offset    = 11;
                    var.green.offset  = 5;
                    var.blue.offset   = 0;
                    break;

               case DSPF_ARGB:
               case DSPF_AiRGB:
                    var.transp.length = 8;
                    var.red.length    = 8;
                    var.green.length  = 8;
                    var.blue.length   = 8;
                    var.transp.offset = 24;
                    var.red.offset    = 16;
                    var.green.offset  = 8;
                    var.blue.offset   = 0;
                    break;

               case DSPF_LUT8:
               case DSPF_RGB24:
               case DSPF_RGB32:
               case DSPF_RGB332:
                    break;

               default:
                    return DFB_UNSUPPORTED;
          }
     }
     else
          var.bits_per_pixel = mode->bpp;

     var.activate = surface ? FB_ACTIVATE_NOW : FB_ACTIVATE_TEST;

     var.xres = mode->xres;
     var.yres = mode->yres;
     var.xres_virtual = mode->xres;
     var.yres_virtual = vyres;

     var.pixclock = mode->pixclock;
     var.left_margin = mode->left_margin;
     var.right_margin = mode->right_margin;
     var.upper_margin = mode->upper_margin;
     var.lower_margin = mode->lower_margin;
     var.hsync_len = mode->hsync_len;
     var.vsync_len = mode->vsync_len;

     var.sync = 0;
     if (mode->hsync_high)
          var.sync |= FB_SYNC_HOR_HIGH_ACT;
     if (mode->vsync_high)
          var.sync |= FB_SYNC_VERT_HIGH_ACT;
     if (mode->csync_high)
          var.sync |= FB_SYNC_COMP_HIGH_ACT;
     if (mode->sync_on_green)
          var.sync |= FB_SYNC_ON_GREEN;
     if (mode->external_sync)
          var.sync |= FB_SYNC_EXT;
     if (mode->broadcast)
          var.sync |= FB_SYNC_BROADCAST;

     var.vmode = 0;
     if (mode->laced)
          var.vmode |= FB_VMODE_INTERLACED;
     if (mode->doubled)
          var.vmode |= FB_VMODE_DOUBLE;

     dfb_gfxcard_lock( GDLF_WAIT | GDLF_SYNC | GDLF_RESET | GDLF_INVALIDATE );

     if (FBDEV_IOCTL( FBIOPUT_VSCREENINFO, &var ) < 0) {
          int erno = errno;

          if (surface)
               D_PERROR( "DirectFB/FBDev: "
                          "Could not set video mode (FBIOPUT_VSCREENINFO)!\n" );

          dfb_gfxcard_unlock();

          return errno2result( erno );
     }

     /*
      * the video mode was set successfully, check if there is enough
      * video ram (for buggy framebuffer drivers)
      */

     if (shared->fix.smem_len < (var.yres_virtual *
                                 var.xres_virtual *
                                 var.bits_per_pixel >> 3))
     {
          if (surface) {
               D_PERROR( "DirectFB/FBDev: "
                          "Could not set video mode (not enough video ram)!\n" );

               /* restore mode */
               FBDEV_IOCTL( FBIOPUT_VSCREENINFO, &shared->current_var );
          }

          dfb_gfxcard_unlock();

          return DFB_INVARG;
     }

     /* If surface is NULL the mode was only tested, otherwise apply changes. */
     if (surface) {
          struct fb_fix_screeninfo fix;
          DFBSurfacePixelFormat    format;

          FBDEV_IOCTL( FBIOGET_VSCREENINFO, &var );


          format = dfb_fbdev_get_pixelformat( &var );
          if (format == DSPF_UNKNOWN || var.yres_virtual < vyres) {
               D_WARN( "fbdev driver possibly buggy" );

               /* restore mode */
               FBDEV_IOCTL( FBIOPUT_VSCREENINFO, &shared->current_var );

               dfb_gfxcard_unlock();

               return DFB_UNSUPPORTED;
          }

          if (!config) {
               dfb_gfxcard_unlock();

               return DFB_OK;
          }

          if (format != config->format) {
               if (DFB_BYTES_PER_PIXEL(format) == 1 ||
                   (format == DSPF_ARGB && config->format == DSPF_AiRGB))
                    format = config->format;
          }

          if (config->format == DSPF_RGB332)
               dfb_fbdev_set_rgb332_palette();
          else
               dfb_fbdev_set_gamma_ramp( config->format );

          shared->current_var = var;
          shared->current_mode = mode;

          surface->width  = mode->xres;
          surface->height = mode->yres;
          surface->format = format;

          /* To get the new pitch */
          FBDEV_IOCTL( FBIOGET_FSCREENINFO, &fix );

          /* ++Tony: Other information (such as visual formats) will also change */
          shared->fix = fix;

          dfb_gfxcard_adjust_heap_offset( var.yres_virtual * fix.line_length );

          surface->front_buffer->surface = surface;
          surface->front_buffer->policy = CSP_VIDEOONLY;
          surface->front_buffer->format = format;
          surface->front_buffer->video.health = CSH_STORED;
          surface->front_buffer->video.pitch = fix.line_length;
          surface->front_buffer->video.offset = 0;

          switch (config->buffermode) {
               case DLBM_FRONTONLY:
                    surface->caps &= ~DSCAPS_FLIPPING;

                    if (surface->back_buffer != surface->front_buffer) {
                         if (surface->back_buffer->system.addr)
                              SHFREE( surface->back_buffer->system.addr );

                         SHFREE( surface->back_buffer );

                         surface->back_buffer = surface->front_buffer;
                    }

                    if (surface->idle_buffer != surface->front_buffer) {
                         if (surface->idle_buffer->system.addr)
                              SHFREE( surface->idle_buffer->system.addr );

                         SHFREE( surface->idle_buffer );

                         surface->idle_buffer = surface->front_buffer;
                    }
                    break;
               case DLBM_BACKVIDEO:
                    surface->caps |= DSCAPS_DOUBLE;
                    surface->caps &= ~DSCAPS_TRIPLE;

                    if (surface->back_buffer == surface->front_buffer) {
                         surface->back_buffer = SHCALLOC( 1, sizeof(SurfaceBuffer) );
                    }
                    else {
                         if (surface->back_buffer->system.addr) {
                              SHFREE( surface->back_buffer->system.addr );
                              surface->back_buffer->system.addr = NULL;
                         }

                         surface->back_buffer->system.health = CSH_INVALID;
                    }
                    surface->back_buffer->surface = surface;
                    surface->back_buffer->policy = CSP_VIDEOONLY;
                    surface->back_buffer->format = format;
                    surface->back_buffer->video.health = CSH_STORED;
                    surface->back_buffer->video.pitch = fix.line_length;
                    surface->back_buffer->video.offset =
                         surface->back_buffer->video.pitch * var.yres;

                    if (surface->idle_buffer != surface->front_buffer) {
                         if (surface->idle_buffer->system.addr)
                              SHFREE( surface->idle_buffer->system.addr );

                         SHFREE( surface->idle_buffer );

                         surface->idle_buffer = surface->front_buffer;
                    }
                    break;
               case DLBM_TRIPLE:
                    surface->caps |= DSCAPS_TRIPLE;
                    surface->caps &= ~DSCAPS_DOUBLE;

                    if (surface->back_buffer == surface->front_buffer) {
                         surface->back_buffer = SHCALLOC( 1, sizeof(SurfaceBuffer) );
                    }
                    else {
                         if (surface->back_buffer->system.addr) {
                              SHFREE( surface->back_buffer->system.addr );
                              surface->back_buffer->system.addr = NULL;
                         }

                         surface->back_buffer->system.health = CSH_INVALID;
                    }
                    surface->back_buffer->surface = surface;
                    surface->back_buffer->policy = CSP_VIDEOONLY;
                    surface->back_buffer->format = format;
                    surface->back_buffer->video.health = CSH_STORED;
                    surface->back_buffer->video.pitch = fix.line_length;
                    surface->back_buffer->video.offset =
                         surface->back_buffer->video.pitch * var.yres;

                    if (surface->idle_buffer == surface->front_buffer) {
                         surface->idle_buffer = SHCALLOC( 1, sizeof(SurfaceBuffer) );
                    }
                    else {
                         if (surface->idle_buffer->system.addr) {
                              SHFREE( surface->idle_buffer->system.addr );
                              surface->idle_buffer->system.addr = NULL;
                         }

                         surface->idle_buffer->system.health = CSH_INVALID;
                    }
                    surface->idle_buffer->surface = surface;
                    surface->idle_buffer->policy = CSP_VIDEOONLY;
                    surface->idle_buffer->format = format;
                    surface->idle_buffer->video.health = CSH_STORED;
                    surface->idle_buffer->video.pitch = fix.line_length;
                    surface->idle_buffer->video.offset =
                         surface->idle_buffer->video.pitch * var.yres * 2;
                    break;
               case DLBM_BACKSYSTEM:
                    surface->caps |= DSCAPS_DOUBLE;
                    surface->caps &= ~DSCAPS_TRIPLE;

                    if (surface->back_buffer == surface->front_buffer) {
                         surface->back_buffer = SHCALLOC( 1, sizeof(SurfaceBuffer) );
                    }
                    surface->back_buffer->surface = surface;
                    surface->back_buffer->policy = CSP_SYSTEMONLY;
                    surface->back_buffer->format = format;
                    surface->back_buffer->video.health = CSH_INVALID;
                    surface->back_buffer->system.health = CSH_STORED;
                    surface->back_buffer->system.pitch =
                         (DFB_BYTES_PER_LINE(format, var.xres) + 3) & ~3;

                    if (surface->back_buffer->system.addr)
                         SHFREE( surface->back_buffer->system.addr );

                    surface->back_buffer->system.addr =
                         SHMALLOC( surface->back_buffer->system.pitch * var.yres );

                    if (surface->idle_buffer != surface->front_buffer) {
                         if (surface->idle_buffer->system.addr)
                              SHFREE( surface->idle_buffer->system.addr );

                         SHFREE( surface->idle_buffer );

                         surface->idle_buffer = surface->front_buffer;
                    }
                    break;
               default:
                    D_BUG( "unexpected buffer mode" );
                    break;
          }

          dfb_fbdev_pan( 0, false );

          dfb_gfxcard_after_set_var();

          dfb_surface_notify_listeners( surface,
                                        CSNF_SIZEFORMAT | CSNF_FLIP |
                                        CSNF_VIDEO      | CSNF_SYSTEM );
     }

     dfb_gfxcard_unlock();

     return DFB_OK;
}

/*
 * parses video modes in /etc/fb.modes and stores them in dfb_fbdev->shared->modes
 * (to be replaced by DirectFB's own config system
 */
static DFBResult dfb_fbdev_read_modes()
{
     FILE *fp;
     char line[80],label[32],value[16];
     int geometry=0, timings=0;
     int dummy;
     VideoMode temp_mode;
     VideoMode *m = dfb_fbdev->shared->modes;

     if (!(fp = fopen("/etc/fb.modes","r")))
          return errno2result( errno );

     while (fgets(line,79,fp)) {
          if (sscanf(line, "mode \"%31[^\"]\"",label) == 1) {
               memset( &temp_mode, 0, sizeof(VideoMode) );
               geometry = 0;
               timings = 0;
               while (fgets(line,79,fp) && !(strstr(line,"endmode"))) {
                    if (5 == sscanf(line," geometry %d %d %d %d %d", &temp_mode.xres, &temp_mode.yres, &dummy, &dummy, &temp_mode.bpp)) {
                         geometry = 1;
                    }
                    else if (7 == sscanf(line," timings %d %d %d %d %d %d %d", &temp_mode.pixclock, &temp_mode.left_margin,  &temp_mode.right_margin,
                                         &temp_mode.upper_margin, &temp_mode.lower_margin, &temp_mode.hsync_len,    &temp_mode.vsync_len)) {
                         timings = 1;
                    }
                    else if (1 == sscanf(line, " hsync %15s",value) && 0 == strcasecmp(value,"high")) {
                         temp_mode.hsync_high = 1;
                    }
                    else if (1 == sscanf(line, " vsync %15s",value) && 0 == strcasecmp(value,"high")) {
                         temp_mode.vsync_high = 1;
                    }
                    else if (1 == sscanf(line, " csync %15s",value) && 0 == strcasecmp(value,"high")) {
                         temp_mode.csync_high = 1;
                    }
                    else if (1 == sscanf(line, " laced %15s",value) && 0 == strcasecmp(value,"true")) {
                         temp_mode.laced = 1;
                    }
                    else if (1 == sscanf(line, " double %15s",value) && 0 == strcasecmp(value,"true")) {
                         temp_mode.doubled = 1;
                    }
                    else if (1 == sscanf(line, " gsync %15s",value) && 0 == strcasecmp(value,"true")) {
                         temp_mode.sync_on_green = 1;
                    }
                    else if (1 == sscanf(line, " extsync %15s",value) && 0 == strcasecmp(value,"true")) {
                         temp_mode.external_sync = 1;
                    }
                    else if (1 == sscanf(line, " bcast %15s",value) && 0 == strcasecmp(value,"true")) {
                         temp_mode.broadcast = 1;
                    }
               }
               if (geometry &&
                   timings &&
                   !dfb_fbdev_set_mode(NULL, &temp_mode, NULL)) {
                    if (!m) {
                         dfb_fbdev->shared->modes = SHCALLOC(1, sizeof(VideoMode));
                         m = dfb_fbdev->shared->modes;
                    }
                    else {
                         m->next = SHCALLOC(1, sizeof(VideoMode));
                         m = m->next;
                    }
                    direct_memcpy (m, &temp_mode, sizeof(VideoMode));
                    D_DEBUG( "DirectFB/FBDev: %20s %4dx%4d  %s%s\n", label, temp_mode.xres, temp_mode.yres,
                              temp_mode.laced ? "interlaced " : "", temp_mode.doubled ? "doublescan" : "" );
               }
          }
     }

     fclose (fp);

     return DFB_OK;
}

/*
 * some fbdev drivers use the palette as gamma ramp in >8bpp modes, to have
 * correct colors, the gamme ramp has to be initialized.
 */

static __u16 dfb_fbdev_calc_gamma(int n, int max)
{
     int ret = 65535.0 * ((float)((float)n/(max)));
     if (ret > 65535) ret = 65535;
     if (ret <     0) ret =     0;
     return ret;
}

static DFBResult dfb_fbdev_set_gamma_ramp( DFBSurfacePixelFormat format )
{
     int i;

     int red_size   = 0;
     int green_size = 0;
     int blue_size  = 0;
     int red_max    = 0;
     int green_max  = 0;
     int blue_max   = 0;

     struct fb_cmap *cmap;

     if (!dfb_fbdev) {
          D_BUG( "dfb_fbdev_set_gamma_ramp() called while dfb_fbdev == NULL!" );

          return DFB_BUG;
     }

     switch (format) {
          case DSPF_ARGB1555:
               red_size   = 32;
               green_size = 32;
               blue_size  = 32;
               break;
          case DSPF_RGB16:
               red_size   = 32;
               green_size = 64;
               blue_size  = 32;
               break;
          case DSPF_RGB24:
          case DSPF_RGB32:
          case DSPF_ARGB:
               red_size   = 256;
               green_size = 256;
               blue_size  = 256;
               break;
          default:
               return DFB_OK;
     }

     /*
      * ++Tony: The gamma ramp must be set differently if in DirectColor,
      *         ie, to mimic TrueColor, index == color[index].
      */
     if (dfb_fbdev->shared->fix.visual == FB_VISUAL_DIRECTCOLOR) {
          red_max   = 65536 / (256/red_size);
          green_max = 65536 / (256/green_size);
          blue_max  = 65536 / (256/blue_size);
     }
     else {
          red_max   = red_size;
          green_max = green_size;
          blue_max  = blue_size;
     }

     cmap = &dfb_fbdev->shared->current_cmap;

     /* assume green to have most weight */
     cmap->len = green_size;

     for (i = 0; i < red_size; i++)
          cmap->red[i] = dfb_fbdev_calc_gamma( i, red_max );

     for (i = 0; i < green_size; i++)
          cmap->green[i] = dfb_fbdev_calc_gamma( i, green_max );

     for (i = 0; i < blue_size; i++)
          cmap->blue[i] = dfb_fbdev_calc_gamma( i, blue_max );

     /* ++Tony: Some drivers use the upper byte, some use the lower */
     if (dfb_fbdev->shared->fix.visual == FB_VISUAL_DIRECTCOLOR) {
          for (i = 0; i < red_size; i++)
               cmap->red[i] |= cmap->red[i] << 8;

          for (i = 0; i < green_size; i++)
               cmap->green[i] |= cmap->green[i] << 8;

          for (i = 0; i < blue_size; i++)
               cmap->blue[i] |= cmap->blue[i] << 8;
     }

     if (FBDEV_IOCTL( FBIOPUTCMAP, cmap ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                     "Could not set gamma ramp" );

          return errno2result(errno);
     }

     return DFB_OK;
}

static DFBResult
dfb_fbdev_set_palette( CorePalette *palette )
{
     int             i;
     struct fb_cmap *cmap = &dfb_fbdev->shared->current_cmap;

     D_ASSERT( palette != NULL );

     cmap->len = palette->num_entries <= 256 ? palette->num_entries : 256;

     for (i = 0; i < (int)cmap->len; i++) {
          cmap->red[i]     = palette->entries[i].r;
          cmap->green[i]   = palette->entries[i].g;
          cmap->blue[i]    = palette->entries[i].b;
          cmap->transp[i]  = 0xff - palette->entries[i].a;

          cmap->red[i]    |= cmap->red[i] << 8;
          cmap->green[i]  |= cmap->green[i] << 8;
          cmap->blue[i]   |= cmap->blue[i] << 8;
          cmap->transp[i] |= cmap->transp[i] << 8;
     }

     if (FBDEV_IOCTL( FBIOPUTCMAP, cmap ) < 0) {
          D_PERROR( "DirectFB/FBDev: Could not set the palette!\n" );

          return errno2result(errno);
     }

     return DFB_OK;
}

static DFBResult dfb_fbdev_set_rgb332_palette()
{
     int red_val;
     int green_val;
     int blue_val;
     int i = 0;

     struct fb_cmap cmap;

     if (!dfb_fbdev) {
          D_BUG( "dfb_fbdev_set_rgb332_palette() called while dfb_fbdev == NULL!" );

          return DFB_BUG;
     }

     cmap.start  = 0;
     cmap.len    = 256;
     cmap.red    = (__u16*)SHMALLOC( 2 * 256 );
     cmap.green  = (__u16*)SHMALLOC( 2 * 256 );
     cmap.blue   = (__u16*)SHMALLOC( 2 * 256 );
     cmap.transp = (__u16*)SHMALLOC( 2 * 256 );


     for (red_val = 0; red_val  < 8 ; red_val++) {
          for (green_val = 0; green_val  < 8 ; green_val++) {
               for (blue_val = 0; blue_val  < 4 ; blue_val++) {
                    cmap.red[i]    = dfb_fbdev_calc_gamma( red_val, 7 );
                    cmap.green[i]  = dfb_fbdev_calc_gamma( green_val, 7 );
                    cmap.blue[i]   = dfb_fbdev_calc_gamma( blue_val, 3 );
                    cmap.transp[i] = (i ? 0x2000 : 0xffff);
                    i++;
               }
          }
     }

     if (FBDEV_IOCTL( FBIOPUTCMAP, &cmap ) < 0) {
          D_PERROR( "DirectFB/FBDev: "
                     "Could not set rgb332 palette" );

          SHFREE( cmap.red );
          SHFREE( cmap.green );
          SHFREE( cmap.blue );
          SHFREE( cmap.transp );

          return errno2result(errno);
     }

     SHFREE( cmap.red );
     SHFREE( cmap.green );
     SHFREE( cmap.blue );
     SHFREE( cmap.transp );

     return DFB_OK;
}

static int
fbdev_ioctl_call_handler( int   caller,
                          int   call_arg,
                          void *call_ptr,
                          void *ctx )
{
     int        ret;
     const char cursoroff_str[] = "\033[?1;0;0c";
     const char blankoff_str[] = "\033[9;0]";

     if (dfb_config->vt) {
          if (!dfb_config->kd_graphics && call_arg == FBIOPUT_VSCREENINFO)
               ioctl( dfb_fbdev->vt->fd, KDSETMODE, KD_GRAPHICS );
     }

     ret = ioctl( dfb_fbdev->fd, call_arg, call_ptr );

     if (dfb_config->vt) {
          if (call_arg == FBIOPUT_VSCREENINFO) {
               if (!dfb_config->kd_graphics)
                    ioctl( dfb_fbdev->vt->fd, KDSETMODE, KD_TEXT );

               write( dfb_fbdev->vt->fd, cursoroff_str, strlen(cursoroff_str) );
               write( dfb_fbdev->vt->fd, blankoff_str, strlen(blankoff_str) );
          }
     }

     return ret;
}

static int
fbdev_ioctl( int request, void *arg, int arg_size )
{
     DirectResult  ret;
     int           erno;
     void         *tmp_shm = NULL;

     D_ASSERT( dfb_fbdev != NULL );
     D_ASSERT( dfb_fbdev->shared != NULL );

     if (dfb_core_is_master( dfb_fbdev->core ))
          return fbdev_ioctl_call_handler( 1, request, arg, NULL );

     if (arg) {
          if (!fusion_is_shared( arg )) {
               tmp_shm = SHMALLOC( arg_size );
               if (!tmp_shm) {
                    errno = ENOMEM;
                    return -1;
               }

               direct_memcpy( tmp_shm, arg, arg_size );
          }
     }

     ret = fusion_call_execute( &dfb_fbdev->shared->fbdev_ioctl,
                                request, tmp_shm ? tmp_shm : arg, &erno );

     if (tmp_shm) {
          direct_memcpy( arg, tmp_shm, arg_size );
          SHFREE( tmp_shm );
     }

     errno = erno;

     return errno ? -1 : 0;
}

