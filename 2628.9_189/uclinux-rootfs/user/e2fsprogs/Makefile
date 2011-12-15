# Generated automatically from Makefile.in by configure.
srcdir = .
top_srcdir = .
top_builddir = .
my_dir = .
INSTALL = /usr/bin/install -c


# Beginning of file MCONFIG

all::

check::

SHELL = /bin/sh

COMPRESS_EXT = gz bz2 bz Z

prefix = /home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/romfs/usr
root_prefix = /home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/romfs
exec_prefix = ${prefix}
root_bindir = ${root_prefix}/bin
root_sbindir = ${root_prefix}/sbin
root_libdir = ${root_prefix}/lib
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libdir = ${exec_prefix}/lib
datadir= ${prefix}/share
root_sysconfdir= ${root_prefix}/etc
includedir = ${prefix}/include
mandir = ${prefix}/man
man1dir = $(mandir)/man1
man3dir = $(mandir)/man3
man8dir = $(mandir)/man8
infodir = ${prefix}/info
datadir = ${prefix}/share



INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
CC = mipsel-linux-gcc
BUILD_CC = gcc
DEFS =  -DENABLE_HTREE=1 -DENABLE_SWAPFS=1 -DPACKAGE=\"e2fsprogs\" -DVERSION=\"1.30\" -DSTDC_HEADERS=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_UNISTD_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_LIMITS_H=1 -DHAVE_LOCALE_H=1 -DHAVE_NL_TYPES_H=1 -DHAVE_MALLOC_H=1 -DHAVE_STRING_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_PARAM_H=1 -DHAVE_GETCWD=1 -DHAVE_MUNMAP=1 -DHAVE_PUTENV=1 -DHAVE_SETENV=1 -DHAVE_SETLOCALE=1 -DHAVE_STRCHR=1 -DHAVE_STRCASECMP=1 -DHAVE_STRDUP=1 -DHAVE_STPCPY=1 -DHAVE_LC_MESSAGES=1 -DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDARG_H=1 -DHAVE_ERRNO_H=1 -DHAVE_MALLOC_H=1 -DHAVE_MNTENT_H=1 -DHAVE_PATHS_H=1 -DHAVE_DIRENT_H=1 -DHAVE_GETOPT_H=1 -DHAVE_SETJMP_H=1 -DHAVE_SIGNAL_H=1 -DHAVE_TERMIOS_H=1 -DHAVE_LINUX_FD_H=1 -DHAVE_LINUX_MAJOR_H=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_SYS_MOUNT_H=1 -DHAVE_SYS_SYSMACROS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_NET_IF_H=1 -DHAVE_NETINET_IN_H=1 -DHAVE_VPRINTF=1 -DHAVE_LSEEK64_PROTOTYPE=1 -DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 -DSIZEOF_LONG_LONG=8 -DHAVE_GETRUSAGE=1 -DHAVE_LLSEEK=1 -DHAVE_LSEEK64=1 -DHAVE_OPEN64=1 -DHAVE_STRCASECMP=1 -DHAVE_SRANDOM=1 -DHAVE_FCHOWN=1 -DHAVE_MALLINFO=1 -DHAVE_FDATASYNC=1 -DHAVE_STRNLEN=1 -DHAVE_SYSCONF=1 -DHAVE_PATHCONF=1 -DHAVE_EXT2_IOCTLS=1 
CFLAGS = -g -O2
CPPFLAGS = 
ALL_CFLAGS = $(CPPFLAGS) $(DEFS) $(USE_WFLAGS) $(CFLAGS) $(XTRA_CFLAGS) \
	-I$(top_builddir)/lib -I$(top_srcdir)/lib $(LINUX_INCLUDE) 
LDFLAGS = 
ALL_LDFLAGS = $(LDFLAGS)
RM = /bin/rm
LN = /bin/ln
LN_S = ln -s
MV = /bin/mv
CP = /bin/cp
CHMOD = /bin/chmod
AR = mipsel-linux-ar
AWK = /usr/bin/awk
SED = /bin/sed
PERL = /usr/bin/perl
RANLIB = mipsel-linux-ranlib
STRIP = mipsel-linux-strip
LD = $(PURE) mipsel-linux-gcc
ARUPD = $(AR) r
LDCONFIG = /sbin/ldconfig

