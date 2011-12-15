
ifndef DIRECTFB_PROJECT_TOP
 export DIRECTFB_PROJECT_TOP=$(shell pwd)
 #$(error DIRECTFB_PROJECT_TOP is not defined)
endif


ifndef CROSS_TOOLCHAIN_PATH
  TOOLCHAIN=$(shell mipsel-uclibc-gcc -print-libgcc-file-name|cut -d / -f 1-4)
 ifdef TOOLCHAIN
   export CROSS_TOOLCHAIN_PATH=$(TOOLCHAIN)
 else
  $(error CROSS_TOOLCHAIN_PATH ${TOOLCHAIN}is not defined)
 endif
endif

ifndef BCM_REFSW_PATH
#$(error BCM_REFSW_PATH is not defined)
 export BCM_REFSW_PATH=$(DIRECTFB_PROJECT_TOP)/../../
endif

ifndef BCM_LINUX_PATH
 ifdef LINUX
  export BCM_LINUX_PATH=$(LINUX)
 else
  $(error BCM_LINUX_PATH is not defined)
 endif
endif

ifndef DIRECTFB_INSTALL_PATH
 export DIRECTFB_INSTALL_PATH=$(BCM_REFSW_PATH)/ndvd/directfb_install
#$(error DIRECTFB_INSTALL_PATH is not defined)
endif

ifndef DIRECTFB_BUILD_PATH
 export DIRECTFB_BUILD_PATH=$(BCM_REFSW_PATH)/ndvd/directfb_build
#$(error DIRECTFB_BUILD_PATH is not defined)
endif

DIRECTFB_SOURCE ?= $(DIRECTFB_PROJECT_TOP)/DirectFB-0.9.22_mipsel-uclibc
export DIRECTFB_SOURCE

DIRECTFB_EXAMPLES_SOURCE ?= $(DIRECTFB_PROJECT_TOP)/DirectFB-examples-0.9.22_mipsel-uclibc
DIRECTFB_BCM_SAMPLECODE_SOURCE ?= $(DIRECTFB_PROJECT_TOP)/directfb_bcm_samplecode
DIRECTFB_DEPS_SOURCE ?= $(DIRECTFB_PROJECT_TOP)/directfb_deps_source

ifndef PLATFORM
$(error PLATFORM is not defined)
endif

ifndef BCHP_VER
$(error BCHP_VER is not defined)
endif

ifndef BCHP_7411_VER
$(error BCHP_7411_VER is not defined)
endif

PLATFORM := $(PLATFORM)
BCHP_VER := $(BCHP_VER)
BCHP_7411_VER := $(BCHP_7411_VER)

DEBUG ?= n

BUILD_LINUX_INPUT ?= n

# Toolchain must be in path, before anything else (maybe there is another
# toolchain with the mipsel prefix
# Also if autoconf setup script was run will put this in the path
PATH := $(CROSS_TOOLCHAIN_PATH)/bin:$(DIRECTFB_PROJECT_TOP)/autoconf_links:${PATH}


DFB_BUILD_EXAMPLES ?= n

ifeq (y,${DFB_BUILD_EXAMPLES})
  DFB_OPTIONAL_TARGETS += build_directfb-examples
endif

DFB_BUILD_BCM_SAMPLECODE ?= n

ifeq (y,${DFB_BUILD_BCM_SAMPLECODE})
  DFB_OPTIONAL_TARGETS += build_directfb_bcm_samplecode
endif

all: build_refsw build_directfb_deps build_directfb ${DFB_OPTIONAL_TARGETS}  # build_directfb-examples

debug:
	echo ${DFB_OPTIONAL_TARGETS}

config_kernel:
	@if [ -f $(BCM_LINUX_PATH)/.config ]; then \
		cp $(BCM_LINUX_PATH)/.config $(BCM_LINUX_PATH)/backup_defconfig; \
	fi
	make -C $(BCM_LINUX_PATH) clean
	make -C $(BCM_LINUX_PATH) distclean
	@if [ $(BUILD_LINUX_INPUT) = "y" ]; then \
		-cd $(BCM_LINUX_PATH); patch -Np1 < $(DIRECTFB_PROJECT_TOP)/stblinux-2425-2.7_usbhiddummykeybd.patch; \
		exit 1; \
	fi
	@if [ -f $(BCM_LINUX_PATH)/backup_defconfig ]; then \
	    mv $(BCM_LINUX_PATH)/backup_defconfig $(BCM_LINUX_PATH)/.config; \
		make -C $(BCM_LINUX_PATH) oldconfig; \
		make -C $(BCM_LINUX_PATH) dep; \
	elif [ -f $(BCM_LINUX_PATH)/arch/mips/defconfig-brcm-bb-$(PLATFORM) ]; then \
		cp $(BCM_LINUX_PATH)/arch/mips/defconfig-brcm-bb-$(PLATFORM) $(BCM_LINUX_PATH)/.config; \
		make -C $(BCM_LINUX_PATH) oldconfig; \
		make -C $(BCM_LINUX_PATH) dep; \
	else \
		echo "Neither $(BCM_LINUX_PATH)/.config nor $(BCM_LINUX_PATH)/arch/mips/defconfig-brcm-bb-$(PLATFORM) exists"; \
		exit 1; \
	fi

