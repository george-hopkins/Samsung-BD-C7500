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

#ifndef __DIRECTFB_H__
#define __DIRECTFB_H__

#include <dfb_types.h>
#include <sys/time.h> /* struct timeval */

#include <directfb_keyboard.h>


#ifdef __cplusplus
extern "C"
{
#endif


/*
 * @internal
 *
 * Forward declaration macro for interfaces.
 */
#define DECLARE_INTERFACE( IFACE )                \
     typedef struct _##IFACE IFACE;

/*
 * @internal
 *
 * Macro for an interface definition.
 */
#define DEFINE_INTERFACE( IFACE, IDATA... )       \
     struct _##IFACE     {                        \
          void       *priv;                       \
          int         magic;                      \
          DFBResult (*AddRef)( IFACE *thiz );     \
          DFBResult (*Release)( IFACE *thiz );    \
                                                  \
          IDATA                                   \
     };


/*
 * Version handling.
 */
extern const unsigned int directfb_major_version;
extern const unsigned int directfb_minor_version;
extern const unsigned int directfb_micro_version;
extern const unsigned int directfb_binary_age;
extern const unsigned int directfb_interface_age;

/*
 * Check for a certain DirectFB version.
 * In case of an error a message is returned describing the mismatch.
 */
const char * DirectFBCheckVersion( unsigned int required_major,
                                   unsigned int required_minor,
                                   unsigned int required_micro );


/*
 * The only interface with a global "Create" function,
 * any other functionality goes from here.
 */
DECLARE_INTERFACE( IDirectFB )

/*
 * Display encoder and output connector settings,
 * input device assignment in multi head environments,
 * power management, synchronization.
 */
DECLARE_INTERFACE( IDirectFBScreen )

/*
 * Layer configuration, creation of windows and background
 * configuration.
 */
DECLARE_INTERFACE( IDirectFBDisplayLayer )

/*
 * Surface locking, setting colorkeys and other drawing
 * parameters, clipping, flipping, blitting, drawing.
 */
DECLARE_INTERFACE( IDirectFBSurface )

/*
 * Access to palette data. Set/get entries, rotate palette.
 */
DECLARE_INTERFACE( IDirectFBPalette )

/*
 * Moving, resizing, raising and lowering.
 * Getting an interface to the window's surface.
 * Setting opacity and handling events.
 */
DECLARE_INTERFACE( IDirectFBWindow )

/*
 * Creation of input buffers and explicit state queries.
 */
DECLARE_INTERFACE( IDirectFBInputDevice )

/*
 * An event buffer puts events from devices or windows into a FIFO.
 */
DECLARE_INTERFACE( IDirectFBEventBuffer )

/*
 * Getting font metrics and pixel width of a string.
 */
DECLARE_INTERFACE( IDirectFBFont )

/*
 * Getting information about and loading one image from file.
 */
DECLARE_INTERFACE( IDirectFBImageProvider )

/*
 * Rendering video data into a surface.
 */
DECLARE_INTERFACE( IDirectFBVideoProvider )

/*
 * Streaming or static data for image or video providers.
 */
DECLARE_INTERFACE( IDirectFBDataBuffer )

/*
 * OpenGL context of a surface.
 */
DECLARE_INTERFACE( IDirectFBGL )

/*
 * <i>No summary yet...</i>
 */
DECLARE_INTERFACE( IDirectFBSurfaceManager )

/*
 * Every interface method returns this result code.<br>
 * Any other value to be returned adds an argument pointing
 * to a location the value should be written to.
 */
typedef enum {
     DFB_OK,             /* No error occured. */
     DFB_FAILURE,        /* A general or unknown error occured. */
     DFB_INIT,           /* A general initialization error occured. */
     DFB_BUG,            /* Internal bug or inconsistency has been detected. */
     DFB_DEAD,           /* Interface has a zero reference counter (available in debug mode). */
     DFB_UNSUPPORTED,    /* The requested operation or an argument is (currently) not supported. */
     DFB_UNIMPLEMENTED,  /* The requested operation is not implemented, yet. */
     DFB_ACCESSDENIED,   /* Access to the resource is denied. */
     DFB_INVARG,         /* An invalid argument has been specified. */
     DFB_NOSYSTEMMEMORY, /* There's not enough system memory. */
     DFB_NOVIDEOMEMORY,  /* There's not enough video memory. */
     DFB_LOCKED,         /* The resource is (already) locked. */
     DFB_BUFFEREMPTY,    /* The buffer is empty. */
     DFB_FILENOTFOUND,   /* The specified file has not been found. */
     DFB_IO,             /* A general I/O error occured. */
     DFB_BUSY,           /* The resource or device is busy. */
     DFB_NOIMPL,         /* No implementation for this interface or content type has been found. */
     DFB_MISSINGFONT,    /* No font has been set. */
     DFB_TIMEOUT,        /* The operation timed out. */
     DFB_MISSINGIMAGE,   /* No image has been set. */
     DFB_THIZNULL,       /* 'thiz' pointer is NULL. */
     DFB_IDNOTFOUND,     /* No resource has been found by the specified id. */
     DFB_INVAREA,        /* An invalid area has been specified or detected. */
     DFB_DESTROYED,      /* The underlying object (e.g. a window or surface) has been destroyed. */
     DFB_FUSION,         /* Internal fusion error detected, most likely related to IPC resources. */
     DFB_BUFFERTOOLARGE, /* Buffer is too large. */
     DFB_INTERRUPTED,    /* The operation has been interrupted. */
     DFB_NOCONTEXT,      /* No context available. */
     DFB_TEMPUNAVAIL,    /* Temporarily unavailable. */
     DFB_LIMITEXCEEDED,  /* Attempted to exceed limit, i.e. any kind of maximum size, count etc. */
     DFB_NOSUCHMETHOD,   /* Requested method is not known to remote site. */
     DFB_NOSUCHINSTANCE, /* Requested instance is not known to remote site. */
     DFB_ITEMNOTFOUND,   /* No such item found. */
     DFB_VERSIONMISMATCH,/* Some versions didn't match. */
     DFB_NOSHAREDMEMORY, /* There's not enough shared memory. */
     DFB_EOF             /* Reached end of file. */
} DFBResult;

/*
 * A boolean.
 */
typedef enum {
     DFB_FALSE = 0,
     DFB_TRUE  = !DFB_FALSE
} DFBBoolean;

/*
 * A point specified by x/y coordinates.
 */
typedef struct {
     int            x;   /* X coordinate of it */
     int            y;   /* Y coordinate of it */
} DFBPoint;

/*
 * A horizontal line specified by x and width.
 */
typedef struct {
     int            x;   /* X coordinate */
     int            w;   /* width of span */
} DFBSpan;

/*
 * A dimension specified by width and height.
 */
typedef struct {
     int            w;   /* width of it */
     int            h;   /* height of it */
} DFBDimension;

/*
 * A rectangle specified by a point and a dimension.
 */
typedef struct {
     int            x;   /* X coordinate of its top-left point */
     int            y;   /* Y coordinate of its top-left point */
     int            w;   /* width of it */
     int            h;   /* height of it */
} DFBRectangle;

/*
 * A rectangle specified by normalized coordinates.
 *
 * E.g. using 0.0, 0.0, 1.0, 1.0 would specify the whole screen.
 */
typedef struct {
     float          x;   /* normalized X coordinate */
     float          y;   /* normalized Y coordinate */
     float          w;   /* normalized width */
     float          h;   /* normalized height */
} DFBLocation;

/*
 * A region specified by two points.
 *
 * The defined region includes both endpoints.
 */
typedef struct {
     int            x1;  /* X coordinate of top-left point */
     int            y1;  /* Y coordinate of top-left point */
     int            x2;  /* X coordinate of lower-right point */
     int            y2;  /* Y coordinate of lower-right point */
} DFBRegion;

/*
 * Insets specify a distance from each edge of a rectangle.
 *
 * Positive values always mean 'outside'.
 */
typedef struct {
     int            l;   /* distance from left edge */
     int            t;   /* distance from top edge */
     int            r;   /* distance from right edge */
     int            b;   /* distance from bottom edge */
} DFBInsets;

/*
 * A triangle specified by three points.
 */
typedef struct {
     int            x1;  /* X coordinate of first edge */
     int            y1;  /* Y coordinate of first edge */
     int            x2;  /* X coordinate of second edge */
     int            y2;  /* Y coordinate of second edge */
     int            x3;  /* X coordinate of third edge */
     int            y3;  /* Y coordinate of third edge */
} DFBTriangle;

/*
 * A color defined by channels with 8bit each.
 */
typedef struct {
     __u8           a;   /* alpha channel */
     __u8           r;   /* red channel */
     __u8           g;   /* green channel */
     __u8           b;   /* blue channel */
} DFBColor;

/*
 * Macro to compare two rectangles.
 */
#define DFB_RECTANGLE_EQUAL(a,b)  ((a).x == (b).x &&  \
                                   (a).y == (b).y &&  \
                                   (a).w == (b).w &&  \
                                   (a).h == (b).h)

/*
 * Macro to compare two locations.
 */
#define DFB_LOCATION_EQUAL(a,b)  ((a).x == (b).x &&  \
                                  (a).y == (b).y &&  \
                                  (a).w == (b).w &&  \
                                  (a).h == (b).h)

/*
 * Macro to compare two regions.
 */
#define DFB_REGION_EQUAL(a,b)  ((a).x1 == (b).x1 &&  \
                                (a).y1 == (b).y1 &&  \
                                (a).x2 == (b).x2 &&  \
                                (a).y2 == (b).y2)

/*
 * Macro to compare two colors.
 */
#define DFB_COLOR_EQUAL(x,y)  ((x).a == (y).a &&  \
                               (x).r == (y).r &&  \
                               (x).g == (y).g &&  \
                               (x).b == (y).b)

/*
 * Print a description of the result code along with an
 * optional message that is put in front with a colon.
 */
DFBResult DirectFBError(
                             const char  *msg,    /* optional message */
                             DFBResult    result  /* result code to interpret */
                       );

/*
 * Behaves like DirectFBError, but shuts down the calling application.
 */
DFBResult DirectFBErrorFatal(
                             const char  *msg,    /* optional message */
                             DFBResult    result  /* result code to interpret */
                            );

/*
 * Returns a string describing 'result'.
 */
const char *DirectFBErrorString(
                         DFBResult    result
                      );

/*
 * Retrieves information about supported command-line flags in the
 * form of a user-readable string formatted suitable to be printed
 * as usage information.
 */
const char *DirectFBUsageString( void );

/*
 * Parses the command-line and initializes some variables. You
 * absolutely need to call this before doing anything else.
 * Removes all options used by DirectFB from argv.
 */
DFBResult DirectFBInit(
                         int         *argc,   /* pointer to main()'s argc */
                         char       **argv[]  /* pointer to main()'s argv */
                      );

/*
 * Sets configuration parameters supported on command line and in
 * config file. Can only be called before DirectFBCreate but after
 * DirectFBInit.
 */
DFBResult DirectFBSetOption(
                         const char  *name,
                         const char  *value
                      );

/*
 * Creates the super interface.
 */
DFBResult DirectFBCreate(
                          IDirectFB **interface  /* pointer to the
                                                    created interface */
                        );


typedef unsigned int DFBScreenID;
typedef unsigned int DFBDisplayLayerID;
typedef unsigned int DFBDisplayLayerSourceID;
typedef unsigned int DFBWindowID;
typedef unsigned int DFBInputDeviceID;

typedef __u32 DFBDisplayLayerIDs;

/*
 * Maximum number of layer ids.
 */
#define DFB_DISPLAYLAYER_IDS_MAX             256

/*
 * Adds the id to the bitmask of layer ids.
 */
#define DFB_DISPLAYLAYER_IDS_ADD(ids,id)     (ids) |=  (1 << (id))

/*
 * Removes the id from the bitmask of layer ids.
 */
#define DFB_DISPLAYLAYER_IDS_REMOVE(ids,id)  (ids) &= ~(1 << (id))

/*
 * Checks if the bitmask of layer ids contains the id.
 */
#define DFB_DISPLAYLAYER_IDS_HAVE(ids,id)    ((ids) & (1 << (id)))

/*
 * Empties (clears) the bitmask of layer ids.
 */
#define DFB_DISPLAYLAYER_IDS_EMPTY(ids)      (ids) = 0

/*
 * The cooperative level controls the super interface's behaviour
 * in functions like SetVideoMode or CreateSurface for the primary.
 */
typedef enum {
     DFSCL_NORMAL        = 0x00000000,  /* Normal shared access, primary
                                           surface will be the buffer of an
                                           implicitly created window at the
                                           resolution given by SetVideoMode().
                                           */
     DFSCL_FULLSCREEN,                  /* Application grabs the primary layer,
                                           SetVideoMode automates layer
                                           control. Primary surface is the
                                           primary layer surface. */
     DFSCL_EXCLUSIVE                    /* All but the primary layer will be
                                           disabled, the application has full
                                           control over layers if desired,
                                           other applications have no
                                           input/output/control. Primary
                                           surface is the primary layer
                                           surface. */
} DFBCooperativeLevel;

/*
 * Capabilities of a display layer.
 */
typedef enum {
     DLCAPS_NONE              = 0x00000000,

     DLCAPS_SURFACE           = 0x00000001,  /* The layer has a surface that can be drawn to. This
                                                may not be provided by layers that display realtime
                                                data, e.g. from an MPEG decoder chip. Playback
                                                control may be provided by an external API. */
     DLCAPS_OPACITY           = 0x00000002,  /* The layer supports blending with layer(s) below
                                                based on a global alpha factor. */
     DLCAPS_ALPHACHANNEL      = 0x00000004,  /* The layer supports blending with layer(s) below
                                                based on each pixel's alpha value. */
     DLCAPS_SCREEN_LOCATION   = 0x00000008,  /* The layer location on the screen can be changed,
                                                this includes position and size as normalized
                                                values. The default is 0.0f, 0.0f, 1.0f, 1.0f. */
     DLCAPS_FLICKER_FILTERING = 0x00000010,  /* Flicker filtering can be enabled for smooth output
                                                on interlaced display devices. */
     DLCAPS_DEINTERLACING     = 0x00000020,  /* The layer provides optional deinterlacing for
                                                displaying interlaced video data on progressive
                                                display devices. */
     DLCAPS_SRC_COLORKEY      = 0x00000040,  /* A specific color can be declared as transparent. */
     DLCAPS_DST_COLORKEY      = 0x00000080,  /* A specific color of layers below can be specified
                                                as the color of the only locations where the layer
                                                is visible. */
     DLCAPS_BRIGHTNESS        = 0x00000100,  /* Adjustment of brightness is supported. */
     DLCAPS_CONTRAST          = 0x00000200,  /* Adjustment of contrast is supported. */
     DLCAPS_HUE               = 0x00000400,  /* Adjustment of hue is supported. */
     DLCAPS_SATURATION        = 0x00000800,  /* Adjustment of saturation is supported. */
     DLCAPS_LEVELS            = 0x00001000,  /* Adjustment of the layer's level
                                                (z position) is supported. */
     DLCAPS_FIELD_PARITY      = 0x00002000,  /* Field parity can be selected */
     DLCAPS_WINDOWS           = 0x00004000,  /* Hardware window support. */
     DLCAPS_SOURCES           = 0x00008000,  /* Sources can be selected. */
     DLCAPS_ALPHA_RAMP        = 0x00010000,  /* Alpha values for formats with one or two alpha bits
                                                can be chosen, i.e. using ARGB1555 or ARGB2554 the
                                                user can define the meaning of the two or four
                                                possibilities. In short, this feature provides a
                                                lookup table for the alpha bits of these formats.
                                                See also IDirectFBSurface::SetAlphaRamp(). */
     DLCAPS_PREMULTIPLIED     = 0x00020000,  /* Surfaces with premultiplied alpha are supported. */

     DLCAPS_SCREEN_POSITION   = 0x00100000,
     DLCAPS_SCREEN_SIZE       = 0x00200000,

     DLCAPS_ALL               = 0x0033FFFF
} DFBDisplayLayerCapabilities;

/*
 * Capabilities of a screen.
 */
typedef enum {
     DSCCAPS_NONE             = 0x00000000,

     DSCCAPS_VSYNC            = 0x00000001,  /* Synchronization with the
                                                vertical retrace supported. */
     DSCCAPS_POWER_MANAGEMENT = 0x00000002,  /* Power management supported. */

     DSCCAPS_MIXERS           = 0x00000010,  /* Has mixers. */
     DSCCAPS_ENCODERS         = 0x00000020,  /* Has encoders. */
     DSCCAPS_OUTPUTS          = 0x00000040,  /* Has outputs. */

     DSCCAPS_ALL              = 0x00000073
} DFBScreenCapabilities;

/*
 * Used to enable some capabilities like flicker filtering or colorkeying.
 */
typedef enum {
     DLOP_NONE                = 0x00000000,  /* None of these. */
     DLOP_ALPHACHANNEL        = 0x00000001,  /* Make usage of alpha channel
                                                for blending on a pixel per
                                                pixel basis. */
     DLOP_FLICKER_FILTERING   = 0x00000002,  /* Enable flicker filtering. */
     DLOP_DEINTERLACING       = 0x00000004,  /* Enable deinterlacing of an
                                                interlaced (video) source. */
     DLOP_SRC_COLORKEY        = 0x00000008,  /* Enable source color key. */
     DLOP_DST_COLORKEY        = 0x00000010,  /* Enable dest. color key. */
     DLOP_OPACITY             = 0x00000020,  /* Make usage of the global alpha
                                                factor set by SetOpacity. */
     DLOP_FIELD_PARITY        = 0x00000040   /* Set field parity */
} DFBDisplayLayerOptions;

/*
 * Layer Buffer Mode.
 */
typedef enum {
     DLBM_UNKNOWN    = 0x00000000,

     DLBM_FRONTONLY  = 0x00000001,      /* no backbuffer */
     DLBM_BACKVIDEO  = 0x00000002,      /* backbuffer in video memory */
     DLBM_BACKSYSTEM = 0x00000004,      /* backbuffer in system memory */
     DLBM_TRIPLE     = 0x00000008,      /* triple buffering */
     DLBM_WINDOWS    = 0x00000010       /* no layer buffers at all,
                                           using buffer of each window */
} DFBDisplayLayerBufferMode;

/*
 * Flags defining which fields of a DFBSurfaceDescription are valid.
 */
typedef enum {
     DSDESC_CAPS         = 0x00000001,  /* caps field is valid */
     DSDESC_WIDTH        = 0x00000002,  /* width field is valid */
     DSDESC_HEIGHT       = 0x00000004,  /* height field is valid */
     DSDESC_PIXELFORMAT  = 0x00000008,  /* pixelformat field is valid */
     DSDESC_PREALLOCATED = 0x00000010,  /* Surface uses data that has been
                                           preallocated by the application.
                                           The field array 'preallocated'
                                           has to be set using the first
                                           element for the front buffer
                                           and eventually the second one
                                           for the back buffer. */
     DSDESC_PALETTE      = 0x00000020,  /* Initialize the surfaces palette
                                           with the entries specified in the
                                           description. */
     DSDESC_DFBPALETTEHANDLE
                         = 0x00000040   /* Pointer to a IDirectFBPalette */
} DFBSurfaceDescriptionFlags;

/*
 * Flags defining which fields of a DFBPaletteDescription are valid.
 */
typedef enum {
     DPDESC_CAPS         = 0x00000001,  /* Specify palette capabilities. */
     DPDESC_SIZE         = 0x00000002,  /* Specify number of entries. */
     DPDESC_ENTRIES      = 0x00000004   /* Initialize the palette with the
                                           entries specified in the
                                           description. */
} DFBPaletteDescriptionFlags;

/*
 * The surface capabilities.
 */