#
# Library definitions
#
LIB = $(top_builddir)/lib
LIBSS = $(LIB)/libss.so
LIBCOM_ERR = $(LIB)/libcom_err.so
LIBE2P = $(LIB)/libe2p.so
LIBEXT2FS = $(LIB)/libext2fs.so
LIBUUID = $(LIB)/libuuid.so  
DEPLIBUUID = $(LIB)/libuuid.so

STATIC_LIBSS = $(LIB)/libss.a
STATIC_LIBCOM_ERR = $(LIB)/libcom_err.a
STATIC_LIBE2P = $(LIB)/libe2p.a
STATIC_LIBEXT2FS = $(LIB)/libext2fs.a
STATIC_LIBUUID = $(LIB)/libuuid.a  
DEPSTATIC_LIBUUID = $(LIB)/libuuid.a

PROFILED_LIBSS = $(LIB)/libss
PROFILED_LIBCOM_ERR = $(LIB)/libcom_err
PROFILED_LIBE2P = $(LIB)/libe2p
PROFILED_LIBEXT2FS = $(LIB)/libext2fs
PROFILED_LIBUUID = $(LIB)/libuuid  
DEPPROFILED_LIBUUID = $(LIB)/libuuid

#
# Use these definitions is you use tools 2.x, x < 16
#
#DLL_BIN=/usr/dll/bin
#JUMP_PREFIX=/usr/dll/jump/

#
# Use these definitions if you use tools 2.16 or above
#
DLL_BIN=/usr/bin
JUMP_PREFIX=/usr/bin/jump

# An include directive pointing to a directory holding enough linux-like
# include files to satisfy some programs here
LINUX_INCLUDE=

#
# A fast substitution command for fixing up man pages, shell scripts, etc.
#
SUBST_CONF=$(top_builddir)/util/subst.conf
SUBSTITUTE= $(top_builddir)/util/subst -f $(SUBST_CONF)
DEP_SUBSTITUTE= $(top_builddir)/util/subst $(SUBST_CONF)

$(top_builddir)/util/subst:
	cd $(top_builddir)/util ; $(MAKE) subst

#
# Warning flags
#
# Run make gcc-wall to do a build with warning messages.
#
#
WFLAGS=		-ansi -D_POSIX_SOURCE -pedantic \
			-Wall -Wwrite-strings -Wpointer-arith \
			-Wcast-qual -Wcast-align -Wtraditional \
			-Wstrict-prototypes -Wmissing-prototypes \
			-Wnested-externs -Winline -DNO_INLINE_FUNCS -Wshadow 

gcc-wall-new:
	(make USE_WFLAGS="$(WFLAGS)" > /dev/null) 2>&1 | sed -f $(top_srcdir)/util/gcc-wall-cleanup 

gcc-wall:
	make clean > /dev/null
	make gcc-wall-new

#
# Installation user and groups
#
BINGRP=		bin
BINOWN=		bin
BINMODE=	555
INCGRP=		bin
INCOWN=		bin
INCMODE=	444
LIBOWN=		bin
LIBGRP=		bin
LIBMODE=	444
MANGRP=		bin
MANOWN=		bin
MANMODE=	444

#
# Autoconf magic...
#

DEP_LIB_MAKEFILES = $(top_srcdir)/lib/Makefile.elf-lib  \
	$(top_srcdir)/lib/Makefile.dll-lib $(top_srcdir)/lib/Makefile.bsd-lib \
	$(top_srcdir)/lib/Makefile.checker $(top_srcdir)/lib/Makefile.profile

$(top_builddir)/config.status: $(top_srcdir)/configure
	cd $(top_builddir); ./config.status --recheck

$(top_builddir)/MCONFIG: $(top_srcdir)/MCONFIG.in $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=MCONFIG ./config.status

