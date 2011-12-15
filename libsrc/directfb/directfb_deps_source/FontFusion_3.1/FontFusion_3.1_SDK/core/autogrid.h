/*
 * Autogrid.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

 


#ifndef ag_ROMAN
typedef int ag_FontCategory;
#define ag_ROMAN 1
#define ag_KANJI 2
#define ag_ROMAN_MONOSPACED 3
#endif


#ifndef __T2K_AUTOGRID__
#define __T2K_AUTOGRID__

#define CMD_AUTOGRID_ALL 		0
#define CMD_AUTOHINT 			1
#define CMD_FINDSTEMS			2
#define CMD_AUTOGRID_YHEIGHTS	3

#ifdef ENABLE_AUTO_GRIDDING_CORE

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


/***********************************************/
/** BEGIN EXTERNAL API data types and defines **/
#define IN
#define OUT
#define INOUT

/***********   CVT    *****************/
/****  Double heights (round+flat) ****/

/*
 * These only have the natural conceptual meaning
 * for the (fontType == ag_ROMAN) case
 */
#define ag_ASCENDER_HEIGHT		0
#define ag_CAP_HEIGHT			1
#define ag_FIGURE_HEIGHT		2
#define ag_X_HEIGHT				3
#define ag_UC_BASE_HEIGHT		4
#define ag_LC_BASE_HEIGHT		5
#define ag_FIGURE_BASE_HEIGHT	6
#define ag_DESCENDER_HEIGHT		7
#define ag_PARENTHESES_TOP		8
#define ag_PARENTHESES_BOTTOM	9
#define ag_MAX_HEIGHTS_IN		10


typedef struct {
	int16	flat;		/* set to actual flat height */
	int16	round;		/* set to actual round height */
	int16	overLap;	/* set to round - flat. This means that this value is normally */
						/* positive for heights at the top of characters, but negative */
						/* at the bottom of characters. */
						/* If the absolute of the overlaps on top and bottom are almost identical it */
						/* is a VERY good idea making their absolute values exactly identical. */
} ag_HeightType;



#define ag_MAXWEIGHTS		12

typedef struct {
	/* PUBLIC, INPUT: set by the caller */
	/* These are common heights */
	ag_HeightType	heights[ag_MAX_HEIGHTS_IN]; /* set unused heights to zero */
	/* These are common weights, at least xWeight[0], and yWeight[0] have to be set */
	int16 xWeight[ag_MAXWEIGHTS]; /* xWeight[0] is the ancestral x-weight, set unused weights to zero */
	int16 yWeight[ag_MAXWEIGHTS]; /* yWeight[0] is the ancestral y-weight, set unused weights to zero */
} ag_GlobalDataType;

typedef struct {
	uint16 maxZones;
	uint16 maxTwilightPoints;
	uint16 maxStorage;
	uint16 maxFunctionDefs;
	uint16 maxInstructionDefs;
	uint16 maxStackElements;
	uint16 maxSizeOfInstructions;
} ag_HintMaxInfoType;


typedef struct {
	/* PUBLIC, INPUT: set by the caller and allocated by the caller */
	int16	contourCount; /* number of contours in the character */
	int16	pointCount;   /* number of points in the characters + zero for the sidebearing points */
	int16	*sp;  		/* sp[contourCount] Start points */
	int16	*ep;  		/* ep[contourCount] End points */
	int16	*oox;		/* oox[pointCount] Unscaled Unhinted Points, add two extra points for lsb, and rsb */
	int16	*ooy;		/* ooy[pointCount] Unscaled Unhinted Points, set y to zero for the two extra points */
						/* Do NOT include the two extra points in sp[], ep[], contourCount */
						/* Do include the two extra points in pointCount */
	uint8 *onCurve;		/* onCurve[pointCount] indicates if a point is on or off the curve, it should be true or false */
	/* PUBLIC, OUTPUT: not set by caller. However this is read and allocated by the caller */
	int32	*x;			/* x[pointCount]Scaled auto-gridded Points, in 26.6 bit format */
	int32	*y;			/* y[pointCount]Scaled auto-gridded Points, in 26.6 bit format  */
	int32	advanceWidth26Dot6;
	int32	advanceWidthInt;
} ag_ElementType;

typedef void *ag_HintHandleType;

/*** END EXTERNAL API data types and defines ***/
/***********************************************/



/***********************************************/
/*** BEGIN EXTERNAL API function entry points **/

/*
 * Description:		Call this first
 * 					This returns the hintHandle which is passed to all the other external calls.
 * 					In a multi-threaded environment each thread needs to have it's own hintHandle.
 * How used:		Call with the maximum point count for the font, the units per Em,
 *					and a pointer (hintHandle) to ag_HintHandleType.
 * Side Effects: 	Sets *hintHandle.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_HintInit( IN tsiMemObject *mem, IN int maxPointCount, IN int16 unitsPerEm, OUT ag_HintHandleType *hintHandle );

/*
 * Description:		Call this to set global hint information.
 * 					Always, set at least one x and one y weight.
 * 					Set any unused heights and weights to zero.
 * 					Call this once, before calling ag_AutoGridOutline(), or ag_AutoHintOutline()
 * How used:		Call with the hintHandle, a pointer to ag_GlobalDataType, and the font type.
 * Side Effects: 	Stores the passed in global hint information and the fontType within the private ag_DataType data structure.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_SetHintInfo( IN ag_HintHandleType hintHandle, IN ag_GlobalDataType *gData, ag_FontCategory fontType );

/*
 * Description:		Always call this when the size changes, but not otherwise
 * 					A call to AutoGridOutline() is invalid without any previous call to SetScale()
 * 					However if the scale stays constant then multiple calls to AutoGridOutline(), or AutoHintOutline()
 * 					is OK as long as SetScale() was called once with the correct scale first.
 * 					For speed reasons do not call SetScale() more often than you have to.
 * How used:		Call with the hintHandle, and the x and y pixels per Em.
 * Side Effects: 	Remembers the x and y pixels per Em and sets up the internal cvt information.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 *					*xWeightIsOne = xWeight == onePix
 */
