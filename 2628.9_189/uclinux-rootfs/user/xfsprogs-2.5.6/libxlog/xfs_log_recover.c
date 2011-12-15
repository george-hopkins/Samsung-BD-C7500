/*
 * Copyright (c) 2000-2003 Silicon Graphics, Inc.  All Rights Reserved.
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

#include <xfs/libxlog.h>

#define xlog_unpack_data_checksum(rhead, dp, log)	((void)0)
#define xlog_clear_stale_blocks(log, tail_lsn)		(0)
#define xfs_readonly_buftarg(buftarg)			(0)

/*
 * This routine finds (to an approximation) the first block in the physical
 * log which contains the given cycle.  It uses a binary search algorithm.
 * Note that the algorithm can not be perfect because the disk will not
 * necessarily be perfect.
 */
int
xlog_find_cycle_start(
	xlog_t		*log,
	xfs_buf_t	*bp,
	xfs_daddr_t	first_blk,
	xfs_daddr_t	*last_blk,
	uint		cycle)
{
	xfs_caddr_t	offset;
	xfs_daddr_t	mid_blk;
	uint		mid_cycle;
	int		error;

	mid_blk = BLK_AVG(first_blk, *last_blk);
	while (mid_blk != first_blk && mid_blk != *last_blk) {
		if ((error = xlog_bread(log, mid_blk, 1, bp)))
			return error;
		offset = xlog_align(log, mid_blk, 1, bp);
		mid_cycle = GET_CYCLE(offset, ARCH_CONVERT);
		if (mid_cycle == cycle) {
			*last_blk = mid_blk;
			/* last_half_cycle == mid_cycle */
		} else {
			first_blk = mid_blk;
			/* first_half_cycle == mid_cycle */
		}
		mid_blk = BLK_AVG(first_blk, *last_blk);
	}
	ASSERT((mid_blk == first_blk && mid_blk+1 == *last_blk) ||
	       (mid_blk == *last_blk && mid_blk-1 == first_blk));

	return 0;
}

/*
 * Check that the range of blocks does not contain the cycle number
 * given.  The scan needs to occur from front to back and the ptr into the
 * region must be updated since a later routine will need to perform another
 * test.  If the region is completely good, we end up returning the same
 * last block number.
 *
 * Set blkno to -1 if we encounter no errors.  This is an invalid block number
 * since we don't ever expect logs to get this large.
 */
STATIC int
xlog_find_verify_cycle(
	xlog_t		*log,
	xfs_daddr_t	start_blk,
	int		nbblks,
	uint		stop_on_cycle_no,
	xfs_daddr_t	*new_blk)
{
	xfs_daddr_t	i, j;
	uint		cycle;
	xfs_buf_t	*bp;
	xfs_daddr_t	bufblks;
	xfs_caddr_t	buf = NULL;
	int		error = 0;

	bufblks = 1 << ffs(nbblks);

	while (!(bp = xlog_get_bp(log, bufblks))) {
		/* can't get enough memory to do everything in one big buffer */
		bufblks >>= 1;
		if (bufblks <= log->l_sectbb_log)
			return ENOMEM;
	}

	for (i = start_blk; i < start_blk + nbblks; i += bufblks) {
		int	bcount;

		bcount = min(bufblks, (start_blk + nbblks - i));

		if ((error = xlog_bread(log, i, bcount, bp)))
			goto out;

		buf = xlog_align(log, i, bcount, bp);
		for (j = 0; j < bcount; j++) {
			cycle = GET_CYCLE(buf, ARCH_CONVERT);
			if (cycle == stop_on_cycle_no) {
				*new_blk = i+j;
				goto out;
			}

			buf += BBSIZE;
		}
	}

	*new_blk = -1;

out:
	xlog_put_bp(bp);
	return error;
}

/*
 * Potentially backup over partial log record write.
 *
 * In the typical case, last_blk is the number of the block directly after
 * a good log record.  Therefore, we subtract one to get the block number
 * of the last block in the given buffer.  extra_bblks contains the number
 * of blocks we would have read on a previous read.  This happens when the
 * last log record is split over the end of the physical log.
 *
 * extra_bblks is the number of blocks potentially verified on a previous
 * call to this routine.
 */
STATIC int
xlog_find_verify_log_record(
	xlog_t			*log,
	xfs_daddr_t		start_blk,
	xfs_daddr_t		*last_blk,
	int			extra_bblks)
{
	xfs_daddr_t		i;
	xfs_buf_t		*bp;
	xfs_caddr_t		offset = NULL;
	xlog_rec_header_t	*head = NULL;
	int			error = 0;
	int			smallmem = 0;
	int			num_blks = *last_blk - start_blk;
	int			xhdrs;

	ASSERT(start_blk != 0 || *last_blk != start_blk);

	if (!(bp = xlog_get_bp(log, num_blks))) {
		if (!(bp = xlog_get_bp(log, 1)))
			return ENOMEM;
		smallmem = 1;
	} else {
		if ((error = xlog_bread(log, start_blk, num_blks, bp)))
			goto out;
		offset = xlog_align(log, start_blk, num_blks, bp);
		offset += ((num_blks - 1) << BBSHIFT);
	}

	for (i = (*last_blk) - 1; i >= 0; i--) {
		if (i < start_blk) {
			/* valid log record not found */
			xlog_warn(
		"XFS: Log inconsistent (didn't find previous header)");
			ASSERT(0);
			error = XFS_ERROR(EIO);
			goto out;
		}

		if (smallmem) {
			if ((error = xlog_bread(log, i, 1, bp)))
				goto out;
			offset = xlog_align(log, i, 1, bp);
		}

		head = (xlog_rec_header_t *)offset;

		if (XLOG_HEADER_MAGIC_NUM ==
		    INT_GET(head->h_magicno, ARCH_CONVERT))
			break;

		if (!smallmem)
			offset -= BBSIZE;
	}

	/*
	 * We hit the beginning of the physical log & still no header.  Return
	 * to caller.  If caller can handle a return of -1, then this routine
	 * will be called again for the end of the physical log.
	 */
	if (i == -1) {
		error = -1;
		goto out;
	}

	/*
	 * We have the final block of the good log (the first block
	 * of the log record _before_ the head. So we check the uuid.
	 */
	if ((error = xlog_header_check_mount(log->l_mp, head)))
		goto out;

	/*
	 * We may have found a log record header before we expected one.
	 * last_blk will be the 1st block # with a given cycle #.  We may end
	 * up reading an entire log record.  In this case, we don't want to
	 * reset last_blk.  Only when last_blk points in the middle of a log
	 * record do we update last_blk.
	 */
	if (XFS_SB_VERSION_HASLOGV2(&log->l_mp->m_sb)) {
		uint	h_size = INT_GET(head->h_size, ARCH_CONVERT);

		xhdrs = h_size / XLOG_HEADER_CYCLE_SIZE;
		if (h_size % XLOG_HEADER_CYCLE_SIZE)
			xhdrs++;
	} else {
		xhdrs = 1;
	}

	if (*last_blk - i + extra_bblks
			!= BTOBB(INT_GET(head->h_len, ARCH_CONVERT)) + xhdrs)
		*last_blk = i;

out:
	xlog_put_bp(bp);
	return error;
}

