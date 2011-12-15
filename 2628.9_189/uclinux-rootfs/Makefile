############################################################################

#
# Makefile -- Top level uClinux makefile.
#
# Copyright (c) 2001-2002, SnapGear (www.snapgear.com)
# Copyright (c) 2001, Lineo
#

VERSIONPKG = 1.3.4
VERSIONSTR = $(CONFIG_VENDOR)/$(CONFIG_PRODUCT) Version $(VERSIONPKG)

# PR25899 - When no target is specificed, give help
show_targets:
	make -f build.mk show_targets
	make -f build-be.mk show_targets

############################################################################
#
# Lets work out what the user wants, and if they have configured us yet
#

ifeq (.config,$(wildcard .config))
include .config

# THT Add prepare target for Linux 2.6.x
#all: prepare subdirs modules modules_install romfs linux image
all: prepare subdirs modules linux modules_install romfs zImage
else
all: config_error
endif

############################################################################
#
# Get the core stuff worked out
#

LINUXDIR = $(CONFIG_LINUXDIR)
LIBCDIR  = $(CONFIG_LIBCDIR)
ROOTDIR  = $(shell pwd)
PATH	 := $(PATH):$(ROOTDIR)/tools
HOSTCC   = unset GCC_EXEC_PREFIX; cc
IMAGEDIR = $(ROOTDIR)/images
ROMFSDIR = $(ROOTDIR)/romfs
ROMFSINST= romfs-inst.sh
SCRIPTSDIR = $(ROOTDIR)/config/scripts
TFTPDIR    = /tftpboot

LINUX_CONFIG  = $(ROOTDIR)/$(LINUXDIR)/.config
CONFIG_CONFIG = $(ROOTDIR)/config/.config
MODULES_CONFIG = $(ROOTDIR)/modules/.config

LINUX_VERSION := $(shell cat $(ROOTDIR)/$(LINUXDIR)/include/linux/utsrelease.h | grep UTS_RELEASE|cut -d" " -f3 | sed 's/\"//g')

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

MAKEARCH = $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

ifeq (config.arch,$(wildcard config.arch))
include config.arch
ARCH_CONFIG = $(ROOTDIR)/config.arch
export ARCH_CONFIG
endif

DIRS    = $(VENDOR_TOPDIRS) lib user
VENDDIR = $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/.

export VENDOR PRODUCT ROOTDIR LINUXDIR HOSTCC CONFIG_SHELL
export CONFIG_CONFIG LINUX_CONFIG ROMFSDIR SCRIPTSDIR
export VERSIONPKG VERSIONSTR ROMFSINST PATH IMAGEDIR TFTPDIR


prepare:
	$(MAKEARCH) -C $(LINUXDIR) prepare0


############################################################################

#
# Config stuff,  we recall ourselves to load the new config.arch before
# running the kernel and other config scripts
#

.PHONY: config.tk config.in

config.in:
	config/mkconfig > config.in

config.tk: config.in
	$(MAKE) -C $(SCRIPTSDIR) tkparse
	ARCH=dummy $(SCRIPTSDIR)/tkparse < config.in > config.tmp
	@if [ -f /usr/local/bin/wish ];	then \
		echo '#!'"/usr/local/bin/wish -f" > config.tk; \
	else \
		echo '#!'"/usr/bin/wish -f" > config.tk; \
	fi
	cat $(SCRIPTSDIR)/header.tk >> ./config.tk
	cat config.tmp >> config.tk
	rm -f config.tmp
	echo "set defaults \"/dev/null\"" >> config.tk
	echo "set help_file \"config/Configure.help\"" >> config.tk
	cat $(SCRIPTSDIR)/tail.tk >> config.tk
	chmod 755 config.tk

.PHONY: xconfig
xconfig: config.tk
	@wish -f config.tk
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_xconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_xconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_xconfig; \
	 fi
	@config/setconfig final

.PHONY: config
config: config.in
	@HELP_FILE=config/Configure.help \
		$(CONFIG_SHELL) $(SCRIPTSDIR)/Configure config.in
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_config; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_config; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_config; \
	 fi
	@config/setconfig final

.PHONY: menuconfig
menuconfig: config.in
	$(MAKE) -C $(SCRIPTSDIR)/lxdialog all
	@HELP_FILE=config/Configure.help \
		$(CONFIG_SHELL) $(SCRIPTSDIR)/Menuconfig config.in
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_menuconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_menuconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_menuconfig; \
	 fi
	 @if egrep "^CONFIG_DEFAULTS_BUSYBOX_UPDATE=y" .config > /dev/null; then \
		$(MAKE) -C user/busybox menuconfig; \
	 fi
	@config/setconfig final

