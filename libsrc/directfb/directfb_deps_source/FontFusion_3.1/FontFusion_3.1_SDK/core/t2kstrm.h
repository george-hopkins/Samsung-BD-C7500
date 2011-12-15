/*
 * T2kstrm.h
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

#ifndef __T2K_STREAM__
#define __T2K_STREAM__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


/* if compressed stream support is enabled,
 * ENABLE_NON_RAM_STREAM support must be disabled! (tohin)
 */
#ifdef COMPRESSED_INPUT_STREAM

#include "scodecdefs.h"

#endif /* COMPRESSED_INPUT_STREAM */


/*
 * This IO callback function needs to return 0 on success,
 * and a negative number should a failure occur. ( -1 recommended )
 * Positive numbers are reserved.
 */
#ifdef ENABLE_NON_RAM_STREAM
typedef int (*PF_READ_TO_RAM) ( void *id, uint8 *dest_ram, uint32 offset, int32 numBytes );
#endif

#ifdef JUST_AN_EXAMPLE_OF_PF_READ_TO_RAM
int ReadFileDataFunc( void *id, uint8 *dest_ram, uint32 offset, int32 numBytes )
{
	int error;
	size_t count;
	FILE *fp = (FILE *)id;

	assert( fp != NULL );
	/* A real version of this function should only for instance call fseek if there is a need */
	error	= fseek( fp, offset, SEEK_SET );
	if ( error )
		return -1; /*****/
	count = fread( dest_ram, sizeof( int8 ), numBytes, fp );
	return (ferror(fp) == 0 && count == (size_t)numBytes ) ? 0 : -1; /*****/
}
#endif


#ifdef ENABLE_NON_RAM_STREAM
/* #define USE_PRE_CACHING */
#define USE_PRE_CACHING
#endif /* ENABLE_NON_RAM_STREAM */


typedef struct {
	/* private */
	uint8 *privateBase;
#ifdef ENABLE_NON_RAM_STREAM
	PF_READ_TO_RAM 	ReadToRamFunc;
	void			*nonRamID;
	uint8			tmp_ch;
#ifdef USE_PRE_CACHING
#define PRE_CACHE_SIZE 512
	uint8 cacheBase[ PRE_CACHE_SIZE ];
	int32 bytesLeftToPreLoad;
#endif
	uint32 cacheCount;
	uint32 cachePosition; /* set to >= 0 when we set privateBase == cacheBase */
#endif

#ifdef COMPRESSED_INPUT_STREAM
	void*	cache;
#endif
	uint32 pos;
	uint32 maxPos; /* one past the last legal position */
	uint32 posZero;
	int8 constructorType;
	tsiMemObject *mem;

	uint32 bitBufferIn,  bitCountIn;  /* "left"  aligned. */
	uint8 decrypted;
	/* public */

} InputStream;

#ifdef USE_PRE_CACHING
void PreLoadT2KInputStream( InputStream *t, int32 requestedByteCount );
int PrimeT2KInputStream(InputStream *t );
#define EnsureWeHaveDataInT2KInputStream( stream, n ) \
	( ( stream->pos - stream->cachePosition + (n) > stream->cacheCount ) ? \
	PrimeT2KInputStream( stream ) : 0 )
#endif /* USE_PRE_CACHING */

/* ALL external clients (top level call to scaler) need to set errCode pointer,
   ALL INTERNAL clients neet to set the errCode pointer == NULL, so that
   we only do setjmp for the top-most external call
   This applies to the 4 constructors and the one destructor
*/
/* Does free data */
InputStream *New_InputStream( tsiMemObject *mem, uint8 *data, uint32 length, int *errCode );
InputStream *New_InputStream2( tsiMemObject *mem, InputStream *in, uint32 offset, uint32 length, int fastIndex, int *errCode );
/* Does not free data */
InputStream *New_InputStream3( tsiMemObject *mem, uint8 *data, uint32 length, int *errCode );
InputStream *New_InputStream4( tsiMemObject *mem, uint8 *data, uint32 length, int *errCode );

#ifdef ENABLE_NON_RAM_STREAM
InputStream *New_NonRamInputStream( tsiMemObject *mem, void *nonRamID, PF_READ_TO_RAM readFunc, uint32 length, int *errCode );
#endif

void Delete_InputStream( InputStream *t, int *errCode );

int32 SizeInStream( InputStream *stream );

int32 ReadInt32( InputStream *stream );
int16 ReadInt16( InputStream *stream );

#define EOF_STREAM -1


