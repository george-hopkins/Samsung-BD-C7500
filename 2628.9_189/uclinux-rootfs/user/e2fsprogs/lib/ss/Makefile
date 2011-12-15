# Generated automatically from Makefile.in by configure.
#
# Makefile for lib/ss
#

srcdir = .
top_srcdir = ../..
top_builddir = ../..
my_dir = lib/ss
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

LIBRARY= libss
LIBDIR= ss

DLL_ADDRESS = 0x66880000
DLL_JUMPSIZE = 0x1000
DLL_GOTSIZE  = 0x1000
DLL_VERSION = 1.0
DLL_IMAGE = libss
DLL_STUB = libss
DLL_LIBS = -L../.. -lcom_err
DLL_MYDIR = ss
DLL_INSTALL_DIR = $(root_libdir)

ELF_VERSION = 2.0
ELF_SO_VERSION = 2
ELF_IMAGE = libss
ELF_MYDIR = ss
ELF_INSTALL_DIR = $(root_libdir)
ELF_OTHER_LIBS = -L../.. -lcom_err

BSDLIB_VERSION = 1.0
BSDLIB_IMAGE = libss
BSDLIB_MYDIR = ss
BSDLIB_INSTALL_DIR = $(root_libdir)

TAGS=etags
COMPILE_ET=../et/compile_et
MK_CMDS=_SS_DIR_OVERRIDE=. ./mk_cmds

# -I.. is so that ss/ss_err.h works
# -I$(srcdir)/.. is so that ss/ss.h works
# -I$(srcdir)/../et is so com_err.h works
XTRA_CFLAGS=-DPOSIX_SIGNALS -I$(srcdir)/../et

.c.o:
	$(CC) $(ALL_CFLAGS) -c $<
#	$(CC) $(ALL_CFLAGS) -g -pg -o profiled/$*.o -c $<
#	$(CC) $(ALL_CFLAGS) -checker -g -o checker/$*.o -c $<
#	(export JUMP_DIR=`pwd`/jump; $(CC) -B$(JUMP_PREFIX) \
#		$(ALL_CFLAGS) -o jump/$*.o -c $<)
	$(CC) $(ALL_CFLAGS) -fPIC -o elfshared/$*.o -c $<
#	$(CC) $(ALL_CFLAGS) -fpic -o pic/$*.o -c $<

# for the library

LIB=	libss.a

# with ss_err.o first, ss_err.h should get rebuilt first too.  should not
# be relying on this, though.
OBJS=	ss_err.o \
	std_rqs.o \
	invocation.o help.o \
	execute_cmd.o listen.o parse.o error.o prompt.o \
	request_tbl.o list_rqs.o pager.o requests.o \
	data.o

SRCS=	$(srcdir)/invocation.c $(srcdir)/help.c \
	$(srcdir)/execute_cmd.c $(srcdir)/listen.c $(srcdir)/parse.c \
	$(srcdir)/error.c $(srcdir)/prompt.c $(srcdir)/request_tbl.c \
	$(srcdir)/list_rqs.c $(srcdir)/pager.c $(srcdir)/requests.c \
	$(srcdir)/data.c 

all:: mk_cmds


all:: subdirs $(LIBRARY).a

install-shlibs::

subdirs::

# THT: 09/24/04 Replace hardlink with symlinks.  Hardlink does not work
# when building using a Clearcase dynamic view 
# (complains about cross-device link).
$(LIBRARY).a: $(OBJS)
	(if test -r $@; then $(RM) -f $@.bak && $(MV) $@ $@.bak; fi)
	$(ARUPD) $@ $(OBJS)
	-$(RANLIB) $@
	$(RM) -f ../$@
	$(LN) $@ ../$@



#
# This is a Makefile stub which handles the creation of Linux ELF shared
# libraries.
#
# In order to use this stub, the following makefile variables must be defined.
# 
# ELF_VERSION = 1.0
# ELF_SO_VERSION = 1
# ELF_IMAGE = libce
# ELF_MYDIR = et
# ELF_INSTALL_DIR = $(SHLIBDIR)
# ELF_OTHER_LIBS = -lc

all:: elfshared image

subdirs:: elfshared

elfshared:
	mkdir elfshared

ELF_LIB = $(ELF_IMAGE).so.$(ELF_VERSION)
ELF_SONAME = $(ELF_IMAGE).so.$(ELF_SO_VERSION)

image:		$(ELF_LIB)

$(ELF_LIB): $(OBJS)
	(cd elfshared; $(CC) --shared -o $(ELF_LIB) -Wl,-soname,$(ELF_SONAME) \
		$(OBJS) $(ELF_OTHER_LIBS))
	$(MV) elfshared/$(ELF_LIB) .
	$(RM) -f ../$(ELF_LIB) ../$(ELF_IMAGE).so ../$(ELF_SONAME)
	$(LN) $(ELF_LIB) ../$(ELF_LIB)
	$(LN) ../$(ELF_LIB) ../$(ELF_IMAGE).so
	$(LN) ../$(ELF_LIB) ../$(ELF_SONAME)

installdirs-elf-lib::
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(ELF_INSTALL_DIR) \
		$(DESTDIR)$(libdir)

installdirs:: installdirs-elf-lib

install-shlibs install:: $(ELF_LIB) installdirs-elf-lib
	$(INSTALL_PROGRAM) $(ELF_LIB) $(DESTDIR)$(ELF_INSTALL_DIR)/$(ELF_LIB)
	$(STRIP) --strip-debug \
		$(DESTDIR)$(ELF_INSTALL_DIR)/$(ELF_LIB)
	$(LN_S) -f $(ELF_LIB) $(DESTDIR)$(ELF_INSTALL_DIR)/$(ELF_SONAME)
	$(LN_S) -f $(ELF_INSTALL_DIR)/$(ELF_SONAME) \
		$(DESTDIR)$(libdir)/$(ELF_IMAGE).so
	-$(LDCONFIG)

