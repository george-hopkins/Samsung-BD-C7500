/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de> and
              Sven Neumann <neo@directfb.org>.

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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <directfb.h>
#include <direct/debug.h>

#include <bsettop.h>

/* the super interface */
static IDirectFB *dfb;

/* the primary surface (surface of primary layer) */
static IDirectFBSurface *primary;

/* provider for our images/font */
static IDirectFBFont *font;

/* Input buffer */
static IDirectFBEventBuffer *events;

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
        {                                                                      \
           err = x;                                                            \
           if (err != DFB_OK) {                                                \
              fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );           \
              DirectFBErrorFatal( #x, err );                                   \
           }                                                                   \
        }

static char *rules[] = { "CLEAR", "SRC", "SRC OVER", "DST OVER",
                         "SRC IN", "DST IN", "SRC OUT", "DST OUT" };
static int num_rules = sizeof( rules ) / sizeof( rules[0] );

static int screen_width, screen_height;

int main( int argc, char *argv[] )
{
     DFBResult ret = EXIT_SUCCESS;

     int                   i;
     int                   step;
     DFBResult             err;
     DFBSurfaceDescription sdsc;
     DFBFontDescription    fdsc;

     /* bsettop init begin */
     bdisplay_t bcm_display;
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
     /* bsettop init end */

     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the super interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* create an event buffer with all devices attached that have keys */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS, DFB_FALSE, &events ));

     /* set our cooperative level to DFSCL_FULLSCREEN
        for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* get the primary surface, i.e. the surface of the
        primary layer we have exclusive access to */
     sdsc.flags       = DSDESC_CAPS | DSDESC_PIXELFORMAT;
     sdsc.caps        = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
     sdsc.pixelformat = DSPF_ARGB;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     primary->GetSize( primary, &screen_width, &screen_height );

     primary->SetColor( primary, 30, 40, 50, 100 );
     primary->FillRectangle( primary, 0, 0, screen_width, screen_height );


     fdsc.flags = DFDESC_HEIGHT;
     fdsc.height = screen_width/32;

     DFBCHECK(dfb->CreateFont( dfb, FONT, &fdsc, &font ));
     DFBCHECK(primary->SetFont( primary, font ));

     primary->SetColor( primary, 0xFF, 0xFF, 0xFF, 0xFF );
     primary->DrawString( primary, "Porter/Duff Demo", -1, screen_width/2, 50, DSTF_CENTER );

     font->Release( font );


     fdsc.height = screen_width/42;

     DFBCHECK(dfb->CreateFont( dfb, FONT, &fdsc, &font ));
     DFBCHECK(primary->SetFont( primary, font ));

     step = (screen_width-40) / num_rules;

     for (i=0; i<num_rules; i++) {
          primary->SetDrawingFlags( primary, DSDRAW_NOFX );
          primary->SetColor( primary, 50, 50, 250, 200 );
          primary->FillRectangle( primary, i * step + 40, 130, 50, 70 );

          primary->SetPorterDuff( primary, i+1 );
          primary->SetColor( primary, 250, 50, 50, 100 );
          primary->SetDrawingFlags( primary, DSDRAW_BLEND );
          primary->FillRectangle( primary, i * step + 55, 160, 50, 70 );

          primary->SetPorterDuff( primary, DSPD_NONE );
          primary->SetColor( primary, i*0x1F, i*0x10+0x7f, 0xFF, 0xFF );
          primary->DrawString( primary, rules[i], -1, i * step + 65, 240, DSTF_CENTER | DSTF_TOP );
     }

     font->Release( font );


     primary->Flip( primary, NULL, DSFLIP_NONE );

     while (1) {
          DFBInputEvent ev;

          events->WaitForEvent( events );

          events->GetEvent( events, DFB_EVENT(&ev) );

          if (ev.type == DIET_KEYRELEASE && ev.key_symbol == DIKS_ESCAPE)
               break;
     }

     /* release our interfaces to shutdown DirectFB */
     primary->Release( primary );
     events->Release( events );
     dfb->Release( dfb );

out:

     /* bsettop uninit begin */
     if (bcm_display) bdisplay_close(bcm_display);
     bsettop_uninit();
     /* bsettop uninit end */
                                                                                                                                                             
     return ret;
}

