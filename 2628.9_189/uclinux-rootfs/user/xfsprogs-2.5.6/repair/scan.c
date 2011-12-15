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

#include <libxfs.h>
#include "avl.h"
#include "globals.h"
#include "agheader.h"
#include "incore.h"
#include "protos.h"
#include "err_protos.h"
#include "dinode.h"
#include "scan.h"
#include "versions.h"
#include "bmap.h"

extern int verify_set_agheader(xfs_mount_t *mp, xfs_buf_t *sbuf, xfs_sb_t *sb,
		xfs_agf_t *agf, xfs_agi_t *agi, xfs_agnumber_t i);

static xfs_mount_t	*mp = NULL;
static xfs_extlen_t	bno_agffreeblks;
static xfs_extlen_t	cnt_agffreeblks;
static xfs_extlen_t	bno_agflongest;
static xfs_extlen_t	cnt_agflongest;
static xfs_agino_t	agicount;
static xfs_agino_t	agifreecount;

void
set_mp(xfs_mount_t *mpp)
{
	mp = mpp;
}

void
scan_sbtree(
	xfs_agblock_t	root,
	int		nlevels,
	xfs_agnumber_t	agno,
	int		suspect,
	void		(*func)(xfs_btree_sblock_t	*block,
				int			level,
				xfs_agblock_t		bno,
				xfs_agnumber_t		agno,
				int			suspect,
				int			isroot),
	int		isroot)
{
	xfs_buf_t	*bp;

	bp = libxfs_readbuf(mp->m_dev, XFS_AGB_TO_DADDR(mp, agno, root),
			XFS_FSB_TO_BB(mp, 1), 0);
	if (!bp) {
		do_error(_("can't read btree block %d/%d\n"), agno, root);
		return;
	}
	(*func)((xfs_btree_sblock_t *)XFS_BUF_PTR(bp),
		nlevels - 1, root, agno, suspect, isroot);
	libxfs_putbuf(bp);
}

/*
 * returns 1 on bad news (inode needs to be cleared), 0 on good
 */
int
scan_lbtree(
	xfs_dfsbno_t	root,
	int		nlevels,
	int		(*func)(xfs_btree_lblock_t	*block,
				int			level,
				int			type,
				int			whichfork,
				xfs_dfsbno_t		bno,
				xfs_ino_t		ino,
				xfs_drfsbno_t		*tot,
				__uint64_t		*nex,
				blkmap_t		**blkmapp,
				bmap_cursor_t		*bm_cursor,
				int			isroot,
				int			check_dups,
				int			*dirty),
	int		type,
	int		whichfork,
	xfs_ino_t	ino,
	xfs_drfsbno_t	*tot,
	__uint64_t	*nex,
	blkmap_t	**blkmapp,
	bmap_cursor_t	*bm_cursor,
	int		isroot,
	int		check_dups)
{
	xfs_buf_t	*bp;
	int		err;
	int		dirty = 0;

	bp = libxfs_readbuf(mp->m_dev, XFS_FSB_TO_DADDR(mp, root),
		      XFS_FSB_TO_BB(mp, 1), 0);
	if (!bp)  {
		do_error(_("can't read btree block %d/%d\n"),
			XFS_FSB_TO_AGNO(mp, root),
			XFS_FSB_TO_AGBNO(mp, root));
		return(1);
	}
	err = (*func)((xfs_btree_lblock_t *)XFS_BUF_PTR(bp), nlevels - 1,
			type, whichfork, root, ino, tot, nex, blkmapp,
			bm_cursor, isroot, check_dups, &dirty);

	ASSERT(dirty == 0 || (dirty && !no_modify));

	if (dirty && !no_modify)
		libxfs_writebuf(bp, 0);
	else
		libxfs_putbuf(bp);

	return(err);
}