/*
 * Head is defined to be the point of the log where the next log write
 * write could go.  This means that incomplete LR writes at the end are
 * eliminated when calculating the head.  We aren't guaranteed that previous
 * LR have complete transactions.  We only know that a cycle number of
 * current cycle number -1 won't be present in the log if we start writing
 * from our current block number.
 *
 * last_blk contains the block number of the first block with a given
 * cycle number.
 *
 * Return: zero if normal, non-zero if error.
 */
int
xlog_find_head(
	xlog_t 		*log,
	xfs_daddr_t	*return_head_blk)
{
	xfs_buf_t	*bp;
	xfs_caddr_t	offset;
	xfs_daddr_t	new_blk, first_blk, start_blk, last_blk, head_blk;
	int		num_scan_bblks;
	uint		first_half_cycle, last_half_cycle;
	uint		stop_on_cycle;
	int		error, log_bbnum = log->l_logBBsize;

	/* Is the end of the log device zeroed? */
	if ((error = xlog_find_zeroed(log, &first_blk)) == -1) {
		*return_head_blk = first_blk;

		/* Is the whole lot zeroed? */
		if (!first_blk) {
			/* Linux XFS shouldn't generate totally zeroed logs -
			 * mkfs etc write a dummy unmount record to a fresh
			 * log so we can store the uuid in there
			 */
			xlog_warn("XFS: totally zeroed log");
		}

		return 0;
	} else if (error) {
		xlog_warn("XFS: empty log check failed");
		return error;
	}

	first_blk = 0;			/* get cycle # of 1st block */
	bp = xlog_get_bp(log, 1);
	if (!bp)
		return ENOMEM;
	if ((error = xlog_bread(log, 0, 1, bp)))
		goto bp_err;
	offset = xlog_align(log, 0, 1, bp);
	first_half_cycle = GET_CYCLE(offset, ARCH_CONVERT);

	last_blk = head_blk = log_bbnum - 1;	/* get cycle # of last block */
	if ((error = xlog_bread(log, last_blk, 1, bp)))
		goto bp_err;
	offset = xlog_align(log, last_blk, 1, bp);
	last_half_cycle = GET_CYCLE(offset, ARCH_CONVERT);
	ASSERT(last_half_cycle != 0);

	/*
	 * If the 1st half cycle number is equal to the last half cycle number,
	 * then the entire log is stamped with the same cycle number.  In this
	 * case, head_blk can't be set to zero (which makes sense).  The below
	 * math doesn't work out properly with head_blk equal to zero.  Instead,
	 * we set it to log_bbnum which is an invalid block number, but this
	 * value makes the math correct.  If head_blk doesn't changed through
	 * all the tests below, *head_blk is set to zero at the very end rather
	 * than log_bbnum.  In a sense, log_bbnum and zero are the same block
	 * in a circular file.
	 */
	if (first_half_cycle == last_half_cycle) {
		/*
		 * In this case we believe that the entire log should have
		 * cycle number last_half_cycle.  We need to scan backwards
		 * from the end verifying that there are no holes still
		 * containing last_half_cycle - 1.  If we find such a hole,
		 * then the start of that hole will be the new head.  The
		 * simple case looks like
		 *        x | x ... | x - 1 | x
		 * Another case that fits this picture would be
		 *        x | x + 1 | x ... | x
		 * In this case the head really is somwhere at the end of the
		 * log, as one of the latest writes at the beginning was
		 * incomplete.
		 * One more case is
		 *        x | x + 1 | x ... | x - 1 | x
		 * This is really the combination of the above two cases, and
		 * the head has to end up at the start of the x-1 hole at the
		 * end of the log.
		 *
		 * In the 256k log case, we will read from the beginning to the
		 * end of the log and search for cycle numbers equal to x-1.
		 * We don't worry about the x+1 blocks that we encounter,
		 * because we know that they cannot be the head since the log
		 * started with x.
		 */
		head_blk = log_bbnum;
		stop_on_cycle = last_half_cycle - 1;
	} else {
		/*
		 * In this case we want to find the first block with cycle
		 * number matching last_half_cycle.  We expect the log to be
		 * some variation on
		 *        x + 1 ... | x ...
		 * The first block with cycle number x (last_half_cycle) will
		 * be where the new head belongs.  First we do a binary search
		 * for the first occurrence of last_half_cycle.  The binary
		 * search may not be totally accurate, so then we scan back
		 * from there looking for occurrences of last_half_cycle before
		 * us.  If that backwards scan wraps around the beginning of
		 * the log, then we look for occurrences of last_half_cycle - 1
		 * at the end of the log.  The cases we're looking for look
		 * like
		 *        x + 1 ... | x | x + 1 | x ...
		 *                               ^ binary search stopped here
		 * or
		 *        x + 1 ... | x ... | x - 1 | x
		 *        <---------> less than scan distance
		 */
		stop_on_cycle = last_half_cycle;
		if ((error = xlog_find_cycle_start(log, bp, first_blk,
						&head_blk, last_half_cycle)))
			goto bp_err;
	}

	/*
	 * Now validate the answer.  Scan back some number of maximum possible
	 * blocks and make sure each one has the expected cycle number.  The
	 * maximum is determined by the total possible amount of buffering
	 * in the in-core log.  The following number can be made tighter if
	 * we actually look at the block size of the filesystem.
	 */
	num_scan_bblks = XLOG_TOTAL_REC_SHIFT(log);
	if (head_blk >= num_scan_bblks) {
		/*
		 * We are guaranteed that the entire check can be performed
		 * in one buffer.
		 */
		start_blk = head_blk - num_scan_bblks;
		if ((error = xlog_find_verify_cycle(log,
						start_blk, num_scan_bblks,
						stop_on_cycle, &new_blk)))
			goto bp_err;
		if (new_blk != -1)
			head_blk = new_blk;
	} else {		/* need to read 2 parts of log */
		/*
		 * We are going to scan backwards in the log in two parts.
		 * First we scan the physical end of the log.  In this part
		 * of the log, we are looking for blocks with cycle number
		 * last_half_cycle - 1.
		 * If we find one, then we know that the log starts there, as
		 * we've found a hole that didn't get written in going around
		 * the end of the physical log.  The simple case for this is
		 *        x + 1 ... | x ... | x - 1 | x
		 *        <---------> less than scan distance
		 * If all of the blocks at the end of the log have cycle number
		 * last_half_cycle, then we check the blocks at the start of
		 * the log looking for occurrences of last_half_cycle.  If we
		 * find one, then our current estimate for the location of the
		 * first occurrence of last_half_cycle is wrong and we move
		 * back to the hole we've found.  This case looks like
		 *        x + 1 ... | x | x + 1 | x ...
		 *                               ^ binary search stopped here
		 * Another case we need to handle that only occurs in 256k
		 * logs is
		 *        x + 1 ... | x ... | x+1 | x ...
		 *                   ^ binary search stops here
		 * In a 256k log, the scan at the end of the log will see the
		 * x + 1 blocks.  We need to skip past those since that is
		 * certainly not the head of the log.  By searching for
		 * last_half_cycle-1 we accomplish that.
		 */
		start_blk = log_bbnum - num_scan_bblks + head_blk;
		ASSERT(head_blk <= INT_MAX &&
			(xfs_daddr_t) num_scan_bblks - head_blk >= 0);
		if ((error = xlog_find_verify_cycle(log, start_blk,
					num_scan_bblks - (int)head_blk,
					(stop_on_cycle - 1), &new_blk)))
			goto bp_err;
		if (new_blk != -1) {
			head_blk = new_blk;
			goto bad_blk;
		}

		/*
		 * Scan beginning of log now.  The last part of the physical
		 * log is good.  This scan needs to verify that it doesn't find
		 * the last_half_cycle.
		 */
		start_blk = 0;
		ASSERT(head_blk <= INT_MAX);
		if ((error = xlog_find_verify_cycle(log,
					start_blk, (int)head_blk,
					stop_on_cycle, &new_blk)))
			goto bp_err;
		if (new_blk != -1)
			head_blk = new_blk;
	}

 bad_blk:
	/*
	 * Now we need to make sure head_blk is not pointing to a block in
	 * the middle of a log record.
	 */
	num_scan_bblks = XLOG_REC_SHIFT(log);
	if (head_blk >= num_scan_bblks) {
		start_blk = head_blk - num_scan_bblks; /* don't read head_blk */

		/* start ptr at last block ptr before head_blk */
		if ((error = xlog_find_verify_log_record(log, start_blk,
							&head_blk, 0)) == -1) {
			error = XFS_ERROR(EIO);
			goto bp_err;
		} else if (error)
			goto bp_err;
	} else {
		start_blk = 0;
		ASSERT(head_blk <= INT_MAX);
		if ((error = xlog_find_verify_log_record(log, start_blk,
							&head_blk, 0)) == -1) {
			/* We hit the beginning of the log during our search */
			start_blk = log_bbnum - num_scan_bblks + head_blk;
			new_blk = log_bbnum;
			ASSERT(start_blk <= INT_MAX &&
				(xfs_daddr_t) log_bbnum-start_blk >= 0);
			ASSERT(head_blk <= INT_MAX);
			if ((error = xlog_find_verify_log_record(log,
							start_blk, &new_blk,
							(int)head_blk)) == -1) {
				error = XFS_ERROR(EIO);
				goto bp_err;
			} else if (error)
				goto bp_err;
			if (new_blk != log_bbnum)
				head_blk = new_blk;
		} else if (error)
			goto bp_err;
	}

	xlog_put_bp(bp);
	if (head_blk == log_bbnum)
		*return_head_blk = 0;
	else
		*return_head_blk = head_blk;
	/*
	 * When returning here, we have a good block number.  Bad block
	 * means that during a previous crash, we didn't have a clean break
	 * from cycle number N to cycle number N-1.  In this case, we need
	 * to find the first block with cycle number N-1.
	 */
	return 0;

 bp_err:
	xlog_put_bp(bp);

	if (error)
	    xlog_warn("XFS: failed to find log head");
	return error;
}

