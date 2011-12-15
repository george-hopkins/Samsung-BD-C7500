#
# Broadcom Settop Box build script for Little Endian
#
# Root access is no longer required to build anything.
#
# To build all kernels (initrd and regular kernels)
# % make -f build.mk [platform]
#
# To build just the initrd kernel for one platform
# % make -f build.mk vmlinuz-initrd-7038b0
#
# To build just the regular kernel for one platform
# % make -f build.mk vmlinuz-7038b0
#
# To build just the rootfs images for 1 platform, after building the kernels for the 7038b0 platform
# % make -f build.mk rootfs-7038b0
#
# To build the kernel and images for all platforms:
# % make -f build.mk all
#
WHOAMI ?= $(shell who am i | cut -d" " -f1 | cut -d'!' -f2)
MYGID = $(shell id -g)
MYUID := $(shell id -u)

# if nothing works, set it to login name
ifeq ($(strip $(WHOAMI)),)
    WHOAMI=$(LOGNAME)
endif

# variables to preserve
SAVEVARS = PATH SHELL HOME USER MFLAGS MAKEFLAGS MAKELEVEL MAKEOVERRIDES

# scrub the environment (PR33041)
unexport $(shell test -f config/cleanenv.pl && \
	perl -w config/cleanenv.pl $(SAVEVARS))

#BCM7XXX= 7400a0 7400a0-smp 7400a0-nand 7038 7038b0 7110 7110-docsis 7112 7115 7315 7317 7319 7320 7328 7329
#VENOM=937xx
# PLATFORMS=$(BCM7XXX) $(VENOM)

# THT For 2.6.18-2.0 we only support a few platforms
# THT: Note there is no 7440b0 NOR flash build. 
PLATFORMS=7440b0 7601 7635 7630 7405a0

#THT Removed 7038 support for 2618-2.0
#KPC Resurrected 7038c0 support for PR36194
XFS_PLATFORMS =
ALL_PLATFORMS = $(PLATFORMS) $(XFS_PLATFORMS)
ROOTLESS_PLATFORMS = 7402b0s
ROOTED_PLATFORMS := $(filter-out $(ROOTLESS_PLATFORMS),$(ALL_PLATFORMS))

BB_PLATFORMS:=$(addprefix vmlinuz-,$(PLATFORMS))
BB_INITRD_PLATFORMS	:= $(addprefix vmlinuz-initrd-,$(PLATFORMS))

XFS_BB_PLATFORMS:=$(addprefix vmlinuz-,$(XFS_PLATFORMS))
XFS_INITRD_PLATFORMS := $(addprefix vmlinuz-initrd-,$(XFS_PLATFORMS))

KGDB_PLATFORMS := $(addprefix vmlinuz-,$(addsuffix -kgdb,$(ALL_PLATFORMS)))
# Pass this flag with a value of "y" to enable kgdb debugging
# over Single Serial Port
# make -f build.mk SINGLE_SERIAL_PORT=y vmlinuz-3560-kgdb
SINGLE_SERIAL_PORT=n

OPROF_PLATFORMS := $(addprefix vmlinuz-,$(addsuffix -opf,$(ALL_PLATFORMS)))
OPROF_INITRD_PLATFORMS := $(addprefix vmlinuz-initrd-,$(addsuffix -opf,$(ALL_PLATFORMS)))
OPROFILE_PLATFORMS := $(addsuffix -opf,$(ALL_PLATFORMS))

NO_ROOTFS_PLATFORMS := $(addprefix rootfs-,$(ROOTLESS_PLATFORMS))
ROOTFS_PLATFORMS := $(addprefix rootfs-,$(ROOTED_PLATFORMS))

VERSION := $(shell sed -e 's/-uclibc-brcm//' version)
ifneq (,${DEBUG})
$(warning DEBUG: VERSION=${VERSION})
endif

TFTPBOOT=/tftpboot
TFTPDIR=$(TFTPBOOT)/$(VERSION)
KERNEL_DIR=linux-2.6.x
CROSS_COMPILE=mipsel-linux-
CC=$(CROSS_COMPILE)gcc
MAKEARCH=make ARCH=mips CROSS_COMPILE=$(CROSS_COMPILE) CC=$(CC)

