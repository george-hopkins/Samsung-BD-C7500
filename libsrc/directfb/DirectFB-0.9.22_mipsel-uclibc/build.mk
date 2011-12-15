###############################################################################
# @par Copyright Information:
#      Copyright (C) 2005, Broadcom.
#      All Rights Reserved.
#      Confidential Property of Broadcom.
#
# @file		Makefile
#
# @brief	Top makefile for the DirectFB project.
#
# $Id: build.mk,v 1.2 2006/02/08 17:37:56 xmiville Exp $
#
# @par Revision History:
# 
# $Log: build.mk,v $
# Revision 1.2  2006/02/08 17:37:56  xmiville
# Commit of DirectFB changes to 060206-1447 release.
#
# Revision 1.2  2006/01/09 18:14:04  xmiville
# Merging work branch from 051128.
#
# Revision 1.2  2005/11/29 16:11:53  steven
# Ported Xav's new DirectFB to ndvd release
#
# Revision 1.3  2005/11/02 14:46:00  xmiville
# Cleanup and multi-layer support.
#
# Revision 1.2  2005/10/27 16:41:37  steven
# Added $(B_REFSW_MAGNUM_INCLUDE_DIRS) as this is now needed
# for all applications using magnum includes
#
# Revision 1.1  2005/10/26 17:47:48  steven
# Initial Checkin
#
# Revision 1.5  2005/10/12 13:10:00  xmiville
# Cleanup. plus checking if autogen.sh worked.
#
# Revision 1.4  2005/10/11 23:12:22  xmiville
# new fixes again, found the libtool problem.
#
# Revision 1.3  2005/10/11 20:25:26  xmiville
# New fixes
#
# Revision 1.2  2005/09/28 18:55:24  xmiville
# New version with copyright notice fix.
#
# Revision 1.1  2005/09/20 20:28:04  xmiville
# Package released to Broadcom
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

ifndef BCHP_VER
$(error BCHP_VER is not defined)
endif

ifndef BCHP_7411_VER
$(error BCHP_7411_VER is not defined)
endif

ifndef BCM_REFSW_PATH
$(error BCM_REFSW_PATH is not defined)
else
BSEAV := $(BCM_REFSW_PATH)/BSEAV
PLATFORM := $(PLATFORM)
BCHP_VER := $(BCHP_VER)
BCHP_7411_VER := $(BCHP_7411_VER)
# This one performs it's own error checking, BSEAV and PLATFORM must be defined
include $(BSEAV)/api/include/api.mak
include $(BSEAV)/api/build/magnum/Makefile.inc
endif

BDVD_CFLAGS := -I$(BSEAV)/../ndvd/a/inc
# Currently bdvd library is built after directfb, so don't link now...
#BDVD_LDFLAGS := -L$(BSEAV)/../ndvd/a/bin -lbdvd

ifndef DIRECTFB_INSTALL_PATH
$(error DIRECTFB_INSTALL_PATH is not defined)
endif

ifndef BCM_LINUX_PATH
$(error BCM_LINUX_PATH is not defined)
endif

# You will need to add a -DBCMUSESLINUXINPUTS if
# --with-inputdrivers=linuxinput when using the
# bcm core system module.

CROSS_DEFINITIONS := \
	DEBUG="$(DEBUG)" \
	CROSS="$(CROSS)" \
	CROSS_CFLAGS="$(CROSS_CFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include -I${BCM_LINUX_PATH}/include" \
	CROSS_CPPFLAGS="$(CROSS_CPPFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include -I${BCM_LINUX_PATH}/include" \
	CROSS_CXXFLAGS="$(CROSS_CXXFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include -I${BCM_LINUX_PATH}/include" \
	CROSS_LDFLAGS="$(CROSS_LDFLAGS) -L${DIRECTFB_INSTALL_PATH}/usr/local/lib -Wl,-rpath-link,${DIRECTFB_INSTALL_PATH}/usr/local/lib" \
	BCMSETTOPAPI_INTERNAL_CFLAGS="-I${BCM_LINUX_PATH}/include $(CFLAGS) $(BDVD_CFLAGS)" \
	BCMSETTOPAPI_CFLAGS="-I${BCM_LINUX_PATH}/include $(BSETTOP_CFLAGS) $(B_REFSW_MAGNUM_INCLUDE_DIRS) $(BDVD_CFLAGS)" \
	BCMSETTOPAPI_LDFLAGS="$(BSETTOP_LDFLAGS) $(BDVD_LDFLAGS)"
	
configure_mipsel-uclibc:
	$(CROSS_DEFINITIONS) \
	sh configure_DirectFB_mipsel-uclibc

build_mipsel-uclibc: configure_mipsel-uclibc
#1- For some weird reason, won't take LD passed at configure time,
#so let's force it at compile time.
#2- On RH7.3, SED need to be defined for libtool
	@if [ -f "/etc/redhat-release" -a "`cat /etc/redhat-release`" == "Red Hat Linux release 7.3 (Valhalla)" ]; then \
		export SED=sed; \
		make LD=$(CROSS)ld; \
		make LD=$(CROSS)ld; \
		make LD=$(CROSS)ld; \
		make LD=$(CROSS)ld; \
		make LD=$(CROSS)ld; \
		make LD=$(CROSS)ld; \
	fi
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

