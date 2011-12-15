/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjl <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __DIRECT__STREAM_H__
#define __DIRECT__STREAM_H__

#include <stdio.h>
#include <sys/time.h>

#include <direct/types.h>

/*
 * Create a stream wrapper.
 * 
 * 'filename' can be a plain file name or one of the following:
 *   http://<host>[:<port>]/<path>
 *   ftp://<host>[:<port>]/<path>
 *   tcp://<host>:<port>
 *   udp://<host>:<port>
 *   file:/<path>
 *   stdin:/
 */
DirectResult  direct_stream_create  ( const char     *filename,
                                      DirectStream  **ret_stream );

/*
 * Open a stream of type FILE from a Direct stream.
 */
DirectResult  direct_stream_fopen   ( DirectStream   *stream,
                                      FILE          **ret_file );

/*
 * True if stream is seekable.
 */
bool          direct_stream_seekable( DirectStream   *stream );

/*
 * True if stream originates from a remote host.
 */
bool          direct_stream_remote  ( DirectStream   *stream );

/*
 * Get stream length.
 */
unsigned int  direct_stream_length  ( DirectStream   *stream );

/*
 * Get stream position.
 */
unsigned int  direct_stream_offset  ( DirectStream   *stream );

/*
 * Wait for data to be available.
 * If 'timeout' is NULL, the function blocks indefinitely.
 * Set the 'timeout' to 0 to make the function return immediatly.
 */
DirectResult  direct_stream_wait    ( DirectStream   *stream,
                                      unsigned int    length,
                                      struct timeval *timeout );

/*
 * Peek 'length' bytes of data at offset 'offset' from the stream.
 */
DirectResult  direct_stream_peek    ( DirectStream   *stream,
                                      unsigned int    length,
                                      int             offset,
                                      void           *buf,
                                      unsigned int   *read_out );

/*
 * Fetch 'length' bytes of data from the stream.
 */
DirectResult  direct_stream_read    ( DirectStream   *stream,
                                      unsigned int    length,
                                      void           *buf,
                                      unsigned int   *read_out );

/*
 * Seek to the specified absolute offset within the stream.
 */
DirectResult  direct_stream_seek    ( DirectStream   *stream,
                                      unsigned int    offset );

/*
 * Destroy the stream wrapper.
 */
void          direct_stream_destroy ( DirectStream   *stream );


#endif /* __DIRECT__STREAM_H__ */

