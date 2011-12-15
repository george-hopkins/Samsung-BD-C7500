###############################################################################
# @par Copyright Information:
#      Copyright (C) 2005,Broadcom. 
#      All Rights Reserved.
#      Confidential Property of Broadcom.
#
# @file		Makefile
#
# @brief	Top makefile for the directfb dependencies.
#
# $Id: build.mk,v 1.5 2006/02/22 18:21:17 mrouthier Exp $
#
# @par Revision History:
# 
# $Log: build.mk,v $
# Revision 1.5  2006/02/22 18:21:17  mrouthier
# *** empty log message ***
#
# Revision 1.4  2006/02/22 16:30:23  xmiville
# Added the choice of a package specific strap script.
#
# Revision 1.3  2006/02/22 15:41:59  mrouthier
# add cairo
#
# Revision 1.2  2006/02/08 17:21:33  xmiville
# Commit of DirectFB changes to 060206-1447 release.
#
# Revision 1.3  2006/01/10 22:31:01  xmiville
# Updated zlib, freetype and libpng, new security and bug fixes in those
# version, these are recommended by the packages respective authors.
#
# Revision 1.2  2006/01/09 18:13:35  xmiville
# Merging work branch from 051128.
#
# Revision 1.2  2005/11/29 16:11:53  steven
# Ported Xav's new DirectFB to ndvd release
#
# Revision 1.1  2005/10/26 17:48:24  steven
# Initial Checkin
#
# Revision 1.3  2005/09/28 18:55:24  xmiville
# New version with copyright notice fix.
#
# Revision 1.2  2005/09/23 20:43:03  xmiville
# Major cleanup of bcmsettopapi.c, more comments, plus adding
# libxml2 and libiconv to dependencies.
#
# Revision 1.1  2005/09/20 20:22:59  xmiville
# Dependencies build.
#
#
###############################################################################

# This will include the toolchain cross-compilation definitions, such as
# prefix and c/cxx/cpp/ld flags. Not being defined is tolerated, in which case
# none of the definitions will exist, and therefore fall back to host toolchain
# definitions. If not tolerated, uncomment the error message instead.
ifndef DIRECTFB_PROJECT_TOP
$(error DIRECTFB_PROJECT_TOP is not defined)
else
include ${DIRECTFB_PROJECT_TOP}/toolchain.inc
endif

ifndef SOURCEDIR
$(error SOURCEDIR is not defined)
endif

ifndef DESTDIR
$(error DESTDIR, the install directory, is not defined)
endif

ifndef BUILDDIR
$(error BUILDDIR, the build directory, is not defined)
endif

ifeq ($(BUILDDIR), $(DESTDIR))
$(error BUILDDIR and DESTDIR must be different directories)
endif

BASE_PACKAGES = zlib-1.2.3 jpeg-6b libpng-1.2.8 freetype-2.3.4 libiconv-1.9.1 libxml2-2.6.16 cairo-20060212

.PHONY: $(BASE_PACKAGES)

all: $(BASE_PACKAGES)

$(BASE_PACKAGES): $(DESTDIR) $(BUILDDIR)
	@echo "Building $@"
	@if [ -f $(SOURCEDIR)/$@/$@.strap ]; then \
	    echo "Using $@.strap"; \
	    ARCH=$(ARCH) BUILD_DIR=$(BUILDDIR) SOURCE_DIR=$(SOURCEDIR)/$@ PATCH_DIR=$(SOURCEDIR)/$@ sh $(SOURCEDIR)/$@/$@.strap $@;\
	else \
	    echo "Using generic.strap"; \
	    ARCH=$(ARCH) BUILD_DIR=$(BUILDDIR) SOURCE_DIR=$(SOURCEDIR)/$@ PATCH_DIR=$(SOURCEDIR)/$@ sh $(SOURCEDIR)/generic.strap $@;\
	fi;
	DIRECTFB_INSTALL_PATH=$(DESTDIR) make -C $(BUILDDIR)/$@_$(ARCH) -f build.mk install_$(ARCH)

$(DESTDIR):
	mkdir $(DESTDIR)

$(BUILDDIR):
	mkdir $(BUILDDIR)

clean:
	@echo "Removing $(BUILDDIR)"
	@rm -Rf $(BUILDDIR)
