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

/*
 * Free realtime space allocation for XFS.
 */

#include <xfs.h>

extern int xfs_lowbit32(__uint32_t);

/*
 * Get a buffer for the bitmap or summary file block specified.
 * The buffer is returned read and locked.
 */
STATIC int				/* error */
xfs_rtbuf_get(
	xfs_mount_t	*mp,		/* file system mount structure */
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_rtblock_t	block,		/* block number in bitmap or summary */
	int		issum,		/* is summary not bitmap */
	xfs_buf_t	**bpp)		/* output: buffer for the block */
{
	xfs_buf_t	*bp;		/* block buffer, result */
	xfs_daddr_t	d;		/* disk addr of block */
	int		error;		/* error value */
	xfs_fsblock_t	fsb;		/* fs block number for block */
	xfs_inode_t	*ip;		/* bitmap or summary inode */

	ip = issum ? mp->m_rsumip : mp->m_rbmip;
	/*
	 * Map from the file offset (block) and inode number to the
	 * file system block.
	 */
	error = xfs_bmapi_single(tp, ip, XFS_DATA_FORK, &fsb, block);
	if (error) {
		return error;
	}
	ASSERT(fsb != NULLFSBLOCK);
	/*
	 * Convert to disk address for buffer cache.
	 */
	d = XFS_FSB_TO_DADDR(mp, fsb);
	/*
	 * Read the buffer.
	 */
	error = xfs_trans_read_buf(mp, tp, mp->m_ddev_targp, d,
				   mp->m_bsize, 0, &bp);
	if (error) {
		return error;
	}
	ASSERT(bp && !XFS_BUF_GETERROR(bp));
	*bpp = bp;
	return 0;
}

/*
 * Searching backward from start to limit, find the first block whose
 * allocated/free state is different from start's.
 */
