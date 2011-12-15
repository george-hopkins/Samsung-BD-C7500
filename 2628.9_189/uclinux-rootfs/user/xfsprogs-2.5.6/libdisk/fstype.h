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

#undef XFS_SUPER_MAGIC

/*
 * From mount(8) source by Andries Brouwer.  Hacked for XFS by mkp.
 * Recent sync's to mount source:
 *      - util-linux-2.10o ... 06 Sep 00
 *      - util-linux-2.10r ... 06 Dec 00
 *      - util-linux-2.11g ... 02 Jul 01
 *      - util-linux-2.11u ... 24 Aug 02
 *	- util-linux-2.11z ... 13 May 03
 */

/* Including <linux/fs.h> became more and more painful.
   Below a very abbreviated version of some declarations,
   only designed to be able to check a magic number
   in case no filesystem type was given. */

#define MINIX_SUPER_MAGIC   0x137F         /* minix v1, 14 char names */
#define MINIX_SUPER_MAGIC2  0x138F         /* minix v1, 30 char names */
#define MINIX2_SUPER_MAGIC  0x2468	   /* minix v2, 14 char names */
#define MINIX2_SUPER_MAGIC2 0x2478         /* minix v2, 30 char names */
struct minix_super_block {
	u_char   s_dummy[16];
	u_char   s_magic[2];
};
#define minixmagic(s)	assemble2le(s.s_magic)

#define ISODCL(from, to) (to - from + 1)
#define ISO_STANDARD_ID "CD001"
struct iso_volume_descriptor {
	char type[ISODCL(1,1)]; /* 711 */
	char id[ISODCL(2,6)];
	char version[ISODCL(7,7)];
	char data[ISODCL(8,2048)];
};

#define HS_STANDARD_ID "CDROM"
struct  hs_volume_descriptor {
	char foo[ISODCL (  1,   8)]; /* 733 */
	char type[ISODCL (  9,   9)]; /* 711 */
	char id[ISODCL ( 10,  14)];
	char version[ISODCL ( 15,  15)]; /* 711 */
	char data[ISODCL(16,2048)];
};

#define EXT_SUPER_MAGIC 0x137D
struct ext_super_block {
	u_char   s_dummy[56];
	u_char   s_magic[2];
};
#define extmagic(s)	assemble2le(s.s_magic)

#define EXT2_PRE_02B_MAGIC  0xEF51
#define EXT2_SUPER_MAGIC    0xEF53
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL 0x0004
struct ext2_super_block {
	u_char 	s_dummy1[56];
	u_char 	s_magic[2];
	u_char	s_dummy2[34];
	u_char	s_feature_compat[4];
	u_char	s_feature_incompat[4];
	u_char	s_feature_ro_compat[4];
	u_char	s_uuid[16];
	u_char 	s_volume_name[16];
	u_char	s_dummy3[88];
	u_char	s_journal_inum[4];	/* ext3 only */
};
#define ext2magic(s)	assemble2le(s.s_magic)

struct reiserfs_super_block
{
	u_char		s_block_count[4];
	u_char		s_free_blocks[4];
	u_char		s_root_block[4];
	u_char		s_journal_block[4];
	u_char		s_journal_dev[4];
	u_char		s_orig_journal_size[4];
	u_char		s_journal_trans_max[4];
	u_char		s_journal_block_count[4];
	u_char		s_journal_max_batch[4];
	u_char		s_journal_max_commit_age[4];
	u_char		s_journal_max_trans_age[4];
	u_char		s_blocksize[2];
	u_char		s_oid_maxsize[2];
	u_char		s_oid_cursize[2];
	u_char		s_state[2];
	u_char		s_magic[12];
};
#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"
#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)
/* the spot for the super in versions 3.5 - 3.5.10 (inclusive) */
#define REISERFS_OLD_DISK_OFFSET_IN_BYTES (8 * 1024)

#define _XIAFS_SUPER_MAGIC 0x012FD16D
struct xiafs_super_block {
    u_char     s_boot_segment[512];     /*  1st sector reserved for boot */
    u_char     s_dummy[60];
    u_char     s_magic[4];
};
#define xiafsmagic(s)	assemble4le(s.s_magic)

/* From jj@sunsite.ms.mff.cuni.cz Mon Mar 23 15:19:05 1998 */
#define UFS_SUPER_MAGIC_LE 0x00011954
#define UFS_SUPER_MAGIC_BE 0x54190100
struct ufs_super_block {
    u_char     s_dummy[0x55c];
    u_char     s_magic[4];
};
#define ufsmagic(s)	assemble4le(s.s_magic)

