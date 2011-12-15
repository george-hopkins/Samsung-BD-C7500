############################################################################
#      THIS MUST APPEAR AT THE TOP OF THE MAKEFILE.  DO NOT MODIFY.
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include ../../bdvd_top.mak
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##


####vvvv###################################vvvv####
# LEGACY_VAR_FIREWALL: 
#   We do not want legacy options comming in and 
#   configuring things the old way.
#
PLATFORM:=
BCHP_VER:=
BCHP_REV:=
BCHP_7411_VER:=
BSEAV:=
MAGNUM:=
DEBUG:=
BCM_LINUX_PATH:=
BSETTOP_CFLAGS:=
B_REFSW_MAGNUM_INCLUDE_DIRS:=
BCM_REFSW_PATH:=
DIRECTFB_INSTALL_PATH:=
####^^^^###################################^^^^####



# This is temporary
ifeq (,$(strip ${BDVD_BUILD_OPT_directfb})) 
  # use whatever the other code is using, which is probably debug
else
  ifeq (,$(strip $(filter -O0 -O1, ${BDVD_BUILD_OPT_directfb})))
    BDVD_DEBUG :=
    BDVD_BUILD_DEBUG :=
   else
    BDVD_DEBUG := y
    BDVD_BUILD_DEBUG := y
  endif
endif

CFLAGS    += ${BDVD_BUILD_OPT_directfb}
CXXFLAGS  += ${BDVD_BUILD_OPT_directfb}

ifeq (0,1)
  $(warning From libsrc/directfb/Makefile: CFLAGS are ${CFLAGS})
  $(warning From libsrc/directfb/Makefile: CXXFLAGS are ${CXXFLAGS})
  $(warning From libsrc/directfb/Makefile: CPPFLAGS are ${CPPFLAGS})
  $(warning From libsrc/directfb/Makefile: BDVD_DEBUG is ${BDVD_DEBUG})
  $(warning From libsrc/directfb/Makefile: BDVD_BUILD_OPT_directfb is ${BDVD_BUILD_OPT_directfb})
endif


TARGET                  ?= DirectFB-0.9.22_mipsel-uclibc
TARGET_DIR              := ${TARGET}
BDVD_SETTOP_CPPFLAGS    := $(addprefix -I${BDVD_STAGE_INC_DIR}/, base settop common chip pifc syslib) ${CPPFLAGS} -DBCHP_CHIP=${BDVD_SOC} -DB_SYSTEM_linux
PREP1_MARKER_FILE       := .makefile_marker_prep1
CPPFLAGS                += -DBCHP_CHIP=${BDVD_SOC} -DBCHP_VER=BCHP_VER_${BDVD_SOC_VER} -DBCM_BOARD=${BDVD_PLATFORM}
CFLAGS                  += -DBCHP_VER=BCHP_VER_${BDVD_SOC_VER} -DBCM_BOARD=${BDVD_PLATFORM}

#   B_REFSW_GENERIC_MAGNUM_CFLAGS += -DBCHP_CHIP=GENERIC -DBCHP_VER=BCHP_VER_A0
#   CFLAGS                   += -DBDBG_DEBUG_BUILD=1 -DBRCM_DBG
#   B_REFSW_OS = linuxuser


DFB_CROSS_DEFINITIONS := \
	DIRECTFB_INSTALL_PATH="${BDVD_STAGE_DIR}" \
	DEBUG="${BDVD_DEBUG}" \
	CROSS="${BDVD_ARCH}-" \
	CROSS_CFLAGS="     ${CFLAGS}   -I${BDVD_STAGE_INC_DIR} -I${BDVD_LINUX_DIR}/include" \
	CROSS_CPPFLAGS="   ${CPPFLAGS} -I${BDVD_STAGE_INC_DIR} -I${BDVD_LINUX_DIR}/include" \
	CROSS_CXXFLAGS="   ${CXXFLAGS} -I${BDVD_STAGE_INC_DIR} -I${BDVD_LINUX_DIR}/include" \
	CROSS_LDFLAGS="    ${LDFLAGS}  -L${BDVD_STAGE_LIB_DIR} -Wl,-rpath-link,-L${BDVD_STAGE_LIB_DIR} $(if $(filter y,${DMALLOC}),-ldmalloc)"\
	BCMSETTOPAPI_CFLAGS="            -I${BDVD_LINUX_DIR}/include ${BDVD_SETTOP_CPPFLAGS} " \
	BCMSETTOPAPI_INTERNAL_CFLAGS="   -I${BDVD_LINUX_DIR}/include ${BDVD_SETTOP_CPPFLAGS} ${CFLAGS}" \
    BCMSETTOPAPI_LDFLAGS="-L${BDVD_STAGE_LIB_DIR} -lsettop -lm -lpthread "\
	BCHP_7411_VER="${BDVD_7411_VER}" BCHP_VER="BCHP_VER_${BDVD_SOC_VER}" BCM_BOARD="${BDVD_PLATFORM}" \
    BCHP_CHIP=${BDVD_SOC}
    # End of DFB_CROSS_DEFINITIONS assignment ----^^^^^----^^^^^----^^^^^----^^^^^----####