######################################################################
# Here is a description of the Perl command below used 
# to form VERSION.  Note that Makefiles require you to 
# specify '$$' when you want an escaped '$'.
# 
# Look at the first 10 lines only
# >> if (10>$$.)
#
# Append field if it matches one of these vars.
# >> $$x.=" $$F[2]" if /^(VERSION|PATCHLEVEL|SUBLEVEL|EXTRAVERSION)/; 
#
# Print text when we are finished.
# >> END {print $$x} 
#################################
TMP := $(shell perl -nae 'if (10>$$.) { $$x.=" $$F[2]" if /^(VERSION|PATCHLEVEL|SUBLEVEL|EXTRAVERSION)/; } END {print $$x}'  ./$(KERNEL_DIR)/Makefile) XXX YYY ZZZ
KERNEL_VERSION      := $(word 1, ${TMP})
KERNEL_PATCHLEVEL   := $(word 2, ${TMP})
KERNEL_SUBLEVEL     := $(word 3, ${TMP})
KERNEL_EXTRAVERSION := $(word 4, ${TMP})
ANDOVER_VERSION     := ${KERNEL_VERSION}.${KERNEL_PATCHLEVEL}.${KERNEL_SUBLEVEL}${KERNEL_EXTRAVERSION}${VERSION}
VERSION             := ${KERNEL_VERSION}${KERNEL_PATCHLEVEL}${KERNEL_SUBLEVEL}${KERNEL_EXTRAVERSION}${VERSION}
BRCM_VERSION=$(VERSION)
ifneq (,${DEBUG})
$(warning DEBUG: VERSION=${VERSION})
$(warning DEBUG: ANDODOVER_VERSION=${ANDODOVER_VERSION})
endif


LINUXDIR_VER=
ROOTFS_VER=
ifeq ($@,) 
	LINUXDIR_VER+=$(shell grep -as "^EXTRAVERSION" ${KERNEL_DIR}/Makefile | sed "s/[A-Za-z\= \-]//g")
	ROOTFS_VER+=$(shell cat version | sed "s/.*\-//")
endif


INITRD_DEFCONFIG=
KERNEL_DEFCONFIG=
ifneq ($(test),) 
	INITRD_DEFCONFIG:=$(test)
	KERNEL_DEFCONFIG:=$(test)
else
	INITRD_DEFCONFIG:=$$CONFIG_LINUXDIR/.config.orig
endif

kernels: vercheck	
	for i in $(ALL_PLATFORMS); do \
		make -f build.mk $$i; \
	done

all: vercheck kernels install

.PHONY: vercheck install rootfs platlist prepare
dump_version:
	@echo ${VERSION}
dump_andover_version:
	@echo ${ANDOVER_VERSION}


prepare:
	test -f ../prepare.sh && (../prepare.sh || (echo "FATAL Error: missing files" ; exit 1)) || echo "succeed"
	# Get the sanatized version of the kernel header files
	$(MAKEARCH) -C $(KERNEL_DIR) headers_install INSTALL_HDR_PATH=$(shell pwd)/kernel_headers

#jipeng - check version mismatch between rootfs and kernel source
vercheck:
	@test -f ${KERNEL_DIR}/Makefile || (echo "FATAL Error: miss ${KERNEL_DIR}/Makefile"; exit 1)
	@if [ "$(LINUXDIR_VER)" == "$(ROOTFS_VER)" ]; then	\
		echo "build binary images for version $(VERSION)";	\
	else	\
		echo "Error: Kernel version $(LINUXDIR_VER) mismatch with rootfs version $(ROOTFS_VER)"; \
		echo "Check ${KERNEL_DIR}/Makefile and ./version"; \
		exit 1;	\
	fi

install: rootfs

platlist:
	> platlist ; \
	for i in $(ALL_PLATFORMS); do \
		echo $$i >> platlist ; \
	done

