/*
 * Copyright (c) 2007 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libxfs.h>
#include "bmap.h"
#include "command.h"
#include "metadump.h"
#include "io.h"
#include "output.h"
#include "type.h"
#include "init.h"
#include "sig.h"
#include "xfs_metadump.h"

#define DEFAULT_MAX_EXT_SIZE	1000

/* copy all metadata structures to/from a file */

static int	metadump_f(int argc, char **argv);
static void	metadump_help(void);

/*
 * metadump commands issue info/wornings/errors to standard error as
 * metadump supports stdout as a destination.
 *
 * All static functions return zero on failure, while the public functions
 * return zero on success.
 */

static const cmdinfo_t	metadump_cmd =
	{ "metadump", NULL, metadump_f, 0, -1, 0,
		N_("[-e] [-g] [-m max_extent] [-w] [-o] filename"),
		N_("dump metadata to a file"), metadump_help };

static FILE		*outf;		/* metadump file */

static xfs_metablock_t 	*metablock;	/* header + index + buffers */
static __be64		*block_index;
static char		*block_buffer;

static int		num_indicies;
static int		cur_index;

static xfs_ino_t	cur_ino;

static int		show_progress = 0;
static int		stop_on_read_error = 0;
static int		max_extent_size = DEFAULT_MAX_EXT_SIZE;
static int		dont_obfuscate = 0;
static int		show_warnings = 0;
static int		progress_since_warning = 0;

void
metadump_init(void)
{
	add_command(&metadump_cmd);
}

static void
metadump_help(void)
{
	dbprintf(_(
"\n"
" The 'metadump' command dumps the known metadata to a compact file suitable\n"
" for compressing and sending to an XFS maintainer for corruption analysis \n"
" or xfs_repair failures.\n\n"
" Options:\n"
"   -e -- Ignore read errors and keep going\n"
"   -g -- Display dump progress\n"
"   -m -- Specify max extent size in blocks to copy (default = %d blocks)\n"
"   -o -- Don't obfuscate names and extended attributes\n"
"   -w -- Show warnings of bad metadata information\n"
"\n"), DEFAULT_MAX_EXT_SIZE);
}

static void
print_warning(const char *fmt, ...)
{
	char		buf[200];
	va_list		ap;

	if (seenint())
		return;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf)-1] = '\0';

	fprintf(stderr, "%s%s: %s\n", progress_since_warning ? "\n" : "",
			progname, buf);
	progress_since_warning = 0;
}

static void
print_progress(const char *fmt, ...)
{
	char		buf[60];
	va_list		ap;
	FILE		*f;

	if (seenint())
		return;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf)-1] = '\0';

	f = (outf == stdout) ? stderr : stdout;
	fprintf(f, "\r%-59s", buf);
	fflush(f);
	progress_since_warning = 1;
}

/*
 * A complete dump file will have a "zero" entry in the last index block,
 * even if the dump is exactly aligned, the last index will be full of
 * zeros. If the last index entry is non-zero, the dump is incomplete.
 * Correspondingly, the last chunk will have a count < num_indicies.
 */

static int
write_index(void)
{
	/*
	 * write index block and following data blocks (streaming)
	 */
	metablock->mb_count = cpu_to_be16(cur_index);
	if (fwrite(metablock, (cur_index + 1) << BBSHIFT, 1, outf) != 1) {
		print_warning("error writing to file: %s", strerror(errno));
		return 0;
	}

	memset(block_index, 0, num_indicies * sizeof(__be64));
	cur_index = 0;
	return 1;
}

static int
write_buf(
	iocur_t		*buf)
{
	char		*data;
	__int64_t	off;
	int		i;

	for (i = 0, off = buf->bb, data = buf->data;
			i < buf->blen;
			i++, off++, data += BBSIZE) {
		block_index[cur_index] = cpu_to_be64(off);
		memcpy(&block_buffer[cur_index << BBSHIFT], data, BBSIZE);
		if (++cur_index == num_indicies) {
			if (!write_index())
				return 0;
		}
	}
	return !seenint();
}


static int
scan_btree(
	xfs_agnumber_t	agno,
	xfs_agblock_t	agbno,
	int		level,
	typnm_t		btype,
	void		*arg,
	int		(*func)(struct xfs_btree_block	*block,
				xfs_agnumber_t		agno,
				xfs_agblock_t		agbno,
				int			level,
				typnm_t			btype,
				void			*arg))
{
	int		rval = 0;

	push_cur();
	set_cur(&typtab[btype], XFS_AGB_TO_DADDR(mp, agno, agbno), blkbb,
			DB_RING_IGN, NULL);
	if (iocur_top->data == NULL) {
		print_warning("cannot read %s block %u/%u", typtab[btype].name,
				agno, agbno);
		rval = !stop_on_read_error;
		goto pop_out;
	}
	if (!write_buf(iocur_top))
		goto pop_out;

	if (!(*func)(iocur_top->data, agno, agbno, level - 1, btype, arg))
		goto pop_out;
	rval = 1;
pop_out:
	pop_cur();
	return rval;
}

/* free space tree copy routines */

static int
valid_bno(
	xfs_agnumber_t		agno,
	xfs_agblock_t		agbno)
{
	if (agno < (mp->m_sb.sb_agcount - 1) && agbno > 0 &&
			agbno <= mp->m_sb.sb_agblocks)
		return 1;
	if (agno == (mp->m_sb.sb_agcount - 1) && agbno > 0 &&
			agbno <= (mp->m_sb.sb_dblocks -
			 (xfs_drfsbno_t)(mp->m_sb.sb_agcount - 1) *
			 mp->m_sb.sb_agblocks))
		return 1;

	return 0;
}


static int
scanfunc_freesp(
	struct xfs_btree_block	*block,
	xfs_agnumber_t		agno,
	xfs_agblock_t		agbno,
	int			level,
	typnm_t			btype,
	void			*arg)
{
	xfs_alloc_ptr_t		*pp;
	int			i;
	int			numrecs;

	if (level == 0)
		return 1;

	numrecs = be16_to_cpu(block->bb_numrecs);
	if (numrecs > mp->m_alloc_mxr[1]) {
		if (show_warnings)
			print_warning("invalid numrecs (%u) in %s block %u/%u",
				numrecs, typtab[btype].name, agno, agbno);
		return 1;
	}

	pp = XFS_ALLOC_PTR_ADDR(mp, block, 1, mp->m_alloc_mxr[1]);
	for (i = 0; i < numrecs; i++) {
		if (!valid_bno(agno, be32_to_cpu(pp[i]))) {
			if (show_warnings)
				print_warning("invalid block number (%u/%u) "
					"in %s block %u/%u",
					agno, be32_to_cpu(pp[i]),
					typtab[btype].name, agno, agbno);
			continue;
		}
		if (!scan_btree(agno, be32_to_cpu(pp[i]), level, btype, arg,
				scanfunc_freesp))
			return 0;
	}
	return 1;
}

