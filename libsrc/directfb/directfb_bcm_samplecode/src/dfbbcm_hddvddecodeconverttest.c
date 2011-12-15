#include <directfb.h>

#include <direct/debug.h>

#include <directfb-internal/misc/util.h>

#include <bdvd.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "dfbbcm_utils.h"
#include "dfbbcm_playbackscagatthread.h"

uint32_t kanjisimplecharset[] = {
    19968, /*ICHI, hito(tsu), one*/
    20108, /*NI, futa(tsu), two*/
    19977, /*SAN, mi, mit(tsu), three*/
    22235, /*SHI, yon, yo, yot(tsu), four*/
    20116, /*GO, itsu(tsu), five*/
    20845, /*ROKU, mut(tsu), six*/
    19971, /*SHICHI, nana(tsu), seven*/
    20843, /*HACHI, yat(tsu), eight*/
    20061, /*KU, KYUU, kokono(tsu), nine*/
    21313, /*JUU, tou, ten*/
    26085, /*NICHI, JITSU hi [bi], (-ka suffix: 'days'), day, sun */
    26376, /*GETSU, GATSU, tsuki [zuki], month, moon*/
    28779, /*KA, hi [bi], fire*/
    27700, /*SUI, mizu, water*/
    26408, /*MOKU, BOKU, ki [gi], tree, wood*/
    37329, /*KIN, gold; KON (gold); kane, money*/
    22303, /*DO, TO, tsuchi, earth, soil*/
    24038, /*SA, hidari, left*/
    21491, /*YUU, U, migi, right*/
    19978, /*JOU; ue, top, above, on; kami, upper;
             nobo(ru), to go up, to go toward Toukyou;
             a(geru), to raise; a(garu), to rise*/
    19979, /*KA, GE; shita, bottom, under, beneath; moto, base;
             shimo, lower; kuda(ru), to go down, to go away from Toukyou;
             sa(geru), to hang (v.t.), to lower;
             sa(garu), to hang down*/
    22823, /*DAI, TAI, ou(kii), big, large, great*/
    20013, /*CHUU, naka, middle, within, inside*/
    23567, /*SHOU, ko, o, chii(sai), small, minor*/
    30446, /*MOKU, me, eye; also used as an ordinal suffix*/
    32819, /*JI, mimi, ear*/
    21475, /*KOU, KU; kuchi [guchi], mouth*/
    25163, /*SHU; te, hand*/
    36275, /*SOKU [ZOKU]; ashi, foot, leg; ta(riru), to be sufficient*/
    20154, /*NIN, JIN; hito [bito], person*/
    23376, /*SHI, SU; ko [go], child*/
    22899, /*JO, NYO; onna, woman, girl*/
    20808, /*SEN; saki, previous, ahead*/
    29983, /*SEI, SHOU; birth, life;  u(mareru), to be born;
             u(mu), to give birth; i(kiru), to live;
             ki, pure, genuine; nama, raw*/
    36196, /*SEKI, SHAKU; aka, aka(i), red*/
    38738, /*SEI, SHOU; ao, ao(i), blue, green, inexperienced*/
    30333, /*HAKU, BYAKU; shiro [jiro], shiro(i), white*/
    23665, /*SAN [ZAN]; yama, mountain*/
    24029, /*SEN; kawa [gawa], river*/
    30000, /*DEN; ta [da], rice field*/
    26862, /*SHIN; mori, forest, grove*/
    38632, /*U; ame, rain*/
    33457, /*KA; hana, flower*/
    30707, /*SEKI, KOKU, SHAKU; ishi, stone*/
    26412, /*HON [BON, PON]; book, suffix for counting long, slender objects*/
    27491, /*SEI, SHOU; tada(shii), correct, right*/
    12293  /*kanji repetition symbol*/
};

