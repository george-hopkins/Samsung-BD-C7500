/*
 * strkconv.h
 * Font Fusion Copyright (c) 2000 all rights reserved by Bitstream Inc.
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

#ifndef __FF_STRKCONV__
#define __FF_STRKCONV__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef ENABLE_STRKCONV

typedef struct
{
	/* private */

	/* public */	
	uint8	*baseAddr;
	int		internal_baseAddr;
	int32	left, right, top, bottom;
	F26Dot6	fTop26Dot6, fLeft26Dot6;
	int32	rowBytes;
	
	/* private */
	tsiMemObject *mem;

	int16	*startPoint;
	int16	*endPoint;
	int16	numberOfContours;

	int32	*x;
	int32	*y;
	int8	*onCurve;
	
	int32	xmin, xmax;
	int32	ymin, ymax;
} ffStrkConv;


/*
 * The stroke-converter constructor
 */
ffStrkConv *ff_NewStrkConv( tsiMemObject *mem, int16 numberOfContours,
							int16 *startPtr, int16 *endPtr,
							int32 *xPtr, int32 *yPtr,
							int8 *onCurvePtr );
/*
 * This uses approximations to gain SPEED at low sizes, with the idea that the approximations will not be visible at low sizes.
 * Useful sizes may be sizes up to about 36 ppem.
 *
 */
void MakeStrkBits( ffStrkConv *t, int8 omitBitMap, FF_GetCacheMemoryPtr funcptr,
				   void *theCache, int bitRange255, uint8 *remapBits,
				   int32 xRadius,  int32 yRadius  );

/*
 * The stroke-converter destructor
 */
void ff_DeleteStrkConv( ffStrkConv *t );

#endif /*  ENABLE_STRKCONV */


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __FF_STRKCONV__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 * 
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/strkconv.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *
 *     $Log: strkconv.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.2  2003/02/26 20:31:35  Shawn_Flynn
 *     dtypED.
 *     
 *
 *
******************************************************************************/
