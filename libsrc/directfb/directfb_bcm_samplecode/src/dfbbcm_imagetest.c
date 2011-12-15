#include <bsettop.h>
#include <bdvd.h>
#include <ctype.h>
#include <dirent.h>
#include <directfb.h>
#include <direct/debug.h>
#include <directfb-internal/misc/util.h>
#include <fcntl.h>
#include <sched.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libmng.h>
#include "bkni.h"
#include "dfbbcm_playbackscagatthread.h"
#include "dfbbcm_utils.h"
#include "backtrace.h"

#define MAX_SMALL_STRING_LEN    (1024)
#define MAX_LARGE_STRING_LEN    (1024*10)
#define MAX_NUM_DECODERS  5
#define INDENT            "    "

#define STD_PRINT(x...) printf(x)

#define ERROR_EXIT_LOG(x...) \
{ \
    ThreadSafeLog(imageThreadParams, x); \
    imageThreadParams->error = true; \
    if (gSettings.displayBlitProtectReserved) { \
        BKNI_SetEvent(gSettings.displayBlitProtect); \
        gSettings.displayBlitProtectReserved = false; \
    } \
    goto exit; \
}

typedef struct ImageTypeInfo ImageTypeInfo;
typedef struct ImageNode ImageNode;
typedef struct ImageTypeResults ImageTypeResults;
typedef struct DecoderType DecoderType;
typedef struct ProgramArguments ProgramArguments;
typedef struct ProgramSettings ProgramSettings;
typedef struct ImageThreadParams ImageThreadParams;
typedef struct LayerThreadParams LayerThreadParams;

typedef void (*DecodeFunction)( ImageThreadParams *imageThreadParams );
typedef void (*ByteCompareFunction)( ImageThreadParams *imageThreadParams );

void DecodeStandard( ImageThreadParams *imageThreadParams );
void DecodeMNG(  ImageThreadParams *imageThreadParams );
void DecodeBDRLE(  ImageThreadParams *imageThreadParams );
void DecodeHDRLE(  ImageThreadParams *imageThreadParams );

void ByteCompareStandard( ImageThreadParams *imageThreadParams );

typedef enum {
    IMAGETYPE_PNG = 0,
    IMAGETYPE_JPEG,
    IMAGETYPE_BMP,
    IMAGETYPE_GIF,
    IMAGETYPE_MNG,
    IMAGETYPE_RLE_HDDVD,
    IMAGETYPE_RLE_BLURAY,
    IMAGETYPE_CIF,
    IMAGETYPE_NUM_TYPES,
} ImageType;

struct ImageTypeInfo {
    char*                   userFriendlyString;
    char*                   extension;
    ImageType               imageType;
    DecodeFunction          decodeFunction;
    ByteCompareFunction     byteCompareFunction;
};

static
ImageTypeInfo validFileTypes[] = {
    {"PNG",     ".png",        IMAGETYPE_PNG,          DecodeStandard, ByteCompareStandard},
    {"JPG",     ".jpg",        IMAGETYPE_JPEG,         DecodeStandard, ByteCompareStandard},
    {"JPG",     ".jpeg",       IMAGETYPE_JPEG,         DecodeStandard, ByteCompareStandard},
    {"BMP",     ".bmp",        IMAGETYPE_BMP,          DecodeStandard, ByteCompareStandard},
    {"GIF",     ".gif",        IMAGETYPE_GIF,          DecodeStandard, ByteCompareStandard},
    {"MNG",     ".mng",        IMAGETYPE_MNG,          DecodeMNG,      NULL},
    {"HD-RLE",  ".rle_hddvd",  IMAGETYPE_RLE_HDDVD,    DecodeHDRLE,    NULL},
    {"BD-RLE",  ".rle_bluray", IMAGETYPE_RLE_BLURAY,   DecodeBDRLE,    NULL},
    {"CIF",     ".cvf",        IMAGETYPE_CIF,          DecodeStandard, ByteCompareStandard},
    {"CIF",     ".cdf",        IMAGETYPE_CIF,          DecodeStandard, ByteCompareStandard},
    {"CIF",     ".cif",        IMAGETYPE_CIF,          DecodeStandard, ByteCompareStandard},
    {"",        "",            IMAGETYPE_NUM_TYPES,    NULL,           NULL}
};

static 
struct ProgramArguments {
    int32_t                 waitForInput;
    int32_t                 millisecDelay;
    int32_t                 iterations;
    int32_t                 scaleImage;
    int32_t                 ppmSurfaceDump;
    int32_t                 cifSurfaceDump;
    int32_t                 useDataBuffer;
    int32_t                 useHWAcceleratedMNG;
    uint32_t                numThreads;
    uint32_t                refreshRate;
    uint32_t                numDecodeCompares;
    uint32_t                numDecodesToAverage;
    bool                    verboseOutput;
    char                    directory[MAX_SMALL_STRING_LEN];
    char                    logFileName[MAX_SMALL_STRING_LEN];
    char                    videoFileName[MAX_SMALL_STRING_LEN];
} gArguments;

static struct ProgramSettings {
    struct ImageTypeResults {
        uint32_t tested;
        uint32_t passed;
        uint32_t decodeRatesCounted;
        double   totalDecodeRates;
        double   averageDecodeRate;
        struct DecoderType {
            char     decoderName[MAX_SMALL_STRING_LEN];
            uint32_t tested;
            uint32_t passed;
            uint32_t decodeRatesCounted;
            double   totalDecodeRates;
            double   averageDecodeRate;
        } decoder[MAX_NUM_DECODERS];
    } results[IMAGETYPE_NUM_TYPES];

    bdvd_display_h          bdisplay;
    IDirectFB*              dfb;
    IDirectFBFont*          font;
    DFBFontDescription      fontDesc;
    char                    fontName[MAX_SMALL_STRING_LEN];
    bool                    displayNameOnScreen;
    bdvd_video_format_e     videoFormat;
    uint32_t                screenHeight;
    uint32_t                screenWidth;
    uint32_t                numImages;
    int32_t                 iterationsCompleted;
    FILE *                  logfp;
    char**                  videoArgs;
    ImageNode*              firstImage;
    struct ImageNode*       currentImage;
    BKNI_EventHandle        threadProtect;
    BKNI_EventHandle        displayBlitProtect; /*Protect render and blit commands in order to ensure we 
                                                 always get accurate decode rates */
    bool                    displayBlitProtectReserved;
} gSettings;


struct ImageNode {
    int32_t                 id;
    ImageNode*              link;
    char*                   fileName;
    char*                   filePath;
    ImageTypeInfo*          imageTypeInfo;
};

struct ImageThreadParams {

    DFBSurfaceDescription   outDesc;
    DFBSurfaceDescription   imgDesc;
    IDirectFBSurface*       decodeSurface;
    ImageTypeInfo*          imageTypeInfo;
    int32_t                 startTop;
    int32_t                 startLeft;
    bool                    error;
    bool                    terminated;
    bool                    userTriggeredContinueReceived;
    bool                    outputDisplayed;
    bool                    logWritten;
    uint32_t                startTime;
    uint32_t                endTime;
    uint32_t                lastDisplayTime;
    uint32_t                id;
    uint32_t                blittedVersions;
    uint32_t                logIndent;
    double                  mpps;
    char*                   fileName;
    char*                   filePath;
    char                    decoderUsed[MAX_SMALL_STRING_LEN];
    char                    decoderReason[MAX_SMALL_STRING_LEN];
    char*                   logOutputPos;
    char*                   screenOutputPos;
    char                    screenOutput[MAX_LARGE_STRING_LEN];
    char                    logOutput[MAX_LARGE_STRING_LEN];
    char                    indentString[MAX_SMALL_STRING_LEN];
    BKNI_EventHandle        doneDisplay;
    BKNI_EventHandle        threadComplete;
};

struct LayerThreadParams {
    pthread_t               layerThread;
    pthread_t*              imageThread;
    bdvd_dfb_ext_layer_type_e layerType;
    int32_t                 layerLevel;
    uint32_t                numHorizontalCubes;
    uint32_t                numVerticalCubes;
    bool                    terminated;
    BKNI_EventHandle        threadComplete;
    IDirectFBSurface *      displaySurface;
    IDirectFBDisplayLayer*  displayLayer;
    ImageThreadParams*      imageThreadParams;
};


bool InitializeSettings(int32_t argc, char *argv[]);
void DestroySettings();
void DecodeToLayer(bdvd_dfb_ext_layer_type_e layerType, uint32_t layerLevel);
void LogImageDecode( ImageThreadParams *imageThreadParams );
void WaitForInput( ImageThreadParams *imageThreadParams );
void StartTimer(char *fileName, ImageThreadParams *imageThreadParams );
void StopTimer(char *fileName, ImageThreadParams *imageThreadParams );
void SleepForDisplay( ImageThreadParams *imageThreadParams );
void LoadFromFile(uint8_t **buf, uint32_t *bufferLength, char *file_path);
void BlitSurface(IDirectFBSurface *srcSurface, ImageThreadParams *imageThreadParams);

void ThreadSafeLog(ImageThreadParams *imageThreadParams, char * fmt,...);
void ThreadSafePrintOnly(ImageThreadParams *imageThreadParams, char * fmt,...);

void PrintThreadSafeLogToScreen(ImageThreadParams *imageThreadParams);
void WriteThreadSafeLogToFile(ImageThreadParams *imageThreadParams);

void ThreadSafeTabIn(ImageThreadParams *imageThreadParams);
void ThreadSafeTabOut(ImageThreadParams *imageThreadParams);
void PrintResults();
void Log(char * fmt,...);


/*********************************************************************
 Main
 *********************************************************************/

