/*
 * Copyright (c) 2000-2001 Silicon Graphics, Inc.  All Rights Reserved.
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

#include <xfs/libxfs.h>
#include "agf.h"
#include "command.h"
#include "type.h"
#include "faddr.h"
#include "fprint.h"
#include "field.h"
#include "io.h"
#include "bit.h"
#include "output.h"
#include "init.h"

static int agf_f(int argc, char **argv);
static void agf_help(void);

static const cmdinfo_t agf_cmd =
	{ "agf", NULL, agf_f, 0, 1, 1, "[agno]",
	  "set address to agf header", agf_help };

const field_t	agf_hfld[] = {
	{ "", FLDT_AGF, OI(0), C1, 0, TYP_NONE },
	{ NULL }
};

#define	OFF(f)	bitize(offsetof(xfs_agf_t, agf_ ## f))
#define	SZ(f)	bitszof(xfs_agf_t, agf_ ## f)
const field_t	agf_flds[] = {
	{ "magicnum", FLDT_UINT32X, OI(OFF(magicnum)), C1, 0, TYP_NONE },
	{ "versionnum", FLDT_UINT32D, OI(OFF(versionnum)), C1, 0, TYP_NONE },
	{ "seqno", FLDT_AGNUMBER, OI(OFF(seqno)), C1, 0, TYP_NONE },
	{ "length", FLDT_AGBLOCK, OI(OFF(length)), C1, 0, TYP_NONE },
	{ "roots", FLDT_AGBLOCK, OI(OFF(roots)), CI(XFS_BTNUM_AGF),
	  FLD_ARRAY|FLD_SKIPALL, TYP_NONE },
	{ "bnoroot", FLDT_AGBLOCK,
	  OI(OFF(roots) + XFS_BTNUM_BNO * SZ(roots[XFS_BTNUM_BNO])), C1, 0,
	  TYP_BNOBT },
	{ "cntroot", FLDT_AGBLOCK,
	  OI(OFF(roots) + XFS_BTNUM_CNT * SZ(roots[XFS_BTNUM_CNT])), C1, 0,
	  TYP_CNTBT },
	{ "levels", FLDT_UINT32D, OI(OFF(levels)), CI(XFS_BTNUM_AGF),
	  FLD_ARRAY|FLD_SKIPALL, TYP_NONE },
	{ "bnolevel", FLDT_UINT32D,
	  OI(OFF(levels) + XFS_BTNUM_BNO * SZ(levels[XFS_BTNUM_BNO])), C1, 0,
	  TYP_NONE },
	{ "cntlevel", FLDT_UINT32D,
	  OI(OFF(levels) + XFS_BTNUM_CNT * SZ(levels[XFS_BTNUM_CNT])), C1, 0,
	  TYP_NONE },
	{ "flfirst", FLDT_UINT32D, OI(OFF(flfirst)), C1, 0, TYP_NONE },
	{ "fllast", FLDT_UINT32D, OI(OFF(fllast)), C1, 0, TYP_NONE },
	{ "flcount", FLDT_UINT32D, OI(OFF(flcount)), C1, 0, TYP_NONE },
	{ "freeblks", FLDT_EXTLEN, OI(OFF(freeblks)), C1, 0, TYP_NONE },
	{ "longest", FLDT_EXTLEN, OI(OFF(longest)), C1, 0, TYP_NONE },
	{ NULL }
};

static void
agf_help(void)
{
	dbprintf(
"\n"
" set allocation group free block list\n"
"\n"
" Example:\n"
"\n"
" agf 2 - move location to AGF in 2nd filesystem allocation group\n"
"\n"
" Located in the second sector of each allocation group, the AGF\n"
" contains the root of two different freespace btrees:\n"
" The 'cnt' btree keeps track freespace indexed on section size.\n"
" The 'bno' btree tracks sections of freespace indexed on block number.\n"
);
}

static int
agf_f(
	int		argc,
	char		**argv)
{
	xfs_agnumber_t	agno;
	char		*p;

	if (argc > 1) {
		agno = (xfs_agnumber_t)strtoul(argv[1], &p, 0);
		if (*p != '\0' || agno >= mp->m_sb.sb_agcount) {
			dbprintf("bad allocation group number %s\n", argv[1]);
			return 0;
		}
		cur_agno = agno;
	} else if (cur_agno == NULLAGNUMBER)
		cur_agno = 0;
	ASSERT(typtab[TYP_AGF].typnm == TYP_AGF);
	set_cur(&typtab[TYP_AGF],
		XFS_AG_DADDR(mp, cur_agno, XFS_AGF_DADDR(mp)),
		XFS_FSS_TO_BB(mp, 1), DB_RING_ADD, NULL);
	return 0;
}

void
agf_init(void)
{
	add_command(&agf_cmd);
}

int
agf_size(
	void	*obj,
	int	startoff,
	int	idx)
{
	return bitize(mp->m_sb.sb_sectsize);
}
