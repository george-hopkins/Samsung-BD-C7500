/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002       convergence GmbH.
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

#include <directfb.h>
#include <direct/debug.h>

#include <bdvd.h>

#include <direct/types.h>     /* for 'bool' ;-) */

#include <sys/time.h> /* for gettimeofday() */
#include <stdio.h>    /* for fprintf()      */
#include <stdlib.h>   /* for rand()         */
#include <unistd.h>   /* for sleep()        */
#include <string.h>   /* for strcmp()       */

/* Problem with unsigned long long support in NBA and PKG41 !!! */
#undef uint64_t
#define uint64_t uint32_t

#include "pngtest3.h"

/* the super interface */
static IDirectFB *dfb;

/* the primary surface */
static IDirectFBSurface *primary;
static IDirectFBSurface *dest;

/* our "Press any key..." screen */
static IDirectFBSurface *intro;

/* some test images for blitting */
static IDirectFBSurface *cardicon;
static IDirectFBSurface *logo;
static IDirectFBSurface *simple;
static IDirectFBSurface *colorkeyed;
static IDirectFBSurface *image32;
static IDirectFBSurface *image32a;
static IDirectFBSurface *image_lut;

static IDirectFBFont    *bench_font;
static IDirectFBFont    *ui_font;

static int stringwidth;
static int bench_fontheight;
static int ui_fontheight;

/* Media super interface and the provider for our images/font */
static IDirectFBImageProvider *provider;

/* Input interfaces: event buffer */
static IDirectFBEventBuffer *key_events;

static int SW, SH;

static int with_intro   = 0;
static int selfrunning  = 0;
static int do_system    = 0;
static int do_offscreen = 0;
static int show_results = 1;
static int mono_fonts   = 0;

/* some defines for benchmark test size and duration */
static int SX = 256;
static int SY = 256;

static int DEMOTIME = 3000;  /* milliseconds */

static const char *fontfile = FONT;


#define MAX(a,b) ((a) > (b) ? (a) : (b))


