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

#include <directfb.h>
#include <directfb_version.h>

#include <misc/conf.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/input.h>
#include <core/layer_context.h>
#include <core/layer_control.h>
#include <core/layers.h>
#include <core/state.h>
#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/windows.h>
#include <core/windowstack.h>

#include <gfx/convert.h>

#include <direct/conf.h>
#include <direct/interface.h>
#include <direct/mem.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <display/idirectfbsurface.h>

#include <idirectfb.h>


IDirectFB *idirectfb_singleton = NULL;

static DFBResult apply_configuration( IDirectFB *dfb );

static DFBResult CreateRemote( const char *host, int session, IDirectFB **ret_interface );

/*
 * Version checking
 */
const unsigned int directfb_major_version = DIRECTFB_MAJOR_VERSION;
const unsigned int directfb_minor_version = DIRECTFB_MINOR_VERSION;
const unsigned int directfb_micro_version = DIRECTFB_MICRO_VERSION;
const unsigned int directfb_binary_age    = DIRECTFB_BINARY_AGE;
const unsigned int directfb_interface_age = DIRECTFB_INTERFACE_AGE;

const char *
DirectFBCheckVersion( unsigned int required_major,
                      unsigned int required_minor,
                      unsigned int required_micro )
{
     if (required_major > DIRECTFB_MAJOR_VERSION)
          return "DirectFB version too old (major mismatch)";
     if (required_major < DIRECTFB_MAJOR_VERSION)
          return "DirectFB version too new (major mismatch)";
     if (required_minor > DIRECTFB_MINOR_VERSION)
          return "DirectFB version too old (minor mismatch)";
     if (required_minor < DIRECTFB_MINOR_VERSION)
          return "DirectFB version too new (minor mismatch)";
     if (required_micro < DIRECTFB_MICRO_VERSION - DIRECTFB_BINARY_AGE)
          return "DirectFB version too new (micro mismatch)";
     if (required_micro > DIRECTFB_MICRO_VERSION)
          return "DirectFB version too old (micro mismatch)";

     return NULL;
}

const char *
DirectFBUsageString( void )
{
     return dfb_config_usage();
}

DFBResult
DirectFBInit( int *argc, char **argv[] )
{
     DFBResult ret;

     ret = dfb_config_init( argc, argv );
     if (ret)
          return ret;

     return DFB_OK;
}

DFBResult
DirectFBSetOption( const char *name, const char *value )
{
     DFBResult ret;

     if (dfb_config == NULL) {
          D_ERROR( "DirectFBSetOption: DirectFBInit has to be "
                   "called before DirectFBSetOption!\n" );
          return DFB_INIT;
     }

     if (idirectfb_singleton) {
          D_ERROR( "DirectFBSetOption: DirectFBSetOption has to be "
                   "called before DirectFBCreate!\n" );
          return DFB_INIT;
     }

     if (!name)
          return DFB_INVARG;

     ret = dfb_config_set( name, value );
     if (ret)
          return ret;

     return DFB_OK;
}

/*
 * Programs have to call this to get the super interface
 * which is needed to access other functions
 */
DFBResult
DirectFBCreate( IDirectFB **interface )
{
     DFBResult  ret;
     CoreDFB   *core_dfb;

     if (!dfb_config) {
          /*  don't use D_ERROR() here, it uses dfb_config  */
          fprintf( stderr,
                   "(!) DirectFBCreate: DirectFBInit has to be called before DirectFBCreate!\n" );
          return DFB_INIT;
     }

     if (!interface)
          return DFB_INVARG;

     if (idirectfb_singleton) {
          idirectfb_singleton->AddRef( idirectfb_singleton );
          *interface = idirectfb_singleton;
          return DFB_OK;
     }

     if (!direct_config->quiet && dfb_config->banner) {
          fprintf( stderr, "\n" );
          fprintf( stderr, "       ---------------------- DirectFB v%d.%d.%d ---------------------\n",
                           DIRECTFB_MAJOR_VERSION, DIRECTFB_MINOR_VERSION, DIRECTFB_MICRO_VERSION );
          fprintf( stderr, "             (c) 2000-2002  convergence integrated media GmbH  \n" );
          fprintf( stderr, "             (c) 2002-2004  convergence GmbH                   \n" );
          fprintf( stderr, "        -----------------------------------------------------------\n" );
          fprintf( stderr, "\n" );
     }

     if (dfb_config->remote.host)
          return CreateRemote( dfb_config->remote.host, dfb_config->remote.session, interface );

     ret = dfb_core_create( &core_dfb );
     if (ret)
          return ret;

     DIRECT_ALLOCATE_INTERFACE( idirectfb_singleton, IDirectFB );

     ret = IDirectFB_Construct( idirectfb_singleton, core_dfb );
     if (ret) {
          idirectfb_singleton = NULL;
          dfb_core_destroy( core_dfb, false );
          return ret;
     }

     if (dfb_core_is_master( core_dfb )) {
          ret = apply_configuration( idirectfb_singleton );
          if (ret) {
               idirectfb_singleton->Release( idirectfb_singleton );
               idirectfb_singleton = NULL;
               return ret;
          }
     }

     *interface = idirectfb_singleton;

     return DFB_OK;
}