rootfs:
	# Make the rootfs images and copy to tftp server
	for i in $(ROOTED_PLATFORMS); do \
		if [ -e images/$$i/initramfs_data_nodev.cpio ]; then \
			$(MAKE) -f build.mk rootfs-$$i || exit 1; \
			case "$$i" in \
			*-nand) \
				cp images/$$i/jffs2-128k.img $(TFTPDIR)/jffs2-128k-$$i.img; \
				cp images/$$i/jffs2-16k.img $(TFTPDIR)/jffs2-16k-$$i.img; \
				cp images/$$i/jffs2-512k.img $(TFTPDIR)/jffs2-512k-$$i.img; \
				cp images/$$i/yaffs-frm-cramfs.img $(TFTPDIR)/yaffs-frm-cramfs-$$i.img; \
				cp images/$$i/cramfs.img $(TFTPDIR)/cramfs-$$i.img; \
				cp images/$$i/squashfs.img $(TFTPDIR)/squashfs-$$i.img; \
				cp images/$$i/modules.tar.bz2 $(TFTPDIR)/modules-$$i.tar.bz2; \
				;;\
			*) \
				cp images/$$i/jffs2.img $(TFTPDIR)/jffs2-$$i.img; \
				if [ "$$i" == "7401c0" ]; then \
					cp images/$$i/jffs2-64k.img $(TFTPDIR)/jffs2-64k-$$i.img; \
				fi; \
				cp images/$$i/cramfs.img $(TFTPDIR)/cramfs-$$i.img; \
				cp images/$$i/squashfs.img $(TFTPDIR)/squashfs-$$i.img; \
				cp images/$$i/modules.tar.bz2 $(TFTPDIR)/modules-$$i.tar.bz2; \
				;;\
			esac; \
		fi; \
	done

.PHONY: $(ALL_PLATFORMS) $(BB_INITRD_PLATFORMS) $(BB_PLATFORMS) \
	$(ROOTFS_PLATFORMS) $(ROOTFS_IMAGES) $(OPROFILE_PLATFORMS) \
	$(NO_ROOTFS_PLATFORMS)


$(ALL_PLATFORMS) :
	echo "Making uclinux-rootfs and kernels for $@ version=$(VERSION)"
	#make -f build.mk distclean
	make -f build.mk vmlinuz-initrd-$@
	make -f build.mk vmlinuz-$@
	
$(OPROFILE_PLATFORMS) :
	echo "Making OPROFILE uclinux-rootfs and kernels for $@ version=$(VERSION)"
	#make -f build.mk distclean
	make -f build.mk vmlinuz-initrd-$@
	make -f build.mk vmlinuz-$@
	
$(ROOTFS_PLATFORMS):
	echo "Making rootfs images for $@ version=$(VERSION)"; \
	# We don't check whether the image is up-todate, but assume that it has been built.
	cp -f defconfigs/defconfig-brcm-uclinux-rootfs-$(subst rootfs-,,$@) .config
	. .config && make -C vendors/"$$CONFIG_VENDOR"/"$$CONFIG_PRODUCT" \
		ROOTDIR=$(shell pwd) \
		LINUX_CONFIG=$(shell pwd)/vendors/"$$CONFIG_VENDOR/$$CONFIG_PRODUCT"/config.linux-2.6.x \
		CONFIG_CONFIG=$(shell pwd)/vendors/"$$CONFIG_VENDOR/$$CONFIG_PRODUCT"/config.vendor-2.6.x \
		LINUXDIR=$$CONFIG_LINUXDIR\
		IMAGEDIR=$(shell pwd)/images\
		PLATFORM=$(subst rootfs-,,$@)\
		ARCH_CONFIG=$(shell pwd)/vendors/"$$CONFIG_VENDOR"/"$$CONFIG_PRODUCT"/config.arch\
		images
	tar -C images/$(subst rootfs-,,$@)/initramfs -jcf images/$(subst rootfs-,,$@)/modules.tar.bz2 lib/modules
	case "$(subst rootfs-,,$@)" in \
		*-nand) \
			cp -f images/$(subst rootfs-,,$@)/jffs2-128k.img $(TFTPDIR)/jffs2-128k-$(subst rootfs-,,$@).img; \
			cp -f images/$(subst rootfs-,,$@)/jffs2-16k.img $(TFTPDIR)/jffs2-16k-$(subst rootfs-,,$@).img; \
			cp -f images/$(subst rootfs-,,$@)/jffs2-512k.img $(TFTPDIR)/jffs2-512k-$(subst rootfs-,,$@).img; \
			cp -f images/$(subst rootfs-,,$@)/yaffs-frm-cramfs.img $(TFTPDIR)/yaffs-frm-cramfs-$(subst rootfs-,,$@).img ; \
			cp -f images/$(subst rootfs-,,$@)/cramfs.img $(TFTPDIR)/cramfs-$(subst rootfs-,,$@).img; \
			cp -f images/$(subst rootfs-,,$@)/squashfs.img $(TFTPDIR)/squashfs-$(subst rootfs-,,$@).img ; \
			cp -f images/$(subst rootfs-,,$@)/modules.tar.bz2 $(TFTPDIR)/modules-$(subst rootfs-,,$@).tar.bz2 ; \
			;;\
		*) \
			cp -f images/$(subst rootfs-,,$@)/jffs2.img $(TFTPDIR)/jffs2-$(subst rootfs-,,$@).img; \
			if [ "$(subst rootfs-,,$@)" == "7401c0" ]; then \
				cp images/$(subst rootfs-,,$@)/jffs2-64k.img $(TFTPDIR)/jffs2-64k-$(subst rootfs-,,$@).img; \
			fi; \
			cp -f images/$(subst rootfs-,,$@)/cramfs.img $(TFTPDIR)/cramfs-$(subst rootfs-,,$@).img; \
			cp -f images/$(subst rootfs-,,$@)/squashfs.img $(TFTPDIR)/squashfs-$(subst rootfs-,,$@).img ; \
			cp -f images/$(subst rootfs-,,$@)/modules.tar.bz2 $(TFTPDIR)/modules-$(subst rootfs-,,$@).tar.bz2 ; \
			;;\
	esac

