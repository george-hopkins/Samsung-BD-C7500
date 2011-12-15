/*
 * Copyright (C) 2006 Claudio Ciccani <klan@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License ver 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <directfb.h>

#include <display/idirectfbsurface.h>

#include <media/idirectfbimageprovider.h>

#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/surfaces.h>

#include <gfx/convert.h>

#include <misc/gfx_util.h>
#include <misc/util.h>

#include <direct/types.h>
#include <direct/messages.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/interface.h>

#define BMP_DL_NONE     0
#define BMP_DL_ERR      1
#define BMP_DL_MSG      2
#define BMP_DL_TRACE    3

#define BMP_DEBUG_LEVEL     BMP_DL_ERR
#define SHOW_BITMAP_DATA    0
#define SHOW_PALETTE        0
#define SHOW_DECODING_MSGS  0

#if BMP_DEBUG_LEVEL >= BMP_DL_ERR
#define BMP_ERR(x...)   D_ERROR(x)
#else
#define BMP_ERR(x...)
#endif
#if BMP_DEBUG_LEVEL >= BMP_DL_MSG
#define BMP_MSG(x...)   printf(x)
#else
#define BMP_MSG(x...)
#endif
#if BMP_DEBUG_LEVEL >= BMP_DL_TRACE
#define BMP_TRACE(x...) printf(x)
#else
#define BMP_TRACE(x...)
#endif

#if SHOW_DECODING_MSGS
#define BMP_DEC_MSG(x...) printf(x)
#else
#define BMP_DEC_MSG(x...)
#endif

static DFBResult Probe(IDirectFBImageProvider_ProbeContext *ctx);

static DFBResult Construct(IDirectFBImageProvider *thiz,
                           IDirectFBDataBuffer    *db);

extern IDirectFB *idirectfb_singleton;

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION(IDirectFBImageProvider, BMP)

#define CHECK_FETCH(func_call)                                  \
{                                                               \
    DFBResult return_code;                                      \
    if ((return_code = (func_call)) != DFB_OK)                  \
    {                                                           \
        BMP_ERR("%s, line %d: Error reading from file.\n",      \
            __FUNCTION__, __LINE__);                            \
        BMP_TRACE("%s: Leave\n", __FUNCTION__);                 \
        return return_code;                                     \
    }                                                           \
}

typedef enum
{
    BMPIC_RGB           = 0,
    BMPIC_RLE8          = 1,
    BMPIC_RLE4          = 2,
    BMPIC_BITFIELDS     = 3,
    BMPIC_JPEG          = 4,
    BMPIC_PNG           = 5
} BMPImageCompression;

typedef enum
{
    BMPCS_CIE_XYZ       = 0,           /* 1931 CIE XYZ standard */
    BMPCS_RGB           = 1,           /* device dependent RGB */
    BMPCS_CMYK          = 2            /* device dependent CMYK */
} BMPColorSpace;

typedef enum
{
    BMPVER_INVALID  = 0,
    BMPVER_1        = 1,
    BMPVER_2        = 2,
    BMPVER_3        = 3,
    BMPVER_NT       = 4,
    BMPVER_4        = 5,
    BMPVER_5        = 6
} BMPVersion;

typedef enum
{
    BM_HDR_SIZE_V2  = 12,
    BM_HDR_SIZE_V3  = 40,
    BM_HDR_SIZE_NT  = 52,
    BM_HDR_SIZE_V4  = 108,
    BM_HDR_SIZE_V5  = 124
} BM_HDR_SIZE;

typedef struct
{
    __u16     type;                             /* File type identifier (v1: always 0, 
                                                                         v2, v3, vnt, v4: always "BM") */
    union
    {
        struct
        {
            __u16     width;                        /* Width of bitmap in pixels */
            __u16     height;                       /* Height of bitmap in scan lines */
            __u16     byte_width;                   /* Width of bitmap in bytes */
            __u8      planes;                       /* Number of color planes (always 1) */
            __u8      bpp;                          /* Number of bits per pixel (1, 4, or 8) */
        } __attribute__((packed)) ddb;
        struct
        {
            __u32     size;                         /* Size of the file in bytes */
            __u16     reserved1;                    /* Always 0 */
            __u16     reserved2;                    /* Always 0 */
            __u32     offset;                       /* Starting position of image data in bytes */
        } __attribute__((packed)) bmp;
    } __attribute__((packed)) v;
} __attribute__((packed)) BMPFileHeader;

#define BMP_SIZEOF_WIDTH    ((data->ver == BMPVER_2) ? sizeof(__u16) : sizeof(__u32))
#define BMP_SIZEOF_HEIGHT   BMP_SIZEOF_WIDTH

typedef struct
{
    /* Version >= 2 */
    __u32     size;                         /* Size of this header in bytes */
    __u32     width;                        /* Image width in pixels (v2: short, v3+: long) */
    __u32     height;                       /* Image height in pixels (v2: short, v3+: long) */
    __u16     planes;                       /* Number of color planes (always 1) */
    __u16     bpp;                          /* Number of bits per pixel (v2, v3:  1, 4, 8, 24; 
                                                                         vnt, v4: adds 16, 32) */
    /* Version >= 3 */
    __u32     comp;                         /* Compression method used (see BMPImageCompression enum;
                                                                        v3: BMPIC_NONE, BMPIC_RLE8, BMPIC_RLE4;
                                                                        vnt, v4: adds BMPIC_BITFIELDS*/
    __u32     bm_size;                      /* Size of bitmap in bytes */
    __u32     horz_res;                     /* Horizontal resolution in pixels per meter */
    __u32     vert_res;                     /* Vertical resolution in pixels per meter */
    __u32     colors_used;                  /* Number of colors in the image (0 = max allowed) */
    __u32     colors_important;             /* Mininum number of important colors */

    /* Version >= NT */
    struct
    {
        __u32     r_mask;                   /* Mask identifying bits of red component */
        __u32     g_mask;                   /* Mask identifying bits of green component */
        __u32     b_mask;                   /* Mask identifying bits of blue component */
    } __attribute__((packed)) nt;

    /* Version >= 4 */
    __u32     a_mask;                       /* Mask identifying bits of alpha component */
    __u32     cs_type;                      /* Color space type (see BMPColorSpace enum) */
    __u32     r_x;                          /* X coordinate of red endpoint (BMPCS_CIE_XYZ only) */
    __u32     r_y;                          /* Y coordinate of red endpoint (BMPCS_CIE_XYZ only) */
    __u32     r_z;                          /* Z coordinate of red endpoint (BMPCS_CIE_XYZ only) */
    __u32     g_x;                          /* X coordinate of green endpoint (BMPCS_CIE_XYZ only) */
    __u32     g_y;                          /* Y coordinate of green endpoint (BMPCS_CIE_XYZ only) */
    __u32     g_z;                          /* Z coordinate of green endpoint (BMPCS_CIE_XYZ only) */
    __u32     b_x;                          /* X coordinate of blue endpoint (BMPCS_CIE_XYZ only) */
    __u32     b_y;                          /* Y coordinate of blue endpoint (BMPCS_CIE_XYZ only) */
    __u32     b_z;                          /* Z coordinate of blue endpoint (BMPCS_CIE_XYZ only) */
    __u32     gamma_red;                    /* Gamma red coordinate scale value */
    __u32     gamma_green;                  /* Gamma green coordinate scale value */
    __u32     gamma_blue;                   /* Gamma blue coordinate scale value */
} __attribute__((packed)) BMPBitmapHeader;

typedef struct
{
    /* Version >= 2 */
    __u8      b;
    __u8      g;
    __u8      r;

    /* Version >= 3 */
    __u8      reserved;
} __attribute__((packed)) BMPPaletteEntry;

typedef struct
{
    int                     ref;
    IDirectFBDataBuffer     *db;

    BMPFileHeader           file_hdr;
    BMPBitmapHeader         bm_hdr;

    BMPVersion              ver;
    DFBSurfacePixelFormat   bm_fmt;
    __u8                    a_bpp;
    __u8                    r_bpp;
    __u8                    g_bpp;
    __u8                    b_bpp;
    __u8                    a_shift;
    __u8                    r_shift;
    __u8                    g_shift;
    __u8                    b_shift;

    IDirectFBPalette        *pal;
    DFBColor                *colors;
    int                     entries;
    DFBSurfaceDescription   surf_desc;

    DIRenderCallback        render_cb;
    void                    *render_cb_ctx;
} IDirectFBImageProvider_BMP_data;