typedef enum {
     DSCAPS_NONE                   = 0x00000000,  /* None of these. */

     DSCAPS_PRIMARY                = 0x00000001,  /* It's the primary surface. */
     DSCAPS_SYSTEMONLY             = 0x00000002,  /* Surface data is permanently stored in system memory.<br>
                                                     There's no video memory allocation/storage. */
     DSCAPS_VIDEOONLY              = 0x00000004,  /* Surface data is permanently stored in video memory.<br>
                                                     There's no system memory allocation/storage. */
     DSCAPS_DOUBLE                 = 0x00000010,  /* Surface is double buffered */
     DSCAPS_SUBSURFACE             = 0x00000020,  /* Surface is just a sub area of another
                                                     one sharing the surface data. */
     DSCAPS_INTERLACED             = 0x00000040,  /* Each buffer contains interlaced video (or graphics)
                                                     data consisting of two fields.<br>
                                                     Their lines are stored interleaved. One field's height
                                                     is a half of the surface's height. */
     DSCAPS_SEPARATED              = 0x00000080,  /* For usage with DSCAPS_INTERLACED.<br>
                                                     DSCAPS_SEPARATED specifies that the fields are NOT
                                                     interleaved line by line in the buffer.<br>
                                                     The first field is followed by the second one. */
     DSCAPS_STATIC_ALLOC           = 0x00000100,  /* The amount of video or system memory allocated for the
                                                     surface is never less than its initial value. This way
                                                     a surface can be resized (smaller and bigger up to the
                                                     initial size) without reallocation of the buffers. It's
                                                     useful for surfaces that need a guaranteed space in
                                                     video memory after resizing. */
     DSCAPS_TRIPLE                  = 0x00000200,  /* Surface is triple buffered. */

     DSCAPS_PREMULTIPLIED           = 0x00001000,  /* Surface stores data with premultiplied alpha. */

     DSCAPS_DEPTH                   = 0x00010000,  /* A depth buffer is allocated. */

     DSCAPS_ALLOC_ALIGNED_HEIGHT_4  = 0x00100000,  /* Allocate the buffer with an 4 byte aligend height.
                                                      This will not change the surface height, on the memory
                                                      allocated for it. */
     DSCAPS_ALLOC_ALIGNED_HEIGHT_8  = 0x00200000,  /* Allocate the buffer with an 4 byte aligend height.
                                                      This will not change the surface height, on the memory
                                                      allocated for it. */
     DSCAPS_ALLOC_ALIGNED_HEIGHT_16 = 0x00400000,  /* Allocate the buffer with an 4 byte aligend height.
                                                      This will not change the surface height, on the memory
                                                      allocated for it. */
     DSCAPS_ALLOC_ALIGNED_WIDTH_4   = 0x01000000,  /* Allocate the buffer with an 4 byte aligend width.
                                                      This will not change the surface width, on the memory
                                                      allocated for it. */
     DSCAPS_ALLOC_ALIGNED_WIDTH_8   = 0x02000000,  /* Allocate the buffer with an 4 byte aligend width.
                                                      This will not change the surface width, on the memory
                                                      allocated for it. */
     DSCAPS_ALLOC_ALIGNED_WIDTH_16  = 0x04000000,  /* Allocate the buffer with an 4 byte aligend width.
                                                      This will not change the surface width, on the memory
                                                      allocated for it. */
     DSCAPS_OPTI_BLIT               = 0x08000000,  /* Optimized the blitting operation for this surface */

     DSCAPS_COPY_BLIT_PG            = 0x10000000,  /* Surface to be used only for BD presentation graphics.
                                                      Blit operations with any flags other than DSBLIT_NOFX
                                                      will fail. */

     DSCAPS_COPY_BLIT_TEXTST        = 0x20000000,  /* Surface to be used only for BD text subtitle graphics.
                                                      Blit operations with any flags other than DSBLIT_NOFX
                                                      will fail. */

     DSCAPS_COPY_BLIT_IG            = 0x40000000,  /* Surface to be used only for BD interactive graphics.
                                                      Blit operations with any flags other than DSBLIT_NOFX
                                                      will fail. */

     DSCAPS_COPY_BLIT_OTHR          = 0x80000000,  /* Surface to be used only for graphics that will not be
                                                      in use simultaneous with any BD graphics. Blit operations 
                                                      with any flags other than DSBLIT_NOFX will fail. */

     DSCAPS_ALL                     = 0x000113F7,  /* All of these. */


     DSCAPS_FLIPPING      = DSCAPS_DOUBLE | DSCAPS_TRIPLE /* Surface needs Flip() calls to make
                                                             updates/changes visible/usable. */
} DFBSurfaceCapabilities;

/*
 * The palette capabilities.
 */
typedef enum {
     DPCAPS_NONE         = 0x00000000   /* None of these. */
} DFBPaletteCapabilities;

/*
 * Flags controlling drawing commands.
 */
typedef enum {
     DSDRAW_NOFX               = 0x00000000, /* uses none of the effects */
     DSDRAW_BLEND              = 0x00000001, /* uses alpha from color */
     DSDRAW_DST_COLORKEY       = 0x00000002, /* write to destination only if the destination pixel
                                                matches the destination color key */
     DSDRAW_SRC_PREMULTIPLY    = 0x00000004, /* multiplies the color's rgb channels by the alpha
                                                channel before drawing */
     DSDRAW_DST_PREMULTIPLY    = 0x00000008, /* modulates the dest. color with the dest. alpha */
     DSDRAW_DEMULTIPLY         = 0x00000010, /* divides the color by the alpha before writing the
                                                data to the destination */
     DSDRAW_XOR                = 0x00000020  /* bitwise xor the destination pixels with the
                                                specified color after premultiplication */
} DFBSurfaceDrawingFlags;

/*
 * Flags controlling blitting commands.
 */
typedef enum {
     DSBLIT_NOFX               = 0x00000000, /* uses none of the effects */
     DSBLIT_BLEND_ALPHACHANNEL = 0x00000001, /* enables blending and uses
                                                alphachannel from source */
     DSBLIT_BLEND_COLORALPHA   = 0x00000002, /* enables blending and uses
                                                alpha value from color */
     DSBLIT_COLORIZE           = 0x00000004, /* modulates source color with
                                                the color's r/g/b values */
     DSBLIT_SRC_COLORKEY       = 0x00000008, /* don't blit pixels matching the source color key */
     DSBLIT_DST_COLORKEY       = 0x00000010, /* write to destination only if the destination pixel
                                                matches the destination color key */
     DSBLIT_SRC_PREMULTIPLY    = 0x00000020, /* modulates the source color with the (modulated)
                                                source alpha */
     DSBLIT_DST_PREMULTIPLY    = 0x00000040, /* modulates the dest. color with the dest. alpha */
     DSBLIT_DEMULTIPLY         = 0x00000080, /* divides the color by the alpha before writing the
                                                data to the destination */
     DSBLIT_DEINTERLACE        = 0x00000100, /* deinterlaces the source during blitting by reading
                                                only one field (every second line of full
                                                image) scaling it vertically by factor two */
     DSBLIT_XOR                = 0x00000200  /* bitwise xor the destination pixels with the source
                                                pixels after premultiplication */
} DFBSurfaceBlittingFlags;

/*
 * Mask of accelerated functions.
 */
typedef enum {
     DFXL_NONE           = 0x00000000,  /* None of these. */

     DFXL_FILLRECTANGLE  = 0x00000001,  /* FillRectangle() is accelerated. */
     DFXL_DRAWRECTANGLE  = 0x00000002,  /* DrawRectangle() is accelerated. */
     DFXL_DRAWLINE       = 0x00000004,  /* DrawLine() is accelerated. */
     DFXL_FILLTRIANGLE   = 0x00000008,  /* FillTriangle() is accelerated. */

     DFXL_BLIT           = 0x00010000,  /* Blit() and TileBlit() are accelerated. */
     DFXL_STRETCHBLIT    = 0x00020000,  /* StretchBlit() is accelerated. */
     DFXL_TEXTRIANGLES   = 0x00040000,  /* TextureTriangles() is accelerated. */

     DFXL_DRAWSTRING     = 0x01000000,  /* DrawString() and DrawGlyph() are accelerated. */

     DFXL_ALL            = 0x0107000F   /* All drawing/blitting functions. */
} DFBAccelerationMask;


/*
 * @internal
 */
#define DFB_DRAWING_FUNCTION(a)    ((a) & 0x0000FFFF)

/*
 * @internal
 */
#define DFB_BLITTING_FUNCTION(a)   ((a) & 0xFFFF0000)

/*
 * Rough information about hardware capabilities.
 */
typedef struct {
     DFBAccelerationMask     acceleration_mask;   /* drawing/blitting functions */
     DFBSurfaceDrawingFlags  drawing_flags;       /* drawing flags */
     DFBSurfaceBlittingFlags blitting_flags;      /* blitting flags */
     unsigned int            video_memory;        /* amount of video memory in bytes */
} DFBCardCapabilities;

/*
 * Type of display layer for basic classification.
 * Values may be or'ed together.
 */
typedef enum {
     DLTF_NONE           = 0x00000000,  /* Unclassified, no specific type. */

     DLTF_GRAPHICS       = 0x00000001,  /* Can be used for graphics output. */
     DLTF_VIDEO          = 0x00000002,  /* Can be used for live video output.*/
     DLTF_STILL_PICTURE  = 0x00000004,  /* Can be used for single frames. */
     DLTF_BACKGROUND     = 0x00000008,  /* Can be used as a background layer.*/

     DLTF_ALL            = 0x0000000F   /* All type flags set. */
} DFBDisplayLayerTypeFlags;

/*
 * Type of input device for basic classification.
 * Values may be or'ed together.
 */
typedef enum {
     DIDTF_NONE          = 0x00000000,  /* Unclassified, no specific type. */

     DIDTF_KEYBOARD      = 0x00000001,  /* Can act as a keyboard. */
     DIDTF_MOUSE         = 0x00000002,  /* Can be used as a mouse. */
     DIDTF_JOYSTICK      = 0x00000004,  /* Can be used as a joystick. */
     DIDTF_REMOTE        = 0x00000008,  /* Is a remote control. */
     DIDTF_VIRTUAL       = 0x00000010,  /* Is a virtual input device. */

     DIDTF_ALL           = 0x0000001F   /* All type flags set. */
} DFBInputDeviceTypeFlags;

/*
 * Basic input device features.
 */
typedef enum {
     DICAPS_KEYS         = 0x00000001,  /* device supports key events */
     DICAPS_AXES         = 0x00000002,  /* device supports axis events */
     DICAPS_BUTTONS      = 0x00000004,  /* device supports button events */

     DICAPS_ALL          = 0x00000007   /* all capabilities */
} DFBInputDeviceCapabilities;

/*
 * Identifier (index) for e.g. mouse or joystick buttons.
 */
typedef enum {
     DIBI_LEFT           = 0x00000000,  /* left mouse button */
     DIBI_RIGHT          = 0x00000001,  /* right mouse button */
     DIBI_MIDDLE         = 0x00000002,  /* middle mouse button */

     DIBI_FIRST          = DIBI_LEFT,   /* other buttons:
                                           DIBI_FIRST + zero based index */
     DIBI_LAST           = 0x0000001F   /* 32 buttons maximum */
} DFBInputDeviceButtonIdentifier;

/*
 * Axis identifier (index) for e.g. mouse or joystick.
 *
 * The X, Y and Z axis are predefined. To access other axes,
 * use DIAI_FIRST plus a zero based index, e.g. the 4th axis
 * would be (DIAI_FIRST + 3).
 */
typedef enum {
     DIAI_X              = 0x00000000,  /* X axis */
     DIAI_Y              = 0x00000001,  /* Y axis */
     DIAI_Z              = 0x00000002,  /* Z axis */

     DIAI_FIRST          = DIAI_X,      /* other axis:
                                           DIAI_FIRST + zero based index */
     DIAI_LAST           = 0x0000001F   /* 32 axes maximum */
} DFBInputDeviceAxisIdentifier;

/*
 * Flags defining which fields of a DFBWindowDescription are valid.
 */
typedef enum {
     DWDESC_CAPS         = 0x00000001,  /* caps field is valid */
     DWDESC_WIDTH        = 0x00000002,  /* width field is valid */
     DWDESC_HEIGHT       = 0x00000004,  /* height field is valid */
     DWDESC_PIXELFORMAT  = 0x00000008,  /* pixelformat field is valid */
     DWDESC_POSX         = 0x00000010,  /* posx field is valid */
     DWDESC_POSY         = 0x00000020,  /* posy field is valid */
     DWDESC_SURFACE_CAPS = 0x00000040   /* Create the window surface with
                                           special capabilities. */
} DFBWindowDescriptionFlags;

/*
 * Flags defining which fields of a DFBDataBufferDescription are valid.
 */
typedef enum {
     DBDESC_FILE         = 0x00000001,  /* Create a static buffer for the
                                           specified filename. */
     DBDESC_MEMORY       = 0x00000002   /* Create a static buffer for the
                                           specified memory area. */
} DFBDataBufferDescriptionFlags;

/*
 * Capabilities a window can have.
 */
typedef enum {
     DWCAPS_NONE         = 0x00000000,  /* None of these. */
     DWCAPS_ALPHACHANNEL = 0x00000001,  /* The window has an alphachannel
                                           for pixel-per-pixel blending. */
     DWCAPS_DOUBLEBUFFER = 0x00000002,  /* The window's surface is double
                                           buffered. This is very useful
                                           to avoid visibility of content
                                           that is still in preparation.
                                           Normally a window's content can
                                           get visible before an update if
                                           there is another reason causing
                                           a window stack repaint. */
     DWCAPS_INPUTONLY    = 0x00000004,  /* The window has no surface.
                                           You can not draw to it but it
                                           receives events */
     DWCAPS_NODECORATION = 0x00000008,  /* The window won't be decorated. */
     DWCAPS_ALL          = 0x0000000F   /* All valid flags. */
} DFBWindowCapabilities;


/*
 * Flags describing how to load a font.
 *
 * These flags describe how a font is loaded and affect how the
 * glyphs are drawn. There is no way to change this after the font
 * has been loaded. If you need to render a font with different
 * attributes, you have to create multiple FontProviders of the
 * same font file.
 */
typedef enum {
     DFFA_NONE           = 0x00000000,  /* none of these flags */
     DFFA_NOKERNING      = 0x00000001,  /* don't use kerning */
     DFFA_NOHINTING      = 0x00000002,  /* don't use hinting */
     DFFA_MONOCHROME     = 0x00000004,  /* don't use anti-aliasing */
     DFFA_NOCHARMAP      = 0x00000008,  /* no char map, glyph indices are
                                           specified directly */
     DFFA_ITALIC         = 0x00000010,  /* Force italic font display */
     DFFA_REVERSE_ITALIC = 0x00000020,  /* Force reverse italic font display */
     DFFA_ROTATE_90      = 0x00000040,  /* Rotate the text 90 degrees and display */
     DFFA_ROTATE_180     = 0x00000080,  /* Rotate the text 180 degrees and display */
     DFFA_ROTATE_270     = 0x00000100,  /* Rotate the text 270 degrees and display */
     DFFA_ROTATE_X       = 0x00000200,  /* Rotate the text X degrees and display.
                                           Specify the desired number of degrees in
                                           DFBFontDescription */
     DFFA_STROKING        = 0x00000400                                      

} DFBFontAttributes;

/*
 * Flags defining which fields of a DFBFontDescription are valid.
 */
typedef enum {
     DFDESC_ATTRIBUTES   = 0x00000001,  /* attributes field is valid */
     DFDESC_HEIGHT       = 0x00000002,  /* height is specified */
     DFDESC_WIDTH        = 0x00000004,  /* width is specified */
     DFDESC_INDEX        = 0x00000008,  /* index is specified */
     DFDESC_FIXEDADVANCE = 0x00000010,  /* specify a fixed advance overriding
                                           any character advance of fixed or
                                           proportional fonts */
     DFDESC_LAYOUT       = 0x00000020   /* index is specified */
} DFBFontDescriptionFlags;


typedef enum {
     DFFL_HORIZONTAL     = 0x00000001,  /* Display the text on the screen horizontally in the default
                                           direction.  If a specific direction is needed use either */
     DFFL_VERTICAL       = 0x00000002,   /* Display the text on the screen vertically.  This layout is
                                           not available with most fonts.  When vertical layout is
                                           not supported, horizontal layout will be used */

     DFFL_LEFT_TO_RIGHT   = (0x00000010 | DFFL_HORIZONTAL),  /* Display the text on the screen from
                                                                left to right. (default) */
     DFFL_RIGHT_TO_LEFT   = (0x00000020 | DFFL_HORIZONTAL),  /* Display the text on the screen from
                                                                right to left. */
     DFFL_TOP_TO_BOTTOM   = (0x00000040 | DFFL_VERTICAL),    /* Display the text on the screen from
                                                                top to bottom. (default) */
     DFFL_BOTTOM_TO_TOP   = (0x00000080 | DFFL_VERTICAL)     /* Display the text on the screen from
                                                                bottom to top. */
     /* Note: In order for DFFL_VERTICAL, DFFL_TOP_TO_BOTTOM or DFFL_BOTTOM_TO_TOP layouts to be used,
        the font must provide the vertical metrics.  In the event a font does not support vertical display,
        DFFL_LEFT_TO_RIGHT will be used as the layout */

} DFBFontLayout;

typedef enum {
     DFB_STROKER_LINECAP_BUTT = 0,
     DFB_STROKER_LINECAP_ROUND,
     DFB_STROKER_LINECAP_SQUARE
} DFBStrokerLineCap;

typedef enum {
     DFB_STROKER_LINEJOIN_ROUND = 0,
     DFB_STROKER_LINEJOIN_BEVEL,
     DFB_STROKER_LINEJOIN_MITER
} DFBStrokerLineJoin;

/*
 * Structure defining the typographic bounding box of a font in integer pixels. The 
 * defined bounding box includes all endpoints.  
 *  
 * Note that due to typical origin placement when a font is designed, yMin 
 * will likely be negative for a horizontal font while xMin, yMin, and yMax 
 * will likely be negative for a vertical font.  
 */
typedef struct {
     int  xMin;
     int  yMin;
     int  xMax;
     int  yMax;
} DFBBoundingBox;


/*
 * Description of how to load glyphs from a font file.
 *
 * The attributes control how the glyphs are rendered. Width and height can be used to specify the
 * desired face size in pixels. If you are loading a non-scalable font, you shouldn't specify a
 * font size.
 *
 * Please note that the height value in the DFBFontDescription doesn't correspond to the height
 * returned by IDirectFBFont::GetHeight().
 *
 * The index field controls which face is loaded from a font file that provides a collection of
 * faces. This is rarely needed.
 */
typedef struct {
     DFBFontDescriptionFlags            flags;
     DFBFontAttributes                  attributes;
     DFBFontLayout                      layout;
     int                                height;
     int                                width;
     unsigned int                       index;
     int                                fixed_advance;
     double                             rotate_degrees;      
     int                                stroking_radius;     
     DFBStrokerLineCap                  stroking_lineCap;
     DFBStrokerLineJoin                 stroking_lineJoin;
     unsigned char                      borderColorIndex;
     unsigned char                      backgroundColorIndex;
     unsigned char                      fontColorIndex;
} DFBFontDescription;

/*
 * @internal
 *
 * Encodes format constants in the following way (bit 31 - 0):
 *
 * lkjj:hhgg | gfff:eeed | cccc:bbbb | baaa:aaaa
 *
 * a) pixelformat index<br>
 * b) effective color (or index) bits per pixel of format<br>
 * c) effective alpha bits per pixel of format<br>
 * d) alpha channel present<br>
 * e) bytes per "pixel in a row" (1/8 fragment, i.e. bits)<br>
 * f) bytes per "pixel in a row" (decimal part, i.e. bytes)<br>
 * g) smallest number of pixels aligned to byte boundary (minus one)<br>
 * h) multiplier for planes minus one (1/4 fragment)<br>
 * j) multiplier for planes minus one (decimal part)<br>
 * k) color and/or alpha lookup table present<br>
 * l) alpha channel is inverted
 */
#define DFB_SURFACE_PIXELFORMAT( index, color_bits, alpha_bits, has_alpha,     \
                                 row_bits, row_bytes, align, mul_f, mul_d,     \
                                 has_lut, inv_alpha )                          \
     ( (((index     ) & 0x7F)      ) |                                         \
       (((color_bits) & 0x1F) <<  7) |                                         \
       (((alpha_bits) & 0x0F) << 12) |                                         \
       (((has_alpha ) ? 1 :0) << 16) |                                         \
       (((row_bits  ) & 0x07) << 17) |                                         \
       (((row_bytes ) & 0x07) << 20) |                                         \
       (((align     ) & 0x07) << 23) |                                         \
       (((mul_f     ) & 0x03) << 26) |                                         \
       (((mul_d     ) & 0x03) << 28) |                                         \
       (((has_lut   ) ? 1 :0) << 30) |                                         \
       (((inv_alpha ) ? 1 :0) << 31) )

/*
 * Pixel format of a surface.
 */