/*
 * Find the sync block number or the tail of the log.
 *
 * This will be the block number of the last record to have its
 * associated buffers synced to disk.  Every log record header has
 * a sync lsn embedded in it.  LSNs hold block numbers, so it is easy
 * to get a sync block number.  The only concern is to figure out which
 * log record header to believe.
 *
 * The following algorithm uses the log record header with the largest
 * lsn.  The entire log record does not need to be valid.  We only care
 * that the header is valid.
 *
 * We could speed up search by using current head_blk buffer, but it is not
 * available.
 */
int
xlog_find_tail(
	xlog_t			*log,
	xfs_daddr_t		*head_blk,
	xfs_daddr_t		*tail_blk,
	int			readonly)
{
	xlog_rec_header_t	*rhead;
	xlog_op_header_t	*op_head;
	xfs_caddr_t		offset = NULL;
	xfs_buf_t		*bp;
	int			error, i, found;
	xfs_daddr_t		umount_data_blk;
	xfs_daddr_t		after_umount_blk;
	xfs_lsn_t		tail_lsn;
	int			hblks;

	found = 0;

	/*
	 * Find previous log record
	 */
	if ((error = xlog_find_head(log, head_blk)))
		return error;

	bp = xlog_get_bp(log, 1);
	if (!bp)
		return ENOMEM;
	if (*head_blk == 0) {				/* special case */
		if ((error = xlog_bread(log, 0, 1, bp)))
			goto bread_err;
		offset = xlog_align(log, 0, 1, bp);
		if (GET_CYCLE(offset, ARCH_CONVERT) == 0) {
			*tail_blk = 0;
			/* leave all other log inited values alone */
			goto exit;
		}
	}

	/*
	 * Search backwards looking for log record header block
	 */
	ASSERT(*head_blk < INT_MAX);
	for (i = (int)(*head_blk) - 1; i >= 0; i--) {
		if ((error = xlog_bread(log, i, 1, bp)))
			goto bread_err;
		offset = xlog_align(log, i, 1, bp);
		if (XLOG_HEADER_MAGIC_NUM ==
		    INT_GET(*(uint *)offset, ARCH_CONVERT)) {
			found = 1;
			break;
		}
	}
	/*
	 * If we haven't found the log record header block, start looking
	 * again from the end of the physical log.  XXXmiken: There should be
	 * a check here to make sure we didn't search more than N blocks in
	 * the previous code.
	 */
	if (!found) {
		for (i = log->l_logBBsize - 1; i >= (int)(*head_blk); i--) {
			if ((error = xlog_bread(log, i, 1, bp)))
				goto bread_err;
			offset = xlog_align(log, i, 1, bp);
			if (XLOG_HEADER_MAGIC_NUM ==
			    INT_GET(*(uint*)offset, ARCH_CONVERT)) {
				found = 2;
				break;
			}
		}
	}
	if (!found) {
		xlog_warn("XFS: xlog_find_tail: couldn't find sync record");
		ASSERT(0);
		return XFS_ERROR(EIO);
	}

	/* find blk_no of tail of log */
	rhead = (xlog_rec_header_t *)offset;
	*tail_blk = BLOCK_LSN(rhead->h_tail_lsn, ARCH_CONVERT);

	/*
	 * Reset log values according to the state of the log when we
	 * crashed.  In the case where head_blk == 0, we bump curr_cycle
	 * one because the next write starts a new cycle rather than
	 * continuing the cycle of the last good log record.  At this
	 * point we have guaranteed that all partial log records have been
	 * accounted for.  Therefore, we know that the last good log record
	 * written was complete and ended exactly on the end boundary
	 * of the physical log.
	 */
	log->l_prev_block = i;
	log->l_curr_block = (int)*head_blk;
	log->l_curr_cycle = INT_GET(rhead->h_cycle, ARCH_CONVERT);
	if (found == 2)
		log->l_curr_cycle++;
	log->l_tail_lsn = INT_GET(rhead->h_tail_lsn, ARCH_CONVERT);
	log->l_last_sync_lsn = INT_GET(rhead->h_lsn, ARCH_CONVERT);
	log->l_grant_reserve_cycle = log->l_curr_cycle;
	log->l_grant_reserve_bytes = BBTOB(log->l_curr_block);
	log->l_grant_write_cycle = log->l_curr_cycle;
	log->l_grant_write_bytes = BBTOB(log->l_curr_block);

	/*
	 * Look for unmount record.  If we find it, then we know there
	 * was a clean unmount.  Since 'i' could be the last block in
	 * the physical log, we convert to a log block before comparing
	 * to the head_blk.
	 *
	 * Save the current tail lsn to use to pass to
	 * xlog_clear_stale_blocks() below.  We won't want to clear the
	 * unmount record if there is one, so we pass the lsn of the
	 * unmount record rather than the block after it.
	 */
	if (XFS_SB_VERSION_HASLOGV2(&log->l_mp->m_sb)) {
		int	h_size = INT_GET(rhead->h_size, ARCH_CONVERT);
		int	h_version = INT_GET(rhead->h_version, ARCH_CONVERT);

		if ((h_version & XLOG_VERSION_2) &&
		    (h_size > XLOG_HEADER_CYCLE_SIZE)) {
			hblks = h_size / XLOG_HEADER_CYCLE_SIZE;
			if (h_size % XLOG_HEADER_CYCLE_SIZE)
				hblks++;
		} else {
			hblks = 1;
		}
	} else {
		hblks = 1;
	}
	after_umount_blk = (i + hblks + (int)
		BTOBB(INT_GET(rhead->h_len, ARCH_CONVERT))) % log->l_logBBsize;
	tail_lsn = log->l_tail_lsn;
	if (*head_blk == after_umount_blk &&
	    INT_GET(rhead->h_num_logops, ARCH_CONVERT) == 1) {
		umount_data_blk = (i + hblks) % log->l_logBBsize;
		if ((error = xlog_bread(log, umount_data_blk, 1, bp))) {
			goto bread_err;
		}
		offset = xlog_align(log, umount_data_blk, 1, bp);
		op_head = (xlog_op_header_t *)offset;
		if (op_head->oh_flags & XLOG_UNMOUNT_TRANS) {
			/*
			 * Set tail and last sync so that newly written
			 * log records will point recovery to after the
			 * current unmount record.
			 */
			ASSIGN_ANY_LSN(log->l_tail_lsn, log->l_curr_cycle,
					after_umount_blk, ARCH_NOCONVERT);
			ASSIGN_ANY_LSN(log->l_last_sync_lsn, log->l_curr_cycle,
					after_umount_blk, ARCH_NOCONVERT);
			*tail_blk = after_umount_blk;
		}
	}

	/*
	 * Make sure that there are no blocks in front of the head
	 * with the same cycle number as the head.  This can happen
	 * because we allow multiple outstanding log writes concurrently,
	 * and the later writes might make it out before earlier ones.
	 *
	 * We use the lsn from before modifying it so that we'll never
	 * overwrite the unmount record after a clean unmount.
	 *
	 * Do this only if we are going to recover the filesystem
	 *
	 * NOTE: This used to say "if (!readonly)"
	 * However on Linux, we can & do recover a read-only filesystem.
	 * We only skip recovery if NORECOVERY is specified on mount,
	 * in which case we would not be here.
	 *
	 * But... if the -device- itself is readonly, just skip this.
	 * We can't recover this device anyway, so it won't matter.
	 */
	if (!xfs_readonly_buftarg(log->l_mp->m_logdev_targp)) {
		error = xlog_clear_stale_blocks(log, tail_lsn);
	}

bread_err:
exit:
	xlog_put_bp(bp);

	if (error)
		xlog_warn("XFS: failed to locate log tail");
	return error;
}

