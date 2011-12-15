/*
 * Dtypes.h
 * Font Fusion Copyright (c) 1989-2002 all rights reserved by Bitstream Inc.
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

#ifndef __T2K_DTYPES__
#define __T2K_DTYPES__

/* 32 bits: These types should always be 32 bits wide. These definitions should work 
 * correctly on 32 bit systems. On 64 Bit systems or some platforms this may have to 
 * be changed so as to have 32 bits wide length only.
 * (LL)P64: Windows. No change may be needed. 
 * LP64   : Sun, SGI, Unix, Mac OS X - Tiger. Here these should be changed to int and 
 *          unsigned int respectively 
 */
typedef long int32;
typedef unsigned long uint32;

/* 16 bits */
typedef short int16;
typedef unsigned short uint16;

/* 8 bits */
typedef signed char int8;
typedef unsigned char uint8;

/* Special: 32 bits */
typedef int32 F26Dot6;
typedef int32 F16Dot16;

/* This class should be of same length as the address length of the base platform. This value is valid 
 * for 32 bit systems. Change may be needed for other platforms.
 * (LL)P64: Windows. Here the typedef should be changed to unsigned long long. 
 * LP64   : Sun, SGI, Unix, Mac OS X - Tiger.  No change may be needed.
 */
typedef unsigned long int uintptr_w;

typedef long Fract;
typedef long Fixed;


#define ONE16Dot16 0x10000


#ifndef false
#define false 0
#endif


#ifndef true
#define true 1
#endif


typedef void *(*FF_GetCacheMemoryPtr)( void *theCache, uint32 length );

#ifdef ENABLE_COMMON_DEFGLYPH
typedef void *(*FF_SearchDefGlyph)(void *theCache);
#endif/*ENABLE_COMMON_DEFGLYPH*/



#endif /* _T2K_DTYPES__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/dtypes.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: dtypes.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.8  2005/09/26 18:21:45  Shawn_Flynn
 *     The following typedefs were changed:
 *          F26Dot6  - int32
 *          F16Dot16 - int32
 *     The following typedef was added for 64 bit pointer support:
 *          uintptr_w - unsigned long int
 *     
 *     Revision 1.7  2005/09/15 19:28:20  Shawn_Flynn
 *     Added typedef for default glyph search routine.
 *     
 *     Revision 1.6  2002/12/16 19:53:41  Shawn_Flynn
 *     Cleaned up for dtypED process.
 *     
 *     Revision 1.5  2002/03/06 21:29:38  Reggers
 *     Updated copyright header
 *     Revision 1.4  2002/03/06 17:56:20  Reggers
 *     Made all atomic data types typedefs instead of #defines
 *     Revision 1.3  1999/09/30 15:11:09  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:56:38  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