STATIC int				/* error */
xfs_rtfind_back(
	xfs_mount_t	*mp,		/* file system mount point */
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_rtblock_t	start,		/* starting block to look at */
	xfs_rtblock_t	limit,		/* last block to look at */
	xfs_rtblock_t	*rtblock)	/* out: start block found */
{
	xfs_rtword_t	*b;		/* current word in buffer */
	int		bit;		/* bit number in the word */
	xfs_rtblock_t	block;		/* bitmap block number */
	xfs_buf_t	*bp;		/* buf for the block */
	xfs_rtword_t	*bufp;		/* starting word in buffer */
	int		error;		/* error value */
	xfs_rtblock_t	firstbit;	/* first useful bit in the word */
	xfs_rtblock_t	i;		/* current bit number rel. to start */
	xfs_rtblock_t	len;		/* length of inspected area */
	xfs_rtword_t	mask;		/* mask of relevant bits for value */
	xfs_rtword_t	want;		/* mask for "good" values */
	xfs_rtword_t	wdiff;		/* difference from wanted value */
	int		word;		/* word number in the buffer */

	/*
	 * Compute and read in starting bitmap block for starting block.
	 */
	block = XFS_BITTOBLOCK(mp, start);
	error = xfs_rtbuf_get(mp, tp, block, 0, &bp);
	if (error) {
		return error;
	}
	bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
	/*
	 * Get the first word's index & point to it.
	 */
	word = XFS_BITTOWORD(mp, start);
	b = &bufp[word];
	bit = (int)(start & (XFS_NBWORD - 1));
	len = start - limit + 1;
	/*
	 * Compute match value, based on the bit at start: if 1 (free)
	 * then all-ones, else all-zeroes.
	 */
	want = (*b & ((xfs_rtword_t)1 << bit)) ? -1 : 0;
	/*
	 * If the starting position is not word-aligned, deal with the
	 * partial word.
	 */
	if (bit < XFS_NBWORD - 1) {
		/*
		 * Calculate first (leftmost) bit number to look at,
		 * and mask for all the relevant bits in this word.
		 */
		firstbit = XFS_RTMAX((xfs_srtblock_t)(bit - len + 1), 0);
		mask = (((xfs_rtword_t)1 << (bit - firstbit + 1)) - 1) <<
			firstbit;
		/*
		 * Calculate the difference between the value there
		 * and what we're looking for.
		 */
		if ((wdiff = (*b ^ want) & mask)) {
			/*
			 * Different.  Mark where we are and return.
			 */
			xfs_trans_brelse(tp, bp);
			i = bit - XFS_RTHIBIT(wdiff);
			*rtblock = start - i + 1;
			return 0;
		}
		i = bit - firstbit + 1;
		/*
		 * Go on to previous block if that's where the previous word is
		 * and we need the previous word.
		 */
		if (--word == -1 && i < len) {
			/*
			 * If done with this block, get the previous one.
			 */
			xfs_trans_brelse(tp, bp);
			error = xfs_rtbuf_get(mp, tp, --block, 0, &bp);
			if (error) {
				return error;
			}
			bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
			word = XFS_BLOCKWMASK(mp);
			b = &bufp[word];
		} else {
			/*
			 * Go on to the previous word in the buffer.
			 */
			b--;
		}
	} else {
		/*
		 * Starting on a word boundary, no partial word.
		 */
		i = 0;
	}
	/*
	 * Loop over whole words in buffers.  When we use up one buffer
	 * we move on to the previous one.
	 */
	while (len - i >= XFS_NBWORD) {
		/*
		 * Compute difference between actual and desired value.
		 */
		if ((wdiff = *b ^ want)) {
			/*
			 * Different, mark where we are and return.
			 */
			xfs_trans_brelse(tp, bp);
			i += XFS_NBWORD - 1 - XFS_RTHIBIT(wdiff);
			*rtblock = start - i + 1;
			return 0;
		}
		i += XFS_NBWORD;
		/*
		 * Go on to previous block if that's where the previous word is
		 * and we need the previous word.
		 */
		if (--word == -1 && i < len) {
			/*
			 * If done with this block, get the previous one.
			 */
			xfs_trans_brelse(tp, bp);
			error = xfs_rtbuf_get(mp, tp, --block, 0, &bp);
			if (error) {
				return error;
			}
			bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
			word = XFS_BLOCKWMASK(mp);
			b = &bufp[word];
		} else {
			/*
			 * Go on to the previous word in the buffer.
			 */
			b--;
		}
	}
	/*
	 * If not ending on a word boundary, deal with the last
	 * (partial) word.
	 */
	if (len - i) {
		/*
		 * Calculate first (leftmost) bit number to look at,
		 * and mask for all the relevant bits in this word.
		 */
		firstbit = XFS_NBWORD - (len - i);
		mask = (((xfs_rtword_t)1 << (len - i)) - 1) << firstbit;
		/*
		 * Compute difference between actual and desired value.
		 */
		if ((wdiff = (*b ^ want) & mask)) {
			/*
			 * Different, mark where we are and return.
			 */
			xfs_trans_brelse(tp, bp);
			i += XFS_NBWORD - 1 - XFS_RTHIBIT(wdiff);
			*rtblock = start - i + 1;
			return 0;
		} else
			i = len;
	}
	/*
	 * No match, return that we scanned the whole area.
	 */
	xfs_trans_brelse(tp, bp);
	*rtblock = start - i + 1;
	return 0;
}

/*
 * Searching forward from start to limit, find the first block whose
 * allocated/free state is different from start's.
 */