static int
copy_free_bno_btree(
	xfs_agnumber_t	agno,
	xfs_agf_t	*agf)
{
	xfs_agblock_t	root;
	int		levels;

	root = be32_to_cpu(agf->agf_roots[XFS_BTNUM_BNO]);
	levels = be32_to_cpu(agf->agf_levels[XFS_BTNUM_BNO]);

	/* validate root and levels before processing the tree */
	if (root == 0 || root > mp->m_sb.sb_agblocks) {
		if (show_warnings)
			print_warning("invalid block number (%u) in bnobt "
					"root in agf %u", root, agno);
		return 1;
	}
	if (levels >= XFS_BTREE_MAXLEVELS) {
		if (show_warnings)
			print_warning("invalid level (%u) in bnobt root "
					"in agf %u", levels, agno);
		return 1;
	}

	return scan_btree(agno, root, levels, TYP_BNOBT, agf, scanfunc_freesp);
}

static int
copy_free_cnt_btree(
	xfs_agnumber_t	agno,
	xfs_agf_t	*agf)
{
	xfs_agblock_t	root;
	int		levels;

	root = be32_to_cpu(agf->agf_roots[XFS_BTNUM_CNT]);
	levels = be32_to_cpu(agf->agf_levels[XFS_BTNUM_CNT]);

	/* validate root and levels before processing the tree */
	if (root == 0 || root > mp->m_sb.sb_agblocks) {
		if (show_warnings)
			print_warning("invalid block number (%u) in cntbt "
					"root in agf %u", root, agno);
		return 1;
	}
	if (levels >= XFS_BTREE_MAXLEVELS) {
		if (show_warnings)
			print_warning("invalid level (%u) in cntbt root "
					"in agf %u", levels, agno);
		return 1;
	}

	return scan_btree(agno, root, levels, TYP_CNTBT, agf, scanfunc_freesp);
}

/* filename and extended attribute obfuscation routines */

typedef struct name_ent {
	struct name_ent		*next;
	xfs_dahash_t		hash;
	int	  	    	namelen;
	uchar_t    	    	name[1];
} name_ent_t;

#define NAME_TABLE_SIZE		4096

static name_ent_t 		**nametable;

static int
create_nametable(void)
{
	nametable = calloc(NAME_TABLE_SIZE, sizeof(name_ent_t));
	return nametable != NULL;
}

static void
clear_nametable(void)
{
	int			i;
	name_ent_t		*p;

	for (i = 0; i < NAME_TABLE_SIZE; i++) {
		while (nametable[i]) {
			p = nametable[i];
			nametable[i] = p->next;
			free(p);
		}
	}
}


#define is_invalid_char(c)	((c) == '/' || (c) == '\0')
#define rol32(x,y)		(((x) << (y)) | ((x) >> (32 - (y))))

static inline uchar_t
random_filename_char(void)
{
	uchar_t			c;

	do {
		c = random() % 127 + 1;
	} while (c == '/');
	return c;
}

static int
is_special_dirent(
	xfs_ino_t		ino,
	int			namelen,
	uchar_t			*name)
{
	static xfs_ino_t	orphanage_ino = 0;
	char			s[32];
	int			slen;

	/*
	 * due to the XFS name hashing algorithm, we cannot obfuscate
	 * names with 4 chars or less.
	 */
	if (namelen <= 4)
		return 1;

	if (ino == 0)
		return 0;

	/*
	 * don't obfuscate lost+found nor any inodes within lost+found with
	 * the inode number
	 */
	if (cur_ino == mp->m_sb.sb_rootino && namelen == 10 &&
			memcmp(name, "lost+found", 10) == 0) {
		orphanage_ino = ino;
		return 1;
	}
	if (cur_ino != orphanage_ino)
		return 0;

	slen = sprintf(s, "%lld", (long long)ino);
	return (slen == namelen && memcmp(name, s, namelen) == 0);
}

static void
generate_obfuscated_name(
	xfs_ino_t		ino,
	int			namelen,
	uchar_t			*name)
{
	xfs_dahash_t		hash;
	name_ent_t		*p;
	int			i;
	int			dup;
	xfs_dahash_t		newhash;
	uchar_t			newname[NAME_MAX];

	if (is_special_dirent(ino, namelen, name))
		return;

	hash = libxfs_da_hashname(name, namelen);

	/* create a random name with the same hash value */

	do {
		dup = 0;
		newname[0] = '/';

		for (;;) {
			/* if the first char is a "/", preserve it */
			i = (name[0] == '/');

			for (newhash = 0; i < namelen - 5; i++) {
				newname[i] = random_filename_char();
				newhash = newname[i] ^ rol32(newhash, 7);
			}
			newhash = rol32(newhash, 3) ^ hash;
			if (name[0] != '/' || namelen > 5) {
				newname[namelen - 5] = (newhash >> 28) |
						(random_filename_char() & 0xf0);
				if (is_invalid_char(newname[namelen - 5]))
					continue;
			}
			newname[namelen - 4] = (newhash >> 21) & 0x7f;
			if (is_invalid_char(newname[namelen - 4]))
				continue;
			newname[namelen - 3] = (newhash >> 14) & 0x7f;
			if (is_invalid_char(newname[namelen - 3]))
				continue;
			newname[namelen - 2] = (newhash >> 7) & 0x7f;
			if (is_invalid_char(newname[namelen - 2]))
				continue;
			newname[namelen - 1] = ((newhash >> 0) ^
					(newname[namelen - 5] >> 4)) & 0x7f;
			if (is_invalid_char(newname[namelen - 1]))
				continue;
			break;
		}

		ASSERT(libxfs_da_hashname(newname, namelen) == hash);

		for (p = nametable[hash % NAME_TABLE_SIZE]; p; p = p->next) {
			if (p->hash == hash && p->namelen == namelen &&
					memcmp(p->name, newname, namelen) == 0){
				dup = 1;
				break;
			}
		}
	} while (dup);

	memcpy(name, newname, namelen);

	p = malloc(sizeof(name_ent_t) + namelen);
	if (p == NULL)
		return;

	p->next = nametable[hash % NAME_TABLE_SIZE];
	p->hash = hash;
	p->namelen = namelen;
	memcpy(p->name, name, namelen);

	nametable[hash % NAME_TABLE_SIZE] = p;
}

