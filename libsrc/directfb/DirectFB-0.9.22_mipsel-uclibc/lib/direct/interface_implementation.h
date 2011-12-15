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

#ifndef __DIRECT__INTERFACE_IMPLEMENTATION_H__
#define __DIRECT__INTERFACE_IMPLEMENTATION_H__

#include <direct/interface.h>


static const char   *GetType();
static const char   *GetImplementation();
static DirectResult  Allocate( void **interface );

static DirectInterfaceFuncs interface_funcs = {
     GetType:            GetType,
     GetImplementation:  GetImplementation,
     Allocate:           Allocate,
     Probe:              (DirectResult (*)( void *, ... )) Probe,
     Construct:          (DirectResult (*)( void *, ... )) Construct
};

#define DIRECT_INTERFACE_IMPLEMENTATION(type, impl)    \
                                                       \
static const char *                                    \
GetType()                                              \
{                                                      \
     return #type;                                     \
}                                                      \
                                                       \
static const char *                                    \
GetImplementation()                                    \
{                                                      \
     return #impl;                                     \
}                                                      \
                                                       \
static DirectResult                                    \
Allocate( void **interface )                           \
{                                                      \
     DIRECT_ALLOCATE_INTERFACE( *interface, type );    \
     return DFB_OK;                                    \
}                                                      \
                                                       \
__attribute__((constructor)) void type##_##impl();     \
                                                       \
void                                                   \
type##_##impl()                                        \
{                                                      \
     DirectRegisterInterface( &interface_funcs );      \
}

#endif

