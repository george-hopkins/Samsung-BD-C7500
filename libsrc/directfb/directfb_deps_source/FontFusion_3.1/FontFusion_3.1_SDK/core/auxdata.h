/*
 * Auxdata.h
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



/*************************** A U X D A T A . H *******************************
 *                                                                           *
 *  AuxData.h - Define standard auxilliary data stored by High Level         *
 *  DLL interface.  This data must be sufficient to recreate LOGFONT         *
 *  and TEXTMETRICS data structures for Windows Apps.                        *
 *                                                                           *
 *  For flexibility, specific data is stored in a chain of blocks, so        *
 *  that new blocks can be added.  The chain format is as follows:           *
 *                                                                           *
 *      Offset	Size 	Contents                                             *
 *      ------	----	--------                                             *
 *         0	  2		len - Block lenght in bytes (always rounded to even) *
 *         2	  2		BlockID defines block contents                       *
 *         4	 len	                                                     *
 *       4+len	  2	 	repeat for subsequent blocks                         *
 *                                                                           *
 *   Chain is terminated by a block with length and ID of 0                  *
 *                                                                           *
 ****************************************************************************/

#ifndef auxdata_h
#define auxdata_h

typedef struct blockHead_tag
{
	uint8 len[2];
	uint8 blockID[2];
	} blockHead_t;

/*  Block IDS */

#define IDWINFACENAME	1	/* Windows FaceName */
#define IDWINTYPEATT	2	/* Typographic attributes and metrics used by windows */
#define IDWINSTYLE		3	/* Windows style name */
#define IDPOSTNAME		4	/* PostScript name */
#define IDADDTYPEATT	5	/* Additional type attributes */
#define IDFULLFONTNAME	6	/* Full font name */
#define IDPFMTYPEATT	7	/* PFM type attributes */
#define IDPAIRKERNDATA	8	/* Pair kerning data */
#define IDTRACKKERNDATA	9	/* Track kerning data */


/* User defined data tags - 0x8000 to 0xffff */

#define IDUSERDATA	0x8000

/*  Block ID = 2 ;  Windows attributes and metrics */

#define CS_WINDOWSANSI		0
#define CS_WINDOWSCUSTOM	1
#define CS_MACANSI			2
#define CS_MACCUSTOM		3
#define CS_UNICODE			4
#define CS_CUSTOM_TRANSLATE	5

typedef struct auxTypeAtt_tag
{
	uint8	panose[10];
	uint8	ascent[2];
	uint8	descent[2];
	uint8	height[2];
	uint8	internalLeading[2];
	uint8	externalLeading[2];
	uint8	aveCharWidth[2];
	uint8	maxCharWidth[2];
	uint8	weight[2];
	int8	italic;
	int8	charSet;
	int8	pitchAndFamily;
	int8	breakChar;
	uint8	overhang[2];
} auxTypeAtt_t;

/*  Block ID = 5 ;  Additional type attributes and metrics */

typedef struct auxAddTypeAtt_tag
{
	uint8	underlinePos[2];
	uint8	underlineThickness[2];
	uint8	strikethroughPos[2];
	uint8	strikethroughThickness[2];
	uint8	subscriptXSize[2];
	uint8	subscriptYSize[2];
	uint8	subscriptXPos[2];
	uint8	subscriptYPos[2];
	uint8	superscriptXSize[2];
	uint8	superscriptYSize[2];
	uint8	superscriptXPos[2];
	uint8	superscriptYPos[2];
} auxAddTypeAtt_t;

/*  Block ID = 7 ;  PFM attributes and metrics */

typedef struct auxPfmTypeAtt_tag
{
	uint8	dfAscent[2];
	uint8	dfMaxWidth[2];
	uint8	dfWeight[2];
	uint8	etmSlant[2];
	uint8	etmLowerCaseDescent[2];
	uint8	lineGap[2];
	uint8	sTypoAscender[2];
	uint8	sTypoDescender[2];
	uint8	dfItalic;
	uint8	fixedPitch;
} auxPfmTypeAtt_t;

/*  Block ID = 8 ;  Pair kerning data */

typedef struct auxPairKernHeader_tag
{
	uint8	nKernPairs[2];
	uint8	baseAdjustment[2];
	uint8	flags;
} auxPairKernHeader_t;


/* Bit assignment for pair kerning header flags */

#define TWO_BYTE_KERN_PAIR_CHAR_CODES 1
#define TWO_BYTE_KERN_PAIR_ADJ_VALUES 2

/*  Block ID = 9 ;  Track kerning data */
typedef struct auxTrackKernHeader_tag
{
	uint8	nKernTracks[2];
} auxTrackKernHeader_t;


/*  Data access macros */

#define PUTAUXINT(chararray,ival) chararray[0] = (uint8)(ival >> 8);chararray[1] = (uint8)(ival & 0xFF);

#define GETAUXINT(chararray) (((uint16)chararray[0] << 8) + (uint16)chararray[1])


#endif /* #ifndef auxdata_h */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/auxdata.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: auxdata.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.4  2003/01/06 17:49:47  Shawn_Flynn
 *     dtypED.
 *     
 *     Revision 1.3  1999/09/30 15:11:06  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:59:25  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