/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                     \
               err = x;                                                    \
               if (err != DFB_OK) {                                        \
                    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
                    DirectFBErrorFatal( #x, err );                         \
               }


/* the benchmarks */

static uint64_t  draw_string             ( long t );
static uint64_t  draw_string_blend       ( long t );
static uint64_t  fill_rect               ( long t );
static uint64_t  fill_rect_blend         ( long t );
static uint64_t  fill_rects              ( long t );
static uint64_t  fill_rects_blend        ( long t );
static uint64_t  fill_triangle           ( long t );
static uint64_t  fill_triangle_blend     ( long t );
static uint64_t  draw_rect               ( long t );
static uint64_t  draw_rect_blend         ( long t );
static uint64_t  draw_lines              ( long t );
static uint64_t  draw_lines_blend        ( long t );
static uint64_t  fill_spans              ( long t );
static uint64_t  fill_spans_blend        ( long t );
static uint64_t  blit                    ( long t );
static uint64_t  blit_colorkeyed         ( long t );
static uint64_t  blit_dst_colorkeyed     ( long t );
static uint64_t  blit_convert            ( long t );
static uint64_t  blit_blend              ( long t );
static uint64_t  blit_lut                ( long t );
static uint64_t  blit_lut_blend          ( long t );
static uint64_t  stretch_blit            ( long t );
static uint64_t  stretch_blit_colorkeyed ( long t );


typedef struct {
     char       *desc;
     char       *message;
     char       *status;
     char       *option;
     bool        default_on;
     int         requested;
     long        result;
     DFBBoolean  accelerated;
     char       *unit;
     uint64_t (* func) ( long );
} Demo;

static Demo demos[] = {
  { "Anti-aliased Text",
    "This is the DirectFB benchmarking tool, let's start with some text!",
    "Anti-aliased Text", "draw-string", true,
    0, 0, 0, "KChars/sec",  draw_string },
  { "Anti-aliased Text (blend)",
    "Alpha blending based on color alpha",
    "Alpha Blended Anti-aliased Text", "draw-string-blend", true,
    0, 0, 0, "KChars/sec",  draw_string_blend },
  { "Fill Rectangle",
    "Ok, we'll go on with some opaque filled rectangles!",
    "Rectangle Filling", "fill-rect", true,
    0, 0, 0, "MPixel/sec", fill_rect },
  { "Fill Rectangle (blend)",
    "What about alpha blended rectangles?",
    "Alpha Blended Rectangle Filling", "fill-rect-blend", true,
    0, 0, 0, "MPixel/sec", fill_rect_blend },
  { "Fill Rectangles [10]",
    "Ok, we'll go on with some opaque filled rectangles!",
    "Rectangle Filling", "fill-rects", true,
    0, 0, 0, "MPixel/sec", fill_rects },
  { "Fill Rectangles [10] (blend)",
    "What about alpha blended rectangles?",
    "Alpha Blended Rectangle Filling", "fill-rects-blend", true,
    0, 0, 0, "MPixel/sec", fill_rects_blend },
  { "Fill Triangles",
    "Ok, we'll go on with some opaque filled triangles!",
    "Triangle Filling", "fill-triangle", true,
    0, 0, 0, "MPixel/sec", fill_triangle },
  { "Fill Triangles (blend)",
    "What about alpha blended triangles?",
    "Alpha Blended Triangle Filling", "fill-triangle-blend", true,
    0, 0, 0, "MPixel/sec", fill_triangle_blend },
  { "Draw Rectangle",
    "Now pass over to non filled rectangles!",
    "Rectangle Outlines", "draw-rect", true,
    0, 0, 0, "KRects/sec", draw_rect },
  { "Draw Rectangle (blend)",
    "Again, we want it with alpha blending!",
    "Alpha Blended Rectangle Outlines", "draw-rect-blend", true,
    0, 0, 0, "KRects/sec", draw_rect_blend },
  { "Draw Lines [10]",
    "Can we have some opaque lines, please?",
    "Line Drawing", "draw-line", true,
    0, 0, 0, "KLines/sec", draw_lines },
  { "Draw Lines [10] (blend)",
    "So what? Where's the blending?",
    "Alpha Blended Line Drawing", "draw-line-blend", true,
    0, 0, 0, "KLines/sec", draw_lines_blend },
  { "Fill Spans",
    "Can we have some spans, please?",
    "Span Filling", "fill-span", true,
    0, 0, 0, "MPixel/sec", fill_spans },
  { "Fill Spans (blend)",
    "So what? Where's the blending?",
    "Alpha Blended Span Filling", "fill-span-blend", true,
    0, 0, 0, "MPixel/sec", fill_spans_blend },
  { "Blit",
    "Now lead to some blitting demos! The simplest one comes first...",
    "Simple BitBlt", "blit", true,
    0, 0, 0, "MPixel/sec", blit },
  { "Blit colorkeyed",
    "Color keying would be nice...",
    "BitBlt with Color Keying", "blit-colorkeyed", false,
    0, 0, 0, "MPixel/sec", blit_colorkeyed },
  { "Blit destination colorkeyed",
    "Destination color keying is also possible...",
    "BitBlt with Destination Color Keying", "blit-dst-colorkeyed", false,
    0, 0, 0, "MPixel/sec", blit_dst_colorkeyed },
  { "Blit with format conversion",
    "What if the source surface has another format?",
    "BitBlt with on-the-fly format conversion", "blit-convert", true,
    0, 0, 0, "MPixel/sec", blit_convert },
  { "Blit from 32bit (alphachannel blend)",
    "Here we go with alpha again!",
    "BitBlt with Alpha Channel", "blit-blend", true,
    0, 0, 0, "MPixel/sec", blit_blend },
  { "Blit from 8bit palette",
    "Or even a palette?",
    "BitBlt from palette", "blit-lut", true,
    0, 0, 0, "MPixel/sec", blit_lut },
  { "Blit from 8bit palette (alphachannel blend)",
    "With alpha blending based on alpha entries",
    "BitBlt from palette (alphachannel blend)", "blit-lut-blend", true,
    0, 0, 0, "MPixel/sec", blit_lut_blend },
  { "Stretch Blit",
    "Stretching!!!!!",
    "Stretch Blit", "stretch-blit", true,
    0, 0, 0, "MPixel/sec", stretch_blit },
  { "Stretch Blit colorkeyed",
    "Stretching with Color Keying!!!",
    "Stretch Blit with Color Keying", "stretch-blit-colorkeyed", false,
    0, 0, 0, "MPixel/sec", stretch_blit_colorkeyed }
};
static int num_demos = sizeof( demos ) / sizeof (demos[0]);

static Demo *current_demo;

static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;

static inline unsigned int myrand()
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
     rand_add  += rand_pool;

     return rand_pool;
}

static inline long myclock()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void print_usage()
{
     int i;

     printf ("DirectFB Benchmarking Demo version " VERSION "\n\n");
     printf ("Usage: df_dok [options]\n\n");
     printf ("Options:\n\n");
     printf ("  --duration <milliseconds>    Duration of each benchmark.\n");
     printf ("  --size     <width>x<height>  Set benchmark size.\n");
     printf ("  --system                     Do benchmarks in system memory.\n");
     printf ("  --offscreen                  Do benchmarks in offscreen memory.\n");
     printf ("  --font <filename>            Use the specified font file.\n");
     printf ("  --mono                       Load fonts without anti-aliasing.\n");
     printf ("  --noresults                  Don't show results screen.\n");
     printf ("  --help                       Print usage information.\n");
     printf ("  --dfb-help                   Output DirectFB usage information.\n\n");
     printf ("The following options allow to specify which benchmarks to run.\n");
     printf ("If none of these are given, all benchmarks are run.\n\n");
     for (i = 0; i < num_demos; i++) {
          printf ("  --%-26s %s\n", demos[i].option, demos[i].desc);
     }
     printf ("\n");
}

