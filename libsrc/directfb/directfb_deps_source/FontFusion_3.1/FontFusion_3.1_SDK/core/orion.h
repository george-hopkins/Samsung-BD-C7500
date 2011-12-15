/*
 * Orion.h
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

#ifndef __T2K_ORION__
#define __T2K_ORION__
#include "scoder.h"
#include "truetype.h"
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/* ----- ----- ----- ----- ----- */
#define ORION_MAJOR_VERSION 1
#define ORION_MINOR_VERSION 0

#define NUM_ECOPYX (8)
#define NUM_EX (NUM_ECOPYX+8+1)

#define ORION_STATE_0 0

typedef struct
{
	/* private */
	tsiMemObject *mem;
	
	/* public */
	int		OrionState;
	int		num_eb1, num_e;
	int		num_eb2;	/* == num_e - num_eb1 - 1 */
	int16	*dx; 		/* [t->num_eb1 * 256] */
	int16	*dy; 		/* [t->num_eb1 * 256] */
	int8	*onCurve;	/* [t->num_eb1 * 256] */

	SCODER	**copy;		/* SCODER *copy[ t->num_eb1 ] */
	SCODER	**literal;	/* SCODER * literal[ t->num_e ] */
	SCODER	*control;
	SCODER	*ep;
} OrionModelClass;


#ifdef ENABLE_WRITE
OrionModelClass *New_OrionModelClass( sfntClass *font );
void Save_OrionModelClass( OrionModelClass *t, OutputStream *out );
int KnownVectorInOrionModel( OrionModelClass *t, int16 dx, int16 dy, int8 onCurve );
#endif
/*
 * The standard OrionModel constructor.
 */
OrionModelClass *New_OrionModelClassFromStream( tsiMemObject *mem, InputStream *in );

/*
 * Updates the orion state.
 */
void Set_OrionState( OrionModelClass *t, int dx, int dy, int8 onCurve );

/*
 * The Destructor.
 */
void Delete_OrionModelClass( OrionModelClass *t );


/* ----- ----- ----- ----- ----- */
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_ORION__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/orion.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: orion.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.5  2003/01/06 21:02:12  Shawn_Flynn
 *     dtypED.
 *     
 *     Revision 1.4  1999/10/18 17:00:13  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:11:29  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:10  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
