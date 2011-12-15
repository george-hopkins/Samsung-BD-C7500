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

#include <libxfs.h>
#include "agheader.h"
#include "globals.h"
#include "protos.h"
#include "err_protos.h"


/*
 * copy the fields of a superblock that are present in primary and
 * secondaries -- preserve fields that are different in the primary.
 */
void
copy_sb(xfs_sb_t *source, xfs_sb_t *dest)
{
	xfs_ino_t	rootino;
	xfs_ino_t	rbmino;
	xfs_ino_t	rsumino;
	xfs_ino_t	uquotino;
	xfs_ino_t	gquotino;
	__uint16_t	versionnum;

	rootino = dest->sb_rootino;
	rbmino = dest->sb_rbmino;
	rsumino = dest->sb_rsumino;
	uquotino = dest->sb_uquotino;
	gquotino = dest->sb_gquotino;

	versionnum = dest->sb_versionnum;

	*dest = *source;

	dest->sb_rootino = rootino;
	dest->sb_rbmino = rbmino;
	dest->sb_rsumino = rsumino;
	dest->sb_uquotino = uquotino;
	dest->sb_gquotino = gquotino;

	dest->sb_versionnum = versionnum;

	/*
	 * copy over version bits that are stamped into all
	 * secondaries and cannot be changed at run time in
	 * the primary superblock
	 */
	if (XFS_SB_VERSION_HASDALIGN(source))
		XFS_SB_VERSION_ADDDALIGN(dest);
	if (XFS_SB_VERSION_HASEXTFLGBIT(source))
		XFS_SB_VERSION_ADDEXTFLGBIT(dest);

	/*
	 * these are all supposed to be zero or will get reset anyway
	 */
	dest->sb_icount = 0;
	dest->sb_ifree = 0;
	dest->sb_fdblocks = 0;
	dest->sb_frextents = 0;

	bzero(source->sb_fname, 12);
}

#define BSIZE	(1024 * 1024)

/*
 * find a secondary superblock, copy it into the sb buffer
 */
int
find_secondary_sb(xfs_sb_t *rsb)
{
	xfs_off_t	off;
	xfs_sb_t	*sb;
	xfs_sb_t	bufsb;
	char		*c_bufsb;
	int		done;
	int		i;
	int		dirty;
	int		retval;
	int		bsize;

	do_warn(_("\nattempting to find secondary superblock...\n"));

	sb = (xfs_sb_t *) memalign(MEM_ALIGN, BSIZE);
	if (!sb) {
		do_error(
	_("error finding secondary superblock -- failed to memalign buffer\n"));
		exit(1);
	}

	bzero(&bufsb, sizeof(xfs_sb_t));
	retval = 0;
	dirty = 0;
	bsize = 0;

	/*
	 * skip first sector since we know that's bad
	 */
	for (done = 0, off = XFS_AG_MIN_BYTES; !done ; off += bsize)  {
		/*
		 * read disk 1 MByte at a time.
		 */
		if (lseek64(fs_fd, off, SEEK_SET) != off)  {
			done = 1;
		}

		if (!done && (bsize = read(fs_fd, sb, BSIZE)) == 0)  {
			done = 1;
		}

		do_warn(".");

		/*
		 * check the buffer 512 bytes at a time since
		 * we don't know how big the sectors really are.
		 */
		for (i = 0; !done && i < bsize; i += BBSIZE)  {
			c_bufsb = (char *) sb + i;
			libxfs_xlate_sb(c_bufsb, &bufsb, 1, ARCH_CONVERT,
				XFS_SB_ALL_BITS);

			if (verify_sb(&bufsb, 0) != XR_OK)
				continue;

			do_warn(_("found candidate secondary superblock...\n"));

			/*
			 * found one.  now verify it by looking
			 * for other secondaries.
			 */
			bcopy(&bufsb, rsb, sizeof(xfs_sb_t));
			rsb->sb_inprogress = 0;
			clear_sunit = 1;

			if (verify_set_primary_sb(rsb, 0, &dirty) == XR_OK)  {
				do_warn(
			_("verified secondary superblock...\n"));
				done = 1;
				retval = 1;
			} else  {
				do_warn(
			_("unable to verify superblock, continuing...\n"));
			}
		}
	}

	free(sb);
	return(retval);
}

/*
 * calculate what inode alignment field ought to be
 * based on internal superblock info
 */
