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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dfb_types.h>

#include <pthread.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/state.h>
#include <core/surfacemanager.h>
#include <core/palette.h>

#include <misc/gfx_util.h>
#include <misc/util.h>
#include <misc/conf.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/cpu_accel.h>
#include <direct/util.h>

#include <gfx/convert.h>
#include <gfx/util.h>

#include "generic.h"
#include "yuvtbl.h"

extern bool software_raster_warning;

/* lookup tables for 2/3bit to 8bit color conversion */
static const __u8 lookup3to8[] = { 0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};
static const __u8 lookup2to8[] = { 0x00, 0x55, 0xaa, 0xff};

#define EXPAND_1to8(v)   ((v) ? 0xff : 0x00)
#define EXPAND_2to8(v)   (lookup2to8[v])
#define EXPAND_3to8(v)   (lookup3to8[v])
#define EXPAND_4to8(v)   (((v) << 4) | ((v)     ))
#define EXPAND_5to8(v)   (((v) << 3) | ((v) >> 2))
#define EXPAND_6to8(v)   (((v) << 2) | ((v) >> 4))
#define EXPAND_7to8(v)   (((v) << 2) | ((v) >> 4))


static int use_mmx = 0;

#ifdef USE_MMX
static void gInit_MMX();
#endif

/********************************* Cop_to_Aop_PFI *****************************/

static void Cop_to_Aop_8( GenefxState *gfxs )
{
     memset( gfxs->Aop, gfxs->Cop, gfxs->length );
}

static void Cop_to_Aop_16( GenefxState *gfxs )
{
     int    w;
     int    l   = gfxs->length;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;

     __u32 DCop = ((Cop << 16) | Cop);

     if (((long)D)&2) {         /* align */
          __u16* tmp = (__u16*) D;
          --l;
          *tmp = Cop;
          D = (__u32*)(tmp+1);
     }

     w = (l >> 1);
     while (w) {
          *D = DCop;
          --w;
          ++D;
     }

     if (l & 1)                 /* do the last ential pixel */
          *((__u16*)D) = (__u16)Cop;
}

static void Cop_to_Aop_24( GenefxState *gfxs )
{
     int   w = gfxs->length;
     __u8 *D = gfxs->Aop;

     while (w) {
          D[0] = gfxs->color.b;
          D[1] = gfxs->color.g;
          D[2] = gfxs->color.r;

          D += 3;
          --w;
     }
}

static void Cop_to_Aop_32( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;

     while (w--)
          *D++ = Cop;
}

static void Cop_to_Aop_NV( GenefxState *gfxs )
{
     if (!gfxs->chroma_plane)
          Cop_to_Aop_8( gfxs );
     else
          Cop_to_Aop_16( gfxs );
}

static GenefxFunc Cop_to_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Cop_to_Aop_16,      /* DSPF_ARGB1555 */
     Cop_to_Aop_16,      /* DSPF_RGB16 */
     Cop_to_Aop_24,      /* DSPF_RGB24 */
     Cop_to_Aop_32,      /* DSPF_RGB32 */
     Cop_to_Aop_32,      /* DSPF_ARGB */
     Cop_to_Aop_8,       /* DSPF_A8 */
     Cop_to_Aop_32,      /* DSPF_YUY2 */
     Cop_to_Aop_8,       /* DSPF_RGB332 */
     Cop_to_Aop_32,      /* DSPF_UYVY */
     Cop_to_Aop_8,       /* DSPF_I420 */
     Cop_to_Aop_8,       /* DSPF_YV12 */
     Cop_to_Aop_8,       /* DSPF_LUT8 */
     Cop_to_Aop_8,       /* DSPF_ALUT44 */
     Cop_to_Aop_32,      /* DSPF_AiRGB */
     NULL,               /* DSPF_A1 */
     Cop_to_Aop_NV,      /* DSPF_NV12 */
     Cop_to_Aop_NV,      /* DSPF_NV16 */
     Cop_to_Aop_16,      /* DSPF_ARGB2554 */
     Cop_to_Aop_16,      /* DSPF_ARGB4444 */
     Cop_to_Aop_NV,      /* DSPF_NV21 */
     NULL,               /* DSPF_AYUV */
     NULL,               /* DSPF_ALUT88 */
     NULL,               /* DSPF_ABGR */
     NULL,               /* DSPF_LUT4 */
     NULL,               /* DSPF_LUT2 */
     NULL,               /* DSPF_LUT1 */
};

/********************************* Cop_toK_Aop_PFI ****************************/

static void Cop_toK_Aop_8( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u8  *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == *D)
               *D = Cop;

          D++;
     }
}

static void Cop_toK_Aop_12( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u16 *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == (*D & 0x0FFF))
               *D = Cop;

          D++;
     }
}

static void Cop_toK_Aop_14( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u16 *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == (*D & 0x3FFF))
               *D = Cop;

          D++;
     }
}

static void Cop_toK_Aop_15( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u16 *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == (*D & 0x7FFF))
               *D = Cop;

          D++;
     }
}

static void Cop_toK_Aop_16( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u16 *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == *D)
               *D = Cop;

          D++;
     }
}

static void Cop_toK_Aop_24( GenefxState *gfxs )
{
     D_ONCE("Cop_toK_Aop_24() unimplemented");
}

static void Cop_toK_Aop_32( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u32 *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == (*D & 0xffffff))
               *D = Cop;

          D++;
     }
}

static void Cop_toK_Aop_alut44( GenefxState *gfxs )
{
     int    w    = gfxs->length;
     __u8  *D    = gfxs->Aop;
     __u32  Cop  = gfxs->Cop;
     __u32  Dkey = gfxs->Dkey;

     while (w--) {
          if (Dkey == (*D & 0x0F))
               *D = Cop;

          D++;
     }
}

static GenefxFunc Cop_toK_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Cop_toK_Aop_15,          /* DSPF_ARGB1555 */
     Cop_toK_Aop_16,          /* DSPF_RGB16 */
     Cop_toK_Aop_24,          /* DSPF_RGB24 */
     Cop_toK_Aop_32,          /* DSPF_RGB32 */
     Cop_toK_Aop_32,          /* DSPF_ARGB */
     Cop_toK_Aop_8,           /* DSPF_A8 */
     NULL,                    /* DSPF_YUY2 */
     Cop_toK_Aop_8,           /* DSPF_RGB332 */
     NULL,                    /* DSPF_UYVY */
     NULL,                    /* DSPF_I420 */
     NULL,                    /* DSPF_YV12 */
     Cop_toK_Aop_8,           /* DSPF_LUT8 */
     Cop_toK_Aop_alut44,      /* DSPF_ALUT44 */
     Cop_toK_Aop_32,          /* DSPF_AiRGB */
     NULL,                    /* DSPF_A1 */
     NULL,                    /* DSPF_NV12 */
     NULL,                    /* DSPF_NV16 */
     Cop_toK_Aop_14,          /* DSPF_ARGB2554 */
     Cop_toK_Aop_12,          /* DSPF_ARGB4444 */
     NULL,                    /* DSPF_NV21 */
     NULL,                    /* DSPF_AYUV */
     NULL,                    /* DSPF_ALUT88 */
     NULL,                    /* DSPF_ABGR */
     NULL,                    /* DSPF_LUT4 */
     NULL,                    /* DSPF_LUT2 */
     NULL,                    /* DSPF_LUT1 */
};

/********************************* Bop_PFI_to_Aop_PFI *************************/

static void Bop_8_to_Aop( GenefxState *gfxs )
{
     direct_memmove( gfxs->Aop, gfxs->Bop, gfxs->length );
}

static void Bop_16_to_Aop( GenefxState *gfxs )
{
     direct_memmove( gfxs->Aop, gfxs->Bop, gfxs->length*2 );
}

static void Bop_24_to_Aop( GenefxState *gfxs )
{
     direct_memmove( gfxs->Aop, gfxs->Bop, gfxs->length*3 );
}

static void Bop_32_to_Aop( GenefxState *gfxs )
{
     direct_memmove( gfxs->Aop, gfxs->Bop, gfxs->length*4 );
}

static void Bop_NV_to_Aop( GenefxState *gfxs )
{
     if (!gfxs->chroma_plane)
          Bop_8_to_Aop( gfxs );
     else
          Bop_16_to_Aop( gfxs );
}

static GenefxFunc Bop_PFI_to_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_16_to_Aop,      /* DSPF_ARGB1555 */
     Bop_16_to_Aop,      /* DSPF_RGB16 */
     Bop_24_to_Aop,      /* DSPF_RGB24 */
     Bop_32_to_Aop,      /* DSPF_RGB32 */
     Bop_32_to_Aop,      /* DSPF_ARGB */
     Bop_8_to_Aop,       /* DSPF_A8 */
     Bop_16_to_Aop,      /* DSPF_YUY2 */
     Bop_8_to_Aop,       /* DSPF_RGB332 */
     Bop_16_to_Aop,      /* DSPF_UYVY */
     Bop_8_to_Aop,       /* DSPF_I420 */
     Bop_8_to_Aop,       /* DSPF_YV12 */
     Bop_8_to_Aop,       /* DSPF_LUT8 */
     Bop_8_to_Aop,       /* DSPF_ALUT44 */
     Bop_32_to_Aop,      /* DSPF_AiRGB */
     NULL,               /* DSPF_A1 */
     Bop_NV_to_Aop,      /* DSPF_NV12 */
     Bop_NV_to_Aop,      /* DSPF_NV16 */
     Bop_16_to_Aop,      /* DSPF_ARGB2554 */
     Bop_16_to_Aop,      /* DSPF_ARGB4444 */
     Bop_NV_to_Aop,      /* DSPF_NV21 */
     NULL,               /* DSPF_AYUV */
     NULL,               /* DSPF_ALUT88 */
     NULL,               /* DSPF_ABGR */
     NULL,               /* DSPF_LUT4 */
     NULL,               /* DSPF_LUT2 */
     NULL,               /* DSPF_LUT1 */
};

/********************************* Bop_PFI_Kto_Aop_PFI ************************/

/* FIXME: take care of gfxs->Ostep! */
static void Bop_rgb15_Kto_Aop( GenefxState *gfxs )
{
     int    w;
     int    l    = gfxs->length;
     __u32 *D    = gfxs->Aop;
     __u32 *S    = gfxs->Bop;
     __u32  Skey = gfxs->Skey;

     __u32 DSkey = (Skey << 16) | Skey;

     if (((long)D)&2) {         /* align */
          __u16 *tmp = gfxs->Aop;
          --l;
          if ((*((__u16*)S) & 0x7FFF) != Skey)
               *tmp = *((__u16*)S);

          D = (__u32*)((__u16*)D+1);
          S = (__u32*)((__u16*)S+1);
     }

     w = (l >> 1);
     while (w) {
          __u32 dpixel = *S;
          __u16 *tmp = (__u16*)D;

          if ((dpixel & 0x7FFF7FFF) != DSkey) {
               if ((dpixel & 0x7FFF0000) != (DSkey & 0x7FFF0000)) {
                    if ((dpixel & 0x00007FFF) != (DSkey & 0x00007FFF)) {
                         *D = dpixel;
                    }
                    else {
#ifdef WORDS_BIGENDIAN
                         tmp[0] = (__u16)(dpixel >> 16);
#else
                         tmp[1] = (__u16)(dpixel >> 16);
#endif
                    }
               }
               else {
#ifdef WORDS_BIGENDIAN
                    tmp[1] = (__u16)dpixel;
#else
                    tmp[0] = (__u16)dpixel;
#endif
               }
          }
          ++S;
          ++D;
          --w;
     }

     if (l & 1) {                 /* do the last potential pixel */
          if ((*((__u16*)S) & 0x7FFF) != Skey)
               *((__u16*)D) = *((__u16*)S);
     }
}

/* FIXME: take care of gfxs->Ostep! */
static void Bop_rgb16_Kto_Aop( GenefxState *gfxs )
{
     int    w, l = gfxs->length;
     __u32 *D    = gfxs->Aop;
     __u32 *S    = gfxs->Bop;
     __u32  Skey = gfxs->Skey;

     __u32 DSkey = (Skey << 16) | Skey;

     if (((long)D)&2) {         /* align */
          __u16 *tmp = gfxs->Aop;
          --l;
          if (*((__u16*)S) != Skey)
               *tmp = *((__u16*)S);

          D = (__u32*)((__u16*)D+1);
          S = (__u32*)((__u16*)S+1);
     }

     w = (l >> 1);
     while (w) {
          __u32 dpixel = *S;
          __u16 *tmp = (__u16*)D;

          if (dpixel != DSkey) {
               if ((dpixel & 0xFFFF0000) != (DSkey & 0xFFFF0000)) {
                    if ((dpixel & 0x0000FFFF) != (DSkey & 0x0000FFFF)) {
                         *D = dpixel;
                    }
                    else {
#ifdef WORDS_BIGENDIAN
                         tmp[0] = (__u16)(dpixel >> 16);
#else
                         tmp[1] = (__u16)(dpixel >> 16);
#endif
                    }
               }
               else {
#ifdef WORDS_BIGENDIAN
                    tmp[1] = (__u16)dpixel;
#else
                    tmp[0] = (__u16)dpixel;
#endif
               }
          }
          ++S;
          ++D;
          --w;
     }

     if (l & 1) {                 /* do the last potential pixel */
          if (*((__u16*)S) != Skey)
               *((__u16*)D) = *((__u16*)S);
     }
}

static void Bop_rgb24_Kto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u8  *D     = gfxs->Aop;
     __u8  *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += (gfxs->length - 1) * 3;
          S += (gfxs->length - 1) * 3;
     }

     while (w--) {
          __u8 b = *S;
          __u8 g = *(S+1);
          __u8 r = *(S+2);

          if (Skey != (__u32)(r<<16 | g<<8 | b )) {
               *D     = b;
               *(D+1) = g;
               *(D+2) = r;
          }

          S += Ostep * 3;
          D += Ostep * 3;
     }
}

static void Bop_rgb32_Kto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u32 *D     = gfxs->Aop;
     __u32 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          __u32 spixel = *S;

          if ((spixel & 0xffffff) != Skey)
               *D = spixel;

          S += Ostep;
          D += Ostep;
     }
}

static void Bop_a8_Kto_Aop( GenefxState *gfxs )
{
     /* no color to key */
     direct_memmove( gfxs->Aop, gfxs->Bop, gfxs->length );
}

static void Bop_8_Kto_Aop( GenefxState *gfxs )
{
     int    i;
     int    w    = gfxs->length;
     __u8  *D    = gfxs->Aop;
     __u8  *S    = gfxs->Bop;
     __u32  Skey = gfxs->Skey;

     if (gfxs->Ostep < 0) {
          for (i=w-1; i>=0; --i) {
               register __u32 pixel = S[i];

               if (pixel != Skey)
                    D[i] = pixel;
          }
     }
#ifndef ARCH_X86
     else if (((long) S & 3) == ((long) D & 3)) {
          __u32 *D32;
          __u32 *S32;
          __u32  Skey32;
          int    n = 0;

          /* Align source & destination to 32 bit. */
          if ((long) D & 3) {
               n = 4 - ((long) D & 3);

               for (i=0; i<n; i++) {
                    register __u32 pixel = S[i];

                    if (pixel != Skey)
                         D[i] = pixel;
               }

               w -= n;
          }

          D32 = gfxs->Aop + n;
          S32 = gfxs->Bop + n;

          /* Fill 32 bit with 8 bit key. */
          Skey32 = (Skey << 24) | (Skey << 16) | (Skey << 8) | Skey;

          /* Compare four pixels at once. */
          n = w >> 2;
          for (i=0; i<n; i++) {
               __u32 P32 = S32[i];
               __u32 X32 = P32 ^ Skey32;

               if (X32) {
                    /* Check for common case. */
                    if ((X32 & 0xff000000) &&
                        (X32 & 0x00ff0000) &&
                        (X32 & 0x0000ff00) &&
                        (X32 & 0x000000ff))
                    {
                         /* Copy all four pixels. */
                         D32[i] = P32;
                    }
                    else {
                         /* Copy one by one. */
                         D = (__u8*) (D32 + i);

#ifdef WORDS_BIGENDIAN
                         if (X32 & 0xff000000)
                              D[0] = (__u8)P32 >> 24;

                         if (X32 & 0x00ff0000)
                              D[1] = P32 >> 16;

                         if (X32 & 0x0000ff00)
                              D[2] = P32 >> 8;

                         if (X32 & 0x000000ff)
                              D[3] = P32;

#else

                         if (X32 & 0x000000ff)
                              D[0] = P32;

                         if (X32 & 0x0000ff00)
                              D[1] = P32 >> 8;

                         if (X32 & 0x00ff0000)
                              D[2] = P32 >> 16;

                         if (X32 & 0xff000000)
                              D[3] = P32 >> 24;
#endif
                    }
               }
          }

          /* Copy remaining pixels one by one. */
          if (w & 3) {
               D = (__u8*) (D32 + n);
               S = (__u8*) (S32 + n);

               n = w & 3;

               for (i=0; i<n; i++) {
                    register __u32 pixel = S[i];

                    if (pixel != Skey)
                         D[i] = pixel;
               }
          }
     }
     else {
          register __u32 out = 0;     /* Collect output pixels. */
          register int   num = 0;     /* Count output pixels. */

          /* Align destination to 32 bit. */
          if (((long) D & 3) && w > 4) {
               int n = 4 - ((long) D & 3);

               for (i=0; i<n; i++) {
                    register __u32 pixel = S[i];

                    if (pixel != Skey)
                         D[i] = pixel;
               }

               w -= n;
               D += n;
               S += n;
          }

          /* Write up to 32 bit at once. */
          for (i=0; i<w; ++i) {
               register __u32 pixel = S[i];

               if (pixel != Skey) {
#ifdef WORDS_BIGENDIAN
                    out = (out << 8) | pixel;
#else
                    out = (out >> 8) | (pixel << 24);
#endif
                    if (++num == 4) {
                         *(__u32*)(D+i-3) = out;

                         num = 0;
                    }
                    else if ((i & 3) == 3) {
                         /* Flush 'out' to realign destination to 32 bit. */
                         switch (num) {
                              case 3:
                                   D[i-2] = out >> 16;
                              case 2:
                                   D[i-1] = out >> 8;
                              case 1:
                                   D[i] = out;
                         }

                         num = 0;
                    }
               }
               else while (num) {
                    /* Flush 'out' due to a keyed pixel. */
                    switch (num) {
                         case 3:
                              D[i-3] = out >> 16;
                         case 2:
                              D[i-2] = out >> 8;
                         case 1:
                              D[i-1] = out;
                    }

                    num = 0;
               }
          }

          /* Flush 'out'. */
          if (num) {
               switch (num) {
                    case 3:
                         D[i-3] = out >> 16;
                    case 2:
                         D[i-2] = out >> 8;
                    case 1:
                         D[i-1] = out;
               }
          }
     }
#else
     else {
          for (i=0; i<w; ++i) {
               register __u32 pixel = S[i];

               if (pixel != Skey)
                    D[i] = pixel;
          }
     }
#endif
}

static void Bop_alut44_Kto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u8  *D     = gfxs->Aop;
     __u8  *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          __u8 spixel = *S;

          if ((spixel & 0x0F) != Skey)
               *D = spixel;

          S += Ostep;
          D += Ostep;
     }
}

static void Bop_yuy2_Kto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length/2;
     __u32 *D     = gfxs->Aop;
     __u32 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          __u32 spixel = *S;

          if (spixel != Skey)
               *D = spixel;

          S += Ostep;
          D += Ostep;
     }
}

static void Bop_argb2554_Kto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u16  Skey  = gfxs->Skey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          __u16 spixel = *S;

          if ((spixel & 0x3FFF) != Skey)
               *D = spixel;

          S += Ostep;
          D += Ostep;
     }
}

static void Bop_argb4444_Kto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u16  Skey  = gfxs->Skey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          __u16 spixel = *S;

          if ((spixel & 0x0FFF) != Skey)
               *D = spixel;

          S += Ostep;
          D += Ostep;
     }
}

static GenefxFunc Bop_PFI_Kto_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_rgb15_Kto_Aop,       /* DSPF_ARGB1555 */
     Bop_rgb16_Kto_Aop,       /* DSPF_RGB16 */
     Bop_rgb24_Kto_Aop,       /* DSPF_RGB24 */
     Bop_rgb32_Kto_Aop,       /* DSPF_RGB32 */
     Bop_rgb32_Kto_Aop,       /* DSPF_ARGB */
     Bop_a8_Kto_Aop,          /* DSPF_A8 */
     Bop_yuy2_Kto_Aop,        /* DSPF_YUY2 */
     Bop_8_Kto_Aop,           /* DSPF_RGB332 */
     Bop_yuy2_Kto_Aop,        /* DSPF_UYVY */
     NULL,                    /* DSPF_I420 */
     NULL,                    /* DSPF_YV12 */
     Bop_8_Kto_Aop,           /* DSPF_LUT8 */
     Bop_alut44_Kto_Aop,      /* DSPF_ALUT44 */
     Bop_rgb32_Kto_Aop,       /* DSPF_AiRGB */
     NULL,                    /* DSPF_A1 */
     NULL,                    /* DSPF_NV12 */
     NULL,                    /* DSPF_NV16 */
     Bop_argb2554_Kto_Aop,    /* DSPF_ARGB2554 */
     Bop_argb4444_Kto_Aop,    /* DSPF_ARGB4444 */
     NULL,                    /* DSPF_NV21 */
     NULL,                    /* DSPF_AYUV */
     NULL,                    /* DSPF_ALUT88 */
     NULL,                    /* DSPF_ABGR */
     NULL,                    /* DSPF_LUT4 */
     NULL,                    /* DSPF_LUT2 */
     NULL,                    /* DSPF_LUT1 */
};