uint32_t kanjicomplexcharset[] = {
    38911, /*U+97ff frequency 603 stroke 20 KYOU echo also sound resound ring vibrate */
    25080, /*U+61f8 frequency 1226 stroke 20 KEN KE suspend hang 10% install depend consult */
    39015, /*U+9867 frequency 1328 stroke 21 KO look back review examine oneself turn around */
    35703, /*U+8b77 frequency 570 stroke 20 GO safeguard protect */
    35186, /*U+8972 frequency 1184 stroke 22 SHUU attack advance on succeed to pile heap */
    37912, /*U+9418 frequency 1356 stroke 20 SHOU bell gong chimes */
    35698, /*U+8b72 frequency 1013 stroke 20 JOU defer turnover transfer convey */
    31821, /*U+7c4d frequency 1350 stroke 20 SEKI enroll domiciliary register membership */
    39472, /*U+9a30 frequency 1289 stroke 20 TOU inflation advancing going */
    39764, /*U+9b54 frequency 1316 stroke 21 MA witch demon evil spirit */
    36493, /*U+8e8d frequency 915 stroke 21 YAKU leap dance skip */
    27396, /*U+6b04 frequency 1307 stroke 20 RAN column handrail blank space */
    38706, /*U+9732 frequency 1176 stroke 21 RO ROU dew tears expose Russia */
    31840, /*U+7c60 frequency 1734 stroke 22 ROU RU basket devote oneself seclude oneself cage coop implied */
    38989, /*U+984d frequency 525 stroke 18 GAKU forehead tablet plaque framed picture sum amount volume */
    37772, /*U+938c frequency 1986 stroke 18 REN KEN sickle scythe trick */
    31777, /*U+7c21 frequency 846 stroke 18 KAN simplicity brevity */
    35251, /*U+89b3 frequency 450 stroke 18 KAN outlook look appearance condition view */
    38867, /*U+97d3 frequency 596 stroke 18 KAN Korea */
    38996, /*U+9854 frequency 517 stroke 18 GAN face expression */
    37857, /*U+93e1 frequency 1484 stroke 19 KYOU KEI mirror speculum barrel-head round rice-cake offering */
    32368, /*U+7e70 frequency 1167 stroke 19 SOU winding reel spin turn (pages) look up refer to */
    35686, /*U+8b66 frequency 335 stroke 19 KEI admonish commandment */
    35672  /*U+8b58 frequency 591 stroke 19 SHIKI discriminating know write */
};

uint32_t latincharset[] = {
    48, /* 0 */
    49, /* 1 */
    50, /* 2 */
    51, /* 3 */
    52, /* 4 */
    53, /* 5 */
    54, /* 6 */
    55, /* 7 */
    56, /* 8 */
    57, /* 9 */
    65, /* A */
    66, /* B */
    67, /* C */
    68, /* D */
    69, /* E */
    70, /* F */
    71, /* G */
    72, /* H */
    73, /* I */
    74, /* J */
    75, /* K */
    76, /* L */
    77, /* M */
    78, /* N */
    79, /* O */
    80, /* P */
    81, /* Q */
    82, /* R */
    83, /* S */
    84, /* T */
    85, /* U */
    86, /* V */
    87, /* W */
    88, /* X */
    89, /* Y */
    90  /* Z */
};

void print_usage(const char * program_name) {
    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "\n");
}