STATIC int				/* error */
xfs_rtfind_forw(
	xfs_mount_t	*mp,		/* file system mount point */
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_rtblock_t	start,		/* starting block to look at */
	xfs_rtblock_t	limit,		/* last block to look at */
	xfs_rtblock_t	*rtblock)	/* out: start block found */
{
	xfs_rtword_t	*b;		/* current word in buffer */
	int		bit;		/* bit number in the word */
	xfs_rtblock_t	block;		/* bitmap block number */
	xfs_buf_t	*bp;		/* buf for the block */
	xfs_rtword_t	*bufp;		/* starting word in buffer */
	int		error;		/* error value */
	xfs_rtblock_t	i;		/* current bit number rel. to start */
	xfs_rtblock_t	lastbit;	/* last useful bit in the word */
	xfs_rtblock_t	len;		/* length of inspected area */
	xfs_rtword_t	mask;		/* mask of relevant bits for value */
	xfs_rtword_t	want;		/* mask for "good" values */
	xfs_rtword_t	wdiff;		/* difference from wanted value */
	int		word;		/* word number in the buffer */

	/*
	 * Compute and read in starting bitmap block for starting block.
	 */
	block = XFS_BITTOBLOCK(mp, start);
	error = xfs_rtbuf_get(mp, tp, block, 0, &bp);
	if (error) {
		return error;
	}
	bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
	/*
	 * Get the first word's index & point to it.
	 */
	word = XFS_BITTOWORD(mp, start);
	b = &bufp[word];
	bit = (int)(start & (XFS_NBWORD - 1));
	len = limit - start + 1;
	/*
	 * Compute match value, based on the bit at start: if 1 (free)
	 * then all-ones, else all-zeroes.
	 */
	want = (*b & ((xfs_rtword_t)1 << bit)) ? -1 : 0;
	/*
	 * If the starting position is not word-aligned, deal with the
	 * partial word.
	 */
	if (bit) {
		/*
		 * Calculate last (rightmost) bit number to look at,
		 * and mask for all the relevant bits in this word.
		 */
		lastbit = XFS_RTMIN(bit + len, XFS_NBWORD);
		mask = (((xfs_rtword_t)1 << (lastbit - bit)) - 1) << bit;
		/*
		 * Calculate the difference between the value there
		 * and what we're looking for.
		 */
		if ((wdiff = (*b ^ want) & mask)) {
			/*
			 * Different.  Mark where we are and return.
			 */
			xfs_trans_brelse(tp, bp);
			i = XFS_RTLOBIT(wdiff) - bit;
			*rtblock = start + i - 1;
			return 0;
		}
		i = lastbit - bit;
		/*
		 * Go on to next block if that's where the next word is
		 * and we need the next word.
		 */
		if (++word == XFS_BLOCKWSIZE(mp) && i < len) {
			/*
			 * If done with this block, get the previous one.
			 */
			xfs_trans_brelse(tp, bp);
			error = xfs_rtbuf_get(mp, tp, ++block, 0, &bp);
			if (error) {
				return error;
			}
			b = bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
			word = 0;
		} else {
			/*
			 * Go on to the previous word in the buffer.
			 */
			b++;
		}
	} else {
		/*
		 * Starting on a word boundary, no partial word.
		 */
		i = 0;
	}
	/*
	 * Loop over whole words in buffers.  When we use up one buffer
	 * we move on to the next one.
	 */
	while (len - i >= XFS_NBWORD) {
		/*
		 * Compute difference between actual and desired value.
		 */
		if ((wdiff = *b ^ want)) {
			/*
			 * Different, mark where we are and return.
			 */
			xfs_trans_brelse(tp, bp);
			i += XFS_RTLOBIT(wdiff);
			*rtblock = start + i - 1;
			return 0;
		}
		i += XFS_NBWORD;
		/*
		 * Go on to next block if that's where the next word is
		 * and we need the next word.
		 */
		if (++word == XFS_BLOCKWSIZE(mp) && i < len) {
			/*
			 * If done with this block, get the next one.
			 */
			xfs_trans_brelse(tp, bp);
			error = xfs_rtbuf_get(mp, tp, ++block, 0, &bp);
			if (error) {
				return error;
			}
			b = bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
			word = 0;
		} else {
			/*
			 * Go on to the next word in the buffer.
			 */
			b++;
		}
	}
	/*
	 * If not ending on a word boundary, deal with the last
	 * (partial) word.
	 */
	if ((lastbit = len - i)) {
		/*
		 * Calculate mask for all the relevant bits in this word.
		 */
		mask = ((xfs_rtword_t)1 << lastbit) - 1;
		/*
		 * Compute difference between actual and desired value.
		 */
		if ((wdiff = (*b ^ want) & mask)) {
			/*
			 * Different, mark where we are and return.
			 */
			xfs_trans_brelse(tp, bp);
			i += XFS_RTLOBIT(wdiff);
			*rtblock = start + i - 1;
			return 0;
		} else
			i = len;
	}
	/*
	 * No match, return that we scanned the whole area.
	 */
	xfs_trans_brelse(tp, bp);
	*rtblock = start + i - 1;
	return 0;
}