/*
 * Is the log zeroed at all?
 *
 * The last binary search should be changed to perform an X block read
 * once X becomes small enough.  You can then search linearly through
 * the X blocks.  This will cut down on the number of reads we need to do.
 *
 * If the log is partially zeroed, this routine will pass back the blkno
 * of the first block with cycle number 0.  It won't have a complete LR
 * preceding it.
 *
 * Return:
 *	0  => the log is completely written to
 *	-1 => use *blk_no as the first block of the log
 *	>0 => error has occurred
 */
int
xlog_find_zeroed(
	xlog_t		*log,
	xfs_daddr_t	*blk_no)
{
	xfs_buf_t	*bp;
	xfs_caddr_t	offset;
	uint	        first_cycle, last_cycle;
	xfs_daddr_t	new_blk, last_blk, start_blk;
	xfs_daddr_t     num_scan_bblks;
	int	        error, log_bbnum = log->l_logBBsize;

	/* check totally zeroed log */
	bp = xlog_get_bp(log, 1);
	if (!bp)
		return ENOMEM;
	if ((error = xlog_bread(log, 0, 1, bp)))
		goto bp_err;
	offset = xlog_align(log, 0, 1, bp);
	first_cycle = GET_CYCLE(offset, ARCH_CONVERT);
	if (first_cycle == 0) {		/* completely zeroed log */
		*blk_no = 0;
		xlog_put_bp(bp);
		return -1;
	}

	/* check partially zeroed log */
	if ((error = xlog_bread(log, log_bbnum-1, 1, bp)))
		goto bp_err;
	offset = xlog_align(log, log_bbnum-1, 1, bp);
	last_cycle = GET_CYCLE(offset, ARCH_CONVERT);
	if (last_cycle != 0) {		/* log completely written to */
		xlog_put_bp(bp);
		return 0;
	} else if (first_cycle != 1) {
		/*
		 * If the cycle of the last block is zero, the cycle of
		 * the first block must be 1. If it's not, maybe we're
		 * not looking at a log... Bail out.
		 */
		xlog_warn("XFS: Log inconsistent or not a log (last==0, first!=1)");
		return XFS_ERROR(EINVAL);
	}

	/* we have a partially zeroed log */
	last_blk = log_bbnum-1;
	if ((error = xlog_find_cycle_start(log, bp, 0, &last_blk, 0)))
		goto bp_err;

	/*
	 * Validate the answer.  Because there is no way to guarantee that
	 * the entire log is made up of log records which are the same size,
	 * we scan over the defined maximum blocks.  At this point, the maximum
	 * is not chosen to mean anything special.   XXXmiken
	 */
	num_scan_bblks = XLOG_TOTAL_REC_SHIFT(log);
	ASSERT(num_scan_bblks <= INT_MAX);

	if (last_blk < num_scan_bblks)
		num_scan_bblks = last_blk;
	start_blk = last_blk - num_scan_bblks;

	/*
	 * We search for any instances of cycle number 0 that occur before
	 * our current estimate of the head.  What we're trying to detect is
	 *        1 ... | 0 | 1 | 0...
	 *                       ^ binary search ends here
	 */
	if ((error = xlog_find_verify_cycle(log, start_blk,
					 (int)num_scan_bblks, 0, &new_blk)))
		goto bp_err;
	if (new_blk != -1)
		last_blk = new_blk;

	/*
	 * Potentially backup over partial log record write.  We don't need
	 * to search the end of the log because we know it is zero.
	 */
	if ((error = xlog_find_verify_log_record(log, start_blk,
				&last_blk, 0)) == -1) {
	    error = XFS_ERROR(EIO);
	    goto bp_err;
	} else if (error)
	    goto bp_err;

	*blk_no = last_blk;
bp_err:
	xlog_put_bp(bp);
	if (error)
		return error;
	return -1;
}

