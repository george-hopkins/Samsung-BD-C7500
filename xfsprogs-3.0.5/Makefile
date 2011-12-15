#
# Copyright (c) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
#

TOPDIR = .
HAVE_BUILDDEFS = $(shell test -f $(TOPDIR)/include/builddefs && echo yes || echo no)

ifeq ($(HAVE_BUILDDEFS), yes)
include $(TOPDIR)/include/builddefs
endif

CONFIGURE = aclocal.m4 configure config.guess config.sub install-sh ltmain.sh
LSRCFILES = configure.in Makepkgs release.sh README VERSION $(CONFIGURE)

LDIRT = config.log .dep config.status config.cache confdefs.h conftest* \
	Logs/* built .census install.* install-dev.* *.gz autom4te.cache/* \
	libtool include/builddefs include/platform_defs.h

LIB_SUBDIRS = libxfs libxlog libxcmd libhandle $(LDISK)
TOOL_SUBDIRS = copy db estimate fsck fsr growfs io logprint mkfs quota \
		mdrestore repair rtcp m4 man doc po debian build

ifneq ($(ENABLE_BLKID), yes)
LIB_SUBDIRS += libdisk
endif

SUBDIRS = include $(LIB_SUBDIRS) $(TOOL_SUBDIRS)

default: include/builddefs include/platform_defs.h
ifeq ($(HAVE_BUILDDEFS), no)
	$(MAKE) -C . $@
else
	$(MAKE) $(SUBDIRS)
endif

# tool/lib dependencies
$(LIB_SUBDIRS) $(TOOL_SUBDIRS): include
copy mdrestore: libxfs
db logprint: libxfs libxlog
fsr: libhandle
growfs: libxfs libxcmd
io: libxcmd libhandle
mkfs: libxfs
quota: libxcmd
repair: libxfs libxlog

ifneq ($(ENABLE_BLKID), yes)
mkfs: libdisk
endif

ifeq ($(HAVE_BUILDDEFS), yes)
include $(BUILDRULES)
else
clean:	# if configure hasn't run, nothing to clean
endif

# Recent versions of libtool require the -i option for copying auxiliary
# files (config.sub, config.guess, install-sh, ltmain.sh), while older
# versions will copy those files anyway, and don't understand -i.
LIBTOOLIZE_INSTALL = `libtoolize -n -i >/dev/null 2>/dev/null && echo -i`

configure:
	libtoolize -c $(LIBTOOLIZE_INSTALL) -f
	cp include/install-sh .
	aclocal -I m4
	autoconf

include/builddefs: configure
	./configure $$LOCAL_CONFIGURE_OPTIONS
	touch .census

include/platform_defs.h: include/builddefs
## Recover from the removal of $@
	@if test -f $@; then :; else \
		rm -f include/builddefs; \
		$(MAKE) $(AM_MAKEFLAGS) include/builddefs; \
	fi

install: default $(addsuffix -install,$(SUBDIRS))
	$(INSTALL) -m 755 -d $(PKG_DOC_DIR)
	$(INSTALL) -m 644 README $(PKG_DOC_DIR)

install-dev: default $(addsuffix -install-dev,$(SUBDIRS))

install-qa: install $(addsuffix -install-qa,$(SUBDIRS))

%-install:
	$(MAKE) -C $* install

%-install-dev:
	$(MAKE) -C $* install-dev

%-install-qa:
	$(MAKE) -C $* install-qa

distclean: clean
	rm -f $(LDIRT)

realclean: distclean
	rm -f $(CONFIGURE)
