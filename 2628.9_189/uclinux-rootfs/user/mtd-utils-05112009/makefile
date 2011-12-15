#
# General settings
#

include $(ROOTDIR)/config.arch

BOGUS_TARGETS		:= FORCE makefile $(ROOTDIR)/config.arch

CC			:= $(MACHINE)-linux-gcc
STRIP			:= $(MACHINE)-linux-strip
CFLAGS			:= -Os

export CC CFLAGS

#
# Settings that are specific to this package
#

TARGETS		:= flash_eraseall flash_erase nanddump flash_unlock \
	flash_lock flash_info mtd_debug nandwrite nandtest sumtool 

ifneq ($(CONFIG_MTD_BRCMNAND),)
TARGETS +=  nandinfo nandstat geneccerror bbtmod ft_mips
endif

BUILDDIR	:= .

export BUILDDIR

DESTDIR		:= $(shell pwd)/out

#
# Build targets
#

.PHONY: all
all::
	$(MAKE) -f Makefile TARGETS="$(TARGETS)" SUBDIRS=""
	$(MAKE) -C ubi-utils SUBDIRS=""

.PHONY: romfs
romfs::
	rm -rf $(DESTDIR)
	mkdir -p $(DESTDIR)
	$(MAKE) -C ubi-utils install SUBDIRS="" DESTDIR=$(DESTDIR) SBINDIR=bin
	$(STRIP) $(TARGETS) $(DESTDIR)/bin/*
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE /bin/flash_erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE -s /bin/flash_erase /bin/erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL /bin/flash_eraseall
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL -s /bin/flash_eraseall /bin/eraseall
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE /bin/flash_info	
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_INFO /bin/flash_info
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDDUMP /bin/nanddump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDTEST /bin/nandtest
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDWRITE /bin/nandwrite
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MTD_DEBUG /bin/mtd_debug

ifneq ($(CONFIG_FTL),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_CHECK    /bin/ftl_check
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_FORMAT   /bin/ftl_format
endif
ifneq ($(CONFIG_NFTL),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTLDUMP     /bin/nftldump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTL_FORMAT  /bin/nftl_format
endif


ifneq ($(CONFIG_JFFS2_SUMMARY),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_SUMTOOL /bin/sumtool
endif

ifneq ($(CONFIG_JFFS_FS),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS     $(DESTDIR)/mkfs.jffs /bin/mkfs.jffs
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS     $(DESTDIR)/mtd_debug /bin/mtd_debug
endif
ifneq ($(CONFIG_JFFS2_FS),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2    $(DESTDIR)/mkfs.jffs2 /bin/mkfs.jffs2
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2    $(DESTDIR)/mtd_debug /bin/mtd_debug
endif


	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_UBI_UTILS $(DESTDIR)/bin /bin/

ifneq ($(CONFIG_MTD_BRCMNAND),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDINFO     /bin/nandinfo
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDSTAT     /bin/nandstat
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_GENECCERROR  /bin/geneccerror
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_BBTMOD       /bin/bbtmod
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTMIPS   $(DESTDIR)/ft_mips    /bin/ft_mips
endif


.PHONY: clean distclean
clean distclean::
	$(MAKE) -f Makefile clean SUBDIRS=""
	$(MAKE) -C ubi-utils clean SUBDIRS=""
	rm -rf $(DESTDIR)

# These targets must not be passed through to the original Makefile
.PHONY: $(BOGUS_TARGETS)
$(BOGUS_TARGETS)::

# Everything else (maybe including clean, distclean) does get passed through
%:: FORCE
	$(MAKE) -f Makefile $@
