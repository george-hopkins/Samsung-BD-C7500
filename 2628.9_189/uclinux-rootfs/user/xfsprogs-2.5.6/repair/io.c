/*
 * Copyright (c) 2000-2002 Silicon Graphics, Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Further, this software is distributed without any warranty that it is
 * free of the rightful claim of any third person regarding infringement
 * or the like.  Any license provided herein, whether implied or
 * otherwise, applies only to this software file.  Patent licenses, if
 * any, provided herein do not apply to combinations of this program with
 * other software, or any other product whatsoever.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Contact information: Silicon Graphics, Inc., 1600 Amphitheatre Pkwy,
 * Mountain View, CA  94043, or:
 *
 * http://www.sgi.com
 *
 * For further information regarding this notice, see:
 *
 * http://oss.sgi.com/projects/GenInfo/SGIGPLNoticeExplan/
 */

#include <libxfs.h>
#include "globals.h"
#include "agheader.h"
#include "protos.h"
#include "err_protos.h"

void
io_init(void)
{
	int i;

	/* open up filesystem device */

	ASSERT(fs_name != NULL && *fs_name != '\0');

	if ((fs_fd = open (fs_name, (no_modify? O_RDONLY : O_RDWR))) < 0)  {
		do_error(_("couldn't open filesystem \"%s\"\n"), fs_name);
	}

	/* initialize i/o buffers */

	iobuf_size = 1000 * 1024;
	smallbuf_size = 4 * 4096;	/* enough for an ag */

	/*
	 * sbbuf_size must be < XFS_MIN_AG_BLOCKS (64) * smallest block size,
	 * otherwise you might get an EOF when reading in the sb/agf from
	 * the last ag if that ag is small
	 */
	sbbuf_size = 2 * 4096;		/* 2 * max sector size */

	if ((iobuf = malloc(iobuf_size)) == NULL)
		do_error(_("couldn't malloc io buffer\n"));

	if ((smallbuf = malloc(smallbuf_size)) == NULL)
		do_error(_("couldn't malloc secondary io buffer\n"));

	for (i = 0; i < NUM_SBS; i++)  {
		if ((sb_bufs[i] = malloc(sbbuf_size)) == NULL)
			do_error(_("couldn't malloc sb io buffers\n"));
	}
}