uninstall-shlibs uninstall::
	$(RM) -f $(DESTDIR)$(ELF_INSTALL_DIR)/$(ELF_LIB) \
		$(DESTDIR)$(ELF_INSTALL_DIR)/$(ELF_SONAME) \
		$(DESTDIR)$(libdir)/$(ELF_IMAGE).so
	-$(LDCONFIG)

clean::
	$(RM) -rf elfshared
	$(RM) -f $(ELF_LIB)
	$(RM) -f ../$(ELF_LIB) ../$(ELF_IMAGE).so ../$(ELF_SONAME)




CODE= $(SRCS) $(MKCMDSFILES)

MKCMDSOBJS=	mk_cmds.o utils.o options.o ct.tab.o cmd_tbl.lex.o

MKCMDSFILES=	mk_cmds.c utils.c options.c ct.y cmd_tbl.lex.l

MKCMDSCSRCS=	mk_cmds.c utils.c options.c ct.tab.c cmd_tbl.lex.c


HFILES=	ss.h ss_internal.h
SHARE_FILES= ct_c.awk ct_c.sed

INSTALL_HFILES=	ss.h 

# for 'tags' and dependencies

CFILES=	$(SRCS) $(MKCMDSCSRCS) test_ss.c

# for building archives

FILES=	$(SRCS) $(MKCMDSFILES) $(HFILES) \
	ss_err.et std_rqs.ct Makefile \
	test_ss.c ss 

all::	libss.a # libss_p.a lint

std_rqs.c: std_rqs.ct mk_cmds
	DIR=$(srcdir) $(MK_CMDS) $(srcdir)/std_rqs.ct

ss_err.c ss_err.h: ss_err.et
	$(COMPILE_ET) $(srcdir)/ss_err.et

ct.tab.c ct.tab.h: ct.y
	$(RM) -f ct.tab.* y.*
	$(YACC) -d $(srcdir)/ct.y
	$(MV) -f y.tab.c ct.tab.c
	$(MV) -f y.tab.h ct.tab.h

#libss.o:	$(OBJS)
#	$(LD) -r -s -o $@ $(OBJS)
#	$(CHMOD) -x $@

mk_cmds: $(DEP_SUBSTITUTE) $(srcdir)/mk_cmds.sh.in
	$(SUBSTITUTE) $(srcdir)/mk_cmds.sh.in mk_cmds
	$(CHMOD) +x mk_cmds

installdirs::
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(libdir) \
		$(DESTDIR)$(includedir)/ss $(DESTDIR)$(datadir)/ss \
		$(DESTDIR)$(bindir)

install:: libss.a $(INSTALL_HFILES) installdirs ss_err.h mk_cmds
	$(INSTALL_DATA) libss.a $(DESTDIR)$(libdir)/libss.a
	$(CHMOD) 644 $(DESTDIR)$(libdir)/libss.a
	-$(RANLIB) $(DESTDIR)$(libdir)/libss.a
	$(CHMOD) $(LIBMODE) $(DESTDIR)$(libdir)/libss.a
	$(RM) -f $(DESTDIR)$(includedir)/ss/*
	for i in $(INSTALL_HFILES); do \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(includedir)/ss/$$i; \
	done
	$(INSTALL_DATA) ss_err.h $(DESTDIR)$(includedir)/ss/ss_err.h
	for i in $(SHARE_FILES); do \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(datadir)/ss/$$i; \
	done
	$(INSTALL) mk_cmds $(DESTDIR)$(bindir)/mk_cmds

uninstall::
	$(RM) -f $(DESTDIR)$(libdir)/libss.a $(DESTDIR)$(bindir)/mk_cmds
	$(RM) -rf $(DESTDIR)$(includedir)/ss $(DESTDIR)$(datadir)/ss

clean::
	$(RM) -f ../libss.a libss.a mk_cmds ss_err.h ss_err.c std_rqs.c \
		*.o *~ \#* *.bak core 

mostlyclean:: clean
distclean:: clean
	$(RM) -f .depend Makefile $(srcdir)/TAGS $(srcdir)/Makefile.in.old

#
# Hack to parallel makes recognize dependencies correctly.
#
$(top_builddir)/lib/ss/ss_err.h: ss_err.h

# +++ Dependency line eater +++
# 
# Makefile dependencies follow.  This must be the last section in
# the Makefile.in file
#
invocation.o: $(srcdir)/invocation.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
help.o: $(srcdir)/help.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
execute_cmd.o: $(srcdir)/execute_cmd.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
listen.o: $(srcdir)/listen.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
parse.o: $(srcdir)/parse.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
error.o: $(srcdir)/error.c $(top_srcdir)/lib/et/com_err.h \
 $(srcdir)/ss_internal.h $(srcdir)/ss.h $(top_builddir)/lib/ss/ss_err.h
prompt.o: $(srcdir)/prompt.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
request_tbl.o: $(srcdir)/request_tbl.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
list_rqs.o: $(srcdir)/list_rqs.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
pager.o: $(srcdir)/pager.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
requests.o: $(srcdir)/requests.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
data.o: $(srcdir)/data.c $(srcdir)/ss_internal.h $(srcdir)/ss.h \
 $(top_builddir)/lib/ss/ss_err.h