static void shutdown()
{
     /* release our interfaces to shutdown DirectFB */
     bench_font->Release( bench_font );
     ui_font->Release( ui_font );
     if (with_intro)
          intro->Release( intro );
     logo->Release( logo );
     simple->Release( simple );
     cardicon->Release( cardicon );
     colorkeyed->Release( colorkeyed );
     image32->Release( image32 );
     image32a->Release( image32a );
     image_lut->Release( image_lut );
     dest->Release( dest );
     primary->Release( primary );
     key_events->Release( key_events );
     dfb->Release( dfb );
}

static void showMessage( const char *msg )
{
     DFBInputEvent ev;
     int err;

     while (key_events->GetEvent( key_events, DFB_EVENT(&ev) ) == DFB_OK) {
          if (ev.type == DIET_KEYPRESS) {
               switch (ev.key_symbol) {
                    case DIKS_ESCAPE:
                    case DIKS_SMALL_Q:
                    case DIKS_CAPITAL_Q:
                    case DIKS_BACK:
                    case DIKS_STOP:
                         shutdown();
                         exit( 42 );
                         break;
                    default:
                         break;
               }
          }
     }

     if (with_intro) {
          primary->SetBlittingFlags( primary, DSBLIT_NOFX );
          DFBCHECK(primary->Blit( primary, intro, NULL, 0, 0 ));

          primary->SetDrawingFlags( primary, DSDRAW_NOFX );
          primary->SetColor( primary, 0xFF, 0xFF, 0xFF, 0xFF );
          DFBCHECK(primary->DrawString( primary,
                                        msg, -1, SW/2, SH/2, DSTF_CENTER ));

          if (selfrunning) {
               usleep(1500000);
          }
          else {
               key_events->Reset( key_events );
               key_events->WaitForEvent( key_events );
          }
     }

     primary->Clear( primary, 0, 0, 0, 0x80 );
}

static void showResult()
{
     IDirectFBSurface       *meter;
     IDirectFBImageProvider *provider;
     DFBSurfaceDescription   dsc;
     DFBRectangle            rect;
     int   i, y, w, h, max_string_width = 0;
     char  rate[32];
     double factor = (SW-60) / 500000.0;

     if (dfb->CreateImageProvider( dfb,
                                   DATADIR"/meter.png", &provider ))
         return;

     provider->GetSurfaceDescription( provider, &dsc );
     dsc.height = dsc.height * SH / 1024;
     dfb->CreateSurface( dfb, &dsc, &meter );
     provider->RenderTo( provider, meter, NULL );
     provider->Release ( provider );

     cardicon->GetSize( cardicon, &w, &h );

     primary->Clear( primary, 0, 0, 0, 0x80 );

     primary->SetDrawingFlags( primary, DSDRAW_NOFX );
     primary->SetColor( primary, 0xFF, 0xFF, 0xFF, 0xFF );
     primary->DrawString( primary, "Results", -1,
                          SW/2, 2, DSTF_TOPCENTER );

     rect.x = 40;
     rect.y = ui_fontheight * 2;
     rect.h = dsc.height;

     primary->SetColor( primary, 0x66, 0x66, 0x66, 0xFF );
     primary->SetBlittingFlags( primary, DSBLIT_NOFX );

     for (i = 0; i < num_demos; i++) {
          if (!demos[i].requested)
               continue;

          rect.w = (double) demos[i].result * factor;
          primary->StretchBlit( primary, meter, NULL, &rect );
          if (rect.w < SW-60)
               primary->DrawLine( primary,
                                  40 + rect.w, rect.y + dsc.height,
                                  SW-20, rect.y + dsc.height );

          rect.y += dsc.height/2 + ui_fontheight + 2;
     }

     meter->Release( meter );

     y = ui_fontheight * 2 + dsc.height/2;
     for (i = 0; i < num_demos; i++) {
          if (!demos[i].requested)
               continue;

          primary->SetColor( primary, 0xCC, 0xCC, 0xCC, 0xFF );
          primary->DrawString( primary, demos[i].desc, -1, 20, y, DSTF_BOTTOMLEFT );

          snprintf( rate, sizeof (rate), "%2ld.%.3ld %s",
                    demos[i].result / 1000, demos[i].result % 1000, demos[i].unit);

          ui_font->GetStringExtents( ui_font, rate, -1, NULL, &rect );
          if (max_string_width < rect.w)
               max_string_width = rect.w;

          primary->SetColor( primary, 0xAA, 0xAA, 0xAA, 0xFF );
          primary->DrawString( primary, rate, -1, SW-20, y, DSTF_BOTTOMRIGHT );

          y += dsc.height/2 + ui_fontheight + 2;
     }

     y = ui_fontheight * 2 + dsc.height/2;
     for (i = 0; i < num_demos; i++) {
          if (!demos[i].requested)
               continue;

          if (demos[i].accelerated)
               primary->SetBlittingFlags( primary, DSBLIT_SRC_COLORKEY );
          else {
               primary->SetBlittingFlags( primary, DSBLIT_COLORIZE | DSBLIT_SRC_COLORKEY );
               primary->SetColor( primary, 0x20, 0x40, 0x40, 0xff );
          }
          primary->Blit( primary, cardicon,
                         NULL, SW - max_string_width - w - 25, y - h );

          y += dsc.height/2 + ui_fontheight + 2;
     }

     key_events->Reset( key_events );
     key_events->WaitForEvent( key_events );
}