int
scanfunc_bmap(
	xfs_btree_lblock_t	*ablock,
	int			level,
	int			type,
	int			whichfork,
	xfs_dfsbno_t		bno,
	xfs_ino_t		ino,
	xfs_drfsbno_t		*tot,
	__uint64_t		*nex,
	blkmap_t		**blkmapp,
	bmap_cursor_t		*bm_cursor,
	int			isroot,
	int			check_dups,
	int			*dirty)
{
	xfs_bmbt_block_t	*block = (xfs_bmbt_block_t *)ablock;
	int			i;
	int			err;
	xfs_bmbt_ptr_t		*pp;
	xfs_bmbt_key_t		*pkey;
	xfs_bmbt_rec_32_t	*rp;
	xfs_dfiloff_t		first_key;
	xfs_dfiloff_t		last_key;
	char			*forkname;

	if (whichfork == XFS_DATA_FORK)
		forkname = _("data");
	else
		forkname = _("attr");

	/*
	 * unlike the ag freeblock btrees, if anything looks wrong
	 * in an inode bmap tree, just bail.  it's possible that
	 * we'll miss a case where the to-be-toasted inode and
	 * another inode are claiming the same block but that's
	 * highly unlikely.
	 */
	if (INT_GET(block->bb_magic, ARCH_CONVERT) != XFS_BMAP_MAGIC) {
		do_warn(
		_("bad magic # %#x in inode %llu (%s fork) bmbt block %llu\n"),
			INT_GET(block->bb_magic, ARCH_CONVERT),
			ino, forkname, bno);
		return(1);
	}
	if (INT_GET(block->bb_level, ARCH_CONVERT) != level) {
		do_warn(
_("expected level %d got %d in inode %llu, (%s fork) bmbt block %llu\n"),
			level, INT_GET(block->bb_level, ARCH_CONVERT),
			ino, forkname, bno);
		return(1);
	}

	if (check_dups == 0)  {
		/*
		 * check sibling pointers. if bad we have a conflict
		 * between the sibling pointers and the child pointers
		 * in the parent block.  blow out the inode if that happens
		 */
		if (bm_cursor->level[level].fsbno != NULLDFSBNO)  {
			/*
			 * this is not the first block on this level
			 * so the cursor for this level has recorded the
			 * values for this's block left-sibling.
			 */
			if (bno != bm_cursor->level[level].right_fsbno)  {
				do_warn(
_("bad fwd (right) sibling pointer (saw %llu parent block says %llu)\n"
  "\tin inode %llu (%s fork) bmap btree block %llu\n"),
					bm_cursor->level[level].right_fsbno,
					bno, ino, forkname,
					bm_cursor->level[level].fsbno);
				return(1);
			}
			if (INT_GET(block->bb_leftsib, ARCH_CONVERT) !=
					bm_cursor->level[level].fsbno)  {
				do_warn(
_("bad back (left) sibling pointer (saw %llu parent block says %llu)\n"
  "\tin inode %llu (%s fork) bmap btree block %llu\n"),
					INT_GET(block->bb_leftsib,
						ARCH_CONVERT),
					bm_cursor->level[level].fsbno,
					ino, forkname, bno);
				return(1);
			}
		} else {
			/*
			 * This is the first or only block on this level.
			 * Check that the left sibling pointer is NULL
			 */
			if (INT_GET(block->bb_leftsib, ARCH_CONVERT) !=
					NULLDFSBNO)  {
				do_warn(
_("bad back (left) sibling pointer (saw %llu should be NULL (0))\n"
  "\tin inode %llu (%s fork) bmap btree block %llu\n"),
					INT_GET(block->bb_leftsib,
						ARCH_CONVERT),
					ino, forkname, bno);
				return(1);
			}
		}

		/*
		 * update cursor block pointers to reflect this block
		 */
		bm_cursor->level[level].fsbno = bno;
		bm_cursor->level[level].left_fsbno =
			INT_GET(block->bb_leftsib, ARCH_CONVERT);
		bm_cursor->level[level].right_fsbno =
			INT_GET(block->bb_rightsib, ARCH_CONVERT);

		switch (get_fsbno_state(mp, bno))  {
		case XR_E_UNKNOWN:
		case XR_E_FREE1:
		case XR_E_FREE:
			set_fsbno_state(mp, bno, XR_E_INUSE);
			break;
		case XR_E_FS_MAP:
		case XR_E_INUSE:
			/*
			 * we'll try and continue searching here since
			 * the block looks like it's been claimed by file
			 * to store user data, a directory to store directory
			 * data, or the space allocation btrees but since
			 * we made it here, the block probably
			 * contains btree data.
			 */
			set_fsbno_state(mp, bno, XR_E_MULT);
			do_warn(
		_("inode 0x%llx bmap block 0x%llx claimed, state is %d\n"),
				ino, (__uint64_t) bno,
				get_fsbno_state(mp, bno));
			break;
		case XR_E_MULT:
		case XR_E_INUSE_FS:
			set_fsbno_state(mp, bno, XR_E_MULT);
			do_warn(
		_("inode 0x%llx bmap block 0x%llx claimed, state is %d\n"),
				ino, (__uint64_t) bno,
				get_fsbno_state(mp, bno));
			/*
			 * if we made it to here, this is probably a bmap block
			 * that is being used by *another* file as a bmap block
			 * so the block will be valid.  Both files should be
			 * trashed along with any other file that impinges on
			 * any blocks referenced by either file.  So we
			 * continue searching down this btree to mark all
			 * blocks duplicate
			 */
			break;
		case XR_E_BAD_STATE:
		default:
			do_warn(
		_("bad state %d, inode 0x%llx bmap block 0x%llx\n"),
				get_fsbno_state(mp, bno),
				ino, (__uint64_t) bno);
			break;
		}
	} else  {
		/*
		 * attribute fork for realtime files is in the regular
		 * filesystem
		 */
		if (type != XR_INO_RTDATA || whichfork != XFS_DATA_FORK)  {
			if (search_dup_extent(mp, XFS_FSB_TO_AGNO(mp, bno),
					XFS_FSB_TO_AGBNO(mp, bno)))
				return(1);
		} else  {
			if (search_rt_dup_extent(mp, bno))
				return(1);
		}
	}
	(*tot)++;
	if (level == 0) {
		if (INT_GET(block->bb_numrecs, ARCH_CONVERT) >
			mp->m_bmap_dmxr[0] ||
		    (isroot == 0 && INT_GET(block->bb_numrecs, ARCH_CONVERT) <
			mp->m_bmap_dmnr[0])) {
				do_warn(
	_("inode 0x%llx bad # of bmap records (%u, min - %u, max - %u)\n"),
				ino, INT_GET(block->bb_numrecs, ARCH_CONVERT),
				mp->m_bmap_dmnr[0], mp->m_bmap_dmxr[0]);
			return(1);
		}
		rp = (xfs_bmbt_rec_32_t *)
			XFS_BTREE_REC_ADDR(mp->m_sb.sb_blocksize, xfs_bmbt,
			block, 1, mp->m_bmap_dmxr[0]);
		*nex += INT_GET(block->bb_numrecs, ARCH_CONVERT);
		/*
		 * XXX - if we were going to fix up the btree record,
		 * we'd do it right here.  For now, if there's a problem,
		 * we'll bail out and presumably clear the inode.
		 */
		if (check_dups == 0)  {
			err = process_bmbt_reclist(mp, rp,
					INT_GET(block->bb_numrecs,
						ARCH_CONVERT),
					type, ino, tot, blkmapp,
					&first_key, &last_key,
					whichfork);
			if (err)
				return(1);
			/*
			 * check that key ordering is monotonically increasing.
			 * if the last_key value in the cursor is set to
			 * NULLDFILOFF, then we know this is the first block
			 * on the leaf level and we shouldn't check the
			 * last_key value.
			 */
			if (first_key <= bm_cursor->level[level].last_key &&
					bm_cursor->level[level].last_key !=
					NULLDFILOFF)  {
				do_warn(
_("out-of-order bmap key (file offset) in inode %llu, %s fork, fsbno %llu\n"),
					ino, forkname, bno);
				return(1);
			}
			/*
			 * update cursor keys to reflect this block.
			 * don't have to check if last_key is > first_key
			 * since that gets checked by process_bmbt_reclist.
			 */
			bm_cursor->level[level].first_key = first_key;
			bm_cursor->level[level].last_key = last_key;

			return(0);
		} else
			return(scan_bmbt_reclist(mp, rp, INT_GET(block->bb_numrecs, ARCH_CONVERT),
						type, ino, tot, whichfork));
	}
	if (INT_GET(block->bb_numrecs, ARCH_CONVERT) > mp->m_bmap_dmxr[1] ||
	    (isroot == 0 &&
	     INT_GET(block->bb_numrecs, ARCH_CONVERT) < mp->m_bmap_dmnr[1])) {
		do_warn(
	_("inode 0x%llx bad # of bmap records (%u, min - %u, max - %u)\n"),
			ino, INT_GET(block->bb_numrecs, ARCH_CONVERT),
			mp->m_bmap_dmnr[1], mp->m_bmap_dmxr[1]);
		return(1);
	}
	pp = XFS_BTREE_PTR_ADDR(mp->m_sb.sb_blocksize, xfs_bmbt, block, 1,
		mp->m_bmap_dmxr[1]);
	pkey = XFS_BTREE_KEY_ADDR(mp->m_sb.sb_blocksize, xfs_bmbt, block, 1,
		mp->m_bmap_dmxr[1]);

	last_key = NULLDFILOFF;

	for (i = 0, err = 0; i < INT_GET(block->bb_numrecs, ARCH_CONVERT); i++)  {
		/*
		 * XXX - if we were going to fix up the interior btree nodes,
		 * we'd do it right here.  For now, if there's a problem,
		 * we'll bail out and presumably clear the inode.
		 */
		if (!verify_dfsbno(mp, INT_GET(pp[i], ARCH_CONVERT)))  {
			do_warn(_("bad bmap btree ptr 0x%llx in ino %llu\n"),
				INT_GET(pp[i], ARCH_CONVERT), ino);
			return(1);
		}

		err = scan_lbtree(INT_GET(pp[i], ARCH_CONVERT),
				level, scanfunc_bmap, type, whichfork,
				ino, tot, nex, blkmapp, bm_cursor, 0,
				check_dups);
		if (err)
			return(1);

		/*
		 * fix key (offset) mismatches between the first key
		 * in the child block (as recorded in the cursor) and the
		 * key in the interior node referencing the child block.
		 *
		 * fixes cases where entries have been shifted between
		 * child blocks but the parent hasn't been updated.  We
		 * don't have to worry about the key values in the cursor
		 * not being set since we only look at the key values of
		 * our child and those are guaranteed to be set by the
		 * call to scan_lbtree() above.
		 */
		if (check_dups == 0 &&
		    INT_GET(pkey[i].br_startoff, ARCH_CONVERT) !=
					bm_cursor->level[level-1].first_key)  {
			if (!no_modify)  {
				do_warn(
		_("correcting bt key (was %llu, now %llu) in inode %llu\n"
		  "\t\t%s fork, btree block %llu\n"),
					INT_GET(pkey[i].br_startoff,
						ARCH_CONVERT),
					bm_cursor->level[level-1].first_key,
					ino,
					forkname, bno);
				*dirty = 1;
				INT_SET(pkey[i].br_startoff, ARCH_CONVERT,
					bm_cursor->level[level-1].first_key);
			} else  {
				do_warn(
	_("bad btree key (is %llu, should be %llu) in inode %llu\n"
	  "\t\t%s fork, btree block %llu\n"),
					INT_GET(pkey[i].br_startoff,
						ARCH_CONVERT),
					bm_cursor->level[level-1].first_key,
					ino,
					forkname, bno);
			}
		}
	}

	/*
	 * Check that the last child block's forward sibling pointer
	 * is NULL.
	 */
	if (check_dups == 0 &&
		bm_cursor->level[level - 1].right_fsbno != NULLDFSBNO)  {
		do_warn(
	_("bad fwd (right) sibling pointer (saw %llu should be NULLDFSBNO)\n"
	  "\tin inode %llu (%s fork) bmap btree block %llu\n"),
			bm_cursor->level[level - 1].right_fsbno,
			ino, forkname,
			bm_cursor->level[level].fsbno);
		return(1);
	}

	/*
	 * update cursor keys to reflect this block
	 */
	if (check_dups == 0)  {
		bm_cursor->level[level].first_key =
				INT_GET(pkey[0].br_startoff, ARCH_CONVERT);
		i = INT_GET(block->bb_numrecs, ARCH_CONVERT) - 1;
		bm_cursor->level[level].last_key =
				INT_GET(pkey[i].br_startoff, ARCH_CONVERT);
	}

	return(0);
}