static void
obfuscate_sf_dir(
	xfs_dinode_t		*dip)
{
	xfs_dir2_sf_t		*sfp;
	xfs_dir2_sf_entry_t	*sfep;
	__uint64_t		ino_dir_size;
	int			i;

	sfp = &dip->di_u.di_dir2sf;
	ino_dir_size = be64_to_cpu(dip->di_core.di_size);
	if (ino_dir_size > XFS_DFORK_DSIZE(dip, mp)) {
		ino_dir_size = XFS_DFORK_DSIZE(dip, mp);
		if (show_warnings)
			print_warning("invalid size in dir inode %llu",
					(long long)cur_ino);
	}

	sfep = xfs_dir2_sf_firstentry(sfp);
	for (i = 0; (i < sfp->hdr.count) &&
			((char *)sfep - (char *)sfp < ino_dir_size); i++) {

		/*
		 * first check for bad name lengths. If they are bad, we
		 * have limitations to how much can be obfuscated.
		 */
		int	namelen = sfep->namelen;

		if (namelen == 0) {
			if (show_warnings)
				print_warning("zero length entry in dir inode "
						"%llu", (long long)cur_ino);
			if (i != sfp->hdr.count - 1)
				break;
			namelen = ino_dir_size - ((char *)&sfep->name[0] -
					 (char *)sfp);
		} else if ((char *)sfep - (char *)sfp +
				xfs_dir2_sf_entsize_byentry(sfp, sfep) >
				ino_dir_size) {
			if (show_warnings)
				print_warning("entry length in dir inode %llu "
					"overflows space", (long long)cur_ino);
			if (i != sfp->hdr.count - 1)
				break;
			namelen = ino_dir_size - ((char *)&sfep->name[0] -
					 (char *)sfp);
		}

		generate_obfuscated_name(xfs_dir2_sf_get_inumber(sfp,
				xfs_dir2_sf_inumberp(sfep)), namelen,
				&sfep->name[0]);

		sfep = (xfs_dir2_sf_entry_t *)((char *)sfep +
				xfs_dir2_sf_entsize_byname(sfp, namelen));
	}
}

static void
obfuscate_sf_symlink(
	xfs_dinode_t		*dip)
{
	__uint64_t		len;

	len = be64_to_cpu(dip->di_core.di_size);
	if (len > XFS_DFORK_DSIZE(dip, mp)) {
		if (show_warnings)
			print_warning("invalid size (%d) in symlink inode %llu",
					len, (long long)cur_ino);
		len = XFS_DFORK_DSIZE(dip, mp);
	}

	while (len > 0)
		dip->di_u.di_symlink[--len] = random() % 127 + 1;
}

static void
obfuscate_sf_attr(
	xfs_dinode_t		*dip)
{
	/*
	 * with extended attributes, obfuscate the names and zero the actual
	 * values.
	 */

	xfs_attr_shortform_t	*asfp;
	xfs_attr_sf_entry_t	*asfep;
	int			ino_attr_size;
	int			i;

	asfp = (xfs_attr_shortform_t *)XFS_DFORK_APTR(dip);
	if (asfp->hdr.count == 0)
		return;

	ino_attr_size = be16_to_cpu(asfp->hdr.totsize);
	if (ino_attr_size > XFS_DFORK_ASIZE(dip, mp)) {
		ino_attr_size = XFS_DFORK_ASIZE(dip, mp);
		if (show_warnings)
			print_warning("invalid attr size in inode %llu",
					(long long)cur_ino);
	}

	asfep = &asfp->list[0];
	for (i = 0; (i < asfp->hdr.count) &&
			((char *)asfep - (char *)asfp < ino_attr_size); i++) {

		int	namelen = asfep->namelen;

		if (namelen == 0) {
			if (show_warnings)
				print_warning("zero length attr entry in inode "
						"%llu", (long long)cur_ino);
			break;
		} else if ((char *)asfep - (char *)asfp +
				XFS_ATTR_SF_ENTSIZE(asfep) > ino_attr_size) {
			if (show_warnings)
				print_warning("attr entry length in inode %llu "
					"overflows space", (long long)cur_ino);
			break;
		}

		generate_obfuscated_name(0, asfep->namelen, &asfep->nameval[0]);
		memset(&asfep->nameval[asfep->namelen], 0, asfep->valuelen);

		asfep = (xfs_attr_sf_entry_t *)((char *)asfep +
				XFS_ATTR_SF_ENTSIZE(asfep));
	}
}

/*
 * dir_data structure is used to track multi-fsblock dir2 blocks between extent
 * processing calls.
 */

static struct dir_data_s {
	int			end_of_data;
	int			block_index;
	int			offset_to_entry;
	int			bad_block;
} dir_data;

