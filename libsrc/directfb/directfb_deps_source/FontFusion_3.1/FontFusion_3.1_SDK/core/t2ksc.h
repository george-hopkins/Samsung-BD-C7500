/*
 * T2ksc.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
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

#ifndef __T2K_SC__
#define __T2K_SC__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#define IS_POS_T2K_EDGE		0x01


typedef struct
{
	int32	coordinate25Dot6_flag1;
	void	*next;
} T2KInterSectType;


#define T2K_SC_BASE_BUFFER_SIZE				32 /* 32 */
#define T2K_SC_FREE_BLOCK_BUFFER_SIZE		32 /* 32 */
#define T2K_SC_FIRST_MEM_BLOCK_BUFFER_SIZE	256*4 /* 256 */

typedef struct
{
	/* private */
#ifdef ENABLE_MORE_TT_COMPATIBILITY
	int32		prevX0, prevY0;
#endif
	/* public */	

	int32		left, right, top, bottom;
	F26Dot6		fTop26Dot6, fLeft26Dot6;
	int32		rowBytes;
	uint8		*baseAddr;
	int			internal_baseAddr;

	/* private */
	int			scanBBoxIsComputed;

	int32		xminSC, xmaxSC;
	int32		yminSC, ymaxSC;

	int32		outlineXMin, outlineXMax;
	int32		outlineYMin, outlineYMax;

	int			couldOverflowIntegerMath;

	T2KInterSectType *yBaseBuffer[T2K_SC_BASE_BUFFER_SIZE];
	T2KInterSectType **yEdgeHead, **yBase;	/* T2KInterSectType *yEdgeHead[], The normal y-scan-line info */

	int32		minYIndex, maxYIndex;							/* max and min index into yEdgeHead[] */

	T2KInterSectType *xBaseBuffer[T2K_SC_BASE_BUFFER_SIZE];
	T2KInterSectType **xEdgeHead, **xBase;	/* Same for x */

	int32		minXIndex, maxXIndex;

	T2KInterSectType *free;			/* first free */
	T2KInterSectType *freeEnd;		/* One past the last legal position */

	T2KInterSectType firstMemBlockBuffer[T2K_SC_FIRST_MEM_BLOCK_BUFFER_SIZE];
	T2KInterSectType *freeMemBlocksBuffer[ T2K_SC_FREE_BLOCK_BUFFER_SIZE ];
	T2KInterSectType **freeMemBlocks; /* *freeMemBlocks[ freeMemBlockMaxCount ] */

	int32		freeMemBlockMaxCount;
	int32		freeMemBlockN;

	int32		maxError;
	uint8		greyScaleLevel;
	int8		xDropOutControl;
	int8		yDropOutControl;
	int8		includeStubs; /* best quality setting is false */
	int8		smartDropout; /* best quality setting is true  */
	int8		doXEdges;

	int			weDidXDropouts;
	int			weDidYDropouts;

	int16		*startPoint;
	int16		*endPoint;
	int16		numberOfContours;

	int32		*x;
	int32		*y;
	int8		*onCurve;

	tsiMemObject	*mem;
} tsiScanConv;

tsiScanConv *tsi_NewScanConv(tsiMemObject *mem, int16 numberOfContours, int16 *startPtr, int16 *endPtr,
							 int32 *xPtr, int32 *yPtr, int8 *onCurvePtr, uint8 greyScaleLevel,
							 int8 curveType, int8 xDropOutControl, int8 yDropOutControl,
							 int smart_droput, int include_stubs, F26Dot6 oneHalfFUnit,
							 int force_size);

void MakeBits(tsiScanConv *t, int8 xWeightIsOne, int8 omitBitMap, FF_GetCacheMemoryPtr funcptr,
			  void *theCache, int bitRange255, uint8 *remapBits, int extraColumn  );

void tsi_DeleteScanConv( tsiScanConv *t );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_SC__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/t2ksc.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: t2ksc.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.9  2005/05/19 15:17:57  Shawn_Flynn
 *     Modified for BtX2 support.
 *     
 *     Revision 1.8  2003/01/07 20:47:48  Shawn_Flynn
 *     dtypED.
 *     
 *     Revision 1.7  2001/11/13 20:41:21  reggers
 *     Fixes for LCD mode for wireless fonts.
 *     Revision 1.6  1999/12/09 22:06:58  reggers
 *     Sampo: multiple TrueType compatibility enhancements (scan converter)
 *     Revision 1.4  1999/09/30 15:12:16  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.3  1999/08/27 20:08:41  reggers
 *     Latest changes from Sampo
 *     Revision 1.2  1999/05/17 15:58:27  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