/* From Richard.Russon@ait.co.uk Wed Feb 24 08:05:27 1999 */
#define NTFS_SUPER_MAGIC "NTFS"
struct ntfs_super_block {
    u_char    s_dummy[3];
    u_char    s_magic[4];
};

/* From inspection of a few FAT filesystems - aeb */
/* Unfortunately I find almost the same thing on an extended partition;
   it looks like a primary has some directory entries where the extended
   has a partition table: IO.SYS, MSDOS.SYS, WINBOOT.SYS */
struct fat_super_block {
    u_char    s_dummy[3];
    u_char    s_os[8];		/* "MSDOS5.0" or "MSWIN4.0" or "MSWIN4.1" */
				/* mtools-3.9.4 writes "MTOOL394" */
    u_char    s_dummy2[32];
    u_char    s_label[11];	/* for DOS? */
    u_char    s_fs[8];		/* "FAT12   " or "FAT16   " or all zero   */
                                /* OS/2 BM has "FAT     " here. */
    u_char    s_dummy3[9];
    u_char    s_label2[11];	/* for Windows? */
    u_char    s_fs2[8];	        /* garbage or "FAT32   " */
};

#define XFS_SUPER_MAGIC "XFSB"
struct xfs_super_block {
    u_char    s_magic[4];
    u_char    s_dummy[28];
    u_char    s_uuid[16];
    u_char    s_dummy2[60];
    u_char    s_fname[12];
};

#define CRAMFS_SUPER_MAGIC 0x28cd3d45
#define CRAMFS_SUPER_MAGIC_BE 0x453dcd28
struct cramfs_super_block {
	u_char    s_magic[4];
	u_char    s_dummy[12];
	u_char    s_id[16];
};
#define cramfsmagic(s)	assemble4le(s.s_magic)

#define HFS_SUPER_MAGIC 0x4244
#define HFSPLUS_SUPER_MAGIC 0x482B
#define HFSPLUS_SUPER_VERSION 0x004
struct hfs_super_block {
	u_char    s_magic[2];
	u_char    s_version[2];
};
#define hfsmagic(s)	assemble2le(s.s_magic)
#define hfsversion(s)	assemble2le(s.s_version)

#define HPFS_SUPER_MAGIC 0xf995e849
struct hpfs_super_block {
	u_char    s_magic[4];
	u_char    s_magic2[4];
};
#define hpfsmagic(s)	assemble4le(s.s_magic)

struct adfs_super_block {
	u_char    s_dummy[448];
	u_char    s_blksize[1];
	u_char    s_dummy2[62];
	u_char    s_checksum[1];
};
#define adfsblksize(s)	((uint) s.s_blksize[0])

/* found in first 4 bytes of block 1 */
struct vxfs_super_block {
	u_char	s_magic[4];
};
#define vxfsmagic(s)	assemble4le(s.s_magic)
#define VXFS_SUPER_MAGIC 0xa501FCF5

struct jfs_super_block {
	char	s_magic[4];
	u_char	s_version[4];
	u_char	s_dummy1[93];
	char	s_fpack[11];
	u_char	s_dummy2[24];
	u_char	s_uuid[16];
	char	s_label[16];
};
#define JFS_SUPER1_OFF 0x8000
#define JFS_MAGIC "JFS1"

struct sysv_super_block {
	u_char  s_dummy1[504];
	u_char  s_magic[4];
	u_char  type[4];
};
#define sysvmagic(s)		assemble4le(s.s_magic)
#define SYSV_SUPER_MAGIC	0xfd187e20

struct mdp_super_block {
	u_char	md_magic[4];
};
#define MD_SB_MAGIC	0xa92b4efc
#define mdsbmagic(s)	assemble4le(s.md_magic)

struct ocfs_volume_header {
	u_char  minor_version[4];
	u_char  major_version[4];
	u_char  signature[128];
};

struct ocfs_volume_label {
	u_char  disk_lock[48];
	u_char  label[64];
	u_char  label_len[2];
};

#define ocfslabellen(o)	assemble2le(o.label_len)
#define OCFS_MAGIC	"OracleCFS"

static inline int
assemble2le(unsigned char *p) {
	return (p[0] | (p[1] << 8));
}

static inline int
assemble4le(unsigned char *p) {
	return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}
