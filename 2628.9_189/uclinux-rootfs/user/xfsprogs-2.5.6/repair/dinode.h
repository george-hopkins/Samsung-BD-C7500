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
#ifndef _XR_DINODE_H
#define _XR_DINODE_H

struct blkmap;

int
verify_agbno(xfs_mount_t	*mp,
		xfs_agnumber_t	agno,
		xfs_agblock_t	agbno);

int
verify_dfsbno(xfs_mount_t	*mp,
		xfs_dfsbno_t	fsbno);

void
convert_extent(
	xfs_bmbt_rec_32_t	*rp,
	xfs_dfiloff_t		*op,	/* starting offset (blockno in file) */
	xfs_dfsbno_t		*sp,	/* starting block (fs blockno) */
	xfs_dfilblks_t		*cp,	/* blockcount */
	int			*fp);	/* extent flag */

int
process_bmbt_reclist(xfs_mount_t	*mp,
		xfs_bmbt_rec_32_t	*rp,
		int			numrecs,
		int			type,
		xfs_ino_t		ino,
		xfs_drfsbno_t		*tot,
		struct blkmap		**blkmapp,
		__uint64_t		*first_key,
		__uint64_t		*last_key,
		int			whichfork);

int
scan_bmbt_reclist(
	xfs_mount_t		*mp,
	xfs_bmbt_rec_32_t	*rp,
	int			numrecs,
	int			type,
	xfs_ino_t		ino,
	xfs_drfsbno_t		*tot,
	int			whichfork);

int
verify_inode_chunk(xfs_mount_t		*mp,
			xfs_ino_t	ino,
			xfs_ino_t	*start_ino);

int	verify_aginode_chunk(xfs_mount_t	*mp,
				xfs_agnumber_t	agno,
				xfs_agino_t	agino,
				xfs_agino_t	*agino_start);

int
clear_dinode(xfs_mount_t *mp, xfs_dinode_t *dino, xfs_ino_t ino_num);

void
update_rootino(xfs_mount_t *mp);

int
process_dinode(xfs_mount_t *mp,
		xfs_dinode_t *dino,
		xfs_agnumber_t agno,
		xfs_agino_t ino,
		int was_free,
		int *dirty,
		int *tossit,
		int *used,
		int check_dirs,
		int check_dups,
		int extra_attr_check,
		int *isa_dir,
		xfs_ino_t *parent);

int
verify_dinode(xfs_mount_t *mp,
		xfs_dinode_t *dino,
		xfs_agnumber_t agno,
		xfs_agino_t ino);

int
verify_uncertain_dinode(xfs_mount_t *mp,
		xfs_dinode_t *dino,
		xfs_agnumber_t agno,
		xfs_agino_t ino);

int
verify_inum(xfs_mount_t		*mp,
		xfs_ino_t	ino);

int
verify_aginum(xfs_mount_t	*mp,
		xfs_agnumber_t	agno,
		xfs_agino_t	agino);

int
process_uncertain_aginodes(xfs_mount_t		*mp,
				xfs_agnumber_t	agno);
void
process_aginodes(xfs_mount_t	*mp,
		xfs_agnumber_t	agno,
		int		check_dirs,
		int		check_dups,
		int		extra_attr_check);

void
check_uncertain_aginodes(xfs_mount_t	*mp,
			xfs_agnumber_t	agno);

xfs_buf_t *
get_agino_buf(xfs_mount_t	*mp,
		xfs_agnumber_t	agno,
		xfs_agino_t	agino,
		xfs_dinode_t	**dipp);

xfs_dfsbno_t
get_bmapi(xfs_mount_t		*mp,
		xfs_dinode_t	*dip,
		xfs_ino_t	ino_num,
		xfs_dfiloff_t	bno,
		int             whichfork );

#endif /* _XR_DINODE_H */