static void showStatus( const char *msg )
{
     primary->SetColor( primary, 0x40, 0x80, 0xFF, 0xFF );
     primary->DrawString( primary,
                          "DirectFB Benchmarking Demo:", -1,
                          ui_fontheight*5/3, SH, DSTF_TOP );

     primary->SetColor( primary, 0xFF, 0x00, 0x00, 0xFF );
     primary->DrawString( primary, msg, -1, SW-2, SH, DSTF_TOPRIGHT );

     if (do_system) {
          primary->SetColor( primary, 0x80, 0x80, 0x80, 0xFF );
          primary->DrawString( primary,
                               "Performing benchmark in system memory...",
                               -1, SW/2, SH/2, DSTF_CENTER );
     }
     else if (do_offscreen) {
          primary->SetColor( primary, 0x80, 0x80, 0x80, 0xFF );
          primary->DrawString( primary,
                               "Performing benchmark in offscreen memory...",
                               -1, SW/2, SH/2, DSTF_CENTER );
     }
}

static void showAccelerated( DFBAccelerationMask  func,
                             IDirectFBSurface    *source )
{
     DFBAccelerationMask mask;

     dest->GetAccelerationMask( dest, source, &mask );

     if (mask & func) {
          primary->SetBlittingFlags( primary, DSBLIT_SRC_COLORKEY );

          current_demo->accelerated = DFB_TRUE;
     }
     else {
          primary->SetBlittingFlags( primary, DSBLIT_COLORIZE | DSBLIT_SRC_COLORKEY );
          primary->SetColor( primary, 0x20, 0x40, 0x40, 0xff );
     }

     primary->Blit( primary, cardicon,
                    NULL, ui_fontheight/4, SH + ui_fontheight/10 );
}

/**************************************************************************************************/

static uint64_t draw_string( long t )
{
     long i;

     dest->SetDrawingFlags( dest, DSDRAW_NOFX );

     showAccelerated( DFXL_DRAWSTRING, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, 0xFF );
          dest->DrawString( dest,
                            "DirectX is dead, this is DirectFB!!!", -1,
                            myrand() % (SW-stringwidth),
                            myrand() % (SH-bench_fontheight),
                            DSTF_TOPLEFT );
     }
     return 1000*36*(uint64_t)i;
}

static uint64_t draw_string_blend( long t )
{
     long i;

     dest->SetDrawingFlags( dest, DSDRAW_BLEND );

     showAccelerated( DFXL_DRAWSTRING, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF,
                          myrand()%0x64 );
          dest->DrawString( dest,
                            "DirectX is dead, this is DirectFB!!!", -1,
                            myrand() % (SW-stringwidth),
                            myrand() % (SH-bench_fontheight),
                            DSTF_TOPLEFT );
     }
     return 1000*36*(uint64_t)i;
}

static uint64_t fill_rect( long t )
{
     long i;

     dest->SetDrawingFlags( dest, DSDRAW_NOFX );

     showAccelerated( DFXL_FILLRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, 0xFF );
          dest->FillRectangle( dest,
                               myrand()%(SW-SX), myrand()%(SH-SY), SX, SY );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t fill_rect_blend( long t )
{
     long i;

     dest->SetDrawingFlags( dest, DSDRAW_BLEND );

     showAccelerated( DFXL_FILLRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->SetColor( dest, myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, myrand()%0x64 );
          dest->FillRectangle( dest, myrand()%(SW-SX), myrand()%(SH-SY), SX, SY );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t fill_rects( long t )
{
     long i, l;
     DFBRectangle rects[10];

     dest->SetDrawingFlags( dest, DSDRAW_NOFX );

     showAccelerated( DFXL_FILLRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          for (l=0; l<10; l++) {
               rects[l].x = myrand()%(SW-SX);
               rects[l].y = myrand()%(SH-SY);
               rects[l].w = SX;
               rects[l].h = SY;
          }

          dest->SetColor( dest, myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, 0xFF );
          dest->FillRectangles( dest, rects, 10 );
     }

     return SX*SY*10*(uint64_t)i;
}

static uint64_t fill_rects_blend( long t )
{
     long i, l;
     DFBRectangle rects[10];

     dest->SetDrawingFlags( dest, DSDRAW_BLEND );

     showAccelerated( DFXL_FILLRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          for (l=0; l<10; l++) {
               rects[l].x = myrand()%(SW-SX);
               rects[l].y = myrand()%(SH-SY);
               rects[l].w = SX;
               rects[l].h = SY;
          }

          dest->SetColor( dest, myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, myrand()%0x64 );
          dest->FillRectangles( dest, rects, 10 );
     }

     return SX*SY*10*(uint64_t)i;
}

static uint64_t fill_triangle( long t )
{
     long i, x, y;

     dest->SetDrawingFlags( dest, DSDRAW_NOFX );

     showAccelerated( DFXL_FILLTRIANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          x = myrand()%(SW-SX);
          y = myrand()%(SH-SY);

          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, 0xFF );
          dest->FillTriangle( dest, x, y, x+SX-1, y+SY/2, x, y+SY-1 );
     }
     return SX*SY*(uint64_t)i/2;
}

static uint64_t fill_triangle_blend( long t )
{
     long i, x, y;

     dest->SetDrawingFlags( dest, DSDRAW_BLEND );

     showAccelerated( DFXL_FILLTRIANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          x = myrand()%(SW-SX);
          y = myrand()%(SH-SY);

          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF,
                          myrand()%0x64 );
          dest->FillTriangle( dest, x, y, x+SX-1, y+SY/2, x, y+SY-1 );
     }
     return SX*SY*(uint64_t)i/2;
}

static uint64_t draw_rect( long t )
{
     long i;

     dest->SetDrawingFlags( dest, DSDRAW_NOFX );

     showAccelerated( DFXL_DRAWRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, 0xFF );
          dest->DrawRectangle( dest,
                               myrand()%(SW-SX), myrand()%(SH-SY), SX, SY );
     }
     return 1000*(uint64_t)i;
}

