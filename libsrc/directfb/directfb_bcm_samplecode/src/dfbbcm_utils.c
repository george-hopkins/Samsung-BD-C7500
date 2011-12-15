#include <directfb.h>

#include <direct/util.h>
#include <direct/debug.h>

#include <bdvd.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dfbbcm_utils.h"

DFBEnumerationResult
enum_layers_callback( unsigned int                 id,
                      DFBDisplayLayerDescription   desc,
                      void                        *data )
{

    printf( "\nLayer %u : %s\n", id, desc.name );

    printf( " Default level: %d\n", desc.level );
    printf( " Number of concurrent regions supported (-1 = unlimited, 0 = unknown/one) : %d\n", desc.regions );
    printf( " Number of selectable sources: %d\n", desc.sources );

    printf(" Listing of DFBDisplayLayerTypeFlags\n");
     
    if (desc.type & DLTF_NONE)
        printf( "  X Unclassified, no specific type.\n" );

    if (desc.type & DLTF_GRAPHICS)
        printf( "  - Can be used for graphics output.\n" );

    if (desc.type & DLTF_VIDEO)
        printf( "  - Can be used for live video output.\n" );

    if (desc.type & DLTF_STILL_PICTURE)
        printf( "  - Can be used for single frames.\n" );

    if (desc.type & DLTF_BACKGROUND)
        printf( "  - Can be used as a background layer.\n" );

    printf(" Listing of DFBDisplayLayerCapabilities\n");

    if (desc.caps & DLCAPS_NONE)
        printf( "  X Has a no capabilities listed.\n" );

    if (desc.caps & DLCAPS_SURFACE)
        printf( "  - Has a surface that can be drawn to.\n" );

    if (desc.caps & DLCAPS_OPACITY)
        printf( "  - Supports blending based on global alpha factor.\n" );

    if (desc.caps & DLCAPS_ALPHACHANNEL)
        printf( "  - Supports blending based on alpha channel.\n" );

    if (desc.caps & DLCAPS_SCREEN_LOCATION)
        printf( "  - Can be positioned on the screen, coverage area includes size (SetScreenLocation).\n" );

    if (desc.caps & DLCAPS_FLICKER_FILTERING)
        printf( "  - Supports flicker filtering.\n" );

    if (desc.caps & DLCAPS_DEINTERLACING)
        printf( "  - Can deinterlace interlaced content for progressive display.\n" );

    if (desc.caps & DLCAPS_SRC_COLORKEY)
        printf( "  - Supports source color keying.\n" );

    if (desc.caps & DLCAPS_DST_COLORKEY)
        printf( "  - Supports destination color keying.\n" );

    if (desc.caps & DLCAPS_BRIGHTNESS)
        printf( "  - Brightness can be adjusted.\n" );

    if (desc.caps & DLCAPS_CONTRAST)
        printf( "  - Contrast can be adjusted.\n" );

    if (desc.caps & DLCAPS_HUE)
        printf( "  - Hue can be adjusted.\n" );

    if (desc.caps & DLCAPS_SATURATION)
        printf( "  - Saturation can be adjusted.\n" );

    if (desc.caps & DLCAPS_LEVELS)
        printf( "  - Level (z position) can be adjusted.\n" );

    if (desc.caps & DLCAPS_FIELD_PARITY)
        printf( "  - Field parity can be selected.\n" );

    if (desc.caps & DLCAPS_WINDOWS)
        printf( "  - Has hardware window support.\n" );

    if (desc.caps & DLCAPS_SOURCES)
        printf( "  - Sources can be selected.\n" );

    if (desc.caps & DLCAPS_WINDOWS)
        printf( "  - Has hardware window support.\n" );

    if (desc.caps & DLCAPS_ALPHA_RAMP)
        printf( "  - Formats with one or two alpha bits can uses those bits as a lookup table index.\n" );

    if (desc.caps & DLCAPS_PREMULTIPLIED) {
        printf( "  - Surfaces with premultiplied alpha are supported.\n" );
        printf( "    (Notes that for current implementation, this is not an option,\n" );
        printf( "     rather it indicates that the surface is indeed premultiplied)\n" );
    }

    if (desc.caps & DLCAPS_SCREEN_POSITION)
        printf( "  - Can be positioned on the screen, without specifing size (SetScreenPosition).\n" );

    if (desc.caps & DLCAPS_SCREEN_SIZE)
        printf( "  - Dependent on DLCAPS_SCREEN_LOCATION.\n" );

    printf("\n");

    return DFENUM_OK;
}