#ifdef COMPRESSED_INPUT_STREAM /* for compressed stream */
uint8 ReadUnsignedByteSlow( InputStream *stream );
#define ReadUnsignedByteMacro( stream )	ReadUnsignedByteSlow(stream)
#define ReadUnsignedByteMacro2( stream ) ( (int32)(stream->pos >= stream->maxPos ? EOF_STREAM : (ReadUnsignedByteMacro(stream)) ) )
#elif !defined(ENABLE_NON_RAM_STREAM)
#define ReadUnsignedByteMacro( stream )  ((uint8)(stream->privateBase[(stream->pos)++]))
#define ReadUnsignedByteMacro2( stream ) ((int32)(stream->pos >= stream->maxPos ? EOF_STREAM : stream->privateBase[(stream->pos)++]) )
#endif /* COMPRESSED_INPUT_STREAM */

#ifdef  ENABLE_NON_RAM_STREAM
#if defined(USE_PRE_CACHING) & !defined(COMPRESSED_INPUT_STREAM)
#define ReadUnsignedByteMacro( stream ) \
	( (uint8) (stream->privateBase != NULL ? \
	( stream->ReadToRamFunc != NULL ? \
	EnsureWeHaveDataInT2KInputStream( stream, 1 ), stream->privateBase[(stream->pos)++ - stream->cachePosition] : \
	stream->privateBase[(stream->pos)++] ) : \
	( ( stream->ReadToRamFunc( stream->nonRamID, &(stream->tmp_ch), (stream->pos)++, 1 ) < 0 ) ? tsi_Error( stream->mem, T2K_EXT_IO_CALLBACK_ERR),0 : stream->tmp_ch))  )
#else
#ifndef COMPRESSED_INPUT_STREAM
#define ReadUnsignedByteMacro( stream ) ( (uint8) (stream->privateBase != NULL ? (stream->privateBase[(stream->pos)++]) : ( stream->ReadToRamFunc( stream->nonRamID, &(stream->tmp_ch), (stream->pos)++, 1 ) < 0 ? tsi_Error( stream->mem, T2K_EXT_IO_CALLBACK_ERR),0 : stream->tmp_ch))  )
#endif /* COMPRESSED_INPUT_STREAM */
#endif /* USE_PRE_CACHING */
#ifndef COMPRESSED_INPUT_STREAM
#define ReadUnsignedByteMacro2( stream ) ( (int32)(stream->pos >= stream->maxPos ? EOF_STREAM : (ReadUnsignedByteMacro(stream)) ) )
#endif /* COMPRESSED_INPUT_STREAM */
#endif /* ENABLE_NON_RAM_STREAM */


uint8 *GetEntireStreamIntoMemory( InputStream *stream  );
void ReadSegment( InputStream *stream, uint8 *dest, int32 numBytes );

void Rewind_InputStream( InputStream *t );
void Seek_InputStream( InputStream *t, uint32 offset );
uint32 Tell_InputStream( InputStream *t );


typedef struct {
	/* private */
	uint8 *base;
	uint32 maxPos;
	uint32 pos;
	uint32 maxLength;
	tsiMemObject *mem;

	uint32 bitBufferOut, bitCountOut; /* "left"  aligned. */
	/* public */

} OutputStream;

#define Tell_OutputStream( out ) (out->pos)

#define GET_POINTER( out ) ( out->base )
OutputStream *New_OutputStream( tsiMemObject *mem, int32 initialSize );

void WriteBitsToStream( OutputStream *out, uint32 bits, uint32 count );
/* When done with all calls to WriteBitsToStream call this to flush remaining
  data to the stream  */
void FlushOutStream( OutputStream *out );
void WriteInt32( OutputStream *stream, int32 value );
void WriteInt16( OutputStream *stream, int16 value );
void WriteUnsignedByte( OutputStream *stream, uint8 value );
void Write( OutputStream *stream, uint8 *src, int32 numBytes );
void Rewind_OutputStream( OutputStream *t );
void Delete_OutputStream( OutputStream *t );

int32 SizeOutStream( OutputStream *stream ); /* max size/position seen */
int32 OutStreamPos( OutputStream *stream );  /* current size/position */

/* uses a variable number of bytes */
void WriteUnsignedNumber( OutputStream *out, uint32 n );
uint32 ReadUnsignedNumber( InputStream *in );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __T2K_STREAM__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/t2kstrm.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: t2kstrm.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.6  2005/05/19 15:16:19  Shawn_Flynn
 *     Updated to support COMPRESSED_INPUT_STREAM functionality.
 *     
 *     Revision 1.5  2002/12/16 19:52:23  Shawn_Flynn
 *     dtypED.
 *     Added support for new member ->decrypted for decrypted T1 streams.
 *
 *     Revision 1.4  1999/11/19 01:42:23  reggers
 *     Make non-ram stream error return possible.
 *     Revision 1.3  1999/09/30 15:12:20  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:33  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
