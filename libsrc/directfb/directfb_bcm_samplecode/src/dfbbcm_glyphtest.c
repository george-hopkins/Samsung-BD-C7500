
#include <stdio.h>
#include <directfb.h>

#define BCMPLATFORM

#ifdef BCMPLATFORM
#include <bdvd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <direct/debug.h>
#include <direct/clock.h>
#include <directfb-internal/misc/util.h>

#ifdef BCMPLATFORM
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_utils.h"
#endif

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

static int show_ascender     = 1;
static int show_descender    = 1;
static int show_baseline     = 1;
static int show_glyphrect    = 1;
static int show_glyphadvance = 1;
static int show_glyphorigin  = 1;

static int print_metrics     = 0;

static int glyphs_per_xline  = 1;
static int glyphs_per_yline  = 1;

/* this is a cheap ripoff of df_fonts.c */
static DFBResult 
render_font_page (IDirectFB          *dfb,
                  IDirectFBSurface   *surface,
                  const char         *fontname,
                  DFBFontDescription *fontdesc,
                  unsigned int        first_char)
{
     DFBResult ret = DFB_OK;
     IDirectFBFont *font = NULL;
     IDirectFBFont *fixedfont = NULL;
     DFBFontDescription fixed_fontdesc;
     unsigned int width, height;
     int bwidth, bheight;
     int xborder, yborder;
     int baseoffset;
     int ascender, descender;
     int default_fontheight;
     char label[32];
     int i, j;

     surface->GetSize (surface, &width, &height);

     /* [xm] I think this is overscan safe area */

     bwidth = width * 3 / 4;
     bheight = height * 3 / 4;

     xborder = (width - bwidth) / 2;
     yborder = (height - bheight) / 2;

     fixed_fontdesc.flags = DFDESC_ATTRIBUTES;
     fixed_fontdesc.attributes = 0;

     if (dfb->CreateFont (dfb, NULL, &fixed_fontdesc, &fixedfont) != DFB_OK) {
         D_ERROR("CreateFont failed\n");
         ret = DFB_FAILURE;
         goto out;
     }

     surface->SetFont (surface, fixedfont);

     default_fontheight = 9 * bheight / glyphs_per_yline / 16;
     
     if (default_fontheight < fontdesc->height) {
        D_INFO("%d font height too big, switching to %d\n", fontdesc->height, default_fontheight);
        fontdesc->height = default_fontheight;
     }

     if (dfb->CreateFont (dfb, fontname, fontdesc, &font) != DFB_OK) {
         D_ERROR("CreateFont failed\n");
         ret = DFB_FAILURE;
         goto out;
     }

     font->GetAscender (font, &ascender);
     font->GetDescender (font, &descender);

     baseoffset = ((bheight / glyphs_per_yline - (ascender - descender)) /
                   2 + ascender);

     surface->SetColor (surface, 0xa0, 0xa0, 0xa0, 0xff);

     surface->DrawString (surface,
                          fontname, -1, width/2, 10, DSTF_TOPCENTER);

     surface->DrawString (surface,
                          fontdesc->attributes & DFFA_NOCHARMAP ? "Raw Map" : "Unicode Map", -1,
                          10, 10, DSTF_TOPLEFT);

     snprintf (label, sizeof(label), "%d pixels", fontdesc->height);
     surface->DrawString (surface,
                          label, -1, width-10, 10, DSTF_TOPRIGHT);

     surface->SetColor (surface, 0xc0, 0xc0, 0xc0, 0xff);

     for (j=0; j<glyphs_per_yline; j++) {
          int basey;

          basey = j * bheight / glyphs_per_yline + yborder + baseoffset;

          snprintf (label, sizeof(label), "%04x",
                    first_char + j * glyphs_per_xline);

          surface->DrawString (surface, label, -1,
                               xborder-10, basey, DSTF_RIGHT);

          snprintf (label, sizeof(label), "%04x",
                    first_char + (j+1) * glyphs_per_yline - 1);

          surface->DrawString (surface, label, -1,
                               bwidth + xborder+10, basey, DSTF_LEFT);
     }

     fixedfont->Release (fixedfont);

     for (i=0; i<=glyphs_per_xline; i++)
          surface->DrawLine (surface,
                             i * bwidth / glyphs_per_xline + xborder,
                             yborder,
                             i * bwidth / glyphs_per_xline + xborder,
                             bheight + yborder);

     for (j=0; j<=glyphs_per_yline; j++)
          surface->DrawLine (surface,
                             xborder,
                             j * bheight / glyphs_per_yline + yborder,
                             bwidth + xborder,
                             j * bheight / glyphs_per_yline + yborder);

     if (show_ascender) {
          surface->SetColor (surface, 0xf0, 0x80, 0x80, 0xff);

          for (j=0; j<glyphs_per_yline; j++) {
               int basey;

               basey = j * bheight / glyphs_per_yline + yborder + baseoffset;
               surface->DrawLine (surface,
                                  xborder, basey - ascender,
                                  bwidth + xborder, basey - ascender);
          }
     }

     if (show_descender) {
          surface->SetColor (surface, 0x80, 0xf0, 0x80, 0xff);

          for (j=0; j<glyphs_per_yline; j++) {
               int basey;

               basey = j * bheight / glyphs_per_yline + yborder + baseoffset;
               surface->DrawLine (surface,
                                  xborder, basey - descender,
                                  bwidth + xborder, basey - descender);
          }
     }

     if (show_baseline) {
          surface->SetColor (surface, 0x80, 0x80, 0xf0, 0xff);

          for (j=0; j<glyphs_per_yline; j++) {
               int basey;

               basey = j * bheight / glyphs_per_yline + yborder + baseoffset;
               surface->DrawLine (surface,
                                  xborder, basey, bwidth + xborder, basey);
          }
     }

     surface->SetFont (surface, font);

     for (j=0; j<glyphs_per_yline; j++) {
          for (i=0; i<glyphs_per_xline; i++) {
               int basex;
               int basey;
               int glyphindex; 
               int glyphadvance;
               DFBRectangle glyphrect;

               basex = (2*i+1) * bwidth / glyphs_per_xline /2 + xborder;
               basey = j * bheight / glyphs_per_yline + yborder + baseoffset;
               
               glyphindex = first_char + i+j*glyphs_per_xline;

               font->GetGlyphExtents (font,
                                      glyphindex, &glyphrect, &glyphadvance);

               if (show_glyphrect) {
                    int x = basex + glyphrect.x - glyphrect.w/2;
                    int y = basey + glyphrect.y;

                    surface->SetColor (surface, 0xc0, 0xc0, 0xf0, 0xff);
                    surface->FillRectangle (surface,
                                            x, y, glyphrect.w, glyphrect.h);
               }

               if (show_glyphadvance) {
                    int y = (j+1) * bheight / glyphs_per_yline + yborder - 4;

                    surface->SetColor (surface, 0x30, 0xc0, 0x30, 0xff);
                    surface->FillRectangle (surface,
                                            basex - glyphrect.w / 2, y,
                                            glyphadvance, 3);
               }

               surface->SetColor (surface, 0x00, 0x00, 0x00, 0xff);

               surface->DrawGlyph (surface, glyphindex,
                                   basex - glyphrect.w/2,
                                   basey, DSTF_LEFT);

               if (show_glyphorigin) {
                    surface->SetColor (surface, 0xff, 0x30, 0x30, 0xff);
                    surface->FillRectangle (surface,
                                            basex-1, basey-1, 2, 2);
               }
          }
     }

out:

     if (font) font->Release (font);
     if (fixedfont) font->Release (fixedfont);

     return ret;
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;

    IDirectFB *dfb = NULL;

#ifdef BCMPLATFORM
    bdvd_dfb_ext_h graphics_dfb_ext = NULL;
    bdvd_dfb_ext_layer_settings_t graphics_dfb_ext_settings;
    DFBDisplayLayerConfig   graphics_layer_config;
#endif
    IDirectFBDisplayLayer * graphics_layer = NULL;
    IDirectFBSurface * graphics_surface = NULL;

#ifndef BCMPLATFORM
    DFBSurfaceDescription graphics_surface_desc;
#endif

    DFBRectangle surface_rect = {0,0,0,0};

    int32_t nbcharsprinted;
    char font_name[256] = "";
    DFBFontDescription fontdesc;
    uint32_t first_char = 65; /* Unicode for letter A */

    char c;

#ifdef BCMPLATFORM
    bvideo_format video_format = bvideo_format_1080i;

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
#endif

    if (argc < 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    fontdesc.flags = DFDESC_HEIGHT | DFDESC_ATTRIBUTES;
    fontdesc.height = 20;
    fontdesc.attributes = 0;

    while ((c = getopt (argc, argv, "c:f:himp:r")) != -1) {
        switch (c) {
        case 'c':
            nbcharsprinted = strtoul(optarg, NULL, 10);
        break;
        case 'f':
            snprintf(font_name, sizeof(font_name)/sizeof(font_name[0]), "%s", optarg);
        break;
        case 'h':
            fontdesc.attributes |= DFFA_NOHINTING;
        break;
        case 'i':
            fontdesc.attributes |= DFFA_NOCHARMAP;
        break;
        case 'm':
            fontdesc.attributes |= DFFA_MONOCHROME;
        break;
        case 'p':
            fontdesc.height = strtoul(optarg, NULL, 10);
        break;
        case 'r':
            video_format = bvideo_format_1080i;
        break;
        case 'o':
            first_char = strtoul(optarg, NULL, 10);
        break;
        case '?':
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
            goto out;
        break;
        }
    }

    if (display_set_video_format(bdisplay, video_format) != DFB_OK) {
        D_ERROR("display_set_video_format failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

#ifdef BCMPLATFORM
    dfb = bdvd_dfb_ext_open_with_params(2, &argc, &argv);
    if (dfb == NULL) {
        D_ERROR("bdvd_dfb_ext_open_with_params failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#if 0
    switch (video_format) {
        case bvideo_format_ntsc:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
        break;
        case bvideo_format_1080i:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
        break;
        default:
        break;
    }
#endif

    graphics_dfb_ext = bdvd_dfb_ext_layer_open(bdvd_dfb_ext_hddvd_ac_graphics_layer);
    if (graphics_dfb_ext == NULL) {
        D_ERROR("bdvd_dfb_ext_layer_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (bdvd_dfb_ext_layer_get(graphics_dfb_ext, &graphics_dfb_ext_settings) != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_layer_get failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (dfb->GetDisplayLayer(dfb, graphics_dfb_ext_settings.id, &graphics_layer) != DFB_OK) {
        D_ERROR("GetDisplayLayer failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->SetCooperativeLevel(graphics_layer, DLSCL_ADMINISTRATIVE) != DFB_OK) {
        D_ERROR("SetCooperativeLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    
    graphics_layer_config.flags = DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_SURFACE_CAPS;
    graphics_layer_config.width = 1920;
    graphics_layer_config.height = 1080;
    graphics_layer_config.surface_caps = DSCAPS_PREMULTIPLIED;
    
    if (graphics_layer->SetConfiguration(graphics_layer, &graphics_layer_config) != DFB_OK) {
        D_ERROR("SetConfiguration failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->SetLevel(graphics_layer, 1) != DFB_OK) {
        D_ERROR("SetLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if (graphics_layer->GetSurface(graphics_layer, &graphics_surface) != DFB_OK) {
        D_ERROR("GetSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#else
    if (DirectFBInit( &argc, &argv ) != DFB_OK) {
        D_ERROR("DirectFBInit failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
     /* create the super interface */
    if (DirectFBCreate( &dfb ) != DFB_OK) {
        D_ERROR("DirectFBCreate failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer. */
    if (dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ) != DFB_OK) {
        D_ERROR("SetCooperativeLevel failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    /* Get the primary surface, i.e. the surface of the primary layer. */
    graphics_surface_desc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;
    graphics_surface_desc.caps = DSCAPS_PRIMARY | DSCAPS_PREMULTIPLIED;
    graphics_surface_desc.width = 960;
    graphics_surface_desc.height = 540;

    if (dfb->CreateSurface( dfb, &graphics_surface_desc, &graphics_surface ) != DFB_OK) {
        D_ERROR("CreateSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
#endif
    /* End of graphics layer setup */
    if (graphics_surface->GetSize(graphics_surface, &surface_rect.w, &surface_rect.h) != DFB_OK) {
        D_ERROR("GetSurface failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_surface->Clear (graphics_surface, 0xff, 0xff, 0xff, 0xff) != DFB_OK) {
        D_ERROR("Clear failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (render_font_page (dfb, graphics_surface, strlen(font_name) ? font_name : NULL, &fontdesc, first_char) != DFB_OK) {
        D_ERROR("render_font_page failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (graphics_surface->Flip(graphics_surface, NULL, DSFLIP_NONE) != DFB_OK) {
        D_ERROR("Flip failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    D_INFO("Press any key to continue\n");
    getchar();

out:

    /* Cleanup */
    if (graphics_layer) {
        if (graphics_layer->SetLevel(graphics_layer, -1) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (graphics_surface) {
        if (graphics_surface->Release(graphics_surface) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (graphics_layer) {
        if (graphics_layer->Release(graphics_layer) != DFB_OK) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
#ifdef BCMPLATFORM
    if (graphics_dfb_ext) {
        if (bdvd_dfb_ext_layer_close(graphics_dfb_ext) != bdvd_ok) {
            D_ERROR("Release failed\n");
            ret = EXIT_FAILURE;
        }
    }
    if (bdvd_dfb_ext_close() != bdvd_ok) {
        D_ERROR("bdvd_dfb_ext_close failed\n");
        ret = EXIT_FAILURE;
    }
    /* End of cleanup */

    /* bdvd_uninit begin */
    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();
    /* bdvd_uninit end */
#else
    dfb->Release( dfb );
#endif

    return EXIT_SUCCESS;
}