FormatString format_strings[] = {
     { "A8",       DSPF_A8       },
     { "ALUT44",   DSPF_ALUT44   },
     { "ARGB",     DSPF_ARGB     },
     { "ARGB1555", DSPF_ARGB1555 },
     { "ARGB2554", DSPF_ARGB2554 },
     { "ARGB4444", DSPF_ARGB4444 },
     { "AiRGB",    DSPF_AiRGB    },
     { "I420",     DSPF_I420     },
     { "LUT8",     DSPF_LUT8     },
     { "NV12",     DSPF_NV12     },
     { "NV21",     DSPF_NV21     },
     { "NV16",     DSPF_NV16     },
     { "RGB16",    DSPF_RGB16    },
     { "RGB24",    DSPF_RGB24    },
     { "RGB32",    DSPF_RGB32    },
     { "RGB332",   DSPF_RGB332   },
     { "UYVY",     DSPF_UYVY     },
     { "YUY2",     DSPF_YUY2     },
     { "YV12",     DSPF_YV12     }
};

uint32_t nb_format_strings() {
    return sizeof(format_strings)/sizeof(FormatString);
}

static int
pixelformat_string_compare (const void *key,
                       const void *base)
{
     return strcmp (((const FormatString *) key)->string, ((const FormatString *) base)->string);
}

static int
pixelformat_enum_compare (const void *key,
                     const void *base)
{
     return (((const FormatString *) key)->format - ((const FormatString *) base)->format);
}

DFBSurfacePixelFormat
convert_string2dspf( const char *format )
{
    FormatString search_format;
    FormatString * format_string;

    qsort(format_strings, nb_format_strings(), sizeof(FormatString), pixelformat_string_compare);

    search_format.string = format;

    format_string = bsearch( &search_format, format_strings,
                             nb_format_strings(), sizeof(FormatString),
                             pixelformat_string_compare );
    if (!format_string)
         return DSPF_UNKNOWN;

    return format_string->format;
}

const char *
convert_dspf2string( DFBSurfacePixelFormat format )
{
    FormatString search_format;
    FormatString * found_format;

    qsort(format_strings, nb_format_strings(), sizeof(FormatString), pixelformat_enum_compare);

    search_format.format = format;

    found_format = bsearch( &search_format, format_strings,
                            nb_format_strings(), sizeof(FormatString),
                            pixelformat_enum_compare );
    if (!found_format)
         return "UNKNOWN";

    return found_format->string;
}

/**
 *  This function sets the bvideo_format for display
 *  handle. Used only with the BCM_DISPLAY_VIDEO_FORMAT env
 *  variable for debug purposes.
 *  @par display
 *       display handle of type bdvd_display_h
 *  @par video_format
 *       video format of type bvideo_format
 *  @return
 *      DFBResult DFB_OK, else error code
 */
