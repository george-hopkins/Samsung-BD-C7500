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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <directfb.h>

#include <core/coretypes.h>

#include <core/surfaces.h>
#include <core/layers.h>

#include <direct/conf.h>
#include <fusion/conf.h>
#include <misc/conf.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>


DFBConfig *dfb_config = NULL;

static const char *config_usage =
     "DirectFB version " DIRECTFB_VERSION "\n"
     "\n"
     " --dfb-help                      Output DirectFB usage information and exit\n"
     " --dfb:<option>[,<option>]...    Pass options to DirectFB (see below)\n"
     "\n"
     "DirectFB options:\n"
     "\n"
     "  system=<system>                Specify the system (FBDev, SDL, etc.)\n"
     "  fbdev=<device>                 Open <device> instead of /dev/fb0\n"
     "  mode=<width>x<height>          Set the default resolution\n"
     "  depth=<pixeldepth>             Set the default pixel depth\n"
     "  pixelformat=<pixelformat>      Set the default pixel format\n"
     "  session=<num>                  Select multi app world (zero based, -1 = new)\n"
     "  remote=<host>[:<session>]      Select remote session to connect to\n"
     "  tmpfs=<directory>              Location of shared memory file\n"
     "  memcpy=<method>                Skip memcpy() probing (help = show list)\n"
     "  primary-layer=<id>             Select an alternative primary layer\n"
     "  quiet                          No text output except debugging\n"
     "  [no-]banner                    Show DirectFB Banner on startup\n"
     "  [no-]debug                     Enable debug output\n"
     "  [no-]trace                     Enable stack trace support\n"
     "  force-windowed                 Primary surface always is a window\n"
     "  force-desktop                  Primary surface is the desktop background\n"
     "  [no-]hardware                  Hardware acceleration\n"
     "  [no-]sync                      Do `sync()' (default=no)\n"
#ifdef USE_MMX
     "  [no-]mmx                       Enable mmx support\n"
#endif
     "  font-format=<pixelformat>      Set the preferred font format\n"
     "  dont-catch=<num>[[,<num>]...]  Don't catch these signals\n"
     "  [no-]sighandler                Enable signal handler\n"
     "  [no-]deinit-check              Enable deinit check at exit\n"
     "  block-all-signals              Block all signals\n"
     "  [no-]vt-switch                 Allocate/switch to a new VT\n"
     "  [no-]vt-switching              Allow Ctrl+Alt+<F?> (EXPERIMENTAL)\n"
     "  [no-]graphics-vt               Put terminal into graphics mode\n"
     "  [no-]-vt                       Use VT handling code at all?\n"
     "  [no-]motion-compression        Mouse motion event compression\n"
     "  mouse-protocol=<protocol>      Mouse protocol (serial mouse)\n"
     "  [no-]lefty                     Swap left and right mouse buttons\n"
     "  [no-]capslock-meta             Map the CapsLock key to Meta\n"
     "  linux-input-ir-only            Ignore all non-IR Linux Input devices\n"
     "  [no-]cursor                    Never create a cursor\n"
     "  wm=<wm>                        Window manager module ('default' or 'unique')\n"
     "  bg-none                        Disable background clear\n"
     "  bg-color=AARRGGBB              Use background color (hex)\n"
     "  bg-image=<filename>            Use background image\n"
     "  bg-tile=<filename>             Use tiled background image\n"
     "  [no-]translucent-windows       Allow translucent windows\n"
     "  [no-]decorations               Enable window decorations (if supported by wm)\n"
     "  videoram-limit=<amount>        Limit amount of Video RAM in kb\n"
     "  screenshot-dir=<directory>     Dump screen content on <Print> key presses\n"
     "  disable-module=<module_name>   suppress loading this module\n"
     "\n"
     "  [no-]matrox-sgram              Use Matrox SGRAM features\n"
     "  [no-]matrox-crtc2              Experimental Matrox CRTC2 support\n"
     "  matrox-tv-standard=(pal|ntsc)  Matrox TV standard (default=pal)\n"
     "  matrox-cable-type=(composite|scart-rgb|scart-composite)\n"
     "                                 Matrox cable type (default=composite)\n"
     "  h3600-device=<device>          Use this device for the H3600 TS driver\n"
     "  mut-device=<device>            Use this device for the MuTouch driver\n"
     "\n"
     " Window surface swapping policy:\n"
     "  window-surface-policy=(auto|videohigh|videolow|systemonly|videoonly)\n"
     "     auto:       DirectFB decides depending on hardware.\n"
     "     videohigh:  Swapping system/video with high priority.\n"
     "     videolow:   Swapping system/video with low priority.\n"
     "     systemonly: Window surface is always stored in system memory.\n"
     "     videoonly:  Window surface is always stored in video memory.\n"
     "\n"
     " Desktop buffer mode:\n"
     "  desktop-buffer-mode=(auto|triple|backvideo|backsystem|frontonly|windows)\n"
     "     auto:       DirectFB decides depending on hardware.\n"
     "     triple:     Triple buffering (video only).\n"
     "     backvideo:  Front and back buffer are video only.\n"
     "     backsystem: Back buffer is system only.\n"
     "     frontonly:  There is no back buffer.\n"
     "     windows:    Special mode with window buffers directly displayed.\n"
     "\n"
     " Force synchronization of vertical retrace:\n"
     "  vsync-after:   Wait for the vertical retrace after flipping.\n"
     "  vsync-none:    disable polling for vertical retrace.\n"
     "\n";

