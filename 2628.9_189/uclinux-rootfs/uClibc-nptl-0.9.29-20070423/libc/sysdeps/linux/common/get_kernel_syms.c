/* vi: set sw=4 ts=4: */
/*
 * get_kernel_syms() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"

struct kernel_sym;
int get_kernel_syms(struct kernel_sym *table attribute_unused);
#ifdef __NR_get_kernel_syms
_syscall1(int, get_kernel_syms, struct kernel_sym *, table);
#else
int get_kernel_syms(struct kernel_sym *table attribute_unused)
{
	__set_errno(ENOSYS);
	return -1;
}
#endif