static void
obfuscate_dir_data_blocks(
	char			*block,
	xfs_dfiloff_t		offset,
	xfs_dfilblks_t		count,
	int			is_block_format)
{
	/*
	 * we have to rely on the fileoffset and signature of the block to
	 * handle it's contents. If it's invalid, leave it alone.
	 * for multi-fsblock dir blocks, if a name crosses an extent boundary,
	 * ignore it and continue.
	 */
	int			c;
	int			dir_offset;
	char			*ptr;
	char			*endptr;

	if (is_block_format && count != mp->m_dirblkfsbs)
		return; /* too complex to handle this rare case */

	for (c = 0, endptr = block; c < count; c++) {

		if (dir_data.block_index == 0) {
			int		wantmagic;

			if (offset % mp->m_dirblkfsbs != 0)
				return;	/* corrupted, leave it alone */

			dir_data.bad_block = 0;

			if (is_block_format) {
				xfs_dir2_leaf_entry_t	*blp;
				xfs_dir2_block_tail_t	*btp;

				btp = xfs_dir2_block_tail_p(mp,
						(xfs_dir2_block_t *)block);
				blp = xfs_dir2_block_leaf_p(btp);
				if ((char *)blp > (char *)btp)
					blp = (xfs_dir2_leaf_entry_t *)btp;

				dir_data.end_of_data = (char *)blp - block;
				wantmagic = XFS_DIR2_BLOCK_MAGIC;
			} else { /* leaf/node format */
				dir_data.end_of_data = mp->m_dirblkfsbs <<
						mp->m_sb.sb_blocklog;
				wantmagic = XFS_DIR2_DATA_MAGIC;
			}
			dir_data.offset_to_entry = offsetof(xfs_dir2_data_t, u);

			if (be32_to_cpu(((xfs_dir2_data_hdr_t*)block)->magic) !=
					wantmagic) {
				if (show_warnings)
					print_warning("invalid magic in dir "
						"inode %llu block %ld",
						(long long)cur_ino,
						(long)offset);
				dir_data.bad_block = 1;
			}
		}
		dir_data.block_index++;
		if (dir_data.block_index == mp->m_dirblkfsbs)
			dir_data.block_index = 0;

		if (dir_data.bad_block)
			continue;

		dir_offset = (dir_data.block_index << mp->m_sb.sb_blocklog) +
				dir_data.offset_to_entry;

		ptr = endptr + dir_data.offset_to_entry;
		endptr += mp->m_sb.sb_blocksize;

		while (ptr < endptr && dir_offset < dir_data.end_of_data) {
			xfs_dir2_data_entry_t	*dep;
			xfs_dir2_data_unused_t	*dup;
			int			length;

			dup = (xfs_dir2_data_unused_t *)ptr;

			if (be16_to_cpu(dup->freetag) == XFS_DIR2_DATA_FREE_TAG) {
				int	length = be16_to_cpu(dup->length);
				if (dir_offset + length > dir_data.end_of_data ||
						length == 0 || (length &
						 (XFS_DIR2_DATA_ALIGN - 1))) {
					if (show_warnings)
						print_warning("invalid length "
							"for dir free space in "
							"inode %llu",
							(long long)cur_ino);
					dir_data.bad_block = 1;
					break;
				}
				if (be16_to_cpu(*xfs_dir2_data_unused_tag_p(dup)) !=
						dir_offset) {
					dir_data.bad_block = 1;
					break;
				}
				dir_offset += length;
				ptr += length;
				if (dir_offset >= dir_data.end_of_data ||
						ptr >= endptr)
					break;
			}

			dep = (xfs_dir2_data_entry_t *)ptr;
			length = xfs_dir2_data_entsize(dep->namelen);

			if (dir_offset + length > dir_data.end_of_data ||
					ptr + length > endptr) {
				if (show_warnings)
					print_warning("invalid length for "
						"dir entry name in inode %llu",
						(long long)cur_ino);
				break;
			}
			if (be16_to_cpu(*xfs_dir2_data_entry_tag_p(dep)) !=
					dir_offset) {
				dir_data.bad_block = 1;
				break;
			}
			generate_obfuscated_name(be64_to_cpu(dep->inumber),
					dep->namelen, &dep->name[0]);
			dir_offset += length;
			ptr += length;
		}
		dir_data.offset_to_entry = dir_offset &
						(mp->m_sb.sb_blocksize - 1);
	}
}

static void
obfuscate_symlink_blocks(
	char			*block,
	xfs_dfilblks_t		count)
{
	int 			i;

	count <<= mp->m_sb.sb_blocklog;
	for (i = 0; i < count; i++)
		block[i] = random() % 127 + 1;
}

#define MAX_REMOTE_VALS		4095

static struct attr_data_s {
	int			remote_val_count;
	xfs_dablk_t		remote_vals[MAX_REMOTE_VALS];
} attr_data;

static inline void
add_remote_vals(
	xfs_dablk_t 		blockidx,
	int			length)
{
	while (length > 0 && attr_data.remote_val_count < MAX_REMOTE_VALS) {
		attr_data.remote_vals[attr_data.remote_val_count] = blockidx;
		attr_data.remote_val_count++;
		blockidx++;
		length -= XFS_LBSIZE(mp);
	}
}

static void
obfuscate_attr_blocks(
	char			*block,
	xfs_dfiloff_t		offset,
	xfs_dfilblks_t		count)
{
	xfs_attr_leafblock_t	*leaf;
	int			c;
	int			i;
	int			nentries;
	xfs_attr_leaf_entry_t 	*entry;
	xfs_attr_leaf_name_local_t *local;
	xfs_attr_leaf_name_remote_t *remote;

	for (c = 0; c < count; c++, offset++, block += XFS_LBSIZE(mp)) {

		leaf = (xfs_attr_leafblock_t *)block;

		if (be16_to_cpu(leaf->hdr.info.magic) != XFS_ATTR_LEAF_MAGIC) {
			for (i = 0; i < attr_data.remote_val_count; i++) {
				if (attr_data.remote_vals[i] == offset)
					memset(block, 0, XFS_LBSIZE(mp));
			}
			continue;
		}

		nentries = be16_to_cpu(leaf->hdr.count);
		if (nentries * sizeof(xfs_attr_leaf_entry_t) +
				sizeof(xfs_attr_leaf_hdr_t) > XFS_LBSIZE(mp)) {
			if (show_warnings)
				print_warning("invalid attr count in inode %llu",
						(long long)cur_ino);
			continue;
		}

		for (i = 0, entry = &leaf->entries[0]; i < nentries;
				i++, entry++) {
			if (be16_to_cpu(entry->nameidx) > XFS_LBSIZE(mp)) {
				if (show_warnings)
					print_warning("invalid attr nameidx "
							"in inode %llu",
							(long long)cur_ino);
				break;
			}
			if (entry->flags & XFS_ATTR_LOCAL) {
				local = XFS_ATTR_LEAF_NAME_LOCAL(leaf, i);
				if (local->namelen == 0) {
					if (show_warnings)
						print_warning("zero length for "
							"attr name in inode %llu",
							(long long)cur_ino);
					break;
				}
				generate_obfuscated_name(0, local->namelen,
					&local->nameval[0]);
				memset(&local->nameval[local->namelen], 0,
					be16_to_cpu(local->valuelen));
			} else {
				remote = XFS_ATTR_LEAF_NAME_REMOTE(leaf, i);
				if (remote->namelen == 0 ||
						remote->valueblk == 0) {
					if (show_warnings)
						print_warning("invalid attr "
							"entry in inode %llu",
							(long long)cur_ino);
					break;
				}
				generate_obfuscated_name(0, remote->namelen,
					&remote->name[0]);
				add_remote_vals(be32_to_cpu(remote->valueblk),
					be32_to_cpu(remote->valuelen));
			}
		}
	}
}