STATIC void
xlog_unpack_data(
	xlog_rec_header_t	*rhead,
	xfs_caddr_t		dp,
	xlog_t			*log)
{
	int			i, j, k;
	xlog_in_core_2_t	*xhdr;

	for (i = 0; i < BTOBB(INT_GET(rhead->h_len, ARCH_CONVERT)) &&
		  i < (XLOG_HEADER_CYCLE_SIZE / BBSIZE); i++) {
		*(uint *)dp = *(uint *)&rhead->h_cycle_data[i];
		dp += BBSIZE;
	}

	if (XFS_SB_VERSION_HASLOGV2(&log->l_mp->m_sb)) {
		xhdr = (xlog_in_core_2_t *)rhead;
		for ( ; i < BTOBB(INT_GET(rhead->h_len, ARCH_CONVERT)); i++) {
			j = i / (XLOG_HEADER_CYCLE_SIZE / BBSIZE);
			k = i % (XLOG_HEADER_CYCLE_SIZE / BBSIZE);
			*(uint *)dp = xhdr[j].hic_xheader.xh_cycle_data[k];
			dp += BBSIZE;
		}
	}

	xlog_unpack_data_checksum(rhead, dp, log);
}

STATIC xlog_recover_t *
xlog_recover_find_tid(
	xlog_recover_t		*q,
	xlog_tid_t    		tid)
{
	xlog_recover_t		*p = q;

	while (p != NULL) {
		if (p->r_log_tid == tid)
		    break;
		p = p->r_next;
	}
	return p;
}

STATIC void
xlog_recover_put_hashq(
	xlog_recover_t	**q,
	xlog_recover_t	*trans)
{
	trans->r_next = *q;
	*q = trans;
}

STATIC void
xlog_recover_new_tid(
	xlog_recover_t		**q,
	xlog_tid_t		tid,
	xfs_lsn_t		lsn)
{
	xlog_recover_t		*trans;

	trans = kmem_zalloc(sizeof(xlog_recover_t), KM_SLEEP);
	trans->r_log_tid   = tid;
	trans->r_lsn	   = lsn;
	xlog_recover_put_hashq(q, trans);
}

STATIC int
xlog_recover_unlink_tid(
	xlog_recover_t		**q,
	xlog_recover_t		*trans)
{
	xlog_recover_t		*tp;
	int			found = 0;

	ASSERT(trans != 0);
	if (trans == *q) {
		*q = (*q)->r_next;
	} else {
		tp = *q;
		while (tp != 0) {
			if (tp->r_next == trans) {
				found = 1;
				break;
			}
			tp = tp->r_next;
		}
		if (!found) {
			xlog_warn(
			     "XFS: xlog_recover_unlink_tid: trans not found");
			ASSERT(0);
			return XFS_ERROR(EIO);
		}
		tp->r_next = tp->r_next->r_next;
	}
	return 0;
}

/*
 * Free up any resources allocated by the transaction
 *
 * Remember that EFIs, EFDs, and IUNLINKs are handled later.
 */
STATIC void
xlog_recover_free_trans(
	xlog_recover_t		*trans)
{
	xlog_recover_item_t	*first_item, *item, *free_item;
	int			i;

	item = first_item = trans->r_itemq;
	do {
		free_item = item;
		item = item->ri_next;
		 /* Free the regions in the item. */
		for (i = 0; i < free_item->ri_cnt; i++) {
			kmem_free(free_item->ri_buf[i].i_addr,
				  free_item->ri_buf[i].i_len);
		}
		/* Free the item itself */
		kmem_free(free_item->ri_buf,
			  (free_item->ri_total * sizeof(xfs_log_iovec_t)));
		kmem_free(free_item, sizeof(xlog_recover_item_t));
	} while (first_item != item);
	/* Free the transaction recover structure */
	kmem_free(trans, sizeof(xlog_recover_t));
}

