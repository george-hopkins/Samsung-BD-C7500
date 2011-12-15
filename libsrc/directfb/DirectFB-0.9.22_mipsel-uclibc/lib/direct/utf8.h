/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   UTF8 routines ported from glib-2.0 and optimized

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

#ifndef __DIRECT__UTF8_H__
#define __DIRECT__UTF8_H__

#include <direct/types.h>


#define DIRECT_UTF8_SKIP(c)     (((__u8)(c) < 0xc0) ? 1 : __direct_utf8_skip[(__u8)(c)&0x3f])

#define DIRECT_UTF8_GET_CHAR(p) (*(const __u8*)(p) < 0xc0 ? \
                                 *(const __u8*)(p) : __direct_utf8_get_char((const __u8*)(p)))


/*
 *  Actually the last two fields used to be zero since they indicate an
 *  invalid UTF-8 string. Changed it to 1 to avoid endless looping on
 *  invalid input.
 */
static const char __direct_utf8_skip[64] = {
     2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
     3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

static inline unichar __direct_utf8_get_char( const __u8 *p )
{
     int              len;
     register unichar result = p[0];

     if (result < 0xc0)
          return result;

     if (result > 0xfd)
          return (unichar) -1;

     len = __direct_utf8_skip[result & 0x3f];

     result &= 0x7c >> len;

     while (--len) {
          int c = *(++p);

          if ((c & 0xc0) != 0x80)
               return (unichar) -1;

          result = (result << 6) | (c & 0x3f);
     }

     return result;
}

#endif