extern int ag_SetScale( IN ag_HintHandleType hintHandle, IN int32 xPixelsPerEm, IN int32 yPixelsPerEm, int8 *xWeightIsOne );

#ifdef ENABLE_AUTO_HINTING
/*
 * Description:		The caller should copy the 3 tables directly into the 3 corresponding TrueType tables
 * 					This function is only called when hints are desired. Not when we are just auto-gridding.
 * 					The caller HAS to deallocate all three memory pointers.
 * How used:		Call with the hintHandle, and three pairs of ((char **) and (long*)) for the font program,
 *					pre program and control value table.
 * Side Effects: 	None.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_GetGlobalHints(IN ag_HintHandleType hintHandle,
							 OUT uint8 **fpgm, OUT int32 *fpgmLength,
							 OUT uint8 **ppgm, OUT int32 *ppgmLength,
							 OUT int16 **cvt,  OUT int32 *cvtCount );

#endif /* ENABLE_AUTO_HINTING */


#ifdef ENABLE_AUTO_GRIDDING_CORE

#define CMD_AUTOGRID_YHEIGHTS_DISABLED -100
/*
 * Description:		This function auto grids the glyph stored within ag_ElementType.
 * How used:		Call with the hintHandle, a pointer to ag_ElementType,
 *					the isFigure boolean (isFigure should be set to true for figures 0..9, and false otherwise),
 *					the curve type (set curveType to 2 for TT and 3 for T1 )
 * Side Effects: 	ag_DataType and ag_ElementType.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_AutoGridOutline( IN ag_HintHandleType hintHandle, INOUT ag_ElementType *elem, IN int16 cmd, IN int16 isFigure, IN int16 curveType, IN int16 grayScale, IN int16 numSBPoints );
#endif /* ENABLE_AUTO_GRIDDING_CORE */

#ifdef ENABLE_AUTO_HINTING
/*
 * Description:		This function auto hints the glyph stored within ag_ElementType.
 * 					The caller HAS to deallocate *hintFragment
 *					This can be called instead of AutoGridOutline() when the client needs
 *					the TT hints instead of the auto-gridded outline.
 * How used:		Call with the hintHandle, a pointer to ag_ElementType,
 *					the isFigure boolean (isFigure should be set to true for figures 0..9, and false otherwise),
 *					the curve type (set curveType to 2 for TT and 3 for T1 ),
 *					and char **hintFragment, and long *hintLength for the TrueType glyph hint data.
 * Side Effects: 	ag_DataType, hintFragment, hintLength.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_AutoHintOutline( IN ag_HintHandleType hintHandle, IN ag_ElementType *elem, IN int16 isFigure, IN int16 curveType, OUT uint8 *hintFragment[], OUT int32 *hintLength );
#endif /* ENABLE_AUTO_HINTING */				   		

/*
 * Description:		This function can be used, for analyzing strokes for the purpose of
 * 					finding global stem-weights. It sets the content of the xDist[], and yDist[] arrays
 * 					It also sets *xDistCount, and *yDistCount to the number of links found in the x and y directions.
 * 					The caller HAS to deallocate *xDist and *yDist
 * How used:		Call with the hintHandle, a pointer to ag_ElementType,
 *					the isFigure boolean (isFigure should be set to true for figures 0..9, and false otherwise),
 *					the curve type (set curveType to 2 for TT and 3 for T1 )
 *					and two pairs of (short **, and long *) for the x and y stem-weight data.
 * Side Effects: 	xDist[], and yDist[], and *xDistCount, *yDistCount.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_AutoFindStems( IN ag_HintHandleType hintHandle, IN ag_ElementType *elem, IN int16 isFigure, IN int16 curveType, OUT int16 *xDist[], OUT int32 *xDistCount, OUT int16 *yDist[], OUT int32 *yDistCount ); 

/*
 * Description:		Call this to get the data for the ag_HintMaxInfoType structure
 * 					This should be called after ALL the glyphs have been hinted.
 * How used:		Call with the hintHandle and pointer to a ag_HintMaxInfoType
 *					data structure.
 * Side Effects: 	Sets the contents of the ag_HintMaxInfoType data structure.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_GetHintMaxInfo( IN ag_HintHandleType hintHandle, OUT ag_HintMaxInfoType *p );

/*
 * Description:		Call this to cause the auto-grider to de-allocate it's memory
 * 					HintInit() and HintEnd() form a matching pair
 * How used:		Call with the hintHandle.
 * Side Effects: 	De-allocates internal memory within the ag_DataType data-structure.
 * Return value: 	Returns < 0 if an error was encountered and 0 otherwise.
 */
extern int ag_HintEnd( IN ag_HintHandleType hintHandle );

/**** END EXTERNAL API function entry points ***/
/***********************************************/


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif	/* ENABLE_AUTO_GRIDDING_CORE */

#endif /*  __T2K_AUTOGRID__*/


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/autogrid.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: autogrid.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.6  2003/01/03 18:04:11  Shawn_Flynn
 *     dtypED
 *     
 *     Revision 1.5  2000/05/09 20:32:49  reggers
 *     Changed a comment for accuracy.
 *     Revision 1.4  1999/09/30 15:11:01  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.3  1999/07/16 17:51:52  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.2  1999/05/17 15:56:13  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

