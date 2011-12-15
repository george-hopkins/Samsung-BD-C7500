/*
 *  linux/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *
 *  May be freely distributed as part of Linux.
 */

#include <linux/compile.h>
#include <linux/module.h>
#include <linux/uts.h>
#include <linux/utsname.h>
#include <linux/utsrelease.h>
#include <linux/version.h>

#ifndef CONFIG_KALLSYMS
#define version(a) Version_ ## a
#define version_string(a) version(a)

extern int version_string(LINUX_VERSION_CODE);
int version_string(LINUX_VERSION_CODE);
#endif

struct uts_namespace init_uts_ns = {
	.kref = {
		.refcount	= ATOMIC_INIT(2),
	},
	.name = {
		.sysname	= UTS_SYSNAME,
		.nodename	= UTS_NODENAME,
		.release	= UTS_RELEASE,
		.version	= UTS_VERSION,
		.machine	= UTS_MACHINE,
		.domainname	= UTS_DOMAINNAME,
	},
};
EXPORT_SYMBOL_GPL(init_uts_ns);

/* HACK for brcm driver */
struct new_utsname system_utsname = {
        .sysname        = UTS_SYSNAME,
        .nodename       = UTS_NODENAME,
        .release        = UTS_RELEASE,
        .version        = UTS_VERSION,
        .machine        = UTS_MACHINE,
        .domainname     = UTS_DOMAINNAME,
};

EXPORT_SYMBOL(system_utsname);



/* FIXED STRINGS! Don't touch! */
#ifndef BRCM_VERSION
const char linux_banner[] =
	"Linux version " UTS_RELEASE " (" LINUX_COMPILE_BY "@"
	LINUX_COMPILE_HOST ") (" LINUX_COMPILER ") " UTS_VERSION "\n";
#else
/* Display Brcm version if defined.  This is defined in the build script build.mk */
const char linux_banner[] = 
	"Linux version " UTS_RELEASE  " build version " BRCM_VERSION " (" LINUX_COMPILE_BY "@"
	LINUX_COMPILE_HOST ") (" LINUX_COMPILER  ") " UTS_VERSION "\n";
#endif

const char linux_proc_banner[] =
	"%s version %s"
	" (" LINUX_COMPILE_BY "@" LINUX_COMPILE_HOST ")"
	" (" LINUX_COMPILER ") %s\n";