int
calc_ino_align(xfs_sb_t *sb)
{
	xfs_extlen_t align;

	align = XFS_INODE_BIG_CLUSTER_SIZE >> sb->sb_blocklog;

	return(align);
}

/*
 * verify a superblock -- does not verify root inode #
 *	can only check that geometry info is internally
 *	consistent.  because of growfs, that's no guarantee
 *	of correctness (e.g. geometry may have changed)
 *
 * fields verified or consistency checked:
 *
 *			sb_magicnum
 *
 *			sb_versionnum
 *
 *			sb_inprogress
 *
 *			sb_blocksize	(as a group)
 *			sb_blocklog
 *
 * geometry info -	sb_dblocks	(as a group)
 *			sb_agcount
 *			sb_agblocks
 *			sb_agblklog
 *
 * inode info -		sb_inodesize	(x-checked with geo info)
 *			sb_inopblock
 *
 * sector size info -
 *			sb_sectsize
 *			sb_sectlog
 *			sb_logsectsize
 *			sb_logsectlog
 *
 * not checked here -
 *			sb_rootino
 *			sb_fname
 *			sb_fpack
 *			sb_logstart
 *			sb_uuid
 *
 *			ALL real-time fields
 *			final 4 summary counters
 */

int
verify_sb(xfs_sb_t *sb, int is_primary_sb)
{
	__uint32_t	bsize;
	xfs_extlen_t	align;
	int		i;

	/* check magic number and version number */

	if (sb->sb_magicnum != XFS_SB_MAGIC)
		return(XR_BAD_MAGIC);

	if (!XFS_SB_GOOD_VERSION(sb))
		return(XR_BAD_VERSION);

	/* does sb think mkfs really finished ? */

	if (is_primary_sb && sb->sb_inprogress == 1)
		return(XR_BAD_INPROGRESS);

	/* check to make sure blocksize is legal 2^N, 9 <= N <= 16 */

	if (sb->sb_blocksize == 0)
		return(XR_BAD_BLOCKSIZE);

	bsize = 1;

	for (i = 0; bsize < sb->sb_blocksize &&
		i < sizeof(sb->sb_blocksize) * NBBY; i++)
		bsize <<= 1;

	if (i < XFS_MIN_BLOCKSIZE_LOG || i > XFS_MAX_BLOCKSIZE_LOG)
		return(XR_BAD_BLOCKSIZE);

	/* check sb blocksize field against sb blocklog field */

	if (i != sb->sb_blocklog)
		return(XR_BAD_BLOCKLOG);

	/* sanity check ag count, size fields against data size field */

	if (sb->sb_dblocks == 0 ||
		sb->sb_dblocks >
			((__uint64_t)sb->sb_agcount * sb->sb_agblocks) ||
		sb->sb_dblocks <
			((__uint64_t)(sb->sb_agcount - 1) * sb->sb_agblocks
			+ XFS_MIN_AG_BLOCKS))
		return(XR_BAD_FS_SIZE_DATA);

	if (sb->sb_agblklog != (__uint8_t)libxfs_log2_roundup(sb->sb_agblocks))
		return(XR_BAD_FS_SIZE_DATA);

	if (sb->sb_inodesize < XFS_DINODE_MIN_SIZE ||
		sb->sb_inodesize > XFS_DINODE_MAX_SIZE ||
		sb->sb_inopblock != howmany(sb->sb_blocksize,sb->sb_inodesize))
		return(XR_BAD_INO_SIZE_DATA);

	/* check to make sure sectorsize is legal 2^N, 9 <= N <= 15 */

	if (sb->sb_sectsize == 0)
		return(XR_BAD_SECT_SIZE_DATA);

	bsize = 1;

	for (i = 0; bsize < sb->sb_sectsize &&
		i < sizeof(sb->sb_sectsize) * NBBY; i++)  {
		bsize <<= 1;
	}

	if (i < XFS_MIN_SECTORSIZE_LOG || i > XFS_MAX_SECTORSIZE_LOG)
		return(XR_BAD_SECT_SIZE_DATA);

	/* check sb sectorsize field against sb sectlog field */

	if (i != sb->sb_sectlog)
		return(XR_BAD_SECT_SIZE_DATA);

	if (XFS_SB_VERSION_HASSECTOR(sb))  {

		/* check to make sure log sector is legal 2^N, 9 <= N <= 15 */

		if (sb->sb_logsectsize == 0)
			return(XR_BAD_SECT_SIZE_DATA);

		bsize = 1;

		for (i = 0; bsize < sb->sb_logsectsize &&
			i < sizeof(sb->sb_logsectsize) * NBBY; i++)  {
			bsize <<= 1;
		}

		if (i < XFS_MIN_SECTORSIZE_LOG || i > XFS_MAX_SECTORSIZE_LOG)
			return(XR_BAD_SECT_SIZE_DATA);

		/* check sb log sectorsize field against sb log sectlog field */

		if (i != sb->sb_logsectlog)
			return(XR_BAD_SECT_SIZE_DATA);
	}

	/*
	 * real-time extent size is always set
	 */
	if (sb->sb_rextsize * sb->sb_blocksize > XFS_MAX_RTEXTSIZE)
		return(XR_BAD_RT_GEO_DATA);

	if (sb->sb_rextsize * sb->sb_blocksize < XFS_MIN_RTEXTSIZE)
			return(XR_BAD_RT_GEO_DATA);

	if (sb->sb_rblocks == 0)  {
		if (sb->sb_rextents != 0)
			return(XR_BAD_RT_GEO_DATA);

		if (sb->sb_rbmblocks != 0)
			return(XR_BAD_RT_GEO_DATA);

		if (sb->sb_rextslog != 0)
			return(XR_BAD_RT_GEO_DATA);

		if (sb->sb_frextents != 0)
			return(XR_BAD_RT_GEO_DATA);
	} else  {
		/*
		 * if we have a real-time partition, sanity-check geometry
		 */
		if (sb->sb_rblocks / sb->sb_rextsize != sb->sb_rextents)
			return(XR_BAD_RT_GEO_DATA);

		if (sb->sb_rextslog !=
				libxfs_highbit32((unsigned int)sb->sb_rextents))
			return(XR_BAD_RT_GEO_DATA);

		if (sb->sb_rbmblocks != (xfs_extlen_t) howmany(sb->sb_rextents,
						NBBY * sb->sb_blocksize))
			return(XR_BAD_RT_GEO_DATA);
	}

	/*
	 * verify correctness of inode alignment if it's there
	 */
	if (XFS_SB_VERSION_HASALIGN(sb))  {
		align = calc_ino_align(sb);

		if (align != sb->sb_inoalignmt)
			return(XR_BAD_INO_ALIGN);
	}

	/*
	 * verify max. % of inodes (sb_imax_pct)
	 */
	if (sb->sb_imax_pct > 100)
		return(XR_BAD_INO_MAX_PCT);

	/*
	 * verify stripe alignment fields if present
	 */
	if (XFS_SB_VERSION_HASDALIGN(sb)) {
		if ((!sb->sb_unit && sb->sb_width) ||
		    (sb->sb_unit && sb->sb_agblocks % sb->sb_unit))
			return(XR_BAD_SB_UNIT);
		if ((sb->sb_unit && !sb->sb_width) ||
		    (sb->sb_width && sb->sb_unit && sb->sb_width % sb->sb_unit))
			return(XR_BAD_SB_WIDTH);
	}

	/*
	 * if shared bit is set, verify that the version number is sane
	 */
	if (XFS_SB_VERSION_HASSHARED(sb))  {
		if (sb->sb_shared_vn > XFS_SB_MAX_SHARED_VN)
			return(XR_BAD_SVN);
	}

	/*
	 * mkfs's that stamped a feature bit besides the ones in the
	 * mask below could leave garbage in the secondary superblock
	 * sectors.  Anything stamping the shared fs bit or better into
	 * the secondaries is ok and should generate clean secondary
	 * superblock sectors.
	 *
	 * check primary and clean secondary superblocks more strictly
	 */
	if (is_primary_sb || sb->sb_versionnum & XR_PART_SECSB_VNMASK)  {
		/*
		 * return errors if shared vn or alignment fields
		 * are set without their feature bits being set
		 */
		if ((!pre_65_beta && (sb->sb_versionnum & XR_PART_SECSB_VNMASK)) ||
		    (pre_65_beta && (sb->sb_versionnum & XR_ALPHA_SECSB_VNMASK))) {
			/*
			 * shared version # and inode alignment fields
			 * should be valid
			 */
			if (sb->sb_shared_vn && !XFS_SB_VERSION_HASSHARED(sb))
				return(XR_BAD_SVN);
			if (sb->sb_inoalignmt && !XFS_SB_VERSION_HASALIGN(sb))
				return(XR_BAD_INO_ALIGN);
		}
		if ((!pre_65_beta &&
		     (sb->sb_versionnum & XR_GOOD_SECSB_VNMASK)) ||
		    (pre_65_beta &&
		     (sb->sb_versionnum & XFS_SB_VERSION_DALIGNBIT)))  {
			/*
			 * stripe alignment values should be valid
			 */
			if (sb->sb_unit && !XFS_SB_VERSION_HASDALIGN(sb))
				return(XR_BAD_SB_UNIT);
			if (sb->sb_width && !XFS_SB_VERSION_HASDALIGN(sb))
				return(XR_BAD_SB_WIDTH);
		}

#if 0
		/*
		 * checks involving later superblock fields get added here...
		 */
		if (sb->sb_versionnum & XR_GOOD_SECSB_VNMASK)  {
		}
#endif
	}

	return(XR_OK);
}

