/*
 * util.c --- utilities for the debugfs program
 * 
 * Copyright (C) 1993, 1994 Theodore Ts'o.  This file may be
 * redistributed under the terms of the GNU Public License.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "debugfs.h"

FILE *open_pager(void)
{
	FILE *outfile;
	const char *pager = getenv("PAGER");

	signal(SIGPIPE, SIG_IGN);
	if (pager) {
		if (strcmp(pager, "__none__") == 0) {
			return stdout;
		}
	} else
		pager = "more";

	outfile = popen(pager, "w");

	return (outfile ? outfile : stdout);
}

void close_pager(FILE *stream)
{
	if (stream && stream != stdout) pclose(stream);
}

/*
 * This routine is used whenever a command needs to turn a string into
 * an inode.
 */
ext2_ino_t string_to_inode(char *str)
{
	ext2_ino_t	ino;
	int		len = strlen(str);
	char		*end;
	int		retval;

	/*
	 * If the string is of the form <ino>, then treat it as an
	 * inode number.
	 */
	if ((len > 2) && (str[0] == '<') && (str[len-1] == '>')) {
		ino = strtoul(str+1, &end, 0);
		if (*end=='>')
			return ino;
	}

	retval = ext2fs_namei(current_fs, root, cwd, str, &ino);
	if (retval) {
		com_err(str, retval, "");
		return 0;
	}
	return ino;
}

/*
 * This routine returns 1 if the filesystem is not open, and prints an
 * error message to that effect.
 */
int check_fs_open(char *name)
{
	if (!current_fs) {
		com_err(name, 0, "Filesystem not open");
		return 1;
	}
	return 0;
}

/*
 * This routine returns 1 if a filesystem is open, and prints an
 * error message to that effect.
 */
int check_fs_not_open(char *name)
{
	if (current_fs) {
		com_err(name, 0,
			"Filesystem %s is still open.  Close it first.\n",
			current_fs->device_name);
		return 1;
	}
	return 0;
}

/*
 * This routine returns 1 if a filesystem is not opened read/write,
 * and prints an error message to that effect.
 */
int check_fs_read_write(char *name)
{
	if (!(current_fs->flags & EXT2_FLAG_RW)) {
		com_err(name, 0, "Filesystem opened read/only");
		return 1;
	}
	return 0;
}

/*
 * This routine returns 1 if a filesystem is doesn't have its inode
 * and block bitmaps loaded, and prints an error message to that
 * effect.
 */
int check_fs_bitmaps(char *name)
{
	if (!current_fs->block_map || !current_fs->inode_map) {
		com_err(name, 0, "Filesystem bitmaps not loaded");
		return 1;
	}
	return 0;
}

/*
 * This function takes a __u32 time value and converts it to a string,
 * using ctime
 */
char *time_to_string(__u32 cl)
{
	time_t	t = (time_t) cl;

	return ctime(&t);
}

/*
 * This function will convert a string to an unsigned long, printing
 * an error message if it fails, and returning success or failure in err.
 */
unsigned long parse_ulong(const char *str, const char *cmd,
			  const char *descr, int *err)
{
	char		*tmp;
	unsigned long	ret;
	
	ret = strtoul(str, &tmp, 0);
	if (*tmp == 0) {
		if (err)
			*err = 0;
		return ret;
	}
	com_err(cmd, 0, "Bad %s - %s", descr, str);
	if (err)
		*err = 1;
	else
		exit(1);
	return 0;
}

/*
 * This function will convert a string to a block number.  It returns
 * 0 on success, 1 on failure.
 */
int strtoblk(const char *cmd, const char *str, blk_t *ret)
{
	blk_t	blk;
	int	err;

	blk = parse_ulong(str, cmd, "block number", &err);
	*ret = blk;
	if (err == 0 && blk == 0) {
		com_err(cmd, 0, "Invalid block number 0");
		err = 1;
	}
	return err;
}

/*
 * This is a common helper function used by the command processing
 * routines
 */
int common_args_process(int argc, char *argv[], int min_argc, int max_argc,
			const char *cmd, const char *usage, int flags)
{
	if (argc < min_argc || argc > max_argc) {
		com_err(argv[0], 0, "Usage: %s %s", cmd, usage);
		return 1;
	}
	if (flags & CHECK_FS_NOTOPEN) {
		if (check_fs_not_open(argv[0]))
			return 1;
	} else {
		if (check_fs_open(argv[0]))
			return 1;
	}
	if ((flags & CHECK_FS_RW) && check_fs_read_write(argv[0]))
		return 1;
	if ((flags & CHECK_FS_BITMAPS) && check_fs_bitmaps(argv[0]))
		return 1;
	return 0;
}

/*
 * This is a helper function used by do_stat, do_freei, do_seti, and
 * do_testi, etc.  Basically, any command which takes a single
 * argument which is a file/inode number specifier.
 */
int common_inode_args_process(int argc, char *argv[],
			      ext2_ino_t *inode, int flags)
{
	if (common_args_process(argc, argv, 2, 2, argv[0], "<file>", flags))
		return 1;
	
	*inode = string_to_inode(argv[1]);
	if (!*inode) 
		return 1;
	return 0;
}

/*
 * This is a helper function used by do_freeb, do_setb, and do_testb
 */
int common_block_args_process(int argc, char *argv[],
			      blk_t *block, int *count)
{
	int	err;

	if (common_args_process(argc, argv, 2, 3, argv[0],
				"<block> [count]", CHECK_FS_BITMAPS))
		return 1;

	if (strtoblk(argv[0], argv[1], block))
		return 1;
	if (argc > 2) {
		*count = parse_ulong(argv[2], argv[0], "count", &err);
		if (err)
			return 1;
	}
	return 0;
}

int debugfs_read_inode(ext2_ino_t ino, struct ext2_inode * inode,
			const char *cmd)
{
	int retval;

	retval = ext2fs_read_inode(current_fs, ino, inode);
	if (retval) {
		com_err(cmd, retval, "while reading inode %u", ino);
		return 1;
	}
	return 0;
}

int debugfs_write_inode(ext2_ino_t ino, struct ext2_inode * inode,
			const char *cmd)
{
	int retval;

	retval = ext2fs_write_inode(current_fs, ino, inode);
	if (retval) {
		com_err(cmd, retval, "while writing inode %u", ino);
		return 1;
	}
	return 0;
}

