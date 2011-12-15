
#include <directfb.h>
#include <direct/debug.h>

#define TEST_FREETYPE2
/*#define TEST_FONTFUSION*/
#define AUTO_OPROFILE	0
#define AUTO_OPROFILE_LOGNAME_FORMATSTR  "/mnt/nfs/fontbench.%d.log"

#ifdef TEST_FREETYPE2
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#endif

#ifdef TEST_FONTFUSION

#define ENABLE_T1
#define ENABLE_SPD
#define ENABLE_CFF
#define ENABLE_PFR
#define ENABLE_T2KS
#define ENABLE_OTF
#define ENABLE_BDF
#define COMPRESSED_INPUT_STREAM
#define ENABLE_NATIVE_T1_HINTS
#define ENABLE_AUTO_GRIDDING
#define ENABLE_SBIT
#define ENABLE_SBITS_TRANSFORM
#define ENABLE_STRKCONV 32

#include <t2k.h>
#endif

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <bdvd.h>

#include "dfbbcm_utils.h"
#include "dfbbcm_playbackscagatthread.h"

static DFBResult err;

#if AUTO_OPROFILE
static int apro_lognum = 1;
static int apro_logging = 0;
static char apro_logname[256] = "fontbench.log";
static char apro_cmd[512];
static char apro_dump[256] = "/usr/bin/opcontrol --dump";
static char apro_start[256] = "/usr/bin/opcontrol --start";
static char apro_stop[256] = "/usr/bin/opcontrol --stop";
static char apro_reset[256] = "/usr/bin/opcontrol --reset";
static int apro_rv = 0;
#define AUTO_PROFILE_LOG  { apro_logging=1; \
    apro_rv = system(apro_dump); \
    sprintf(apro_logname,AUTO_OPROFILE_LOGNAME_FORMATSTR,apro_lognum++); \
    sprintf(apro_cmd,"/usr/bin/opreport -l > %s", apro_logname); \
    apro_rv = system(apro_cmd); \
    D_INFO("system(\"%s\") returns %d\n", apro_cmd, apro_rv); }

#define AUTO_PROFILE_START  { if (apro_logging=1) \
       { \
       apro_rv = system(apro_stop); \
       apro_rv = system(apro_reset); \
       } \
    apro_rv = system(apro_start); \
    D_INFO("system(\"%s\") returns %d\n", apro_start, apro_rv); }

#define AUTO_PROFILE_STOP  { if (apro_logging=1) \
       { \
       apro_rv = system(apro_stop); \
       D_INFO("system(\"%s\") returns %d\n", apro_stop, apro_rv); \
       } \
     }
#endif

static char manual_dummystr[256] = "";

#define MANUAL_CHECKPOINT  manual_dummystr[0] = (char) NULL; while (manual_dummystr[0] != 'y') {  D_INFO("\nEnter 'y' to continue: "); fgets(manual_dummystr,sizeof(manual_dummystr),stdin); }

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                    \
     {                                                                    \
          err = x;                                                        \
          if (err != DFB_OK) {                                            \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );     \
               DirectFBErrorFatal( #x, err );                             \
          }                                                               \
     }

/* In DPI */
#define OUTPUT_RESOLUTION               72

#define FONTBENCH_TRACE

/* Something is broken into main line for GetDisplayLayer of DLID_PRIMARY.
 * If used and soft access, then segfault will occur, something to do with
 * the buffer.
 * CreateSurface DSCAPS_PRIMARY works fine, and this is no longer an issue
 * with the 7438 branch. If not gone with 7438 branch, will investigate.
 */
#define GETPRIMARYLAYERBROKEN

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