void
scanfunc_bno(
	xfs_btree_sblock_t	*ablock,
	int			level,
	xfs_agblock_t		bno,
	xfs_agnumber_t		agno,
	int			suspect,
	int			isroot
	)
{
	xfs_agblock_t		b;
	xfs_alloc_block_t	*block = (xfs_alloc_block_t *)ablock;
	int			i;
	xfs_alloc_ptr_t		*pp;
	xfs_alloc_rec_t		*rp;
	int			hdr_errors = 0;
	int			numrecs;
	int			state;

	if (INT_GET(block->bb_magic, ARCH_CONVERT) != XFS_ABTB_MAGIC) {
		do_warn(_("bad magic # %#x in btbno block %d/%d\n"),
			INT_GET(block->bb_magic, ARCH_CONVERT), agno, bno);
		hdr_errors++;
		if (suspect)
			return;
	}
	if (INT_GET(block->bb_level, ARCH_CONVERT) != level) {
		do_warn(_("expected level %d got %d in btbno block %d/%d\n"),
			level, INT_GET(block->bb_level, ARCH_CONVERT),
			agno, bno);
		hdr_errors++;
		if (suspect)
			return;
	}

	/*
	 * check for btree blocks multiply claimed
	 */
	state = get_agbno_state(mp, agno, bno);

	switch (state)  {
	case XR_E_UNKNOWN:
		set_agbno_state(mp, agno, bno, XR_E_FS_MAP);
		break;
	default:
		set_agbno_state(mp, agno, bno, XR_E_MULT);
		do_warn(
_("bno freespace btree block claimed (state %d), agno %d, bno %d, suspect %d\n"),
				state, agno, bno, suspect);
		return;
	}

	if (level == 0) {
		numrecs = INT_GET(block->bb_numrecs, ARCH_CONVERT);

		if (INT_GET(block->bb_numrecs, ARCH_CONVERT) >
		    mp->m_alloc_mxr[0])  {
			numrecs = mp->m_alloc_mxr[0];
			hdr_errors++;
		}
		if (isroot == 0 && INT_GET(block->bb_numrecs, ARCH_CONVERT) <
		    mp->m_alloc_mnr[0])  {
			numrecs = mp->m_alloc_mnr[0];
			hdr_errors++;
		}

		if (hdr_errors)
			suspect++;

		rp = XFS_BTREE_REC_ADDR(mp->m_sb.sb_blocksize, xfs_alloc, block,
			1, mp->m_alloc_mxr[0]);
		for (i = 0; i < numrecs; i++) {
			if (INT_GET(rp[i].ar_blockcount, ARCH_CONVERT) == 0 ||
			    INT_GET(rp[i].ar_startblock, ARCH_CONVERT) == 0 ||
			    !verify_agbno(mp, agno,
				INT_GET(rp[i].ar_startblock, ARCH_CONVERT)) ||
			    INT_GET(rp[i].ar_blockcount, ARCH_CONVERT) >
					MAXEXTLEN)
				continue;

			bno_agffreeblks +=
				INT_GET(rp[i].ar_blockcount, ARCH_CONVERT);
			if (INT_GET(rp[i].ar_blockcount, ARCH_CONVERT) >
			    bno_agflongest)
				bno_agflongest = INT_GET(rp[i].ar_blockcount,
							ARCH_CONVERT);
			for (b = INT_GET(rp[i].ar_startblock, ARCH_CONVERT);
			     b < INT_GET(rp[i].ar_startblock, ARCH_CONVERT) +
				 INT_GET(rp[i].ar_blockcount, ARCH_CONVERT);
			     b++)  {
				if (get_agbno_state(mp, agno, b)
							== XR_E_UNKNOWN)
					set_agbno_state(mp, agno, b,
							XR_E_FREE1);
				else  {
					do_warn(
	_("block (%d,%d) multiply claimed by bno space tree, state - %d\n"),
						agno, b,
						get_agbno_state(mp, agno, b));
				}
			}
		}
		return;
	}

	/*
	 * interior record
	 */
	pp = XFS_BTREE_PTR_ADDR(mp->m_sb.sb_blocksize, xfs_alloc, block, 1,
		mp->m_alloc_mxr[1]);

	numrecs = INT_GET(block->bb_numrecs, ARCH_CONVERT);
	if (INT_GET(block->bb_numrecs, ARCH_CONVERT) > mp->m_alloc_mxr[1])  {
		numrecs = mp->m_alloc_mxr[1];
		hdr_errors++;
	}
	if (isroot == 0 &&
	    INT_GET(block->bb_numrecs, ARCH_CONVERT) < mp->m_alloc_mnr[1])  {
		numrecs = mp->m_alloc_mnr[1];
		hdr_errors++;
	}

	/*
	 * don't pass bogus tree flag down further if this block
	 * looked ok.  bail out if two levels in a row look bad.
	 */

	if (suspect && !hdr_errors)
		suspect = 0;

	if (hdr_errors)  {
		if (suspect)
			return;
		else suspect++;
	}

	for (i = 0; i < numrecs; i++)  {
		/*
		 * XXX - put sibling detection right here.
		 * we know our sibling chain is good.  So as we go,
		 * we check the entry before and after each entry.
		 * If either of the entries references a different block,
		 * check the sibling pointer.  If there's a sibling
		 * pointer mismatch, try and extract as much data
		 * as possible.
		 */
		if (INT_GET(pp[i], ARCH_CONVERT) != 0 &&
		    verify_agbno(mp, agno, INT_GET(pp[i], ARCH_CONVERT)))
			scan_sbtree(INT_GET(pp[i], ARCH_CONVERT),
					level, agno, suspect, scanfunc_bno, 0);
	}
}