/********************************* Bop_PFI_toK_Aop_PFI ************************/

static void Bop_rgb15_toK_Aop( GenefxState *gfxs )
{
     int    w, l = gfxs->length;
     __u32 *D    = gfxs->Aop;
     __u32 *S    = gfxs->Bop;
     __u32  Lkey = gfxs->Dkey;
     __u32  Hkey = Lkey << 16;

     if (((long)D)&2) {         /* align */
          __u16 *tmp = gfxs->Aop;
          --l;
          if (*((__u16*)D) == Lkey)
               *tmp = *((__u16*)S);

          D = (__u32*)((__u16*)D+1);
          S = (__u32*)((__u16*)S+1);
     }

     w = (l >> 1);
     while (w) {
          __u32 dpixel = *D;

          if ((dpixel & 0x7FFF0000) == Hkey) {
               if ((dpixel & 0x00007FFF) == Lkey) {
                    *D = *S;
               }
               else {
                    __u16 *tmp = (__u16*)D;
#ifdef WORDS_BIGENDIAN
                    tmp[0] = (__u16)(*S >> 16);
#else
                    tmp[1] = (__u16)(*S >> 16);
#endif
               }
          }
          else if ((dpixel & 0x00007FFF) == Lkey) {
               __u16 *tmp = (__u16*)D;
#ifdef WORDS_BIGENDIAN
               tmp[1] = (__u16)*S;
#else
               tmp[0] = (__u16)*S;
#endif
          }
          ++S;
          ++D;
          --w;
     }

     if (l & 1) {                 /* do the last potential pixel */
          if (*((__u16*)D) == Lkey)
               *((__u16*)D) = *((__u16*)S);
     }

}

static void Bop_rgb16_toK_Aop( GenefxState *gfxs )
{
     int    w, l = gfxs->length;
     __u32 *D    = gfxs->Aop;
     __u32 *S    = gfxs->Bop;
     __u32  Dkey = gfxs->Dkey;

     __u32 DDkey = (Dkey << 16) | Dkey;

     if (((long)D)&2) {         /* align */
          __u16 *tmp = gfxs->Aop;
          --l;
          if (*((__u16*)D) == Dkey)
               *tmp = *((__u16*)S);

          D = (__u32*)((__u16*)D+1);
          S = (__u32*)((__u16*)S+1);
     }

     w = (l >> 1);
     while (w) {
          __u32 dpixel = *D;
          __u16 *tmp = (__u16*)D;

          if ((dpixel & 0xFFFF0000) == (DDkey & 0xFFFF0000)) {
               if ((dpixel & 0x0000FFFF) == (DDkey & 0x0000FFFF)) {
                    *D = *S;
               }
               else {
#ifdef WORDS_BIGENDIAN
                    tmp[0] = (__u16)(*S >> 16);
#else
                    tmp[1] = (__u16)(*S >> 16);
#endif
               }
          }
          else if ((dpixel & 0x0000FFFF) == (DDkey & 0x0000FFFF)) {
#ifdef WORDS_BIGENDIAN
               tmp[1] = (__u16)*S;
#else
               tmp[0] = (__u16)*S;
#endif
          }
          ++S;
          ++D;
          --w;
     }

     if (l & 1) {                 /* do the last potential pixel */
          if (*((__u16*)D) == Dkey)
               *((__u16*)D) = *((__u16*)S);
     }

}

static void Bop_rgb32_toK_Aop( GenefxState *gfxs )
{
      int    w    = gfxs->length;
      __u32 *S    = gfxs->Bop;
      __u32 *D    = gfxs->Aop;
      __u32  Dkey = gfxs->Dkey;

      while (w--) {
           if (Dkey == *D) {
                *D = *S;
           }
           D++;
           S++;
      }
}

static void Bop_argb2554_toK_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u16  Dkey  = gfxs->Dkey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          if (Dkey == (*D & 0x3FFF))
               *D = *S;

          S += Ostep;
          D += Ostep;
     }
}

static void Bop_argb4444_toK_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u16  Dkey  = gfxs->Dkey;
     int    Ostep = gfxs->Ostep;

     if (Ostep < 0) {
          D += gfxs->length - 1;
          S += gfxs->length - 1;
     }

     while (w--) {
          if (Dkey == (*D & 0x0FFF))
               *D = *S;

          S += Ostep;
          D += Ostep;
     }
}

static GenefxFunc Bop_PFI_toK_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_rgb15_toK_Aop,       /* DSPF_ARGB1555 */
     Bop_rgb16_toK_Aop,       /* DSPF_RGB16 */
     NULL,                    /* DSPF_RGB24 */
     Bop_rgb32_toK_Aop,       /* DSPF_RGB32 */
     NULL,                    /* DSPF_ARGB */
     NULL,                    /* DSPF_A8 */
     NULL,                    /* DSPF_YUY2 */
     NULL,                    /* DSPF_RGB332 */
     NULL,                    /* DSPF_UYVY */
     NULL,                    /* DSPF_I420 */
     NULL,                    /* DSPF_YV12 */
     NULL,                    /* DSPF_LUT8 */
     NULL,                    /* DSPF_ALUT44 */
     NULL,                    /* DSPF_AiRGB */
     NULL,                    /* DSPF_A1 */
     NULL,                    /* DSPF_NV12 */
     NULL,                    /* DSPF_NV16 */
     Bop_argb2554_toK_Aop,    /* DSPF_ARGB2554 */
     Bop_argb4444_toK_Aop,    /* DSPF_ARGB4444 */
     NULL,                    /* DSPF_NV21 */
     NULL,                    /* DSPF_AYUV */
     NULL,                    /* DSPF_ALUT88 */
     NULL,                    /* DSPF_ABGR */
     NULL,                    /* DSPF_LUT4 */
     NULL,                    /* DSPF_LUT2 */
     NULL,                    /* DSPF_LUT1 */
};



/********************************* Bop_PFI_Sto_Aop_PFI ****************************/

static void Bop_16_Sto_Aop( GenefxState *gfxs )
{
     int    w2;
     int    w      = gfxs->length;
     int    i      = 0;
     __u32 *D      = gfxs->Aop;
     __u16 *S      = gfxs->Bop;
     int    SperD  = gfxs->SperD;
     int    SperD2 = SperD << 1;

     if (((long)D)&2) {
          *(__u16*)D = *S;
          i += SperD;
          w--;
          D = gfxs->Aop + 2;
     }

     w2 = (w >> 1);
     while (w2--) {
#ifdef WORDS_BIGENDIAN
          *D++ =  S[i>>16] << 16 | S[(i+SperD)>>16];
#else
          *D++ = (S[(i+SperD)>>16] << 16) | S[i>>16];
#endif
          i += SperD2;
     }
     if (w&1) {
          *(__u16*)D = S[i>>16];
     }
}

static void Bop_24_Sto_Aop( GenefxState *gfxs )
{
     int   w     = gfxs->length;
     int   i     = 0;
     __u8 *D     = gfxs->Aop;
     __u8 *S     = gfxs->Bop;
     int   SperD = gfxs->SperD;

     while (w--) {
          int pixelstart = (i>>16)*3;

          *D++ = S[pixelstart+0];
          *D++ = S[pixelstart+1];
          *D++ = S[pixelstart+2];

          i += SperD;
     }
}

static void Bop_32_Sto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u32 *D     = gfxs->Aop;
     __u32 *S     = gfxs->Bop;
     int    SperD = gfxs->SperD;

     while (w--) {
          *D++ = S[i>>16];

          i += SperD;
     }
}

static void Bop_8_Sto_Aop( GenefxState *gfxs )
{
     int   w     = gfxs->length;
     int   i     = 0;
     __u8 *D     = gfxs->Aop;
     __u8 *S     = gfxs->Bop;
     int   SperD = gfxs->SperD;

     while (w--) {
          *D++ = S[i>>16];

          i += SperD;
     }
}

static void Bop_yuy2_Sto_Aop( GenefxState *gfxs )
{
     int    w2;
     int    w     = gfxs->length;
     int    i     = 0;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     int    SperD = gfxs->SperD;

     if ((__u32)D & 2) {
          *D++ = *S;
          i = SperD;
          w--;
     }

     for (w2 = w/2; w2--;) {
          register __u32 Dpix;

          Dpix  = ((__u32*)S)[i>>17] & 0xff00ff00;
#ifdef WORDS_BIGENDIAN
          Dpix |= (S[i>>16]          & 0x00ff) << 16;
          Dpix |= (S[(i+SperD)>>16]  & 0x00ff);
#else
          Dpix |= (S[i>>16]          & 0x00ff);
          Dpix |= (S[(i+SperD)>>16]  & 0x00ff) << 16;
#endif
          *((__u32*)D) = Dpix;
          D += 2;

          i += SperD << 1;
     }

     if (w & 1)
          *D = S[i>>16];
}

static void Bop_uyvy_Sto_Aop( GenefxState *gfxs )
{
     int    w2;
     int    w     = gfxs->length;
     int    i     = 0;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     int    SperD = gfxs->SperD;

     if ((__u32)D & 2) {
          *D++ = *S;
          i = SperD;
          w--;
     }

     for (w2 = w/2; w2--;) {
          register __u32 Dpix;

          Dpix  = ((__u32*)S)[i>>17] & 0x00ff00ff;
#ifdef WORDS_BIGENDIAN
          Dpix |= (S[i>>16]          & 0xff00) << 16;
          Dpix |= (S[(i+SperD)>>16]  & 0xff00);
#else
          Dpix |= (S[i>>16]          & 0xff00);
          Dpix |= (S[(i+SperD)>>16]  & 0xff00) << 16;
#endif
          *((__u32*)D) = Dpix;
          D += 2;

          i += SperD << 1;
     }

     if (w & 1)
          *D = S[i>>16];
}

static void Bop_NV_Sto_Aop( GenefxState *gfxs )
{
     if (!gfxs->chroma_plane)
          Bop_8_Sto_Aop( gfxs );
     else
          Bop_16_Sto_Aop( gfxs );
}

static GenefxFunc Bop_PFI_Sto_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_16_Sto_Aop,          /* DSPF_ARGB1555 */
     Bop_16_Sto_Aop,          /* DSPF_RGB16 */
     Bop_24_Sto_Aop,          /* DSPF_RGB24 */
     Bop_32_Sto_Aop,          /* DSPF_RGB32 */
     Bop_32_Sto_Aop,          /* DSPF_ARGB */
     Bop_8_Sto_Aop,           /* DSPF_A8 */
     Bop_yuy2_Sto_Aop,        /* DSPF_YUY2 */
     Bop_8_Sto_Aop,           /* DSPF_RGB332 */
     Bop_uyvy_Sto_Aop,        /* DSPF_UYVY */
     Bop_8_Sto_Aop,           /* DSPF_I420 */
     Bop_8_Sto_Aop,           /* DSPF_YV12 */
     Bop_8_Sto_Aop,           /* DSPF_LUT8 */
     Bop_8_Sto_Aop,           /* DSPF_ALUT44 */
     Bop_32_Sto_Aop,          /* DSPF_AiRGB */
     NULL,                    /* DSPF_A1 */
     Bop_NV_Sto_Aop,          /* DSPF_NV12 */
     Bop_NV_Sto_Aop,          /* DSPF_NV16 */
     Bop_16_Sto_Aop,          /* DSPF_ARGB2554 */
     Bop_16_Sto_Aop,          /* DSPF_ARGB4444 */
     Bop_NV_Sto_Aop,          /* DSPF_NV21 */
     NULL,                    /* DSPF_AYUV */
     NULL,                    /* DSPF_ALUT88 */
     NULL,                    /* DSPF_ABGR */
     NULL,                    /* DSPF_LUT4 */
     NULL,                    /* DSPF_LUT2 */
     NULL,                    /* DSPF_LUT1 */
};

/********************************* Bop_PFI_SKto_Aop_PFI ***************************/

static void Bop_rgb15_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u16 s = S[i>>16];

          if ((s & 0x7FFF) != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static void Bop_rgb16_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u16 s = S[i>>16];

          if (s != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static void Bop_rgb24_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u8  *D     = gfxs->Aop;
     __u8  *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          int pixelstart = (i>>16)*3;

          __u8 b = S[pixelstart+0];
          __u8 g = S[pixelstart+1];
          __u8 r = S[pixelstart+2];

          if (Skey != (__u32)(r<<16 | g<<8 | b )) {
               *D     = b;
               *(D+1) = g;
               *(D+2) = r;
          }

          D += 3;
          i += SperD;
     }
}

static void Bop_rgb32_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u32 *D     = gfxs->Aop;
     __u32 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u32 s = S[i>>16];

          if ((s & 0xffffff) != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static void Bop_a8_SKto_Aop( GenefxState *gfxs )
{
     int   w     = gfxs->length;
     int   i     = 0;
     __u8 *D     = gfxs->Aop;
     __u8 *S     = gfxs->Bop;
     int   SperD = gfxs->SperD;

     /* no color to key */
     while (w--) {
          *D++ = S[i>>16];

          i += SperD;
     }
}

static void Bop_8_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u8  *D     = gfxs->Aop;
     __u8  *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u8 s = S[i>>16];

          if (s != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static void Bop_alut44_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u8  *D     = gfxs->Aop;
     __u8  *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u8 s = S[i>>16];

          if ((s & 0x0f) != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static void Bop_argb2554_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u16 s = S[i>>16];

          if ((s & 0x3FFF) != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static void Bop_argb4444_SKto_Aop( GenefxState *gfxs )
{
     int    w     = gfxs->length;
     int    i     = 0;
     __u16 *D     = gfxs->Aop;
     __u16 *S     = gfxs->Bop;
     __u32  Skey  = gfxs->Skey;
     int    SperD = gfxs->SperD;

     while (w--) {
          __u16 s = S[i>>16];

          if ((s & 0x0FFF) != Skey)
               *D = s;

          D++;
          i += SperD;
     }
}

static GenefxFunc Bop_PFI_SKto_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_rgb15_SKto_Aop,      /* DSPF_ARGB1555 */
     Bop_rgb16_SKto_Aop,      /* DSPF_RGB16 */
     Bop_rgb24_SKto_Aop,      /* DSPF_RGB24 */
     Bop_rgb32_SKto_Aop,      /* DSPF_RGB32 */
     Bop_rgb32_SKto_Aop,      /* DSPF_ARGB */
     Bop_a8_SKto_Aop,         /* DSPF_A8 */
     NULL,                    /* DSPF_YUY2 */
     Bop_8_SKto_Aop,          /* DSPF_RGB332 */
     NULL,                    /* DSPF_UYVY */
     NULL,                    /* DSPF_I420 */
     NULL,                    /* DSPF_YV12 */
     Bop_8_SKto_Aop,          /* DSPF_LUT8 */
     Bop_alut44_SKto_Aop,     /* DSPF_ALUT44 */
     Bop_rgb32_SKto_Aop,      /* DSPF_AiRGB */
     NULL,                    /* DSPF_A1 */
     NULL,                    /* DSPF_NV12 */
     NULL,                    /* DSPF_NV16 */
     Bop_argb2554_SKto_Aop,   /* DSPF_ARGB2554 */
     Bop_argb4444_SKto_Aop,   /* DSPF_ARGB4444 */
     NULL,                    /* DSPF_NV21 */
     NULL,                    /* DSPF_AYUV */
     NULL,                    /* DSPF_ALUT88 */
     NULL,                    /* DSPF_ABGR */
     NULL,                    /* DSPF_LUT4 */
     NULL,                    /* DSPF_LUT2 */
     NULL,                    /* DSPF_LUT1 */
};

/********************************* Sop_PFI_Sto_Dacc ***************************/

static void Sop_argb1555_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     while (w--) {
          __u16 s = S[i>>16];

          D->a = (s & 0x8000) ? 0xff : 0;
          D->r = (s & 0x7C00) >> 7;
          D->g = (s & 0x03E0) >> 2;
          D->b = (s & 0x001F) << 3;

          i += SperD;

          D++;
     }
}

static void Sop_rgb16_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     while (w--) {
          __u16 s = S[i>>16];

          D->a = 0xFF;
          D->r = (s & 0xF800) >> 8;
          D->g = (s & 0x07E0) >> 3;
          D->b = (s & 0x001F) << 3;

          i += SperD;

          D++;
     }
}

static void Sop_rgb24_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     while (w--) {
          int pixelstart = (i>>16)*3;

          D->a = 0xFF;
          D->r = S[pixelstart+2];
          D->g = S[pixelstart+1];
          D->b = S[pixelstart+0];

          i += SperD;

          D++;
     }
}

static void Sop_rgb32_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 s = S[i>>16];

          D->a = 0xFF;
          D->r = (s & 0x00FF0000) >> 16;
          D->g = (s & 0x0000FF00) >>  8;
          D->b = (s & 0x000000FF);

          i += SperD;

          D++;
     }
}


static void Sop_argb_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 s = S[i>>16];

          D->a = (s & 0xFF000000) >> 24;
          D->r = (s & 0x00FF0000) >> 16;
          D->g = (s & 0x0000FF00) >>  8;
          D->b = (s & 0x000000FF);

          i += SperD;

          D++;
     }
}

static void Sop_airgb_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 s = S[i>>16];

          D->a = ((s & 0xFF000000) >> 24) ^ 0xff;
          D->r = (s & 0x00FF0000) >> 16;
          D->g = (s & 0x0000FF00) >>  8;
          D->b = (s & 0x000000FF);

          i += SperD;

          D++;
     }
}

static void Sop_a8_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     while (w--) {
          __u8 s = S[i>>16];

          D->a = s;
          D->r = 0xFF;
          D->g = 0xFF;
          D->b = 0xFF;

          i += SperD;

          D++;
     }
}

static void Sop_rgb332_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     while (w--) {
          __u8 s = S[i>>16];

          D->a = 0xFF;
          D->r = EXPAND_3to8(s >> 5);
          D->g = EXPAND_3to8((s & 0x1C) >> 2);
          D->b = EXPAND_2to8(s & 0x03);

          i += SperD;

          D++;
     }
}

static void Sop_lut8_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = S[i>>16];

          D->a = entries[s].a;
          D->r = entries[s].r;
          D->g = entries[s].g;
          D->b = entries[s].b;

          i += SperD;

          D++;
     }
}

static void Sop_alut44_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = S[i>>16];

          D->a = s & 0xF0;
          s &= 0x0F;
          D->r = entries[s].r;
          D->g = entries[s].g;
          D->b = entries[s].b;

          i += SperD;

          D++;
     }
}

static void Sop_yuy2_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length/2;
     int i     = 0;
     int j     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 y0, cb, y1, cr;
          __s32 r, g, b;

#ifdef WORDS_BIGENDIAN
          cb = (S[i>>16] & 0xFF000000) >> 24;
          cr = (S[i>>16] & 0x0000FF00) >>  8;
#else
          cb = (S[i>>16] & 0x0000FF00) >>  8;
          cr = (S[i>>16] & 0xFF000000) >> 24;
#endif
          y0 = ((__u16*)S)[j>>16]         & 0x00FF;
          y1 = ((__u16*)S)[(j+SperD)>>16] & 0x00FF;

          y0 = y_for_rgb[y0];
          r  = y0 + cr_for_r[cr];
          g  = y0 + cr_for_g[cr] + cb_for_g[cb];
          b  = y0 + cb_for_b[cb];

          D[0].a = 0xFF;
          D[0].r = CLAMP( r, 0x00, 0xFF );
          D[0].g = CLAMP( g, 0x00, 0xFF );
          D[0].b = CLAMP( b, 0x00, 0xFF );

          y1 = y_for_rgb[y1];
          r  = y1 + cr_for_r[cr];
          g  = y1 + cr_for_g[cr] + cb_for_g[cb];
          b  = y1 + cb_for_b[cb];

          D[1].a = 0xFF;
          D[1].r = CLAMP( r, 0x00, 0xFF );
          D[1].g = CLAMP( g, 0x00, 0xFF );
          D[1].b = CLAMP( b, 0x00, 0xFF );

          D += 2;
          i += SperD;
          j += SperD << 1;
     }
}

static void Sop_ayuv_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
		  __u32 a0, y, cb, cr;
          __s32 r, g, b;

          __u32 s = S[i>>16];

          a0 = (s & 0xFF000000) >> 24;
          y  = (s & 0x00FF0000) >> 16;
          cb = (s & 0x0000FF00) >>  8;
          cr = (s & 0x000000FF);

          y = y_for_rgb[y];
          r  = y + cr_for_r[cr];
          g  = y + cr_for_g[cr] + cb_for_g[cb];
          b  = y + cb_for_b[cb];

          D->a = a0;
          D->r = CLAMP( r, 0x00, 0xFF );
          D->g = CLAMP( g, 0x00, 0xFF );
          D->b = CLAMP( b, 0x00, 0xFF );

		  i += SperD;

          D++;
     }
}