void
write_primary_sb(xfs_sb_t *sbp, int size)
{
	void *buf;

	if (no_modify)
		return;

	if ((buf = calloc(size, 1)) == NULL) {
		do_error(_("failed to malloc superblock buffer\n"));
		return;
	}

	if (lseek64(fs_fd, 0LL, SEEK_SET) != 0LL) {
		free(buf);
		do_error(_("couldn't seek to offset 0 in filesystem\n"));
	}

	libxfs_xlate_sb(buf, sbp, -1, ARCH_CONVERT, XFS_SB_ALL_BITS);

	if (write(fs_fd, buf, size) != size) {
		free(buf);
		do_error(_("primary superblock write failed!\n"));
	}

	free(buf);
}

/*
 * get a possible superblock -- don't check for internal consistency
 */
int
get_sb(xfs_sb_t *sbp, xfs_off_t off, int size, xfs_agnumber_t agno)
{
	int error, rval;
	void *buf;

	if ((buf = calloc(size, 1)) == NULL) {
		do_error(
	_("error reading superblock %u -- failed to malloc buffer\n"),
			agno, off);
		exit(1);
	}

	/* try and read it first */

	if (lseek64(fs_fd, off, SEEK_SET) != off)  {
		do_warn(
	_("error reading superblock %u -- seek to offset %lld failed\n"),
			agno, off);
		return(XR_EOF);
	}

	if ((rval = read(fs_fd, buf, size)) != size)  {
		error = errno;
		do_warn(
	_("superblock read failed, offset %lld, size %d, ag %u, rval %d\n"),
			off, size, rval, agno);
		do_error("%s\n", strerror(error));
	}
	libxfs_xlate_sb(buf, sbp, 1, ARCH_CONVERT, XFS_SB_ALL_BITS);
	free(buf);

	return (verify_sb(sbp, 0));
}