$(NO_ROOTFS_PLATFORMS):
	echo "No rootfs available for $@, skipping."

# The echo Done at the end is to prevent make from rep orting errors.
$(BB_INITRD_PLATFORMS) $(XFS_INITRD_PLATFORMS): prepare
	cp -f defconfigs/defconfig-brcm-uclinux-rootfs-$(subst vmlinuz-initrd-,,$@) .config
	rm -f $(KERNEL_DIR)/.config
	(. .config; \
		cp -f vendors/$${CONFIG_VENDOR}/$${CONFIG_PRODUCT}/config.linux-2.6.x $${CONFIG_LINUXDIR}/.config; \
	)
	# sets up .config files
	$(MAKEARCH) defaultconfig
	# Set CONFIG_INITRAMFS_ROOT_UID & GID accordingly
	(. .config; cp -f "$${CONFIG_LINUXDIR}"/.config "$${CONFIG_LINUXDIR}"/.config.orig; \
		rm -f "$${CONFIG_LINUXDIR}"/.config; \
		sed -e "s/^CONFIG_INITRAMFS_ROOT_UID=0/CONFIG_INITRAMFS_ROOT_UID=$(MYUID)/" \
		    -e "s/^CONFIG_INITRAMFS_ROOT_GID=0/CONFIG_INITRAMFS_ROOT_GID=$(MYGID)/" \
		    -e "s/^CONFIG_BRCM_BUILD_TARGET.*/CONFIG_BRCM_BUILD_TARGET=\"$(subst vmlinuz-initrd-,,$@)\"/" \
			${INITRD_DEFCONFIG} > $${CONFIG_LINUXDIR}/.config; \
		cp -f vendors/$${CONFIG_VENDOR}/$${CONFIG_PRODUCT}/config.uClibc uClibc/.config; \
		cp -f vendors/$${CONFIG_VENDOR}/$${CONFIG_PRODUCT}/config.busybox user/busybox/.config; \
		cp -f vendors/$${CONFIG_VENDOR}/$${CONFIG_PRODUCT}/config.vendor-2.6.x config/.config; \
		cd $${CONFIG_LINUXDIR} && $(MAKEARCH) silentoldconfig; \
	)
# PR25899 - show targets - following make command should specific target all
	$(MAKEARCH) BRCM_VERSION=$(VERSION) all
	test -d $(TFTPDIR) || mkdir -p $(TFTPDIR)
	if [ "$(subst vmlinuz-initrd-,,$@)" == "937xx" -o "$(subst vmlinuz-initrd-,,$@)" == "937xx-xfs" ]; then \
		cp -f images/937xx/vmlinux $(TFTPDIR)/$(subst vmlinuz-,vmlinux-,$@) ; \
	else \
		cp -f images/$(subst vmlinuz-initrd-,,$@)/vmlinuz $(TFTPDIR)/vmlinuz-initrd-$(VERSION)-$(subst vmlinuz-initrd-,,$@) ;\
	fi
	echo "Done building $@"