typedef enum {
     DSPF_UNKNOWN   = 0x00000000,  /* unknown or unspecified format */

     /* 16 bit  ARGB (2 byte, alpha 1@15, red 5@10, green 5@5, blue 5@0) */
     DSPF_ARGB1555  = DFB_SURFACE_PIXELFORMAT(  0, 15, 1, 1, 0, 2, 0, 0, 0, 0, 0 ),

     /* 16 bit   RGB (2 byte, red 5@11, green 6@5, blue 5@0) */
     DSPF_RGB16     = DFB_SURFACE_PIXELFORMAT(  1, 16, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),

     /* 24 bit   RGB (3 byte, red 8@16, green 8@8, blue 8@0) */
     DSPF_RGB24     = DFB_SURFACE_PIXELFORMAT(  2, 24, 0, 0, 0, 3, 0, 0, 0, 0, 0 ),

     /* 24 bit   RGB (4 byte, nothing@24, red 8@16, green 8@8, blue 8@0) */
     DSPF_RGB32     = DFB_SURFACE_PIXELFORMAT(  3, 24, 0, 0, 0, 4, 0, 0, 0, 0, 0 ),

     /* 32 bit  ARGB (4 byte, alpha 8@24, red 8@16, green 8@8, blue 8@0) */
     DSPF_ARGB      = DFB_SURFACE_PIXELFORMAT(  4, 24, 8, 1, 0, 4, 0, 0, 0, 0, 0 ),

     /*  8 bit alpha (1 byte, alpha 8@0), e.g. anti-aliased glyphs */
     DSPF_A8        = DFB_SURFACE_PIXELFORMAT(  5,  0, 8, 1, 0, 1, 0, 0, 0, 0, 0 ),

     /* 16 bit   YUV (4 byte/ 2 pixel, macropixel contains CbYCrY [31:0]) */
     DSPF_YUY2      = DFB_SURFACE_PIXELFORMAT(  6, 16, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),

     /*  8 bit   RGB (1 byte, red 3@5, green 3@2, blue 2@0) */
     DSPF_RGB332    = DFB_SURFACE_PIXELFORMAT(  7,  8, 0, 0, 0, 1, 0, 0, 0, 0, 0 ),

     /* 16 bit   YUV (4 byte/ 2 pixel, macropixel contains YCbYCr [31:0]) */
     DSPF_UYVY      = DFB_SURFACE_PIXELFORMAT(  8, 16, 0, 0, 0, 2, 0, 0, 0, 0, 0 ),

     /* 12 bit   YUV (8 bit Y plane followed by 8 bit quarter size U/V planes) */
     DSPF_I420      = DFB_SURFACE_PIXELFORMAT(  9, 12, 0, 0, 0, 1, 0, 2, 0, 0, 0 ),

     /* 12 bit   YUV (8 bit Y plane followed by 8 bit quarter size V/U planes) */
     DSPF_YV12      = DFB_SURFACE_PIXELFORMAT( 10, 12, 0, 0, 0, 1, 0, 2, 0, 0, 0 ),

     /*  8 bit   LUT (8 bit color and alpha lookup from palette) */
     DSPF_LUT8      = DFB_SURFACE_PIXELFORMAT( 11,  8, 0, 1, 0, 1, 0, 0, 0, 1, 0 ),

     /*  8 bit  ALUT (1 byte, alpha 4@4, color lookup 4@0) */
     DSPF_ALUT44    = DFB_SURFACE_PIXELFORMAT( 12,  4, 4, 1, 0, 1, 0, 0, 0, 1, 0 ),

     /* 32 bit  ARGB (4 byte, inv. alpha 8@24, red 8@16, green 8@8, blue 8@0) */
     DSPF_AiRGB     = DFB_SURFACE_PIXELFORMAT( 13, 24, 8, 1, 0, 4, 0, 0, 0, 0, 1 ),

     /*  1 bit alpha (1 byte/ 8 pixel, most significant bit used first) */
     DSPF_A1        = DFB_SURFACE_PIXELFORMAT( 14,  0, 1, 1, 1, 0, 7, 0, 0, 0, 0 ),

     /* 12 bit   YUV (8 bit Y plane followed by one 16 bit quarter size CbCr [15:0] plane) */
     DSPF_NV12      = DFB_SURFACE_PIXELFORMAT( 15, 12, 0, 0, 0, 1, 0, 2, 0, 0, 0 ),

     /* 16 bit   YUV (8 bit Y plane followed by one 16 bit half width CbCr [15:0] plane) */
     DSPF_NV16      = DFB_SURFACE_PIXELFORMAT( 16, 24, 0, 0, 0, 1, 0, 0, 1, 0, 0 ),

     /* 16 bit  ARGB (2 byte, alpha 2@14, red 5@9, green 5@4, blue 4@0) */
     DSPF_ARGB2554  = DFB_SURFACE_PIXELFORMAT( 17, 14, 2, 1, 0, 2, 0, 0, 0, 0, 0 ),

     /* 16 bit  ARGB (2 byte, alpha 4@12, red 4@8, green 4@4, blue 4@0) */
     DSPF_ARGB4444  = DFB_SURFACE_PIXELFORMAT( 18, 12, 4, 1, 0, 2, 0, 0, 0, 0, 0 ),

     /* 12 bit   YUV (8 bit Y plane followed by one 16 bit quarter size CrCb [15:0] plane) */
     DSPF_NV21      = DFB_SURFACE_PIXELFORMAT( 19, 12, 0, 0, 0, 1, 0, 2, 0, 0, 0 ),

     /* 32 bit  AYUV (4 byte, alpha 8@24, Y 8@16, Cb 8@8, Cr 8@0) */
     DSPF_AYUV      = DFB_SURFACE_PIXELFORMAT( 20, 24, 8, 1, 0, 4, 0, 0, 0, 0, 0 ),

     /* 16 bit  ALUT (2 byte, alpha 8@8, color lookup 8@0) */
     DSPF_ALUT88    = DFB_SURFACE_PIXELFORMAT( 21,  8, 8, 1, 0, 2, 0, 0, 0, 1, 0 ),

     /* 32 bit  ABGR (4 byte, alpha 8@24, blue 8@0, green 8@8, red 8@16) */
     DSPF_ABGR      = DFB_SURFACE_PIXELFORMAT( 22, 24, 8, 1, 0, 4, 0, 0, 0, 0, 0 ),

     /*  4 bit   LUT (4 bit color and alpha lookup from palette) */
     DSPF_LUT4      = DFB_SURFACE_PIXELFORMAT( 23,  4, 0, 1, 4, 0, 3, 0, 0, 1, 0 ),

     /*  2 bit   LUT (2 bit color and alpha lookup from palette) */
     DSPF_LUT2      = DFB_SURFACE_PIXELFORMAT( 24,  2, 0, 1, 2, 0, 5, 0, 0, 1, 0 ),

     /*  1 bit   LUT (1 bit color and alpha lookup from palette) */
     DSPF_LUT1      = DFB_SURFACE_PIXELFORMAT( 25,  1, 0, 1, 1, 0, 7, 0, 0, 1, 0 )

} DFBSurfacePixelFormat;

/* Number of pixelformats defined */
#define DFB_NUM_PIXELFORMATS            26

/* These macros extract information about the pixel format. */
#define DFB_PIXELFORMAT_INDEX(fmt)      (((fmt) & 0x0000007F)      )

#define DFB_COLOR_BITS_PER_PIXEL(fmt)   (((fmt) & 0x00000F80) >>  7)

#define DFB_ALPHA_BITS_PER_PIXEL(fmt)   (((fmt) & 0x0000F000) >> 12)

#define DFB_PIXELFORMAT_HAS_ALPHA(fmt)  (((fmt) & 0x00010000) >> 16)

#define DFB_BITS_PER_PIXEL(fmt)         (((fmt) & 0x007E0000) >> 17)

#define DFB_BYTES_PER_PIXEL(fmt)        (((fmt) & 0x00700000) >> 20)

#define DFB_BYTES_PER_LINE(fmt,width)   (((((fmt) & 0x007E0000) >> 17) * (width) + 7) >> 3)

#define DFB_PIXELFORMAT_ALIGNMENT(fmt)  (((fmt) & 0x03800000) >> 23)

#define DFB_PLANE_MULTIPLY(fmt,height)  ((((((fmt) & 0x3C000000) >> 26) + 4) * (height)) >> 2)

#define DFB_PIXELFORMAT_IS_INDEXED(fmt) (((fmt) & 0x40000000) >> 30)

#define DFB_PLANAR_PIXELFORMAT(fmt)     (((fmt) & 0x3C000000) ? 1 : 0)

#define DFB_PIXELFORMAT_INV_ALPHA(fmt)  (((fmt) & 0x80000000) >> 31)


/*
 * Description of the surface that is to be created.
 */
typedef struct {
     DFBSurfaceDescriptionFlags         flags;       /* field validation */

     DFBSurfaceCapabilities             caps;        /* capabilities */
     int                                width;       /* pixel width */
     int                                height;      /* pixel height */
     DFBSurfacePixelFormat              pixelformat; /* pixel format */

     struct {
          void                         *data;        /* data pointer of existing buffer */
          int                           pitch;       /* pitch of buffer */
     } preallocated[2];

     struct {
          DFBColor                     *entries;
          unsigned int                  size;
     } palette;                                      /* initial palette values */

     IDirectFBPalette                  *dfbpalette;  /* initial IDirectFBPalette */
	 unsigned int isAnimated;
	 unsigned int num_frames;
	 DFBColor background_color;
	 
} DFBSurfaceDescription;

/*
 * Description of the palette that is to be created.
 */
typedef struct {
     DFBPaletteDescriptionFlags         flags;       /* Validation of fields. */

     DFBPaletteCapabilities             caps;        /* Palette capabilities. */
     unsigned int                       size;        /* Number of entries. */
     DFBColor                          *entries;     /* Preset palette
                                                        entries. */
} DFBPaletteDescription;


#define DFB_DISPLAY_LAYER_DESC_NAME_LENGTH   32

/*
 * Description of the display layer capabilities.
 */
typedef struct {
     DFBDisplayLayerTypeFlags           type;        /* Classification of the
                                                        display layer. */
     DFBDisplayLayerCapabilities        caps;        /* Capability flags of
                                                        the display layer. */

     char name[DFB_DISPLAY_LAYER_DESC_NAME_LENGTH];  /* Display layer name. */

     int                                level;       /* Default level. */
     int                                regions;     /* Number of concurrent regions supported.<br>
                                                        -1 = unlimited,
                                                         0 = unknown/one,
                                                        >0 = actual number */
     int                                sources;     /* Number of selectable sources. */
} DFBDisplayLayerDescription;


#define DFB_DISPLAY_LAYER_SOURCE_DESC_NAME_LENGTH    24

/*
 * Description of a display layer source.
 */
typedef struct {
     DFBDisplayLayerSourceID            source_id;          /* ID of the source. */

     char name[DFB_DISPLAY_LAYER_SOURCE_DESC_NAME_LENGTH];  /* Name of the source. */
} DFBDisplayLayerSourceDescription;


#define DFB_SCREEN_DESC_NAME_LENGTH          32

/*
 * Description of the display encoder capabilities.
 */
typedef struct {
     DFBScreenCapabilities              caps;        /* Capability flags of
                                                        the screen. */

     char name[DFB_SCREEN_DESC_NAME_LENGTH];         /* Rough description. */

     int                                mixers;      /* Number of mixers
                                                        available. */
     int                                encoders;    /* Number of display
                                                        encoders available. */
     int                                outputs;     /* Number of output
                                                        connectors available. */
} DFBScreenDescription;


#define DFB_INPUT_DEVICE_DESC_NAME_LENGTH    32
#define DFB_INPUT_DEVICE_DESC_VENDOR_LENGTH  40

/*
 * Description of the input device capabilities.
 */
typedef struct {
     DFBInputDeviceTypeFlags            type;        /* classification of
                                                        input device */
     DFBInputDeviceCapabilities         caps;        /* capabilities,
                                                        validates the
                                                        following fields */

     int                                min_keycode; /* minimum hardware
                                                        keycode or -1 if
                                                        no differentiation
                                                        between hardware
                                                        keys is made */
     int                                max_keycode; /* maximum hardware
                                                        keycode or -1 if
                                                        no differentiation
                                                        between hardware
                                                        keys is made */
     DFBInputDeviceAxisIdentifier       max_axis;    /* highest axis
                                                        identifier */
     DFBInputDeviceButtonIdentifier     max_button;  /* highest button
                                                        identifier */

     char name[DFB_INPUT_DEVICE_DESC_NAME_LENGTH];   /* Device name */

     char vendor[DFB_INPUT_DEVICE_DESC_VENDOR_LENGTH]; /* Device vendor */
} DFBInputDeviceDescription;

/*
 * Description of the window that is to be created.
 */
typedef struct {
     DFBWindowDescriptionFlags          flags;        /* field validation */

     DFBWindowCapabilities              caps;         /* capabilities */
     int                                width;        /* pixel width */
     int                                height;       /* pixel height */
     DFBSurfacePixelFormat              pixelformat;  /* pixel format */
     int                                posx;         /* distance from left layer border */
     int                                posy;         /* distance from upper layer border */
     DFBSurfaceCapabilities             surface_caps; /* pixel format */
} DFBWindowDescription;

/*
 * Description of a data buffer that is to be created.
 */
typedef struct {
     DFBDataBufferDescriptionFlags      flags;       /* field validation */

     const char                        *file;        /* for file based data buffers */

     struct {
          const void                   *data;        /* static data pointer */
          unsigned int                  length;      /* length of buffer */
     } memory;                                       /* memory based buffers */
} DFBDataBufferDescription;

/*
 * Return value of callback function of enumerations.
 */
typedef enum {
     DFENUM_OK           = 0x00000000,  /* Proceed with enumeration */
     DFENUM_CANCEL       = 0x00000001   /* Cancel enumeration */
} DFBEnumerationResult;

/*
 * Called for each supported video mode.
 */
typedef DFBEnumerationResult (*DFBVideoModeCallback) (
     int                       width,
     int                       height,
     int                       bpp,
     void                     *callbackdata
);

/*
 * Called for each existing screen.
 * "screen_id" can be used to get an interface to the screen.
 */
typedef DFBEnumerationResult (*DFBScreenCallback) (
     DFBScreenID                        screen_id,
     DFBScreenDescription               desc,
     void                              *callbackdata
);

/*
 * Called for each existing display layer.
 * "layer_id" can be used to get an interface to the layer.
 */
typedef DFBEnumerationResult (*DFBDisplayLayerCallback) (
     DFBDisplayLayerID                  layer_id,
     DFBDisplayLayerDescription         desc,
     void                              *callbackdata
);

/*
 * Called for each existing input device.
 * "device_id" can be used to get an interface to the device.
 */
typedef DFBEnumerationResult (*DFBInputDeviceCallback) (
     DFBInputDeviceID                   device_id,
     DFBInputDeviceDescription          desc,
     void                              *callbackdata
);

/*
 * Called for each block of continous data requested, e.g. by a
 * Video Provider. Write as many data as you can but not more
 * than specified by length. Return the number of bytes written
 * or 'EOF' if no data is available anymore.
 */
typedef int (*DFBGetDataCallback) (
     void                              *buffer,
     unsigned int                       length,
     void                              *callbackdata
);

/*
 * Information about an IDirectFBVideoProvider.
 */
typedef enum {
     DVCAPS_BASIC      = 0x00000000,  /* basic ops (PlayTo, Stop)       */
     DVCAPS_SEEK       = 0x00000001,  /* supports SeekTo                */
     DVCAPS_SCALE      = 0x00000002,  /* can scale the video            */
     DVCAPS_INTERLACED = 0x00000004,  /* supports interlaced surfaces   */
     DVCAPS_BRIGHTNESS = 0x00000010,  /* supports Brightness adjustment */
     DVCAPS_CONTRAST   = 0x00000020,  /* supports Contrast adjustment   */
     DVCAPS_HUE        = 0x00000040,  /* supports Hue adjustment        */
     DVCAPS_SATURATION  = 0x00000080,  /* supports Saturation adjustment */
     DVCAPS_INTERACTIVE = 0x00000100   /* supports SendEvent             */
} DFBVideoProviderCapabilities;

/*
 * Information about the status of an IDirectFBVideoProvider.
 */
typedef enum {
     DVSTATE_UNKNOWN    = 0x00000000, /* unknown status            */
     DVSTATE_PLAY       = 0x00000001, /* video provider is playing */
     DVSTATE_STOP       = 0x00000002, /* playback was stopped      */
     DVSTATE_FINISHED   = 0x00000003  /* playback is finished      */
} DFBVideoProviderStatus;

/*
 * Flags defining which fields of a DFBColorAdjustment are valid.
 */
typedef enum {
     DCAF_NONE         = 0x00000000,  /* none of these              */
     DCAF_BRIGHTNESS   = 0x00000001,  /* brightness field is valid  */
     DCAF_CONTRAST     = 0x00000002,  /* contrast field is valid    */
     DCAF_HUE          = 0x00000004,  /* hue field is valid         */
     DCAF_SATURATION   = 0x00000008   /* saturation field is valid  */
} DFBColorAdjustmentFlags;

/*
 * Color Adjustment used to adjust video colors.
 *
 * All fields are in the range 0x0 to 0xFFFF with
 * 0x8000 as the default value (no adjustment).
 */
typedef struct {
     DFBColorAdjustmentFlags  flags;

     __u16                    brightness;
     __u16                    contrast;
     __u16                    hue;
     __u16                    saturation;
} DFBColorAdjustment;

/*
 * <i>No summary yet...</i>
 */
typedef enum {
    DSMT_SURFACEMANAGER_FIRSTFIT,
    DSMT_SURFACEMANAGER_BUDDY,
    DSMT_SURFACEMANAGER_SEQUENTIAL
} DFBSurfaceManagerTypes;

/*
 * <i><b>IDirectFB</b></i> is the main interface. It can be
 * retrieved by a call to <i>DirectFBCreate</i>. It's the only
 * interface with a global creation facility. Other interfaces
 * are created by this interface or interfaces created by it.
 *
 * <b>Hardware capabilities</b> such as the amount of video
 * memory or a list of supported drawing/blitting functions and
 * flags can be retrieved.  It also provides enumeration of all
 * supported video modes.
 *
 * <b>Input devices</b> and <b>display layers</b> that are
 * present can be enumerated via a callback mechanism. The
 * callback is given the capabilities and the device or layer
 * ID. An interface to specific input devices or display layers
 * can be retrieved by passing the device or layer ID to the
 * corresponding method.
 *
 * <b>Surfaces</b> for general purpose use can be created via
 * <i>CreateSurface</i>. These surfaces are so called "offscreen
 * surfaces" and could be used for sprites or icons.
 *
 * The <b>primary surface</b> is an abstraction and API shortcut
 * for getting a surface for visual output. Fullscreen games for
 * example have the whole screen as their primary
 * surface. Alternatively fullscreen applications can be forced
 * to run in a window. The primary surface is also created via
 * <i>CreateSurface</i> but with the special capability
 * DSCAPS_PRIMARY.
 *
 * The <b>cooperative level</b> selects the type of the primary
 * surface.  With a call to <i>SetCooperativeLevel</i> the
 * application can choose between the surface of an implicitly
 * created window and the surface of the primary layer
 * (deactivating the window stack). The application doesn't need
 * to have any extra functionality to run in a window. If the
 * application is forced to run in a window the call to
 * <i>SetCooperativeLevel</i> fails with DFB_ACCESSDENIED.
 * Applications that want to be "window aware" shouldn't exit on
 * this error.
 *
 * The <b>video mode</b> can be changed via <i>SetVideoMode</i>
 * and is the size and depth of the primary surface, i.e. the
 * screen when in exclusive cooperative level. Without exclusive
 * access <i>SetVideoMode</i> sets the size of the implicitly
 * created window.
 *
 * <b>Event buffers</b> can be created with an option to
 * automatically attach input devices matching the specified
 * capabilities. If DICAPS_NONE is passed an event buffer with
 * nothing attached to is created. An event buffer can be
 * attached to input devices and windows.
 *
 * <b>Fonts, images and videos</b> are created by this
 * interface. There are different implementations for different
 * content types. On creation a suitable implementation is
 * automatically chosen.
 */
