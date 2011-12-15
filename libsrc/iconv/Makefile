############################################################################
#      THIS MUST APPEAR AT THE TOP OF THE MAKEFILE.  DO NOT MODIFY.
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include  $(if $(wildcard ../../bdvd_top.mak),../../bdvd_top.mak,../../../ndvd/bdvd_top.mak)
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##



TARGET             ?= libiconv-1.9.1
TARGET_DIR         := ${TARGET}
TARGET_DIR_MARKER  := ${TARGET_DIR}/.empty_file
TARGET_PATCH_FILE  := ${TARGET}.${BDVD_ARCH}.patch

BDVD_STAGE_DIR     := $(shell cd ../..;pawd)
BDVD_STAGE_DIR     := ${BDVD_STAGE_DIR}/stage
CPPFLAGS           += -I${BDVD_STAGE_DIR}/usr/local/include
LDFLAGS            += -L${BDVD_STAGE_DIR}/usr/local/lib 
CROSS              := ${BDVD_ARCH}
DIRECTFB_INSTALL_PATH := ${BDVD_STAGE_DIR}

ifeq (y,${DMALLOC})
  DMALLOC_COMMAND = sed -ie 's/^CFLAGS[ \t]*=/CFLAGS = $$(if $$(filter xmalloc xstrdup memmove,$$(\*F)),-UDMALLOC,-DBDVD_INCLUDE_EARLY_XMALLOC) /' ${TARGET_DIR}/srclib/Makefile
  LDFLAGS            += -ldmalloc
else 
  DMALLOC_COMMAND = true
endif

export CFLAGS CPPFLAGS LDFLAGS CC CPP CXX LD AR RANLIB STRIP DIRECTFB_INSTALL_PATH

all:   ${TARGET_DIR}/Makefile
	${MAKE} -C ${TARGET_DIR}


${TARGET_DIR_MARKER}: ${TARGET}.tar.gz
	tar xzvf ${TARGET}.tar.gz
	patch -N ${TARGET_DIR}/libcharset/lib/localcharset.c	localcharset.c.patch
	patch -N ${TARGET_DIR}/libcharset/lib/relocatable.c	relocatable.c.patch
	patch -N ${TARGET_DIR}/lib/relocatable.c		lib-relocatable.c.patch
	patch -N ${TARGET_DIR}/configure		configure.patch
	touch ${TARGET_DIR_MARKER}

${TARGET_DIR}/Makefile: ${TARGET_DIR_MARKER} 
	cd ${TARGET_DIR} && ./configure --enable-shared --enable-static --prefix=${BDVD_STAGE_DIR}/usr/local --build=i686-pc-linux-gnu --host=${BDVD_ARCH}-gnu
	${DMALLOC_COMMAND} 

prep0: ${TARGET_DIR_MARKER}

prep1: ${TARGET_DIR}/Makefile

install: ${TARGET_DIR}/Makefile
	${MAKE} -C ${TARGET_DIR} install


quick: ${TARGET_DIR}/Makefile
	${MAKE} -q -C ${TARGET_DIR}/lib all && ${MAKE} -q -C ${TARGET_DIR}/srclib all && ${MAKE} -q -C ${TARGET_DIR}/src iconv_no_i18n iconv.o  && ${MAKE} -q -C ${TARGET_DIR}/po all

clean: 
	if [ -f ${TARGET_DIR}/Makefile ] ; then DESTDIR=${BDVD_STAGE_DIR} ${MAKE} -C ${TARGET_DIR} clean ; fi

distclean:
	if [ -f ${TARGET_DIR}/Makefile ] ; then DESTDIR=${BDVD_STAGE_DIR} ${MAKE} -C ${TARGET_DIR} distclean ; fi
	rm -fr ${TARGET_DIR} ${TARGET}


.PHONY: all install quick clean distclean prep0 prep1

############################################################################
#                      DO NOT MODIFY ANYTHING BELOW
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include  $(if $(wildcard ../../bdvd_btm.mak),../../bdvd_btm.mak,../../../ndvd/bdvd_btm.mak)
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##


