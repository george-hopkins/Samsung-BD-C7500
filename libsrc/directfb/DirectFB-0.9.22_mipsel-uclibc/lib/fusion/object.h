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

#ifndef __FUSION__OBJECT_H__
#define __FUSION__OBJECT_H__

#include <fusion/types.h>

#include <fusion/lock.h>
#include <direct/list.h>
#include <fusion/ref.h>
#include <fusion/reactor.h>

#include <direct/debug.h>

typedef void (*FusionObjectDestructor)( FusionObject *object, bool zombie );

typedef enum {
     FOS_INIT,
     FOS_ACTIVE,
     FOS_DEINIT
} FusionObjectState;

struct __Fusion_FusionObject {
     DirectLink         link;
     FusionObjectPool  *pool;

     int                magic;

     int                id;

     FusionObjectState  state;

     FusionRef          ref;
     FusionReactor     *reactor;
};


typedef bool (*FusionObjectCallback)( FusionObjectPool *pool,
                                      FusionObject     *object,
                                      void             *ctx );


FusionObjectPool *fusion_object_pool_create ( const char            *name,
                                              int                    object_size,
                                              int                    message_size,
                                              FusionObjectDestructor destructor );

DirectResult      fusion_object_pool_destroy( FusionObjectPool      *pool );


DirectResult      fusion_object_pool_enum   ( FusionObjectPool      *pool,
                                              FusionObjectCallback   callback,
                                              void                  *ctx );


FusionObject     *fusion_object_create  ( FusionObjectPool *pool );

DirectResult      fusion_object_set_lock( FusionObject     *object,
                                          FusionSkirmish   *lock );

DirectResult      fusion_object_activate( FusionObject     *object );

DirectResult      fusion_object_destroy ( FusionObject     *object );


#define FUSION_OBJECT_METHODS(type, prefix)                                    \
                                                                               \
static inline DirectResult                                                     \
prefix##_attach( type         *object,                                         \
                 ReactionFunc  func,                                           \
                 void         *ctx,                                            \
                 Reaction     *ret_reaction )                                  \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_reactor_attach( ((FusionObject*)object)->reactor,           \
                                   func, ctx, ret_reaction );                  \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_detach( type     *object,                                             \
                 Reaction *reaction )                                          \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_reactor_detach( ((FusionObject*)object)->reactor,           \
                                   reaction );                                 \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_attach_global( type           *object,                                \
                        int             index,                                 \
                        void           *ctx,                                   \
                        GlobalReaction *reaction )                             \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_reactor_attach_global( ((FusionObject*)object)->reactor,    \
                                          index, ctx, reaction );              \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_detach_global( type           *object,                                \
                        GlobalReaction *reaction )                             \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_reactor_detach_global( ((FusionObject*)object)->reactor,    \
                                          reaction );                          \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_dispatch( type               *object,                                 \
                   void               *message,                                \
                   const ReactionFunc *globals )                               \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_reactor_dispatch( ((FusionObject*)object)->reactor,         \
                                     message, true, globals );                 \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_ref( type *object )                                                   \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_ref_up( &((FusionObject*)object)->ref, false );             \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_unref( type *object )                                                 \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     return fusion_ref_down( &((FusionObject*)object)->ref, false );           \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_link( type **link,                                                    \
               type  *object )                                                 \
{                                                                              \
     DirectResult ret;                                                         \
                                                                               \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
                                                                               \
     ret = fusion_ref_up( &((FusionObject*)object)->ref, true );               \
     if (ret)                                                                  \
          return ret;                                                          \
                                                                               \
     *link = object;                                                           \
                                                                               \
     return DFB_OK;                                                            \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_unlink( type **link )                                                 \
{                                                                              \
     type *object = *link;                                                     \
                                                                               \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
                                                                               \
     *link = NULL;                                                             \
                                                                               \
     return fusion_ref_down( &((FusionObject*)object)->ref, true );            \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_inherit( type *object,                                                \
                  void *from )                                                 \
{                                                                              \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
     D_MAGIC_ASSERT( (FusionObject*) from, FusionObject );                     \
                                                                               \
     return fusion_ref_inherit( &((FusionObject*)object)->ref,                 \
                                &((FusionObject*)from)->ref );                 \
}                                                                              \
                                                                               \
static inline DirectResult                                                     \
prefix##_globalize( type *object )                                             \
{                                                                              \
     DirectResult ret;                                                         \
                                                                               \
     D_MAGIC_ASSERT( (FusionObject*) object, FusionObject );                   \
                                                                               \
     ret = fusion_ref_up( &((FusionObject*)object)->ref, true );               \
     if (ret)                                                                  \
          return ret;                                                          \
                                                                               \
     ret = fusion_ref_down( &((FusionObject*)object)->ref, false );            \
     if (ret)                                                                  \
          fusion_ref_down( &((FusionObject*)object)->ref, true );              \
                                                                               \
     return ret;                                                               \
}

FUSION_OBJECT_METHODS( void, fusion_object );

#endif

