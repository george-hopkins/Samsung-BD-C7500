############################################################################
#      THIS MUST APPEAR AT THE TOP OF THE MAKEFILE.  DO NOT MODIFY.
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include ../../bdvd_top.mak
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##


# This is against new-build-system rules.
export DIRECTFB_SOURCE := ${BDVD_ROOT_DIR}/libsrc/directfb/DirectFB-0.9.22
export DIRECTFB_PROJECT_TOP := ${BDVD_ROOT_DIR}/directfb
export BDVD_STAGE_DIR
export DIRECTFB_INSTALL_PATH := ${BDVD_STAGE_DIR}
export CROSS_CPPFLAGS := ${CPPFLAGS} 
export CROSS_CXXFLAGS := ${CXXFLAGS}
export CROSS_CFLAGS := ${CFLAGS} -I${BDVD_STAGE_INC_DIR} -I${BDVD_STAGE_INC_DIR}/directfb
export CROSS_LDFLAGS := ${LDFLAGS}
export CROSS := mipsel-uclibc-
export SLEEP_CMD = @true

CROSS_CPPFLAGS += -I${BDVD_STAGE_INC_DIR}


TARGET       := cairo-20060212
TARGET_DIR   := ${TARGET}_${BDVD_ARCH}
PREP0_MARKER := ${TARGET_DIR}/.marker_prep0
PREP1_MARKER := ${TARGET_DIR}/.marker_prep1
PREP2_MARKER := ${TARGET_DIR}/.marker_prep2

quick all: ${PREP2_MARKER}


${PREP2_MARKER}: ${PREP1_MARKER}
	cd ${TARGET_DIR} && ${MAKE} all
	touch $@

install: all
	cd ${TARGET_DIR} && ${MAKE} DESTDIR=$(BDVD_STAGE_DIR) install

clean:
	if [ -f ${TARGET_DIR}/Makefile ] ; then ${MAKE} -C ${TARGET_DIR} clean ; fi
	rm -f ${PREP2_MARKER}

distclean: clean
	if [ -f ${TARGET_DIR}/Makefile ] ; then ${MAKE} -C ${TARGET_DIR} distclean ; fi
	rm -fr ${TARGET_DIR}

${PREP0_MARKER}:
	rm -fr ${TARGET_DIR}
	tar xzvf ${TARGET}.tar.gz
	mv ${TARGET} ${TARGET_DIR}
	-patch -p1 < ${TARGET}.${BDVD_ARCH}.patch  # This patch file doesn't seem to exist.
	cp configure_cairo_${BDVD_ARCH} ${TARGET_DIR}
	cp build.mk ${TARGET_DIR}
	cp automake_output.tgz ${TARGET_DIR}
	cp *.patch ${TARGET_DIR}
	cd ${TARGET_DIR} && patch -N ./src/cairoint.h                cairoint.h.patch
	cd ${TARGET_DIR} && patch -N ./src/cairo-font.c              cairo-font.c.patch
	cd ${TARGET_DIR} && patch -N ./test/cairo-test-directfb.c    cairo-test-directfb.c.patch
	cd ${TARGET_DIR} && patch -N -p1 <                           autoconf_automake.patch
	cd ${TARGET_DIR} && patch -N ./src/cairo-surface.c           cairo-surface.c.patch
	cd ${TARGET_DIR} && patch -N ./src/cairo-clip.c              cairo-clip.c.patch
	@touch $@

${PREP1_MARKER}: ${PREP0_MARKER}
	cd ${TARGET_DIR} && sh configure_cairo_${BDVD_ARCH} 
	@touch $@

prep0: ${PREP0_MARKER}
prep1: ${PREP1_MARKER}
prep2: ${PREP2_MARKER}

.PHONY: prep0 prep1 all install clean distclean


############################################################################
#                      DO NOT MODIFY ANYTHING BELOW
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include ../../bdvd_btm.mak
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##