/*****************************************************************************/

static DFBResult bmp_decode_header(
    IDirectFBImageProvider_BMP_data *data
);
static DFBResult fetch_data(
    IDirectFBDataBuffer *db, 
    void *ptr, 
    int len
);
static DFBResult bmp_count_mask_bits(
    __u32 mask, 
    __u8 *bpp, 
    __u8 *shift
);
static DFBResult bmp_decode_bf_masks(
    IDirectFBImageProvider_BMP_data *data
);
static DFBResult bmp_decode(
    IDirectFBImageProvider_BMP_data *data, 
    IDirectFBSurface *surf
);
static void IDirectFBImageProvider_BMP_Destruct(
    IDirectFBImageProvider *thiz
);
static DFBResult IDirectFBImageProvider_BMP_AddRef(
    IDirectFBImageProvider *thiz
);
static DFBResult IDirectFBImageProvider_BMP_Release(
    IDirectFBImageProvider *thiz
);
static DFBResult IDirectFBImageProvider_BMP_RenderTo(
    IDirectFBImageProvider *thiz,
    IDirectFBSurface       *destination,
    const DFBRectangle     *dest_rect
);
static DFBResult IDirectFBImageProvider_BMP_SetRenderCallback(
    IDirectFBImageProvider *thiz,
    DIRenderCallback        callback,
    void                   *ctx
);
static DFBResult IDirectFBImageProvider_BMP_GetSurfaceDescription(
    IDirectFBImageProvider *thiz,
    DFBSurfaceDescription  *desc
);
static DFBResult Probe(
    IDirectFBImageProvider_ProbeContext *ctx
);
static DFBResult Construct(
    IDirectFBImageProvider *thiz,
    IDirectFBDataBuffer    *db
);


/*****************************************************************************/

DFBColor windows_palette_2[2] = 
{
    {0xff, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff}
};

DFBColor windows_palette_16[16] = 
{
    /*
     * 16 Standard HTML Colors
     */
    {0xff, 0x00, 0x00, 0x00},
    {0xff, 0xc0, 0xc0, 0xc0},
    {0xff, 0x80, 0x80, 0x80},
    {0xff, 0xff, 0xff, 0xff},
    {0xff, 0x80, 0x00, 0x00},
    {0xff, 0xff, 0x00, 0x00},
    {0xff, 0x80, 0x00, 0x80},
    {0xff, 0xff, 0x00, 0xff},
    {0xff, 0x00, 0x80, 0x00},
    {0xff, 0x00, 0xff, 0x00},
    {0xff, 0x80, 0x80, 0x00},
    {0xff, 0xff, 0xff, 0x00},
    {0xff, 0x00, 0x00, 0x80},
    {0xff, 0x00, 0x00, 0xff},
    {0xff, 0x00, 0x80, 0x80},
    {0xff, 0x00, 0xff, 0xff}
};