DEFINE_INTERFACE(   IDirectFB,

   /** Cooperative level, video mode **/

     /*
      * Puts the interface into the specified cooperative level.
      *
      * Function fails with DFB_LOCKED if another instance already
      * is in a cooperative level other than DFSCL_NORMAL.
      */
     DFBResult (*SetCooperativeLevel) (
          IDirectFB                *thiz,
          DFBCooperativeLevel       level
     );

     /*
      * Switch the current video mode (primary layer).
      *
      * If in shared cooperative level this function sets the
      * resolution of the window that is created implicitly for
      * the primary surface.
      */
     DFBResult (*SetVideoMode) (
          IDirectFB                *thiz,
          int                       width,
          int                       height,
          int                       bpp
     );


   /** Hardware capabilities **/

     /*
      * Get a rough description of all drawing/blitting functions
      * along with drawing/blitting flags supported by the hardware.
      *
      * For more detailed information use
      * IDirectFBSurface::GetAccelerationMask().
      */
     DFBResult (*GetCardCapabilities) (
          IDirectFB                *thiz,
          DFBCardCapabilities      *ret_caps
     );

     /*
      * Enumerate supported video modes.
      *
      * Calls the given callback for all available video modes.
      * Useful to select a certain mode to be used with
      * IDirectFB::SetVideoMode().
      */
     DFBResult (*EnumVideoModes) (
          IDirectFB                *thiz,
          DFBVideoModeCallback      callback,
          void                     *callbackdata
     );


   /** Surfaces & Palettes **/

     /*
      * Create a surface matching the specified description.
      */
     DFBResult (*CreateSurface) (
          IDirectFB                     *thiz,
          const DFBSurfaceDescription   *desc,
          IDirectFBSurface             **ret_interface
     );

     /*
      * Create a palette matching the specified description.
      *
      * Passing a NULL description creates a default palette with
      * 256 entries filled with colors matching the RGB332 format.
      */
     DFBResult (*CreatePalette) (
          IDirectFB                    *thiz,
          const DFBPaletteDescription  *desc,
          IDirectFBPalette            **ret_interface
     );


   /** Screens **/

     /*
      * Enumerate all existing screen.
      *
      * Calls the given callback for each available screen.
      * The callback is passed the screen id that can be
      * used to retrieve an interface to a specific screen using
      * IDirectFB::GetScreen().
      */
     DFBResult (*EnumScreens) (
          IDirectFB                *thiz,
          DFBScreenCallback         callback,
          void                     *callbackdata
     );

     /*
      * Retrieve an interface to a specific screen.
      */
     DFBResult (*GetScreen) (
          IDirectFB                *thiz,
          DFBScreenID               screen_id,
          IDirectFBScreen         **ret_interface
     );


   /** Display Layers **/

     /*
      * Enumerate all existing display layers.
      *
      * Calls the given callback for each available display
      * layer. The callback is passed the layer id that can be
      * used to retrieve an interface to a specific layer using
      * IDirectFB::GetDisplayLayer().
      */
     DFBResult (*EnumDisplayLayers) (
          IDirectFB                *thiz,
          DFBDisplayLayerCallback   callback,
          void                     *callbackdata
     );

     /*
      * Retrieve an interface to a specific display layer.
      *
      * The default <i>layer_id</i> is DLID_PRIMARY.
      * Others can be obtained using IDirectFB::EnumDisplayLayers().
      */
     DFBResult (*GetDisplayLayer) (
          IDirectFB                *thiz,
          DFBDisplayLayerID         layer_id,
          IDirectFBDisplayLayer   **ret_interface
     );


   /** Input Devices **/

     /*
      * Enumerate all existing input devices.
      *
      * Calls the given callback for all available input devices.
      * The callback is passed the device id that can be used to
      * retrieve an interface on a specific device using
      * IDirectFB::GetInputDevice().
      */
     DFBResult (*EnumInputDevices) (
          IDirectFB                *thiz,
          DFBInputDeviceCallback    callback,
          void                     *callbackdata
     );

     /*
      * Retrieve an interface to a specific input device.
      */
     DFBResult (*GetInputDevice) (
          IDirectFB                *thiz,
          DFBInputDeviceID          device_id,
          IDirectFBInputDevice    **ret_interface
     );

     /*
      * Create a buffer for events.
      *
      * Creates an empty event buffer without event sources connected to it.
      */
     DFBResult (*CreateEventBuffer) (
          IDirectFB                   *thiz,
          IDirectFBEventBuffer       **ret_buffer
     );

     /*
      * Create a buffer for events with input devices connected.
      *
      * Creates an event buffer and attaches all input devices
      * with matching capabilities. If no input devices match,
      * e.g. by specifying DICAPS_NONE, a buffer will be returned
      * that has no event sources connected to it.
      *
      * If global is DFB_FALSE events will only be delivered if this
      * instance of IDirectFB has a focused primary (either running fullscreen
      * or running in windowed mode with the window being focused).
      *
      * If global is DFB_TRUE no event will be discarded.
      */
     DFBResult (*CreateInputEventBuffer) (
          IDirectFB                   *thiz,
          DFBInputDeviceCapabilities   caps,
          DFBBoolean                   global,
          IDirectFBEventBuffer       **ret_buffer
     );

   /** Media **/

     /*
      * Create an image provider for the specified file.
      */
     DFBResult (*CreateImageProvider) (
          IDirectFB                *thiz,
          const char               *filename,
          IDirectFBImageProvider  **ret_interface
     );

     /*
      * Create a video provider.
      */
     DFBResult (*CreateVideoProvider) (
          IDirectFB                *thiz,
          const char               *filename,
          IDirectFBVideoProvider  **ret_interface
     );

     /*
      * Load a font from the specified file given a description
      * of how to load the glyphs.
      */
     DFBResult (*CreateFont) (
          IDirectFB                     *thiz,
          const char                    *filename,
          const DFBFontDescription      *desc,
          IDirectFBFont                **ret_interface
     );

     /*
      * Create a data buffer.
      *
      * If no description is specified (NULL) a streamed data buffer
      * is created.
      */
     DFBResult (*CreateDataBuffer) (
          IDirectFB                       *thiz,
          const DFBDataBufferDescription  *desc,
          IDirectFBDataBuffer            **ret_interface
     );


   /** Clipboard **/

     /*
      * Set clipboard content.
      *
      * This is an experimental and intermediate API call that is
      * supposed to change soon.
      *
      * If timestamp is non null DirectFB returns the time stamp
      * that it associated with the new data.
      */
     DFBResult (*SetClipboardData) (
          IDirectFB                *thiz,
          const char               *mime_type,
          const void               *data,
          unsigned int              size,
          struct timeval           *ret_timestamp
     );

     /*
      * Get clipboard content.
      *
      * Memory returned in *ret_mimetype and *ret_data has to be freed.
      *
      * This is an experimental and intermediate API call that is
      * supposed to change soon.
      */
     DFBResult (*GetClipboardData) (
          IDirectFB                *thiz,
          char                    **ret_mimetype,
          void                    **ret_data,
          unsigned int             *ret_size
     );

     /*
      * Get time stamp of last SetClipboardData call.
      *
      * This is an experimental and intermediate API call that is
      * supposed to change soon.
      */
     DFBResult (*GetClipboardTimeStamp) (
          IDirectFB                *thiz,
          struct timeval           *ret_timestamp
     );


   /** Misc **/

     /*
      * Suspend DirectFB, no other calls to DirectFB are allowed
      * until Resume has been called.
      */
     DFBResult (*Suspend) (
          IDirectFB                *thiz
     );

     /*
      * Resume DirectFB, only to be called after Suspend.
      */
     DFBResult (*Resume) (
          IDirectFB                *thiz
     );

     /*
      * Wait until graphics card is idle,
      * i.e. finish all drawing/blitting functions.
      */
     DFBResult (*WaitIdle) (
          IDirectFB                *thiz
     );

     /*
      * Wait for next vertical retrace.
      */
     DFBResult (*WaitForSync) (
          IDirectFB                *thiz
     );


   /** Extensions **/

     /*
      * Load an implementation of a specific interface type.
      *
      * This methods loads an interface implementation of the specified
      * <i>type</i> of interface, e.g. "IFusionSound".
      *
      * A specific implementation can be forced with the optional
      * <i>implementation</i> argument.
      *
      * Implementations are passed <i>arg</i> during probing and construction.
      *
      * If an implementation has been successfully probed and the interface
      * has been constructed, the resulting interface pointer is stored in
      * <i>interface</i>.
      */
     DFBResult (*GetInterface) (
          IDirectFB                *thiz,
          const char               *type,
          const char               *implementation,
          void                     *arg,
          void                    **ret_interface
     );

   /** Surface managers **/

     /*
      */
     DFBResult (*CreateSurfaceManager) (
          IDirectFB                *thiz,
          unsigned int              size,
          unsigned int              minimum_page_size,
          DFBSurfaceManagerTypes    type,
          IDirectFBSurfaceManager **ret_interface
     );

   /** Media **/

     /*
      * Load a font from a given memory buffer given a description
      * of how to load the glyphs.
      */
     DFBResult (*CreateMemoryFont) (
          IDirectFB                *thiz,
          const void               *data,
          unsigned int              length,
          const DFBFontDescription *desc,
          IDirectFBFont           **ret_interface
     );
)

/* predefined layer ids */
#define DLID_PRIMARY          0x0000
#define DLID_PRIMARY_0        DLID_PRIMARY
#define DLID_PRIMARY_1        0x0001

/* predefined layer source ids */
#define DLSID_SURFACE         0x0000

/* predefined screen ids */
#define DSCID_PRIMARY         0x0000

/* predefined input device ids */
#define DIDID_KEYBOARD        0x0000    /* primary keyboard       */
#define DIDID_MOUSE           0x0001    /* primary mouse          */
#define DIDID_JOYSTICK        0x0002    /* primary joystick       */
#define DIDID_REMOTE          0x0003    /* primary remote control */
#define DIDID_ANY             0x0010    /* no primary device      */


/*
 * Cooperative level handling the access permissions.
 */
typedef enum {
     DLSCL_SHARED             = 0, /* shared access */
     DLSCL_EXCLUSIVE,              /* exclusive access,
                                      fullscreen/mode switching */
     DLSCL_ADMINISTRATIVE          /* administrative access,
                                      enumerate windows, control them */
} DFBDisplayLayerCooperativeLevel;

/*
 * Background mode defining how to erase/initialize the area
 * for a windowstack repaint
 */
typedef enum {
     DLBM_DONTCARE            = 0, /* do not clear the layer before
                                      repainting the windowstack */
     DLBM_COLOR,                   /* fill with solid color
                                      (SetBackgroundColor) */
     DLBM_IMAGE,                   /* use an image (SetBackgroundImage) */
     DLBM_TILE                     /* use a tiled image (SetBackgroundImage) */
} DFBDisplayLayerBackgroundMode;

/*
 * Layer configuration flags
 */
typedef enum {
     DLCONF_NONE              = 0x00000000,

     DLCONF_WIDTH             = 0x00000001,
     DLCONF_HEIGHT            = 0x00000002,
     DLCONF_PIXELFORMAT       = 0x00000004,
     DLCONF_BUFFERMODE        = 0x00000008,
     DLCONF_OPTIONS           = 0x00000010,
     DLCONF_SOURCE            = 0x00000020,
     DLCONF_SURFACE_CAPS      = 0x00000040,

     DLCONF_ALL               = 0x0000007F
} DFBDisplayLayerConfigFlags;

/*
 * Layer configuration
 */
typedef struct {
     DFBDisplayLayerConfigFlags    flags;         /* Which fields of the configuration are set */

     int                           width;         /* Pixel width */
     int                           height;        /* Pixel height */
     DFBSurfacePixelFormat         pixelformat;   /* Pixel format */
     DFBDisplayLayerBufferMode     buffermode;    /* Buffer mode */
     DFBDisplayLayerOptions        options;       /* Enable capabilities */
     DFBDisplayLayerSourceID       source;        /* Selected layer source */

     DFBSurfaceCapabilities        surface_caps;  /* Choose surface capabilities, available:
                                                     INTERLACED, SEPARATED, PREMULTIPLIED. */
} DFBDisplayLayerConfig;

/*
 * Screen Power Mode.
 */
typedef enum {
     DSPM_ON        = 0,
     DSPM_STANDBY,
     DSPM_SUSPEND,
     DSPM_OFF
} DFBScreenPowerMode;


/*
 * Capabilities of a mixer.
 */
typedef enum {
     DSMCAPS_NONE         = 0x00000000, /* None of these. */

     DSMCAPS_FULL         = 0x00000001, /* Can mix full tree as specified in the description. */
     DSMCAPS_SUB_LEVEL    = 0x00000002, /* Can set a maximum layer level, e.g.
                                           to exclude an OSD from VCR output. */
     DSMCAPS_SUB_LAYERS   = 0x00000004, /* Can select a number of layers individually as
                                           specified in the description. */
     DSMCAPS_BACKGROUND   = 0x00000008  /* Background color is configurable. */
} DFBScreenMixerCapabilities;


#define DFB_SCREEN_MIXER_DESC_NAME_LENGTH    24

/*
 * Description of a mixer.
 */
typedef struct {
     DFBScreenMixerCapabilities  caps;

     DFBDisplayLayerIDs          layers;             /* Visible layers if the
                                                        full tree is selected. */

     int                         sub_num;            /* Number of layers that can
                                                        be selected in sub mode. */
     DFBDisplayLayerIDs          sub_layers;         /* Layers available for sub mode
                                                        with layer selection. */

     char name[DFB_SCREEN_MIXER_DESC_NAME_LENGTH];   /* Mixer name */
} DFBScreenMixerDescription;

/*
 * Flags for mixer configuration.
 */
typedef enum {
     DSMCONF_NONE         = 0x00000000, /* None of these. */

     DSMCONF_TREE         = 0x00000001, /* (Sub) tree is selected. */
     DSMCONF_LEVEL        = 0x00000002, /* Level is specified. */
     DSMCONF_LAYERS       = 0x00000004, /* Layer selection is set. */

     DSMCONF_BACKGROUND   = 0x00000010, /* Background color is set. */

     DSMCONF_ALL          = 0x00000017
} DFBScreenMixerConfigFlags;

/*
 * (Sub) tree selection.
 */
typedef enum {
     DSMT_UNKNOWN         = 0x00000000, /* Unknown mode */

     DSMT_FULL            = 0x00000001, /* Full tree. */
     DSMT_SUB_LEVEL       = 0x00000002, /* Sub tree via maximum level. */
     DSMT_SUB_LAYERS      = 0x00000003  /* Sub tree via layer selection. */
} DFBScreenMixerTree;

/*
 * Configuration of a mixer.
 */
typedef struct {
     DFBScreenMixerConfigFlags   flags;      /* Validates struct members. */

     DFBScreenMixerTree          tree;       /* Selected (sub) tree. */

     int                         level;      /* Max. level of sub level mode. */
     DFBDisplayLayerIDs          layers;     /* Layers for sub layers mode. */

     DFBColor                    background; /* Background color. */
} DFBScreenMixerConfig;


/*
 * Capabilities of an output.
 */
typedef enum {
     DSOCAPS_NONE          = 0x00000000, /* None of these. */

     DSOCAPS_CONNECTORS    = 0x00000001, /* Output connectors are available. */

     DSOCAPS_ENCODER_SEL   = 0x00000010, /* Encoder can be selected. */
     DSOCAPS_SIGNAL_SEL    = 0x00000020, /* Signal(s) can be selected. */
     DSOCAPS_CONNECTOR_SEL = 0x00000040, /* Connector(s) can be selected. */

     DSOCAPS_ALL           = 0x00000071
} DFBScreenOutputCapabilities;

/*
 * Type of output connector.
 */
typedef enum {
     DSOC_UNKNOWN        = 0x00000000, /* Unknown type */

     DSOC_VGA            = 0x00000001, /* VGA connector */
     DSOC_SCART          = 0x00000002, /* SCART connector */
     DSOC_YC             = 0x00000004, /* Y/C connector */
     DSOC_CVBS           = 0x00000008  /* CVBS connector */
} DFBScreenOutputConnectors;

/*
 * Type of output signal.
 */
typedef enum {
     DSOS_NONE           = 0x00000000, /* No signal */

     DSOS_VGA            = 0x00000001, /* VGA signal */
     DSOS_YC             = 0x00000002, /* Y/C signal */
     DSOS_CVBS           = 0x00000004, /* CVBS signal */
     DSOS_RGB            = 0x00000008, /* R/G/B signal */
     DSOS_YCBCR          = 0x00000010  /* Y/Cb/Cr signal */
} DFBScreenOutputSignals;


#define DFB_SCREEN_OUTPUT_DESC_NAME_LENGTH    24

/*
 * Description of a screen output.
 */
typedef struct {
     DFBScreenOutputCapabilities   caps;             /* Screen capabilities. */

     DFBScreenOutputConnectors     all_connectors;   /* Output connectors. */
     DFBScreenOutputSignals        all_signals;      /* Output signals. */

     char name[DFB_SCREEN_OUTPUT_DESC_NAME_LENGTH];  /* Output name */
} DFBScreenOutputDescription;

/*
 * Flags for screen output configuration.
 */
typedef enum {
     DSOCONF_NONE         = 0x00000000, /* None of these. */

     DSOCONF_ENCODER      = 0x00000001, /* Set encoder the signal(s) comes from. */
     DSOCONF_SIGNALS      = 0x00000002, /* Select signal(s) from encoder. */
     DSOCONF_CONNECTORS   = 0x00000004, /* Select output connector(s). */

     DSOCONF_ALL          = 0x00000007
} DFBScreenOutputConfigFlags;

/*
 * Configuration of an output.
 */
typedef struct {
     DFBScreenOutputConfigFlags  flags;          /* Validates struct members. */

     int                         encoder;        /* Chosen encoder. */
     DFBScreenOutputSignals      out_signals;    /* Selected encoder signal(s). */
     DFBScreenOutputConnectors   out_connectors; /* Selected output connector(s). */
} DFBScreenOutputConfig;


/*
 * Capabilities of a display encoder.
 */
typedef enum {
     DSECAPS_NONE         = 0x00000000, /* None of these. */

     DSECAPS_TV_STANDARDS = 0x00000001, /* TV standards can be selected. */
     DSECAPS_TEST_PICTURE = 0x00000002, /* Test picture generation supported. */
     DSECAPS_MIXER_SEL    = 0x00000004, /* Mixer can be selected. */
     DSECAPS_OUT_SIGNALS  = 0x00000008, /* Different output signals are supported. */
     DSECAPS_SCANMODE     = 0x00000010, /* Can switch between interlaced and progressive output. */

     DSECAPS_BRIGHTNESS   = 0x00000100, /* Adjustment of brightness is supported. */
     DSECAPS_CONTRAST     = 0x00000200, /* Adjustment of contrast is supported. */
     DSECAPS_HUE          = 0x00000400, /* Adjustment of hue is supported. */
     DSECAPS_SATURATION   = 0x00000800, /* Adjustment of saturation is supported. */

     DSECAPS_ALL          = 0x00000f1f
} DFBScreenEncoderCapabilities;

/*
 * Type of display encoder.
 */
typedef enum {
     DSET_UNKNOWN         = 0x00000000, /* Unknown type */

     DSET_CRTC            = 0x00000001, /* Encoder is a CRTC. */
     DSET_TV              = 0x00000002  /* TV output encoder. */
} DFBScreenEncoderType;

/*
 * TV standards.
 */
typedef enum {
     DSETV_UNKNOWN        = 0x00000000, /* Unknown standard */

     DSETV_PAL            = 0x00000001, /* PAL */
     DSETV_NTSC           = 0x00000002, /* NTSC */
     DSETV_SECAM          = 0x00000004  /* SECAM */
} DFBScreenEncoderTVStandards;

/*
 * Scan modes.
 */
typedef enum {
     DSESM_UNKNOWN        = 0x00000000, /* Unknown mode */

     DSESM_INTERLACED     = 0x00000001, /* Interlaced scan mode */
     DSESM_PROGRESSIVE    = 0x00000002  /* Progressive scan mode */
} DFBScreenEncoderScanMode;


#define DFB_SCREEN_ENCODER_DESC_NAME_LENGTH    24

/*
 * Description of a display encoder.
 */
typedef struct {
     DFBScreenEncoderCapabilities  caps;               /* Encoder capabilities. */
     DFBScreenEncoderType          type;               /* Type of encoder. */

     DFBScreenEncoderTVStandards   tv_standards;       /* Supported TV standards. */
     DFBScreenOutputSignals        out_signals;        /* Supported output signals. */

     char name[DFB_SCREEN_ENCODER_DESC_NAME_LENGTH];   /* Encoder name */
} DFBScreenEncoderDescription;

/*
 * Flags for display encoder configuration.
 */
typedef enum {
     DSECONF_NONE         = 0x00000000, /* None of these. */

     DSECONF_TV_STANDARD  = 0x00000001, /* Set TV standard. */
     DSECONF_TEST_PICTURE = 0x00000002, /* Set test picture mode. */
     DSECONF_MIXER        = 0x00000004, /* Select mixer. */
     DSECONF_OUT_SIGNALS  = 0x00000008, /* Select generated output signal(s). */
     DSECONF_SCANMODE     = 0x00000010, /* Select interlaced or progressive output. */
     DSECONF_TEST_COLOR   = 0x00000020, /* Set color for DSETP_SINGLE. */
     DSECONF_ADJUSTMENT   = 0x00000040, /* Set color adjustment. */

     DSECONF_ALL          = 0x0000007F
} DFBScreenEncoderConfigFlags;

/*
 * Test picture mode.
 */
typedef enum {
     DSETP_OFF      = 0x00000000,  /* Disable test picture. */

     DSETP_MULTI    = 0x00000001,  /* Show color bars. */
     DSETP_SINGLE   = 0x00000002,  /* Whole screen as defined in configuration. */

     DSETP_WHITE    = 0x00000010,  /* Whole screen (ff, ff, ff). */
     DSETP_YELLOW   = 0x00000020,  /* Whole screen (ff, ff, 00). */
     DSETP_CYAN     = 0x00000030,  /* Whole screen (00, ff, ff). */
     DSETP_GREEN    = 0x00000040,  /* Whole screen (00, ff, 00). */
     DSETP_MAGENTA  = 0x00000050,  /* Whole screen (ff, 00, ff). */
     DSETP_RED      = 0x00000060,  /* Whole screen (ff, 00, 00). */
     DSETP_BLUE     = 0x00000070,  /* Whole screen (00, 00, ff). */
     DSETP_BLACK    = 0x00000080   /* Whole screen (00, 00, 00). */
} DFBScreenEncoderTestPicture;

/*
 * Configuration of a display encoder.
 */
typedef struct {
     DFBScreenEncoderConfigFlags   flags;         /* Validates struct members. */

     DFBScreenEncoderTVStandards   tv_standard;   /* TV standard. */
     DFBScreenEncoderTestPicture   test_picture;  /* Test picture mode. */
     int                           mixer;         /* Selected mixer. */
     DFBScreenOutputSignals        out_signals;   /* Generated output signals. */
     DFBScreenEncoderScanMode      scanmode;      /* Interlaced or progressive output. */

     DFBColor                      test_color;    /* Color for DSETP_SINGLE. */

     DFBColorAdjustment            adjustment;    /* Color adjustment. */
} DFBScreenEncoderConfig;