STATIC int
xlog_recover_commit_trans(
	xlog_t			*log,
	xlog_recover_t		**q,
	xlog_recover_t		*trans,
	int			pass)
{
	int			error;

	if ((error = xlog_recover_unlink_tid(q, trans)))
		return error;
	if ((error = xlog_recover_do_trans(log, trans, pass)))
		return error;
	xlog_recover_free_trans(trans);			/* no error */
	return 0;
}

STATIC void
xlog_recover_insert_item_backq(
	xlog_recover_item_t	**q,
	xlog_recover_item_t	*item)
{
	if (*q == 0) {
		item->ri_prev = item->ri_next = item;
		*q = item;
	} else {
		item->ri_next		= *q;
		item->ri_prev		= (*q)->ri_prev;
		(*q)->ri_prev		= item;
		item->ri_prev->ri_next	= item;
	}
}

STATIC void
xlog_recover_add_item(
	xlog_recover_item_t	**itemq)
{
	xlog_recover_item_t	*item;

	item = kmem_zalloc(sizeof(xlog_recover_item_t), 0);
	xlog_recover_insert_item_backq(itemq, item);
}

STATIC int
xlog_recover_add_to_cont_trans(
	xlog_recover_t		*trans,
	xfs_caddr_t		dp,
	int			len)
{
	xlog_recover_item_t	*item;
	xfs_caddr_t		ptr, old_ptr;
	int			old_len;

	item = trans->r_itemq;
	if (item == 0) {
		/* finish copying rest of trans header */
		xlog_recover_add_item(&trans->r_itemq);
		ptr = (xfs_caddr_t) &trans->r_theader +
				sizeof(xfs_trans_header_t) - len;
		memcpy(ptr, dp, len); /* d, s, l */
		return 0;
	}
	item = item->ri_prev;

	old_ptr = item->ri_buf[item->ri_cnt-1].i_addr;
	old_len = item->ri_buf[item->ri_cnt-1].i_len;

	ptr = kmem_realloc(old_ptr, len+old_len, old_len, 0);
	memcpy(&ptr[old_len], dp, len); /* d, s, l */
	item->ri_buf[item->ri_cnt-1].i_len += len;
	item->ri_buf[item->ri_cnt-1].i_addr = ptr;
	return 0;
}

/*
 * The next region to add is the start of a new region.  It could be
 * a whole region or it could be the first part of a new region.  Because
 * of this, the assumption here is that the type and size fields of all
 * format structures fit into the first 32 bits of the structure.
 *
 * This works because all regions must be 32 bit aligned.  Therefore, we
 * either have both fields or we have neither field.  In the case we have
 * neither field, the data part of the region is zero length.  We only have
 * a log_op_header and can throw away the header since a new one will appear
 * later.  If we have at least 4 bytes, then we can determine how many regions
 * will appear in the current log item.
 */
STATIC int
xlog_recover_add_to_trans(
	xlog_recover_t		*trans,
	xfs_caddr_t		dp,
	int			len)
{
	xfs_inode_log_format_t	*in_f;			/* any will do */
	xlog_recover_item_t	*item;
	xfs_caddr_t		ptr;

	if (!len)
		return 0;
	item = trans->r_itemq;
	if (item == 0) {
		ASSERT(*(uint *)dp == XFS_TRANS_HEADER_MAGIC);
		if (len == sizeof(xfs_trans_header_t))
			xlog_recover_add_item(&trans->r_itemq);
		memcpy(&trans->r_theader, dp, len); /* d, s, l */
		return 0;
	}

	ptr = kmem_alloc(len, KM_SLEEP);
	memcpy(ptr, dp, len);
	in_f = (xfs_inode_log_format_t *)ptr;

	if (item->ri_prev->ri_total != 0 &&
	     item->ri_prev->ri_total == item->ri_prev->ri_cnt) {
		xlog_recover_add_item(&trans->r_itemq);
	}
	item = trans->r_itemq;
	item = item->ri_prev;

	if (item->ri_total == 0) {		/* first region to be added */
		item->ri_total	= in_f->ilf_size;
		ASSERT(item->ri_total <= XLOG_MAX_REGIONS_IN_ITEM);
		item->ri_buf = kmem_zalloc((item->ri_total *
					    sizeof(xfs_log_iovec_t)), 0);
	}
	ASSERT(item->ri_total > item->ri_cnt);
	/* Description region is ri_buf[0] */
	item->ri_buf[item->ri_cnt].i_addr = ptr;
	item->ri_buf[item->ri_cnt].i_len  = len;
	item->ri_cnt++;
	return 0;
}

STATIC int
xlog_recover_unmount_trans(
	xlog_recover_t	*trans)
{
	/* Do nothing now */
	xlog_warn("XFS: xlog_recover_unmount_trans: Unmount LR");
	return 0;
}

/*
 * There are two valid states of the r_state field.  0 indicates that the
 * transaction structure is in a normal state.  We have either seen the
 * start of the transaction or the last operation we added was not a partial
 * operation.  If the last operation we added to the transaction was a
 * partial operation, we need to mark r_state with XLOG_WAS_CONT_TRANS.
 *
 * NOTE: skip LRs with 0 data length.
 */