/* inode copy routines */

static int
process_bmbt_reclist(
	xfs_bmbt_rec_t 		*rp,
	int 			numrecs,
	typnm_t			btype)
{
	int			i;
	xfs_dfiloff_t		o, op = NULLDFILOFF;
	xfs_dfsbno_t		s;
	xfs_dfilblks_t		c, cp = NULLDFILOFF;
	int			f;
	xfs_dfiloff_t		last;
	xfs_agnumber_t		agno;
	xfs_agblock_t		agbno;

	if (btype == TYP_DATA)
		return 1;

	convert_extent(&rp[numrecs - 1], &o, &s, &c, &f);
	last = o + c;

	for (i = 0; i < numrecs; i++, rp++) {
		convert_extent(rp, &o, &s, &c, &f);

		/*
		 * ignore extents that are clearly bogus, and if a bogus
		 * one is found, stop processing remaining extents
		 */
		if (i > 0 && op + cp > o) {
			if (show_warnings)
				print_warning("bmap extent %d in %s ino %llu "
					"starts at %llu, previous extent "
					"ended at %llu", i,
					typtab[btype].name, (long long)cur_ino,
					o, op + cp - 1);
			break;
		}

		if (c > max_extent_size) {
			/*
			 * since we are only processing non-data extents,
			 * large numbers of blocks in a metadata extent is
			 * extremely rare and more than likely to be corrupt.
			 */
			if (show_warnings)
				print_warning("suspicious count %u in bmap "
					"extent %d in %s ino %llu", c, i,
					typtab[btype].name, (long long)cur_ino);
			break;
		}

		op = o;
		cp = c;

		agno = XFS_FSB_TO_AGNO(mp, s);
		agbno = XFS_FSB_TO_AGBNO(mp, s);

		if (!valid_bno(agno, agbno)) {
			if (show_warnings)
				print_warning("invalid block number %u/%u "
					"(%llu) in bmap extent %d in %s ino "
					"%llu", agno, agbno, s, i,
					typtab[btype].name, (long long)cur_ino);
			break;
		}

		if (!valid_bno(agno, agbno + c - 1)) {
			if (show_warnings)
				print_warning("bmap extent %i in %s inode %llu "
					"overflows AG (end is %u/%u)", i,
					typtab[btype].name, (long long)cur_ino,
					agno, agbno + c - 1);
			break;
		}

		push_cur();
		set_cur(&typtab[btype], XFS_FSB_TO_DADDR(mp, s), c * blkbb,
				DB_RING_IGN, NULL);
		if (iocur_top->data == NULL) {
			print_warning("cannot read %s block %u/%u (%llu)",
					typtab[btype].name, agno, agbno, s);
			if (stop_on_read_error) {
				pop_cur();
				return 0;
			}
		} else {
			if (!dont_obfuscate)
			    switch (btype) {
				case TYP_DIR2:
					if (o < mp->m_dirleafblk)
						obfuscate_dir_data_blocks(
							iocur_top->data, o, c,
							last == mp->m_dirblkfsbs);
					break;

				case TYP_SYMLINK:
					obfuscate_symlink_blocks(
						iocur_top->data, c);
					break;

				case TYP_ATTR:
					obfuscate_attr_blocks(iocur_top->data,
						o, c);
					break;

				default: ;
			    }
			if (!write_buf(iocur_top)) {
				pop_cur();
				return 0;
			}
		}
		pop_cur();
	}

	return 1;
}

static int
scanfunc_bmap(
	struct xfs_btree_block	*block,
	xfs_agnumber_t		agno,
	xfs_agblock_t		agbno,
	int			level,
	typnm_t			btype,
	void			*arg)	/* ptr to itype */
{
	int			i;
	xfs_bmbt_ptr_t		*pp;
	int			nrecs;

	nrecs = be16_to_cpu(block->bb_numrecs);

	if (level == 0) {
		if (nrecs > mp->m_bmap_dmxr[0]) {
			if (show_warnings)
				print_warning("invalid numrecs (%u) in %s "
					"block %u/%u", nrecs,
					typtab[btype].name, agno, agbno);
			return 1;
		}
		return process_bmbt_reclist(XFS_BMBT_REC_ADDR(mp, block, 1),
					    nrecs, *(typnm_t*)arg);
	}

	if (nrecs > mp->m_bmap_dmxr[1]) {
		if (show_warnings)
			print_warning("invalid numrecs (%u) in %s block %u/%u",
					nrecs, typtab[btype].name, agno, agbno);
		return 1;
	}
	pp = XFS_BMBT_PTR_ADDR(mp, block, 1, mp->m_bmap_dmxr[1]);
	for (i = 0; i < nrecs; i++) {
		xfs_agnumber_t	ag;
		xfs_agblock_t	bno;

		ag = XFS_FSB_TO_AGNO(mp, be64_to_cpu(pp[i]));
		bno = XFS_FSB_TO_AGBNO(mp, be64_to_cpu(pp[i]));

		if (bno == 0 || bno > mp->m_sb.sb_agblocks ||
				ag > mp->m_sb.sb_agcount) {
			if (show_warnings)
				print_warning("invalid block number (%u/%u) "
					"in %s block %u/%u", ag, bno,
					typtab[btype].name, agno, agbno);
			continue;
		}

		if (!scan_btree(ag, bno, level, btype, arg, scanfunc_bmap))
			return 0;
	}
	return 1;
}