/*******************
 * IDirectFBScreen *
 *******************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBScreen,

   /** Retrieving information **/

     /*
      * Get the unique screen ID.
      */
     DFBResult (*GetID) (
          IDirectFBScreen                    *thiz,
          DFBScreenID                        *ret_screen_id
     );

     /*
      * Get a description of this screen, i.e. the capabilities.
      */
     DFBResult (*GetDescription) (
          IDirectFBScreen                    *thiz,
          DFBScreenDescription               *ret_desc
     );


   /** Display Layers **/

     /*
      * Enumerate all existing display layers for this screen.
      *
      * Calls the given callback for each available display
      * layer. The callback is passed the layer id that can be
      * used to retrieve an interface to a specific layer using
      * IDirectFB::GetDisplayLayer().
      */
     DFBResult (*EnumDisplayLayers) (
          IDirectFBScreen                    *thiz,
          DFBDisplayLayerCallback             callback,
          void                               *callbackdata
     );


   /** Power management **/

     /*
      * Set screen power mode.
      */
     DFBResult (*SetPowerMode) (
          IDirectFBScreen                    *thiz,
          DFBScreenPowerMode                  mode
     );


   /** Synchronization **/

     /*
      * Wait for next vertical retrace.
      */
     DFBResult (*WaitForSync) (
          IDirectFBScreen                    *thiz
     );


   /** Mixers **/

     /*
      * Get a description of available mixers.
      *
      * All descriptions are written to the array pointed to by
      * <b>ret_descriptions</b>. The number of mixers is returned by
      * IDirectFBScreen::GetDescription().
      */
     DFBResult (*GetMixerDescriptions) (
          IDirectFBScreen                    *thiz,
          DFBScreenMixerDescription          *ret_descriptions
     );

     /*
      * Get current mixer configuration.
      */
     DFBResult (*GetMixerConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 mixer,
          DFBScreenMixerConfig               *ret_config
     );

     /*
      * Test mixer configuration.
      *
      * If configuration fails and 'ret_failed' is not NULL it will
      * indicate which fields of the configuration caused the error.
      */
     DFBResult (*TestMixerConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 mixer,
          const DFBScreenMixerConfig         *config,
          DFBScreenMixerConfigFlags          *ret_failed
     );

     /*
      * Set mixer configuration.
      */
     DFBResult (*SetMixerConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 mixer,
          const DFBScreenMixerConfig         *config
     );


   /** Encoders **/

     /*
      * Get a description of available display encoders.
      *
      * All descriptions are written to the array pointed to by
      * <b>ret_descriptions</b>. The number of encoders is returned by
      * IDirectFBScreen::GetDescription().
      */
     DFBResult (*GetEncoderDescriptions) (
          IDirectFBScreen                    *thiz,
          DFBScreenEncoderDescription        *ret_descriptions
     );

     /*
      * Get current encoder configuration.
      */
     DFBResult (*GetEncoderConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 encoder,
          DFBScreenEncoderConfig             *ret_config
     );

     /*
      * Test encoder configuration.
      *
      * If configuration fails and 'ret_failed' is not NULL it will
      * indicate which fields of the configuration caused the
      * error.
      */
     DFBResult (*TestEncoderConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 encoder,
          const DFBScreenEncoderConfig       *config,
          DFBScreenEncoderConfigFlags        *ret_failed
     );

     /*
      * Set encoder configuration.
      */
     DFBResult (*SetEncoderConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 encoder,
          const DFBScreenEncoderConfig       *config
     );


   /** Outputs **/

     /*
      * Get a description of available outputs.
      *
      * All descriptions are written to the array pointed to by
      * <b>ret_descriptions</b>. The number of outputs is returned by
      * IDirectFBScreen::GetDescription().
      */
     DFBResult (*GetOutputDescriptions) (
          IDirectFBScreen                    *thiz,
          DFBScreenOutputDescription         *ret_descriptions
     );

     /*
      * Get current output configuration.
      */
     DFBResult (*GetOutputConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 output,
          DFBScreenOutputConfig              *ret_config
     );

     /*
      * Test output configuration.
      *
      * If configuration fails and 'ret_failed' is not NULL it will
      * indicate which fields of the configuration caused the error.
      */
     DFBResult (*TestOutputConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 output,
          const DFBScreenOutputConfig        *config,
          DFBScreenOutputConfigFlags         *ret_failed
     );

     /*
      * Set output configuration.
      */
     DFBResult (*SetOutputConfiguration) (
          IDirectFBScreen                    *thiz,
          int                                 output,
          const DFBScreenOutputConfig        *config
     );
)


/*************************
 * IDirectFBDisplayLayer *
 *************************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBDisplayLayer,

   /** Information **/

     /*
      * Get the unique layer ID.
      */
     DFBResult (*GetID) (
          IDirectFBDisplayLayer              *thiz,
          DFBDisplayLayerID                  *ret_layer_id
     );

     /*
      * Get a description of this display layer, i.e. the capabilities.
      */
     DFBResult (*GetDescription) (
          IDirectFBDisplayLayer              *thiz,
          DFBDisplayLayerDescription         *ret_desc
     );

     /*
      * Get a description of available sources.
      *
      * All descriptions are written to the array pointed to by
      * <b>ret_descriptions</b>. The number of sources is returned by
      * IDirectFBDisplayLayer::GetDescription().
      */
     DFBResult (*GetSourceDescriptions) (
          IDirectFBDisplayLayer              *thiz,
          DFBDisplayLayerSourceDescription   *ret_descriptions
     );

     /*
      * For an interlaced display, this returns the currently inactive
      * field: 0 for the top field, and 1 for the bottom field.
      *
      * The inactive field is the one you should draw to next to avoid
      * tearing, the active field is the one currently being displayed.
      *
      * For a progressive output, this should always return 0.  We should
      * also have some other call to indicate whether the display layer
      * is interlaced or progressive, but this is a start.
      */
     DFBResult (*GetCurrentOutputField) (
          IDirectFBDisplayLayer              *thiz,
          int                                *ret_field
     );


   /** Interfaces **/

     /*
      * Get an interface to layer's surface.
      *
      * Only available in exclusive mode.
      */
     DFBResult (*GetSurface) (
          IDirectFBDisplayLayer              *thiz,
          IDirectFBSurface                  **ret_interface
     );

     /*
      * Get an interface to the screen to which the layer belongs.
      */
     DFBResult (*GetScreen) (
          IDirectFBDisplayLayer              *thiz,
          IDirectFBScreen                   **ret_interface
     );


   /** Configuration **/

     /*
      * Set cooperative level to get control over the layer
      * or the windows within this layer.
      */
     DFBResult (*SetCooperativeLevel) (
          IDirectFBDisplayLayer              *thiz,
          DFBDisplayLayerCooperativeLevel     level
     );

     /*
      * Get current layer configuration.
      */
     DFBResult (*GetConfiguration) (
          IDirectFBDisplayLayer              *thiz,
          DFBDisplayLayerConfig              *ret_config
     );

     /*
      * Test layer configuration.
      *
      * If configuration fails and 'failed' is not NULL it will
      * indicate which fields of the configuration caused the
      * error.
      */
     DFBResult (*TestConfiguration) (
          IDirectFBDisplayLayer              *thiz,
          const DFBDisplayLayerConfig        *config,
          DFBDisplayLayerConfigFlags         *ret_failed
     );

     /*
      * Set layer configuration.
      *
      * Only available in exclusive or administrative mode.
      */
     DFBResult (*SetConfiguration) (
          IDirectFBDisplayLayer              *thiz,
          const DFBDisplayLayerConfig        *config
     );


   /** Layout **/

     /*
      * Set location on screen as normalized values.
      *
      * So the whole screen is 0.0, 0.0, -1.0, 1.0.
      */
     DFBResult (*SetScreenLocation) (
          IDirectFBDisplayLayer              *thiz,
          float                               x,
          float                               y,
          float                               width,
          float                               height
     );

     /*
      * Set location on screen in pixels.
      */
     DFBResult (*SetScreenPosition) (
          IDirectFBDisplayLayer              *thiz,
          int                                 x,
          int                                 y
     );

     /*
      * Set location on screen in pixels.
      */
     DFBResult (*SetScreenRectangle) (
          IDirectFBDisplayLayer              *thiz,
          int                                 x,
          int                                 y,
          int                                 width,
          int                                 height
     );


   /** Misc Settings **/

     /*
      * Set global alpha factor for blending with layer(s) below.
      */
     DFBResult (*SetOpacity) (
          IDirectFBDisplayLayer              *thiz,
          __u8                                opacity
     );

     /*
      * Set the source rectangle.
      *
      * Only this part of the layer will be displayed.
      */
     DFBResult (*SetSourceRectangle) (
          IDirectFBDisplayLayer              *thiz,
          int                                 x,
          int                                 y,
          int                                 width,
          int                                 height
     );

     /*
      * For an interlaced display, this sets the field parity.
      * field: 0 for top field first, and 1 for bottom field first.
      */
     DFBResult (*SetFieldParity) (
          IDirectFBDisplayLayer              *thiz,
          int                                 field
     );


   /** Color keyes */

     /*
      * Set the source color key.
      *
      * If a pixel of the layer matches this color the underlying
      * pixel is visible at this point.
      */
     DFBResult (*SetSrcColorKey) (
          IDirectFBDisplayLayer              *thiz,
          __u8                                r,
          __u8                                g,
          __u8                                b
     );

     /*
      * Set the destination color key.
      *
      * The layer is only visible at points where the underlying
      * pixel matches this color.
      */
     DFBResult (*SetDstColorKey) (
          IDirectFBDisplayLayer              *thiz,
          __u8                                r,
          __u8                                g,
          __u8                                b
     );


   /** Z Order **/

     /*
      * Get the current display layer level.
      *
      * The level describes the z axis position of a layer. The
      * primary layer is always on level zero unless a special
      * driver adds support for level adjustment on the primary
      * layer.  Layers above have a positive level, e.g. video
      * overlays.  Layers below have a negative level, e.g. video
      * underlays or background layers.
      */
     DFBResult (*GetLevel) (
          IDirectFBDisplayLayer              *thiz,
          int                                *ret_level
     );

     /*
      * Set the display layer level.
      *
      * Moves the layer to the specified level. The order of all
      * other layers won't be changed. Note that only a few
      * layers support level adjustment which is reflected by
      * their capabilities.
      */
     DFBResult (*SetLevel) (
          IDirectFBDisplayLayer              *thiz,
          int                                 level
     );


   /** Background handling **/

     /*
      * Set the erase behaviour for windowstack repaints.
      *
      * Only available in exclusive or administrative mode.
      */
     DFBResult (*SetBackgroundMode) (
          IDirectFBDisplayLayer              *thiz,
          DFBDisplayLayerBackgroundMode       mode
     );

     /*
      * Set the background image for the imaged background mode.
      *
      * Only available in exclusive or administrative mode.
      */
     DFBResult (*SetBackgroundImage) (
          IDirectFBDisplayLayer              *thiz,
          IDirectFBSurface                   *surface
     );

     /*
      * Set the color for a solid colored background.
      *
      * Only available in exclusive or administrative mode.
      */
     DFBResult (*SetBackgroundColor) (
          IDirectFBDisplayLayer              *thiz,
          __u8                                r,
          __u8                                g,
          __u8                                b,
          __u8                                a
     );

   /** Color adjustment **/

     /*
      * Get the layers color adjustment.
      */
     DFBResult (*GetColorAdjustment) (
          IDirectFBDisplayLayer              *thiz,
          DFBColorAdjustment                 *ret_adj
     );

     /*
      * Set the layers color adjustment.
      *
      * Only available in exclusive or administrative mode.
      *
      * This function only has an effect if the underlying
      * hardware supports this operation. Check the layers
      * capabilities to find out if this is the case.
      */
     DFBResult (*SetColorAdjustment) (
          IDirectFBDisplayLayer              *thiz,
          const DFBColorAdjustment           *adj
     );


   /** Windows **/

     /*
      * Create a window within this layer given a
      * description of the window that is to be created.
      */
     DFBResult (*CreateWindow) (
          IDirectFBDisplayLayer              *thiz,
          const DFBWindowDescription         *desc,
          IDirectFBWindow                   **ret_interface
     );

     /*
      * Retrieve an interface to an existing window.
      *
      * The window is identified by its window id.
      */
     DFBResult (*GetWindow) (
          IDirectFBDisplayLayer              *thiz,
          DFBWindowID                         window_id,
          IDirectFBWindow                   **ret_interface
     );


   /** Cursor handling **/

     /*
      * Enable/disable the mouse cursor for this layer.
      *
      * Windows on a layer will only receive motion events if
      * the cursor is enabled. This function is only available
      * in exclusive/administrative mode.
      */
     DFBResult (*EnableCursor) (
          IDirectFBDisplayLayer              *thiz,
          int                                 enable
     );

     /*
      * Returns the x/y coordinates of the layer's mouse cursor.
      */
     DFBResult (*GetCursorPosition) (
          IDirectFBDisplayLayer              *thiz,
          int                                *ret_x,
          int                                *ret_y
     );

     /*
      * Move cursor to specified position.
      *
      * Handles movement like a real one, i.e. generates events.
      */
     DFBResult (*WarpCursor) (
          IDirectFBDisplayLayer              *thiz,
          int                                 x,
          int                                 y
     );

     /*
      * Set cursor acceleration.
      *
      * Sets the acceleration of cursor movements. The amount
      * beyond the 'threshold' will be multiplied with the
      * acceleration factor. The acceleration factor is
      * 'numerator/denominator'.
      */
     DFBResult (*SetCursorAcceleration) (
          IDirectFBDisplayLayer              *thiz,
          int                                 numerator,
          int                                 denominator,
          int                                 threshold
     );

     /*
      * Set the cursor shape and the hotspot.
      */
     DFBResult (*SetCursorShape) (
          IDirectFBDisplayLayer              *thiz,
          IDirectFBSurface                   *shape,
          int                                 hot_x,
          int                                 hot_y
     );

     /*
      * Set the cursor opacity.
      *
      * This function is especially useful if you want
      * to hide the cursor but still want windows on this
      * display layer to receive motion events. In this
      * case, simply set the cursor opacity to zero.
      */
     DFBResult (*SetCursorOpacity) (
          IDirectFBDisplayLayer              *thiz,
          __u8                                opacity
     );


   /** Synchronization **/

     /*
      * Wait for next vertical retrace.
      */
     DFBResult (*WaitForSync) (
          IDirectFBDisplayLayer              *thiz
     );
)


/*
 * Flipping flags controlling the behaviour of IDirectFBSurface::Flip().
 */
typedef enum {
     DSFLIP_NONE         = 0x00000000,  /* None of these. */

     DSFLIP_WAIT         = 0x00000001,  /* Flip() returns upon vertical sync. Flipping is still done
                                           immediately unless DSFLIP_ONSYNC is specified, too.  */
     DSFLIP_BLIT         = 0x00000002,  /* Copy from back buffer to front buffer rather than
                                           just swapping these buffers. This behaviour is enforced
                                           if the region passed to Flip() is not NULL or if the
                                           surface being flipped is a sub surface. */
     DSFLIP_ONSYNC       = 0x00000004,  /* Do the actual flipping upon the next vertical sync.
                                           The Flip() method will still return immediately unless
                                           DSFLIP_WAIT is specified, too. */

     DSFLIP_PIPELINE     = 0x00000008,

     DSFLIP_WAITFORSYNC  = DSFLIP_WAIT | DSFLIP_ONSYNC
} DFBSurfaceFlipFlags;

/*
 * Flags controlling the text layout.
 */
typedef enum {

     DSTF_DEFAULT        = 0x00000000,  /* Default text output*/
     DSTF_CENTER         = 0x00000001,  /* Horizontally centered around the x coordinate for
                                           horizontal text, or vertically centered around the y
                                           coordinate for vertical text*/

     DSTF_LEFT           = 0x00000002,  /* The left side of the text is aligned to the given point */
     DSTF_RIGHT          = 0x00000004,  /* The right side of the text is aligned to the given point */

     DSTF_TOP            = 0x00000008,  /* The given point is the top most point for the font display */
     DSTF_BOTTOM         = 0x00000010,  /* The given point is the bottom most point for the font display */

     DSTF_TOPLEFT        = DSTF_TOP | DSTF_LEFT,
     DSTF_TOPCENTER      = DSTF_TOP | DSTF_CENTER,
     DSTF_TOPRIGHT       = DSTF_TOP | DSTF_RIGHT,

     DSTF_BOTTOMLEFT     = DSTF_BOTTOM | DSTF_LEFT,
     DSTF_BOTTOMCENTER   = DSTF_BOTTOM | DSTF_CENTER,
     DSTF_BOTTOMRIGHT    = DSTF_BOTTOM | DSTF_RIGHT,

     /*
      * these flag specify that glyphs should be compressed to occupy only
      * half of its normal height or width on the surface it's blit on.
      */
     DSTF_HALFHEIGHT     = 0x80000000,
     DSTF_HALFWIDTH      = 0x40000000

} DFBSurfaceTextFlags;

/*
 * Flags defining the type of data access.
 * These are important for surface swapping management.
 */
typedef enum {
     DSLF_READ           = 0x00000001,  /* request read access while
                                           surface is locked */
     DSLF_WRITE          = 0x00000002   /* request write access */
} DFBSurfaceLockFlags;

/*
 * Available Porter/Duff rules.
 */
typedef enum {
                              /* pixel = (source * fs + destination * fd),
                                 sa = source alpha,
                                 da = destination alpha */
     DSPD_NONE           = 0, /* fs: sa      fd: 1.0-sa (defaults) */
     DSPD_CLEAR          = 1, /* fs: 0.0     fd: 0.0    */
     DSPD_SRC            = 2, /* fs: 1.0     fd: 0.0    */
     DSPD_SRC_OVER       = 3, /* fs: 1.0     fd: 1.0-sa */
     DSPD_DST_OVER       = 4, /* fs: 1.0-da  fd: 1.0    */
     DSPD_SRC_IN         = 5, /* fs: da      fd: 0.0    */
     DSPD_DST_IN         = 6, /* fs: 0.0     fd: sa     */
     DSPD_SRC_OUT        = 7, /* fs: 1.0-da  fd: 0.0    */
     DSPD_DST_OUT        = 8, /* fs: 0.0     fd: 1.0-sa */
     DSPD_XOR            = 12 /* fs: 1.0-da  fd: 1.0-sa */
} DFBSurfacePorterDuffRule;

/*
 * Blend functions to use for source and destination blending
 */
typedef enum {
     DSBF_ZERO               = 1,  /* */
     DSBF_ONE                = 2,  /* */
     DSBF_SRCCOLOR           = 3,  /* */
     DSBF_INVSRCCOLOR        = 4,  /* */
     DSBF_SRCALPHA           = 5,  /* */
     DSBF_INVSRCALPHA        = 6,  /* */
     DSBF_DESTALPHA          = 7,  /* */
     DSBF_INVDESTALPHA       = 8,  /* */
     DSBF_DESTCOLOR          = 9,  /* */
     DSBF_INVDESTCOLOR       = 10, /* */
     DSBF_SRCALPHASAT        = 11  /* */
} DFBSurfaceBlendFunction;

/*
 * Transformed vertex of a textured triangle.
 */
typedef struct {
     float x;   /* Destination X coordinate (in pixels) */
     float y;   /* Destination Y coordinate (in pixels) */
     float z;   /* Z coordinate */
     float w;   /* W coordinate */

     float s;   /* Texture S coordinate */
     float t;   /* Texture T coordinate */
} DFBVertex;

/*
 * Way of building triangles from the list of vertices.
 */
typedef enum {
     DTTF_LIST,  /* 0/1/2  3/4/5  6/7/8 ... */
     DTTF_STRIP, /* 0/1/2  1/2/3  2/3/4 ... */
     DTTF_FAN    /* 0/1/2  0/2/3  0/3/4 ... */
} DFBTriangleFormation;

