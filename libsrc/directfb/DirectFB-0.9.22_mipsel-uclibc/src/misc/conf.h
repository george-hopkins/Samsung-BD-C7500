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

#ifndef __CONF_H__
#define __CONF_H__

#include <signal.h>

#include <directfb.h>
#include <fusion/types.h>

typedef struct
{
     DFBDisplayLayerBackgroundMode layer_bg_mode; /* background mode for
                                                     primary layer */
     DFBColor  layer_bg_color;                    /* background color for
                                                     primary layer */
     char     *layer_bg_filename;                 /* background image for
                                                     primary layer */

     bool      mouse_motion_compression;          /* use motion compression? */
     char     *mouse_protocol;                    /* mouse protocol */
     char     *mouse_source;                      /* mouse source device name */
     bool      mouse_gpm_source;                  /* mouse source is gpm? */

     int       window_policy;                     /* swapping policy for the
                                                     surface of a window */
     int       buffer_mode;                       /* default buffer mode for
                                                     primary layer */

     bool      pollvsync_after;
     bool      pollvsync_none;
     bool      software_only;                     /* disable hardware
                                                     acceleration */

     bool      mmx;                               /* mmx support */

     bool      banner;                            /* startup banner */

     bool      force_windowed;                    /* prohibit exclusive modes */

     bool      deinit_check;

     bool      vt_switch;                         /* allocate a new VT */
     bool      kd_graphics;                       /* put terminal into graphics
                                                     mode */

     bool      matrox_ntsc;                       /* Matrox TV standard */
     int       matrox_cable;                      /* Matrox cable type */
     bool      matrox_sgram;                      /* Use Matrox SGRAM features */
     bool      matrox_crtc2;                      /* Experimental CRTC2 stuff */

     bool      sync;                              /* Do sync() in core_init() */
     bool      vt_switching;                      /* Allow VT switching by
                                                     pressing Ctrl+Alt+<F?> */

     char     *fb_device;                         /* Used framebuffer device,
                                                     e.g. "/dev/fb0" */

     bool      lefty;                             /* Left handed mouse, swaps
                                                     left/right mouse buttons */
     bool      no_cursor;                         /* Never create a cursor */
     bool      translucent_windows;               /* Allow translucent
                                                     windows */

     struct {
          int                   width;            /* primary layer width */
          int                   height;           /* primary layer height */
          int                   depth;            /* primary layer depth */
          DFBSurfacePixelFormat format;           /* primary layer format */
     } mode;

     int       videoram_limit;                    /* limit amount of video
                                                     memory used by DirectFB */

     char     *screenshot_dir;                    /* dump screen content into
                                                     this directory */

     char     *system;                            /* FBDev, SDL, etc. */

     bool      capslock_meta;                     /* map CapsLock -> Meta */

     bool      block_all_signals;                 /* block all signals */

     int       session;                           /* select multi app world */

     int       primary_layer;                     /* select alternative primary
                                                     display layer */

     bool      force_desktop;                     /* Desktop background is
                                                     the primary surface. */

     bool      linux_input_ir_only;               /* Ignore non-IR devices. */

     struct {
          char *host;                             /* Remote host to connect to. */
          int   session;                          /* Remote session number. */
     } remote;

     char      *wm;                               /* Window manager to use. */

     bool       vt;                               /* Use VT stuff at all? */

     bool       decorations;                      /* Enable window decorations. */

     DFBSurfacePixelFormat font_format;           /* Preferred font format. */

     char      *h3600_device;                     /* H3600 Touchscreen Device */

     char      *mut_device;                       /* MuTouch Device */
} DFBConfig;

extern DFBConfig *dfb_config;

/*
 * Allocate Config struct, fill with defaults and parse command line options
 * for overrides. Options identified as DirectFB options are stripped out
 * of the array.
 */
DFBResult dfb_config_init( int *argc, char **argv[] );

/*
 * Read configuration options from file. Called by config_init().
 *
 * Returns DFB_IO if config file could not be opened.
 * Returns DFB_UNSUPPORTED if file contains an invalid option.
 * Returns DFB_INVARG if an invalid option assignment is done,
 * e.g. "--desktop-buffer-mode=somethingwrong".
 */
DFBResult dfb_config_read( const char *filename );


/*
 * Set indiviual option. Used by config_init(), config_read() and
 * DirectFBSetOption()
 */
DFBResult dfb_config_set( const char *name, const char *value );

const char *dfb_config_usage( void );

#endif

