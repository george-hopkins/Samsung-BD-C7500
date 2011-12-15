###############################################################################
# @par Copyright Information:
#      Copyright (C) 2005, Broadcom.
#      All Rights Reserved.
#      Confidential Property of Broadcom.
#
# @file		Makefile
#
# @brief	Cross compilation definitions for the toolchain.
#
# $Id: toolchain.inc,v 1.2 2006/02/08 17:22:45 xmiville Exp $
#
# @par Revision History:
# 
# $Log: toolchain.inc,v $
# Revision 1.2  2006/02/08 17:22:45  xmiville
# Commit of DirectFB changes to 060206-1447 release.
#
# Revision 1.2  2005/09/28 18:55:24  xmiville
# New version with copyright notice fix.
#
# Revision 1.1  2005/09/20 20:24:17  xmiville
# Package released to Broadcom
#
#
###############################################################################

ARCH := mipsel-uclibc
CROSS := $(ARCH)-
CROSS_CFLAGS := -mabi=32 -mips32 -march=mips32 -mtune=mips32
CROSS_CPPFLAGS :=
CROSS_CXXFLAGS := $(CROSS_CFLAGS)
CROSS_LDFLAGS :=
CROSS_STANDARD_INCLUDE := -I${CROSS_TOOLCHAIN_PATH}/lib/gcc-lib/$(ARCH)/3.3.3/include

CC := $(CROSS)gcc
CPP := $(CROSS)cpp
CXX := $(CROSS)g++
LD := $(CROSS)ld
AR := $(CROSS)ar
RANLIB := $(CROSS)ranlib
STRIP := $(CROSS)strip

