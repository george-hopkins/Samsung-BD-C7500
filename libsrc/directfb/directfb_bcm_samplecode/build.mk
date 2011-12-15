###############################################################################
# @par Copyright Information:
#      Copyright (C) 2005, Broadcom.
#      All Rights Reserved.
#      Confidential Property of Broadcom.
#
# @file		Makefile
#
# @brief	Top makefile for the directfb_bcm_samplecode project.
#
# $Id: build.mk,v 1.1 2006/02/08 17:37:56 xmiville Exp $
#
# @par Revision History:
# 
# $Log: build.mk,v $
# Revision 1.1  2006/02/08 17:37:56  xmiville
# Commit of DirectFB changes to 060206-1447 release.
#
# Revision 1.1  2006/01/13 21:41:20  xmiville
# Moved the esi examples to the new sample code project.
#
#
#
###############################################################################

ifndef DIRECTFB_PROJECT_TOP
$(error DIRECTFB_PROJECT_TOP is not defined)
else
include ${DIRECTFB_PROJECT_TOP}/toolchain.inc
endif

ifndef DEBUG
$(error DEBUG is not defined)
endif

ifndef PLATFORM
$(error PLATFORM is not defined)
endif

ifndef CHIP_REVISION
$(error CHIP_REVISION is not defined)
endif

ifndef BCM_REFSW_PATH
$(error BCM_REFSW_PATH is not defined)
else
BSEAV := $(BCM_REFSW_PATH)/BSEAV
PLATFORM := $(PLATFORM)
BCHP_REV := $(CHIP_REVISION)
# This one performs it's own error checking, BSEAV and PLATFORM must be defined
include $(BSEAV)/api/include/api.mak
endif

ifndef DIRECTFB_INSTALL_PATH
$(error DIRECTFB_INSTALL_PATH is not defined)
endif

ifndef BCM_LINUX_PATH
$(error BCM_LINUX_PATH is not defined)
endif

BDVD_CFLAGS := -I$(BSEAV)/../ndvd/a/inc
BDVD_CPPFLAGS := $(BDVD_CFLAGS)
BDVD_CXXFLAGS := $(BDVD_CFLAGS)
BDVD_LDFLAGS := -L$(BSEAV)/../ndvd/a/bin -lbdvd

CROSS_DEFINITIONS := \
	DEBUG="$(DEBUG)" \
	CROSS="$(CROSS)" \
	CROSS_CFLAGS="$(CROSS_CFLAGS) -I${BCM_LINUX_PATH}/include $(BSETTOP_CFLAGS) $(B_REFSW_MAGNUM_INCLUDE_DIRS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include -I${BCM_LINUX_PATH}/include $(BDVD_CFLAGS)" \
	CROSS_CPPFLAGS="$(CROSS_CPPFLAGS) -I${BCM_LINUX_PATH}/include $(BSETTOP_CFLAGS) $(B_REFSW_MAGNUM_INCLUDE_DIRS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include -I${BCM_LINUX_PATH}/include $(BDVD_CPPFLAGS)" \
	CROSS_CXXFLAGS="$(CROSS_CXXFLAGS) -I${BCM_LINUX_PATH}/include $(BSETTOP_CFLAGS) $(B_REFSW_MAGNUM_INCLUDE_DIRS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include -I${BCM_LINUX_PATH}/include $(BDVD_CXXFLAGS)" \
	CROSS_LDFLAGS="$(CROSS_LDFLAGS) $(BSETTOP_LDFLAGS) -L${DIRECTFB_INSTALL_PATH}/usr/local/lib -Wl,-rpath-link,${DIRECTFB_INSTALL_PATH}/usr/local/lib $(BDVD_LDFLAGS)"
	
configure_mipsel-uclibc:
	$(CROSS_DEFINITIONS) \
	sh configure_directfb_bcm_samplecode_mipsel-uclibc

build_mipsel-uclibc: configure_mipsel-uclibc
#1- For some weird reason, won't take LD passed at configure time,
#so let's force it at compile time.
#2- On RH7.3, SED need to be defined for libtool
	SED=sed make LD=$(CROSS)ld

install_mipsel-uclibc: build_mipsel-uclibc
#See comments for build_mipsel-uclibc target
	SED=sed make LD=$(CROSS)ld DESTDIR=${DIRECTFB_INSTALL_PATH} install

clean:
	-make clean

distclean: clean
	-make distclean
	rm -Rf autom4te-*.cache

clean_mipsel-uclibc: clean

distclean_mipsel-uclibc: distclean