void
scanfunc_cnt(
	xfs_btree_sblock_t	*ablock,
	int			level,
	xfs_agblock_t		bno,
	xfs_agnumber_t		agno,
	int			suspect,
	int			isroot
	)
{
	xfs_alloc_block_t	*block;
	xfs_alloc_ptr_t		*pp;
	xfs_alloc_rec_t		*rp;
	xfs_agblock_t		b;
	int			i;
	int			hdr_errors;
	int			numrecs;
	int			state;

	block = (xfs_alloc_block_t *)ablock;
	hdr_errors = 0;

	if (INT_GET(block->bb_magic, ARCH_CONVERT) != XFS_ABTC_MAGIC) {
		do_warn(_("bad magic # %#x in btcnt block %d/%d\n"),
			INT_GET(block->bb_magic, ARCH_CONVERT), agno, bno);
		hdr_errors++;
		if (suspect)
			return;
	}
	if (INT_GET(block->bb_level, ARCH_CONVERT) != level) {
		do_warn(_("expected level %d got %d in btcnt block %d/%d\n"),
			level, INT_GET(block->bb_level, ARCH_CONVERT),
			agno, bno);
		hdr_errors++;
		if (suspect)
			return;
	}

	/*
	 * check for btree blocks multiply claimed
	 */
	state = get_agbno_state(mp, agno, bno);

	switch (state)  {
	case XR_E_UNKNOWN:
		set_agbno_state(mp, agno, bno, XR_E_FS_MAP);
		break;
	default:
		set_agbno_state(mp, agno, bno, XR_E_MULT);
		do_warn(
_("bcnt freespace btree block claimed (state %d), agno %d, bno %d, suspect %d\n"),
			state, agno, bno, suspect);
		return;
	}

	if (level == 0) {
		numrecs = INT_GET(block->bb_numrecs, ARCH_CONVERT);

		if (INT_GET(block->bb_numrecs, ARCH_CONVERT) >
		    mp->m_alloc_mxr[0])  {
			numrecs = mp->m_alloc_mxr[0];
			hdr_errors++;
		}
		if (isroot == 0 && INT_GET(block->bb_numrecs, ARCH_CONVERT) <
		    mp->m_alloc_mnr[0])  {
			numrecs = mp->m_alloc_mnr[0];
			hdr_errors++;
		}

		if (hdr_errors)
			suspect++;

		rp = XFS_BTREE_REC_ADDR(mp->m_sb.sb_blocksize, xfs_alloc, block,
			1, mp->m_alloc_mxr[0]);
		for (i = 0; i < numrecs; i++) {
			if (INT_GET(rp[i].ar_blockcount, ARCH_CONVERT) == 0 ||
			    INT_GET(rp[i].ar_startblock, ARCH_CONVERT) == 0 ||
			    !verify_agbno(mp, agno,
				INT_GET(rp[i].ar_startblock, ARCH_CONVERT)) ||
			    INT_GET(rp[i].ar_blockcount, ARCH_CONVERT) >
						MAXEXTLEN)
				continue;

			cnt_agffreeblks +=
				INT_GET(rp[i].ar_blockcount, ARCH_CONVERT);
			if (INT_GET(rp[i].ar_blockcount, ARCH_CONVERT) >
			    cnt_agflongest)
				cnt_agflongest = INT_GET(rp[i].ar_blockcount,
							ARCH_CONVERT);
			for (b = INT_GET(rp[i].ar_startblock, ARCH_CONVERT);
			     b < INT_GET(rp[i].ar_startblock, ARCH_CONVERT) +
				 INT_GET(rp[i].ar_blockcount, ARCH_CONVERT);
			     b++)  {
				state = get_agbno_state(mp, agno, b);
				/*
				 * no warning messages -- we'll catch
				 * FREE1 blocks later
				 */
				switch (state)  {
				case XR_E_FREE1:
					set_agbno_state(mp, agno, b, XR_E_FREE);
					break;
				case XR_E_UNKNOWN:
					set_agbno_state(mp, agno, b,
							XR_E_FREE1);
					break;
				default:
					do_warn(
				_("block (%d,%d) already used, state %d\n"),
						agno, b, state);
					break;
				}
			}
		}
		return;
	}

	/*
	 * interior record
	 */
	pp = XFS_BTREE_PTR_ADDR(mp->m_sb.sb_blocksize, xfs_alloc, block, 1,
		mp->m_alloc_mxr[1]);

	numrecs = INT_GET(block->bb_numrecs, ARCH_CONVERT);
	if (INT_GET(block->bb_numrecs, ARCH_CONVERT) > mp->m_alloc_mxr[1])  {
		numrecs = mp->m_alloc_mxr[1];
		hdr_errors++;
	}
	if (isroot == 0 &&
	    INT_GET(block->bb_numrecs, ARCH_CONVERT) < mp->m_alloc_mnr[1])  {
		numrecs = mp->m_alloc_mnr[1];
		hdr_errors++;
	}

	/*
	 * don't pass bogus tree flag down further if this block
	 * looked ok.  bail out if two levels in a row look bad.
	 */

	if (suspect && !hdr_errors)
		suspect = 0;

	if (hdr_errors)  {
		if (suspect)
			return;
		else suspect++;
	}

	for (i = 0; i < numrecs; i++)
		if (INT_GET(pp[i], ARCH_CONVERT) != 0 &&
		    verify_agbno(mp, agno, INT_GET(pp[i], ARCH_CONVERT)))
			scan_sbtree(INT_GET(pp[i], ARCH_CONVERT), level, agno,
				suspect, scanfunc_cnt, 0);
}