int32_t main( int32_t argc, char *argv[] )
{
    arm_backtrace();

    memset(&gSettings, 0, sizeof(gSettings));
    memset(&gArguments, 0, sizeof(gArguments));

    if (!InitializeSettings(argc, argv)) {
        DecodeToLayer( bdvd_dfb_ext_osd_layer, 1);
        PrintResults();
    }

    DestroySettings(&gSettings);

    return 0;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************      DECODE A STANDARD IMAGE        ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void DecodeStandard( ImageThreadParams *imageThreadParams )
{
    IDirectFBDataBuffer *buffer = NULL;
    IDirectFBSurface *imageSurface = NULL;
    IDirectFBPalette *imagePalette = NULL;
    IDirectFBImageProvider *provider = NULL;
    DFBImageDescription     imageDesc;
    DFBPaletteDescription   palDesc;
    DFBSurfaceDescription   surfDesc;
    DFBDataBufferDescription  ddsc;
    uint8_t *fileBuffer = NULL;
    uint32_t fileBufferLength = 0;
    uint32_t i;

    if (imageThreadParams == NULL)
        ERROR_EXIT_LOG("%s - imageThreadParams is NULL!\n", __FUNCTION__);

    if (gArguments.useDataBuffer)
    {
        ThreadSafeLog(imageThreadParams, "Rendering data buffer...\n");
        LoadFromFile(&fileBuffer, &fileBufferLength, imageThreadParams->filePath);
        if (fileBuffer == NULL)
            ERROR_EXIT_LOG("%s - fileBuffer is NULL!\n", __FUNCTION__);

        /* create a data buffer for memory */
        ddsc.flags         = DBDESC_MEMORY;
        ddsc.memory.data   = fileBuffer;
        ddsc.memory.length = fileBufferLength;

        if(gSettings.dfb->CreateDataBuffer( gSettings.dfb, &ddsc, &buffer ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateDataBuffer() FAILED!\n", __FUNCTION__);

        if(buffer->CreateImageProvider( buffer, &provider ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateImageProvider() FAILED!\n", __FUNCTION__);
    }
    else
    {
        if(gSettings.dfb->CreateImageProvider( gSettings.dfb, imageThreadParams->filePath, &provider ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateImageProvider() FAILED!\n", __FUNCTION__);
    }

    if (provider->GetSurfaceDescription(provider, &imageThreadParams->imgDesc) != DFB_OK)
        ERROR_EXIT_LOG("%s - GetSurfaceDescription() FAILED!\n", __FUNCTION__);

    if(gSettings.dfb->CreateSurface( gSettings.dfb, &imageThreadParams->imgDesc, &imageSurface ) != DFB_OK)
        ERROR_EXIT_LOG("%s - CreateSurface() FAILED!\n", __FUNCTION__);

    if (imageThreadParams->imgDesc.flags & DSDESC_PALETTE)
    {
        palDesc.flags      = DPDESC_SIZE;
        palDesc.caps       = DPCAPS_NONE;
        palDesc.size       = imageThreadParams->imgDesc.palette.size;
        palDesc.entries    = imageThreadParams->imgDesc.palette.entries;
        if (gSettings.dfb->CreatePalette(gSettings.dfb, &palDesc, &imagePalette) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreatePalette() FAILED!\n", __FUNCTION__);

    }

    if(imageSurface->Clear( imageSurface, 0, 0, 0, 0) != DFB_OK)
        ERROR_EXIT_LOG("%s - Clear() FAILED!\n", __FUNCTION__);

    if(provider->GetImageDescription( provider, &imageDesc) != DFB_OK)
        ERROR_EXIT_LOG("%s - GetImageDescription() FAILED!\n", __FUNCTION__);

    strcpy(imageThreadParams->decoderUsed, imageDesc.decoderName);
    strcpy(imageThreadParams->decoderReason, imageDesc.decoderReason);

    StartTimer(imageThreadParams->filePath, imageThreadParams);
    for (i=0; i < gArguments.numDecodesToAverage; i++)
    {
        if(provider->RenderTo( provider, imageSurface, NULL ) != DFB_OK)
            ERROR_EXIT_LOG("%s - RenderTo() FAILED!\n", __FUNCTION__);
    }
    StopTimer(imageThreadParams->filePath, imageThreadParams);

    SleepForDisplay(imageThreadParams);

    imageThreadParams->decodeSurface->Clear( imageThreadParams->decodeSurface, 0, 0, 0, 0);
    BlitSurface(imageSurface, imageThreadParams);

exit:

    if (buffer != NULL)
        buffer->Release( buffer );
    if (provider != NULL)
        provider->Release( provider );
    if (imageSurface != NULL)
        imageSurface->Release(imageSurface);
    if (imagePalette != NULL)
        imagePalette->Release(imagePalette);
    if (fileBuffer != NULL)
        free(fileBuffer);
}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************      BYTE COMPARE A STANDARD IMAGE        ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


void ByteCompareStandard( ImageThreadParams *imageThreadParams )
{
    uint32_t i, x, y;
    DFBSurfacePixelFormat pixelFormat = DSPF_ARGB;
    uint32_t bytesPerPixel = 0;

    IDirectFBSurface *controlSurface = NULL;
    IDirectFBImageProvider *controlProvider = NULL;
    uint8_t *controlSurfaceAddress = NULL;
    int32_t controlSurfacePitch = 0;
    uint8_t *controlAddress = NULL;

    IDirectFBSurface *testSurface = NULL;
    IDirectFBImageProvider *testProvider = NULL;
    uint8_t *testSurfaceAddress = NULL;
    int32_t testSurfacePitch = 0;
    uint8_t *testAddress = NULL;


    if (gArguments.numDecodeCompares >= 1) {
        if(gSettings.dfb->CreateImageProvider( gSettings.dfb, imageThreadParams->filePath, &controlProvider ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateImageProvider() Control FAILED!\n", __FUNCTION__);

        if (controlProvider->GetSurfaceDescription(controlProvider, &imageThreadParams->imgDesc) != DFB_OK)
            ERROR_EXIT_LOG("%s - GetSurfaceDescription() Control FAILED!\n", __FUNCTION__);

        /* force ARGB output */
        imageThreadParams->imgDesc.pixelformat=pixelFormat;

        if(gSettings.dfb->CreateSurface( gSettings.dfb, &imageThreadParams->imgDesc, &controlSurface ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateSurface() Control FAILED!\n", __FUNCTION__);

        if(controlSurface->Clear( controlSurface, 0, 0, 0, 0) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateSurface() Control FAILED!\n", __FUNCTION__);

        if(controlProvider->RenderTo( controlProvider, controlSurface, NULL ) != DFB_OK)
            ERROR_EXIT_LOG("%s - RenderTo() Control FAILED!\n", __FUNCTION__);

        controlSurface->GetPixelFormat(controlSurface, &pixelFormat);
        bytesPerPixel = DFB_BYTES_PER_PIXEL(pixelFormat);
    }

    for(i = 0; i < gArguments.numDecodeCompares; i ++) {

        testProvider = NULL;
        testSurface = NULL;

        if(gSettings.dfb->CreateImageProvider( gSettings.dfb, imageThreadParams->filePath, &testProvider ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateImageProvider() Test %d FAILED!\n",
                                    __FUNCTION__, i);

        if (testProvider->GetSurfaceDescription(testProvider, &imageThreadParams->imgDesc) != DFB_OK)
            ERROR_EXIT_LOG("%s - GetSurfaceDescription() Test %d FAILED!\n", __FUNCTION__, i);

        /* force ARGB output */
        imageThreadParams->imgDesc.pixelformat=pixelFormat;

        if(gSettings.dfb->CreateSurface( gSettings.dfb, &imageThreadParams->imgDesc, &testSurface ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateSurface() Test %d FAILED!\n", __FUNCTION__, i);

        if(testSurface->Clear( testSurface, 0, 0, 0, 0) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateSurface() Test %d FAILED!\n", __FUNCTION__, i);

        if(testProvider->RenderTo( testProvider, testSurface, NULL ) != DFB_OK)
            ERROR_EXIT_LOG("%s - RenderTo() Test %d FAILED!\n", __FUNCTION__, i);


        ThreadSafeLog(imageThreadParams, "Comparing control surface to surface %d...\n", i);
        gSettings.dfb->WaitIdle(gSettings.dfb);

        testSurface->Lock( testSurface, DSLF_READ|DSLF_WRITE, (void*)&testSurfaceAddress,  &testSurfacePitch);
        controlSurface->Lock( controlSurface, DSLF_READ|DSLF_WRITE, (void*)&controlSurfaceAddress,  &controlSurfacePitch);

        for (y =0; (y < imageThreadParams->imgDesc.height) && !imageThreadParams->error; y++) {
            testAddress = ((uint8_t*)(testSurfaceAddress) + (testSurfacePitch * y));
            controlAddress = ((uint8_t*)(controlSurfaceAddress) + (controlSurfacePitch * y));
            for (x =0; (x < imageThreadParams->imgDesc.width * bytesPerPixel) && !imageThreadParams->error; x++) {
                if ( *(((uint8_t*)controlAddress) + x) !=
                    *(((uint8_t*)testAddress) + x)) {
                    ThreadSafeLog(imageThreadParams, "COMPARE ERROR LINE %d ARGB byte %d! Surface %d is 0x%02x, should be 0x%02x!\n", y, x, i, *(((uint8_t*)testAddress) + x), *(((uint8_t*)controlAddress) + x));
                    imageThreadParams->error = true;
                }
            }
        }

        if (controlSurfaceAddress != NULL)
            controlSurface->Unlock( controlSurface );
        if (testSurfaceAddress != NULL)
            testSurface->Unlock( testSurface );

        if (testProvider != NULL)
            testProvider->Release( testProvider );
        if (testSurface != NULL)
            testSurface->Release( testSurface );

        controlSurfaceAddress = NULL;
        testSurfaceAddress = NULL;
        testProvider = NULL;
        testSurface = NULL;
    }


exit:
    if (controlSurfaceAddress != NULL)
        controlSurface->Unlock( controlSurface );
    if (controlProvider != NULL)
        controlProvider->Release( controlProvider );
    if (controlSurface != NULL)
        controlSurface->Release( controlSurface );

    if (testSurfaceAddress != NULL)
        testSurface->Unlock( testSurface );
    if (testProvider != NULL)
        testProvider->Release( testProvider );
    if (testSurface != NULL)
        testSurface->Release( testSurface );

}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   BD RLE SOFTWARE DECODE  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////



bool ParseBDRLEHeader(uint8_t* pData, uint32_t dataLength,
                        DFBColor *entries,
                        DFBSurfaceDescription *imgDescription,
                        bdvd_graphics_sid_decode_params *sidDecParams)
{
    uint8_t *pIn = pData;
    int32_t paletteSegSize = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t paletteSize = 0;

    /* first four bytes are "rled" */
    if ((pIn[0] != 'r') || (pIn[1] != 'l') || (pIn[2] != 'e') ||
        (pIn[3] != 'd') || (pIn[4] != 0x14))
        return false;

    /* Get the palette size. Skip the palette id, palette version and previous 6 bytes*/
    paletteSegSize = ((pIn[5] << 8) | pIn[6]) - 2; /* subtract two for the palette id and version */
    pIn+=9;
    while (paletteSegSize > 0)
    {
        int32_t r, g, b;

        r = pIn[1] + (int)(1.370705 * (pIn[2]-128));
        g = pIn[1] - (int)((0.698001 * (pIn[2]-128)) - (0.337633 * (pIn[3]-128)));
        b = pIn[1] + (int)(1.732446 * (pIn[3]-128));
        r = (r > 255) ? 255 : (r < 0) ? 0 : r;
        g = (g > 255) ? 255 : (g < 0) ? 0 : g;
        b = (b > 255) ? 255 : (b < 0) ? 0 : b;

        entries[paletteSize].r = r;
        entries[paletteSize].g = g;
        entries[paletteSize].b = b;
        entries[paletteSize].a = pIn[4];
        paletteSize++;
        paletteSegSize -= 5;
        pIn+=5;
    }

    // next byte should be object segment type
    if ((paletteSegSize != 0) || (pIn[0] != 0x15))
        return false;

    // Get the size of the object segment. Skip past id and version number, sequence flags
    width = (pIn[10] << 8) | pIn[11];
    height = (pIn[12] << 8) | pIn[13];
    pIn+=14;

    imgDescription->flags=DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT|DSDESC_PALETTE;
    imgDescription->pixelformat=DSPF_LUT8;
    imgDescription->palette.entries=entries;
    imgDescription->palette.size=paletteSize;
    imgDescription->width = width;
    imgDescription->height = height;

    sidDecParams->image_format = BGFX_SID_IMAGE_FORMAT_RLE_BD;
    sidDecParams->src = (uint32_t *)(pIn);
    sidDecParams->src_size = (dataLength - (pIn - pData));
    sidDecParams->image_width = width;
    sidDecParams->image_height = height;
    sidDecParams->image_pitch = ((width + 3) & ~3);
    sidDecParams->cb_func = NULL;

    return true;
}



void DecodeBDRLE( ImageThreadParams *imageThreadParams )
{
    bdvd_graphics_sid_decode_params dec_params;
    IDirectFBSurface *imageSurface = NULL;
    IDirectFBPalette *palette = NULL;
    DFBColor entries[256];
    uint8_t *fileBuffer = NULL;
    uint32_t bufferLength;
    uint32_t i;
    bool surfaceLocked = false;

    sprintf(imageThreadParams->decoderUsed, "SID BD-RLE");
    sprintf(imageThreadParams->decoderReason, "only decoder available");

    LoadFromFile(&fileBuffer, &bufferLength, imageThreadParams->filePath);
    if (fileBuffer == NULL)
        ERROR_EXIT_LOG("%s - fileBuffer is NULL!\n", __FUNCTION__);

    if (!ParseBDRLEHeader(fileBuffer, bufferLength, entries, &imageThreadParams->imgDesc, &dec_params))
        ERROR_EXIT_LOG("%s - ParseBDRLEHeader FAILED!\n", __FUNCTION__);

    if(gSettings.dfb->CreateSurface( gSettings.dfb, &imageThreadParams->imgDesc, &imageSurface ) != DFB_OK)
        ERROR_EXIT_LOG("%s - CreateSurface() FAILED!\n", __FUNCTION__);

    if(imageSurface->Clear( imageSurface, 0, 0, 0, 0) != DFB_OK)
        ERROR_EXIT_LOG("%s - CreateSurface() FAILED!\n", __FUNCTION__);

    if (imageSurface->GetBackBufferContext(imageSurface, (void**)&dec_params.dst_surface) != DFB_OK)
        ERROR_EXIT_LOG("%s - GetBackBufferContext() FAILED!\n", __FUNCTION__);

    if (imageSurface->HardwareLock( imageSurface ) != DFB_OK)
        ERROR_EXIT_LOG("%s - HardwareLock() FAILED!\n", __FUNCTION__);
    surfaceLocked = true;

    StartTimer(imageThreadParams->filePath, imageThreadParams);
    for (i=0; i < gArguments.numDecodesToAverage; i++)
    {
        if (bdvd_sid_decode(NULL, &dec_params) != b_ok)
            ERROR_EXIT_LOG("%s - bdvd_sid_decode() FAILED!\n", __FUNCTION__);
    }
    StopTimer(imageThreadParams->filePath, imageThreadParams);

    if (surfaceLocked) {
        if (imageSurface->Unlock( imageSurface ) != DFB_OK)
            ERROR_EXIT_LOG("%s - Unlock() FAILED!\n", __FUNCTION__);
        surfaceLocked = false;
    }

    SleepForDisplay(imageThreadParams);

    imageThreadParams->decodeSurface->Clear( imageThreadParams->decodeSurface, 0, 0, 0, 0);
    BlitSurface(imageSurface, imageThreadParams);

exit:

    if (surfaceLocked)
        imageSurface->Unlock( imageSurface );
    if (imageSurface != NULL)
        imageSurface->Release(imageSurface);
    if (fileBuffer != NULL)
        bdvd_sid_free_input_buffer(fileBuffer);
    if (palette)
        palette->Release(palette);
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   HD RLE SOFTWARE DECODE  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


#define NUM_FIELDS 2
#define MAX_DCC 9
#define EXTRACT16(ptr, ret) \
{ \
    ret = *ptr++ << 8; \
    ret |= *ptr++; \
}

#define EXTRACT8(ptr, ret) \
{ \
    ret = *ptr++; \
} \

#define EXTRACT32(ptr, ret) \
{ \
    ret = *ptr++ << 24; \
    ret |= *ptr++ << 16; \
    ret |= *ptr++ << 8; \
    ret |= *ptr++ << 0; \
}

typedef enum DCC_e{
    FSTA_DSP    = 0x00, 
    STA_DSP     = 0x01,
    STP_DSP     = 0x02,
    SET_DSPXA   = 0x06,
    SET_COLOR2  = 0x83,
    SET_CONTR2  = 0x84,
    SET_DAREA2  = 0x85,
    SET_DSPXA2  = 0x86,
    CHG_COLCON2 = 0x87,
    CMD_END     = 0xFF,
} DCC;

typedef struct DCCMD_pair_t
{
    DCC cmd;
    uint32_t val;
}DCCMD_pair;

typedef struct DCSQ_t
{
   uint16_t startTime;
   uint32_t offsetToNextDCSQ;
   DCCMD_pair DCCCommand[MAX_DCC];
}DCSQ;

/*HD RLE specific information*/

typedef struct HD_RLE_Info_t
{
    uint16_t id;
    uint32_t spu_size;
    uint32_t pxd_offset;
    uint32_t dcsqt_offset;

    uint16_t left;
    uint16_t right;
    uint16_t top;
    uint16_t bottom;
    uint32_t fieldOffset[NUM_FIELDS];


    uint32_t height;
    uint32_t width;
    uint32_t palette[256];
    uint32_t palette_size;

    DCSQ sequenceTable;

    uint8_t* buff;
    uint8_t bitRemaining;
    uint32_t offset;
    uint8_t workingByte;

}HD_RLE_Info;

bool extract_hd_rle_info(uint8_t* fileBuffer, HD_RLE_Info *hd_rle_info, ImageThreadParams *imageThreadParams);

void DecodeHDRLE( ImageThreadParams *imageThreadParams )
{
    uint32_t curField;
    uint32_t fieldSize[NUM_FIELDS];
    bdvd_graphics_sid_decode_params dec_params;
    bool legacyRLE = false;
    DFBColor entries[256];
    HD_RLE_Info hd_rle_info;
    uint8_t *fileBuffer = NULL;
    IDirectFBSurface *imageSurface = NULL;
    IDirectFBPalette *palette    = NULL;
    DFBPaletteDescription palletDsc;
    bool surfaceLocked = false;
    uint32_t bufferLength;
    uint32_t i;

    hd_rle_info.buff = NULL;
    hd_rle_info.bitRemaining = 8;
    hd_rle_info.offset = 0;
    hd_rle_info.workingByte = 0;

    sprintf(imageThreadParams->decoderUsed, "SID HD-RLE");
    sprintf(imageThreadParams->decoderReason, "only decoder available");

    LoadFromFile(&fileBuffer, &bufferLength, imageThreadParams->filePath);
    if (fileBuffer == NULL)
        ERROR_EXIT_LOG("%s - fileBuffer is NULL!\n", __FUNCTION__);

    if (!extract_hd_rle_info(fileBuffer, &hd_rle_info, imageThreadParams))
    {
        memcpy(entries, hd_rle_info.palette, hd_rle_info.palette_size*4);

        palletDsc.flags = (DFBPaletteDescriptionFlags) (DPDESC_SIZE | DPDESC_ENTRIES);
        palletDsc.size = 256;
        palletDsc.entries = entries;

        imageThreadParams->imgDesc.width =  hd_rle_info.width;
        imageThreadParams->imgDesc.height = (hd_rle_info.height & ~1);

        imageThreadParams->imgDesc.width =  imageThreadParams->imgDesc.width > 1 ? 
                                            imageThreadParams->imgDesc.width : 1;
        imageThreadParams->imgDesc.height = imageThreadParams->imgDesc.height > 1 ?
                                            imageThreadParams->imgDesc.height : 1;

        imageThreadParams->imgDesc.pixelformat = legacyRLE ? DSPF_LUT2 : DSPF_LUT8;

        imageThreadParams->imgDesc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_WIDTH | 
                                            DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS);
        imageThreadParams->imgDesc.caps = (DFBSurfaceCapabilities)(DSCAPS_VIDEOONLY |
                                            DSCAPS_PREMULTIPLIED | DSCAPS_INTERLACED |
                                            DSCAPS_SEPARATED | DSCAPS_OPTI_BLIT);

        dec_params.image_format = legacyRLE ?   BGFX_SID_IMAGE_FORMAT_RLE_LEGACY_DVD :
                                                BGFX_SID_IMAGE_FORMAT_RLE_HD_DVD;

        dec_params.image_width = imageThreadParams->imgDesc.width;
        dec_params.image_height = hd_rle_info.height / NUM_FIELDS;
        dec_params.image_pitch = legacyRLE ? 
                                 (((imageThreadParams->imgDesc.width / 4) + 3) & ~3) :
                                 ((imageThreadParams->imgDesc.width + 3) & ~3);

        dec_params.cb_func = NULL;

        if (hd_rle_info.fieldOffset[1] != hd_rle_info.fieldOffset[0]) {
            fieldSize[0] = hd_rle_info.fieldOffset[1] - hd_rle_info.fieldOffset[0];
            fieldSize[1] = hd_rle_info.dcsqt_offset - hd_rle_info.fieldOffset[1];
        }
        else {
            fieldSize[0] = fieldSize[1] = hd_rle_info.dcsqt_offset - hd_rle_info.pxd_offset;
        }

        if(gSettings.dfb->CreateSurface( gSettings.dfb, &imageThreadParams->imgDesc, &imageSurface ) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateSurface() FAILED!\n", __FUNCTION__);

        if (gSettings.dfb->CreatePalette(gSettings.dfb, &palletDsc, &palette) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreatePalette() FAILED!\n", __FUNCTION__);

        if (palette->SetEntries(palette, entries, 256, 0) != DFB_OK)
            ERROR_EXIT_LOG("%s - SetEntries() FAILED!\n", __FUNCTION__);

        if (imageSurface->SetPalette(imageSurface, palette) != DFB_OK)
            ERROR_EXIT_LOG("%s - SetPalette() FAILED!\n", __FUNCTION__);

        if(imageSurface->Clear( imageSurface, 0, 0, 0, 0) != DFB_OK)
            ERROR_EXIT_LOG("%s - CreateSurface() FAILED!\n", __FUNCTION__);

        /* lock the dfb surface buffer for the object and RLE decode into it */
        if (imageSurface->HardwareLock( imageSurface ) != DFB_OK)
            ERROR_EXIT_LOG("%s - HardwareLock() field %d FAILED!\n", __FUNCTION__);
        surfaceLocked = true;

        StartTimer(imageThreadParams->filePath, imageThreadParams);
        for (i=0; i < gArguments.numDecodesToAverage; i++)
        {
            for(curField = 0; curField < NUM_FIELDS; curField++)
            {
                dec_params.src = (uint32_t *)(fileBuffer + hd_rle_info.fieldOffset[curField]);
                dec_params.src_size = fieldSize[curField];

                /* Create a surface for this field */
                if (imageSurface->SetField(imageSurface, curField) != DFB_OK)
                    ERROR_EXIT_LOG("%s - SetField() field %d FAILED!\n", __FUNCTION__, curField);

                /* Get the surface address */
                if (imageSurface->GetBackBufferContext(imageSurface, (void**)&dec_params.dst_surface) != DFB_OK)
                    ERROR_EXIT_LOG("%s - GetBackBufferContext() field %d FAILED!\n", __FUNCTION__, curField);

                /* RLE decode into the surface */
                if ((hd_rle_info.width != 0) && (hd_rle_info.height != 0))
                {
                    if (bdvd_sid_decode(NULL, &dec_params) != b_ok)
                        ERROR_EXIT_LOG("%s - bdvd_sid_decode() field %d FAILED!\n", __FUNCTION__, curField);
                }
            }
        }
        StopTimer(imageThreadParams->filePath, imageThreadParams);

        if (surfaceLocked) {
            if (imageSurface->Unlock( imageSurface ) != DFB_OK)
                ERROR_EXIT_LOG("%s - Unlock() FAILED!\n", __FUNCTION__);
            surfaceLocked = false;
        }

        SleepForDisplay(imageThreadParams);

        if (imageThreadParams->decodeSurface->Clear( imageThreadParams->decodeSurface, 0, 0, 0, 0) != DFB_OK)
            ERROR_EXIT_LOG("%s - Clear() FAILED!\n", __FUNCTION__);

        if (imageThreadParams->decodeSurface->SetBlittingFlags(imageThreadParams->decodeSurface, DSBLIT_DEINTERLACE) != DFB_OK)
            ERROR_EXIT_LOG("%s - SetBlittingFlags() FAILED!\n", __FUNCTION__);

        BlitSurface(imageSurface, imageThreadParams);
    }
exit:

    if (surfaceLocked)
        imageSurface->Unlock( imageSurface );
    if (imageSurface != NULL)
        imageSurface->Release(imageSurface);
    if (fileBuffer != NULL)
        bdvd_sid_free_input_buffer(fileBuffer);
    if (palette)
        palette->Release(palette);

}

__inline void InitializeBitReader(HD_RLE_Info *hd_rle_info, uint8_t *ptr)
{
    hd_rle_info->buff = ptr;
    hd_rle_info->offset = 0;
    hd_rle_info->workingByte = 0;
    hd_rle_info->bitRemaining = 0;
}

__inline void GetCleanByte(HD_RLE_Info *hd_rle_info)
{
   if (hd_rle_info->bitRemaining != 8)
   {
      hd_rle_info->workingByte = *(hd_rle_info->buff + hd_rle_info->offset);
      hd_rle_info->offset++;
      hd_rle_info->bitRemaining = 8;
   }
}

__inline uint32_t GetBits(HD_RLE_Info *hd_rle_info, uint32_t numBitsRequested)
{
    uint32_t ret = 0, mvBits=0;
    while (numBitsRequested)
    {
        if (hd_rle_info->bitRemaining == 0)
            GetCleanByte(hd_rle_info);

        mvBits = hd_rle_info->bitRemaining > numBitsRequested ? numBitsRequested : hd_rle_info->bitRemaining;
        ret |= (hd_rle_info->workingByte >> (8-mvBits)) << (numBitsRequested - mvBits);
        hd_rle_info->workingByte <<= mvBits;
        hd_rle_info->bitRemaining-=mvBits;
        numBitsRequested-=mvBits;
    }
    return ret;
}

bool extract_hd_rle_info(uint8_t* fileBuffer, HD_RLE_Info *hd_rle_info, ImageThreadParams *imageThreadParams)
{
   uint8_t *dcsqt;
   uint8_t *start_fileBuffer = fileBuffer;
   uint8_t *pPal;
   bool endCmdFound = false;
   uint32_t i;
   uint8_t y, cr, cb, a = 0xff;
   uint32_t pos;

   EXTRACT16(fileBuffer, hd_rle_info->id);
   EXTRACT32(fileBuffer, hd_rle_info->spu_size);
   EXTRACT32(fileBuffer, hd_rle_info->dcsqt_offset);

   hd_rle_info->dcsqt_offset -= 2;
   hd_rle_info->pxd_offset = (uint32_t)(fileBuffer - start_fileBuffer);
   hd_rle_info->fieldOffset[0] = hd_rle_info->pxd_offset;
   hd_rle_info->fieldOffset[1] = hd_rle_info->pxd_offset;
   dcsqt = start_fileBuffer + hd_rle_info->dcsqt_offset;

   EXTRACT16(dcsqt, hd_rle_info->sequenceTable.startTime);
   EXTRACT32(dcsqt, hd_rle_info->sequenceTable.offsetToNextDCSQ);

   for(i = 0; i < MAX_DCC && !imageThreadParams->error && !endCmdFound; i++)
   {
      EXTRACT8(dcsqt, hd_rle_info->sequenceTable.DCCCommand[i].cmd);

      switch(hd_rle_info->sequenceTable.DCCCommand[i].cmd)
      {
         case FSTA_DSP:
         case STA_DSP:
         case STP_DSP:
            break;
         case SET_COLOR2:
          pPal = (uint8_t*)hd_rle_info->palette;
          hd_rle_info->palette_size = 256;
          for(pos = 0; pos != hd_rle_info->palette_size; pos++)
         {
            EXTRACT8(dcsqt, y);
            EXTRACT8(dcsqt, cr);
            EXTRACT8(dcsqt, cb);
            hd_rle_info->palette[pos] =  (y << 24) | (cr << 16) | (cb << 8) | (a << 0);
         }
            break;
         case SET_CONTR2:
            EXTRACT16(dcsqt, hd_rle_info->sequenceTable.DCCCommand[i].val);
            break;
         case SET_DAREA2:
            InitializeBitReader(hd_rle_info, dcsqt);
            hd_rle_info->left = GetBits(hd_rle_info, 12) & 0x7FF;  // only take the last 11 bits
            hd_rle_info->right = GetBits(hd_rle_info, 12) & 0x7FF;  // only take the last 11 bits
             hd_rle_info->top = GetBits(hd_rle_info, 12) & 0x7FE;  // only take the last 11 bits, last bit should always be 0
             hd_rle_info->bottom = GetBits(hd_rle_info, 12) & 0x7FF;  // only take the last 11 bits
            hd_rle_info->height = hd_rle_info->bottom - hd_rle_info->top + 1;
            hd_rle_info->width = hd_rle_info->right - hd_rle_info->left + 1;
            dcsqt+=6; // finised this 6 byte section
            break;
         case CHG_COLCON2:
            EXTRACT16(dcsqt, hd_rle_info->sequenceTable.DCCCommand[i].val);
            break;
         case SET_DSPXA:
            EXTRACT16(dcsqt, hd_rle_info->fieldOffset[0]);
            EXTRACT16(dcsqt, hd_rle_info->fieldOffset[1]);
            break;
         case SET_DSPXA2:
            EXTRACT32(dcsqt, hd_rle_info->fieldOffset[0]);
            EXTRACT32(dcsqt, hd_rle_info->fieldOffset[1]);
            break;
         case CMD_END:
            endCmdFound = true;
            break;
         default:
            ERROR_EXIT_LOG("Unkown CMD 0x%02x!\n", hd_rle_info->sequenceTable.DCCCommand[i].cmd);
      }
   }

exit:

   return imageThreadParams->error;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************      DECODE AN MNG IMAGE        ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

#define MNG_CDB_SIZE (2*1024*1024)


typedef struct MNGThreadParams_tag{
    mng_handle mng_handle;

    pthread_t threadHandle;
    int32_t resumeTimeout;
    bool beginShutDown;
    BKNI_EventHandle mngThreadComplete;
    BKNI_EventHandle pendingRequest;
}MNGThreadParams;


typedef struct
{
    mng_handle mng_handle;
    ImageThreadParams *imageThreadParams;

    int32_t fd;
    IDirectFB* dfb;
    char *filePath;
    char *mng_cdb;
    BKNI_EventHandle showComplete;
    DFBSurfaceDescription *imgDesc;

    IDirectFBSurface* backBuffer;
    void* backBufferAddress;
    int32_t backBufferPitch;

    IDirectFBSurface* frontBuffer;
    void* frontBufferAddress;
    int32_t frontBufferPitch;

    MNGThreadParams delyInfo;
} priv_t;


mng_bool openstream(mng_handle hHandle);
mng_ptr my_memalloc(mng_size_t size);
void my_memfree(mng_ptr p, mng_size_t size);
mng_bool my_traceproc (mng_handle hHandle, mng_int32  iFuncnr, mng_int32  iFuncseq, mng_pchar  zFuncname);
mng_bool errorproc(mng_handle hHandle,mng_int32   iErrorcode, mng_int8    iSeverity, mng_chunkid iChunkname, mng_uint32  iChunkseq, mng_int32   iExtra1, mng_int32   iExtra2, mng_pchar   zErrortext);
mng_uint32 gettickcount(mng_handle hHandle);
mng_bool settimer(mng_handle hHandle, mng_uint32 iMsecs);
mng_bool closestream(mng_handle hHandle);
mng_bool readdata(mng_handle  hHandle, mng_ptr     pBuf, 	mng_uint32  iBuflen, 	mng_uint32p pRead);
mng_bool processheader(mng_handle hHandle, mng_uint32  iWidth, mng_uint32  iHeight);
mng_ptr getcanvasline(mng_handle hHandle, mng_uint32  iLinenr);
mng_ptr getbkgline(mng_handle hHandle, mng_uint32  iLinenr);
void getdfbsurface(mng_handle hHandle, IDirectFBSurface** surface);
void getdfbbkgsurface(mng_handle hHandle, IDirectFBSurface** surface);
void destroydfbsurface(mng_handle hHandle, IDirectFBSurface* surface);
void decodetodfbsurface(mng_handle hHandle, mng_ptr pBuf, mng_uint32 iBufLength, IDirectFBSurface** surface, DFBSurfaceBlittingFlags *surfaceBlitFlags);
mng_bool refresh(mng_handle hHandle, mng_uint32  iX, mng_uint32  iY, mng_uint32  iWidth, mng_uint32  iHeight);
void* MNGTimerThread(void* void_context);

void DecodeMNG( ImageThreadParams *imageThreadParams )
{
    priv_t priv;
    mng_retcode mngRet;
    memset(&priv,0,sizeof(priv));
    priv.dfb = gSettings.dfb;
    priv.imageThreadParams = imageThreadParams;
    priv.imgDesc = &imageThreadParams->imgDesc;
    priv.filePath = imageThreadParams->filePath;
    priv.fd=open(priv.filePath,O_RDONLY);
    priv.delyInfo.beginShutDown = FALSE;
    priv.mng_cdb = malloc(MNG_CDB_SIZE);

    /* Create needed thread protection */
    priv.showComplete = imageThreadParams->doneDisplay;
    BKNI_CreateEvent(&priv.delyInfo.mngThreadComplete);
    BKNI_CreateEvent(&priv.delyInfo.pendingRequest);
    BKNI_ResetEvent(priv.showComplete);
    BKNI_ResetEvent(priv.delyInfo.mngThreadComplete);
    BKNI_ResetEvent(priv.delyInfo.pendingRequest);

    /* TODO: Only create 1 timer thread!!!  In this test, a thread is created for each image, this is not necessary.  Only one MNG timer thread should be needed and a queue of pending requests for it to handle. */
    pthread_create(&priv.delyInfo.threadHandle,NULL,MNGTimerThread,&priv.delyInfo);
    pthread_detach(priv.delyInfo.threadHandle);

    /* Initialize libMNG and setup the callbacks */
    priv.mng_handle = mng_initialize(&priv,my_memalloc,my_memfree,my_traceproc);
    mngRet = mng_setcb_openstream(priv.mng_handle,openstream);
    mngRet = mng_setcb_closestream(priv.mng_handle,closestream);
    mngRet = mng_setcb_readdata(priv.mng_handle,readdata);
    mngRet = mng_setcb_processheader(priv.mng_handle,processheader);
    mngRet = mng_setcb_getcanvasline(priv.mng_handle,getcanvasline);
    mngRet = mng_setcb_refresh(priv.mng_handle,refresh);
    mngRet = mng_setcb_gettickcount(priv.mng_handle,gettickcount);
    mngRet = mng_setcb_errorproc(priv.mng_handle,errorproc);
    mngRet = mng_setcb_settimer(priv.mng_handle,settimer);
    mngRet = mng_setcb_getbkgdline(priv.mng_handle,getbkgline);
    mngRet = mng_setcb_getdfbsurface(priv.mng_handle,getdfbsurface);
    mngRet = mng_setcb_getdfbbkgsurface(priv.mng_handle,getdfbbkgsurface);
    mngRet = mng_setcb_decodetodfbsurface(priv.mng_handle,decodetodfbsurface);
    mngRet = mng_setcb_destroydfbsurface(priv.mng_handle,destroydfbsurface);
    mngRet = mng_set_canvasstyle(priv.mng_handle, MNG_CANVAS_ARGB8);
    mngRet = mng_set_bkgdstyle(priv.mng_handle, MNG_CANVAS_RGB8);
    /* Turn on directFB assisted MNG decode */
    if (gArguments.useHWAcceleratedMNG) {
        mngRet = mng_set_enabledfbdecode(priv.mng_handle, priv.mng_cdb, MNG_CDB_SIZE);
        sprintf(imageThreadParams->decoderUsed, "hardware assisted MNG");
        sprintf(imageThreadParams->decoderReason, "best decoder available");
    }
    else {
        sprintf(imageThreadParams->decoderUsed, "software MNG");
        sprintf(imageThreadParams->decoderReason, "user specified");
    }


    imageThreadParams->decodeSurface->Clear( imageThreadParams->decodeSurface, 0, 0, 0, 0);

    /* Start decoding the MNG file */
    mng_readdisplay(priv.mng_handle);

    /* Wait for decode complete */
    BKNI_WaitForEvent(priv.showComplete, BKNI_INFINITE);

    mymsleep(gArguments.millisecDelay);

    /* Send a request to the timer thread */
    priv.delyInfo.beginShutDown = TRUE;

    if (priv.delyInfo.pendingRequest != NULL)
        BKNI_SetEvent(priv.delyInfo.pendingRequest);

    /* Very important:  wait for the thread to stop to ensure libMNG gets to a clean state.
       If you do not wait for libMNG to get to a clean exit point, you will see stabillity issues.  To do this you must essentially have the thread signal you back that it did not restart libMNG for this image */
    if (priv.delyInfo.mngThreadComplete != NULL)
    {
        if (priv.delyInfo.threadHandle != 0)
            BKNI_WaitForEvent(priv.delyInfo.mngThreadComplete, BKNI_INFINITE);

        BKNI_DestroyEvent(priv.delyInfo.mngThreadComplete);
    }

    if (priv.delyInfo.pendingRequest != NULL)
        BKNI_DestroyEvent(priv.delyInfo.pendingRequest);

    /* Close lbMNG*/
    if (priv.mng_handle != NULL)
        mng_cleanup(&priv.mng_handle);

    if (priv.mng_cdb != NULL)
        free(priv.mng_cdb);

    /* Unlock the frame buffers if they are locked */
    if (priv.frontBufferAddress != NULL)
        priv.frontBuffer->Unlock(priv.frontBuffer);

    if (priv.backBufferAddress != NULL)
        priv.backBuffer->Unlock(priv.backBuffer);

    /* Free the frame buffers */
    if (priv.frontBuffer)
        priv.frontBuffer->Release(priv.frontBuffer);

    if (priv.backBuffer)
        priv.backBuffer->Release(priv.backBuffer);
}

mng_ptr my_memalloc(mng_size_t size) 
{
    mng_ptr p = malloc(size); 
    memset(p, 0, size); 
    return p;
}

void my_memfree(mng_ptr p, mng_size_t size) 
{
    if (p != NULL)
        free(p);
}

mng_bool my_traceproc (mng_handle hHandle,
                          mng_int32  iFuncnr,
                          mng_int32  iFuncseq,
                          mng_pchar  zFuncname)
{
    priv_t* priv=mng_get_userdata(hHandle);

    if (zFuncname == NULL)
        ThreadSafeLog(priv->imageThreadParams, "%s: %d iFuncnr=0x%08x iFuncseq=0x%08x\n", __FUNCTION__, __LINE__, iFuncnr, iFuncseq);
    else
        ThreadSafeLog(priv->imageThreadParams, "%s: %d iFuncnr=0x%08x iFuncseq=0x%08x in %s\n", __FUNCTION__, __LINE__, iFuncnr, iFuncseq, zFuncname);

    return MNG_TRUE;
}


mng_bool errorproc(mng_handle hHandle,mng_int32   iErrorcode,
                                                  mng_int8    iSeverity,
                                                  mng_chunkid iChunkname,
                                                  mng_uint32  iChunkseq,
                                                  mng_int32   iExtra1,
                                                  mng_int32   iExtra2,
                                                  mng_pchar   zErrortext)
{
    priv_t* priv=mng_get_userdata(hHandle);

    ThreadSafeLog(priv->imageThreadParams, "MNG ERROR: %s\n",zErrortext);
    if (priv->fd != 0)
        close(priv->fd);
    priv->fd = 0;
    BKNI_SetEvent(priv->showComplete);
    return MNG_TRUE;
}


mng_uint32 gettickcount(mng_handle hHandle)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}

mng_bool settimer(mng_handle hHandle, mng_uint32 iMsecs)
{
    priv_t* priv=mng_get_userdata(hHandle);

    priv->delyInfo.mng_handle=hHandle;
    priv->delyInfo.resumeTimeout=iMsecs;
    BKNI_SetEvent(priv->delyInfo.pendingRequest);
    return TRUE;
}

mng_bool closestream(mng_handle hHandle)
{
    priv_t* priv=mng_get_userdata(hHandle);
    if (priv->fd != 0)
        close(priv->fd);
    priv->fd = 0;
    BKNI_SetEvent(priv->showComplete);
    return MNG_TRUE;
}

mng_bool openstream(mng_handle hHandle) 
{	
  return MNG_TRUE; 
}


mng_bool readdata(mng_handle  hHandle,
                mng_ptr     pBuf,
                mng_uint32  iBuflen,
                mng_uint32p pRead)
{
    priv_t* priv=mng_get_userdata(hHandle);
    if(read(priv->fd,pBuf,iBuflen)==-1)
        ThreadSafeLog(priv->imageThreadParams, "read from file ERROR!\n");
    *pRead=iBuflen;
    return MNG_TRUE;
}

mng_bool processheader(mng_handle hHandle, mng_uint32  iWidth, mng_uint32  iHeight)
{
    priv_t* priv=mng_get_userdata(hHandle);

    priv->imgDesc->flags=DSDESC_CAPS|DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT;
    priv->imgDesc->caps=DSCAPS_VIDEOONLY;
    priv->imgDesc->width = iWidth;
    priv->imgDesc->height = iHeight;

    priv->imgDesc->pixelformat=DSPF_ARGB;
    priv->dfb->CreateSurface( priv->dfb, priv->imgDesc, &priv->frontBuffer );
    priv->frontBuffer->Clear(priv->frontBuffer, 0xaa, 0xaa, 0xaa, 0xff);

    priv->imgDesc->pixelformat=DSPF_RGB32;
    priv->dfb->CreateSurface( priv->dfb, priv->imgDesc, &priv->backBuffer );
    priv->backBuffer->Clear(priv->backBuffer, 0xaa, 0xaa, 0xaa, 0xff);

    priv->frontBufferAddress = NULL;
    priv->backBufferAddress = NULL;
    return MNG_TRUE;
}

mng_ptr getcanvasline(mng_handle hHandle, mng_uint32  iLinenr)
{
  priv_t* priv=mng_get_userdata(hHandle);
  if (priv->frontBufferAddress == NULL)
    priv->frontBuffer->Lock( priv->frontBuffer, DSLF_READ|DSLF_WRITE, &priv->frontBufferAddress, &priv->frontBufferPitch);
  return (char*)priv->frontBufferAddress + priv->frontBufferPitch*iLinenr;
}

mng_ptr getbkgline(mng_handle hHandle, mng_uint32  iLinenr)
{
    priv_t* priv=mng_get_userdata(hHandle);
    if (priv->backBufferAddress == NULL)
        priv->backBuffer->Lock( priv->backBuffer, DSLF_READ|DSLF_WRITE, &priv->backBufferAddress, &priv->backBufferPitch);
    return (char*)priv->backBufferAddress + priv->backBufferPitch*iLinenr;
}

void getdfbsurface(mng_handle hHandle, IDirectFBSurface** surface)
{
    priv_t* priv=mng_get_userdata(hHandle);
    if (priv->frontBufferAddress != NULL)
    {
        priv->frontBuffer->Unlock(priv->frontBuffer);
        priv->frontBufferAddress = NULL;
    }

    *surface = priv->frontBuffer;
    return;
}


void getdfbbkgsurface(mng_handle hHandle, IDirectFBSurface** surface)
{
    priv_t* priv=mng_get_userdata(hHandle);
    if (priv->backBufferAddress != NULL)
    {
        priv->backBuffer->Unlock(priv->backBuffer);
        priv->backBufferAddress = NULL;
    }

    *surface = priv->backBuffer;
    return;
}

void decodetodfbsurface(mng_handle hHandle, mng_ptr pBuf, mng_uint32 iBufLength, IDirectFBSurface** surface, DFBSurfaceBlittingFlags *surfaceBlitFlags)
{	
    priv_t* priv=(priv_t*)mng_get_userdata(hHandle);
    IDirectFBImageProvider *provider;
    IDirectFBDataBuffer      *buffer;
    DFBDataBufferDescription  ddsc;
    DFBSurfaceDescription dsc;
    DFBImageDescription imageDsc;

    ddsc.flags         = DBDESC_MEMORY;
    ddsc.memory.data   = pBuf;
    ddsc.memory.length = iBufLength;

    priv->dfb->CreateDataBuffer( priv->dfb, &ddsc, &buffer );
    buffer->CreateImageProvider( buffer, &provider );
    provider->GetSurfaceDescription(provider, &dsc);
    provider->GetImageDescription(provider, &imageDsc);

    dsc.pixelformat=DSPF_ARGB;

    priv->dfb->CreateSurface( priv->dfb, &dsc, surface );
    provider->RenderTo( provider, *surface, NULL );

    *surfaceBlitFlags = DSBLIT_BLEND_ALPHACHANNEL;

    if (imageDsc.caps & DICAPS_COLORKEY)
    {
        (*surface)->SetSrcColorKey(*surface, imageDsc.colorkey_r, imageDsc.colorkey_g, imageDsc.colorkey_b);
        *surfaceBlitFlags |= DSBLIT_SRC_COLORKEY;
    }

    if (provider)
        provider->Release(provider);
    if (buffer)
        buffer->Release(buffer);
}

void destroydfbsurface(mng_handle hHandle, IDirectFBSurface* surface)
{
    if (surface)
        surface->Release(surface);
}

mng_bool refresh(mng_handle hHandle, mng_uint32  iX, mng_uint32  iY, mng_uint32  iWidth, mng_uint32  iHeight)
{
    priv_t* priv=mng_get_userdata(hHandle);

    if (priv->frontBufferAddress != NULL)
    {
        priv->frontBuffer->Unlock(priv->frontBuffer);
        priv->frontBufferAddress = NULL;
    }

    if (priv->backBufferAddress != NULL)
    {
        priv->backBuffer->Unlock(priv->backBuffer);
        priv->backBufferAddress = NULL;
    }

    BlitSurface(priv->frontBuffer, priv->imageThreadParams);
    return MNG_TRUE;
}

void* MNGTimerThread(void* void_context)
{
    MNGThreadParams* ctx=void_context;

    /* TODO: Make this handle more than 1 MNG image so you only need to have one timer thread!
       Right now, a thread is spawned for each image, this is not necessary */
    while (TRUE)
    {
        BKNI_WaitForEvent(ctx->pendingRequest, BKNI_INFINITE);
        if (ctx->beginShutDown)
        break;
            usleep(ctx->resumeTimeout*1000);
            mng_display_resume(ctx->mng_handle);
    }

    BKNI_SetEvent(ctx->mngThreadComplete);
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   Directory List Functions  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////




/*********************************************************************
  FindImageType
  Determine wether the current files extension is
  in the list of possible files. Returns true if it is
 *********************************************************************/
void FindImageType(char * extension, ImageTypeInfo **imageTypeInfo)
{
    char *cp, *buff = NULL;
    uint32_t i = 0;

    *imageTypeInfo = NULL;

    if (extension != NULL)
    {
        if ((buff = strdup(extension)) == NULL)
            STD_PRINT("ERROR: Unable to strdup(\"%s\") in FindImageType.\n", extension);
        else {
            /*Converts all the extensions to lower case*/
            cp = buff;
            while (*cp != '\0')
                *cp++ = (char) tolower((int) *cp);

            cp = *buff == '.' ? buff+1 : buff;

            while ((strcmp("", validFileTypes[i].extension + 1) != 0) &&
                   (strcmp(cp, validFileTypes[i].extension + 1) != 0))
                i++;

            *imageTypeInfo = &validFileTypes[i];
        }
    }

    if (buff)
        free(buff);
}



bool AddFileToList(char *newFileName, bool nameContainsDirctory)
{
    char * filePath;
    char * fileName;
    char * fileExtension;
    char * directory = gArguments.directory;
    char *fName = newFileName;
    char tempDirBuffer[MAX_SMALL_STRING_LEN];
    char tempNameBuffer[MAX_SMALL_STRING_LEN];
    ImageTypeInfo *imageTypeInfo = NULL;
    bool added = false;

    if (nameContainsDirctory)
    {
        int32_t i = strlen(newFileName) - 1;
        while((newFileName[i] != '/') && (newFileName[i] != '\\') && (i > 0)) i--;

        if (i != 0)
        {
            strcpy(tempNameBuffer, newFileName+i+1);
            strncpy(tempDirBuffer, newFileName, i);
            tempDirBuffer[i] = '\0';
            fName = tempNameBuffer;

        }
        else
        {
            fName = newFileName;
            strcpy(tempDirBuffer, "./");
        }
        directory = tempDirBuffer;
    }

    fileExtension = strrchr(fName, '.');
    FindImageType(fileExtension, &imageTypeInfo);
    if (( imageTypeInfo != NULL) && (imageTypeInfo->imageType != IMAGETYPE_NUM_TYPES)) {
        filePath = malloc(strlen(directory)+strlen(fName)+2);
        fileName = malloc(strlen(fName)+2);
        if (filePath)
        { 
            if (directory[strlen(directory)-1] == '/')
                sprintf(filePath, "%s%s", directory, fName);
            else
                sprintf(filePath, "%s/%s", directory, fName);

            sprintf(fileName, "%s", fName);

            if (gSettings.firstImage == NULL){
                gSettings.firstImage = (ImageNode *) malloc(sizeof(ImageNode));
                gSettings.currentImage = gSettings.firstImage;
            }
            else {
                gSettings.currentImage->link = (ImageNode *) malloc(sizeof(ImageNode));
                gSettings.currentImage = gSettings.currentImage->link;
            }

            gSettings.currentImage->link = NULL;
            gSettings.currentImage->id = (gSettings.numImages++) + 1;
            gSettings.currentImage->fileName = fileName;
            gSettings.currentImage->filePath = filePath;
            gSettings.currentImage->imageTypeInfo = imageTypeInfo;
            added = true;
        }
    }

    return added;
}


/*********************************************************************
  BuildList
  This takes a directory and builds a link list
  from the valid image files in the supplied directory
*********************************************************************/

void BuildList()
{
    struct dirent *currentImage;
    DIR * directory;

    gSettings.firstImage = NULL;
    gSettings.currentImage = NULL;
    gSettings.numImages = 0;
    directory = opendir(gArguments.directory);

    if (directory != NULL)
    {
        while ((currentImage = readdir(directory)) != NULL)
            AddFileToList(currentImage->d_name, false);
        gSettings.currentImage = gSettings.firstImage;
    }
    else {
        STD_PRINT("ERROR - Cannot open directory \"%s\".\n", gArguments.directory);
    }
}

/*********************************************************************
 GetNextImage 
 Returns the next file in the list
 *********************************************************************/

ImageNode* GetNextImage()
{
    ImageNode *currentImage = NULL;

    currentImage = gSettings.currentImage;

    if (gSettings.currentImage == NULL)
    {
        gSettings.iterationsCompleted++;
        if (gSettings.iterationsCompleted > gArguments.iterations)
            gSettings.iterationsCompleted = gArguments.iterations;
        if (gSettings.iterationsCompleted < gArguments.iterations)
            gSettings.currentImage = gSettings.firstImage;

        currentImage = gSettings.currentImage;
    }
    else
        gSettings.currentImage = gSettings.currentImage->link;

    return currentImage;
}



/*********************************************************************
  ClearList
  Clears the linked list and deallocated the memory
*********************************************************************/

void ClearList()
{
    gSettings.currentImage = gSettings.firstImage;

    while (gSettings.firstImage != NULL)
    {
        gSettings.currentImage = gSettings.currentImage->link;

        free(gSettings.firstImage->filePath);
        free(gSettings.firstImage->fileName);
        free(gSettings.firstImage);
        gSettings.firstImage = gSettings.currentImage;
    }
}







///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   Layer/Image Thread functions  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


void* ImageDecodeThread(void* ctx) {
    ImageThreadParams *imageThreadParams = (ImageThreadParams*)ctx;

    BKNI_WaitForEvent(gSettings.threadProtect, BKNI_INFINITE);
    ImageNode *currentFile = GetNextImage(gSettings);
    BKNI_SetEvent(gSettings.threadProtect);
    bool errorFound;
    ImageTypeResults *workingResults;
    DecoderType      *workingDecoder;
    uint32_t          decoderIndex;

    while ((currentFile != NULL) && (!imageThreadParams->terminated)) {
        /* Reset needed decode thread params */
        imageThreadParams->blittedVersions = 0;
        imageThreadParams->logIndent = 0;
        imageThreadParams->indentString[0] = '\0';
        imageThreadParams->error = false;
        imageThreadParams->id = currentFile->id;
        imageThreadParams->screenOutputPos = imageThreadParams->screenOutput;
        imageThreadParams->logOutputPos = imageThreadParams->logOutput;
        imageThreadParams->filePath = currentFile->filePath;
        imageThreadParams->fileName = currentFile->fileName;
        imageThreadParams->userTriggeredContinueReceived = false;
        imageThreadParams->outputDisplayed = false;
        imageThreadParams->logWritten = false;
        sprintf(imageThreadParams->decoderUsed, "undefined");
        sprintf(imageThreadParams->decoderReason, "undefined");
        imageThreadParams->mpps = 0;

        imageThreadParams->decodeSurface->SetBlittingFlags(imageThreadParams->decodeSurface, DSBLIT_NOFX);


        errorFound = false;

        ThreadSafeLog(imageThreadParams, "------------------------------------------------\n");
        ThreadSafeLog(imageThreadParams, "IMAGE %d/%d: %s\n\n", imageThreadParams->id, gSettings.numImages, imageThreadParams->fileName);
        ThreadSafeLog(imageThreadParams, "Full Path: %s\n", imageThreadParams->filePath);
        ThreadSafeLog(imageThreadParams, "Itteration: %d\n", gSettings.iterationsCompleted + 1);

        if (currentFile->imageTypeInfo != NULL) {

            ThreadSafeLog(imageThreadParams, "\n");
            imageThreadParams->error = false;

            if (currentFile->imageTypeInfo->decodeFunction != NULL)
            {
                ThreadSafeLog(imageThreadParams, "DECODE INFO:\n");
                ThreadSafeTabIn(imageThreadParams);

                currentFile->imageTypeInfo->decodeFunction( imageThreadParams );
                ThreadSafeLog(imageThreadParams, "Using %s decoder because -- %s\n",
                              imageThreadParams->decoderUsed, imageThreadParams->decoderReason);
                ThreadSafeLog(imageThreadParams, " --- %s ---\n",
                              imageThreadParams->error ? "FAILED" : "PASSED");

                ThreadSafeTabOut(imageThreadParams);
                ThreadSafeLog(imageThreadParams, "\n");
                errorFound |= imageThreadParams->error;
                imageThreadParams->error = false;
            }

            if (currentFile->imageTypeInfo->byteCompareFunction != NULL)
            {
                ThreadSafeLog(imageThreadParams, "BYTE COMPARE INFO:\n");
                ThreadSafeTabIn(imageThreadParams);

                currentFile->imageTypeInfo->byteCompareFunction( imageThreadParams );

                ThreadSafeLog(imageThreadParams, " --- %s ---\n",
                              imageThreadParams->error ? "FAILED" : "PASSED");

                ThreadSafeTabOut(imageThreadParams);
                ThreadSafeLog(imageThreadParams, "\n");
                errorFound |= imageThreadParams->error;
                imageThreadParams->error = false;
            }
        }
        ThreadSafeLog(imageThreadParams, "------------------------------------------------\n");


        if (!errorFound && !gArguments.verboseOutput)
        {
            imageThreadParams->outputDisplayed = true;
            imageThreadParams->logWritten = true;
            imageThreadParams->userTriggeredContinueReceived = true;
        }

        WaitForInput( imageThreadParams );
        PrintThreadSafeLogToScreen( imageThreadParams );
        WriteThreadSafeLogToFile( imageThreadParams );

        BKNI_WaitForEvent(gSettings.threadProtect, BKNI_INFINITE);

        workingResults = &gSettings.results[currentFile->imageTypeInfo->imageType];
        decoderIndex = 0;
        while ((strcmp("", workingResults->decoder[decoderIndex].decoderName) != 0) &&
               (strcmp(imageThreadParams->decoderUsed, workingResults->decoder[decoderIndex].decoderName) != 0))
           decoderIndex++;

        workingDecoder = &workingResults->decoder[decoderIndex];

        sprintf(workingDecoder->decoderName, "%s", imageThreadParams->decoderUsed);
        workingResults->tested++;
        workingDecoder->tested++;
        if (!errorFound)
        {
            workingResults->passed++;
            workingDecoder->passed++;
            if ((imageThreadParams->mpps > 0) && (imageThreadParams->mpps < 50))
            {
                workingResults->decodeRatesCounted++;
                workingResults->totalDecodeRates += imageThreadParams->mpps;
                workingResults->averageDecodeRate = workingResults->totalDecodeRates /
                                                    workingResults->decodeRatesCounted;

                workingDecoder->decodeRatesCounted++;
                workingDecoder->totalDecodeRates += imageThreadParams->mpps;
                workingDecoder->averageDecodeRate = workingDecoder->totalDecodeRates /
                                                    workingDecoder->decodeRatesCounted;
            }
        }

        currentFile = GetNextImage();
        BKNI_SetEvent(gSettings.threadProtect);
    }
    BKNI_SetEvent(imageThreadParams->threadComplete);
    return NULL;
}


void* LayerDisplayThread(void* ctx) {
    LayerThreadParams *layerParams = (LayerThreadParams *)ctx;
    ImageThreadParams* imageThreadParams = layerParams->imageThreadParams;
    int32_t i;

    while (!layerParams->terminated)
    {
        BKNI_WaitForEvent(gSettings.displayBlitProtect, BKNI_INFINITE);

        /* Draw each "latestFrame" to the display surface */
        for(i = 0; i != gArguments.numThreads; i++)
        {
            if (imageThreadParams[i].decodeSurface != NULL)
            {
                layerParams->displaySurface->Blit(  layerParams->displaySurface,
                                                    imageThreadParams[i].decodeSurface, 
                                                    NULL,
                                                    imageThreadParams[i].startLeft,
                                                    imageThreadParams[i].startTop);
            }
        }

        /* Flip the display surface to the screen */
        layerParams->displaySurface->Flip(layerParams->displaySurface, NULL, DSFLIP_NONE);

        BKNI_SetEvent(gSettings.displayBlitProtect);

        /* Wait whatever the refresh rate is to illustrate dropped frames */
        usleep(gArguments.refreshRate * 1000);
    }
 
    BKNI_SetEvent(layerParams->threadComplete);

    return NULL;
}


void DecodeToLayer(bdvd_dfb_ext_layer_type_e layerType, uint32_t layerLevel)
{
    int32_t i;
    LayerThreadParams layerThreadParams;
    DFBDisplayLayerConfig displayLayerConfig;
    DFBLocation displayLayerLocation;
    bdvd_dfb_ext_h dfbExt;
    bdvd_dfb_ext_layer_settings_t dfbExtSettings;
    IDirectFBSurface * displaySurface;
    IDirectFBDisplayLayer * displayLayer;
    DFBResult err = DFB_OK;

    memset(&layerThreadParams, 0, sizeof(layerThreadParams));
    memset(&displayLayerConfig, 0, sizeof(displayLayerConfig));
    memset(&displayLayerLocation, 0, sizeof(displayLayerLocation));

    displayLayerLocation.w = 1;
    displayLayerLocation.h = 1;

    dfbExt=bdvd_dfb_ext_layer_open(layerType);
    bdvd_dfb_ext_layer_get(dfbExt, &dfbExtSettings);

    /* Let's set the layer configuration if required.    */
    DFBCHECK( gSettings.dfb->GetDisplayLayer(gSettings.dfb, dfbExtSettings.id, &displayLayer) );
    DFBCHECK( displayLayer->SetCooperativeLevel(displayLayer, DLSCL_ADMINISTRATIVE) );

    DFBCHECK( displayLayer->SetBackgroundColor(displayLayer, 0, 0, 0, 0 ) );
    DFBCHECK( displayLayer->SetBackgroundMode(displayLayer, DLBM_COLOR ) );
    displayLayerConfig.flags |= (DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT);
    displayLayerConfig.width = gSettings.screenWidth;
    displayLayerConfig.height = gSettings.screenHeight;
    displayLayerConfig.pixelformat = DSPF_ARGB;

    DFBCHECK( displayLayer->SetConfiguration(displayLayer, &displayLayerConfig) );

    displayLayer->GetSurface(displayLayer, &displaySurface);
    displaySurface->Clear(displaySurface, 0, 0, 0, 0);
    displaySurface->SetBlittingFlags(displaySurface, DSBLIT_NOFX );
    displayLayer->SetLevel(displayLayer, layerLevel);

    BKNI_CreateEvent(&layerThreadParams.threadComplete);
    BKNI_ResetEvent(layerThreadParams.threadComplete);

    layerThreadParams.displaySurface = displaySurface;
    layerThreadParams.displayLayer = displayLayer;
    layerThreadParams.layerType = layerType;
    layerThreadParams.layerLevel = layerLevel;
    layerThreadParams.imageThreadParams = malloc(sizeof(*layerThreadParams.imageThreadParams) * gArguments.numThreads);
    layerThreadParams.imageThread =  malloc(sizeof(*layerThreadParams.imageThread) * gArguments.numThreads);

    layerThreadParams.numHorizontalCubes = 1;
    layerThreadParams.numVerticalCubes = 1;

    while(gArguments.numThreads > (layerThreadParams.numHorizontalCubes * layerThreadParams.numVerticalCubes))
    {
        if (layerThreadParams.numHorizontalCubes == layerThreadParams.numVerticalCubes)
            layerThreadParams.numVerticalCubes++;
        else
            layerThreadParams.numHorizontalCubes++;
    }

    for(i = 0; i != gArguments.numThreads; i ++)
    {
        uint32_t cubeWidth = gSettings.screenWidth/layerThreadParams.numHorizontalCubes;
        uint32_t cubeHeight = gSettings.screenHeight/layerThreadParams.numVerticalCubes;
        layerThreadParams.imageThreadParams[i].startTop =  cubeHeight * (i / layerThreadParams.numHorizontalCubes);
        layerThreadParams.imageThreadParams[i].startLeft = cubeWidth * (i % layerThreadParams.numHorizontalCubes);
        layerThreadParams.imageThreadParams[i].fileName = NULL;
        layerThreadParams.imageThreadParams[i].filePath = NULL;
        layerThreadParams.imageThreadParams[i].id = 0;
        layerThreadParams.imageThreadParams[i].lastDisplayTime = -1;
        layerThreadParams.imageThreadParams[i].blittedVersions = 0;
        layerThreadParams.imageThreadParams[i].terminated = false;

        layerThreadParams.imageThreadParams[i].outDesc.flags=DSDESC_CAPS|DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT;
        layerThreadParams.imageThreadParams[i].outDesc.caps=DSCAPS_VIDEOONLY;
        layerThreadParams.imageThreadParams[i].outDesc.width = cubeWidth;
        layerThreadParams.imageThreadParams[i].outDesc.height = cubeHeight;
        layerThreadParams.imageThreadParams[i].outDesc.pixelformat= DSPF_ARGB;
        gSettings.dfb->CreateSurface( gSettings.dfb, &layerThreadParams.imageThreadParams[i].outDesc, &layerThreadParams.imageThreadParams[i].decodeSurface );
        layerThreadParams.imageThreadParams[i].decodeSurface->Clear( layerThreadParams.imageThreadParams[i].decodeSurface, 0, 0, 0, 0);
        layerThreadParams.imageThreadParams[i].decodeSurface->SetFont(
                        layerThreadParams.imageThreadParams[i].decodeSurface,
                        gSettings.font);

        BKNI_CreateEvent(&layerThreadParams.imageThreadParams[i].doneDisplay);
        BKNI_ResetEvent(layerThreadParams.imageThreadParams[i].doneDisplay);
        BKNI_CreateEvent(&layerThreadParams.imageThreadParams[i].threadComplete);
        BKNI_ResetEvent(layerThreadParams.imageThreadParams[i].threadComplete);

        pthread_create(&layerThreadParams.imageThread[i],NULL,ImageDecodeThread,&layerThreadParams.imageThreadParams[i]);
        pthread_detach(layerThreadParams.imageThread[i]);
    }

    STD_PRINT("Starting Image decodes...\n");
    pthread_create(&layerThreadParams.layerThread,NULL,LayerDisplayThread,&layerThreadParams);
    pthread_detach(layerThreadParams.layerThread);

    /* Wait for all decode threads to complete */
    for(i = 0; i != gArguments.numThreads; i ++)
        BKNI_WaitForEvent(layerThreadParams.imageThreadParams[i].threadComplete, BKNI_INFINITE);

    layerThreadParams.terminated = true;
    BKNI_WaitForEvent(layerThreadParams.threadComplete, BKNI_INFINITE);

    STD_PRINT("Finished Image decodes\n");

    if (layerThreadParams.imageThreadParams != NULL)
    {
        for(i = 0; i != gArguments.numThreads; i ++)
        {
            if (layerThreadParams.imageThreadParams[i].decodeSurface)
                layerThreadParams.imageThreadParams[i].decodeSurface->Release(layerThreadParams.imageThreadParams[i].decodeSurface);
            if (layerThreadParams.imageThreadParams[i].doneDisplay != NULL)
                BKNI_DestroyEvent(layerThreadParams.imageThreadParams[i].doneDisplay);
        }
        free(layerThreadParams.imageThreadParams);
        BKNI_DestroyEvent(layerThreadParams.threadComplete);
    }

    if (layerThreadParams.imageThread)
        free(layerThreadParams.imageThread);

    if (displaySurface != NULL)
        displaySurface->Release( displaySurface );

    if (displayLayer != NULL)
        displayLayer->Release( displayLayer );

    if (dfbExt != NULL)
        bdvd_dfb_ext_layer_close(dfbExt);


}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   User Input Contol Functions  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////




char **MakeVideoArgList(char * args)
{
   int i = 0, argCount =0, length;
   char **argList = NULL;
   char *pStart;
   for(i = 0; i != strlen(args) + 1; i++)
   {
      if ((args[i] == ' ') || (args[i] == '\0'))
         argCount++;
   }

   argList = malloc(4 *argCount);
   pStart = args;
   argCount = 0;

   for(i = 0; i != strlen(args) + 1; i++)
   {
      if ((args[i] == ' ') || (args[i] == '\0'))
      {
         length = (args + i) - pStart;
         argList[argCount] = malloc(length);
	 memcpy(argList[argCount], pStart, length);
	 argList[argCount][length] = '\0';
	 argCount++;
	 pStart = (args + i) + 1;
      }
   }

   return argList;
}


/*********************************************************************
  PrintTestParams
    Prints out the commandline arguments currently in use
*********************************************************************/

void PrintTestParams()
{
    STD_PRINT("\twaitForInput=%d\n", gArguments.waitForInput);
    STD_PRINT("\titerations=%d\n", gArguments.iterations);
    STD_PRINT("\tmillisecDelay=%d\n", gArguments.millisecDelay);
    STD_PRINT("\tppmSurfaceDump=%d\n", gArguments.ppmSurfaceDump);
    STD_PRINT("\tcifSurfaceDump=%d\n", gArguments.cifSurfaceDump);
    STD_PRINT("\tscaleImage=%d\n", gArguments.scaleImage);
    STD_PRINT("\tuseDataBuffer=%d\n", gArguments.useDataBuffer);
    STD_PRINT("\tuseHWAcceleratedMNG=%d\n", gArguments.useHWAcceleratedMNG);
    STD_PRINT("\tverboseOutput=%d\n", gArguments.verboseOutput);
    STD_PRINT("\tnumThreads=%d\n", gArguments.numThreads);
    STD_PRINT("\trefreshRate=%d\n", gArguments.refreshRate);
    STD_PRINT("\tnumDecodeCompares=%d\n", gArguments.numDecodeCompares);
    STD_PRINT("\tnumDecodesToAverage=%d\n", gArguments.numDecodesToAverage);
    STD_PRINT("\tdirectory=\"%s\"\n", gArguments.directory);
    STD_PRINT("\tvideoFileName=\"%s\"\n", gArguments.videoFileName);
    STD_PRINT("\tlogFileName=\"%s\"\n", gArguments.logFileName);
}


/*********************************************************************
  PrintUsage
  Prints out the optional agruments this program has
**********************************************************************/

void PrintUsage(const char * program_name) 
{
    STD_PRINT("\nUsage: \n"
              "%s [-d dir] [-v videoFile] [-i count] [-l logfile] [-m msec] [-u numCompares]\n"
              "\t[-t numThreads] [-r msec] [-o resolution] [-w] [-s] [-r] [-b] [-c] [-p] [-k] [-x] [filelist...]\n\n", program_name);
    STD_PRINT("This test application will display one or more images using DirectFB.\n\n");
    STD_PRINT("The -d option will set the directory where the image files are located.\n");
    STD_PRINT("The -x option will enable verbose output.\n");
    STD_PRINT("The -i option will set the number of iterations.\n");
    STD_PRINT("The -r option will set the refresh rate for the dispaly.\n");
    STD_PRINT("The -t option will set the number of decoder threads to run.\n");
    STD_PRINT("The -l option specifies the name of the logfile.\n");
    STD_PRINT("The -u option specifies the number of times to decode the image and compare the decoded output for errors\n");
    STD_PRINT("The -a option specifies the number of times to decode an image to get an average decode rate.\n");
    STD_PRINT("The -m option pauses for the specified number of millisec after each image.\n");
    STD_PRINT("The -s option will scale the image to the screen.\n");
    STD_PRINT("The -p option will dump each image to a PPM file located in the same directory as the souce image.\n");
    STD_PRINT("The -c option will dump each image to a CIF file located in the same directory as the souce image.\n");
    STD_PRINT("The -u option will decode the image X number of times and compare the pixel output for consistancy.\n");
    STD_PRINT("The -b option will decode the image from a data buffer.\n");
    STD_PRINT("The -y option will set the output resolution.  Valid settings are 480, 720, and 1080.\n");
    STD_PRINT("The -v option will play the video file in a loop whle decodeing the images.\n");
    STD_PRINT("The -k option will prevent hardware accelerated MNG decode.\n");
    STD_PRINT("The -w wait for user input before going on to the next image. Only one thread is allowed with this option.\n");

    STD_PRINT("\nAny trailing filenames will invoke added for decode.\n");
    STD_PRINT("\n");
    STD_PRINT("Current Test Parameters:\n"); 
    PrintTestParams();
}


/*********************************************************************
  InitializeSettings
  Gets the options from the commandline and saves them
*********************************************************************/

bool InitializeSettings(int32_t argc, char *argv[])
{
    int32_t index;
    bool error = false;
    uint32_t resolution;
    char args[MAX_SMALL_STRING_LEN];
    char c;

    gArguments.waitForInput = 0;
    gArguments.iterations= 1;
    gArguments.refreshRate= 10;
    gArguments.numThreads = 1;
    gArguments.scaleImage= 0;
    gArguments.ppmSurfaceDump=0;
    gArguments.cifSurfaceDump=0;
    gArguments.useDataBuffer = 0;
    gArguments.numDecodeCompares = 0;
    gArguments.numDecodesToAverage = 1;
    gArguments.useHWAcceleratedMNG = 1;
    gArguments.verboseOutput = 0;
    gArguments.videoFileName[0] = '\0';
    gArguments.directory[0] = '\0';
    gArguments.logFileName[0] = '\0';

    gSettings.iterationsCompleted = 0;
    gSettings.videoFormat = bdvd_video_format_1080i;
    gSettings.screenHeight = 1080;
    gSettings.screenWidth = 1920;
    gSettings.logfp = NULL;
    gSettings.displayNameOnScreen = true;
    gSettings.displayBlitProtectReserved = false;
    BKNI_CreateEvent(&gSettings.threadProtect);
    BKNI_ResetEvent(gSettings.threadProtect);
    BKNI_SetEvent(gSettings.threadProtect);
    BKNI_CreateEvent(&gSettings.displayBlitProtect);
    BKNI_ResetEvent(gSettings.displayBlitProtect);
    BKNI_SetEvent(gSettings.displayBlitProtect);

    while ((c = getopt(argc, argv, "xwophsbckd:i:u:a:m:r:y:t:l:v:")) != -1) {
        switch (c){
        case 'v':
            strncpy(gArguments.videoFileName, optarg, sizeof(gArguments.videoFileName));
            sprintf(args, "./dfbbrcm_imagetest -loop -file %s", gArguments.videoFileName);
            gSettings.videoArgs = MakeVideoArgList(args);
            Log(" --> Playing video file %s.\n", gArguments.videoFileName);
            break;
        case 'd':
            strncpy(gArguments.directory, optarg, sizeof(gArguments.directory));
            Log(" --> Parsing Directory %s...\n", gArguments.directory);
            break;
        case 'i':
            gArguments.iterations = strtoul(optarg, NULL, 10);
            Log(" --> Setting itterations to %d.\n", gArguments.iterations);
            break;
        case 'l':
            if (gArguments.logFileName[0] == (char) NULL)
            {
                strncpy(gArguments.logFileName, optarg, sizeof(gArguments.logFileName));

                gSettings.logfp = fopen(gArguments.logFileName, "w");
                if (!gSettings.logfp)
                    STD_PRINT(" **! ERROR: Could not create log file \"%s\"!\n", gArguments.logFileName);
                else
                    Log(" --> Logging output to %s.\n", gArguments.logFileName);
            }
            break;
        case 'x':
            gArguments.verboseOutput = true;
            Log(" --> Enabling verbose output.\n");
            break;
        case 'm':
            gArguments.millisecDelay = strtoul(optarg, NULL, 10);
            Log(" --> Setting decode delay to %d.\n", gArguments.millisecDelay);
            break;
        case 'u':
            gArguments.numDecodeCompares = strtoul(optarg, NULL, 10);
            Log(" --> Setting number of decode compares to %d.\n", gArguments.numDecodeCompares);
            break;
        case 'a':
            gArguments.numDecodesToAverage = strtoul(optarg, NULL, 10);
            Log(" --> Setting number of decodes to perform to %d.\n", gArguments.numDecodesToAverage);
            break;
        case 'r':
            gArguments.refreshRate = strtoul(optarg, NULL, 10);
            Log(" --> Setting refresh rate to %d.\n", gArguments.refreshRate);
            break;
        case 't':
            gArguments.numThreads = strtoul(optarg, NULL, 10);
            gArguments.waitForInput = 0;
            if (gArguments.numThreads > 1)
                gSettings.displayNameOnScreen = false;

            Log(" --> Setting number of threads to %d.  Disabling \"wait for input\"\n", gArguments.numThreads);
            break;
        case 'w':
            gArguments.waitForInput = 1;
            gArguments.numThreads = 1;
            gArguments.verboseOutput = true;
            Log(" --> Enabling wait for input.  Forcing number of threads to 1\n");
            break;
        case 'p':
            gArguments.ppmSurfaceDump = 1;
            Log(" --> Dumping PPM output surfaces to the folder contining the original image\n");
            break;
        case 'c':
            gArguments.cifSurfaceDump = 1;
            Log(" --> Dumping CIF output surfaces to the folder contining the original image\n");
            break;
        case 's':
            gArguments.scaleImage = 1;
            Log(" --> Scaling images to fit the screen\n");
            break;
        case 'k':
            gArguments.useHWAcceleratedMNG = 0;
            Log(" --> Disabling DirectFB MNG decode\n");
            break;
        case 'b':
            gArguments.useDataBuffer = 1;
            Log(" --> Enabling decode with data buffers\n");
            break;
        case 'y':
            resolution = strtoul(optarg, NULL, 10);
            switch(resolution){
            case 480:
                gSettings.screenHeight = 480;
                gSettings.screenWidth = 720;
                gSettings.videoFormat = bdvd_video_format_ntsc;
                Log(" --> Setting output resolution to 480\n");
                break;
            case 720:
                gSettings.screenHeight = 720;
                gSettings.screenWidth = 1024;
                gSettings.videoFormat = bdvd_video_format_720p;
                Log(" --> Setting output resolution to 720p\n");
                break;
            case 1080:
                gSettings.screenHeight = 1080;
                gSettings.screenWidth = 1920;
                gSettings.videoFormat = bdvd_video_format_1080i;
                Log(" --> Setting output resolution to 1080i\n");
                break;
            default:
                STD_PRINT("**Unknown resolution setting.Valid options are 480, 720, and 1080\n");
                error = true;
                break;
            }
            break;
        case '?':
        case 'h':
            PrintUsage(argv[0]);
            error = true;
            break;
        default:
            break;
        }
    }

    if ( !error )
    {
        if (gArguments.directory[0] != '\0')
        {
            BuildList();
            if (gSettings.numImages == 0)
                STD_PRINT(" **! ERROR: Found no files in the directory to decode!");
        }

        if (optind < argc)
        {
            for (index = optind; index < argc; index++)
            {
                if (AddFileToList(argv[index], true))
                    Log(" --> Explicit file[%d]: %s\n", (index-optind)+1, argv[index]);
                else
                    STD_PRINT(" **! ERROR: Explicit file \"%s\" not valid!\n", argv[index]);

            }
            gSettings.currentImage = gSettings.firstImage;
        }

        if (gSettings.numImages == 0) {
            STD_PRINT(" **! ERROR: No files found for decode.  Exiting.\n");
            error = true;
        }
    }

    Log("\n");

    if ( !error )
    {
        bdvd_init(BDVD_VERSION);
        gSettings.bdisplay = bdvd_display_open(B_ID(0));
        gSettings.dfb=bdvd_dfb_ext_open(1);
        display_set_video_format(gSettings.bdisplay, gSettings.videoFormat);
        gSettings.dfb->SetVideoMode( gSettings.dfb, gSettings.screenWidth, gSettings.screenHeight, 32 );

        gSettings.fontDesc.flags = DFDESC_HEIGHT | DFDESC_ATTRIBUTES;
        gSettings.fontDesc.attributes = DFFA_NOKERNING;
        gSettings.fontDesc.height = 32;

        snprintf(gSettings.fontName, sizeof(gSettings.fontName)/sizeof(gSettings.fontName[0]), DATADIR"/fonts/msgothic.ttc");

        gSettings.dfb->CreateFont (gSettings.dfb, gSettings.fontName, &gSettings.fontDesc, &gSettings.font);

        if (gSettings.videoArgs != NULL) {
            if (playbackscagat_start(gSettings.bdisplay, NULL, NULL, NULL, 4, gSettings.videoArgs) != bdvd_ok)
                STD_PRINT("Video playback failed!\n");
        }
    }

    return error;
}


void DestroySettings()
{
    if (gSettings.logfp != NULL)
        fclose(gSettings.logfp);

    ClearList();

    if (gSettings.threadProtect != NULL)
        BKNI_DestroyEvent(gSettings.threadProtect);

    if (gSettings.displayBlitProtect != NULL)
        BKNI_DestroyEvent(gSettings.displayBlitProtect);

    if (gSettings.videoArgs != NULL)
        free(gSettings.videoArgs);

    if (gSettings.dfb != NULL) {
        gSettings.dfb->Release( gSettings.dfb );
        bdvd_dfb_ext_close();
    }

    if (gSettings.bdisplay != NULL)
        bdvd_display_close(gSettings.bdisplay);

    /*bdvd_uninit();*/
}





///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   Logging and Timer Functions  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


void StartTimer(char *fileName, ImageThreadParams *imageThreadParams )
{
    /* This mutex ensures we get an accurate value for the decode time.  However, we are only going to lock this if there is 1 thread.  For multiple threads we are interested in making sure things are thread safe not the decode rate. */
    if (gArguments.numThreads == 1) {
        BKNI_WaitForEvent(gSettings.displayBlitProtect, BKNI_INFINITE);
        gSettings.displayBlitProtectReserved = true;
    }



    gSettings.dfb->WaitIdle(gSettings.dfb);
    imageThreadParams->startTime = myclock();
}

void StopTimer(char *fileName, ImageThreadParams *imageThreadParams )
{
    uint32_t totalTime = 0;
    uint32_t totalPixels = 0;

    gSettings.dfb->WaitIdle(gSettings.dfb);
    imageThreadParams->endTime = myclock();

    if (gSettings.displayBlitProtectReserved) {
        BKNI_SetEvent(gSettings.displayBlitProtect);
        gSettings.displayBlitProtectReserved = false;
    }

    totalTime = imageThreadParams->endTime - imageThreadParams->startTime;
    totalPixels = imageThreadParams->imgDesc.width * imageThreadParams->imgDesc.height;
    imageThreadParams->mpps = ((((double)totalPixels)/(((double)totalTime/(double)gArguments.numDecodesToAverage)))/1000.0);
    ThreadSafeLog(imageThreadParams, "Size: %dx%d\n", 
            imageThreadParams->imgDesc.width, 
            imageThreadParams->imgDesc.height);

    ThreadSafeLog(imageThreadParams, "Total Time Elapsed: (%ums) \n", totalTime);
    ThreadSafeLog(imageThreadParams, "Average Rate for %d Decodes: %1.3fMpps (%ums) \n", 
            gArguments.numDecodesToAverage, 
            imageThreadParams->mpps,
            totalTime/gArguments.numDecodesToAverage);
}

void SleepForDisplay( ImageThreadParams *imageThreadParams )
{
    int32_t timeElapsed = 0;

    if (gArguments.millisecDelay > 0)
    {
        if (imageThreadParams->lastDisplayTime != -1)
            timeElapsed = (int)myclock() - (int)imageThreadParams->lastDisplayTime;
        if (gArguments.millisecDelay - timeElapsed > 0)
            mymsleep(gArguments.millisecDelay - timeElapsed);
    }
}


void ThreadSafePrintOnly(ImageThreadParams *imageThreadParams, char * fmt,...)
{
    char line[MAX_LARGE_STRING_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(line, fmt, ap);

    strcpy(imageThreadParams->screenOutputPos, imageThreadParams->indentString);
    imageThreadParams->screenOutputPos += strlen(imageThreadParams->indentString);

    strcpy(imageThreadParams->screenOutputPos, line);
    imageThreadParams->screenOutputPos += strlen(line);
    va_end(ap);
}

void Log(char * fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    if (gSettings.logfp != NULL)
    {
        vfprintf(gSettings.logfp, fmt, ap);
        fflush(gSettings.logfp);
    }
    va_end(ap);
}


void ThreadSafeLog(ImageThreadParams *imageThreadParams, char * fmt,...)
{
    char line[MAX_LARGE_STRING_LEN];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(line, fmt, ap);
    va_end(ap);

    if ((gArguments.numThreads == 1) && (gArguments.verboseOutput)) {
        STD_PRINT("%s%s", imageThreadParams->indentString, line);
    }
    else {
        strcpy(imageThreadParams->screenOutputPos, imageThreadParams->indentString);
        imageThreadParams->screenOutputPos += strlen(imageThreadParams->indentString);

        strcpy(imageThreadParams->screenOutputPos, line);
        imageThreadParams->screenOutputPos += strlen(line);
    }

    strcpy(imageThreadParams->logOutputPos, imageThreadParams->indentString);
    imageThreadParams->logOutputPos += strlen(imageThreadParams->indentString);

    strcpy(imageThreadParams->logOutputPos, line);
    imageThreadParams->logOutputPos += strlen(line);
}


void ThreadSafeTabIn( ImageThreadParams *imageThreadParams ) 
{
    imageThreadParams->logIndent++;
    sprintf(imageThreadParams->indentString, "%s%s", imageThreadParams->indentString, INDENT);
}

void ThreadSafeTabOut( ImageThreadParams *imageThreadParams )
{
    uint32_t i;

    if (imageThreadParams->logIndent > 0)
        imageThreadParams->logIndent--;

    imageThreadParams->indentString[0] = '\0';
    for(i =0; i != imageThreadParams->logIndent; i++)
        sprintf(imageThreadParams->indentString, "%s%s", imageThreadParams->indentString, INDENT);
}

void PrintThreadSafeLogToScreen( ImageThreadParams *imageThreadParams )
{
    if (!imageThreadParams->outputDisplayed)
        STD_PRINT("%s", imageThreadParams->screenOutput);
    imageThreadParams->outputDisplayed = true;
}

void WriteThreadSafeLogToFile(ImageThreadParams *imageThreadParams)
{
    if ((!imageThreadParams->logWritten) && (gSettings.logfp != NULL))
    {
        fprintf(gSettings.logfp, "%s", imageThreadParams->logOutput);
        fflush(gSettings.logfp);
    }
    imageThreadParams->logWritten = true;
}


void WaitForInput( ImageThreadParams *imageThreadParams )
{
    char input[MAX_LARGE_STRING_LEN];
    char * inputPointer;

    if ((gArguments.waitForInput) && (!imageThreadParams->userTriggeredContinueReceived))
    {
        ThreadSafePrintOnly(imageThreadParams, "Enter 'c' to continue, 'n' to switch to automated mode, 'q' to quit, or '.' to log a comment:\n");

        PrintThreadSafeLogToScreen(imageThreadParams);

        fflush(stdout);
        input[0] = (char) NULL;
        while (input[0] != 'c' && input[0] != 'q' && input[0] != '.' && input[0] != 'n'){
            inputPointer = fgets(input,sizeof(input),stdin);
            while (input[0] < ' ')
                inputPointer = fgets(input,sizeof(input),stdin);
        }

        if (input[0] == '.')
        {
            ThreadSafeLog(imageThreadParams, "COMMENT: %s", &input[1]);
        }
        else if (input[0] == 'n')
        {
            ThreadSafePrintOnly(imageThreadParams, "Switching to automated mode\n");
            gArguments.waitForInput = false;
        }
        else if (input[0] == 'q')
        {
            ThreadSafePrintOnly(imageThreadParams, "Quit request received\n");
            imageThreadParams->terminated = true;
        }

        imageThreadParams->userTriggeredContinueReceived = true;
    }
}



void PrintResults()
{
    uint32_t i, loc;
    char *fileTypeString = "FILE TYPE";
    int32_t maxStringLen = strlen(fileTypeString);
    int32_t alignmentNeeded = 0;

    Log("\n\n\n");

    for(i = 0; i < IMAGETYPE_NUM_TYPES; i++)
    {
        int32_t decoderIndex = 0;
        loc = 0;
        while ((i != validFileTypes[loc].imageType) &&
               (IMAGETYPE_NUM_TYPES != validFileTypes[loc].imageType))
            loc++;

        if (validFileTypes[loc].imageType >= IMAGETYPE_NUM_TYPES )
            continue;

        maxStringLen = strlen(validFileTypes[loc].userFriendlyString) > maxStringLen ?
                        strlen(validFileTypes[loc].userFriendlyString) : maxStringLen;


        while ((strcmp("", gSettings.results[loc].decoder[decoderIndex].decoderName) != 0) && 
               (decoderIndex < MAX_NUM_DECODERS))
        {
            if ((strlen(gSettings.results[loc].decoder[decoderIndex].decoderName) + 4) > maxStringLen)
                maxStringLen = strlen(gSettings.results[loc].decoder[decoderIndex].decoderName) + 4;
            decoderIndex++;
        }

    }

    maxStringLen+=5;

    Log("%s", fileTypeString);
    alignmentNeeded = maxStringLen - strlen(fileTypeString);
    for(; alignmentNeeded > 0; alignmentNeeded--)
        Log(" ");
    Log("   TESTED     ");
    Log("PASSED     ");
    Log("FAILED     ");
    Log("AVG DECODE RATE\n");

    for(i=0; i < 53+maxStringLen; i++)
        Log("-");
    Log("\n");

    for(i = 0; i < IMAGETYPE_NUM_TYPES; i++)
    {
        int32_t decoderIndex = 0;
        loc = 0;
        while ((i != validFileTypes[loc].imageType) &&
               (IMAGETYPE_NUM_TYPES != validFileTypes[loc].imageType))
            loc++;

        if (validFileTypes[loc].imageType >= IMAGETYPE_NUM_TYPES)
            continue;
        Log("%s", validFileTypes[loc].userFriendlyString);
        alignmentNeeded = maxStringLen - strlen(validFileTypes[loc].userFriendlyString);
        for(; alignmentNeeded > 0; alignmentNeeded--)
            Log(" ");
        Log("%6d     ", gSettings.results[i].tested);
        Log("%6d     ", gSettings.results[i].passed);
        Log("%6d         ", gSettings.results[i].tested - gSettings.results[i].passed);

        if (gSettings.results[i].averageDecodeRate != 0)
            Log("%1.3f(Mpps)     \n", gSettings.results[i].averageDecodeRate);
        else
            Log("    N/A         \n");


        while (strcmp("", gSettings.results[i].decoder[decoderIndex].decoderName) != 0)
        {
            Log(" -->%s", gSettings.results[i].decoder[decoderIndex].decoderName);
            alignmentNeeded = maxStringLen - strlen(gSettings.results[i].decoder[decoderIndex].decoderName) - 4;
            for(; alignmentNeeded > 0; alignmentNeeded--)
                Log(" ");
            Log("%6d     ", gSettings.results[i].decoder[decoderIndex].tested);
            Log("%6d     ", gSettings.results[i].decoder[decoderIndex].passed);
            Log("%6d         ", gSettings.results[i].decoder[decoderIndex].tested - gSettings.results[i].decoder[decoderIndex].passed);
    
            if (gSettings.results[i].decoder[decoderIndex].averageDecodeRate != 0)
                Log("%1.3f(Mpps)     \n", gSettings.results[i].decoder[decoderIndex].averageDecodeRate);
            else
                Log("    N/A         \n");


            decoderIndex++;
        }
    }

    Log("\n\n\n");
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   Helper Surface Functions  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

unsigned long MakeARGBfromYCrCb(unsigned char y, unsigned char cr, unsigned char cb, unsigned char t)
{
    unsigned long pixel32;
    int r, g, b;

    r = y + (int)(1.370705 * (cr-128));
    g = y - (int)((0.698001 * (cr-128)) - (0.337633 * (cb-128)));
    b = y + (int)(1.732446 * (cb-128));

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    pixel32 = ((b << 24) | (g << 16) | (r << 8) | (t<<0));
    return pixel32;
}

bool DumpSurfaceToPPM(char *filePath, IDirectFBSurface* surface, uint32_t scaleNumerator, uint32_t scaleDenominator)
{
    DFBResult err;
    bool error = false;
    FILE*  out_file = NULL;
    uint32_t *out_buffer = NULL;
    uint32_t out_buffer_size = 0;
    uint32_t stream_length = 0;
    const uint32_t MAX_PPM_HEADER_SIZE = 25;


    /* Find the buffer size needed */
    surface->CreatePPM(surface, scaleNumerator, scaleDenominator, NULL, 0, &out_buffer_size);

    /* Allocate the buffer */
    out_buffer = malloc(out_buffer_size);

    /* Create the PPM stream */
    err = surface->CreatePPM(surface, scaleNumerator, scaleDenominator, out_buffer, out_buffer_size, &stream_length);

    if (err == DFB_OK) {
        /* Write the PPM file */
        out_file = fopen(filePath, "wb");
        if (out_file) {
            fwrite ( out_buffer, 1, stream_length, out_file);
            fclose(out_file);
        }
        else {
            STD_PRINT("fopen falied!\n");
            error = true;
        }
    }
    else {
        STD_PRINT("CreatePPM falied!\n");
        error = true;
    }

    if (out_buffer) {
        free(out_buffer);
    }

    return error;
}


bool DumpSurfaceToCIF(char *filePath, IDirectFBSurface* surface, uint32_t scaleNumerator, uint32_t scaleDenominator)
{
    DFBResult err;
    bool error = false;
    FILE*  out_file = NULL;
    uint32_t *out_buffer = NULL;
    uint32_t out_buffer_size = 0;
    uint32_t stream_length = 0;
    const uint32_t MAX_PPM_HEADER_SIZE = 25;


    /* Find the buffer size needed */
    surface->CreateCIF(surface, scaleNumerator, scaleDenominator, NULL, 0, &out_buffer_size);

    /* Allocate the buffer */
    out_buffer = malloc(out_buffer_size);

    /* Create the PPM stream */
    err = surface->CreateCIF(surface, scaleNumerator, scaleDenominator, out_buffer, out_buffer_size, &stream_length);

    if (err == DFB_OK) {
        /* Write the PPM file */
        out_file = fopen(filePath, "wb");
        if (out_file) {
            fwrite ( out_buffer, 1, stream_length, out_file);
            fclose(out_file);
        }
        else {
            STD_PRINT("fopen falied!\n");
            error = true;
        }
    }
    else {
        STD_PRINT("CreatePPM falied!\n");
        error = true;
    }

    if (out_buffer) {
        free(out_buffer);
    }

    return error;
}

void PrintLineToScreen(ImageThreadParams *imageThreadParams, uint32_t lineNum, char * fmt,...)
{
    IDirectFBSurface *dstSurface = imageThreadParams->decodeSurface;
    int32_t startTop = 0;
    int32_t startLeft = 0;
    char line[MAX_LARGE_STRING_LEN];
    DFBRectangle destRect;
    uint32_t width;
    uint32_t height;

    if ((gSettings.displayNameOnScreen) && (gSettings.font))
    {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(line, fmt, ap);
        va_end(ap);

        dstSurface->GetSize(dstSurface, &destRect.w, &destRect.h);
        gSettings.font->GetStringAdvance(gSettings.font, line, -1, &width);
        height = gSettings.fontDesc.height;
        startTop =  (destRect.h * 5)/6;
        startLeft = (destRect.w * 1)/8;

        startTop += lineNum * gSettings.fontDesc.height;

        dstSurface->SetColor(dstSurface, 0x00, 0x00, 0x00, 0xFF);
        dstSurface->FillRectangle(dstSurface, startLeft, startTop, width, height);
        dstSurface->SetColor(dstSurface, 0xaa, 0xaa, 0xaa, 0xFF);
        dstSurface->DrawString(dstSurface, line, -1,
                                    startLeft, startTop, DSTF_LEFT | DSTF_TOP);
    }
}


void BlitSurface(IDirectFBSurface *srcSurface, ImageThreadParams *imageThreadParams)
{
    DFBRectangle srcRect, destRect;
    IDirectFBSurface *dstSurface = imageThreadParams->decodeSurface;
    int32_t startTop = 0;
    int32_t startLeft = 0;
    int32_t width;
    int32_t height;
    double heightRatio;
    double widthRatio;
    double scale;

    dstSurface->GetSize(dstSurface, &destRect.w, &destRect.h);
    srcSurface->GetSize(srcSurface, &srcRect.w, &srcRect.h);

    if (gArguments.scaleImage)
    {
        width = srcRect.w;
        height = srcRect.h;
        widthRatio = (destRect.w/(double)width);
        heightRatio = (destRect.h/(double)height);
        scale = heightRatio < widthRatio ? heightRatio : widthRatio;

        width*=scale;
        height*=scale;
        if (height < destRect.h)
            startTop = (destRect.h - height)/2;
        if (width < destRect.w)
            startLeft = (destRect.w - width)/2;

        DFBRectangle scaleRect={startLeft, startTop, width, height};
        dstSurface->StretchBlit( dstSurface, srcSurface, NULL, &scaleRect );
    }
    else
    { 
        if(srcRect.h < destRect.h)
            startTop =  (destRect.h - srcRect.h)/2;
        if (srcRect.w < destRect.w)
            startLeft = (destRect.w - srcRect.w)/2;

        dstSurface->Blit( dstSurface, srcSurface, NULL, startLeft, startTop);
        imageThreadParams->lastDisplayTime = myclock();
    }

    PrintLineToScreen(imageThreadParams, 0, "File: %s", imageThreadParams->fileName);
    PrintLineToScreen(imageThreadParams, 1, "Number: %d/%d", imageThreadParams->id, gSettings.numImages);
    PrintLineToScreen(imageThreadParams, 2, "Size: %dx%d", imageThreadParams->imgDesc.width,
                                                          imageThreadParams->imgDesc.height);
    if (gArguments.ppmSurfaceDump)
    {
        char outFilePath[MAX_SMALL_STRING_LEN];

        sprintf(outFilePath, "%s%03d.ppm", imageThreadParams->fileName, imageThreadParams->blittedVersions);
        ThreadSafePrintOnly(imageThreadParams, "Writing File: %s\n", outFilePath);
        if (DumpSurfaceToPPM(outFilePath, srcSurface, 1, 1))
            ThreadSafePrintOnly(imageThreadParams, "Failed to write file!\n");
    }

    if (gArguments.cifSurfaceDump)
    {
        char outFilePath[MAX_SMALL_STRING_LEN];

        sprintf(outFilePath, "%s%03d.cif", imageThreadParams->fileName, imageThreadParams->blittedVersions);
        ThreadSafePrintOnly(imageThreadParams, "Writing File: %s\n", outFilePath);
        if (DumpSurfaceToCIF(outFilePath, srcSurface, 1, 1))
            ThreadSafePrintOnly(imageThreadParams, "Failed to write file!\n");
    }

    imageThreadParams->blittedVersions++;

}





///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
//              *************   Random Helper Functions  ***************
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////




/*********************************************************************
  GetFileSize
  Returns the size of a file for Windows or Linux
*********************************************************************/

int GetFileSize(char* file_name)
{
    int file_size = 0;

#ifdef WIN32
    struct _stat stat_buf;
    int status = _stat ( file_name, &stat_buf );
    if ( status == 0 )
        file_size = stat_buf.st_size;
#else
    struct stat stat_buf;
    int status = stat ( file_name, &stat_buf );
    if (status == 0)
        file_size = stat_buf.st_size;
#endif

    return file_size;
}



/*********************************************************************
  LoadFromFile
  Takes a file path, opens it and returns the buffer
*********************************************************************/

void LoadFromFile(uint8_t **buf, uint32_t *bufferLength, char *file_path)
{
    FILE*  in_file = NULL;
    bool error = false;

    *buf = NULL;
    *bufferLength = GetFileSize(file_path);
    in_file = fopen(file_path, "rb");

    if (in_file) {
        *buf = bdvd_sid_alloc_input_buffer(*bufferLength + 4);
        if (fread(*buf, 1, *bufferLength, in_file) != *bufferLength) {
            STD_PRINT("Could not read entire file");
            error = false;
        }
        fclose(in_file);
    }
    else {
        STD_PRINT("Could not read input file\n");
        error = true;
    }

    if (error) {
        if (*buf)
            free(*buf);
        *buf = NULL;
    }
}