DFBResult
display_set_video_format(bdvd_display_h display,
                         bdvd_video_format_e video_format) {
    DFBResult ret = DFB_OK;
    bdvd_display_settings_t display_settings;

    if (bdvd_display_get(display, &display_settings) != bdvd_ok) {
        D_ERROR("display_set_video_format: bdvd_display_get failed\n");
        ret = DFB_FAILURE;
        goto out;
    }

    display_settings.format = video_format;
    display_settings.rf     = NULL;
    display_settings.hdmi    = getenv("dfbbcm_usehdmi") ? bdvd_output_hdmi_open(B_ID(0)) : NULL;

    if (getenv("dfbbcm_usecvbs")) {
        if (display_settings.format != bdvd_video_format_ntsc) {
            D_ERROR("display_set_video_format: must be set to ntsc when doing cvbs/svideo\n");
            ret = DFB_FAILURE;
            goto out;
        }
        D_INFO("display_set_video_format: setting to ntsc with cvbs/svideo outputs only\n");
        display_settings.component = NULL;
        display_settings.composite = bdvd_output_composite_open(B_ID(0));
        display_settings.svideo    = bdvd_output_svideo_open(B_ID(0));
    }
    else {
        switch (display_settings.format) {
            case bdvd_video_format_ntsc:
                D_INFO("display_set_video_format: setting to ntsc\n");
                display_settings.component = bdvd_output_component_open(B_ID(0));
                display_settings.composite = NULL;
                display_settings.svideo    = NULL;
            break;
            case bdvd_video_format_480p:
                D_INFO("display_set_video_format: setting to 480p\n");
                display_settings.component = bdvd_output_component_open(B_ID(0));
                display_settings.composite = NULL;
                display_settings.svideo = NULL;
            break;
            case bdvd_video_format_720p:
                D_INFO("display_set_video_format: setting to 720p\n");
                display_settings.component = bdvd_output_component_open(B_ID(0));
                display_settings.composite = NULL;
                display_settings.svideo = NULL;
            break;
            case bdvd_video_format_1080i:
                D_INFO("display_set_video_format: setting to 1080i\n");
                display_settings.component = bdvd_output_component_open(B_ID(0));
                display_settings.composite = NULL;
                display_settings.svideo = NULL;
            break;
            default:
                D_INFO("display_set_video_format: video format does not exist\n");
                ret = DFB_INVARG;
                goto out;
            break;
        }
    }

    if (display_settings.hdmi != NULL) {
        D_INFO("display_set_video_format: opening hdmi\n");
        bdvd_output_hdmi_settings_t hdmi_settings;
        if (bdvd_output_hdmi_get(display_settings.hdmi, &hdmi_settings) != bdvd_ok) {
            D_ERROR("display_set_video_format: bdvd_output_hdmi_get failed\n");
            ret = DFB_FAILURE;
            goto out;
        }
        hdmi_settings.hdcp = false;
        if (bdvd_output_hdmi_set(display_settings.hdmi, &hdmi_settings) != bdvd_ok) {
            D_ERROR("display_set_video_format: could not set hdmi output\n");
            ret = DFB_FAILURE;
            goto out;
        }
    }
    
    if (bdvd_display_set(display, &display_settings) != bdvd_ok) {
        D_ERROR("display_set_video_format: could not set display settings\n");
        ret = DFB_FAILURE;
        goto out;
    }
    
out:
    
    return DFB_OK;
}

DFBResult 
generate_palette_pattern_entries( IDirectFBPalette *palette, ePaletteGenType gen_type )
{
    uint32_t          i;
    DFBResult         ret = DFB_OK;
    DFBColor          colors[256];
    
    /* Let's say color index 0 is always transparent black */
    colors[0].a = 0;
    colors[0].r = 0;
    colors[0].g = 0;
    colors[0].b = 0;

    switch (gen_type) {
        case ePaletteGenType_red_gradient:
            /* Calculate RGB values. */
            for (i = 1; i < 256; i++) {
                colors[i].a = 0xFF;
                colors[i].r = i;
                colors[i].g = 0;
                colors[i].b = 0;
            }
        break;
        case ePaletteGenType_green_gradient:
            /* Calculate RGB values. */
            for (i = 1; i < 256; i++) {
                colors[i].a = 0xFF;
                colors[i].r = 0;
                colors[i].g = i;
                colors[i].b = 0;
            }
        break;        
        case ePaletteGenType_blue_gradient:
            /* Calculate RGB values. */
            for (i = 1; i < 256; i++) {
                colors[i].a = 0xFF;
                colors[i].r = 0;
                colors[i].g = 0;
                colors[i].b = i;
            }
        break;
        default:
            DirectFBError( "Palette type does not exist", ret );
            goto error;
        break;
    }

    if ((ret = palette->SetEntries( palette, colors, 256, 0 )) != DFB_OK) {
        DirectFBError( "IDirectFBPalette::SetEntries() failed", ret );
        goto error;
    }
    
    return DFB_OK;

error:

    return ret;
        
}