/*
 * this one walks the inode btrees sucking the info there into
 * the incore avl tree.  We try and rescue corrupted btree records
 * to minimize our chances of losing inodes.  Inode info from potentially
 * corrupt sources could be bogus so rather than put the info straight
 * into the tree, instead we put it on a list and try and verify the
 * info in the next phase by examining what's on disk.  At that point,
 * we'll be able to figure out what's what and stick the corrected info
 * into the tree.  We do bail out at some point and give up on a subtree
 * so as to avoid walking randomly all over the ag.
 *
 * Note that it's also ok if the free/inuse info wrong, we can correct
 * that when we examine the on-disk inode.  The important thing is to
 * get the start and alignment of the inode chunks right.  Those chunks
 * that we aren't sure about go into the uncertain list.
 */
void
scanfunc_ino(
	xfs_btree_sblock_t	*ablock,
	int			level,
	xfs_agblock_t		bno,
	xfs_agnumber_t		agno,
	int			suspect,
	int			isroot
	)
{
	xfs_ino_t		lino;
	xfs_inobt_block_t	*block;
	int			i;
	xfs_agino_t		ino;
	xfs_agblock_t		agbno;
	int			j;
	int			nfree;
	int			off;
	int			numrecs;
	int			state;
	xfs_inobt_ptr_t		*pp;
	xfs_inobt_rec_t		*rp;
	ino_tree_node_t		*ino_rec, *first_rec, *last_rec;
	int			hdr_errors;

	block = (xfs_inobt_block_t *)ablock;
	hdr_errors = 0;

	if (INT_GET(block->bb_magic, ARCH_CONVERT) != XFS_IBT_MAGIC) {
		do_warn(_("bad magic # %#x in inobt block %d/%d\n"),
			INT_GET(block->bb_magic, ARCH_CONVERT), agno, bno);
		hdr_errors++;
		bad_ino_btree = 1;
		if (suspect)
			return;
	}
	if (INT_GET(block->bb_level, ARCH_CONVERT) != level) {
		do_warn(_("expected level %d got %d in inobt block %d/%d\n"),
			level, INT_GET(block->bb_level, ARCH_CONVERT),
			agno, bno);
		hdr_errors++;
		bad_ino_btree = 1;
		if (suspect)
			return;
	}

	/*
	 * check for btree blocks multiply claimed, any unknown/free state
	 * is ok in the bitmap block.
	 */
	state = get_agbno_state(mp, agno, bno);

	switch (state)  {
	case XR_E_UNKNOWN:
	case XR_E_FREE1:
	case XR_E_FREE:
		set_agbno_state(mp, agno, bno, XR_E_FS_MAP);
		break;
	default:
		set_agbno_state(mp, agno, bno, XR_E_MULT);
		do_warn(
_("inode btree block claimed (state %d), agno %d, bno %d, suspect %d\n"),
			state, agno, bno, suspect);
	}

	numrecs = INT_GET(block->bb_numrecs, ARCH_CONVERT);

	/*
	 * leaf record in btree
	 */
	if (level == 0) {
		/* check for trashed btree block */

		if (INT_GET(block->bb_numrecs, ARCH_CONVERT) >
		    mp->m_inobt_mxr[0])  {
			numrecs = mp->m_inobt_mxr[0];
			hdr_errors++;
		}
		if (isroot == 0 && INT_GET(block->bb_numrecs, ARCH_CONVERT) <
		    mp->m_inobt_mnr[0])  {
			numrecs = mp->m_inobt_mnr[0];
			hdr_errors++;
		}

		if (hdr_errors)  {
			bad_ino_btree = 1;
			do_warn(_("dubious inode btree block header %d/%d\n"),
				agno, bno);
			suspect++;
		}

		rp = XFS_BTREE_REC_ADDR(mp->m_sb.sb_blocksize, xfs_inobt, block,
			1, mp->m_inobt_mxr[0]);

		/*
		 * step through the records, each record points to
		 * a chunk of inodes.  The start of inode chunks should
		 * be block-aligned.  Each inode btree rec should point
		 * to the start of a block of inodes or the start of a group
		 * of INODES_PER_CHUNK (64) inodes.  off is the offset into
		 * the block.  skip processing of bogus records.
		 */
		for (i = 0; i < numrecs; i++) {
			ino = INT_GET(rp[i].ir_startino, ARCH_CONVERT);
			off = XFS_AGINO_TO_OFFSET(mp, ino);
			agbno = XFS_AGINO_TO_AGBNO(mp, ino);
			lino = XFS_AGINO_TO_INO(mp, agno, ino);
			/*
			 * on multi-block block chunks, all chunks start
			 * at the beginning of the block.  with multi-chunk
			 * blocks, all chunks must start on 64-inode boundaries
			 * since each block can hold N complete chunks. if
			 * fs has aligned inodes, all chunks must start
			 * at a fs_ino_alignment*N'th agbno.  skip recs
			 * with badly aligned starting inodes.
			 */
			if (ino == 0 ||
			    (inodes_per_block <= XFS_INODES_PER_CHUNK &&
			     off !=  0) ||
			    (inodes_per_block > XFS_INODES_PER_CHUNK &&
			     off % XFS_INODES_PER_CHUNK != 0) ||
			    (fs_aligned_inodes &&
			     agbno % fs_ino_alignment != 0))  {
				do_warn(
			_("badly aligned inode rec (starting inode = %llu)\n"),
					lino);
				suspect++;
			}

			/*
			 * verify numeric validity of inode chunk first
			 * before inserting into a tree.  don't have to
			 * worry about the overflow case because the
			 * starting ino number of a chunk can only get
			 * within 255 inodes of max (NULLAGINO).  if it
			 * gets closer, the agino number will be illegal
			 * as the agbno will be too large.
			 */
			if (verify_aginum(mp, agno, ino))  {
				do_warn(
_("bad starting inode # (%llu (0x%x 0x%x)) in ino rec, skipping rec\n"),
					lino, agno, ino);
				suspect++;
				continue;
			}

			if (verify_aginum(mp, agno,
					ino + XFS_INODES_PER_CHUNK - 1))  {
				do_warn(
_("bad ending inode # (%llu (0x%x 0x%x)) in ino rec, skipping rec\n"),
					lino + XFS_INODES_PER_CHUNK - 1,
					agno, ino + XFS_INODES_PER_CHUNK - 1);
				suspect++;
				continue;
			}

			/*
			 * set state of each block containing inodes
			 */
			if (off == 0 && !suspect)  {
				for (j = 0;
				     j < XFS_INODES_PER_CHUNK;
				     j += mp->m_sb.sb_inopblock)  {
					agbno = XFS_AGINO_TO_AGBNO(mp, ino + j);
					state = get_agbno_state(mp,
							agno, agbno);

					if (state == XR_E_UNKNOWN)  {
						set_agbno_state(mp, agno,
							agbno, XR_E_INO);
					} else if (state == XR_E_INUSE_FS &&
						agno == 0 &&
						ino + j >= first_prealloc_ino &&
						ino + j < last_prealloc_ino)  {
						set_agbno_state(mp, agno,
							agbno, XR_E_INO);
					} else  {
						do_warn(
_("inode chunk claims used block, inobt block - agno %d, bno %d, inopb %d\n"),
							agno, bno,
							mp->m_sb.sb_inopblock);
						suspect++;
						/*
						 * XXX - maybe should mark
						 * block a duplicate
						 */
						continue;
					}
				}
			}
			/*
			 * ensure only one avl entry per chunk
			 */
			find_inode_rec_range(agno, ino,
					ino + XFS_INODES_PER_CHUNK,
					&first_rec,
					&last_rec);
			if (first_rec != NULL)  {
				/*
				 * this chunk overlaps with one (or more)
				 * already in the tree
				 */
				do_warn(
_("inode rec for ino %llu (%d/%d) overlaps existing rec (start %d/%d)\n"),
					lino, agno, ino,
					agno, first_rec->ino_startnum);
				suspect++;

				/*
				 * if the 2 chunks start at the same place,
				 * then we don't have to put this one
				 * in the uncertain list.  go to the next one.
				 */
				if (first_rec->ino_startnum == ino)
					continue;
			}

			agicount += XFS_INODES_PER_CHUNK;
			agifreecount +=
				INT_GET(rp[i].ir_freecount, ARCH_CONVERT);
			nfree = 0;

			/*
			 * now mark all the inodes as existing and free or used.
			 * if the tree is suspect, put them into the uncertain
			 * inode tree.
			 */
			if (!suspect)  {
				if (XFS_INOBT_IS_FREE(&rp[i], 0, ARCH_CONVERT)) {
					nfree++;
					ino_rec = set_inode_free_alloc(agno,
									ino);
				} else  {
					ino_rec = set_inode_used_alloc(agno,
									ino);
				}
				for (j = 1; j < XFS_INODES_PER_CHUNK; j++) {
					if (XFS_INOBT_IS_FREE(&rp[i], j, ARCH_CONVERT)) {
						nfree++;
						set_inode_free(ino_rec, j);
					} else  {
						set_inode_used(ino_rec, j);
					}
				}
			} else  {
				for (j = 0; j < XFS_INODES_PER_CHUNK; j++) {
					if (XFS_INOBT_IS_FREE(&rp[i], j, ARCH_CONVERT)) {
						nfree++;
						add_aginode_uncertain(agno,
								ino + j, 1);
					} else  {
						add_aginode_uncertain(agno,
								ino + j, 0);
					}
				}
			}

			if (nfree != INT_GET(rp[i].ir_freecount, ARCH_CONVERT)) {
				do_warn(
_("ir_freecount/free mismatch, inode chunk %d/%d, freecount %d nfree %d\n"),
					agno, ino, INT_GET(rp[i].ir_freecount,
						ARCH_CONVERT), nfree);
			}
		}

		if (suspect)
			bad_ino_btree = 1;

		return;
	}

	/*
	 * interior record, continue on
	 */
	if (INT_GET(block->bb_numrecs, ARCH_CONVERT) > mp->m_inobt_mxr[1])  {
		numrecs = mp->m_inobt_mxr[1];
		hdr_errors++;
	}
	if (isroot == 0 &&
	    INT_GET(block->bb_numrecs, ARCH_CONVERT) < mp->m_inobt_mnr[1])  {
		numrecs = mp->m_inobt_mnr[1];
		hdr_errors++;
	}

	pp = XFS_BTREE_PTR_ADDR(mp->m_sb.sb_blocksize, xfs_inobt, block, 1,
		mp->m_inobt_mxr[1]);

	/*
	 * don't pass bogus tree flag down further if this block
	 * looked ok.  bail out if two levels in a row look bad.
	 */

	if (suspect && !hdr_errors)
		suspect = 0;

	if (hdr_errors)  {
		bad_ino_btree = 1;
		if (suspect)
			return;
		else suspect++;
	}

	for (i = 0; i < numrecs; i++)  {
		if (INT_GET(pp[i], ARCH_CONVERT) != 0 &&
		    verify_agbno(mp, agno, INT_GET(pp[i], ARCH_CONVERT)))
			scan_sbtree(INT_GET(pp[i], ARCH_CONVERT), level,
					agno, suspect, scanfunc_ino, 0);
	}
}

