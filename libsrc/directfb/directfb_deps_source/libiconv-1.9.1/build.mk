###############################################################################
# @par Copyright Information:
#      Copyright (C) 2005, Broadcom.
#      All Rights Reserved.
#      Confidential Property of Broadcom.
#
# @file		Makefile
#
# @brief	Top makefile for the libiconv project.
#
# $Id: build.mk,v 1.2 2006/02/08 17:21:23 xmiville Exp $
#
# @par Revision History:
# 
# $Log: build.mk,v $
# Revision 1.2  2006/02/08 17:21:23  xmiville
# Commit of DirectFB changes to 060206-1447 release.
#
# Revision 1.2  2006/01/09 18:13:35  xmiville
# Merging work branch from 051128.
#
# Revision 1.3  2006/01/09 15:18:33  xmiville
# Added optimization flags to deps packages.
#
# Revision 1.2  2005/11/29 16:11:53  steven
# Ported Xav's new DirectFB to ndvd release
#
# Revision 1.1  2005/10/26 17:48:24  steven
# Initial Checkin
#
# Revision 1.2  2005/09/28 18:55:24  xmiville
# New version with copyright notice fix.
#
# Revision 1.1  2005/09/23 20:43:03  xmiville
# Major cleanup of bcmsettopapi.c, more comments, plus adding
# libxml2 and libiconv to dependencies.
#
#
###############################################################################

ifndef DIRECTFB_PROJECT_TOP
$(error DIRECTFB_PROJECT_TOP is not defined)
else
include ${DIRECTFB_PROJECT_TOP}/toolchain.inc
endif

ADDITIONAL_OPTS_C := -O2

CROSS_DEFINITIONS := \
	CROSS="$(CROSS)" \
	CROSS_CFLAGS="$(ADDITIONAL_OPTS_C) $(CROSS_CFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include" \
	CROSS_CPPFLAGS="$(CROSS_CPPFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include" \
	CROSS_CXXFLAGS="$(CROSS_CXXFLAGS) -I${DIRECTFB_INSTALL_PATH}/usr/local/include" \
	CROSS_LDFLAGS="$(CROSS_LDFLAGS) -L${DIRECTFB_INSTALL_PATH}/usr/local/lib"
	
configure_mipsel-uclibc:
	$(CROSS_DEFINITIONS) \
	sh configure_libiconv_mipsel-uclibc

build_mipsel-uclibc: configure_mipsel-uclibc
	make

install_mipsel-uclibc: build_mipsel-uclibc
	make DESTDIR=${DIRECTFB_INSTALL_PATH} install

clean:
	make clean

distclean:
	make distclean

clean_mipsel-uclibc: clean

distclean_mipsel-uclibc: distclean

