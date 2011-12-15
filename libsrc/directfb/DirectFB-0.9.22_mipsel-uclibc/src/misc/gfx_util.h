/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   Scaling routines ported from gdk_pixbuf by Sven Neumann
   <sven@convergence.de>.

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

#ifndef __GFX_UTIL_H__
#define __GFX_UTIL_H__

#include <directfb.h>

#include <core/coretypes.h>

void dfb_copy_buffer_32( __u32 *src,
                         void  *dst, int dpitch, DFBRectangle *drect,
                         CoreSurface *dst_surface );

void dfb_scale_linear_32( __u32 *src, int sw, int sh,
                          void *dst, int dpitch, DFBRectangle *drect,
                          CoreSurface *dst_surface );

void dfb_copy_to_argb( __u8 *src, int w, int h, int spitch, DFBSurfacePixelFormat src_format,
                      __u8  *dst, int dpitch, CorePalette *palette );

#endif