build_kernel: $(DIRECTFB_INSTALL_PATH)/usr/bcm config_kernel
	make -C $(BCM_LINUX_PATH) vmlinux
	gzip -3fc $(BCM_LINUX_PATH)/vmlinux > $(DIRECTFB_INSTALL_PATH)/usr/bcm/zImage-97398

build_refsw: $(DIRECTFB_INSTALL_PATH)/usr/bcm
	PATH=$(PATH) make INSTALL_DIR=$(DIRECTFB_INSTALL_PATH)/usr/bcm STATICLIB=no PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) LINUX=$(BCM_LINUX_PATH) DEBUG=$(DEBUG) -C $(BCM_REFSW_PATH)/BSEAV/api/build install

build_directfb_deps: $(DIRECTFB_INSTALL_PATH)
	@if [ ! -d $(DIRECTFB_BUILD_PATH) ]; then \
		mkdir -p $(DIRECTFB_BUILD_PATH); \
		PATH=$(PATH) make BUILDDIR=$(DIRECTFB_BUILD_PATH) DESTDIR=$(DIRECTFB_INSTALL_PATH) SOURCEDIR=$(DIRECTFB_DEPS_SOURCE) -f $(DIRECTFB_DEPS_SOURCE)/build.mk all; \
	else \
		echo $(DIRECTFB_BUILD_PATH) exist, skipping; \
	fi

build_directfb: $(DIRECTFB_INSTALL_PATH)
#build_directfb_deps and build_refsw are deps, but deps does not do inc build...
	PATH=$(PATH) make PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) DEBUG=$(DEBUG) -C $(DIRECTFB_SOURCE) -f $(DIRECTFB_SOURCE)/build.mk install_mipsel-uclibc

build_directfb-examples: $(DIRECTFB_INSTALL_PATH)
#build_directfb is deps, but deps does not do inc build...
	PATH=$(PATH) make PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) DEBUG=$(DEBUG) -C $(DIRECTFB_EXAMPLES_SOURCE) -f $(DIRECTFB_EXAMPLES_SOURCE)/build.mk install_mipsel-uclibc

build_directfb_bcm_samplecode: $(DIRECTFB_INSTALL_PATH)
	PATH=$(PATH) make -C $(DIRECTFB_DEPS_SOURCE)/FontFusion_3.1/FontFusion_3.1_SDK/demo/libt2k DEBUG=n DESTDIR=$(DIRECTFB_INSTALL_PATH) install
	PATH=$(PATH) make PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) DEBUG=$(DEBUG) -C $(DIRECTFB_BCM_SAMPLECODE_SOURCE) -f $(DIRECTFB_BCM_SAMPLECODE_SOURCE)/build.mk install_mipsel-uclibc

$(DIRECTFB_INSTALL_PATH)/usr/bcm:
	mkdir -p $(DIRECTFB_INSTALL_PATH)/usr/bcm

$(DIRECTFB_INSTALL_PATH):
	mkdir -p $(DIRECTFB_INSTALL_PATH)

distclean:
	make -C $(DIRECTFB_SOURCE) -f $(DIRECTFB_SOURCE)/build.mk PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) DEBUG=$(DEBUG) distclean
	make -C $(DIRECTFB_EXAMPLES_SOURCE) -f $(DIRECTFB_EXAMPLES_SOURCE)/build.mk PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) DEBUG=$(DEBUG) distclean
	make -C $(DIRECTFB_BCM_SAMPLECODE_SOURCE) -f $(DIRECTFB_BCM_SAMPLECODE_SOURCE)/build.mk PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) DEBUG=$(DEBUG) distclean
	rm -rf $(DIRECTFB_BUILD_PATH)

cvsclean:
	find . -name ".cvsignore"|while read ignorefile; do (cd `dirname $$ignorefile` && cat .cvsignore|while read pattern; do rm -Rf $$pattern; done); done

refswclean:
	PATH=$(PATH) make INSTALL_DIR=$(DIRECTFB_INSTALL_PATH)/usr/bcm PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) LINUX=$(BCM_LINUX_PATH) -C $(BCM_REFSW_PATH)/BSEAV/linux/driver/build/$(PLATFORM) clean
	PATH=$(PATH) make INSTALL_DIR=$(DIRECTFB_INSTALL_PATH)/usr/bcm STATICLIB=no PLATFORM=$(PLATFORM) BCHP_VER=$(BCHP_VER) BCHP_7411_VER=$(BCHP_7411_VER) LINUX=$(BCM_LINUX_PATH) DEBUG=$(DEBUG) -C $(BCM_REFSW_PATH)/BSEAV/api/build clean

packageclean: distclean refswclean cvsclean
	rm -Rf autoconf_links
	rm -rf $(DIRECTFB_INSTALL_PATH)

