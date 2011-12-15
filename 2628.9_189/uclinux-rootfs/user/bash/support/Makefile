# Generated automatically from Makefile.in by configure.
#
# Simple Makefile for the support programs.
#
# documentation support: man2html
# testing support: printenv recho zecho
#
# bashbug lives here but is created by the top-level makefile
#
# Currently only man2html is built
#
#
# Copyright (C) 1998 Free Software Foundation, Inc.     

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA.

#
# Boilerplate
#
topdir = ..
srcdir = .
VPATH = .:.
BUILD_DIR = /home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/user/bash

RM	= rm -f
SHELL	= /bin/sh
CC = mips-linux-gcc

EXEEXT = 

#
# Compiler options:
#
PROFILE_FLAGS = 
CFLAGS = -Os -Dlinux -D__linux__ -Dunix -D__uClinux__ -DEMBED -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/lib/uClibc/include -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/linux-2.6.x/include
LOCAL_CFLAGS = 
CPPFLAGS = 
DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL
LDFLAGS = -Os  -Dlinux -D__linux__ -Dunix -D__uClinux__ -DEMBED -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/lib/uClibc/include -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs -fno-builtin -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/linux-2.6.x/include -Os  -Dlinux -D__linux__ -Dunix -D__uClinux__ -DEMBED -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/lib/uClibc/include -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs -fno-builtin -I/home/jipeng/build-2.6.18/jipeng2.6.18-1.5-prerel-package/src/stblinux-2.6.18-0.1/uclinux-rootfs/linux-2.6.x/include $(LOCAL_LDFLAGS) $(CFLAGS)
LOCAL_LDFLAGS = -rdynamic
LIBS = -ldl 

INCLUDES = -I${BUILD_DIR} -I${topdir}

#
CCFLAGS = ${PROFILE_FLAGS} $(DEFS) $(LOCAL_DEFS) $(SYSTEM_FLAGS) $(CPPFLAGS) \
	  ${INCLUDES} $(LOCAL_CFLAGS) $(CFLAGS)

SRC1 =	man2html.c
OBJ1 =	man2html.o

.c.o:
	$(RM) $@
	$(CC) -c $(CCFLAGS) $<

all: man2html$(EXEEXT)

man2html$(EXEEXT): $(OBJ1)
	$(CC) $(CCFLAGS) $(OBJ1) -o $@ ${LIBS}

clean:
	rm man2html

man2html.o: man2html.c