.PHONY: oldconfig
oldconfig:
	# Leave this command on echo, as it helps catch BE to LE (and vice versa)
	# build without a distclean.
	$(MAKE) oldconfig_linux
	@$(MAKE) oldconfig_modules
	@$(MAKE) oldconfig_config
	@sh config/setconfig final
	
.PHONY: defaultconfig
defaultconfig: config.in
	@sh config/setconfig defaults
	# Leave this command on echo, as it helps catch BE to LE (and vice versa)
	# build without a distclean.
	$(MAKE) oldconfig_linux
	@$(MAKE) oldconfig_modules
	@$(MAKE) oldconfig_config
	@sh config/setconfig final
	
.PHONY: oprofile
oprofile: config.in
	# PR 27755: on kernel 2612-4.0, oprofile can not be built
	# @sh config/setconfig oprofile defaults
	# Leave this command on echo, as it helps catch BE to LE (and vice versa)
	# build without a distclean.
	$(MAKE) oldconfig_linux
	@$(MAKE) oldconfig_modules
	@$(MAKE) oldconfig_config
	@sh config/setconfig oprofile final



.PHONY: modules
modules:
	. $(LINUXDIR)/.config; if [ "$$CONFIG_MODULES" = "y" ]; then \
		[ -d $(LINUXDIR)/modules ] || mkdir $(LINUXDIR)/modules; \
		$(MAKEARCH) -C $(LINUXDIR) modules; \
	fi