$(BB_PLATFORMS) $(XFS_BB_PLATFORMS): prepare install
	if [ "x$(KERNEL_DEFCONFIG)" != "x" ]; then	\
		test -f $(KERNEL_DEFCONFIG) && rm -f $(KERNEL_DIR)/.config; \
		sed -e "s/^CONFIG_BRCM_BUILD_TARGET.*/CONFIG_BRCM_BUILD_TARGET=\"$(subst vmlinuz-,,$@)\"/" \
			$(KERNEL_DEFCONFIG) > $(KERNEL_DIR)/.config \
		|| exit 1;	\
	else	\
		if test -f $(KERNEL_DIR)/arch/mips/configs/bcm9$(subst vmlinuz-,,$@)_defconfig; then	\
			rm -f $(KERNEL_DIR)/.config; \
			sed -e "s/^CONFIG_BRCM_BUILD_TARGET.*/CONFIG_BRCM_BUILD_TARGET=\"$(subst vmlinuz-,,$@)\"/" \
				$(KERNEL_DIR)/arch/mips/configs/bcm9$(subst vmlinuz-,,$@)_defconfig > $(KERNEL_DIR)/.config \
			|| exit 1;	\
		else	\
			rm -f $(KERNEL_DIR)/.config; \
			sed -e "s/^CONFIG_BRCM_BUILD_TARGET.*/CONFIG_BRCM_BUILD_TARGET=\"$(subst vmlinuz-,,$@)\"/" \
				$(KERNEL_DIR)/arch/mips/configs/bcm$(subst vmlinuz-,,$@)_defconfig > $(KERNEL_DIR)/.config \
			|| exit 1;	\
		fi;	\
	fi;	\
	$(MAKEARCH) -C $(KERNEL_DIR) silentoldconfig
	$(MAKEARCH) -C $(KERNEL_DIR) BRCM_VERSION=$(VERSION)
	$(MAKEARCH) -C $(KERNEL_DIR) bcmheaders_install 
	test -d $(TFTPDIR) || mkdir -p $(TFTPDIR)
	if [ $(subst vmlinuz-,,$@) != "937xx" -a $(subst vmlinuz-,,$@) != "937xx-xfs" ]; then \
		gzip -3fc $(KERNEL_DIR)/vmlinux > $(TFTPDIR)/vmlinuz-$(VERSION)-$(subst vmlinuz-,,$@) ; \
		cd images/$(subst vmlinuz-,,$@)/initramfs; tar cvfz $(TFTPDIR)/uclinux-rootfs-$(VERSION)-$(subst vmlinuz-,,$@)-le.bin.tgz * ; \
	else \
		cp -f $(KERNEL_DIR)/vmlinux $(TFTPDIR)/$(subst vmlinuz-,vmlinux-,$@) ; \
	fi