int main( int argc, char *argv[] )
{
     int ret = EXIT_SUCCESS;
     DFBResult err;
     IDirectFB               *dfb;
     /* only used for allocation of the layer context,
      * (done beneath by directfb on GetLayer).
      */
#define GETPRIMARYLAYERBROKEN
      
#ifndef GETPRIMARYLAYERBROKEN
    IDirectFBDisplayLayer   *primary_layer;
    DFBDisplayLayerConfig   primary_layer_config;
#endif
     IDirectFBSurface * primary_surface;

     IDirectFBImageProvider *provider;
     IDirectFBSurface * png32_surface;
     IDirectFBSurface * png8_surface;
     IDirectFBSurface * jpg_surface;
     DFBSurfaceDescription other_desc;

     DFBFontDescription fontdesc;
     IDirectFBFont *font;
     int width, height;
     uint32_t glyphindex; /* Unicode index */
     uint32_t i;
     
     char font_name[80];
     int fd_input_file = -1;
     char input_file_name[80] = "";
     
     uint32_t nbcharsprinted = 5;
     uint32_t nbglyphs;
     uint32_t nbitter = 500;
     
     int basex, basey;
     int glyphadvance;
     DFBRectangle glyphrect;
     
     uint32_t codepoint;
     
     uint32_t ppem_vertical = 24;
     float pointsize;
     
     char c;
     
     bvideo_format video_format = bvideo_format_ntsc;

     uint32_t total_duration;
     uint32_t first_itteration_duration = 0;
     uint32_t second_itteration_duration = 0;
     uint32_t prelast_itteration_duration = 0;
     uint32_t last_itteration_duration = 0;
     uint32_t step1a_duration;
     uint32_t step1b_duration;
     uint32_t step2a_duration;
     uint32_t step2b_duration;
     uint32_t step3a_duration;
     uint32_t step3b_duration;
     
     bool clear_glyph = false;
     
     bdvd_display_h bdisplay = NULL;

     if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
         D_ERROR("Could not init bdvd\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
         D_ERROR("Could not open bdvd_display B_ID(0)\n");
         ret = EXIT_FAILURE;
         goto out;
     }

     if (argc < 1) {
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
        goto out;
     }

    snprintf(font_name, sizeof(font_name)/sizeof(font_name[0]), DATADIR"/fonts/msgothic.ttc");

    fontdesc.flags = DFDESC_HEIGHT | DFDESC_ATTRIBUTES;
    
    fontdesc.attributes = DFFA_NOKERNING;

    fontdesc.height = 24;

    while ((c = getopt (argc, argv, "c:ef:h:i:p:n:rz:")) != -1) {
        switch (c) {
        case 'c':
            nbcharsprinted = strtoul(optarg, NULL, 10);
        break;
        case 'e':
            clear_glyph = true;
        break;
        case 'f':
            snprintf(font_name, sizeof(font_name)/sizeof(font_name[0]), DATADIR"/fonts/%s", optarg);
        break;
        case 'h':
            fontdesc.height = strtoul(optarg, NULL, 10);
        break;
        case 'i':
            fontdesc.flags |= DFDESC_INDEX;
            fontdesc.index = strtoul(optarg, NULL, 10);
        break;
        case 'p':
            ppem_vertical = strtoul(optarg, NULL, 10);
        break;
        case 'n':
            nbitter = strtoul(optarg, NULL, 10);
        break;
        case 'r':
            video_format = bvideo_format_1080i;
        break;
        case 'z':
            snprintf(input_file_name, sizeof(input_file_name)/sizeof(input_file_name[0]), "%s", optarg);
            if ((fd_input_file = open(input_file_name, O_RDONLY)) == -1) {
                fprintf(stderr, "Error opening %s: %s\n", input_file_name, strerror(errno));
                ret = EXIT_FAILURE;
                goto out;
            }
        break;
        case '?':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        break;
        }
    }

    fprintf(stderr,
            "Font height in pixels is: %d\n"
            "(this is the vertical distance from one baseline to the next when writing several lines of text)\n"
            "Vertical ppem used in calculation is: %u\n"
            "(face->size->metrics.y_ppem obtained from freetype)\n",
            fontdesc.height, ppem_vertical);

    DFBCHECK(DirectFBInit(&argc, &argv));

    DFBCHECK(DirectFBCreate(&dfb));

#ifdef GETPRIMARYLAYERBROKEN
    DFBCHECK(dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ));
    /* Get the primary surface, i.e. the surface of the primary layer. */
    other_desc.flags = DSDESC_CAPS;
    other_desc.caps = DSCAPS_PRIMARY;

    DFBCHECK(dfb->CreateSurface( dfb, &other_desc, &primary_surface ));
    
    memset(&other_desc, 0, sizeof(other_desc));
#else
    DFBCHECK( dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &primary_layer) );
    DFBCHECK( primary_layer->SetCooperativeLevel(primary_layer, DLSCL_ADMINISTRATIVE) );
    DFBCHECK( primary_layer->SetBackgroundColor(primary_layer, 0, 0, 0, 0));
    DFBCHECK( primary_layer->SetBackgroundMode(primary_layer, DLBM_COLOR));

    primary_layer_config.flags = DLCONF_BUFFERMODE;
    primary_layer_config.buffermode = DLBM_FRONTONLY;

    DFBCHECK( primary_layer->SetConfiguration(primary_layer, &primary_layer_config) );

    DFBCHECK( display_set_video_format(bdisplay, video_format) );

    switch (video_format) {
        case bvideo_format_ntsc:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 720, 480, 32 ) );
        break;
        case bvideo_format_1080i:
            /* Setting output format */
           DFBCHECK( dfb->SetVideoMode( dfb, 1920, 1080, 32 ) );
        break;
        default:;
    }

    DFBCHECK( primary_layer->GetSurface(primary_layer, &primary_surface) );
