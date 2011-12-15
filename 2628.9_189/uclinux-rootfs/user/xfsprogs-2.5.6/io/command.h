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

#define min(a,b)	(((a)<(b))?(a):(b))

typedef int (*cfunc_t)(int argc, char **argv);
typedef void (*helpfunc_t)(void);

typedef struct cmdinfo {
	const char	*name;
	const char	*altname;
	cfunc_t		cfunc;
	int		argmin;
	int		argmax;
	int		canpush;
	const char	*args;
	const char	*oneline;
	helpfunc_t      help;
} cmdinfo_t;

extern cmdinfo_t	*cmdtab;
extern int		ncmds;

extern void		add_command(const cmdinfo_t *ci);
extern int		command(int argc, char **argv);
extern const cmdinfo_t	*find_command(const char *cmd);
extern void		init_commands(void);

extern void		bmap_init(void);
extern void		help_init(void);
extern void		open_init(void);
extern void		pread_init(void);
extern void		prealloc_init(void);
extern void		pwrite_init(void);
extern void		quit_init(void);
extern void		resblks_init(void);
extern void		fsync_init(void);
extern void		truncate_init(void);

extern off64_t		filesize(void);
extern int		openfile(char *, xfs_fsop_geom_t *,
					int, int, int, int, int, int, int);

extern void		*buffer;
extern ssize_t		buffersize;
extern int		alloc_buffer(ssize_t, unsigned int);
extern int		read_buffer(int, off64_t, ssize_t, ssize_t *, int, int);