static uint64_t draw_rect_blend( long t )
{
     long i;

     dest->SetDrawingFlags( dest, DSDRAW_BLEND );

     showAccelerated( DFXL_DRAWRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF,
                          myrand()%0x64 );
          dest->DrawRectangle( dest,
                               myrand()%(SW-SX), myrand()%(SH-SY), SX, SY );
     }
     return 1000*(uint64_t)i;
}

static uint64_t draw_lines( long t )
{
     long i, l, x, y, dx, dy;
     DFBRegion lines[10];

     dest->SetDrawingFlags( dest, DSDRAW_NOFX );

     showAccelerated( DFXL_DRAWLINE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {

          for (l=0; l<10; l++) {
               x  = myrand() % (SW-SX) + SX/2;
               y  = myrand() % (SH-SY) + SY/2;
               dx = myrand() % (2*SX) - SX;
               dy = myrand() % (2*SY) - SY;

               lines[l].x1 = x - dx/2;
               lines[l].y1 = y - dy/2;
               lines[l].x2 = x + dx/2;
               lines[l].y2 = y + dy/2;
          }

          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF, 0xFF );
          dest->DrawLines( dest, lines, 10 );
     }
     return 1000*10*(uint64_t)i;
}

static uint64_t draw_lines_blend( long t )
{
     long i, l, x, y, dx, dy;
     DFBRegion lines[10];

     dest->SetDrawingFlags( dest, DSDRAW_BLEND );

     showAccelerated( DFXL_DRAWLINE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {

          for (l=0; l<10; l++) {
               x  = myrand() % (SW-SX) + SX/2;
               y  = myrand() % (SH-SY) + SY/2;
               dx = myrand() % (2*SX) - SX;
               dy = myrand() % (2*SY) - SY;

               lines[l].x1 = x - dx/2;
               lines[l].y1 = y - dy/2;
               lines[l].x2 = x + dx/2;
               lines[l].y2 = y + dy/2;
          }

          dest->SetColor( dest,
                          myrand()%0xFF, myrand()%0xFF, myrand()%0xFF,
                          myrand()%0x64 );
          dest->DrawLines( dest, lines, 10 );
     }
     return 1000*10*(uint64_t)i;
}

static uint64_t fill_spans_with_flags( long t, DFBSurfaceDrawingFlags flags )
{
     long    i;
     DFBSpan spans[SY];

     dest->SetDrawingFlags( dest, flags );

     showAccelerated( DFXL_FILLRECTANGLE, NULL );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          int w = myrand() % 25 + 5;
          int x = myrand() % (SW-SX-w*2) + w;
          int d = 0;
          int a = 1;
          int l;

          for (l=0; l<SY; l++) {
               spans[l].x = x + d;
               spans[l].w = SX;

               d += a;

               if (d == w)
                    a = -1;
               else if (d == -w)
                    a = 1;
          }

          dest->SetColor( dest, myrand()%0xFF, myrand()%0xFF, myrand()%0xFF,
                          (flags & DSDRAW_BLEND) ? myrand()%0x64 : 0xff );
          dest->FillSpans( dest, myrand() % (SH-SY), spans, SY );
     }

     return SX * SY * (uint64_t) i;
}

static uint64_t fill_spans( long t )
{
     return fill_spans_with_flags( t, DSDRAW_NOFX );
}

static uint64_t fill_spans_blend( long t )
{
     return fill_spans_with_flags( t, DSDRAW_BLEND );
}