static int
process_btinode(
	xfs_dinode_t 		*dip,
	typnm_t			itype)
{
	xfs_bmdr_block_t	*dib;
	int			i;
	xfs_bmbt_ptr_t		*pp;
	int			level;
	int			nrecs;
	int			maxrecs;
	int			whichfork;
	typnm_t			btype;

	whichfork = (itype == TYP_ATTR) ? XFS_ATTR_FORK : XFS_DATA_FORK;
	btype = (itype == TYP_ATTR) ? TYP_BMAPBTA : TYP_BMAPBTD;

	dib = (xfs_bmdr_block_t *)XFS_DFORK_PTR(dip, whichfork);
	level = be16_to_cpu(dib->bb_level);
	nrecs = be16_to_cpu(dib->bb_numrecs);

	if (level > XFS_BM_MAXLEVELS(mp, whichfork)) {
		if (show_warnings)
			print_warning("invalid level (%u) in inode %lld %s "
					"root", level, (long long)cur_ino,
					typtab[btype].name);
		return 1;
	}

	if (level == 0) {
		return process_bmbt_reclist(XFS_BMDR_REC_ADDR(dib, 1),
					    nrecs, itype);
	}

	maxrecs = xfs_bmdr_maxrecs(mp, XFS_DFORK_SIZE(dip, mp, whichfork), 0);
	if (nrecs > maxrecs) {
		if (show_warnings)
			print_warning("invalid numrecs (%u) in inode %lld %s "
					"root", nrecs, (long long)cur_ino,
					typtab[btype].name);
		return 1;
	}

	pp = XFS_BMDR_PTR_ADDR(dib, 1, maxrecs);
	for (i = 0; i < nrecs; i++) {
		xfs_agnumber_t	ag;
		xfs_agblock_t	bno;

		ag = XFS_FSB_TO_AGNO(mp, be64_to_cpu(pp[i]));
		bno = XFS_FSB_TO_AGBNO(mp, be64_to_cpu(pp[i]));

		if (bno == 0 || bno > mp->m_sb.sb_agblocks ||
				ag > mp->m_sb.sb_agcount) {
			if (show_warnings)
				print_warning("invalid block number (%u/%u) "
						"in inode %llu %s root", ag,
						bno, (long long)cur_ino,
						typtab[btype].name);
			continue;
		}

		if (!scan_btree(ag, bno, level, btype, &itype, scanfunc_bmap))
			return 0;
	}
	return 1;
}

static int
process_exinode(
	xfs_dinode_t 		*dip,
	typnm_t			itype)
{
	int			whichfork;
	xfs_extnum_t		nex;

	whichfork = (itype == TYP_ATTR) ? XFS_ATTR_FORK : XFS_DATA_FORK;

	nex = XFS_DFORK_NEXTENTS(dip, whichfork);
	if (nex < 0 || nex > XFS_DFORK_SIZE(dip, mp, whichfork) /
						sizeof(xfs_bmbt_rec_t)) {
		if (show_warnings)
			print_warning("bad number of extents %d in inode %lld",
				nex, (long long)cur_ino);
		return 1;
	}

	return process_bmbt_reclist((xfs_bmbt_rec_t *)XFS_DFORK_PTR(dip,
					whichfork), nex, itype);
}

static int
process_inode_data(
	xfs_dinode_t		*dip,
	typnm_t			itype)
{
	switch (dip->di_core.di_format) {
		case XFS_DINODE_FMT_LOCAL:
			if (!dont_obfuscate)
				switch (itype) {
					case TYP_DIR2:
						obfuscate_sf_dir(dip);
						break;

					case TYP_SYMLINK:
						obfuscate_sf_symlink(dip);
						break;

					default: ;
				}
			break;

		case XFS_DINODE_FMT_EXTENTS:
			return process_exinode(dip, itype);

		case XFS_DINODE_FMT_BTREE:
			return process_btinode(dip, itype);
	}
	return 1;
}

static int
process_inode(
	xfs_agnumber_t		agno,
	xfs_agino_t 		agino,
	xfs_dinode_t 		*dip)
{
	int			success;

	success = 1;
	cur_ino = XFS_AGINO_TO_INO(mp, agno, agino);

	/* copy appropriate data fork metadata */
	switch (be16_to_cpu(dip->di_core.di_mode) & S_IFMT) {
		case S_IFDIR:
			memset(&dir_data, 0, sizeof(dir_data));
			success = process_inode_data(dip, TYP_DIR2);
			break;
		case S_IFLNK:
			success = process_inode_data(dip, TYP_SYMLINK);
			break;
		case S_IFREG:
			success = process_inode_data(dip, TYP_DATA);
			break;
		default: ;
	}
	clear_nametable();

	/* copy extended attributes if they exist and forkoff is valid */
	if (success && XFS_DFORK_DSIZE(dip, mp) < XFS_LITINO(mp)) {
		attr_data.remote_val_count = 0;
		switch (dip->di_core.di_aformat) {
			case XFS_DINODE_FMT_LOCAL:
				if (!dont_obfuscate)
					obfuscate_sf_attr(dip);
				break;

			case XFS_DINODE_FMT_EXTENTS:
				success = process_exinode(dip, TYP_ATTR);
				break;

			case XFS_DINODE_FMT_BTREE:
				success = process_btinode(dip, TYP_ATTR);
				break;
		}
		clear_nametable();
	}
	return success;
}

static __uint32_t	inodes_copied = 0;

static int
copy_inode_chunk(
	xfs_agnumber_t 		agno,
	xfs_inobt_rec_t 	*rp)
{
	xfs_agino_t 		agino;
	int			off;
	xfs_agblock_t		agbno;
	int			i;
	int			rval = 0;

	agino = be32_to_cpu(rp->ir_startino);
	agbno = XFS_AGINO_TO_AGBNO(mp, agino);
	off = XFS_INO_TO_OFFSET(mp, agino);

	if (agino == 0 || agino == NULLAGINO || !valid_bno(agno, agbno) ||
			!valid_bno(agno, XFS_AGINO_TO_AGBNO(mp,
					agino + XFS_INODES_PER_CHUNK - 1))) {
		if (show_warnings)
			print_warning("bad inode number %llu (%u/%u)",
				XFS_AGINO_TO_INO(mp, agno, agino), agno, agino);
		return 1;
	}

	push_cur();
	set_cur(&typtab[TYP_INODE], XFS_AGB_TO_DADDR(mp, agno, agbno),
			XFS_FSB_TO_BB(mp, XFS_IALLOC_BLOCKS(mp)),
			DB_RING_IGN, NULL);
	if (iocur_top->data == NULL) {
		print_warning("cannot read inode block %u/%u", agno, agbno);
		rval = !stop_on_read_error;
		goto pop_out;
	}

	/*
	 * check for basic assumptions about inode chunks, and if any
	 * assumptions fail, don't process the inode chunk.
	 */

	if ((mp->m_sb.sb_inopblock <= XFS_INODES_PER_CHUNK && off != 0) ||
			(mp->m_sb.sb_inopblock > XFS_INODES_PER_CHUNK &&
					off % XFS_INODES_PER_CHUNK != 0) ||
			(xfs_sb_version_hasalign(&mp->m_sb) &&
					agbno % mp->m_sb.sb_inoalignmt != 0)) {
		if (show_warnings)
			print_warning("badly aligned inode (start = %llu)",
					XFS_AGINO_TO_INO(mp, agno, agino));
		goto skip_processing;
	}

	/*
	 * scan through inodes and copy any btree extent lists, directory
	 * contents and extended attributes.
	 */
	for (i = 0; i < XFS_INODES_PER_CHUNK; i++) {
		xfs_dinode_t            *dip;

		if (XFS_INOBT_IS_FREE_DISK(rp, i))
			continue;

		dip = (xfs_dinode_t *)((char *)iocur_top->data +
				((off + i) << mp->m_sb.sb_inodelog));

		if (!process_inode(agno, agino + i, dip))
			goto pop_out;
	}
skip_processing:
	if (!write_buf(iocur_top))
		goto pop_out;

	inodes_copied += XFS_INODES_PER_CHUNK;

	if (show_progress)
		print_progress("Copied %u of %u inodes (%u of %u AGs)",
				inodes_copied, mp->m_sb.sb_icount, agno,
				mp->m_sb.sb_agcount);
	rval = 1;
pop_out:
	pop_cur();
	return rval;
}