typedef struct {
     const char            *string;
     DFBSurfacePixelFormat  format;
} FormatString;

static const FormatString format_strings[] = {
     { "A8",       DSPF_A8       },
     { "ALUT44",   DSPF_ALUT44   },
     { "ARGB",     DSPF_ARGB     },
     { "ARGB1555", DSPF_ARGB1555 },
     { "ARGB2554", DSPF_ARGB2554 },
     { "ARGB4444", DSPF_ARGB4444 },
     { "AiRGB",    DSPF_AiRGB    },
     { "I420",     DSPF_I420     },
     { "LUT8",     DSPF_LUT8     },
     { "NV12",     DSPF_NV12     },
     { "NV21",     DSPF_NV21     },
     { "NV16",     DSPF_NV16     },
     { "RGB16",    DSPF_RGB16    },
     { "RGB24",    DSPF_RGB24    },
     { "RGB32",    DSPF_RGB32    },
     { "RGB332",   DSPF_RGB332   },
     { "UYVY",     DSPF_UYVY     },
     { "YUY2",     DSPF_YUY2     },
     { "YV12",     DSPF_YV12     }
};

#define NUM_FORMAT_STRINGS (sizeof(format_strings) / sizeof(FormatString))

static const FormatString font_format_strings[] = {
     { "A1",       DSPF_A1       },
     { "A8",       DSPF_A8       },
     { "ARGB",     DSPF_ARGB     },
     { "ARGB1555", DSPF_ARGB1555 },
     { "ARGB2554", DSPF_ARGB2554 },
     { "ARGB4444", DSPF_ARGB4444 },
     { "AiRGB",    DSPF_AiRGB    }
};

#define NUM_FONT_FORMAT_STRINGS (sizeof(font_format_strings) / sizeof(FormatString))

/* serial mouse device names */
#define DEV_NAME     "/dev/mouse"
#define DEV_NAME_GPM "/dev/gpmdata"

static int
format_string_compare (const void *key,
                       const void *base)
{
     return strcmp ((const char *) key, ((const FormatString *) base)->string);
}

static DFBSurfacePixelFormat
parse_pixelformat( const char *format )
{
     FormatString *format_string;

     format_string = bsearch( format, format_strings,
                              NUM_FORMAT_STRINGS, sizeof(FormatString),
                              format_string_compare );
     if (!format_string)
          return DSPF_UNKNOWN;

     return format_string->format;
}

static DFBSurfacePixelFormat
parse_font_format( const char *format )
{
     FormatString *format_string;

     format_string = bsearch( format, font_format_strings,
                              NUM_FONT_FORMAT_STRINGS, sizeof(FormatString),
                              format_string_compare );
     if (!format_string)
          return DSPF_UNKNOWN;

     return format_string->format;
}