/*
 * Mark an extent specified by start and len freed.
 * Updates all the summary information as well as the bitmap.
 */
STATIC int				/* error */
xfs_rtfree_range(
	xfs_mount_t	*mp,		/* file system mount point */
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_rtblock_t	start,		/* starting block to free */
	xfs_extlen_t	len,		/* length to free */
	xfs_buf_t	**rbpp,		/* in/out: summary block buffer */
	xfs_fsblock_t	*rsb)		/* in/out: summary block number */
{
	xfs_rtblock_t	end;		/* end of the freed extent */
	int		error;		/* error value */
	xfs_rtblock_t	postblock;	/* first block freed > end */
	xfs_rtblock_t	preblock;	/* first block freed < start */

	end = start + len - 1;
	/*
	 * Modify the bitmap to mark this extent freed.
	 */
	error = xfs_rtmodify_range(mp, tp, start, len, 1);
	if (error) {
		return error;
	}
	/*
	 * Assume we're freeing out of the middle of an allocated extent.
	 * We need to find the beginning and end of the extent so we can
	 * properly update the summary.
	 */
	error = xfs_rtfind_back(mp, tp, start, 0, &preblock);
	if (error) {
		return error;
	}
	/*
	 * Find the next allocated block (end of allocated extent).
	 */
	error = xfs_rtfind_forw(mp, tp, end, mp->m_sb.sb_rextents - 1,
		&postblock);
	/*
	 * If there are blocks not being freed at the front of the
	 * old extent, add summary data for them to be allocated.
	 */
	if (preblock < start) {
		error = xfs_rtmodify_summary(mp, tp,
			XFS_RTBLOCKLOG(start - preblock),
			XFS_BITTOBLOCK(mp, preblock), -1, rbpp, rsb);
		if (error) {
			return error;
		}
	}
	/*
	 * If there are blocks not being freed at the end of the
	 * old extent, add summary data for them to be allocated.
	 */
	if (postblock > end) {
		error = xfs_rtmodify_summary(mp, tp,
			XFS_RTBLOCKLOG(postblock - end),
			XFS_BITTOBLOCK(mp, end + 1), -1, rbpp, rsb);
		if (error) {
			return error;
		}
	}
	/*
	 * Increment the summary information corresponding to the entire
	 * (new) free extent.
	 */
	error = xfs_rtmodify_summary(mp, tp,
		XFS_RTBLOCKLOG(postblock + 1 - preblock),
		XFS_BITTOBLOCK(mp, preblock), 1, rbpp, rsb);
	return error;
}

/*
 * Set the given range of bitmap bits to the given value.
 * Do whatever I/O and logging is required.
 */