STATIC int
xlog_recover_process_data(
	xlog_t			*log,
	xlog_recover_t		*rhash[],
	xlog_rec_header_t	*rhead,
	xfs_caddr_t		dp,
	int			pass)
{
	xfs_caddr_t		lp;
	int			num_logops;
	xlog_op_header_t	*ohead;
	xlog_recover_t		*trans;
	xlog_tid_t		tid;
	int			error;
	unsigned long		hash;
	uint			flags;

	lp = dp + INT_GET(rhead->h_len, ARCH_CONVERT);
	num_logops = INT_GET(rhead->h_num_logops, ARCH_CONVERT);

	/* check the log format matches our own - else we can't recover */
	if (xlog_header_check_recover(log->l_mp, rhead))
		return (XFS_ERROR(EIO));

	while ((dp < lp) && num_logops) {
		ASSERT(dp + sizeof(xlog_op_header_t) <= lp);
		ohead = (xlog_op_header_t *)dp;
		dp += sizeof(xlog_op_header_t);
		if (ohead->oh_clientid != XFS_TRANSACTION &&
		    ohead->oh_clientid != XFS_LOG) {
			xlog_warn(
		"XFS: xlog_recover_process_data: bad clientid");
			ASSERT(0);
			return (XFS_ERROR(EIO));
		}
		tid = INT_GET(ohead->oh_tid, ARCH_CONVERT);
		hash = XLOG_RHASH(tid);
		trans = xlog_recover_find_tid(rhash[hash], tid);
		if (trans == NULL) {		   /* not found; add new tid */
			if (ohead->oh_flags & XLOG_START_TRANS)
				xlog_recover_new_tid(&rhash[hash], tid,
					INT_GET(rhead->h_lsn, ARCH_CONVERT));
		} else {
			ASSERT(dp+INT_GET(ohead->oh_len, ARCH_CONVERT) <= lp);
			flags = ohead->oh_flags & ~XLOG_END_TRANS;
			if (flags & XLOG_WAS_CONT_TRANS)
				flags &= ~XLOG_CONTINUE_TRANS;
			switch (flags) {
			case XLOG_COMMIT_TRANS:
				error = xlog_recover_commit_trans(log,
						&rhash[hash], trans, pass);
				break;
			case XLOG_UNMOUNT_TRANS:
				error = xlog_recover_unmount_trans(trans);
				break;
			case XLOG_WAS_CONT_TRANS:
				error = xlog_recover_add_to_cont_trans(trans,
						dp, INT_GET(ohead->oh_len,
							ARCH_CONVERT));
				break;
			case XLOG_START_TRANS:
				xlog_warn(
			"XFS: xlog_recover_process_data: bad transaction");
				ASSERT(0);
				error = XFS_ERROR(EIO);
				break;
			case 0:
			case XLOG_CONTINUE_TRANS:
				error = xlog_recover_add_to_trans(trans,
						dp, INT_GET(ohead->oh_len,
							ARCH_CONVERT));
				break;
			default:
				xlog_warn(
			"XFS: xlog_recover_process_data: bad flag");
				ASSERT(0);
				error = XFS_ERROR(EIO);
				break;
			}
			if (error)
				return error;
		}
		dp += INT_GET(ohead->oh_len, ARCH_CONVERT);
		num_logops--;
	}
	return 0;
}

STATIC int
xlog_valid_rec_header(
	xlog_t			*log,
	xlog_rec_header_t	*rhead,
	xfs_daddr_t		blkno)
{
	int			bblks;

	if (unlikely(
	    (INT_GET(rhead->h_magicno, ARCH_CONVERT) !=
			XLOG_HEADER_MAGIC_NUM))) {
		XFS_ERROR_REPORT("xlog_valid_rec_header(1)",
				XFS_ERRLEVEL_LOW, log->l_mp);
		return XFS_ERROR(EFSCORRUPTED);
	}
	if (unlikely(
	    (INT_ISZERO(rhead->h_version, ARCH_CONVERT) ||
	    (INT_GET(rhead->h_version, ARCH_CONVERT) &
			(~XLOG_VERSION_OKBITS)) != 0))) {
		xlog_warn("XFS: %s: unrecognised log version (%d).",
			__FUNCTION__, INT_GET(rhead->h_version, ARCH_CONVERT));
		return XFS_ERROR(EIO);
	}

	/* LR body must have data or it wouldn't have been written */
	bblks = INT_GET(rhead->h_len, ARCH_CONVERT);
	if (unlikely( bblks <= 0 || bblks > INT_MAX )) {
		XFS_ERROR_REPORT("xlog_valid_rec_header(2)",
				XFS_ERRLEVEL_LOW, log->l_mp);
		return XFS_ERROR(EFSCORRUPTED);
	}
	if (unlikely( blkno > log->l_logBBsize || blkno > INT_MAX )) {
		XFS_ERROR_REPORT("xlog_valid_rec_header(3)",
				XFS_ERRLEVEL_LOW, log->l_mp);
		return XFS_ERROR(EFSCORRUPTED);
	}
	return 0;
}

/*
 * Read the log from tail to head and process the log records found.
 * Handle the two cases where the tail and head are in the same cycle
 * and where the active portion of the log wraps around the end of
 * the physical log separately.  The pass parameter is passed through
 * to the routines called to process the data and is not looked at
 * here.
 */
