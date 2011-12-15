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

#ifndef __DIRECT__SERIAL_H__
#define __DIRECT__SERIAL_H__

#include <direct/types.h>
#include <direct/debug.h>

struct __D_DirectSerial {
     int   magic;

     __u32 value;
     __u32 overflow;
};

static inline void
direct_serial_init( DirectSerial *serial )
{
     D_ASSERT( serial != NULL );

     serial->value    = 0;
     serial->overflow = 0;

     D_MAGIC_SET( serial, DirectSerial );
}

static inline void
direct_serial_deinit( DirectSerial *serial )
{
     D_MAGIC_CLEAR( serial );
}

static inline void
direct_serial_increase( DirectSerial *serial )
{
     D_MAGIC_ASSERT( serial, DirectSerial );

     ++serial->value || serial->overflow++;
}

static inline void
direct_serial_copy( DirectSerial *serial, const DirectSerial *source )
{
     D_MAGIC_ASSERT( serial, DirectSerial );
     D_MAGIC_ASSERT( source, DirectSerial );

     serial->value    = source->value;
     serial->overflow = source->overflow;
}

static inline bool
direct_serial_update( DirectSerial *serial, const DirectSerial *source )
{
     D_MAGIC_ASSERT( serial, DirectSerial );
     D_MAGIC_ASSERT( source, DirectSerial );

     if (serial->overflow < source->overflow) {
          serial->overflow = source->overflow;
          serial->value    = source->value;

          return true;
     }
     else if (serial->overflow == source->overflow && serial->value < source->value) {
          serial->value = source->value;

          return true;
     }

     D_ASSUME( serial->value == source->value );

     return false;
}

#endif