static int
scanfunc_ino(
	struct xfs_btree_block	*block,
	xfs_agnumber_t		agno,
	xfs_agblock_t		agbno,
	int			level,
	typnm_t			btype,
	void			*arg)
{
	xfs_inobt_rec_t		*rp;
	xfs_inobt_ptr_t		*pp;
	int			i;
	int			numrecs;

	numrecs = be16_to_cpu(block->bb_numrecs);

	if (level == 0) {
		if (numrecs > mp->m_inobt_mxr[0]) {
			if (show_warnings)
				print_warning("invalid numrecs %d in %s "
					"block %u/%u", numrecs,
					typtab[btype].name, agno, agbno);
			numrecs = mp->m_inobt_mxr[0];
		}
		rp = XFS_INOBT_REC_ADDR(mp, block, 1);
		for (i = 0; i < numrecs; i++, rp++) {
			if (!copy_inode_chunk(agno, rp))
				return 0;
		}
		return 1;
	}

	if (numrecs > mp->m_inobt_mxr[1]) {
		if (show_warnings)
			print_warning("invalid numrecs %d in %s block %u/%u",
				numrecs, typtab[btype].name, agno, agbno);
		numrecs = mp->m_inobt_mxr[1];
	}

	pp = XFS_INOBT_PTR_ADDR(mp, block, 1, mp->m_inobt_mxr[1]);
	for (i = 0; i < numrecs; i++) {
		if (!valid_bno(agno, be32_to_cpu(pp[i]))) {
			if (show_warnings)
				print_warning("invalid block number (%u/%u) "
					"in %s block %u/%u",
					agno, be32_to_cpu(pp[i]),
					typtab[btype].name, agno, agbno);
			continue;
		}
		if (!scan_btree(agno, be32_to_cpu(pp[i]), level,
				btype, arg, scanfunc_ino))
			return 0;
	}
	return 1;
}

static int
copy_inodes(
	xfs_agnumber_t		agno,
	xfs_agi_t		*agi)
{
	xfs_agblock_t		root;
	int			levels;

	root = be32_to_cpu(agi->agi_root);
	levels = be32_to_cpu(agi->agi_level);

	/* validate root and levels before processing the tree */
	if (root == 0 || root > mp->m_sb.sb_agblocks) {
		if (show_warnings)
			print_warning("invalid block number (%u) in inobt "
					"root in agi %u", root, agno);
		return 1;
	}
	if (levels >= XFS_BTREE_MAXLEVELS) {
		if (show_warnings)
			print_warning("invalid level (%u) in inobt root "
					"in agi %u", levels, agno);
		return 1;
	}

	return scan_btree(agno, root, levels, TYP_INOBT, agi, scanfunc_ino);
}

static int
scan_ag(
	xfs_agnumber_t	agno)
{
	xfs_agf_t	*agf;
	xfs_agi_t	*agi;
	int		stack_count = 0;
	int		rval = 0;

	/* copy the superblock of the AG */
	push_cur();
	stack_count++;
	set_cur(&typtab[TYP_SB], XFS_AG_DADDR(mp, agno, XFS_SB_DADDR),
			XFS_FSS_TO_BB(mp, 1), DB_RING_IGN, NULL);
	if (!iocur_top->data) {
		print_warning("cannot read superblock for ag %u", agno);
		if (stop_on_read_error)
			goto pop_out;
	} else {
		if (!write_buf(iocur_top))
			goto pop_out;
	}

	/* copy the AG free space btree root */
	push_cur();
	stack_count++;
	set_cur(&typtab[TYP_AGF], XFS_AG_DADDR(mp, agno, XFS_AGF_DADDR(mp)),
			XFS_FSS_TO_BB(mp, 1), DB_RING_IGN, NULL);
	agf = iocur_top->data;
	if (iocur_top->data == NULL) {
		print_warning("cannot read agf block for ag %u", agno);
		if (stop_on_read_error)
			goto pop_out;
	} else {
		if (!write_buf(iocur_top))
			goto pop_out;
	}

	/* copy the AG inode btree root */
	push_cur();
	stack_count++;
	set_cur(&typtab[TYP_AGI], XFS_AG_DADDR(mp, agno, XFS_AGI_DADDR(mp)),
			XFS_FSS_TO_BB(mp, 1), DB_RING_IGN, NULL);
	agi = iocur_top->data;
	if (iocur_top->data == NULL) {
		print_warning("cannot read agi block for ag %u", agno);
		if (stop_on_read_error)
			goto pop_out;
	} else {
		if (!write_buf(iocur_top))
			goto pop_out;
	}

	/* copy the AG free list header */
	push_cur();
	stack_count++;
	set_cur(&typtab[TYP_AGFL], XFS_AG_DADDR(mp, agno, XFS_AGFL_DADDR(mp)),
			XFS_FSS_TO_BB(mp, 1), DB_RING_IGN, NULL);
	if (iocur_top->data == NULL) {
		print_warning("cannot read agfl block for ag %u", agno);
		if (stop_on_read_error)
			goto pop_out;
	} else {
		if (!write_buf(iocur_top))
			goto pop_out;
	}

	/* copy AG free space btrees */
	if (agf) {
		if (show_progress)
			print_progress("Copying free space trees of AG %u",
					agno);
		if (!copy_free_bno_btree(agno, agf))
			goto pop_out;
		if (!copy_free_cnt_btree(agno, agf))
			goto pop_out;
	}

	/* copy inode btrees and the inodes and their associated metadata */
	if (agi) {
		if (!copy_inodes(agno, agi))
			goto pop_out;
	}
	rval = 1;
pop_out:
	while (stack_count--)
		pop_cur();
	return rval;
}