#
# KGDB targets: will output kernel images of the form
# vmlinuz-<platform>-kgdb, for example, vmlinuz-7401c0-kgdb
# Only non-initrd images can be targets, for now.
#
# Should do a make clean manually before involking any kgdb targets.
# Omitted here since these are normally one shot deals.
$(KGDB_PLATFORMS): prepare
	-test -f $(KERNEL_DIR)/.config && rm -f $(KERNEL_DIR)/.config
	if test -f $(KERNEL_DIR)/arch/mips/configs/bcm9$(subst vmlinuz-,,$(subst -kgdb,,$@))_defconfig; then	\
		cp -f $(KERNEL_DIR)/arch/mips/configs/bcm9$(subst vmlinuz-,,$(subst -kgdb,,$@))_defconfig .kgdb_target;		\
	else	\
		cp -f $(KERNEL_DIR)/arch/mips/configs/bcm$(subst vmlinuz-,,$(subst -kgdb,,$@))_defconfig .kgdb_target;		\
	fi;	\
	cat .kgdb_target $(KERNEL_DIR)/arch/mips/configs/bcm97xxx-kgdb.tpl | 				\
	sed 's,# CONFIG_DEBUG_KERNEL is not set,CONFIG_DEBUG_KERNEL=y,g' > $(KERNEL_DIR)/.config;	\
	rm -f .kgdb_target;	\
	if [ "$(SINGLE_SERIAL_PORT)" == "y" ]; then \
		echo "CONFIG_SINGLE_SERIAL_PORT=y" >> $(KERNEL_DIR)/.config; \
	else \
		echo "# CONFIG_SINGLE_SERIAL_PORT is not set" >> $(KERNEL_DIR)/.config; \
	fi
	$(MAKEARCH) -C $(KERNEL_DIR) silentoldconfig
	$(MAKEARCH) -C $(KERNEL_DIR) BRCM_VERSION=$(VERSION)
	test -d $(TFTPDIR) || mkdir -p $(TFTPDIR)
	if [ $(subst vmlinuz-,,$@) != "937xx-kgdb" -a $(subst vmlinuz-,,$@) != "937xx-xfs-kgdb" ]; then \
		gzip -3fc $(KERNEL_DIR)/vmlinux > $(TFTPDIR)/$@ ; \
	else \
		cp -f $(KERNEL_DIR)/vmlinux $(TFTPDIR)/$(subst vmlinuz-,vmlinux-,$@); \
	fi

# The echo Done at the end is to prevent make from reporting errors.
$(OPROF_INITRD_PLATFORMS) : prepare
	cp -f defconfigs/defconfig-brcm-uclinux-rootfs-$(subst -opf,,$(subst vmlinuz-initrd-,,$@)) .config
	# Set locale stuffs.  We will need
	. .config; export LANG="C"; export LC_MESSAGES="C"; export LC_CTYPE="C"; export LC_ALL="C"; \
		test -f "$$CONFIG_LINUXDIR"/.config && rm -f "$$CONFIG_LINUXDIR"/.config; \
		sh config/setconfig oprofile defaults; \
		sed -e "s/CONFIG_INITRAMFS_ROOT_UID=0/CONFIG_INITRAMFS_ROOT_UID=$(MYUID)/"\
			vendors/"$$CONFIG_VENDOR"/"$$CONFIG_PRODUCT"/config."$$CONFIG_LINUXDIR"  \
		| sed -e "s/CONFIG_INITRAMFS_ROOT_GID=0/CONFIG_INITRAMFS_ROOT_GID=$(MYGID)/" \
		| sed -e "s/# CONFIG_INSTRUMENTATION is not set/CONFIG_INSTRUMENTATION=y/" \
			> "$$CONFIG_LINUXDIR"/.config; \
		echo "CONFIG_PROFILING=y" >> "$$CONFIG_LINUXDIR"/.config; \
		echo "CONFIG_OPROFILE=y" >> "$$CONFIG_LINUXDIR"/.config; \
		echo "# CONFIG_MARKERS is not set" >> "$$CONFIG_LINUXDIR"/.config; \
		echo "# CONFIG_USER_PROFILE_OPROFILE_FULL is not set" >> "$$CONFIG_LINUXDIR"/.config; \
		cp -f vendors/"$$CONFIG_VENDOR"/"$$CONFIG_PRODUCT"/config.uClibc uClibc/.config; \
		cp -f vendors/"$$CONFIG_VENDOR"/"$$CONFIG_PRODUCT"/config.busybox user/busybox/.config; \
		test -f config/.config && rm -f config/.config; \
		sed -e "s/# CONFIG_USER_PROFILE_OPROFILE is not set/CONFIG_USER_PROFILE_OPROFILE=y/" \
			vendors/"$$CONFIG_VENDOR"/"$$CONFIG_PRODUCT"/config.vendor-2.6.x  \
		| sed -e "s/# CONFIG_USER_PROFILE_POFT is not set/CONFIG_USER_PROFILE_POFT=y/" > config/.config; \
		(cd "$$CONFIG_LINUXDIR" && $(MAKEARCH) silentoldconfig);
	$(MAKEARCH) oprofile