DFBColor windows_palette_256[256] = 
{
    /*
     * Default Windows 256 color palette. 
     *  
     * 0-9, 246-255:    20 system colors
     * 20-235:          safety palette 
     * 10-19, 236-245:  ??? 
     */
    {0xff, 0x80, 0x00, 0x00},
    {0xff, 0x00, 0x80, 0x00},
    {0xff, 0x80, 0x80, 0x00},
    {0xff, 0x00, 0x00, 0x80},
    {0xff, 0x80, 0x00, 0x80},
    {0xff, 0x00, 0x80, 0x80},
    {0xff, 0xc0, 0xc0, 0xc0},
    {0xff, 0xc0, 0xdc, 0xc0},
    {0xff, 0xa6, 0xca, 0xf0},
    {0xff, 0x00, 0x00, 0x33},
    {0xff, 0x33, 0x00, 0x00},
    {0xff, 0x33, 0x00, 0x33},
    {0xff, 0x00, 0x33, 0x33},
    {0xff, 0x16, 0x16, 0x16},
    {0xff, 0x1c, 0x1c, 0x1c},
    {0xff, 0x22, 0x22, 0x22},
    {0xff, 0x29, 0x29, 0x29},
    {0xff, 0x55, 0x55, 0x55},
    {0xff, 0x4d, 0x4d, 0x4d},
    {0xff, 0x42, 0x42, 0x42},
    {0xff, 0x39, 0x39, 0x39},
    {0xff, 0xff, 0x7c, 0x80},
    {0xff, 0xff, 0x50, 0x50},
    {0xff, 0xd6, 0x00, 0x93},
    {0xff, 0xcc, 0xec, 0xff},
    {0xff, 0xef, 0xd6, 0xc6},
    {0xff, 0xe7, 0xe7, 0xd6},
    {0xff, 0xad, 0xa9, 0x90},
    {0xff, 0x33, 0xff, 0x00},
    {0xff, 0x66, 0x00, 0x00},
    {0xff, 0x99, 0x00, 0x00},
    {0xff, 0xcc, 0x00, 0x00},
    {0xff, 0x00, 0x33, 0x00},
    {0xff, 0x33, 0x33, 0x00},
    {0xff, 0x66, 0x33, 0x00},
    {0xff, 0x99, 0x33, 0x00},
    {0xff, 0xcc, 0x33, 0x00},
    {0xff, 0xff, 0x33, 0x00},
    {0xff, 0x00, 0x66, 0x00},
    {0xff, 0x33, 0x66, 0x00},
    {0xff, 0x66, 0x66, 0x00},
    {0xff, 0x99, 0x66, 0x00},
    {0xff, 0xcc, 0x66, 0x00},
    {0xff, 0xff, 0x66, 0x00},
    {0xff, 0x00, 0x99, 0x00},
    {0xff, 0x33, 0x99, 0x00},
    {0xff, 0x66, 0x99, 0x00},
    {0xff, 0x99, 0x99, 0x00},
    {0xff, 0xcc, 0x99, 0x00},
    {0xff, 0xff, 0x99, 0x00},
    {0xff, 0x00, 0xcc, 0x00},
    {0xff, 0x33, 0xcc, 0x00},
    {0xff, 0x66, 0xcc, 0x00},
    {0xff, 0x99, 0xcc, 0x00},
    {0xff, 0xcc, 0xcc, 0x00},
    {0xff, 0xff, 0xcc, 0x00},
    {0xff, 0x66, 0xff, 0x00},
    {0xff, 0x99, 0xff, 0x00},
    {0xff, 0xcc, 0xff, 0x00},
    {0xff, 0x00, 0xff, 0x33},
    {0xff, 0x33, 0x00, 0xff},
    {0xff, 0x66, 0x00, 0x33},
    {0xff, 0x99, 0x00, 0x33},
    {0xff, 0xcc, 0x00, 0x33},
    {0xff, 0xff, 0x00, 0x33},
    {0xff, 0x00, 0x33, 0xff},
    {0xff, 0x33, 0x33, 0x33},
    {0xff, 0x66, 0x33, 0x33},
    {0xff, 0x99, 0x33, 0x33},
    {0xff, 0xcc, 0x33, 0x33},
    {0xff, 0xff, 0x33, 0x33},
    {0xff, 0x00, 0x66, 0x33},
    {0xff, 0x33, 0x66, 0x33},
    {0xff, 0x66, 0x66, 0x33},
    {0xff, 0x99, 0x66, 0x33},
    {0xff, 0xcc, 0x66, 0x33},
    {0xff, 0xff, 0x66, 0x33},
    {0xff, 0x00, 0x99, 0x33},
    {0xff, 0x33, 0x99, 0x33},
    {0xff, 0x66, 0x99, 0x33},
    {0xff, 0x99, 0x99, 0x33},
    {0xff, 0xcc, 0x99, 0x33},
    {0xff, 0xff, 0x99, 0x33},
    {0xff, 0x00, 0xcc, 0x33},
    {0xff, 0x33, 0xcc, 0x33},
    {0xff, 0x66, 0xcc, 0x33},
    {0xff, 0x99, 0xcc, 0x33},
    {0xff, 0xcc, 0xcc, 0x33},
    {0xff, 0xff, 0xcc, 0x33},
    {0xff, 0x33, 0xff, 0x33},
    {0xff, 0x66, 0xff, 0x33},
    {0xff, 0x99, 0xff, 0x33},
    {0xff, 0xcc, 0xff, 0x33},
    {0xff, 0xff, 0xff, 0x33},
    {0xff, 0x00, 0x00, 0x66},
    {0xff, 0x33, 0x00, 0x66},
    {0xff, 0x66, 0x00, 0x66},
    {0xff, 0x99, 0x00, 0x66},
    {0xff, 0xcc, 0x00, 0x66},
    {0xff, 0xff, 0x00, 0x66},
    {0xff, 0x00, 0x33, 0x66},
    {0xff, 0x33, 0x33, 0x66},
    {0xff, 0x66, 0x33, 0x66},
    {0xff, 0x99, 0x33, 0x66},
    {0xff, 0xcc, 0x33, 0x66},
    {0xff, 0xff, 0x33, 0x66},
    {0xff, 0x00, 0x66, 0x66},
    {0xff, 0x33, 0x66, 0x66},
    {0xff, 0x66, 0x66, 0x66},
    {0xff, 0x99, 0x66, 0x66},
    {0xff, 0xcc, 0x66, 0x66},
    {0xff, 0x00, 0x99, 0x66},
    {0xff, 0x33, 0x99, 0x66},
    {0xff, 0x66, 0x99, 0x66},
    {0xff, 0x99, 0x99, 0x66},
    {0xff, 0xcc, 0x99, 0x66},
    {0xff, 0xff, 0x99, 0x66},
    {0xff, 0x00, 0xcc, 0x66},
    {0xff, 0x33, 0xcc, 0x66},
    {0xff, 0x99, 0xcc, 0x66},
    {0xff, 0xcc, 0xcc, 0x66},
    {0xff, 0xff, 0xcc, 0x66},
    {0xff, 0x00, 0xff, 0x66},
    {0xff, 0x33, 0xff, 0x66},
    {0xff, 0x99, 0xff, 0x66},
    {0xff, 0xcc, 0xff, 0x66},
    {0xff, 0xff, 0x00, 0xcc},
    {0xff, 0xcc, 0x00, 0xff},
    {0xff, 0x00, 0x99, 0x99},
    {0xff, 0x99, 0x33, 0x99},
    {0xff, 0x99, 0x00, 0x99},
    {0xff, 0xcc, 0x00, 0x99},
    {0xff, 0x00, 0x00, 0x99},
    {0xff, 0x33, 0x33, 0x99},
    {0xff, 0x66, 0x00, 0x99},
    {0xff, 0xcc, 0x33, 0x99},
    {0xff, 0xff, 0x00, 0x99},
    {0xff, 0x00, 0x66, 0x99},
    {0xff, 0x33, 0x66, 0x99},
    {0xff, 0x66, 0x33, 0x99},
    {0xff, 0x99, 0x66, 0x99},
    {0xff, 0xcc, 0x66, 0x99},
    {0xff, 0xff, 0x33, 0x99},
    {0xff, 0x33, 0x99, 0x99},
    {0xff, 0x66, 0x99, 0x99},
    {0xff, 0x99, 0x99, 0x99},
    {0xff, 0xcc, 0x99, 0x99},
    {0xff, 0xff, 0x99, 0x99},
    {0xff, 0x00, 0xcc, 0x99},
    {0xff, 0x33, 0xcc, 0x99},
    {0xff, 0x66, 0xcc, 0x66},
    {0xff, 0x99, 0xcc, 0x99},
    {0xff, 0xcc, 0xcc, 0x99},
    {0xff, 0xff, 0xcc, 0x99},
    {0xff, 0x00, 0xff, 0x99},
    {0xff, 0x33, 0xff, 0x99},
    {0xff, 0x66, 0xcc, 0x99},
    {0xff, 0x99, 0xff, 0x99},
    {0xff, 0xcc, 0xff, 0x99},
    {0xff, 0xff, 0xff, 0x99},
    {0xff, 0x00, 0x00, 0xcc},
    {0xff, 0x33, 0x00, 0x99},
    {0xff, 0x66, 0x00, 0xcc},
    {0xff, 0x99, 0x00, 0xcc},
    {0xff, 0xcc, 0x00, 0xcc},
    {0xff, 0x00, 0x33, 0x99},
    {0xff, 0x33, 0x33, 0xcc},
    {0xff, 0x66, 0x33, 0xcc},
    {0xff, 0x99, 0x33, 0xcc},
    {0xff, 0xcc, 0x33, 0xcc},
    {0xff, 0xff, 0x33, 0xcc},
    {0xff, 0x00, 0x66, 0xcc},
    {0xff, 0x33, 0x66, 0xcc},
    {0xff, 0x66, 0x66, 0x99},
    {0xff, 0x99, 0x66, 0xcc},
    {0xff, 0xcc, 0x66, 0xcc},
    {0xff, 0xff, 0x66, 0x99},
    {0xff, 0x00, 0x99, 0xcc},
    {0xff, 0x33, 0x99, 0xcc},
    {0xff, 0x66, 0x99, 0xcc},
    {0xff, 0x99, 0x99, 0xcc},
    {0xff, 0xcc, 0x99, 0xcc},
    {0xff, 0xff, 0x99, 0xcc},
    {0xff, 0x00, 0xcc, 0xcc},
    {0xff, 0x33, 0xcc, 0xcc},
    {0xff, 0x66, 0xcc, 0xcc},
    {0xff, 0x99, 0xcc, 0xcc},
    {0xff, 0xcc, 0xcc, 0xcc},
    {0xff, 0xff, 0xcc, 0xcc},
    {0xff, 0x00, 0xff, 0xcc},
    {0xff, 0x33, 0xff, 0xcc},
    {0xff, 0x66, 0xff, 0x99},
    {0xff, 0x99, 0xff, 0xcc},
    {0xff, 0xcc, 0xff, 0xcc},
    {0xff, 0xff, 0xff, 0xcc},
    {0xff, 0x33, 0x00, 0xcc},
    {0xff, 0x66, 0x00, 0xff},
    {0xff, 0x99, 0x00, 0xff},
    {0xff, 0x00, 0x33, 0xcc},
    {0xff, 0x33, 0x33, 0xff},
    {0xff, 0x66, 0x33, 0xff},
    {0xff, 0x99, 0x33, 0xff},
    {0xff, 0xcc, 0x33, 0xff},
    {0xff, 0xff, 0x33, 0xff},
    {0xff, 0x00, 0x66, 0xff},
    {0xff, 0x33, 0x66, 0xff},
    {0xff, 0x66, 0x66, 0xcc},
    {0xff, 0x99, 0x66, 0xff},
    {0xff, 0xcc, 0x66, 0xff},
    {0xff, 0xff, 0x66, 0xcc},
    {0xff, 0x00, 0x99, 0xff},
    {0xff, 0x33, 0x99, 0xff},
    {0xff, 0x66, 0x99, 0xff},
    {0xff, 0x99, 0x99, 0xff},
    {0xff, 0xcc, 0x99, 0xff},
    {0xff, 0xff, 0x99, 0xff},
    {0xff, 0x00, 0xcc, 0xff},
    {0xff, 0x33, 0xcc, 0xff},
    {0xff, 0x66, 0xcc, 0xff},
    {0xff, 0x99, 0xcc, 0xff},
    {0xff, 0xcc, 0xcc, 0xff},
    {0xff, 0xff, 0xcc, 0xff},
    {0xff, 0x33, 0xff, 0xff},
    {0xff, 0x66, 0xff, 0xcc},
    {0xff, 0x99, 0xff, 0xff},
    {0xff, 0xcc, 0xff, 0xff},
    {0xff, 0xff, 0x66, 0x66},
    {0xff, 0x66, 0xff, 0x66},
    {0xff, 0xff, 0xff, 0x66},
    {0xff, 0x66, 0x66, 0xff},
    {0xff, 0xff, 0x66, 0xff},
    {0xff, 0x66, 0xff, 0xff},
    {0xff, 0xa5, 0x00, 0x21},
    {0xff, 0x5f, 0x5f, 0x5f},
    {0xff, 0x77, 0x77, 0x77},
    {0xff, 0x86, 0x86, 0x86},
    {0xff, 0x96, 0x96, 0x96},
    {0xff, 0xcb, 0xcb, 0xcb},
    {0xff, 0xb2, 0xb2, 0xb2},
    {0xff, 0xd7, 0xd7, 0xd7},
    {0xff, 0xdd, 0xdd, 0xdd},
    {0xff, 0xe3, 0xe3, 0xe3},
    {0xff, 0xea, 0xea, 0xea},
    {0xff, 0xf1, 0xf1, 0xf1},
    {0xff, 0xf8, 0xf8, 0xf8},
    {0xff, 0xff, 0xfb, 0xf0},
    {0xff, 0xa0, 0xa0, 0xa4},
    {0xff, 0x80, 0x80, 0x80},
    {0xff, 0xff, 0x00, 0x00},
    {0xff, 0x00, 0xff, 0x00},
    {0xff, 0xff, 0xff, 0x00},
    {0xff, 0x00, 0x00, 0xff},
    {0xff, 0xff, 0x00, 0xff},
    {0xff, 0x00, 0xff, 0xff},
    {0xff, 0xff, 0xff, 0xff}
};

