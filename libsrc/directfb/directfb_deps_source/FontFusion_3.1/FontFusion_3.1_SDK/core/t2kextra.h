/*
 * T2kextra.h
 * Font Fusion Copyright (c) 1989-2005 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
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


#ifndef __T2K_EXTRA__
#define __T2K_EXTRA__

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */
/* List of extra FontFusion capabilities that can be enabled or disabled by activating the commented out defines in this list */
/* #define ENABLE_T2K_TV_EFFECTS */

/* #define ENABLE_COLORBORDERS */

/* #define ENABLE_BOLDFILTER */

/* #define ENABLE_FLICKER_FILTER */

/* #define ENABLE_UNDERLINEFILTER */

/* #define ENABLE_MULTIPLE_FILTERS */

/* End of list */

#ifdef ENABLE_MULTIPLE_FILTERS 
#define T2K_NUM_MULTIPLE_FILTERS  6
#endif


#ifdef ENABLE_T2K_TV_EFFECTS

typedef struct {
	uint8 greyScaleLevel;	/* greyScaleLevel used */
	uint8 selector;			/* FCC style  : 0 none, 1 raised, 2 depressed, 3 uniform(outline), 4 drop-shadow-left, 5 drop-shadow-right */
	int32 dX, dY;			/* dX,dY is the thickhness of the border. (Should be 1 or 2 ) */
	uint32 Red;				/* Red, Green, Blue is the color of the character. */
	uint32 Green;			/* (All values should be between 0 and 255) */
	uint32 Blue;
	uint32 borderR, borderG, borderB;	/* borderR, borderG, borderB is the color of the border.  (All values should be between 0 and 255) */
	uint32 sunnyBorderR, sunnyBorderG, sunnyBorderB;	/* This is the sunny side of the border. Only used for raised and depressed styles  (All values should be between 0 and 255) */
} T2K_TVFilterParams;

/*
 * A plug in filter which implements the 6 FCC character edge effects
 *
 * This function can be invoked right after T2K_RenderGlyph().
 * You probably should use T2K_RenderGlyph with GrayScale and in T2K_TV_MODE_2 for best results.
 * (Alternatively you could also try native hints combined with GrayScale)
 *
 * t:       is a pointer to the T2K scaler object;
 * params:  is a void pointer that poins at T2K_TVFilterParams
 * myParams->selector : selects the FCC styles : 0 none, 1 raised, 2 depressed, 3 uniform(outline), 4 drop-shadow-left, 5 drop-shadow-right
 *
 */
void T2K_TV_Effects( T2K *t, void *params );

#endif /* ENABLE_T2K_TV_EFFECTS */


#ifdef ENABLE_COLORBORDERS
typedef struct {
	uint8 greyScaleLevel;	/* greyScaleLevel used */
	int32 dX, dY;			/* dX,dY is the thickhness of the border. (Should be 1 or 2 ) */
	uint32 Red;				/* Red, Green, Blue is the color of the character. */
	uint32 Green;			/* (All values should be between 0 and 255) */
	uint32 Blue;
	uint32 borderR, borderG, borderB;	/* borderR, borderG, borderB is the color of the border.  (All values should be between 0 and 255) */
} T2K_BorderFilerParams;

/*
 * Creates a bordered colored antialiased character.
 *
 * This function can be invoked right after T2K_RenderGlyph().
 * You probably should use T2K_RenderGlyph with GrayScale and in T2K_TV_MODE for best results.
 *
 * t:       is a pointer to the T2K scaler object;
 * params:  is a void pointer that poins at T2K_BorderFilerParams
 *
 */
void T2K_CreateBorderedCharacter( T2K *t, void *params );
#endif /* ENABLE_COLORBORDERS */

#ifdef ENABLE_BOLDFILTER
/*
 * Creates a bold character by overprinting the shapes.
 * It my be more optimal to not use this and instead use a draw_string
 * function that can do the same thing since this would eliminate the need
 * to cache the bold shapes.
 *
 * This function can be invoked right after T2K_RenderGlyph().
 *
 * t:       is a pointer to the T2K scaler object;
 * params:  is a void pointer that poins at T2K_BoldFilterParams
 *
 */
void T2K_CreateBoldCharacter( T2K *t, void *params );

typedef struct {
	uint8 greyScaleLevel;	/* greyScaleLevel used */
	int32 dX;				/* dX is the amout of pixels to add to the character shapes, should be 1 or higher */
} T2K_BoldFilterParams;		/* You may want to make dx = 1 at low size, but at larger sizes make it bigger than 1 */

#endif/* ENABLE_BOLDFILTER */


#ifdef ENABLE_LCD_OPTION

#define QUANT_VALUE 4
#define LEVEL_COUNT (QUANT_VALUE*3 +1)

/*
 * This trivial routine just drives the implicit gray-pixels
 * on RGB like displays, where the pixels alternate between
 * the different RGB colors. The gray-pixel this routine is
 * driving is 5 pixels wide (shape 1,2,3,2,1) and 1 tall.
 * This implements the Gibson LCD Option for T2K
 */
void T2K_WriteToGrayPixels( T2K *t );

#endif /* ENABLE_LCD_OPTION */

#ifdef ENABLE_EXTENDED_LCD_OPTION

void T2K_WriteToExtHorLCDPixels( T2K *t, uint16 cmd );
void T2K_WriteToExtVerLCDPixels( T2K *t, uint16 cmd );