# PR25899 - show targets - following make command should specific target all
	$(MAKEARCH) BRCM_VERSION=$(VERSION) all
	test -d $(TFTPDIR) || mkdir -p $(TFTPDIR)
	if [ "$(subst -opf,,$(subst vmlinuz-initrd-,,$@))" == "937xx" -o \
        "$(subst -opf,,$(subst vmlinuz-initrd-,,$@))" == "937xx-xfs" ]; then \
		cp -f images/937xx/vmlinux $(TFTPDIR)/vmlinux-$(subst -opf,,$(subst vmlinuz-initrd-,,$@)) ; \
	else \
		cp -f images/$(subst -opf,,$(subst vmlinuz-initrd-,,$@))/vmlinuz $(TFTPDIR)/$@ ;\
	fi
	echo "Done building $@"

$(OPROF_PLATFORMS) : prepare 
	-test -f $(KERNEL_DIR)/.config && rm -f $(KERNEL_DIR)/.config
	if test -f $(KERNEL_DIR)/arch/mips/configs/bcm$(subst vmlinuz-,,$(subst -opf,,$@))_defconfig; then	\
		cat $(KERNEL_DIR)/arch/mips/configs/bcm$(subst vmlinuz-,,$(subst -opf,,$@))_defconfig | \
			sed 's/# CONFIG_INSTRUMENTATION is not set/CONFIG_INSTRUMENTATION=y/g' \
				> $(KERNEL_DIR)/.config; \
		echo "CONFIG_PROFILING=y" >> $(KERNEL_DIR)/.config; \
		echo "CONFIG_OPROFILE=y" >> $(KERNEL_DIR)/.config; \
		echo "# CONFIG_MARKERS is not set" >> $(KERNEL_DIR)/.config; \
	else \
		cat $(KERNEL_DIR)/arch/mips/configs/bcm9$(subst vmlinuz-,,$(subst -opf,,$@))_defconfig | \
			sed 's/# CONFIG_INSTRUMENTATION is not set/CONFIG_INSTRUMENTATION=y/g' \
				> $(KERNEL_DIR)/.config; \
		echo "CONFIG_PROFILING=y" >> $(KERNEL_DIR)/.config; \
		echo "CONFIG_OPROFILE=y" >> $(KERNEL_DIR)/.config; \
		echo "# CONFIG_MARKERS is not set" >> $(KERNEL_DIR)/.config; \
	fi
	$(MAKEARCH) -C $(KERNEL_DIR) silentoldconfig
	$(MAKEARCH) -C $(KERNEL_DIR) BRCM_VERSION=$(VERSION)
	test -d $(TFTPDIR) || mkdir -p $(TFTPDIR)
	if [ $(subst -opf,,$(subst vmlinuz-,,$@)) != "937xx" -a \
		$(subst -opf,,$(subst vmlinuz-,,$@)) != "937xx-xfs" ]; then \
		gzip -3fc $(KERNEL_DIR)/vmlinux > $(TFTPDIR)/$@ ; \
	else \
		cp -f $(KERNEL_DIR)/vmlinux $(TFTPDIR)/$(subst vmlinuz,vmlinux,$@); \
	fi
	
clean:
	rm -f $(ALL_PLATFORMS) $(BB_INITRD_PLATFORMS) $(BB_PLATFORMS)
	$(MAKEARCH) $@

distclean:
	rm -f $(ALL_PLATFORMS) $(BB_INITRD_PLATFORMS) $(BB_PLATFORMS)
	$(MAKEARCH) $@


# PR25899 - Show available targets
show_targets:
	@echo "============================================================================"
	@echo "Little Endian Targets are (make -f build.mk {target} where {target} is): "
	@echo "============================================================================"
	@for i in $(ALL_PLATFORMS); do \
		echo "vmlinuz-"$$i; \
 	done
	@for i in $(ALL_PLATFORMS); do \
		echo "vmlinuz-initrd-"$$i; \
 	done
	@for i in $(ALL_PLATFORMS); do \
		echo "vmlinuz-"$$i"-kgdb"; \
 	done
	@for i in $(ALL_PLATFORMS); do \
		echo "vmlinuz-initrd-"$$i"-kgdb"; \
 	done
	@for i in $(ALL_PLATFORMS); do \
		echo "vmlinuz-"$$i"-opf"; \
 	done
	@for i in $(ALL_PLATFORMS); do \
		echo "vmlinuz-initrd-"$$i"-opf"; \
 	done
	@for i in $(ALL_PLATFORMS); do \
		echo "rootfs-"$$i; \
 	done