static int
copy_ino(
	xfs_ino_t		ino,
	typnm_t			itype)
{
	xfs_agnumber_t		agno;
	xfs_agblock_t		agbno;
	xfs_agino_t		agino;
	int			offset;
	int			rval = 0;

	if (ino == 0)
		return 1;

	agno = XFS_INO_TO_AGNO(mp, ino);
	agino = XFS_INO_TO_AGINO(mp, ino);
	agbno = XFS_AGINO_TO_AGBNO(mp, agino);
	offset = XFS_AGINO_TO_OFFSET(mp, agino);

	if (agno >= mp->m_sb.sb_agcount || agbno >= mp->m_sb.sb_agblocks ||
			offset >= mp->m_sb.sb_inopblock) {
		if (show_warnings)
			print_warning("invalid %s inode number (%lld)",
					typtab[itype].name, (long long)ino);
		return 1;
	}

	push_cur();
	set_cur(&typtab[TYP_INODE], XFS_AGB_TO_DADDR(mp, agno, agbno),
			blkbb, DB_RING_IGN, NULL);
	if (iocur_top->data == NULL) {
		print_warning("cannot read %s inode %lld",
				typtab[itype].name, (long long)ino);
		rval = !stop_on_read_error;
		goto pop_out;
	}
	off_cur(offset << mp->m_sb.sb_inodelog, mp->m_sb.sb_inodesize);

	cur_ino = ino;
	rval = process_inode_data(iocur_top->data, itype);
pop_out:
	pop_cur();
	return rval;
}


static int
copy_sb_inodes(void)
{
	if (!copy_ino(mp->m_sb.sb_rbmino, TYP_RTBITMAP))
		return 0;

	if (!copy_ino(mp->m_sb.sb_rsumino, TYP_RTSUMMARY))
		return 0;

	if (!copy_ino(mp->m_sb.sb_uquotino, TYP_DQBLK))
		return 0;

	return copy_ino(mp->m_sb.sb_gquotino, TYP_DQBLK);
}

static int
copy_log(void)
{
	if (show_progress)
		print_progress("Copying log");

	push_cur();
	set_cur(&typtab[TYP_LOG], XFS_FSB_TO_DADDR(mp, mp->m_sb.sb_logstart),
			mp->m_sb.sb_logblocks * blkbb, DB_RING_IGN, NULL);
	if (iocur_top->data == NULL) {
		pop_cur();
		print_warning("cannot read log");
		return !stop_on_read_error;
	}
	return write_buf(iocur_top);
}

static int
metadump_f(
	int 		argc,
	char 		**argv)
{
	xfs_agnumber_t	agno;
	int		c;
	int		start_iocur_sp;
	char		*p;

	exitcode = 1;
	show_progress = 0;
	show_warnings = 0;
	stop_on_read_error = 0;

	if (mp->m_sb.sb_magicnum != XFS_SB_MAGIC) {
		print_warning("bad superblock magic number %x, giving up",
				mp->m_sb.sb_magicnum);
		return 0;
	}

	while ((c = getopt(argc, argv, "egm:ow")) != EOF) {
		switch (c) {
			case 'e':
				stop_on_read_error = 1;
				break;
			case 'g':
				show_progress = 1;
				break;
			case 'm':
				max_extent_size = (int)strtol(optarg, &p, 0);
				if (*p != '\0' || max_extent_size <= 0) {
					print_warning("bad max extent size %s",
							optarg);
					return 0;
				}
				break;
			case 'o':
				dont_obfuscate = 1;
				break;
			case 'w':
				show_warnings = 1;
				break;
			default:
				print_warning("bad option for metadump command");
				return 0;
		}
	}

	if (optind != argc - 1) {
		print_warning("too few options for metadump (no filename given)");
		return 0;
	}

	metablock = (xfs_metablock_t *)calloc(BBSIZE + 1, BBSIZE);
	if (metablock == NULL) {
		print_warning("memory allocation failure");
		return 0;
	}
	metablock->mb_blocklog = BBSHIFT;
	metablock->mb_magic = cpu_to_be32(XFS_MD_MAGIC);

	if (!create_nametable()) {
		print_warning("memory allocation failure");
		free(metablock);
		return 0;
	}

	block_index = (__be64 *)((char *)metablock + sizeof(xfs_metablock_t));
	block_buffer = (char *)metablock + BBSIZE;
	num_indicies = (BBSIZE - sizeof(xfs_metablock_t)) / sizeof(__be64);
	cur_index = 0;
	start_iocur_sp = iocur_sp;

	if (strcmp(argv[optind], "-") == 0) {
		if (isatty(fileno(stdout))) {
			print_warning("cannot write to a terminal");
			free(nametable);
			free(metablock);
			return 0;
		}
		outf = stdout;
	} else {
		outf = fopen(argv[optind], "wb");
		if (outf == NULL) {
			print_warning("cannot create dump file");
			free(nametable);
			free(metablock);
			return 0;
		}
	}

	exitcode = 0;

	for (agno = 0; agno < mp->m_sb.sb_agcount; agno++) {
		if (!scan_ag(agno)) {
			exitcode = 1;
			break;
		}
	}

	/* copy realtime and quota inode contents */
	if (!exitcode)
		exitcode = !copy_sb_inodes();

	/* copy log if it's internal */
	if ((mp->m_sb.sb_logstart != 0) && !exitcode)
		exitcode = !copy_log();

	/* write the remaining index */
	if (!exitcode)
		exitcode = !write_index();

	if (progress_since_warning)
		fputc('\n', (outf == stdout) ? stderr : stdout);

	if (outf != stdout)
		fclose(outf);

	/* cleanup iocur stack */
	while (iocur_sp > start_iocur_sp)
		pop_cur();

	free(nametable);
	free(metablock);

	return 0;
}