/********************
 * IDirectFBSurface *
 ********************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBSurface,

   /** Retrieving information **/

     /*
      * Return the capabilities of this surface.
      */
     DFBResult (*GetCapabilities) (
          IDirectFBSurface         *thiz,
          DFBSurfaceCapabilities   *ret_caps
     );

     /*
      * Get the surface's width and height in pixels.
      */
     DFBResult (*GetSize) (
          IDirectFBSurface         *thiz,
          int                      *ret_width,
          int                      *ret_height
     );

     /*
      * Created sub surfaces might be clipped by their parents,
      * this function returns the resulting rectangle relative
      * to this surface.
      *
      * For non sub surfaces this function returns
      * { 0, 0, width, height }.
      */
     DFBResult (*GetVisibleRectangle) (
          IDirectFBSurface         *thiz,
          DFBRectangle             *ret_rect
     );

     /*
      * Get the current pixel format.
      */
     DFBResult (*GetPixelFormat) (
          IDirectFBSurface         *thiz,
          DFBSurfacePixelFormat    *ret_format
     );

     /*
      * Get a mask of drawing functions that are hardware
      * accelerated with the current settings.
      *
      * If a source surface is specified the mask will also
      * contain accelerated blitting functions.  Note that there
      * is no guarantee that these will actually be accelerated
      * since the surface storage (video/system) is examined only
      * when something actually gets drawn or blitted.
      */
     DFBResult (*GetAccelerationMask) (
          IDirectFBSurface         *thiz,
          IDirectFBSurface         *source,
          DFBAccelerationMask      *ret_mask
     );


   /** Palette & Alpha Ramp **/

     /*
      * Get access to the surface's palette.
      *
      * Returns an interface that can be used to gain
      * read and/or write access to the surface's palette.
      */
     DFBResult (*GetPalette) (
          IDirectFBSurface         *thiz,
          IDirectFBPalette        **ret_interface
     );

     /*
      * Change the surface's palette.
      */
     DFBResult (*SetPalette) (
          IDirectFBSurface         *thiz,
          IDirectFBPalette         *palette
     );

     /*
      * Set the alpha ramp for formats with one or two alpha bits.
      *
      * Either all four values or the first and the
      * last one are used, depending on the format.
      * Default values are: 0x00, 0x55, 0xaa, 0xff.
      */
     DFBResult (*SetAlphaRamp) (
          IDirectFBSurface         *thiz,
          __u8                      a0,
          __u8                      a1,
          __u8                      a2,
          __u8                      a3
     );


   /** Buffer operations **/

     /*
      * Lock the surface for the access type specified.
      *
      * Returns a data pointer and the line pitch of it.
      */
     DFBResult (*Lock) (
          IDirectFBSurface         *thiz,
          DFBSurfaceLockFlags       flags,
          void                    **ret_ptr,
          int                      *ret_pitch
     );

     /*
      * Unlock the surface after direct access.
      */
     DFBResult (*Unlock) (
          IDirectFBSurface         *thiz
     );

     /*
      * Flip the two buffers of the surface.
      *
      * If no region is specified the whole surface is flipped,
      * otherwise blitting is used to update the region.
      * This function fails if the surfaces capabilities don't
      * include DSCAPS_FLIPPING.
      */
     DFBResult (*Flip) (
          IDirectFBSurface         *thiz,
          const DFBRegion          *region,
          DFBSurfaceFlipFlags       flags
     );

     /*
      * Set the active field.
      *
      * Interlaced surfaces consist of two fields. Software driven
      * deinterlacing uses this method to manually switch the field
      * that is displayed, e.g. scaled up vertically by two.
      */
     DFBResult (*SetField) (
          IDirectFBSurface         *thiz,
          int                       field
     );

     /*
      * Clear the surface and its depth buffer if existent.
      *
      * Fills the whole (sub) surface with the specified color while ignoring
      * drawing flags and color of the current state, but limited to the current clip.
      *
      * As with all drawing and blitting functions the backbuffer is written to.
      * If you are initializing a double buffered surface you may want to clear both
      * buffers by doing a Clear-Flip-Clear sequence.
      */
     DFBResult (*Clear) (
          IDirectFBSurface         *thiz,
          __u8                      r,
          __u8                      g,
          __u8                      b,
          __u8                      a
     );


   /** Drawing/blitting control **/

     /*
      * Set the clipping region used to limit the area for
      * drawing, blitting and text functions.
      *
      * If no region is specified (NULL passed) the clip is set
      * to the surface extents (initial clip).
      */
     DFBResult (*SetClip) (
          IDirectFBSurface         *thiz,
          const DFBRegion          *clip
     );

     /*
      * Set the color used for drawing/text functions or
      * alpha/color modulation (blitting functions).
      *
      * If you are not using the alpha value it should be set to
      * 0xff to ensure visibility when the code is ported to or
      * used for surfaces with an alpha channel.
      *
      * This method should be avoided for surfaces with an indexed
      * pixelformat, e.g. DSPF_LUT8, otherwise an expensive search
      * in the color/alpha lookup table occurs.
      */
     DFBResult (*SetColor) (
          IDirectFBSurface         *thiz,
          __u8                      r,
          __u8                      g,
          __u8                      b,
          __u8                      a
     );

     /*
      * Set the color like with SetColor() but using
      * an index to the color/alpha lookup table.
      *
      * This method is only supported by surfaces with an
      * indexed pixelformat, e.g. DSPF_LUT8. For these formats
      * this method should be used instead of SetColor().
      */
     DFBResult (*SetColorIndex) (
          IDirectFBSurface         *thiz,
          unsigned int              index
     );

     /*
      * Set the blend function that applies to the source.
      */
     DFBResult (*SetSrcBlendFunction) (
          IDirectFBSurface         *thiz,
          DFBSurfaceBlendFunction   function
     );

     /*
      * Set the blend function that applies to the destination.
      */
     DFBResult (*SetDstBlendFunction) (
          IDirectFBSurface         *thiz,
          DFBSurfaceBlendFunction   function
     );

     /*
      * Set the source and destination blend function by
      * specifying a Porter/Duff rule.
      */
     DFBResult (*SetPorterDuff) (
          IDirectFBSurface         *thiz,
          DFBSurfacePorterDuffRule  rule
     );

     /*
      * Set the source color key, i.e. the color that is excluded
      * when blitting FROM this surface TO another that has
      * source color keying enabled.
      */
     DFBResult (*SetSrcColorKey) (
          IDirectFBSurface         *thiz,
          __u8                      r,
          __u8                      g,
          __u8                      b
     );

     /*
      * Set the source color key like with SetSrcColorKey() but using
      * an index to the color/alpha lookup table.
      *
      * This method is only supported by surfaces with an
      * indexed pixelformat, e.g. DSPF_LUT8. For these formats
      * this method should be used instead of SetSrcColorKey().
      */
     DFBResult (*SetSrcColorKeyIndex) (
          IDirectFBSurface         *thiz,
          unsigned int              index
     );

     /*
      * Set the destination color key, i.e. the only color that
      * gets overwritten by drawing and blitting to this surface
      * when destination color keying is enabled.
      */
     DFBResult (*SetDstColorKey) (
          IDirectFBSurface         *thiz,
          __u8                      r,
          __u8                      g,
          __u8                      b
     );

     /*
      * Set the destination color key like with SetDstColorKey() but using
      * an index to the color/alpha lookup table.
      *
      * This method is only supported by surfaces with an
      * indexed pixelformat, e.g. DSPF_LUT8. For these formats
      * this method should be used instead of SetDstColorKey().
      */
     DFBResult (*SetDstColorKeyIndex) (
          IDirectFBSurface         *thiz,
          unsigned int              index
     );


   /** Blitting functions **/

     /*
      * Set the flags for all subsequent blitting commands.
      */
     DFBResult (*SetBlittingFlags) (
          IDirectFBSurface         *thiz,
          DFBSurfaceBlittingFlags   flags
     );

     /*
      * Blit an area from the source to this surface.
      *
      * Pass a NULL rectangle to use the whole source surface.
      * Source may be the same surface.
      */
     DFBResult (*Blit) (
          IDirectFBSurface         *thiz,
          IDirectFBSurface         *source,
          const DFBRectangle       *source_rect,
          int                       x,
          int                       y
     );

     /*
      * Blit an area from the source tiled to this surface.
      *
      * Pass a NULL rectangle to use the whole source surface.
      * Source may be the same surface.
      */
     DFBResult (*TileBlit) (
          IDirectFBSurface         *thiz,
          IDirectFBSurface         *source,
          const DFBRectangle       *source_rect,
          int                       x,
          int                       y
     );

     /*
      * Blit a bunch of areas at once.
      *
      * Source may be the same surface.
      */
     DFBResult (*BatchBlit) (
          IDirectFBSurface         *thiz,
          IDirectFBSurface         *source,
          const DFBRectangle       *source_rects,
          const DFBPoint           *dest_points,
          int                       num
     );

     /*
      * Blit an area scaled from the source to the destination
      * rectangle.
      *
      * Pass a NULL rectangle to use the whole source surface.
      */
     DFBResult (*StretchBlit) (
          IDirectFBSurface         *thiz,
          IDirectFBSurface         *source,
          const DFBRectangle       *source_rect,
          const DFBRectangle       *destination_rect
     );

     /*
      * Preliminary texture mapping support.
      *
      * Maps a <b>texture</b> onto triangles being built
      * from <b>vertices</b> according to the chosen <b>formation</b>.
      *
      * Optional <b>indices</b> can be used to avoid rearrangement of vertex lists,
      * otherwise the vertex list is processed consecutively, i.e. as if <b>indices</b>
      * are ascending numbers starting at zero.
      *
      * Either the number of <b>indices</b> (if non NULL) or the number of <b>vertices</b> is
      * specified by <b>num</b> and has to be three at least. If the chosen <b>formation</b>
      * is DTTF_LIST it also has to be a multiple of three.
      */
     DFBResult (*TextureTriangles) (
          IDirectFBSurface         *thiz,
          IDirectFBSurface         *texture,
          const DFBVertex          *vertices,
          const int                *indices,
          int                       num,
          DFBTriangleFormation      formation
     );


   /** Drawing functions **/

     /*
      * Set the flags for all subsequent drawing commands.
      */
     DFBResult (*SetDrawingFlags) (
          IDirectFBSurface         *thiz,
          DFBSurfaceDrawingFlags    flags
     );

     /*
      * Fill the specified rectangle with the given color
      * following the specified flags.
      */
     DFBResult (*FillRectangle) (
          IDirectFBSurface         *thiz,
          int                       x,
          int                       y,
          int                       w,
          int                       h
     );

     /*
      * Draw an outline of the specified rectangle with the given
      * color following the specified flags.
      */
     DFBResult (*DrawRectangle) (
          IDirectFBSurface         *thiz,
          int                       x,
          int                       y,
          int                       w,
          int                       h
     );

     /*
      * Draw a line from one point to the other with the given color
      * following the drawing flags.
      */
     DFBResult (*DrawLine) (
          IDirectFBSurface         *thiz,
          int                       x1,
          int                       y1,
          int                       x2,
          int                       y2
     );

     /*
      * Draw 'num_lines' lines with the given color following the
      * drawing flags. Each line specified by a DFBRegion.
      */
     DFBResult (*DrawLines) (
          IDirectFBSurface         *thiz,
          const DFBRegion          *lines,
          unsigned int              num_lines
     );

     /*
      * Fill a non-textured triangle.
      */
     DFBResult (*FillTriangle) (
          IDirectFBSurface         *thiz,
          int                       x1,
          int                       y1,
          int                       x2,
          int                       y2,
          int                       x3,
          int                       y3
     );

     /*
      * Fill a bunch of rectangles with a single call.
      *
      * Fill <b>num</b> rectangles with the current color following the
      * drawing flags. Each rectangle specified by a DFBRectangle.
      */
     DFBResult (*FillRectangles) (
          IDirectFBSurface         *thiz,
          const DFBRectangle       *rects,
          unsigned int              num
     );

     /*
      * Fill spans specified by x and width.
      *
      * Fill <b>num</b> spans with the given color following the
      * drawing flags. Each span is specified by a DFBSpan.
      */
     DFBResult (*FillSpans) (
          IDirectFBSurface         *thiz,
          int                       y,
          const DFBSpan            *spans,
          unsigned int              num
     );


   /** Text functions **/

     /*
      * Set the font used by DrawString() and DrawGlyph().
      * You can pass NULL here to unset the font.
      */
     DFBResult (*SetFont) (
          IDirectFBSurface         *thiz,
          IDirectFBFont            *font
     );

     /*
      * Get the font associated with a surface.
      *
      * This function increases the font's reference count.
      */
     DFBResult (*GetFont) (
          IDirectFBSurface         *thiz,
          IDirectFBFont           **ret_font
     );

     /*
      * Draw an UTF-8 string at the specified position with the
      * given color following the specified flags.
      *
      * If font was loaded with the DFFA_CHARMAP flag, the string
      * specifies UTF-8 encoded raw glyph indices.
      *
      * Bytes specifies the number of bytes to take from the
      * string or -1 for the complete NULL-terminated string. You
      * need to set a font using the SetFont() method before
      * calling this function.
      */
     DFBResult (*DrawString) (
          IDirectFBSurface         *thiz,
          const char               *text,
          int                       bytes,
          int                       x,
          int                       y,
          DFBSurfaceTextFlags       flags
     );

     /*
      * Draw a single glyph specified by its Unicode index at the
      * specified position with the given color following the
      * specified flags.
      *
      * If font was loaded with the DFFA_NOCHARMAP flag, index specifies
      * the raw glyph index in the font.
      *
      * You need to set a font using the SetFont() method before
      * calling this function.
      */
     DFBResult (*DrawGlyph) (
          IDirectFBSurface         *thiz,
          unsigned int              index,
          int                       x,
          int                       y,
          DFBSurfaceTextFlags       flags
     );


   /** Lightweight helpers **/

     /*
      * Get an interface to a sub area of this surface.
      *
      * No image data is duplicated, this is a clipped graphics
      * within the original surface. This is very helpful for
      * lightweight components in a GUI toolkit.  The new
      * surface's state (color, drawingflags, etc.) is
      * independent from this one. So it's a handy graphics
      * context.  If no rectangle is specified, the whole surface
      * (or a part if this surface is a subsurface itself) is
      * represented by the new one.
      */
     DFBResult (*GetSubSurface) (
          IDirectFBSurface         *thiz,
          const DFBRectangle       *rect,
          IDirectFBSurface        **ret_interface
     );


   /** OpenGL **/

     /*
      * Get an OpenGL context for this surface.
      */
     DFBResult (*GetGL) (
          IDirectFBSurface         *thiz,
          IDirectFBGL             **ret_interface
     );


   /** Debug **/

     /*
      * Dump the contents of the surface to one or two files.
      *
      * Creates a PPM file containing the RGB data and a PGM file with
      * the alpha data if present.
      *
      * The complete filenames will be
      * <b>directory</b>/<b>prefix</b>_<i>####</i>.ppm for RGB and
      * <b>directory</b>/<b>prefix</b>_<i>####</i>.pgm for the alpha channel
      * if present. Example: "/directory/prefix_0000.ppm". No existing files
      * will be overwritten.
      */
     DFBResult (*Dump) (
          IDirectFBSurface         *thiz,
          const char               *directory,
          const char               *prefix
     );

     DFBResult (*HardwareLock) (
          IDirectFBSurface         *thiz
     );


     DFBResult (*GetBackBufferContext) (
          IDirectFBSurface         *thiz,
          void      **ctx
     );

     DFBResult (*ResetSource) (
          IDirectFBSurface         *thiz
     );


     /*
      * Create a CIF encoded image based on the contents of the
      * given surface.  The ouput CIF image will be written to
      * the buffer provide by "out_buffer".  The size of
      * "out_buffer" must also be provided by "out_buffer_size".
      * The size of the CIF image, and the number of bytes used
      * in "out_buffer" will be returned to the caller with
      * "stream_length"
      *
      * "scale_numerator" and "scale_denominator" must be passed
      * by the application.  Setting either value to a number other
      * than one, will cause the output CIF image to be scaled down
      * at the rate "scale_numerator"/"scale_denominator".Both
      * values must be greater than zero, and less than 16.
      * "scale_numerator" must also be less than "scale_denominator".

      * A source surface of type DSPF_UYVY will be output in CVF
      * format and surface of type DSPF_ARGB will be output in CDF
      * format.  All other YUV pixel formats will be converted to
      * DSPF_UYVY and then output as a CVF image.  Any non YUV pixel
      * formats will be converted to DSPF_ARGB and output as CDF.
      */

     DFBResult (*CreateCIF) (
          IDirectFBSurface* thiz,
          unsigned int  scale_numerator,
          unsigned int  scale_denominator,
          unsigned int *out_buffer,
          unsigned int  out_buffer_size,
          unsigned int *stream_length
     );


     /*
      * Create a PPM encoded image based on the contents of the
      * given surface.  The ouput PPM image will be written to
      * the buffer provide by "out_buffer".  The size of
      * "out_buffer" must also be provided by "out_buffer_size".
      * The size of the PPM image, and the number of bytes used
      * in "out_buffer" will be returned to the caller with
      * "stream_length"
      *
      * "scale_numerator" and "scale_denominator" must be passed
      * by the application.  Setting either value to a number other
      * than one, will cause the output PPM image to be scaled down
      * at the rate "scale_numerator"/"scale_denominator".Both
      * values must be greater than zero, and less than 16.
      * "scale_numerator" must also be less than "scale_denominator".
      */

     DFBResult (*CreatePPM) (
          IDirectFBSurface* thiz,
          unsigned int  scale_numerator,
          unsigned int  scale_denominator,
          unsigned int *out_buffer,
          unsigned int  out_buffer_size,
          unsigned int *stream_length
     );
)


/********************
 * IDirectFBPalette *
 ********************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBPalette,

   /** Retrieving information **/

     /*
      * Return the capabilities of this palette.
      */
     DFBResult (*GetCapabilities) (
          IDirectFBPalette         *thiz,
          DFBPaletteCapabilities   *ret_caps
     );

     /*
      * Get the number of entries in the palette.
      */
     DFBResult (*GetSize) (
          IDirectFBPalette         *thiz,
          unsigned int             *ret_size
     );


   /** Palette entries **/

     /*
      * Write entries to the palette.
      *
      * Writes the specified number of entries to the palette at the
      * specified offset.
      */
     DFBResult (*SetEntries) (
          IDirectFBPalette         *thiz,
          const DFBColor           *entries,
          unsigned int              num_entries,
          unsigned int              offset
     );

     /*
      * Read entries from the palette.
      *
      * Reads the specified number of entries from the palette at the
      * specified offset.
      */
     DFBResult (*GetEntries) (
          IDirectFBPalette         *thiz,
          DFBColor                 *ret_entries,
          unsigned int              num_entries,
          unsigned int              offset
     );

     /*
      * Find the best matching entry.
      *
      * Searches the map for an entry which best matches the specified color.
      */
     DFBResult (*FindBestMatch) (
          IDirectFBPalette         *thiz,
          __u8                      r,
          __u8                      g,
          __u8                      b,
          __u8                      a,
          unsigned int             *ret_index
     );


   /** Clone **/

     /*
      * Create a copy of the palette.
      */
     DFBResult (*CreateCopy) (
          IDirectFBPalette         *thiz,
          IDirectFBPalette        **ret_interface
     );
)


/*
 * Specifies whether a key is currently down.
 */
typedef enum {
     DIKS_UP             = 0x00000000,  /* key is not pressed */
     DIKS_DOWN           = 0x00000001   /* key is pressed */
} DFBInputDeviceKeyState;

/*
 * Specifies whether a button is currently pressed.
 */
typedef enum {
     DIBS_UP             = 0x00000000,  /* button is not pressed */
     DIBS_DOWN           = 0x00000001   /* button is pressed */
} DFBInputDeviceButtonState;

/*
 * Flags specifying which buttons are currently down.
 */
typedef enum {
     DIBM_LEFT           = 0x00000001,  /* left mouse button */
     DIBM_RIGHT          = 0x00000002,  /* right mouse button */
     DIBM_MIDDLE         = 0x00000004   /* middle mouse button */
} DFBInputDeviceButtonMask;

/*
 * Flags specifying which modifiers are currently pressed.
 */
typedef enum {
     DIMM_SHIFT     = (1 << DIMKI_SHIFT),    /* Shift key is pressed */
     DIMM_CONTROL   = (1 << DIMKI_CONTROL),  /* Control key is pressed */
     DIMM_ALT       = (1 << DIMKI_ALT),      /* Alt key is pressed */
     DIMM_ALTGR     = (1 << DIMKI_ALTGR),    /* AltGr key is pressed */
     DIMM_META      = (1 << DIMKI_META),     /* Meta key is pressed */
     DIMM_SUPER     = (1 << DIMKI_SUPER),    /* Super key is pressed */
     DIMM_HYPER     = (1 << DIMKI_HYPER)     /* Hyper key is pressed */
} DFBInputDeviceModifierMask;