STATIC int				/* error */
xfs_rtmodify_range(
	xfs_mount_t	*mp,		/* file system mount point */
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_rtblock_t	start,		/* starting block to modify */
	xfs_extlen_t	len,		/* length of extent to modify */
	int		val)		/* 1 for free, 0 for allocated */
{
	xfs_rtword_t	*b;		/* current word in buffer */
	int		bit;		/* bit number in the word */
	xfs_rtblock_t	block;		/* bitmap block number */
	xfs_buf_t	*bp;		/* buf for the block */
	xfs_rtword_t	*bufp;		/* starting word in buffer */
	int		error;		/* error value */
	xfs_rtword_t	*first;		/* first used word in the buffer */
	int		i;		/* current bit number rel. to start */
	int		lastbit;	/* last useful bit in word */
	xfs_rtword_t	mask;		/* mask o frelevant bits for value */
	int		word;		/* word number in the buffer */

	/*
	 * Compute starting bitmap block number.
	 */
	block = XFS_BITTOBLOCK(mp, start);
	/*
	 * Read the bitmap block, and point to its data.
	 */
	error = xfs_rtbuf_get(mp, tp, block, 0, &bp);
	if (error) {
		return error;
	}
	bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
	/*
	 * Compute the starting word's address, and starting bit.
	 */
	word = XFS_BITTOWORD(mp, start);
	first = b = &bufp[word];
	bit = (int)(start & (XFS_NBWORD - 1));
	/*
	 * 0 (allocated) => all zeroes; 1 (free) => all ones.
	 */
	val = -val;
	/*
	 * If not starting on a word boundary, deal with the first
	 * (partial) word.
	 */
	if (bit) {
		/*
		 * Compute first bit not changed and mask of relevant bits.
		 */
		lastbit = XFS_RTMIN(bit + len, XFS_NBWORD);
		mask = (((xfs_rtword_t)1 << (lastbit - bit)) - 1) << bit;
		/*
		 * Set/clear the active bits.
		 */
		if (val)
			*b |= mask;
		else
			*b &= ~mask;
		i = lastbit - bit;
		/*
		 * Go on to the next block if that's where the next word is
		 * and we need the next word.
		 */
		if (++word == XFS_BLOCKWSIZE(mp) && i < len) {
			/*
			 * Log the changed part of this block.
			 * Get the next one.
			 */
			xfs_trans_log_buf(tp, bp,
				(uint)((char *)first - (char *)bufp),
				(uint)((char *)b - (char *)bufp));
			error = xfs_rtbuf_get(mp, tp, ++block, 0, &bp);
			if (error) {
				return error;
			}
			first = b = bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
			word = 0;
		} else {
			/*
			 * Go on to the next word in the buffer
			 */
			b++;
		}
	} else {
		/*
		 * Starting on a word boundary, no partial word.
		 */
		i = 0;
	}
	/*
	 * Loop over whole words in buffers.  When we use up one buffer
	 * we move on to the next one.
	 */
	while (len - i >= XFS_NBWORD) {
		/*
		 * Set the word value correctly.
		 */
		*b = val;
		i += XFS_NBWORD;
		/*
		 * Go on to the next block if that's where the next word is
		 * and we need the next word.
		 */
		if (++word == XFS_BLOCKWSIZE(mp) && i < len) {
			/*
			 * Log the changed part of this block.
			 * Get the next one.
			 */
			xfs_trans_log_buf(tp, bp,
				(uint)((char *)first - (char *)bufp),
				(uint)((char *)b - (char *)bufp));
			error = xfs_rtbuf_get(mp, tp, ++block, 0, &bp);
			if (error) {
				return error;
			}
			first = b = bufp = (xfs_rtword_t *)XFS_BUF_PTR(bp);
			word = 0;
		} else {
			/*
			 * Go on to the next word in the buffer
			 */
			b++;
		}
	}
	/*
	 * If not ending on a word boundary, deal with the last
	 * (partial) word.
	 */
	if ((lastbit = len - i)) {
		/*
		 * Compute a mask of relevant bits.
		 */
		bit = 0;
		mask = ((xfs_rtword_t)1 << lastbit) - 1;
		/*
		 * Set/clear the active bits.
		 */
		if (val)
			*b |= mask;
		else
			*b &= ~mask;
		b++;
	}
	/*
	 * Log any remaining changed bytes.
	 */
	if (b > first)
		xfs_trans_log_buf(tp, bp, (uint)((char *)first - (char *)bufp),
			(uint)((char *)b - (char *)bufp - 1));
	return 0;
}

/*
 * Read and modify the summary information for a given extent size,
 * bitmap block combination.
 * Keeps track of a current summary block, so we don't keep reading
 * it from the buffer cache.
 */