#endif

    DFBCHECK(primary_surface->GetSize(primary_surface, &width, &height));
    fprintf(stderr, "Surface width %d height %d\n", width, height);

    playbackscagat_start(bdisplay, NULL, NULL, NULL, argc, argv);

    DFBCHECK(dfb->CreateFont (dfb, font_name, &fontdesc, &font));

    DFBCHECK(primary_surface->SetFont( primary_surface, font ));

    basex = width/2;
    basey = height/2;

    fprintf(stderr, "press any key to start\n");
    getchar();

    fprintf(stderr, "Testing Glyph rendering\n");
    fprintf(stderr, "Taken from Microsoft Typography TrueType fundamentals\n"
                    "ppem = (pixels per inch) * (inches per pica point) * (pica points per em)\n"
                    "     = dpi * 1 / 72 * pointSize\n"
                    "pointSize = ppem * 1 / dpi * 72\n"
                    "if dpi = 96 (standard for pc monitor)\n"
                    "thus pointSize = ppem * 3 / 4\n");

    if (fd_input_file == -1) {

    fprintf(stderr, "Testing Glyph rendering with Latin character set\n");

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
    DFBCHECK(primary_surface->DrawString (primary_surface,
                         "Testing Glyph rendering with Latin character set", -1, width/4, height/4, DSTF_TOPCENTER));

    nbglyphs = sizeof(latincharset)/sizeof(latincharset[0]);

    for (glyphindex = 0; glyphindex < nbcharsprinted && glyphindex < nbglyphs; glyphindex++ ) {
        total_duration = myclock();

        DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
        DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                            latincharset[glyphindex],
                            basex,
                            basey,
                            DSTF_LEFT));

        DFBCHECK(dfb->WaitIdle( dfb ));
        
        total_duration = myclock() - total_duration;
    
        fprintf(stderr,
                "Displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                latincharset[glyphindex],
                total_duration,
                (float)total_duration/1000.0f);

        fprintf(stderr, "press any key\n");
        getchar();

        DFBCHECK(font->GetGlyphExtents (font,
                               latincharset[glyphindex],
                               &glyphrect,
                               &glyphadvance));

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
        DFBCHECK(primary_surface->FillRectangle (primary_surface,
                                basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
    }

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));

    total_duration = myclock();

    for (i = 0; i < nbitter; i++ ) {
        for (glyphindex = 0; glyphindex < nbglyphs; glyphindex++) {
            if ( clear_glyph ) {
                DFBCHECK(font->GetGlyphExtents (font,
                    latincharset[glyphindex],
                    &glyphrect,
                    &glyphadvance));
                DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
                DFBCHECK(primary_surface->FillRectangle (primary_surface,
                    basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
                DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
            }
            DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                latincharset[glyphindex],
                basex,
                basey,
                DSTF_LEFT));
        }
        if (i == 0) {
            first_itteration_duration = myclock();
        }
        else if (i == 1) {
            second_itteration_duration = myclock();
        }
        else if (i == nbitter-2) {
            prelast_itteration_duration = myclock();
        }
        else if (i == nbitter-1) {
            last_itteration_duration = myclock();
        }
    }

    DFBCHECK(dfb->WaitIdle( dfb ));

    last_itteration_duration = last_itteration_duration - prelast_itteration_duration;
    second_itteration_duration = second_itteration_duration - first_itteration_duration;
    first_itteration_duration = first_itteration_duration - total_duration;

    total_duration = myclock() - total_duration;

    fprintf(stderr, "Displayed characters [A-Z,0-9] (%u fonts):\n"
                    "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                    nbglyphs,
                    first_itteration_duration,
                    (float)first_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)first_itteration_duration / 1000.0f),
                    second_itteration_duration,
                    (float)second_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)second_itteration_duration / 1000.0f),
                    last_itteration_duration,
                    (float)last_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)last_itteration_duration / 1000.0f),
                    i,
                    i*(nbglyphs),
                    total_duration,
                    (float)total_duration/1000.0f,
                    (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f));

    pointsize = (float)ppem_vertical*3.0f/4.0f;
    fprintf(stderr, "if ppem is %u, pointSize = %f\n"
                    "-------------------------------------------------------------------------\n"
                    "y > 2000 / pointSize - 800 / (pointSize - 5.5)\n"
                    "y > %f\n"
                    "y = ( %u / %f ) / 30 = %f\n",
                    ppem_vertical,
                    pointsize,
                    2000.0f / pointsize - 800.0f / (pointsize - 5.5f),
                    i*(nbglyphs), ((float)total_duration / 1000.0f),
                    ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);

    fprintf(stderr, "press any key\n");
    getchar();

    DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
    DFBCHECK(primary_surface->FillRectangle (primary_surface,
                              0, 0, width, height));

    fprintf(stderr, "Testing Glyph rendering with simple Kanji character set (xxx average stroke count)\n");

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
    DFBCHECK(primary_surface->DrawString (primary_surface,
                         "Testing Glyph rendering with simple Kanji character set", -1, width/4, height/4, DSTF_TOPCENTER));

    nbglyphs = sizeof(kanjisimplecharset)/sizeof(kanjisimplecharset[0]);

    for (glyphindex = 0; glyphindex < nbcharsprinted && glyphindex < nbglyphs; glyphindex++ ) {
        total_duration = myclock();

        DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
        DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                            kanjisimplecharset[glyphindex],
                            basex,
                            basey,
                            DSTF_LEFT));

        DFBCHECK(dfb->WaitIdle( dfb ));
        
        total_duration = myclock() - total_duration;
    
        fprintf(stderr,
                "Displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                kanjisimplecharset[glyphindex],
                total_duration,
                (float)total_duration/1000.0f);

        fprintf(stderr, "press any key\n");
        getchar();

        DFBCHECK(font->GetGlyphExtents (font,
                               kanjisimplecharset[glyphindex],
                               &glyphrect,
                               &glyphadvance));

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
        DFBCHECK(primary_surface->FillRectangle (primary_surface,
                                basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
    }

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));

    total_duration = myclock();

    for (i = 0; i < nbitter; i++ ) {
        for (glyphindex = 0; glyphindex < nbglyphs; glyphindex++) {
            if ( clear_glyph ) {
                DFBCHECK(font->GetGlyphExtents (font,
                    kanjisimplecharset[glyphindex],
                    &glyphrect,
                    &glyphadvance));
                DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
                DFBCHECK(primary_surface->FillRectangle (primary_surface,
                    basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
                DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
            }
            DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                kanjisimplecharset[glyphindex],
                basex,
                basey,
                DSTF_LEFT));
        }
        if (i == 0) {
            first_itteration_duration = myclock();
        }
        else if (i == 1) {
            second_itteration_duration = myclock();
        }
        else if (i == nbitter-2) {
            prelast_itteration_duration = myclock();
        }
        else if (i == nbitter-1) {
            last_itteration_duration = myclock();
        }
    }

    DFBCHECK(dfb->WaitIdle( dfb ));

    last_itteration_duration = last_itteration_duration - prelast_itteration_duration;
    second_itteration_duration = second_itteration_duration - first_itteration_duration;
    first_itteration_duration = first_itteration_duration - total_duration;

    total_duration = myclock() - total_duration;

    fprintf(stderr, "Displayed characters [ICHI, NI, SANA, etc] (%u fonts):\n"
                    "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                    nbglyphs,
                    first_itteration_duration,
                    (float)first_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)first_itteration_duration / 1000.0f),
                    second_itteration_duration,
                    (float)second_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)second_itteration_duration / 1000.0f),
                    last_itteration_duration,
                    (float)last_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)last_itteration_duration / 1000.0f),
                    i,
                    i*(nbglyphs),
                    total_duration,
                    (float)total_duration/1000.0f,
                    (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f));

    pointsize = (float)ppem_vertical*3.0f/4.0f;
    fprintf(stderr, "if ppem is %u, pointSize = %f\n"
                    "-------------------------------------------------------------------------\n"
                    "y > 2000 / pointSize - 800 / (pointSize - 5.5)\n"
                    "y > %f\n"
                    "y = ( %u / %f ) / 30 = %f\n",
                    ppem_vertical,
                    pointsize,
                    2000.0f / pointsize - 800.0f / (pointsize - 5.5f),
                    i*(nbglyphs), ((float)total_duration / 1000.0f),
                    ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);

    fprintf(stderr, "press any key\n");
    getchar();

    DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
    DFBCHECK(primary_surface->FillRectangle (primary_surface,
                              0, 0, width, height));

    fprintf(stderr, "Testing Glyph rendering with complex Kanji character set (19.66 average stroke count)\n");

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
    DFBCHECK(primary_surface->DrawString (primary_surface,
                         "Testing Glyph rendering with complex Kanji character set", -1, width/4, height/4, DSTF_TOPCENTER));

    nbglyphs = sizeof(kanjicomplexcharset)/sizeof(kanjicomplexcharset[0]);

    for (glyphindex = 0; glyphindex < nbcharsprinted && glyphindex < nbglyphs; glyphindex++ ) {
        total_duration = myclock();

        DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
        DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                            kanjicomplexcharset[glyphindex],
                            basex,
                            basey,
                            DSTF_LEFT));

        DFBCHECK(dfb->WaitIdle( dfb ));
        
        total_duration = myclock() - total_duration;
    
        fprintf(stderr,
                "Displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                kanjicomplexcharset[glyphindex],
                total_duration,
                (float)total_duration/1000.0f);

        fprintf(stderr, "press any key\n");
        getchar();

        DFBCHECK(font->GetGlyphExtents (font,
                               kanjicomplexcharset[glyphindex],
                               &glyphrect,
                               &glyphadvance));

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
        DFBCHECK(primary_surface->FillRectangle (primary_surface,
                                basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
    }

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));

    total_duration = myclock();

    for (i = 0; i < nbitter; i++ ) {
        for (glyphindex = 0; glyphindex < nbglyphs; glyphindex++) {
            if ( clear_glyph ) {
                DFBCHECK(font->GetGlyphExtents (font,
                    kanjicomplexcharset[glyphindex],
                    &glyphrect,
                    &glyphadvance));
                DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
                DFBCHECK(primary_surface->FillRectangle (primary_surface,
                    basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
                DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
            }
            DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                kanjicomplexcharset[glyphindex],
                basex,
                basey,
                DSTF_LEFT));
        }
        if (i == 0) {
            first_itteration_duration = myclock();
        }
        else if (i == 1) {
            second_itteration_duration = myclock();
        }
        else if (i == nbitter-2) {
            prelast_itteration_duration = myclock();
        }
        else if (i == nbitter-1) {
            last_itteration_duration = myclock();
        }
    }

    DFBCHECK(dfb->WaitIdle( dfb ));

    last_itteration_duration = last_itteration_duration - prelast_itteration_duration;
    second_itteration_duration = second_itteration_duration - first_itteration_duration;
    first_itteration_duration = first_itteration_duration - total_duration;

    total_duration = myclock() - total_duration;

    fprintf(stderr, "Displayed characters [KYOU, KEN, KO, GO, etc] (%u fonts):\n"
                    "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                    nbglyphs,
                    first_itteration_duration,
                    (float)first_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)first_itteration_duration / 1000.0f),
                    second_itteration_duration,
                    (float)second_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)second_itteration_duration / 1000.0f),
                    last_itteration_duration,
                    (float)last_itteration_duration/1000.0f,
                    (float)(nbglyphs) / ((float)last_itteration_duration / 1000.0f),
                    i,
                    i*(nbglyphs),
                    total_duration,
                    (float)total_duration/1000.0f,
                    (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f));

    pointsize = (float)ppem_vertical*3.0f/4.0f;
    fprintf(stderr, "if ppem is %u, pointSize = %f\n"
                    "-------------------------------------------------------------------------\n"
                    "y > 2000 / pointSize - 800 / (pointSize - 5.5)\n"
                    "y > %f\n"
                    "y = ( %u / %f ) / 30 = %f\n",
                    ppem_vertical,
                    pointsize,
                    2000.0f / pointsize - 800.0f / (pointsize - 5.5f),
                    i*(nbglyphs), ((float)total_duration / 1000.0f),
                    ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);

    }
    else {

    for (glyphindex = 0; glyphindex < nbcharsprinted; glyphindex++ ) {
        total_duration = myclock();

        if ((ret = read(fd_input_file, &codepoint, sizeof(codepoint))) == -1) {
            fprintf(stderr, "Error reading %s: %s\n", input_file_name, strerror(errno));
            ret = EXIT_FAILURE;
            goto out;
        }
        else if (ret != sizeof(codepoint)) {
            fprintf(stderr, "Read only %d from %s\n", ret, input_file_name);
            ret = EXIT_FAILURE;
            goto out;
        }

        DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
        DFBCHECK(primary_surface->DrawGlyph (primary_surface,
                            codepoint,
                            basex,
                            basey,
                            DSTF_LEFT));

        DFBCHECK(dfb->WaitIdle( dfb ));
        
        total_duration = myclock() - total_duration;
    
        fprintf(stderr,
                "Displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                codepoint,
                total_duration,
                (float)total_duration/1000.0f);

        fprintf(stderr, "press any key\n");
        getchar();

        DFBCHECK(font->GetGlyphExtents (font,
                               codepoint,
                               &glyphrect,
                               &glyphadvance));

        DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
        DFBCHECK(primary_surface->FillRectangle (primary_surface,
                                basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
    }

    DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));

    nbglyphs = 0;

    if (lseek(fd_input_file, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Error seeking %s: %s\n", input_file_name, strerror(errno));
        ret = EXIT_FAILURE;
        goto out;
    }

    total_duration = myclock();

    while ((ret = read(fd_input_file, &codepoint, sizeof(codepoint))) == sizeof(codepoint)) {
        if ( clear_glyph ) {
            DFBCHECK(font->GetGlyphExtents (font,
                codepoint,
                &glyphrect,
                &glyphadvance));
            DFBCHECK(primary_surface->SetColor (primary_surface, 0x00, 0x00, 0x00, 0x00));
            DFBCHECK(primary_surface->FillRectangle (primary_surface,
                basex-1, basey-glyphrect.h-1, 2*glyphrect.w, 2*glyphrect.h));
            DFBCHECK(primary_surface->SetColor (primary_surface, 0xa0, 0xa0, 0xa0, 0xff));
        }
        DFBCHECK(primary_surface->DrawGlyph (primary_surface,
            codepoint,
            basex,
            basey,
            DSTF_LEFT));
            
        nbglyphs++;
    }
    
    DFBCHECK(dfb->WaitIdle( dfb ));

    total_duration = myclock() - total_duration;
    
    if (ret == -1) {
        fprintf(stderr, "Error reading %s: %s\n", input_file_name, strerror(errno));
        ret = EXIT_FAILURE;
        goto out;
    }
    else if (ret && ret != sizeof(codepoint)) {
        fprintf(stderr, "Read only %d from %s\n", ret, input_file_name);
        ret = EXIT_FAILURE;
        goto out;
    }

    fprintf(stderr, "Displayed %u characters from file %s:\n"
                    "A total of %u mseconds (%f seconds), rate is %f glyphs/sec\n",
                    nbglyphs,
                    input_file_name,
                    total_duration,
                    (float)total_duration/1000.0f,
                    (float)(nbglyphs) / ((float)total_duration / 1000.0f));

    pointsize = (float)ppem_vertical*3.0f/4.0f;
    fprintf(stderr, "if ppem is %u, pointSize = %f\n"
                    "-------------------------------------------------------------------------\n"
                    "y > 2000 / pointSize - 800 / (pointSize - 5.5)\n"
                    "y > %f\n"
                    "y = ( %u / %f ) / 30 = %f\n",
                    ppem_vertical,
                    pointsize,
                    2000.0f / pointsize - 800.0f / (pointsize - 5.5f),
                    nbglyphs, ((float)total_duration / 1000.0f),
                    ( (float)(nbglyphs) / ((float)total_duration / 1000.0f) ) / 30.0f);

    if (close(fd_input_file) == -1) {
        fprintf(stderr, "Error closing %s: %s\n", input_file_name, strerror(errno));
        ret = EXIT_FAILURE;
        goto out;
    }
        
    }

    fprintf(stderr, "press any key\n");

    DFBCHECK(font->Release( font ));

    getchar();

    /* Preloading the png and jpg interfaces */
    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/1080i_fluorescence.png",
                                             &provider ));
    DFBCHECK(provider->Release( provider ));

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/1080i_bluexmas.jpg",
                                             &provider ));
    DFBCHECK(provider->Release( provider ));

    fprintf(stderr, "Testing Image decoding\n");

    total_duration = myclock();

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/1080i_fluorescence.png",
                                             &provider ));

    DFBCHECK(provider->GetSurfaceDescription( provider, &other_desc ));
    other_desc.flags = DSDESC_CAPS;
    other_desc.caps &= ~DSCAPS_SYSTEMONLY;
    other_desc.caps |= DSCAPS_VIDEOONLY;
    DFBCHECK(dfb->CreateSurface( dfb, &other_desc, &png8_surface ));

    step1a_duration = myclock() - total_duration;
    
    DFBCHECK(provider->RenderTo( provider, png8_surface, NULL ));

    step1b_duration = myclock() - total_duration;

    DFBCHECK(provider->Release( provider ));

    total_duration = myclock();
    
    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/1080i_idyll.png",
                                             &provider ));

    DFBCHECK(provider->GetSurfaceDescription( provider, &other_desc ));
    other_desc.flags = DSDESC_CAPS;
    other_desc.caps &= ~DSCAPS_SYSTEMONLY;
    other_desc.caps |= DSCAPS_VIDEOONLY;
    DFBCHECK(dfb->CreateSurface( dfb, &other_desc, &png32_surface ));

    step2a_duration = myclock() - total_duration;

    DFBCHECK(provider->RenderTo( provider, png32_surface, NULL ));

    step2b_duration = myclock() - total_duration;

    DFBCHECK(provider->Release( provider ));

    total_duration = myclock();

    DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/1080i_bluexmas.jpg",
                                             &provider ));

    DFBCHECK(provider->GetSurfaceDescription( provider, &other_desc ));
    other_desc.flags = DSDESC_CAPS;
    other_desc.caps &= ~DSCAPS_SYSTEMONLY;
    other_desc.caps |= DSCAPS_VIDEOONLY;
    DFBCHECK(dfb->CreateSurface( dfb, &other_desc, &jpg_surface ));

    step3a_duration = myclock() - total_duration;
    
    DFBCHECK(provider->RenderTo( provider, jpg_surface, NULL ));

    step3b_duration = myclock() - total_duration;

    DFBCHECK(provider->Release( provider ));

    fprintf(stderr, "1080i_fluorescence.png (1080i 8-bit png)\n"
                    "CreateImageProvider and CreateSurface in %u mseconds (%f seconds)\n"
                    "RenderTo in %u mseconds (%f seconds)\n"
                    "Total time: %u mseconds (%f seconds)\n",
                    step1a_duration,
                    (float)step1a_duration/1000.0f,
                    (step1b_duration-step1a_duration),
                    (float)(step1b_duration-step1a_duration)/1000.0f,
                    step1b_duration,
                    (float)step1b_duration/1000.0f);

    DFBCHECK(primary_surface->StretchBlit( primary_surface, png8_surface, NULL, NULL ));

    DFBCHECK(png8_surface->Release( png8_surface ));

    fprintf(stderr, "press any key\n");
    getchar();

    fprintf(stderr, "Decoded and loaded 1080i_idyll.png (1080i 32-bit png)\n"
                    "CreateImageProvider and CreateSurface in %u mseconds (%f seconds)\n"
                    "RenderTo in %u mseconds (%f seconds)\n"
                    "Total time: %u mseconds (%f seconds)\n",
                    step2a_duration,
                    (float)step2a_duration/1000.0f,
                    (step2b_duration-step2a_duration),
                    (float)(step2b_duration-step2a_duration)/1000.0f,
                    step2b_duration,
                    (float)step2b_duration/1000.0f);

    DFBCHECK(primary_surface->StretchBlit( primary_surface, png32_surface, NULL, NULL ));

    DFBCHECK(png32_surface->Release( png32_surface ));

    fprintf(stderr, "press any key\n");
    getchar();

    fprintf(stderr, "Decoded and loaded 1080i_bluexmas.jpg (1080i jpg)\n"
                    "CreateImageProvider and CreateSurface in %u mseconds (%f seconds)\n"
                    "RenderTo in %u mseconds (%f seconds)\n"
                    "Total time: %u mseconds (%f seconds)\n",
                    step3a_duration,
                    (float)step3a_duration/1000.0f,
                    (step3b_duration-step3a_duration),
                    (float)(step3b_duration-step3a_duration)/1000.0f,
                    step3b_duration,
                    (float)step3b_duration/1000.0f);

    DFBCHECK(primary_surface->StretchBlit( primary_surface, jpg_surface, NULL, NULL ));

    DFBCHECK(jpg_surface->Release( jpg_surface ));

    fprintf(stderr, "press any key\n");
    getchar();

    playbackscagat_stop();

    DFBCHECK(primary_surface->Release( primary_surface ));
#ifndef GETPRIMARYLAYERBROKEN
    DFBCHECK(primary_layer->Release( primary_layer ));
#endif
    DFBCHECK(dfb->Release( dfb ));

    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();

out:

    return ret;
}

