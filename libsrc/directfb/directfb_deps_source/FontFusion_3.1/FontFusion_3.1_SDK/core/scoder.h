/*
 * Scoder.h
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

#ifndef __T2K_SCODER__
#define __T2K_SCODER__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/* private */
#define No_of_chars 256                 /* Number of character symbols      */

typedef struct {
	/* private */
	tsiMemObject	*mem;
	uint8			*numBitsUsed;
	uint32			numEntries, maxBits;
	uint8			*LookUpSymbol;	/* maps a bitpattern to a symbol */
	uint16			*LookUpBits;		/* maps a symbol the bitpattern  */
	/* public */
} SCODER;

/* Private methods. */
/*
 * Internal function used for sequencing the look-up table.
 */
void SCODER_SequenceLookUp( SCODER *t );

/* Public methods. */

/* Two different constructors. */
/* count is 256 entries large array with event counts for all the bytes. */
/* codingCost is an informative output from this constructor. */

#ifdef ENABLE_WRITE
SCODER *New_SCODER( tsiMemObject *mem, int32 *count, int32 *codingCost);

/*
 * This method saves an SCODER model to the stream, so that it can later be
 * recreated with New_SCODER_FromStream().
 */
void SCODER_Save( SCODER *t, OutputStream *out );

/*
 * Write a symbol to the output stream.
 */
int SCODER_EncodeSymbol( SCODER *t, OutputStream *out, uint8 symbol );

#endif /* ENABLE_WRITE */

/*
 * This standard constructor recreates the SCODER object from a stream.
 */
SCODER *New_SCODER_FromStream( tsiMemObject *mem, InputStream *in );


/*
 * Read a symbol from the input stream.
 */
uint8 SCODER_ReadSymbol( SCODER *t, InputStream *in );


/*
 * The destructor.
 */
void Delete_SCODER( SCODER *t );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_SCODER__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/scoder.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: scoder.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.4  2003/01/07 18:13:58  Shawn_Flynn
 *     dtypED.
 *     
 *     Revision 1.3  1999/09/30 15:11:37  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:33  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