#ifndef ENABLE_LCD_OPTION

#define QUANT_VALUE 4
#define LEVEL_COUNT (QUANT_VALUE*3 +1)

#endif /* ENABLE_LCD_OPTION */
#endif /* ENABLE_EXTENDED_LCD_OPTION */

#ifdef ENABLE_FLICKER_FILTER
void T2K_FlickerFilterExample( T2K *t, void *params );
#endif /* ENABLE_FLICKER_FILTER */


#ifdef ENABLE_UNDERLINEFILTER

/**
 * Creates a character with underline or strike-through. 
 *
 * This function can be invoked right after T2K_RenderGlyph().
 *
 * t:       is a pointer to the T2K scaler object;
 * params:  is a void pointer that points at T2K_UnderLineFilterParams
 */
void T2K_CreateUnderlineCharacter( T2K *t, void *params );

typedef struct {
	uint8 greyScaleLevel;    /* greyScaleLevel used */
	uint8 isUnderline;       /* non zero if underline is needed else strike through */
#ifdef ENABLE_EXTENDED_LCD_OPTION
	/* 0 for gray and monochrome else for LCD mode */
	uint16 mode;             /* If ext LCD modes are used then the modes used should be
							  * passed in this member variable */
#endif /* ENABLE_EXTENDED_LCD_OPTION */
} T2K_UnderLineFilterParams;

#endif/* ENABLE_UNDERLINEFILTER */


#ifdef ENABLE_MULTIPLE_FILTERS

typedef struct {
	/* The array of filter method pointers */
	FF_T2K_FilterFuncPtr activeFilters[ T2K_NUM_MULTIPLE_FILTERS ];
	/* The array of filter arguments */
	void *parameters[ T2K_NUM_MULTIPLE_FILTERS ];
	/* Current number of registered filters */
	int16 numOfFilters;
	/* Current filter tag */
	uint8 filterTag;
} T2K_MultipleFilterParams;

/**
 * Makes a character glyph to pass through all the registered filters to the multiple filter. 
 *
 * This function can be invoked right after T2K_RenderGlyph().
 *
 * t:      is a pointer to the T2K scaler object;
 * params: is a void pointer that points at T2K_MultipleFilterParams
 */
void T2K_MultipleFilter ( T2K *t, void *params );

/**
 * Initializes the the multiple filter class.
 *
 * filterParams: is a pointer that points at T2K_MultipleFilterParams.
 */
void T2K_MultipleFilter_Init ( T2K_MultipleFilterParams *filterParams );

/**
 * Adds a new filter to the multiple filter class.
 *
 * filterParams: is a pointer that points at T2K_MultipleFilterParams.
 * filterTag   : the filter that should be ORed with the current filter.
 * index       : the index at which the filter will be set. This value also decides the 
 *               the order at whih the filter will be applied.
 * filterToAdd : pointer to the filter method that has to be added.
 * params      : pointer to the arguments that has to be passed when this particular filter
 *               is applied.
 * errCode     : will be non-zero in case of exceptional conditions.
 *
 * The new filterTag is returned if the addition of the new filter is successful.
 * 
 */
uint8 T2K_MultipleFilter_Add ( T2K_MultipleFilterParams *filterParams, uint8 filterTag, uint8 index, FF_T2K_FilterFuncPtr filterToAdd, void *Params, int *errCode );

/**
 * Deletes the existing filter from the multiple filter class.
 *
 * filterParams: is a pointer that points at T2K_MultipleFilterParams.
 * filterTag   : the filter that should be ORed with the current filter.
 * index       : the index at which the filter will be deleted. 
 * errCode     : will be non-zero in case of exceptional conditions.
 *
 * The new filterTag is returned if the deletion of the filter is successful.
 * 
 */
uint8 T2K_MultipleFilter_Delete ( T2K_MultipleFilterParams *filterParams, uint8 filterTag, uint8 index, int *errCode );

#endif/* ENABLE_MULTIPLE_FILTERS */

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_EXTRA__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/t2kextra.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: t2kextra.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.12  2005/08/22 15:17:47  Shawn_Flynn
 *     Modified to add support for the Multiple  bitmap Filter functionality.
 *     
 *     Revision 1.11  2005/07/18 19:38:52  Shawn_Flynn
 *     Initial version of underline/strike-thru filter function.
 *     
 *     Revision 1.10  2005/04/21 17:35:36  Shawn_Flynn
 *     Modified to support the new ENABLE_EXTENDED_LCD_OPTION mode.
 *
 *     Revision 1.9  2003/04/30 19:30:19  Shawn_Flynn
 *     Changed elements R,G and B to Red, Green and Blue to avoid compiler
 *     warnings.
 *
 *     Revision 1.8  2003/01/14 19:41:13  Shawn_Flynn
 *     Add data structures and prototype statement for a bitmap bold-smear
 *     filter function.
 *
 *     Revision 1.7  2002/03/06 21:26:46  Reggers
 *     New filter functions for FCC compliance
 *     Revision 1.6  2000/06/14 21:31:25  reggers
 *     Removed extraneous LCD_OPTION setting.
 *     Revision 1.5  2000/03/27 22:17:15  reggers
 *     Updates for new LCD mode and functionality
 *     Revision 1.4  1999/10/19 16:17:14  shawn
 *     Added a manifest for a Flicker Filter.
 *
 *     Revision 1.3  1999/09/30 15:12:04  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:16  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