#if 0
int
check_growfs(xfs_off_t off, int bufnum, xfs_agnumber_t agnum)
{
	int rval;

	ASSERT(bufnum < NUM_SBS);

	/* try and read it first */

	if (lseek64(fs_fd, off, SEEK_SET) != off)
		return(XR_EOF);

	if ((rval = read(fs_fd, sb_bufs[bufnum], sbbuf_size)) != sbbuf_size)  {
		/*
		 * we didn't get a full block so the filesystem
		 * could not have been grown.  return a non-XR_OK
		 * result code.
		 */
		return(XR_EOF);
	}

	return(get_sb(off, bufnum, agnum));
}
#endif
/* returns element on list with highest reference count */

fs_geo_list_t *
get_best_geo(fs_geo_list_t *list)
{
	int cnt = 0;
	fs_geo_list_t *current, *rval = NULL;

	current = list;

	while (current != NULL)  {
		if (current->refs > cnt)  {
			rval = current;
			cnt = current->refs;
		}
		current = current->next;
	}

	return(rval);
}

/* adds geometry info to linked list.  returns (sometimes new) head of list */

fs_geo_list_t *
add_geo(fs_geo_list_t *list, fs_geometry_t *geo_p, int index)
{
	fs_geo_list_t	*current = list;

	while (current != NULL)  {
		if (memcmp(geo_p, &current->geo, sizeof(fs_geometry_t)) == 0)  {
			current->refs++;
			return(list);
		}

		current = current->next;
	}

	if ((current = malloc(sizeof(fs_geo_list_t))) == NULL) {
		do_error(_("couldn't malloc geometry structure\n"));
		exit(1);
	}

	current->geo = *geo_p;
	current->refs = 1;
	current->next = list;
	current->index = index;

	return(current);
}

