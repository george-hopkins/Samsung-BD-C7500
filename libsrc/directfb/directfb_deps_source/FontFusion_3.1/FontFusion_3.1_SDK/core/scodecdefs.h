/*
 * scodecdefs.h
 * Font Fusion Copyright (c) 1989-2005 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Anup Belambe/Tohin
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

#ifndef SCODEC_H_
#define	SCODEC_H_

#include "ldecode.h"
#include "config.h"

#define NCACHE_BLOCKS	1		/* the number of decoded blocks that are maintained in the cache! */
#define C_MAX_LENGTH_MASK	0x8000

typedef	void	*DECODER;

typedef struct tag_CBLOCK
{
	uint32		*offset;		/* offset from the beginning of the stream data. */
	uint16		*clength;	/* length of the compressed/encoded buffer in bytes. */
} CBLOCK, *PCBLOCK, **PPCBLOCK;

typedef struct tag_CSTREAMHEADER
{
	uint32 last_block_length;	/* length of the last block, may not be equal to blockSize */
	uint32 enc_data_length;	/* length of the encoded data in bytes. */
	uint32 data_length;	/* length of the plain data in bytes */
	uint16 blockSize; /* size of block used for compression */
	uint16 nblocks;	/* number of blocks in the table. */
	uint8 majorVer; /* Decoder Major Version */
	uint8 minorVer; /* Decoder Minor Version */
} CSTREAMHEADER, *PCSTREAMHEADER, **PPCSTREAMHEADER;

typedef struct tag_CBLOCKTABLE
{
	CSTREAMHEADER	header;	/* header */
	CBLOCK	 blocks;
} CBLOCKTABLE, *PCBLOCKTABLE, **PPCBLOCKTABLE;

typedef struct tag_DECODER_INFO
{
	uint16 lzmaInternalSize;
	uint8 lc;
	uint8 lp;
	uint8 pb;
} DECODER_INFO;

typedef struct tag_CBLOCKCACHE
{
	DECODER_INFO	decInfo;
	tsiMemObject	*mem;
	PCBLOCKTABLE	table;
	uint8			*enc_stream;			/* stream/buffer that stores the encoded data. */
	/* buffer for cached data */
	uint8			*d_data;
	uint8			*d_pdata[NCACHE_BLOCKS];
	/* points to which portion of the actual data is being stored in the cache. */
	uint32			d_start_pos; 			/* lower range */
	uint32			d_end_pos_plus_1; 		/* upper range */
	uint32			d_current_pos; 			/* pointer to the current data in cache */
	/* we need a decoder model to decode the compressed data in stream */
	DECODER			decoder;
	int16			first_block_id; /* the id of the block which is just prior to the cached blocks. */
	int16			last_block_id; 	/* the id of the block which is just next  to the cached blocks. */
	int8			nblocks_in_use; /* the number of blocks that have been cached, must be <= NCACHE_BLOCKS */
} CBLOCKCACHE, *PCBLOCKCACHE, **PPCBLOCKCACHE;

#define _isencoded(l)			((l) &  C_MAX_LENGTH_MASK)
#define _setencoded(l)			((l) |= (clength) C_MAX_LENGTH_MASK)
#define _resetencoded(l)		((l) &= (~C_MAX_LENGTH_MASK))
#define _offset(x,i)			((PCBLOCKCACHE) (x))->table->blocks.offset[(i)]

#define _table(x) 				((PCBLOCKCACHE) (x))->table
#define _header(x) 				((PCBLOCKCACHE) (x))->table->header
#define _nblocks(x) 			((PCBLOCKCACHE) (x))->table->header.nblocks
#define _version(x)				((PCBLOCKCACHE) (x))->table->header.version
#define _last_block_length(x)	((PCBLOCKCACHE) (x))->table->header.last_block_length
#define  _data_length(x)		((PCBLOCKCACHE) (x))->table->header.data_length
#define _enc_data_length(x)		((PCBLOCKCACHE) (x))->table->header.enc_data_length
#define _enc_stream(x)			((PCBLOCKCACHE) (x))->enc_stream
#define _start_pos(x)			((PCBLOCKCACHE) (x))->d_start_pos
#define _end_pos(x)				((PCBLOCKCACHE) (x))->d_end_pos_plus_1
#define _d_current_pos(x)		((PCBLOCKCACHE) (x))->d_current_pos
#define _decoder(x)				((PCBLOCKCACHE) (x))->decoder
#define _blocks(x)				((PCBLOCKCACHE) (x))->table->blocks
#define _e_length(x,i)			((PCBLOCKCACHE) (x))->table->blocks.clength[(i)]
#define _d_data_3(x)			((PCBLOCKCACHE) (x))->d_data
#define _d_data(x)				(uint8*)(((PCBLOCKCACHE) (x))->d_data + 3)
#define _pblocks(x,i)			((PCBLOCKCACHE) (x))->d_pdata[(i)]
#define _mem(x)					((PCBLOCKCACHE) (x))->mem
#define _first_block(x)			((PCBLOCKCACHE) (x))->first_block_id
#define _last_block(x)			((PCBLOCKCACHE) (x))->last_block_id
#define _nblocks_in_use(x)		((PCBLOCKCACHE) (x))->nblocks_in_use
#define _start_pos_abs(x)		(((PCBLOCKCACHE)(x))->first_block_id * ((PCBLOCKCACHE)(x))->table->header.blockSize)
#define _d_current_ptr(x)		(uint8*)((((PCBLOCKCACHE) (x))->d_data + 3) + ((PCBLOCKCACHE) (x))->d_current_pos)

/* data availability macros */
/* checks if cache is exhausted */
#define _exhausted(x)			((PCBLOCKCACHE)  (x))->d_current_pos >= ((PCBLOCKCACHE) (x))->d_end_pos_plus_1
#define _bytes_available(x)		(((PCBLOCKCACHE) (x))->d_end_pos_plus_1 - ((PCBLOCKCACHE) (x))->d_current_pos)
#define _end_of_file(x)			(((PCBLOCKCACHE)  (x))->d_current_pos >= ((PCBLOCKCACHE) (x))->table->header.data_length)

#endif	/* SCODEC_H_ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/scodecdefs.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *
 *     $Log: scodecdefs.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.1  2005/05/04 19:11:39  Shawn_Flynn
 *     Initial revision
 *
******************************************************************************/