static uint64_t blit( long t )
{
     long i;

     dest->SetBlittingFlags( dest, DSBLIT_NOFX );

     showAccelerated( DFXL_BLIT, simple );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, simple, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SH-SY)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}


static uint64_t blit_dst_colorkeyed( long t )
{
     long i;
     DFBRegion clip;

     clip.x1 = 0;
     clip.x2 = SW-1;
     clip.y1 = 0;
     clip.y2 = SH-1;

     dest->SetClip( dest, &clip );
     dest->SetBlittingFlags( dest, DSBLIT_NOFX );
     dest->TileBlit( dest, logo, NULL, 0, 0 );
     dest->SetClip( dest, NULL );

     dest->SetBlittingFlags( dest, DSBLIT_DST_COLORKEY );
     dest->SetDstColorKey( dest, 0xFF, 0xFF, 0xFF );

     showAccelerated( DFXL_BLIT, simple );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, simple, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SY-SH)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t blit_colorkeyed( long t )
{
     long i;

     dest->SetBlittingFlags( dest, DSBLIT_SRC_COLORKEY );

     showAccelerated( DFXL_BLIT, colorkeyed );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, colorkeyed, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SY-SH)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t blit_convert( long t )
{
     long i;

     dest->SetBlittingFlags( dest, DSBLIT_NOFX );

     showAccelerated( DFXL_BLIT, image32 );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, image32, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SY-SH)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t blit_lut( long t )
{
     long i;

     dest->SetBlittingFlags( dest, DSBLIT_NOFX );

     showAccelerated( DFXL_BLIT, image_lut );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, image_lut, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SY-SH)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t blit_lut_blend( long t )
{
     long i;

     dest->SetBlittingFlags( dest, DSBLIT_BLEND_ALPHACHANNEL );

     showAccelerated( DFXL_BLIT, image_lut );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, image_lut, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SY-SH)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t blit_blend( long t )
{
     long i;

     dest->SetBlittingFlags( dest, DSBLIT_BLEND_ALPHACHANNEL );

     showAccelerated( DFXL_BLIT, image32a );

     for (i=0; i%100 || myclock()<(t+DEMOTIME); i++) {
          dest->Blit( dest, image32a, NULL,
                      (SW!=SX) ? myrand() % (SW-SX) : 0,
                      (SY-SH)  ? myrand() % (SH-SY) : 0 );
     }
     return SX*SY*(uint64_t)i;
}

static uint64_t stretch_blit( long t )
{
     long i, j;
     uint64_t pixels = 0;

     dest->SetBlittingFlags( dest, DSBLIT_NOFX );

     showAccelerated( DFXL_STRETCHBLIT, simple );

     for (j=1; myclock()<(t+DEMOTIME); j++) {
          if (j>SH) {
               j = 10;
          }
          for (i=10; i<SH; i+=j) {
               DFBRectangle dr = { SW/2-i/2, SH/2-i/2, i, i };

               dest->StretchBlit( dest, simple, NULL, &dr );

               pixels += dr.w * dr.h;
          }
     }
     return pixels;
}

static uint64_t stretch_blit_colorkeyed( long t )
{
     long i, j;
     uint64_t pixels = 0;

     dest->SetBlittingFlags( dest, DSBLIT_SRC_COLORKEY );

     showAccelerated( DFXL_STRETCHBLIT, simple );

     for (j=1; myclock()<(t+DEMOTIME); j++) {
          if (j>SH) {
               j = 10;
          }
          for (i=10; i<SH; i+=j) {
               DFBRectangle dr = { SW/2-i/2, SH/2-i/2, i, i };

               dest->StretchBlit( dest, colorkeyed, NULL, &dr );

               pixels += dr.w * dr.h;
          }
     }
     return pixels;
}

/**************************************************************************************************/