STATIC int				/* error */
xfs_rtmodify_summary(
	xfs_mount_t	*mp,		/* file system mount point */
	xfs_trans_t	*tp,		/* transaction pointer */
	int		log,		/* log2 of extent size */
	xfs_rtblock_t	bbno,		/* bitmap block number */
	int		delta,		/* change to make to summary info */
	xfs_buf_t	**rbpp,		/* in/out: summary block buffer */
	xfs_fsblock_t	*rsb)		/* in/out: summary block number */
{
	xfs_buf_t	*bp;		/* buffer for the summary block */
	int		error;		/* error value */
	xfs_fsblock_t	sb;		/* summary fsblock */
	int		so;		/* index into the summary file */
	xfs_suminfo_t	*sp;		/* pointer to returned data */

	/*
	 * Compute entry number in the summary file.
	 */
	so = XFS_SUMOFFS(mp, log, bbno);
	/*
	 * Compute the block number in the summary file.
	 */
	sb = XFS_SUMOFFSTOBLOCK(mp, so);
	/*
	 * If we have an old buffer, and the block number matches, use that.
	 */
	if (rbpp && *rbpp && *rsb == sb)
		bp = *rbpp;
	/*
	 * Otherwise we have to get the buffer.
	 */
	else {
		/*
		 * If there was an old one, get rid of it first.
		 */
		if (rbpp && *rbpp)
			xfs_trans_brelse(tp, *rbpp);
		error = xfs_rtbuf_get(mp, tp, sb, 1, &bp);
		if (error) {
			return error;
		}
		/*
		 * Remember this buffer and block for the next call.
		 */
		if (rbpp) {
			*rbpp = bp;
			*rsb = sb;
		}
	}
	/*
	 * Point to the summary information, modify and log it.
	 */
	sp = XFS_SUMPTR(mp, bp, so);
	*sp += delta;
	xfs_trans_log_buf(tp, bp, (uint)((char *)sp - (char *)XFS_BUF_PTR(bp)),
		(uint)((char *)sp - (char *)XFS_BUF_PTR(bp) + sizeof(*sp) - 1));
	return 0;
}

/*
 * Free an extent in the realtime subvolume.  Length is expressed in
 * realtime extents, as is the block number.
 */
int					/* error */
xfs_rtfree_extent(
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_rtblock_t	bno,		/* starting block number to free */
	xfs_extlen_t	len)		/* length of extent freed */
{
	int		error;		/* error value */
	xfs_inode_t	*ip;		/* bitmap file inode */
	xfs_mount_t	*mp;		/* file system mount structure */
	xfs_fsblock_t	sb;		/* summary file block number */
	xfs_buf_t	*sumbp;		/* summary file block buffer */

	mp = tp->t_mountp;
	/*
	 * Synchronize by locking the bitmap inode.
	 */
	error = xfs_trans_iget(mp, tp, mp->m_sb.sb_rbmino, XFS_ILOCK_EXCL, &ip);
	if (error) {
		return error;
	}
#if defined(__KERNEL__) && defined(DEBUG)
	/*
	 * Check to see that this whole range is currently allocated.
	 */
	{
		int	stat;		/* result from checking range */

		error = xfs_rtcheck_alloc_range(mp, tp, bno, len, &stat);
		if (error) {
			return error;
		}
		ASSERT(stat);
	}
#endif
	sumbp = NULL;
	/*
	 * Free the range of realtime blocks.
	 */
	error = xfs_rtfree_range(mp, tp, bno, len, &sumbp, &sb);
	if (error) {
		return error;
	}
	/*
	 * Mark more blocks free in the superblock.
	 */
	xfs_trans_mod_sb(tp, XFS_TRANS_SB_FREXTENTS, (long)len);
	/*
	 * If we've now freed all the blocks, reset the file sequence
	 * number to 0.
	 */
	if (tp->t_frextents_delta + mp->m_sb.sb_frextents ==
	    mp->m_sb.sb_rextents) {
		if (!(ip->i_d.di_flags & XFS_DIFLAG_NEWRTBM))
			ip->i_d.di_flags |= XFS_DIFLAG_NEWRTBM;
		*(__uint64_t *)&ip->i_d.di_atime = 0;
		xfs_trans_log_inode(tp, ip, XFS_ILOG_CORE);
	}
	return 0;
}