void
scan_freelist(
	xfs_agf_t	*agf)
{
	xfs_agfl_t	*agfl;
	xfs_buf_t	*agflbuf;
	xfs_agblock_t	bno;
	int		count;
	int		i;

	if (XFS_SB_BLOCK(mp) != XFS_AGFL_BLOCK(mp) &&
	    XFS_AGF_BLOCK(mp) != XFS_AGFL_BLOCK(mp) &&
	    XFS_AGI_BLOCK(mp) != XFS_AGFL_BLOCK(mp))
		set_agbno_state(mp, INT_GET(agf->agf_seqno, ARCH_CONVERT),
			XFS_AGFL_BLOCK(mp), XR_E_FS_MAP);
	if (INT_GET(agf->agf_flcount, ARCH_CONVERT) == 0)
		return;
	agflbuf = libxfs_readbuf(mp->m_dev,
			XFS_AG_DADDR(mp, INT_GET(agf->agf_seqno, ARCH_CONVERT),
				XFS_AGFL_DADDR(mp)), XFS_FSS_TO_BB(mp, 1), 0);
	if (!agflbuf)  {
		do_abort(_("can't read agfl block for ag %d\n"),
			INT_GET(agf->agf_seqno, ARCH_CONVERT));
		return;
	}
	agfl = XFS_BUF_TO_AGFL(agflbuf);
	i = INT_GET(agf->agf_flfirst, ARCH_CONVERT);
	count = 0;
	for (;;) {
		bno = INT_GET(agfl->agfl_bno[i], ARCH_CONVERT);
		if (verify_agbno(mp, INT_GET(agf->agf_seqno,ARCH_CONVERT), bno))
			set_agbno_state(mp,
				INT_GET(agf->agf_seqno, ARCH_CONVERT),
				bno, XR_E_FREE);
		else
			do_warn(_("bad agbno %u in agfl, agno %d\n"),
				bno, INT_GET(agf->agf_seqno, ARCH_CONVERT));
		count++;
		if (i == INT_GET(agf->agf_fllast, ARCH_CONVERT))
			break;
		if (++i == XFS_AGFL_SIZE(mp))
			i = 0;
	}
	if (count != INT_GET(agf->agf_flcount, ARCH_CONVERT)) {
		do_warn(_("freeblk count %d != flcount %d in ag %d\n"), count,
			INT_GET(agf->agf_flcount, ARCH_CONVERT),
			INT_GET(agf->agf_seqno, ARCH_CONVERT));
	}
	libxfs_putbuf(agflbuf);
}