static DFBResult
parse_args( const char *args )
{
     char *buf = alloca( strlen(args) + 1 );

     strcpy( buf, args );

     while (buf && buf[0]) {
          DFBResult  ret;
          char      *value;
          char      *next;

          if ((next = strchr( buf, ',' )) != NULL)
               *next++ = '\0';

          if (strcmp (buf, "help") == 0) {
               fprintf( stderr, config_usage );
               exit(1);
          }

          if (strcmp (buf, "memcpy=help") == 0) {
               direct_print_memcpy_routines();
               exit(1);
          }

          if ((value = strchr( buf, '=' )) != NULL)
               *value++ = '\0';

          ret = dfb_config_set( buf, value );
          switch (ret) {
               case DFB_OK:
                    break;
               case DFB_UNSUPPORTED:
                    D_ERROR( "DirectFB/Config: Unknown option '%s'!\n", buf );
                    break;
               default:
                    return ret;
          }

          buf = next;
     }

     return DFB_OK;
}

/*
 * The following function isn't used because the configuration should
 * only go away if the application is completely terminated. In that case
 * the memory is freed anyway.
 */

#if 0
static void config_cleanup()
{
     if (!dfb_config) {
          D_BUG("config_cleanup() called with no config allocated!");
          return;
     }

     if (dfb_config->fb_device)
          D_FREE( dfb_config->fb_device );

     if (dfb_config->layer_bg_filename)
          D_FREE( dfb_config->layer_bg_filename );

     D_FREE( dfb_config );
     dfb_config = NULL;
}
#endif

/*
 * allocates config and fills it with defaults
 */
static void config_allocate()
{
     if (dfb_config)
          return;

     dfb_config = (DFBConfig*) calloc( 1, sizeof(DFBConfig) );

     dfb_config->layer_bg_color.a         = 0xFF;
     dfb_config->layer_bg_color.r         = 0x10;
     dfb_config->layer_bg_color.g         = 0x7c;
     dfb_config->layer_bg_color.b         = 0xe8;
     dfb_config->layer_bg_mode            = DLBM_COLOR;

     dfb_config->banner                   = true;
     dfb_config->deinit_check             = true;
     dfb_config->mmx                      = true;
     dfb_config->vt                       = true;
     dfb_config->vt_switch                = true;
     dfb_config->vt_switching             = true;
     dfb_config->kd_graphics              = true;
     dfb_config->translucent_windows      = true;
     dfb_config->mouse_motion_compression = true;
     dfb_config->mouse_gpm_source         = false;
     dfb_config->mouse_source             = D_STRDUP( DEV_NAME );
     dfb_config->window_policy            = -1;
     dfb_config->buffer_mode              = -1;
     dfb_config->wm                       = D_STRDUP( "default" );
     dfb_config->decorations              = true;

     /* default to fbdev */
     dfb_config->system = D_STRDUP( "FBDev" );

     /* default to no-vt-switch if we don't have root privileges */
     if (geteuid())
          dfb_config->vt_switch = false;
}

const char *dfb_config_usage( void )
{
     return config_usage;
}