static void Sop_uyvy_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length/2;
     int i     = 0;
     int j     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 y0, cb, y1, cr;
          __s32 r, g, b;

#ifdef WORDS_BIGENDIAN
          cb = (S[i>>16] & 0x00FF0000) >> 16;
          cr = (S[i>>16] & 0x000000FF);
#else
          cb = (S[i>>16] & 0x000000FF);
          cr = (S[i>>16] & 0x00FF0000) >> 16;
#endif
          y0 = (((__u16*)S)[j>>16]         & 0xFF00) >> 8;
          y1 = (((__u16*)S)[(j+SperD)>>16] & 0xFF00) >> 8;

          y0 = y_for_rgb[y0];
          r  = y0 + cr_for_r[cr];
          g  = y0 + cr_for_g[cr] + cb_for_g[cb];
          b  = y0 + cb_for_b[cb];

          D[0].a = 0xFF;
          D[0].r = CLAMP( r, 0x00, 0xFF );
          D[0].g = CLAMP( g, 0x00, 0xFF );
          D[0].b = CLAMP( b, 0x00, 0xFF );

          y1 = y_for_rgb[y1];
          r  = y1 + cr_for_r[cr];
          g  = y1 + cr_for_g[cr] + cb_for_g[cb];
          b  = y1 + cb_for_b[cb];

          D[1].a = 0xFF;
          D[1].r = CLAMP( r, 0x00, 0xFF );
          D[1].g = CLAMP( g, 0x00, 0xFF );
          D[1].b = CLAMP( b, 0x00, 0xFF );

          D += 2;
          i += SperD;
          j += SperD << 1;
     }
}

static void Sop_argb2554_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     while (w--) {
          __u16 s = S[i>>16];

          D->a = EXPAND_2to8((s         ) >> 14);
          D->r = EXPAND_5to8((s & 0x3E00) >>  9);
          D->g = EXPAND_5to8((s & 0x01F0) >>  4);
          D->b = EXPAND_4to8((s & 0x000F)      );

          i += SperD;

          D++;
     }
}

static void Sop_argb4444_Sto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     while (w--) {
          __u16 s = S[i>>16];

          D->a = EXPAND_4to8((s         ) >> 12);
          D->r = EXPAND_4to8((s & 0x0F00) >>  8);
          D->g = EXPAND_4to8((s & 0x00F0) >>  4);
          D->b = EXPAND_4to8((s & 0x000F)      );

          i += SperD;

          D++;
     }
}

static GenefxFunc Sop_PFI_Sto_Dacc[DFB_NUM_PIXELFORMATS] = {
     Sop_argb1555_Sto_Dacc,        /* DSPF_ARGB1555 */
     Sop_rgb16_Sto_Dacc,           /* DSPF_RGB16 */
     Sop_rgb24_Sto_Dacc,           /* DSPF_RGB24 */
     Sop_rgb32_Sto_Dacc,           /* DSPF_RGB32 */
     Sop_argb_Sto_Dacc,            /* DSPF_ARGB */
     Sop_a8_Sto_Dacc,              /* DSPF_A8 */
     Sop_yuy2_Sto_Dacc,            /* DSPF_YUY2 */
     Sop_rgb332_Sto_Dacc,          /* DSPF_RGB332 */
     Sop_uyvy_Sto_Dacc,            /* DSPF_UYVY */
     NULL,                         /* DSPF_I420 */
     NULL,                         /* DSPF_YV12 */
     Sop_lut8_Sto_Dacc,            /* DSPF_LUT8 */
     Sop_alut44_Sto_Dacc,          /* DSPF_ALUT44 */
     Sop_airgb_Sto_Dacc,           /* DSPF_AiRGB */
     NULL,                         /* DSPF_A1 */
     NULL,                         /* DSPF_NV12 */
     NULL,                         /* DSPF_NV16 */
     Sop_argb2554_Sto_Dacc,        /* DSPF_ARGB2554 */
     Sop_argb4444_Sto_Dacc,        /* DSPF_ARGB4444 */
     NULL,                         /* DSPF_NV21 */
     Sop_ayuv_Sto_Dacc,            /* DSPF_AYUV */
     NULL,                         /* DSPF_ALUT88 */
     NULL,                         /* DSPF_ABGR */
     NULL,                         /* DSPF_LUT4 */
     NULL,                         /* DSPF_LUT2 */
     NULL,                         /* DSPF_LUT1 */
};

/********************************* Sop_PFI_SKto_Dacc **************************/

static void Sop_argb1555_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = S[i>>16];

          if ((s & 0x7fff) != Skey) {
               D->a = (s & 0x8000) ? 0xff : 0;
               D->r = (s & 0x7C00) >> 7;
               D->g = (s & 0x03E0) >> 2;
               D->b = (s & 0x001F) << 3;
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_rgb16_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = S[i>>16];

          if (s != Skey) {
               D->a = 0xFF;
               D->r = (s & 0xF800) >> 8;
               D->g = (s & 0x07E0) >> 3;
               D->b = (s & 0x001F) << 3;
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_rgb24_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          int pixelstart = (i>>16)*3;

          __u8 b = S[pixelstart+0];
          __u8 g = S[pixelstart+1];
          __u8 r = S[pixelstart+2];

          if (Skey != (__u32)(r<<16 | g<<8 | b )) {
               D->a = 0xFF;
               D->r = r;
               D->g = g;
               D->b = b;
          }
          else
               D->a = 0xFF00;

          i += SperD;

          D++;
     }
}

static void Sop_rgb32_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u32             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u32 s = S[i>>16] & 0x00FFFFFF;

          if (s != Skey) {
               D->a = 0xFF;
               D->r = (s & 0x00FF0000) >> 16;
               D->g = (s & 0x0000FF00) >>  8;
               D->b = (s & 0x000000FF);
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}


static void Sop_argb_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u32             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u32 s = S[i>>16];

          if ((s & 0xffffff) != Skey) {
               D->a = (s & 0xFF000000) >> 24;
               D->r = (s & 0x00FF0000) >> 16;
               D->g = (s & 0x0000FF00) >>  8;
               D->b = (s & 0x000000FF);
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_airgb_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u32             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u32 s = S[i>>16];

          if ((s & 0xffffff) != Skey) {
               D->a = ((s & 0xFF000000) >> 24) ^ 0xff;
               D->r = (s & 0x00FF0000) >> 16;
               D->g = (s & 0x0000FF00) >>  8;
               D->b = (s & 0x000000FF);
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_a8_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     /* no color to key */
     while (w--) {
          __u8 s = S[i>>16];

          D->a = s;
          D->r = 0xFF;
          D->g = 0xFF;
          D->b = 0xFF;

          i += SperD;

          D++;
     }
}

static void Sop_lut8_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = S[i>>16];

          if (s != Skey) {
               D->a = entries[s].a;
               D->r = entries[s].r;
               D->g = entries[s].g;
               D->b = entries[s].b;
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_alut44_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = S[i>>16];

          if ((s & 0x0F) != Skey) {
               D->a = ((s & 0xF0) >> 4) | (s & 0xF0);
               s &= 0x0F;
               D->r = entries[s].r;
               D->g = entries[s].g;
               D->b = entries[s].b;
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_rgb332_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u8               Skey = gfxs->Skey;

     while (w--) {
          __u8 s = S[i>>16];

          if (s != Skey) {
               D->a = 0xFF;
               D->r = EXPAND_3to8(s >> 5);
               D->g = EXPAND_3to8((s & 0x1C) >> 2);
               D->b = EXPAND_2to8(s & 0x03);
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_yuy2_SKto_Dacc( GenefxState *gfxs )
{
     int   w     = gfxs->length/2;
     int   i     = 0;
     int   j     = 0;
     int   SperD = gfxs->SperD;
     __u32 Ky    = (gfxs->Skey & 0x000000FF);
#ifdef WORDS_BIGENDIAN
     __u32 Kcb   = (gfxs->Skey & 0xFF000000) >> 24;
     __u32 Kcr   = (gfxs->Skey & 0x0000FF00) >>  8;
#else
     __u32 Kcb   = (gfxs->Skey & 0x0000FF00) >>  8;
     __u32 Kcr   = (gfxs->Skey & 0xFF000000) >> 24;
#endif

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 y0, cb, y1, cr;
          __s32 r, g, b;

#ifdef WORDS_BIGENDIAN
          cb = (S[i>>16] & 0xFF000000) >> 24;
          cr = (S[i>>16] & 0x0000FF00) >>  8;
#else
          cb = (S[i>>16] & 0x0000FF00) >>  8;
          cr = (S[i>>16] & 0xFF000000) >> 24;
#endif
          y0 = ((__u16*)S)[j>>16]         & 0x00FF;
          y1 = ((__u16*)S)[(j+SperD)>>16] & 0x00FF;

          if (y0 != Ky || cb != Kcb || cr != Kcr) {
               y0 = y_for_rgb[y0];
               r  = y0 + cr_for_r[cr];
               g  = y0 + cr_for_g[cr] + cb_for_g[cb];
               b  = y0 + cb_for_b[cb];

               D[0].a = 0xFF;
               D[0].r = CLAMP( r, 0x00, 0xFF );
               D[0].g = CLAMP( g, 0x00, 0xFF );
               D[0].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[0].a = 0xF000;

          if (y0 != Ky || cb != Kcb || cr != Kcr) {
               y1 = y_for_rgb[y1];
               r  = y1 + cr_for_r[cr];
               g  = y1 + cr_for_g[cr] + cb_for_g[cb];
               b  = y1 + cb_for_b[cb];

               D[1].a = 0xFF;
               D[1].r = CLAMP( r, 0x00, 0xFF );
               D[1].g = CLAMP( g, 0x00, 0xFF );
               D[1].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[1].a = 0xF000;

          D += 2;
          i += SperD;
          j += SperD << 1;
     }
}

static void Sop_uyvy_SKto_Dacc( GenefxState *gfxs )
{
     int   w     = gfxs->length/2;
     int   i     = 0;
     int   j     = 0;
     int   SperD = gfxs->SperD;
     __u32 Ky    = (gfxs->Skey & 0x0000FF00) >>  8;
#ifdef WORDS_BIGENDIAN
     __u32 Kcb   = (gfxs->Skey & 0x00FF0000) >> 16;
     __u32 Kcr   = (gfxs->Skey & 0x000000FF);
#else
     __u32 Kcb   = (gfxs->Skey & 0x000000FF);
     __u32 Kcr   = (gfxs->Skey & 0x00FF0000) >> 16;
#endif

     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 y0, cb, y1, cr;
          __s32 r, g, b;

#ifdef WORDS_BIGENDIAN
          cb = (S[i>>16] & 0x00FF0000) >> 16;
          cr = (S[i>>16] & 0x000000FF);
#else
          cb = (S[i>>16] & 0x000000FF);
          cr = (S[i>>16] & 0x00FF0000) >> 16;
#endif
          y0 = (((__u16*)S)[j>>16]         & 0xFF00) >> 8;
          y1 = (((__u16*)S)[(j+SperD)>>16] & 0xFF00) >> 8;

          if (y0 != Ky || cb != Kcb || cr != Kcr) {
               y0 = y_for_rgb[y0];
               r  = y0 + cr_for_r[cr];
               g  = y0 + cr_for_g[cr] + cb_for_g[cb];
               b  = y0 + cb_for_b[cb];

               D[0].a = 0xFF;
               D[0].r = CLAMP( r, 0x00, 0xFF );
               D[0].g = CLAMP( g, 0x00, 0xFF );
               D[0].b = CLAMP( b, 0x00, 0xFF );
          }
          else
              D[0].a = 0xF000;

          if (y0 != Ky || cb != Kcb || cr != Kcr) {
               y1 = y_for_rgb[y1];
               r  = y1 + cr_for_r[cr];
               g  = y1 + cr_for_g[cr] + cb_for_g[cb];
               b  = y1 + cb_for_b[cb];

               D[1].a = 0xFF;
               D[1].r = CLAMP( r, 0x00, 0xFF );
               D[1].g = CLAMP( g, 0x00, 0xFF );
               D[1].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[1].a = 0xF000;

          D += 2;
          i += SperD;
          j += SperD << 1;
     }
}

static void Sop_argb2554_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u16              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = S[i>>16];

          if ((s & 0x3FFF) != Skey) {
               D->a = EXPAND_2to8((s         ) >> 14);
               D->r = EXPAND_5to8((s & 0x3E00) >>  9);
               D->g = EXPAND_5to8((s & 0x01F0) >>  4);
               D->b = EXPAND_4to8((s & 0x000F)      );
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static void Sop_argb4444_SKto_Dacc( GenefxState *gfxs )
{
     int w     = gfxs->length;
     int i     = 0;
     int SperD = gfxs->SperD;

     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u16              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = S[i>>16];

          if ((s & 0x0FFF) != Skey) {
               D->a = EXPAND_4to8((s         ) >> 12);
               D->r = EXPAND_4to8((s & 0x0F00) >>  8);
               D->g = EXPAND_4to8((s & 0x00F0) >>  4);
               D->b = EXPAND_4to8((s & 0x000F)      );
          }
          else
               D->a = 0xF000;

          i += SperD;

          D++;
     }
}

static GenefxFunc Sop_PFI_SKto_Dacc[DFB_NUM_PIXELFORMATS] = {
     Sop_argb1555_SKto_Dacc,       /* DSPF_ARGB1555 */
     Sop_rgb16_SKto_Dacc,          /* DSPF_RGB16 */
     Sop_rgb24_SKto_Dacc,          /* DSPF_RGB24 */
     Sop_rgb32_SKto_Dacc,          /* DSPF_RGB32 */
     Sop_argb_SKto_Dacc,           /* DSPF_ARGB */
     Sop_a8_SKto_Dacc,             /* DSPF_A8 */
     Sop_yuy2_SKto_Dacc,           /* DSPF_YUY2 */
     Sop_rgb332_SKto_Dacc,         /* DSPF_RGB332 */
     Sop_uyvy_SKto_Dacc,           /* DSPF_UYVY */
     NULL,                         /* DSPF_I420 */
     NULL,                         /* DSPF_YV12 */
     Sop_lut8_SKto_Dacc,           /* DSPF_LUT8 */
     Sop_alut44_SKto_Dacc,         /* DSPF_ALUT44 */
     Sop_airgb_SKto_Dacc,          /* DSPF_AiRGB */
     NULL,                         /* DSPF_A1 */
     NULL,                         /* DSPF_NV12 */
     NULL,                         /* DSPF_NV16 */
     Sop_argb2554_SKto_Dacc,       /* DSPF_ARGB2554 */
     Sop_argb4444_SKto_Dacc,       /* DSPF_ARGB4444 */
     NULL,                         /* DSPF_NV21 */
     NULL,                         /* DSPF_AYUV */
     NULL,                         /* DSPF_ALUT88 */
     NULL,                         /* DSPF_ABGR */
     NULL,                         /* DSPF_LUT4 */
     NULL,                         /* DSPF_LUT2 */
     NULL,                         /* DSPF_LUT1 */
};

/********************************* Sop_PFI_to_Dacc ****************************/

static void Sop_argb1555_to_Dacc( GenefxState *gfxs )
{
     int                l;
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     if (((long)S)&2) {
          __u16 spixel = *S;

          D->a = (spixel & 0x8000) ? 0xff : 0;
          D->r = (spixel & 0x7C00) >> 7;
          D->g = (spixel & 0x03E0) >> 2;
          D->b = (spixel & 0x001F) << 3;

          ++S;
          ++D;
          --w;
     }

     l = w >> 1;
     while (l) {
          __u32 spixel2 = *((__u32*)S);

#ifdef WORDS_BIGENDIAN
          D[0].a = (spixel2 & 0x80000000) ? 0xff : 0;
          D[0].r = (spixel2 & 0x7C000000) >> 23;
          D[0].g = (spixel2 & 0x03E00000) >> 18;
          D[0].b = (spixel2 & 0x001F0000) >> 13;

          D[1].a = (spixel2 & 0x8000) ? 0xff : 0;
          D[1].r = (spixel2 & 0x7C00) >> 7;
          D[1].g = (spixel2 & 0x03E0) >> 2;
          D[1].b = (spixel2 & 0x001F) << 3;
#else
          D[0].a = (spixel2 & 0x8000) ? 0xff : 0;
          D[0].r = (spixel2 & 0x7C00) >> 7;
          D[0].g = (spixel2 & 0x03E0) >> 2;
          D[0].b = (spixel2 & 0x001F) << 3;

          D[1].a = (spixel2 & 0x80000000) ? 0xff : 0;
          D[1].r = (spixel2 & 0x7C000000) >> 23;
          D[1].g = (spixel2 & 0x03E00000) >> 18;
          D[1].b = (spixel2 & 0x001F0000) >> 13;
#endif

          S += 2;
          D += 2;

          --l;
     }

     if (w&1) {
          __u16 spixel = *S;

          D->a = (spixel & 0x8000) ? 0xff : 0;
          D->r = (spixel & 0x7C00) >> 7;
          D->g = (spixel & 0x03E0) >> 2;
          D->b = (spixel & 0x001F) << 3;
     }
}

static void Sop_rgb16_to_Dacc( GenefxState *gfxs )
{
     int                l;
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     if (((long)S)&2) {
          __u16 spixel = *S;

          D->a = 0xFF;
          D->r = (spixel & 0xF800) >> 8;
          D->g = (spixel & 0x07E0) >> 3;
          D->b = (spixel & 0x001F) << 3;

          ++S;
          ++D;
          --w;
     }

     l = w >> 1;
     while (l) {
          __u32 spixel2 = *((__u32*)S);

#ifdef WORDS_BIGENDIAN
          D[0].a = 0xFF;
          D[0].r = (spixel2 & 0xF8000000) >> 24;
          D[0].g = (spixel2 & 0x07E00000) >> 19;
          D[0].b = (spixel2 & 0x001F0000) >> 13;

          D[1].a = 0xFF;
          D[1].r = (spixel2 & 0xF800) >> 8;
          D[1].g = (spixel2 & 0x07E0) >> 3;
          D[1].b = (spixel2 & 0x001F) << 3;
#else
          D[0].a = 0xFF;
          D[0].r = (spixel2 & 0xF800) >> 8;
          D[0].g = (spixel2 & 0x07E0) >> 3;
          D[0].b = (spixel2 & 0x001F) << 3;

          D[1].a = 0xFF;
          D[1].r = (spixel2 & 0xF8000000) >> 24;
          D[1].g = (spixel2 & 0x07E00000) >> 19;
          D[1].b = (spixel2 & 0x001F0000) >> 13;
#endif

          S += 2;
          D += 2;

          --l;
     }

     if (w&1) {
          __u16 spixel = *S;

          D->a = 0xFF;
          D->r = (spixel & 0xF800) >> 8;
          D->g = (spixel & 0x07E0) >> 3;
          D->b = (spixel & 0x001F) << 3;
     }
}

static void Sop_rgb24_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     while (w--) {
          D->a = 0xFF;
          D->b = *S++;
          D->g = *S++;
          D->r = *S++;

          D++;
     }
}

static void Sop_a8_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     while (w--) {
          D->a = *S++;
          D->r = 0xFF;
          D->g = 0xFF;
          D->b = 0xFF;

          D++;
     }
}

static void Sop_rgb32_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 s = *S++;

          D->a = 0xFF;
          D->r = (s & 0xFF0000) >> 16;
          D->g = (s & 0x00FF00) >>  8;
          D->b = (s & 0x0000FF);

          D++;
     }
}

static void Sop_argb_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 s = *S++;

          D->a = (s & 0xFF000000) >> 24;
          D->r = (s & 0x00FF0000) >> 16;
          D->g = (s & 0x0000FF00) >>  8;
          D->b = (s & 0x000000FF);

          D++;
     }
}

static void Sop_airgb_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 s = *S++;

          D->a = ((s & 0xFF000000) >> 24) ^ 0xff;
          D->r = (s & 0x00FF0000) >> 16;
          D->g = (s & 0x0000FF00) >>  8;
          D->b = (s & 0x000000FF);

          D++;
     }
}

static void Sop_rgb332_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     while (w--) {
          __u8 s = *S++;

          D->a = 0xFF;
          D->r = EXPAND_3to8(s >> 5);
          D->g = EXPAND_3to8((s & 0x1C) >> 2);
          D->b = EXPAND_2to8(s & 0x03);

          D++;
     }
}

static void Sop_lut8_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = *S++;

          D->a = entries[s].a;
          D->r = entries[s].r;
          D->g = entries[s].g;
          D->b = entries[s].b;

          D++;
     }
}

static void Sop_alut44_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = *S++;

          D->a = s & 0xF0;
          s &= 0x0F;
          D->r = entries[s].r;
          D->g = entries[s].g;
          D->b = entries[s].b;

          D++;
     }
}

static void Sop_yuy2_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length/2;
     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 y0, cb, y1, cr;
          __s32 r, g, b;

#ifdef WORDS_BIGENDIAN
          y0 = (*S & 0x00FF0000) >> 16;
          cb = (*S & 0xFF000000) >> 24;
          y1 = (*S & 0x000000FF);
          cr = (*S & 0x0000FF00) >>  8;
#else
          y0 = (*S & 0x000000FF);
          cb = (*S & 0x0000FF00) >>  8;
          y1 = (*S & 0x00FF0000) >> 16;
          cr = (*S & 0xFF000000) >> 24;
