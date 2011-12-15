/*
 * bdf.h
 * Font Fusion Copyright (c) 1989-2005 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Ritesh Singh
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */


#ifndef __BDF_HEADER_
#define __BDF_HEADER_
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#include "config.h"

typedef struct {
	uint16 uniCode;
	uint32 offset;
} bdfUnicodeOffset;

typedef struct {
int32 vVectorx , vVectory;
} bdfGlobalVector;

typedef struct {
	int32 sWidth0_1, sWidth0_2;
	int32 dWidth0_1, dWidth0_2;
	int32 sWidth1_1, sWidth1_2;
	int32 dWidth1_1, dWidth1_2;	
	bdfGlobalVector *vVector;
} bdfMetricsWidths;

typedef struct  {
	tsiMemObject	*mem; /*sfntClass->mem*/ 
	InputStream		*in;

	uint32 copyrightNotice;/* COPYRIGHT **/
	uint32 fontFamilyName; /* FAMILY_NAME **/
	uint32 fontSubfamilyName; /* WEIGHT_NAME **/
	uint32 uniqueFontIdentifier; /* UNIQUE_FONT_ID **/
	uint32 fullFontName; /* FACE_NAME **/
	uint32 fontVersionString; /* VERSION **/
	uint32 fontPostscriptName; /* FACE_NAME **/
	uint32 fontTradeMark; /* VENDOR **/

	int8 *linePtr;
	uint32 length;
	int32 metricsSet;
	bdfMetricsWidths *sdwidths;
	uint16 pixelSize;
	uint32 bitsAtPtSize;/* Tells that, for what pt size bitmaps are actually available */
	uint16 firstCharCode;
	uint16 lastCharCode;
	int32 fontNum;
	uint32 numGlyf; /* Total number of glyph */
	int32 pointSize;
	int32 newPointSize;
	int32 xres, yres, bits;/*here bits is used to store the bitdepth 
	                        *i.e if nothing is there than mono .if something 
	                        *is there than 32 ..currently i am coding with this 
	                        *assumption*/ 

	int32 xmax, ymax, xmin, ymin;/* fontbounding box */
	int32 ascent, descent;
	
	int32 bbx;
	int32 encoding ;/*Keep it as signed as it can be -1 when no proper encoding is mentioned in the font*/
	int32 defCharEncoding; /*Stores the number which tells us that what will be the encoding type of default char
	                        *But we can surely remove this variable as we are assuming the default variable as the first glyph*/
	int32 underLinePos;
	int32 underLineThickness;
	bdfUnicodeOffset *ptr;
    int32 newPpemX  ;
	int32 newPpemY ;/* Use bitmap of this size instead with scaling, if different from ppemX and ppemY. */
	uint8 requestedBitDepth; /* For storing the user request */	
	
} bdfClass;

bdfClass *tsi_NewBDFClass( tsiMemObject *mem, InputStream *in, int32 fontNum );

int tsi_BDFGetSbits ( void *p, int32 aCode, uint8 greyScaleLevel, uint16 cmd
#ifdef ENABLE_SBITS_TRANSFORM
				  , int8 xFracPenDelta, int8 yFracPenDelta
#endif /*ENABLE_SBITS_TRANSFORM*/
				  );

uint8 *GetBdfNameProperty( bdfClass *bdfPtr, uint16 languageID, uint16 nameID );

uint16 tsi_BDFGetGlyphIndex ( bdfClass *bdfPtr, uint16 charCode );

void Bdf_GetFontWideMetrics( void *t, T2K_FontWideMetrics *hori, T2K_FontWideMetrics *vert );

void tsi_DeleteBDFClass( bdfClass *t );

uint16 FF_GetAW_BDFClass ( void *param1, register uint16 index );

void tsi_BDFGetGlyphByIndex ( bdfClass *bdfPtr, uint16 index, uint16 *aWidth, uint16 *aHeight );



#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* ENABLE_BDF */



/*********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                            *
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/bdf.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                            *
 *     $Log: bdf.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.1  2005/10/06 20:08:14  Shawn_Flynn
 *     Initial revision
 *                                                                            *
 ******************************************************************************/