DFBResult dfb_config_set( const char *name, const char *value )
{
     if (strcmp (name, "disable-module" ) == 0) {
          if (value) {
	       int n = 0;

	       while (direct_config->disable_module &&
		      direct_config->disable_module[n])
		    n++;

	       direct_config->disable_module = D_REALLOC( direct_config->disable_module,
                                                          sizeof(char*) * (n + 2) );

	       direct_config->disable_module[n] = D_STRDUP( value );
               direct_config->disable_module[n+1] = NULL;
          }
          else {
               D_ERROR("DirectFB/Config 'disable_module': No module name specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "system" ) == 0) {
          if (value) {
               if (dfb_config->system)
                    D_FREE( dfb_config->system );
               dfb_config->system = D_STRDUP( value );
          }
          else {
               D_ERROR("DirectFB/Config 'system': No system specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "wm" ) == 0) {
          if (value) {
               if (dfb_config->wm)
                    D_FREE( dfb_config->wm );
               dfb_config->wm = D_STRDUP( value );
          }
          else {
               D_ERROR("DirectFB/Config 'wm': No window manager specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "fbdev" ) == 0) {
          if (value) {
               if (dfb_config->fb_device)
                    D_FREE( dfb_config->fb_device );
               dfb_config->fb_device = D_STRDUP( value );
          }
          else {
               D_ERROR("DirectFB/Config 'fbdev': No device name specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "tmpfs" ) == 0) {
          if (value) {
               if (fusion_config->tmpfs)
                    D_FREE( fusion_config->tmpfs );
               fusion_config->tmpfs = D_STRDUP( value );
          }
          else {
               D_ERROR("DirectFB/Config 'tmpfs': No directory specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "memcpy" ) == 0) {
          if (value) {
               if (direct_config->memcpy)
                    D_FREE( direct_config->memcpy );
               direct_config->memcpy = D_STRDUP( value );
          }
          else {
               D_ERROR("DirectFB/Config 'memcpy': No method specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "screenshot-dir" ) == 0) {
          if (value) {
               if (dfb_config->screenshot_dir)
                    D_FREE( dfb_config->screenshot_dir );
               dfb_config->screenshot_dir = D_STRDUP( value );
          }
          else {
               D_ERROR("DirectFB/Config 'screenshot-dir': No directory name specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "mode" ) == 0) {
          if (value) {
               int width, height;

               if (sscanf( value, "%dx%d", &width, &height ) < 2) {
                    D_ERROR("DirectFB/Config 'mode': Could not parse mode!\n");
                    return DFB_INVARG;
               }

               dfb_config->mode.width  = width;
               dfb_config->mode.height = height;
          }
          else {
               D_ERROR("DirectFB/Config 'mode': No mode specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "depth" ) == 0) {
          if (value) {
               int depth;

               if (sscanf( value, "%d", &depth ) < 1) {
                    D_ERROR("DirectFB/Config 'depth': Could not parse value!\n");
                    return DFB_INVARG;
               }

               dfb_config->mode.depth = depth;
          }
          else {
               D_ERROR("DirectFB/Config 'depth': No value specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "primary-layer" ) == 0) {
          if (value) {
               int id;

               if (sscanf( value, "%d", &id ) < 1) {
                    D_ERROR("DirectFB/Config 'primary-layer': Could not parse id!\n");
                    return DFB_INVARG;
               }

               dfb_config->primary_layer = id;
          }
          else {
               D_ERROR("DirectFB/Config 'primary-layer': No id specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "pixelformat" ) == 0) {
          if (value) {
               DFBSurfacePixelFormat format;

               format = parse_pixelformat( value );
               if (format == DSPF_UNKNOWN) {
                    D_ERROR("DirectFB/Config 'pixelformat': Could not parse format!\n");
                    return DFB_INVARG;
               }

               dfb_config->mode.format = format;
          }
          else {
               D_ERROR("DirectFB/Config 'pixelformat': No format specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "font-format" ) == 0) {
          if (value) {
               DFBSurfacePixelFormat format;

               format = parse_font_format( value );
               if (format == DSPF_UNKNOWN) {
                    D_ERROR("DirectFB/Config 'font-format': Could not parse format!\n");
                    return DFB_INVARG;
               }

               dfb_config->font_format = format;
          }
          else {
               D_ERROR("DirectFB/Config 'font-format': No format specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "session" ) == 0) {
          if (value) {
               int session;

               if (sscanf( value, "%d", &session ) < 1) {
                    D_ERROR("DirectFB/Config 'session': Could not parse value!\n");
                    return DFB_INVARG;
               }

               dfb_config->session = session;
          }
          else {
               D_ERROR("DirectFB/Config 'session': No value specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "remote" ) == 0) {
          if (value) {
               char host[128];
               int  session = 0;

               if (sscanf( value, "%127s:%d", host, &session ) < 1) {
                    D_ERROR("DirectFB/Config 'remote': "
                            "Could not parse value (format is <host>[:<session>])!\n");
                    return DFB_INVARG;
               }

               if (dfb_config->remote.host)
                    D_FREE( dfb_config->remote.host );

               dfb_config->remote.host    = D_STRDUP( host );
               dfb_config->remote.session = session;
          }
          else {
               D_ERROR("DirectFB/Config 'remote': No value specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "videoram-limit" ) == 0) {
          if (value) {
               int limit;

               if (sscanf( value, "%d", &limit ) < 1) {
                    D_ERROR("DirectFB/Config 'videoram-limit': Could not parse value!\n");
                    return DFB_INVARG;
               }

               if (limit < 0)
                    limit = 0;

               dfb_config->videoram_limit = limit << 10;
          }
          else {
               D_ERROR("DirectFB/Config 'videoram-limit': No value specified!\n");
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "quiet" ) == 0) {
          direct_config->quiet = true;
     } else
     if (strcmp (name, "banner" ) == 0) {
          dfb_config->banner = true;
     } else
     if (strcmp (name, "no-banner" ) == 0) {
          dfb_config->banner = false;
     } else
     if (strcmp (name, "debug" ) == 0) {
          if (value)
               direct_debug_config_domain( value, true );
          else
               direct_config->debug = true;
     } else
     if (strcmp (name, "no-debug" ) == 0) {
          if (value)
               direct_debug_config_domain( value, false );
          else
               direct_config->debug = false;
     } else
     if (strcmp (name, "trace" ) == 0) {
          direct_config->trace = true;
     } else
     if (strcmp (name, "no-trace" ) == 0) {
          direct_config->trace = false;
     } else
     if (strcmp (name, "force-windowed" ) == 0) {
          dfb_config->force_windowed = true;
     } else
     if (strcmp (name, "force-desktop" ) == 0) {
          dfb_config->force_desktop = true;
     } else
     if (strcmp (name, "hardware" ) == 0) {
          dfb_config->software_only = false;
     } else
     if (strcmp (name, "no-hardware" ) == 0) {
          dfb_config->software_only = true;
     } else
     if (strcmp (name, "mmx" ) == 0) {
          dfb_config->mmx = true;
     } else
     if (strcmp (name, "no-mmx" ) == 0) {
          dfb_config->mmx = false;
     } else
     if (strcmp (name, "vt" ) == 0) {
          dfb_config->vt = true;
     } else
     if (strcmp (name, "no-vt" ) == 0) {
          dfb_config->vt = false;
     } else
     if (strcmp (name, "sighandler" ) == 0) {
          direct_config->sighandler = true;
     } else
     if (strcmp (name, "no-sighandler" ) == 0) {
          direct_config->sighandler = false;
     } else
     if (strcmp (name, "block-all-signals" ) == 0) {
          dfb_config->block_all_signals = true;
     } else
     if (strcmp (name, "deinit-check" ) == 0) {
          dfb_config->deinit_check = true;
     } else
     if (strcmp (name, "no-deinit-check" ) == 0) {
          dfb_config->deinit_check = false;
     } else
     if (strcmp (name, "cursor" ) == 0) {
          dfb_config->no_cursor = false;
     } else
     if (strcmp (name, "no-cursor" ) == 0) {
          dfb_config->no_cursor = true;
     } else
     if (strcmp (name, "linux-input-ir-only" ) == 0) {
          dfb_config->linux_input_ir_only = true;
     } else
     if (strcmp (name, "motion-compression" ) == 0) {
          dfb_config->mouse_motion_compression = true;
     } else
     if (strcmp (name, "no-motion-compression" ) == 0) {
          dfb_config->mouse_motion_compression = false;
     } else
     if (strcmp (name, "mouse-protocol" ) == 0) {
          if (value) {
               dfb_config->mouse_protocol = D_STRDUP( value );
          }
          else {
               D_ERROR( "DirectFB/Config: No mouse protocol specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "mouse-source" ) == 0) {
          if (value) {
               D_FREE( dfb_config->mouse_source );	
               dfb_config->mouse_source = D_STRDUP( value );
          }
          else {
               D_ERROR( "DirectFB/Config: No mouse source specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "mouse-gpm-source" ) == 0) {
          dfb_config->mouse_gpm_source = true;
	  D_FREE( dfb_config->mouse_source );
	  dfb_config->mouse_source = D_STRDUP( DEV_NAME_GPM );
     } else
     if (strcmp (name, "translucent-windows" ) == 0) {
          dfb_config->translucent_windows = true;
     } else
     if (strcmp (name, "no-translucent-windows" ) == 0) {
          dfb_config->translucent_windows = false;
     } else
     if (strcmp (name, "decorations" ) == 0) {
          dfb_config->decorations = true;
     } else
     if (strcmp (name, "no-decorations" ) == 0) {
          dfb_config->decorations = false;
     } else
     if (strcmp (name, "vsync-none" ) == 0) {
          dfb_config->pollvsync_none = true;
     } else
     if (strcmp (name, "vsync-after" ) == 0) {
          dfb_config->pollvsync_after = true;
     } else
     if (strcmp (name, "vt-switch" ) == 0) {
          dfb_config->vt_switch = true;
     } else
     if (strcmp (name, "no-vt-switch" ) == 0) {
          dfb_config->vt_switch = false;
     } else
     if (strcmp (name, "vt-switching" ) == 0) {
          dfb_config->vt_switching = true;
     } else
     if (strcmp (name, "no-vt-switching" ) == 0) {
          dfb_config->vt_switching = false;
     } else
     if (strcmp (name, "graphics-vt" ) == 0) {
          dfb_config->kd_graphics = true;
     } else
     if (strcmp (name, "no-graphics-vt" ) == 0) {
          dfb_config->kd_graphics = false;
     } else
     if (strcmp (name, "bg-none" ) == 0) {
          dfb_config->layer_bg_mode = DLBM_DONTCARE;
     } else
     if (strcmp (name, "bg-image" ) == 0 || strcmp (name, "bg-tile" ) == 0) {
          if (value) {
               if (dfb_config->layer_bg_filename)
                    D_FREE( dfb_config->layer_bg_filename );
               dfb_config->layer_bg_filename = D_STRDUP( value );
               dfb_config->layer_bg_mode =
                    strcmp (name, "bg-tile" ) ? DLBM_IMAGE : DLBM_TILE;
          }
          else {
               D_ERROR( "DirectFB/Config: No image filename specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "window-surface-policy" ) == 0) {
          if (value) {
               if (strcmp( value, "auto" ) == 0) {
                    dfb_config->window_policy = -1;
               } else
               if (strcmp( value, "videohigh" ) == 0) {
                    dfb_config->window_policy = CSP_VIDEOHIGH;
               } else
               if (strcmp( value, "videolow" ) == 0) {
                    dfb_config->window_policy = CSP_VIDEOLOW;
               } else
               if (strcmp( value, "systemonly" ) == 0) {
                    dfb_config->window_policy = CSP_SYSTEMONLY;
               } else
               if (strcmp( value, "videoonly" ) == 0) {
                    dfb_config->window_policy = CSP_VIDEOONLY;
               }
               else {
                    D_ERROR( "DirectFB/Config: "
                             "Unknown window surface policy `%s'!\n", value );
                    return DFB_INVARG;
               }
          }
          else {
               D_ERROR( "DirectFB/Config: "
                        "No window surface policy specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "desktop-buffer-mode" ) == 0) {
          if (value) {
               if (strcmp( value, "auto" ) == 0) {
                    dfb_config->buffer_mode = -1;
               } else
               if (strcmp( value, "triple" ) == 0) {
                    dfb_config->buffer_mode = DLBM_TRIPLE;
               } else
               if (strcmp( value, "backvideo" ) == 0) {
                    dfb_config->buffer_mode = DLBM_BACKVIDEO;
               } else
               if (strcmp( value, "backsystem" ) == 0) {
                    dfb_config->buffer_mode = DLBM_BACKSYSTEM;
               } else
               if (strcmp( value, "frontonly" ) == 0) {
                    dfb_config->buffer_mode = DLBM_FRONTONLY;
               } else
               if (strcmp( value, "windows" ) == 0) {
                    dfb_config->buffer_mode = DLBM_WINDOWS;
               } else {
                    D_ERROR( "DirectFB/Config: Unknown buffer mode "
                             "'%s'!\n", value );
                    return DFB_INVARG;
               }
          }
          else {
               D_ERROR( "DirectFB/Config: "
                        "No desktop buffer mode specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "bg-color" ) == 0) {
          if (value) {
               char *error;
               __u32 argb;

               argb = strtoul( value, &error, 16 );

               if (*error) {
                    D_ERROR( "DirectFB/Config: Error in bg-color: "
                             "'%s'!\n", error );
                    return DFB_INVARG;
               }

               dfb_config->layer_bg_color.b = argb & 0xFF;
               argb >>= 8;
               dfb_config->layer_bg_color.g = argb & 0xFF;
               argb >>= 8;
               dfb_config->layer_bg_color.r = argb & 0xFF;
               argb >>= 8;
               dfb_config->layer_bg_color.a = argb & 0xFF;

               dfb_config->layer_bg_mode = DLBM_COLOR;
          }
          else {
               D_ERROR( "DirectFB/Config: No background color specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "dont-catch" ) == 0) {
          if (value) {
               char *signals   = D_STRDUP( value );
               char *p, *r, *s = signals;

               while ((r = strtok_r( s, ",", &p ))) {
                    char          *error;
                    unsigned long  signum;

                    direct_trim( &r );

                    signum = strtoul( r, &error, 10 );

                    if (*error) {
                         D_ERROR( "DirectFB/Config: Error in dont-catch: "
                                  "'%s'!\n", error );
                         D_FREE( signals );
                         return DFB_INVARG;
                    }

                    sigaddset( &direct_config->dont_catch, signum );

                    s = NULL;
               }

               D_FREE( signals );
          }
          else {
               D_ERROR( "DirectFB/Config: Missing value for dont-catch!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "matrox-tv-standard" ) == 0) {
          if (value) {
               if (strcmp( value, "pal" ) == 0) {
                    dfb_config->matrox_ntsc = false;
               } else
               if (strcmp( value, "ntsc" ) == 0) {
                    dfb_config->matrox_ntsc = true;
               } else {
                    D_ERROR( "DirectFB/Config: Unknown TV standard "
                             "'%s'!\n", value );
                    return DFB_INVARG;
               }
          }
          else {
               D_ERROR( "DirectFB/Config: "
                        "No TV standard specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "matrox-cable-type" ) == 0) {
          if (value) {
               if (strcmp( value, "composite" ) == 0) {
                    dfb_config->matrox_cable = 0;
               } else
               if (strcmp( value, "scart-rgb" ) == 0) {
                    dfb_config->matrox_cable = 1;
               } else
               if (strcmp( value, "scart-composite" ) == 0) {
                    dfb_config->matrox_cable = 2;
               } else {
                    D_ERROR( "DirectFB/Config: Unknown cable type "
                             "'%s'!\n", value );
                    return DFB_INVARG;
               }
          }
          else {
               D_ERROR( "DirectFB/Config: "
                        "No cable type specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "matrox-sgram" ) == 0) {
          dfb_config->matrox_sgram = true;
     } else
     if (strcmp (name, "matrox-crtc2" ) == 0) {
          dfb_config->matrox_crtc2 = true;
     } else
     if (strcmp (name, "no-matrox-sgram" ) == 0) {
          dfb_config->matrox_sgram = false;
     } else
     if (strcmp (name, "sync" ) == 0) {
          dfb_config->sync = true;
     } else
     if (strcmp (name, "no-sync" ) == 0) {
          dfb_config->sync = false;
     } else
     if (strcmp (name, "lefty" ) == 0) {
          dfb_config->lefty = true;
     } else
     if (strcmp (name, "no-lefty" ) == 0) {
          dfb_config->lefty = false;
     } else
     if (strcmp (name, "capslock-meta" ) == 0) {
          dfb_config->capslock_meta = true;
     } else
     if (strcmp (name, "no-capslock-meta" ) == 0) {
          dfb_config->capslock_meta = false;
     } else
     if (strcmp (name, "h3600-device" ) == 0) {
          if (value) {
               if (dfb_config->h3600_device)
                    D_FREE( dfb_config->h3600_device );

               dfb_config->h3600_device = D_STRDUP( value );
          }
          else {
               D_ERROR( "DirectFB/Config: No H3600 TS device specified!\n" );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "mut-device" ) == 0) {
          if (value) {
               if (dfb_config->mut_device)
                    D_FREE( dfb_config->mut_device );

               dfb_config->mut_device = D_STRDUP( value );
          }
          else {
               D_ERROR( "DirectFB/Config: No MuTouch device specified!\n" );
               return DFB_INVARG;
          }
     }
     else
          return DFB_UNSUPPORTED;

     return DFB_OK;
}

DFBResult dfb_config_init( int *argc, char **argv[] )
{
     DFBResult ret;
     int i;
     char *home = getenv( "HOME" );
     char *prog = NULL;
     char *session;
     char *dfbargs;

     if (dfb_config)
          return DFB_OK;

     config_allocate();

     /* Current session is default. */
     session = getenv( "DIRECTFB_SESSION" );
     if (session) {
          ret = dfb_config_set( "session", session );
          if (ret)
               return ret;
     }

     /* Read system settings. */
     ret = dfb_config_read( "/etc/directfbrc" );
     if (ret  &&  ret != DFB_IO)
          return ret;

     /* Read user settings. */
     if (home) {
          int  len = strlen(home) + strlen("/.directfbrc") + 1;
          char buf[len];

          snprintf( buf, len, "%s/.directfbrc", home );

          ret = dfb_config_read( buf );
          if (ret  &&  ret != DFB_IO)
               return ret;
     }

     /* Get application name. */
     if (argc && *argc && argv && *argv) {
          prog = strrchr( (*argv)[0], '/' );

          if (prog)
               prog++;
          else
               prog = (*argv)[0];
     }

     /* Read global application settings. */
     if (prog && prog[0]) {
          int  len = strlen("/etc/directfbrc.") + strlen(prog) + 1;
          char buf[len];

          snprintf( buf, len, "/etc/directfbrc.%s", prog );

          ret = dfb_config_read( buf );
          if (ret  &&  ret != DFB_IO)
               return ret;
     }

     /* Read user application settings. */
     if (home && prog && prog[0]) {
          int  len = strlen(home) + strlen("/.directfbrc.") + strlen(prog) + 1;
          char buf[len];

          snprintf( buf, len, "%s/.directfbrc.%s", home, prog );

          ret = dfb_config_read( buf );
          if (ret  &&  ret != DFB_IO)
               return ret;
     }

     /* Read settings from environment variable. */
     dfbargs = getenv( "DFBARGS" );
     if (dfbargs) {
          ret = parse_args( dfbargs );
          if (ret)
               return ret;
     }

     /* Read settings from command line. */
     if (argc && argv) {
          for (i = 1; i < *argc; i++) {

               if (strcmp ((*argv)[i], "--dfb-help") == 0) {
                    fprintf( stderr, config_usage );
                    exit(1);
               }

               if (strncmp ((*argv)[i], "--dfb:", 6) == 0) {
                    ret = parse_args( (*argv)[i] + 6 );
                    if (ret)
                         return ret;

                    (*argv)[i] = NULL;
               }
          }

          for (i = 1; i < *argc; i++) {
               int k;

               for (k = i; k < *argc; k++)
                    if ((*argv)[k] != NULL)
                         break;

               if (k > i) {
                    int j;

                    k -= i;

                    for (j = i + k; j < *argc; j++)
                         (*argv)[j-k] = (*argv)[j];

                    *argc -= k;
               }
          }
     }

     if (!dfb_config->vt_switch)
          dfb_config->kd_graphics = true;

     return DFB_OK;
}

DFBResult dfb_config_read( const char *filename )
{
     DFBResult ret = DFB_OK;
     char line[400];
     FILE *f;

     config_allocate();

     f = fopen( filename, "r" );
     if (!f) {
          D_DEBUG( "DirectFB/Config: Unable to open config file `%s'!\n", filename );
          return DFB_IO;
     } else {
          D_INFO( "DirectFB/Config: Parsing config file '%s'.\n", filename );
     }

     while (fgets( line, 400, f )) {
          char *name = line;
          char *value = strchr( line, '=' );

          if (value) {
               *value++ = 0;
               direct_trim( &value );
          }

          direct_trim( &name );

          if (!*name  ||  *name == '#')
               continue;

          ret = dfb_config_set( name, value );
          if (ret) {
               if (ret == DFB_UNSUPPORTED)
                    D_ERROR( "DirectFB/Config: In config file `%s': "
                             "Invalid option `%s'!\n", filename, name );
               break;
          }
     }

     fclose( f );

     return ret;
}