void
free_geo(fs_geo_list_t *list)
{
	fs_geo_list_t	*next;
	fs_geo_list_t	*current;

	current = list;

	for (current = list; current != NULL; current = next)  {
		next = current->next;
		free(current);
	}
}

void
get_sb_geometry(fs_geometry_t *geo, xfs_sb_t *sbp)
{
	bzero(geo, sizeof(fs_geometry_t));

	/*
	 * blindly set fields that we know are always good
	 */
	geo->sb_blocksize = sbp->sb_blocksize;
	geo->sb_dblocks = sbp->sb_dblocks;
	geo->sb_rblocks = sbp->sb_rblocks;
	geo->sb_rextents = sbp->sb_rextents;
	geo->sb_logstart = sbp->sb_logstart;
	geo->sb_rextsize = sbp->sb_rextsize;
	geo->sb_agblocks = sbp->sb_agblocks;
	geo->sb_agcount = sbp->sb_agcount;
	geo->sb_rbmblocks = sbp->sb_rbmblocks;
	geo->sb_logblocks = sbp->sb_logblocks;
	geo->sb_sectsize = sbp->sb_sectsize;
	geo->sb_inodesize = sbp->sb_inodesize;

	if (XFS_SB_VERSION_HASALIGN(sbp))
		geo->sb_ialignbit = 1;

	if (XFS_SB_VERSION_HASSHARED(sbp) ||
	    sbp->sb_versionnum & XR_PART_SECSB_VNMASK)
		geo->sb_sharedbit = 1;

	if (XFS_SB_VERSION_HASDALIGN(sbp))
		geo->sb_salignbit = 1;

	if (XFS_SB_VERSION_HASEXTFLGBIT(sbp))
		geo->sb_extflgbit = 1;

	/*
	 * protect against pre-6.5 mkfs-generated garbaged
	 * fields in the secondary superblocks.  pay attention
	 * to those fields if and only if their corresponding
	 * feature bits are set in the feature bits of the
	 * version number or we can deduce from the version bits
	 * that are set that our field was properly initialized
	 * because a field after the field we care about was
	 * properly initialized as well.
	 */

	/*
	 * inode alignment field lives before the data alignment field
	 */
	if ((!pre_65_beta && (sbp->sb_versionnum & XR_PART_SECSB_VNMASK)) ||
	    (pre_65_beta && (sbp->sb_versionnum & XR_ALPHA_SECSB_VNMASK)))
		geo->sb_inoalignmt = sbp->sb_inoalignmt;

	if ((!pre_65_beta && (sbp->sb_versionnum & XR_GOOD_SECSB_VNMASK)) ||
	    (pre_65_beta && XFS_SB_VERSION_HASDALIGN(sbp))) {
		geo->sb_unit = sbp->sb_unit;
		geo->sb_width = sbp->sb_width;
	}

	/*
	 * shared vn always set if either ino or data alignment is on
	 * since that field lives between the quota and inode alignment
	 * fields
	 */
	if (sbp->sb_versionnum & XR_PART_SECSB_VNMASK)
		geo->sb_shared_vn = sbp->sb_shared_vn;

	/*
	 * superblock fields located after sb_widthfields get set
	 * into the geometry structure only if we can determine
	 * from the features enabled in this superblock whether
	 * or not the sector was bzero'd at mkfs time.
	 */
	if ((!pre_65_beta && (sbp->sb_versionnum & XR_GOOD_SECSB_VNMASK)) ||
	    (pre_65_beta && (sbp->sb_versionnum & XR_ALPHA_SECSB_VNMASK))) {
		geo->sb_fully_zeroed = 1;
	}
}

/*
 * the way to verify that a primary sb is consistent with the
 * filesystem is find the secondaries given the info in the
 * primary and compare the geometries in the secondaries against
 * the geometry indicated by the primary.
 *
 * returns 1 if bad, 0 if ok
 */
