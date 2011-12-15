/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   This file is subject to the terms and conditions of the MIT License:

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <directfb.h>

#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>

#include <direct/util.h>
#include <direct/debug.h>

#include <bsettop.h>

/******************************************************************************/

static IDirectFB            *dfb     = NULL;
static IDirectFBSurface     *primary = NULL;
static IDirectFBEventBuffer *events  = NULL;

static int screen_width, screen_height;
static unsigned int video_total, screen_total;

/******************************************************************************/

static void init_application( int *argc, char **argv[] );
static void exit_application( int status );

static void update_display();

/******************************************************************************/

int
main( int argc, char *argv[] )
{
     DFBResult ret = EXIT_SUCCESS;

     /* bsettop init begin */
     bdisplay_t bcm_display;
     bgraphics_t bcm_graphics;
     if (bsettop_init(BSETTOP_VERSION) != b_ok) {
         D_ERROR("Could not init bsettop\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     if ((bcm_display = bdisplay_open(B_ID(0))) == NULL) {
         D_ERROR("Could not open bdisplay B_ID(0)\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     if ((bcm_graphics = bgraphics_open(B_ID(0), bcm_display)) == NULL) {
         D_ERROR("Could not open bgraphics B_ID(0)\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     /* bsettop init end */

     /* Initialize application. */
     init_application( &argc, &argv );

     /* Main loop. */
     while (1) {
          DFBInputEvent event;

          update_display();

          events->WaitForEventWithTimeout( events, 0, 100 );

          /* Check for new events. */
          while (events->GetEvent( events, DFB_EVENT(&event) ) == DFB_OK) {

               /* Handle key press events. */
               if (event.type == DIET_KEYPRESS) {
                    switch (event.key_symbol) {
                         case DIKS_ESCAPE:
                         case DIKS_POWER:
                         case DIKS_BACK:
                         case DIKS_SMALL_Q:
                         case DIKS_CAPITAL_Q:
                              exit_application( 0 );
                              break;

                         default:
                              break;
                    }
               }
          }
     }

out:

     /* bsettop uninit begin */
     if (bcm_graphics) bgraphics_close(bcm_graphics);
     if (bcm_display) bdisplay_close(bcm_display);
     bsettop_uninit();
     /* bsettop uninit end */

     return ret;
}

/******************************************************************************/

static void
init_application( int *argc, char **argv[] )
{
     DFBResult             ret;
     DFBSurfaceDescription desc;
     DFBCardCapabilities   caps;

     /* Initialize DirectFB including command line parsing. */
     ret = DirectFBInit( argc, argv );
     if (ret) {
          DirectFBError( "DirectFBInit() failed", ret );
          exit_application( 1 );
     }

     /* Create the super interface. */
     ret = DirectFBCreate( &dfb );
     if (ret) {
          DirectFBError( "DirectFBCreate() failed", ret );
          exit_application( 2 );
     }

     dfb->GetCardCapabilities( dfb, &caps );

     video_total = caps.video_memory;

     /* Request fullscreen mode. */
     //dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* Fill the surface description. */
     desc.flags = DSDESC_CAPS;
     desc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     /* Create the primary surface. */
     ret = dfb->CreateSurface( dfb, &desc, &primary );
     if (ret) {
          DirectFBError( "IDirectFB::CreateSurface() failed", ret );
          exit_application( 3 );
     }

     /* Create an event buffer with key capable devices attached. */
     ret = dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS, DFB_FALSE, &events );
     if (ret) {
          DirectFBError( "IDirectFB::CreateEventBuffer() failed", ret );
          exit_application( 4 );
     }
}

static void
exit_application( int status )
{
     /* Release the event buffer. */
     if (events)
          events->Release( events );

     /* Release the primary surface. */
     if (primary)
          primary->Release( primary );

     /* Release the super interface. */
     if (dfb)
          dfb->Release( dfb );

     /* Terminate application. */
     exit( status );
}

static DFBEnumerationResult
chunk_callback( SurfaceBuffer *buffer,
                int            offset,
                int            length,
                int            tolerations,
                void          *ctx )
{
     __u8         r = 0, g = 0, b = 0;
     unsigned int screen_length, screen_offset;

     if (buffer) {
          switch (buffer->policy) {
               case CSP_VIDEOLOW:
                    g = 0x80;
                    break;
               case CSP_VIDEOHIGH:
                    g = 0xa0;
                    break;
               case CSP_VIDEOONLY:
                    b = 0xa0;
                    break;
               default:
                    break;
          }

          r = tolerations*2/3;
     }

     primary->SetColor( primary, r, g, b, 0xff );

     screen_length = (unsigned int)( (long long)length *
                                     (long long)screen_total /
                                     (long long)(video_total ? video_total : 1)  );
     screen_offset = (unsigned int)( (long long)offset *
                                     (long long)screen_total /
                                     (long long)(video_total ? video_total : 1) );

     while (screen_length > 0) {
          int x = screen_offset % screen_width;
          int y = screen_offset / screen_width;

          int w = MIN( screen_length, screen_width - x );

          primary->FillRectangle( primary, x, y, w, 1 );

          screen_length -= w;
          screen_offset += w;
     }

     return DFENUM_OK;
}

static void
update_display( void )
{
     primary->GetSize( primary, &screen_width, &screen_height );

     screen_total = screen_width * screen_height;

     primary->Clear( primary, 0x30, 0x30, 0x30, 0x80 );

     dfb_surfacemanager_enumerate_chunks( dfb_gfxcard_surface_manager(),
                                          chunk_callback, NULL );

     primary->Flip( primary, NULL, 0 );
}