uint32_t kanjisimplecharsetindex[] = {
/*U+4E00*/ 2555,
/*U+4E8C*/ 2641,
/*U+4E09*/ 2563,
/*U+56DB*/ 4030,
/*U+4E94*/ 2646,
/*U+516D*/ 3152,
/*U+4E03*/ 2558,
/*U+516B*/ 3150,
/*U+4E5D*/ 2619,
/*U+5341*/ 3488,
/*U+65E5*/ 6371,
/*U+6708*/ 6572,
/*U+706B*/ 7979,
/*U+6C34*/ 7379,
/*U+6728*/ 6593,
/*U+91D1*/ 12932,
/*U+571F*/ 4079,
/*U+5DE6*/ 5056,
/*U+53F3*/ 3613,
/*U+4E0A*/ 2564,
/*U+4E0B*/ 2565,
/*U+5927*/ 4386,
/*U+4E2D*/ 2587,
/*U+5C0F*/ 4810,
/*U+76EE*/ 8929,
/*U+8033*/ 10314,
/*U+53E3*/ 3597,
/*U+624B*/ 5819,
/*U+8DB3*/ 12279,
/*U+4EBA*/ 2673,
/*U+5B50*/ 4662,
/*U+5973*/ 4442,
/*U+5148*/ 3125,
/*U+751F*/ 8610,
/*U+8D64*/ 12242,
/*U+9752*/ 13645,
/*U+767D*/ 8853,
/*U+5C71*/ 4874,
/*U+5DDD*/ 5049,
/*U+7530*/ 8624,
/*U+68EE*/ 6890,
/*U+96E8*/ 13582,
/*U+82B1*/ 10702,
/*U+77F3*/ 9081,
/*U+672C*/ 6596,
/*U+6B63*/ 7260,
/*U+3005*/ 1944
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

uint32_t kanjicomplexcharsetindex[] = {
/*U+97FF*/ 13760,
/*U+61F8*/ 5760,
/*U+9867*/ 13832,
/*U+8B77*/ 12052,
/*U+8972*/ 11717,
/*U+9418*/ 13305,
/*U+8B72*/ 12049,
/*U+7C4D*/ 9739,
/*U+9A30*/ 14043,
/*U+9B54*/ 14199,
/*U+8E8D*/ 12409,
/*U+6B04*/ 7203,
/*U+9732*/ 13626,
/*U+7C60*/ 9756,
/*U+984D*/ 13813,
/*U+938C*/ 13224,
/*U+7C21*/ 9714,
/*U+89B3*/ 11760,
/*U+97D3*/ 13735,
/*U+9854*/ 13819,
/*U+93E1*/ 13276,
/*U+7E70*/ 10117,
/*U+8B66*/ 12042,
/*U+8B58*/ 12034
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

uint32_t latincharsetindex[] = {
/*U+0030*/ 18,
/*U+0031*/ 19,
/*U+0032*/ 20,
/*U+0033*/ 21,
/*U+0034*/ 22,
/*U+0035*/ 23,
/*U+0036*/ 24,
/*U+0037*/ 25,
/*U+0038*/ 26,
/*U+0039*/ 27,
/*U+0041*/ 35,
/*U+0042*/ 36,
/*U+0043*/ 37,
/*U+0044*/ 38,
/*U+0045*/ 39,
/*U+0046*/ 40,
/*U+0047*/ 41,
/*U+0048*/ 42,
/*U+0049*/ 43,
/*U+004A*/ 44,
/*U+004B*/ 45,
/*U+004C*/ 46,
/*U+004D*/ 47,
/*U+004E*/ 48,
/*U+004F*/ 49,
/*U+0050*/ 50,
/*U+0051*/ 51,
/*U+0052*/ 52,
/*U+0053*/ 53,
/*U+0054*/ 54,
/*U+0055*/ 55,
/*U+0056*/ 56,
/*U+0057*/ 57,
/*U+0058*/ 58,
/*U+0059*/ 59,
/*U+005A*/ 60
};

/**************************************************/

DFBResult FontBench_read_file(const char * font_name, uint8_t ** fontfile_data, uint32_t * fontfile_length)
{
    DFBResult ret = DFB_OK;
    int errCode;

    uint32_t count;
    uint8_t * temp_data = NULL;
    uint32_t temp_length = 0;
    
    FILE *fpID = NULL;

    /* Open the font. */
    fpID = fopen(font_name, "rb");
    if (fpID == NULL)
    {
        D_ERROR("Failed fopen with file %s: %s\n", font_name, strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

    errCode = fseek( fpID, 0L, SEEK_END );
    if (errCode != 0)
    {
        D_ERROR("Failed fseek(): %s\n", strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

    temp_length = (uint32_t)ftell( fpID );
    errCode = ferror(fpID);
    if (errCode != 0)
    {
        D_ERROR("Failed ftell(): %s\n", strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

    errCode = fseek( fpID, 0L, SEEK_SET );
    if (errCode != 0)
    {
        D_ERROR("Failed fseek(): %s\n", strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

    /* Read the font into memory. */    
    temp_data = (uint8_t*)malloc( sizeof( uint8_t ) * temp_length );
    if (temp_data == NULL)
    {
        D_ERROR("Failed malloc(): %s\n", strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

    count = fread( temp_data, sizeof( uint8_t ), temp_length, fpID );
    errCode = ferror(fpID);
    if (errCode != 0 || count != temp_length)
    {
        D_ERROR("Failed fread(): %s\n", strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

    errCode = fclose( fpID );
    if (errCode != 0)
    {
        D_ERROR("Failed fclose(): %s\n", strerror(errno));
        ret = DFB_FAILURE;
        goto out;
    }

out:

    *fontfile_data = temp_data;
    *fontfile_length = temp_length;

    return ret;    
    
}

#ifdef TEST_FONTFUSION

typedef struct {
    uint8_t * fontfile_data;
    uint32_t fontfile_length;
	tsiMemObject * mem;
	InputStream * in;
	sfntClass * font;
	T2K * scaler;
} FontFusion_lib_context;

DFBResult FontFusion_init( void * context )
{
	DFBResult ret = DFB_OK;
	int errCode;

	FontFusion_lib_context * ff_context = (FontFusion_lib_context *)context;

	memset(ff_context, 0, sizeof(FontFusion_lib_context));

	/* Create the Memhandler object. */
	ff_context->mem = tsi_NewMemhandler( &errCode );
	if (errCode != 0)
	{
		D_ERROR("Failed tsi_NewMemhandler()!\n");
		ret = DFB_FAILURE;
		goto out;
	}

out:

	return ret;
}

DFBResult FontFusion_load_face(void * context, int font_index, int point_size, bool enable_embedded)
{
	DFBResult ret = DFB_OK;
	int errCode;

	FontFusion_lib_context * ff_context = (FontFusion_lib_context *)context;

	int16 fontType = FONT_TYPE_TT_OR_T2K;

	T2K_TRANS_MATRIX trans;
    
    int enableSbits = 0;
	
	/* Create the InputStream object, with data already in memory */
	ff_context->in = New_InputStream3( ff_context->mem, ff_context->fontfile_data, ff_context->fontfile_length, &errCode ); /* */
	if (errCode != 0)
	{
		D_ERROR("Failed New_InputStream3(), errCode = %d\n", errCode);
		ret = DFB_FAILURE;
		goto out;
	}

	fontType = FF_FontTypeFromStream(ff_context->in, &errCode);
	if (fontType == -1)
	{
		D_ERROR("Failed FF_FontTypeFromStream(), errCode = %d\n", errCode);
		D_ERROR("Failed to recognise Font type\n");
		ret = DFB_FAILURE;
		goto out;
	}

	/* Create an sfntClass object*/
	ff_context->font = FF_New_sfntClass( ff_context->mem, fontType, font_index, ff_context->in, NULL, NULL, &errCode );
	if (errCode != 0)
	{
		D_ERROR("Failed New_sfntClass(), errCode = %d\n", errCode);
		ret = DFB_FAILURE;
		goto out;
	}

	/* Create a T2K font scaler object.  */
	ff_context->scaler = NewT2K( ff_context->font->mem, ff_context->font, &errCode );
    if (errCode != 0)
	{
		D_ERROR("Failed NewT2K(), errCode = %d\n", errCode);
		ret = DFB_FAILURE;
		goto out;
	}
    Set_PlatformID( ff_context->scaler, 3 );
    Set_PlatformSpecificID( ff_context->scaler, 1);
    T2K_SetNameString( ff_context->scaler , 0x409 , 4 , &errCode );
    if (errCode != 0)
    {
        D_ERROR("Failed T2K_SetNameString(), errCode = %d\n", errCode);
        ret = DFB_FAILURE;
        goto out;
    }

    if (ff_context->scaler->nameString8) {
        FONTBENCH_TRACE( "FontFusion_load_face: ff_context->scaler->nameString8 is %s\n", ff_context->scaler->nameString8 );
    }

	trans.t00 = ONE16Dot16 * point_size;
	trans.t01 = 0;
	trans.t10 = 0;
	trans.t11 = ONE16Dot16 * point_size;

    if (enable_embedded) {
        enableSbits = 1;
    }

	/* Set the transformation */
	T2K_NewTransformation( ff_context->scaler, true, OUTPUT_RESOLUTION, OUTPUT_RESOLUTION, &trans, enableSbits, &errCode );
	if (errCode != 0)
	{
		D_ERROR("Failed T2K_NewTransformation(), errCode = %d\n", errCode);
		ret = DFB_FAILURE;
		goto out;
	}

    FONTBENCH_TRACE( "FontFusion_load_face: scaler->font->preferedPlatformID is %d\n", ff_context->scaler->font->preferedPlatformID );
    FONTBENCH_TRACE( "FontFusion_load_face: scaler->font->preferedPlatformSpecificID is %d\n", ff_context->scaler->font->preferedPlatformSpecificID );
    FONTBENCH_TRACE( "FontFusion_load_face: scaler->font->cmap->selectedPlatformID is %d\n", ff_context->scaler->font->cmap->selectedPlatformID );
    FONTBENCH_TRACE( "FontFusion_load_face: scaler->font->cmap->selectedPlatformSpecificID is %d\n", ff_context->scaler->font->cmap->selectedPlatformSpecificID );

out:

	return ret;

}

DFBResult FontFusion_load_bitmap(void * context, IDirectFB * dfb, IDirectFBSurface ** surface, uint32_t char_code, DFBFontAttributes attributes)
{
	DFBResult ret = DFB_OK;
	int errCode;

    uint8_t greyScaleLevel;
    uint16_t cmd;

    DFBSurfaceDescription dsc;

	FontFusion_lib_context * ff_context = (FontFusion_lib_context *)context;

    if (attributes & DFFA_MONOCHROME) {
        FONTBENCH_TRACE("FontFusion_load_bitmap: Monochrome glyph\n");
        greyScaleLevel = BLACK_AND_WHITE_BITMAP;
    }
    else {
        FONTBENCH_TRACE("FontFusion_load_bitmap: Anti-aliased glyph\n");
        greyScaleLevel = GREY_SCALE_BITMAP_HIGH_QUALITY;
        FF_SetBitRange255(ff_context->scaler, true);
    }
    
    if (attributes & DFFA_NOHINTING) {
        FONTBENCH_TRACE("FontFusion_load_bitmap: Hinting disabled\n");
        cmd = T2K_TV_MODE | T2K_SCAN_CONVERT;
    }
    else {
        FONTBENCH_TRACE("FontFusion_load_bitmap: Hinting enabled\n");
        cmd = T2K_NAT_GRID_FIT | T2K_GRID_FIT | T2K_SCAN_CONVERT;
    }
    
    if (attributes & DFFA_NOCHARMAP) {
        cmd |= T2K_CODE_IS_GINDEX;
    }

	FONTBENCH_TRACE("Loading bitmap for Unicode 0x%08X (index is %d)\n", char_code, T2K_GetGlyphIndex(ff_context->scaler, char_code, &errCode));

    /* Create a character */
	T2K_RenderGlyph(ff_context->scaler,
					char_code,
					0, 0,
					greyScaleLevel,
					cmd,
					&errCode );
	if (errCode != 0) {
		D_ERROR("Failed T2K_RenderGlyph(), errCode = %d!\n", errCode);
		ret = DFB_FAILURE;
		goto out;
	}

    if (ff_context->scaler->embeddedBitmapWasUsed) {
        FONTBENCH_TRACE("FontFusion_load_bitmap: using embedded bitmap\n");
    }
    else {
        FONTBENCH_TRACE("FontFusion_load_bitmap: bitmap generated from outline\n");
    }

    if (surface) {
        dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_PREALLOCATED;
        dsc.width = ff_context->scaler->width;
        dsc.height = ff_context->scaler->height;
        if (attributes & DFFA_MONOCHROME) {
            if (ff_context->scaler->bitDepth != 1) {
                D_ERROR( "FontFusion_load_bitmap: Pixel mode is not A8\n" );
                ret = DFB_FAILURE;
                goto out;
            }
            dsc.pixelformat = DSPF_A1;
        }
        else {
            if (ff_context->scaler->bitDepth != 8) {
                D_ERROR( "FontFusion_load_bitmap: Pixel mode is not A8\n" );
                ret = DFB_FAILURE;
                goto out;
            }
            dsc.pixelformat = DSPF_A8;
        }
        dsc.preallocated[0].data = ff_context->scaler->baseAddr;
        dsc.preallocated[0].pitch = ff_context->scaler->rowBytes;
    
        FONTBENCH_TRACE( "FontFusion_load_bitmap:\n"
                "dsc.flags 0x%08X\n"
                "dsc.width %d\n"
                "dsc.height %d\n"
                "dsc.pixelformat %d\n"
                "dsc.preallocated[0].data %p\n"
                "dsc.preallocated[0].pitch %d\n",
                dsc.flags,
                dsc.width,
                dsc.height,
                dsc.pixelformat,
                dsc.preallocated[0].data,
                dsc.preallocated[0].pitch );
    
        DFBCHECK(dfb->CreateSurface( dfb, &dsc, surface ));
    }

	/* Free up memory */
	T2K_PurgeMemory( ff_context->scaler, 1, &errCode );
	if (errCode != 0 ) {
		D_ERROR("Failed T2K_PurgeMemory(), errCode = %d!\n", errCode);
		ret = DFB_FAILURE;
		goto out;
	}

out:

	return ret;
	
}

DFBResult FontFusion_uninit( void * context )
{
	DFBResult ret = DFB_OK;
	int errCode;

	FontFusion_lib_context * ff_context = (FontFusion_lib_context *)context;

	if (ff_context->scaler) {
		/* Destroy the T2K font scaler object. */
		DeleteT2K( ff_context->scaler, &errCode );
		if (errCode != 0) {
			D_ERROR("Failed DeleteT2K(), errCode = %d\n", errCode);
			ret = DFB_FAILURE;
		}
	}

	if (ff_context->font) {
		/* Destroy the sfntClass object. */
		Delete_sfntClass( ff_context->font, &errCode );
		if (errCode != 0) {
			D_ERROR("Failed Delete_sfntClass(), errCode = %d\n", errCode);
			ret = DFB_FAILURE;
		}
	}

	if (ff_context->in) {
		/* Destroy the InputStream object. */
		Delete_InputStream( ff_context->in, &errCode );
		if (errCode != 0) {
			D_ERROR("Failed Delete_InputStream(), errCode = %d\n", errCode);
			ret = DFB_FAILURE;
		}
	}
		
	/* Destroy the Memhandler object. */
	tsi_DeleteMemhandler( ff_context->mem );
	
	return ret;
}

#endif

#ifdef TEST_FREETYPE2

typedef struct {
    uint8_t * fontfile_data;
    uint32_t fontfile_length;
	FT_Library library;
	FT_Face face;
    bool enable_embedded;
} FreeType2_lib_context;

DFBResult FreeType2_init( void * context )
{
	DFBResult ret = DFB_OK;
	FT_Error err;
	FreeType2_lib_context * ft2_context = (FreeType2_lib_context *)context;
		
	err = FT_Init_FreeType(&ft2_context->library);
	if(err) {
//	 	D_ERROR( "FT_Init_FreeType: error %d...\n", err);
		ret = DFB_FAILURE;
		goto out;
	}
	
out:
	return ret;
}

DFBResult FreeType2_load_face(void * context, int font_index, int point_size, bool enable_embedded)
{
	DFBResult ret = DFB_OK;
	FT_Error err;
	FreeType2_lib_context * ft2_context = (FreeType2_lib_context *)context;

	FONTBENCH_TRACE("font_load_face: loading index %d\n", font_index);
	
	err = FT_New_Memory_Face( ft2_context->library,
                              ft2_context->fontfile_data, ft2_context->fontfile_length,
                              font_index, &ft2_context->face );
	if(err || ft2_context->face==NULL) {
	 	D_ERROR( "FT_New_Face: error %d...\n", err);
		ret = DFB_FAILURE;
		goto out;
	}
	
	err = FT_Set_Char_Size( ft2_context->face, point_size*64, point_size*64, OUTPUT_RESOLUTION, OUTPUT_RESOLUTION);
	if(err) {
	 	D_ERROR( "FT_Set_Char_Size: error %d...\n", err);
		ret = DFB_FAILURE;
		goto out;
	}
	
	if(ft2_context->face->face_flags & FT_FACE_FLAG_KERNING)
		FONTBENCH_TRACE("Face contains kernings\n");
	else
		FONTBENCH_TRACE( "Face does not contain\n" );
	
	FONTBENCH_TRACE( "has %d fixed sizes\n", ft2_context->face->num_fixed_sizes );
	int i;
	for(i=0;i<ft2_context->face->num_fixed_sizes;i++)
	{
		FONTBENCH_TRACE( "size %d: %dx%d %d points\n",
                         i,
						 ft2_context->face->available_sizes[i].width,
						 ft2_context->face->available_sizes[i].height,
						 (int)ft2_context->face->available_sizes[i].size>>6);
	}

	FONTBENCH_TRACE( "FreeType2_load_face metrics:\n"
			"Ascender: %d\n"
			"Descender: %d\n"
			"Max advance: %d\n"
			"y ppem: %d\n",
			ft2_context->face->size->metrics.ascender >> 6,
			ft2_context->face->size->metrics.descender >> 6,
			ft2_context->face->size->metrics.max_advance >> 6,
			ft2_context->face->size->metrics.y_ppem);

    FONTBENCH_TRACE( "FreeType2_load_face: face->num_faces is %d\n", ft2_context->face->num_faces );
    FONTBENCH_TRACE( "FreeType2_load_face: face->num_glyphs is %d\n", ft2_context->face->num_glyphs );
    FONTBENCH_TRACE( "FreeType2_load_face: face->family_name is %s\n", ft2_context->face->family_name );
    FONTBENCH_TRACE( "FreeType2_load_face: face->style_name is %s\n", ft2_context->face->style_name );

    FONTBENCH_TRACE( "FreeType2_load_face: face->num_charmaps is %d\n", ft2_context->face->num_charmaps );
    FONTBENCH_TRACE( "FreeType2_load_face: face->charmap->platform_id is %d\n", ft2_context->face->charmap->platform_id );
    FONTBENCH_TRACE( "FreeType2_load_face: face->charmap->encoding_id is %d\n", ft2_context->face->charmap->encoding_id );

    ft2_context->enable_embedded = enable_embedded;

out:

	return ret;
}

/* UGLY HACK BECAUSE FONT FUSION CANNOT FIND THE CHAR CODE INDEX */
#define FontFusion_find_index FreeType2_find_index

static inline DFBResult FreeType2_find_index(void * context, uint32_t char_code, uint32_t * index) {
    FreeType2_lib_context * ft2_context = (FreeType2_lib_context *)context;
    /* 0 means "undefined character code" */
    if ((*index = FT_Get_Char_Index( ft2_context->face, char_code )) == 0) {
      //  D_ERROR( "FreeType2_find_index: Could not find character %d\n", char_code );
        return DFB_FAILURE;
    }
    return DFB_OK;
}

DFBResult FreeType2_load_bitmap(void * context, IDirectFB * dfb, IDirectFBSurface ** surface, uint32_t char_code, DFBFontAttributes attributes, bool autogridfitting)
{
	DFBResult ret = DFB_OK;
	FT_Error     err;
	FreeType2_lib_context * ft2_context = (FreeType2_lib_context *)context;
	FT_Int       load_flags;

    DFBSurfaceDescription dsc;
    
    FONTBENCH_TRACE( "FreeType2_load_bitmap:\n"
            "char_code %u\n"
            "attributes %d\n",
            char_code,
            attributes );

    FONTBENCH_TRACE("Loading bitmap for Unicode 0x%08X (index is %d)\n", char_code, FT_Get_Char_Index( ft2_context->face, char_code ));

	load_flags = (FT_Int) ft2_context->face->generic.data;

    FONTBENCH_TRACE("FT_LOAD_XXX flags are currently set to 0x%08X\n", load_flags);

	load_flags |= FT_LOAD_RENDER;

    if (ft2_context->enable_embedded) {
        load_flags &= ~FT_LOAD_NO_BITMAP;
    }
    else {
        load_flags |= FT_LOAD_NO_BITMAP;
    }

    if (attributes & DFFA_MONOCHROME) {
        FONTBENCH_TRACE("FreeType2_load_bitmap: Monochrome glyph\n");
#ifdef FT_LOAD_TARGET_MONO
          load_flags |= FT_LOAD_TARGET_MONO;
#else
          load_flags |= FT_LOAD_MONOCHROME;
#endif
    }
    else {
        FONTBENCH_TRACE("FreeType2_load_bitmap: Anti-aliased glyph\n");
#ifdef FT_LOAD_TARGET_MONO
        load_flags &= ~FT_LOAD_TARGET_MONO;
#else
        load_flags &= ~FT_LOAD_MONOCHROME;
#endif
    }

    if (attributes & DFFA_NOHINTING) {
        FONTBENCH_TRACE("FreeType2_load_bitmap: Hinting disabled\n");
        load_flags |= FT_LOAD_NO_HINTING;
    }
    else {
        FONTBENCH_TRACE("FreeType2_load_bitmap: Hinting enabled\n");
        load_flags &= ~FT_LOAD_NO_HINTING;
        if (autogridfitting) {
            FONTBENCH_TRACE("FreeType2_load_bitmap: using auto hinter\n");
            load_flags |= FT_LOAD_FORCE_AUTOHINT;
            load_flags &= ~FT_LOAD_NO_AUTOHINT;
        }
        else {
            load_flags &= ~FT_LOAD_FORCE_AUTOHINT;
#ifdef FT_FACE_FLAG_HINTER
/* Not supported right now in 2.1.10, only 2.2.1 */
            if(ft2_context->face->face_flags & FT_FACE_FLAG_HINTER) {
                FONTBENCH_TRACE("FreeType2_load_bitmap: using native hinter\n");
                load_flags |= FT_LOAD_NO_AUTOHINT;
                load_flags &= ~FT_LOAD_FORCE_AUTOHINT;
            }
            else {
                D_ERROR( "FreeType2_load_bitmap: Face does support native hinting, must enable autofitter at command line\n" );
                ret = DFB_FAILURE;
                goto out;
            }
#endif
        }
    }

    FONTBENCH_TRACE("FT_LOAD_XXX flags are now set to 0x%08X\n", load_flags);

    if (attributes & DFFA_NOCHARMAP) {
    	if ((err = FT_Load_Glyph( ft2_context->face, char_code, load_flags ))) {
//    		D_ERROR( "FreeType2_load_bitmap: Could not render glyph for character %d\n", char_code );
    		ret = DFB_FAILURE;
            goto out;
    	}
    }
    else {
        if ((err = FT_Load_Char( ft2_context->face, char_code, load_flags ))) {
            D_ERROR( "FreeType2_load_bitmap: Could not render glyph for character %d\n", char_code );
            ret = DFB_FAILURE;
            goto out;
        }
    }

    if (surface) {
        dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_PREALLOCATED;
        dsc.width = ft2_context->face->glyph->bitmap.width;
        dsc.height = ft2_context->face->glyph->bitmap.rows; /* height */
        if (attributes & DFFA_MONOCHROME) {
            if (ft2_context->face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO) {
                D_ERROR( "FreeType2_load_bitmap: Pixel mode is not A1\n" );
                ret = DFB_FAILURE;
                goto out;
            }
        	dsc.pixelformat = DSPF_A1;
        }
        else {
            if (ft2_context->face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
                D_ERROR( "FreeType2_load_bitmap: Pixel mode is not A8\n" );
                ret = DFB_FAILURE;
                goto out;
            }
            dsc.pixelformat = DSPF_A8;
        }
    	dsc.preallocated[0].data = ft2_context->face->glyph->bitmap.buffer;
    	dsc.preallocated[0].pitch = ft2_context->face->glyph->bitmap.pitch;
    
        FONTBENCH_TRACE( "FreeType2_load_bitmap:\n"
                "dsc.flags 0x%08X\n"
                "dsc.width %d\n"
                "dsc.height %d\n"
                "dsc.pixelformat %d\n"
                "dsc.preallocated[0].data %p\n"
                "dsc.preallocated[0].pitch %d\n",
                dsc.flags,
                dsc.width,
                dsc.height,
                dsc.pixelformat,
                dsc.preallocated[0].data,
                dsc.preallocated[0].pitch );
    
    	DFBCHECK(dfb->CreateSurface( dfb, &dsc, surface ));
    }
 
out:

    return ret;

}

#endif // TEST_FREETYPE2

void print_usage(const char * program_name) {
    D_INFO("usage: %s\n", program_name);
    D_INFO("\n");
    D_INFO("Permitted options:\n"); 
    D_INFO(" -a: force auto-fitter on instead of native grid fitting\n");
    D_INFO(" -b: blend_glyph = true;\n");
    D_INFO(" -c nbcharsprinted: number of glyphs to render to screen\n");
    D_INFO(" -f font_name\n");
    D_INFO(" -h: enable DFFA_NOHINTING;\n");
    D_INFO(" -i: enable DFFA_NOCHARMAP;\n");
    D_INFO(" -m: enable DFFA_MONOCHROME;\n");
    D_INFO(" -n nbitter: number of iterations of each charset\n");
    D_INFO(" -p point_size\n");
    D_INFO(" -r: video_format = bvideo_format_1080i;\n");
    D_INFO(" -z input_file_name: use text file instead of charset\n");
}

int main(int argc, char * argv[]) {
#ifdef TEST_FREETYPE2
    FreeType2_lib_context ft2_context;
#endif

#ifdef TEST_FONTFUSION
	FontFusion_lib_context ff_context;	
#endif

	int ret = EXIT_SUCCESS;
    IDirectFB               *dfb;
#ifndef GETPRIMARYLAYERBROKEN
    IDirectFBDisplayLayer   *primary_layer;
    DFBDisplayLayerConfig   primary_layer_config;
#endif
    IDirectFBSurface        *primary_surface;
    DFBSurfaceDescription dsc;
    IDirectFBSurface        *glyph_surface;
    char font_name[256] = "";

    int font_index = 0;
    uint32_t char_code;
    
    char c;
    
    int fd_input_file = -1;
    
    int point_size = 22;
    int width, height;
    
    uint8_t * fontfile_data;
    uint32_t fontfile_length;

    uint32_t nbglyphs = 0;
    uint32_t nbcharsprinted = 5;
    uint32_t nbitter = 500;
    uint32_t glyphindex = 0; /* Charset index */
    uint32_t * charset = NULL;
    uint8_t * title_string = NULL;
    
    char input_file_name[80] = "";
    
    int i = 0;
    int j;
    uint32_t total_duration = 0;
    uint32_t first_itteration_duration = 0;
    uint32_t second_itteration_duration = 0;
    uint32_t prelast_itteration_duration = 0;
    uint32_t last_itteration_duration = 0;

    DFBFontAttributes attributes = 0;

    /* Default is native, or explicit grid fitting */
    bool autogridfitting = false;

    bdvd_display_h bdisplay = NULL;

    bvideo_format video_format = bvideo_format_ntsc;

    bool video_running = true;
    bool blend_glyph = false;

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

    while ((c = getopt (argc, argv, "abc:f:himn:p:rz:")) != -1) {
        switch (c) {
        case 'a':
            autogridfitting = true;
        break;
        case 'b':
            blend_glyph = true;
        break;
        case 'c':
            nbcharsprinted = strtoul(optarg, NULL, 10);
        break;
        case 'f':
            snprintf(font_name, sizeof(font_name)/sizeof(font_name[0]), "%s", optarg);
        break;
        case 'h':
            attributes |= DFFA_NOHINTING;
        break;
        case 'i':
            attributes |= DFFA_NOCHARMAP;
        break;
        case 'm':
            attributes |= DFFA_MONOCHROME;
        break;
        case 'n':
            nbitter = strtoul(optarg, NULL, 10);
        break;
        case 'p':
            point_size = strtoul(optarg, NULL, 10);
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
			ret = EXIT_FAILURE;
			goto out;
        break;
        }
    }

    if (!font_name[0]) {
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
        goto out;
    }

    DFBCHECK(DirectFBInit( &argc, &argv ));

    /* create the super interface */
    DFBCHECK(DirectFBCreate( &dfb ));

#ifdef GETPRIMARYLAYERBROKEN
    DFBCHECK(dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ));
    /* Get the primary surface, i.e. the surface of the primary layer. */
    dsc.flags = DSDESC_CAPS;
    dsc.caps = DSCAPS_PRIMARY;

    DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary_surface ));
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
    DFBCHECK( primary_surface->GetSize( primary_surface, &width, &height ));
    D_INFO( "Surface width %d height %d\n", width, height );

    if (video_running) {
        D_INFO("Creating video stress thread\n");
        playbackscagat_start(bdisplay, NULL, NULL, NULL, argc, argv);
    }

#ifdef TEST_FREETYPE2
	FreeType2_init(&ft2_context);
#endif

#ifdef TEST_FONTFUSION
    FontFusion_init(&ff_context);
#endif

    FontBench_read_file(font_name, &fontfile_data, &fontfile_length);

#ifdef TEST_FREETYPE2
    ft2_context.fontfile_data = fontfile_data;
    ft2_context.fontfile_length = fontfile_length;
#endif

#ifdef TEST_FONTFUSION
    ff_context.fontfile_data = fontfile_data;
    ff_context.fontfile_length = fontfile_length;
#endif

#ifdef TEST_FREETYPE2
	FreeType2_load_face(&ft2_context, font_index, point_size, false);
#endif

#ifdef TEST_FONTFUSION
    FontFusion_load_face(&ff_context, font_index, point_size, false);
#endif

    DFBCHECK( primary_surface->Clear(primary_surface, 0, 0, 0, 0) );

    D_INFO("ready to start test\n");
    MANUAL_CHECKPOINT;

    DFBCHECK(primary_surface->SetBlittingFlags( primary_surface, DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE ));

    DFBCHECK(primary_surface->SetColor( primary_surface, 255, 255, 255, 0xFF ));

    if (fd_input_file == -1) {
        for (j = 0; j < 3; j++) {
            switch (j) {
                case 0:
                    if (attributes & DFFA_NOCHARMAP) {
                        charset = latincharsetindex;
                    }
                    else {
                        charset = latincharset;
                    }
                    nbglyphs = sizeof(latincharset)/sizeof(latincharset[0]);
                    title_string = "Latin character set [A-Z,0-9]";
                break;
                case 1:
                    if (attributes & DFFA_NOCHARMAP) {
                        charset = kanjisimplecharsetindex;
                    }
                    else {
                        charset = kanjisimplecharset;
                    }
                    nbglyphs = sizeof(kanjisimplecharset)/sizeof(kanjisimplecharset[0]);
                    title_string = "Simple Kanji character set (xxx average stroke count) [ICHI, NI, SANA, etc]";
                break;
                case 2:
                    if (attributes & DFFA_NOCHARMAP) {
                        charset = kanjicomplexcharsetindex;
                    }
                    else {
                        charset = kanjicomplexcharset;
                    }
                    nbglyphs = sizeof(kanjicomplexcharset)/sizeof(kanjicomplexcharset[0]);
                    title_string = "Complex Kanji character set (19.66 average stroke count) [KYOU, KEN, KO, GO, etc]";
                break;
            }

            D_INFO( "Testing Glyph rendering with %s\n", title_string );
        
            for (glyphindex = 0; glyphindex < nbcharsprinted && glyphindex < nbglyphs; glyphindex++ ) {
                total_duration = myclock();

                FreeType2_load_bitmap(&ft2_context, dfb, &glyph_surface, charset[glyphindex], attributes, autogridfitting);

                DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width*10/100, height*10/100));
        
                total_duration = myclock() - total_duration;
            
                D_INFO( "FreeType2 displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                        charset[glyphindex],
                        total_duration,
                        (float)total_duration/1000.0f);
        
                DFBCHECK(glyph_surface->Release(glyph_surface));
        
                DFBCHECK(dfb->WaitIdle( dfb ));

#ifdef TEST_FONTFUSION
                total_duration = myclock();

                FontFusion_load_bitmap(&ff_context, dfb, &glyph_surface, charset[glyphindex], attributes);
        
                DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width/2, height*10/100));
        
                total_duration = myclock() - total_duration;
            
                D_INFO( "FontFusion displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                        charset[glyphindex],
                        total_duration,
                        (float)total_duration/1000.0f);
        
                DFBCHECK(glyph_surface->Release(glyph_surface));
#endif
                D_INFO("ready %s\n", (glyphindex+1 < nbcharsprinted && glyphindex+1 < nbglyphs) ? "to continue displaying glyphs" : "to start glyph rasterizing stress test" );
                MANUAL_CHECKPOINT;
        
#if AUTO_OPROFILE
                if (!(glyphindex+1 < nbcharsprinted && glyphindex+1 < nbglyphs))
                {
                   AUTO_PROFILE_START;
                }
#endif
                /* Get size of glyph??? */
                DFBCHECK(primary_surface->Clear( primary_surface, 0, 0, 0, 0 ));
            }

            total_duration = myclock();
        
            for (i = 0; i < nbitter; i++ ) {
                for (glyphindex = 0; glyphindex < nbglyphs; glyphindex++) {
                    FreeType2_load_bitmap(&ft2_context, dfb, blend_glyph ? &glyph_surface : NULL, charset[glyphindex], attributes, autogridfitting);
                    if (blend_glyph) {
                        DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width/2, height*10/100));
                        DFBCHECK(glyph_surface->Release(glyph_surface));
                    }
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
        
            last_itteration_duration = last_itteration_duration - prelast_itteration_duration;
            second_itteration_duration = second_itteration_duration - first_itteration_duration;
            first_itteration_duration = first_itteration_duration - total_duration;
        
            total_duration = myclock() - total_duration;
        
            D_INFO( "FreeType2 displayed %s (%u fonts):\n"
                    "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                    title_string,
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
        
            D_INFO( "point_size = %d\n"
                    "-------------------------------------------------------------------------\n"
                    "y > 2000 / point_size - 800 / (point_size - 5.5)\n"
                    "y > %f\n"
                    "y = ( %u / %f ) / 30 = %f\n",
                    point_size,
                    2000.0f / (float)point_size - 800.0f / ((float)point_size - 5.5f),
                    i*(nbglyphs), ((float)total_duration / 1000.0f),
                    ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);

#ifdef TEST_FONTFUSION
            total_duration = myclock();
        
            for (i = 0; i < nbitter; i++ ) {
                for (glyphindex = 0; glyphindex < nbglyphs; glyphindex++) {
                    FontFusion_load_bitmap(&ff_context, dfb, blend_glyph ? &glyph_surface : NULL, charset[glyphindex], attributes);
                    if (blend_glyph) {
                        DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width/2, height*10/100));
                        DFBCHECK(glyph_surface->Release(glyph_surface));
                    }
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
        
            last_itteration_duration = last_itteration_duration - prelast_itteration_duration;
            second_itteration_duration = second_itteration_duration - first_itteration_duration;
            first_itteration_duration = first_itteration_duration - total_duration;
        
            total_duration = myclock() - total_duration;
        
            D_INFO( "FontFusion displayed %s (%u fonts):\n"
                    "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                    "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                    title_string,
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
        
            D_INFO( "point_size = %d\n"
                    "-------------------------------------------------------------------------\n"
                    "y > 2000 / point_size - 800 / (point_size - 5.5)\n"
                    "y > %f\n"
                    "y = ( %u / %f ) / 30 = %f\n",
                    point_size,
                    2000.0f / (float)point_size - 800.0f / ((float)point_size - 5.5f),
                    i*(nbglyphs), ((float)total_duration / 1000.0f),
                    ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);
#endif

#if AUTO_OPROFILE
            AUTO_PROFILE_LOG;
#endif
            D_INFO( "ready\n");
            MANUAL_CHECKPOINT;
        }
    }
    else {
        title_string = input_file_name;

        for (glyphindex = 0; glyphindex < nbcharsprinted; glyphindex++ ) {
            if ((ret = read(fd_input_file, &char_code, sizeof(char_code))) == -1) {
                fprintf(stderr, "Error reading %s: %s\n", input_file_name, strerror(errno));
                ret = EXIT_FAILURE;
                goto out;
            }
            else if (ret != sizeof(char_code)) {
                fprintf(stderr, "Read only %d from %s\n", ret, input_file_name);
                ret = EXIT_FAILURE;
                goto out;
            }

            total_duration = myclock();

            /* Doing this for both FT2 and FF */
            if (attributes & DFFA_NOCHARMAP) {
                FreeType2_find_index(&ft2_context, char_code, &char_code);
            }

            FreeType2_load_bitmap(&ft2_context, dfb, &glyph_surface, char_code, attributes, autogridfitting);

            DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width*10/100, height*10/100));

            total_duration = myclock() - total_duration;

            D_INFO( "FreeType2 displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                    char_code,
                    total_duration,
                    (float)total_duration/1000.0f);

            DFBCHECK(glyph_surface->Release(glyph_surface));

            DFBCHECK(dfb->WaitIdle( dfb ));

#ifdef TEST_FONTFUSION
            total_duration = myclock();

            FontFusion_load_bitmap(&ff_context, dfb, &glyph_surface, char_code, attributes);

            DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width/2, height*10/100));

            total_duration = myclock() - total_duration;

            D_INFO( "FontFusion displayed unicode char U+%04X in %u mseconds (%f seconds).\n",
                    char_code,
                    total_duration,
                    (float)total_duration/1000.0f);

            DFBCHECK(glyph_surface->Release(glyph_surface));
#endif

            D_INFO("ready\n");
            MANUAL_CHECKPOINT;

            /* Get size of glyph??? */
            DFBCHECK(primary_surface->Clear( primary_surface, 0, 0, 0, 0 ));
        }
    
        nbglyphs = 0;
    
        if (lseek(fd_input_file, 0, SEEK_SET) == -1) {
            fprintf(stderr, "Error seeking %s: %s\n", input_file_name, strerror(errno));
            ret = EXIT_FAILURE;
            goto out;
        }
    
        total_duration = myclock();
    
        while ((ret = read(fd_input_file, &char_code, sizeof(char_code))) == sizeof(char_code)) {
            if (attributes & DFFA_NOCHARMAP) {
                /* Charmap support is still buggy with font fusion using FT2 to find index */
                FreeType2_find_index(&ft2_context, char_code, &char_code);
            }
#ifdef TEST_FONTFUSION
            FontFusion_load_bitmap(&ff_context, dfb, blend_glyph ? &glyph_surface : NULL, char_code, attributes);
            if (blend_glyph) {
                DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width/2, height*10/100));
                DFBCHECK(glyph_surface->Release(glyph_surface));
            }
#endif
            nbglyphs++;
        }
        
        DFBCHECK(dfb->WaitIdle( dfb ));
    
        total_duration = myclock() - total_duration;
        
        if (ret == -1) {
            fprintf(stderr, "Error reading %s: %s\n", input_file_name, strerror(errno));
            ret = EXIT_FAILURE;
            goto out;
        }
        else if (ret && ret != sizeof(char_code)) {
            fprintf(stderr, "Read only %d from %s\n", ret, input_file_name);
            ret = EXIT_FAILURE;
            goto out;
        }
    
        D_INFO( "Font Fusion displayed %s (%u fonts):\n"
                "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                title_string,
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
    
        D_INFO( "point_size = %d\n"
                "-------------------------------------------------------------------------\n"
                "y > 2000 / point_size - 800 / (point_size - 5.5)\n"
                "y > %f\n"
                "y = ( %u / %f ) / 30 = %f\n",
                point_size,
                2000.0f / (float)point_size - 800.0f / ((float)point_size - 5.5f),
                i*(nbglyphs), ((float)total_duration / 1000.0f),
                ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);

        nbglyphs = 0;
    
        if (lseek(fd_input_file, 0, SEEK_SET) == -1) {
            fprintf(stderr, "Error seeking %s: %s\n", input_file_name, strerror(errno));
            ret = EXIT_FAILURE;
            goto out;
        }
    
        total_duration = myclock();
    
        while ((ret = read(fd_input_file, &char_code, sizeof(char_code))) == sizeof(char_code)) {
            if (attributes & DFFA_NOCHARMAP) {
                FreeType2_find_index(&ft2_context, char_code, &char_code);
            }
            FreeType2_load_bitmap(&ft2_context, dfb, blend_glyph ? &glyph_surface : NULL, char_code, attributes, autogridfitting);
            if (blend_glyph) {
                DFBCHECK(primary_surface->Blit(primary_surface, glyph_surface, NULL, width/2, height*10/100));
                DFBCHECK(glyph_surface->Release(glyph_surface));
            }
            nbglyphs++;
        }
        
        DFBCHECK(dfb->WaitIdle( dfb ));
    
        total_duration = myclock() - total_duration;
        
        if (ret == -1) {
            fprintf(stderr, "Error reading %s: %s\n", input_file_name, strerror(errno));
            ret = EXIT_FAILURE;
            goto out;
        }
        else if (ret && ret != sizeof(char_code)) {
            fprintf(stderr, "Read only %d from %s\n", ret, input_file_name);
            ret = EXIT_FAILURE;
            goto out;
        }
    
        D_INFO( "FreeType2 displayed %s (%u fonts):\n"
                "First pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                "Second pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                "Last pass of this set rendered in %u mseconds (%f seconds), rate is %f glyphs/sec\n"
                "A total of %u times (total glyph rendered %u) in %u mseconds (%f seconds), is %f glyphs/sec\n",
                title_string,
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
    
        D_INFO( "point_size = %d\n"
                "-------------------------------------------------------------------------\n"
                "y > 2000 / point_size - 800 / (point_size - 5.5)\n"
                "y > %f\n"
                "y = ( %u / %f ) / 30 = %f\n",
                point_size,
                2000.0f / (float)point_size - 800.0f / ((float)point_size - 5.5f),
                i*(nbglyphs), ((float)total_duration / 1000.0f),
                ( (float)(i*(nbglyphs)) / ((float)total_duration / 1000.0f) ) / 30.0f);
    
        if (close(fd_input_file) == -1) {
            fprintf(stderr, "Error closing %s: %s\n", input_file_name, strerror(errno));
            ret = EXIT_FAILURE;
            goto out;
        }
    }

    D_INFO( "ready to quit\n" );
    MANUAL_CHECKPOINT;
#if AUTO_OPROFILE
    AUTO_PROFILE_STOP;
#endif

    if (video_running) {
        playbackscagat_stop();
    }

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
