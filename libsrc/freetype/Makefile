#!/bin/sh

############################################################################
#      THIS MUST APPEAR AT THE TOP OF THE MAKEFILE.  DO NOT MODIFY.
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include  $(if $(wildcard ../../bdvd_top.mak),../../bdvd_top.mak,../../../ndvd/bdvd_top.mak)
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##



#	make BUILD_DIR=`pwd`/builds/unix PLATFORM="unix" all; make


TARGET             ?= ${BDVD_FREETYPE_VERSION}


TARGET_MARKER  := ${TARGET}/.empty_file
TARGET_PATCH_MARKER:= ${TARGET}/.target_patch_marker
TARGET_PATCH_FILE  := sfobjs.patch

BDVD_STAGE_DIR     := $(shell cd ../..;pawd)
BDVD_STAGE_DIR     := ${BDVD_STAGE_DIR}/stage
CPPFLAGS           += -I${BDVD_STAGE_DIR}/usr/local/include
LDFLAGS            += -L${BDVD_STAGE_DIR}/usr/local/lib

CROSS              := ${BDVD_ARCH}

DIRECTFB_INSTALL_PATH := ${BDVD_STAGE_DIR}

ifeq (y,${DMALLOC})
  DMALLOC_COMMAND = sed -ie '26 s/^/\#undef malloc\n\#undef free\n\#undef alloc\n\#undef realloc\n/' ${TARGET}/src/base/ftsystem.c ; sed -ie '16 s/^/\#undef realloc\n\#undef free\n/' ${TARGET}/src/base/ftutil.c
else 
  DMALLOC_COMMAND = true
endif


export CFLAGS CPPFLAGS LDFLAGS CC CPP CXX LD AR RANLIB STRIP DIRECTFB_INSTALL_PATH


all:   ${TARGET}/Makefile ${TARGET_PATCH_MARKER}
	${MAKE} --directory=${TARGET} all

${TARGET_MARKER}: ${TARGET}.tar.gz
	tar xzvf ${TARGET}.tar.gz
	touch ${TARGET_MARKER}

${TARGET}/Makefile: ${TARGET_MARKER} 

${TARGET_PATCH_MARKER}: ${TARGET_PATCH_FILE}
	-patch -d ${TARGET} -Np1 < ${TARGET_PATCH_FILE}
	cd ${TARGET} && ./configure --prefix=${BDVD_STAGE_DIR}/usr/local --enable-shared --enable-static --build=i386-pc-linux-gnu --host=${BDVD_ARCH}-gnu
	${DMALLOC_COMMAND}
	touch ${TARGET_PATCH_MARKER}

prep0: ${TARGET_MARKER}

prep1: ${TARGET}/Makefile ${TARGET_PATCH_MARKER}

install: ${TARGET}/Makefile ${TARGET_PATCH_MARKER}
	 ${MAKE} --directory=${TARGET} install

quick: all
	@echo SKIPPING MAKE INSTALL OF ${TARGET}!

clean: 
	rm -f ${TARGET_PATCH_MARKER}
	if [ -f ${TARGET}/Makefile ] ; then DESTDIR=${BDVD_STAGE_DIR} ${MAKE} --directory=${TARGET} clean ; fi

distclean:
	if [ -f ${TARGET}/Makefile ] ; then DESTDIR=${BDVD_STAGE_DIR} ${MAKE} --directory=${TARGET} distclean ; fi
	rm -fr ${TARGET}


############################################################################
#                      DO NOT MODIFY ANYTHING BELOW
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include  $(if $(wildcard ../../bdvd_btm.mak),../../bdvd_btm.mak,../../../ndvd/bdvd_btm.mak)
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##