#endif

          y0 = y_for_rgb[y0];
          r  = y0 + cr_for_r[cr];
          g  = y0 + cr_for_g[cr] + cb_for_g[cb];
          b  = y0 + cb_for_b[cb];

          D[0].a = 0xFF;
          D[0].r = CLAMP( r, 0x00, 0xFF );
          D[0].g = CLAMP( g, 0x00, 0xFF );
          D[0].b = CLAMP( b, 0x00, 0xFF );

          y1 = y_for_rgb[y1];
          r  = y1 + cr_for_r[cr];
          g  = y1 + cr_for_g[cr] + cb_for_g[cb];
          b  = y1 + cb_for_b[cb];

          D[1].a = 0xFF;
          D[1].r = CLAMP( r, 0x00, 0xFF );
          D[1].g = CLAMP( g, 0x00, 0xFF );
          D[1].b = CLAMP( b, 0x00, 0xFF );

          S++;
          D += 2;
     }
}

static void Sop_uyvy_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length/2;
     GenefxAccumulator *D = gfxs->Dacc;
     __u32             *S = gfxs->Sop;

     while (w--) {
          __u32 y0, cb, y1, cr;
          __s32 r, g, b;

#ifdef WORDS_BIGENDIAN
          cb = (*S & 0x00FF0000) >> 16;
          y0 = (*S & 0xFF000000) >> 24;
          cr = (*S & 0x000000FF);
          y1 = (*S & 0x0000FF00) >> 8;
#else
          cb = (*S & 0x000000FF);
          y0 = (*S & 0x0000FF00) >> 8;
          cr = (*S & 0x00FF0000) >> 16;
          y1 = (*S & 0xFF000000) >> 24;
#endif

          y0 = y_for_rgb[y0];
          r  = y0 + cr_for_r[cr];
          g  = y0 + cr_for_g[cr] + cb_for_g[cb];
          b  = y0 + cb_for_b[cb];

          D[0].a = 0xFF;
          D[0].r = CLAMP( r, 0x00, 0xFF );
          D[0].g = CLAMP( g, 0x00, 0xFF );
          D[0].b = CLAMP( b, 0x00, 0xFF );

          y1 = y_for_rgb[y1];
          r  = y1 + cr_for_r[cr];
          g  = y1 + cr_for_g[cr] + cb_for_g[cb];
          b  = y1 + cb_for_b[cb];

          D[1].a = 0xFF;
          D[1].r = CLAMP( r, 0x00, 0xFF );
          D[1].g = CLAMP( g, 0x00, 0xFF );
          D[1].b = CLAMP( b, 0x00, 0xFF );

          S++;
          D += 2;
     }
}

static void Sop_argb2554_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     while (w--) {
          __u16 s = *S++;

          D->a = EXPAND_2to8((s         ) >> 14);
          D->r = EXPAND_5to8((s & 0x3E00) >>  9);
          D->g = EXPAND_5to8((s & 0x01F0) >>  4);
          D->b = EXPAND_4to8((s & 0x000F)      );

          D++;
     }
}

static void Sop_argb4444_to_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u16             *S = gfxs->Sop;

     while (w--) {
          __u16 s = *S++;

          D->a = EXPAND_4to8((s         ) >> 12);
          D->r = EXPAND_4to8((s & 0x0F00) >>  8);
          D->g = EXPAND_4to8((s & 0x00F0) >>  4);
          D->b = EXPAND_4to8((s & 0x000F)      );

          D++;
     }
}

static GenefxFunc Sop_PFI_to_Dacc[DFB_NUM_PIXELFORMATS] = {
     Sop_argb1555_to_Dacc,         /* DSPF_ARGB1555 */
     Sop_rgb16_to_Dacc,            /* DSPF_RGB16 */
     Sop_rgb24_to_Dacc,            /* DSPF_RGB24 */
     Sop_rgb32_to_Dacc,            /* DSPF_RGB32 */
     Sop_argb_to_Dacc,             /* DSPF_ARGB */
     Sop_a8_to_Dacc,               /* DSPF_A8 */
     Sop_yuy2_to_Dacc,             /* DSPF_YUY2 */
     Sop_rgb332_to_Dacc,           /* DSPF_RGB332 */
     Sop_uyvy_to_Dacc,             /* DSPF_UYVY */
     NULL,                         /* DSPF_I420 */
     NULL,                         /* DSPF_YV12 */
     Sop_lut8_to_Dacc,             /* DSPF_LUT8 */
     Sop_alut44_to_Dacc,           /* DSPF_ALUT44 */
     Sop_airgb_to_Dacc,            /* DSPF_AiRGB */
     NULL,                         /* DSPF_A1 */
     NULL,                         /* DSPF_NV12 */
     NULL,                         /* DSPF_NV16 */
     Sop_argb2554_to_Dacc,         /* DSPF_ARGB2554 */
     Sop_argb4444_to_Dacc,         /* DSPF_ARGB4444 */
     NULL,                         /* DSPF_NV21 */
     NULL,                         /* DSPF_AYUV */
     NULL,                         /* DSPF_ALUT88 */
     NULL,                         /* DSPF_ABGR */
     NULL,                         /* DSPF_LUT4 */
     NULL,                         /* DSPF_LUT2 */
     NULL,                         /* DSPF_LUT1 */
};

/********************************* Sop_PFI_Kto_Dacc ***************************/