.PHONY: modules_install
modules_install:
	. $(LINUXDIR)/.config; if [ "$$CONFIG_MODULES" = "y" ]; then \
		[ -d $(ROMFSDIR)/lib/modules ] || mkdir -p $(ROMFSDIR)/lib/modules; \
		$(MAKEARCH) -C $(LINUXDIR) INSTALL_MOD_PATH=$(ROMFSDIR) DEPMOD=$(ROOTDIR)/tools/$(CROSS)depmod modules_install || exit 1; \
		rm -f $(ROMFSDIR)/lib/modules/*/build; \
		find $(ROMFSDIR)/lib/modules -type f | grep -v "modules\." | xargs -r $(STRIP) -g; \
		if [ "$$CONFIG_HOTPLUG" != "y" ]; then \
		rm -f $(ROMFSDIR)/lib/modules/$(LINUX_VERSION)/*map;		\
		rm -f $(ROMFSDIR)/lib/modules/$(LINUX_VERSION)/modules.alias;	\
		rm -f $(ROMFSDIR)/lib/modules/$(LINUX_VERSION)/modules.symbols;	\
		fi	\
	fi

linux_xconfig:
	$(MAKEARCH) -C $(LINUXDIR) xconfig
linux_menuconfig:
	$(MAKEARCH) -C $(LINUXDIR) menuconfig
linux_config:
	$(MAKEARCH) -C $(LINUXDIR) config
modules_xconfig:
	[ ! -d modules ] || $(MAKEARCH) -C modules xconfig
modules_menuconfig:
	[ ! -d modules ] || $(MAKEARCH) -C modules menuconfig
modules_config:
	[ ! -d modules ] || $(MAKEARCH) -C modules config
modules_clean:
	-[ ! -d modules ] || $(MAKEARCH) -C modules clean
config_xconfig:
	$(MAKEARCH) -C config xconfig
config_menuconfig:
	$(MAKEARCH) -C config menuconfig
config_config:
	$(MAKEARCH) -C config config
oldconfig_config:
	$(MAKEARCH) -C config oldconfig
oldconfig_modules:
	[ ! -d modules ] || $(MAKEARCH) -C modules silentoldconfig
oldconfig_linux:
	$(MAKEARCH) -C $(LINUXDIR) silentoldconfig


############################################################################
#
# normal make targets
#

.PHONY: romfs
romfs:
	for dir in $(DIRS) ; do $(MAKEARCH) -C $$dir romfs || exit 1 ; done
	-find $(ROMFSDIR)/. -name CVS | xargs -r rm -rf
# PR12154 Remove *.h and *.a files from root filesystem
	-find $(ROMFSDIR)/. -name "*.[ha]" | xargs -r rm -rf

.PHONY: image
image:
	[ -d $(IMAGEDIR) ] || mkdir $(IMAGEDIR)
	$(MAKEARCH) -C $(VENDDIR) image

.PHONY: zImage
zImage:
	$(MAKEARCH) -C $(LINUXDIR) $(LINUXTARGET) || exit 1
	if [ -f $(LINUXDIR)/vmlinux ]; then \
		ln -f $(LINUXDIR)/vmlinux $(LINUXDIR)/linux ; \
	fi
	$(MAKE) image

#
# fancy target that allows a vendor to have other top level
# make targets,  for example "make vendor_flash" will run the
# vendor_flash target in the vendors directory
#

vendor_%:
	$(MAKEARCH) -C $(VENDDIR) $@

.PHONY: linux
linux linux%_only:
	# THT 2.6 build no longer requires a 'make dep'
	@if [ $(LINUXDIR) == linux-2.4.x -a ! -f $(LINUXDIR)/.depend ] ; then \
		echo "ERROR: you need to do a 'make dep' first" ; \
		exit 1 ; \
	fi
	. $(LINUXDIR)/.config; if [ "$$CONFIG_BLK_DEV_INITRD" = "y" ]; then \
		$(MAKE) -C $(VENDDIR) romfs initrd; \
	else \
		exit 1 ; \
	fi
	echo "Build initrd image"
	$(MAKEARCH) -C $(LINUXDIR) BRCM_VERSION=$(BRCM_VERSION) $(LINUXTARGET) || exit 1
	if [ -f $(LINUXDIR)/vmlinux ]; then \
		ln -f $(LINUXDIR)/vmlinux $(LINUXDIR)/linux ; \
	fi

.PHONY: subdirs
subdirs:
	for dir in $(DIRS) ; do $(MAKEARCH) -C $$dir || exit 1 ; done

dep:		# Now no longer needed in 2.6.x
#	@if [ ! -f $(LINUXDIR)/.config ] ; then \
#		echo "ERROR: you need to do a 'make config' first" ; \
#		exit 1 ; \
#	fi
#	$(MAKEARCH) -C $(LINUXDIR) dep

# This one removes all executables from the tree and forces their relinking
.PHONY: relink
relink:
	find user -name '*.gdb' | sed 's/^\(.*\)\.gdb/\1 \1.gdb/' | xargs rm -f
	find $(VENDDIR) -name '*.gdb' | sed 's/^\(.*\)\.gdb/\1 \1.gdb/' | xargs rm -f

clean: modules_clean
	# These need to be rebuilt every time, and should be first because some .config file may be missing
	-make -C $(VENDDIR)/../common/kinit clean
	-rm -f $(LINUXDIR)/usr/initramfs.dev
	-make -C $(LINUXDIR)/usr clean
	for dir in $(LINUXDIR) $(VENDDIR) $(DIRS); do $(MAKEARCH) -C $$dir clean ; done
	-rm -rf $(ROMFSDIR)/*
	-rm -f config.tk
	-rm -f $(LINUXDIR)/linux
	-rm -rf $(LINUXDIR)/net/ipsec/alg/libaes $(LINUXDIR)/net/ipsec/alg/perlasm
	
# Requires root access
root_clean:
	-rm -rf $(IMAGEDIR)/*

real_clean distclean mrproper: clean root_clean
	-$(MAKEARCH) -C $(LINUXDIR) mrproper
	-$(MAKEARCH) -C config clean
	-$(MAKEARCH) -C uClibc distclean
	-$(MAKEARCH) -C user/busybox distclean
	rm -rf romfs config.in config.arch config.tk images
	rm -f modules/config.tk

%_only:
	[ ! -d "$(@:_only=)" ] || $(MAKEARCH) -C $(@:_only=)

%_clean:
	[ ! -d "$(@:_clean=)" ] || $(MAKEARCH) -C $(@:_clean=) clean

config_error:
	@echo "*************************************************"
	@echo "You have not run make config."
	@echo "The build sequence for this source tree is:"
	@echo "1. 'make config' or 'make xconfig'"
	@echo "2. 'make dep'"
	@echo "3. 'make'"
	@echo "*************************************************"
	@exit 1

prune:
	$(MAKE) -C user prune

dist-prep:
	-find $(ROOTDIR) -name 'Makefile*.bin' | while read t; do \
		$(MAKEARCH) -C `dirname $$t` -f `basename $$t` $@; \
	 done

############################################################################