all: bdvd_include_files_hack ${TARGET_DIR}/Makefile
#1- For some weird reason, won't take LD passed at configure time,
#so let's force it at compile time.
#2- On RH7.3, SED need to be defined for libtool
	SED=sed LD=${LD} PLATFORM=${BDVD_PLATFORM} CHIP_REVISION=${BDVD_SOC_VER} DIRECTFB_INSTALL_PATH=${BDVD_STAGE_DIR} DEBUG=${BDVD_DEBUG}  CPPFLAGS="${CPPFLAGS}"  CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" \
		${MAKE} ${BDVD_MAKE_J_ARG} -C  ${TARGET}


# This is a violation of the new build system put put here temporarily to allow code to compile.
# In the future (hopefully), dfb will not have any dependencies on the bdvd library and API.
bdvd_include_files_hack:
	${MAKE} -C ../bdvd install_incs


${TARGET_DIR}/Makefile:
	cd ${TARGET_DIR} && ${DFB_CROSS_DEFINITIONS} \
        sh configure_DirectFB_mipsel-uclibc


prep0:
	@echo All good - nothing to be uncompressed.

prep1: ${TARGET_DIR}/Makefile


install: all
#1- For some weird reason, won't take LD passed at configure time,
#so let's force it at compile time.
#2- On RH7.3, SED need to be defined for libtool
	${MAKE} -C ${TARGET} SED="sed" LD="${LD}" PLATFORM="${BDVD_PLATFORM}" CHIP_REVISION="${BDVD_SOC_VER}" DEBUG="${BDVD_DEBUG}" DESTDIR="${BDVD_STAGE_DIR}" install 
# HiDef-DVD.PR5653 added this line:
	install -m 664 ${TARGET}/interfaces/IDirectFBVideoProvider/idirectfbvideoprovider_bcm.o ${BDVD_STAGE_LIB_DIR}/${BDVD_DFB_VERSION}${BDVD_BUILD_OPT_SUFFIX_directfb}/interfaces/IDirectFBVideoProvider

quick: all
	@echo SKIPPING MAKE INSTALL OF ${TARGET}!

clean:
	-cd ${TARGET_DIR} && if [ -f Makefile ] ; then ${MAKE} clean ; fi


distclean: clean
	-cd ${TARGET_DIR} && if [ -f Makefile ] ; then ${MAKE} distclean ; fi
	-cd ${TARGET_DIR} && rm -f aclocal.m4 config.h config.h.in config.log config.status configure configure.BEFORE directfb-internal.pc directfb.pc directfb.spec libtool directfb-config Makefile Makefile.in stamp-h1 stamp-h.in config.guess config.sub COPYING INSTALL
	-cd ${TARGET_DIR} && rm -fr autom4te*.cache .marker_* configure.newversion*
	-cd ${TARGET_DIR} && find . -name "Makefile.in" | xargs rm -f 
	-cd ${BDVD_STAGE_LIB_DIR}/${BDVD_DFB_VERSION}${BDVD_BUILD_OPT_SUFFIX_directfb} && rm -fr *
	-cd ${BDVD_STAGE_INC_DIR}/directfb && rm -fr *
	-cd ${BDVD_STAGE_INC_DIR}/directfb-internal && rm -fr *

.PHONY: prep0 prep1 all install clean distclean


###################################################################################
## 
## THIS IS THE SEQUENCE FOR BUILDING DIRECTFB ON THE OLD BUILD SYSTEM. 
## 
## FILE: ./build-all.sh
##           #!/bin/sh
##           ...
##           make -C directfb  -f build.mk
##           ...
## 
## 
## FILE: ./directfb/build.mk
##           ...
##           build_directfb:
## 	          PATH=$(PATH)  PLATFORM=$(PLATFORM) CHIP_REVISION=$(CHIP_REVISION) DEBUG=$(DEBUG) \
##                 ${MAKE} -C DirectFB-0.9.22_mipsel-uclibc  -f build.mk  install_mipsel-uclibc 
##           ...
## 
## 
## FILE: ./directfb/DirectFB-0.9.22_mipsel-uclibc/build.mk
##           ...
##           configure_mipsel-uclibc:
##               ${CROSS_DEFS} \
##                 sh configure_DirectFB_mipsel-uclibc
##           ...
##           build_mipsel-uclibc: configure_mipsel-uclibc
##               SED=sed LD=$(CROSS)ld \
##                 ${MAKE}
##           ...
##           install_mipsel-uclibc: build_mipsel-uclibc
##               SED=sed LD=$(CROSS)ld DESTDIR=${DIRECTFB_INSTALL_PATH} \
##                 ${MAKE} install
## 
## 
## FILE: ./directfb/DirectFB-0.9.22_mipsel-uclibc/configure_DirectFB_mipsel-uclibc
##           ${CROSS_DEFS} \
##               sh configure \
##                      --enable_bcm \
##                      --build=i686-pc-linux-gnu \
##                      --host=mipsel-linux-gnu \
##                      $CONFIGURE_DEBUG_OPTS \
##                      --disable-static \
##                      --enable-shared \
##                      ...
##           
###################################################################################






############################################################################
#                      DO NOT MODIFY ANYTHING BELOW
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include ../../bdvd_btm.mak
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##