/*****************************************************************************/

/*
 * This function is VERY slow. Try to limit fetches to a minimum 
 * of a full line. 
 */
static DFBResult
fetch_data(IDirectFBDataBuffer *db, void *ptr, int len)
{
    DFBResult ret;
    unsigned int read;

    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    while (len > 0)
    {
        ret = db->WaitForData(db, len);
        if (ret == DFB_OK)
        {
            ret = db->GetData(db, len, ptr, &read);
        }
        if (ret) 
        {
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }

        ptr += read;
        len -= read;
    }

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
bmp_decode_header(IDirectFBImageProvider_BMP_data *data)
{
    DFBResult               ret;
    __u8                    buf[sizeof(BMPFileHeader) + sizeof(BMPBitmapHeader)];
    BMPFileHeader           *file_hdr;
    BMPBitmapHeader         *bm_hdr;
    DFBSurfaceDescription   surf_desc;
    DFBPaletteDescription   pal_desc;
    int                     bpp;
    __u8                    *bmp_pal;
    int                     i;
    int                     entry_size;
    int                     bmp_entries;
    int                     pal_size;
    int                     bmp_pal_size;
    DFBColor                *pal;
    __u8                    *b;
    __u32                   offset;


    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    file_hdr =  &data->file_hdr;
    bm_hdr =    &data->bm_hdr;

    memset(buf, 0, sizeof(buf));

    /*
     * Determine file ver & read file header accordingly.
     */
    CHECK_FETCH(fetch_data(data->db, &file_hdr->type, sizeof(file_hdr->type)));

    data->ver = BMPVER_INVALID;
    if (file_hdr->type == 0)
    {
        data->ver = BMPVER_1;
        CHECK_FETCH(fetch_data(data->db, &file_hdr->v.ddb, sizeof(file_hdr->v.ddb)));
    }
    else if (file_hdr->type != 0x4d42)
    {
        BMP_ERR("IDirectFBImageProvider_BMP: "
                "Invalid magic (0x%04x)!\n", file_hdr->type);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return DFB_UNSUPPORTED;
    }

    /*
     * Read bitmap header & determine version.
     */
    data->ver = BMPVER_2;
    CHECK_FETCH(fetch_data(data->db, &file_hdr->v.bmp, sizeof(file_hdr->v.bmp)));
    CHECK_FETCH(fetch_data(data->db, &bm_hdr->size, sizeof(bm_hdr->size)));
    if (bm_hdr->size > BM_HDR_SIZE_V2)
    {
        data->ver = BMPVER_3;
    }
    CHECK_FETCH(fetch_data(data->db, &bm_hdr->width, BMP_SIZEOF_WIDTH));
    CHECK_FETCH(fetch_data(data->db, &bm_hdr->height, BMP_SIZEOF_HEIGHT));
    CHECK_FETCH(fetch_data(data->db, &bm_hdr->planes, 
        bm_hdr->size - (sizeof(bm_hdr->size) + BMP_SIZEOF_WIDTH + BMP_SIZEOF_HEIGHT)));

    if (bm_hdr->size == BM_HDR_SIZE_V3 &&
         bm_hdr->comp == BMPIC_BITFIELDS)
    {
        data->ver = BMPVER_NT;
        bm_hdr->size += sizeof(bm_hdr->nt);
        CHECK_FETCH(fetch_data(data->db, &bm_hdr->nt, sizeof(bm_hdr->nt)));
    }
    else 
    {
        if (bm_hdr->size > BM_HDR_SIZE_V3)
        {
            data->ver = BMPVER_NT;
            if (bm_hdr->size > BM_HDR_SIZE_NT)
            {    
                data->ver = BMPVER_4;
                if (bm_hdr->size > BM_HDR_SIZE_V4)
                {
                    data->ver = BMPVER_5;
                }
            }
        }
    }
    BMP_MSG("BMP Version: %d\n", data->ver);
    BMP_MSG("\nFile Header:\n");
    if (data->ver == BMPVER_1)
    {
        BMP_MSG("type:          0x%04x\n", file_hdr->type);
        BMP_MSG("width:         0x%04x\n", file_hdr->v.ddb.width);
        BMP_MSG("height:        0x%04x\n", file_hdr->v.ddb.height);
        BMP_MSG("byte_width:    0x%02x\n", file_hdr->v.ddb.byte_width);
        BMP_MSG("bpp:           0x%02x\n", file_hdr->v.ddb.bpp);
    }
    else
    {
        BMP_MSG("type:          0x%04x\n", file_hdr->type);
        BMP_MSG("size:          0x%08x\n", file_hdr->v.bmp.size);
        BMP_MSG("reserved1:     0x%04x\n", file_hdr->v.bmp.reserved1);
        BMP_MSG("reserved2:     0x%04x\n", file_hdr->v.bmp.reserved2);
        BMP_MSG("offset:        0x%08x\n", file_hdr->v.bmp.offset);

        BMP_MSG("\nBitmapInfo Header:\n");
        BMP_MSG("size:              0x%08x\n", bm_hdr->size);
        BMP_MSG("width:             0x%08x\n", bm_hdr->width);
        BMP_MSG("height:            0x%08x\n", bm_hdr->height);
        BMP_MSG("planes:            0x%04x\n", bm_hdr->planes);
        BMP_MSG("bpp:               0x%04x\n", bm_hdr->bpp);
        if (data->ver >= BMPVER_3)
        {
            BMP_MSG("comp:              0x%08x\n", bm_hdr->comp);
            BMP_MSG("bm_size:           0x%08x\n", bm_hdr->bm_size);
            BMP_MSG("horz_res:          0x%08x\n", bm_hdr->horz_res);
            BMP_MSG("vert_res:          0x%08x\n", bm_hdr->vert_res);
            BMP_MSG("colors_used:       0x%08x\n", bm_hdr->colors_used);
            BMP_MSG("colors_important:  0x%08x\n", bm_hdr->colors_important);
            if (data->ver >= BMPVER_NT)
            {
                BMP_MSG("r_mask:            0x%08x\n", bm_hdr->nt.r_mask);
                BMP_MSG("g_mask:            0x%08x\n", bm_hdr->nt.g_mask);
                BMP_MSG("b_mask:            0x%08x\n", bm_hdr->nt.b_mask);
                if (data->ver >= BMPVER_4)
                {
                    BMP_MSG("a_mask:            0x%08x\n", bm_hdr->a_mask);
                    BMP_MSG("cs_type:           0x%08x\n", bm_hdr->cs_type);
                    BMP_MSG("r_x:               0x%08x\n", bm_hdr->r_x);
                    BMP_MSG("r_y:               0x%08x\n", bm_hdr->r_y);
                    BMP_MSG("r_z:               0x%08x\n", bm_hdr->r_z);
                    BMP_MSG("g_x:               0x%08x\n", bm_hdr->g_x);
                    BMP_MSG("g_y:               0x%08x\n", bm_hdr->g_y);
                    BMP_MSG("g_z:               0x%08x\n", bm_hdr->g_z);
                    BMP_MSG("b_x:               0x%08x\n", bm_hdr->b_x);
                    BMP_MSG("b_y:               0x%08x\n", bm_hdr->b_y);
                    BMP_MSG("b_z:               0x%08x\n", bm_hdr->b_z);
                    BMP_MSG("gamma_red:         0x%08x\n", bm_hdr->gamma_red);
                    BMP_MSG("gamma_green:       0x%08x\n", bm_hdr->gamma_green);
                    BMP_MSG("gamma_blue:        0x%08x\n", bm_hdr->gamma_blue);
                }
            }
        }
    }
    BMP_MSG("\n");

    /*
     * Determine bitmap format;
     */
    BMP_MSG("\nBitmap Format:     ");
    if (data->ver == BMPVER_1)
    {
        bpp = data->file_hdr.v.ddb.bpp;
        if (bpp > 8)
        {
            BMP_ERR("%s: Erroneous BPP value in BMP ver %d file. BPP = %d\n", 
                    __FUNCTION__, data->ver, bpp);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return DFB_FAILURE;
        }
    }
    else
    {
        bpp = data->bm_hdr.bpp;
        if ((bpp == 16 || bpp == 32) &&
            data->ver < BMPVER_3)
        {
            BMP_ERR("%s: Erroneous BPP value in BMP ver %d file. BPP = %d\n", 
                    __FUNCTION__, data->ver, bpp);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return DFB_FAILURE;
        }
    }
    switch (bpp)
    {
        case 1:
            data->bm_fmt = DSPF_LUT1;
            BMP_MSG("DSPF_LUT1\n");
            break;

        case 4:
            data->bm_fmt = DSPF_LUT4;
            BMP_MSG("DSPF_LUT4\n");
            break;

        case 8:
            data->bm_fmt = DSPF_LUT8;
            BMP_MSG("DSPF_LUT8\n");
            break;

        case 16:
            if (data->bm_hdr.comp == BMPIC_BITFIELDS)
            {
                ret = bmp_decode_bf_masks(data);
                if (ret != DFB_OK)
                {
                    BMP_TRACE("%s: Leave\n", __FUNCTION__);
                    return ret;
                }
            }
            else
            {
                data->bm_fmt = DSPF_ARGB1555;
                BMP_MSG("DSPF_ARGB1555\n");
            }
            break;

        case 24:
            data->bm_fmt = DSPF_RGB24;
            BMP_MSG("DSPF_RGB24\n");
            break;

        case 32:
            if (data->bm_hdr.comp == BMPIC_BITFIELDS)
            {
                ret = bmp_decode_bf_masks(data);
                if (ret != DFB_OK)
                {
                    BMP_TRACE("%s: Leave\n", __FUNCTION__);
                    return ret;
                }
            }
            else
            {
                data->bm_fmt = DSPF_RGB32;
                BMP_MSG("DSPF_RGB32\n");
            }
            break;

        default:
            BMP_ERR("%s: Erroneous BPP value in BMP file. BPP = %d\n", 
                __FUNCTION__, bpp);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return DFB_FAILURE;
    }

    /* 
     * Get palette if there is one.
     */
    if (bpp < 16)
    {
        BMP_MSG("\nPalette:\n");

        data->entries = (1 << bpp);
        BMP_MSG("entries:       %d\n", data->entries);
        data->colors = D_MALLOC(data->entries * sizeof(DFBColor));
        if (data->colors == NULL)
        {
            BMP_ERR("%s: Error allocating temp buffer for colors.\n", __FUNCTION__);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return D_OOM();
        }

        pal_desc.flags      = DPDESC_SIZE;
        pal_desc.caps       = DPCAPS_NONE;
        pal_desc.size       = data->entries;
        pal_desc.entries    = NULL;
        ret = idirectfb_singleton->CreatePalette(idirectfb_singleton, &pal_desc, &data->pal);
        if (ret != DFB_OK)
        {
            BMP_ERR("%s: Error creating palette.\n", __FUNCTION__);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }

        if (data->ver == BMPVER_1)
        {
            /*
             * Use Windows standard color palette.
             */
            BMP_MSG("using Windows palette\n");
            pal = bpp == 8 ? windows_palette_256 : bpp == 4 ? windows_palette_16 : windows_palette_2;
            ret = data->pal->SetEntries(data->pal, pal, data->entries, 0);
            if (ret != DFB_OK)
            {
                BMP_ERR("%s: Error setting palette entries.\n", __FUNCTION__);
                D_FREE(data->colors);
                BMP_TRACE("%s: Leave\n", __FUNCTION__);
                return ret;
            }
        }
        else 
        {
            entry_size = data->ver == BMPVER_2 ? 3 : 4;
            BMP_MSG("entry_size     %d\n", entry_size);
            pal_size = data->entries * entry_size;
            BMP_MSG("pal_size:      %d\n", pal_size);
    
            if (data->ver == BMPVER_2)
            {
                ret = data->db->GetPosition(data->db, &offset);
                if (ret != DFB_OK)
                {
                    BMP_ERR("%s: Error getting file position.\n", __FUNCTION__);
                    D_FREE(data->colors);
                    BMP_TRACE("%s: Leave\n", __FUNCTION__);
                    return ret;
                }
                bmp_entries = (data->file_hdr.v.bmp.offset - offset) / entry_size;
            }
            else
            {
                bmp_entries = data->bm_hdr.colors_used != 0 ? 
                    data->bm_hdr.colors_used : (1 << bpp);
            }
            bmp_pal_size = bmp_entries * entry_size;
            bmp_pal = D_MALLOC(bmp_pal_size);
            if (bmp_pal == NULL)
            {
                BMP_ERR("%s: Error allocating buffer for reading bmp palette.\n", __FUNCTION__);
                D_FREE(data->colors);
                BMP_TRACE("%s: Leave\n", __FUNCTION__);
                return D_OOM();
            }
            ret = fetch_data(data->db, bmp_pal, bmp_pal_size);
            if (ret != DFB_OK)
            {
                BMP_ERR("%s: Error reading from file.\n", __FUNCTION__);
                D_FREE(data->colors);
                D_FREE(bmp_pal);
                BMP_TRACE("%s: Leave\n", __FUNCTION__);
                return ret;
            }
    
            for (i = 0; i < data->entries; i++)
            {
                if (i < bmp_entries)
                {
                    data->colors[i].a = 0xff;
                    data->colors[i].r = bmp_pal[i * entry_size + 2];
                    data->colors[i].g = bmp_pal[i * entry_size + 1];
                    data->colors[i].b = bmp_pal[i * entry_size + 0];
                }
                else
                {
                    data->colors[i].a = 0;
                    data->colors[i].r = 0;
                    data->colors[i].g = 0;
                    data->colors[i].b = 0;
                }
#if SHOW_PALETTE
                if (i == 0 || i == data->entries - 1)
                {
                    BMP_MSG("entry[%03d]:   {0x%02x, 0x%02x, 0x%02x, 0x%02x}\n", 
                        i, data->colors[i].a, data->colors[i].r, data->colors[i].g, 
                        data->colors[i].b);
                }
#endif
            }
    
            ret = data->pal->SetEntries(data->pal, data->colors, data->entries, 0);
            if (ret != DFB_OK)
            {
                BMP_ERR("%s: Error setting palette entries.\n", __FUNCTION__);
                D_FREE(data->colors);
                D_FREE(bmp_pal);
                BMP_TRACE("%s: Leave\n", __FUNCTION__);
                return ret;
            }
    
            D_FREE(bmp_pal);
        }
    }

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
bmp_count_mask_bits(__u32 mask, __u8 *bpp, __u8 *shift)
{
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    for (*shift = 0; !(mask & 1) && (*shift < 32); (*shift)++, mask >>= 1);
    if (*shift >= 32)
    {
        *shift = 0;
        *bpp = 0;
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return DFB_OK;
    }
    for (*bpp = 0; (mask & 1) && (*bpp < 32); (*bpp)++, mask >>= 1);

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
bmp_decode_bf_masks(IDirectFBImageProvider_BMP_data *data)
{
    BMPBitmapHeader *bm_hdr;

    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    bm_hdr = &data->bm_hdr;

    data->a_bpp = 0;
    data->a_shift = 0;
    if (data->ver > BMPVER_NT)
    {
        bmp_count_mask_bits(bm_hdr->a_mask, &data->a_bpp, &data->a_shift);
    }
    bmp_count_mask_bits(bm_hdr->nt.r_mask, &data->r_bpp, &data->r_shift);
    bmp_count_mask_bits(bm_hdr->nt.g_mask, &data->g_bpp, &data->g_shift);
    bmp_count_mask_bits(bm_hdr->nt.b_mask, &data->b_bpp, &data->b_shift);
    BMP_MSG("{%d, %d, %d, %d} ", data->a_bpp, data->r_bpp, data->g_bpp, data->b_bpp);

    data->bm_fmt = DSPF_UNKNOWN;
    switch (bm_hdr->bpp)
    {
        case 16:
            switch (data->a_bpp)
            {
                case 0:
                    if (data->r_bpp == 5 && data->g_bpp == 5 && data->b_bpp == 5)
                    {
                        data->bm_fmt = DSPF_ARGB1555;
                        BMP_MSG("DSPF_ARGB1555\n");
                    }
                    else if (data->r_bpp == 5 && data->g_bpp == 6 && data->b_bpp == 5)
                    {
                        data->bm_fmt = DSPF_RGB16;
                        BMP_MSG("DSPF_RGB16\n");
                    }
                    else if (data->r_bpp == 4 && data->g_bpp == 4 && data->b_bpp == 4)
                    {
                        data->bm_fmt = DSPF_ARGB4444;
                        BMP_MSG("DSPF_ARGB4444\n");
                    }
                    break;

                case 1:
                    if (data->r_bpp == 5 && data->g_bpp == 5 && data->b_bpp == 5)
                    {
                        data->bm_fmt = DSPF_ARGB1555;
                        BMP_MSG("DSPF_ARGB1555\n");
                    }
                    break;

                case 4:
                    if (data->r_bpp == 4 && data->g_bpp == 4 && data->b_bpp == 4)
                    {
                        data->bm_fmt = DSPF_ARGB4444;
                        BMP_MSG("DSPF_ARGB4444\n");
                    }
                    break;

                default:
                    BMP_MSG("Unsupported alpha BPP (%d)\n", data->a_bpp);
                    break;
            }
            break;

        case 32:
            switch (data->a_bpp)
            {
                case 0:
                    if (data->r_bpp == 8 && data->g_bpp == 8 && data->b_bpp == 8)
                    {
                        data->bm_fmt = DSPF_RGB32;
                        BMP_MSG("DSPF_RGB32\n");
                    }
                    break;

                case 8:
                    if (data->r_bpp == 8 && data->g_bpp == 8 && data->b_bpp == 8)
                    {
                        data->bm_fmt = DSPF_ARGB;
                        BMP_MSG("DSPF_ARGB\n");
                    }
                    break;

                default:
                    BMP_MSG("Unsupported alpha BPP (%d)\n", data->a_bpp);
                    break;
            }
            break;

        default:
            BMP_ERR("%s: Erroneous compression value (%d) BPP (%d) combination in file.\n", 
                    __FUNCTION__, bm_hdr->comp, bm_hdr->bpp);
            break;
    }

    if (data->bm_fmt == DSPF_UNKNOWN)
    {
        BMP_MSG("DSPF_UNKNOWN\n");
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return DFB_UNSUPPORTED;
    }
    else
    {
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return DFB_OK;
    }
}

static DFBResult
bmp_decode(IDirectFBImageProvider_BMP_data *data, IDirectFBSurface *surf)
{
    DFBResult   ret;
    void        *dst;
    __u32       *dst32;
    __u16       *dst16;
    __u8        *dst8;
    __u32       *src32;
    __u16       *src16;
    int         width, height, bpp, width_bytes;
    int         first_line, last_line, dy;
    int         src_pitch, dst_pitch;
    int         idx;
    DFBColor    c;
    int         x, y;
    __u8        start_code, marker, count, pix_value, x_offset, y_offset;
    __u8        tmp[32];
    int         i;
    __u8        mask;

    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    width       = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.width : data->bm_hdr.width;
    height      = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.height : data->bm_hdr.height;
    bpp         = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.bpp : data->bm_hdr.bpp;
    width_bytes = bpp < 8 ? (((width - 1) * bpp) >> 3) + 1 : (width * bpp) >> 3;

    ret = surf->Lock(surf, DSLF_WRITE, &dst, &dst_pitch);
    if (ret != DFB_OK)
    {
        BMP_ERR("%s: Error locking destination surface.\n", __FUNCTION__);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }

    if (data->ver >= BMPVER_2)
    {
        ret = data->db->SeekTo(data->db, data->file_hdr.v.bmp.offset);
        if (ret != DFB_OK)
        {
            BMP_ERR("%s: Error seeking to file position.\n", __FUNCTION__);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }
    }

    if (height > 0)
    {
        first_line  = height - 1;
        last_line   = -1;
        dy          = -1;
    }
    else
    {
        first_line  = 0;
        last_line   = height * -1;
        dy          = 1;
    }
    dst32 = (__u32 *)dst;
    dst16 = (__u16 *)dst;
    dst8 =  (__u8  *)dst;
    switch (bpp)
    {
        case 1:
        case 4:
        case 8:
            if (data->ver >= BMPVER_3 &&
                (data->bm_hdr.comp == BMPIC_RLE4 || data->bm_hdr.comp == BMPIC_RLE8))
            {
                x = 0;
                y = first_line; 
                while (y != last_line)
                {
                    CHECK_FETCH(fetch_data(data->db, &start_code, 1));

                    switch (start_code)
                    {
                        case 00:    /* marker */
                            CHECK_FETCH(fetch_data(data->db, &marker, 1));
                            switch (marker)
                            {
                                case 00:    /* end of scan line */
                                    BMP_DEC_MSG("EOL ");
                                    if (x != 0)
                                    {
                                        x = 0;
                                        y += dy;
                                    }
                                    break;

                                case 01:    /* end of bitmap data */
                                    BMP_DEC_MSG("EOD ");
                                    y = last_line;
                                    break;

                                case 02:    /* forward x, y offset */
                                    BMP_DEC_MSG("SKP ");
                                    CHECK_FETCH(fetch_data(data->db, &x_offset, 1));
                                    CHECK_FETCH(fetch_data(data->db, &y_offset, 1));
                                    BMP_DEC_MSG("%03d %03d", x_offset, y_offset);
                                    x += x_offset;
                                    y += (dy * y_offset) + (dy * (x / width));
                                    x %= width;
                                    break;

                                default:    /* uncompressed pixel values */
                                    BMP_DEC_MSG("RAW ");
                                    count = marker;
                                    BMP_DEC_MSG("%03d ", count);
                                    for (i = 0; i < count; i++, x++)
                                    {
                                        if (x >= width)
                                        {
                                            x = 0;
                                            y += dy;
                                            break;
                                        }
                                        if (data->bm_hdr.comp != BMPIC_RLE4 || !(i & 1))
                                        {
                                            CHECK_FETCH(fetch_data(data->db, &pix_value, 1));
                                        }
                                        if (data->bm_hdr.comp == BMPIC_RLE4)
                                        {
                                            tmp[0] = pix_value;
                                            mask = x & 1 ? 0x0f : 0xf0;
                                            if ((i & 1) != (x & 1))
                                            {
                                                if (x & 1)
                                                {
                                                    tmp[0] >>= 4;
                                                }
                                                else
                                                {
                                                    tmp[0] <<= 4;
                                                }
                                            }
                                            tmp[0] &= mask;
                                            dst8[y * dst_pitch + (x >> 1)] &= ~mask;
                                            dst8[y * dst_pitch + (x >> 1)] |= tmp[0];
                                            BMP_DEC_MSG("%1x ", 
                                                (dst8[y * dst_pitch + (x >> 1)] & mask) >> (x & 1 ? 0 : 4));
                                        }
                                        else
                                        {
                                            dst8[y * dst_pitch + x] = pix_value;
                                            BMP_DEC_MSG("%02x ", dst8[y * dst_pitch + x]);
                                        }
                                    }
                                    if (data->bm_hdr.comp == BMPIC_RLE4)
                                    {
                                        if (!(((count - 1) >> 1) & 1))
                                        {
                                            CHECK_FETCH(fetch_data(data->db, &pix_value, 1));
                                        }
                                    } else
                                    {
                                        if (count & 1)
                                        {
                                            CHECK_FETCH(fetch_data(data->db, &pix_value, 1));
                                        }
                                    }
                                    break;
                            }
                            break;

                        default:    /* encoded run */
                            BMP_DEC_MSG("ENC ");
                            count = start_code;
                            BMP_DEC_MSG("%03d ", count);
                            CHECK_FETCH(fetch_data(data->db, &pix_value, 1));
                            BMP_DEC_MSG("%02x ", pix_value);
                            for (i = 0; i < count; i++, x++)
                            {
                                if (x >= width)
                                {
                                    x = 0;
                                    y += dy;
                                    break;
                                }
                                if (data->bm_hdr.comp == BMPIC_RLE4)
                                {
                                    tmp[0] = pix_value;
                                    mask = x & 1 ? 0x0f : 0xf0;
                                    if ((i & 1) != (x & 1))
                                    {
                                        if (x & 1)
                                        {
                                            tmp[0] >>= 4;
                                        }
                                        else
                                        {
                                            tmp[0] <<= 4;
                                        }
                                    }
                                    tmp[0] &= mask;
                                    dst8[y * dst_pitch + (x >> 1)] &= ~mask;
                                    dst8[y * dst_pitch + (x >> 1)] |= tmp[0];
                                }
                                else
                                {
                                    dst8[y * dst_pitch + x] = pix_value;
                                }
                            }
                            break;
                    }
                    BMP_DEC_MSG("\n");
                }
            }
            else
            {
                if (data->ver == BMPVER_1)
                {
                    src_pitch = (width_bytes + 1) & ~1;
                }
                else
                {
                    src_pitch = (width_bytes + 3) & ~3;
                }
    
                /*
                 * Both BMP files & DFB both use little endian bit ordering (MSB is leftmost pixel), 
                 * so we can just copy each BMP line directly into the DFB surface buffer. 
                 */
#if SHOW_BITMAP_DATA
                BMP_MSG("\nBitmap Data: \n");
#endif
                for (y = first_line; y != last_line; y += dy)
                {
                    CHECK_FETCH(fetch_data(data->db, &dst8[y * dst_pitch], width_bytes));
                    if (src_pitch > width_bytes)
                    {
                        CHECK_FETCH(fetch_data(data->db, tmp, src_pitch - width_bytes));
                    }
#if SHOW_BITMAP_DATA
                    if (y == first_line || y == last_line - dy)
                    {
                        BMP_MSG("\nLine 0x%04x:\n", y);
                        for (x = 0; x < width_bytes; x++)
                        {
                            if (x % 16 == 0)
                            {
                                BMP_MSG("\n");
                            }
                            BMP_MSG("%02x ", dst8[y * dst_pitch + x]);
                        }
                        BMP_MSG("\n");
                    }
#endif
                }
            }
            break;

        case 24:
            src_pitch = (width_bytes + 3) & ~3;
#if SHOW_BITMAP_DATA
            BMP_MSG("\nBitmap Data: \n");
#endif
            for (y = first_line; y != last_line; y += dy)
            {
                CHECK_FETCH(fetch_data(data->db, &dst8[y * dst_pitch], width_bytes));
                if (src_pitch > width_bytes)
                {
                    CHECK_FETCH(fetch_data(data->db, tmp, src_pitch - width_bytes));
                }
#if SHOW_BITMAP_DATA
                if (y == first_line || y == last_line - dy)
                {
                    BMP_MSG("\nLine 0x%04x:\n", y);
                    for (x = 0; x < width_bytes; x++)
                    {
                        if (x % 16 == 0)
                        {
                            BMP_MSG("\n");
                        }
                        BMP_MSG("%02x ", dst8[y * dst_pitch + x]);
                    }
                    BMP_MSG("\n");
                }
#endif
            }
            break;

        case 16:
        case 32:
            if (data->ver >= BMPVER_NT && data->bm_hdr.comp == BMPIC_BITFIELDS)
            {
                src_pitch = (width_bytes + 3) & ~3;
                for (y = first_line; y != last_line; y += dy)
                {
                    CHECK_FETCH(fetch_data(data->db, &dst8[y * dst_pitch], width_bytes));
                    for (x = 0; x < width; x++)
                    {
                        dst32 = (__u32 *)&dst8[y * dst_pitch + (x * (bpp >> 3))];
                        dst16 = (__u16 *)dst32;
                        src32 = dst32;
                        c.r = (*src32 & data->bm_hdr.nt.r_mask) >> data->r_shift;
                        c.g = (*src32 & data->bm_hdr.nt.g_mask) >> data->g_shift;
                        c.b = (*src32 & data->bm_hdr.nt.b_mask) >> data->b_shift;
                        if (data->a_bpp == 0)
                        {
                            c.a = 0xff; 
                        }
                        else
                        {
                            c.a = (*src32 & data->bm_hdr.a_mask) >> data->a_shift;
                        }

                        switch (data->bm_fmt)
                        {
                            case DSPF_ARGB1555:
                                *dst16 = (c.a << 15) | (c.r << 10) | (c.g << 5) | c.b;
                                break;

                            case DSPF_RGB16:
                                *dst16 = (c.r << 11) | (c.g << 5) | c.b;
                                break;

                            case DSPF_ARGB4444:
                                *dst16 = (c.a << 12) | (c.r << 8) | (c.g << 4) | c.b;
                                break;

                            case DSPF_RGB32:
                                *dst32 = (c.r << 16) | (c.g << 8) | c.b;
                                break;

                            case DSPF_ARGB:
                                *dst32 = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
                                break;

                            default:
                                BMP_ERR("%s: Error decoding BITFIELD data.\n", __FUNCTION__);
                                BMP_TRACE("%s: Leave\n", __FUNCTION__);
                                return DFB_FAILURE;
                        }
                    }

                    if (src_pitch > width_bytes)
                    {
                        CHECK_FETCH(fetch_data(data->db, tmp, src_pitch - width_bytes));
                    }
                }
            }
            else
            {
                /*
                 * Compression != BITFIELDS: 
                 *  
                 * Default 16bpp BMP format: x1555 --> surface format: DSPF_ARGB1555
                 * Default 32bpp BMP format: x8888 --> surface format: DSPF_RGB32
                 */
                src_pitch = (width_bytes + 3) & ~3;
                for (y = first_line; y != last_line; y += dy)
                {
                    CHECK_FETCH(fetch_data(data->db, &dst8[y * dst_pitch], width_bytes));
                    if (src_pitch > width_bytes)
                    {
                        CHECK_FETCH(fetch_data(data->db, tmp, src_pitch - width_bytes));
                    }

                    /* 
                     * Set alpha to opaque for 16bpp format.
                     */
                    if (bpp == 16)
                    {
                        dst16 = (__u16 *)&dst8[y * dst_pitch];
                        for (x = 0; x < width; x++)
                        {
                            dst16[x] |= 0x8000;
                        }
                    }
                }
            }
            break; 
        default:
            break;
    }

    ret = surf->Unlock(surf);
    if (ret != DFB_OK)
    {
        BMP_ERR("%s: Error unlocking destination surface.\n", __FUNCTION__);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}    

/*****************************************************************************/

static void
IDirectFBImageProvider_BMP_Destruct(IDirectFBImageProvider *thiz)
{
    IDirectFBImageProvider_BMP_data *data = thiz->priv;

    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    if (data->db)
    {
        data->db->Release(data->db);
    }

    if (data->pal)
    {
        data->pal->Release(data->pal);
    }

    if (data->colors)
    {
        D_FREE(data->colors);
    }

    DIRECT_DEALLOCATE_INTERFACE(thiz);
    BMP_TRACE("%s: Leave\n", __FUNCTION__);
}

static DFBResult
IDirectFBImageProvider_BMP_AddRef(IDirectFBImageProvider *thiz)
{
    DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    data->ref++;

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_BMP_Release(IDirectFBImageProvider *thiz)
{
    DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    if (--data->ref == 0)
    {
        IDirectFBImageProvider_BMP_Destruct(thiz);
    }

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_BMP_RenderTo(IDirectFBImageProvider *thiz,
                                    IDirectFBSurface       *destination,
                                    const DFBRectangle     *dest_rect)
{
    DFBResult               ret = DFB_OK;
    IDirectFBSurface_data  *dst_data;
    CoreSurface            *dst_surf;
    int                     width, height;
    IDirectFBSurface        *temp_surf;
    DFBRectangle            rect;
    DFBSurfacePixelFormat   dst_fmt;

    DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    if (!destination)
    {
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return DFB_INVARG;
    }

    dst_data = destination->priv;
    if (!dst_data || !dst_data->surface)
    {
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return DFB_DESTROYED;
    }

    dst_surf = dst_data->surface;

    rect.x = rect.y = 0;
    ret = destination->GetSize( destination, &rect.w, &rect.h );
    if (ret != DFB_OK)
    {
        BMP_ERR("%s: Couldn't get size of destination.\n", __FUNCTION__);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }
    if (dest_rect)
    {
        if (!dfb_rectangle_intersect(&rect, dest_rect))
        {
            BMP_ERR("%s: Specified dest_rect & destination do not overlap.\n", __FUNCTION__);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return DFB_OK;
        }
    }

    /*
     * Create a temporary surface that we will decode into.
     */
    width   = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.width : data->bm_hdr.width;
    height  = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.height : data->bm_hdr.height;
    if (height < 0)
    {
        height *= -1;
    }

    /*
     * Verify we can blit to the output surface.
     */
    ret = destination->GetPixelFormat(destination, &dst_fmt);
    if (ret != DFB_OK)
    {
        BMP_ERR("%s: Couldn't get pixel format of destination.\n", __FUNCTION__);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }
    if (DFB_PIXELFORMAT_IS_INDEXED(dst_fmt))
    {
        if (dst_fmt != data->bm_fmt)
        {
            BMP_ERR("%s: Can't render a palettized BMP to a surface of\n"
                    "differing palette format.\n", __FUNCTION__);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }
        else if (rect.w != width || rect.h != height)
        {
            BMP_ERR("%s: Can't scale palettized formats.\n", __FUNCTION__);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }
    }

    ret = idirectfb_singleton->CreateSurface(idirectfb_singleton, &data->surf_desc, &temp_surf);
    if (ret != DFB_OK)
    {
        BMP_ERR("%s: Failed to create temporary surface.\n", __FUNCTION__);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }

    ret = bmp_decode(data, temp_surf);
    if (ret != DFB_OK)
    {
        BMP_ERR("%s: Failed to decode BMP.\n", __FUNCTION__);
        temp_surf->Release(temp_surf);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }

    if (rect.w != width || rect.h != height)
    {
        ret = destination->StretchBlit(destination, temp_surf, NULL, &rect);
        if (ret != DFB_OK)
        {
            BMP_ERR("%s: Failed to StretchBlit temporary surface to dst surface.\n", __FUNCTION__);
            temp_surf->Release(temp_surf);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }
    }
    else
    {
        ret = destination->Blit(destination, temp_surf, NULL, rect.x, rect.y);
        if (ret != DFB_OK)
        {
            BMP_ERR("%s: Failed to Blit temporary surface to dst surface.\n", __FUNCTION__);
            temp_surf->Release(temp_surf);
            BMP_TRACE("%s: Leave\n", __FUNCTION__);
            return ret;
        }
    }

    temp_surf->Release(temp_surf);
    
    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    BMP_MSG("\n");
    return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_BMP_SetRenderCallback(IDirectFBImageProvider *thiz,
                                             DIRenderCallback        callback,
                                             void                   *ctx)
{
    DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    data->render_cb     = callback;
    data->render_cb_ctx = ctx;

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_BMP_GetSurfaceDescription(IDirectFBImageProvider *thiz,
                                                 DFBSurfaceDescription  *desc)
{
    DFBResult   rc;

    DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    if (!desc)
    {
        return DFB_INVARG;
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
    }

    desc->flags         = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
    desc->width         = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.width : data->bm_hdr.width;
    desc->height        = data->ver == BMPVER_1 ? data->file_hdr.v.ddb.height : data->bm_hdr.height;
    if (desc->height < 0)
    {
        desc->height *= -1;
    }
    desc->pixelformat   = data->bm_fmt;
    if (data->bm_fmt == DSPF_RGB24)
    {
        /*
         * RGB24 is not supported by M2MC.
         */
        desc->caps = DSCAPS_SYSTEMONLY;
    }
    else
    {
        desc->caps = DSCAPS_VIDEOONLY;
    }
    if (DFB_PIXELFORMAT_HAS_ALPHA(desc->pixelformat))
    {
        desc->caps |= DSCAPS_PREMULTIPLIED;
    }
    if (desc->pixelformat == DSPF_LUT1 ||
        desc->pixelformat == DSPF_LUT4 ||
        desc->pixelformat == DSPF_LUT8)
    {
        desc->flags             |= DSDESC_PALETTE;
        desc->palette.entries   = data->colors;
        desc->palette.size      = data->entries;
    }

    data->surf_desc = *desc;

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

static DFBResult
IDirectFBImageProvider_BMP_GetImageDescription(IDirectFBImageProvider *thiz,
                                               DFBImageDescription    *desc)
{
    DIRECT_INTERFACE_GET_DATA(IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    if (!desc)
    {
        return DFB_INVARG;
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
    }

    desc->caps = DICAPS_NONE;

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}

/* exported symbols */

static DFBResult
Probe(IDirectFBImageProvider_ProbeContext *ctx)
{
    BMP_TRACE("%s: Enter\n", __FUNCTION__);
    if (ctx->header[0] == 'B' && ctx->header[1] == 'M')
    {
        return DFB_OK;
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
    }

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_UNSUPPORTED;
}

static DFBResult
Construct(IDirectFBImageProvider *thiz,
          IDirectFBDataBuffer    *db)
{
    DFBResult ret;

    DIRECT_ALLOCATE_INTERFACE_DATA(thiz, IDirectFBImageProvider_BMP)
    BMP_TRACE("%s: Enter\n", __FUNCTION__);

    data->ref    = 1;
    data->db = db;

    db->AddRef(db);

    data->colors = NULL;

    ret = bmp_decode_header(data);
    if (ret)
    {
        IDirectFBImageProvider_BMP_Destruct(thiz);
        BMP_TRACE("%s: Leave\n", __FUNCTION__);
        return ret;
    }

    thiz->AddRef                = IDirectFBImageProvider_BMP_AddRef;
    thiz->Release               = IDirectFBImageProvider_BMP_Release;
    thiz->RenderTo              = IDirectFBImageProvider_BMP_RenderTo;
    thiz->SetRenderCallback     = IDirectFBImageProvider_BMP_SetRenderCallback;
    thiz->GetImageDescription   = IDirectFBImageProvider_BMP_GetImageDescription;
    thiz->GetSurfaceDescription = IDirectFBImageProvider_BMP_GetSurfaceDescription;
    thiz->GetSurfaceDescriptionUnaltered = IDirectFBImageProvider_BMP_GetSurfaceDescription;
    thiz->RenderToUnaltered     = IDirectFBImageProvider_BMP_RenderTo;

    BMP_TRACE("%s: Leave\n", __FUNCTION__);
    return DFB_OK;
}
