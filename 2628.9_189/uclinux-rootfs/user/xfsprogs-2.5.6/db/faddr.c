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
#include "type.h"
#include "faddr.h"
#include "inode.h"
#include "io.h"
#include "bit.h"
#include "bmap.h"
#include "output.h"
#include "init.h"

void
fa_agblock(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_agblock_t	bno;

	if (cur_agno == NULLAGNUMBER) {
		dbprintf("no current allocation group, cannot set new addr\n");
		return;
	}
	bno = (xfs_agblock_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == NULLAGBLOCK) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], XFS_AGB_TO_DADDR(mp, cur_agno, bno), blkbb,
		DB_RING_ADD, NULL);
}

/*ARGSUSED*/
void
fa_agino(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_agino_t	agino;

	if (cur_agno == NULLAGNUMBER) {
		dbprintf("no current allocation group, cannot set new addr\n");
		return;
	}
	agino = (xfs_agino_t)getbitval(obj, bit, bitsz(agino), BVUNSIGNED);
	if (agino == NULLAGINO) {
		dbprintf("null inode number, cannot set new addr\n");
		return;
	}
	set_cur_inode(XFS_AGINO_TO_INO(mp, cur_agno, agino));
}

/*ARGSUSED*/
void
fa_attrblock(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	bmap_ext_t	bm;
	__uint32_t	bno;
	xfs_dfsbno_t	dfsbno;
	int		nex;

	bno = (__uint32_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == 0) {
		dbprintf("null attribute block number, cannot set new addr\n");
		return;
	}
	nex = 1;
	bmap(bno, 1, XFS_ATTR_FORK, &nex, &bm);
	if (nex == 0) {
		dbprintf("attribute block is unmapped\n");
		return;
	}
	dfsbno = bm.startblock + (bno - bm.startoff);
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], (__int64_t)XFS_FSB_TO_DADDR(mp, dfsbno), blkbb,
		DB_RING_ADD, NULL);
}

void
fa_cfileoffa(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	bmap_ext_t	bm;
	xfs_dfiloff_t	bno;
	xfs_dfsbno_t	dfsbno;
	int		nex;

	bno = (xfs_dfiloff_t)getbitval(obj, bit, BMBT_STARTOFF_BITLEN,
		BVUNSIGNED);
	if (bno == NULLDFILOFF) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	nex = 1;
	bmap(bno, 1, XFS_ATTR_FORK, &nex, &bm);
	if (nex == 0) {
		dbprintf("file block is unmapped\n");
		return;
	}
	dfsbno = bm.startblock + (bno - bm.startoff);
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], XFS_FSB_TO_DADDR(mp, dfsbno), blkbb, DB_RING_ADD,
		NULL);
}

void
fa_cfileoffd(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	bbmap_t		bbmap;
	bmap_ext_t	*bmp;
	xfs_dfiloff_t	bno;
	xfs_dfsbno_t	dfsbno;
	int		nb;
	int		nex;

	bno = (xfs_dfiloff_t)getbitval(obj, bit, BMBT_STARTOFF_BITLEN,
		BVUNSIGNED);
	if (bno == NULLDFILOFF) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	nex = nb = next == TYP_DIR2 ? mp->m_dirblkfsbs : 1;
	bmp = malloc(nb * sizeof(*bmp));
	bmap(bno, nb, XFS_DATA_FORK, &nex, bmp);
	if (nex == 0) {
		dbprintf("file block is unmapped\n");
		free(bmp);
		return;
	}
	dfsbno = bmp->startblock + (bno - bmp->startoff);
	ASSERT(typtab[next].typnm == next);
	if (nex > 1)
		make_bbmap(&bbmap, nex, bmp);
	set_cur(&typtab[next], XFS_FSB_TO_DADDR(mp, dfsbno), nb * blkbb,
		DB_RING_ADD, nex > 1 ? &bbmap: NULL);
	free(bmp);
}

void
fa_cfsblock(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_dfsbno_t	bno;

	bno = (xfs_dfsbno_t)getbitval(obj, bit, BMBT_STARTBLOCK_BITLEN,
		BVUNSIGNED);
	if (bno == NULLDFSBNO) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], XFS_FSB_TO_DADDR(mp, bno), blkbb, DB_RING_ADD,
		NULL);
}

void
fa_dfiloffa(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	bmap_ext_t	bm;
	xfs_dfiloff_t	bno;
	xfs_dfsbno_t	dfsbno;
	int		nex;

	bno = (xfs_dfiloff_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == NULLDFILOFF) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	nex = 1;
	bmap(bno, 1, XFS_ATTR_FORK, &nex, &bm);
	if (nex == 0) {
		dbprintf("file block is unmapped\n");
		return;
	}
	dfsbno = bm.startblock + (bno - bm.startoff);
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], XFS_FSB_TO_DADDR(mp, dfsbno), blkbb, DB_RING_ADD,
		NULL);
}