DFBResult
DirectFBError( const char *msg, DFBResult error )
{
     if (msg)
          fprintf( stderr, "(#) DirectFBError [%s]: %s\n", msg,
                   DirectFBErrorString( error ) );
     else
          fprintf( stderr, "(#) DirectFBError: %s\n",
                   DirectFBErrorString( error ) );

     return error;
}

const char *
DirectFBErrorString( DFBResult error )
{
     return DirectResultString( error );
}

DFBResult
DirectFBErrorFatal( const char *msg, DFBResult error )
{
     DirectFBError( msg, error );

     //if (idirectfb_singleton)
          //IDirectFB_Destruct( idirectfb_singleton );

     exit( error );
}

/**************************************************************************************************/

static DFBResult
apply_configuration( IDirectFB *dfb )
{
     DFBResult              ret;
     CoreLayer             *layer;
     CoreLayerContext      *context;
     CoreWindowStack       *stack;
     DFBDisplayLayerConfig  layer_config;

     /* the primary layer */
     layer = dfb_layer_at_translated( DLID_PRIMARY );

     /* get the default (shared) context */
     ret = dfb_layer_get_primary_context( layer, true, &context );
     if (ret) {
          D_ERROR( "DirectFB/DirectFBCreate: "
                    "Could not get default context of primary layer!\n" );
          return ret;
     }

     stack = dfb_layer_context_windowstack( context );

     D_ASSERT( stack != NULL );

     /* set buffer mode for desktop */
     layer_config.flags = DLCONF_BUFFERMODE;

     D_DEBUG("apply_configuration dfb_config->buffer_mode is %d\n", dfb_config->buffer_mode);

     if (dfb_config->buffer_mode == -1) {
          CardCapabilities caps;

          dfb_gfxcard_get_capabilities( &caps );

          if (caps.accel & DFXL_BLIT)
               layer_config.buffermode = DLBM_BACKVIDEO;
          else
               layer_config.buffermode = DLBM_BACKSYSTEM;
     }
     else
          layer_config.buffermode = dfb_config->buffer_mode;

     if (dfb_layer_context_set_configuration( context, &layer_config )) {
          D_ERROR( "DirectFB/DirectFBCreate: "
                    "Setting desktop buffer mode failed!\n"
                    "     -> No virtual resolution support or not enough memory?\n"
                    "        Falling back to system back buffer.\n" );

          layer_config.buffermode = DLBM_BACKSYSTEM;

          if (dfb_layer_context_set_configuration( context, &layer_config ))
               D_ERROR( "DirectFB/DirectFBCreate: "
                         "Setting system memory desktop back buffer failed!\n"
                         "     -> Using front buffer only mode.\n" );
     }

     /* temporarily disable background */
     dfb_windowstack_set_background_mode( stack, DLBM_DONTCARE );

     /* set desktop background color */
     dfb_windowstack_set_background_color( stack, &dfb_config->layer_bg_color );

     /* set desktop background image */
     if (dfb_config->layer_bg_mode == DLBM_IMAGE ||
         dfb_config->layer_bg_mode == DLBM_TILE)
     {
          DFBSurfaceDescription   desc;
          IDirectFBImageProvider *provider;
          IDirectFBSurface       *image;
          IDirectFBSurface_data  *image_data;

          ret = dfb->CreateImageProvider( dfb, dfb_config->layer_bg_filename, &provider );
          if (ret) {
               DirectFBError( "Failed loading background image", ret );
               dfb_layer_context_unref( context );
               return DFB_INIT;
          }

          dfb_layer_context_get_configuration( context, &layer_config );

          if (dfb_config->layer_bg_mode == DLBM_IMAGE) {
               desc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT;
               desc.width  = layer_config.width;
               desc.height = layer_config.height;
          }
          else {
               provider->GetSurfaceDescription( provider, &desc );
          }

          desc.flags |= DSDESC_PIXELFORMAT;
          desc.pixelformat = layer_config.pixelformat;

          ret = dfb->CreateSurface( dfb, &desc, &image );
          if (ret) {
               DirectFBError( "Failed creating surface for background image", ret );

               provider->Release( provider );

               dfb_layer_context_unref( context );
               return DFB_INIT;
          }

          ret = provider->RenderTo( provider, image, NULL );
          if (ret) {
               DirectFBError( "Failed loading background image", ret );

               image->Release( image );
               provider->Release( provider );

               dfb_layer_context_unref( context );
               return DFB_INIT;
          }

          provider->Release( provider );

          image_data = (IDirectFBSurface_data*) image->priv;

          dfb_windowstack_set_background_image( stack, image_data->surface );

          image->Release( image );
     }

     /* now set the background mode */
     dfb_windowstack_set_background_mode( stack, dfb_config->layer_bg_mode );

     dfb_layer_context_unref( context );

     return DFB_OK;
}

/**************************************************************************************************/

static DFBResult
CreateRemote( const char *host, int session, IDirectFB **ret_interface )
{
     DFBResult             ret;
     DirectInterfaceFuncs *funcs;
     void                 *interface;

     D_ASSERT( host != NULL );
     D_ASSERT( ret_interface != NULL );

     ret = DirectGetInterface( &funcs, "IDirectFB", "Requestor", NULL, NULL );
     if (ret)
          return ret;

     ret = funcs->Allocate( &interface );
     if (ret)
          return ret;

     ret = funcs->Construct( interface, host, session );
     if (ret)
          return ret;

     *ret_interface = idirectfb_singleton = interface;

     return DFB_OK;
}

