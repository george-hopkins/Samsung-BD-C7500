/*
 * Simple command-line tool to set affinity
 * 	Robert Love, 20020311
 *  Modified by Troy Trammel
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <linux/unistd.h>

/*
 * provide the proper syscall information if our libc
 * is not yet updated.
 */
_syscall3 (int, sched_setaffinity, pid_t, pid, unsigned int, len, unsigned long *, user_mask_ptr)
_syscall3 (int, sched_getaffinity, pid_t, pid, unsigned int, len, unsigned long *, user_mask_ptr)

int main(int argc, char * argv[])
{
	unsigned long new_mask;
	unsigned long cur_mask;
	unsigned int len = sizeof(new_mask);
	pid_t pid;

	if (argc != 3) {
		printf(" usage: %s <pid> <cpu_mask>\n", argv[0]);
		return -1;
	}

	pid = atol(argv[1]);
	sscanf(argv[2], "%08lx", &new_mask);

	if (sched_getaffinity(pid, len, &cur_mask) < 0) {
		printf("error: could not get pid %d's affinity.\n", pid);
		return -1;
	}

	printf(" pid %d's old affinity: %08lx\n", pid, cur_mask);

	if (sched_setaffinity(pid, len, &new_mask)) {
		printf("error: could not set pid %d's affinity.\n", pid);
		return -1;
	}

	if (sched_getaffinity(pid, len, &cur_mask) < 0) {
		printf("error: could not get pid %d's affinity.\n", pid);
		return -1;
	}

	printf(" pid %d's new affinity: %08lx\n", pid, cur_mask);

	return 0;
}
