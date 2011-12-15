###############################################################################
# @par Copyright Information:
#      Copyright (C) 2005, Broadcom.
#      All Rights Reserved.
#      Confidential Property of Broadcom 
#
# @file		Makefile
#
# @brief	Top makefile for the cairo project.
#
# $Id: build.mk,v 1.7 2006/02/22 19:26:18 mrouthier Exp $
#
# @par Revision History:
# 
# $Log: build.mk,v $
# Revision 1.7  2006/02/22 19:26:18  mrouthier
# *** empty log message ***
#
# Revision 1.6  2006/02/22 17:46:42  mrouthier
# *** empty log message ***
#
# Revision 1.5  2006/02/22 16:51:07  mrouthier
# *** empty log message ***
#
# Revision 1.4  2006/02/22 16:32:11  mrouthier
# use strap now
#
# Revision 1.3  2006/02/22 15:41:34  mrouthier
# *** empty log message ***
#
# Revision 1.2  2006/02/20 19:46:54  mrouthier
# *** empty log message ***
#
# Revision 1.1  2006/02/20 19:32:40  mrouthier
# *** empty log message ***
#
#
###############################################################################

ifndef DIRECTFB_PROJECT_TOP
$(error DIRECTFB_PROJECT_TOP is not defined)
else
include ${DIRECTFB_PROJECT_TOP}/toolchain.inc
endif

SLEEP_CMD ?= sleep 5


ADDITIONAL_OPTS_C := -O2

CROSS_DEFINITIONS := \
	DIRECTFB_INSTALL_PATH="$(DIRECTFB_INSTALL_PATH)" \
	DIRECTFB_SOURCE="$(DIRECTFB_SOURCE)" \
	CROSS="$(CROSS)" \
	CROSS_CFLAGS="$(ADDITIONAL_OPTS_C) $(CROSS_CFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include" \
	CROSS_CPPFLAGS="$(CROSS_CPPFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include" \
	CROSS_CXXFLAGS="$(CROSS_CXXFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include" \
	CROSS_LDFLAGS="$(CROSS_LDFLAGS) -L${DIRECTFB_INSTALL_PATH}/usr/local/lib"

configure_mipsel-uclibc:
	echo making configure_mipsel-uclibc...
	${SLEEP_CMD}
	$(CROSS_DEFINITIONS) \
	sh configure_cairo_mipsel-uclibc

build_mipsel-uclibc: configure_mipsel-uclibc
	echo making build_mipsel-uclibc...
	${SLEEP_CMD}
	make all

install_mipsel-uclibc: build_mipsel-uclibc
	echo making install_mipsel-uclibc...
	${SLEEP_CMD}
	make DESTDIR=$(DIRECTFB_INSTALL_PATH) install

clean:
	make clean

distclean:
	make distclean

clean_mipsel-uclibc: clean

distclean_mipsel-uclibc: distclean
