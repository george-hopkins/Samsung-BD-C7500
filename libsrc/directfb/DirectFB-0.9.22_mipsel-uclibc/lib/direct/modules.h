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

#ifndef __DIRECT__MODULES_H__
#define __DIRECT__MODULES_H__

#include <pthread.h>

#include <direct/types.h>
#include <direct/list.h>
#include <direct/util.h>


struct __D_DirectModuleEntry {
     DirectLink         link;

     int                magic;

     DirectModuleDir   *directory;

     bool               loaded;
     bool               dynamic;
     bool               disabled;

     char              *name;
     const void        *funcs;

     int                refs;
     char              *file;
     void              *handle;
};

struct __D_DirectModuleDir {
     pthread_mutex_t    lock;

     const char        *path;
     unsigned int       abi_version;

     DirectLink        *entries;

     DirectModuleEntry *loading;
};

#define DECLARE_MODULE_DIRECTORY(d)  \
     extern DirectModuleDir d

#define DEFINE_MODULE_DIRECTORY(d,p,n)                                     \
     DirectModuleDir d = {                                                 \
          lock:        DIRECT_UTIL_RECURSIVE_PTHREAD_MUTEX_INITIALIZER,    \
          path:        MODULEDIR"/"p,                                      \
          abi_version: n,                                                  \
          entries:     NULL,                                               \
          loading:     NULL                                                \
     }

int   direct_modules_explore_directory( DirectModuleDir *directory );

void  direct_modules_register( DirectModuleDir *directory,
                               unsigned int     abi_version,
                               const char      *name,
                               const void      *funcs );

const void *direct_module_ref  ( DirectModuleEntry *module );
void        direct_module_unref( DirectModuleEntry *module );

#endif

