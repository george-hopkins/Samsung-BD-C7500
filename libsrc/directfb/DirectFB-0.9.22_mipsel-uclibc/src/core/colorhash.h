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

#ifndef __CORE__COLORHASH_H__
#define __CORE__COLORHASH_H__

#include <directfb.h>

#include <core/coretypes.h>

void          dfb_colorhash_attach     (CorePalette *palette);
void          dfb_colorhash_detach     (CorePalette *palette);
unsigned int  dfb_colorhash_lookup     (CorePalette *palette,
                                        __u8         r,
                                        __u8         g,
                                        __u8         b,
                                        __u8         a);
void          dfb_colorhash_invalidate (CorePalette *palette);

#endif