void dump_mem(uint32_t addr, uint8_t * buffer, uint32_t word_size, int len) {
  /* miscellaneous variables, mostly just counting vars. */
  int i, j = 0, k = 0;

  for (i = 0;  i < len;) {
    /* 
     * here we are counting in blocks of 16, so that each row in the 
     * table will be labeled, and will have exactly 16 entries, one for 
     * each byte in the address range. 
     */
    /* The Row label */
    printf("0x%08x:   ", ((uint32_t)addr)+((uint32_t)i));

    /* The row contents */
    for (j = 0; j != 16; i+=word_size, j+=word_size) {
      if(j >= len) break; // We have print all the buffer, no need to continue...
      if (word_size == 1) {
          printf("0x%02x", *((uint8_t *)(buffer+i)));
      }
      else if (word_size == 2) {
          printf("0x%04x", *((uint16_t *)(buffer+i)));
      }
      else if (word_size == 4) {
          printf("0x%08x", *((uint32_t *)(buffer+i)));
      }
      
      printf(" ");
    }

    /* Now show what characters are in the memory locations of this row.  */
    printf("\"");
    { unsigned char * Na = buffer+k;
      for (;k < i; Na++, k++)
        /* Make sure that the character is a printable character! */
        if (*Na >= 0x20 && *Na <= 0x7e) {
          printf("%c", *Na);  /* Force convert of number to character */
        }
        else {
        /* 
         * if the character is not printable, then simply print a floating 
         */
          printf("%c", 0x2e);
        }
    }
    printf("\"\n");
  }
}

DFBResult
init_wakeup_call(wakeup_call_t * wakeup_call) {
    DFBResult ret = DFB_OK;
    int rc;

    /* Always returns 0 */
    pthread_mutexattr_init(&wakeup_call->cond_mutex_attr);

#ifdef D_DEBUG_ENABLED
    if ((rc = pthread_mutexattr_settype(&wakeup_call->cond_mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP))) {
#else
    if ((rc = pthread_mutexattr_settype(&wakeup_call->cond_mutex_attr, PTHREAD_MUTEX_FAST_NP))) {
#endif
        D_ERROR("pthread_mutexattr_settype failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

    /* Always returns 0 */        
    pthread_mutex_init(&wakeup_call->cond_mutex, &wakeup_call->cond_mutex_attr);

    /* This does really nothing in LinuxThread */
    if ((rc = pthread_condattr_init(&wakeup_call->cond_attr))) {
        D_ERROR("pthread_condattr_init failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

    if ((rc = pthread_cond_init(&wakeup_call->cond, &wakeup_call->cond_attr))) {
        D_ERROR("pthread_cond_init failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }
    
quit:

    return ret;
}

DFBResult
wait_wakeup_call(wakeup_call_t * wakeup_call) {
    DFBResult ret = DFB_OK;
    int rc;
    
    D_ASSERT( wakeup_call != NULL );
    
    /* No goto error before this */
    if ((rc = pthread_mutex_lock(&wakeup_call->cond_mutex))) {
        D_ERROR("pthread_mutex_lock failed %s\n", strerror(rc));
        return DFB_FAILURE;
    }

    if ((rc = pthread_cond_wait(&wakeup_call->cond, &wakeup_call->cond_mutex))) {
        D_ERROR("pthread_cond_wait failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

quit:

    if ((rc = pthread_mutex_unlock(&wakeup_call->cond_mutex))) {
        D_ERROR("pthread_mutex_unlock failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
    }

    return ret;
}

DFBResult
broadcast_wakeup_call(wakeup_call_t * wakeup_call) {
    DFBResult ret = DFB_OK;
    int rc;
    
    D_ASSERT( wakeup_call != NULL );
    
    /* No goto error before this */
    if ((rc = pthread_mutex_lock(&wakeup_call->cond_mutex))) {
        D_ERROR("pthread_mutex_lock failed %s\n", strerror(rc));
        return DFB_FAILURE;
    }

    if ((rc = pthread_cond_broadcast(&wakeup_call->cond))) {
        D_ERROR("pthread_cond_broadcast failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
        goto quit;
    }

quit:

    if ((rc = pthread_mutex_unlock(&wakeup_call->cond_mutex))) {
        D_ERROR("pthread_mutex_unlock failed %s\n", strerror(rc));
        ret = DFB_FAILURE;
    }

    return ret;
}

DFBResult
uninit_wakeup_call(wakeup_call_t * wakeup_call) {
    /* Do nothing for now */
    return DFB_OK;
}