static void Sop_argb1555_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = *S++;

          if ((s & 0x7fff) != Skey) {
               D->a = (s & 0x8000) ? 0xff : 0;
               D->r = (s & 0x7C00) >> 7;
               D->g = (s & 0x03E0) >> 2;
               D->b = (s & 0x001F) << 3;
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_rgb16_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = *S++;

          if (s != Skey) {
               D->a = 0xFF;
               D->r = (s & 0xF800) >> 8;
               D->g = (s & 0x07E0) >> 3;
               D->b = (s & 0x001F) << 3;
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_rgb24_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u8 b = *S++;
          __u8 g = *S++;
          __u8 r = *S++;

          if (Skey != (__u32)(r<<16 | g<<8 | b )) {
               D->a = 0xFF;
               D->r = r;
               D->g = g;
               D->b = b;
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_rgb32_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u32             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u32 s = *S++ & 0x00FFFFFF;

          if (s != Skey) {
               D->a = 0xFF;
               D->r = s >> 16;
               D->g = (s & 0x00FF00) >>  8;
               D->b = (s & 0x0000FF);
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_argb_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u32             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u32 s = *S++;

          if ((s & 0xFFFFFF) != Skey) {
               D->a = s >> 24;
               D->r = (s & 0x00FF0000) >> 16;
               D->g = (s & 0x0000FF00) >>  8;
               D->b = (s & 0x000000FF);
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_airgb_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u32             *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u32 s = *S++;

          if ((s & 0xFFFFFF) != Skey) {
               D->a = (s >> 24) ^ 0xff;
               D->r = (s & 0x00FF0000) >> 16;
               D->g = (s & 0x0000FF00) >>  8;
               D->b = (s & 0x000000FF);
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_a8_Kto_Dacc( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     __u8              *S = gfxs->Sop;

     /* no color to key */
     while (w--) {
          D->a = *S++;
          D->r = 0xFF;
          D->g = 0xFF;
          D->b = 0xFF;

          D++;
     }
}

static void Sop_rgb332_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     while (w--) {
          __u8 s = *S++;

          if (s != Skey) {
               D->a = 0xFF;
               D->r = EXPAND_3to8(s >> 5);
               D->g = EXPAND_3to8((s & 0x1C) >> 2);
               D->b = EXPAND_2to8(s & 0x03);
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_lut8_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = *S++;

          if (s != Skey) {
               D->a = entries[s].a;
               D->r = entries[s].r;
               D->g = entries[s].g;
               D->b = entries[s].b;
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_alut44_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u8              *S    = gfxs->Sop;
     __u32              Skey = gfxs->Skey;

     DFBColor *entries = gfxs->Slut->entries;

     while (w--) {
          __u8 s = *S++;

          if ((s & 0x0F) != Skey) {
               D->a = ((s & 0xF0) >> 4) | (s & 0xF0);
               s &= 0x0F;
               D->r = entries[s].r;
               D->g = entries[s].g;
               D->b = entries[s].b;
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_yuy2_Kto_Dacc( GenefxState *gfxs )
{
     int                w  = gfxs->length/2;
     GenefxAccumulator *D  = gfxs->Dacc;
     __u32             *S  = gfxs->Sop;
     __u32              K0;
     __u32              K1;

#ifdef WORDS_BIGENDIAN
#define P0_MASK  0xFFFFFF00
#define P1_MASK  0xFF00FFFF
#else
#define P0_MASK  0xFF00FFFF
#define P1_MASK  0xFFFFFF00
#endif

     K0 = gfxs->Skey & P0_MASK;
     K1 = gfxs->Skey & P1_MASK;

     while (w--) {
          __u32 Spix;
          __u32 y, cb, cr;
          __s32 r, g, b;

          Spix = *S++;
#ifdef WORDS_BIGENDIAN
          cb = (Spix & 0xFF000000) >> 24;
          cr = (Spix & 0x0000FF00) >>  8;
#else
          cb = (Spix & 0x0000FF00) >>  8;
          cr = (Spix & 0xFF000000) >> 24;
#endif

          if ((Spix & P0_MASK) != K0) {
#ifdef WORDS_BIGENDIAN
               y = y_for_rgb[(Spix & 0x00FF0000)>>16];
#else
               y = y_for_rgb[(Spix & 0x000000FF)];
#endif
               r = y + cr_for_r[cr];
               g = y + cr_for_g[cr] + cb_for_g[cb];
               b = y + cb_for_b[cb];

               D[0].a = 0xFF;
               D[0].r = CLAMP( r, 0x00, 0xFF );
               D[0].g = CLAMP( g, 0x00, 0xFF );
               D[0].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[0].a = 0xF000;

          if ((Spix & P1_MASK) != K1) {
#ifdef WORDS_BIGENDIAN
               y = y_for_rgb[(Spix & 0x000000FF)];
#else
               y = y_for_rgb[(Spix & 0x00FF0000)>>16];
#endif
               r = y + cr_for_r[cr];
               g = y + cr_for_g[cr] + cb_for_g[cb];
               b = y + cb_for_b[cb];

               D[1].a = 0xFF;
               D[1].r = CLAMP( r, 0x00, 0xFF );
               D[1].g = CLAMP( g, 0x00, 0xFF );
               D[1].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[1].a = 0xF000;

          D += 2;
     }
#undef P0_MASK
#undef P1_MASK
}

static void Sop_uyvy_Kto_Dacc( GenefxState *gfxs )
{
     int                w  = gfxs->length/2;
     GenefxAccumulator *D  = gfxs->Dacc;
     __u32             *S  = gfxs->Sop;
     __u32              K0;
     __u32              K1;

#ifdef WORDS_BIGENDIAN
#define P0_MASK 0xFFFF00FF
#define P1_MASK 0x00FFFFFF
#else
#define P0_MASK 0x00FFFFFF
#define P1_MASK 0xFFFF00FF
#endif

     K0 = gfxs->Skey & P0_MASK;
     K1 = gfxs->Skey & P1_MASK;

     while (w--) {
          __u32 Spix;
          __u32 y, cb, cr;
          __s32 r, g, b;

          Spix = *S++;
#ifdef WORDS_BIGENDIAN
          cb = (Spix & 0x00FF0000) >> 16;
          cr = (Spix & 0x000000FF);
#else
          cb = (Spix & 0x000000FF);
          cr = (Spix & 0x00FF0000) >> 16;
#endif

          if ((Spix & P0_MASK) != K0) {
#ifdef WORDS_BIGENDIAN
               y = y_for_rgb[(Spix & 0xFF000000)>>24];
#else
               y = y_for_rgb[(Spix & 0x0000FF00)>>8];
#endif
               r = y + cr_for_r[cr];
               g = y + cr_for_g[cr] + cb_for_g[cb];
               b = y + cb_for_b[cb];

               D[0].a = 0xFF;
               D[0].r = CLAMP( r, 0x00, 0xFF );
               D[0].g = CLAMP( g, 0x00, 0xFF );
               D[0].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[0].a = 0xF000;

          if ((Spix & P1_MASK) != K1) {
#ifdef WORDS_BIGENDIAN
               y = y_for_rgb[(Spix & 0x0000FF00)>>8];
#else
               y = y_for_rgb[(Spix & 0xFF000000)>>24];
#endif
               r = y + cr_for_r[cr];
               g = y + cr_for_g[cr] + cb_for_g[cb];
               b = y + cb_for_b[cb];

               D[1].a = 0xFF;
               D[1].r = CLAMP( r, 0x00, 0xFF );
               D[1].g = CLAMP( g, 0x00, 0xFF );
               D[1].b = CLAMP( b, 0x00, 0xFF );
          }
          else
               D[1].a = 0xF000;

          D += 2;
     }
#undef P0_MASK
#undef P1_MASK
}

static void Sop_argb2554_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u16              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = *S++;

          if ((s & 0x3FFF) != Skey) {
               D->a = EXPAND_2to8((s         ) >> 14);
               D->r = EXPAND_5to8((s & 0x3E00) >>  9);
               D->g = EXPAND_5to8((s & 0x01F0) >>  4);
               D->b = EXPAND_4to8((s & 0x000F)      );
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static void Sop_argb4444_Kto_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     __u16             *S    = gfxs->Sop;
     __u16              Skey = gfxs->Skey;

     while (w--) {
          __u16 s = *S++;

          if ((s & 0x0FFF) != Skey) {
               D->a = EXPAND_4to8((s         ) >> 12);
               D->r = EXPAND_4to8((s & 0x0F00) >>  8);
               D->g = EXPAND_4to8((s & 0x00F0) >>  4);
               D->b = EXPAND_4to8((s & 0x000F)      );
          }
          else
               D->a = 0xF000;

          D++;
     }
}

static GenefxFunc Sop_PFI_Kto_Dacc[DFB_NUM_PIXELFORMATS] = {
     Sop_argb1555_Kto_Dacc,        /* DSPF_ARGB1555 */
     Sop_rgb16_Kto_Dacc,           /* DSPF_RGB16 */
     Sop_rgb24_Kto_Dacc,           /* DSPF_RGB24 */
     Sop_rgb32_Kto_Dacc,           /* DSPF_RGB32 */
     Sop_argb_Kto_Dacc,            /* DSPF_ARGB */
     Sop_a8_Kto_Dacc,              /* DSPF_A8 */
     Sop_yuy2_Kto_Dacc,            /* DSPF_YUY2 */
     Sop_rgb332_Kto_Dacc,          /* DSPF_RGB332 */
     Sop_uyvy_Kto_Dacc,            /* DSPF_UYVY */
     NULL,                         /* DSPF_I420 */
     NULL,                         /* DSPF_YV12 */
     Sop_lut8_Kto_Dacc,            /* DSPF_LUT8 */
     Sop_alut44_Kto_Dacc,          /* DSPF_ALUT44 */
     Sop_airgb_Kto_Dacc,           /* DSPF_AiRGB */
     NULL,                         /* DSPF_A1 */
     NULL,                         /* DSPF_NV12 */
     NULL,                         /* DSPF_NV16 */
     Sop_argb2554_Kto_Dacc,        /* DSPF_ARGB2554 */
     Sop_argb4444_Kto_Dacc,        /* DSPF_ARGB4444 */
     NULL,                         /* DSPF_NV21 */
     NULL,                         /* DSPF_AYUV */
     NULL,                         /* DSPF_ALUT88 */
     NULL,                         /* DSPF_ABGR */
     NULL,                         /* DSPF_LUT4 */
     NULL,                         /* DSPF_LUT2 */
     NULL,                         /* DSPF_LUT1 */
};

/********************************* Sacc_to_Aop_PFI ****************************/

static void Sacc_to_Aop_argb1555( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u16             *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_ARGB1555( (S->a & 0xFF00) ? 0xFF : S->a,
                                    (S->r & 0xFF00) ? 0xFF : S->r,
                                    (S->g & 0xFF00) ? 0xFF : S->g,
                                    (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_rgb16( GenefxState *gfxs )
{
     int                l;
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u16             *D = gfxs->Aop;

     if ((long) D & 2) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_RGB16( (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }

          ++S;
          ++D;
          --w;
     }

     l = w >> 1;
     while (l) {
          __u32 *D2 = (__u32*) D;

          if (!(S[0].a & 0xF000) && !(S[1].a & 0xF000)) {
#ifdef WORDS_BIGENDIAN
               *D2 = PIXEL_RGB16( (S[1].r & 0xFF00) ? 0xFF : S[1].r,
                                  (S[1].g & 0xFF00) ? 0xFF : S[1].g,
                                  (S[1].b & 0xFF00) ? 0xFF : S[1].b ) |
                     PIXEL_RGB16( (S[0].r & 0xFF00) ? 0xFF : S[0].r,
                                  (S[0].g & 0xFF00) ? 0xFF : S[0].g,
                                  (S[0].b & 0xFF00) ? 0xFF : S[0].b ) << 16;
#else
               *D2 = PIXEL_RGB16( (S[0].r & 0xFF00) ? 0xFF : S[0].r,
                                  (S[0].g & 0xFF00) ? 0xFF : S[0].g,
                                  (S[0].b & 0xFF00) ? 0xFF : S[0].b ) |
                     PIXEL_RGB16( (S[1].r & 0xFF00) ? 0xFF : S[1].r,
                                  (S[1].g & 0xFF00) ? 0xFF : S[1].g,
                                  (S[1].b & 0xFF00) ? 0xFF : S[1].b ) << 16;
#endif
          }
          else {
               if (!(S[0].a & 0xF000)) {
                    D[0] = PIXEL_RGB16( (S[0].r & 0xFF00) ? 0xFF : S[0].r,
                                        (S[0].g & 0xFF00) ? 0xFF : S[0].g,
                                        (S[0].b & 0xFF00) ? 0xFF : S[0].b );
               }
               else
                    if (!(S[1].a & 0xF000)) {
                    D[1] = PIXEL_RGB16( (S[1].r & 0xFF00) ? 0xFF : S[1].r,
                                        (S[1].g & 0xFF00) ? 0xFF : S[1].g,
                                        (S[1].b & 0xFF00) ? 0xFF : S[1].b );
               }
          }

          S += 2;
          D += 2;

          --l;
     }

     if (w & 1) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_RGB16( (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }
     }
}

static void Sacc_to_Aop_rgb24( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D++ = (S->b & 0xFF00) ? 0xFF : S->b;
               *D++ = (S->g & 0xFF00) ? 0xFF : S->g;
               *D++ = (S->r & 0xFF00) ? 0xFF : S->r;
          }
          else
               D += 3;

          S++;
     }
}

static void Sacc_to_Aop_rgb32( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u32             *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_RGB32( (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_argb( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u32             *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_ARGB( (S->a & 0xFF00) ? 0xFF : S->a,
                                (S->r & 0xFF00) ? 0xFF : S->r,
                                (S->g & 0xFF00) ? 0xFF : S->g,
                                (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_airgb( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u32             *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_AiRGB( (S->a & 0xFF00) ? 0xFF : S->a,
                                 (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_a8( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000))
               *D = (S->a & 0xFF00) ? 0xFF : S->a;

          D++;
          S++;
     }
}

static void Sacc_to_Aop_rgb332( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_RGB332( (S->r & 0xFF00) ? 0xFF : S->r,
                                  (S->g & 0xFF00) ? 0xFF : S->g,
                                  (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_lut8( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = dfb_palette_search( gfxs->Alut,
                                        (S->r & 0xFF00) ? 0xFF : S->r,
                                        (S->g & 0xFF00) ? 0xFF : S->g,
                                        (S->b & 0xFF00) ? 0xFF : S->b,
                                        (S->a & 0xFF00) ? 0xFF : S->a );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_alut44( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = (S->a & 0xFF00) ? 0xF0 : (S->a & 0xF0) +
                    dfb_palette_search( gfxs->Alut,
                                        (S->r & 0xFF00) ? 0xFF : S->r,
                                        (S->g & 0xFF00) ? 0xFF : S->g,
                                        (S->b & 0xFF00) ? 0xFF : S->b,
                                        0x80 );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_argb2554( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u16             *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_ARGB2554( (S->a & 0xFF00) ? 0xFF : S->a,
                                    (S->r & 0xFF00) ? 0xFF : S->r,
                                    (S->g & 0xFF00) ? 0xFF : S->g,
                                    (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_to_Aop_argb4444( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u16             *D = gfxs->Aop;

     while (w--) {
          if (!(S->a & 0xF000)) {
               *D = PIXEL_ARGB4444( (S->a & 0xFF00) ? 0xFF : S->a,
                                    (S->r & 0xFF00) ? 0xFF : S->r,
                                    (S->g & 0xFF00) ? 0xFF : S->g,
                                    (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static GenefxFunc Sacc_to_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Sacc_to_Aop_argb1555,         /* DSPF_ARGB1555 */
     Sacc_to_Aop_rgb16,            /* DSPF_RGB16 */
     Sacc_to_Aop_rgb24,            /* DSPF_RGB24 */
     Sacc_to_Aop_rgb32,            /* DSPF_RGB32 */
     Sacc_to_Aop_argb,             /* DSPF_ARGB */
     Sacc_to_Aop_a8,               /* DSPF_A8 */
     NULL,                         /* DSPF_YUY2 */
     Sacc_to_Aop_rgb332,           /* DSPF_RGB332 */
     NULL,                         /* DSPF_UYVY */
     NULL,                         /* DSPF_I420 */
     NULL,                         /* DSPF_YV12 */
     Sacc_to_Aop_lut8,             /* DSPF_LUT8 */
     Sacc_to_Aop_alut44,           /* DSPF_ALUT44 */
     Sacc_to_Aop_airgb,            /* DSPF_AiRGB */
     NULL,                         /* DSPF_A1 */
     NULL,                         /* DSPF_NV12 */
     NULL,                         /* DSPF_NV16 */
     Sacc_to_Aop_argb2554,         /* DSPF_ARGB2554 */
     Sacc_to_Aop_argb4444,         /* DSPF_ARGB4444 */
     NULL,                         /* DSPF_NV21 */
     NULL,                         /* DSPF_AYUV */
     NULL,                         /* DSPF_ALUT88 */
     NULL,                         /* DSPF_ABGR */
     NULL,                         /* DSPF_LUT4 */
     NULL,                         /* DSPF_LUT2 */
     NULL,                         /* DSPF_LUT1 */
};

/******************************** Sacc_toK_Aop_PFI ****************************/

static void Sacc_toK_Aop_argb1555( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u16             *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0x7fff) == Dkey)) {
               *D = PIXEL_ARGB1555( (S->a & 0xFF00) ? 0xFF : S->a,
                                    (S->r & 0xFF00) ? 0xFF : S->r,
                                    (S->g & 0xFF00) ? 0xFF : S->g,
                                    (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_rgb16( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u16             *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && (*D == Dkey)) {
               *D = PIXEL_RGB16( (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_rgb24( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;
     __u8               r = (gfxs->Dkey >> 16);
     __u8               g = (gfxs->Dkey >>  8) & 0xff;
     __u8               b = (gfxs->Dkey      ) & 0xff;

     while (w--) {
          if (!(S->a & 0xF000) && D[0] == b && D[1] == g && D[2] == r) {
               *D++ = (S->b & 0xFF00) ? 0xFF : S->b;
               *D++ = (S->g & 0xFF00) ? 0xFF : S->g;
               *D++ = (S->r & 0xFF00) ? 0xFF : S->r;
          }
          else
               D += 3;

          S++;
     }
}

static void Sacc_toK_Aop_rgb32( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u32             *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0xffffff) == Dkey)) {
               *D = PIXEL_RGB32( (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_argb( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u32             *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0xffffff) == Dkey)) {
               *D = PIXEL_ARGB( (S->a & 0xFF00) ? 0xFF : S->a,
                                (S->r & 0xFF00) ? 0xFF : S->r,
                                (S->g & 0xFF00) ? 0xFF : S->g,
                                (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_airgb( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u32             *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0xffffff) == Dkey)) {
               *D = PIXEL_AiRGB( (S->a & 0xFF00) ? 0xFF : S->a,
                                 (S->r & 0xFF00) ? 0xFF : S->r,
                                 (S->g & 0xFF00) ? 0xFF : S->g,
                                 (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_a8( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     __u8              *D = gfxs->Aop;

     /* FIXME: do all or do none? */
     while (w--) {
          if (!(S->a & 0xF000))
               *D = (S->a & 0xFF00) ? 0xFF : S->a;

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_rgb332( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u8              *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && (*D == Dkey)) {
               *D = PIXEL_RGB332( (S->r & 0xFF00) ? 0xFF : S->r,
                                  (S->g & 0xFF00) ? 0xFF : S->g,
                                  (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_lut8( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u8              *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && (*D == Dkey)) {
               *D = dfb_palette_search( gfxs->Alut,
                                        (S->r & 0xFF00) ? 0xFF : S->r,
                                        (S->g & 0xFF00) ? 0xFF : S->g,
                                        (S->b & 0xFF00) ? 0xFF : S->b,
                                        (S->a & 0xFF00) ? 0xFF : S->a );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_alut44( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u8              *D    = gfxs->Aop;
     __u32              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0x0F) == Dkey)) {
               *D = (S->a & 0xFF00) ? 0xF0 : (S->a & 0xF0) +
                    dfb_palette_search( gfxs->Alut,
                                        (S->r & 0xFF00) ? 0xFF : S->r,
                                        (S->g & 0xFF00) ? 0xFF : S->g,
                                        (S->b & 0xFF00) ? 0xFF : S->b,
                                        0x80 );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_argb2554( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u16             *D    = gfxs->Aop;
     __u16              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0x3FFF) == Dkey)) {
               *D = PIXEL_ARGB2554( (S->a & 0xFF00) ? 0xFF : S->a,
                                    (S->r & 0xFF00) ? 0xFF : S->r,
                                    (S->g & 0xFF00) ? 0xFF : S->g,
                                    (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static void Sacc_toK_Aop_argb4444( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *S    = gfxs->Sacc;
     __u16             *D    = gfxs->Aop;
     __u16              Dkey = gfxs->Dkey;

     while (w--) {
          if (!(S->a & 0xF000) && ((*D & 0x0FFF) == Dkey)) {
               *D = PIXEL_ARGB4444( (S->a & 0xFF00) ? 0xFF : S->a,
                                    (S->r & 0xFF00) ? 0xFF : S->r,
                                    (S->g & 0xFF00) ? 0xFF : S->g,
                                    (S->b & 0xFF00) ? 0xFF : S->b );
          }

          D++;
          S++;
     }
}

static GenefxFunc Sacc_toK_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Sacc_toK_Aop_argb1555,        /* DSPF_ARGB1555 */
     Sacc_toK_Aop_rgb16,           /* DSPF_RGB16 */
     Sacc_toK_Aop_rgb24,           /* DSPF_RGB24 */
     Sacc_toK_Aop_rgb32,           /* DSPF_RGB32 */
     Sacc_toK_Aop_argb,            /* DSPF_ARGB */
     Sacc_toK_Aop_a8,              /* DSPF_A8 */
     NULL,                         /* DSPF_YUY2 */
     Sacc_toK_Aop_rgb332,          /* DSPF_RGB332 */
     NULL,                         /* DSPF_UYVY */
     NULL,                         /* DSPF_I420 */
     NULL,                         /* DSPF_YV12 */
     Sacc_toK_Aop_lut8,            /* DSPF_LUT8 */
     Sacc_toK_Aop_alut44,          /* DSPF_ALUT44 */
     Sacc_toK_Aop_airgb,           /* DSPF_AiRGB */
     NULL,                         /* DSPF_A1 */
     NULL,                         /* DSPF_NV12 */
     NULL,                         /* DSPF_NV16 */
     Sacc_toK_Aop_argb2554,        /* DSPF_ARGB2554 */
     Sacc_toK_Aop_argb4444,        /* DSPF_ARGB4444 */
     NULL,                         /* DSPF_NV21 */
     NULL,                         /* DSPF_AYUV */
     NULL,                         /* DSPF_ALUT88 */
     NULL,                         /* DSPF_ABGR */
     NULL,                         /* DSPF_LUT4 */
     NULL,                         /* DSPF_LUT2 */
     NULL,                         /* DSPF_LUT1 */
};

/************** Bop_a8_set_alphapixel_Aop_PFI *********************************/

#define DUFF_1(format) \
               case 1:\
                    SET_ALPHA_PIXEL_##format( D[0], S[0] );

#define DUFF_2(format) \
               case 3:\
                    SET_ALPHA_PIXEL_##format( D[2], S[2] );\
               case 2:\
                    SET_ALPHA_PIXEL_##format( D[1], S[1] );\
               DUFF_1(format)

#define DUFF_3(format) \
               case 7:\
                    SET_ALPHA_PIXEL_##format( D[6], S[6] );\
               case 6:\
                    SET_ALPHA_PIXEL_##format( D[5], S[5] );\
               case 5:\
                    SET_ALPHA_PIXEL_##format( D[4], S[4] );\
               case 4:\
                    SET_ALPHA_PIXEL_##format( D[3], S[3] );\
               DUFF_2(format)

#define DUFF_4(format) \
               case 15:\
                    SET_ALPHA_PIXEL_##format( D[14], S[14] );\
               case 14:\
                    SET_ALPHA_PIXEL_##format( D[13], S[13] );\
               case 13:\
                    SET_ALPHA_PIXEL_##format( D[12], S[12] );\
               case 12:\
                    SET_ALPHA_PIXEL_##format( D[11], S[11] );\
               case 11:\
                    SET_ALPHA_PIXEL_##format( D[10], S[10] );\
               case 10:\
                    SET_ALPHA_PIXEL_##format( D[9], S[9] );\
               case 9:\
                    SET_ALPHA_PIXEL_##format( D[8], S[8] );\
               case 8:\
                    SET_ALPHA_PIXEL_##format( D[7], S[7] );\
               DUFF_3(format)

#define SET_ALPHA_PIXEL_DUFFS_DEVICE_N(D, S, w, format, n) \
     while (w) {\
          register int l = w & ((1 << n) - 1);\
          switch (l) {\
               default:\
                    l = (1 << n);\
                    SET_ALPHA_PIXEL_##format( D[(1 << n)-1], S[(1 << n)-1] );\
               DUFF_##n(format)\
          }\
          D += l;\
          S += l;\
          w -= l;\
     }

/* change the last value to adjust the size of the device (1-4) */
#define SET_ALPHA_PIXEL_DUFFS_DEVICE(D, S, w, format) \
          SET_ALPHA_PIXEL_DUFFS_DEVICE_N(D, S, w, format, 3)


static void Bop_a8_set_alphapixel_Aop_argb1555( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u16 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;
     __u32  rb  = Cop & 0x7c1f;
     __u32  g   = Cop & 0x03e0;

#define SET_ALPHA_PIXEL_ARGB1555(d,a) \
     switch (a) {\
          case 0xff: d = Cop;\
          case 0: break;\
          default: {\
               register __u32  s = (a>>3)+1;\
               register __u32 t1 = (d & 0x7c1f);\
               register __u32 t2 = (d & 0x03e0);\
               d = (((d >> 8) | (a & 0x80)) << 8) | \
                   ((((rb-t1)*s+(t1<<5)) & 0x000f83e0) + \
                    ((( g-t2)*s+(t2<<5)) & 0x00007c00)) >> 5;\
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, ARGB1555 );

#undef SET_ALPHA_PIXEL_ARGB1555
}


static void Bop_a8_set_alphapixel_Aop_rgb16( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u16 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;
     __u32  rb  = Cop & 0xf81f;
     __u32  g   = Cop & 0x07e0;

#define SET_ALPHA_PIXEL_RGB16(d,a)\
     switch (a) {\
          case 0xff: d = Cop;\
          case 0: break;\
          default: {\
               register __u32  s = (a>>2)+1;\
               register __u32 t1 = (d & 0xf81f);\
               register __u32 t2 = (d & 0x07e0);\
               d  = ((((rb-t1)*s+(t1<<6)) & 0x003e07c0) + \
                     ((( g-t2)*s+(t2<<6)) & 0x0001f800)) >> 6;\
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, RGB16 );

#undef SET_ALPHA_PIXEL_RGB16
}

static void Bop_a8_set_alphapixel_Aop_rgb24( GenefxState *gfxs )
{
     int       w     = gfxs->length;
     __u8     *S     = gfxs->Bop;
     __u8     *D     = gfxs->Aop;
     DFBColor  color = gfxs->color;

#define SET_ALPHA_PIXEL_RGB24(d,r,g,b,a)\
     switch (a) {\
         case 0xff:\
               d[0] = b;\
               d[1] = g;\
               d[2] = r;\
          case 0: break;\
          default: {\
               register __u16 s = a+1;\
               d[0] = ((b-d[0]) * s + (d[0] << 8)) >> 8;\
               d[1] = ((g-d[1]) * s + (d[1] << 8)) >> 8;\
               d[2] = ((r-d[2]) * s + (d[2] << 8)) >> 8;\
          }\
     }

     while (w>4) {
          SET_ALPHA_PIXEL_RGB24( D, color.r, color.g, color.b, *S ); D+=3; S++;
          SET_ALPHA_PIXEL_RGB24( D, color.r, color.g, color.b, *S ); D+=3; S++;
          SET_ALPHA_PIXEL_RGB24( D, color.r, color.g, color.b, *S ); D+=3; S++;
          SET_ALPHA_PIXEL_RGB24( D, color.r, color.g, color.b, *S ); D+=3; S++;
          w-=4;
     }
     while (w--) {
          SET_ALPHA_PIXEL_RGB24( D, color.r, color.g, color.b, *S ); D+=3, S++;
     }

#undef SET_ALPHA_PIXEL_RGB24
}

static void Bop_a8_set_alphapixel_Aop_rgb32( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;
     __u32  rb  = Cop & 0xff00ff;
     __u32  g   = Cop & 0x00ff00;

#define SET_ALPHA_PIXEL_RGB32(d,a)\
     switch (a) {\
          case 0xff: d = Cop;\
          case 0: break;\
          default: {\
               register __u32  s = a+1;\
               register __u32 t1 = (d & 0x00ff00ff);\
               register __u32 t2 = (d & 0x0000ff00);\
               d = ((((rb-t1)*s+(t1<<8)) & 0xff00ff00) + \
                    ((( g-t2)*s+(t2<<8)) & 0x00ff0000)) >> 8;\
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, RGB32 );

#undef SET_ALPHA_PIXEL_RGB32
}


/* saturating alpha blend */

static void Bop_a8_set_alphapixel_Aop_argb( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop | 0xff000000;
     __u32  rb  = Cop & 0x00ff00ff;
     __u32  g   = gfxs->color.g;

#define SET_ALPHA_PIXEL_ARGB(d,a)\
     switch (a) {\
          case 0xff: d = Cop;\
          case 0: break;\
          default: {\
               register __u32  s = a+1;\
               register __u32 s1 = 256-s;\
               register __u32 sa = (d >> 24) + a;\
               if (sa & 0xff00) sa = 0xff;\
               d = (sa << 24) + \
                    (((((d & 0x00ff00ff)       * s1) + (rb  * s)) >> 8) & 0x00ff00ff) + \
                    (((((d & 0x0000ff00) >> 8) * s1) + ((g) * s))       & 0x0000ff00);  \
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, ARGB );

#undef SET_ALPHA_PIXEL_ARGB
}

static void Bop_a8_set_alphapixel_Aop_airgb( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;
     __u32  rb  = Cop & 0x00ff00ff;
     __u32  g   = gfxs->color.g;

#define SET_ALPHA_PIXEL_AiRGB(d,a)\
     switch (a) {\
          case 0xff: d = Cop;\
          case 0: break;\
          default: {\
               register __u32  s = a+1;\
               register __u32 s1 = 256-s;\
               register __s32 sa = (d >> 24) - a;\
               if (sa < 0) sa = 0;\
               d = (sa << 24) + \
                    (((((d & 0x00ff00ff)       * s1) + (rb  * s)) >> 8) & 0x00ff00ff) + \
                    (((((d & 0x0000ff00) >> 8) * s1) + ((g) * s))       & 0x0000ff00);  \
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, AiRGB );

#undef SET_ALPHA_PIXEL_AiRGB
}

static void Bop_a8_set_alphapixel_Aop_a8( GenefxState *gfxs )
{
     int    w = gfxs->length;
     __u8  *S = gfxs->Bop;
     __u8  *D = gfxs->Aop;

#define SET_ALPHA_PIXEL_A8(d,a)\
     switch (a) {\
          case 0xff: d = 0xff;\
          case 0: break; \
          default: {\
               register __u16 s  = (a)+1;\
               register __u16 s1 = 256-s;\
               d = (d * s1 + s) >> 8;\
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, A8 );

#undef SET_ALPHA_PIXEL_A8
}

static void Bop_a8_set_alphapixel_Aop_rgb332( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u8  *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;
     __u32  rgb = ((Cop & 0xe0) << 16) | ((Cop & 0x1c) << 8) | (Cop & 0x03);

#define SET_ALPHA_PIXEL_RGB332(d,a) \
     switch (a) {\
          case 0xff: d = Cop;\
          case 0: break;\
          default: {\
               register __u32 s = a + 1;\
               register __u32 t = ((d & 0xe0) << 16) | ((d & 0x1c) << 8) | (d & 0x03);\
               register __u32 c = ((rgb-t)*s + (t<<8)) & 0xe01c0300;\
               d = (c >> 24) | ((c >> 16) & 0xff) | ((c >> 8) & 0xff);\
          }\
     }

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, RGB332 );
#undef SET_ALPHA_PIXEL_RGB332
}

static void Bop_a8_set_alphapixel_Aop_lut8( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u8  *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;

	 /*PR10573*/
	 bool en = gfxs->stroking_enabled;

#if 0
     DFBColor  color   = gfxs->color;
     DFBColor *entries = gfxs->Alut->entries;

# define SET_ALPHA_PIXEL_LUT8(d,alpha) \
     switch (alpha) {\
          case 0xff: d = Cop;\
          case 0: break; \
          default: {\
               register __u16 s = alpha+1;\
               DFBColor      dc = entries[d];\
               __u16         sa = alpha + dc.a;\
               dc.r = ((color.r - dc.r) * s + (dc.r << 8)) >> 8;\
               dc.g = ((color.g - dc.g) * s + (dc.g << 8)) >> 8;\
               dc.b = ((color.b - dc.b) * s + (dc.b << 8)) >> 8;\
               d = dfb_palette_search( gfxs->Alut, dc.r, dc.g, dc.b,\
                                       sa & 0xff00 ? 0xff : sa );\
          }\
     }
#else	/*PR10573*/
# define SET_ALPHA_PIXEL_LUT8(d,a) \
     if (en) \ 
          d = a;	\
     else	\
     if (a & 0x80) \
          d = Cop;
		  	
	 	
#endif

     SET_ALPHA_PIXEL_DUFFS_DEVICE( D, S, w, LUT8 );
#undef SET_ALPHA_PIXEL_LUT8
}

static void Bop_a8_set_alphapixel_Aop_alut44( GenefxState *gfxs )
{
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u8  *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;

     DFBColor  color   = gfxs->color;
     DFBColor *entries = gfxs->Alut->entries;

#define SET_ALPHA_PIXEL_ALUT44(d,alpha) \
     switch (alpha) {\
          case 0xff: d = Cop;\
          case 0: break; \
          default: {\
               register __u16 s = alpha+1;\
               DFBColor      dc = entries[d & 0x0f];\
               __u16         sa = (d & 0xf0) + alpha;\
               dc.r = ((color.r - dc.r) * s + (dc.r << 8)) >> 8;\
               dc.g = ((color.g - dc.g) * s + (dc.g << 8)) >> 8;\
               dc.b = ((color.b - dc.b) * s + (dc.b << 8)) >> 8;\
               if (sa & 0xff00) sa = 0xf0;\
               d = (sa & 0xf0) + \
                    dfb_palette_search( gfxs->Alut, dc.r, dc.g, dc.b, 0x80 );\
          }\
     }

     while (w--) {
          SET_ALPHA_PIXEL_ALUT44( *D, *S );
          D++, S++;
     }

#undef SET_ALPHA_PIXEL_ALUT44
}

static GenefxFunc Bop_a8_set_alphapixel_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_a8_set_alphapixel_Aop_argb1555,          /* DSPF_ARGB1555 */
     Bop_a8_set_alphapixel_Aop_rgb16,             /* DSPF_RGB16 */
     Bop_a8_set_alphapixel_Aop_rgb24,             /* DSPF_RGB24 */
     Bop_a8_set_alphapixel_Aop_rgb32,             /* DSPF_RGB32 */
     Bop_a8_set_alphapixel_Aop_argb,              /* DSPF_ARGB */
     Bop_a8_set_alphapixel_Aop_a8,                /* DSPF_A8 */
     NULL,                                        /* DSPF_YUY2 */
     Bop_a8_set_alphapixel_Aop_rgb332,            /* DSPF_RGB332 */
     NULL,                                        /* DSPF_UYVY */
     NULL,                                        /* DSPF_I420 */
     NULL,                                        /* DSPF_YV12 */
     Bop_a8_set_alphapixel_Aop_lut8,              /* DSPF_LUT8 */
     Bop_a8_set_alphapixel_Aop_alut44,            /* DSPF_ALUT44 */
     Bop_a8_set_alphapixel_Aop_airgb,             /* DSPF_AiRGB */
     NULL,                                        /* DSPF_A1 */
     NULL,                                        /* DSPF_NV12 */
     NULL,                                        /* DSPF_NV16 */
     NULL,                                        /* DSPF_ARGB2554 */
     NULL,                                        /* DSPF_ARGB4444 */
     NULL,                                        /* DSPF_NV21 */
     NULL,                                        /* DSPF_AYUV */
     NULL,                                        /* DSPF_ALUT88 */
     NULL,                                        /* DSPF_ABGR */
     NULL,                                        /* DSPF_LUT4 */
     NULL,                                        /* DSPF_LUT2 */
     NULL,                                        /* DSPF_LUT1 */
};

/************** Bop_a1_set_alphapixel_Aop_PFI *********************************/

static void Bop_a1_set_alphapixel_Aop_argb1555( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u16 *D   = gfxs->Aop;
     __u16  Cop = gfxs->Cop | 0x8000;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}


static void Bop_a1_set_alphapixel_Aop_rgb16( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u16 *D   = gfxs->Aop;
     __u16  Cop = gfxs->Cop;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_rgb24( GenefxState *gfxs )
{
     int       i;
     int       w     = gfxs->length;
     __u8     *S     = gfxs->Bop;
     __u8     *D     = gfxs->Aop;
     DFBColor  color = gfxs->color;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7))) {
               D[0] = color.b;
               D[1] = color.g;
               D[2] = color.r;
          }

          D += 3;
     }
}

static void Bop_a1_set_alphapixel_Aop_rgb32( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_argb( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop | 0xFF000000;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_airgb( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u32 *D   = gfxs->Aop;
     __u32  Cop = gfxs->Cop & 0x00FFFFFF;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_a8( GenefxState *gfxs )
{
     int   i;
     int   w = gfxs->length;
     __u8 *S = gfxs->Bop;
     __u8 *D = gfxs->Aop;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = 0xff;
     }
}

static void Bop_a1_set_alphapixel_Aop_rgb332( GenefxState *gfxs )
{
     int   i;
     int   w   = gfxs->length;
     __u8 *S   = gfxs->Bop;
     __u8 *D   = gfxs->Aop;
     __u8  Cop = gfxs->Cop;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_lut8( GenefxState *gfxs )
{
     int   i;
     int   w   = gfxs->length;
     __u8 *S   = gfxs->Bop;
     __u8 *D   = gfxs->Aop;
     __u8  Cop = gfxs->Cop;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_alut44( GenefxState *gfxs )
{
     int   i;
     int   w   = gfxs->length;
     __u8 *S   = gfxs->Bop;
     __u8 *D   = gfxs->Aop;
     __u8  Cop = gfxs->Cop;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_argb2554( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u16 *D   = gfxs->Aop;
     __u16  Cop = gfxs->Cop | 0xC000;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static void Bop_a1_set_alphapixel_Aop_argb4444( GenefxState *gfxs )
{
     int    i;
     int    w   = gfxs->length;
     __u8  *S   = gfxs->Bop;
     __u16 *D   = gfxs->Aop;
     __u16  Cop = gfxs->Cop | 0xF000;

     for (i=0; i<w; i++) {
          if (S[i>>3] & (0x80 >> (i&7)))
               D[i] = Cop;
     }
}

static GenefxFunc Bop_a1_set_alphapixel_Aop_PFI[DFB_NUM_PIXELFORMATS] = {
     Bop_a1_set_alphapixel_Aop_argb1555,          /* DSPF_ARGB1555 */
     Bop_a1_set_alphapixel_Aop_rgb16,             /* DSPF_RGB16 */
     Bop_a1_set_alphapixel_Aop_rgb24,             /* DSPF_RGB24 */
     Bop_a1_set_alphapixel_Aop_rgb32,             /* DSPF_RGB32 */
     Bop_a1_set_alphapixel_Aop_argb,              /* DSPF_ARGB */
     Bop_a1_set_alphapixel_Aop_a8,                /* DSPF_A8 */
     NULL,                                        /* DSPF_YUY2 */
     Bop_a1_set_alphapixel_Aop_rgb332,            /* DSPF_RGB332 */
     NULL,                                        /* DSPF_UYVY */
     NULL,                                        /* DSPF_I420 */
     NULL,                                        /* DSPF_YV12 */
     Bop_a1_set_alphapixel_Aop_lut8,              /* DSPF_LUT8 */
     Bop_a1_set_alphapixel_Aop_alut44,            /* DSPF_ALUT44 */
     Bop_a1_set_alphapixel_Aop_airgb,             /* DSPF_AiRGB */
     NULL,                                        /* DSPF_A1 */
     NULL,                                        /* DSPF_NV12 */
     NULL,                                        /* DSPF_NV16 */
     Bop_a1_set_alphapixel_Aop_argb2554,          /* DSPF_ARGB2554 */
     Bop_a1_set_alphapixel_Aop_argb4444,          /* DSPF_ARGB4444 */
     NULL,                                        /* DSPF_NV21 */
     NULL,                                        /* DSPF_AYUV */
     NULL,                                        /* DSPF_ALUT88 */
     NULL,                                        /* DSPF_ABGR */
     NULL,                                        /* DSPF_LUT4 */
     NULL,                                        /* DSPF_LUT2 */
     NULL,                                        /* DSPF_LUT1 */
};


/********************************* Xacc_blend *********************************/

static void Xacc_blend_zero( GenefxState *gfxs )
{
     int                i;
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;

     for (i=0; i<w; i++) {
          if (!(X[i].a & 0xF000))
               X[i].a = X[i].r = X[i].g = X[i].b = 0;
     }
}

static void Xacc_blend_one( GenefxState *gfxs )
{
}

static void Xacc_blend_srccolor( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;

     if (gfxs->Sacc) {
          GenefxAccumulator *S = gfxs->Sacc;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    X->r = ((S->r + 1) * X->r) >> 8;
                    X->g = ((S->g + 1) * X->g) >> 8;
                    X->b = ((S->b + 1) * X->b) >> 8;
                    X->a = ((S->a + 1) * X->a) >> 8;
               }

               X++;
               S++;
          }
     }
     else {
          GenefxAccumulator Cacc = gfxs->Cacc;
          Cacc.r = Cacc.r + 1;
          Cacc.g = Cacc.g + 1;
          Cacc.b = Cacc.b + 1;
          Cacc.a = Cacc.a + 1;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    X->r = (Cacc.r * X->r) >> 8;
                    X->g = (Cacc.g * X->g) >> 8;
                    X->b = (Cacc.b * X->b) >> 8;
                    X->a = (Cacc.a * X->a) >> 8;
               }

               X++;
          }
     }
}

static void Xacc_blend_invsrccolor( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;

     if (gfxs->Sacc) {
          GenefxAccumulator *S = gfxs->Sacc;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    X->r = ((0x100 - S->r) * X->r) >> 8;
                    X->g = ((0x100 - S->g) * X->g) >> 8;
                    X->b = ((0x100 - S->b) * X->b) >> 8;
                    X->a = ((0x100 - S->a) * X->a) >> 8;
               }

               X++;
               S++;
          }
     }
     else {
          GenefxAccumulator Cacc = gfxs->Cacc;
          Cacc.r = 0x100 - Cacc.r;
          Cacc.g = 0x100 - Cacc.g;
          Cacc.b = 0x100 - Cacc.b;
          Cacc.a = 0x100 - Cacc.a;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    X->r = (Cacc.r * X->r) >> 8;
                    X->g = (Cacc.g * X->g) >> 8;
                    X->b = (Cacc.b * X->b) >> 8;
                    X->a = (Cacc.a * X->a) >> 8;
               }

               X++;
          }
     }
}

static void Xacc_blend_srcalpha( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;

     if (gfxs->Sacc) {
          GenefxAccumulator *S = gfxs->Sacc;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    register __u16 Sa = S->a + 1;

                    X->r = (Sa * X->r) >> 8;
                    X->g = (Sa * X->g) >> 8;
                    X->b = (Sa * X->b) >> 8;
                    X->a = (Sa * X->a) >> 8;
               }

               X++;
               S++;
          }
     }
     else {
          register __u16 Sa = gfxs->color.a + 1;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    X->r = (Sa * X->r) >> 8;
                    X->g = (Sa * X->g) >> 8;
                    X->b = (Sa * X->b) >> 8;
                    X->a = (Sa * X->a) >> 8;
               }

               X++;
          }
     }
}

static void Xacc_blend_invsrcalpha( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;

     if (gfxs->Sacc) {
          GenefxAccumulator *S = gfxs->Sacc;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    register __u16 Sa = 0x100 - S->a;

                    X->r = (Sa * X->r) >> 8;
                    X->g = (Sa * X->g) >> 8;
                    X->b = (Sa * X->b) >> 8;
                    X->a = (Sa * X->a) >> 8;
               }

               X++;
               S++;
          }
     }
     else {
          register __u16 Sa = 0x100 - gfxs->color.a;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    X->a = (Sa * X->a) >> 8;
                    X->r = (Sa * X->r) >> 8;
                    X->g = (Sa * X->g) >> 8;
                    X->b = (Sa * X->b) >> 8;
               }

               X++;
          }
     }
}

static void Xacc_blend_dstalpha( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(X->a & 0xF000)) {
               register __u16 Da = D->a + 1;

               X->r = (Da * X->r) >> 8;
               X->g = (Da * X->g) >> 8;
               X->b = (Da * X->b) >> 8;
               X->a = (Da * X->a) >> 8;
          }

          X++;
          D++;
     }
}

static void Xacc_blend_invdstalpha( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(X->a & 0xF000)) {
               register __u16 Da = 0x100 - D->a;

               X->r = (Da * X->r) >> 8;
               X->g = (Da * X->g) >> 8;
               X->b = (Da * X->b) >> 8;
               X->a = (Da * X->a) >> 8;
          }

          X++;
          D++;
     }
}

static void Xacc_blend_destcolor( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(X->a & 0xF000)) {
               X->r = ((D->r + 1) * X->r) >> 8;
               X->g = ((D->g + 1) * X->g) >> 8;
               X->b = ((D->b + 1) * X->b) >> 8;
               X->a = ((D->a + 1) * X->a) >> 8;
          }

          X++;
          D++;
     }
}

static void Xacc_blend_invdestcolor( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(X->a & 0xF000)) {
               X->r = ((0x100 - D->r) * X->r) >> 8;
               X->g = ((0x100 - D->g) * X->g) >> 8;
               X->b = ((0x100 - D->b) * X->b) >> 8;
               X->a = ((0x100 - D->a) * X->a) >> 8;
          }

          X++;
          D++;
     }
}

static void Xacc_blend_srcalphasat( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *X = gfxs->Xacc;
     GenefxAccumulator *D = gfxs->Dacc;

     if (gfxs->Sacc) {
          GenefxAccumulator *S = gfxs->Sacc;

          while (w--) {
               if (!(X->a & 0xF000)) {
                    register __u16 Sa = MIN( S->a + 1, 0x100 - D->a );

                    X->r = (Sa * X->r) >> 8;
                    X->g = (Sa * X->g) >> 8;
                    X->b = (Sa * X->b) >> 8;
               }

               X++;
               D++;
               S++;
          }
     }
     else {
          while (w--) {
               if (!(X->a & 0xF000)) {
                    register __u16 Sa = MIN( gfxs->color.a + 1, 0x100 - D->a );

                    X->r = (Sa * X->r) >> 8;
                    X->g = (Sa * X->g) >> 8;
                    X->b = (Sa * X->b) >> 8;
               }

               X++;
               D++;
          }
     }
}

static GenefxFunc Xacc_blend[] = {
     Xacc_blend_zero,         /* DSBF_ZERO         */
     Xacc_blend_one,          /* DSBF_ONE          */
     Xacc_blend_srccolor,     /* DSBF_SRCCOLOR     */
     Xacc_blend_invsrccolor,  /* DSBF_INVSRCCOLOR  */
     Xacc_blend_srcalpha,     /* DSBF_SRCALPHA     */
     Xacc_blend_invsrcalpha,  /* DSBF_INVSRCALPHA  */
     Xacc_blend_dstalpha,     /* DSBF_DESTALPHA    */
     Xacc_blend_invdstalpha,  /* DSBF_INVDESTALPHA */
     Xacc_blend_destcolor,    /* DSBF_DESTCOLOR    */
     Xacc_blend_invdestcolor, /* DSBF_INVDESTCOLOR */
     Xacc_blend_srcalphasat   /* DSBF_SRCALPHASAT  */
};

/********************************* Dacc_modulation ****************************/

static void Dacc_set_alpha( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     int                a = gfxs->color.a;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a = a;
          }

          D++;
     }
}

static void Dacc_modulate_alpha( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;
     int                a = gfxs->Cacc.a;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a = (a * D->a) >> 8;
          }

          D++;
     }
}

static void Dacc_modulate_rgb( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     GenefxAccumulator  Cacc = gfxs->Cacc;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->r = (Cacc.r * D->r) >> 8;
               D->g = (Cacc.g * D->g) >> 8;
               D->b = (Cacc.b * D->b) >> 8;
          }

          D++;
     }
}

static void Dacc_modulate_rgb_set_alpha( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     GenefxAccumulator  Cacc = gfxs->Cacc;
     int                a    = gfxs->color.a;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a = a;
               D->r = (Cacc.r * D->r) >> 8;
               D->g = (Cacc.g * D->g) >> 8;
               D->b = (Cacc.b * D->b) >> 8;
          }

          D++;
     }
}

static void Dacc_modulate_argb( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     GenefxAccumulator  Cacc = gfxs->Cacc;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a = (Cacc.a * D->a) >> 8;
               D->r = (Cacc.r * D->r) >> 8;
               D->g = (Cacc.g * D->g) >> 8;
               D->b = (Cacc.b * D->b) >> 8;
          }

          D++;
     }
}

static GenefxFunc Dacc_modulation[] = {
     NULL,
     NULL,
     Dacc_set_alpha,
     Dacc_modulate_alpha,
     Dacc_modulate_rgb,
     Dacc_modulate_rgb,
     Dacc_modulate_rgb_set_alpha,
     Dacc_modulate_argb
};

/********************************* misc accumulator operations ****************/

static void Dacc_premultiply( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(D->a & 0xF000)) {
               register __u16 Da = D->a + 1;

               D->r = (Da * D->r) >> 8;
               D->g = (Da * D->g) >> 8;
               D->b = (Da * D->b) >> 8;
          }

          D++;
     }
}

static void Dacc_demultiply( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(D->a & 0xF000)) {
               register __u16 Da = D->a + 1;

               D->r = (D->r << 8) / Da;
               D->g = (D->g << 8) / Da;
               D->b = (D->b << 8) / Da;
          }

          D++;
     }
}

static void Dacc_xor( GenefxState *gfxs )
{
     int                w     = gfxs->length;
     GenefxAccumulator *D     = gfxs->Dacc;
     DFBColor           color = gfxs->color;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a ^= color.a;
               D->r ^= color.r;
               D->g ^= color.g;
               D->b ^= color.b;
          }

          D++;
     }
}

static void Cacc_to_Dacc( GenefxState *gfxs )
{
     int                w    = gfxs->length;
     GenefxAccumulator *D    = gfxs->Dacc;
     GenefxAccumulator  Cacc = gfxs->Cacc;

     while (w--)
          *D++ = Cacc;
}



static void SCacc_add_to_Dacc_C( GenefxState *gfxs )
{
     int                w     = gfxs->length;
     GenefxAccumulator *D     = gfxs->Dacc;
     GenefxAccumulator  SCacc = gfxs->SCacc;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a += SCacc.a;
               D->r += SCacc.r;
               D->g += SCacc.g;
               D->b += SCacc.b;
          }
          D++;
     }
}

static GenefxFunc SCacc_add_to_Dacc = SCacc_add_to_Dacc_C;

static void Sacc_add_to_Dacc_C( GenefxState *gfxs )
{
     int                w = gfxs->length;
     GenefxAccumulator *S = gfxs->Sacc;
     GenefxAccumulator *D = gfxs->Dacc;

     while (w--) {
          if (!(D->a & 0xF000)) {
               D->a += S->a;
               D->r += S->r;
               D->g += S->g;
               D->b += S->b;
          }
          D++;
          S++;
     }
}

static GenefxFunc Sacc_add_to_Dacc = Sacc_add_to_Dacc_C;

static void Sop_is_Aop( GenefxState *gfxs ) { gfxs->Sop = gfxs->Aop;}
static void Sop_is_Bop( GenefxState *gfxs ) { gfxs->Sop = gfxs->Bop;}

static void Slut_is_Alut( GenefxState *gfxs ) { gfxs->Slut = gfxs->Alut;}
static void Slut_is_Blut( GenefxState *gfxs ) { gfxs->Slut = gfxs->Blut;}

static void Sacc_is_NULL( GenefxState *gfxs ) { gfxs->Sacc = NULL;}
static void Sacc_is_Aacc( GenefxState *gfxs ) { gfxs->Sacc = gfxs->Aacc;}
static void Sacc_is_Bacc( GenefxState *gfxs ) { gfxs->Sacc = gfxs->Bacc;}

static void Dacc_is_Aacc( GenefxState *gfxs ) { gfxs->Dacc = gfxs->Aacc;}
static void Dacc_is_Bacc( GenefxState *gfxs ) { gfxs->Dacc = gfxs->Bacc;}

static void Xacc_is_Aacc( GenefxState *gfxs ) { gfxs->Xacc = gfxs->Aacc;}
static void Xacc_is_Bacc( GenefxState *gfxs ) { gfxs->Xacc = gfxs->Bacc;}


/******************************************************************************/

void gGetDriverInfo( GraphicsDriverInfo *info )
{
     snprintf( info->name,
               DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH, "Software Driver" );

#ifdef USE_MMX
     if (direct_mm_accel() & MM_MMX) {
          if (!dfb_config->mmx) {
               D_INFO( "DirectFB/Genefx: MMX detected, but disabled by option 'no-mmx'\n");
          }
          else {
               gInit_MMX();

               snprintf( info->name, DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH,
                         "MMX Software Driver" );

               D_INFO( "DirectFB/Genefx: MMX detected and enabled\n");
          }
     }
     else {
          D_INFO( "DirectFB/Genefx: No MMX detected\n" );
     }
#endif

     snprintf( info->vendor, DFB_GRAPHICS_DRIVER_INFO_VENDOR_LENGTH,
               "convergence integrated media GmbH" );

     info->version.major = 0;
     info->version.minor = 6;
}

void gGetDeviceInfo( GraphicsDeviceInfo *info )
{
     snprintf( info->name, DFB_GRAPHICS_DEVICE_INFO_NAME_LENGTH,
               "Software Rasterizer" );

     snprintf( info->vendor, DFB_GRAPHICS_DEVICE_INFO_VENDOR_LENGTH,
               use_mmx ? "MMX" : "Generic" );

     info->caps.accel    = DFXL_NONE;
     info->caps.flags    = 0;
     info->caps.drawing  = DSDRAW_NOFX;
     info->caps.blitting = DSBLIT_NOFX;
}

#define MODULATION_FLAGS (DSBLIT_BLEND_ALPHACHANNEL |  \
                          DSBLIT_BLEND_COLORALPHA |    \
                          DSBLIT_COLORIZE |            \
                          DSBLIT_DST_PREMULTIPLY |     \
                          DSBLIT_SRC_PREMULTIPLY |     \
                          DSBLIT_DEMULTIPLY)

bool gAcquire( CardState *state, DFBAccelerationMask accel )
{
     GenefxState *gfxs;
     GenefxFunc  *funcs;
     int          dst_pfi;
     int          src_pfi     = 0;
     CoreSurface *destination = state->destination;
     CoreSurface *source      = state->source;
     DFBColor     color       = state->color;

     DFBSurfaceLockFlags lock_flags;

     if (software_raster_warning)
         D_WARN( "DirectFB/Genefx: gAcquire was invoked for accel 0x%08X!\n", accel );

     if (!state->gfxs) {
          gfxs = D_CALLOC( 1, sizeof(GenefxState) );
          if (!gfxs) {
               D_ERROR( "DirectFB/Genefx: Couldn't allocate state struct!\n" );
               return false;
          }

          state->gfxs = gfxs;
     }

     gfxs  = state->gfxs;
     funcs = gfxs->funcs;

     /* Destination may have been destroyed. */
     if (!destination)
          return false;

     /* Source may have been destroyed. */
     if (DFB_BLITTING_FUNCTION( accel ) && !source)
          return false;


     gfxs->dst_caps   = destination->caps;
     gfxs->dst_height = destination->height;
     gfxs->dst_format = destination->back_buffer->format;
     gfxs->dst_bpp    = DFB_BYTES_PER_PIXEL( gfxs->dst_format );
     dst_pfi          = DFB_PIXELFORMAT_INDEX( gfxs->dst_format );

     if (DFB_BLITTING_FUNCTION( accel )) {
          gfxs->src_caps   = source->caps;
          gfxs->src_height = source->height;
          gfxs->src_format = source->front_buffer->format;
          gfxs->src_bpp    = DFB_BYTES_PER_PIXEL( gfxs->src_format );
          src_pfi          = DFB_PIXELFORMAT_INDEX( gfxs->src_format );

          lock_flags = state->blittingflags & ( DSBLIT_BLEND_ALPHACHANNEL |
                                                DSBLIT_BLEND_COLORALPHA   |
                                                DSBLIT_DST_COLORKEY ) ?
                       DSLF_READ | DSLF_WRITE : DSLF_WRITE;
     }
     else
          lock_flags = state->drawingflags & ( DSDRAW_BLEND |
                                               DSDRAW_DST_COLORKEY ) ?
                       DSLF_READ | DSLF_WRITE : DSLF_WRITE;

     gfxs->color = color;

     switch (gfxs->dst_format) {
          case DSPF_ARGB1555:
               gfxs->Cop = PIXEL_ARGB1555( color.a, color.r, color.g, color.b );
               break;
          case DSPF_RGB16:
               gfxs->Cop = PIXEL_RGB16( color.r, color.g, color.b );
               break;
          case DSPF_RGB24:
               gfxs->Cop = PIXEL_RGB32( color.r, color.g, color.b );
               break;
          case DSPF_RGB32:
               gfxs->Cop = PIXEL_RGB32( color.r, color.g, color.b );
               break;
          case DSPF_ARGB:
               gfxs->Cop = PIXEL_ARGB( color.a, color.r, color.g, color.b );
               break;
          case DSPF_AiRGB:
               gfxs->Cop = PIXEL_AiRGB( color.a, color.r, color.g, color.b );
               break;
          case DSPF_A1:
               gfxs->Cop = color.a >> 7;
               break;
          case DSPF_A8:
               gfxs->Cop = color.a;
               break;
          case DSPF_YUY2:
               RGB_TO_YCBCR( color.r, color.g, color.b,
                             gfxs->Cop, gfxs->CbCop, gfxs->CrCop );
               gfxs->Cop = PIXEL_YUY2( gfxs->Cop, gfxs->CbCop, gfxs->CrCop );
               break;
          case DSPF_RGB332:
               gfxs->Cop = PIXEL_RGB332( color.r, color.g, color.b );
               break;
          case DSPF_UYVY:
               RGB_TO_YCBCR( color.r, color.g, color.b,
                             gfxs->Cop, gfxs->CbCop, gfxs->CrCop );
               gfxs->Cop = PIXEL_UYVY( gfxs->Cop, gfxs->CbCop, gfxs->CrCop );
               break;
          case DSPF_I420:
          case DSPF_NV12:
          case DSPF_NV16:
               RGB_TO_YCBCR( color.r, color.g, color.b,
                             gfxs->Cop, gfxs->CbCop, gfxs->CrCop );
               break;
          case DSPF_YV12:
          case DSPF_NV21:
               RGB_TO_YCBCR( color.r, color.g, color.b,
                             gfxs->Cop, gfxs->CrCop, gfxs->CbCop );
               break;
          case DSPF_LUT8:
               gfxs->Cop  = state->color_index;
               gfxs->Alut = destination->palette;
               break;
          case DSPF_ALUT44:
               gfxs->Cop  = (color.a & 0xF0) + state->color_index;
               gfxs->Alut = destination->palette;
               break;
          case DSPF_ARGB2554:
               gfxs->Cop = PIXEL_ARGB2554( color.a, color.r, color.g, color.b );
               break;
          case DSPF_ARGB4444:
               gfxs->Cop = PIXEL_ARGB4444( color.a, color.r, color.g, color.b );
               break;
          default:
               D_ONCE("unsupported destination format");
               return false;
     }

     if (DFB_BLITTING_FUNCTION( accel )) {
          switch (gfxs->src_format) {
               case DSPF_ARGB1555:
               case DSPF_ARGB2554:
               case DSPF_ARGB4444:
               case DSPF_RGB16:
               case DSPF_RGB24:
               case DSPF_RGB32:
               case DSPF_ARGB:
               case DSPF_AiRGB:
               case DSPF_A1:
               case DSPF_A8:
               case DSPF_RGB332:
                    break;
               case DSPF_YUY2:
               case DSPF_UYVY:
               case DSPF_AYUV:
                    if (state->blittingflags & ~DSBLIT_SRC_COLORKEY) {
                         D_ONCE("blits with effects are not"
                                " supported for YUV in software");
                         return false;
                    }
                    if (gfxs->src_format != gfxs->dst_format) {
                         if (gfxs->dst_format == DSPF_YUY2 ||
                             gfxs->dst_format == DSPF_UYVY ||
                             gfxs->dst_format == DSPF_YV12 ||
                             gfxs->dst_format == DSPF_I420 ||
                             gfxs->dst_format == DSPF_NV12 ||
                             gfxs->dst_format == DSPF_NV21 ||
                             gfxs->dst_format == DSPF_NV16) {
                              D_ONCE("YUV to YUV conversion is not"
                                     " supported in software");
                              return false;
                         }
                    }
                    break;
               case DSPF_I420:
               case DSPF_YV12:
                    if ((gfxs->dst_format != DSPF_I420 && gfxs->dst_format != DSPF_YV12) ||
                        state->blittingflags != DSBLIT_NOFX) {
                         D_ONCE("only copying/scaling blits supported"
                                " for YV12/I420 in software");
                         return false;
                    }
                    break;
               case DSPF_NV12:
               case DSPF_NV21:
               case DSPF_NV16:
                    if (gfxs->src_format != gfxs->dst_format ||
                        state->blittingflags != DSBLIT_NOFX) {
                         D_ONCE("only copying/scaling blits supported"
                                " for NV12/NV21/NV16 in software");
                         return false;
                    }
                    break;
               case DSPF_LUT8:
               case DSPF_ALUT44:
                    gfxs->Blut = source->palette;
                    break;
               default:
                    D_ONCE("unsupported source format");
                    return false;
          }
     }


     dfb_surfacemanager_lock( destination->manager );

     if (DFB_BLITTING_FUNCTION( accel )) {
          if (dfb_surface_software_lock( source, source->front_buffer, DSLF_READ, &gfxs->src_org[0],
                                         &gfxs->src_pitch )) {
               dfb_surfacemanager_unlock( destination->manager );
               return false;
          }
          switch (gfxs->src_format) {
               case DSPF_I420:
                    gfxs->src_org[1] = gfxs->src_org[0] + gfxs->src_height * gfxs->src_pitch;
                    gfxs->src_org[2] = gfxs->src_org[1] + gfxs->src_height * gfxs->src_pitch / 4;
                    break;
               case DSPF_YV12:
                    gfxs->src_org[2] = gfxs->src_org[0] + gfxs->src_height * gfxs->src_pitch;
                    gfxs->src_org[1] = gfxs->src_org[2] + gfxs->src_height * gfxs->src_pitch / 4;
                    break;
               case DSPF_NV12:
               case DSPF_NV21:
               case DSPF_NV16:
                    gfxs->src_org[1] = gfxs->src_org[0] + gfxs->src_height * gfxs->src_pitch;
                    break;
               default:
                    break;
          }

          gfxs->src_field_offset = gfxs->src_height/2 * gfxs->src_pitch;

          state->flags |= CSF_SOURCE_LOCKED;
     }

     if (dfb_surface_software_lock( state->destination, state->destination->back_buffer, 
          lock_flags, &gfxs->dst_org[0], &gfxs->dst_pitch ))
     {
          if (state->flags & CSF_SOURCE_LOCKED) {
               dfb_surface_unlock( source, source->front_buffer, 0 );
               state->flags &= ~CSF_SOURCE_LOCKED;
          }

          dfb_surfacemanager_unlock( destination->manager );
          return false;
     }
     switch (gfxs->dst_format) {
          case DSPF_I420:
               gfxs->dst_org[1] = gfxs->dst_org[0] + gfxs->dst_height * gfxs->dst_pitch;
               gfxs->dst_org[2] = gfxs->dst_org[1] + gfxs->dst_height * gfxs->dst_pitch / 4;
               break;
          case DSPF_YV12:
               gfxs->dst_org[2] = gfxs->dst_org[0] + gfxs->dst_height * gfxs->dst_pitch;
               gfxs->dst_org[1] = gfxs->dst_org[2] + gfxs->dst_height * gfxs->dst_pitch / 4;
               break;
          case DSPF_NV12:
          case DSPF_NV21:
          case DSPF_NV16:
               gfxs->dst_org[1] = gfxs->dst_org[0] + gfxs->dst_height * gfxs->dst_pitch;
               break;
          default:
               break;
     }

     gfxs->dst_field_offset = gfxs->dst_height/2 * gfxs->dst_pitch;

     dfb_surfacemanager_unlock( destination->manager );


     switch (accel) {
          case DFXL_FILLRECTANGLE:
          case DFXL_DRAWRECTANGLE:
          case DFXL_DRAWLINE:
          case DFXL_FILLTRIANGLE:
               if (state->drawingflags & (DSDRAW_BLEND | DSDRAW_XOR)) {
                    GenefxAccumulator Cacc, SCacc;

                    /* not yet completed optimizing checks */
                    if (state->src_blend == DSBF_ZERO) {
                         if (state->dst_blend == DSBF_ZERO) {
                              gfxs->Cop = 0;
                              if (state->drawingflags & DSDRAW_DST_COLORKEY) {
                                   gfxs->Dkey = state->dst_colorkey;
                                   *funcs++ = Cop_toK_Aop_PFI[dst_pfi];
                              }
                              else
                                   *funcs++ = Cop_to_Aop_PFI[dst_pfi];
                              break;
                         }
                         else if (state->dst_blend == DSBF_ONE) {
                              break;
                         }
                    }

                    /* load from destination */
                    *funcs++ = Sop_is_Aop;
                    if (DFB_PIXELFORMAT_IS_INDEXED(gfxs->dst_format))
                         *funcs++ = Slut_is_Alut;
                    *funcs++ = Dacc_is_Aacc;
                    *funcs++ = Sop_PFI_to_Dacc[dst_pfi];

                    /* premultiply destination */
                    if (state->drawingflags & DSDRAW_DST_PREMULTIPLY)
                         *funcs++ = Dacc_premultiply;

                    /* xor destination */
                    if (state->drawingflags & DSDRAW_XOR)
                         *funcs++ = Dacc_xor;

                    /* load source (color) */
                    Cacc.a = color.a;
                    Cacc.r = color.r;
                    Cacc.g = color.g;
                    Cacc.b = color.b;

                    /* premultiply source (color) */
                    if (state->drawingflags & DSDRAW_SRC_PREMULTIPLY) {
                         __u16 ca = color.a + 1;

                         Cacc.r = (Cacc.r * ca) >> 8;
                         Cacc.g = (Cacc.g * ca) >> 8;
                         Cacc.b = (Cacc.b * ca) >> 8;
                    }

                    if (state->drawingflags & DSDRAW_BLEND) {
                         /* source blending */
                         switch (state->src_blend) {
                              case DSBF_ZERO:
                                   break;
                              case DSBF_ONE:
                                   SCacc = Cacc;
                                   break;
                              case DSBF_SRCCOLOR:
                                   SCacc.a = (Cacc.a * (Cacc.a + 1)) >> 8;
                                   SCacc.r = (Cacc.r * (Cacc.r + 1)) >> 8;
                                   SCacc.g = (Cacc.g * (Cacc.g + 1)) >> 8;
                                   SCacc.b = (Cacc.b * (Cacc.b + 1)) >> 8;
                                   break;
                              case DSBF_INVSRCCOLOR:
                                   SCacc.a = (Cacc.a * (0x100 - Cacc.a)) >> 8;
                                   SCacc.r = (Cacc.r * (0x100 - Cacc.r)) >> 8;
                                   SCacc.g = (Cacc.g * (0x100 - Cacc.g)) >> 8;
                                   SCacc.b = (Cacc.b * (0x100 - Cacc.b)) >> 8;
                                   break;
                              case DSBF_SRCALPHA: {
                                        __u16 ca = color.a + 1;

                                        SCacc.a = (Cacc.a * ca) >> 8;
                                        SCacc.r = (Cacc.r * ca) >> 8;
                                        SCacc.g = (Cacc.g * ca) >> 8;
                                        SCacc.b = (Cacc.b * ca) >> 8;
                                        break;
                                   }
                              case DSBF_INVSRCALPHA: {
                                        __u16 ca = 0x100 - color.a;

                                        SCacc.a = (Cacc.a * ca) >> 8;
                                        SCacc.r = (Cacc.r * ca) >> 8;
                                        SCacc.g = (Cacc.g * ca) >> 8;
                                        SCacc.b = (Cacc.b * ca) >> 8;
                                        break;
                                   }
                              case DSBF_SRCALPHASAT:
                                   *funcs++ = Sacc_is_NULL;
                              case DSBF_DESTALPHA:
                              case DSBF_INVDESTALPHA:
                              case DSBF_DESTCOLOR:
                              case DSBF_INVDESTCOLOR:
                                   *funcs++ = Dacc_is_Bacc;
                                   *funcs++ = Cacc_to_Dacc;

                                   *funcs++ = Dacc_is_Aacc;
                                   *funcs++ = Xacc_is_Bacc;
                                   *funcs++ = Xacc_blend[state->src_blend - 1];

                                   break;
                         }

                         /* destination blending */
                         *funcs++ = Sacc_is_NULL;
                         *funcs++ = Xacc_is_Aacc;
                         *funcs++ = Xacc_blend[state->dst_blend - 1];

                         /* add source to destination accumulator */
                         switch (state->src_blend) {
                              case DSBF_ZERO:
                                   break;
                              case DSBF_ONE:
                              case DSBF_SRCCOLOR:
                              case DSBF_INVSRCCOLOR:
                              case DSBF_SRCALPHA:
                              case DSBF_INVSRCALPHA:
                                   if (SCacc.a || SCacc.r || SCacc.g || SCacc.b)
                                        *funcs++ = SCacc_add_to_Dacc;
                                   break;
                              case DSBF_DESTALPHA:
                              case DSBF_INVDESTALPHA:
                              case DSBF_DESTCOLOR:
                              case DSBF_INVDESTCOLOR:
                              case DSBF_SRCALPHASAT:
                                   *funcs++ = Sacc_is_Bacc;
                                   *funcs++ = Sacc_add_to_Dacc;
                                   break;
                         }
                    }

                    /* demultiply result */
                    if (state->blittingflags & DSDRAW_DEMULTIPLY)
                         *funcs++ = Dacc_demultiply;

                    /* write to destination */
                    *funcs++ = Sacc_is_Aacc;
                    if (state->drawingflags & DSDRAW_DST_COLORKEY) {
                         gfxs->Dkey = state->dst_colorkey;
                         *funcs++ = Sacc_toK_Aop_PFI[dst_pfi];
                    }
                    else
                         *funcs++ = Sacc_to_Aop_PFI[dst_pfi];

                    /* store computed Cacc */
                    gfxs->Cacc  = Cacc;
                    gfxs->SCacc = SCacc;
               }
               else {
                    if (state->drawingflags & DSDRAW_DST_COLORKEY) {
                         gfxs->Dkey = state->dst_colorkey;
                         *funcs++ = Cop_toK_Aop_PFI[dst_pfi];
                    }
                    else
                         *funcs++ = Cop_to_Aop_PFI[dst_pfi];
               }
               break;
          case DFXL_BLIT:
               if (state->blittingflags == (DSBLIT_COLORIZE |
                                            DSBLIT_BLEND_ALPHACHANNEL) &&
                   state->src_blend == DSBF_SRCALPHA &&
                   state->dst_blend == DSBF_INVSRCALPHA)
               {
                    if (gfxs->src_format == DSPF_A8 &&
                        Bop_a8_set_alphapixel_Aop_PFI[dst_pfi])
                    {
                         *funcs++ = Bop_a8_set_alphapixel_Aop_PFI[dst_pfi];
                         break;
                    }
                    if (gfxs->src_format == DSPF_A1 &&
                        Bop_a1_set_alphapixel_Aop_PFI[dst_pfi])
                    {
                         *funcs++ = Bop_a1_set_alphapixel_Aop_PFI[dst_pfi];
                         break;
                    }
               }
               /* fallthru */
          case DFXL_STRETCHBLIT: {
                    int modulation = state->blittingflags & MODULATION_FLAGS;

                    if (modulation) {
                         bool read_destination = false;
                         bool source_needs_destination = false;

                         /* check if destination has to be read */
                         if (state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL |
                                                     DSBLIT_BLEND_COLORALPHA)) {
                              switch (state->src_blend) {
                                   case DSBF_DESTALPHA:
                                   case DSBF_DESTCOLOR:
                                   case DSBF_INVDESTALPHA:
                                   case DSBF_INVDESTCOLOR:
                                   case DSBF_SRCALPHASAT:
                                        source_needs_destination = true;
                                   default:
                                        ;
                              }

                              read_destination = source_needs_destination ||
                                                 (state->dst_blend != DSBF_ZERO);
                         }

                         /* read the destination if needed */
                         if (read_destination) {
                              *funcs++ = Sop_is_Aop;
                              if (DFB_PIXELFORMAT_IS_INDEXED(gfxs->dst_format))
                                   *funcs++ = Slut_is_Alut;
                              *funcs++ = Dacc_is_Aacc;
                              *funcs++ = Sop_PFI_to_Dacc[dst_pfi];

                              if (state->blittingflags & DSBLIT_DST_PREMULTIPLY)
                                   *funcs++ = Dacc_premultiply;
                         }

                         /* read the source */
                         *funcs++ = Sop_is_Bop;
                         if (DFB_PIXELFORMAT_IS_INDEXED(gfxs->src_format))
                              *funcs++ = Slut_is_Blut;
                         *funcs++ = Dacc_is_Bacc;
                         if (state->blittingflags & DSBLIT_SRC_COLORKEY) {
                              gfxs->Skey = state->src_colorkey;
                              if (accel == DFXL_BLIT)
                                   *funcs++ = Sop_PFI_Kto_Dacc[src_pfi];
                              else
                                   *funcs++ = Sop_PFI_SKto_Dacc[src_pfi];
                         }
                         else {
                              if (accel == DFXL_BLIT)
                                   *funcs++ = Sop_PFI_to_Dacc[src_pfi];
                              else
                                   *funcs++ = Sop_PFI_Sto_Dacc[src_pfi];
                         }

                         /* modulate the source if requested */
                         if (Dacc_modulation[modulation & 0x7]) {
                              /* modulation source */
                              gfxs->Cacc.a = color.a + 1;
                              gfxs->Cacc.r = color.r + 1;
                              gfxs->Cacc.g = color.g + 1;
                              gfxs->Cacc.b = color.b + 1;

                              *funcs++ = Dacc_modulation[modulation & 0x7];
                         }

                         if (state->blittingflags & DSBLIT_SRC_PREMULTIPLY)
                              *funcs++ = Dacc_premultiply;

                         /* do blend functions and combine both accumulators */
                         if (state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL |
                                                     DSBLIT_BLEND_COLORALPHA)) {
                              /* Xacc will be blended and written to while
                                 Sacc and Dacc point to the SRC and DST
                                 as referenced by the blending functions */
                              *funcs++ = Sacc_is_Bacc;
                              *funcs++ = Dacc_is_Aacc;

                              if (source_needs_destination &&
                                  state->dst_blend != DSBF_ONE) {
                                   /* blend the source */
                                   *funcs++ = Xacc_is_Bacc;
                                   *funcs++ = Xacc_blend[state->src_blend - 1];

                                   /* blend the destination */
                                   *funcs++ = Xacc_is_Aacc;
                                   *funcs++ = Xacc_blend[state->dst_blend - 1];
                              }
                              else {
                                   /* blend the destination if needed */
                                   if (read_destination) {
                                        *funcs++ = Xacc_is_Aacc;
                                        *funcs++ = Xacc_blend[state->dst_blend - 1];
                                   }

                                   /* blend the source */
                                   *funcs++ = Xacc_is_Bacc;
                                   *funcs++ = Xacc_blend[state->src_blend - 1];
                              }

                              /* add the destination to the source */
                              if (read_destination) {
                                   *funcs++ = Sacc_is_Aacc;
                                   *funcs++ = Dacc_is_Bacc;
                                   *funcs++ = Sacc_add_to_Dacc;
                              }
                         }

                         if (state->blittingflags & DSBLIT_DEMULTIPLY) {
                              *funcs++ = Dacc_is_Bacc;
                              *funcs++ = Dacc_demultiply;
                         }

                         /* write source to destination */
                         *funcs++ = Sacc_is_Bacc;
                         *funcs++ = Sacc_to_Aop_PFI[dst_pfi];
                    }
                    else if (gfxs->src_format == gfxs->dst_format ||
                             ((gfxs->src_format == DSPF_I420 || gfxs->src_format == DSPF_YV12) &&
                              (gfxs->dst_format == DSPF_I420 || gfxs->dst_format == DSPF_YV12))/* &&
                             (!DFB_PIXELFORMAT_IS_INDEXED(src_format) ||
                              Alut == Blut)*/) {
                         if (accel == DFXL_BLIT) {
                              if (state->blittingflags & DSBLIT_SRC_COLORKEY) {
                                   gfxs->Skey = state->src_colorkey;
                                   *funcs++ = Bop_PFI_Kto_Aop_PFI[dst_pfi];
                              }
                              else if (state->blittingflags & DSBLIT_DST_COLORKEY) {
                                   gfxs->Dkey = state->dst_colorkey;
                                   *funcs++ = Bop_PFI_toK_Aop_PFI[dst_pfi];
                              }
                              else
                                   *funcs++ = Bop_PFI_to_Aop_PFI[dst_pfi];
                         }
                         else {
                              if (state->blittingflags & DSBLIT_SRC_COLORKEY) {
                                   gfxs->Skey = state->src_colorkey;
                                   *funcs++ = Bop_PFI_SKto_Aop_PFI[dst_pfi];
                              }
                              else
                                   *funcs++ = Bop_PFI_Sto_Aop_PFI[dst_pfi];
                         }
                    }
                    else {
                         /* slow */
                         *funcs++ = Sacc_is_Aacc;

                         *funcs++ = Dacc_is_Aacc;

                         *funcs++ = Sop_is_Bop;

                         if (DFB_PIXELFORMAT_IS_INDEXED(gfxs->src_format))
                              *funcs++ = Slut_is_Blut;

                         if (accel == DFXL_BLIT) {
                              if (state->blittingflags & DSBLIT_SRC_COLORKEY ) {
                                   gfxs->Skey = state->src_colorkey;
                                   *funcs++ = Sop_PFI_Kto_Dacc[src_pfi];
                              }
                              else
                                   *funcs++ = Sop_PFI_to_Dacc[src_pfi];
                         }
                         else { /* DFXL_STRETCHBLIT */

                              if (state->blittingflags & DSBLIT_SRC_COLORKEY ) {
                                   gfxs->Skey = state->src_colorkey;
                                   *funcs++ = Sop_PFI_SKto_Dacc[src_pfi];
                              }
                              else
                                   *funcs++ = Sop_PFI_Sto_Dacc[src_pfi];
                         }

                         *funcs++ = Sacc_to_Aop_PFI[dst_pfi];
                    }
                    break;
               }
          default:
               D_ONCE("unimplemented drawing/blitting function");
               gRelease( state );
               return false;
     }

     *funcs = NULL;

     dfb_state_update( state, state->flags & CSF_SOURCE_LOCKED );

     return true;
}

void gRelease( CardState *state )
{
     dfb_surface_unlock( state->destination, state->destination->back_buffer, 0 );

     if (state->flags & CSF_SOURCE_LOCKED) {
          dfb_surface_unlock( state->source, state->source->front_buffer, 0 );
          state->flags &= ~CSF_SOURCE_LOCKED;
     }
}

#define CHECK_PIPELINE()           \
     {                             \
          if (!gfxs->funcs[0])     \
               return;             \
     }

#define RUN_PIPELINE()                     \
     {                                     \
          int         i;                   \
          GenefxFunc *funcs = gfxs->funcs; \
                                           \
          for (i=0; funcs[i]; ++i)         \
               funcs[i]( gfxs );           \
     }


static inline void Aop_xy( GenefxState *gfxs,
                           void *org, int x, int y, int pitch )
{
     gfxs->Aop = org;

     if (gfxs->dst_caps & DSCAPS_SEPARATED) {
          gfxs->Aop_field = y & 1;
          if (gfxs->Aop_field)
               gfxs->Aop += gfxs->dst_field_offset;

          y /= 2;
     }

     D_ASSUME( !(x & DFB_PIXELFORMAT_ALIGNMENT(gfxs->dst_format)) );

     gfxs->Aop += y * pitch  +  DFB_BYTES_PER_LINE( gfxs->dst_format, x );
}

static inline void Aop_next( GenefxState *gfxs, int pitch )
{
     if (gfxs->dst_caps & DSCAPS_SEPARATED) {
          gfxs->Aop_field = !gfxs->Aop_field;

          if (gfxs->Aop_field)
               gfxs->Aop += gfxs->dst_field_offset;
          else
               gfxs->Aop += pitch - gfxs->dst_field_offset;
     }
     else
          gfxs->Aop += pitch;
}

static inline void Aop_prev( GenefxState *gfxs, int pitch )
{
     if (gfxs->dst_caps & DSCAPS_SEPARATED) {
          gfxs->Aop_field = !gfxs->Aop_field;

          if (gfxs->Aop_field)
               gfxs->Aop += gfxs->dst_field_offset - pitch;
          else
               gfxs->Aop -= gfxs->dst_field_offset;
     }
     else
          gfxs->Aop -= pitch;
}


static inline void Bop_xy( GenefxState *gfxs,
                           void *org, int x, int y, int pitch )
{
     gfxs->Bop = org;

     if (gfxs->src_caps & DSCAPS_SEPARATED) {
          gfxs->Bop_field = y & 1;
          if (gfxs->Bop_field)
               gfxs->Bop += gfxs->src_field_offset;

          y /= 2;
     }

     D_ASSUME( !(x & DFB_PIXELFORMAT_ALIGNMENT(gfxs->src_format)) );

     gfxs->Bop += y * pitch  +  DFB_BYTES_PER_LINE( gfxs->src_format, x );
}

static inline void Bop_next( GenefxState *gfxs, int pitch )
{
     if (gfxs->src_caps & DSCAPS_SEPARATED) {
          gfxs->Bop_field = !gfxs->Bop_field;

          if (gfxs->Bop_field)
               gfxs->Bop += gfxs->src_field_offset;
          else
               gfxs->Bop += pitch - gfxs->src_field_offset;
     }
     else
          gfxs->Bop += pitch;
}

static inline void Bop_prev( GenefxState *gfxs, int pitch )
{
     if (gfxs->src_caps & DSCAPS_SEPARATED) {
          gfxs->Bop_field = !gfxs->Bop_field;

          if (gfxs->Bop_field)
               gfxs->Bop += gfxs->src_field_offset - pitch;
          else
               gfxs->Bop -= gfxs->src_field_offset;
     }
     else
          gfxs->Bop -= pitch;
}

static bool
ABacc_prepare( GenefxState *gfxs, int size )
{
     size = (size < 256) ? 256 : (1 << direct_log2(size));

     if (gfxs->ABsize < size) {
          void *ABstart = D_MALLOC( size * sizeof(GenefxAccumulator) * 2 + 7 );

          if (!ABstart) {
               D_WARN( "out of memory" );
               return false;
          }

          if (gfxs->ABstart)
               D_FREE( gfxs->ABstart );

          gfxs->ABstart = ABstart;
          gfxs->ABsize  = size;
          gfxs->Aacc    = (GenefxAccumulator*) (((__u32)ABstart+7) & ~7);
          gfxs->Bacc    = gfxs->Aacc + size;
     }

     return true;
}

void gFillRectangle( CardState *state, DFBRectangle *rect )
{
     int          h;
     GenefxState *gfxs = state->gfxs;

     D_ASSERT( gfxs != NULL );

     CHECK_PIPELINE();

     if (!ABacc_prepare( gfxs, rect->w ))
          return;

     gfxs->length = rect->w;

     if (gfxs->dst_format == DSPF_YUY2 || gfxs->dst_format == DSPF_UYVY)
          gfxs->length /= 2;

     Aop_xy( gfxs, gfxs->dst_org[0], rect->x, rect->y, gfxs->dst_pitch );

     h = rect->h;
     while (h--) {
          RUN_PIPELINE();

          Aop_next( gfxs, gfxs->dst_pitch );
     }

     if (gfxs->dst_format == DSPF_I420 || gfxs->dst_format == DSPF_YV12) {
          rect->x /= 2;
          rect->y /= 2;
          rect->w  = (rect->w + 1) / 2;
          rect->h  = (rect->h + 1) / 2;

          gfxs->dst_field_offset /= 4;

          gfxs->length = rect->w;

          gfxs->Cop = gfxs->CbCop;
          Aop_xy( gfxs, gfxs->dst_org[1], rect->x, rect->y, gfxs->dst_pitch/2 );
          h = rect->h;
          while (h--) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch/2 );
          }

          gfxs->Cop = gfxs->CrCop;
          Aop_xy( gfxs, gfxs->dst_org[2], rect->x, rect->y, gfxs->dst_pitch/2 );
          h = rect->h;
          while (h--) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch/2 );
          }

          gfxs->dst_field_offset *= 4;
     }
     else if (gfxs->dst_format == DSPF_NV12 || gfxs->dst_format == DSPF_NV21 ||
              gfxs->dst_format == DSPF_NV16) {
          rect->x &= ~1;
          rect->w  = (rect->w + 1) / 2;

          if (gfxs->dst_format != DSPF_NV16) {
               rect->y /= 2;
               rect->h  = (rect->h + 1) / 2;

               gfxs->dst_field_offset /= 2;
          }

          gfxs->chroma_plane = true;
          gfxs->length = rect->w;

          gfxs->Cop = (gfxs->CrCop << 8) | gfxs->CbCop;
          Aop_xy( gfxs, gfxs->dst_org[1], rect->x, rect->y, gfxs->dst_pitch );
          h = rect->h;
          while (h--) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch );
          }

          gfxs->chroma_plane = false;

          if (gfxs->dst_format != DSPF_NV16) {
               gfxs->dst_field_offset *= 2;
          }
     }
}

void gDrawLine( CardState *state, DFBRegion *line )
{
     GenefxState *gfxs = state->gfxs;

     int i,dx,dy,sdy,dxabs,dyabs,x,y,px,py;

     D_ASSERT( gfxs != NULL );

     CHECK_PIPELINE();

     /* the horizontal distance of the line */
     dx = line->x2 - line->x1;
     dxabs = abs(dx);

     if (!ABacc_prepare( gfxs, dxabs ))
          return;

     /* the vertical distance of the line */
     dy = line->y2 - line->y1;
     dyabs = abs(dy);

     if (!dx || !dy) {              /* draw horizontal/vertical line */
          DFBRectangle rect = {
               MIN (line->x1, line->x2),
               MIN (line->y1, line->y2),
               dxabs + 1, dyabs + 1};

          gFillRectangle( state, &rect );
          return;
     }

     sdy = SIGN(dy) * SIGN(dx);
     x = dyabs >> 1;
     y = dxabs >> 1;

     if (dx > 0) {
          px  = line->x1;
          py  = line->y1;
     }
     else {
          px  = line->x2;
          py  = line->y2;
     }

     if (dxabs >= dyabs) { /* the line is more horizontal than vertical */

          for (i=0, gfxs->length=1; i<dxabs; i++, gfxs->length++) {
               y += dyabs;
               if (y >= dxabs) {
                    Aop_xy( gfxs, gfxs->dst_org[0], px, py, gfxs->dst_pitch );
                    RUN_PIPELINE();
                    px += gfxs->length;
                    gfxs->length = 0;
                    y -= dxabs;
                    py += sdy;
               }
          }
          Aop_xy( gfxs, gfxs->dst_org[0], px, py, gfxs->dst_pitch );
          RUN_PIPELINE();
     }
     else { /* the line is more vertical than horizontal */

          gfxs->length = 1;
          Aop_xy( gfxs, gfxs->dst_org[0], px, py, gfxs->dst_pitch );
          RUN_PIPELINE();

          for (i=0; i<dyabs; i++) {
               x += dxabs;
               if (x >= dyabs) {
                    x -= dyabs;
                    px++;
               }
               py += sdy;

               Aop_xy( gfxs, gfxs->dst_org[0], px, py, gfxs->dst_pitch );
               RUN_PIPELINE();
          }
     }
}

static void gDoBlit( GenefxState *gfxs,
                     int sx,     int sy,
                     int width,  int height,
                     int dx,     int dy,
                     int spitch, int dpitch,
                     void *sorg, void *dorg )
{
     if (sorg == dorg && dy > sy) {
          /* we must blit from bottom to top */
          gfxs->length = width;

          Aop_xy( gfxs, dorg, dx, dy + height - 1, dpitch );
          Bop_xy( gfxs, sorg, sx, sy + height - 1, spitch );

          while (height--) {
               RUN_PIPELINE();

               Aop_prev( gfxs, dpitch );
               Bop_prev( gfxs, spitch );
          }
     }
     else {
          /* we must blit from top to bottom */
          gfxs->length = width;

          Aop_xy( gfxs, dorg, dx, dy, dpitch );
          Bop_xy( gfxs, sorg, sx, sy, spitch );

          while (height--) {
               RUN_PIPELINE();

               Aop_next( gfxs, dpitch );
               Bop_next( gfxs, spitch );
          }
     }
}

void gBlit( CardState *state, DFBRectangle *rect, int dx, int dy )
{
     GenefxState *gfxs = state->gfxs;

	 /*PR10573*/
	 if (state->stroking_enabled)
	 	gfxs->stroking_enabled = true;
	 else
	 	gfxs->stroking_enabled = false;

	 

     D_ASSERT( gfxs != NULL );

     CHECK_PIPELINE();

     if (!ABacc_prepare( gfxs, rect->w ))
          return;

     if (state->blittingflags & DSBLIT_DEINTERLACE) {
          int i;
          int height = rect->h;

          gfxs->length = rect->w;

          Aop_xy( gfxs, gfxs->dst_org[0], dx, dy, gfxs->dst_pitch );
          Bop_xy( gfxs, gfxs->src_org[0], rect->x, rect->y, gfxs->src_pitch );

          if (state->source->field) {
               Aop_next( gfxs, gfxs->dst_pitch );
               Bop_next( gfxs, gfxs->src_pitch );
               height--;
          }

          height >>= 1;

          for (i=0; i<height; i++) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch );

               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch );

               Bop_next( gfxs, gfxs->src_pitch );
               Bop_next( gfxs, gfxs->src_pitch );
          }

          return;
     }

     if (gfxs->src_org[0] == gfxs->dst_org[0] && dx > rect->x)
          /* we must blit from right to left */
          gfxs->Ostep = -1;
     else
          /* we must blit from left to right*/
          gfxs->Ostep = 1;

     switch (gfxs->src_format) {
          case DSPF_YUY2:
          case DSPF_UYVY:
               dx      &= ~1;
               rect->x &= ~1;
               rect->w  = (rect->w + 1) & ~1;
               break;

          default:
               break;
     }

     gDoBlit( gfxs, rect->x, rect->y, rect->w, rect->h, dx, dy,
              gfxs->src_pitch, gfxs->dst_pitch, gfxs->src_org[0], gfxs->dst_org[0] );

     /* do other planes */
     if (gfxs->src_format == DSPF_I420 || gfxs->src_format == DSPF_YV12) {
          dx      /= 2;
          dy      /= 2;
          rect->x /= 2;
          rect->y /= 2;
          rect->w  = (rect->w + 1) / 2;
          rect->h  = (rect->h + 1) / 2;

          gfxs->dst_field_offset /= 4;
          gfxs->src_field_offset /= 4;

          gDoBlit( gfxs, rect->x, rect->y, rect->w, rect->h, dx, dy,
                   gfxs->src_pitch/2, gfxs->dst_pitch/2, gfxs->src_org[1], gfxs->dst_org[1] );

          gDoBlit( gfxs, rect->x, rect->y, rect->w, rect->h, dx, dy,
                   gfxs->src_pitch/2, gfxs->dst_pitch/2, gfxs->src_org[2], gfxs->dst_org[2] );

          gfxs->dst_field_offset *= 4;
          gfxs->src_field_offset *= 4;
     }
     else if (gfxs->src_format == DSPF_NV12 || gfxs->src_format == DSPF_NV21 ||
              gfxs->src_format == DSPF_NV16) {
          dx      &= ~1;
          rect->x &= ~1;
          rect->w  = (rect->w + 1) / 2;

          if (gfxs->src_format != DSPF_NV16) {
               dy      /= 2;
               rect->y /= 2;
               rect->h  = (rect->h + 1) / 2;

               gfxs->dst_field_offset /= 2;
               gfxs->src_field_offset /= 2;
          }

          gfxs->chroma_plane = true;

          gDoBlit( gfxs, rect->x, rect->y, rect->w, rect->h, dx, dy,
                   gfxs->src_pitch, gfxs->dst_pitch, gfxs->src_org[1], gfxs->dst_org[1] );

          gfxs->chroma_plane = false;

          if (gfxs->src_format != DSPF_NV16) {
               gfxs->dst_field_offset *= 2;
               gfxs->src_field_offset *= 2;
          }
     }
}