void
fa_dfiloffd(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	bbmap_t		bbmap;
	bmap_ext_t	*bmp;
	xfs_dfiloff_t	bno;
	xfs_dfsbno_t	dfsbno;
	int		nb;
	int		nex;

	bno = (xfs_dfiloff_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == NULLDFILOFF) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	nex = nb = next == TYP_DIR2 ? mp->m_dirblkfsbs : 1;
	bmp = malloc(nb * sizeof(*bmp));
	bmap(bno, nb, XFS_DATA_FORK, &nex, bmp);
	if (nex == 0) {
		dbprintf("file block is unmapped\n");
		free(bmp);
		return;
	}
	dfsbno = bmp->startblock + (bno - bmp->startoff);
	ASSERT(typtab[next].typnm == next);
	if (nex > 1)
		make_bbmap(&bbmap, nex, bmp);
	set_cur(&typtab[next], XFS_FSB_TO_DADDR(mp, dfsbno), nb * blkbb,
		DB_RING_ADD, nex > 1 ? &bbmap : NULL);
	free(bmp);
}

void
fa_dfsbno(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_dfsbno_t	bno;

	bno = (xfs_dfsbno_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == NULLDFSBNO) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], XFS_FSB_TO_DADDR(mp, bno), blkbb, DB_RING_ADD,
		NULL);
}

/*ARGSUSED*/
void
fa_dirblock(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	bbmap_t		bbmap;
	bmap_ext_t	*bmp;
	__uint32_t	bno;
	xfs_dfsbno_t	dfsbno;
	int		nex;

	bno = (__uint32_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == 0) {
		dbprintf("null directory block number, cannot set new addr\n");
		return;
	}
	nex = mp->m_dirblkfsbs;
	bmp = malloc(nex * sizeof(*bmp));
	bmap(bno, mp->m_dirblkfsbs, XFS_DATA_FORK, &nex, bmp);
	if (nex == 0) {
		dbprintf("directory block is unmapped\n");
		free(bmp);
		return;
	}
	dfsbno = bmp->startblock + (bno - bmp->startoff);
	ASSERT(typtab[next].typnm == next);
	if (nex > 1)
		make_bbmap(&bbmap, nex, bmp);
	set_cur(&typtab[next], (__int64_t)XFS_FSB_TO_DADDR(mp, dfsbno),
		XFS_FSB_TO_BB(mp, mp->m_dirblkfsbs), DB_RING_ADD,
		nex > 1 ? &bbmap : NULL);
	free(bmp);
}

void
fa_drfsbno(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_drfsbno_t	bno;

	bno = (xfs_drfsbno_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == NULLDRFSBNO) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	ASSERT(typtab[next].typnm == next);
	set_cur(&typtab[next], (__int64_t)XFS_FSB_TO_BB(mp, bno), blkbb,
		DB_RING_ADD, NULL);
}

/*ARGSUSED*/
void
fa_drtbno(
	void	*obj,
	int	bit,
	typnm_t	next)
{
	xfs_drtbno_t	bno;

	bno = (xfs_drtbno_t)getbitval(obj, bit, bitsz(bno), BVUNSIGNED);
	if (bno == NULLDRTBNO) {
		dbprintf("null block number, cannot set new addr\n");
		return;
	}
	/* need set_cur to understand rt subvolume */
}

/*ARGSUSED*/
void
fa_ino(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_ino_t	ino;

	ASSERT(next == TYP_INODE);
	ino = (xfs_ino_t)getbitval(obj, bit, bitsz(ino), BVUNSIGNED);
	if (ino == NULLFSINO) {
		dbprintf("null inode number, cannot set new addr\n");
		return;
	}
	set_cur_inode(ino);
}

void
fa_ino4(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_ino_t	ino;
	xfs_dir2_ino4_t	ino4;

	ASSERT(next == TYP_INODE);
	ino = (xfs_ino_t)getbitval(obj, bit, bitsz(ino4), BVUNSIGNED);
	if (ino == NULLFSINO) {
		dbprintf("null inode number, cannot set new addr\n");
		return;
	}
	set_cur_inode(ino);
}

void
fa_ino8(
	void		*obj,
	int		bit,
	typnm_t		next)
{
	xfs_ino_t	ino;
	xfs_dir2_ino8_t	ino8;

	ASSERT(next == TYP_INODE);
	ino = (xfs_ino_t)getbitval(obj, bit, bitsz(ino8), BVUNSIGNED);
	if (ino == NULLFSINO) {
		dbprintf("null inode number, cannot set new addr\n");
		return;
	}
	set_cur_inode(ino);
}