void
scan_ag(
	xfs_agnumber_t	agno)
{
	xfs_agf_t	*agf;
	xfs_buf_t	*agfbuf;
	int		agf_dirty;
	xfs_agi_t	*agi;
	xfs_buf_t	*agibuf;
	int		agi_dirty;
	xfs_sb_t	*sb;
	xfs_buf_t	*sbbuf;
	int		sb_dirty;
	int		status;

	cnt_agffreeblks = cnt_agflongest = 0;
	bno_agffreeblks = bno_agflongest = 0;

	agi_dirty = agf_dirty = sb_dirty = 0;

	agicount = agifreecount = 0;

	sbbuf = libxfs_readbuf(mp->m_dev, XFS_AG_DADDR(mp, agno, XFS_SB_DADDR),
				1, 0);
	if (!sbbuf)  {
		do_error(_("can't get root superblock for ag %d\n"), agno);
		return;
	}

	sb = (xfs_sb_t *)calloc(BBSIZE, 1);
	if (!sb) {
		do_error(_("can't allocate memory for superblock\n"));
		libxfs_putbuf(sbbuf);
		return;
	}
	libxfs_xlate_sb(XFS_BUF_TO_SBP(sbbuf), sb, 1, ARCH_CONVERT,
			XFS_SB_ALL_BITS);

	agfbuf = libxfs_readbuf(mp->m_dev,
			XFS_AG_DADDR(mp, agno, XFS_AGF_DADDR(mp)),
			XFS_FSS_TO_BB(mp, 1), 0);
	if (!agfbuf)  {
		do_error(_("can't read agf block for ag %d\n"), agno);
		libxfs_putbuf(sbbuf);
		free(sb);
		return;
	}
	agf = XFS_BUF_TO_AGF(agfbuf);

	agibuf = libxfs_readbuf(mp->m_dev,
			XFS_AG_DADDR(mp, agno, XFS_AGI_DADDR(mp)),
			XFS_FSS_TO_BB(mp, 1), 0);
	if (!agibuf)  {
		do_error(_("can't read agi block for ag %d\n"), agno);
		libxfs_putbuf(agfbuf);
		libxfs_putbuf(sbbuf);
		free(sb);
		return;
	}
	agi = XFS_BUF_TO_AGI(agibuf);

	/* fix up bad ag headers */

	status = verify_set_agheader(mp, sbbuf, sb, agf, agi, agno);

	if (status & XR_AG_SB_SEC)  {
		if (!no_modify)
			sb_dirty = 1;
		/*
		 * clear bad sector bit because we don't want
		 * to skip further processing.  we just want to
		 * ensure that we write out the modified sb buffer.
		 */
		status &= ~XR_AG_SB_SEC;
	}
	if (status & XR_AG_SB)  {
		if (!no_modify) {
			do_warn(_("reset bad sb for ag %d\n"), agno);
			sb_dirty = 1;
		} else {
			do_warn(_("would reset bad sb for ag %d\n"), agno);
		}
	}
	if (status & XR_AG_AGF)  {
		if (!no_modify) {
			do_warn(_("reset bad agf for ag %d\n"), agno);
			agf_dirty = 1;
		} else {
			do_warn(_("would reset bad agf for ag %d\n"), agno);
		}
	}
	if (status & XR_AG_AGI)  {
		if (!no_modify) {
			do_warn(_("reset bad agi for ag %d\n"), agno);
			agi_dirty = 1;
		} else {
			do_warn(_("would reset bad agi for ag %d\n"), agno);
		}
	}

	if (status && no_modify)  {
		libxfs_putbuf(agibuf);
		libxfs_putbuf(agfbuf);
		libxfs_putbuf(sbbuf);
		free(sb);

		do_warn(_("bad uncorrected agheader %d, skipping ag...\n"),
			agno);

		return;
	}

	scan_freelist(agf);

	if (INT_GET(agf->agf_roots[XFS_BTNUM_BNO], ARCH_CONVERT) != 0 &&
	    verify_agbno(mp, agno,
			INT_GET(agf->agf_roots[XFS_BTNUM_BNO], ARCH_CONVERT)))
		scan_sbtree(
			INT_GET(agf->agf_roots[XFS_BTNUM_BNO], ARCH_CONVERT),
			INT_GET(agf->agf_levels[XFS_BTNUM_BNO], ARCH_CONVERT),
			agno, 0, scanfunc_bno, 1);
	else
		do_warn(_("bad agbno %u for btbno root, agno %d\n"),
			INT_GET(agf->agf_roots[XFS_BTNUM_BNO], ARCH_CONVERT),
			agno);

	if (INT_GET(agf->agf_roots[XFS_BTNUM_CNT], ARCH_CONVERT) != 0 &&
	    verify_agbno(mp, agno,
			INT_GET(agf->agf_roots[XFS_BTNUM_CNT], ARCH_CONVERT)))
		scan_sbtree(
			INT_GET(agf->agf_roots[XFS_BTNUM_CNT], ARCH_CONVERT),
			INT_GET(agf->agf_levels[XFS_BTNUM_CNT], ARCH_CONVERT),
			agno, 0, scanfunc_cnt, 1);
	else
		do_warn(_("bad agbno %u for btbcnt root, agno %d\n"),
			INT_GET(agf->agf_roots[XFS_BTNUM_CNT], ARCH_CONVERT),
			agno);

	if (INT_GET(agi->agi_root, ARCH_CONVERT) != 0 &&
	    verify_agbno(mp, agno, INT_GET(agi->agi_root, ARCH_CONVERT)))
		scan_sbtree(
			INT_GET(agi->agi_root, ARCH_CONVERT),
			INT_GET(agi->agi_level, ARCH_CONVERT),
			agno, 0, scanfunc_ino, 1);
	else
		do_warn(_("bad agbno %u for inobt root, agno %d\n"),
			INT_GET(agi->agi_root, ARCH_CONVERT), agno);

	ASSERT(agi_dirty == 0 || (agi_dirty && !no_modify));

	if (agi_dirty && !no_modify)
		libxfs_writebuf(agibuf, 0);
	else
		libxfs_putbuf(agibuf);

	ASSERT(agf_dirty == 0 || (agf_dirty && !no_modify));

	if (agf_dirty && !no_modify)
		libxfs_writebuf(agfbuf, 0);
	else
		libxfs_putbuf(agfbuf);

	ASSERT(sb_dirty == 0 || (sb_dirty && !no_modify));

	if (sb_dirty && !no_modify) {
		libxfs_xlate_sb(XFS_BUF_PTR(sbbuf), sb, -1, ARCH_CONVERT,
				XFS_SB_ALL_BITS);
		libxfs_writebuf(sbbuf, 0);
	} else
		libxfs_putbuf(sbbuf);
	free(sb);
}