int main( int argc, char *argv[] )
{
     DFBResult ret = EXIT_SUCCESS;

     DFBResult err;
     DFBSurfacePixelFormat pixelformat;
     DFBSurfaceDescription dsc;
     DFBImageDescription image_dsc;
     DFBRectangle clip;
     int i, n;
     int demo_requested = 0;

     /* bdvd_init begin */
     bdvd_display_h bdisplay = NULL;
     if (bdvd_init(BDVD_VERSION) != b_ok) {
         D_ERROR("Could not init bdvd\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
         printf("bdvd_display_open failed\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     /* bdvd_init end */

     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     for (n = 1; n < argc; n++) {
          if (strncmp (argv[n], "--", 2) == 0) {
               for (i = 0; i < num_demos; i++) {
                    if (strcmp (argv[n] + 2, demos[i].option) == 0) {
                         demo_requested = 1;
                         demos[i].requested = 1;
                         break;
                    }
               }
               if (i == num_demos) {
                    if (strcmp (argv[n] + 2, "help") == 0) {
                         print_usage();
                         return EXIT_SUCCESS;
                    }
                    else
                    if (strcmp (argv[n] + 2, "noresults") == 0) {
                         show_results = 0;
                         continue;
                    }
                    else
                    if (strcmp (argv[n] + 2, "system") == 0) {
                         do_system = 1;
                         do_offscreen = 1;
                         continue;
                    }
                    else
                    if (strcmp (argv[n] + 2, "offscreen") == 0) {
                         do_offscreen = 1;
                         continue;
                    }
                    else
                    if (strcmp (argv[n] + 2, "mono") == 0) {
                         mono_fonts = 1;
                         demos[0].desc   = "Monochrome Text";
                         demos[0].status = "Monochrome Text";
                         demos[1].desc   = "Monochrome Text (blend)";
                         demos[1].status = "Alpha Blended Monochrome Text";
                         continue;
                    }
                    else
                    if (strcmp (argv[n] + 2, "size") == 0 &&
                        ++n < argc &&
                        sscanf (argv[n], "%dx%d", &SX, &SY) == 2) {
                         continue;
                    }
                    else
                    if (strcmp (argv[n] + 2, "duration") == 0 &&
                        ++n < argc &&
                        sscanf (argv[n], "%d", &DEMOTIME) == 1) {
                         continue;
                    }
                    else
                    if (strcmp (argv[n] + 2, "font") == 0 &&
                        ++n < argc && argv[n]) {
                         fontfile = argv[n];
                         continue;
                    }
               }
               else {
                    continue;
               }
          }

          print_usage();
          return EXIT_FAILURE;
     }
     if (!demo_requested) {
          for (i = 0; i < num_demos; i++) {
               demos[i].requested = demos[i].default_on;
          }
     }

     DirectFBSetOption ("bg-none", NULL);

     /* create the super interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* create an input buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS,
                                           DFB_FALSE, &key_events ));

     /* Set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer. */
     err = dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );
     if (err)
          DirectFBError( "Failed to get exclusive access", err );

     /* Get the primary surface, i.e. the surface of the primary layer. */
     dsc.flags = DSDESC_CAPS;
     dsc.caps = DSCAPS_PRIMARY;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));
     primary->GetPixelFormat( primary, &pixelformat );
     primary->GetSize( primary, &SW, &SH );
     primary->Clear( primary, 0, 0, 0, 0x80 );

     if (do_offscreen) {
          dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
          dsc.width = SW;
          dsc.height = SH;
          dsc.pixelformat = pixelformat;

          if (do_system) {
              dsc.flags |= DSDESC_CAPS;
              dsc.caps = DSCAPS_SYSTEMONLY;
          }

          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &dest ));

          dest->Clear( dest, 0, 0, 0, 0x80 );
     }
     else {
          primary->GetSubSurface (primary, NULL, &dest);
     }

     {
          DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/biglogo.png",
                                             &provider ));
          DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));

          dsc.width  = (SH / 8) * dsc.width / dsc.height;
          dsc.height = SH / 8;

          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &logo ));
          DFBCHECK(provider->RenderTo( provider, logo, NULL ));
          provider->Release( provider );

          primary->SetBlittingFlags( primary, DSBLIT_BLEND_ALPHACHANNEL );
          primary->Blit( primary, logo, NULL, (SW - dsc.width) / 2, SH / 5 );
     }

     {
          DFBFontDescription desc;

          desc.flags      = DFDESC_HEIGHT | DFDESC_ATTRIBUTES;
          desc.height     = 22;
          desc.attributes = mono_fonts ? DFFA_MONOCHROME : DFFA_NONE;

          DFBCHECK(dfb->CreateFont( dfb, fontfile, &desc, &bench_font ));

          bench_font->GetHeight( bench_font, &bench_fontheight );

          bench_font->GetStringWidth( bench_font,
                                      "DirectX is dead, this is DirectFB!!!", -1,
                                      &stringwidth );

          dest->SetFont( dest, bench_font );
     }

     primary->SetFont( primary, bench_font );
     primary->SetColor( primary, 0xA0, 0xA0, 0xA0, 0xFF );
     primary->DrawString( primary, "Preparing...", -1,
                          SW / 2, SH / 2, DSTF_CENTER );

     {
          DFBFontDescription desc;

          desc.flags      = DFDESC_HEIGHT | DFDESC_ATTRIBUTES;
          desc.height     = 1 + 24 * SH / 1024;
          desc.attributes = mono_fonts ? DFFA_MONOCHROME : DFFA_NONE;

          DFBCHECK(dfb->CreateFont( dfb, fontfile, &desc, &ui_font ));

          ui_font->GetHeight( ui_font, &ui_fontheight );

          primary->SetFont( primary, ui_font );
     }

     SH -= ui_fontheight;

     if (SX > SW - 10)
          SX = SW - 10;
     if (SY > SH - 10)
          SY = SH - 10;

     /* create a surface and render an image to it */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/card.png", &provider ));
     DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));
     dsc.width  = dsc.width * (ui_fontheight - ui_fontheight/5) / dsc.height;
     dsc.height = (ui_fontheight - ui_fontheight/5);
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &cardicon ));
     DFBCHECK(provider->RenderTo( provider, cardicon, NULL ));
     provider->Release( provider );

     /* create a surface and render an image to it */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/melted.png",
                                        &provider ));
     DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));

     dsc.width = SX;
     dsc.height = SY;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &simple ));
     DFBCHECK(provider->RenderTo( provider, simple, NULL ));
     provider->Release( provider );

     /* create a surface and render an image to it */

     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/colorkeyed.gif",
                                        &provider ));
     DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));

     dsc.width = SX;
     dsc.height = SY;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &colorkeyed ));
     DFBCHECK(provider->RenderTo( provider, colorkeyed, NULL ));

     provider->GetImageDescription( provider, &image_dsc);

     if (image_dsc.caps & DICAPS_COLORKEY)
          colorkeyed->SetSrcColorKey( colorkeyed,
                                      image_dsc.colorkey_r,
                                      image_dsc.colorkey_g,
                                      image_dsc.colorkey_b );

     provider->Release( provider );

     /* create a surface and render an image to it */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/pngtest.png",
                                        &provider ));
     DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));

     dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     dsc.width = SX;
     dsc.height = SY;
     dsc.pixelformat = DFB_BYTES_PER_PIXEL(pixelformat) == 2 ?
                       DSPF_RGB32 : DSPF_RGB16;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image32 ));
     DFBCHECK(provider->RenderTo( provider, image32, NULL ));
     provider->Release( provider );

     /* create a surface and render an image to it */
     dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     dsc.width = SX;
     dsc.height = SY;
     dsc.pixelformat = DSPF_ARGB;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image32a ));
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/pngtest2.png",
                                        &provider ));
     DFBCHECK(provider->RenderTo( provider, image32a, NULL ));
     provider->Release( provider );

     /* create a surface and render an image to it */
     {
          IDirectFBSurface *tmp;
          IDirectFBPalette *palette;
          DFBSurfaceCapabilities primary_caps;
          uint8_t  *ptr;
          unsigned int  pitch;
          int i;

          DFBCHECK(dfb->CreateSurface( dfb, &pngtest3_png_desc, &tmp ));

          DFBCHECK(tmp->GetPalette( tmp, &palette ));

          dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
          dsc.width = SX;
          dsc.height = SY;
          dsc.pixelformat = DSPF_LUT8;

          if (getenv("lut_surfaces_premult")) {
              DFBCHECK(primary->GetCapabilities(primary, &primary_caps));

              if (primary_caps & DSCAPS_PREMULTIPLIED) {
                  dsc.flags |= DSDESC_CAPS;
                  dsc.caps = DSCAPS_PREMULTIPLIED;
              }
          }

          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image_lut ));

          DFBCHECK(image_lut->SetPalette( image_lut, palette ));

          image_lut->StretchBlit( image_lut, tmp, NULL, NULL );

          palette->Release( palette );
          tmp->Release( tmp );
     }

     if (with_intro) {
          /* create a surface and render an image to it */
          DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/intro.png",
                                             &provider ));
          DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));

          dsc.width = SW;
          dsc.height = SH + ui_fontheight;

          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &intro ));

          DFBCHECK(provider->RenderTo( provider, intro, NULL ));
          provider->Release( provider );
     }


     printf( "\nBenchmarking with %dx%d in %dbit mode... (%dbit)\n\n",
             SX, SY, DFB_COLOR_BITS_PER_PIXEL(pixelformat),
             DFB_BYTES_PER_PIXEL(pixelformat) * 8 );

     sync();
     sleep(2);

     clip.x = 0;
     clip.y = 0;
     clip.w = SW;
     clip.h = SH;

     for (i = 0; i < num_demos; i++) {
           long t, dt;
           uint64_t pixels;

           if (!demos[i].requested)
                continue;

           current_demo = &demos[i];
           
           showMessage( demos[i].message );
           showStatus( demos[i].status );

           sync();
           dfb->WaitIdle( dfb );
           t = myclock();
           pixels = (* demos[i].func)(t);
           dfb->WaitIdle( dfb );
           dt = myclock() - t;
           demos[i].result = pixels / (uint64_t) dt;
           printf( "%-44s %3ld.%.3ld secs (%s%4ld.%.3ld %s)\n",
                   demos[i].desc, dt / 1000, dt % 1000,
                   demos[i].accelerated ? "*" : " ",
                   demos[i].result / 1000,
                   demos[i].result % 1000, demos[i].unit);
           if (do_offscreen) {
                primary->SetBlittingFlags (primary, DSBLIT_NOFX);
                primary->Blit( primary, dest, &clip, 0, 0);
                sleep(2);
                dest->Clear( dest, 0, 0, 0, 0x80 );
           }
     }

     if (show_results)
          showResult();

     printf( "\n" );

out:

     shutdown();

     /* bdvd_uninit begin */
     if (bdisplay) bdvd_display_close(bdisplay);
     bdvd_uninit();
     /* bdvd_uninit end */
                                                                                                                                                             
     return ret;
}