void gStretchBlit( CardState *state, DFBRectangle *srect, DFBRectangle *drect )
{
     GenefxState *gfxs = state->gfxs;

     int f;
     int i = 0;
     int h;

     D_ASSERT( gfxs != NULL );

     CHECK_PIPELINE();

     if (!ABacc_prepare( gfxs, drect->w ))
          return;

     gfxs->length = drect->w;
     gfxs->SperD  = (srect->w << 16) / drect->w;

     f = (srect->h << 16) / drect->h;
     h = drect->h;

     Aop_xy( gfxs, gfxs->dst_org[0], drect->x, drect->y, gfxs->dst_pitch );
     Bop_xy( gfxs, gfxs->src_org[0], srect->x, srect->y, gfxs->src_pitch );

     while (h--) {
          RUN_PIPELINE();

          Aop_next( gfxs, gfxs->dst_pitch );

          i += f;

          while (i > 0xFFFF) {
               i -= 0x10000;
               Bop_next( gfxs, gfxs->src_pitch );
          }
     }

     /* scale other planes */
     if (gfxs->src_format == DSPF_YV12 || gfxs->src_format == DSPF_I420) {
          srect->x /= 2;
          srect->y /= 2;
          srect->w  = (srect->w + 1) / 2;
          srect->h  = (srect->h + 1) / 2;
          drect->x /= 2;
          drect->y /= 2;
          drect->w  = (drect->w + 1) / 2;
          drect->h  = (drect->h + 1) / 2;

          gfxs->dst_field_offset /= 4;
          gfxs->src_field_offset /= 4;

          gfxs->length = drect->w;

          Aop_xy( gfxs, gfxs->dst_org[1], drect->x, drect->y, gfxs->dst_pitch/2 );
          Bop_xy( gfxs, gfxs->src_org[1], srect->x, srect->y, gfxs->src_pitch/2 );

          i = 0;
          h = drect->h;
          while (h--) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch/2 );

               i += f;

               while (i > 0xFFFF) {
                    i -= 0x10000;
                    Bop_next( gfxs, gfxs->src_pitch/2 );
               }
          }

          Aop_xy( gfxs, gfxs->dst_org[2], drect->x, drect->y, gfxs->dst_pitch/2 );
          Bop_xy( gfxs, gfxs->src_org[2], srect->x, srect->y, gfxs->src_pitch/2 );

          i = 0;
          h = drect->h;
          while (h--) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch/2 );

               i += f;

               while (i > 0xFFFF) {
                    i -= 0x10000;
                    Bop_next( gfxs, gfxs->src_pitch/2 );
               }
          }

          gfxs->dst_field_offset *= 4;
          gfxs->src_field_offset *= 4;
     }
     else if (gfxs->src_format == DSPF_NV12 || gfxs->src_format == DSPF_NV21 ||
              gfxs->src_format == DSPF_NV16) {
          srect->x &= ~1;
          srect->w  = (srect->w + 1) / 2;
          drect->x &= ~1;
          drect->w  = (drect->w + 1) / 2;

          if (gfxs->src_format != DSPF_NV16) {
               srect->y /= 2;
               srect->h  = (srect->h + 1) / 2;
               drect->y /= 2;
               drect->h  = (drect->h + 1) / 2;

               gfxs->dst_field_offset /= 2;
               gfxs->src_field_offset /= 2;
          }

          gfxs->chroma_plane = true;
          gfxs->length = drect->w;

          Aop_xy( gfxs, gfxs->dst_org[1], drect->x, drect->y, gfxs->dst_pitch );
          Bop_xy( gfxs, gfxs->src_org[1], srect->x, srect->y, gfxs->src_pitch );

          i = 0;
          h = drect->h;
          while (h--) {
               RUN_PIPELINE();

               Aop_next( gfxs, gfxs->dst_pitch );

               i += f;

               while (i > 0xFFFF) {
                    i -= 0x10000;
                    Bop_next( gfxs, gfxs->src_pitch );
               }
          }

          gfxs->chroma_plane = false;

          if (gfxs->src_format != DSPF_NV16) {
               gfxs->dst_field_offset *= 2;
               gfxs->src_field_offset *= 2;
          }
     }
}