int
xlog_do_recovery_pass(
	xlog_t			*log,
	xfs_daddr_t		head_blk,
	xfs_daddr_t		tail_blk,
	int			pass)
{
	xlog_rec_header_t	*rhead;
	xfs_daddr_t		blk_no;
	xfs_caddr_t		bufaddr, offset;
	xfs_buf_t		*hbp, *dbp;
	int			error = 0, h_size;
	int			bblks, split_bblks;
	int			hblks, split_hblks, wrapped_hblks;
	xlog_recover_t		*rhash[XLOG_RHASH_SIZE];

	ASSERT(head_blk != tail_blk);

	/*
	 * Read the header of the tail block and get the iclog buffer size from
	 * h_size.  Use this to tell how many sectors make up the log header.
	 */
	if (XFS_SB_VERSION_HASLOGV2(&log->l_mp->m_sb)) {
		/*
		 * When using variable length iclogs, read first sector of
		 * iclog header and extract the header size from it.  Get a
		 * new hbp that is the correct size.
		 */
		hbp = xlog_get_bp(log, 1);
		if (!hbp)
			return ENOMEM;
		if ((error = xlog_bread(log, tail_blk, 1, hbp)))
			goto bread_err1;
		offset = xlog_align(log, tail_blk, 1, hbp);
		rhead = (xlog_rec_header_t *)offset;
		error = xlog_valid_rec_header(log, rhead, tail_blk);
		if (error)
			goto bread_err1;
		h_size = INT_GET(rhead->h_size, ARCH_CONVERT);
		if ((INT_GET(rhead->h_version, ARCH_CONVERT)
				& XLOG_VERSION_2) &&
		    (h_size > XLOG_HEADER_CYCLE_SIZE)) {
			hblks = h_size / XLOG_HEADER_CYCLE_SIZE;
			if (h_size % XLOG_HEADER_CYCLE_SIZE)
				hblks++;
			xlog_put_bp(hbp);
			hbp = xlog_get_bp(log, hblks);
		} else {
			hblks = 1;
		}
	} else {
		ASSERT(log->l_sectbb_log == 0);
		hblks = 1;
		hbp = xlog_get_bp(log, 1);
		h_size = XLOG_BIG_RECORD_BSIZE;
	}

	if (!hbp)
		return ENOMEM;
	dbp = xlog_get_bp(log, BTOBB(h_size));
	if (!dbp) {
		xlog_put_bp(hbp);
		return ENOMEM;
	}

	memset(rhash, 0, sizeof(rhash));
	if (tail_blk <= head_blk) {
		for (blk_no = tail_blk; blk_no < head_blk; ) {
			if ((error = xlog_bread(log, blk_no, hblks, hbp)))
				goto bread_err2;
			offset = xlog_align(log, blk_no, hblks, hbp);
			rhead = (xlog_rec_header_t *)offset;
			error = xlog_valid_rec_header(log, rhead, blk_no);
			if (error)
				goto bread_err2;

			/* blocks in data section */
			bblks = (int)BTOBB(INT_GET(rhead->h_len, ARCH_CONVERT));
			error = xlog_bread(log, blk_no + hblks, bblks, dbp);
			if (error)
				goto bread_err2;
			offset = xlog_align(log, blk_no + hblks, bblks, dbp);
			xlog_unpack_data(rhead, offset, log);
			if ((error = xlog_recover_process_data(log,
						rhash, rhead, offset, pass)))
				goto bread_err2;
			blk_no += bblks + hblks;
		}
	} else {
		/*
		 * Perform recovery around the end of the physical log.
		 * When the head is not on the same cycle number as the tail,
		 * we can't do a sequential recovery as above.
		 */
		blk_no = tail_blk;
		while (blk_no < log->l_logBBsize) {
			/*
			 * Check for header wrapping around physical end-of-log
			 */
			offset = NULL;
			split_hblks = 0;
			wrapped_hblks = 0;
			if (blk_no + hblks <= log->l_logBBsize) {
				/* Read header in one read */
				error = xlog_bread(log, blk_no, hblks, hbp);
				if (error)
					goto bread_err2;
				offset = xlog_align(log, blk_no, hblks, hbp);
			} else {
				/* This LR is split across physical log end */
				if (blk_no != log->l_logBBsize) {
					/* some data before physical log end */
					ASSERT(blk_no <= INT_MAX);
					split_hblks = log->l_logBBsize - (int)blk_no;
					ASSERT(split_hblks > 0);
					if ((error = xlog_bread(log, blk_no,
							split_hblks, hbp)))
						goto bread_err2;
					offset = xlog_align(log, blk_no,
							split_hblks, hbp);
				}
				/*
				 * Note: this black magic still works with
				 * large sector sizes (non-512) only because:
				 * - we increased the buffer size originally
				 *   by 1 sector giving us enough extra space
				 *   for the second read;
				 * - the log start is guaranteed to be sector
				 *   aligned;
				 * - we read the log end (LR header start)
				 *   _first_, then the log start (LR header end)
				 *   - order is important.
				 */
				bufaddr = XFS_BUF_PTR(hbp);
				XFS_BUF_SET_PTR(hbp,
						bufaddr + BBTOB(split_hblks),
						BBTOB(hblks - split_hblks));
				wrapped_hblks = hblks - split_hblks;
				error = xlog_bread(log, 0, wrapped_hblks, hbp);
				if (error)
					goto bread_err2;
				XFS_BUF_SET_PTR(hbp, bufaddr, hblks);
				if (!offset)
					offset = xlog_align(log, 0,
							wrapped_hblks, hbp);
			}
			rhead = (xlog_rec_header_t *)offset;
			error = xlog_valid_rec_header(log, rhead,
						split_hblks ? blk_no : 0);
			if (error)
				goto bread_err2;

			bblks = (int)BTOBB(INT_GET(rhead->h_len, ARCH_CONVERT));
			blk_no += hblks;

			/* Read in data for log record */
			if (blk_no + bblks <= log->l_logBBsize) {
				error = xlog_bread(log, blk_no, bblks, dbp);
				if (error)
					goto bread_err2;
				offset = xlog_align(log, blk_no, bblks, dbp);
			} else {
				/* This log record is split across the
				 * physical end of log */
				offset = NULL;
				split_bblks = 0;
				if (blk_no != log->l_logBBsize) {
					/* some data is before the physical
					 * end of log */
					ASSERT(!wrapped_hblks);
					ASSERT(blk_no <= INT_MAX);
					split_bblks =
						log->l_logBBsize - (int)blk_no;
					ASSERT(split_bblks > 0);
					if ((error = xlog_bread(log, blk_no,
							split_bblks, dbp)))
						goto bread_err2;
					offset = xlog_align(log, blk_no,
							split_bblks, dbp);
				}
				/*
				 * Note: this black magic still works with
				 * large sector sizes (non-512) only because:
				 * - we increased the buffer size originally
				 *   by 1 sector giving us enough extra space
				 *   for the second read;
				 * - the log start is guaranteed to be sector
				 *   aligned;
				 * - we read the log end (LR header start)
				 *   _first_, then the log start (LR header end)
				 *   - order is important.
				 */
				bufaddr = XFS_BUF_PTR(dbp);
				XFS_BUF_SET_PTR(dbp,
						bufaddr + BBTOB(split_bblks),
						BBTOB(bblks - split_bblks));
				if ((error = xlog_bread(log, wrapped_hblks,
						bblks - split_bblks, dbp)))
					goto bread_err2;
				XFS_BUF_SET_PTR(dbp, bufaddr,
						XLOG_BIG_RECORD_BSIZE);
				if (!offset)
					offset = xlog_align(log, wrapped_hblks,
						bblks - split_bblks, dbp);
			}
			xlog_unpack_data(rhead, offset, log);
			if ((error = xlog_recover_process_data(log, rhash,
							rhead, offset, pass)))
				goto bread_err2;
			blk_no += bblks;
		}

		ASSERT(blk_no >= log->l_logBBsize);
		blk_no -= log->l_logBBsize;

		/* read first part of physical log */
		while (blk_no < head_blk) {
			if ((error = xlog_bread(log, blk_no, hblks, hbp)))
				goto bread_err2;
			offset = xlog_align(log, blk_no, hblks, hbp);
			rhead = (xlog_rec_header_t *)offset;
			error = xlog_valid_rec_header(log, rhead, blk_no);
			if (error)
				goto bread_err2;
			bblks = (int)BTOBB(INT_GET(rhead->h_len, ARCH_CONVERT));
			if ((error = xlog_bread(log, blk_no+hblks, bblks, dbp)))
				goto bread_err2;
			offset = xlog_align(log, blk_no+hblks, bblks, dbp);
			xlog_unpack_data(rhead, offset, log);
			if ((error = xlog_recover_process_data(log, rhash,
							rhead, offset, pass)))
				goto bread_err2;
			blk_no += bblks + hblks;
		}
	}

 bread_err2:
	xlog_put_bp(dbp);
 bread_err1:
	xlog_put_bp(hbp);
	return error;
}