/************************
 * IDirectFBInputDevice *
 ************************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBInputDevice,

   /** Retrieving information **/

     /*
      * Get the unique device ID.
      */
     DFBResult (*GetID) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceID              *ret_device_id
     );

     /*
      * Get a description of this device, i.e. the capabilities.
      */
     DFBResult (*GetDescription) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceDescription     *ret_desc
     );


   /** Key mapping **/

     /*
      * Fetch one entry from the keymap for a specific hardware keycode.
      */
     DFBResult (*GetKeymapEntry) (
          IDirectFBInputDevice          *thiz,
          int                            keycode,
          DFBInputDeviceKeymapEntry     *ret_entry
     );


   /** Event buffers **/

     /*
      * Create an event buffer for this device and attach it.
      */
     DFBResult (*CreateEventBuffer) (
          IDirectFBInputDevice          *thiz,
          IDirectFBEventBuffer         **ret_buffer
     );

     /*
      * Attach an existing event buffer to this device.
      *
      * NOTE: Attaching multiple times generates multiple events.
      *
      */
     DFBResult (*AttachEventBuffer) (
          IDirectFBInputDevice          *thiz,
          IDirectFBEventBuffer          *buffer
     );


   /** General state queries **/

     /*
      * Get the current state of one key.
      */
     DFBResult (*GetKeyState) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceKeyIdentifier    key_id,
          DFBInputDeviceKeyState        *ret_state
     );

     /*
      * Get the current modifier mask.
      */
     DFBResult (*GetModifiers) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceModifierMask    *ret_modifiers
     );

     /*
      * Get the current state of the key locks.
      */
     DFBResult (*GetLockState) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceLockState       *ret_locks
     );

     /*
      * Get a mask of currently pressed buttons.
      *
      * The first button corrensponds to the right most bit.
      */
     DFBResult (*GetButtons) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceButtonMask      *ret_buttons
     );

     /*
      * Get the state of a button.
      */
     DFBResult (*GetButtonState) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceButtonIdentifier button,
          DFBInputDeviceButtonState     *ret_state
     );

     /*
      * Get the current value of the specified axis.
      */
     DFBResult (*GetAxis) (
          IDirectFBInputDevice          *thiz,
          DFBInputDeviceAxisIdentifier   axis,
          int                           *ret_pos
     );


   /** Specialized queries **/

     /*
      * Utility function combining two calls to GetAxis().
      *
      * You may leave one of the x/y arguments NULL.
      */
     DFBResult (*GetXY) (
          IDirectFBInputDevice          *thiz,
          int                           *ret_x,
          int                           *ret_y
     );
)


/*
 * Event class.
 */
typedef enum {
     DFEC_NONE           = 0x00,   /* none of these */
     DFEC_INPUT          = 0x01,   /* raw input event */
     DFEC_WINDOW         = 0x02,   /* windowing event */
     DFEC_USER           = 0x03    /* custom events for
                                      the user of this library */
} DFBEventClass;

/*
 * The type of an input event.
 */
typedef enum {
     DIET_UNKNOWN        = 0,      /* unknown event */
     DIET_KEYPRESS,                /* a key is been pressed */
     DIET_KEYRELEASE,              /* a key is been released */
     DIET_BUTTONPRESS,             /* a (mouse) button is been pressed */
     DIET_BUTTONRELEASE,           /* a (mouse) button is been released */
     DIET_AXISMOTION               /* mouse/joystick movement */
} DFBInputEventType;

/*
 * Flags defining which additional (optional) event fields are valid.
 */
typedef enum {
     DIEF_NONE           = 0x000,   /* no additional fields */
     DIEF_TIMESTAMP      = 0x001,   /* timestamp is valid */
     DIEF_AXISABS        = 0x002,   /* axis and axisabs are valid */
     DIEF_AXISREL        = 0x004,   /* axis and axisrel are valid */

     DIEF_KEYCODE        = 0x008,   /* used internally by the input core,
                                       always set at application level */
     DIEF_KEYID          = 0x010,   /* used internally by the input core,
                                       always set at application level */
     DIEF_KEYSYMBOL      = 0x020,   /* used internally by the input core,
                                       always set at application level */
     DIEF_MODIFIERS      = 0x040,   /* used internally by the input core,
                                       always set at application level */
     DIEF_LOCKS          = 0x080,   /* used internally by the input core,
                                       always set at application level */
     DIEF_BUTTONS        = 0x100,   /* used internally by the input core,
                                       always set at application level */
     DIEF_GLOBAL         = 0x200    /* Only for event buffers creates by
                                       IDirectFB::CreateInputEventBuffer()
                                       with global events enabled.
                                       Indicates that the event would have been
                                       filtered if the buffer hadn't been
                                       global. */
} DFBInputEventFlags;

/*
 * An input event, item of an input buffer.
 */
typedef struct {
     DFBEventClass                   clazz;      /* clazz of event */

     DFBInputEventType               type;       /* type of event */
     DFBInputDeviceID                device_id;  /* source of event */
     DFBInputEventFlags              flags;      /* which optional fields
                                                    are valid? */

     /* additionally (check flags) */
     struct timeval                  timestamp;  /* time of event creation */

/* DIET_KEYPRESS, DIET_KEYRELEASE */
     int                             key_code;   /* hardware keycode, no
                                                    mapping, -1 if device
                                                    doesn't differentiate
                                                    between several keys */
     DFBInputDeviceKeyIdentifier     key_id;     /* basic mapping,
                                                    modifier independent */
     DFBInputDeviceKeySymbol         key_symbol; /* advanced mapping,
                                                    unicode compatible,
                                                    modifier dependent */
     /* additionally (check flags) */
     DFBInputDeviceModifierMask      modifiers;  /* pressed modifiers
                                                    (optional) */
     DFBInputDeviceLockState         locks;      /* active locks
                                                    (optional) */

/* DIET_BUTTONPRESS, DIET_BUTTONRELEASE */
     DFBInputDeviceButtonIdentifier  button;     /* in case of a button
                                                    event */
     DFBInputDeviceButtonMask        buttons;    /* mask of currently
                                                    pressed buttons */

/* DIET_AXISMOTION */
     DFBInputDeviceAxisIdentifier    axis;       /* in case of an axis
                                                    event */
     /* one of these two (check flags) */
     int                             axisabs;    /* absolute mouse/
                                                    joystick coordinate */
     int                             axisrel;    /* relative mouse/
                                                    joystick movement */
} DFBInputEvent;

/*
 * Window Event Types - can also be used as flags for event filters.
 */
typedef enum {
     DWET_NONE           = 0x00000000,

     DWET_POSITION       = 0x00000001,  /* window has been moved by
                                           window manager or the
                                           application itself */
     DWET_SIZE           = 0x00000002,  /* window has been resized
                                           by window manager or the
                                           application itself */
     DWET_CLOSE          = 0x00000004,  /* closing this window has been
                                           requested only */
     DWET_DESTROYED      = 0x00000008,  /* window got destroyed by global
                                           deinitialization function or
                                           the application itself */
     DWET_GOTFOCUS       = 0x00000010,  /* window got focus */
     DWET_LOSTFOCUS      = 0x00000020,  /* window lost focus */

     DWET_KEYDOWN        = 0x00000100,  /* a key has gone down while
                                           window has focus */
     DWET_KEYUP          = 0x00000200,  /* a key has gone up while
                                           window has focus */

     DWET_BUTTONDOWN     = 0x00010000,  /* mouse button went down in
                                           the window */
     DWET_BUTTONUP       = 0x00020000,  /* mouse button went up in
                                           the window */
     DWET_MOTION         = 0x00040000,  /* mouse cursor changed its
                                           position in window */
     DWET_ENTER          = 0x00080000,  /* mouse cursor entered
                                           the window */
     DWET_LEAVE          = 0x00100000,  /* mouse cursor left the window */

     DWET_WHEEL          = 0x00200000,  /* mouse wheel was moved while
                                           window has focus */

     DWET_POSITION_SIZE  = DWET_POSITION | DWET_SIZE,/* initially sent to
                                                        window when it's
                                                        created */

     DWET_ALL            = 0x003F033F   /* all event types */
} DFBWindowEventType;

/*
 * Event from the windowing system.
 */
typedef struct {
     DFBEventClass                   clazz;      /* clazz of event */

     DFBWindowEventType              type;       /* type of event */
     DFBWindowID                     window_id;  /* source of event */

     /* used by DWET_MOVE, DWET_MOTION, DWET_BUTTONDOWN, DWET_BUTTONUP,
        DWET_ENTER, DWET_LEAVE */
     int                             x;          /* x position of window
                                                    or coordinate within
                                                    window */
     int                             y;          /* y position of window
                                                    or coordinate within
                                                    window */

     /* used by DWET_MOTION, DWET_BUTTONDOWN, DWET_BUTTONUP,
        DWET_ENTER, DWET_LEAVE */
     int                             cx;         /* x cursor position */
     int                             cy;         /* y cursor position */

     /* used by DWET_WHEEL */
     int                             step;       /* wheel step */

     /* used by DWET_RESIZE */
     int                             w;          /* width of window */
     int                             h;          /* height of window */

     /* used by DWET_KEYDOWN, DWET_KEYUP */
     int                             key_code;   /* hardware keycode, no
                                                    mapping, -1 if device
                                                    doesn't differentiate
                                                    between several keys */
     DFBInputDeviceKeyIdentifier     key_id;     /* basic mapping,
                                                    modifier independent */
     DFBInputDeviceKeySymbol         key_symbol; /* advanced mapping,
                                                    unicode compatible,
                                                    modifier dependent */
     DFBInputDeviceModifierMask      modifiers;  /* pressed modifiers */
     DFBInputDeviceLockState         locks;      /* active locks */

     /* used by DWET_BUTTONDOWN, DWET_BUTTONUP */
     DFBInputDeviceButtonIdentifier  button;     /* button being
                                                    pressed or released */
     /* used by DWET_MOTION, DWET_BUTTONDOWN, DWET_BUTTONUP */
     DFBInputDeviceButtonMask        buttons;    /* mask of currently
                                                    pressed buttons */

     struct timeval                  timestamp;  /* always set */
} DFBWindowEvent;

/*
 * Event for usage by the user of this library.
 */
typedef struct {
     DFBEventClass                   clazz;      /* clazz of event */

     unsigned int                    type;       /* custom type */
     void                           *data;       /* custom data */
} DFBUserEvent;

/*
 * General container for a DirectFB Event.
 */
typedef union {
     DFBEventClass            clazz;   /* clazz of event */
     DFBInputEvent            input;   /* field for input events */
     DFBWindowEvent           window;  /* field for window events */
     DFBUserEvent             user;    /* field for user-defined events */
} DFBEvent;

#define DFB_EVENT(e)          ((DFBEvent *) (e))

/************************
 * IDirectFBEventBuffer *
 ************************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBEventBuffer,


   /** Buffer handling **/

     /*
      * Clear all events stored in this buffer.
      */
     DFBResult (*Reset) (
          IDirectFBEventBuffer     *thiz
     );


   /** Waiting for events **/

     /*
      * Wait for the next event to occur.
      * Thread is idle in the meantime.
      */
     DFBResult (*WaitForEvent) (
          IDirectFBEventBuffer     *thiz
     );

     /*
      * Block until next event to occur or timeout is reached.
      * Thread is idle in the meantime.
      */
     DFBResult (*WaitForEventWithTimeout) (
          IDirectFBEventBuffer     *thiz,
          unsigned int              seconds,
          unsigned int              milli_seconds
     );


   /** Fetching events **/

     /*
      * Get the next event and remove it from the FIFO.
      */
     DFBResult (*GetEvent) (
          IDirectFBEventBuffer     *thiz,
          DFBEvent                 *ret_event
     );

     /*
      * Get the next event but leave it there, i.e. do a preview.
      */
     DFBResult (*PeekEvent) (
          IDirectFBEventBuffer     *thiz,
          DFBEvent                 *ret_event
     );

     /*
      * Check if there is a pending event in the queue. This
      * function returns DFB_OK if there is at least one event,
      * DFB_BUFFER_EMPTY otherwise.
      */
     DFBResult (*HasEvent) (
          IDirectFBEventBuffer     *thiz
     );


   /** Sending events **/

     /*
      * Put an event into the FIFO.
      *
      * This function does not wait until the event got fetched.
      */
     DFBResult (*PostEvent) (
          IDirectFBEventBuffer     *thiz,
          const DFBEvent           *event
     );

     /*
      * Wake up any thread waiting for events in this buffer.
      *
      * This method causes any IDirectFBEventBuffer::WaitForEvent() or
      * IDirectFBEventBuffer::WaitForEventWithTimeout() call to return with DFB_INTERRUPTED.
      *
      * It should be used rather than sending wake up messages which
      * may pollute the queue and consume lots of CPU and memory compared to
      * this 'single code line method'.
      */
     DFBResult (*WakeUp) (
          IDirectFBEventBuffer     *thiz
     );


   /** Special handling **/

     /*
      * Create a file descriptor for reading events.
      *
      * This method provides an alternative for reading events from an event buffer.
      * It creates a file descriptor which can be used in select(), poll() or read().
      *
      * In general only non-threaded applications which already use select() or poll() need it.
      *
      * <b>Note:</b> This method flushes the event buffer. After calling this method all other
      * methods except IDirectFBEventBuffer::PostEvent() will return DFB_UNSUPPORTED.
      * Calling this method again will return DFB_BUSY.
      */
     DFBResult (*CreateFileDescriptor) (
          IDirectFBEventBuffer     *thiz,
          int                      *ret_fd
     );
)

/*
 * Flags controlling the appearance and behaviour of the window.
 */
typedef enum {
     DWOP_NONE           = 0x00000000,  /* none of these */
     DWOP_COLORKEYING    = 0x00000001,  /* enable color key */
     DWOP_ALPHACHANNEL   = 0x00000002,  /* enable alpha blending using the
                                           window's alpha channel */
     DWOP_OPAQUE_REGION  = 0x00000004,  /* overrides DWOP_ALPHACHANNEL for the
                                           region set by SetOpaqueRegion() */
     DWOP_SHAPED         = 0x00000008,  /* window doesn't receive mouse events for
                                           invisible regions, must be used with
                                           DWOP_ALPHACHANNEL or DWOP_COLORKEYING */
     DWOP_KEEP_POSITION  = 0x00000010,  /* window can't be moved
                                           with the mouse */
     DWOP_KEEP_SIZE      = 0x00000020,  /* window can't be resized
                                           with the mouse */
     DWOP_KEEP_STACKING  = 0x00000040,  /* window can't be raised
                                           or lowered with the mouse */
     DWOP_GHOST          = 0x00001000,  /* never get focus or input,
                                           clicks will go through,
                                           implies DWOP_KEEP... */
     DWOP_INDESTRUCTIBLE = 0x00002000,  /* window can't be destroyed
                                           by internal shortcut */
     DWOP_ALL            = 0x0000307F   /* all possible options */
} DFBWindowOptions;

/*
 * The stacking class restricts the stacking order of windows.
 */
typedef enum {
     DWSC_MIDDLE         = 0x00000000,  /* This is the default stacking
                                           class of new windows. */
     DWSC_UPPER          = 0x00000001,  /* Window is always above windows
                                           in the middle stacking class.
                                           Only windows that are also in
                                           the upper stacking class can
                                           get above them. */
     DWSC_LOWER          = 0x00000002   /* Window is always below windows
                                           in the middle stacking class.
                                           Only windows that are also in
                                           the lower stacking class can
                                           get below them. */
} DFBWindowStackingClass;

/*******************
 * IDirectFBWindow *
 *******************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBWindow,

   /** Retrieving information **/

     /*
      * Get the unique window ID.
      */
     DFBResult (*GetID) (
          IDirectFBWindow               *thiz,
          DFBWindowID                   *ret_window_id
     );

     /*
      * Get the current position of this window.
      */
     DFBResult (*GetPosition) (
          IDirectFBWindow               *thiz,
          int                           *ret_x,
          int                           *ret_y
     );

     /*
      * Get the size of the window in pixels.
      */
     DFBResult (*GetSize) (
          IDirectFBWindow               *thiz,
          int                           *ret_width,
          int                           *ret_height
     );


   /** Event handling **/

     /*
      * Create an event buffer for this window and attach it.
      */
     DFBResult (*CreateEventBuffer) (
          IDirectFBWindow               *thiz,
          IDirectFBEventBuffer         **ret_buffer
     );

     /*
      * Attach an existing event buffer to this window.
      *
      * NOTE: Attaching multiple times generates multiple events.
      *
      */
     DFBResult (*AttachEventBuffer) (
          IDirectFBWindow               *thiz,
          IDirectFBEventBuffer          *buffer
     );

     /*
      * Enable specific events to be sent to the window.
      *
      * The argument is a mask of events that will be set in the
      * window's event mask. The default event mask is DWET_ALL.
      */
     DFBResult (*EnableEvents) (
          IDirectFBWindow               *thiz,
          DFBWindowEventType             mask
     );

     /*
      * Disable specific events from being sent to the window.
      *
      * The argument is a mask of events that will be cleared in
      * the window's event mask. The default event mask is DWET_ALL.
      */
     DFBResult (*DisableEvents) (
          IDirectFBWindow               *thiz,
          DFBWindowEventType             mask
     );


   /** Surface handling **/

     /*
      * Get an interface to the backing store surface.
      *
      * This surface has to be flipped to make previous drawing
      * commands visible, i.e. to repaint the windowstack for
      * that region.
      */
     DFBResult (*GetSurface) (
          IDirectFBWindow               *thiz,
          IDirectFBSurface             **ret_surface
     );


   /** Appearance and Behaviour **/

     /*
      * Set options controlling appearance and behaviour of the window.
      */
     DFBResult (*SetOptions) (
          IDirectFBWindow               *thiz,
          DFBWindowOptions               options
     );

     /*
      * Get options controlling appearance and behaviour of the window.
      */
     DFBResult (*GetOptions) (
          IDirectFBWindow               *thiz,
          DFBWindowOptions              *ret_options
     );

     /*
      * Set the window color key.
      *
      * If a pixel of the window matches this color the
      * underlying window or the background is visible at this
      * point.
      */
     DFBResult (*SetColorKey) (
          IDirectFBWindow               *thiz,
          __u8                           r,
          __u8                           g,
          __u8                           b
     );

     /*
      * Set the window color key (indexed).
      *
      * If a pixel (indexed format) of the window matches this
      * color index the underlying window or the background is
      * visible at this point.
      */
     DFBResult (*SetColorKeyIndex) (
          IDirectFBWindow               *thiz,
          unsigned int                   index
     );

     /*
      * Set the window's global opacity factor.
      *
      * Set it to "0" to hide a window.
      * Setting it to "0xFF" makes the window opaque if
      * it has no alpha channel.
      */
     DFBResult (*SetOpacity) (
          IDirectFBWindow               *thiz,
          __u8                           opacity
     );

     /*
      * Disable alpha channel blending for one region of the window.
      *
      * If DWOP_ALPHACHANNEL and DWOP_OPAQUE_REGION are set but not DWOP_COLORKEYING
      * and the opacity of the window is 255 the window gets rendered
      * without alpha blending within the specified region.
      *
      * This is extremely useful for alpha blended window decorations while
      * the main content stays opaque and gets rendered faster.
      */
     DFBResult (*SetOpaqueRegion) (
          IDirectFBWindow               *thiz,
          int                            x1,
          int                            y1,
          int                            x2,
          int                            y2
     );

     /*
      * Get the current opacity factor of this window.
      */
     DFBResult (*GetOpacity) (
          IDirectFBWindow               *thiz,
          __u8                          *ret_opacity
     );

     /*
      * Bind a cursor shape to this window.
      *
      * This method will set a per-window cursor shape. Everytime
      * the cursor enters this window, the specified shape is set.
      *
      * Passing NULL will unbind a set shape and release its surface.
      */
     DFBResult (*SetCursorShape) (
          IDirectFBWindow               *thiz,
          IDirectFBSurface              *shape,
          int                            hot_x,
          int                            hot_y
     );


   /** Focus handling **/

     /*
      * Pass the focus to this window.
      */
     DFBResult (*RequestFocus) (
          IDirectFBWindow               *thiz
     );

     /*
      * Grab the keyboard, i.e. all following keyboard events are
      * sent to this window ignoring the focus.
      */
     DFBResult (*GrabKeyboard) (
          IDirectFBWindow               *thiz
     );

     /*
      * Ungrab the keyboard, i.e. switch to standard key event
      * dispatching.
      */
     DFBResult (*UngrabKeyboard) (
          IDirectFBWindow               *thiz
     );

     /*
      * Grab the pointer, i.e. all following mouse events are
      * sent to this window ignoring the focus.
      */
     DFBResult (*GrabPointer) (
          IDirectFBWindow               *thiz
     );

     /*
      * Ungrab the pointer, i.e. switch to standard mouse event
      * dispatching.
      */
     DFBResult (*UngrabPointer) (
          IDirectFBWindow               *thiz
     );

     /*
      * Grab a specific key, i.e. all following events of this key are
      * sent to this window ignoring the focus.
      */
     DFBResult (*GrabKey) (
          IDirectFBWindow               *thiz,
          DFBInputDeviceKeySymbol        symbol,
          DFBInputDeviceModifierMask     modifiers
     );

     /*
      * Ungrab a specific key, i.e. switch to standard key event
      * dispatching.
      */
     DFBResult (*UngrabKey) (
          IDirectFBWindow               *thiz,
          DFBInputDeviceKeySymbol        symbol,
          DFBInputDeviceModifierMask     modifiers
     );


   /** Position and Size **/

     /*
      * Move the window by the specified distance.
      */
     DFBResult (*Move) (
          IDirectFBWindow               *thiz,
          int                            dx,
          int                            dy
     );

     /*
      * Move the window to the specified coordinates.
      */
     DFBResult (*MoveTo) (
          IDirectFBWindow               *thiz,
          int                            x,
          int                            y
     );

     /*
      * Resize the window.
      */
     DFBResult (*Resize) (
          IDirectFBWindow               *thiz,
          int                            width,
          int                            height
     );


   /** Stacking **/

     /*
      * Put the window into a specific stacking class.
      */
     DFBResult (*SetStackingClass) (
          IDirectFBWindow               *thiz,
          DFBWindowStackingClass         stacking_class
     );

     /*
      * Raise the window by one within the window stack.
      */
     DFBResult (*Raise) (
          IDirectFBWindow               *thiz
     );

     /*
      * Lower the window by one within the window stack.
      */
     DFBResult (*Lower) (
          IDirectFBWindow               *thiz
     );

     /*
      * Put the window on the top of the window stack.
      */
     DFBResult (*RaiseToTop) (
          IDirectFBWindow               *thiz
     );

     /*
      * Send a window to the bottom of the window stack.
      */
     DFBResult (*LowerToBottom) (
          IDirectFBWindow               *thiz
     );

     /*
      * Put a window on top of another window.
      */
     DFBResult (*PutAtop) (
          IDirectFBWindow               *thiz,
          IDirectFBWindow               *lower
     );

     /*
      * Put a window below another window.
      */
     DFBResult (*PutBelow) (
          IDirectFBWindow               *thiz,
          IDirectFBWindow               *upper
     );


   /** Closing **/

     /*
      * Send a close message to the window.
      *
      * This function sends a message of type DWET_CLOSE to the window.
      * It does NOT actually close it.
      */
     DFBResult (*Close) (
          IDirectFBWindow               *thiz
     );

     /*
      * Destroys the window and sends a destruction message.
      *
      * This function sends a message of type DWET_DESTROY to
      * the window after removing it from the window stack and
      * freeing its data.  Some functions called from this
      * interface will return DFB_DESTROYED after that.
      */
     DFBResult (*Destroy) (
          IDirectFBWindow               *thiz
     );
)