$(top_builddir)/lib/substitute_sh: $(top_srcdir)/lib/substitute_sh.in \
		$(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=lib/substitute_sh ./config.status

$(top_builddir)/util/subst.conf: $(top_srcdir)/util/subst.conf.in \
		$(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=util/subst.conf ./config.status

Makefile: $(srcdir)/Makefile.in $(top_builddir)/MCONFIG \
		$(DEP_MAKEFILE) $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=$(my_dir)/Makefile ./config.status

$(top_srcdir)/configure: $(top_srcdir)/configure.in
	cd $(top_srcdir) && autoconf

#
# Make depend magic...
#

.depend: Makefile $(SRCS) $(top_srcdir)/depfix.sed $(top_srcdir)/wordwrap.pl
	if test -n "$(SRCS)" ; then \
		$(CC) -M $(ALL_CFLAGS) $(SRCS) | \
			$(SED) -f $(top_srcdir)/depfix.sed \
			    -e 's; $(srcdir)/; $$(srcdir)/;g' \
			    -e 's; $(top_srcdir)/; $$(top_srcdir)/;g' \
			    -e 's; $(top_builddir)/; $$(top_builddir)/;g' \
			    -e 's; \./; ;g' \
			    -e '/^ *\\$$/d' | \
			$(PERL) $(top_srcdir)/wordwrap.pl > .depend; \
	else :; fi

depend:: .depend
	if test -n "$(SRCS)" ; then \
		sed -e '/^# +++ Dependency line eater +++/,$$d' \
			< $(srcdir)/Makefile.in | cat - .depend \
			> $(srcdir)/Makefile.in.new; \
	if cmp -s $(srcdir)/Makefile.in $(srcdir)/Makefile.in.new ; then \
		$(RM) $(srcdir)/Makefile.in.new ; \
	else \
		$(MV) $(srcdir)/Makefile.in $(srcdir)/Makefile.in.old; \
		$(MV) $(srcdir)/Makefile.in.new $(srcdir)/Makefile.in; \
	fi ; else :; fi

# End of file MCONFIG

RESIZE_DIR= resize
DEBUGFS_DIR= debugfs
EVMS_DIR= lib/evms

LIB_SUBDIRS=lib/et lib/ss lib/e2p lib/ext2fs lib/uuid # lib/finddev
PROG_SUBDIRS=e2fsck $(DEBUGFS_DIR) misc $(RESIZE_DIR) tests/progs $(EVMS_DIR)
SUBDIRS=util $(LIB_SUBDIRS) $(PROG_SUBDIRS) tests

SUBS= lib/ext2fs/ext2_types.h

TAR=tar

all:: $(SUBS) 
	for i in $(LIB_SUBDIRS); do \
		$(MAKE) -C $$i all || exit 1; \
	done
	for i in $(PROG_SUBDIRS); do \
		$(MAKE) -C $$i all || exit 1; \
	done
	$(MAKE) -f Makefile install-libs

progs: $(SUBS) all-progs-recursive
libs: $(SUBS) all-libs-recursive

docs:
	cd doc && $(MAKE) libext2fs.info

install-doc-libs:
	cd doc && $(MAKE) install-doc-libs

uninstall-doc-libs:
	cd doc && $(MAKE) uninstall-doc-libs

clean-doc:
	cd doc && $(MAKE) clean

distclean-doc:
	cd doc && $(MAKE) distclean

install: $(SUBS) all-libs-recursive install-progs-recursive \
	install-shlibs-libs-recursive install-doc-libs
#	export MANPATH=$(DESTDIR)$(mandir); $(srcdir)/install-utils/compile_manpages

uninstall: uninstall-progs-recursive uninstall-shlibs-libs-recursive uninstall-doc-libs

install-libs: install-libs-recursive

uninstall-libs: uninstall-libs-recursive

TAGS clean-recursive distclean-recursive depend-recursive check-recursive \
	    mostlyclean-recursive realclean-recursive install-recursive:
	for subdir in $(SUBDIRS); do \
	  if test -d $$subdir ; then \
	    target=`echo $@|$(SED) 's/-recursive//'`; \
	    echo making $$target in $$subdir; \
	    (cd $$subdir && $(MAKE) $$target) || exit 1; \
	  fi ; \
	done

all-progs-recursive install-progs-recursive uninstall-progs-recursive:
	for subdir in $(PROG_SUBDIRS); do \
	  if test -d $$subdir ; then \
	    target=`echo $@|$(SED) 's/-progs-recursive//'`; \
	    echo making $$target in $$subdir; \
	    (cd $$subdir && $(MAKE) $$target) || exit 1; \
	  fi ; \
	done

all-libs-recursive install-libs-recursive uninstall-libs-recursive install-shlibs-libs-recursive uninstall-shlibs-libs-recursive:
	for subdir in $(LIB_SUBDIRS); do \
	  if test -d $$subdir ; then \
	    target=`echo $@|$(SED) 's/-libs-recursive//'`; \
	    echo making $$target in $$subdir; \
	    (cd $$subdir && $(MAKE) $$target) || exit 1; \
	  fi ; \
	done

mostlyclean: mostlyclean-recursive mostlyclean-local

clean: clean-recursive clean-local clean-doc
	$(RM) -f $(SUBS)

distclean: distclean-doc distclean-recursive distclean-local 

realclean: realclean-recursive realclean-local

depend:: depend-recursive

lib/ext2fs/ext2_types.h: $(DEP_SUBSTITUTE) $(srcdir)/lib/ext2fs/ext2_types.h.in
	$(SUBSTITUTE) $(srcdir)/lib/ext2fs/ext2_types.h.in \
	> lib/ext2fs/ext2_types.h

mostlyclean-local:
	$(RM) -f \#* *~ *.orig core MAKELOG 

clean-local: mostlyclean-local

distclean-local: clean-local
	$(RM) -f $(SUBS) $(SUBST_CONF) \
		config.status config.log config.cache MCONFIG Makefile \
		$(srcdir)/TAGS $(srcdir)/Makefile.in.old

realclean-local: distclean-local
	$(RM) -f configure

check::	$(SUBS) check-recursive

distribution_tar_file:
	$(RM) -rf /tmp/dest
	$(MAKE) DESTDIR=/tmp/dest install
	cp -r $(srcdir)/README $(srcdir)/install-utils /tmp/dest
	$(RM) -rf /tmp/dest/install-utils/CVS /tmp/dest/install-utils/ChangeLog
	cp $(srcdir)/INSTALL.elfbin /tmp/dest/INSTALL
	(cd /tmp/dest && $(TAR) cf - . ) | gzip -9 \
		> e2fsprogs-1.30-elfbin.tar.gz

SRCROOT = `echo e2fsprogs-1.30 | sed -e 's/-WIP//' \
			-e 's/pre-//' -e 's/-PLUS//'`

$(srcdir)/.exclude-file:
	a=$(SRCROOT); \
	(cd $(srcdir)/.. && find e2fsprogs \( -name \*~ -o -name \*.orig \
		-o -name CVS -o -name \*.rej -o -name Makefile.pq \
		-o -name TAGS -o -name \*.old -o -name \*.gmo -o -name SCCS \
		-o -name changed-files -o -name .#\* \) \
		-print) | sed -e "s/e2fsprogs/$$a/" > $(srcdir)/.exclude-file
	echo "$(SRCROOT)/BitKeeper" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/build" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/rpm.log" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/TODO" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/powerquest" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/.exclude-file" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/po/stamp-cat-id" >> $(srcdir)/.exclude-file
	echo "$(SRCROOT)/po/cat-id-tbl.c" >> $(srcdir)/.exclude-file
	echo $(SRCROOT)/e2fsprogs-1.30.tar.gz \
		>> $(srcdir)/.exclude-file

source_tar_file: $(srcdir)/.exclude-file
	cd $(srcdir)/.. && a=$(SRCROOT); rm -f $$a ; ln -sf e2fsprogs $$a ; \
		$(TAR) -c -h -v -f - \
			-X $$a/.exclude-file $$a | \
		gzip -9 > e2fsprogs-1.30.tar.gz
	rm -f $(srcdir)/.exclude-file