int
verify_set_primary_sb(xfs_sb_t		*rsb,
			int		sb_index,
			int		*sb_modified)
{
	xfs_off_t	off;
	fs_geometry_t	geo;
	xfs_sb_t	*sb;
	fs_geo_list_t	*list;
	fs_geo_list_t	*current;
	char		*checked;
	xfs_agnumber_t	agno;
	int		num_sbs;
	int		skip;
	int		size;
	int		num_ok;
	int		retval;
	int		round;

	/*
	 * select the number of secondaries to try for
	 */
	num_sbs = MIN(NUM_SBS, rsb->sb_agcount);
	skip = howmany(num_sbs, rsb->sb_agcount);
	size = NUM_AGH_SECTS * rsb->sb_sectsize;
	retval = 0;
	list = NULL;
	num_ok = 0;
	*sb_modified = 0;

	sb = (xfs_sb_t *) alloc_ag_buf(size);
	checked = calloc(rsb->sb_agcount, sizeof(char));
	if (!checked) {
		do_error(_("calloc failed in verify_set_primary_sb\n"));
		exit(1);
	}

	/*
	 * put the primary sb geometry info onto the geometry list
	 */
	checked[sb_index] = 1;
	get_sb_geometry(&geo, rsb);
	list = add_geo(list, &geo, sb_index);

	/*
	 * grab N secondaries.  check them off as we get them
	 * so we only process each one once
	 */
	for (round = 0; round < skip; round++)  {
		for (agno = round; agno < rsb->sb_agcount; agno += skip)  {
			if (checked[agno])
				continue;

			off = (xfs_off_t)agno * rsb->sb_agblocks << rsb->sb_blocklog;

			checked[agno] = 1;

			if (get_sb(sb, off, size, agno) == XR_EOF)  {
				retval = 1;
				goto out;
			}

			if (verify_sb(sb, 0) == XR_OK)  {
				/*
				 * save away geometry info.
				 * don't bother checking the sb
				 * against the agi/agf as the odds
				 * of the sb being corrupted in a way
				 * that it is internally consistent
				 * but not consistent with the rest
				 * of the filesystem is really really low.
				 */
				get_sb_geometry(&geo, sb);
				list = add_geo(list, &geo, agno);
				num_ok++;
			}
		}
	}

	/*
	 * see if we have enough superblocks to bother with
	 */
	if (num_ok < num_sbs / 2)
		return(XR_INSUFF_SEC_SB);

	current = get_best_geo(list);

	/*
	 * check that enough sbs agree that we're willing to
	 * go with this geometry.  if not, print out the
	 * geometry and a message about the force option.
	 */
	switch (num_sbs)  {
	case 2:
		/*
		 * all them have to be right.  if not, report geometry
		 * and get out unless force option is in effect (-F)
		 */
		if (current->refs != 2)  {
			if (!force_geo)  {
				do_warn(
	_("Only two AGs detected and they do not match - cannot proceed.\n"));
				exit(1);
			}
		}
		break;
	case 1:
		/*
		 * just report the geometry info and get out.
		 * refuse to run further unless the force (-F)
		 * option is in effect.
		 */
		if (!force_geo)  {
			do_warn(_("Only one AG detected - cannot proceed.\n"));
			exit(1);
		}
	default:
		/*
		 * at least half of the probed superblocks have
		 * to agree.  if they don't, this fs is probably
		 * too far gone anyway considering the fact that
		 * XFS normally doesn't alter the secondary superblocks.
		 */
		if (current->refs < num_sbs / 2)  {
			do_warn(
		_("Not enough matching superblocks - cannot proceed.\n"));
			exit(1);
		}
	}

	/*
	 * set the geometry into primary superblock if necessary.
	 */

	if (current->index != sb_index)  {
		*sb_modified = 1;
		off = current->index * current->geo.sb_agblocks
			* current->geo.sb_blocksize;
		if (get_sb(sb, off, current->geo.sb_sectsize,
				current->index) != XR_OK)
			do_error(_("could not read superblock\n"));

		copy_sb(sb, rsb);

		/*
		 * turn off inprogress bit since this is the primary.
		 * also save away values that we need to ensure are
		 * consistent in the other secondaries.
		 */
		rsb->sb_inprogress = 0;
		sb_inoalignmt = sb->sb_inoalignmt;
		sb_unit = sb->sb_unit;
		sb_width = sb->sb_width;
	}

	free_geo(list);
out:
	free(sb);
	free(checked);
	return(retval);
}