/*****************
 * IDirectFBFont *
 *****************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBFont,

   /** Retrieving information **/

     /*
      * Get the distance from the baseline to the top of the
      * logical extents of this font.
      */
     DFBResult (*GetAscender) (
          IDirectFBFont       *thiz,
          int                 *ret_ascender
     );

     /*
      * Get the distance from the baseline to the bottom of
      * the logical extents of this font.
      *
      * This is a negative value!
      */
     DFBResult (*GetDescender) (
          IDirectFBFont       *thiz,
          int                 *ret_descender
     );

     /*
      * Get the bounding box required for the largest character 
      * for this font. 
      */
     DFBResult (*GetBoundingBox) (
          IDirectFBFont         *thiz,
          DFBBoundingBox      *bbox
     );

     /*
      * Get the layout of the font.  This is the format
      * that the font will be displayed on the screen.
      */
     DFBResult (*GetTextLayout) (
          IDirectFBFont       *thiz,
          DFBFontLayout       *ret_layout
     );


     /*
      * Get the logical height of one text line. This is the vertical
      * distance from one baseline to the next when writing
      * several lines of text. Note that this value does not
      * correspond the height value specified when loading the
      * font.
      */
     DFBResult (*GetHeight) (
          IDirectFBFont       *thiz,
          int                 *ret_height
     );

     /*
      * Get the maximum character width.
      *
      * This is a somewhat dubious value. Not all fonts
      * specify it correcly. It can give you an idea of
      * the maximum expected width of a rendered string.
      */
     DFBResult (*GetMaxAdvance) (
          IDirectFBFont       *thiz,
          int                 *ret_maxadvance
     );

     /*
      * Get the kerning to apply between two glyphs specified by
      * their Unicode indices.
      */
     DFBResult (*GetKerning) (
          IDirectFBFont       *thiz,
          unsigned int         prev_index,
          unsigned int         current_index,
          int                 *ret_kern_x,
          int                 *ret_kern_y
     );

   /** String extents measurement **/

     /*
      * Get the logical length of the specified UTF-8 string
      * as if it were drawn with this font.  For horizontal
      * font output the string width will be returned, for
      * vertical output the height will be retuned.
      *
      * Bytes specifies the number of bytes to take from the
      * string or -1 for the complete NULL-terminated string.
      *
      * The returned length may be different than the actual drawn
      * length of the text since this function returns the logical
      * length that should be used to layout the text. A negative
      * length indicates right-to-left or top-to-bottom rendering.
      */
     DFBResult (*GetStringAdvance) (
          IDirectFBFont       *thiz,
          const char          *text,
          int                  bytes,
          int                 *ret_width
     );

     /*
      * Get the logical and real extents of the specified
      * UTF-8 string as if it were drawn with this font.
      *
      * Bytes specifies the number of bytes to take from the
      * string or -1 for the complete NULL-terminated string.
      *
      * The logical rectangle describes the typographic extents
      * and should be used to layout text. The ink rectangle
      * describes the smallest rectangle containing all pixels
      * that are touched when drawing the string. If you only
      * need one of the rectangles, pass NULL for the other one.
      *
      * The ink rectangle is guaranteed to be a valid rectangle
      * with positive width and height, while the logical
      * rectangle may have negative width indicating right-to-left
      * layout.
      *
      * The rectangle offsets are reported relative to the
      * baseline and refer to the text being drawn using
      * DSTF_DEFAULT.
      */
     DFBResult (*GetStringExtents) (
          IDirectFBFont       *thiz,
          const char          *text,
          int                  bytes,
          DFBRectangle        *ret_logical_rect,
          DFBRectangle        *ret_ink_rect
     );

     /*
      * Get the extents of a glyph specified by its Unicode
      * index.
      *
      * The rectangle describes the the smallest rectangle
      * containing all pixels that are touched when drawing the
      * glyph. It is reported relative to the baseline. If you
      * only need the advance, pass NULL for the rectangle.
      *
      * The advance describes the horizontal offset to the next
      * glyph (without kerning applied). It may be a negative
      * value indicating left-to-right rendering. If you don't
      * need this value, pass NULL for advance.
      */
    DFBResult (*GetGlyphExtents) (
          IDirectFBFont       *thiz,
          unsigned int         index,
          DFBRectangle        *ret_rect,
          int                 *ret_advance
     );


   /** DEPRICATED FUNCTIONS - DO NOT CALL THESE!! **/

     /*
      *  ***DEPRICATED - Call GetStringAdvance instead
      */
     DFBResult (*GetStringWidth) (
          IDirectFBFont       *thiz,
          const char          *text,
          int                  bytes,
          int                 *ret_width
     );
 )

/*
 * Capabilities of an image.
 */
typedef enum {
     DICAPS_NONE            = 0x00000000,  /* None of these.            */
     DICAPS_ALPHACHANNEL    = 0x00000001,  /* The image data contains an
                                              alphachannel.             */
     DICAPS_COLORKEY        = 0x00000002   /* The image has a colorkey,
                                              e.g. the transparent color
                                              of a GIF image.           */
} DFBImageCapabilities;

/*
 * Information about an image including capabilities and values
 * belonging to available capabilities.
 */
typedef struct {
     DFBImageCapabilities     caps;        /* capabilities              */

     __u8                     colorkey_r;  /* colorkey red channel      */
     __u8                     colorkey_g;  /* colorkey green channel    */
     __u8                     colorkey_b;  /* colorkey blue channel     */
    char                      decoderName[128]; /* user friendly name of the
                                              image decoder that will
                                              decode this image */
    char                      decoderReason[128]; /* user friendly reason why
                                                    the decoer in use is being
                                                    used */
} DFBImageDescription;

typedef enum {
	DI_BLEND_SOURCE_APNG = 0,
	DI_BLEND_OVER_APNG = 1,
	DI_BLEND_AGIF = 2
} DFBAPNGBlend;	/*Blend methods for Animated PNG*/

typedef enum {
	DI_DISPOSE_NONE         = 0,     /* Don't do anything */
	DI_DISPOSE_KEEP         = 1,     /* Keep image (same as 0) */
	DI_DISPOSE_RESTORE_BG   = 2,     /* Restore background after delay */
	DI_DISPOSE_RESTORE_PREV = 3,     /* Restore previous contents (generally not supported) */
	DI_DISPOSE_UNUSED = 0xff
} DFBDisposeOp;



typedef struct {

	unsigned int frameDelay;
	unsigned int firstFrame;
	unsigned int x_offset;
	unsigned int y_offset;
	unsigned int width;
	unsigned int height;
	DFBDisposeOp dispose_op;
	DFBAPNGBlend blend_op; /*Used only for APNG for now*/
	unsigned int isPartOfAnimation;

}DFBAnimationDescription;


/*
 * Called whenever a chunk of the image is decoded.
 * Has to be registered with IDirectFBImageProvider::SetRenderCallback().
 */
typedef void (*DIRenderCallback)(DFBRectangle *rect, void *ctx);

typedef enum {
	unknown = 0,
	CIF = 1,
	CVF = 2,
	GIF = 3,
	IMLIB2 = 4,
	JPEG = 5,
	MPEG2 = 6,
	PNG = 7,
	PNM = 8,
	SVG = 9
	
} ImageFormat;

typedef struct {

	ImageFormat imageFormat;
	int first_segment;
	int segment_height;
	int segmentedDecodingOn;
	
} DFBPipelineImageDecodingAndScaling;

/**************************
 * IDirectFBImageProvider *
 **************************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBImageProvider,

   /** Retrieving information **/

     /*
      * Get a surface description for best performance on the
      * system.
      */
     DFBResult (*GetSurfaceDescription) (
          IDirectFBImageProvider   *thiz,
          DFBSurfaceDescription    *ret_dsc
     );

     /*
      * Get a description of the image.
      *
      * This includes stuff that does not belong into the surface
      * description, e.g. a colorkey of a transparent GIF.
      */
     DFBResult (*GetImageDescription) (
          IDirectFBImageProvider   *thiz,
          DFBImageDescription      *ret_dsc
     );


   /** Rendering **/

     /*
      * Render the file contents into the destination contents
      * doing automatic scaling and color format conversion.
      *
      * If the image file has an alpha channel it is rendered
      * with alpha channel if the destination surface is of the
      * ARGB pixelformat. Otherwise, transparent areas are
      * blended over a black background.
      *
      * If a destination rectangle is specified, the rectangle is
      * clipped to the destination surface. If NULL is passed as
      * destination rectangle, the whole destination surface is
      * taken. The image is stretched to fill the rectangle.
      */
     DFBResult (*RenderTo) (
          IDirectFBImageProvider   *thiz,
          IDirectFBSurface         *destination,
          const DFBRectangle       *destination_rect
     );

     /*
      * Registers a callback for progressive image loading.
      *
      * The function is called each time a chunk of the image is decoded.
      */
     DFBResult (*SetRenderCallback) (
          IDirectFBImageProvider   *thiz,
          DIRenderCallback          callback,
          void                     *callback_data
     );

     /*
      * Render the file contents into the destination given
      * the surface is proper size and color format. If those
      * conditions are not met, the function will fail.
      *
      */
     DFBResult (*RenderToUnaltered) (
          IDirectFBImageProvider   *thiz,
          IDirectFBSurface         *destination,
          const DFBRectangle       *destination_rect
     );

     /*
      * Get a surface description that best matches the image
      * contained in the file.
      *
      * For opaque image formats the pixel format of the primary
      * layer is used. For images with alpha channel an ARGB
      * surface description is returned.
      */
     DFBResult (*GetSurfaceDescriptionUnaltered) (
          IDirectFBImageProvider   *thiz,
          DFBSurfaceDescription    *ret_dsc
     );

	 DFBPipelineImageDecodingAndScaling PipelineImageDecodingAndScaling;

	 DFBAnimationDescription AnimationDescription;
)

/*
 * Capabilities of an audio/video stream.
 */
typedef enum {
     DVSCAPS_NONE         = 0x00000000, /* None of these.         */
     DVSCAPS_VIDEO        = 0x00000001, /* Stream contains video. */
     DVSCAPS_AUDIO        = 0x00000002  /* Stream contains audio. */
     /* DVSCAPS_SUBPICTURE ?! */
} DFBStreamCapabilities;

#define DFB_STREAM_DESC_ENCODING_LENGTH   30
#define DFB_STREAM_DESC_TITLE_LENGTH     255
#define DFB_STREAM_DESC_AUTHOR_LENGTH    255
#define DFB_STREAM_DESC_ALBUM_LENGTH     255
#define DFB_STREAM_DESC_GENRE_LENGTH      32
#define DFB_STREAM_DESC_COMMENT_LENGTH   255

/*
 * Informations about an audio/video stream.
 */
typedef struct {
     DFBStreamCapabilities  caps;         /* capabilities                       */

     struct {
          char              encoding[DFB_STREAM_DESC_ENCODING_LENGTH]; /*
                                             encoding (e.g. "MPEG4")            */
          double            framerate;    /* number of frames per second        */
          double            aspect;       /* frame aspect ratio                 */
          int               bitrate;      /* amount of bits per second          */
     } video;

     struct {
          char              encoding[DFB_STREAM_DESC_ENCODING_LENGTH]; /*
                                             encoding (e.g. "AAC")              */
          int               samplerate;   /* number of samples per second       */
          int               channels;     /* number of channels per sample      */
          int               bitrate;      /* amount of bits per second          */
     } audio;

     char                   title[DFB_STREAM_DESC_TITLE_LENGTH];     /* title   */
     char                   author[DFB_STREAM_DESC_AUTHOR_LENGTH];   /* author  */
     char                   album[DFB_STREAM_DESC_ALBUM_LENGTH];     /* album   */
     short                  year;                                    /* year    */
     char                   genre[DFB_STREAM_DESC_GENRE_LENGTH];     /* genre   */
     char                   comment[DFB_STREAM_DESC_COMMENT_LENGTH]; /* comment */
} DFBStreamDescription;

/*
 * Called for each written frame.
 */
typedef int (*DVFrameCallback)(void *ctx);


/**************************
 * IDirectFBVideoProvider *
 **************************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBVideoProvider,

   /** Retrieving information **/

     /*
      * Retrieve information about the video provider's
      * capabilities.
      */
     DFBResult (*GetCapabilities) (
          IDirectFBVideoProvider        *thiz,
          DFBVideoProviderCapabilities  *ret_caps
     );

     /*
      * Get a surface description that best matches the video
      * contained in the file.
      */
     DFBResult (*GetSurfaceDescription) (
          IDirectFBVideoProvider   *thiz,
          DFBSurfaceDescription    *ret_dsc
     );

     /*
      * Get a description of the video stream.
      */
     DFBResult (*GetStreamDescription) (
          IDirectFBVideoProvider   *thiz,
          DFBStreamDescription     *ret_dsc
     );

   /** Playback **/

     /*
      * Play the video rendering it into the specified rectangle
      * of the destination surface.
      *
      * Optionally a callback can be registered that is called
      * for each rendered frame. This is especially important if
      * you are playing to a flipping surface. In this case, you
      * should flip the destination surface in your callback.
      */
     DFBResult (*PlayTo) (
          IDirectFBVideoProvider   *thiz,
          IDirectFBSurface         *destination,
          const DFBRectangle       *destination_rect,
          DVFrameCallback           callback,
          void                     *ctx
     );

     /*
      * Stop rendering into the destination surface.
      */
     DFBResult (*Stop) (
          IDirectFBVideoProvider   *thiz
     );

     /*
      * Get the status of the playback.
      */
     DFBResult (*GetStatus) (
          IDirectFBVideoProvider   *thiz,
          DFBVideoProviderStatus   *ret_status
     );

   /** Media Control **/

     /*
      * Seeks to a position within the stream.
      */
     DFBResult (*SeekTo) (
          IDirectFBVideoProvider   *thiz,
          double                    seconds
     );

     /*
      * Gets current position within the stream.
      */
     DFBResult (*GetPos) (
          IDirectFBVideoProvider   *thiz,
          double                   *ret_seconds
     );

     /*
      * Gets the length of the stream.
      */
     DFBResult (*GetLength) (
          IDirectFBVideoProvider   *thiz,
          double                   *ret_seconds
     );

   /** Color Adjustment **/

     /*
      * Gets the current video color settings.
      */
     DFBResult (*GetColorAdjustment) (
          IDirectFBVideoProvider   *thiz,
          DFBColorAdjustment       *ret_adj
     );

     /*
      * Adjusts the video colors.
      *
      * This function only has an effect if the video provider
      * supports this operation. Check the providers capabilities
      * to find out if this is the case.
      */
     DFBResult (*SetColorAdjustment) (
          IDirectFBVideoProvider   *thiz,
          const DFBColorAdjustment *adj
     );

   /** Interactivity **/

     /*
      * Send an input or window event.
      *
      * This method allows to redirect events to an interactive
      * video provider. Events must be relative to the specified
      * rectangle of the destination surface.
      */
     DFBResult (*SendEvent) (
          IDirectFBVideoProvider   *thiz,
          const DFBEvent           *event
     );
)

/***********************
 * IDirectFBDataBuffer *
 ***********************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBDataBuffer,


   /** Buffer handling **/

     /*
      * Flushes all data in this buffer.
      *
      * This method only applies to streaming buffers.
      */
     DFBResult (*Flush) (
          IDirectFBDataBuffer      *thiz
     );

     /*
      * Finish writing into a streaming buffer.
      *
      * Subsequent calls to PutData will fail,
      * while attempts to fetch data from the buffer will return EOF
      * unless there is still data available.
      */
     DFBResult (*Finish) (
          IDirectFBDataBuffer      *thiz
     );

     /*
      * Seeks to a given byte position.
      *
      * This method only applies to static buffers.
      */
     DFBResult (*SeekTo) (
          IDirectFBDataBuffer      *thiz,
          unsigned int              offset
     );

     /*
      * Get the current byte position within a static buffer.
      *
      * This method only applies to static buffers.
      */
     DFBResult (*GetPosition) (
          IDirectFBDataBuffer      *thiz,
          unsigned int             *ret_offset
     );

     /*
      * Get the length of a static or streaming buffer in bytes.
      *
      * The length of a static buffer is its static size.
      * A streaming buffer has a variable length reflecting
      * the amount of buffered data.
      */
     DFBResult (*GetLength) (
          IDirectFBDataBuffer      *thiz,
          unsigned int             *ret_length
     );


   /** Waiting for data **/

     /*
      * Wait for data to be available.
      * Thread is idle in the meantime.
      *
      * This method blocks until at least the specified number of bytes
      * is available.
      */
     DFBResult (*WaitForData) (
          IDirectFBDataBuffer      *thiz,
          unsigned int              length
     );

     /*
      * Wait for data to be available within an amount of time.
      * Thread is idle in the meantime.
      *
      * This method blocks until at least the specified number of bytes
      * is available or the timeout is reached.
      */
     DFBResult (*WaitForDataWithTimeout) (
          IDirectFBDataBuffer      *thiz,
          unsigned int              length,
          unsigned int              seconds,
          unsigned int              milli_seconds
     );


   /** Retrieving data **/

     /*
      * Fetch data from a streaming or static buffer.
      *
      * Static buffers will increase the data pointer.
      * Streaming buffers will flush the data portion.
      *
      * The maximum number of bytes to fetch is specified by "length",
      * the actual number of bytes fetched is returned via "read".
      */
     DFBResult (*GetData) (
          IDirectFBDataBuffer      *thiz,
          unsigned int              length,
          void                     *ret_data,
          unsigned int             *ret_read
     );

     /*
      * Peek data from a streaming or static buffer.
      *
      * Unlike GetData() this method won't increase the data
      * pointer or flush any portions of the data held.
      *
      * Additionally an offset relative to the current data pointer
      * or beginning of the streaming buffer can be specified.
      *
      * The maximum number of bytes to peek is specified by "length",
      * the actual number of bytes peeked is returned via "read".
      */
     DFBResult (*PeekData) (
          IDirectFBDataBuffer      *thiz,
          unsigned int              length,
          int                       offset,
          void                     *ret_data,
          unsigned int             *ret_read
     );

     /*
      * Check if there is data available.
      *
      * This method returns DFB_OK if there is data available,
      * DFB_BUFFER_EMPTY otherwise.
      */
     DFBResult (*HasData) (
          IDirectFBDataBuffer      *thiz
     );


   /** Providing data **/

     /*
      * Appends a block of data to a streaming buffer.
      *
      * This method does not wait until the data got fetched.
      *
      * Static buffers don't support this method.
      */
     DFBResult (*PutData) (
          IDirectFBDataBuffer      *thiz,
          const void               *data,
          unsigned int              length
     );


   /** Media from data **/

     /*
      * Creates an image provider using the buffers data.
      */
     DFBResult (*CreateImageProvider) (
          IDirectFBDataBuffer      *thiz,
          IDirectFBImageProvider  **interface
     );

     /*
      * Creates a video provider using the buffers data.
      */
     DFBResult (*CreateVideoProvider) (
          IDirectFBDataBuffer      *thiz,
          IDirectFBVideoProvider  **interface
     );
)

/***************************
 * IDirectFBSurfaceManager *
 ***************************/

/*
 * <i>No summary yet...</i>
 */
DEFINE_INTERFACE(   IDirectFBSurfaceManager,


   /** Surfaces **/

     /*
      * Create a surface matching the specified description.
      */
     DFBResult (*CreateSurface) (
          IDirectFBSurfaceManager       *thiz,
          const DFBSurfaceDescription   *desc,
          IDirectFBSurface             **ret_interface
     );
)

#ifdef __cplusplus
}
#endif

#endif