#ifdef USE_MMX

#include "generic_mmx.h"

/*
 * patches function pointers to MMX functions
 */
static void gInit_MMX()
{
     use_mmx = 1;

/********************************* Sop_PFI_Sto_Dacc ***************************/
     Sop_PFI_Sto_Dacc[DFB_PIXELFORMAT_INDEX(DSPF_ARGB)] = Sop_argb_Sto_Dacc_MMX;
/********************************* Sop_PFI_to_Dacc ****************************/
     Sop_PFI_to_Dacc[DFB_PIXELFORMAT_INDEX(DSPF_RGB16)] = Sop_rgb16_to_Dacc_MMX;
     Sop_PFI_to_Dacc[DFB_PIXELFORMAT_INDEX(DSPF_RGB32)] = Sop_rgb32_to_Dacc_MMX;
     Sop_PFI_to_Dacc[DFB_PIXELFORMAT_INDEX(DSPF_ARGB )] = Sop_argb_to_Dacc_MMX;
     Sop_PFI_to_Dacc[DFB_PIXELFORMAT_INDEX(DSPF_YUY2 )] = Sop_yuy2_to_Dacc_MMX;
     Sop_PFI_to_Dacc[DFB_PIXELFORMAT_INDEX(DSPF_UYVY )] = Sop_uyvy_to_Dacc_MMX;
/********************************* Sacc_to_Aop_PFI ****************************/
     Sacc_to_Aop_PFI[DFB_PIXELFORMAT_INDEX(DSPF_RGB16)] = Sacc_to_Aop_rgb16_MMX;
     Sacc_to_Aop_PFI[DFB_PIXELFORMAT_INDEX(DSPF_RGB32)] = Sacc_to_Aop_rgb32_MMX;
/********************************* Xacc_blend *********************************/
     Xacc_blend[DSBF_SRCALPHA-1] = Xacc_blend_srcalpha_MMX;
     Xacc_blend[DSBF_INVSRCALPHA-1] = Xacc_blend_invsrcalpha_MMX;
/********************************* Dacc_modulation ****************************/
     Dacc_modulation[DSBLIT_BLEND_ALPHACHANNEL |
                     DSBLIT_BLEND_COLORALPHA |
                     DSBLIT_COLORIZE] = Dacc_modulate_argb_MMX;
/********************************* misc accumulator operations ****************/
     SCacc_add_to_Dacc = SCacc_add_to_Dacc_MMX;
     Sacc_add_to_Dacc = Sacc_add_to_Dacc_MMX;
}

#endif

